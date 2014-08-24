//
//  DataConnection.cpp
//  LIGOclock
//
//  Created by William Lindmeier on 7/25/14.
//
//

#include "DataConnection.h"
#include "SharedTypes.hpp"
#include "ProcessingPipeline.h"
#include "ChannelDataIO.h"
#include "cinder/app/AppBasic.h"
#include "AppConfig.h"
#include "ChannelStateManager.h"

using namespace ligo;
using namespace std;
using namespace ci;

DataConnection::DataConnection() :
ThreadedWorker()
{
    loadChannels();
};

void DataConnection::loadChannels()
{
    // We only want one place that loads channels.
    // Get them from the ChannelManager.
    mOrderedChannels.clear();
    vector<DataChannelRef> approvedChannels = ChannelStateManager::getInstance().getAllChannels();
    for ( DataChannelRef & channel : approvedChannels )
    {
        mOrderedChannels.push_back(channel->name);
    }
    
    mChannelNames = std::unordered_set<std::string>(mOrderedChannels.begin(), mOrderedChannels.end());
    
    // Always include the timeline channel name so we get the data for drawing.
    if ( mChannelNames.find(Config::TimelineWaveformChannelName()) == mChannelNames.end() )
    {
        mOrderedChannels.push_back(Config::TimelineWaveformChannelName());
        mChannelNames.insert(Config::TimelineWaveformChannelName());
    }

    // Always include the DARM DQ channel
    if ( mChannelNames.find(Config::TimelineFFTChannelName()) == mChannelNames.end() )
    {
        mOrderedChannels.push_back(Config::TimelineFFTChannelName());
        mChannelNames.insert(Config::TimelineFFTChannelName());
    }
}

const std::vector<std::string> & DataConnection::getChannelNames()
{
    return mOrderedChannels;
}

#pragma mark - Publishing Data

void DataConnection::publishData( const std::map<std::string, ligo::ChannelDataFrameRef> & newData,
                                  long startTime, long endTime )
{
    ProcessingPipeline::getInstance().enqueueParsedData(DataPacket(newData, startTime, endTime));
}

#pragma mark - Connection

void DataConnection::openConnection()
{
    assert( mChannelNames.size() > 0 );
    if ( mIsConnected )
    {
        closeConnection();
    }
    mIsConnected = true;
    // ... Implement the rest in the subclass
}

void DataConnection::closeConnection()
{
    killWorkerThread();
    mIsConnected = false;
}
