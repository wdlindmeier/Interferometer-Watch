//
//  ChannelDataIO.cpp
//  gpuPS
//
//  Created by William Lindmeier on 7/16/14.
//
//

#include "ChannelDataIO.h"
#include <algorithm>

#ifdef DEPRECIATED
static NSString * const kLIGODataKeyStartTime = @"startTime";
static NSString * const kLIGODataKeyEndTime = @"endTime";
static NSString * const kLIGODataKeyDataChannels = @"channels";
static NSString * const kLIGODataKeyChannelName = @"name";
static NSString * const kLIGODataKeyChannelFilterValues = @"filterValues";
static NSString * const kLIGODataKeyChannelFilterRanks = @"filterRanks";
#endif

using namespace ci;
using namespace std;
using namespace ligo;

const std::vector<std::string> ReadChannelList( const ci::fs::path & filePath )
{
    std::vector<std::string> channelNames;
    ReadCSV( filePath.string(), [&]( const vector<string> & lineTokens )
            {
                if ( lineTokens.size() == 0 || lineTokens[0].at(0) == '#' )
                {
                    // Ignore empty lines, headers, & comments
                    return;
                }

                channelNames.push_back(lineTokens[0]);
            });
    return channelNames;
}

extern const std::map<std::string, ligo::CategoryRef> ReadGroupingCategories( const ci::fs::path & categoryKeyPath )
{
    map<string, CategoryRef> categoriesByKey;

    ReadCSV( categoryKeyPath.string(), [&]( const vector<string> & lineTokens )
            {
                if ( lineTokens.size() == 0 || // No tokens
                     (lineTokens[0].size() > 0 && lineTokens[0].at(0) == '#') ) // Line begins with #
                {
                    // Ignore empty lines, headers, & comments
                    return;
                }
                if ( lineTokens.size() == 2 )
                {
                    string categoryKey = lineTokens[0];
                    // Always downcase the keys so the data input is easier
                    std::transform(categoryKey.begin(), categoryKey.end(), categoryKey.begin(), ::tolower);
                    string categoryName = lineTokens[1];
                    CategoryRef category(new Category);
                    category->name = categoryName;
                    category->key = categoryKey;
                    categoriesByKey[categoryKey] = category;
                }
                else
                {
                    cout << "ERROR: Category line doesn't have 2 tokens.\n";
                    cout << boost::join(lineTokens, ",") << "\n";
                }
            });
    
    return categoriesByKey;
}

#ifdef DEPRECIATED

const map<string, CategoryRef> ReadChannelCategories( const fs::path & channelCategoriesPath,
                                                      const fs::path & categoryDescriptionPath,
                                                      map<string, CategoryRef> *categoriesByKey )
{
    if ( categoriesByKey == NULL )
    {
        cout << "Creating a tmp collection for categories.\n";
        // If they didn't pass one in, create a tmp collection for our purposes
        map<string, CategoryRef> catsByKey;
        categoriesByKey = &catsByKey;
    }
    
    bool didPassCategoryDescriptions = categoryDescriptionPath.string() != "";

    if ( didPassCategoryDescriptions )
    {
        ReadCSV( categoryDescriptionPath.string(), [&]( const vector<string> & lineTokens )
                {
                    if ( lineTokens.size() == 0 || lineTokens[0].at(0) == '#' )
                    {
                        // Ignore empty lines, headers, & comments
                        return;
                    }
                    if ( lineTokens.size() == 2 )
                    {
                        string categoryKey = lineTokens[0];
                        string categoryName = lineTokens[1];
                        CategoryRef category(new Category);
                        category->name = categoryName;
                        category->key = categoryKey;
                        (*categoriesByKey)[categoryKey] = category;
                    }
                    else
                    {
                        cout << "ERROR: Category line doesn't have 2 tokens.\n";
                        cout << boost::join(lineTokens, ",") << "\n";
                    }
                });
    }
    else
    {
        cout << "No category descriptions passed in. Creating categories on the fly.\n";
    }
    
    map<string,string> channelsByCategoryKey;
    set<string> uniqueCategories;
    
    ReadCSV( channelCategoriesPath.string(), [&]( const vector<string> & lineTokens )
    {
        if ( lineTokens.size() == 0 || lineTokens[0].at(0) == '#' )
        {
            // Ignore empty lines, headers, & comments
            return;
        }
        if ( lineTokens.size() == 2 )
        {
            channelsByCategoryKey[lineTokens[0]] = lineTokens[1];
            uniqueCategories.insert(lineTokens[1]);
        }
        else
        {
            cout << "ERROR: Channel line doesn't have 2 tokens.\n";
            cout << boost::join(lineTokens, ",") << "\n";
        }
    });
    
    // Create categories just based on their keys
    for ( const string & categoryKey : uniqueCategories )
    {
        if ( categoriesByKey->count(categoryKey) == 0 )
        {
            // NOTE: If the data contains categories that don't appear in the category descriptions,
            // make placeholders on-the-fly.
            CategoryRef category(new Category);
            category->name = categoryKey;
            category->key = categoryKey;
            (*categoriesByKey)[categoryKey] = category;
        }
    }
    
    map<string, CategoryRef> channelMapping;
    for ( auto kvp : channelsByCategoryKey )
    {
        channelMapping[kvp.first] = (*categoriesByKey)[kvp.second];
    }
    
    return channelMapping;
}

extern void ReadFilterDataFromFile(FilteredChannelDataRef & filter,
                                   std::map<string, DataChannelRef> *dataChannels )
{
    NSString *nsPath = [NSString stringWithUTF8String:filter->channelValuesPath.c_str()];
    assert( [[NSFileManager defaultManager] fileExistsAtPath:nsPath] );
    NSData * jsonData = [NSData dataWithContentsOfFile:nsPath];
    if ( jsonData )
    {
        NSError *readError = nil;
        NSDictionary *diffObject = [NSJSONSerialization JSONObjectWithData:jsonData
                                                                   options:0
                                                                     error:&readError];
        if ( readError )
        {
            NSLog(@"ERROR: Couldn't parse JSON: %@", readError);
        }
        else
        {
            NSNumber *nsStartTime = [diffObject valueForKey:kLIGODataKeyStartTime];
            filter->dataStartTime = [nsStartTime longValue];
            
            NSNumber *nsEndTime = [diffObject valueForKey:kLIGODataKeyEndTime];
            filter->dataEndTime = [nsEndTime longValue];
            
            NSDictionary *channels = [diffObject valueForKey:kLIGODataKeyDataChannels];
            assert( [channels isKindOfClass:[NSArray class]] );
            for ( NSDictionary *channelInfo : channels )
            {
                // First, we need to check if this DataChannel already exists.
                // If it does, use that one.
                string channelName = [[channelInfo valueForKey:kLIGODataKeyChannelName] UTF8String];
                DataChannelRef channel;
                if ( (*dataChannels).count(channelName) == 0 )
                {
                    channel = DataChannelRef(new DataChannel());
                    channel->name = channelName;
                }
                else
                {
                    channel = (*dataChannels)[channelName];
                }
                
                vector<FilterTimelineValue> timelineValues;

                for ( NSNumber *filterValue in [channelInfo valueForKey:kLIGODataKeyChannelFilterValues] )
                {
                    FilterTimelineValue tv;
                    tv.value = [filterValue floatValue];
                    timelineValues.push_back(tv);
                }
                
                int valueIdx = -1;
                for ( NSNumber *filterRank in [channelInfo valueForKey:kLIGODataKeyChannelFilterRanks] )
                {
                    valueIdx++;
                    FilterTimelineValue & tv = timelineValues[valueIdx];
                    tv.rank = [filterRank intValue];
                }
                
                channel->filterTimelineValues[filter->name] = timelineValues;

                if ( filter->numSamples <= 0 )
                {
                    filter->numSamples = timelineValues.size();
                }
                
                (*dataChannels)[channel->name] = channel;
            }
            
            filter->didLoad = true;            
        }
    }
    else
    {
        NSLog(@"ERROR: Coudln't read JSON at file: %@", nsPath);
    }
}

#endif
