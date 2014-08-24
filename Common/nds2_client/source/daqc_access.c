/* -*- mode: c; c-basic-offset: 4; -*- */
#if HAVE_CONFIG_H
#include "daq_config.h"
#endif /* HAVE_CONFIG_H */

#include <ctype.h>

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */
#if HAVE_INTTYPES_H
#include <inttypes.h>
#else
#include "stdint.h"
#endif /* HAVE_INTTYPES_H */
#if HAVE_UNISTD_H
#include <unistd.h>
#else
#include <io.h>
#endif

#include "daqc.h"
#include "daqc_private.h"
#include "daqc_response.h"
#include "daqc_internal.h"
#include "daqc_net.h"
#include "nds_logging.h"
#include "nds_os.h"
#include "nds1.h"
#include "nds2.h"
#include "trench.h"

/***************************************************************************
 *                                                                         *
 *  Note that this code was moved here from daq_internal.h because it      *
 *  uses macros that are only defined during the library compilation       *
 *  and was therefore preventing external packages from compiling against  *
 *  nds2-client. This code should probably find a permanent home in a      *
 *  truely internal header, in daq_config.h or some similar location.      *
 *                                                                         *
 ***************************************************************************/

#if SIZEOF_SIZE_T == SIZEOF_INT
#define PRISIZE_T "u"
#elif SIZEOF_SIZE_T == SIZEOF_LONG
#define PRISIZE_T "lu"
#elif SIZEOF_SIZE_T == SIZEOF_LONG_LONG
#define PRISIZE_T "llu"
#else
#error Unable to figure out how to print size_t
#endif

/**************************************************************************/

#define TRACE_ENTRY_EXIT 0

#ifndef SHUT_RDWR
#ifdef SD_BOTH
#define SHUT_RDWR SD_BOTH
#endif /* SD_BOTH */
#endif /* SHUT_RDWR */

#ifndef SIZEOF_SIZE_T
#error  Size of size_t is unknown
#endif

/*  Channel data type conversion functions
 */
size_t
data_type_size (daq_data_t dtype) {
    switch (dtype) {
    case _16bit_integer: // 16 bit integer
	return 2;
    case _32bit_integer: // 32 bit integer
    case _32bit_float: // 32 bit float
	return 4;
    case _64bit_integer: // 64 bit integer
    case _64bit_double: // 64 bit double
	return 8;
    case _32bit_complex: // 32 bit complex
	return 4*2;
    default:
	return 1;
    }
}

/*   Specify the word size for swapping purposes. */
size_t
data_type_word (daq_data_t dtype) {
    switch (dtype) {

    /*  data types with 2-byte word length. */
    case _16bit_integer:
	return 2;

    /*  data types with 4-byte word length. */
    case _32bit_integer:
    case _32bit_float:
    case _32bit_complex:
	return 4;

    /*  data types with 8-byte word length. */
    case _64bit_integer:
    case _64bit_double:
	return 8;

    /*  innocuous value for undefined data types */
    default:
	return 1;
    }
}

double
data_type_max(daq_data_t dtype) {
    switch (dtype) {
    case _16bit_integer: // 16 bit integer
	return INT16_MAX;
    case _32bit_integer: // 32 bit integer
	return INT32_MAX;
    case _32bit_float:   // 32 bit float
    case _32bit_complex: // 32 bit complex (2^127)
	return 1e38;
    case _64bit_integer: // 64 bit integer
	return (double)INT64_MAX;
    case _64bit_double:  // 64 bit double (2^1023)
	return 8e307;
    default:
	return _undefined;
    }
}

const char*
data_type_name(daq_data_t dtype) {
    switch (dtype) {
    case _16bit_integer: // 16 bit integer
	return "int_2";
    case _32bit_integer: // 32 bit integer
	return "int_4";
    case _32bit_float: // 32 bit float
	return "real_4";
    case _64bit_integer: // 64 bit integer
	return "int_8";
    case _64bit_double: // 64 bit double
	return "real_8";
    case _32bit_complex: // 32 bit complex
	return "complex_8";
    default:
	return "undefined";
    }
}

daq_data_t
data_type_code(const char* name) {
#define TEST_NAME(x) if (!strcmp(name, data_type_name(x))) return x
    TEST_NAME(_16bit_integer);
    TEST_NAME(_32bit_integer);
    TEST_NAME(_32bit_float);
    TEST_NAME(_64bit_integer);
    TEST_NAME(_64bit_double);
    TEST_NAME(_32bit_complex);
#undef TEST_NAME
    return _undefined;
}


/*
 *  Disconnect from the server and close the socket file descriptor
 */
int
daq_disconnect (daq_t *daq) {
    int rc = 0;
    switch(daq->nds_versn) {
    case nds_v1:
	rc = nds1_disconnect(daq);
	break;
    case nds_v2:
	rc = nds2_disconnect(daq);
	break;
    default:
	rc = DAQD_NOT_CONFIGURED;
    }
    if ( daq )
    {
	daq_private_srvr_disconnect( daq->conceal );
	daq_private_delete( &(daq->conceal) );

    }
    return rc;
}

/*
 *  Connect to the DAQD server on the host identified by `ip' address.
 *  Returns zero if OK or the error code if failed.
 */
int
daq_connect (daq_t *daq, const char *host, int port, enum nds_version vrsn)
{
    int rc = 0;

    /*  Set the version number
     */
    daq -> nds_versn = vrsn;


    /*  Initialize the data buffer, conversion list and channel request list.
     */
    daq -> tb_size = 0;
    daq -> tb      = NULL;

    daq -> s_size = 0;
    daq -> s      = NULL;

    daq -> num_chan_request = 0;
    daq -> num_chan_alloc   = 0;
    daq -> chan_req_list    = NULL;
    daq -> auth_ctx         = NULL;


    daq_private_create( &(daq->conceal) );

    /*  Perform version-specified processing.
     */
    switch(vrsn) {
    case nds_try:
	/*---------------     Try nds2 first                         */
	rc = nds2_connect(daq, host, port);
	if (!rc) {
	    daq -> nds_versn = nds_v2;
	} 

	/*---------------     Bail if authentication failed          */
	else if (rc == DAQD_SASL) {
	    break;
	}

	/*---------------     Try nds1 if there was a protocol error */
	else if (rc != DAQD_INVALID_IP_ADDRESS && 
		 rc != DAQD_CONNECT) {
	    rc = nds1_connect(daq, host, port);
	    if (!rc) daq -> nds_versn = nds_v1;
	}
	break;
    case nds_v1:
	rc = nds1_connect(daq, host, port);
	break;
    case nds_v2:
	rc = nds2_connect(daq, host, port);
	break;
    }
    return rc;
}

/*  daq_send (daq_t *daq, char *command)
 *  Send `command' to DAQD, read and return response code
 */
int
daq_send (daq_t *daq, const char *command)
{
#if _WIN32
typedef int write_len_t;
#else /* _WIN32 */
typedef size_t write_len_t;
#endif /* _WIN32 */
    static const char* METHOD = "daq_send";
    register size_t	cmdl;
    int			resp;

    if ( nds_logging_check( NDS_LOG_GROUP_VERBOSE_ERRORS,
			    0 )
	 && nds_logging_check( NDS_LOG_GROUP_TRACE_ENTRY_EXIT,
			       0 ) )
    {
	nds_logging_printf( "INFO: %s: entry\n",
			    METHOD );
    }
    cmdl = strlen( command );

    if ( write( daq->conceal->sockfd, command, (write_len_t)( cmdl ) ) != cmdl )
    {
	perror("Error in write()");
	return DAQD_WRITE;
    }

    if (write (daq->conceal->sockfd, "\n", (size_t)1) != 1) {
	perror("Error in write()");
	return DAQD_WRITE;
    }

    /* Read server response */
    resp = (int)( read_server_response_wait (daq->conceal->sockfd, 3) );
    if ( nds_logging_check( NDS_LOG_GROUP_VERBOSE_ERRORS,
			    0 ) )
    {
	if ( ( resp > 0 )
	     && ( resp != DAQD_SASL ) )
	{
	    nds_logging_printf( "ERROR: %s: Server response error: %s, command: %s\n",
				METHOD,
				daq_strerror(resp),
				command );
	}
	else if (resp < 0)
	{
	    nds_logging_printf( "ERROR: %s: Server error: %s\n",
				METHOD,
				daq_strerror(resp) );
	}
    }
    if ( nds_logging_check( NDS_LOG_GROUP_VERBOSE_ERRORS,
			    0 )
	 && nds_logging_check( NDS_LOG_GROUP_TRACE_ENTRY_EXIT,
			       0 ) )
    {
	nds_logging_printf( "INFO: %s: exit\n",
			    METHOD );
    }
    return resp;
}

/*  daq_recv_block (daq_t *daq)
 *
 *  Receive one data block (data channel samples). 
 *
 *  Data block is malloced and realloced as needed and pointer is assigned to
 *  `daq -> tb'. Block size is assigned to `daq -> tb_size'.
 *
 *  Returns 0 if zero length data block is received. Zero length block consists
 *  of the block header and no data. It is sent by the server when it failed to
 *  find the data for the second, specified in the block header. This could
 *  only happen for the off-line data request.
 *
 *  Returns -1 on error.
 *
 *  Returns -2 on channel reconfiguration. Special reconfiguration data block 
 *  was received from the server. For the client it meens he need to reread 
 *  channel data conversion variables and status from the *daq structure.
 *
 *  Returns number bytes of sample data read otherwise.
 */
int
daq_recv_block (daq_t *daq) {
    static const char* METHOD = "daq_recv_block";
    register nds_socket_type fd;
    size_t block_len;
    uint4_type seconds, tmp;
    size_t bsize;
    daq_block_t* ptb;
    long oread;

    /* read block length */
    fd = daq->conceal->datafd;

    daq->err_num = read_uint4 (fd, &tmp);
    block_len = tmp;
    if (daq->err_num) {
	if ( nds_logging_check( NDS_LOG_GROUP_VERBOSE_ERRORS,
				0 ) )
	{
	    nds_logging_printf( "ERROR: %s: Error reading block length\n",
				METHOD );
	}
	return -1;
    }

    daq->err_num = read_uint4 (fd, &seconds);
    if (daq->err_num) {
	if ( nds_logging_check( NDS_LOG_GROUP_VERBOSE_ERRORS,
			  0 ) )
	{
	    nds_logging_printf( "ERROR: %s: Error reading start time\n",
				METHOD );
	}
	return -1;
    }

    /* channel reconfiguration block (special block that's not data) */
    if (seconds == 0xffffffff) {
	/* finish reading the header */
	uint4_type dum;
	read_uint4 (fd, &dum);
	read_uint4 (fd, &dum);
	read_uint4 (fd, &dum);
	block_len -= HEADER_LEN;

	switch (daq->nds_versn) {
	case nds_v1:
	    return nds1_receive_reconfigure(daq, block_len);
	case nds_v2:
	    return nds2_receive_reconfigure(daq, block_len);
	default:
	    daq->err_num = DAQD_NOT_CONFIGURED;
	    return -1;
	}
    }

    bsize = sizeof(daq_block_t) + block_len;

    ptb = daq->tb;
    if (! ptb) {
	ptb = (daq_block_t *) malloc (bsize);
	daq->tb = ptb;
	if (ptb) {
	    daq -> tb_size = bsize;
	} else {
	    if ( nds_logging_check( NDS_LOG_GROUP_VERBOSE_ERRORS,
				    0 ) )
	    {
		nds_logging_printf ( "ERROR: %s: malloc(%" PRISIZE_T ") failed; errno=%d\n",
				     METHOD, bsize, errno);
	    }
	    daq -> tb_size = 0;
	    daq->err_num = DAQD_MALLOC;
	    return -1;
	}
    } else if (daq -> tb_size < bsize) {
	ptb = (daq_block_t *) realloc ((void *) ptb, bsize);
	daq->tb = ptb;

	if (ptb) {
	    daq -> tb_size = bsize;
	} else {
	    if ( nds_logging_check( NDS_LOG_GROUP_VERBOSE_ERRORS,
				    0 ) )
	    {
		nds_logging_printf ( "ERROR: %s: realloc(%" PRISIZE_T ") failed; errno=%d\n",
				     METHOD, bsize, errno);
	    }
	    daq->tb_size = 0;
	    daq->err_num   = DAQD_MALLOC;
	    return -1;
	}
    }

    ptb -> secs = seconds;
    read_uint4(fd, &ptb->gps);
    read_uint4(fd, &ptb->gpsn);
    read_uint4(fd, &ptb->seq_num);

    /*  block length does not include the length of itself while the 
     *  header length does 
     */
    block_len -= HEADER_LEN;

    /* zero length block received */
    if (! block_len) return 0;
 
    /* read data samples */
    oread = read_bytes(fd, ptb->data, block_len);
    if (!oread) {
	daq->err_num = DAQD_ERROR;
	return -1;
    }
    return (int)( oread );
}

/*
 *  This returns 0 for online data feed and positive number for off-line.
 */
int
daq_recv_block_num (daq_t *daq) {
    uint4_type bnum;
    if (read_uint4(daq->conceal->datafd, &bnum))
    {
	return -1;
    }
    /*printf("Received block number: %i\n", bnum);*/
    return (int)bnum;
}

/*
 *    Receive one block of data. Skip processed reconfiguration blocks.
 *    Swap the bytes of the data received.
 */
int
daq_recv_next (daq_t *daq) {
    static const char* METHOD = "daq_recv_next";
    uint4_type i, N, off;
    size_t blen;
    long dt;

    /*----------------------------------  Receive, loop over reconfigure blocks
     */
    int rc = 0;
    do {
	rc = daq_recv_block(daq);
    } while (rc < -1);
    if ( nds_logging_check( NDS_LOG_GROUP_VERBOSE_ERRORS,
			    0 ) )
    {
	nds_logging_printf( "ERROR: %s: rc: %d\n",
			    METHOD, rc );
    }

    N = daq->num_chan_request;
    if (rc <= 0) {
	if (!rc) rc = DAQD_NOT_FOUND;
	else     rc = daq->err_num;
	for (i=0; i<N; ++i) {
	    daq->chan_req_list[i].status = -rc;
	}
	if ( nds_logging_check( NDS_LOG_GROUP_VERBOSE_ERRORS,
				0 ) )
	{
	    nds_logging_printf( "ERROR: %s: rc: %d\n",
				METHOD, rc );
	}
	return rc;
    }
    blen = (size_t)rc;
    rc = DAQD_OK;

    /*----------------------------------  Swap bytes and fill in the status.
     */
    dt  = (long) daq->tb->secs;
    off = 0;
    for (i=0; i<N; ++i) {
	daq->chan_req_list[i].offset = off;
	if (daq->chan_req_list[i].status >= 0) {
	    size_t len;
	    daq_data_t typ = daq->chan_req_list[i].data_type;

	    /*----  Get the number of words for this channel. 
	     *      Note that rates less than 1Hz (only minute trends at 
	     *	    this time) are assume to be an integer fraction of 1 Hz.
	     *-------------------------------------------------------------*/
	    size_t nwd;
	    if (daq->chan_req_list[i].rate < 1) {
		long rinv = (long) ( 1.0/daq->chan_req_list[i].rate + 0.5);
		nwd = (size_t) (dt / rinv);
	    } else {
		nwd = (size_t) (dt * daq->chan_req_list[i].rate);
	    }

	    len = nwd * data_type_size(typ);
	    if ( nds_logging_check( NDS_LOG_GROUP_VERBOSE_ERRORS,
				    0 ) )
	    {
		nds_logging_printf("ERROR: %s: Returned data name: %s rate: %f type: %i\n",
				   METHOD,
				   daq->chan_req_list[i].name, 
				   daq->chan_req_list[i].rate, 
				   typ);
	    }
	    if (rc || off + len > blen) {
		daq->chan_req_list[i].status = -rc;
		rc = DAQD_ERROR;
		if ( nds_logging_check( NDS_LOG_GROUP_VERBOSE_ERRORS,
					0 ) )
		{
		    nds_logging_printf( "ERROR: %s: Wrong block length!\n",
					METHOD );
		}
	    } else {
		daq->chan_req_list[i].status = (int)len;
		swap_data(data_type_word(typ), len, daq->tb->data + off);
		off += (uint4_type)len;
	    }
	}
    }

    if (!rc && off != blen) {
	if ( nds_logging_check( NDS_LOG_GROUP_VERBOSE_ERRORS,
				0 ) )
	{
	    nds_logging_printf( "ERROR: %s: Data length mismatch, block-len=%" PRISIZE_T ", request-len=%u\n",
				METHOD, blen, off );
	}
	rc = DAQD_ERROR;
    }
    return rc;
}

/*
 *  Close data connection socket 
 */
int
daq_recv_shutdown (daq_t *daq)
{
    int rc = DAQD_OK;
    if (daq -> tb)
	free (daq -> tb);

    daq -> tb = 0;
    daq -> tb_size = 0;

    if (daq -> s)
	free (daq -> s);

    daq -> s = 0;
    daq -> s_size = 0;

    daq_clear_channel_list(daq);
    if (daq -> chan_req_list)
	free (daq -> chan_req_list);
    daq -> num_chan_request = 0;
    daq -> num_chan_alloc   = 0;

    /*-------------------------  Close the  socket ---------------*/
    {
	int status = DAQD_OK;

	status = daq_private_data_close( daq->conceal );
	if ( status != DAQD_OK )
	{
	    rc = status;
	}
    }
    return rc;
}

/*
 *  Get channel descriptors and store them in the list provided. Set
 *  `*num_channels_received' to the number of configured channels.
 *  Returns a DAQD error code.
 */
int
daq_recv_channels (daq_t *daq, daq_channel_t *channel,
		   int num_channels, int *num_received) {
    return daq_recv_channel_list(daq, channel, num_channels, num_received,
				 (time_t)0, cUnknown);
}

/*
 *  Get a list of channel names.
 */
int
daq_recv_channel_list(daq_t *daq, daq_channel_t *channel, int num_channels, 
		      int *num_received, time_t gps, enum chantype type) 
{
    int rc = DAQD_OK;
    switch(daq->nds_versn) {
    case nds_v1:
	rc = nds1_recv_channels(daq, channel, num_channels, num_received);
	break;
    case nds_v2:
	rc = nds2_recv_channel_list(daq, channel, num_channels, num_received,
				    gps, type);
	break;
    default:
	rc = DAQD_NOT_CONFIGURED;
    }
    return rc;
}

/*  daq_recv_id (daq_t *daq)
 *
 *  Receive some ID, coded as two server responses, i.e. two groups, 
 *  four hex digits in each
 */
#define SERVER_ID_LENGTH 8
long
daq_recv_id (daq_t *daq) {
    char buf [SERVER_ID_LENGTH + 1];
    int rc = read_bytes(daq->conceal->sockfd, buf, (size_t)SERVER_ID_LENGTH);
    if (rc != SERVER_ID_LENGTH) {
	if (rc < 0) perror("daq_recv_id: Error in read");
	else nds_logging_printf("daq_recv_id: Wrong length read (%i)\n", rc);
	return -1;
    }
    buf [SERVER_ID_LENGTH] = '\000';
    return dca_strtol (buf);
}

/*
 *  Get a list of channel names.
 */
int
daq_recv_source_list(daq_t *daq, char* list, size_t max_len, 
		     time_t gps, long* str_len) 
{
    int rc = DAQD_OK;
    switch(daq->nds_versn) {
    case nds_v1:
	rc = DAQD_VERSION_MISMATCH;
	break;
    case nds_v2:
	rc = nds2_recv_source_list(daq, list, max_len, gps, str_len);
	break;
    default:
	rc = DAQD_NOT_CONFIGURED;
    }
    return rc;
}

/**********************************************************************
 *                                                                    *
 *    Channel list management functions                               *
 *                                                                    *
 **********************************************************************/
#define DAQ_REQUEST_BLOCK_SIZE 16

/*  get_request_entry(daq_t*)
 *
 *  Get a pointer to a new channel request entry in the daq channel request
 *  list. If a free entry is available the pointer is returned and the 
 *  of allocated entries is incremented. If no free entries are available,
 *  a new block of entries is allocated, and any existing requests are copied
 *  to the new memory.
 */
static chan_req_t*
get_request_entry(daq_t* daq) {
    if ( daq->num_chan_request >= daq->num_chan_alloc) {
	chan_req_t* pnew;

	daq->num_chan_alloc += DAQ_REQUEST_BLOCK_SIZE;
	pnew = malloc(sizeof(chan_req_t) * daq->num_chan_alloc);
	if (!pnew) return pnew;
	if (daq->num_chan_request > 0) {
	    memcpy(pnew, 
		   daq->chan_req_list, 
		   daq->num_chan_request*sizeof(chan_req_t)
		   );
	    free(daq->chan_req_list);
	}
	daq->chan_req_list = pnew;
    }
    return daq->chan_req_list + daq->num_chan_request++;
}

/*  daq_request_channel(daq_t*, const char* name, enum chantype type, 
 *                      double rate)
 *
 *  Add an entry to the channel request list.
 *  Get a pointer to a new channel request entry in the daq channel request
 *  list. If a free entry is available the pointer is returned and the number 
 *  of allocated entries is incremented. If no free entries are available,
 *  a new block of entries is allocated, and any existing requests are copied
 *  to the new memory.
 */
int
daq_request_channel(daq_t* daq, const char* name, enum chantype type,
		    double rate) {
    daq_channel_t chan;
    daq_init_channel(&chan, name, type, rate, _undefined);
    daq_request_channel_from_chanlist(daq, &chan);
    return DAQD_OK;
}

/*  daq_request_channel_from_chanlist(daq_t*, daq_channel_t*)
 *
 *  Add an entry to the channel request list. The channel added is described
 *  by the specified channel list entry.
 */
int
daq_request_channel_from_chanlist(daq_t* daq, daq_channel_t* pchan) {
    chan_req_t* pnew = get_request_entry(daq);
    if (!pnew) return DAQD_MALLOC;
    pnew->name      = strdup(pchan->name);
    if (!pnew->name) return DAQD_MALLOC;
    pnew->type      = pchan->type;
    pnew->rate      = pchan->rate;
    pnew->data_type = pchan->data_type;
    pnew->offset    = 0;
    pnew->status    = 0;
    pnew->s.signal_gain   = pchan->s.signal_gain;
    pnew->s.signal_offset = pchan->s.signal_offset;
    pnew->s.signal_slope  = pchan->s.signal_slope;
    strcpy_s(pnew->s.signal_units, sizeof( pnew->s.signal_units ),
	     pchan->s.signal_units);
    return DAQD_OK;
}

/*  daq_clear_channel_list(daq_t*)
 *  
 *  Clear all entries in the channel request list. The name strings are 
 *  freed but the storage allocated for the channel list is retained.
 */
int
daq_clear_channel_list(daq_t* daq) {
    uint4_type id;
    for (id=0; id<daq->num_chan_request; ++id) {
	free(daq->chan_req_list[id].name);
	daq->chan_req_list[id].name = 0;
    }
    daq->num_chan_request = 0;
    return DAQD_OK;
}

/*  daq_get_channel_status(daq_t *daq, const char* channel)
 *
 *  Locate the channel request/status entry for the specified channel.
 */
chan_req_t*
daq_get_channel_status(daq_t *daq, const char* channel) {
    uint4_type id;
    for (id=0; id<daq->num_chan_request; ++id) {
	if (!strcmp(daq->chan_req_list[id].name, channel)) {
	    return daq->chan_req_list + id;
	}
    }
    return NULL;
}

/*  daq_get_data_length(daq_t *daq, const char* channel);
 *
 *  Get the number of bytes of data retrieved for the specified channel.
 */
int
daq_get_data_length(daq_t *daq, const char* channel) {
    chan_req_t* req = daq_get_channel_status(daq, channel);
    if (!req || req->status < 0) return 0;
    return req->status;
}

/*  Get the addres of the data from a specified channel.
 */
const char*
daq_get_channel_addr(daq_t *daq, const char* channel) {
    chan_req_t* req = daq_get_channel_status(daq, channel);
    if (!req) {
        daq->err_num = DAQD_NOT_FOUND;
        return NULL;
    }
    else if (req->status < 0) {
        daq->err_num = - req->status;
        return NULL;
    }
    return daq->tb->data + req->offset;
}

/*  Copy the data from a specified channel name to the target address.
 */
char*
daq_get_channel_data(daq_t *daq, const char* channel, char* data) {
    chan_req_t* req = daq_get_channel_status(daq, channel);
    if (!req) {
	daq->err_num = DAQD_NOT_FOUND;
	return 0;
    }
    else if (req->status < 0) {
	daq->err_num = - req->status;
	return 0;
    }
    memcpy(data, daq->tb->data + req->offset, (size_t) req->status);
    return data;
}

/*  Copy the data from a specified channel name to the target address.
 */
int
daq_get_scaled_data(daq_t *daq, const char* channel, float* data) {
    size_t i, N;
    float scale, bias;
    long nByt;
    chan_req_t* req = daq_get_channel_status(daq, channel);

    if (!req) return 0;
    nByt = req->status;
    if (nByt < 0) return 0;
    N = (size_t)nByt / data_type_size(req->data_type);
    scale = req->s.signal_slope;
    bias  = req->s.signal_offset;
    switch (req->data_type) {
    case _16bit_integer: {
	int16_t* p = (int16_t*) (daq->tb->data + req->offset);
	for (i=0; i<N; i++) data[i] = (float)p[i] * scale + bias;
	break;
    }
    case _32bit_integer: {
	int32_t* p = (int32_t*) (daq->tb->data + req->offset);
	for (i=0; i<N; i++) data[i] = (float)p[i] * scale + bias;
	break;
    }
    case _32bit_float: {
	float* p = (float*) (daq->tb->data + req->offset);
	for (i=0; i<N; i++) data[i] = p[i] * scale + bias;
	break;
    }
    case _64bit_integer: {
	int64_t* p = (int64_t*) (daq->tb->data + req->offset);
	for (i=0; i<N; i++) data[i] = (float)((double)p[i] * scale + bias);
	break;
    }
    case _64bit_double: {
	double* p = (double*) (daq->tb->data + req->offset);
	for (i=0; i<N; i++) data[i] = (float)(p[i] * scale + bias);
	break;
    }
    /*  Ignore complex data... I don't think that the scaling is meaningful
     *  and there is a danger that the user will not provide enough space in
     *  the array.
     */
    case _32bit_complex:
    default:
	N = 0;
    }
    return (int) N;
}

/*  daq_request_data(daq_t*, time_t, time_t, time_t)
 *
 *  Get requested channel data for the specified interval.
 */
int
daq_request_data(daq_t* daq, time_t start, time_t end, time_t dt) {
    int rc =0;
    switch (daq->nds_versn) {
    case nds_v1:
	{
	    time_t delta = dt;

	    if ( end != 0 )
	    {
		delta = end - start;
	    }
	    rc = nds1_request_data(daq, start, delta );
	}
	break;
    case nds_v2:
	rc = nds2_request_data(daq, start, end, dt);
	break;
    default:
	rc = DAQD_NOT_CONFIGURED;
    }
    return rc;
}

/**
 *  Set the default epoch for this session.
 */
int
daq_set_epoch(daq_t* daq, const char* epoch) {
    int rc =0;
    switch (daq->nds_versn) {
    case nds_v1:
	rc = DAQD_NOT_SUPPORTED;
	break;
    case nds_v2:
	rc = nds2_set_epoch(daq, epoch);
	break;
    default:
	rc = DAQD_NOT_CONFIGURED;
    }
    return rc;
}

const char* daq_strerror(int errornum)
{
    static const char* METHOD = "daq_strerror";

    const char* retval = (const char*)NULL;

    if ( nds_logging_check( NDS_LOG_GROUP_TRACE_ENTRY_EXIT, 10 ) )
    {
	nds_logging_printf( "INFO: %s: ENTRY: errornum: %d\n",
			    METHOD,
			    errornum );
    }
    switch (errornum)
    {
        case DAQD_OK:
            retval= "Successful completion";
	    break;
        case DAQD_ERROR:
            retval = "Unspecified error";
	    break;
        case DAQD_NOT_CONFIGURED:
            retval = "DAQ structure not initialized";
	    break;
        case DAQD_INVALID_IP_ADDRESS:
            retval = "Invalid IP address";
	    break;
        case DAQD_INVALID_CHANNEL_NAME:
            retval = "Invalid channel name";
	    break;
        case DAQD_SOCKET:
            retval = "Client failed to get socket";
	    break;
        case DAQD_SETSOCKOPT:
            retval = "Unable to set client socket options";
	    break;
        case DAQD_CONNECT:
            retval = "Unable to connect to server";
	    break;
        case DAQD_BUSY:
            retval = "NDS server is overloaded";
	    break;
        case DAQD_MALLOC:
            retval = "Insufficient memory for allocation";
	    break;
        case DAQD_WRITE:
            retval = "Error occurred trying to write to socket";
	    break;
        case DAQD_VERSION_MISMATCH:
            retval = "Communication protocol version mismatch";
	    break;
        case DAQD_NO_SUCH_NET_WRITER:
            retval = "No such net-writer";
	    break;
        case DAQD_NOT_FOUND:
            retval = "Requested data were not found.";
	    break;
        case DAQD_GETPEERNAME:
            retval = "Could not get clients IP address";
	    break;
        case DAQD_DUP:
            retval = "Error in dup().";
	    break;
        case DAQD_INVALID_CHANNEL_DATA_RATE:
            retval = "Requested data rate is invalid for channel";
	    break;
        case DAQD_SHUTDOWN:
            retval = "Shutdown request failed.";
	    break;
        case DAQD_NO_TRENDER:
            retval = "Trend data are not available";
	    break;
        case DAQD_NO_MAIN:
            retval = "Full channel ata are not available";
	    break;
        case DAQD_NO_OFFLINE:
            retval = "Offline data are not available";
	    break;
        case DAQD_THREAD_CREATE:
            retval = "Unable to create listener thread.";
	    break;
        case DAQD_TOO_MANY_CHANNELS:
            retval = "Too many channels or too much data requested.";
	    break;
        case DAQD_COMMAND_SYNTAX:
            retval = "Command syntax error";
	    break;
        case DAQD_SASL:
            retval = "Request SASL authentication protocol";
	    break;
        case DAQD_NOT_SUPPORTED:
            retval = "Requested feature is not supported";
	    break;
        default:
            retval = "unknown error";
	    break;
    }
    if ( nds_logging_check( NDS_LOG_GROUP_TRACE_ENTRY_EXIT, 10 ) )
    {
	nds_logging_printf( "INFO: %s: EXIT: retval: %s\n",
			    METHOD,
			    retval );
    }
    return retval;
}

/*
 *  Initialize the nds1 and nds2 client interface.
 */
int
daq_startup(void) {
    /*
     * Setup logging as part of startup.
     */
    nds_logging_init( );

#if _WIN32
    /*
     * Initialize the socket package
     */
    {
	const int MAJOR = 2;
	const int MINOR = 0;

	WORD wVersionRequested;
	WSADATA wsaData;
	int err;

	/* Use the MAKEWORD(lowbyte, highbyte) macro declared in Windef.h */
	wVersionRequested = MAKEWORD(MAJOR, MINOR);
	err = WSAStartup(wVersionRequested, &wsaData);
	if (err != 0)
	{
	    /* Tell the user that we could not find a usable */
	    /* Winsock DLL.                                  */
	    printf("WSAStartup failed with error: %d\n", err);
	    return DAQD_ERROR;
	}
	/* Confirm that the WinSock DLL supports MAJOR.MINOR.*/
	/* Note that if the DLL supports versions greater    */
	/* than MAJOR.MINOR in addition to MAJOR.MINOR, it will still return */
	/* MAJOR.MINOR in wVersion since that is the version we      */
	/* requested.                                        */
	if ( ( LOBYTE(wsaData.wVersion) != MAJOR )
	     || ( HIBYTE(wsaData.wVersion) != MINOR ) )
	{
	    /* Tell the user that we could not find a usable */
	    /* WinSock DLL.                                  */
	    printf("Could not find a usable version of Winsock.dll\n");
	    WSACleanup();
	    return DAQD_ERROR;
	}
    }
#endif /* _WIN32 */
    if (nds1_startup() || nds2_startup()) return DAQD_ERROR;
    return DAQD_OK;
}

/*
 *  Initialize channel structure.
 */
void
daq_init_channel(daq_channel_t* chan, const char* name, enum chantype ctype,
		 double rate, daq_data_t dtype) {

    struct trench_struct tst;
    trench_init(&tst);
    trench_parse(&tst, name);
    trench_infer_chan_info(&tst, ctype, rate, dtype);

    strncpy_s( chan->name,
	       sizeof( chan->name ),
	       tst.str,
	       (size_t)(MAX_LONG_CHANNEL_NAME_LENGTH + 1));

    chan->name[MAX_LONG_CHANNEL_NAME_LENGTH] = 0;
    chan->type      = tst.ctype;
    chan->rate      = tst.rate;
    chan->data_type = tst.dtype;
    chan->bps       = (int) data_type_size(chan->data_type);
    chan->tpnum     = 0;
    chan->chNum     = 0;
    chan->s.signal_gain     = 1.0;
    chan->s.signal_slope    = 1.0;
    chan->s.signal_offset   = 0.0;
    chan->s.signal_units[0] = 0;

    trench_destroy(&tst);
}

/**********************************************************************
 *                                                                    *
 *    Channel type conversion functions                               *
 *                                                                    *
 **********************************************************************/

/**  Convert a channel type code to a string.
 */
const char*
cvt_chantype_str(enum chantype t) {
    switch (t) {
    case cUnknown:
	return "unknown";
    case cOnline:
	return "online";
    case cRaw:
	return "raw";
    case cRDS:
	return "reduced";
    case cSTrend:
	return "s-trend";
    case cMTrend:
	return "m-trend";
    case cTestPoint:
	return "test-pt";
    case cStatic:
	return "static";
    default:
	break;
    }
    return "unknown";
}

/*  Convert a channel type string to a channel type code
 */
enum chantype
cvt_str_chantype(const char* name) {
#define TEST_NAME(x) if (!strcmp(name, cvt_chantype_str(x))) return x
    TEST_NAME(cOnline);
    TEST_NAME(cRaw);
    TEST_NAME(cRDS);
    TEST_NAME(cSTrend);
    TEST_NAME(cMTrend);
    TEST_NAME(cTestPoint);
    TEST_NAME(cStatic);
#undef TEST_NAME
    return cUnknown;
}
