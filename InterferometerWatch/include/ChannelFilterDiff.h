//
//  ChannelFilterDiff.h
//  LIGOclock
//
//  Created by William Lindmeier on 7/25/14.
//
//

#pragma once

#include "cinder/Cinder.h"
#include "ChannelFilter.h"

/*
 
 ChannelFilterDiff
 
 An example filter that scores changes in a signal. It looks at 2 second windows of the data
 and compares the range and average against the window that came before it.
 
*/


namespace ligo
{
    const static std::string kChannelFilterKeyDiff = "DiffFilter";
    
    struct FilterDiffResults : public FilterResults
    {
        // Used for aggregates
        std::vector<float> scores;
        
        FilterDiffResults() :
        FilterResults()
        {};
    };
    
    typedef std::shared_ptr<FilterDiffResults> FilterDiffResultsRef;

    class ChannelFilterDiff : public ChannelFilter
    {
        
    protected:
        
    public:
        
        virtual int filterIndexFromFilterTime( const FilterResultsRef & results, void *userData );
        
        virtual const std::string & getFilterKey(){ return kChannelFilterKeyDiff; };
        
        virtual FilterResultsRef processChannelData( const ChannelDataFrameRef & channelData,
                                                     const ChannelDataFrameRef & prevChannelData );
        
        virtual std::map<std::string, FilterResultsRef> processBundleData( const std::vector<DataChannelRef> & channels,
                                                                           FilteredDataPacket & channelResults,
                                                                           ChannelGroupingRef & bundle,
                                                                           void *userData );

        // Override the visual accessors. Only "Index" (not "Time") versions need to be
        // overridden, because the superclass will do the conversion.
        virtual float timelineIntensityForBundleValue( float bundleValue  );
        
        virtual float rankForIndex( const FilterResultsRef & results,
                                          int sampleIndex,
                                          void *userData );

        virtual float barIntensityForIndex( const FilterResultsRef & results,
                                        int sampleIndex,
                                        void *userData );

        virtual float connectionIntensityForIndex( const FilterResultsRef & results,
                                                  int sampleIndex,
                                                  void *userData );

        virtual float magnitudeForIndex( const FilterResultsRef & results,
                                               int sampleIndex,
                                               void *userData );
    };
    
    typedef std::shared_ptr<ChannelFilterDiff> ChannelFilterDiffRef;
}