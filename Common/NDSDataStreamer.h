//
//  NDSDataStreamer.h
//  NDS2Connection
//
//  Created by William Lindmeier on 7/23/14.
//
//

#ifndef __NDS2Connection__NDSDataStreamer__
#define __NDS2Connection__NDSDataStreamer__

#include "GPSTime.hpp"
#include "daqc.h"
#include "nds2.h"
#include "SharedTypes.hpp"

#include <iostream>
#include <thread>

namespace ligo
{
    class NDSDataStreamer;
    
    // Make your client a subclass of NDSDataListener to receive data
    class NDSDataListener
    {
        
    public:
        
        NDSDataListener(){}
        virtual ~NDSDataListener(){}
        
        // WARNING: This will be called on a secondary thread
        virtual void ndsDataDidReceiveNewChannelData( std::map<std::string, ChannelDataFrameRef> &,
                                                      NDSDataStreamer *streamer ) = 0;
    };

    class NDSDataStreamer
    {
        
    protected:
        
        // Connection
        daq_t mDaqConnection;
        bool mIsConnected;
        bool mHasLoadedFirstData;
        std::string mHostname;
        int mPort;
        
        // Channels
        std::vector<std::string> mRequestedChannels;
        
        // Attributes
        std::string mName;

        // Data buffers
        std::map<std::string, ChannelDataFrameRef> mLastWindowData;
        std::map<std::string, DataRange> mLastWindowRanges;

        // Timestamps
        time_t mTimeStride;
        time_t mTimeLoadedDataStarts;
        time_t mTimeDelay;
        time_t mTimeDisplayedDataStarts;
        time_t mTimeDisplayedDataEnds;

        double mTimeRequestBegan;
        // IMPORTANT: Keep this declaration below mTimeStride
        float  mRequestTimeout;
        
        // Requests
        bool mIsLoadingData;
        void requestChannels( const std::vector<std::string> & channels );
        void loadDataForTimeRange(const time_t startTime, const time_t endTime);
        void requestData( const time_t startTime, const time_t endTime, const time_t stride );
        
        void timeout();
        
        // Parsing
        void parseRequestedData( const time_t startTime,
                                 const time_t endTime,
                                 const std::vector<std::string> & channelNames );
        std::thread mDataThread;

        // Listeners
        std::vector<NDSDataListener *> mDataListeners;
        
    public:
        
        // Constructors
        NDSDataStreamer( const std::string & name = "Data Streamer -1" );
        ~NDSDataStreamer(){ disconnect(); };
        
        // Attributes
        const std::string getName();
        
        // Listeners
        void addDataListener(NDSDataListener * dataListener);
        void removeDataListener(NDSDataListener * dataListener);
        
        // Connection
        bool isConnected();
        bool connect( const std::string & hostname, int port = 31200, nds_version ndsVersion = nds_v2 );
        void disconnect( );

        // Channels
        void setRequestedChannels( std::vector<std::string> & channelNames );
        const std::vector<std::string> getRequestedChannels();
        
        // Time
        // Returns timestamps for the data currently displayed
        double worldTimeToDataTime( double aGPSTime );
        void getDataTimestamps(time_t * dataStartTime,
                               time_t * dataEndTime,
                               double * dataPlayheadTime );
        const long getTimeStride(){ return mTimeStride; }
        const void setServerDataTimeDelay( long delay ){ mTimeDelay = delay; }

        // Update
        // Call tick in the update loop
        void tick();
    };
    
    typedef std::shared_ptr<NDSDataStreamer> NDSDataStreamerRef;

}

#endif /* defined(__NDS2Connection__NDSDataStreamer__) */
