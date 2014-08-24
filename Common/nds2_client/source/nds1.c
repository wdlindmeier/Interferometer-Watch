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
#include "nds1.h"
#include "nds_logging.h"
#include "nds_os.h"

/*
 *  Connect to the DAQD server on the host identified by `ip' address.
 *  Returns zero if OK or the error code if failed.
 */
int
nds1_connect (daq_t *daq, const char* host, int port) {
    int resp;
    int v, rv;
    int exp_rv;
    int connect_rc = 0;

    /*  Get a socket.
     */
    resp = daq_private_srvr_open( daq->conceal );
    if ( resp != DAQD_OK )
    {
	return resp;
    }

    /*----------------------------------  Get the server address.     */
    resp = daq_set_server(daq, host, port);
    if (resp) {
	daq_private_srvr_close( daq->conceal );
	return resp;
    }

    /*-----------------------------------  Connect to the server      */
    daq_private_srvr_nonblocking( daq->conceal, 1 );
    if ( nds_logging_check( NDS_LOG_GROUP_STATUS_UPDATE,
			    0 ) )
    {
	nds_logging_printf("Connecting NDS1 ..");
	nds_logging_flush( );
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
    if ( connect_rc != NDS_SOCKET_OK )
    {
	if ( nds_logging_check( NDS_LOG_GROUP_STATUS_UPDATE,
				0 ) )
	{
	    char pmsg[ 256 ];

	    nds_logging_printf(" failed\n");
	    strerror_r( errno, pmsg, sizeof( pmsg ) );
	    nds_logging_printf( "connect( ): errno: %d - %s\n",
				errno, pmsg );
	}
	daq_private_srvr_close( daq->conceal );
	return DAQD_CONNECT;
    }
    if ( nds_logging_check( NDS_LOG_GROUP_STATUS_UPDATE,
			    0 ) )
    {
	nds_logging_printf(" done\n");
    }
    daq_private_srvr_nonblocking( daq->conceal, 0 );

    // Doing this does not work on Solaris
#ifdef __linux__
    {
	/* Do protocol version check */
	if ( nds_logging_check( NDS_LOG_GROUP_STATUS_UPDATE,
				0 ) )
	{
	    nds_logging_printf(" GET NDS version\n");
	}
	if ((resp = daq_send (daq, "version;"))) {
	    daq_private_srvr_close( daq->conceal );
	    return resp;
	}
    }
#else
    /* Do protocol version check */
    if ( nds_logging_check( NDS_LOG_GROUP_STATUS_UPDATE,
			    0 ) )
    {
	nds_logging_printf(" GET NDS version\n");
    }
    if ((resp = daq_send (daq, "version;"))) {
	daq_private_srvr_close( daq->conceal );
	return resp;
    }
#endif

    /* Read server response */
    v = (int)read_server_response(daq->conceal->sockfd);
    if ( nds_logging_check( NDS_LOG_GROUP_STATUS_UPDATE,
			    0 ) )
    {
	nds_logging_printf("NDS version of server is %d\n",v);
    }
    if ((v > MAX_NDS1_PROTOCOL_VERSION) || (v < MIN_NDS1_PROTOCOL_VERSION)) {
	if ( nds_logging_check( NDS_LOG_GROUP_STATUS_UPDATE,
				0 ) )
	{
	    nds_logging_printf( "unsupported communication protocol version: ");
	    nds_logging_printf( "received %d, only support %d to %d\n",v,MIN_NDS1_PROTOCOL_VERSION, MAX_NDS1_PROTOCOL_VERSION);
	    nds_logging_printf(" version %d not between %d to %d\n",v,MIN_NDS1_PROTOCOL_VERSION,MAX_NDS1_PROTOCOL_VERSION);
	}
	daq_private_srvr_close( daq->conceal );
	return DAQD_VERSION_MISMATCH;
    }

    /* Set things based on protocol version of server */
    if ( v >= 12 )
    {
	exp_rv = 0;
    }
    else
    {
	exp_rv = 4;
    }
    daq -> nds1_ver = v;

    /* Do protocol revision check */
    resp = daq_send (daq, "revision;");
    if (resp) {
	daq_private_srvr_close( daq->conceal );
	return resp;
    }

    /* Read server response */
    rv = (int)read_server_response(daq->conceal->sockfd);
    if (rv != exp_rv) {
	if ( nds_logging_check( NDS_LOG_GROUP_STATUS_UPDATE,
				0 ) )
	{
	    nds_logging_printf( "Warning: communication protocol revision mismatch: ");
	    nds_logging_printf(  "expected %d.%d, received %d.%d\n",
				 v, exp_rv, v, rv);
	}
    }
    daq -> nds1_rev = rv;
    daq -> conceal -> datafd = daq -> conceal -> sockfd;

#ifdef VXWORKS
    {
	int blen = 4*4096;
	if (setsockopt (daq -> conceal -> sockfd, SOL_SOCKET, SO_SNDBUF, &blen, 
			sizeof (blen)) < 0) {
	    if ( nds_logging_check( NDS_LOG_GROUP_STATUS_UPDATE,
				    0 ) )
	    {
		nds_logging_printf ( "setsockopt SO_SNDBUF failed\n" );
	    }
	    daq_private_srvr_close( daq->conceal );
	    return DAQD_SETSOCKOPT;
	}
    }
#endif
    return 0;
}


/*
 *  Disconnect from the server and close the socket file descriptor
 */
int
nds1_disconnect (daq_t *daq) {
#if _WIN32
    typedef int write_len_t;
#else /* _WIN32 */
    typedef size_t write_len_t;
#endif /* _WIN32 */

    static const char* METHOD = "nds1_disconnect";
    char *command = "quit;\n";
    size_t  lcmd = strlen(command);
    if ( nds_logging_check( NDS_LOG_GROUP_TRACE_ENTRY_EXIT, 10 ) )
    {
	nds_logging_printf( "INFO: %s: ENTRY\n",
			    METHOD );
    }
    if (write (daq -> conceal -> sockfd, command, (write_len_t)( lcmd ) ) != lcmd) {
	if ( nds_logging_check( NDS_LOG_GROUP_VERBOSE_ERRORS,
				0 ) )
	{
	    nds_logging_printf ( "ERROR: %s: write errno=%d\n",
				 METHOD, errno);
	}
	if ( nds_logging_check( NDS_LOG_GROUP_TRACE_ENTRY_EXIT, 10 ) )
	{
	    nds_logging_printf( "INFO: %s: EXIT - %d\n",
				METHOD,
				DAQD_WRITE );
	}
	return DAQD_WRITE;
    }
    usleep(100000);
    daq_private_srvr_close( daq->conceal );
    if ( nds_logging_check( NDS_LOG_GROUP_TRACE_ENTRY_EXIT, 10 ) )
    {
	nds_logging_printf( "INFO: %s: EXIT - 0\n",
			    METHOD );
    }
    return 0;
}


static void
cvthex(const char** s, char* buf, void* x) {
    memcpy(buf, *s, (size_t)8);
    buf[8] = 0;
    (*s) += 8;
    *(int*)x = (int)( dca_strtol(buf) );
}

/*
 *  Receive channel data using the NDS1 protocol
 */
int
nds1_recv_channels (daq_t *daq, daq_channel_t *channel, int num_channels, 
		    int *num_channels_received) {
    static const char* METHOD = "nds1_recv_channels";
    int i;
    int resp;
    size_t lChanWd = 8;
    int    channels = -1;
#if _WIN32
    nds_socket_type f;
#else
    FILE* f;
#endif /* 0 */

#define CHAN_BUF_SIZE_MAX 149

    const char* chan_req = "status channels 2;\n";

    int chan_buf_size;
    int channel_name_size;
    int signal_unit_size;
    char buf[ CHAN_BUF_SIZE_MAX ];

    /* set channel name, signal unit size based on protocol - K. Thorne*/
    if ( daq->nds1_ver >= 12 )
    {
	channel_name_size = 60;
	signal_unit_size = 40;
    }
    else
    {
	channel_name_size = 40;
	signal_unit_size = 40;
    }

    chan_buf_size =
	channel_name_size
	+ signal_unit_size
	+ 49;

    if ( nds_logging_check( NDS_LOG_GROUP_VERBOSE_ERRORS,
			    10 ) )
    {
	nds_logging_printf( "INFO: %s: nds1_recv_channels: entry\n",
			    METHOD );
    }
    resp = daq_send(daq, chan_req);
    if ( nds_logging_check( NDS_LOG_GROUP_VERBOSE_ERRORS,
			    10 ) )
    {
	nds_logging_printf( "INFO: %s: nds1_recv_channels: resp: %s\n",
			    METHOD,
			    ( resp ) ? "yes" : "no" );
    }
    if (resp) {
	return resp;
    }

    resp = read_bytes(daq->conceal->sockfd, buf, lChanWd);
    if (resp == lChanWd) {
	buf[lChanWd] = 0;
	channels = (int)( dca_strtol(buf) );
    }

    if (channels <= 0) {
	if ( nds_logging_check( NDS_LOG_GROUP_STATUS_UPDATE,
				0 ) )
	{	
	    nds_logging_printf ( "Couldn't determine the number of data channels\n" );
	}
	return DAQD_ERROR;
    }

    *num_channels_received = channels;

#if _WIN32
    f = daq -> conceal-> sockfd;
#else
    /*  Open the socket as a file
     */
    f = fdopen(dup(daq ->conceal-> sockfd), "r");
#endif /* ! _WIN32 */

    for (i = 0; i < channels; i++) {
#if _WIN32
	if (_daq_read_cstring(f, chan_buf_size, buf ) <= 0) {
	    perror("string read failed");
	    return DAQD_ERROR;
	}
	if ( nds_logging_check( NDS_LOG_GROUP_VERBOSE_ERRORS,
				10 ) )
	{
	    nds_logging_printf( "INFO: %s: read[%d/%d]: %s\n",
				METHOD, i, channels, buf );
	}

#else /* _WIN32 */
	if (fgets(buf, (int)( chan_buf_size ), f) <= 0) {
	    perror("fgets failed");
	    fclose(f);
	    return DAQD_ERROR;
	}
#endif /* _WIN32 */
	/* printf("chan string: %s\n", buf); */

	if (i < num_channels) {
	    int intval;
	    const char* s = buf;

	    /* Channel name                                 */
	    _daq_get_string(&s, buf+channel_name_size, channel[i].name, sizeof(channel->name));
	    s = buf + channel_name_size;

	    /* Read rate                                    */
	    cvthex(&s, buf, &intval);
	    channel[i].rate = intval;

	    /* Read testpoint number                        */
	    cvthex(&s, buf, &channel[i].tpnum);

	    /* Group number and data type are 16-bit fields */
	    cvthex(&s, buf, &intval);
	    channel[i].type = cvt_group_chantype(intval >> 16);
	    channel[i].data_type = (intval & 0xffff);
	    channel[i].bps= (int)data_type_size(channel[i].data_type);

	    /* Read gain,slope and offset                   */
	    cvthex(&s, buf, &channel[i].s.signal_gain);
	    cvthex(&s, buf, &channel[i].s.signal_slope);
	    cvthex(&s, buf, &channel[i].s.signal_offset);

	    /* Read units                                   */
	    _daq_get_string(&s, s+signal_unit_size, channel[i].s.signal_units, 
			    sizeof(channel->s.signal_units));

	    if ( nds_logging_check( NDS_LOG_GROUP_VERBOSE_ERRORS,
				    10 ) )
	    {
		nds_logging_printf( "INFO: %s: channel: %s  rate: %f  dtype: %i ctype: %i\n",
				    METHOD,
				    channel[i].name,
				    channel[i].rate,
				    channel[i].data_type,
				    intval );
	    }
	}
    }
#if ! _WIN32
    fclose(f);
#endif /* ! _WIN32 */
    return 0;
}

/*  nds1_request_data(daq_t*, time_t, time_t)
 *
 *  Get requested channel data for the specified interval.
 */
int
nds1_request_data(daq_t* daq, time_t start, time_t dt) {
    static const char* METHOD = "nds1_request_data";
    enum chantype reqty   = cUnknown;
    size_t        namel   = 0;
    size_t        txt_len = 0;    
    uint4_type    i;
    char          *cmd_txt, *p, *cmd;
    int           rc =DAQD_OK;
    uint4_type	  N = 0;

    /*----------------------------------  Build the command
     */
    /*  Check the dt
     */
    if (dt > 2000000000) {
	if ( nds_logging_check( NDS_LOG_GROUP_VERBOSE_ERRORS, 0 ) )
        {
            nds_logging_printf( "ERROR: %s: time stride is too long\n",
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

    for (i=0; i<N; ++i) {
	if (reqty == cUnknown) {
	    reqty = daq->chan_req_list[i].type;
	} else if (reqty != daq->chan_req_list[i].type) {
	    if ( nds_logging_check( NDS_LOG_GROUP_VERBOSE_ERRORS,
				    0 ) )
	    {
		nds_logging_printf( "ERROR: %s: Incompatible channel types\n",
				    METHOD );
	    }
	    return DAQD_ERROR;
	}
	namel += strlen(daq->chan_req_list[i].name) + 1;
    }

    /*----------------------------------  Resolve  Unknown channel types
     */
    if (reqty == cUnknown) {
	if (!start) reqty = cOnline;
	else        reqty = cRaw;
    }

    /*----------------------------------  Build up a command
     */
    cmd = NULL;
    switch (reqty) {

    /*----------------------------------  Online channel type.
     */
    case cOnline:
    case cRaw:
	cmd = "start net-writer";
	break;

    /*----------------------------------  Second trend channel type.
     */
    case cSTrend:
	cmd = "start trend net-writer";
	break;

    /*----------------------------------  Minute trend channel type.
     */
    case cMTrend:
	cmd = "start trend 60 net-writer";
	break;

    /*----------------------------------  Channel type not supported by NDS1
     */
    default:
	if ( nds_logging_check( NDS_LOG_GROUP_VERBOSE_ERRORS,
				0 ) )
	{
	    nds_logging_printf( "ERROR: %s: Channel type %s not supported\n",
				METHOD,
				cvt_chantype_str(reqty));
	}
	return DAQD_ERROR;
    }

    /*----------------------------------  Get the text length, allocat a buffer
     */
    txt_len = strlen(cmd)  /* length of commmand text */
	    + strlen(" xxxxxxxxxx xxxxxxxxxx {};") /* punctuation, times */
	    + namel + N; /* length of channel names, blanks and NULL*/
    cmd_txt = (char*)malloc(txt_len);

    /*----------------------------------  Build the command
     */
    p = cmd_txt;
    while (*cmd) *p++ = *cmd++;

    //----------------------------------  Include time, duration if specified.
    if (start) {
	*p++ = ' ';
	p += daq_cvt_timet_string(start, p);
	*p++ = ' ';
	p += daq_cvt_timet_string(dt,    p);
    }
    *p++ = ' '; *p++ = '{';
    for (i=0; i<N; i++) {
	char* pchn;

	if (i) *p++ = ' ';
	*p++ = '"';
	pchn = daq->chan_req_list[i].name;
	while (*pchn) *p++ = *pchn++;
	*p++ = '"';
	daq->chan_req_list[i].status = 0;
    }
    *p++ = '}'; *p++ = ';';  *p++ = 0;  

    /*----------------------------------  Send the command.
     */
    rc = daq_send(daq, cmd_txt);
    if (rc) {
	if ( nds_logging_check( NDS_LOG_GROUP_STATUS_UPDATE,
				0 ) )
	{
	    nds_logging_printf("Error in daq_send: %i\n", rc);
	}
    } else {
	int wid = (int)( daq_recv_id(daq) );
	if (wid < 0) {
	    if ( nds_logging_check( NDS_LOG_GROUP_STATUS_UPDATE,
				    0 ) )
	    {
		nds_logging_printf("Error in daq_recv_id: %i\n", wid);
	    }
	} else {
	    int bl = daq_recv_block_num(daq);
	    if ( ( bl < 0 )
		 && nds_logging_check( NDS_LOG_GROUP_STATUS_UPDATE,
				       0 ) )
	    {
		nds_logging_printf("Error in daq_recv_block_num = %i\n", bl);
	    }
	}
    }

    /*----------------------------------  Free the command.
     */
    free(cmd_txt);
    cmd_txt = 0;
    return rc;
}

/*  nds1_receive_reconfigure(daq_t* daq, long block_len)
 *
 *  Receive a reconfigure block. receive_reconfigure is received after the 
 *  block header has been read in. The block length does not include the
 *  header length.
 */
int 
nds1_receive_reconfigure(daq_t* daq, size_t block_len) {
    size_t nchannels, alloc_size, i;
    nds_socket_type fd;
    signal_conv_t1* ps;

    nchannels = block_len / sizeof(signal_conv_t1);
    if (nchannels * sizeof(signal_conv_t1) != block_len) {
	if ( nds_logging_check( NDS_LOG_GROUP_STATUS_UPDATE,
				0 ) )
	{
	    nds_logging_printf ( "warning:channel reconfigure block length has bad length"
				 );
	}
    }
    if (nchannels == 0) return -2;
    alloc_size = sizeof(signal_conv_t1)*nchannels;

    if (!daq->s) {
	daq->s = (signal_conv_t1 *) malloc (alloc_size);
	if (daq -> s) {
	    daq -> s_size = (int) nchannels;
	} else {
	    if ( nds_logging_check( NDS_LOG_GROUP_STATUS_UPDATE,
				    0 ) )
	    {
		nds_logging_printf( "malloc(%zi) failed; errno=%d\n", 
				    alloc_size, errno);
	    }
	    daq -> s_size = 0;
	    return -1;
	}
    } else if (daq->s_size != nchannels) {
	daq->s = (signal_conv_t1 *) realloc((void *)daq->s, alloc_size);
	if (daq->s) {
	    daq->s_size = (int) nchannels;
	} else {
	    if ( nds_logging_check( NDS_LOG_GROUP_STATUS_UPDATE,
				    0 ) )
	    {
		nds_logging_printf ( "realloc(%zi) failed; errno=%d\n",
				     alloc_size, errno );
	    }
	    daq->s_size = 0;
	    return -1;
	}
    }

    fd = daq->conceal->datafd;
    ps = daq->s;
    for (i = 0; i < nchannels; i++) {
	if (read_float(fd, &ps[i].signal_offset)) 
	    return DAQD_ERROR;

	if (read_float(fd, &ps[i].signal_slope))
	    return DAQD_ERROR;
	read_uint4(fd, &ps[i].signal_status);
    }
    return -2; 
}

/*
 *  Initialize the nds1 protocol.
 */
int
nds1_startup(void) {
    return 0;
}
