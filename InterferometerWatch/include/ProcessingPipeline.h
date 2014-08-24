//
//  ProcessingPipeline.h
//  LIGOclock
//
//  Created by William Lindmeier on 7/28/14.
//
//

#ifndef __LIGOclock__ProcessingPipeline__
#define __LIGOclock__ProcessingPipeline__

#include <iostream>
#include <queue>
#include "SharedTypes.hpp"
#include "ChannelFilter.h"

/*
 
 ProcessingPipeline
 
 This acts as a waiting room for data that's been partly processed. Once packets have been gathered 
 by the DataConnection, they wait here until the DataParser is ready for them. After they've been
 parsed, they wait here again until the FilteredDataServer is ready to catalog them.
 
*/


namespace ligo
{
    class ProcessingPipeline
    {
        
    private:
        
        // Private for singleton
        ProcessingPipeline()
        {}
        
        // Don't implement so the singleton can't be copied
        ProcessingPipeline( ProcessingPipeline const & );
        void operator = ( ProcessingPipeline const & );
        
        std::queue<ligo::DataPacket> mParsedDataQueue;
        std::queue<ligo::FilteredDataPacket> mFilteredDataQueue;
        
        std::mutex mParsedDataMutex;
        std::mutex mFilteredDataMutex;

    public:
        
        static ProcessingPipeline & getInstance()
        {
            // Instantiated on first use & guaranteed to be destroyed.
            static ProcessingPipeline instance;
            return instance;
        }
        
        void enqueueParsedData( const ligo::DataPacket & packet );
        const ligo::DataPacket dequeueParsedData();
        bool hasParsedData();

        void enqueueFilteredData( const ligo::FilteredDataPacket & packet );
        const ligo::FilteredDataPacket dequeueFilteredData();
        bool hasFilteredData();

    };
}

#endif /* defined(__LIGOclock__ProcessingPipeline__) */
