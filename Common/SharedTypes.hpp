//
//  SharedTypes.hpp
//  gpuPS
//
//  Created by William Lindmeier on 7/10/14.
//
//

#ifndef gpuPS_SharedTypes_hpp
#define gpuPS_SharedTypes_hpp

#include <iostream>
#include <map>
#include <vector>
#include "cinder/Cinder.h"
#include "cinder/Color.h"
#include "cinder/Filesystem.h"

namespace ligo
{
    
struct Category
{
    ci::ColorAf color;
    std::string name;
    std::string key;
    std::string groupingName;
    long numChannels;
    Category() :
    groupingName("")
    ,numChannels(0)
    ,name("")
    ,color(1,1,1,1)
    {}
};

typedef std::shared_ptr<Category> CategoryRef;

struct CategoryAttractor
{
    CategoryRef category;
    ci::Vec3f position;
};

typedef std::shared_ptr<CategoryAttractor> CategoryAttractorRef;

struct FilterTimelineValue
{
    FilterTimelineValue() :
    value(0)
    ,rank(-1)
    {}
    
    FilterTimelineValue( float value, long rank ) :
    value(value)
    ,rank(rank)
    {}
    
    float value;
    long rank;
};

struct DataChannel
{
    DataChannel(){};
    DataChannel(const std::string & name) : name(name) {}
    
    std::string name;
    
    // Grouping Name : Category
    std::map<std::string, CategoryRef> categoryMemberships;
    
#ifdef DEPRECIATED
    // Filter Name : Values
    std::map<std::string, std::vector<FilterTimelineValue> > filterTimelineValues;
#endif
};

typedef std::shared_ptr<DataChannel> DataChannelRef;

struct ChannelGrouping
{
    ChannelGrouping() :
    name("")
    ,isBundle(false)
    ,didLoad(false)
    ,bundlingColumnIndex(-1)
    {}
#ifdef DEPRECIATED
    ci::fs::path categoryChannelBundlingPath;
    std::vector<CategoryRef> categories;
#else
    std::map<std::string, CategoryRef> categories;
#endif
    bool didLoad;
    ci::fs::path categoryKeyPath;
    int bundlingColumnIndex;
    std::string name;
    bool isBundle;
    
    // Simple compare by name
    bool operator<(const ChannelGrouping & otherGrouping)
    {
        return name < otherGrouping.name;
    }
    
    bool operator>(const ChannelGrouping & otherGrouping)
    {
        return name > otherGrouping.name;
    }
};

typedef std::shared_ptr<ChannelGrouping> ChannelGroupingRef;

struct FilteredChannelData
{
    FilteredChannelData() :
    name("")
    ,didLoad(false)
    ,dataStartTime(-1)
    ,dataEndTime(-1)
    ,numSamples(-1)
    {}
    
    std::string name;
    bool didLoad;
    ci::fs::path channelValuesPath;
    
    long dataStartTime;
    long dataEndTime;
    long numSamples;
};
    
typedef std::shared_ptr<FilteredChannelData> FilteredChannelDataRef;
    
struct ChannelDataFrame
{
    ChannelDataFrame() :
    minValue(0)
    ,maxValue(0)
    ,meanValue(0)
    ,sampleRate(0)
    {}
    time_t startTime;
    time_t endTime;
    long sampleRate;
    std::string channelName;
    std::vector<double> data;
    double minValue;
    double maxValue;
    double meanValue;
};

typedef std::shared_ptr<ChannelDataFrame> ChannelDataFrameRef;
struct DataRange
{
    double min;
    double max;
    bool wasSet;
    DataRange() : min(-1), max(-1), wasSet(false){};
};
//typedef std::pair<double,double> DataRange;
    
struct DataPacket
{
    std::map<std::string, ligo::ChannelDataFrameRef> channelData;
    long dataGPSStartTime;
    long dataGPSEndTime;
    
    DataPacket(const std::map<std::string, ligo::ChannelDataFrameRef> & channelData,
                     long dataGPSStartTime, long dataGPSEndTime ) :
    dataGPSStartTime(dataGPSStartTime)
    ,dataGPSEndTime(dataGPSEndTime)
    ,channelData(channelData)
    {};
};
    
static const std::string KeyForBundleCategory(ChannelGroupingRef & bundle, CategoryRef & category )
{
    if ( !bundle->isBundle )
    {
        return bundle->name;
    }
    return bundle->name + "_" + category->name;
}

#ifdef DEPRECIATED
    
struct ChannelDiff
{
    std::string name;
    CategoryRef category;
    std::vector<float> diffValues;
    std::vector<long> diffRanks;
};

typedef std::shared_ptr<ChannelDiff> ChannelDiffRef;

#endif // DEPRECIATED
    
}

#endif
