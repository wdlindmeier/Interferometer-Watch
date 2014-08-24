//
//  DataParser.h
//  LIGOclock
//
//  Created by William Lindmeier on 7/25/14.
//
//

#pragma once

#include "cinder/Cinder.h"
#include "ChannelFilter.h"
#include "DataConnection.h"
#include "ThreadedWorker.hpp"

/*
 
 DataParser
 
 Takes DataPackets that are waiting in the pipeline and runs them through all of the active plugins.
 Once they've been processed, they're added back to the pipeline as FilteredDataPackets.
 This class probably deserves a different name.
 
*/


namespace ligo
{
    class DataParser;

    class DataParser : public ThreadedWorker
    {        

    private:
        
        // Private for singleton
        DataParser() :
        ThreadedWorker()
        {
            setupChannelFilters();
            startWorkerThread();
        }
        
        // Don't implement so the singleton can't be copied
        DataParser( DataParser const & );
        void operator = ( DataParser const & );

    protected:
        
        std::vector<ChannelFilterRef> mChannelFilters;
        std::shared_ptr<ligo::DataPacket> mPrevPacket;
        
        void setupChannelFilters();
        
        virtual void work();

    public:
        
        static DataParser & getInstance()
        {
            // Instantiated on first use & guaranteed to be destroyed.
            static DataParser instance;
            return instance;
        }
        
        const std::vector<ChannelFilterRef> getAllFilters();
        
        virtual ~DataParser(){ killWorkerThread(); }

    };
    
    typedef std::shared_ptr<DataParser> DataParserRef;
}