//
//  ChannelFilter.h
//  LIGOclock
//
//  Created by William Lindmeier on 7/25/14.
//
//

#pragma once

#include "cinder/Cinder.h"
#include "SharedTypes.hpp"

/*
 
 ChannelFilter
 
     This is the base-class for any filter/plugin that you want to add to your app. This class probably
     doesn't need to be modified itself—all customization should happen in sub-classes by overloading
     the virtual methods.
 
 FilterResults
 
     Filter results hold filter-specific data about channels and category bundles.
     For example, the diff results could store a time-sequence array of diff values
     which could be for a specific channel, or an aggregate of category values.
     There's also a rank property so they can be sorted within their timeframe.
     Rank changes from sample to sample.

     When creating a new filter, you should also create a new sub-struct of FilterResults.
     This acts as a polymorphic "payload" struct for both channels and bundles.
     Specific sub-struct properties should only be accessed by the Filter itself,
     and use those to return visual-traits (e.g. barIntensityForTime, connectionIntensityForTime).
 
 FilteredDataPacket
    
    This is a struct that wraps the original DataPacket and includes filter results for the 
    same time period. This is the data structure that the front-end will use.
 
*/

namespace ligo
{
    struct FilterResults
    {
        float sampleTimeInterval;
        int numSamples;
        std::vector<int> ranks;
        
        FilterResults() :
        sampleTimeInterval(-1)
        ,numSamples(-1)
        {}
        // NOTE: The filter-specific data are in sub-structs
    };
    
    typedef std::shared_ptr<FilterResults> FilterResultsRef;
    
    struct FilteredDataPacket
    {
        DataPacket parsedData;
        
        // Channels:
        // These values are used in the "Moment" view. Looking at individual channels.
        // channelResults[channelName][filterName] = Channel FilterResultsRef;
        std::map<std::string, std::map<std::string, FilterResultsRef > > filterResultsByChannel;

        // Aggregate (by Bundle):
        // These values are used in the "Timeline" view. Looking at category aggregate values.
        // bundleResults[unique_bundling_key][categoryKey] = FilterResultsRef
        std::map<std::string, std::map<std::string, FilterResultsRef > > filterResultsByNavBundling;

        FilteredDataPacket( const DataPacket & parsedData ) :
        parsedData(parsedData)
        {};
    };
    
    typedef std::shared_ptr<FilteredDataPacket> FilteredDataPacketRef;

    const static std::string kChannelFilterKeyNone = "No Filter";
    
    class ChannelFilter
    {
        
    protected:
        
        std::string mDescription;
        
    public:
        
        virtual int filterIndexFromFilterTime( const FilterResultsRef & results, void *userData ) = 0;
        
        virtual const std::string & getFilterKey(){ return kChannelFilterKeyNone; };
        
        virtual FilterResultsRef processChannelData( const ChannelDataFrameRef & channelData,
                                                     const ChannelDataFrameRef & prevChannelData ) = 0;
        
        // returns map<category_key, FilterResultsRef>
        virtual std::map<std::string, FilterResultsRef> processBundleData( const std::vector<DataChannelRef> & channels,
                                                                           FilteredDataPacket & channelResults,
                                                                           ChannelGroupingRef & bundle,
                                                                           void *userData ) = 0;
        
        void setDescription(const std::string & desc ){ mDescription = desc; }
        std::string getDescription(){ return mDescription; };
        
        // The accessors below tell the app how to draw data which has been processed by the
        // filter. This is the only interface which should be queried by the font-end, so
        // the filters can remain polymorphic (with the possible exception of filter-specific interface. E.g. The FFT spectrum).
        virtual float rankForIndex( const FilterResultsRef & results,
                                    int sampleIndex,
                                    void *userData ) = 0;
        
        virtual float rankForTime( const FilterResultsRef & results,
                                   void *userData );

        virtual float timelineIntensityForBundleValue( float bundleValue  ) = 0;

        virtual float barIntensityForIndex( const FilterResultsRef & results,
                                            int sampleIndex,
                                            void *userData ) = 0;
        
        virtual float barIntensityForTime( const FilterResultsRef & results,
                                           void *userData );

        virtual float connectionIntensityForIndex( const FilterResultsRef & results,
                                                   int sampleIndex,
                                                   void *userData ) = 0;

        virtual float connectionIntensityForTime( const FilterResultsRef & results,
                                                  void *userData );
        
        virtual float magnitudeForIndex( const FilterResultsRef & results,
                                         int sampleIndex,
                                         void *userData ) = 0;
        
        virtual float magnitudeForTime( const FilterResultsRef & results,
                                        void *userData );
    };
    
    typedef std::shared_ptr<ChannelFilter> ChannelFilterRef;
    
}

