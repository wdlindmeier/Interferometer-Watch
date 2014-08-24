//
//  DiffScorer.m
//  LIGOFrames
//
//  Created by William Lindmeier on 7/10/14.
//
//

#import "DiffScorer.h"
#include "cinder/Rand.h"
#include "cinder/app/App.h"
#include <Cocoa/Cocoa.h>
#include <vector>
#include <map>

using namespace std;
using namespace ci;
using namespace ligo;

static NSString * const kLIGODataKeyStartTime = @"startTime";
static NSString * const kLIGODataKeyEndTime = @"endTime";
static NSString * const kLIGODataKeySampleDuration = @"sampleDuration";
static NSString * const kLIGODataKeyCategories = @"categories";
static NSString * const kLIGODataKeyDataChannels = @"channels";
static NSString * const kLIGODataKeyChannelCategory = @"category";
static NSString * const kLIGODataKeyColor = @"color";
static NSString * const kLIGODataKeyColorRed = @"r";
static NSString * const kLIGODataKeyColorGreen = @"g";
static NSString * const kLIGODataKeyColorBlue = @"b";
static NSString * const kLIGODataKeyColorAlpha = @"a";
static NSString * const kLIGODataKeyChannelName = @"name";
static NSString * const kLIGODataKeyCategoryName = @"name";
static NSString * const kLIGODataKeyChannelFilterValues = @"filterValues";
static NSString * const kLIGODataKeyChannelFilterRanks = @"filterRanks";

#pragma mark - DEPRECIATED

const static int kNumTopRanks = 20;

extern void ReadDiffDataFromFile(const ci::fs::path & jsonPath,
                                 vector<CategoryRef> *categories,
                                 vector<ChannelDiffRef> *channelDiffs,
                                 vector< vector<ChannelDiffRef> > *topDiffs,
                                 long *startTime,
                                 long *endTime )
{
    NSString *nsPath = [NSString stringWithUTF8String:jsonPath.c_str()];
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
            (*topDiffs).clear();
            
            NSNumber *nsStartTime = [diffObject valueForKey:kLIGODataKeyStartTime];
            *startTime = [nsStartTime longValue];
            
            NSNumber *nsEndTime = [diffObject valueForKey:kLIGODataKeyEndTime];
            *endTime = [nsEndTime longValue];
            
            // Do something with this?
            // NSNumber *nsSampleDuration = [diffObject valueForKey:kLIGODataKeySampleDuration];
            
            // NOTE: Make categories first
            NSDictionary *categoriesInfo = [diffObject valueForKey:kLIGODataKeyCategories];
            assert( [categoriesInfo isKindOfClass:[NSDictionary class]] );
            map<string, CategoryRef> categoryDict;
            for ( NSString * catKey in categoriesInfo )
            {
                // Categories:
                // ColorAf color
                // string name
                
                NSDictionary *categoryInfo = [categoriesInfo valueForKey:catKey];
                CategoryRef category = CategoryRef(new Category());
                category->name = [[categoryInfo valueForKey:kLIGODataKeyCategoryName] UTF8String];
                NSDictionary *colorInfo = [categoryInfo valueForKey:kLIGODataKeyColor];
                float r = [[colorInfo valueForKey:kLIGODataKeyColorRed] floatValue];
                float g = [[colorInfo valueForKey:kLIGODataKeyColorGreen] floatValue];
                float b = [[colorInfo valueForKey:kLIGODataKeyColorBlue] floatValue];
                category->color = ColorAf(r,g,b,1.f);
                categoryDict[category->name] = category;
                (*categories).push_back(category);
            }
            
            NSDictionary *channels = [diffObject valueForKey:kLIGODataKeyDataChannels];
            assert( [channels isKindOfClass:[NSArray class]] );
            for ( NSDictionary *channelInfo : channels )
            {
                // ChannelDiffs:
                // string name;
                // CategoryRef category;
                // vector<float> diffValues;
                // vector<long> diffRanks;
                
                ChannelDiffRef channel = ChannelDiffRef(new ChannelDiff());
                string catKey = [[channelInfo valueForKey:kLIGODataKeyChannelCategory] UTF8String];
                channel->category = categoryDict[catKey];
                channel->name = [[channelInfo valueForKey:kLIGODataKeyChannelName] UTF8String];
                
                for ( NSNumber *diffScore in [channelInfo valueForKey:kLIGODataKeyChannelFilterValues] )
                {
                    channel->diffValues.push_back([diffScore floatValue]);
                }
                
                if ( (*topDiffs).size() == 0 )
                {
                    // Create a collection of the approp size
                    int numSamples = channel->diffValues.size();
                    for ( int s = 0; s < numSamples; ++s )
                    {
                        vector<ChannelDiffRef> windowRanks;
                        for ( int r = 0; r < kNumTopRanks; ++r )
                        {
                            windowRanks.push_back(ChannelDiffRef());
                        }
                        (*topDiffs).push_back(windowRanks);
                    }
                }
                
                int diffIdx = -1;
                for ( NSNumber *diffRank in [channelInfo valueForKey:kLIGODataKeyChannelFilterRanks] )
                {
                    diffIdx++;
                    
                    int channelRank = [diffRank intValue];
                    channel->diffRanks.push_back(channelRank);
                    if ( channelRank < kNumTopRanks )
                    {
                        (*topDiffs)[diffIdx][channelRank] = channel;
                    }
                }
                (*channelDiffs).push_back(channel);
            }
        }
    }
    else
    {
        NSLog(@"ERROR: Coudln't read JSON at file: %@", nsPath);
    }
}

void WriteDiffDataToFile(std::map<std::string, std::vector<SampleWindowRef> > sampleWindows,
                         std::map<std::string, std::vector<long> > sampleRanks,
                         const std::string & filename )
{
    //    NSLog(@"TODO: WriteDiffDataToFile\nsampleWindows.size: %lu sampleRanks.size: %lu",
    //          sampleWindows.size(), sampleRanks.size());
    
    long minTimestamp = -1;
    long maxTimestamp = -1;
    int windowDuration = -1;
    
    NSMutableArray *dataChannels = [[NSMutableArray alloc] init];
    NSMutableDictionary *categories = [[NSMutableDictionary alloc] init];
    NSMutableArray *channelNames = [[NSMutableArray alloc] init];
    
    for ( auto kvp : sampleRanks )
    {
        string dataName = kvp.first;
        NSString *nsDataName = [NSString stringWithUTF8String:dataName.c_str()];
        string categoryName = string(dataName).substr(3, 3);
        NSString *nsCategoryName = [NSString stringWithUTF8String:categoryName.c_str()];
        
        // Create the channel
        NSMutableDictionary *channel = [[NSMutableDictionary alloc] init];
        //        Name
        //        Cat idx
        //        Diff scores
        //        Diff ranks
        [channel setValue:nsDataName forKey:kLIGODataKeyChannelName];
        [channel setValue:nsCategoryName forKey:kLIGODataKeyChannelCategory];
        NSMutableArray *diffValues = [[NSMutableArray alloc] init];
        for ( SampleWindowRef sample : sampleWindows[dataName] )
        {
            if ( sample->startTime < minTimestamp || minTimestamp < 0 )
            {
                minTimestamp = sample->startTime;
            }
            if ( sample->endTime > maxTimestamp || maxTimestamp < 0 )
            {
                maxTimestamp = sample->endTime;
            }
            if ( windowDuration < 0 )
            {
                windowDuration = sample->endTime - sample->startTime;
            }
            float diff = sample->signedDiff;
            if ( isnan(diff) )
            {
                NSLog(@"WARNING: diff is NAN for channel %s", dataName.c_str());
                diff = 0;
            }
            [diffValues addObject:@(diff)];
        }
        [channel setValue:diffValues forKey:kLIGODataKeyChannelFilterValues];
        [diffValues release]; diffValues = nil;
        
        NSMutableArray *ranks = [[NSMutableArray alloc] init];
        for ( long rank : sampleRanks[dataName] )
        {
            if ( isnan(rank) )
            {
                NSLog(@"WARNING: rank is NAN for channel %s", dataName.c_str());
                rank = -1;
            }
            [ranks addObject:@(rank)];
        }
        [channel setValue:ranks forKey:kLIGODataKeyChannelFilterRanks];
        [ranks release]; ranks = nil;
        
        [dataChannels addObject:channel];
        [channel release];
        
        // Create the category if it's not already
        if ( ![categories valueForKey:nsCategoryName] )
        {
            // Create a new category
            ci::Vec3f randColor = ci::Rand::randVec3f();
            NSDictionary *category = @{ kLIGODataKeyCategoryName : nsCategoryName,
                                        kLIGODataKeyColor : @{ kLIGODataKeyColorRed : @(abs(randColor.x)),
                                                               kLIGODataKeyColorGreen : @(abs(randColor.y)),
                                                               kLIGODataKeyColorBlue : @(abs(randColor.z)) } };
            [categories setValue:category forKey:nsCategoryName];
        }
    }
    
    NSDictionary *rootJSON = @{ kLIGODataKeyCategories : categories,
                                kLIGODataKeyDataChannels : dataChannels,
                                kLIGODataKeyStartTime : @(minTimestamp),
                                kLIGODataKeyEndTime : @(maxTimestamp),
                                kLIGODataKeySampleDuration : @(windowDuration) };
    NSError *jsonError = nil;
    NSData *jsonData = [NSJSONSerialization dataWithJSONObject:rootJSON
                                                       options:NSJSONWritingPrettyPrinted
                                                         error:&jsonError];
    if ( jsonError )
    {
        NSLog(@"ERROR: Couldn't write JSON.\n%@", jsonError);
    }
    else
    {
        ci::fs::path appPath = ci::app::getAppPath() / ".." / filename;
        NSString *nsPath = [NSString stringWithUTF8String:appPath.c_str()];
        NSError *writeError = nil;
        BOOL didWrite = [jsonData writeToFile:nsPath options:0 error:&writeError];
        if ( didWrite )
        {
            NSLog(@"SUCCESSfully wrote JSON to %@", nsPath);
        }
        else
        {
            NSLog(@"ERROR writing JSON to %@\n%@", nsPath, writeError);
        }
    }
    
    ///...
    [dataChannels release];
    [categories release];
    [channelNames release];
}
