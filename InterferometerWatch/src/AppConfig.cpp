//
//  ConfigLoader.cpp
//  LIGOclock
//
//  Created by William Lindmeier on 8/11/14.
//
//

#include "AppConfig.h"
#include "ChannelDataIO.h"
#include <numeric>

using namespace ci;
using namespace ligo;
using namespace std;

// Defaults
static std::string sTimelineWaveformChannelName = "C1:LSC-DARM_OUT16";
static std::string sTimelineFFTChannelName = "C1:LSC-DARM_IN1_DQ";
static bool sShouldUseRetinaDisplay = true;
static bool sShouldUseLiveData = false;
static string sFlatFilesPath = "";
static string sBundlingFilesPath = "";
static int sTimelineDuration = 0;
static string sServerAddress = "";
static long sServerPort = 31200;
static int sMaxNumberLiveChannels = std::numeric_limits<int>::max();
static int sLiveDataTimeDelay = 120;
static ci::Vec2i sScreenSize = ci::Vec2i(1280,800);
static bool sShouldUseFullScreen = true;
static int sNDSVersion = 2;

const static string kConfigSettingNameFFTChannel = "primary_channel_fft_name";
const static string kConfigSettingNameTimelineChannel = "primary_channel_timeline_name";
const static string kConfigSettingNameUseRetinaDisplay = "use_retina_display";
const static string kConfigSettingNameUseLiveData = "use_live_data";
const static string kConfigSettingNameMaxLiveChannels = "max_live_channels";
const static string kConfigSettingNameFlatFilesPath = "frame_files_path";
const static string kConfigSettingNameBundlingFilesPath = "bundling_files_path";
const static string kConfigSettingNameTimelineDuration = "timeline_duration_sec";
const static string kConfigSettingNameServerAddress = "live_data_server_address";
const static string kConfigSettingNameServerPort = "live_data_server_port";
const static string kConfigSettingNameLiveDataTimeDelay = "live_data_time_delay";
const static string kConfigSettingNameUseFullScreen = "use_fullscreen";
const static string kConfigSettingNameScreenWidth = "screen_width";
const static string kConfigSettingNameScreenHeight = "screen_height";
const static string kConfigSettingNameNDSVersion = "nds_version";

const std::string Config::TimelineWaveformChannelName()
{
    return sTimelineWaveformChannelName;
}

const std::string Config::TimelineFFTChannelName()
{
    return sTimelineFFTChannelName;
}

const bool Config::ShouldUseRetinaDisplay()
{
    return sShouldUseRetinaDisplay;
}

const float Config::ScreenScale()
{
    return sShouldUseRetinaDisplay ? 2.0f : 1.0f;
}

const bool Config::ShouldUseLiveData()
{
    return sShouldUseLiveData;
}

const string Config::FlatFilesPath()
{
    return sFlatFilesPath;
}

const std::string Config::BundlingFilesPath()
{
    return sBundlingFilesPath;
}

const int Config::TimelineDuration()
{
    return sTimelineDuration;
}
const std::string Config::ServerAddress()
{
    return sServerAddress;
}

const long Config::ServerPort()
{
    return sServerPort;
}

const int Config::MaxNumberLiveChannels()
{
    return sMaxNumberLiveChannels;
}

const int Config::LiveDataTimeDelay()
{
    return sLiveDataTimeDelay;
}

const bool Config::ShouldUseFullScreen()
{
    return sShouldUseFullScreen;
}

const ci::Vec2i Config::ScreenSize()
{
    return sScreenSize;
}

const int Config::NDSVersion()
{
    return sNDSVersion;
}

std::vector<ci::ColorAf> Config::ApprovedColors()
{
    static std::vector<ci::ColorAf> ApprovedColors;
    if ( ApprovedColors.size() == 0 )
    {
        ApprovedColors.push_back( ColorAf( 134/255.f, 109/255.f, 170/255.f, 1.f ) ); // Purple
        ApprovedColors.push_back( ColorAf( 227/255.f, 123/255.f, 156/255.f, 1.f ) ); // Pink
        ApprovedColors.push_back( ColorAf( 72/255.f, 185/255.f, 217/255.f, 1.f ) ); // Cyan
        ApprovedColors.push_back( ColorAf( 60/255.f, 177/255.f, 156/255.f, 1.f ) ); // Teal
        ApprovedColors.push_back( ColorAf( 163/255.f, 204/255.f, 63/255.f, 1.f ) ); // Lime
        ApprovedColors.push_back( ColorAf( 240/255.f, 202/255.f, 77/255.f, 1.f ) ); // Yellow
        ApprovedColors.push_back( ColorAf( 228/255.f, 122/255.f, 60/255.f, 1.f ) ); // Orange
        ApprovedColors.push_back( ColorAf( 224/255.f, 73/255.f, 73/255.f, 1.f ) ); // Red
        // Let's also create intermediate steps if we get this far
        int numIntermediate = ApprovedColors.size();
        for ( int i = 0; i < numIntermediate; ++i )
        {
            int mixWithI = (i + 1) % numIntermediate;
            ColorAf a = ApprovedColors.at(i);
            ColorAf b = ApprovedColors.at(mixWithI);
            ApprovedColors.push_back((a + b) / 2.f);
        }
        for ( int i = 0; i < 100; ++i )
        {
            // Lets also create 100 random colors just in case
            float randHue = (arc4random() % 10000) / 10000.f;
            ApprovedColors.push_back(ColorAf(CM_HSV, randHue, 1, 1, 1));
        }
    }
    return ApprovedColors;
}

void Config::Load( const ci::fs::path & configPath )
{
    ReadCSV( configPath.string(), [&]( const vector<string> & lineTokens )
            {
                if ( lineTokens.size() == 0 || lineTokens[0].at(0) == '#' )
                {
                    return;
                }
                else
                {
                    string configName = lineTokens[0];
                    string configValue = lineTokens[1];
                    if ( configName == kConfigSettingNameFFTChannel )
                    {
                        sTimelineFFTChannelName = configValue;
                    }
                    else if ( configName == kConfigSettingNameTimelineChannel )
                    {
                        sTimelineWaveformChannelName = configValue;
                    }
                    else if ( configName == kConfigSettingNameFlatFilesPath )
                    {
                        sFlatFilesPath = configValue;
                    }
                    else if ( configName == kConfigSettingNameUseLiveData )
                    {
                        sShouldUseLiveData = !!stoi(configValue);
                    }
                    else if ( configName == kConfigSettingNameUseRetinaDisplay )
                    {
                        sShouldUseRetinaDisplay = !!stoi(configValue);
                    }
                    else if ( configName == kConfigSettingNameTimelineDuration )
                    {
                        sTimelineDuration = stoi(configValue);
                    }
                    else if ( configName == kConfigSettingNameServerAddress )
                    {
                        sServerAddress = configValue;
                    }
                    else if ( configName == kConfigSettingNameServerPort )
                    {
                        sServerPort = stol(configValue);
                    }
                    else if ( configName == kConfigSettingNameMaxLiveChannels )
                    {
                        sMaxNumberLiveChannels = stoi(configValue);
                    }
                    else if ( configName == kConfigSettingNameLiveDataTimeDelay )
                    {
                        sLiveDataTimeDelay = stoi(configValue);
                    }
                    else if ( configName == kConfigSettingNameUseFullScreen )
                    {
                        sShouldUseFullScreen = !!stoi(configValue);
                    }
                    else if ( configName == kConfigSettingNameScreenWidth )
                    {
                        sScreenSize.x = stoi(configValue);
                    }
                    else if ( configName == kConfigSettingNameScreenHeight )
                    {
                        sScreenSize.y = stoi(configValue);
                    }
                    else if ( configName == kConfigSettingNameNDSVersion )
                    {
                        sNDSVersion = stoi(configValue);
                    }
                    else if ( configName == kConfigSettingNameBundlingFilesPath )
                    {
                        sBundlingFilesPath = configValue;
                    }
                }
            }, true);
}