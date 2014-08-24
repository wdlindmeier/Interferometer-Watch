//
//  ChannelStateManager.h
//  LIGOclock
//
//  Created by William Lindmeier on 7/25/14.
//
//

#pragma once

#include "cinder/Cinder.h"
#include "FilteredDataServer.h"
#include "GroupingNavigation.h"
#include "SharedTypes.hpp"
#include "DataConnection.h"

/*
 
 ChannelStateManager
 
 This class acts as a state machine for which channels are displayed on the screen at a given
 time, based on the navigation state. It's also the only place that channels should be loaded.
 Channels are associated with bundles and categories as they're loaded.
 
*/

namespace ligo
{
    class ChannelStateManager
    {
        
    private:

        // Private for singleton
        ChannelStateManager();
        
        // Don't implement so the singleton can't be copied
        ChannelStateManager( ChannelStateManager const & );
        void operator = ( ChannelStateManager const & );
        
    protected:
        
        std::vector<DataChannelRef> mOrderedChannels;
        std::vector<DataChannelRef> mDisplayChannels;
        ChannelFilterRef mSelectedFilter;
        
        std::vector<DataChannelRef> channelsSelectedByNav();
        void loadChannels();
        
    public:
        
        static ChannelStateManager & getInstance()
        {
            // Instantiated on first use & guaranteed to be destroyed.
            static ChannelStateManager instance;
            return instance;
        }

        // Channels that are currently visible
        const std::vector<DataChannelRef> & getDisplayChannels();
        // All channels, sorted as they're listed in channel_bundling
        const std::vector<DataChannelRef> & getAllChannels();
        GroupingNavigation & getGroupingNavigation();
        ChannelFilterRef & getCurrentFilter();
        
        // Changes the nav state 
        void selectFilter( ChannelFilterRef & filter );
        void selectBundle( ChannelGroupingRef & bundleGrouping );
        void selectCategory( CategoryRef & category );
        void goBack();
        void goBackUntil( const std::string & navGroupingName );
        
    };
    
    typedef std::shared_ptr<ChannelStateManager> ChannelStateManagerRef;
}