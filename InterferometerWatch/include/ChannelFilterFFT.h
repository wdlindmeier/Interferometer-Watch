//
//  ChannelFilterFFT.h
//  LIGOclock
//
//  Created by William Lindmeier on 7/25/14.
//
//

#pragma once

#include "cinder/Cinder.h"
#include "ChannelFilter.h"

/*
 
 ChannelFilterFFT
 
 Another sample filter than runs each channel through FFT and looks for peaks across a 
 channels bands. The magnitude of each band is stored as well as it's "peakiness". Peaks are 
 measured by averaging a set number of bands and then comparing how much a given band diverges 
 from that average.
 
*/

namespace ligo
{
    const static std::string kChannelFilterKeyFFT = "FFTFilter";
    const static int kMinFFTFilterBandCount = 256; // pick a power of 2
    
    struct FilterFFTResults : FilterResults
    {
        int numBands;
        double maxMagnitude;
        std::vector< std::vector<float> > bandMagnitudes;
        std::vector< std::vector<float> > bandPeakiness;
        float frequencyInterval;
        
        FilterFFTResults() :
        FilterResults()
        ,numBands(0)
        ,frequencyInterval(0)
        ,maxMagnitude(0)
        {};
    };
    
    typedef std::shared_ptr<FilterFFTResults> FilterFFTResultsRef;

    class ChannelFilterFFT : public ChannelFilter
    {
        
    protected:
        
        template <typename Func>
        float filterValueForIndex( const FilterResultsRef & results, int sampleIndex, void *userData, Func func );
        
    public:
        
        virtual int filterIndexFromFilterTime( const FilterResultsRef & results, void *userData );
        
        virtual const std::string & getFilterKey(){ return kChannelFilterKeyFFT; };        
        virtual FilterResultsRef processChannelData( const ChannelDataFrameRef & channelData,
                                                     const ChannelDataFrameRef & prevChannelData );
        
        virtual std::map<std::string, FilterResultsRef> processBundleData( const std::vector<DataChannelRef> & channels,
                                                                           FilteredDataPacket & channelResults,
                                                                           ChannelGroupingRef & bundle,
                                                                           void *userData );
        
        virtual float timelineIntensityForBundleValue( float bundleValue );
        
        // user data should be a map<string, double> of values including:
        // "freqHz" == the target freq. The closest bin will be selected.
        virtual float magnitudeForIndex( const FilterResultsRef & results,
                                               int sampleIndex,
                                               void *userData );

        virtual float barIntensityForIndex( const FilterResultsRef & results,
                                            int sampleIndex,
                                            void *userData );

        virtual float rankForIndex( const FilterResultsRef & results,
                                    int sampleIndex,
                                    void *userData );
        
        virtual float connectionIntensityForIndex( const FilterResultsRef & results,
                                                   int sampleIndex,
                                                   void *userData );

    };
    
    typedef std::shared_ptr<ChannelFilterFFT> ChannelFilterFFTRef;
}