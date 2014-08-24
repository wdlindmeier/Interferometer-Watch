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
#else
#include <io.h>
#endif /* HAVE_UNISTD_H */

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

#include "daqc.h"
#include "daqc_private.h"
#include "daqc_response.h"
#include "daqc_internal.h"
#include "daqc_net.h"
#include "nds_auth.h"
#include "nds_logging.h"
#include "nds_os.h"
#include "nds2.h"

/*
 *  Connect to the DAQD server on the host identified by `ip' address.
 *  Returns zero if OK or the error code if failed.
 */
int
nds2_connect (daq_t *daq, const char* host, int port)
{
    static const char* METHOD = "nds2_connect";
    int resp = DAQD_OK;
    int connect_rc = 0;

    printf("nds2 1\n");
    
    if ( nds_logging_check( NDS_LOG_GROUP_TRACE_ENTRY_EXIT, 10 ) )
    {
	nds_logging_printf( "INFO: %s: ENTRY\n",
			    METHOD );
    }
    /*  Get a socket.
     */
    if ( (resp = daq_private_srvr_open( daq->conceal ) )
	 != DAQD_OK )
    {
	if ( nds_logging_check( NDS_LOG_GROUP_TRACE_ENTRY_EXIT, 10 ) )
	{
	    nds_logging_printf( "INFO: %s: EXIT: %d\n",
				METHOD,
				resp );
	}
	return resp;
    }

    if ( nds_logging_check( NDS_LOG_GROUP_TRACE_ENTRY_EXIT, 20 ) )
    {
	nds_logging_printf( "INFO: %s: %6d - %s\n",
			    METHOD,
			    __LINE__,
			    __FILE__ );
    }
    /*----------------------------------  Get the server address.     */
    resp = daq_set_server(daq, host, port);
    if ( resp != DAQD_OK ) {
	perror("Error in daq_set_server");
	daq_private_srvr_close( daq->conceal );
	if ( nds_logging_check( NDS_LOG_GROUP_TRACE_ENTRY_EXIT, 10 ) )
	{
	    nds_logging_printf( "INFO: %s: EXIT: %d\n",
				METHOD,
				resp );
	}
	return resp;
    }

    if ( nds_logging_check( NDS_LOG_GROUP_TRACE_ENTRY_EXIT, 20 ) )
    {
	nds_logging_printf( "INFO: %s: %6d - %s\n",
			    METHOD,
			    __LINE__,
			    __FILE__ );
    }
    /*-----------------------------------  Connect to the server      */
    daq_private_srvr_nonblocking( daq->conceal, 1 );
    if ( nds_logging_check( NDS_LOG_GROUP_STATUS_UPDATE,
			    0 ) )
    {
	nds_logging_printf("Connecting NDS2 ..");
	nds_logging_flush( );
    }
    if ( nds_logging_check( NDS_LOG_GROUP_TRACE_ENTRY_EXIT, 20 ) )
    {
	nds_logging_printf( "INFO: %s: %6d - %s\n",
			    METHOD,
			    __LINE__,
			    __FILE__ );
    }
	connect_rc = daq_private_srvr_connect( daq->conceal );
	if ( connect_rc == NDS_SOCKET_TRANSIENT_FAILURE )
	{
		struct timeval timeout = {4, 0};
		fd_set writefds;
		int ret;
		FD_ZERO(&writefds);
		FD_SET(daq->conceal->sockfd, &writefds);
		ret = select(daq->conceal->sockfd + 1, NULL, &writefds, NULL, &timeout);
		if (ret == 1)
			connect_rc = NDS_SOCKET_OK;
    }
    if ( nds_logging_check( NDS_LOG_GROUP_TRACE_ENTRY_EXIT, 20 ) )
    {
	nds_logging_printf( "INFO: %s: %6d - %s\n",
			    METHOD,
			    __LINE__,
			    __FILE__ );
    }
    if ( connect_rc != NDS_SOCKET_OK )
    {
	if ( nds_logging_check( NDS_LOG_GROUP_STATUS_UPDATE,
			    0 ) )
	{
	    char	pmsg[ 256 ];

	    nds_logging_printf(" failed\n");
	    strerror_r( errno, pmsg, sizeof( pmsg ) );
	    nds_logging_printf( "connect( ): errno: %d - %s\n",
				errno, pmsg );
	}
	daq_private_srvr_close( daq->conceal );
	daq->conceal->sockfd = -1;
	if ( nds_logging_check( NDS_LOG_GROUP_TRACE_ENTRY_EXIT, 10 ) )
	{
	    nds_logging_printf( "INFO: %s: EXIT: %d\n",
				METHOD,
				DAQD_CONNECT );
	}
	return DAQD_CONNECT;
    }

    if ( nds_logging_check( NDS_LOG_GROUP_STATUS_UPDATE,
			    0 ) )
    {
	nds_logging_printf(" authenticate");
    }
    daq_private_srvr_nonblocking( daq->conceal, 0 );
    resp = daq_send(daq, "authorize");
    if ( nds_logging_check( NDS_LOG_GROUP_TRACE_ENTRY_EXIT, 20 ) )
    {
	nds_logging_printf( "INFO: %s: %6d - %s\n",
			    METHOD,
			    __LINE__,
			    __FILE__ );
    }
    if (resp == DAQD_SASL) {
	if ( nds_logging_check( NDS_LOG_GROUP_STATUS_UPDATE,
			    0 ) )
	{
	    nds_logging_printf(" ...");
	}
	resp = nds_authenticate(daq, host);
	if (resp) resp = DAQD_SASL;
    }
    if ( nds_logging_check( NDS_LOG_GROUP_TRACE_ENTRY_EXIT, 20 ) )
    {
	nds_logging_printf( "INFO: %s: %6d - %s\n",
			    METHOD,
			    __LINE__,
			    __FILE__ );
    }

    if (resp == DAQD_OK) {
	if ( nds_logging_check( NDS_LOG_GROUP_TRACE_ENTRY_EXIT, 20 ) )
	{
	    nds_logging_printf( "INFO: %s: %6d - %s\n",
				METHOD,
				__LINE__,
				__FILE__ );
	}
	if ( nds_logging_check( NDS_LOG_GROUP_STATUS_UPDATE,
			    0 ) )
	{
	    nds_logging_printf(" done\n");
	}
	daq->conceal->datafd = daq->conceal->sockfd;
    } else {
	if ( nds_logging_check( NDS_LOG_GROUP_TRACE_ENTRY_EXIT, 20 ) )
	{
	    nds_logging_printf( "INFO: %s: %6d - %s\n",
				METHOD,
				__LINE__,
				__FILE__ );
	}
	if ( nds_logging_check( NDS_LOG_GROUP_STATUS_UPDATE,
			    0 ) )
	{
	    nds_logging_printf(" failed: %s\n", daq_strerror(resp));
	}
	daq_private_srvr_close( daq->conceal );
    }
    if ( nds_logging_check( NDS_LOG_GROUP_TRACE_ENTRY_EXIT, 10 ) )
    {
	nds_logging_printf( "INFO: %s: EXIT: %d\n",
			    METHOD,
			    resp );
    }
    return resp;
}


/*
 *  Disconnect from the server and close the socket file descriptor
 */
int
nds2_disconnect (daq_t *daq) {
#if _WIN32
    typedef int write_len_t;
#else /* _WIN32 */
    typedef size_t write_len_t;
#endif /* _WIN32 */

    /*  Send a quit command */
    static const char* METHOD = "nds2_disconnect";
    char *command = "quit;\n";
    size_t   lcmd = strlen(command);

    if ( nds_logging_check( NDS_LOG_GROUP_TRACE_ENTRY_EXIT, 10 ) )
    {
	nds_logging_printf( "INFO: %s: ENTRY\n",
			    METHOD );
    }
    if (write (daq ->conceal->sockfd, command, (write_len_t)( lcmd ) ) != lcmd) {
	if ( nds_logging_check( NDS_LOG_GROUP_VERBOSE_ERRORS,
				0 ) )
	{
	    nds_logging_printf( "daq_disconnect: write errno=%d\n",
				METHOD,
				errno );
	}
	return DAQD_WRITE;
    }
    usleep(100000);
    nds_auth_disconnect(daq);
    daq_private_srvr_close( daq->conceal );
    if ( nds_logging_check( NDS_LOG_GROUP_TRACE_ENTRY_EXIT, 10 ) )
    {
	nds_logging_printf( "INFO: %s: EXIT: %d\n",
			    METHOD,
			    0 );
    }
    return 0;
}

/*
 *  Receive channel data using the NDS2 protocol
 */
int
nds2_recv_channel_list(daq_t *daq, daq_channel_t *channel, int num_channels, 
		   int *num_channels_received, time_t gps, enum chantype type)
{
#define BUF_LENGTH 512
#define MAX_TYPESTR_LENGTH 16
    const char* METHOD = "nds2_recv_channel_list";
    char buf [BUF_LENGTH];
    char typestr [MAX_TYPESTR_LENGTH];
    int resp, i;
    int4_type	channels;
    nds_socket_type	fd;

    /*----------------------------------  Issue the request               */
    if (num_channels != 0) {
#if HAVE_SPRINTF_S
	if ( nds_logging_check( NDS_LOG_GROUP_VERBOSE_ERRORS,
			  20 ) )
	{
	    nds_logging_printf( "ERROR: %s: sizeof( buf ):%d %s - %d\n",
				METHOD,
				sizeof( buf ),
				__FILE__,
				__LINE__ );
	}
	sprintf_s(buf, sizeof( buf ),
		  "get-channels %i %s;", (int)gps, cvt_chantype_str(type));
#else	/* HAVE_SPRINTF_S */
	sprintf(buf,
		"get-channels %i %s;", (int)gps, cvt_chantype_str(type));
#endif /* HAVE_SPRINTF_S */
	if ( nds_logging_check( NDS_LOG_GROUP_VERBOSE_ERRORS,
				10 ) )
	{
	    nds_logging_printf("INFO: Requesting channel list\n");
	}
    } else {
	if ( nds_logging_check( NDS_LOG_GROUP_VERBOSE_ERRORS,
				10 ) )
	{
	    nds_logging_printf( "INFO: %s: sizeof( buf ):%d %s - %d\n",
				METHOD,
				sizeof( buf ),
				__FILE__,
				__LINE__ );
	}
#if HAVE_SPRINTF_S
	sprintf_s(buf, sizeof( buf ),
		  "count-channels %i %s;",(int)gps, cvt_chantype_str(type));
#else /* HAVE_SPRINTF_S */
	sprintf(buf,
		"count-channels %i %s;",(int)gps, cvt_chantype_str(type));
#endif /* HAVE_SPRINTF_S */
	if ( nds_logging_check( NDS_LOG_GROUP_VERBOSE_ERRORS,
				10 ) )
	{
	    nds_logging_printf( "INFO: %s: Requesting channel count\n",
			        METHOD );
	}
    }
    if ( nds_logging_check( NDS_LOG_GROUP_VERBOSE_ERRORS,
			    10 ) )
    {
	nds_logging_printf("INFO: %s: buf - %s\n",
			   METHOD,  buf );
    }
    resp = daq_send(daq, buf);
    if (resp) {
	if ( nds_logging_check( NDS_LOG_GROUP_VERBOSE_ERRORS,
				0 ) )
	{
	    nds_logging_printf("ERROR: %s: Error return from daq_send(): %s\n",
			       METHOD,
			       daq_strerror(resp));
	}
	return resp;
    }

    /*----------------------------------  Get the number of channels      */
    fd = daq->conceal->sockfd;
    if (read_uint4(fd, (uint4_type*)&channels) || channels < 0) {
	if ( nds_logging_check( NDS_LOG_GROUP_STATUS_UPDATE,
				0 ) )
	{
	    nds_logging_printf( "Couldn't determine the number of data channels\n" );
	}
	return DAQD_ERROR;
    }
    *num_channels_received = channels;
    if ( nds_logging_check( NDS_LOG_GROUP_VERBOSE_ERRORS,
			    10 ) )
    {
	nds_logging_printf( "INFO: %s: channels: %d num_channels_received: %d\n",
			    METHOD,
			    channels, *num_channels_received );
    }
    if (num_channels == 0) return DAQD_OK;

    /*----------------------------------  Loop over channels              */
    for (i=0; i < channels; i++) {
	enum chantype ctype;
	double rate;
	int len=_daq_read_string(fd, (size_t)BUF_LENGTH, buf);
	daq_data_t dtype;

	if ( nds_logging_check( NDS_LOG_GROUP_VERBOSE_ERRORS,
				0 ) )
	{
	    nds_logging_printf( "INFO: %s: i: %d len: %d\n",
				METHOD, i, len );
	}
	if (len <= 0) {
	    return DAQD_ERROR;
	}
	if (i < num_channels) {
	    const char* p = buf;
	    const char* end = buf + len;
	    char name[MAX_CHANNEL_NAME_LENGTH];
	    _daq_get_string(&p, end, name, sizeof(name));
	    _daq_get_string(&p, end, typestr, sizeof(typestr));
	    ctype = cvt_str_chantype(typestr);
	    rate = strtod(p, (char**)&p);
	    _daq_get_string(&p, end, typestr, sizeof(typestr));
	    dtype = data_type_code(typestr);
	    daq_init_channel(channel + i, name, ctype, rate, dtype);
	}
    }
    if ( nds_logging_check( NDS_LOG_GROUP_VERBOSE_ERRORS,
			    10 ) )
    {
	nds_logging_printf("DEBUG: Channel List worked OK\n");
    }
    return DAQD_OK;
}

/*    Get the name string length
 */
static size_t
_nds2_name_list_length(daq_t *daq) {
    uint4_type i;
    uint4_type N = daq->num_chan_request;
    size_t namel = 0;
    chan_req_t* req = daq->chan_req_list;
    for (i=0; i<N; ++i) {
	chantype_t ctype;

	namel += strlen(req->name) + 1;
	ctype = req->type;
	if (ctype != cUnknown) namel += strlen(cvt_chantype_str(ctype)) + 1;
	req++;
    }
    return namel;
}

/*    Get the name string length
 */
static size_t
_nds2_get_name_list(daq_t *daq, char* p) {
    chan_req_t* req;

    size_t i = 0;
    size_t j = 0;
    size_t N = daq->num_chan_request;
    p[i++] = '{';
    req = daq->chan_req_list;
    for (j=0; j<N; j++) {
	const char* pchn = req->name;
	chantype_t ctype;

	if (j) p[i++] = ' ';
	while (*pchn) p[i++] = *pchn++;
	ctype = req->type;
	if (ctype != cUnknown) {
	    p[i++] = ',';
	    pchn = cvt_chantype_str(ctype);
	    while (*pchn) p[i++] = *pchn++;
	}
	req++;
    }
    p[i++] = '}';
    return i;
}

/*
 *  Receive source list using the NDS2 protocol
 */
int 
nds2_recv_source_list(daq_t *daq, char* list, size_t max_len, time_t gps, 
		      long* str_len) {
    int    rc = 0;
    size_t namel, txt_len;
    char   *cmd, *cmd_txt, *p; 

    *str_len = 0;
    namel = _nds2_name_list_length(daq);

    /*----------------------------------  Build up a command
     */
    cmd = "get-source-list";

    /*----------------------------------  Get the text length, allocate a buffer
     */
    txt_len = strlen(cmd)
	           + strlen(" xxxxxxxxxx {};") 
	           + namel + daq->num_chan_request;
    cmd_txt = (char*)malloc(txt_len);

    /*----------------------------------  Build the command
     */
    p = cmd_txt;
    while (*cmd) *p++ = *cmd++;
    *p++ = ' ';
    p += daq_cvt_timet_string(gps, p);
    *p++ = ' ';
    p += _nds2_get_name_list(daq, p);
    *p++ = ';';
    *p++ = 0;  

    /*----------------------------------  Send the command free the command
     */
    rc = daq_send(daq, cmd_txt);
    free(cmd_txt);
    cmd_txt = 0;

    if (rc) {
	printf("Error in daq_send: %s\n", daq_strerror(rc));
	return rc;
    }

    /*----------------------------------  Read the source string      */
    *str_len = _daq_read_string(daq->conceal->sockfd, max_len, list);
    if (*str_len < 0) return DAQD_ERROR;
    return rc;
}

/*  nds2_request_data(daq_t*, time_t, time_t)
 *
 *  Get requested channel data for the specified interval.
 */
int
nds2_request_data(daq_t* daq, time_t start, time_t end, time_t dt) {
    static const char* METHOD = "nds2_request_data";
    int    wid, bl, rc = DAQD_OK;
    size_t N, i, namel, txt_len;
    char   *cmd, *cmd_txt, *p;
    chan_req_t* req;

    /*  Check the dt number (j.i.c)
     */
    if (dt > 2000000000) {
        if ( nds_logging_check( NDS_LOG_GROUP_VERBOSE_ERRORS, 0 ) )
        {
            nds_logging_printf( "ERROR: %s: Data stride too long\n",
                                METHOD );
        }
        return DAQD_ERROR;
    }  

    /*  Get the request type and sum channel name length
     */
    N = daq->num_chan_request;
    if (N == 0) {
	if ( nds_logging_check( NDS_LOG_GROUP_VERBOSE_ERRORS,
				0 ) )
	{
	    nds_logging_printf( "ERROR: %s: No channels requested\n",
				METHOD );
	}
	return DAQD_ERROR;
    }

    namel = _nds2_name_list_length(daq);

    /*----------------------------------  Build up a command
     */
    cmd = "get-data";

    /*----------------------------------  Get the text length, allocate a buffer
     */
    txt_len = strlen(cmd)
	    + strlen(" xxxxxxxxxx xxxxxxxxxx xxxxxxxxxx xxxxxxxxxx {};") 
	    + namel + N;
    cmd_txt = (char*)malloc(txt_len);

    /*----------------------------------  Build the command
     */
    p = cmd_txt;
    while (*cmd) *p++ = *cmd++;
    *p++ = ' ';
    p += daq_cvt_timet_string(start, p);
    *p++ = ' ';
    p += daq_cvt_timet_string(end, p);
    *p++ = ' ';
    p += daq_cvt_timet_string(dt, p);
    *p++ = ' ';
    p += _nds2_get_name_list(daq, p);
    *p++ = ';';
    *p++ = 0;  

    /*----------------------------------  Reset status words before request
     */
    req = daq->chan_req_list;
    for (i=0; i<N; ++i) {
	req[i].status = 0;
    }

    /*----------------------------------  Send the command free the command
     */
    rc = daq_send(daq, cmd_txt);
    free(cmd_txt);
    cmd_txt = 0;


    if (rc) {
	printf("Error in daq_send: %s\n", daq_strerror(rc));
	return rc;
    }

    /*------------------------------  Get the writer ID
     */
    wid = (int)( daq_recv_id(daq) );
    if (wid < 0) {
	printf("Error reading writerID in daq_recv_block\n");
	return DAQD_ERROR;
    }

    /*------------------------------  Get the offline flag
     */
    bl = daq_recv_block_num(daq);
    if (bl < 0) {
	printf("Error reading offline flag in daq_recv_block\n");
	rc = DAQD_ERROR;
    }
    return rc;
}


struct nds2_chan_status {
    int status;
    int offset;
    int data_type;
    float rate;
    float signal_offset;
    float signal_slope;
};

/*  nds2_receive_reconfigure(daq_t* daq, long block_len)
 *
 *  Receive a reconfigure block. receive_reconfigure is received after the 
 *  block header has been read in. The block length does not include the
 *  header length.
 */
int 
nds2_receive_reconfigure(daq_t* daq, size_t block_len) {
    nds_socket_type fd;
    size_t nchannels, i;
    chan_req_t* list;

    nchannels = block_len / sizeof(struct nds2_chan_status);
    if (nchannels * sizeof(struct nds2_chan_status) != block_len) {
	if ( nds_logging_check( NDS_LOG_GROUP_STATUS_UPDATE,
				0 ) )
	{
	    nds_logging_printf( "Channel reconfigure block length has bad length (%zi)\n",
				block_len );
	}
    }
    if (nchannels != daq->num_chan_request) return -1;

    fd = daq->conceal->datafd;
    list = daq->chan_req_list;
    for (i = 0; i < nchannels; i++) {
	float rate;
	uint4_type dtype;
	if (read_uint4(fd, (uint4_type*)&list[i].status))  return DAQD_ERROR;
	if (read_uint4(fd, &list[i].offset))  return DAQD_ERROR;
	if (read_uint4(fd, &dtype))           return DAQD_ERROR;
	if (list[i].type == cUnknown) list[i].type = dtype >> 16;
	list[i].data_type = (dtype & 0xffff);
	if (read_float(fd, &rate))           return DAQD_ERROR;
	list[i].rate = rate;
	if (read_float(fd, &list[i].s.signal_offset))  return DAQD_ERROR;
	if (read_float(fd, &list[i].s.signal_slope))   return DAQD_ERROR;
    }
    return -2; 
}

/*
 *  Set the default epoch for this session.
 */
int
nds2_set_epoch(daq_t* daq, const char* epoch) {
    int rc = 0;
    char* cmd = (char*)malloc(12 + strlen(epoch));
    strcpy(cmd, "set-epoch ");
    strcat(cmd, epoch);
    strcat(cmd, ";");
    rc = daq_send(daq, cmd);
    free(cmd);
    return rc;
}

/*
 *  User authentication
 */
int
nds2_startup(void) {
    return nds_auth_startup();
}
