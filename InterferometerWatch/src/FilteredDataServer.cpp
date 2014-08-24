//
//  FilteredDataServer.cpp
//  LIGOclock
//
//  Created by William Lindmeier on 7/25/14.
//
//

#include "FilteredDataServer.h"
#include "ProcessingPipeline.h"
#include "GPSTime.hpp"
#include "AppConfig.h"

using namespace ligo;
using namespace ci;
using namespace std;

void FilteredDataServer::work()
{
    bool hasFilteredData = ProcessingPipeline::getInstance().hasFilteredData();
    if ( hasFilteredData )
    {
        // How much memory does each packet consume?
        // Hundreds of MB

        // Store the data w/ a timestamp
        // Copy the packet into a shared pointer
        FilteredDataPacket * packetPointer = new FilteredDataPacket( ProcessingPipeline::getInstance().dequeueFilteredData());
        
        cout << "Dispatch found " << packetPointer->parsedData.channelData.size()
             << " new channels for timestamp " << packetPointer->parsedData.dataGPSStartTime
             << "\n";
        
        FilteredDataPacketRef packetRef(packetPointer);
        mPacketData.push_back(packetRef);
        time_t packetGPSTime = packetRef->parsedData.dataGPSStartTime;
        mPacketTimestamps.push_back(packetGPSTime);
        assert(mPacketTimestamps.size() == mPacketTimestamps.size());
        
        // Purge expired data so our memory footprint doesn't continue to grow out of control.
        // Purging will always be measured against the newest packet.
        purgeExpiredData( packetGPSTime );
    }
}

void FilteredDataServer::purgeExpiredData( time_t currentGPSTime )
{
    // Find timestamps that are older than Config::TimelineDuration()
    // NOTE: We're giving it a 0.5 duration buffer so it should never draw blank data at the
    // wrapping edge.
    time_t oldestTimestamp = currentGPSTime - (Config::TimelineDuration() * 1.5);
    vector<int> purgeIndexes;
    for ( int i = 0; i < mPacketTimestamps.size(); ++i )
    {
        time_t timestamp = mPacketTimestamps[i];
        if ( timestamp < oldestTimestamp )
        {
            cout << "Purging data for " << timestamp << "\n";
            purgeIndexes.push_back(i);
        }
        else
        {
            // Assuming the timestamps are sorted in chrono order.
            // Dont keep looking if this timestamp is newer than the cutoff.
            break;
        }
    }
    
    if ( purgeIndexes.size() > 0 )
    {
        for ( int i : purgeIndexes )
        {
            mPacketData.erase( mPacketData.begin() + i );
            mPacketTimestamps.erase( mPacketTimestamps.begin() + i );
        }
    }
}

time_t FilteredDataServer::getNewestTimestamp()
{
    if ( mPacketTimestamps.size() > 0 )
    {
        return mPacketTimestamps.back();
    }
    return -1;
}

int FilteredDataServer::getDataIndexForTimestamp( const time_t timestamp )
{
    auto const it = std::upper_bound( mPacketTimestamps.begin(), mPacketTimestamps.end(), timestamp );
    if ( it == mPacketTimestamps.end() )
    {
        // This might mean that the last index should be returned.
        // OR that the data doesn't exist yet.
        // Check the last packet end time.
        int packetCount = mPacketData.size();
        if ( packetCount > 0 )
        {
            ligo::FilteredDataPacketRef lastPacket = mPacketData.back();
            if ( timestamp < lastPacket->parsedData.dataGPSEndTime )
            {
                return packetCount - 1;
            }
        }
        return -1;
    }
    return (it - mPacketTimestamps.begin()) - 1;
}

const ligo::FilteredDataPacketRef FilteredDataServer::getDataForTimestamp( const time_t timestamp )
{
    int foundIndex = getDataIndexForTimestamp(timestamp);
    
    if ( foundIndex == -1 )
    {
        // NOTE: The data doesn't exist yet.
        return FilteredDataPacketRef();
    }

    return mPacketData[foundIndex];
}