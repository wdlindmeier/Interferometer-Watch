//
//  ChannelFilterFFT.cpp
//  LIGOclock
//
//  Created by William Lindmeier on 7/25/14.
//
//

#include "ChannelFilterFFT.h"
#include "cinder/audio/FftProcessor.h"
#include "DataConnection.h"
#include "GroupingNavigation.h"
#include "AppConfig.h"

using namespace ligo;
using namespace std;
using namespace cinder;
using namespace cinder::audio;

static bool IsPowerOfTwo(long x)
{
    return (x & (x - 1)) == 0;
}

int FFTBinIndexForTargetFrequency( const FilterResultsRef & results, float targetFreq )
{
    FilterFFTResultsRef fftResults = static_pointer_cast<FilterFFTResults>(results);
    int fftBinNum = 0;
    if ( targetFreq > 0 )
    {
        fftBinNum = targetFreq / fftResults->frequencyInterval;
    }
    return fftBinNum;
}

FilterResultsRef ChannelFilterFFT::processChannelData( const ChannelDataFrameRef & channelData,
                                                       const ChannelDataFrameRef & prevChannelData )
{
    FilterFFTResults *results = new FilterFFTResults();

    if ( channelData->maxValue == channelData->minValue ||
         channelData->sampleRate < (kMinFFTFilterBandCount * 2) )
    {
#if DEBUG
        if ( Config::TimelineFFTChannelName() == channelData->channelName )
        {
            cout << "WARN: Primary FFT channel has no data. "
                 << "Min: " << channelData->minValue
                 << " Max: " << channelData->maxValue
                 << " Sample Rate: " << channelData->sampleRate << "\n";
            if ( !Config::ShouldUseLiveData() )
            {
                cout << "Break\n";
            }
        }
        else
        {
            if ( channelData->sampleRate >= (kMinFFTFilterBandCount * 2) )
            {
                for ( double d : channelData->data )
                {
                    assert( d == channelData->minValue && d == channelData->maxValue );
                }
            }
        }
#endif
        // This channel isn't appropriate for FFT analysis
        results->numBands = 0;
        results->numSamples = 0;
        results->sampleTimeInterval = 0;
        results->frequencyInterval = 0;
        // cout << "Cant FFT sample rate: " << channelData->sampleRate << "\n";
    }
    else
    {

        // FFT
        // http://stackoverflow.com/questions/1270018/explain-the-fft-to-me
        double dataDuration = channelData->endTime - channelData->startTime;
        long numData = channelData->data.size();
        
        // Scale up the band count according to sample rate
        int bandCount = kMinFFTFilterBandCount;
        if ( channelData->sampleRate == 1024 )
        {
            bandCount = std::max<int>(256, kMinFFTFilterBandCount);
        }
        else if ( channelData->sampleRate == 2048 )
        {
            bandCount = std::max<int>(512, kMinFFTFilterBandCount);
        }
        else if ( channelData->sampleRate >= 16384 )
        {
            bandCount = 2048; //channelData->sampleRate / 16;
        }

        // Sample 1 second at a time.
        // NOTE: This should probably be different from channel to channel, depending upon it's nature
        float samplesPerWindow = numData / dataDuration;
        // We're assuming all of the sample rates are powers of 2
        // NOTE: If not, just pick a generic sample rate.
        if ( !IsPowerOfTwo(samplesPerWindow) )
        {
            cout << "WARNING: " << channelData->channelName << " sample rate is " << samplesPerWindow << "\n";
            samplesPerWindow = 1024.0f;
        }

        int intervalMulti = samplesPerWindow / 2 / bandCount;
        float numWindows = numData / samplesPerWindow;
        float sampleRateHz = numData / dataDuration;
        float frqHzBinInterval = (sampleRateHz / samplesPerWindow) * intervalMulti;
        
        // FFT Results
        //    int numBins;
        //    std::vector<float> magnitudes;
        //    std::vector<float> frequencies;
        //    float sampleTimeInterval;
        //    int numSamples;
        results->numBands = bandCount;//kMinFFTFilterBandCount;
        results->numSamples = numWindows;
        results->sampleTimeInterval = dataDuration / numWindows;
        results->frequencyInterval = frqHzBinInterval;

        // Iterate over the data stream and get values for each window
        for ( int i = 0; i < numWindows; ++i )
        {
            vector<float> magnitudes;
            long firstSampleIndex = i * samplesPerWindow;
            // long lastSampleIndex = std::min<long>(firstSampleIndex + samplesPerWindow, numData);
            
            vector<float> sampleData;
            for ( long i = 0; i < samplesPerWindow; ++i )
            {
                sampleData.push_back( *(channelData->data.begin() + firstSampleIndex + i) );
            }
            
            Buffer32fRef buffer(new Buffer32f());
            buffer->mNumberChannels = 1; //
            buffer->mDataByteSize = sizeof(float) * samplesPerWindow;
            buffer->mSampleCount = samplesPerWindow;
            buffer->mData = &sampleData[0];

            std::shared_ptr<float> fftRef = calculateFft(buffer, bandCount); //kMinFFTFilterBandCount);
            float *fftBuffer = fftRef.get();        
            if ( !fftBuffer )
            {
                cout << "No FFT data for " << channelData->channelName << "\n";
            }

            // This is just a reference point to create "scalars" against.
            // Maybe this should be reconsidered.
            const static float kStandardMaxValue = 20.f;
            
            bool isDataCorrupt = false;
            for ( int i = 0; i < bandCount; ++i )
            {
                float fftValue = fftBuffer[i];
                if ( isinf(fftValue) || isnan(fftValue) )
                {
                    isDataCorrupt = true;
                }
                else if ( fftValue > results->maxMagnitude )
                {
                    results->maxMagnitude = fftValue;
                }                
            }
            
            if ( isDataCorrupt )
            {
                cout << "WARN: " << channelData->channelName << " fft data is corrupt\n";
            }
            
            if ( channelData->channelName == Config::TimelineFFTChannelName() )
            {
                // Keep the DARM benchmark in a sane range
                if ( results->maxMagnitude == 0 
                    || results->maxMagnitude < 10.f
                    || results->maxMagnitude > kStandardMaxValue * 10.f )
                {
                    results->maxMagnitude = kStandardMaxValue;
                }
            }
            
            for ( int i = 0; i < bandCount; ++i )
            {
                float fftValue = fftBuffer[i];
                if ( isinf(fftValue) || isnan(fftValue) )
                {
                    magnitudes.push_back(0);
                }
                else
                {
                    float logScalar = log(1.0 + fftValue) / log(1.f + results->maxMagnitude);
                    magnitudes.push_back( logScalar );
                }
            }

            // Save the magnitudes on the DARM channel so we can draw the full FFT
            results->bandMagnitudes.push_back(magnitudes);

            // Measure the "peakiness" not the raw FFT data.
            // We're looking at how much a given band breaks the trend
            // in an upward direction.
            int bandCount = magnitudes.size();
            const float kNumAverages = ceil(sqrt(samplesPerWindow) / 2.0f);
            float curveMomentum = 0.0f;
            vector<float> peaks;
            for ( int x = 0; x < bandCount; ++ x )
            {
                float scalarMag = magnitudes[x];
                float peakiness = 0.0f;
                
                if ( x > 0 && x < bandCount - 1 )
                {
                    curveMomentum = ((curveMomentum * (kNumAverages-1)) + scalarMag) / kNumAverages;
                    peakiness = std::min(1.0, std::max(0.0, 1.0 - ((curveMomentum * 1.25) / scalarMag)));
                }
                
                peaks.push_back(peakiness);
            }
            
            results->bandPeakiness.push_back(peaks);
        }
    }
    
    return FilterResultsRef(results);
}

std::map<std::string, FilterResultsRef> ChannelFilterFFT::processBundleData( const std::vector<DataChannelRef> & channels,
                                                                             FilteredDataPacket & channelResults,
                                                                             ChannelGroupingRef & bundle,
                                                                             void *userData )
{
    assert( userData != NULL );
    std::map<std::string, double> *userMap = static_cast<std::map<std::string, double> *>(userData);
    if ( userMap == NULL )
    {
        std::cout << "ERROR: ChannelFilterFFTResults requires a map<string, double> for user info.\n";
        assert(false);
    }
    
    map<string, FilterResultsRef> resultsByCategory;

    map<string, int> numCategoryContributors;
    bool isAll = bundle->name == kRootGroupingName;
    
    if ( isAll )
    {
        resultsByCategory[kRootGroupingName] = FilterFFTResultsRef(new FilterFFTResults());
    }
    else
    {
        assert( bundle->categories.size() > 0 );
        
        // Create a placeholder map for the categories
        for ( auto kvp : bundle->categories )
        {
            CategoryRef & category = kvp.second;
            assert( category->numChannels > 0 );
            string catKey = KeyForBundleCategory(bundle, category);
            resultsByCategory[catKey] = FilterFFTResultsRef(new FilterFFTResults());
        }
    }

    // Get the primary channel FFT.
    // NOTE: When we're analyzing all of the bands in aggregate,
    // we're using the frequency bands that are found in the primary
    // channel, not the constituent channels.
    string primaryChannelName = Config::TimelineFFTChannelName();
    FilterFFTResultsRef channelFilterValues = static_pointer_cast<FilterFFTResults>(channelResults.filterResultsByChannel[primaryChannelName][getFilterKey()]);
    int numBands = channelFilterValues->numBands;
    if ( numBands <= 0 )
    {
        cout << "WARN: Primary FFT has 0 bands\n";
        return resultsByCategory;
    }
        
    float fftFreqTarget = (int)(*userMap)["freqHz"];
    int masterFFTBinNum = FFTBinIndexForTargetFrequency(channelFilterValues, fftFreqTarget);
    float masterFreqInterval = channelFilterValues->frequencyInterval;
    
    long numSamples = channelFilterValues->numSamples;
    
    assert(numSamples > 0);
    
    // Iterate over channels
    for ( const DataChannelRef & channel : channels )
    {
        // Get the filter values
        FilterFFTResultsRef channelFilterValues = static_pointer_cast<FilterFFTResults>(channelResults.filterResultsByChannel[channel->name][ getFilterKey() ]);
        
        // Not an FFT channel
        if ( channelFilterValues->numSamples == 0 )
        {
            continue;
        }
        else
        {
            assert(channelFilterValues->numSamples == numSamples);
        }

        string catKey;
        if ( isAll )
        {
            catKey = kRootGroupingName;
        }
        else
        {
            // Get bundle category membership
            CategoryRef channelCat = channel->categoryMemberships[bundle->name];
            
            // Get key from cat
            catKey = KeyForBundleCategory(bundle, channelCat);
        }
        
        FilterFFTResultsRef categoryResults = static_pointer_cast<FilterFFTResults>(resultsByCategory[catKey]);
        bool wasEmpty = categoryResults->numSamples <= 0;
        if ( wasEmpty )
        {
            categoryResults->numSamples = numSamples;
            categoryResults->numBands = numBands;
            categoryResults->frequencyInterval = masterFreqInterval;
            categoryResults->ranks = vector<int>();
            numCategoryContributors[catKey] = 0;
        }
        
        numCategoryContributors[catKey] += 1;
        
        // Iterate over the samples
        for ( int i = 0; i < numSamples; ++i )
        {
            vector<float> bandMagnitudes;
            vector<float> bandPeaks;
            vector<int> ranks = categoryResults->ranks;
            
            bool hasIValues = categoryResults->bandMagnitudes.size() > i;
            if ( hasIValues )
            {
                bandMagnitudes = categoryResults->bandMagnitudes[i];
                bandPeaks = categoryResults->bandPeakiness[i];
            }
            
            for ( int b = 0; b < numBands; ++b )
            {
                
                bool hasBValues = bandMagnitudes.size() > b;
                // bool hasBValues = bandMagnitudes.size() > 0;
                
                if ( !hasBValues )
                {
                    ranks.push_back(0);
                    bandMagnitudes.push_back(0);
                    bandPeaks.push_back(0);
                }
                
                if ( b == masterFFTBinNum )
                {
                    // 1) Get the primary frequency
                    // float targetHz = masterFreqInterval * masterFFTBinNum;

                    // 2) Find the closest band in the channel
                    int fftBinNum = FFTBinIndexForTargetFrequency(channelFilterValues, fftFreqTarget);

                    // 3) IF there is a closest band, add their value to the aggregate
                    if ( fftBinNum < channelFilterValues->bandPeakiness[i].size() )
                    {
                        bandMagnitudes[b] += channelFilterValues->bandMagnitudes[i][fftBinNum];
                        bandPeaks[b] += channelFilterValues->bandPeakiness[i][fftBinNum];
                    }
                    // 4) Otherwise, nothing.
                }
                else
                {
                    // Dont bother calculating the non-target frequencies
                    bandMagnitudes[b] = 0;
                    bandPeaks[b] = 0;
                }
            }
            
            if ( !hasIValues )
            {
                categoryResults->bandMagnitudes.push_back( bandMagnitudes );
                categoryResults->bandPeakiness.push_back( bandPeaks );
                categoryResults->ranks.push_back( 0 );
            }
            else
            {
                // Overwrite with new vector
                categoryResults->bandMagnitudes[i] = bandMagnitudes;
                categoryResults->bandPeakiness[i] = bandPeaks;
                //categoryResults->ranks[i] = ranks;
            }
        }
    }
    
    // Kill any results that don't have values.
    // This can occur when we're passing in an arbitrary list of channels.
    vector<string> ignoreCategories;
    for ( auto kvp : resultsByCategory )
    {
        FilterFFTResultsRef categoryResults = static_pointer_cast<FilterFFTResults>(kvp.second);
        if ( categoryResults->bandMagnitudes.size() == 0 )
        {
            ignoreCategories.push_back(kvp.first);
        }
    }
    for ( string & catKey : ignoreCategories )
    {
        resultsByCategory.erase(resultsByCategory.find(catKey));
    }
    
    vector< pair<string, FilterFFTResultsRef> > sortedResults;
    
    // Iterate over the categories
    for ( auto kvp : resultsByCategory )
    {
        // Average the values
        string catKey = kvp.first;
        FilterFFTResultsRef categoryResults = static_pointer_cast<FilterFFTResults>(kvp.second);
        
        for ( int i = 0; i < categoryResults->numSamples; ++i )
        {
            //for ( int b = 0; b < numBands; ++b )
            // ONLY avg the target bin
            int b = masterFFTBinNum;
            {
                // NOTE: In the code above, we're just piling them on w/out averaging.
                // This averages them.
                double mag = categoryResults->bandMagnitudes[i][b];
                double peak = categoryResults->bandPeakiness[i][b];
                
                double avgMag = mag / numCategoryContributors[catKey];
                double avgPeak = peak / numCategoryContributors[catKey];
                
                // NOTE: Using peakiness in the bundle, because that's what's used for the timeline render.
                // HOWEVER, this could also be a different call to the filter API
                // categoryResults->bandMagnitudes[i][b] = avgMag;
                categoryResults->bandMagnitudes[i][b] = avgPeak;
                categoryResults->bandPeakiness[i][b] = avgPeak;
            }
        }
        
        sortedResults.push_back(pair<string, FilterFFTResultsRef>(kvp.first, categoryResults));
    }
    
    // Rank them for every sample in the results.
    // NOTE: This may be a little burdensome.
    for ( int i = 0; i < numSamples; ++i )
    {
        //for ( int b = 0; b < numBands; ++b )
        // ONLY sort the target bin.
        // The values in rank will refer to this target bin.
        int b = masterFFTBinNum;
        {
            sort(sortedResults.begin(), sortedResults.end(), [&](pair<string, FilterFFTResultsRef> & rA,
                                                                 pair<string, FilterFFTResultsRef> & rB)
                 {
                     // Sorting by the peakiness, not magnitude
                     return rA.second->bandPeakiness[i][b] > rB.second->bandPeakiness[i][b];
                 });
            
            int rank = 0;
            float lastValue = 0;
            for ( auto kvp : sortedResults )
            {
                FilterFFTResultsRef result = static_pointer_cast<FilterFFTResults>(resultsByCategory[kvp.first]);
                
                assert( rank == 0 || result->bandPeakiness[i][b] <= lastValue );
                lastValue = result->bandPeakiness[i][b];
                
                result->ranks[i] = rank;
                rank++;
            }
        }
    }
    
    return resultsByCategory;
}

int ChannelFilterFFT::filterIndexFromFilterTime( const FilterResultsRef & results, void *userData )
{
    assert( userData != NULL );
    std::map<std::string, double> *userMap = static_cast<std::map<std::string, double> *>(userData);
    if ( userMap == NULL )
    {
        std::cout << "ERROR: ChannelFilterFFTResults requires a map<string, double> for user info.\n";
        assert(false);
    }
    
    FilterFFTResultsRef fftResults = static_pointer_cast<FilterFFTResults>(results);
    assert(!!fftResults);
    
    assert( (*userMap).count("scalarTime") > 0 );
    
    double scalarTime = (*userMap)["scalarTime"];
    
    return scalarTime * fftResults->numSamples;
}

float ChannelFilterFFT::timelineIntensityForBundleValue( float bundleValue )
{
    float bundleIntensity = sqrt(bundleValue);
    return bundleIntensity * bundleIntensity * 3;
}

template <typename Func>
float ChannelFilterFFT::filterValueForIndex( const FilterResultsRef & results,
                                             int sampleIndex,
                                             void *userData,
                                             Func func )
{
    assert( userData != NULL );
    std::map<std::string, double> *userMap = static_cast<std::map<std::string, double> *>(userData);
    if ( userMap == NULL )
    {
        std::cout << "ERROR: ChannelFilterFFTResults requires a map<string, double> for user info.\n";
        assert(false);
    }
    FilterFFTResultsRef fftResults = static_pointer_cast<FilterFFTResults>(results);
    assert(!!fftResults);

    float fftFreqTarget = (int)(*userMap)["freqHz"];
    
    if ( ((fftResults->numBands + 1) * fftResults->frequencyInterval) < fftFreqTarget )
    {
        // This channel doesn't have enough info
        return 0;
    }

    const std::vector< std::vector<float> > & samples = func(fftResults);
    if ( samples.size() <= sampleIndex )
    {
        return 0;
    }
    
    const std::vector<float> & bins = samples[sampleIndex];
    
    int fftBinNum = FFTBinIndexForTargetFrequency(results, fftFreqTarget);

    if ( bins.size() <= fftBinNum )
    {
        return 0;
    }
    return bins[fftBinNum];
}

float ChannelFilterFFT::magnitudeForIndex( const FilterResultsRef & results,
                                                 int sampleIndex,
                                                 void *userData )
{
    return filterValueForIndex(results, sampleIndex, userData, [&]
    (FilterFFTResultsRef & fftResults)
    {
        return fftResults->bandMagnitudes;
    });
}

float ChannelFilterFFT::rankForIndex( const FilterResultsRef & results, int sampleIndex, void *userData )
{
    FilterFFTResultsRef fftResults = static_pointer_cast<FilterFFTResults>(results);
    assert(!!fftResults);
    return fftResults->ranks[sampleIndex]; //[fftBinNum];
}

float ChannelFilterFFT::barIntensityForIndex( const FilterResultsRef & results,
                                              int sampleIndex,
                                              void *userData )
{
    // Dim out channels that don't have a peak
    float peakiness = connectionIntensityForIndex(results, sampleIndex, userData);
    return 0.35f + (0.65f * peakiness);
}

float ChannelFilterFFT::connectionIntensityForIndex( const FilterResultsRef & results,
                                                    int sampleIndex,
                                                    void *userData )
{
    float peakiness = filterValueForIndex(results, sampleIndex, userData, [&]
                                          (FilterFFTResultsRef & fftResults)
                                          {
                                              return fftResults->bandPeakiness;
                                          });
    // Set a threshold
    const static float kFFTPeakThreshold = 0.15f;
    // Weights any connection between this node and others.
    // We'll ignore anything less than kFFTPeakThreshold.
    // If the value returned is <= 0, the line isn't drawn.
    return std::max(0.f, peakiness - kFFTPeakThreshold) * (1.2f + kFFTPeakThreshold);
}