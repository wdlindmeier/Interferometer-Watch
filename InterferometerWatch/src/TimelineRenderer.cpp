//
//  TimelineRenderer.cpp
//  LIGOclock
//
//  Created by William Lindmeier on 8/1/14.
//
//

#include <stdio.h>
#include "TimelineRenderer.h"
#include "cinder/gl/gl.h"
#include "cinder/app/AppBasic.h"
#include "CinderHelpers.hpp"
#include "ChannelFilterDiff.h"
#include "ChannelFilterFFT.h"
#include "ChannelStateManager.h"
#include "FilteredDataServer.h"
#include "GPSTime.hpp"
#include "AppConfig.h"
#include "FontBook.hpp"

using namespace ligo;
using namespace std;
using namespace cinder;
using namespace wdl;

#pragma mark - Setup

const static float kSignalFBOHeight = 100.f;
const static float kInnerRadius = 40.f;

void TimelineRenderer::setupFBOs()
{
    gl::Fbo::Format format;
    format.enableDepthBuffer(false);
    format.enableColorBuffer(true);
    format.setMinFilter( GL_LINEAR );
    format.setMagFilter( GL_LINEAR );
    format.setWrapS(GL_CLAMP);
    format.setWrapT(GL_CLAMP);
    format.setColorInternalFormat( GL_RGBA );
    
    // Circumference of a circle:
    float fboWidth = 2 * M_PI * (mOuterRadius + kSignalFBOHeight);
    
    mFBOPrimarySignal = gl::Fbo( fboWidth * Config::ScreenScale(),
                                 kSignalFBOHeight * Config::ScreenScale(),
                                 format );
    mFBOPrimarySignal.bindFramebuffer();
    gl::clear(Color(0,0,0));
    mFBOPrimarySignal.unbindFramebuffer();

    mFBOHistory = gl::Fbo( fboWidth * Config::ScreenScale(),
                           (mOuterRadius - kInnerRadius) * Config::ScreenScale(),
                           format );
    mFBOHistory.bindFramebuffer();
    gl::clear(Color(0,0,0));
    mFBOHistory.unbindFramebuffer();
    
    mRadarTexture = gl::Texture::create(loadImage(ci::app::AppBasic::getResourcePath("radar_trail.png")));
    mRadarTexture->setWrap(GL_CLAMP, GL_CLAMP);
    mRadarTexture->setMinFilter( GL_LINEAR );
    mRadarTexture->setMagFilter( GL_LINEAR );

}

void TimelineRenderer::setupVBOs()
{
    const static int kNumTimelineRingSegments = 360;
    TriMesh2d historyTriMesh;
    TriMesh2d ringTriMesh;
    
    float innerRadius = kInnerRadius;
    float outerRadius = mOuterRadius;
    float innerRingRadius = mOuterRadius;
    float outerRingRadius = mOuterRadius + kSignalFBOHeight;
    
    for( int i = 0; i < kNumTimelineRingSegments + 1; ++i )
    {
        // min @ 0.999 so the texture doesn't wrap around at the edge
        float scalarA = std::min(float(i) / kNumTimelineRingSegments, 0.99999f);
        float scalarB = std::min(float(i+1) / kNumTimelineRingSegments, 0.99999f);

        float segmentRadsA = toRadians((scalarA * 360.f) - 90.f);
        float segmentRadsB = toRadians((scalarB * 360.f) - 90.f);
        
        float segmentXA = cos(segmentRadsA);
        float segmentYA = sin(segmentRadsA);

        float segmentXB = cos(segmentRadsB);
        float segmentYB = sin(segmentRadsB);

        Vec2f unitPosA(segmentXA, segmentYA);
        Vec2f unitPosB(segmentXB, segmentYB);
        
        // Top Heavy Triangle
        // Top A
        historyTriMesh.appendVertex(unitPosA * outerRadius);
        historyTriMesh.appendTexCoord( Vec2f(scalarA, 1) ); // top
        ringTriMesh.appendVertex(unitPosA * outerRingRadius);
        ringTriMesh.appendTexCoord( Vec2f(scalarA, 1) ); // top

        // Bottom A
        historyTriMesh.appendVertex(unitPosA * innerRadius);
        historyTriMesh.appendTexCoord( Vec2f(scalarA, 0) ); // bottom
        ringTriMesh.appendVertex(unitPosA * innerRingRadius);
        ringTriMesh.appendTexCoord( Vec2f(scalarA, 0) ); // bottom
        
        // Top B
        historyTriMesh.appendVertex(unitPosB * outerRadius);
        historyTriMesh.appendTexCoord( Vec2f(scalarB, 1) ); // top
        ringTriMesh.appendVertex(unitPosB * outerRingRadius);
        ringTriMesh.appendTexCoord( Vec2f(scalarB, 1) ); // top

        // Bottom Heavy Triangle
        // Top B
        historyTriMesh.appendVertex(unitPosB * outerRadius);
        historyTriMesh.appendTexCoord( Vec2f(scalarB, 1) ); // top
        ringTriMesh.appendVertex(unitPosB * outerRingRadius);
        ringTriMesh.appendTexCoord( Vec2f(scalarB, 1) ); // top
        
        // Bottom A
        historyTriMesh.appendVertex(unitPosA * innerRadius);
        historyTriMesh.appendTexCoord( Vec2f(scalarA, 0) ); // bottom
        ringTriMesh.appendVertex(unitPosA * innerRingRadius);
        ringTriMesh.appendTexCoord( Vec2f(scalarA, 0) ); // bottom
        
        // Bottom B
        historyTriMesh.appendVertex(unitPosB * innerRadius);
        historyTriMesh.appendTexCoord( Vec2f(scalarB, 0) ); // bottom
        ringTriMesh.appendVertex(unitPosB * innerRingRadius);
        ringTriMesh.appendTexCoord( Vec2f(scalarB, 0) ); // bottom
    }
    
    gl::VboMesh::Layout layout;
    layout.setStaticIndices();
    layout.setStaticPositions();
    layout.setStaticTexCoords2d();

    mVboMeshPrimarySignal = gl::VboMesh::create(ringTriMesh, layout);
    mVboMeshHistory = gl::VboMesh::create(historyTriMesh, layout);
}

void TimelineRenderer::setupGL()
{
    setupFBOs();
    setupVBOs();
}

#pragma mark - Accessors 

void TimelineRenderer::setCenterPosition( const ci::Vec2f & centerPos )
{
    mCenterPosition = centerPos;
}

ci::Vec2f TimelineRenderer::getCenterPosition()
{
    return mCenterPosition;
}

void TimelineRenderer::setOuterRadius( float outerRadius )
{
    mOuterRadius = outerRadius;
    // We don't want to change our radius after we've created the VBO
    assert( !mVboMeshPrimarySignal || mVboMeshPrimarySignal->getNumIndices() == 0 );
}

time_t TimelineRenderer::getOuterRadius()
{
    return mOuterRadius;
}

void TimelineRenderer::setDuration( time_t durationTimeline )
{
    mTimelineDuration = durationTimeline;
}

time_t TimelineRenderer::getDuration()
{
    return mTimelineDuration;
}

void TimelineRenderer::setDataStartTime( time_t dataStartTime )
{
    mDataStartTime = dataStartTime;
}

time_t TimelineRenderer::getDataStartTime()
{
    return mDataStartTime;
}

double TimelineRenderer::getPlayheadTime()
{
    return mCurrentPlayheadTime;
}

void TimelineRenderer::setIsPaused(bool isPaused)
{
    mIsPaused = isPaused;
}

bool TimelineRenderer::getIsPaused()
{
    return mIsPaused;
}

#pragma mark - Mouse Input

bool TimelineRenderer::containsMousePosition( const ci::Vec2f & mousePos )
{
    float distFromCenter = mousePos.distance(mCenterPosition);
    return distFromCenter <= mOuterRadius + kSignalFBOHeight;
}

void TimelineRenderer::updateClockWithMousePosition( const ci::Vec2f & mousePos )
{
    mIsPaused = true;
    
    Vec2f direction(mousePos - mCenterPosition);
    direction.normalize();
    float dirRads = atan2(direction.y,direction.x);
    if ( dirRads < 0 )
    {
        dirRads = (M_PI * 2) + dirRads;
    }
    
    float scalarPlayheadTime = fmod((dirRads / (M_PI * 2) + 0.25), 1.0);
    
    double relativeClockTime = fmod((mCurrentDataTime - mDataStartTime), mTimelineDuration);
    float scalarClockTime = relativeClockTime / mTimelineDuration;
    
    float scalarOffset = scalarPlayheadTime - scalarClockTime;
    bool hasCompletedFullRotation = (mCurrentDataTime - mDataStartTime) >= mTimelineDuration;

    double secOffset = scalarOffset * mTimelineDuration;

    // draggedToTime
    mCurrentPlayheadTime = mCurrentDataTime + secOffset;
    
    // Wrap around if we've completed a full rotation
    if ( mCurrentPlayheadTime > mCurrentDataTime && hasCompletedFullRotation )
    {
        mCurrentPlayheadTime = mCurrentDataTime -= (mTimelineDuration - secOffset);
    }
    
    // Check if it's outside of the collected data bounds
    if ( mCurrentPlayheadTime > mCurrentDataTime ||
         mCurrentPlayheadTime < mDataStartTime )
    {
        // Check which one it's closer to.
        // It can only snap to the "start" time if the current time is within it's first loop.
        if ( fabs(fmod(1.0 - scalarPlayheadTime, 1.0)) < fabs(scalarOffset) )
        {
            mCurrentPlayheadTime = mDataStartTime;
        }
        else
        {
            mCurrentPlayheadTime = mCurrentDataTime;
        }
    }
}

#pragma mark - Update

void TimelineRenderer::update( double currentDataTime )
{
    mCurrentDataTime = currentDataTime;
    if ( !mIsPaused )
    {
        mCurrentPlayheadTime = mCurrentDataTime;
    }
}

#pragma mark - Draw

void TimelineRenderer::scalarTimesWithDataStartTimeDataEndTime( time_t dataStartTime,
                                                                time_t dataEndTime,
                                                                float *_scalarWindowTimeStart,
                                                                float *_scalarWindowTimeEnd,
                                                                float *_scalarDrawTimeEnd )
{
    float scalarWindowTimeStart = double(dataStartTime - mDataStartTime) / double(mTimelineDuration);
    float scalarWindowTimeEnd = double(dataEndTime - mDataStartTime) / double(mTimelineDuration);
    float scalarDrawTimeEnd = double(mCurrentDataTime - mDataStartTime) / double(mTimelineDuration);
    
    // Wrap
    scalarWindowTimeStart = fmod(scalarWindowTimeStart, 1.0f);
    scalarWindowTimeEnd = fmod(scalarWindowTimeEnd, 1.0f);
    if ( scalarWindowTimeEnd < scalarWindowTimeStart )
    {
        scalarWindowTimeEnd = 1.0f + scalarWindowTimeEnd;
    }
    scalarDrawTimeEnd = fmod(scalarDrawTimeEnd, 1.0f);
    if ( scalarDrawTimeEnd < scalarWindowTimeStart )
    {
        scalarDrawTimeEnd = 1.0f + scalarDrawTimeEnd;
    }
    
    *_scalarWindowTimeStart = scalarWindowTimeStart;
    *_scalarWindowTimeEnd = scalarWindowTimeEnd;
    *_scalarDrawTimeEnd = scalarDrawTimeEnd;
}

// Similar to Moment Renderer's "frameRenderKey", each unique navigation state
// has a unique key so we know if we should redraw the entire clock,
// or if we can just draw this data window.
static string NavigationStateKey()
{
    string navBundingKey;
    for ( ChannelGroupingRef & grouping : GroupingNavigation::getInstance().getGroupingStack() )
    {
        navBundingKey += grouping->name;
        navBundingKey += "_";
    }
    ChannelFilterRef & currentFilter = ChannelStateManager::getInstance().getCurrentFilter();
    navBundingKey += currentFilter->getFilterKey();
    return navBundingKey;
}

void TimelineRenderer::drawHistory( float selectedFrequency )
{
    ChannelFilterRef & currentFilter = ChannelStateManager::getInstance().getCurrentFilter();
    std::deque<ChannelGroupingRef> navStack = GroupingNavigation::getInstance().getGroupingStack();
    
    ChannelGroupingRef currentGrouping = navStack.back();

    string navStateKey = NavigationStateKey();
    if ( currentFilter->getFilterKey() == kChannelFilterKeyFFT )
    {
        // FFT render also needs frequency range keys
        navStateKey += to_string((int)selectedFrequency);
    }

    bool shouldRedrawEntireClock = false;
    if ( mNavStateRendered != navStateKey )
    {
        shouldRedrawEntireClock = true;
#if DEBUG
        cout << "NEW NAV STATE. Redraw entire clock.\n";
#endif
        mNavStateRendered = navStateKey;
    }

    map<string, CategoryRef> drawCategories = currentGrouping->categories;
    if ( !currentGrouping->isBundle )
    {
        // 0 == Nothing, 1 == All, 2 == Bundle
        if ( navStack.size() > 2 )
        {
            currentGrouping = navStack[navStack.size() - 2];
            assert( currentGrouping->isBundle );
        }
        else
        {
            // This is the root node
            assert(currentGrouping->name == kRootGroupingName);
            
            // Just give it a stub category.
            // THIS FEELS HACKY...
            CategoryRef stubCategory(new Category());
            stubCategory->color = ColorAf(1,1,1,1);
            drawCategories[currentGrouping->name] = stubCategory;
        }
    }
    
    // Get the time
    // This needs to happen after updateDataForPlayheadTimestamp
   DrawToFBO(mFBOHistory, [&]( const Vec2f & fboSize )
    {
        time_t nowTime = mCurrentDataTime;
        time_t clockStartTime = std::max(nowTime - mTimelineDuration, mDataStartTime);
        double nextDrawTime = nowTime;
        if ( shouldRedrawEntireClock )
        {
            gl::clear(Color(0,0,0));
            nextDrawTime = clockStartTime;
        }
        int numPacketsDrawn = 0;

        // Start with the clockStartTime and work our way up
        while ( nextDrawTime <= nowTime )
        {
            const FilteredDataPacketRef & dataPacket = FilteredDataServer::getInstance().getDataForTimestamp( nextDrawTime );

            if ( !dataPacket )
            {
                // Iterate by 1 second until we find some data.
                nextDrawTime += 1;
                continue;
            }
            
            numPacketsDrawn++;
            
            time_t dataDuration = dataPacket->parsedData.dataGPSEndTime -
                                  dataPacket->parsedData.dataGPSStartTime;
            
            nextDrawTime += dataDuration;

            time_t dataStartTime = dataPacket->parsedData.dataGPSStartTime;
            time_t dataEndTime = dataPacket->parsedData.dataGPSEndTime;
            float currentTimeDataWindowScalar = double( mCurrentDataTime - dataStartTime ) /
                                                double( dataEndTime - dataStartTime );
            // It's possible that the current time falls after the data packet
            currentTimeDataWindowScalar = std::min(currentTimeDataWindowScalar, 1.0f);

            float scalarWindowTimeStart;
            float scalarWindowTimeEnd;
            float scalarDrawTimeEnd;
            
            scalarTimesWithDataStartTimeDataEndTime( dataStartTime,
                                                     dataEndTime,
                                                     &scalarWindowTimeStart,
                                                     &scalarWindowTimeEnd,
                                                     &scalarDrawTimeEnd );
            
            float windowStartX = fboSize.x * scalarWindowTimeStart;
            float windowEndX = fboSize.x * scalarWindowTimeEnd;
            float windowRangeX = windowEndX - windowStartX;
            float drawEndX = fboSize.x * scalarDrawTimeEnd;
            
            // Draw a black rect under the signal to clobber the previous data
            gl::color(0, 0, 0);
            gl::drawSolidRect(Rectf(windowStartX, 0,
                                    drawEndX, fboSize.y));

            map<string, double> filterUserInfo;
            // This is only necessary for the FFT filter, but we'll pass it along regardless.
            filterUserInfo["freqHz"] = selectedFrequency;

            // Check if we've got cached values for this nav-state in the packet.
            std::map<std::string, FilterResultsRef> categoryFilterAggregates;
            if ( dataPacket->filterResultsByNavBundling.count(navStateKey) )
            {
                categoryFilterAggregates = dataPacket->filterResultsByNavBundling[navStateKey];
            }
            else
            {
                // If we don't, lazy-load the aggregate values.
                // And cache them.
                std::vector<DataChannelRef> channels = ChannelStateManager::getInstance().getDisplayChannels();
                std::map<std::string, FilterResultsRef> uncachedCategoryFilterResults =
                categoryFilterAggregates = currentFilter->processBundleData( channels,
                                                                             *dataPacket,
                                                                             currentGrouping,
                                                                             &filterUserInfo );
                dataPacket->filterResultsByNavBundling[navStateKey] = categoryFilterAggregates;
            }
            
            int numCategories = categoryFilterAggregates.size();

            for ( auto kvp : drawCategories )
            {
                string categoryKey = KeyForBundleCategory(currentGrouping, kvp.second);

                FilterResultsRef filterData = categoryFilterAggregates[categoryKey];

                if ( !filterData )
                {
                    continue;
                }

                long numData = filterData->numSamples;
                
                for ( long i = 0; i < numData; ++i )
                {
                    float scalarPositionI = i / double(numData);
                    float scalarPositionIPlusOne = std::min(float((i+1) / double(numData)), currentTimeDataWindowScalar);
                    
                    // Don't draw past the current time
                    if ( scalarPositionI > currentTimeDataWindowScalar )
                    {
                        continue;
                    }
                    
                    float categoryMagnitude = currentFilter->magnitudeForIndex(filterData, i, &filterUserInfo);
                    int categoryRank = currentFilter->rankForIndex( filterData, i, &filterUserInfo );
#if DEBUG
                    // This could be anywhere. It basically just activates the breakpoint on a keypress.
                    if ( mShouldLog )
                    {
                        FilterFFTResultsRef filterRef = static_pointer_cast<FilterFFTResults>(filterData);
                        cout << " PAUSE\n";
                    }
#endif
                    // If this is the only category, extend across the entire circle
                    float scalarYBegin = 0;
                    float scalarYEnd = 1;
                    // Otherwise, sort by rank
                    if ( numCategories > 1 )
                    {
                        assert( categoryRank < numCategories );
                        scalarYBegin = float(categoryRank) / numCategories;
                        scalarYEnd = scalarYBegin + (1.0 / numCategories);
                    }
                    
                    float drawY0 = scalarYBegin * fboSize.y;
                    float drawY1 = scalarYEnd * fboSize.y;
                    
                    float drawX0 = windowStartX + (windowRangeX * scalarPositionI);
                    float drawX1 = windowStartX + (windowRangeX * scalarPositionIPlusOne);
                    
                    ColorAf catColor = kvp.second->color;
                    
                    float alpha = currentFilter->timelineIntensityForBundleValue( categoryMagnitude );
                    catColor.a = alpha;

                    gl::color(catColor);
                    gl::drawSolidRect(Rectf(drawX0, drawY0, drawX1, drawY1));
                }
            }
        }
    });
}

void TimelineRenderer::drawPrimarySignal( ChannelDataFrameRef & primarySignal )
{
    DrawToFBO(mFBOPrimarySignal, [&]( const Vec2f & fboSize )
    {
        long numData = primarySignal->data.size();
        
        float currentTimeDataWindowScalar = double( mCurrentDataTime - primarySignal->startTime ) /
                                            double( primarySignal->endTime - primarySignal->startTime );

        // Allow the range to drift as the signal drifts
        if ( mCurPrimaryMin == -1 && mCurPrimaryMax == -1 )
        {
            // Initial values
            mCurPrimaryMin = primarySignal->minValue;
            mCurPrimaryMax = primarySignal->maxValue;
            mPrevPrimaryMin = mCurPrimaryMin;
            mPrevPrimaryMax = mCurPrimaryMax;
        }
        else if ( mCurPrimaryMin != primarySignal->minValue )
        {
            // The values have changed. Swap the prev values and cur values.
            mPrevPrimaryMin = mCurPrimaryMin;
            mPrevPrimaryMax = mCurPrimaryMax;
            mCurPrimaryMin = primarySignal->minValue;
            mCurPrimaryMax = primarySignal->maxValue;
        }
        else
        {
            assert( mCurPrimaryMin != -1 || mPrevPrimaryMax != -1 );
        }

        float scalarWindowTimeStart;
        float scalarWindowTimeEnd;
        float scalarDrawTimeEnd;
        
        scalarTimesWithDataStartTimeDataEndTime( primarySignal->startTime,
                                                 primarySignal->endTime,
                                                 &scalarWindowTimeStart,
                                                 &scalarWindowTimeEnd,
                                                 &scalarDrawTimeEnd );

        float windowStartX = fboSize.x * scalarWindowTimeStart;
        float windowEndX = fboSize.x * scalarWindowTimeEnd;
        float windowRangeX = windowEndX - windowStartX;
        float drawEndX = fboSize.x * scalarDrawTimeEnd;
        
        // Draw a black rect under the signal to clobber the previous data
        gl::color(0, 0, 0);
        gl::drawSolidRect(Rectf(windowStartX, 0,
                                drawEndX, fboSize.y));
        
        gl::color(1, 1, 1, 0.75);
        gl::lineWidth(1);
                  
        Vec2f prevPos;
        for ( long i = 0; i < numData; ++i )
        {
            float scalarPosition = i / double(numData);

            // Don't draw past the current time
            if ( scalarPosition > currentTimeDataWindowScalar )
            {
                continue;
            }
            
            // Constantly adjusting the min/max range in case the
            // overall signal is drifting in one way or the other.
            double kPrimarySignalMin = (mPrevPrimaryMin * (1.0-scalarPosition)) +
                                       (mCurPrimaryMin * scalarPosition);
            double kPrimarySignalMax = (mPrevPrimaryMax * (1.0-scalarPosition)) +
                                       (mCurPrimaryMax * scalarPosition);
            double kPrimarySignalMid = (kPrimarySignalMax + kPrimarySignalMin) / 2.0;
            // The range is averaged over time so the signal generally takes up the same amount of real-estate.
            // HOWEVER, this creates a constantly mutating range. In extreme circumstances, the signal
            // line can actually morph over time.
            // TODO: Find a better way to dynamically pick a range that doesn't cause distortion.
            // The range needs to adapt to the values of the primary signal.
            double range = (kPrimarySignalMax - kPrimarySignalMin);
            if ( mPrimarySignalRange == 0 )
            {
                mPrimarySignalRange = range;
            }
            // Give the range a long decay so it slowly adjusts to current range of the signal.
            mPrimarySignalRange = ((mPrimarySignalRange * 9999) + range) / 10000.f;
            
            // Give it a minimum range so small values dont look big.
            // and a maximum value so large values dont look small.
            kPrimarySignalMin = kPrimarySignalMid - ( mPrimarySignalRange * 0.5 );
            kPrimarySignalMax = kPrimarySignalMid + ( mPrimarySignalRange * 0.5 );
            
            double val = primarySignal->data[i];

            // This centers the value around the 0 line
            float scalarValue = (kPrimarySignalMid - val) / mPrimarySignalRange;
            float dataY = (fboSize.y * 0.5) + ((fboSize.y * 0.5) * scalarValue);
            
            float dataX = windowStartX + (windowRangeX * scalarPosition);
            Vec2f dataPos(dataX, dataY);
            
            // Only draw if it's within our draw window
            if ( i > 0 )
            {
                gl::drawLine(prevPos, dataPos);
            }
            prevPos = dataPos;
        }
    });
}

void TimelineRenderer::draw()
{
    gl::lineWidth(1);
    
    // Draw a ring with a radian playhead
    gl::pushMatrices();
    gl::translate(mCenterPosition);
    
    // Draw the primary signal
    gl::color(1,1,1);
    gl::enableAdditiveBlending();
    mFBOPrimarySignal.getTexture().setFlipped(true);
    mFBOPrimarySignal.getTexture().enableAndBind();
    gl::draw( mVboMeshPrimarySignal );
    mFBOPrimarySignal.getTexture().unbind();
    gl::enableAlphaBlending();
    
    // Draw this history
    mFBOHistory.getTexture().setFlipped(true);
    mFBOHistory.getTexture().enableAndBind();
    gl::draw( mVboMeshHistory );
    mFBOHistory.getTexture().unbind();

    gl::lineWidth(2);
    
    // Draw the arm
    // This is the CURRENT data time—it will always update with the clock.
    float scalarNowTime = fmod(fmod((mCurrentDataTime - mDataStartTime), mTimelineDuration) /
                               double(mTimelineDuration), 1.0f);
    float nowRads = toRadians((scalarNowTime * 360.f) - 90.f);
    // "Then" is the trailing end of the radar shadow
    float thenRads = toRadians(((scalarNowTime + 0.05) * 360.f) - 90.f);
    
    // Draw the hand-trail
    mRadarTexture->enableAndBind();
    gl::color(1, 1, 1, 0.75);

    gl::begin(GL_TRIANGLE_STRIP);
    const static int kSegmentsPerShadow = 20;
    float outerExtentRadius = mOuterRadius + kSignalFBOHeight;
    
    // TODO: This could be a VBO, which may slightly improve the framerate.
    float segmentRads = (thenRads - nowRads) / kSegmentsPerShadow;
    for ( int s = 0; s < kSegmentsPerShadow + 1; ++s )
    {
        float rad = nowRads + (segmentRads * s);
        float segXOuter = cos(rad) * outerExtentRadius;
        float segYOuter = sin(rad) * outerExtentRadius;
        float scalarX = s / float(kSegmentsPerShadow);
        gl::texCoord(scalarX, 1);
        gl::vertex(Vec2f(segXOuter, segYOuter));
        
        gl::texCoord(scalarX, 0);
        gl::vertex(Vec2f(0, 0));
    }
    gl::end();
    mRadarTexture->unbind();
    
    // Draw the hand
    if ( mIsPaused )
    {
        gl::color(1, 1, 1, 0.5);
    }
    else
    {
        gl::color(1, 1, 1, 1);
    }
    float curTimeX0 = cos(nowRads) * kInnerRadius;
    float curTimeY0 = sin(nowRads) * kInnerRadius;
    float curTimeX1 = cos(nowRads) * outerExtentRadius;
    float curTimeY1 = sin(nowRads) * outerExtentRadius;
    gl::drawLine(Vec2f(curTimeX0,curTimeY0),
                 Vec2f(curTimeX1, curTimeY1));
    
    // This is the DRAGGED playhead
    if ( mIsPaused )
    {
        float scalarPlayheadTime = fmod(fmod((mCurrentPlayheadTime - mDataStartTime), mTimelineDuration) /
                                        double(mTimelineDuration), 1.0f);
        float playheadRads = toRadians((scalarPlayheadTime * 360.f) - 90.f);
        gl::color(1, 0, 0);

        float playheadX0 = cos(playheadRads) * kInnerRadius;
        float playheadY0 = sin(playheadRads) * kInnerRadius;
        float playheadX1 = cos(playheadRads) * outerExtentRadius;
        float playheadY1 = sin(playheadRads) * outerExtentRadius;
        
        gl::drawLine( Vec2f(playheadX0, playheadY0),
                      Vec2f(playheadX1, playheadY1) );
    }

    // Draw the duration
    gl::color(0.7,0.7,0.7);
    
    gl::TextureFontRef labelFont;
    
    if ( Config::ShouldUseRetinaDisplay() )
    {
        labelFont = FontBook::getFont("book");
    }
    else
    {
        labelFont = FontBook::getFont("medium");
    }

    string durationLabel = to_string(mTimelineDuration) + " sec";
    ligo::drawCenteredString( to_string(mTimelineDuration) + " sec",
                              Vec2f(0, 8),
                              0.2,
                              labelFont );
    
    if ( mIsPaused )
    {
        gl::color(1,0,0);
    }
    else
    {
        gl::color(1,1,1);
    }
    string playheadTimeLabel = ligo::gpsTimeToString( mCurrentPlayheadTime, "%H:%M:%S" );
    ligo::drawCenteredString(playheadTimeLabel,
                             Vec2f(0, -8),
                             0.25,
                             FontBook::getFont("bold"));
    
    // Draw the rings
    gl::color(1, 1, 1);
    gl::drawStrokedCircle( Vec2f(0, 0), kInnerRadius );

    gl::popMatrices();
    
    mShouldLog = false;
}
