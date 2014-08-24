//
//  ThreadedWorker.hpp
//  LIGOclock
//
//  Created by William Lindmeier on 7/28/14.
//
//

#ifndef LIGOclock_ThreadedWorker_hpp
#define LIGOclock_ThreadedWorker_hpp

#include "cinder/Cinder.h"
#include "SharedTypes.hpp"

/*
 
 ThreadedWorker
 
 A basic class that performs work on it's own thread. This is the superclass of DataParser, 
 DataConnection and FilterDataServer. 
 
*/

namespace ligo
{

    class ThreadedWorker
    {
        
    protected:
        
        bool mRunThread;
        std::thread mWorkerThread;
        
        void startWorkerThread()
        {
            // First, kill any thread that's already running
            killWorkerThread();
            mRunThread = true;
            mWorkerThread = std::thread(&ThreadedWorker::threadLoop,
                                        this);
        }
        
        void killWorkerThread()
        {
            mRunThread = false;
            if ( mWorkerThread.joinable() )
            {
                mWorkerThread.join();
            }
        }
    
        void threadLoop()
        {
            while ( mRunThread )
            {
                // Do something
                this->work();
            }
        }
    
    public:
        
        ThreadedWorker(){};
        ~ThreadedWorker(){ killWorkerThread(); };
        
        virtual void work() = 0;
    };

}

#endif
