//
//  DataConnection.h
//  LIGOclock
//
//  Created by William Lindmeier on 7/25/14.
//
//

#pragma once

#include "cinder/Cinder.h"
#include "SharedTypes.hpp"
#include "ThreadedWorker.hpp"
#include <unordered_set>

/*
 
 DataConnection
 
 The base class for loading data, and parsing it into native structs. Data connection creates
 DataPackets and dumps them into the pipeline. The connection runs on it's own thread.
 
*/

namespace ligo
{
    class DataConnection;
    
    class DataConnection : public ThreadedWorker
    {
        
    protected:

        void publishData(const std::map<std::string, ligo::ChannelDataFrameRef> &,
                         long startTime, long endTime );
        bool mIsConnected;
        std::vector<std::string> mOrderedChannels;
        std::unordered_set<std::string> mChannelNames;
        
        void loadChannels();
        
    public:
        
        DataConnection();
        
        virtual ~DataConnection() { closeConnection(); };
        
        virtual double worldTimeToGPSDataTime( double unixTime ) = 0;

        virtual void openConnection();
        virtual void closeConnection();
        
        const std::vector<std::string> & getChannelNames();
        
    };
    
    typedef std::shared_ptr<DataConnection> DataConnectionRef;
    
}