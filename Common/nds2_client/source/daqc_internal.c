/* -*- mode: c; c-basic-offset: 4; -*- */
#if HAVE_CONFIG_H
#include "daq_config.h"
#endif /* HAVE_CONFIG_H */

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#if HAVE_UNISTD_H
#include <unistd.h>
#else
#include <io.h>
#endif /* HAVE_UNISTD_H */
#if HAVE_NETDB_H
#include <netdb.h>
#endif /* HAVE_NETDB_H */
#if HAVE_INTTYPES_H
#include <inttypes.h>
#else
#include <stdint.h>
#endif

#include "channel.h"
#include "daqc.h"
#include "daqc_private.h"
#include "daqc_response.h"
#include "daqc_internal.h"
#include "daqc_net.h"
#include "nds_logging.h"
#include "nds_os.h"

/*--------------------------------------------------------------------
 *
 *    Select includes depend on posix versions
 *--------------------------------------------------------------------*/
#if _POSIX_C_SOURCE < 200100L
#if HAVE_SYS_TIME_H
#include <sys/time.h>
#endif /* HAVE_SYS_TIME_H */
#include <sys/types.h>
#else
#include <sys/select.h>
#endif

/*  Swap the specified data in 
 */
#if __GNUC__
#pragma GCC diagnostic ignored "-Wconversion"
#endif /* __GCC */
size_t
swap_data(size_t gran, size_t nBytes, char* data) {
    uint16_t one = 1;
    size_t i, n=0;

    if ( (gran < 2) || (ntohs(one) == one) )
    {
	return nBytes/gran;
    }
    if (gran == 2 && !((long)data & 1)) {
	uint16_t* p = (uint16_t*)data;
	for (i=0; i<nBytes; i+=gran) {
	    *p = ntohs(*p);
	    p++; n++;
	}
    } else if (gran == 4 && !((long)data & 3)) {
	uint32_t* p = (uint32_t*)data;
	for (i=0; i<nBytes; i+=gran) {
	    *p = ntohl(*p);
	    p++; n++;
	}
    } else {
	for (i=0; i<nBytes; i += gran) {	    
	    char* head = data + i;
	    char* tail = head + gran;
	    while (head < tail) {
		char t  = *head;
		*head++ = *(--tail);
		*tail   = t;
	    }
	    n++;
	}
    }
    return n;
}
#if __GNUC__
#pragma GCC diagnostic warning "-Wconversion"
#endif /* __GCC */

/* convert nds1 group number to nds2 type code.
 */
enum chantype
cvt_group_chantype(int group_num) {
    switch (group_num) {
    case 1000:
	return cMTrend;
    case 0:
	return cOnline;
    default:
	break;
    }
    return cUnknown;
}

/**  Convert a null terminated hex string to a long integer.
 *  \param str Pointer to hexidecimal string.
 *  \return Value of hex string or -1 if syntax error.
 */
long
dca_strtol (const char *str) {
    char *ptr;
    long res = strtol (str, &ptr, 16);
    if (*ptr) return -1;
    return res;
}

/*  Wait for data on the specified port. 
 *  return values:
 *   1 Data available
 *   0 Timeout.
 *  -1 Error
 */
int
_daq_wait_data(nds_socket_type fd, int wt) {
    static const char* METHOD = "_daq_wait_data";
    int rc = -1;
    int nfds =
#if _WIN32
	1
#else /* _WIN32 */
	( fd + 1 )
#endif /* _WIN32 */
	;

    struct timeval tspec, *tptr;
    fd_set readfds;

    tspec.tv_sec  = wt;
    tspec.tv_usec = 0;
    tptr = &tspec;

    FD_ZERO(&readfds);
    FD_SET(fd, &readfds);

    while (rc < 0) {
	errno = 0;
	rc = select(nfds, &readfds, 0, 0, tptr);
#if _WIN32
	if ( rc == SOCKET_ERROR )
	{
	    if ( nds_logging_check( NDS_LOG_GROUP_VERBOSE_ERRORS, 20 ) )
	    {
		/*
		 * Only show the error message if something went wrong
		 *   and logging is requested
		 */
		nds_logging_print_errno( METHOD );
	    }
	    rc = -1;
	    break;
	}
#else /* _WIN32 */
	if ( ( rc < 0 ) 
	     && ( nds_logging_check( NDS_LOG_GROUP_VERBOSE_ERRORS, 20 ) ) )
	{
	    /*
	     * Only show the error message if something went wrong
	     *   and logging is requested
	     */
	    nds_logging_print_errno( METHOD );
	}
	if (rc < 0 && errno != EINTR)
	{
	    break;
	}
#endif /* _WIN32 */
    }
    if (!rc) {
	fprintf(stderr, "Timeout in _daq_wait_data\n");
    } else if (rc < 0) {
	perror("Error in _daq_wait_data()");
    }
    return rc;
}

/* 
 *   Read `numb' bytes.
 *   Returns number of bytes read or 0 on the error or EOF
 */
int
read_bytes (nds_socket_type fd, char *cptr, size_t numb) {
    static const char* METHOD = "read_bytes";
    size_t oread = 0;
    while (oread < numb) {
#if _WIN32
	register const int	bytes_to_read = (int)(numb-oread);
#else /* _WIN32 */
	register const size_t	bytes_to_read = (numb-oread);
#endif /* _WIN32 */

	size_t bread = read (fd, cptr+oread, bytes_to_read);
	if (bread > 0) {
	    oread += (size_t) bread;
	} else if (!bread) {
	    break;
	} else if (errno != EAGAIN) {
	    if ( nds_logging_check( NDS_LOG_GROUP_VERBOSE_ERRORS,
				    0 ) )
	    {
		char	pmsg[ 256 ];

		strerror_r( errno, pmsg, sizeof( pmsg ) );
		
		nds_logging_printf( "ERROR: %s: read( ) error: %s\n",
				    METHOD,
				    pmsg );
	    }
	    return 0;
        }
    }
    /* printf("read(%i,...,%i) = %i\n", fd, numb,oread); */
    return (int)oread;
}

/* Read server response -- 4 bytes */
/* Convert the response to integer */
/* Return -1 for invalid response */
long
read_server_response_wait(nds_socket_type fd, int wt) {
    if (wt > 0 && _daq_wait_data(fd, wt) <= 0) {
	perror("read_server_response_wait: Error waiting for data");
	return -1;
    }
    return read_server_response(fd);
}

long 
read_server_response (nds_socket_type fd) { 
#define response_length 4
    char buf [response_length + 1];
    int rc = read_bytes(fd, buf, (size_t)response_length);
    if (rc != response_length) {
	if (rc < 0) perror("read_server_response: Error in read");
	else fprintf(stderr, "read_server_response: Wrong length read (%i)\n",
		     rc);
	return -1;
    }
    buf [response_length] = '\000';
    return dca_strtol (buf);
#undef response_length
}

/*  read_float(nds_socket_type fd, float*)
 *
 *  read a float from the indicated fd and swap bytes from network
 *  byte ordering (big-endian) to host byte ordering.
 */
int
read_float(nds_socket_type fd, float* data) {
    union {uint32_t i; float f;} d;
    if (read_uint4(fd, &d.i)) return DAQD_ERROR;
    *data = d.f;
    return DAQD_OK;
}

/*  Read a 4-byte `long' integer. The byte ordering is swapped on little-
 *  endian machines.
 */
int
read_uint4 (nds_socket_type fd, uint4_type* data) {
    uint4_type buf;
    if (read_bytes(fd, (char*)&buf, sizeof(buf)) != sizeof(buf)) 
	return DAQD_ERROR;
    *data = ntohl(buf);
    return DAQD_OK;
}

/*
 *  Read an NDS2 byte-counted string from the specified fd. The string is 
 *  transferred as an integer byte-count followed by text.
 */
int
_daq_read_string(nds_socket_type fd, size_t maxlen, char* buf) {
    int rc;
    uint4_type tmp = 0;
    size_t len;
    if (read_uint4(fd, &tmp)) return -1;
    len = tmp;
    if (len == 0) {
	if (maxlen && buf) *buf = 0;
	return (int)len;
    }
    if (len > maxlen) rc = read_bytes(fd, buf, maxlen);
    else              rc = read_bytes(fd, buf, len);
    if (rc < 0) return -1;
    if ((size_t)rc < maxlen) buf[rc] = 0;
    return (int)len;
}

/*
 *  Read an NDS null terminated string from the specified fd.
 *  The string is transferred as a null-terminated  nt followed by text.
 */
int
_daq_read_cstring(nds_socket_type fd, size_t maxlen, char* buf) {
#if _WIN32
    static const int	bytes_to_read = 1;
#else /* _WIN32 */
    static const size_t	bytes_to_read = 1;
#endif /* _WIN32 */
    int			rc = 0;
    size_t		len = 0;
    char*		pos = buf;

    while( ( len < ( maxlen - 1) ) &&
	   ( ( rc = read( fd, pos, bytes_to_read ) ) == 1 ) )
    {
	if ( *pos == '\n' )
	{
	    break;
	}
	++pos;
	++len;
    }
    *pos = '\0';
    if ( ( len == 0 ) && ( rc < 0 ) )
    {
	return -1;
    }
    return (int)len;
}

int
null_term(char* p, int len) {
    int i;
    for (i=len-1; i; --i) {
	if (!isspace(p[i-1])) {
	    p[i] = 0;
	    return i;
	}
    }
    *p = 0;
    return 0;
}

/** read data from string pointed to by *p. skip past any leading spaces.
  * copy all data to out until a space or a null is found or out is full. 
  * return the number of characters copied and increment *p as appropriate.
  */
size_t 
_daq_get_string(const char** p, const char* end, char* out, size_t lmax) {
    size_t i;
    const char* in  = *p;
    while (in < end && *in == ' ') in++;
    for (i=0; i<lmax && in < end; i++) {
	if (*in == ' ') break;
	out[i] = *in++;
    }
    *p = in;
    if (i < lmax) out[i]      = 0;
    return i;
}

/*  _daq_cvt_string(unsigned long, char*)
 *
 *  convert the specified unsigned long int to a string
 */
int
_daq_cvt_string(unsigned long in, char* out) {
    int n = 0;
    if (in >= 10)
    {
	n = _daq_cvt_string(in/10, out);
    }
    out[n] = (char)('0' + (in%10));
    return n+1;
}

/*  _daq_cvt_ll_string(unsigned long long, char*)
 *
 *  convert the specified unsigned long long int to a string
 */
int
_daq_cvt_ll_string(unsigned long long in, char* out) {
    int n = 0;
    if (in >= 10) {
	n = _daq_cvt_ll_string(in/10, out);
    }
    out[n] = (char)('0' + (in%10));
    return n+1;
}


/* Return the number of secons spanned by the data in
 * the block.
 */
uint4_type
daq_get_block_secs( daq_t* daq )
{
    return daq->tb->secs;
}

/* Return time stamp second field of the first
 *  sample in the block
 */
uint4_type
daq_get_block_gps( daq_t* daq )
{
    return daq->tb->gps;
}

/* Return time stamp nano-second field of the first
 *  sample in the block
 */
uint4_type
daq_get_block_gpsn( daq_t* daq )
{
    return daq->tb->gpsn;
}

/* Return the block sequence number; shows if any blocks
 *  were dropped.
 */
uint4_type
daq_get_block_seq_num( daq_t* daq )
{
    return daq->tb->seq_num;
}


/* Return starting address of the data in the block.
 */
char*
daq_get_block_data( daq_t* daq )
{
    return daq->tb->data;
}

/*
 *  Convert the specified host name and port number into an ip address
 */
#if __GNUC__
#pragma GCC diagnostic ignored "-Wconversion"
#endif /* __GCC */
int
daq_set_server(daq_t *daq, const char* host, int port)
{
    static const char* METHOD = "daq_set_server";
    socket_addr_t*
	srvr_addr = daq->conceal->srvr_addr;

#if defined(__linux__) || defined(__APPLE__) || defined(_WIN32)
    struct hostent* hentp = gethostbyname (host);
    if (!hentp) {
	fprintf (stderr, "Can't find hostname `%s'\n", host);
	if ( nds_logging_check( NDS_LOG_GROUP_VERBOSE_ERRORS, 20 ) )
	{
	    char	pmsg[ 256 ];

	    strerror_r( errno, pmsg, sizeof( pmsg ) );
	    nds_logging_printf("ERROR: %s: gethostbyname() error: %s\n",
			       METHOD,
			       pmsg );
	}
	return DAQD_INVALID_IP_ADDRESS;
    }
    memcpy(&srvr_addr->sin_addr.s_addr,
	   *hentp -> h_addr_list, 
	   sizeof (srvr_addr->sin_addr.s_addr));

#else
#define HOST_BUF_LENGTH 2048
    struct hostent hent;
    char buf [HOST_BUF_LENGTH];
    int gherr;
    if (! gethostbyname_r (host, &hent, buf, HOST_BUF_LENGTH, &gherr)) {
	fprintf (stderr, "Can't find hostname `%s'\n", host);
	if ( nds_logging_check( NDS_LOG_GROUP_VERBOSE_ERRORS, 20 ) )
	{
	    char	pmsg[ 256 ];

	    strerror_r( errno, pmsg, sizeof( pmsg ) );
	    nds_logging_prtintf( "ERROR: %s: gethostbyname_r() error: %s\n",
				 METHOD,
				 pmsg );
	}
	return DAQD_INVALID_IP_ADDRESS;
    }

    memcpy(&daq->srvr_addr->sin_addr.s_addr, *hent.h_addr_list, 
	   sizeof(daq->srvr_addr->srsin_addr.s_addr));
#endif

    srvr_addr->sin_family = AF_INET;
    /*  srvr_addr->sin_addr.s_addr = inet_addr (ip); */
    srvr_addr->sin_port = htons ( port ? port : DAQD_PORT );
    return DAQD_OK;
}
#if __GNUC__
#pragma GCC diagnostic warning "-Wconversion"
#endif /* __GCC */

/*--------------------------------------------------------------------------*
 *                                                                          *
 *         Obsolete functions                                               *
 *                                                                          *
 *--------------------------------------------------------------------------*/

/*  daq_recv_channel_groups()
 *
 *  Get channel groups description
 *  Returns zero on success.
 *  Sets `*num_channel_groups_received' to the actual number of 
 *  configured channel groups.
 */
int
daq_recv_channel_groups (daq_t *daq, daq_channel_group_t *group,
			 int num_groups, int *num_channel_groups_received)
{
    int i;
    int resp;
    int groups;

fprintf( stderr,
	 "daq_recv_channel_groups: entry\n" );
    if ((resp = daq_send (daq, "status channel-groups;")))
	return resp;

    /* Read the number of channels */
    if ( ( groups
	   = read_server_response (daq -> conceal -> sockfd)) <= 0) {
	fprintf (stderr, "couldn't determine the number of channel groups\n");
	return DAQD_ERROR;
    }

fprintf( stderr,
	 "daq_recv_channel_groups: num_groups: %d groups: %d\n",
	 num_groups, groups );

    *num_channel_groups_received = groups;
    if (num_groups < groups)
	groups = num_groups;

    for (i = 0; i < groups; i++) {
	int len=read_bytes(daq->conceal->sockfd,
			   group[i].name, 
			   (size_t)MAX_CHANNEL_NAME_LENGTH);
	if (len != MAX_CHANNEL_NAME_LENGTH)
	    return DAQD_ERROR;
	null_term(group [i].name, len);

	group [i].group_num
	    = read_server_response (daq -> conceal -> sockfd);
	if (group [i].group_num < 0) return DAQD_ERROR;
    }
    return 0;
}

#if _WIN32
#undef perror
void
nds_perror( const char* Message )
{
    if ( errno != 0 )
    {
	perror( Message );
	return;
    }
    
    fprintf( stderr,
	     "%s: Error: %d\n",
	     Message,
	     WSAGetLastError( ) );
    fflush( stderr );
}

void
pstrerror_r( char* StrErrBuf,
		 size_t StrErrBufLen )
{
    if ( errno != 0 )
    {
	strerror_r( errno,
		    StrErrBuf,
		    StrErrBufLen );
    }
    else
    {
	sprintf_s( StrErrBuf, StrErrBufLen,
		   "%s", WSAGetLastError( ) );
    }
}

#else

void
pstrerror_r( char* StrErrBuf,
	     size_t StrErrBufLen )
{
    strerror_r( errno,
		StrErrBuf,
		StrErrBufLen );
}


#endif /* _WIN32 */
