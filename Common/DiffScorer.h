//
//  DiffScorer.h
//  LIGOFrames
//
//  Created by William Lindmeier on 7/10/14.
//
//

#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <map>
#include "cinder/Cinder.h"
#include "cinder/Filesystem.h"
#include "cinder/Color.h"
#include "SampleWindow.h"
#include "SharedTypes.hpp"

// TODO: Redefine "Sample Window"
//extern void WriteCategoryDataToFile(std::map<std::string, std::vector<SampleWindowRef> > sampleWindows,
//                                    std::map<std::string, std::vector<long> > sampleRanks,
//                                    const std::string & filename );

#pragma mark - DEPRECIATED

extern void WriteDiffDataToFile(std::map<std::string, std::vector<SampleWindowRef> > sampleWindows,
                                std::map<std::string, std::vector<long> > sampleRanks,
                                const std::string & filename );

extern void ReadDiffDataFromFile(const ci::fs::path & jsonPath,
                                 std::vector<ligo::CategoryRef> *categories,
                                 std::vector<ligo::ChannelDiffRef> *channelDiffs,
                                 std::vector< std::vector<ligo::ChannelDiffRef> > *topDiffs,
                                 long *startTime,
                                 long *endTime);

