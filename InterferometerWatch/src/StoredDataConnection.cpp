//
//  StoredDataConnection.cpp
//  LIGOclock
//
//  Created by William Lindmeier on 7/25/14.
//
//

#include "StoredDataConnection.h"
#include "GPSTime.hpp"
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include "FrameL.h"
#include <regex>

using namespace ligo;
using namespace ci;
using namespace std;

StoredDataConnection::StoredDataConnection() :
DataConnection()
,mDatasetGPSStartTime(-1)
,mDatasetGPSEndTime(-1)
,mFileTimeInterval(-1)
,mNextPacketGPSStartTime(-1)
,mNextPacketGPSEndTime(-1)
{}

#pragma mark - Connection

void StoredDataConnection::openConnectionWithDataPath( const ci::fs::path & frameFilePath )
{
    DataConnection::openConnection();
    mDataDirectory = frameFilePath;
    loadDataPaths();
    mTimeDataStarted = ligo::getTimeNow();
    startWorkerThread();
}

void StoredDataConnection::closeConnection()
{
    DataConnection::closeConnection();
}

#pragma mark - Accessors

long StoredDataConnection::getDataStartTime()
{
    return mDatasetGPSStartTime;
}

long StoredDataConnection::getDataEndTime()
{
    return mDatasetGPSEndTime;
}

#pragma mark - Loading Data

// IMPORATANT:
// This assumes the filenames are named like:
// "C-R-1087299008-16.gwf"
// with a timestamp and time interval.
// This should be updated in the future to allow for flexibility.
void StoredDataConnection::loadDataPaths()
{
    mDataPaths.clear();
    
    int fileTimeInterval = -1;
    mFileTimestamps.clear();
    
#if DEBUG
    cout << "Loading data from " << mDataDirectory << "\n";
#endif
    fs::directory_iterator iter(mDataDirectory), end;
    
    for(;iter != end; ++iter)
    {
        if (iter->path().extension() == ".gwf")
        {
            string filename = iter->path().filename().string();
            std::regex base_regex( ".*\\D(\\d+)\\-(\\d+)\\.gwf" );
            std::smatch base_match;
            if (std::regex_match(filename, base_match, base_regex))
            {
                assert(base_match.size() >= 3); // First match is entire string, next N are the parentheses
                
                std::ssub_match timestampMatch = base_match[1];
                std::string timestampString = timestampMatch.str();
                mFileTimestamps.push_back(stol(timestampString));
                
                std::ssub_match intervalMatch = base_match[2];
                std::string intervalString = intervalMatch.str();
                int interval = stoi(intervalString);
                if ( fileTimeInterval == -1 )
                {
                    fileTimeInterval = interval;
                }
                mDataPaths.push_back(filename);
#if DEBUG
                cout << "File " << filename << " timestamp " << timestampString
                     << " interval " << fileTimeInterval << "\n";
#endif
            }
            else
            {
                cout << "ERROR: Cant parse file name: " << filename << "\n";
            }
        }
    }
    
    // Make sure the files are sorted by timestamp.
    for ( int i = 1; i < mFileTimestamps.size(); ++i )
    {
        assert(mFileTimestamps[i] > mFileTimestamps[i-1]);
    }
    
#if DEBUG
    cout << "Found " << mFileTimestamps.size() << " files. \n";
#endif
    mDatasetGPSStartTime = mFileTimestamps.front();
    mDatasetGPSEndTime = mFileTimestamps.back();
    mFileTimeInterval = fileTimeInterval;
}

#pragma mark - Time

double StoredDataConnection::worldTimeToGPSDataTime( double unixTime )
{
    // No time delays here since all the data is canned.
    // Just modulo the GPS time.
    // The stored data will play back as a loop.
    double amtTimeRunning = unixTime - mTimeDataStarted;// + mStartTimeOffset;
    double dataTimeSpan = mDatasetGPSEndTime - mDatasetGPSStartTime;
    return mDatasetGPSStartTime + fmod(amtTimeRunning, dataTimeSpan);
}

#pragma mark - Thread

// NOTE: Maybe this value should be derived from the file time intervals...
const static int kLoopTimeInterval = 16;

// NOTE: This method is designed with a static / limited dataset it mind.
// It iterates over the data directory EVERY new data request and picks the approp
// files. It was not designed with performance in mind, since it's
// intended to be for testing only (not live data).

void StoredDataConnection::work()
{
    double unixTime = getTimeNow();
    double gpsTime = worldTimeToGPSDataTime( unixTime );

    // NOTE: Start parsing 10 seconds before it's needed
    if ( gpsTime >= mNextPacketGPSStartTime - kLoopTimeInterval )
    {
        
#if DEBUG
        ci::Timer perfTimer( true );
#endif
        std::map<std::string, ligo::ChannelDataFrameRef> returnData;
        
        bool hasLoadedFirstData = mNextPacketGPSStartTime > 0;

        // Check the time and get the latest data
        if ( !hasLoadedFirstData )
        {
            // First load.
            // This will be published in the next loop.
            mNextPacketGPSStartTime = worldTimeToGPSDataTime( unixTime );
        }
        else
        {
            // Just start where we left off.
            mNextPacketGPSStartTime = mNextPacketGPSEndTime;
        }
        
        // Account for wrapping.
        // NOTE: Depending upon the loop interval, this may not be a seamless playback
        // if there's a time remainder, and nothing to show.
        if ( mNextPacketGPSStartTime >= mDatasetGPSEndTime )
        {
            mNextPacketGPSStartTime = mDatasetGPSStartTime;
        }
        
        mNextPacketGPSEndTime = mNextPacketGPSStartTime + kLoopTimeInterval;

        // EVERY TIME we load data, we re-open the files.
        // This can be made more efficient, but since we're on our own 10 second thread,
        // It should be OK for now.
        // IN ADDITION, if the directory is constantly updating w/ new files, this approach
        // wont work because the filenames, etc. are cached.
        int numFiles = mFileTimestamps.size();
        vector<string> loadPaths;
        for ( int i = 0; i < numFiles; ++i )
        {
            long fileStart = mFileTimestamps[i];
            long fileEnd = fileStart + mFileTimeInterval;
            if ( ( fileStart >= mNextPacketGPSStartTime && fileStart < mNextPacketGPSEndTime ) ||
                 ( fileEnd >= mNextPacketGPSStartTime && fileEnd < mNextPacketGPSEndTime ) ||
                 ( fileStart <= mNextPacketGPSStartTime && fileEnd >= mNextPacketGPSEndTime ) )
            {
                loadPaths.push_back(mDataPaths[i]);
            }
        }
        
        for ( string & filename : loadPaths )
        {
            appendFileData( filename, &returnData, mNextPacketGPSStartTime, mNextPacketGPSEndTime );
        }

        assert ( returnData.size() > 0 );
        
#if DEBUG
        perfTimer.stop();
        cout << "Data AQ required: " << perfTimer.getSeconds() << " sec.\n";
        cout << "Found " << returnData.size() << " channels.\n";
#endif
        // Publish as soon as it's done
        publishData(returnData, mNextPacketGPSStartTime, mNextPacketGPSEndTime);
    }
}

void StoredDataConnection::appendFileData(string & filename,
                                          map<string, ChannelDataFrameRef> * returnData,
                                          long dataStartTimestamp,
                                          long dataEndTimestamp )
{
    // Make sure channels are loaded
    assert( mChannelNames.size() > 0 );
    
    fs::path dataPath = mDataDirectory / filename;
    assert(fs::exists(dataPath));
    char *cFilename = (char *)(dataPath).string().c_str();
    FrFile *dataFile = FrFileINew( cFilename );
    if( dataFile == NULL )
    {
        cout << "Couldn't read " << filename << "\n";
    }
    else
    {
        try
        {
            FrameH *frame = FrameRead(dataFile);
            FrRawData *rawData = frame->rawData;
            FrAdcData *adcData = NULL;
            for( adcData = rawData->firstAdc; adcData != NULL; adcData = adcData->next )
            {
                string dataName(adcData->name);
                bool foundChannel = mChannelNames.find(dataName) != mChannelNames.end();
                if ( !foundChannel )
                {
                    // Only append whitelisted channels
                    continue;
                }
                
                ChannelDataFrameRef dataFrame;
                bool isNewDataFrame = returnData->count(dataName) == 0;
                if ( isNewDataFrame )
                {
                    dataFrame = ChannelDataFrameRef(new ChannelDataFrame());
                    dataFrame->channelName = dataName;
                    dataFrame->startTime = dataStartTimestamp;
                    dataFrame->endTime = dataEndTimestamp;
                    dataFrame->sampleRate = adcData->data->nData / frame->dt;
                    (*returnData)[dataName] = dataFrame;
                }
                else
                {
                    dataFrame = (*returnData)[dataName];
                }

                FrVect *vectData = adcData->data;
                long numData = vectData->nData;
                
                double localMinVal = 0;
                double localMaxVal = 0;
                bool didSetMinMax = false;
                double sumValue = dataFrame->meanValue * dataFrame->data.size();
                for ( int i = 0; i < numData; ++i )
                {
                    // Make sure it's within our time range
                    double sampleTimestamp = frame->GTimeS + (i * (frame->dt / numData));
                    if ( sampleTimestamp >= dataStartTimestamp &&
                         sampleTimestamp < dataEndTimestamp )
                    {
                        assert( i < vectData->nData );
                        double value = FrVectGetValueI(vectData, i);
                        // NOTE:
                        // Why would this ever be nan?
                        if ( isnan(value) || isinf(value) )
                        {
                            value = 0;
                        }
                        else
                        {
                            // Don't let nan or inf change the min / max
                            sumValue += value;
                            if ( !didSetMinMax )
                            {
                                localMinVal = value;
                                localMaxVal = value;
                                didSetMinMax = true;
                            }
                            if ( value < localMinVal )
                            {
                                localMinVal = value;
                            }
                            if ( value > localMaxVal )
                            {
                                localMaxVal = value;
                            }
                        }
                        // Append
                        dataFrame->data.push_back(value);
                    }
                }
#if DEBUG
                assert( !isnan(sumValue) );
#endif
                // Update min/max/mean
                if ( isNewDataFrame || localMinVal < dataFrame->minValue )
                {
                    dataFrame->minValue = localMinVal;
                }
                if ( isNewDataFrame || localMaxVal > dataFrame->maxValue )
                {
                    dataFrame->maxValue = localMaxVal;
                }
                int frameDataSize = dataFrame->data.size();
                if ( numData > 0 && frameDataSize > 0 )
                {
                    // Only update it if there's new data
                    dataFrame->meanValue = sumValue / frameDataSize;
#if DEBUG
                    assert( !isnan(dataFrame->meanValue) );
                    assert( dataFrame->maxValue >= dataFrame->meanValue );
                    assert( dataFrame->minValue <= dataFrame->meanValue );
#endif
                }
            }
            FrameFree(frame);
        }
        catch (Exception e)
        {
            cout << "ERROR reading frame " << cFilename << ":\n";
            cout << e.what() << "\n";
        }
    }
    
    // We've got the data.
    // Free the file.
    FrFileIEnd(dataFile);
    dataFile = NULL;
}