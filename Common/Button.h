//
//  Button.h
//  LIGOclock
//
//  Copyright (c) 2014 __MyCompanyName__. All rights reserved.
//

#pragma once

#ifndef BUTTON_H_
#define BUTTON_H_

#include <boost/signals2.hpp>
#include <iostream>
#include "cinder/Cinder.h"
#include "cinder/gl/TextureFont.h"

namespace ligo
{
    class Button
    {
        
    public:

        typedef boost::signals2::signal<void( Button* )> ButtonSignal;
        
        Button();
        Button(const std::string & name, const ci::Vec2i & position, const ci::Vec2i & minSize = ci::Vec2i(0,0));
        virtual ~Button(){ mSignal.disconnect_all_slots(); }
        
        ButtonSignal * getSignal() { return &mSignal; }; // Notice we return a pointer to the signal

        void setFont( const ci::gl::TextureFontRef & font );
        
        bool contains( const ci::Vec2f & position );
        
        ci::Vec2f getRenderSize();
        
        std::string getKey();
        void setKey( const std::string & key );

        std::string getName();
        void setName( const std::string & name );

        ci::Vec2i getPosition();
        void setPosition( const ci::Vec2i & position );

        ci::Vec2i getMinSize();
        void setMinSize( const ci::Vec2i & minSize );

        ci::ColorAf getBackgroundColor();
        void setBackgroundColor( const ci::ColorAf & backgroundColor );

        ci::ColorAf getTextColor();
        void setTextColor( const ci::ColorAf & textColor );
        
        ci::ColorAf getBorderColor();
        void setBorderColor( const ci::ColorAf & borderColor );

        void setPadding( const ci::Vec2i & upperLeft, const ci::Vec2i & lowerRight );

        void setShouldDrawBackgroundWhenNotHovering( bool shouldDraw );

        bool getIsHidden();
        void setIsHidden( bool isHidden );

        void draw();
        
        bool mouseDown( const ci::Vec2f & mousePos );
        bool mouseUp( const ci::Vec2f & mousePos );
        bool mouseMove( const ci::Vec2f & mousePos );
        bool mouseDrag( const ci::Vec2f & mousePos );

    protected:
        
        void clicked();
        
    private:
        
        ButtonSignal    mSignal;
        std::string     mName;
        std::string     mKey;
        ci::Vec2i       mPosition;
        ci::ColorAf     mBackgroundColor;
        ci::ColorAf     mTextColor;
        ci::ColorAf     mBorderColor;
        ci::gl::TextureFontRef mFont;
        bool            mMouseDownInButton;
        bool            mMouseIsHovering;
        bool            mIsHidden;
        bool            mShouldDrawBackgroundWhenNotHovering;
        float           mStringScale;
        ci::Vec2i       mPaddingUpperLeft;
        ci::Vec2i       mPaddingLowerRight;
        ci::Vec2i       mMinSize;
        
    };
    
    typedef std::shared_ptr<Button> ButtonRef;
    
}

#endif /* BUTTON_H_ */
