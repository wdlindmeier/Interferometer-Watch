//
//  ChannelStateManager.cpp
//  LIGOclock
//
//  Created by William Lindmeier on 7/25/14.
//
//

#include "ChannelStateManager.h"
#include "ChannelDataIO.h"
#include "DataParser.h"
#include "ChannelFilterDiff.h"
#include "ChannelFilterFFT.h"
#include "AppConfig.h"

using namespace ligo;
using namespace std;
using namespace ci;

ChannelStateManager::ChannelStateManager()
{
    loadChannels();
    for (ChannelFilterRef filter : DataParser::getInstance().getAllFilters() )
    {
        if ( filter->getFilterKey() == kChannelFilterKeyDiff )
        {
            mSelectedFilter = filter;
            break;
        }
    }
    assert(!!mSelectedFilter);
}

void ChannelStateManager::loadChannels()
{
    ci::fs::path channelsPath = fs::path(Config::BundlingFilesPath()) / "channel_bundling.csv";
    mOrderedChannels.clear();
    std::map<std::string, ChannelGroupingRef> groupings = GroupingNavigation::getInstance().getAllBundleGroupings();
    
    ReadCSV( channelsPath.string(), [&]( const vector<string> & lineTokens )
            {
                if ( lineTokens.size() == 0 || lineTokens[0].at(0) == '#' )
                {
                    // Ignore empty lines, headers, & comments
                    return;
                }
                
                DataChannelRef channel(new DataChannel());
                channel->name = lineTokens[0];
                
                // Now, get the bundles
                for ( auto kvp : groupings )
                {
                    string groupingName = kvp.first;
                    ChannelGroupingRef grouping = kvp.second;
                    int groupingColumn = grouping->bundlingColumnIndex;
                    // Make sure the column exists
                    assert( lineTokens.size() > groupingColumn );
                    string channelGroupingValue = lineTokens[groupingColumn];
                    std::transform(channelGroupingValue.begin(),
                                   channelGroupingValue.end(),
                                   channelGroupingValue.begin(), ::tolower);
                    
                    // Make sure this category exists
                    assert( grouping->categories.count(channelGroupingValue) );
                    
                    CategoryRef channelCat = grouping->categories[channelGroupingValue];
                    channel->categoryMemberships[grouping->name] = channelCat;
                    channelCat->numChannels += 1;
                }
                
                mOrderedChannels.push_back(channel);
                
            });

    // Reduce the number of live channels as per our config.
    if ( Config::ShouldUseLiveData() )
    {
        // Peel off as many channels as we need to
        while ( mOrderedChannels.size() > Config::MaxNumberLiveChannels() )
        {
            // randomly pick one to erase
            int randIndex = arc4random() % mOrderedChannels.size();
            DataChannelRef channel = mOrderedChannels[randIndex];
            // It must not be a required channel
            if ( channel->name != Config::TimelineFFTChannelName() &&
                 channel->name != Config::TimelineWaveformChannelName() )
            {
                mOrderedChannels.erase(mOrderedChannels.begin() + randIndex);
            }
        }
        
        assert( mOrderedChannels.size() <= Config::MaxNumberLiveChannels() );
    }

    // NOW: Kill any categories that don't have members.
    for ( auto kvp : groupings )
    {
        string groupingName = kvp.first;
        ChannelGroupingRef grouping = kvp.second;
        std::map<std::string, CategoryRef> categories = grouping->categories;
        for ( auto kvp : categories )
        {
            if ( kvp.second->numChannels <= 0 )
            {
                grouping->categories.erase(grouping->categories.find(kvp.first));
            }
        }
    }

    mDisplayChannels = mOrderedChannels;
}

ChannelFilterRef & ChannelStateManager::getCurrentFilter()
{
    return mSelectedFilter;
}

GroupingNavigation & ChannelStateManager::getGroupingNavigation()
{
    //return mGroupingNavigation;
    return GroupingNavigation::getInstance();
}

const std::vector<DataChannelRef> & ChannelStateManager::getDisplayChannels()
{
    return mDisplayChannels;
}

const std::vector<DataChannelRef> & ChannelStateManager::getAllChannels()
{
    return mOrderedChannels;
}

// Use this to set mDisplayChannels
std::vector<DataChannelRef> ChannelStateManager::channelsSelectedByNav()
{
    vector<DataChannelRef> selectedChannels;
    
    vector<CategoryRef> categorySelections;
    for ( ChannelGroupingRef group : GroupingNavigation::getInstance().getGroupingStack() )
    {
        if ( !group->isBundle )
        {
            // This indicates a selection in the hierarchy.
            // Add category
            for ( auto kvp : group->categories )
            {
                categorySelections.push_back(kvp.second);
            }
        }
    }
    
    for ( DataChannelRef channel : mOrderedChannels )
    {
        bool shouldAppear = true;
        for ( CategoryRef & selectedCategory : categorySelections )
        {
            assert( selectedCategory->groupingName != "" );
            
            bool belongsToCategory = channel->categoryMemberships[selectedCategory->groupingName] ==
                                              selectedCategory;
            if ( !belongsToCategory )
            {
                shouldAppear = false;
                break;
            }
        }

        if ( shouldAppear )
        {
            selectedChannels.push_back(channel);
        }
    }
    
    // Sort channels by current grouping
    ChannelGroupingRef & grouping = GroupingNavigation::getInstance().getCurrentGrouping();
    // NOTE: If there's only 1 channel, dont bother
    if ( grouping->categories.size() > 1 )
    {
        sort(selectedChannels.begin(), selectedChannels.end(), [&](const DataChannelRef & channelA,
                                                                   const DataChannelRef & channelB)
             {
                 CategoryRef categoryA = channelA->categoryMemberships[grouping->name];
                 CategoryRef categoryB = channelB->categoryMemberships[grouping->name];
                 assert(categoryA && categoryB);
                 return categoryA < categoryB;
             });
    }

    return selectedChannels;    
}

#pragma mark - Selection

void ChannelStateManager::selectFilter( ChannelFilterRef & filter )
{
    // Assert that this filter is active
    std::vector<ChannelFilterRef> allFilters = DataParser::getInstance().getAllFilters();
    assert( find(allFilters.begin(), allFilters.end(), filter) != allFilters.end() );
    mSelectedFilter = filter;
    mDisplayChannels = channelsSelectedByNav();
}

void ChannelStateManager::selectBundle( ChannelGroupingRef & bundleGrouping )
{
    GroupingNavigation::getInstance().pushBundle( bundleGrouping );
    mDisplayChannels = channelsSelectedByNav();
}

void ChannelStateManager::selectCategory( CategoryRef & category )
{
    GroupingNavigation::getInstance().pushCategory( category );
    mDisplayChannels = channelsSelectedByNav();
}

void ChannelStateManager::goBack()
{
    GroupingNavigation::getInstance().popGrouping();
    mDisplayChannels = channelsSelectedByNav();
}

void ChannelStateManager::goBackUntil( const std::string & navGroupingName )
{
    ChannelGroupingRef lastGrouping;
    auto navStack = GroupingNavigation::getInstance().getGroupingStack();
    for ( int i = navStack.size() - 1; i > -1; i-- )
    {
        if ( navStack.at(i)->name == navGroupingName )
        {
            lastGrouping = navStack.at(i);
            break;
        }
    }
    if ( lastGrouping )
    {
        GroupingNavigation::getInstance().popUntil(lastGrouping);
        mDisplayChannels = channelsSelectedByNav();
    }
}
