/* -*- mode: c; c-basic-offset: 4; -*- */
#ifndef DAQ_LISTENER_H
#define DAQ_LISTENER_H

#include "daqc.h"

struct sockaddr_in;

#if HAVE_PTHREAD_H
typedef pthread_t	thread_t;
typedef pthread_mutex_t	mutex_t;
#elif _WIN32
typedef HANDLE thread_t;
typedef HANDLE mutex_t;
#else /* HAVE_PTHREAD_H */
#endif /* HAVE_PTHREAD_H */

#ifndef VXWORKS

/**  \defgroup daq2_listener Multi-threaded listener
  *  The daq2_listen module contains the code to spawn a listener thread
  *  that can be used to receive data from the original NDS server. It
  *  was originally part of the Daqc API and shared the control structures.
  *  A listener is useful in cases where one or more streams of (usually 
  *  online) data are sent to a port other than the control port to be 
  *  processed by an asynchronous thread. Note that no alternate port 
  *  capability is currently planned for NDS2 and that firewalls would 
  *  interfere with any such activity crossing network borders. The 
  *  listener code depends on Posix threads (pthreads) and is not available
  *  on VxWorks platforms.
  *
  *  A typical use of the NDS listener is as follows:
  *  \code
  *  -- Start a listener thread.
  *  --
  *    daq_t cmd;
  *    int rc;
  *    rc = daq_connect(&cmd, server_ip, server_port, 1);
  *    daq_listen_t lstnr;
  *    daq_initialize(&lstnr, &cmd, data_port, start_func);
  *
  *    ...
  *
  *    daq_shutdown(&lstnr);
  *
  *  -- Data reception function.
  *  --
  *  void* start_funct(void*) {
  *
  *  }
  *  \endcode
  *
  *  \author John Zweizig (based on code by A. Ivanov)
  *  \version 1.0; last modified March 27, 2008
  *  \{
  */


/**  daq_listen_t contains all the control/status information for an NDS1
  *  listener thread.
  */
typedef struct {

    /** Associated DAQ client address */
    daq_t* daq;

    /** Listener thread socket address. */
    struct sockaddr_in* listener_addr;

    /** Processing thread entry address */
    void * (*interpreter)(void *);

    /**  Shut down status flag.
      */
    int shutting_down;

    /** Network listener thread ID */
    thread_t listener_tid;

    /** Interpreter thread ID */
    thread_t interpreter_tid;

    /**  Mutex used to synchronize main thread with the listener thread 
	 initialization */
    mutex_t lock;

} daq_listen_t;

/**  Create network listener on `*tcp_port' that listens for DAQD server
  *  connections and spawns `start_func' as POSIX threads passing `daq' as 
  *  the argument when connection is established. A `daq_t' structure is 
  *  allocated and initialized if a NULL pointer is passed as `daq'.
  *  \remarks
  *  Not implemented on VXWORKS.
  *  \brief Create a network listener thread.
  *  \param lstn Listener parameter/status structure.
  *  \param daq  Client structure.
  *  \param tcp_port is set to the actual port used. Port number can be 
  *  increased to allow more than one program copy on the same system.
  *  \param start_func Pointer to the funtion to be executed asynchronously
  *  when a connection is accepted by the listener.
  *  \return Pointer to the initialized daq_t structure or 0 if failed.
  */
daq_listen_t*
daq_initialize (daq_listen_t* lstn, daq_t* daq, int* tcp_port, 
		void * (*start_func)(void *));

/**  Kill listener thread by setting the "shutting_down" flag and then 
  *  connection to the listener address. This relies on the fact that the 
  *  interpreter thread (with which listener is synchronized) is finished 
  *  and listener is blocked on the accept() right now.  If this is too 
  *  much to bother about you could just exit(). Not available on VXWORKS.
  *  \brief Kill the listener thread.
  *  \remarks
  *  Not implemented on VXWORKS.
  *  \param lstn Pointer to listener status structure.
  *  \return DAQD status code.
  */
int 
daq_shutdown (daq_listen_t* lstn);

/**  \}
  */

#endif /* defined(VXWORKS) */
#endif /* defined(DAQ_LISTENER_H) */
