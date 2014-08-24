//
//  ChannelDataIO.h
//  gpuPS
//
//  Created by William Lindmeier on 7/16/14.
//
//

#ifndef __gpuPS__ChannelDataIO__
#define __gpuPS__ChannelDataIO__

#include <iostream>
#include <map>
#include <set>
#include "cinder/Filesystem.h"
#include "SharedTypes.hpp"
#include "cinder/Cinder.h"
#include "CinderHelpers.hpp"
#include "cinder/Utilities.h"
#include <fstream>
#include <string>
#include <boost/tokenizer.hpp>

template<typename Func>
const static void ReadCSV( const std::string & filePath, Func func, bool ignoreEmptyValues = false)
{
    assert( ci::fs::exists(filePath) );
    
    using namespace boost;
    
    std::string line;
    std::ifstream myfile ( filePath );
    
    if ( myfile.is_open() )
    {
        while ( getline ( myfile, line ) )
        {
            // NOTE: escaped_list_separator default values:
            // escape: \ split: , quote: "
            tokenizer<escaped_list_separator<char> > tok(line);
            std::vector<std::string> tokens;
            for(tokenizer<escaped_list_separator<char> >::iterator beg=tok.begin(); beg!=tok.end();++beg)
            {
                // Trim and ignore empty strings
                std::string tok(*beg);
                boost::trim(tok);
                if ( !ignoreEmptyValues || tok != "" )
                {
                    tokens.push_back(tok);
                }
            }
            func(tokens);
        }
        myfile.close();
    }
    else std::cout << "ERROR: Unable to open CSV file " << filePath << "\n";
}

// Simply reads a list of channel names from a CSV
extern const std::vector<std::string> ReadChannelList( const ci::fs::path & filePath );

extern const std::map<std::string, ligo::CategoryRef> ReadGroupingCategories( const ci::fs::path & categoryKeyPath );

#ifdef DEPRECIATED
extern const std::map<std::string, ligo::CategoryRef> ReadChannelCategories( const ci::fs::path & channelCategoriesPath,
                                                                             const ci::fs::path & categoryDescriptionPath = ci::fs::path(),
                                                                             std::map<std::string, ligo::CategoryRef> *categoriesByKey = NULL );

extern void ReadFilterDataFromFile(ligo::FilteredChannelDataRef & filter,
                                   std::map<std::string, ligo::DataChannelRef> *dataChannels );
#endif

#endif /* defined(__gpuPS__ChannelDataIO__) */
