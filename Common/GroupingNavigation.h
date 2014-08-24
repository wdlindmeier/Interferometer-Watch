//
//  BundlingNavigation.h
//  gpuPS
//
//  Created by William Lindmeier on 7/21/14.
//
//

#ifndef __gpuPS__BundlingNavigation__
#define __gpuPS__BundlingNavigation__

#include <iostream>
#include <map>
#include "SharedTypes.hpp"

/*
 
    GroupingNavigation

    This acts as a navigation stack. It also loads and stores all of the bundles.

    TERMINOLOGY:
    "Grouping" is any collection of channels
    "Category" is one entity that a node can be grouped with (e.g. "SUS" subsytem).
      A grouping can be all nodes that belong to a single category.
    "Bundle" is a Grouping of channels that can belong to more than 1 categories.
      Users select (drill down) into a Category within a bundle.
 
 
*/
 
namespace ligo
{
    const static std::string kRootGroupingName = "All Channels";
    
    class GroupingNavigation
    {
        
    public:
        
        typedef boost::signals2::signal<void( GroupingNavigation *, std::deque<ChannelGroupingRef> & )> NavigationDidChangeSignal;
        
    private:
        
        // Private for singleton
        GroupingNavigation();
        
        // Don't implement so the singleton can't be copied
        GroupingNavigation( GroupingNavigation const & );
        void operator = ( GroupingNavigation const & );
        
        NavigationDidChangeSignal mNavChangedSignal;

    public:
        
        static GroupingNavigation & getInstance()
        {
            // Instantiated on first use & guaranteed to be destroyed.
            static GroupingNavigation instance;
            return instance;
        }

        void popGrouping();
        void popUntil( ChannelGroupingRef grouping );
        
        // Push category will make the category a 1 attractor group
        void pushCategory( CategoryRef & category );
        void pushBundle( ChannelGroupingRef & bundleGrouping );
        
        ChannelGroupingRef & getCurrentGrouping();
        ChannelGroupingRef getLastBundle();
        
        const std::map<std::string, ChannelGroupingRef> getAllBundleGroupings();
        std::deque<ChannelGroupingRef> & getGroupingStack();

        bool hasBundleSelected( const std::string & bundleName );
        
        NavigationDidChangeSignal * getNavigationDidChangeSignal(){ return &mNavChangedSignal; };
        
    protected:
        
        void loadRootGrouping();
        void loadBundleGroupings();
        void loadGroupingCategoriesFromFile( const std::string & groupingName );
        std::map<std::string, ChannelGroupingRef> mBundleGroupings;
        std::deque<ChannelGroupingRef> mGroupingStack;
    };
}

#endif /* defined(__gpuPS__BundlingNavigation__) */
