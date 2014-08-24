//
//  BundlingNavigation.cpp
//  gpuPS
//
//  Created by William Lindmeier on 7/21/14.
//
//

#include "GroupingNavigation.h"
#include "cinder/Cinder.h"
#include "cinder/app/AppBasic.h"
#include "ChannelDataIO.h"
#include "AppConfig.h"

using namespace ligo;
using namespace ci;
using namespace ci::app;
using namespace std;

GroupingNavigation::GroupingNavigation()
{
    mBundleGroupings.clear();

    loadRootGrouping();
    loadBundleGroupings();
}

void GroupingNavigation::loadRootGrouping()
{
    ChannelGroupingRef rootGrouping = ChannelGroupingRef(new ChannelGrouping());
    rootGrouping->isBundle = false;
    rootGrouping->name = kRootGroupingName;
    // I'm not sure if we want this in the "All groupings" collection yet
    // mAllGroupings.push_back(rootGrouping);
    // NOTE: Also adding "All" to the stack as the Root state
    mGroupingStack.push_back(rootGrouping);
    mNavChangedSignal( this, mGroupingStack );
}

void GroupingNavigation::loadBundleGroupings()
{
    // Build a list of possible bundle groupings
    fs::path bundleManifestPath = fs::path(Config::BundlingFilesPath()) / "channel_bundling_manifest.csv";
    assert(fs::exists(bundleManifestPath));
    
    ReadCSV( bundleManifestPath.string(), [&]( const vector<string> & lineTokens )
            {
                if ( lineTokens.size() == 0 || lineTokens[0].at(0) == '#' )
                {
                    // Ignore empty lines, headers, & comments
                    return;
                }
                else
                {
                    assert( lineTokens.size() >= 3 );
                    ChannelGroupingRef grouping = ChannelGroupingRef(new ChannelGrouping());
                    grouping->name = lineTokens[0];
                    grouping->isBundle = true;
                    grouping->categories.clear();
                    grouping->categoryKeyPath = fs::path(Config::BundlingFilesPath()) / lineTokens[2];
                    grouping->bundlingColumnIndex = stoi(lineTokens[1]);
                    mBundleGroupings[grouping->name] = grouping;
                }
            });

    
    console() << "Loaded " << mBundleGroupings.size() << " bundles from the manifest. \n";

    // Load up the categories
    for ( auto kvp : mBundleGroupings )
    {
        loadGroupingCategoriesFromFile( kvp.second->name );
    }
}

#pragma mark - Accessors

const std::map<std::string, ChannelGroupingRef> GroupingNavigation::getAllBundleGroupings()
{
    return mBundleGroupings;
}

ChannelGroupingRef & GroupingNavigation::getCurrentGrouping()
{
    return mGroupingStack.back();
}

ChannelGroupingRef GroupingNavigation::getLastBundle()
{
    ChannelGroupingRef currentGrouping = getCurrentGrouping();
    int stackSize = mGroupingStack.size();
    for ( int i = stackSize-1; i > -1; --i )
    {
        ChannelGroupingRef bundle = mGroupingStack.at( i );
        if ( bundle->isBundle )
        {
            return bundle;
        }
    }
    return ChannelGroupingRef();
}

std::deque<ChannelGroupingRef> & GroupingNavigation::getGroupingStack()
{
    return mGroupingStack;
}

bool GroupingNavigation::hasBundleSelected( const string & groupingName )
{
    return find_if(mGroupingStack.begin(), mGroupingStack.end(), [=]( ChannelGroupingRef & group ){
        return group->name == groupingName;
    }) != mGroupingStack.end();
}

#pragma mark - Bundle Stack Management

void GroupingNavigation::popGrouping()
{
    if ( mGroupingStack.size() > 1 ) // Leave an empty root vector in-place
    {
        mGroupingStack.pop_back();
        mNavChangedSignal( this, mGroupingStack );
    }
    else
    {
        console() << "ERROR: No more to pop\n";
    }
}

void GroupingNavigation::popUntil( ChannelGroupingRef grouping )
{
    while ( mGroupingStack.back() != grouping )
    {
        mGroupingStack.pop_back();
    }
    mNavChangedSignal( this, mGroupingStack );
}

void GroupingNavigation::pushCategory( CategoryRef & category )
{
    // Create a grouping around a category
    ChannelGroupingRef grouping(new ChannelGrouping());
    grouping->name = category->name;
    grouping->categories[category->key] = category;
    mGroupingStack.push_back(grouping);
    mNavChangedSignal( this, mGroupingStack );
}

void GroupingNavigation::loadGroupingCategoriesFromFile( const string & groupingName )
{
    assert ( mBundleGroupings.count(groupingName) > 0 );
    
    ChannelGroupingRef bundleGrouping = mBundleGroupings[groupingName];

    bundleGrouping->categories.clear();
    
    map<string, CategoryRef> categories = ReadGroupingCategories( bundleGrouping->categoryKeyPath );
    for ( auto kvp : categories )
    {
        // Store the grouping name
        kvp.second->groupingName = groupingName;
        CategoryRef category = kvp.second;
        bundleGrouping->categories[category->key] = category;
    }

    bundleGrouping->didLoad = true;
}

void GroupingNavigation::pushBundle( ChannelGroupingRef & bundleGrouping )
{
    assert( bundleGrouping->isBundle );
    
    // NOTE: If the current state is a bundle, pop it so the new bundle
    // can take it's place.
    if ( mGroupingStack.back()->isBundle )
    {
        popGrouping();
    }

    assert(bundleGrouping->didLoad);
    
    // Each time we push a new bundle, divide the category colors across the spectrum.
    auto catIt = bundleGrouping->categories.begin();
    int numCategories = bundleGrouping->categories.size();
    vector<ColorAf> colors = Config::ApprovedColors();
    assert( numCategories < colors.size() );
    for ( int i = 0; i < numCategories; ++i )
    {
        CategoryRef cat = catIt->second;
        cat->color = colors[i];
        std::advance(catIt, 1);
    }
    
    mGroupingStack.push_back( bundleGrouping );
    mNavChangedSignal( this, mGroupingStack );
}

