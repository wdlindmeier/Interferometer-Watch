//
//  NDSDataStreamer.cpp
//  NDS2Connection
//
//  Created by William Lindmeier on 7/23/14.
//
//

#include "NDSDataStreamer.h"
#include "cinder/Cinder.h"

using namespace cinder;
using namespace std;
using namespace ligo;

#pragma mark - Constructors

NDSDataStreamer::NDSDataStreamer( const std::string & name ) :
mIsConnected(false)
,mTimeStride( 20 ) // 20 seconds of data at a time
,mRequestTimeout( mTimeStride * 0.8 ) // The requests should all complete within the stride
,mTimeLoadedDataStarts(-1)
,mTimeDisplayedDataEnds(-1)
,mTimeDisplayedDataStarts(-1)
,mTimeDelay(120) // Only ask for data that's at least N seconds old
,mIsLoadingData(false)
,mName(name)
,mTimeRequestBegan(-1)
{
    int rc = daq_startup();
    if (rc)
    {
        cout << "ERROR: NDS daq failed to initialize\n";
        assert(false);
    }
#if DEBUG
    else
    {
        printf("NDS Initialized\n");
    }
#endif
}

#pragma mark - Attributes

const std::string NDSDataStreamer::getName()
{
    return mName;
}

#pragma mark - Data Listeners

void NDSDataStreamer::addDataListener(NDSDataListener * dataListener)
{
    if ( find(mDataListeners.begin(), mDataListeners.end(), dataListener) == mDataListeners.end() )
    {
        mDataListeners.push_back(dataListener);
    }
}

void NDSDataStreamer::removeDataListener(NDSDataListener * dataListener)
{
    auto position = find(mDataListeners.begin(), mDataListeners.end(), dataListener);
    if ( position != mDataListeners.end() )
    {
        mDataListeners.erase(position);
    }
}

#pragma mark - Connection

bool NDSDataStreamer::isConnected()
{
    return mIsConnected;
}

bool NDSDataStreamer::connect( const std::string & hostname,
                               int port,
                               nds_version ndsVersion )
{
    disconnect();
    
    mHostname = hostname;
    mPort = port;
        
    int rc = daq_connect(&mDaqConnection, mHostname.c_str(), mPort, ndsVersion);
    if (rc)
    {
        printf("Connection failed with error: %s (%i)\n", daq_strerror(rc), rc);
        return false;
    }
    
    mHasLoadedFirstData = false;
    mIsConnected = true;
#if DEBUG
    printf("Connected to %s\n", mHostname.c_str());
#endif
    return true;
}

void NDSDataStreamer::disconnect( )
{
    // Set disconnected right away so other threads don't assume we're ready to load when the
    // thread wraps up.
    bool wasConnected = mIsConnected;
    mIsConnected = false;
    mIsLoadingData = false;
    
    if ( mDataThread.joinable() )
    {
        mDataThread.join();
    }
        
    if ( wasConnected )
    {
        daq_disconnect(&mDaqConnection);
#if DEBUG
        printf("Disconnected\n");
#endif
    }
}

#pragma mark - Channels

void NDSDataStreamer::setRequestedChannels(std::vector<std::string> & channelNames)
{
    mRequestedChannels = channelNames;
}

const std::vector<std::string> NDSDataStreamer::getRequestedChannels()
{
    return mRequestedChannels;
}

#pragma mark - Requests

void NDSDataStreamer::requestChannels( const vector<string> & channels )
{
    int numRequestedChannels = channels.size();
    int numSuccessfulRequests = 0;
    
    for (int i = 0; i < numRequestedChannels; i++ )
    {
        if ( !mIsConnected )
        {
            return;
        }
        
        const char *channelName = channels[i].c_str();
        int rc = daq_request_channel( &mDaqConnection, channelName, cUnknown, 0 );
        if ( rc )
        {
            printf("Request channel failed with error: %s\n", daq_strerror(rc));
        }
        else
        {
            numSuccessfulRequests++;
        }
    }
#if DEBUG
    cout << "~~~ " << mName.c_str()
            << " Successfully requested "
            << numSuccessfulRequests
            << " channels. "
            << (numRequestedChannels - numSuccessfulRequests) << " unavailable.\n";
#endif
}

void NDSDataStreamer::requestData( const time_t startTime, const time_t endTime, const time_t stride )
{
#if DEBUG
    printf("~~ %s Requesting data between %lu and %lu\n", mName.c_str(), startTime, endTime);
#endif
    mLastWindowData.clear();
    mLastWindowRanges.clear();
    
    for ( const string & channelName : mRequestedChannels )
    {
        ChannelDataFrameRef df(new ChannelDataFrame());
        df->startTime = startTime;
        df->endTime = endTime;
        df->channelName = channelName;
        mLastWindowData[channelName] = df;
        mLastWindowRanges[channelName] = DataRange();
    }

    int numTotalChannels = mRequestedChannels.size();
    // NOTE: 5 seems to be about the max
    static const int kChannelBatchSize = 5;
    for ( int i = 0; i < numTotalChannels; i+=kChannelBatchSize )
    {
        if ( !mIsConnected )
        {
            break;
        }
        
        int firstIndex = i;
        int lastIndex = min(i + kChannelBatchSize, numTotalChannels);
        vector<string> channels(mRequestedChannels.begin() + firstIndex,
                                mRequestedChannels.begin() + lastIndex);
        assert(channels.size() > 0);

        // FIRST: Clear the previously requested channels
        daq_clear_channel_list( &mDaqConnection );
        
        // SECOND: Request the new bacth
        requestChannels( channels );
        
        // THIRD: Request the data for the specified channels
        int rc = daq_request_data(&mDaqConnection, startTime, endTime, stride);
        if ( rc )
        {
            printf("ERROR: Data request for channels %i-%i failed with error: %s (code: %i)\n",
                   firstIndex,
                   lastIndex,
                   daq_strerror(rc),
                   rc);
        }
        else
        {
            // FOURTH: Parse / handle the channel data
            parseRequestedData( startTime, endTime, channels );
        }
    }
#if DEBUG
    printf("~~~~x THREAD COMPLETE\n");
#endif
    mHasLoadedFirstData = true;
    mIsLoadingData = false;
}

void NDSDataStreamer::loadDataForTimeRange(const time_t startTime, const time_t endTime)
{
    if ( !mIsConnected )
    {
        cout << mName << " is not connected. Will not load.\n";
        return;
    }
    assert( !mIsLoadingData );
    assert( startTime >= mTimeLoadedDataStarts );
 
    // NOTE: mIsLoadingData is set to false at the end of the thread
    mIsLoadingData = true;
    
    mTimeLoadedDataStarts = startTime;
    
    mTimeRequestBegan = getTimeNow();
#if DEBUG
    printf("~ %s LOADING data for %lu\n", mName.c_str(), mTimeLoadedDataStarts);
#endif
    if ( mDataThread.joinable() )
    {
        // Wait until the previous thread is done.
        mDataThread.join();
    }
    
    mDataThread = std::thread(&NDSDataStreamer::requestData,
                              this,
                              startTime,
                              endTime,
                              mTimeStride);
}

#pragma mark - Time / Tick

double NDSDataStreamer::worldTimeToDataTime( double aGPSTime )
{
    return aGPSTime - mTimeDelay - mTimeStride;
}

void NDSDataStreamer::getDataTimestamps(time_t * dataStartTime,
                                        time_t * dataEndTime,
                                        double * dataPlayheadTime )
{
    *dataStartTime = mTimeDisplayedDataStarts;
    *dataEndTime = mTimeDisplayedDataEnds;
    
    double unixTime = getTimeNow();
    double gpsTime = unix2gps<double>( unixTime );
    *dataPlayheadTime = worldTimeToDataTime( gpsTime );
}

void NDSDataStreamer::tick()
{
    if ( mIsConnected && mRequestedChannels.size() > 0 )
    {
        if ( !mIsLoadingData )
        {
            double currentTime = unix2gps<double>( time(NULL) );
            double dataStartTime = worldTimeToDataTime( currentTime );

            if ( !mHasLoadedFirstData )
            {
                // Load it for the first time
                time_t firstEndTime = dataStartTime + mTimeStride;
                
                loadDataForTimeRange(dataStartTime, firstEndTime);
                
                return;
            }
            else if ( dataStartTime >= mTimeLoadedDataStarts ) // is loading means loading the data associated with the timestamp
            {
                // The current time falls within the loaded data time range.
                // RELEASE data for the current timestamp.
#if DEBUG
                printf("%s RELEASING previous data for %lu\n", mName.c_str(), mTimeLoadedDataStarts);
#endif
                mTimeDisplayedDataStarts = mTimeLoadedDataStarts;
                mTimeDisplayedDataEnds = mTimeDisplayedDataStarts + mTimeStride;
                
                for ( NDSDataListener * listener : mDataListeners )
                {
                    listener->ndsDataDidReceiveNewChannelData(mLastWindowData, this);
                }
                
                // LOAD up the next batch as soon as we can.
                // This gives us ample time to do any processing on it before it's released.
                // time_t nextStartTime = dataStartTime + mTimeStride;            
                time_t nextStartTime = mTimeLoadedDataStarts + mTimeStride;
                time_t nextEndTime = nextStartTime + mTimeStride;
                
                loadDataForTimeRange(nextStartTime, nextEndTime);

                return;
            }
        }
        else // It's loading. Check if we've exceeded the timeout.
        {
            // Check to see how long we've been working.
            // If it exceeds the timeout, kill the thread.
            double timeNow = getTimeNow();
            float timeRunning = timeNow - mTimeRequestBegan;
            if ( timeRunning > mRequestTimeout )
            {
                // NOTE: THIS DOESN'T DO ANYTHING YET
                timeout();
            }
        }
        // Else, just let 'er ride until the next timestamp
    }
}

// This should be a FAST connection killer.
// Called from tick.
// Is there ANY beter way of doing this?
void NDSDataStreamer::timeout()
{
    // TODO
    // printf("TODO: Timeout the request in %s\n", mName.c_str());
}

#pragma mark - Parsing

template <typename T, typename Func>
void handleBufferData(chan_req_t* stat,
                      void * buffer,
                      Func func)
{
    T *tBuffer = (T *)buffer;
    int numReceived = stat->status / sizeof(T);
    // Offset doesn't matter here.
    // We get the approp offset from daq_get_channel_status
    for ( int i = 0; i < numReceived; ++i )
    {
        int index = i;
        func(i, tBuffer[index]);
    }
}

DataRange & pushFrameValue(ChannelDataFrameRef frame, double value, DataRange & existingRange)
{
    frame->data.push_back(value);
    
    if ( value < existingRange.min || !existingRange.wasSet )
    {
        existingRange.min = value;
    }
    if ( value > existingRange.max || !existingRange.wasSet )
    {
        existingRange.max = value;
    }
    
    existingRange.wasSet = true;
    
    return existingRange;
}

void NDSDataStreamer::parseRequestedData( const time_t startTime,
                                          const time_t endTime,
                                          const vector<string> & channelNames )
{
    int numChannels = channelNames.size();
    
    for ( time_t t = startTime; t < endTime ; t += mTimeStride )
    {
        int rc = daq_recv_next( &mDaqConnection);
        
        if (rc)
        {
            printf("Receive data failed with error: %s (code: %i)\n", daq_strerror(rc), rc);
            printf("No more data? Bailing.\n");
            // This may just be that we've run out of data
            break;
        }
        
        for ( int i = 0; i < numChannels; ++i )
        {
            string channelName = channelNames[i];
            const char *cChannelName = channelName.c_str();
            chan_req_t* stat = daq_get_channel_status( &mDaqConnection, cChannelName );

            if ( !stat || stat->status <= 0 ) continue;

            DataRange & range = mLastWindowRanges[channelName];
            
            ChannelDataFrameRef channelDataFrame = mLastWindowData[channelName];
            channelDataFrame->sampleRate = stat->rate;
#if DEBUG
            // This is suspicious
            assert( channelDataFrame->sampleRate < 65000 );
#endif

            void *buffer = malloc( stat->status );
            if ( !daq_get_channel_data( &mDaqConnection, cChannelName, (char *)buffer) )
            {
                printf("Error getting channel %s data\n", cChannelName);

                free(buffer);
                buffer = NULL;
                
                continue;
            }
            else
            {
#if DEBUG
                printf( "Success getting channel data: %s\n", cChannelName );
#endif
                switch (stat->data_type)
                {
                    case _undefined:
                        printf("Type is undefined. Unclear how to proceed.\n");
                        break;
                    case _16bit_integer:
                        handleBufferData<short>(stat, buffer, [&](int index, short value)
                                                {
                                                    range = pushFrameValue(channelDataFrame, value, range);
                                                });
                        break;
                    case _32bit_integer:
                        handleBufferData<int>(stat, buffer, [&](int index, int value)
                                              {
                                                  range = pushFrameValue(channelDataFrame, value, range);
                                              });
                        break;
                    case _64bit_integer:
                        handleBufferData<long>(stat, buffer, [&](int index, long value)
                                               {
                                                   range = pushFrameValue(channelDataFrame, value, range);
                                               });
                        break;
                    case _32bit_float:
                        handleBufferData<float>(stat, buffer, [&](int index, float value)
                                                {
                                                    range = pushFrameValue(channelDataFrame, value, range);
                                                });
                        break;
                    case _64bit_double:
                        handleBufferData<double>(stat, buffer, [&](int index, double value)
                                                 {
                                                     range = pushFrameValue(channelDataFrame, value, range);
                                                 });
                        break;
                    case _32bit_complex:
                        printf("Type is _32bit_complex. Unclear how to proceed.\n");
                        break;
                }
            }
            
            mLastWindowRanges[channelName] = range;
            channelDataFrame->minValue = range.min;
            channelDataFrame->maxValue = range.max;
            
            free(buffer);
            buffer = NULL;
        }
    }
}
