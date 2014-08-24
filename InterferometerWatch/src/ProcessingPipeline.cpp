//
//  ProcessingPipeline.cpp
//  LIGOclock
//
//  Created by William Lindmeier on 7/28/14.
//
//

#include "ProcessingPipeline.h"
#include "cinder/Cinder.h"

using namespace ligo;
using namespace std;
using namespace ci;

void ProcessingPipeline::enqueueParsedData( const ligo::DataPacket & packet )
{
    std::lock_guard<std::mutex> lock(mParsedDataMutex);
    mParsedDataQueue.push(packet);
}

const ligo::DataPacket ProcessingPipeline::dequeueParsedData()
{
    std::lock_guard<std::mutex> lock(mParsedDataMutex);
    ligo::DataPacket packet = mParsedDataQueue.front();
    mParsedDataQueue.pop();
    return packet;
}

bool ProcessingPipeline::hasParsedData()
{
    // NOTE; Not locking since this will be called  bajillion times and it doesn't modify data
    return mParsedDataQueue.size() > 0;
}

void ProcessingPipeline::enqueueFilteredData( const ligo::FilteredDataPacket & packet )
{
    std::lock_guard<std::mutex> lock(mFilteredDataMutex);
    mFilteredDataQueue.push(packet);
}

const ligo::FilteredDataPacket ProcessingPipeline::dequeueFilteredData()
{
    std::lock_guard<std::mutex> lock(mFilteredDataMutex);
    ligo::FilteredDataPacket packet = mFilteredDataQueue.front();
    mFilteredDataQueue.pop();
    return packet;
}

bool ProcessingPipeline::hasFilteredData()
{
    // NOTE; Not locking since this will be called  bajillion times and it doesn't modify data
    return mFilteredDataQueue.size() > 0;
}