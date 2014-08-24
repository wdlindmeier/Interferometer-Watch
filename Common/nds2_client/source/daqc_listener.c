/* -*- mode: c; c-basic-offset: 4; -*- */
#if HAVE_CONFIG_H
#include "daq_config.h"
#endif /* HAVE_CONFIG_H */

#if HAVE_STDLIB_H
#include <stdlib.h>
#endif /* HAVE_STDLIB_H */
#if HAVE_UNISTD_H
#include <unistd.h>
#else
#include <io.h>
#endif /* HAVE_UNISTD_H */
#include <string.h>
#include <stdio.h>
#if HAVE_PTHREAD_H
#include <pthread.h>
#endif /* HAVE_PTHREAD_H */
#include <errno.h>

#include "daqc.h"
#include "daqc_private.h"
#include "daqc_response.h"
#include "daqc_net.h"
#include "nds_logging.h"
#include "nds_os.h"

#include "daqc_listener.h"

static struct listenerArgs {
    nds_socket_type listenfd;
    mutex_t *lock;
    int error;
} listenerArgs;

/* Cleanup in the listener thread */
void
listener_cleanup (void* args) {
    close (((struct listenerArgs *)args) -> listenfd);
    ((struct listenerArgs *)args) -> error = 1;
#if _WIN32
    ReleaseMutex ((((struct listenerArgs *)args) -> lock));
#else /* _WIN32 */
    pthread_mutex_unlock ((((struct listenerArgs *)args) -> lock));
#endif /* _WIN32 */
}

/* Thread that listens for requests */
#if __GNUC__
#pragma GCC diagnostic ignored "-Wconversion"
#endif /* __GCC */
static void*
listener (void* a) {
    static const char* METHOD = "listener";
    int rc;
    daq_listen_t* lstn = (daq_listen_t *) a;
    daq_t* daq = lstn -> daq;

    nds_socket_type listenfd = socket (AF_INET, SOCK_STREAM, 0);
    if (listenfd < 0) {
	if ( nds_logging_check( NDS_LOG_GROUP_VERBOSE_ERRORS,
				10 ) )
	{
	    nds_logging_printf ( "ERROR: %s: socket(); errno=%d\n",
				 METHOD,
				 errno);
	}
	return NULL;
    }

    listenerArgs.listenfd = listenfd;
    listenerArgs.lock = &lstn -> lock;
    listenerArgs.error = 0;
/*
  pthread_cleanup_push (listener_cleanup, (void *) &listenerArgs);
*/

#ifdef not_def
    {
/* 64K seems to be system imposed limit on Solaris   */
/*
    const int max_allowed = 64 * 1024; 
    int sendbuf_size = (ssize && ssize < max_allowed)? ssize: max_allowed;
*/
	int rcvbuf_size = 1024 * 10;
	int rc = setsockopt (listenfd, SOL_SOCKET, SO_RCVBUF, 
			     (const char *) &rcvbuf_size, sizeof (rcvbuf_size));
	if ( rc &&
	     nds_logging_check( NDS_LOG_GROUP_VERBOSE_ERRORS,
				0 ) )
	{
	    nds_logging_printf ( "ERROR: %s: setsockopt(%d, %d); errno=%d\n",
				 METHOD,
				 listenfd, rcvbuf_size, errno );
	}

	{
	    int rcvbuf_size_len = 4;
	    rcvbuf_size = -1;
	    rc = getsockopt(listenfd, SOL_SOCKET, SO_RCVBUF, 
			    (const char *) &rcvbuf_size, &rcvbuf_size_len);
	    if ( rc )
	    {
		if ( nds_logging_check( NDS_LOG_GROUP_VERBOSE_ERRORS,
					0 ) )
		{
		    nds_logging_printf ( "ERROR: %s: getsockopt(%d, %d); errno=%d\n", 
					 METHOD, listenfd, rcvbuf_size, errno);
		}
	    }
	    else
	    {
		if ( nds_logging_check( NDS_LOG_GROUP_VERBOSE_ERRORS,
					10 ) )
		{
		    nds_logging_printf ( "INFO: %s: RCVBUF size is %d\n",
					 METHOD, rcvbuf_size );
		}
	    } /* if ( rc ) */
	}

    }
#endif /* defined(not_def) */

    /* This helps to avoid waitings for dead socket to drain */
    {
	int on = 1;
	setsockopt (listenfd, SOL_SOCKET, SO_REUSEADDR, (const char *) &on, 
		    (socklen_t) sizeof (on));
    }

    errno = 0;

    rc = bind (listenfd, (struct sockaddr *) lstn -> listener_addr, 
	       (socklen_t) sizeof(*lstn->listener_addr));
    if (rc < 0) {
	if (errno ==  EADDRINUSE || errno == 0) {
	    short port = ntohs(lstn -> listener_addr->sin_port) + 1;
	    int stop_port = port + 1024;
	    /* Listener's TCP port is already bound (in use)
	       Let's try to bind to the next one */
	    for (; port < stop_port; port++) {
		lstn -> listener_addr->sin_port = htons (port);
		rc = bind (listenfd, (struct sockaddr *) lstn->listener_addr, 
			   (socklen_t) sizeof(*(lstn->listener_addr)));
		if (rc < 0) {
		    if (errno != EADDRINUSE && errno != 0) {
			fprintf (stderr, "bind(); errno=%d\n", errno);
			return NULL;
		    }
		} else {
		    break;
		}
	    }
	} else {
	    fprintf (stderr, "bind(); errno=%d\n", errno);
	    return NULL;
	}
    }

#ifdef not_def
    {
//      const int max_allowed = 64 * 1024; /* 64K seems to be system imposed limit on Solaris */
//      int sendbuf_size = (ssize && ssize < max_allowed)? ssize: max_allowed;
	int rcvbuf_size = 1024 * 10;
    
	if (setsockopt (listenfd, SOL_SOCKET, SO_RCVBUF, 
			(const char *) &rcvbuf_size, sizeof (rcvbuf_size)))
	if ( nds_logging_check( NDS_LOG_GROUP_VERBOSE_ERRORS,
				0 ) )
	{
	    nds_logging_printf ( "ERROR: %s: setsockopt(%d, %d); errno=%d\n",
				 METHOD,
				 listenfd, 
				 rcvbuf_size, errno);
	}

	{
	    int rcvbuf_size_len = 4;
	    rcvbuf_size = -1;
	    rc = getsockopt (listenfd, 6, SO_RCVBUF, 
			     (const char *) &rcvbuf_size, &rcvbuf_size_len);
	    if (rc)
	    {
		if ( nds_logging_check( NDS_LOG_GROUP_VERBOSE_ERRORS,
					0 ) )
		{
		    nds_logging_printf ( "ERROR: %s: getsockopt(%d, %d); errno=%d\n",
					 METHOD,
					 listenfd, rcvbuf_size, errno );
		}
	    }
	    else
	    {
		if ( nds_logging_check( NDS_LOG_GROUP_VERBOSE_ERRORS,
					10 ) )
		{
		    nds_logging_printf ( "INFO: %s: RCVBUF size is %d\n",
					 METHOD,
					 rcvbuf_size );
		}
	    } /* if ( rc ) */
	}
    }
#endif

    if (listen (listenfd, 2) < 0) {
	fprintf (stderr, "listen(); errno=%d\n", errno);
	return NULL;
    }
    
#if _WIN32
    ReleaseMutex (&lstn -> lock);
#else /* _WIN32 */
    pthread_mutex_unlock (&lstn -> lock);
#endif /* _WIN32 */
    
    for (;;) {
	nds_socket_type	connfd;
	struct sockaddr raddr;
	unsigned int len = sizeof (raddr);
 
	if (lstn -> shutting_down)
	    break;

	if ( (connfd = accept (listenfd, &raddr, &len)) < 0) {
#ifdef	EPROTO
	    if (errno == EPROTO) continue;
#endif
	    if (errno == ECONNABORTED) {
		continue;
	    } else {
		fprintf (stderr, "accept(); errno=%d\n", errno);
		return NULL;
	    }
	}

#ifdef not_def
	{
	    int rcvbuf_size_len = 4;

	    int rcvbuf_size = 1024 * 10;

#ifdef not_def    
	    rc = setsockopt (connfd, SOL_SOCKET, SO_RCVBUF, 
			     (const char *) &rcvbuf_size, 
			     sizeof (rcvbuf_size));
	    if (rc} {
		fprintf (stderr, "setsockopt(%d, %d); errno=%d\n", 
			 connfd, rcvbuf_size, errno);
	    }
#endif

	    rcvbuf_size = -1;
	    rc = getsockopt (connfd, SOL_SOCKET, SO_RCVBUF, 
			     (const char *) &rcvbuf_size, &rcvbuf_size_len);
	    if (rc) fprintf (stderr, "getsockopt(%d, %d); errno=%d\n", 
			     connfd, rcvbuf_size, errno);
	    else printf ("connfd RCVBUF size is %d\n", rcvbuf_size);
	}
#endif

	/* Spawn working thread */
	{
	    int err_no;
#ifdef not_def
	    fprintf (stderr, "#connfd=%d\n", connfd);
#endif
	    daq -> conceal -> datafd = connfd;

#if _WIN32
	    {
		DWORD	tid;

		err_no = 0;
		lstn -> interpreter_tid = CreateThread (NULL,	/* default security attributes */
							0,    /* default stack size */
							(LPTHREAD_START_ROUTINE)lstn -> interpreter, 
							(void *) lstn, // thread function arguments
							0,   /* default creation flags */
							&tid );

		if ( lstn->interpreter_tid == NULL )
		{
		    err_no = GetLastError( );
		}
	    }
#else /* _WIN32 */
	    err_no = pthread_create (&lstn -> interpreter_tid, NULL, 
				     (void *(*)(void *))lstn -> interpreter, 
				     (void *) lstn);
#endif /* _WIN32 */
	    if (err_no) {
		fprintf (stderr, "failed to spawn interpreter thread; err=%d", 
			 err_no);
		close (connfd);
	    } else {
#ifdef not_def
		fprintf (stderr, "#interpreter started; tid=%d\n", 
			 lstn -> interpreter_tid);
#endif
#if _WIN32
#else /* _WIN32 */
		pthread_join (lstn -> interpreter_tid, NULL);
#endif /* _WIN32 */
	    }
	}
    }
    /*
      pthread_cleanup_pop(1);
    */
    return NULL;
}

#if __GNUC__
#pragma GCC diagnostic warning "-Wconversion"
#endif /* __GCC */

/*
  Create network listener on `*tcp_port' that listens for DAQD server
  connections and spawns `start_func' as POSIX thread passing `daq' as the
  argument when connection is established.

  Returns: `daq' or `0' if failed; *tcp_port is set to the actual port used
  Port number can be increased to allow more than one program copy on the same
  system.

*/
#if __GNUC__
#pragma GCC diagnostic ignored "-Wconversion"
#endif /* __GCC */
daq_listen_t*
daq_initialize (daq_listen_t* lstn, daq_t* daq, int* tcp_port, 
		void * (*start_func)(void *)) {
    int err_no;


    memset (lstn, 0, sizeof(daq_listen_t));

    lstn->daq = daq;
    lstn->listener_addr =
	(struct sockaddr_in*)malloc( sizeof(struct sockaddr_in) );
    memset( lstn->listener_addr, 0,
	    sizeof( sizeof( *(lstn->listener_addr) ) ) );

    /* Assign TCP/IP address for the listener to listen at */
    lstn -> listener_addr->sin_family = AF_INET;
    lstn -> listener_addr->sin_addr.s_addr = htonl(INADDR_ANY);
    lstn -> listener_addr->sin_port = htons (*tcp_port);

    lstn -> interpreter = start_func;

    /* Initialize mutex to synchronize with the listener initialization */
    /* Start listener thread */
#if _WIN32
    lstn -> lock = CreateMutex (NULL, FALSE, NULL);
    WaitForSingleObject (&lstn -> lock, INFINITE);
#else /* _WIN32 */
    pthread_mutex_init (&lstn -> lock, NULL);
    pthread_mutex_lock (&lstn -> lock);
#endif /* _WIN32 */

#if _WIN32
    {
	DWORD	tid;

	err_no = 0;
	lstn -> listener_tid = CreateThread (NULL,	/* default security attributes */
					     0,    /* default stack size */
					     (LPTHREAD_START_ROUTINE)listener, 
					     (void *) lstn, // thread function arguments
					     0,   /* default creation flags */
					     &tid );
	
	if ( lstn->listener_tid == NULL )
	{
	    err_no = GetLastError( );
	}
    }
#else /* _WIN32 */
    err_no = pthread_create (&lstn -> listener_tid, NULL, (void *(*)(void *))listener, (void *) lstn);
#endif /* _WIN32 */
    if (err_no) {
	fprintf (stderr, 
		 "pthread_create() failed to spawn a listener thread; err=%d", 
		 err_no);
#if _WIN32
	ReleaseMutex (&lstn -> lock);
	CloseHandle (&lstn -> lock);
#else /* _WIN32 */
	pthread_mutex_unlock (&lstn -> lock);
	pthread_mutex_destroy (&lstn -> lock);
#endif /* _WIN32 */
	return 0;
    }

    /* Wait until listener unlocks it, signalling us that the
       initialization complete */
#if _WIN32
    CloseHandle (&lstn -> lock);
#else /* _WIN32 */
    pthread_mutex_lock (&lstn -> lock);
#endif /* _WIN32 */

    if (listenerArgs.error) return 0;

    /* Update listener port, which may be changed by the listener thread */
    *tcp_port = (int)( ntohs (lstn -> listener_addr->sin_port) );

    return lstn;
}
#if __GNUC__
#pragma GCC diagnostic warning "-Wconversion"
#endif /* __GCC */

/*
  Kill listener thread. This relies on the fact that the interpreter thread
  (with which listener is synchronized) is finished and listener is blocked on
  the accept() right now.  If this is too much to bother about you could just
  exit().
*/
int
daq_shutdown (daq_listen_t *lstn) {
    static const char* METHOD = "daq_shutdown";
    nds_socket_type sockfd;

    lstn -> shutting_down = 1;

    /*
      For each listener thread: cancel it then connect to
      kick off the accept()
    */
    //pthread_cancel (lstn -> listener_tid);

    if ((sockfd = socket (AF_INET, SOCK_STREAM, 0)) < 0) {
	if ( nds_logging_check( NDS_LOG_GROUP_VERBOSE_ERRORS,
				0 ) )
	{
	    nds_logging_printf ( "ERROR: %s: socket(); errno=%d\n",
				 METHOD,
				 errno);
	}
	return DAQD_SOCKET;
    }
    connect (sockfd, (struct sockaddr *) lstn -> listener_addr, 
	     (socklen_t) sizeof(*(lstn->listener_addr)));
    free( lstn->listener_addr );
    lstn->listener_addr = (struct sockaddr_in*)NULL;
    return 0;
}

