//
//  ChannelFilter.cpp
//  LIGOclock
//
//  Created by William Lindmeier on 7/25/14.
//
//

#include "ChannelFilter.h"

using namespace ligo;

float ChannelFilter::magnitudeForTime( const FilterResultsRef & results,
                                          void *userData )
{
    int filterIndex = filterIndexFromFilterTime(results, userData);
    return magnitudeForIndex( results, filterIndex, userData);
}

float ChannelFilter::rankForTime( const FilterResultsRef & results, void *userData )
{
    int filterIndex = filterIndexFromFilterTime(results, userData);
    return rankForIndex( results, filterIndex, userData);
}

float ChannelFilter::connectionIntensityForTime( const FilterResultsRef & results,
                                                    void *userData )
{
    int filterIndex = filterIndexFromFilterTime(results, userData);
    return connectionIntensityForIndex(results, filterIndex, userData);
}

float ChannelFilter::barIntensityForTime( const FilterResultsRef & results,
                                             void *userData )
{
    int filterIndex = filterIndexFromFilterTime(results, userData);
    return barIntensityForIndex( results, filterIndex, userData);
}
