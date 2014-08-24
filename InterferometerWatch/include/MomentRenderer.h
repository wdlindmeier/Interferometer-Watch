//
//  MomentRenderer.h
//  LIGOclock
//
//  Created by William Lindmeier on 7/31/14.
//
//

#pragma once

#include "SharedTypes.hpp"
#include "FilteredDataServer.h"
#include "ChannelFilter.h"
#include "cinder/gl/Fbo.h"

/*
 
 MomentRenderer
 
 This class renders the "moment view" which is the right-hand circle. All of the channels
 are rendered as radial lines around the circle and connections are drawn within.
 
*/

namespace ligo
{
    class MomentRenderer
    {
        ci::Vec2f mCenterPosition;
        float mInnerRadius;
        bool mShouldLog;
        bool mShouldSort;
        ci::gl::Fbo mFBOChannels;
        std::string mLastFrameRenderKey;
        
        std::vector<DataChannelRef> mFrameChannels;
        int mSelectedChannelIndex;
        
    public:
        
        MomentRenderer( const ci::Vec2f & centerPosition = ci::Vec2f(0,0),
                        const float innerRadius = 50 ) :
        mCenterPosition(centerPosition)
        ,mInnerRadius(innerRadius)
        ,mShouldLog(false)
        ,mShouldSort(false)
        ,mLastFrameRenderKey("")
        ,mSelectedChannelIndex(-1)
        {};
        
        void mouseUp( const ci::Vec2f & mousePos );
        
        bool centerContainsMousePosition( const ci::Vec2f & mousePos );
        bool ringContainsMousePosition( const ci::Vec2f & mousePos );
        void selectChannelAtMousePosition( const ci::Vec2f & mousePos );
        void deselectChannel();

        void setupGL();
        
        void setCenterPosition(const ci::Vec2f & centerPosition );
        const ci::Vec2f getCenterPosition();

        void setInnerRadius( const float radius );
        const float getInnerRadius();
        
        bool getIsSorted (){ return mShouldSort; };
        void setIsSorted ( bool shouldSort ){ mShouldSort = shouldSort; };
        
        void setShouldLog( bool shouldLog ){ mShouldLog = shouldLog; };

        void draw( FilteredDataPacketRef & dataPacket,
                   bool isPaused,
                   std::map<std::string, double> *filterUserInfo );

    };
}