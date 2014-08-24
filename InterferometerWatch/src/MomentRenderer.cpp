//
//  MomentRenderer.cpp
//  LIGOclock
//
//  Created by William Lindmeier on 7/31/14.
//
//

#include <stdio.h>
#include "ChannelFilterFFT.h"
#include "MomentRenderer.h"
#include "cinder/Cinder.h"
#include "cinder/gl/gl.h"
#include "CinderHelpers.hpp"
#include "ChannelStateManager.h"
#include "GroupingNavigation.h"
#include "AppConfig.h"
#include "cinder/gl/Vbo.h"
#include "FontBook.hpp"

using namespace std;
using namespace ligo;
using namespace ci;
using namespace wdl;

const static float kUnitLength = 100.f;

void MomentRenderer::mouseUp( const ci::Vec2f & mousePos )
{
    // Nothing to see here
}

void MomentRenderer::setCenterPosition(const Vec2f & centerPosition )
{
    mCenterPosition = centerPosition;
}

const ci::Vec2f MomentRenderer::getCenterPosition()
{
    return mCenterPosition;
}

void MomentRenderer::setInnerRadius( const float radius )
{
    mInnerRadius = radius;
}

const float MomentRenderer::getInnerRadius()
{
    return mInnerRadius;
}

void MomentRenderer::setupGL()
{
    gl::Fbo::Format format;
    format.enableDepthBuffer(false);
    format.enableColorBuffer(true);
    format.setMinFilter( GL_LINEAR );
    format.setMagFilter( GL_LINEAR );
    format.setWrapS(GL_CLAMP);
    format.setWrapT(GL_CLAMP);
    format.setSamples(8);
    format.setColorInternalFormat( GL_RGBA );

    Vec2i screenSize = ci::app::getWindowSize();
    mFBOChannels = gl::Fbo( screenSize.x * Config::ScreenScale(),
                            screenSize.y * Config::ScreenScale(),
                            format );
    
    mFBOChannels.bindFramebuffer();
    gl::clear(Color(0,0,0));
    mFBOChannels.unbindFramebuffer();
}

bool MomentRenderer::centerContainsMousePosition( const ci::Vec2f & mousePos )
{
    float dist = mousePos.distance(mCenterPosition);
    return dist < mInnerRadius;
}

bool MomentRenderer::ringContainsMousePosition( const ci::Vec2f & mousePos )
{
    float dist = mousePos.distance(mCenterPosition);
    return dist > mInnerRadius && dist <= mInnerRadius + kUnitLength;
}

void MomentRenderer::deselectChannel()
{
    mSelectedChannelIndex = -1;
}

void MomentRenderer::selectChannelAtMousePosition( const ci::Vec2f & mousePos )
{
    int numChannels = mFrameChannels.size();
    if ( numChannels == 0 )
    {
        return;
    }
    Vec2f direction(mousePos - mCenterPosition);
    direction.normalize();
    float dirRads = atan2(direction.y,direction.x);
    if ( dirRads < 0 )
    {
        dirRads = (M_PI * 2) + dirRads;
    }
    float scalarPlayheadTime = fmod((dirRads / (M_PI * 2) + 0.25), 1.0);
    int selectedIndex = (int)(round(numChannels * scalarPlayheadTime)) % numChannels;
    if ( mFrameChannels.size() > selectedIndex )
    {
        // We've got the scalar position. Use that to get a channel index.
        //mSelectedChannel = mFrameChannels[selectedIndex];
        mSelectedChannelIndex = selectedIndex;
    }
    else
    {
        deselectChannel();
    }
}

struct ChannelRingConnectionPoint
{
    Vec3f position;
    float connectionStrength;
    ColorAf color;
    
    ChannelRingConnectionPoint( Vec3f position, float connectionStrength, ColorAf color ) :
    position(position)
    ,connectionStrength(connectionStrength)
    ,color(color)
    {}
};

void MomentRenderer::draw( FilteredDataPacketRef & dataPacket,
                           bool isPaused,
                           map<string, double> *filterUserInfo )
{
    if ( !dataPacket )
    {
        return;
    }
    
    ChannelFilterRef & currentFilter = ChannelStateManager::getInstance().getCurrentFilter();
    
    mFrameChannels = ChannelStateManager::getInstance().getDisplayChannels();
    ChannelGroupingRef currentGrouping = GroupingNavigation::getInstance().getGroupingStack().back();
    string groupingName = currentGrouping->name;
    string bundleName = groupingName;
    
    if ( !currentGrouping->isBundle )
    {
        ChannelGroupingRef lastBundle = GroupingNavigation::getInstance().getLastBundle();
        if ( lastBundle )
        {
            bundleName = lastBundle->name;
        }
    }

    int numChannels = mFrameChannels.size();

    if ( mShouldSort )
    {
        // Sort the channels by magnitude.
        sort(mFrameChannels.begin(), mFrameChannels.end(),
             [&](DataChannelRef & channelA, DataChannelRef & channelB)
        {
            std::map<std::string, FilterResultsRef > filterResultsA = dataPacket->filterResultsByChannel[channelA->name];
            FilterResultsRef filterDataA = filterResultsA[currentFilter->getFilterKey()];
            float lineMagnitudeA = currentFilter->magnitudeForTime(filterDataA, filterUserInfo);
            
            std::map<std::string, FilterResultsRef > filterResultsB = dataPacket->filterResultsByChannel[channelB->name];
            FilterResultsRef filterDataB = filterResultsB[currentFilter->getFilterKey()];
            float lineMagnitudeB = currentFilter->magnitudeForTime(filterDataB, filterUserInfo);
            
            return lineMagnitudeA > lineMagnitudeB;
        });
    }
    
    float radInterval = toRadians(360.0f / numChannels);

    // This is a slightly Hacky way of getting the current data index
    // so we can tell if we've already drawn this yet.
    std::map<std::string, FilterResultsRef > tmpFilterResults = dataPacket->filterResultsByChannel[Config::TimelineFFTChannelName()];
    FilterResultsRef tmpFilterData = tmpFilterResults[currentFilter->getFilterKey()];
    if ( !tmpFilterData )
    {
        // We don't have a FFT channel.
        // That's a primary assumption.
        cout << "WARN: Couldn't find the FFT result channel\n";
        return;
    }
    
    int dataIndex = currentFilter->filterIndexFromFilterTime( tmpFilterData, filterUserInfo );
    
    // Render keys are unique combinations of render variables so we know if we have
    // to re-draw the data or not. Anything that might change the output should be included.
    string frameRenderKey = to_string(dataIndex) +
                            to_string(dataPacket->parsedData.dataGPSStartTime) +
                            currentFilter->getFilterKey() +
                            groupingName;
    
    if ( currentFilter->getFilterKey() == kChannelFilterKeyFFT )
    {
        // FFT render also needs frequency range keys
        frameRenderKey += to_string((int)(*filterUserInfo)["freqHz"]);
    }
    
    if ( frameRenderKey != mLastFrameRenderKey )
    {
        mLastFrameRenderKey = frameRenderKey;

        DrawToFBO(mFBOChannels, [&]( const Vec2f & fboSize )
        {
            gl::enableAlphaBlending();
            
            gl::clear( Color(0, 0.0, 0) );
            
            gl::pushMatrices();
            
            gl::translate(mCenterPosition * Config::ScreenScale());

            vector< ChannelRingConnectionPoint > channelRingValues;

            vector<Vec3f> linePositions;
            vector<ColorAf> lineColors;
            
            for ( int i = 0; i < numChannels; ++i )
            {
                DataChannelRef channel = mFrameChannels[i];

                CategoryRef channelCategory = channel->categoryMemberships[bundleName];
                ColorAf categoryColor(1,1,1,1);
                if ( channelCategory )
                {
                    categoryColor = channelCategory->color;
                }
                
                std::map<std::string, FilterResultsRef > filterResults = dataPacket->filterResultsByChannel[channel->name];
                FilterResultsRef filterData = filterResults[currentFilter->getFilterKey()];
                if ( !filterData )
                {
                    if ( !Config::ShouldUseLiveData() )
                    {
                        cout << "ERROR: No channel filter results\n";
                    }
                    continue;
                }
                float lineMagnitude = currentFilter->magnitudeForTime(filterData, filterUserInfo);
#if DEBUG
                assert( !isnan(lineMagnitude) );
                assert( !isinf(lineMagnitude) );
#endif
                float intensity = currentFilter->barIntensityForTime(filterData, filterUserInfo);
                categoryColor.a = std::max(intensity, 0.1f);
                ColorAf lineColor = categoryColor;
                
                float x = cos((i * radInterval) - (M_PI * 0.5f));
                float y = sin((i * radInterval) - (M_PI * 0.5f));

                gl::color(lineColor);
                
                // Wide bars take up 1/Nth of the circle.
                // Otherwise they're a fixed width.
//#define SHOULD_DRAW_WIDE_BAR    1

                float momentCircumference = 2 * M_PI * mInnerRadius;
                bool shouldUseWideBar = numChannels < (momentCircumference / 2);
                // cout << "momentCircumference / 2 " << (momentCircumference / 2) << " numChannels " << numChannels << "\n";
  
                float innerDist = mInnerRadius * Config::ScreenScale();
                float outerDist = (mInnerRadius + (std::max(0.05f, lineMagnitude) * kUnitLength)) * Config::ScreenScale();

                if ( shouldUseWideBar )
                {
//#if SHOULD_DRAW_WIDE_BAR
                
                    float middleRad = i * radInterval;
                    // This limits the size of the bars
                    // float barRads = std::min<float>(radInterval, 0.05);
                    float barRads = std::max<float>(radInterval, 0.01);
                    
                    float startRad = middleRad - (barRads * 0.5);
                    float endRad = middleRad + (barRads * 0.5);
                    int numSegments = ceil((endRad - startRad) * 10) + 1;
                    
                    float x1 = cos(startRad - (M_PI * 0.5f));
                    float y1 = sin(startRad - (M_PI * 0.5f));
                    
                    float x2 = cos(endRad - (M_PI * 0.5f));
                    float y2 = sin(endRad - (M_PI * 0.5f));
                    
                    float outerAlpha = 1.0f;
                    // Fade the outer alpha the further out it gets.
                    // This lets us see through them if they overlap other interface elements.
                    if ( lineMagnitude > 1.5 )
                    {
                        outerAlpha = 1.0f - (lineMagnitude - 1.f);
                    }
                    
                    gl::begin(GL_TRIANGLE_STRIP);
                    float segmentRads = (endRad - startRad) / numSegments;
                    for ( int s = 0; s < numSegments + 1; ++s )
                    {
                        float sRad = startRad + (s * segmentRads);
                        float xS = cos(sRad - (M_PI * 0.5f));
                        float yS = sin(sRad - (M_PI * 0.5f));
                        if ( outerAlpha < 1 )
                        {
                            gl::color(ColorAf(lineColor, outerAlpha));
                        }
                        gl::vertex(xS * outerDist, yS * outerDist);
                        if ( outerAlpha < 1 )
                        {
                            gl::color(lineColor);
                        }
                        gl::vertex(xS * innerDist, yS * innerDist);
                    }
                    gl::end();
                    
                    // Draw divider lines
                    gl::lineWidth( 1 * Config::ScreenScale() );
                    gl::color(0,0,0);
                    gl::drawLine(Vec2f(x1,y1) * innerDist,
                                 Vec2f(x1,y1) * (outerDist + 1));
                    gl::drawLine(Vec2f(x2,y2) * innerDist,
                                 Vec2f(x2,y2) * (outerDist + 1));
                        
                } // wide bar
                else
                {
//#else
                // Fixed-width bars.
                
                    gl::lineWidth(2);
                    gl::drawLine(Vec2f(x,y) * mInnerRadius * Config::ScreenScale(),
                                 Vec2f(x,y) * (mInnerRadius + (std::max(0.05f, lineMagnitude) * kUnitLength)) * Config::ScreenScale());
                }
//#endif
                
                Vec3f myPosition(x * innerDist, y * innerDist, 0);
                float connectionStrength = currentFilter->connectionIntensityForTime(filterData, filterUserInfo);
                ChannelRingConnectionPoint myRingValue(myPosition, connectionStrength, lineColor);
                channelRingValues.push_back( myRingValue );
            }
            
            // Sort so we draw the dimmest connections first.
            // Otherwise it looks odd to have darker lines overlapping bright lines.
            sort(channelRingValues.begin(), channelRingValues.end(),
                 [](ChannelRingConnectionPoint & a, ChannelRingConnectionPoint & b){
                return a.connectionStrength < b.connectionStrength;
            });

            for ( int i = 0; i < channelRingValues.size(); ++i )
            {
                ChannelRingConnectionPoint myRingValue = channelRingValues[i];

                // By ommitting this condition, we get a nice hazy background that changes color.
                // if ( myRingValue.connectionStrength > 0 )
                {
                    for ( int j = i+1; j < channelRingValues.size(); ++j )
                    {
                        ChannelRingConnectionPoint otherRingValue = channelRingValues[j];
                        if ( otherRingValue.connectionStrength > 0 )
                        {
                            lineColors.push_back(ColorAf(otherRingValue.color,
                                                         otherRingValue.connectionStrength));
                            linePositions.push_back(otherRingValue.position);
                            //
                            lineColors.push_back(ColorAf(myRingValue.color,
                                                         myRingValue.connectionStrength));
                            linePositions.push_back(myRingValue.position);
                        }
                    }
                }
            }
            
            // Draw the lines.
            // There's not a super efficient way of doing this short of omitting
            // lines, or calculating it all on the GPU.
            // Just don't draw them more than we need to.
            gl::lineWidth( 0.5f * Config::ScreenScale() );
            gl::VboMesh mesh;
            gl::VboMesh::Layout layout;
            layout.setStaticPositions();
            layout.setStaticIndices();
            layout.setStaticColorsRGBA();
            int numVerts = linePositions.size();
            vector<uint32> lineIndices;
            for ( int v = 0; v < numVerts; ++v )
            {
                lineIndices.push_back(v);
            }
            mesh = gl::VboMesh( numVerts,
                                numVerts,
                                layout,
                                GL_LINES );
            mesh.bufferColorsRGBA(lineColors);
            mesh.bufferPositions(linePositions);
            mesh.bufferIndices(lineIndices);
            gl::draw(mesh);
            
            gl::popMatrices();
            
        });
        
    }
    
    gl::pushMatrices();
    
    mFBOChannels.getTexture().setFlipped(true);
    
    Rectf drawBounds = mFBOChannels.getTexture().getBounds();
    drawBounds = RectMult<float>( drawBounds, 1.f / Config::ScreenScale() );

    gl::color(1,1,1);
    gl::draw(mFBOChannels.getTexture(), drawBounds);
    
    gl::popMatrices();
    
    // Draw the chrome over the data
    gl::pushMatrices();

    gl::translate(mCenterPosition);
    
    // Draw the channel selection
    if ( mSelectedChannelIndex != -1 )
    {
        float x = cos((mSelectedChannelIndex * radInterval) - (M_PI * 0.5f));
        float y = sin((mSelectedChannelIndex * radInterval) - (M_PI * 0.5f));
        Vec2f selectedChannelVec(x,y);

        DataChannelRef selectedChannel = mFrameChannels[mSelectedChannelIndex];
        
        // Highlight the selected channel
        gl::lineWidth(10);
        gl::color(1,1,1,0.25f);
        gl::drawLine(selectedChannelVec * mInnerRadius,
                     selectedChannelVec * (mInnerRadius + kUnitLength));

        // Draw the selected channel name
        gl::color(ColorAf(1,1,1,1));

        string channelName = selectedChannel->name;
        ligo::drawCenteredString( channelName,
                                  Vec2f(0,-8),
                                  0.2,
                                  FontBook::getFont("bold") );
        
        CategoryRef channelCategory = selectedChannel->categoryMemberships[bundleName];
        if ( channelCategory )
        {
            
            ligo::drawCenteredString( channelCategory->name,
                                      Vec2f(0, 12),
                                      0.25,
                                      FontBook::getFont("heavy") );
        }
    }
    
    // Draw the measurement rings
    gl::lineWidth(1 * Config::ScreenScale());
    ColorAf ringColor = isPaused ? ColorAf(1,0,0,1) : ColorAf(1,1,1,1);
    
//  0
    gl::color( ColorAf(1,1,1,1) );
    gl::drawStrokedCircle(Vec2f(0,0), mInnerRadius);
    gl::color( ringColor );

//  0.5
    if ( !isPaused )
    {
        ringColor.a = 0.75;
        gl::color(ringColor);
    }
    gl::drawStrokedCircle(Vec2f(0,0), (mInnerRadius + kUnitLength * 0.5));

//  1
    if ( !isPaused )
    {
        ringColor.a = 0.5;
        gl::color(ringColor);
    }
    gl::drawStrokedCircle(Vec2f(0,0), (mInnerRadius + kUnitLength));

    gl::popMatrices();
    
    mShouldLog = false;
}

