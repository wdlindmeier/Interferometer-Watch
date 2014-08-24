//
//  DataParser.cpp
//  LIGOclock
//
//  Created by William Lindmeier on 7/25/14.
//
//

#include "DataParser.h"
#include "ChannelDataIO.h"
#include "ProcessingPipeline.h"
#include "ChannelFilterDiff.h"
#include "ChannelFilterFFT.h"
#include "GroupingNavigation.h"
#include "ChannelStateManager.h"
#include "AppConfig.h"

using namespace ligo;
using namespace std;
using namespace ci;

void DataParser::work()
{
    bool hasParsedData = ProcessingPipeline::getInstance().hasParsedData();
    if ( hasParsedData )
    {
        ci::Timer perfTimer( true );

        // Ok, we got some data from the pipeline.
        // Now filter it.
        ligo::DataPacket packet = ProcessingPipeline::getInstance().dequeueParsedData();

        FilteredDataPacket filteredPacket(packet);
        
        // Pass the data through all of the filters
        for ( auto kvp : packet.channelData )
        {
            string channelName = kvp.first;
            ChannelDataFrameRef channelData = kvp.second;
            ChannelDataFrameRef prevChannelData;
            if ( mPrevPacket )
            {
                prevChannelData = mPrevPacket->channelData[channelName];
            }
            map<string, FilterResultsRef> filterResults;
            // Run the data through the filters
            for ( ChannelFilterRef & filter : mChannelFilters )
            {
                filterResults[filter->getFilterKey()] = filter->processChannelData(channelData, prevChannelData);
            }

            filteredPacket.filterResultsByChannel[channelName] = filterResults;
        }
        
        // NOTE: These aggregate calcs are now done on the fly because the number of permutations
        // in the navigation are many. The values are lazy-loaded then cached when needed.
        // The implementation below is kept to display how these could be done in the pipeline.
        
        /*
        // Count values for every bundle category
        // It's arguable that the bundle library should be under the purview of the grouping nav...
        std::map<std::string, ChannelGroupingRef> bundles = GroupingNavigation::getInstance().getAllBundleGroupings();

        // Pass the filter the bundle categories, the channels (with memberships), and the data
        const std::vector<DataChannelRef> & allChannels = ChannelStateManager::getInstance().getAllChannels();
        for ( ChannelFilterRef & filter : mChannelFilters )
        {
            for ( auto kvp : bundles )
            {
                std::map<std::string, FilterResultsRef> categoryFilterResults = filter->processBundleData(allChannels,
                                                                                                          filteredPacket,
                                                                                                          kvp.second);
                
                // NOW... store categoryFilterResults
                // NOTE: The bundle categories are all stored in a flat map—not in a bundle->cat heierarchy
                for ( auto r : categoryFilterResults )
                {
                    filteredPacket.filterResultsByBundleCategory[r.first][filter->getFilterKey()] = r.second;
                }
            }
            
            // We also want to run this on All
            ChannelGroupingRef allBundle = GroupingNavigation::getInstance().getGroupingStack().front();
            assert(allBundle->name == kRootGroupingName);
            std::map<std::string, FilterResultsRef> categoryFilterResults = filter->processBundleData(allChannels,
                                                                                                      filteredPacket,
                                                                                                      allBundle);
            for ( auto r : categoryFilterResults )
            {
                filteredPacket.filterResultsByBundleCategory[r.first][filter->getFilterKey()] = r.second;
            }
        }
        */
        
        perfTimer.stop();

        mPrevPacket = std::shared_ptr<DataPacket>(new DataPacket(packet));
#if DEBUG
        cout << "Data Filtering required: " << perfTimer.getSeconds() << " sec.\n";
#endif
        // Send it back to the pipeline as a FilteredDataPacket.
        // This will next get picked up by the FilteredDataServer
        ProcessingPipeline::getInstance().enqueueFilteredData( filteredPacket );
    }
    ::sleep(1);
}

template<typename T, typename U>
static inline bool FindIn(const T & t, const U & u )
{
    return find(u.begin(), u.end(), t) != u.end();
}

void DataParser::setupChannelFilters()
{
    ci::fs::path filtersPath = fs::path(Config::BundlingFilesPath()) / "filtering_manifest.csv";
    ReadCSV( filtersPath.string(), [&]( const vector<string> & lineTokens )
    {
        if ( lineTokens.size() == 0 || lineTokens[0].at(0) == '#' )
        {
            return;
        }
        else
        {
            assert(lineTokens.size() >= 3);
            
            string filterKey = lineTokens[0];
            string decription = lineTokens[2];
            int filterIsEnabled = stoi(lineTokens[1]);
            if ( filterIsEnabled == 1 )
            {
                // Make sure these filters are approved
                if ( filterKey == kChannelFilterKeyDiff )
                {
                    ChannelFilterRef diffFilter(new ChannelFilterDiff());
                    diffFilter->setDescription(decription);
                    mChannelFilters.push_back( diffFilter );
                }
                else if ( filterKey == kChannelFilterKeyFFT )
                {
                    ChannelFilterRef fftFilter(new ChannelFilterFFT());
                    fftFilter->setDescription(decription);
                    mChannelFilters.push_back( fftFilter );
                }
                // Add a new filter here:
//                else if ( filterKey == YOUR_CHANNEL_FILTER_KEY )
//                {
//                    ChannelFilterRef yourFilter(new YourChannelFilterClass());
//                    yourFilter->setDescription(decription);
//                    mChannelFilters.push_back( yourFilter );
//                }
            }
            else
            {
                assert(filterIsEnabled == 0);
            }
        }
    }, true);
    
}

const std::vector<ChannelFilterRef> DataParser::getAllFilters()
{
    return mChannelFilters;
}