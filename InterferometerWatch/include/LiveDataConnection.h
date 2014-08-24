//
//  LiveDataConnection.h
//  LIGOclock
//
//  Created by William Lindmeier on 7/25/14.
//
//

#pragma once

#include "cinder/Cinder.h"
#include "DataConnection.h"
#include "NDSDataStreamer.h"

/*
 
 LiveDataConnection
 
 A subclass of DataConnection that connects to an NDS server and requests data on a timed loop.
 
*/

namespace ligo
{
    typedef std::map<std::string, ChannelDataFrameRef> ChannelDataBatch;
    
    class LiveDataConnection : public DataConnection, public NDSDataListener
    {
        
    private:
        
        std::mutex mDataMutex;
        
        std::vector<NDSDataStreamerRef> mDataStreamers;
        std::map<time_t, ChannelDataBatch > mTimedDataBatches;

        void loadStreamers();
        void releaseDataBatch(ChannelDataBatch & batch, time_t startTime, time_t endTime );
        
    protected:
        
        virtual void work();
        
    public:
        
        LiveDataConnection();
        
        virtual void closeConnection();
        
        void openConnectionToNDServer( const std::string & hostname, int port );
        
        virtual void ndsDataDidReceiveNewChannelData( std::map<std::string, ChannelDataFrameRef> &,
                                                     NDSDataStreamer *streamer );
        
        virtual double worldTimeToGPSDataTime( double unixTime );
        
    };
    
    typedef std::shared_ptr<LiveDataConnection> LiveDataConnectionRef;
}