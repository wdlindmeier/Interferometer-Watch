//
//  StoredDataConnection.h
//  LIGOclock
//
//  Created by William Lindmeier on 7/25/14.
//
//

#pragma once

#include "cinder/Cinder.h"
#include "DataConnection.h"

/*
 
 StoredDataConnection
 
 A subclass of DataConnection that reads data from flat files (in FrameLibrary format). The data
 will be "played back" in real-time as if it were live. This class uses the nds2_client library 
 to parse the frame files. When the app is loaded, it looks in the folder defined by 
 Config::FlatFilesPath() and caches the filenames that have a .gwf suffix. Those are the only
 files that will be read, even if the contents of the folder changes during the run of the app.
 
*/


namespace ligo
{
    class StoredDataConnection : public DataConnection
    {
        ci::fs::path mDataDirectory;
        std::vector<std::string> mDataPaths;
        std::vector<long> mFileTimestamps;
        long mNextPacketGPSStartTime;
        long mNextPacketGPSEndTime;
        long mDatasetGPSStartTime;
        long mDatasetGPSEndTime;
        long mFileTimeInterval;
        double mTimeDataStarted;
        
        void loadDataPaths();
        void appendFileData(std::string & filename,
                            std::map<std::string, ligo::ChannelDataFrameRef> * returnData,
                            long dataStartTimestamp,
                            long dataEndTimestamp );
        
        virtual void work();
        
    public:
        
        StoredDataConnection();
        
        long getDataStartTime();
        long getDataEndTime();
        
        void openConnectionWithDataPath( const ci::fs::path & frameFilePath );
        virtual void closeConnection();
        
        virtual double worldTimeToGPSDataTime( double unixTime );
        
    };
    
    typedef std::shared_ptr<StoredDataConnection> StoredDataConnectionRef;
}