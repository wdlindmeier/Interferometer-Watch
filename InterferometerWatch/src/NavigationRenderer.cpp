//
//  NavigationRenderer.cpp
//  LIGOclock
//
//  Copyright (c) 2014 __MyCompanyName__. All rights reserved.
//

#include "NavigationRenderer.h"
#include "ChannelStateManager.h"
#include "CinderHelpers.hpp"
#include "GroupingNavigation.h"
#include "FontBook.hpp"
#include "AppConfig.h"

using namespace ligo;
using namespace cinder;
using namespace std;
using namespace wdl;

const static float kNavButtonHeight = 20.f;

void NavigationRenderer::setup()
{
    mSelectedFilterButtonPosition = Vec2f(100, app::getWindowHeight() - kNavButtonHeight - 20);
    
    GroupingNavigation::getInstance().getNavigationDidChangeSignal()->connect(boost::bind(&NavigationRenderer::navigationDidChange, this, _1, _2));

    mSelectCategoryButton = ButtonRef(new Button("Select a Category:", Vec2i(20, 50), Vec2i(0, kNavButtonHeight)));
    mSelectCategoryButton->getSignal()->connect( boost::bind( &NavigationRenderer::selectCategoryButtonWasClicked, this, _1 ) );
    mSelectCategoryButton->setTextColor(ColorAf(0.5,0.5,0.5,1));
    mSelectCategoryButton->setIsHidden(false);
    mSelectCategoryButton->setFont( FontBook::getFont("bold") );
    mButtons[mSelectCategoryButton->getKey()] = mSelectCategoryButton;
    
    mSelectBundleButton = ButtonRef(new Button("Select a Bundle:", Vec2i(20, 50), Vec2i(0, kNavButtonHeight)));
    mSelectBundleButton->getSignal()->connect( boost::bind( &NavigationRenderer::selectBundleButtonWasClicked, this, _1 ) );
    mSelectBundleButton->setTextColor(ColorAf(0.5,0.5,0.5,1));
    mSelectBundleButton->setIsHidden(false);
    mSelectBundleButton->setFont( FontBook::getFont("bold") );
    mButtons[mSelectBundleButton->getKey()] = mSelectBundleButton;
    
    float sortButtonX = app::getWindowWidth() - 200;
    mSortButton = ButtonRef(new Button("Bundle",
                                       Vec2i(sortButtonX,
                                             mSelectedFilterButtonPosition.y),
                                       Vec2i(120, kNavButtonHeight)));
    mSortButton->getSignal()->connect( boost::bind( &NavigationRenderer::sortButtonWasClicked, this, _1 ) );
    mSortButton->setTextColor( ColorAf(1,1,1,1) );
    mSortButton->setBackgroundColor(ColorAf(0,0,0,1));
    mSortButton->setBorderColor( ColorAf(1,1,1,1) );
    mSortButton->setIsHidden(false);
    mSortButton->setKey("sort");
    mSortButton->setFont( FontBook::getFont("bold") );
    mButtons[mSortButton->getKey()] = mSortButton;
    
    createNavigationButtons();
    createBundleButtons();
    createFilterButtons();
}

void NavigationRenderer::shutdown()
{
    for ( auto kvp : mButtons )
    {
        kvp.second->getSignal()->disconnect_all_slots();
    }
}

#pragma mark - Accessors

Button::ButtonSignal * NavigationRenderer::getButtonSignal( const std::string & buttonName )
{
    return mButtons[buttonName]->getSignal();
}

void NavigationRenderer::setSortLabel( const std::string & label )
{
    mSortButton->setName( label );
}

#pragma mark - Button Callbacks

void NavigationRenderer::backButtonWasClicked( Button *button )
{
    ChannelStateManager::getInstance().goBack();
}

void NavigationRenderer::bundleButtonWasClicked( Button *button )
{
    std::map<std::string, ChannelGroupingRef> bundles = GroupingNavigation::getInstance().getAllBundleGroupings();
    ChannelGroupingRef selectedBundle = bundles[button->getKey()];
    if ( selectedBundle )
    {
        if ( !GroupingNavigation::getInstance().hasBundleSelected( selectedBundle->name ) )
        {
            ChannelStateManager::getInstance().selectBundle( selectedBundle );
        }
        else
        {
            cout << "ERROR: A bundle is already selected.\n";
        }
    }
    else
    {
        cout << "ERROR: No selected bundle.\n";
    }
}

void NavigationRenderer::categoryButtonWasClicked( Button *button )
{
    ChannelGroupingRef lastBundle = GroupingNavigation::getInstance().getLastBundle();
    assert( !!lastBundle );
    CategoryRef selectedCategory = lastBundle->categories[button->getKey()];
    ChannelStateManager::getInstance().selectCategory( selectedCategory );
}

void NavigationRenderer::navigationButtonWasClicked( Button *button )
{
    ChannelGroupingRef lastGrouping = GroupingNavigation::getInstance().getGroupingStack().back();
    if ( button->getName() == lastGrouping->name )
    {
        // This is the "last" node.
        // Toggle the grouping buttons
        if ( lastGrouping->isBundle )
        {
            selectCategoryButtonWasClicked( mSelectCategoryButton.get() );
        }
        else
        {
            selectBundleButtonWasClicked( mSelectBundleButton.get() );
        }
    }
    else
    {
        ChannelStateManager::getInstance().goBackUntil( button->getName() );
    }
}

void NavigationRenderer::selectCategoryButtonWasClicked( Button *button )
{
    mShowGroupingButtons = !mShowGroupingButtons;
    for ( ButtonRef & button : mCategoryButtons )
    {
        button->setIsHidden( !mShowGroupingButtons );
    }
}

void NavigationRenderer::selectBundleButtonWasClicked( Button *button )
{
    mShowGroupingButtons = !mShowGroupingButtons;
    for ( ButtonRef & button : mBundleButtons )
    {
        button->setIsHidden( !mShowGroupingButtons );
    }
}

void NavigationRenderer::filterButtonWasClicked( Button *button )
{
    if ( mShowFilterButtons )
    {
        // Select a filter
        // Reposition selected button
        // Change background color
        // Set is hidden
        for ( ButtonRef & filterButton : mFilterButtons )
        {
            filterButton->setPosition(mSelectedFilterButtonPosition);
            if ( filterButton->getKey() == button->getKey() )
            {
                filterButton->setIsHidden(false);
            }
            else
            {
                filterButton->setIsHidden(true);
            }
        }
        selectFilterNamed(button->getKey());
    }
    else
    {
        // Show the filters
        // Reposition all of the buttons
        // change background color
        // Set is not hidden
        int yOffset = 0;
        
        for ( ButtonRef & filterButton : mFilterButtons )
        {
            // Only reposition the filters that arent current
            if ( filterButton->getKey() != button->getKey() )
            {
                yOffset -= button->getRenderSize().y + 5;
                
                float buttonY = mSelectedFilterButtonPosition.y + yOffset;
                filterButton->setPosition(Vec2i(mSelectedFilterButtonPosition.x,
                                                buttonY));
                filterButton->setIsHidden(false);
            }
        }
    }
    mShowFilterButtons = !mShowFilterButtons;
}

void NavigationRenderer::sortButtonWasClicked( Button *button )
{
    // Nothing to see here. We're actually handling the sort callback in the LIGOclockApp class.
}

#pragma mark - Selection

void NavigationRenderer::selectFilterNamed( const string & filterKey )
{
    const std::vector<ChannelFilterRef> filters = DataParser::getInstance().getAllFilters();
    for ( ChannelFilterRef filter : filters )
    {
        if ( filter->getFilterKey() == filterKey )
        {
            ChannelStateManager::getInstance().selectFilter( filter );
            break;
        }
    }
}

#pragma mark - Mouse

void NavigationRenderer::mouseDown( const ci::Vec2f & mousePos )
{
    for ( auto kvp : mButtons )
    {
        if ( kvp.second->mouseDown( mousePos ) )
        {
            // Kick out of the loop once one is found
            break;
        }
    }
}

void NavigationRenderer::mouseUp( const ci::Vec2f & mousePos )
{
    for ( auto kvp : mButtons )
    {
        if ( kvp.second->mouseUp( mousePos ) )
        {
            // Kick out of the loop once one is found
            break;
        }
    }
}

void NavigationRenderer::mouseMove( const ci::Vec2f & mousePos )
{
    for ( auto kvp : mButtons )
    {
        kvp.second->mouseMove( mousePos );
    }
}

void NavigationRenderer::mouseDrag( const ci::Vec2f & mousePos )
{
    for ( auto kvp : mButtons )
    {
        kvp.second->mouseDrag( mousePos );
    }
}

#pragma mark - Navitation Change

void NavigationRenderer::createBundleButtons()
{
    for ( ButtonRef & button : mBundleButtons )
    {
        button->getSignal()->disconnect_all_slots();
        mButtons.erase(mButtons.find(button->getKey()));
    }
    
    mBundleButtons.clear();

    const std::map<std::string, ChannelGroupingRef> bundles = GroupingNavigation::getInstance().getAllBundleGroupings();
    for ( auto kvp : bundles )
    {
        if ( !GroupingNavigation::getInstance().hasBundleSelected( kvp.second->name ) )
        {
            ButtonRef bundleButton(new Button(kvp.second->name, Vec2i(0, 0), Vec2i(0, kNavButtonHeight)));
            bundleButton->getSignal()->connect(boost::bind(&NavigationRenderer::bundleButtonWasClicked, this, _1));
            bundleButton->setIsHidden( !mShowGroupingButtons );
            bundleButton->setBackgroundColor(ColorAf(0,0,0,0.75));
            bundleButton->setTextColor(ColorAf(1,1,1));
            bundleButton->setFont( FontBook::getFont("bold") );
            mButtons[bundleButton->getKey()] = bundleButton;
            mBundleButtons.push_back(bundleButton);
        }
    }
}

void NavigationRenderer::createCategoryButtons()
{
    ChannelGroupingRef bundle = GroupingNavigation::getInstance().getLastBundle();

    for ( ButtonRef & button : mCategoryButtons )
    {
        button->getSignal()->disconnect_all_slots();
        mButtons.erase(mButtons.find(button->getKey()));
    }
    
    mCategoryButtons.clear();
    
    for ( auto kvp : bundle->categories )
    {
        CategoryRef cat = kvp.second;
        ButtonRef catButton(new Button(cat->name, Vec2i(0, 0), Vec2i(0, kNavButtonHeight)));
        catButton->getSignal()->connect(boost::bind(&NavigationRenderer::categoryButtonWasClicked, this, _1));
        catButton->setKey(cat->key);
        catButton->setTextColor(cat->color);
        catButton->setBackgroundColor(ColorAf(0,0,0,0.75));
        catButton->setIsHidden( !mShowGroupingButtons );
        catButton->setFont( FontBook::getFont("bold") );
        mButtons[catButton->getKey()] = catButton;
        mCategoryButtons.push_back(catButton);
    }
}

void NavigationRenderer::createNavigationButtons()
{
    for ( ButtonRef & button : mNavigationButtons )
    {
        button->getSignal()->disconnect_all_slots();
        mButtons.erase(mButtons.find(button->getKey()));
    }
    
    mNavigationButtons.clear();

    std::deque<ChannelGroupingRef> & navStack = GroupingNavigation::getInstance().getGroupingStack();
    for ( ChannelGroupingRef & group : navStack )
    {
        ButtonRef navButton(new Button(group->name, Vec2i(0, 0), Vec2i(0, kNavButtonHeight)));
        navButton->getSignal()->connect(boost::bind(&NavigationRenderer::navigationButtonWasClicked, this, _1));
        navButton->setKey("nav_" + group->name );
        navButton->setBackgroundColor(ColorAf(0,0,0,0.75));
        navButton->setTextColor(ColorAf(1,1,1));
        navButton->setFont( FontBook::getFont("bold") );
        mButtons[navButton->getKey()] = navButton;
        mNavigationButtons.push_back(navButton);
    }
}

void NavigationRenderer::createFilterButtons()
{
    for ( ButtonRef & button : mFilterButtons )
    {
        button->getSignal()->disconnect_all_slots();
        mButtons.erase(mButtons.find(button->getKey()));
    }
    
    mFilterButtons.clear();
    
    ChannelFilterRef & currentFilter = ChannelStateManager::getInstance().getCurrentFilter();
    vector<ChannelFilterRef> filters = DataParser::getInstance().getAllFilters();
    for ( ChannelFilterRef & filter : filters )
    {
        ButtonRef filterButton(new Button(filter->getDescription(),
                                          mSelectedFilterButtonPosition,
                                          Vec2i(120, kNavButtonHeight)));
        filterButton->getSignal()->connect(boost::bind(&NavigationRenderer::filterButtonWasClicked, this, _1));
        filterButton->setKey( filter->getFilterKey() );
        filterButton->setBackgroundColor( ColorAf(0,0,0) );
        filterButton->setTextColor( ColorAf(1,1,1,1) );
        filterButton->setBorderColor( ColorAf(1,1,1,1) );
        filterButton->setFont( FontBook::getFont("bold") );
        // Only show the current filter
        filterButton->setIsHidden(filter != currentFilter);
        mButtons[filterButton->getKey()] = filterButton;
        mFilterButtons.push_back(filterButton);
    }
}

void NavigationRenderer::navigationDidChange( GroupingNavigation * groupingNavigation,
                                              std::deque<ChannelGroupingRef> & navStack )
{
    createNavigationButtons();
    createBundleButtons();
    
    bool isBundle = navStack.back()->isBundle;
    if ( isBundle )
    {
        createCategoryButtons();
    }
    
    for ( ButtonRef & button : mCategoryButtons )
    {
        button->setIsHidden(!mShowGroupingButtons || !isBundle);
    }
    
    for ( ButtonRef & button : mBundleButtons )
    {
        button->setIsHidden(!mShowGroupingButtons || isBundle);
    }

}

#pragma mark - Render

void NavigationRenderer::draw()
{
    // Draw the channel stats

    auto addButton = [&]( ButtonRef & navButton, Vec2f & navSize, float y, bool drawArrow = true )
    {
        navSize += Vec2f(5, 0); // margin
        Vec2f buttonPosition = Vec2f(navSize.x, y);
        Vec2f buttonSize = navButton->getRenderSize();
        navSize += buttonSize;

        navButton->setPosition( buttonPosition );
        
        if ( drawArrow )
        {
            gl::color(0.5,0.5,0.5);
            navSize += Vec2f(5, 0); // margin
            navSize += ligo::drawString(">",
                                        Vec2f(navSize.x, y + (buttonSize.y * 0.75)),
                                        0.2,
                                        FontBook::getFont("book"));
        }
    };
    
    const float kYBaseline = 20.f;
    
    float y = kYBaseline;
    Vec2f navSize(20, 0);
    int i = 0;
    
    const Vec2f buttonOffset(5, -12);
    
    // Draw the nav stack
    gl::color(1, 1, 0);

    navSize.x = 20;

    for ( ButtonRef & navButton : mNavigationButtons )
    {
        gl::color(1, 1, 1);
        addButton(navButton, navSize, y);
    }
    
    navSize += Vec2f(5, 0); // margin

    i = 0;
    for ( ButtonRef & button : mBundleButtons )
    {
        float vertOffset = kNavButtonHeight + 5 + (i * (kNavButtonHeight + 6));
        button->setPosition( Vec2i( navSize.x, y + vertOffset ) );
        i++;
    }
    // Do it again for categories
    i = 0;
    for ( ButtonRef & button : mCategoryButtons )
    {
        float vertOffset = kNavButtonHeight + 5 + (i * (kNavButtonHeight + 6));
        button->setPosition( Vec2i( navSize.x, y + vertOffset ) );
        i++;
    }
    
    navSize -= Vec2f(5, 0);
    
    gl::color(0.5,0.5,0.5);
    bool isBundle = GroupingNavigation::getInstance().getGroupingStack().back()->isBundle;
    mSelectCategoryButton->setIsHidden( !isBundle );
    mSelectBundleButton->setIsHidden( isBundle );
    if ( isBundle )
    {
        addButton(mSelectCategoryButton, navSize, y, false);
    }
    else
    {
        addButton(mSelectBundleButton, navSize, y, false);
    }

    for ( auto kvp : mButtons )
    {
        ButtonRef button = kvp.second;
        button->draw();
    }
    
    // Draw the filter lable
    gl::color(0.5,0.5,0.5);
    
    gl::TextureFontRef labelFont;
    if ( Config::ShouldUseRetinaDisplay() )
    {
        labelFont = FontBook::getFont("book");
    }
    else
    {
        labelFont = FontBook::getFont("medium");
    }
    
    Vec2f labelSize = ligo::measureString("Plugin:", 0.25, labelFont);
    ligo::drawString("Plugin:",
                     Vec2f(mSelectedFilterButtonPosition.x - (labelSize.x + 10),
                           mSelectedFilterButtonPosition.y + 17),
                     0.25,
                     labelFont);
    
    // Draw the sort lable
    gl::color(0.5,0.5,0.5);
    labelSize = ligo::measureString("Sort:", 0.25, labelFont);
    ligo::drawString("Sort:",
                     Vec2f(mSortButton->getPosition().x - (labelSize.x + 10),
                           mSortButton->getPosition().y + 17),
                     0.25,
                     labelFont);

}