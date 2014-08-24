//
//  Slider.hpp
//  GridMaker
//
//  Created by William Lindmeier on 10/17/13.
//
//

#pragma once

#include "cinder/Cinder.h"

namespace wdl
{
    struct Slider
    {
        Slider(ci::Rectf rect) :
        mRect(rect)
        ,mValue(0)
        ,mIsActive(false)
        ,mDrawTrack(true)
        ,mSliderColor(1,0,0)
        ,mSliderWidth(6)
        {};
        
        Slider(){};
        ~Slider(){};
        
        ci::Rectf mRect;
        float mValue;
        bool mIsActive;
        bool mDrawTrack;
        ci::ColorAf mSliderColor;
        float mSliderWidth;
        
        void update(ci::Vec2f mousePosition)
        {
            ci::Vec2f relPos = mousePosition - mRect.getUpperLeft();
            float scalarVal = relPos.x / mRect.getWidth();
            mValue = std::min<float>(1.0f, std::max<float>(0.0f, scalarVal));
        }
        
        bool contains(ci::Vec2f position)
        {
            return mRect.contains(position);
        }
        
        ci::Rectf getRect()
        {
            return mRect;
        }
        
        void setRect(const ci::Rectf & rect )
        {
            mRect = rect;
        }
        
        float getValue()
        {
            return mValue;
        }
        
        void setValue(const float value)
        {
            mValue = value;
        }
        
        void setIsActive(bool isActive)
        {
            mIsActive = isActive;
        }
        
        bool getIsActive()
        {
            return mIsActive;
        }
        
        void setShouldDrawTrack( bool shouldDrawTrack )
        {
            mDrawTrack = shouldDrawTrack;
        }
        
        void setSliderWidth( float sliderWidth )
        {
            mSliderWidth = sliderWidth;
        }
        
        void setSliderColor( const ci::ColorAf & color )
        {
            mSliderColor = color;
        }
        
        void render(bool isEnabled)
        {
            if ( mDrawTrack )
            {
                ci::gl::lineWidth(1.0f);
                ci::gl::color(ci::Color::white());
                ci::gl::drawLine(ci::Vec2f(mRect.x1, mRect.getCenter().y),
                                 ci::Vec2f(mRect.x2, mRect.getCenter().y));
            }
            ci::gl::lineWidth(6.0f);
            if (isEnabled)
            {
                ci::gl::color(mSliderColor);
            }
            else
            {
                ci::gl::color(ci::ColorAf(0.5f, 0.5f, 0.5f, 1.0f));
            }
            float offsetX = mRect.getWidth() * mValue;
            ci::gl::drawLine(ci::Vec2f(mRect.x1 + offsetX,
                                       mRect.y1),
                             ci::Vec2f(mRect.x1 + offsetX,
                                       mRect.y2));
        }
    };
}