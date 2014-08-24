//
//  SampleWindow.h
//  LIGOFrames
//
//  Created by William Lindmeier on 7/10/14.
//
//

#ifndef LIGOFrames_SampleWindow_h
#define LIGOFrames_SampleWindow_h

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include "cinder/Cinder.h"

struct SampleWindow
{
    SampleWindow(double gpsStartTime, double gpsEndTime) :
    startTime(gpsStartTime)
    ,endTime(gpsEndTime)
    ,minValue(-1)
    ,maxValue(-1)
    ,avgValue(-1)
    ,valueRange(-1)
    ,avgDiff(0)
    ,rangeDiff(0)
    ,diff(0)
    ,signedDiff(0)
    {}
    
    void sampleData(std::vector<double> & dataValues,
                    double dataGPSStartTime,
                    double dataGPSEndTime)
    {
        float scalarStart = MAX(0, (startTime - dataGPSStartTime) / (dataGPSEndTime - dataGPSStartTime));
        float scalarEnd = MIN(1.0, (endTime - dataGPSStartTime) / (dataGPSEndTime - dataGPSStartTime));
        long dataIdxStart = dataValues.size() * scalarStart;
        long dataIdxEnd = MIN(ceil(dataValues.size() * scalarEnd), dataValues.size());
        
        assert( dataIdxEnd - dataIdxStart > 0 );
        
        for ( long i = dataIdxStart; i < dataIdxEnd; ++i )
        {
            double val = dataValues[i];
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
        avgValue = (maxValue + minValue) / 2.0;
        valueRange = maxValue - minValue;
    }
    
    void calculateDiff( std::shared_ptr<SampleWindow> otherWindow,
                        double standardAverage,
                        double standardRange )
    {
        assert( minValue != -1 || maxValue != -1 || avgValue != -1 );
        rangeDiff = (otherWindow->valueRange - valueRange) / standardRange;
        avgDiff = ((avgValue - standardAverage) - (otherWindow->avgValue - standardAverage)) / standardRange;
        diff = (fabs(rangeDiff) + fabs(avgDiff)) * 0.5;
        signedDiff = (rangeDiff + avgDiff) * 0.5f;
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

typedef std::shared_ptr<SampleWindow> SampleWindowRef;

#endif
