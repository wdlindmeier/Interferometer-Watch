//
//  ChannelFilterDiff.cpp
//  LIGOclock
//
//  Created by William Lindmeier on 7/25/14.
//
//

#include "ChannelFilterDiff.h"
#include "GroupingNavigation.h"
#include <limits>
#include "AppConfig.h"

using namespace ligo;
using namespace std;

struct DiffFrame
{
    DiffFrame(double gpsStartTime, double gpsEndTime) :
    startTime(gpsStartTime)
    ,endTime(gpsEndTime)
    ,minValue(std::numeric_limits<double>::min())
    ,maxValue(std::numeric_limits<double>::min())
    ,avgValue(std::numeric_limits<double>::min())
    ,valueRange(std::numeric_limits<double>::min())
    ,avgDiff(0)
    ,rangeDiff(0)
    ,diff(0)
    ,signedDiff(0)
    {}
    
    // Get a subset from the larger data-set. Calculate the bounds.
    void sampleData(const std::vector<double> & dataValues,
                    double dataGPSStartTime,
                    double dataGPSEndTime)
    {
        if ( dataValues.size() == 0 )
        {
            // Just zero it out
            minValue = 0;
            maxValue = 0;
            avgValue = 0;
            valueRange = 0;
            return;
        }
        
        float scalarStart = MAX(0, (startTime - dataGPSStartTime) / (dataGPSEndTime - dataGPSStartTime));
        float scalarEnd = MIN(1.0, (endTime - dataGPSStartTime) / (dataGPSEndTime - dataGPSStartTime));
        long dataIdxStart = dataValues.size() * scalarStart;
        long dataIdxEnd = MIN(ceil(dataValues.size() * scalarEnd), dataValues.size());
        double sumValue = 0;
        
        assert( dataIdxEnd - dataIdxStart > 0 );
        
        long numValues = dataIdxEnd - dataIdxStart;
        assert( numValues > 0 );
        for ( long i = dataIdxStart; i < dataIdxEnd; ++i )
        {
            double val = dataValues[i];
            sumValue += val;
            if ( i == dataIdxStart )
            {
                minValue = val;
                maxValue = val;
            }
            else
            {
                if ( val < minValue )
                {
                    minValue = val;
                }
                if ( val > maxValue )
                {
                    maxValue = val;
                }
            }
        }
        avgValue = sumValue / numValues;
        valueRange = maxValue - minValue;
    }
    
    // Compare this frame with another frame (usually the one that came before it).
    void calculateDiff( DiffFrame & otherFrame,
                        double standardAverage,
                        double standardRange )
    {
        assert( minValue != std::numeric_limits<double>::min() ||
                maxValue != std::numeric_limits<double>::min() ||
                avgValue != std::numeric_limits<double>::min() );
        
        if ( standardRange != 0 )
        {
            rangeDiff = (otherFrame.valueRange - valueRange) / standardRange;
            avgDiff = ((avgValue - standardAverage) - (otherFrame.avgValue - standardAverage)) / standardRange;
            diff = (fabs(rangeDiff) + fabs(avgDiff)) * 0.5;
            signedDiff = (rangeDiff + avgDiff) * 0.5f;
#if DEBUG
            assert(!isnan(rangeDiff) && !isinf(rangeDiff));
            assert(!isnan(avgDiff) && !isinf(avgDiff));
            assert(!isnan(diff) && !isinf(diff));
            assert(!isnan(signedDiff) && !isinf(signedDiff));
#endif
        }
        else
        {
            rangeDiff = 0;
            avgDiff = 0;
            diff = 0;
            signedDiff = 0;
        }
    }
    
    double startTime;
    double endTime;
    double avgValue;
    double valueRange;
    double minValue;
    double maxValue;
    float avgDiff;
    float rangeDiff;
    float diff;
    float signedDiff;
};

FilterResultsRef ChannelFilterDiff::processChannelData( const ChannelDataFrameRef & channelData,
                                                        const ChannelDataFrameRef & prevChannelData )
{
    // Channel Data
//    time_t startTime;
//    time_t endTime;
//    long sampleRate;
//    std::string channelName;
//    std::vector<double> data;
//    double minValue;
//    double maxValue;
//    double meanValue;

    // 1) Create DiffFrames from the channel data
    vector<DiffFrame> samples;
    const static double kNumSecPerSampleWindow = 2.0f;//2.0;
    double rangeSum = 0;
    for ( double start = channelData->startTime;
          start < channelData->endTime;
          start += kNumSecPerSampleWindow )
    {
        DiffFrame frame(start, MIN(start+kNumSecPerSampleWindow, channelData->endTime));
        frame.sampleData(channelData->data,
                         channelData->startTime,
                         channelData->endTime);
        samples.push_back(frame);
        rangeSum += frame.valueRange;
    }
    
    int numSamples = samples.size();
    double standardRange = 0;
    if ( numSamples > 0 && rangeSum > 0 )
    {
        standardRange = rangeSum / numSamples;
    }
    
    // 2) Iterate over the frames to create scores
    // TODO: How do we score the first window?
    for ( int i = 0; i < numSamples; ++i )
    {
        DiffFrame frameA(-1,-1);
        if ( i == 0 )
        {
            if ( ! prevChannelData )
            {
                // There is no previous data.
                // Leave the scores blank.
                continue;
            }
            // Create a window for the previous sample
            frameA = DiffFrame(prevChannelData->endTime - kNumSecPerSampleWindow,
                               prevChannelData->endTime);
            frameA.sampleData(prevChannelData->data,
                              prevChannelData->startTime,
                              prevChannelData->endTime);
        }
        else
        {
            frameA = samples[i-1];
        }
        DiffFrame & frameB = samples[i];
        frameB.calculateDiff(frameA,
                             channelData->meanValue,
                             standardRange);
    }
    
    // 3) Add scores to the ChannelFilterDiffResults
    // Diff Results
    //    std::vector<float> scores;
    //    float sampleTimeInterval;
    //    int numSamples;
    
    FilterDiffResults *results = new FilterDiffResults();
    results->sampleTimeInterval = kNumSecPerSampleWindow;
    results->numSamples = numSamples;
    for ( int i = 0; i < numSamples; ++i )
    {
        // NOTE: We can choose to use the "diff" or the "signed diff" depending upon our predilections.
        results->scores.push_back(samples[i].diff);
    }

    return FilterResultsRef(results);
}

std::map<std::string, FilterResultsRef> ChannelFilterDiff::processBundleData( const std::vector<DataChannelRef> & channels,
                                                                              FilteredDataPacket & channelResults,
                                                                              ChannelGroupingRef & bundle,
                                                                              void *userData )
{
    map<string, FilterResultsRef> resultsByCategory;
    map<string, int> numCategoryContributors;
    bool isAll = bundle->name == kRootGroupingName;
    
    if ( isAll )
    {
        resultsByCategory[kRootGroupingName] = FilterDiffResultsRef(new FilterDiffResults());
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
            resultsByCategory[catKey] = FilterDiffResultsRef(new FilterDiffResults());
        }
    }
    
    long numSamples = -1;
    
    // Iterate over channels
    for ( const DataChannelRef & channel : channels )
    {
        // Get the filter values
        FilterDiffResultsRef channelFilterValues = static_pointer_cast<FilterDiffResults>(channelResults.filterResultsByChannel[channel->name][ getFilterKey() ]);
        
        // In a networked world, we can miss channel data
        if ( !channelFilterValues )
        {
            if ( !Config::ShouldUseLiveData() )
            {
                cout << "ERROR: Channel filter data not found\n";
            }
            continue;
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
        
        FilterDiffResultsRef categoryResults = static_pointer_cast<FilterDiffResults>(resultsByCategory[catKey]);
        bool wasEmpty = categoryResults->numSamples <= 0;
        if ( wasEmpty )
        {
            numSamples = channelFilterValues->numSamples;
            categoryResults->numSamples = numSamples;
            numCategoryContributors[catKey] = 0;
        }
        else
        {
            assert(numSamples == channelFilterValues->numSamples);
        }
        
        numCategoryContributors[catKey] += 1;
        
        // Iterate over the scores
        for ( int i = 0; i < categoryResults->numSamples; ++i )
        {
            if ( wasEmpty )
            {
                categoryResults->scores.push_back(0);
                categoryResults->ranks.push_back(0);
            }
            // Add values to the cat results sum
            categoryResults->scores[i] += channelFilterValues->scores[i];
        }
    }
    
    // Kill any results that don't have values.
    // This can occur when we're passing in an arbitrary list of channels.
    vector<string> ignoreCategories;
    for ( auto kvp : resultsByCategory )
    {
        FilterDiffResultsRef categoryResults = static_pointer_cast<FilterDiffResults>(kvp.second);
        if ( categoryResults->scores.size() == 0 )
        {
            ignoreCategories.push_back(kvp.first);
        }
    }
    for ( string & catKey : ignoreCategories )
    {
        resultsByCategory.erase(resultsByCategory.find(catKey));
    }

    vector< pair<string, FilterDiffResultsRef> > sortedResults;

    // Iterate over the categories
    for ( auto kvp : resultsByCategory )
    {
        // Average the values
        string catKey = kvp.first;
        FilterDiffResultsRef categoryResults = static_pointer_cast<FilterDiffResults>(kvp.second);
        
        for ( int i = 0; i < categoryResults->numSamples; ++i )
        {
            double score = categoryResults->scores[i];
            double avgScore = score / numCategoryContributors[catKey]; // categoryResults->numSamples;
            categoryResults->scores[i] = avgScore;
        }
        
        sortedResults.push_back(pair<string, FilterDiffResultsRef>(kvp.first, categoryResults));
    }
    
    // Rank them for every sample in the results.
    // NOTE: This may be a little burdensome.
    for ( int i = 0; i < numSamples; ++i )
    {
        sort(sortedResults.begin(), sortedResults.end(), [&](pair<string, FilterDiffResultsRef> & a,
                                                             pair<string, FilterDiffResultsRef> & b)
        {
            return a.second->scores[i] > b.second->scores[i];
        }
             );
        
        int rank = 0;
        for ( auto kvp : sortedResults )
        {
            FilterDiffResultsRef result = static_pointer_cast<FilterDiffResults>(resultsByCategory[kvp.first]);
            result->ranks[i] = rank;
            rank++;
        }
    }
    
    return resultsByCategory;
}

float ChannelFilterDiff::timelineIntensityForBundleValue( float bundleValue  )
{
    float bundleIntensity = sqrt(bundleValue);
    bundleIntensity = bundleIntensity * bundleIntensity * 3;
    return bundleIntensity;
}

int ChannelFilterDiff::filterIndexFromFilterTime( const FilterResultsRef & results, void *userData )
{
    assert( userData != NULL );
    std::map<std::string, double> *userMap = static_cast<std::map<std::string, double> *>(userData);
    if ( userMap == NULL )
    {
        std::cout << "ERROR: ChannelFilterDiff requires a map<string, double> for user info.\n";
        assert(false);
    }
    
    assert( (*userMap).count("scalarTime") > 0 );
    double scalarTime = (*userMap)["scalarTime"];
    
    FilterDiffResultsRef diffResults = static_pointer_cast< FilterDiffResults >( results );
    return diffResults->scores.size() * scalarTime;
}

float ChannelFilterDiff::magnitudeForIndex( const FilterResultsRef & results,
                                                  int sampleIndex,
                                                  void *userData )
{
    FilterDiffResultsRef diffResults = static_pointer_cast< FilterDiffResults >( results );
    assert(!!diffResults);
    return fabs(diffResults->scores[sampleIndex]);
}

float ChannelFilterDiff::rankForIndex( const FilterResultsRef & results, int sampleIndex, void *userData )
{
    FilterDiffResultsRef diffResults = static_pointer_cast< FilterDiffResults >( results );
    assert(!!diffResults);
    return diffResults->ranks[sampleIndex];
}

float ChannelFilterDiff::connectionIntensityForIndex( const FilterResultsRef & results,
                                                     int sampleIndex,
                                                     void *userData )
{
    const static float kDiffMagThreshold = 0.15f;
    float diffMagnitude = magnitudeForIndex(results, sampleIndex, userData);
    // Weights any connection between this node and others.
    // We'll ignore anything less than kDiffMagThreshold.
    // If the value returned is <= 0, the line isn't drawn.    
    return std::max(0.f, diffMagnitude - kDiffMagThreshold) * (1.2f + kDiffMagThreshold);
}

float ChannelFilterDiff::barIntensityForIndex( const FilterResultsRef & results,
                                               int sampleIndex,
                                               void *userData )
{
    float mag = magnitudeForIndex(results, sampleIndex, userData);
    if ( mag == 0 )
    {
        return 0.45f;
    }
    return 1.0f;
}


