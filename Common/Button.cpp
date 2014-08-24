//
//  Button.cpp
//  LIGOclock
//
//  Copyright (c) 2014 __MyCompanyName__. All rights reserved.
//

#include "Button.h"
#include "boost/bind.hpp"
#include "CinderHelpers.hpp"
#include "FontBook.hpp"
#include "AppConfig.h"

using namespace std;
using namespace cinder;
using namespace ligo;
using namespace wdl;

Button::Button() :
Button( "Button ID", Vec2i(0,0) )
{
}

Button::Button( const std::string & name, const ci::Vec2i & position, const ci::Vec2i & minSize ) :
mName(name)
,mKey(name)
,mMinSize(minSize)
,mPosition(position)
,mBackgroundColor(ColorAf(0,0,0,1))
,mTextColor(ColorAf(1,1,1,1))
,mBorderColor(ColorAf(0,0,0,0))
,mMouseIsHovering(false)
,mShouldDrawBackgroundWhenNotHovering(true)
,mIsHidden(false)
,mStringScale(0.25)
,mPaddingUpperLeft(5, 0)
,mPaddingLowerRight(5, minSize.y * 0.35)
{
}

void Button::setFont( const ci::gl::TextureFontRef & font )
{
    mFont = font;
}

std::string Button::getName()
{
    return mName;
}

void Button::setName( const std::string & name )
{
    mName = name;
}

void Button::setPadding( const ci::Vec2i & upperLeft, const ci::Vec2i & lowerRight )
{
    mPaddingUpperLeft = upperLeft;
    mPaddingLowerRight = lowerRight;
}

ci::Vec2i Button::getPosition()
{
    return mPosition;
}

void Button::setPosition(const ci::Vec2i &position)
{
    mPosition = position;
}

ci::ColorAf Button::getBackgroundColor()
{
    return mBackgroundColor;
}

void Button::setBackgroundColor( const ci::ColorAf & backgroundColor )
{
    mBackgroundColor = backgroundColor;
}

ci::ColorAf Button::getTextColor()
{
    return mTextColor;
}

void Button::setTextColor( const ci::ColorAf & textColor )
{
    mTextColor = textColor;
}

ci::ColorAf Button::getBorderColor()
{
    return mBorderColor;
}

void Button::setBorderColor( const ci::ColorAf & borderColor )
{
    mBorderColor = borderColor;
}

bool Button::getIsHidden()
{
    return mIsHidden;
}

void Button::setIsHidden( bool isHidden )
{
    mIsHidden = isHidden;
}

void Button::setShouldDrawBackgroundWhenNotHovering( bool shouldDraw )
{
    mShouldDrawBackgroundWhenNotHovering = shouldDraw;
}

std::string Button::getKey()
{
    return mKey;
}

void Button::setKey( const std::string & key )
{
    mKey = key;
}

ci::Vec2f Button::getRenderSize()
{
    Vec2f stringSize = ligo::measureString(mName, mStringScale, mFont);
    Vec2f practicalSize(stringSize.x + (mPaddingUpperLeft.x + mPaddingLowerRight.x),
                        stringSize.y + (mPaddingUpperLeft.y + mPaddingLowerRight.y));
    return Vec2f(std::max<float>(practicalSize.x, mMinSize.x),
                 std::max<float>(practicalSize.y, mMinSize.y));
}

void Button::draw()
{
    if ( mIsHidden )
    {
        return;
    }
    
    Vec2f buttonSize = getRenderSize();
    Rectf bgRect(mPosition.x, mPosition.y, 0, 0);
    bgRect.x2 = bgRect.x1 + buttonSize.x;
    bgRect.y2 = bgRect.y1 + buttonSize.y;
    
    if ( mMouseIsHovering )
    {
        gl::color(mTextColor);
        gl::drawSolidRect(bgRect);
    }
    else if ( mShouldDrawBackgroundWhenNotHovering )
    {
        gl::color(mBackgroundColor);
        gl::drawSolidRect(bgRect);
    }
    
    if ( mMouseIsHovering )
    {
        gl::color(mBackgroundColor);
    }
    else
    {
        gl::color(mTextColor);
    }

    ligo::drawString(mName,
                     Vec2f(bgRect.x1 + mPaddingUpperLeft.x,
                           bgRect.y2 - mPaddingLowerRight.y ),
                     mStringScale,
                     mFont);
    
    // Draw the outline
    gl::lineWidth(1 * Config::ScreenScale());
    gl::color(mBorderColor);
    gl::drawStrokedRect(bgRect);
}

#pragma mark - Mouse

bool Button::contains( const ci::Vec2f & position )
{
    Vec2f buttonSize = getRenderSize();
    Rectf bgRect(mPosition.x, mPosition.y, 0, 0);
    bgRect.x2 = bgRect.x1 + buttonSize.x;
    bgRect.y2 = bgRect.y1 + buttonSize.y;
    return bgRect.contains(position);
}

bool Button::mouseDown( const ci::Vec2f & mousePos )
{
    if ( mIsHidden )
    {
        return false;
    }
    mMouseDownInButton = contains(mousePos);
    return mMouseDownInButton;
}

bool Button::mouseUp( const ci::Vec2f & mousePos )
{
    if ( mIsHidden )
    {
        return false;
    }
    bool isInButton = contains(mousePos);
    if ( isInButton && mMouseDownInButton && !mIsHidden )
    {
        clicked();
    }
    mMouseDownInButton = false;
    return isInButton;
}

bool Button::mouseMove( const ci::Vec2f & mousePos )
{
    if ( mIsHidden )
    {
        return false;
    }
    mMouseIsHovering = contains(mousePos);
    return mMouseIsHovering;
}

bool Button::mouseDrag( const ci::Vec2f & mousePos )
{
    if ( mIsHidden )
    {
        return false;
    }
    mMouseIsHovering = contains(mousePos);
    return mMouseIsHovering;
}

ci::Vec2i Button::getMinSize()
{
    return mMinSize;
}

void Button::setMinSize( const ci::Vec2i & minSize )
{
    mMinSize = minSize;
}

/**
 * Dispatch the event to all listeners
 */
void Button::clicked()
{
    mSignal( this );
}
