//
//  NavigationRenderer.h
//  LIGOclock
//
//  Copyright (c) 2014 __MyCompanyName__. All rights reserved.
//

#pragma once

#include "Button.h"
#include "GroupingNavigation.h"

/*

 NavigationRenderer
 
 This class is responsible for drawing the navigation stack, as well as handling user input
 and modifying the nav state accordingly.
 
*/

namespace ligo
{
    class NavigationRenderer
    {
        ci::Vec2f mSelectedFilterButtonPosition;
        
        std::map< std::string, ButtonRef > mButtons;
        
        std::vector< ButtonRef > mBundleButtons;
        std::vector< ButtonRef > mCategoryButtons;
        std::vector< ButtonRef > mNavigationButtons;
        std::vector< ButtonRef > mFilterButtons;
        
        ButtonRef mSelectCategoryButton;
        ButtonRef mSelectBundleButton;
        ButtonRef mSortButton;
        
        float mNavHorizontalExtent;
        bool mShowGroupingButtons;
        bool mShowFilterButtons;
        
        void backButtonWasClicked( Button *button );
        void bundleButtonWasClicked( Button *button );
        void categoryButtonWasClicked( Button *button );
        void filterButtonWasClicked( Button *button );
        void navigationButtonWasClicked( Button *button );
        void selectCategoryButtonWasClicked( Button *button );
        void selectBundleButtonWasClicked( Button *button );
        void sortButtonWasClicked( Button *button );
        
        void selectFilterNamed( const std::string & filterKey );
        
        void createBundleButtons();
        void createCategoryButtons();
        void createNavigationButtons();
        void createFilterButtons();
        
    public:
        
        NavigationRenderer() :
        mShowGroupingButtons(true)
        ,mShowFilterButtons(false)
        ,mNavHorizontalExtent(0)
        {};
        
        ~NavigationRenderer(){}
        
        // This should be called in App::setup
        void setup();
        void shutdown();
        
        void draw();
        
        void mouseDown( const ci::Vec2f & mousePos );
        void mouseUp( const ci::Vec2f & mousePos );
        void mouseMove( const ci::Vec2f & mousePos );
        void mouseDrag( const ci::Vec2f & mousePos );
        
        void navigationDidChange( GroupingNavigation * , std::deque<ChannelGroupingRef> & );
        
        Button::ButtonSignal * getButtonSignal( const std::string & buttonName );
        
        void setSortLabel( const std::string & label );
        
    };
}