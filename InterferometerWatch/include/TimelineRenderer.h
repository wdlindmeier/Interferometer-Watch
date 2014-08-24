//
//  TimelineRenderer.h
//  LIGOclock
//
//  Created by William Lindmeier on 8/1/14.
//
//

#pragma once

#include "SharedTypes.hpp"
#include "cinder/gl/Fbo.h"
#include "cinder/gl/Vbo.h"
#include "ChannelFilter.h"

/*
 
 TimelineRenderer
 
 This class is responsible for rendering the timeline (left hand circle) which includes the history 
 for the aggregate bundle values and the primary signal (wave around the outside ring). 
 It also manages the "current time" of the data.
 
 */

namespace ligo
{
    class TimelineRenderer
    {
        time_t mDataStartTime;
        time_t mTimelineDuration;
        double mCurrentDataTime;
        double mCurrentPlayheadTime;
        bool mShouldLog;
        
        ci::Vec2f mCenterPosition;
        float mOuterRadius;
        bool mIsPaused;
        ci::gl::Fbo mFBOPrimarySignal;
        ci::gl::Fbo mFBOHistory;
        ci::gl::VboMeshRef mVboMeshPrimarySignal;
        ci::gl::VboMeshRef mVboMeshHistory;
        ci::gl::TextureRef mRadarTexture;
        
        std::string mNavStateRendered;
        
        double mPrevPrimaryMin;
        double mPrevPrimaryMax;
        double mCurPrimaryMin;
        double mCurPrimaryMax;
        double mPrimarySignalRange;
        
        void setupFBOs();
        void setupVBOs();
        
        void scalarTimesWithDataStartTimeDataEndTime( time_t dataStartTime,
                                                     time_t dataEndTime,
                                                     float *_scalarWindowTimeStart,
                                                     float *_scalarWindowTimeEnd,
                                                     float *_scalarDrawTimeEnd );
        
    public:
        
        TimelineRenderer(const ci::Vec2f & centerPosition = ci::Vec2f(0,0),
                         const float outerRadius = 150,
                         time_t duration = 60 * 60,
                         time_t dataStartTime = 0) :
        mDataStartTime(dataStartTime)
        ,mTimelineDuration(duration)
        ,mCenterPosition(centerPosition)
        ,mOuterRadius(outerRadius)
        ,mIsPaused(false)
        ,mPrevPrimaryMin(-1)
        ,mPrevPrimaryMax(-1)
        ,mCurPrimaryMin(-1)
        ,mCurPrimaryMax(-1)
        ,mPrimarySignalRange(0)
        {};
        
        // Any required setup once we have an opengl context
        void setupGL();

        void setCenterPosition( const ci::Vec2f & centerPos );
        ci::Vec2f getCenterPosition();

        void setOuterRadius( float outerRadius );
        time_t getOuterRadius();

        void setDuration( time_t durationTimeline );
        time_t getDuration();
        
        void setDataStartTime( time_t dataStartTime );
        time_t getDataStartTime();
        
        double getPlayheadTime();
        
        bool containsMousePosition( const ci::Vec2f & mousePos );
        void updateClockWithMousePosition( const ci::Vec2f & mousePos );
        
        void setIsPaused( bool isPaused );
        bool getIsPaused();
        
        void setShouldLog( bool shouldLog ){ mShouldLog = shouldLog; }
        
        void update( double currentDataTime );
        void draw();
        void drawPrimarySignal( ChannelDataFrameRef & primarySignal );

        void drawHistory( float selectedFrequency );
    };
}

