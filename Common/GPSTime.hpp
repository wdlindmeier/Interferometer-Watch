//
//  GPSTime.hpp
//  LIGOFrames
//
//  Created by William Lindmeier on 6/30/14.
//
//

#ifndef LIGOFrames_GPSTime_hpp
#define LIGOFrames_GPSTime_hpp

#include <stdio.h>
#include <chrono>

namespace ligo
{
    static double getTimeNow()
    {
        // http://www.cplusplus.com/reference/chrono/time_point/time_since_epoch/
        using namespace std::chrono;
        system_clock::time_point tp = system_clock::now();
        system_clock::duration dtn = tp.time_since_epoch();
        return double(dtn.count() * system_clock::period::num) / system_clock::period::den;
    }

    // Heh. I suppose we should keep these up to date.
    static const int kNumLeapSeconds = 16;
    static long kLeapSeconds[kNumLeapSeconds] = { 46828800, 78364801, 109900802, 173059203, 252028804,
                                                  315187205, 346723206, 393984007, 425520008, 457056009,
                                                  504489610, 551750411, 599184012, 820108813, 914803214,
                                                  1025136015 };

    // Test to see if a GPS second is a leap second
    static inline bool isleap(long gpsTime)
    {
        bool isLeap = false;
        for (int i = 0; i < kNumLeapSeconds; ++i)
        {
            if ( gpsTime == kLeapSeconds[i] )
            {
                isLeap = true;
                break;
            }
        }
        return isLeap;
    }

    typedef enum TimeCompareTypes
    {
        TimeCompareGPSToUnix = 0,
        TimeCompareUNIXToGPS = 1
    } TimeCompareType;

    // Count number of leap seconds that have passed
    template<typename T>
    static inline int countleaps(T gpsTime, TimeCompareType compareType)
    {
        int nLeaps = 0;  // number of leap seconds prior to gpsTime
        for ( int i = 0; i < kNumLeapSeconds; ++i )
        {
            if ( compareType == TimeCompareUNIXToGPS ) // !strcmp('unix2gps', $dirFlag) )
            {
                if ( gpsTime >= kLeapSeconds[i] - i )
                {
                    nLeaps++;
                }
            }
            else if ( compareType == TimeCompareGPSToUnix ) //!strcmp('gps2unix', $dirFlag))
            {
                if ( gpsTime >= kLeapSeconds[i] )
                {
                    nLeaps++;
                }
            }
            else
            {
                printf("ERROR: Unknown time compare type\n");
            }
        }
        return nLeaps;
    }

    // Convert Unix Time to GPS Time
    template<typename T, typename U>
    static inline T unix2gps( U unixTime )
    {
        T gpsTime = unixTime - 315964800;
        int nLeaps = countleaps( gpsTime, TimeCompareUNIXToGPS );
        gpsTime = gpsTime + nLeaps;// + isLeap;
        return gpsTime;
    }

    // Convert GPS Time to Unix Time
    // T was time_t, U was long
    template<typename T, typename U>
    static inline T gps2unix( U gpsTime )
    {
        // Add offset in seconds
        T unixTime = gpsTime + 315964800.0;
        int nLeaps = countleaps( gpsTime, TimeCompareGPSToUnix );
        unixTime = unixTime - nLeaps;
        if ( isleap( gpsTime ) )
        {
            unixTime = unixTime + 0.5;
        }
        return unixTime;
    }

    static std::string unixTimeToString( const long timestamp, const char * format = "%c")
    {
        struct tm * dt;
        char timestr[30];
        dt = localtime( &timestamp );
        strftime(timestr, sizeof(timestr), format, dt);
        return std::string(timestr);
    }
    
    static std::string gpsTimeToString( const long gpsTime, const char * format = "%c")
    {
        const long timestamp = gps2unix<long>( gpsTime );
        return unixTimeToString( timestamp, format );
    }

}

#endif
