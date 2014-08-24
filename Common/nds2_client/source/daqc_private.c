#include "daq_config.h"

#if HAVE_ERRNO_H
#include <errno.h>
#endif /* HAVE_ERRNO_H */
#if HAVE_FCNTL_H
#include <fcntl.h>
#endif /* HAVE_FCNTL_H */
#if HAVE_STDIO_H
#include <stdio.h>
#endif /* HAVE_STDIO_H */
#if HAVE_STDLIB_H
#include <stdlib.h>
#endif /* HAVE_STDLIB_H */
#if HAVE_STRING_H
#include <string.h>
#endif /* HAVE_STRING_H */
#if HAVE_UNISTD_H
#include <unistd.h> 
#endif /* HAVE_UNISTD_H */

#include "nds_logging.h"
#include "daqc_private.h"
#include "daqc_response.h"

#if _WIN32
typedef int socklen_t;
#ifndef SHUT_RD
#define SHUT_RD SD_RECEIVE
#endif /* SHUT_RD */
#ifndef SHUT_WR
#define SHUT_WR SD_SEND
#endif /* SHUT_WR */
#ifndef SHUT_RDWR
#define SHUT_RDWR SD_BOTH
#endif /* SHUT_RDWR */
#else /* _WIN32 */
#define INVALID_SOCKET -1
#endif /* _WIN32 */

void
daq_private_init( daq_private_t* Private )
{
  if ( Private )
  {
    /*  Mark the sockets not open
     */
    Private->sockfd = INVALID_SOCKET;
    Private->datafd = INVALID_SOCKET;

    if ( Private->srvr_addr )
    {
      /* Initialize the srvr_address buffer
       */
      memset( Private->srvr_addr,
	      0,
	      sizeof( sizeof( *(Private->srvr_addr) ) ) );
    }
  }
}

void
daq_private_create( daq_private_t** Private )
{
  if ( Private )
  {
    *Private = (daq_private_t*)malloc( sizeof( daq_private_t) );
    (*Private)->srvr_addr
      = (socket_addr_t*)malloc( sizeof(socket_addr_t) );

    daq_private_init( *Private );

  }
       
}

void
daq_private_delete( daq_private_t** Private )
{
  if ( Private
       && *Private )
  {
    daq_private_srvr_disconnect( *Private );
    free( *Private );
    *Private = (daq_private_t*)NULL;
  }
}

int
daq_private_srvr_nonblocking( daq_private_t* Private, int NonBlocking )
{
  int	retval = 0;
#if _WIN32
#if 0
  long flag = (long)Blocking;
	
  ioctlsocket( daq->sockfd, FIONBIO, &flag );
#endif /* 0 */
#else
  {
    int flag = fcntl( Private -> sockfd, F_GETFL, 0);

    retval = ( flag & O_NONBLOCK ) ? 1 : 0;
    if ( NonBlocking )
    {
      flag |= O_NONBLOCK;
    }
    else
    {
      flag &= ~O_NONBLOCK;
    }
    fcntl( Private->sockfd, F_SETFL, flag );
  }
#endif
  return retval;
}

void
daq_private_srvr_close( daq_private_t* Private )
{
  static const char* METHOD = "daq_private_srvr_close";

  if ( nds_logging_check( NDS_LOG_GROUP_TRACE_ENTRY_EXIT, 10 ) )
  {
    nds_logging_printf( "ENTRY: %s\n",
			METHOD );
  }
  if ( Private
       && ( Private->sockfd != INVALID_SOCKET ) )
  {
    if ( nds_logging_check( NDS_LOG_GROUP_TRACE_ENTRY_EXIT, 10 ) )
    {
      nds_logging_printf( "INFO: %s: sockfd: %d\n",
			  METHOD,
			  Private->sockfd );
    }
#if _WIN32
#ifndef SD_BOTH
#define SD_BOTH 2
#endif
    shutdown(Private->sockfd, SD_BOTH);
    closesocket(Private->sockfd);
#else /* _WIN32 */
#ifndef SHUT_RDWR
#define SHUT_RDWR 2
#endif
    shutdown(Private->sockfd, SHUT_RDWR);
    close(Private->sockfd);
#endif /* _WIN32 */
    Private->sockfd = INVALID_SOCKET;
  }
  if ( nds_logging_check( NDS_LOG_GROUP_TRACE_ENTRY_EXIT, 10 ) )
  {
    nds_logging_printf( "EXIT: %s\n",
			METHOD );
  }
}

int
daq_private_srvr_connect( daq_private_t* Private )
{
  static const char* METHOD = "daq_private_srvr_connect";
  int connect_rc = INVALID_SOCKET;

  connect_rc
    = connect ( Private->sockfd, 
		(struct sockaddr *) Private-> srvr_addr, 
		(socklen_t) sizeof (*(Private-> srvr_addr))
		);
  if ( nds_logging_check( NDS_LOG_GROUP_CONNECTION, 10 ) )
  {
    nds_logging_printf( "INFO: %s: connect_rc: %d\n",
			METHOD,
			connect_rc );
  }

#if _WIN32
  if ( connect_rc != 0 )
  {
    if ( ( WSAGetLastError( ) == WSAEWOULDBLOCK )  )
    {
      connect_rc = NDS_SOCKET_TRANSIENT_FAILURE;
    }
    else
    {
      connect_rc = NDS_SOCKET_FAILURE;
    }
  }
#else /* _WIN32 */
  if ( connect_rc < 0 )
  {
    if ( nds_logging_check( NDS_LOG_GROUP_CONNECTION, 20 ) )
    {
      char	pmsg[ 256 ];

      strerror_r( errno, pmsg, sizeof( pmsg ) );
      nds_logging_printf( "WARN: %s: errno: %d - %s\n",
			  METHOD,
			  errno, pmsg );
    }
    if ( errno == EISCONN )
    {
      connect_rc = NDS_SOCKET_OK;
    }
    else if ( ( errno == EINPROGRESS )
	      || ( errno == EALREADY ) )
    {
      connect_rc = NDS_SOCKET_TRANSIENT_FAILURE;
    }
    else
    {
      connect_rc = NDS_SOCKET_FAILURE;
    }
  }
#endif /* _WIN32 */
  else
  {
    connect_rc = NDS_SOCKET_OK;
  }
  if ( nds_logging_check( NDS_LOG_GROUP_VERBOSE_ERRORS, 10 ) )
  {
    nds_logging_printf( "INFO: %s: On exit: connect_rc: %d\n",
			METHOD,
			connect_rc );
  }

  return connect_rc;
}

void
daq_private_srvr_disconnect( daq_private_t* Private )
{
  static const char* METHOD = "daq_private_srvr_disconnect";

  if ( nds_logging_check( NDS_LOG_GROUP_TRACE_ENTRY_EXIT, 10 ) )
  {
    nds_logging_printf( "INFO: %s: ENTRY\n",
			METHOD );
  }
  if ( ( Private ) &&
       ( Private->srvr_addr ) )
  {
    free( Private->srvr_addr );
    Private->srvr_addr = (struct sockaddr_in*)NULL;
  }
  if ( nds_logging_check( NDS_LOG_GROUP_TRACE_ENTRY_EXIT, 10 ) )
  {
    nds_logging_printf( "INFO: %s: EXIT\n",
			METHOD );
  }
}

int
daq_private_srvr_open( daq_private_t* Private )
{
  static const char* METHOD = "daq_private_srvr_open";
  int on = 1;

  if ( nds_logging_check( NDS_LOG_GROUP_TRACE_ENTRY_EXIT,
			  20 ) )
  {
    nds_logging_printf( "INFO: %s: ENTRY\n",
			METHOD );
  }
  Private->sockfd = socket (AF_INET, SOCK_STREAM, 0);
#if _WIN32
  if ( Private->sockfd == INVALID_SOCKET )
  {
    if ( nds_logging_check( NDS_LOG_GROUP_VERBOSE_ERRORS,
			    0 ) )
    {
      nds_logging_printf( "ERROR: %s: Socket failure: socket( ): %s\n",
			  METHOD, GetLastError( ) );
    }
    return DAQD_SOCKET;
  }
#else
  if ( Private -> sockfd < 0)
  {
    Private->sockfd = INVALID_SOCKET;
    if ( nds_logging_check( NDS_LOG_GROUP_VERBOSE_ERRORS,
			    0 ) )
    {
      char	pmsg[ 256 ];

      strerror_r( errno, pmsg, sizeof( pmsg ) );
      nds_logging_printf( "ERROR: %s: socket( ): %d - %s\n",
			  METHOD, errno, pmsg );
    }
    return DAQD_SOCKET;
  }
#endif /* _WIN32 */

  setsockopt ( Private->sockfd, SOL_SOCKET, SO_REUSEADDR, 
	       (const char *) &on,
	       (socklen_t) sizeof (on) );


  if ( nds_logging_check( NDS_LOG_GROUP_TRACE_ENTRY_EXIT,
			  20 ) )
  {
    nds_logging_printf( "INFO: %s: EXIT: socket( ): Success: %d\n",
			METHOD,
			Private->sockfd );
  }

  return DAQD_OK;
}

int
daq_private_data_close( daq_private_t* Private )
{
  static const char* METHOD = "daq_private_data_close";

  int retval = DAQD_OK;

  if ( nds_logging_check( NDS_LOG_GROUP_TRACE_ENTRY_EXIT, 10 ) )
  {
    nds_logging_printf( "INFO: %s: ENTRY\n",
			METHOD );
  }
  if ( Private )
  {
    if ( nds_logging_check( NDS_LOG_GROUP_CONNECTION, 10 ) )
    {
      nds_logging_printf( "INFO: %s: sockfd: %d\n",
			  METHOD,
			  Private->sockfd );
    }
    if ( Private->datafd !=  INVALID_SOCKET )
    {
      if ( shutdown ( Private->datafd, SHUT_RDWR ) )
      {
	retval = DAQD_SHUTDOWN;
      }
      if ( Private->datafd != Private->sockfd )
      {
#if _WIN32
	if ( closesocket( Private->datafd ) )
	{
	  retval = DAQD_SHUTDOWN;
	}
#else
	if ( close( Private->datafd ) )
	{
	  retval = DAQD_SHUTDOWN;
	}
#endif /* _WIN32 */
      }
      Private->datafd = INVALID_SOCKET;
    }
  }
  if ( nds_logging_check( NDS_LOG_GROUP_TRACE_ENTRY_EXIT, 10 ) )
  {
    nds_logging_printf( "INFO: %s: EXIT: %d\n",
			METHOD,
			retval );
  }
  return retval;
}

