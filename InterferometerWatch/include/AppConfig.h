//
//  ConfigLoader.h
//  LIGOclock
//
//  Created by William Lindmeier on 8/11/14.
//
//

#ifndef __LIGOclock__ConfigLoader__
#define __LIGOclock__ConfigLoader__

#include <iostream>
#include "cinder/Cinder.h"

/*
 
 Config
 
 This class loads config values from the flat file and also defines defaults.
 Values can be queried using static accessors.
 
*/

namespace ligo
{
    class Config
    {
        
    public:

        static void Load( const ci::fs::path & configPath );

        // NOTE: This channel will always be loaded from the data set even if it's not in
        // channel_bundling.csv. We use it as a timeline.
        // The name is defined in ligo_vis_config.csv
        static const std::string TimelineWaveformChannelName();
        
        // Ditto.
        // This is the "gravity wave" channel, which is a higher resolution
        // channel of the above. We get the DARM FFT from it.
        // Also defined in ligo_vis_config.csv
        static const std::string TimelineFFTChannelName();
        
        static const bool ShouldUseRetinaDisplay();
        static const bool ShouldUseLiveData();
        static const std::string FlatFilesPath();
        static const std::string BundlingFilesPath();
        static const int TimelineDuration();
        static const float ScreenScale();
        static std::vector<ci::ColorAf> ApprovedColors();
        static const std::string ServerAddress();
        static const long ServerPort();
        static const int MaxNumberLiveChannels();
        static const int LiveDataTimeDelay();
        static const int NDSVersion();
        static const bool ShouldUseFullScreen();
        static const ci::Vec2i ScreenSize();
    };
}

#endif /* defined(__LIGOclock__ConfigLoader__) */
