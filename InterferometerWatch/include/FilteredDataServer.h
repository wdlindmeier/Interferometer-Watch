//
//  FilteredDataServer.h
//  LIGOclock
//
//  Created by William Lindmeier on 7/25/14.
//
//

#pragma once

#include "cinder/Cinder.h"
#include "DataParser.h"
#include "SharedTypes.hpp"
#include <unordered_map>

/*
 
 FilteredDataServer
 
 This is the "data dispatcher". It takes FilteredDataPackets waiting in the pipeline and stores
 them for time-based lookup. When data packets expire (by being older than the timeline duration) 
 they are deleted.
 
*/

namespace ligo
{
    class FilteredDataServer : ThreadedWorker
    {
        
    private:

        // Private for singleton
        FilteredDataServer() :
        ThreadedWorker()
        {
            startWorkerThread();
        }
        
        // Don't implement so the singleton can't be copied
        FilteredDataServer( FilteredDataServer const & );
        void operator = ( FilteredDataServer const & );
                
    protected:
        
        std::vector<time_t> mPacketTimestamps;
        std::vector<ligo::FilteredDataPacketRef> mPacketData;
        
        virtual void work();
        
        void purgeExpiredData( time_t currentGPSTime );
        
    public:

        static FilteredDataServer & getInstance()
        {
            // Instantiated on first use & guaranteed to be destroyed.
            static FilteredDataServer instance;
            return instance;
        }
        
        virtual ~FilteredDataServer(){ killWorkerThread(); };
        
        void setHistoryDuration(long bufferDuration);
        
        time_t getNewestTimestamp();
        
        // NOTE: getDataIndexForTimestamp can be used to see if the data exists
        // yet, and if the data has changed. That's why it's public.
        // Call this before calling getDataForTimestamp to make sure the data
        // exists.
        // TODO:
        // This should be refactored to be more semantic.
        int getDataIndexForTimestamp( const time_t timestamp );
        const ligo::FilteredDataPacketRef getDataForTimestamp( const time_t timestamp );
        
    };
    
    typedef std::shared_ptr<FilteredDataServer> FilteredDataServerRef;
}