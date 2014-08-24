//
//  LiveDataConnection.cpp
//  LIGOclock
//
//  Created by William Lindmeier on 7/25/14.
//
//

#include "LiveDataConnection.h"
#include "ChannelStateManager.h"
#include "AppConfig.h"

using namespace ligo;
using namespace std;
using namespace cinder;

LiveDataConnection::LiveDataConnection() :
DataConnection()
{
    loadStreamers();
}

void LiveDataConnection::loadStreamers()
{
    // Create the streamers
    vector<std::string> channelNames(mChannelNames.begin(), mChannelNames.end());
    int numChannels = channelNames.size();
    const static int kNumStreams = 1;//8; // Approx the number of CPU cores
    int channelsPerStream = ceil(float(numChannels) / kNumStreams);
#if DEBUG
    printf("Loading %i channels. %i channels per stream\n", numChannels, channelsPerStream);
#endif
    
    // Create a NDS connection which will each handle 1/kNumStreams of the requests.
    for ( int i = 0; i < kNumStreams; ++i )
    {
        NDSDataStreamerRef dataStreamer( new NDSDataStreamer( "Data Streamer " + std::to_string(i) ) );
        dataStreamer->addDataListener( static_cast<NDSDataListener *>(this) );
        dataStreamer->setServerDataTimeDelay( Config::LiveDataTimeDelay() );
        
        int firstChannelIndex = i * channelsPerStream;
        if ( firstChannelIndex < numChannels )
        {
            int lastChannelIndex = min(firstChannelIndex + channelsPerStream, numChannels);
            vector<string> channels(channelNames.begin() + firstChannelIndex,
                                    channelNames.begin() + lastChannelIndex);
            dataStreamer->setRequestedChannels(channels);
        }

        mDataStreamers.push_back(dataStreamer);
    }
}

void LiveDataConnection::closeConnection()
{
    for ( NDSDataStreamerRef dataStreamer : mDataStreamers )
    {
        dataStreamer->disconnect();
    }
    DataConnection::closeConnection();
}

void LiveDataConnection::openConnectionToNDServer( const std::string & hostname, int port )
{
    printf("Connecting to %s : %i\n", hostname.c_str(), port);
    DataConnection::openConnection();
    int ndsVersion = Config::NDSVersion();
    nds_version ndsv;
    switch ( ndsVersion )
    {
        case 1:
            ndsv = nds_v1;
            break;
        case 2:
            ndsv = nds_v2;
            break;
        default:
            cout << "ERROR: Unknown NDS Version: " << ndsVersion << ". Bailing.\n";
            break;
    }
    
    for ( NDSDataStreamerRef dataStreamer : mDataStreamers )
    {
        dataStreamer->connect(hostname, port, ndsv);
    }
    startWorkerThread();
}

double LiveDataConnection::worldTimeToGPSDataTime( double unixTime )
{
    if ( mDataStreamers.size() > 0 )
    {
        double gpsTime = unix2gps<double>( unixTime );
        NDSDataStreamerRef firstStreamer = mDataStreamers[0];
        double requestTime = firstStreamer->worldTimeToDataTime( gpsTime );
        // We have to subtract the amount of time we think it's gonna take to
        // get from the request to the front end, which is the stride.
        return requestTime - firstStreamer->getTimeStride();
    }
    return -1;
};


void LiveDataConnection::work()
{
    for ( NDSDataStreamerRef & streamer : mDataStreamers )
    {
        streamer->tick();
    }
    // What's a good sleep time here?
    sleep(0.1);
}

#pragma mark - NDSDataListener

void LiveDataConnection::ndsDataDidReceiveNewChannelData( std::map<std::string, ChannelDataFrameRef> & dataFrames,
                                                          NDSDataStreamer *streamer )
{
#if DEBUG
    cout << "App received new data from " << streamer->getName() << "\n";
#endif
    std::lock_guard<std::mutex> lock(mDataMutex);
    
    time_t dataStartTime;
    time_t dataEndTime;
    double dataPlayheadTime;
    streamer->getDataTimestamps(&dataStartTime,
                                &dataEndTime,
                                &dataPlayheadTime);

    // Always add it to the approp batch
    if ( mTimedDataBatches.count(dataStartTime) == 0 )
    {
        // Create it the first time.
        mTimedDataBatches[dataStartTime] = ChannelDataBatch();
    }
    ChannelDataBatch & batch = mTimedDataBatches[dataStartTime];
    for ( auto kvp : dataFrames )
    {
        string channelName = kvp.first;
        ChannelDataFrameRef frame = kvp.second;
        batch[kvp.first] = kvp.second;
    }

    if ( batch.size() >= mChannelNames.size() )
    {
        // The batch is full. Release it.
        releaseDataBatch( batch, dataStartTime, dataEndTime );
        assert( mTimedDataBatches.count(dataStartTime) == 0 );
    }
#if DEBUG
    else
    {
        cout << "Received " << batch.size() << "/" << mChannelNames.size() << " channels for " << dataStartTime << "\n";
    }
#endif
}

void LiveDataConnection::releaseDataBatch(ChannelDataBatch & batch, time_t startTime, time_t endTime )
{
#if DEBUG
    cout << "LIVE CONNECTION Publishing data from " << startTime << "\n";
#endif
    assert( batch.size() >= mChannelNames.size() );
    
    publishData(batch, startTime, endTime);
    mTimedDataBatches.erase(startTime);
}
