/* -*- mode: c; c-basic-offset: 4; -*- */
#ifndef DAQC_NDS1_H
#define DAQC_NDS1_H

/*
 *  Connect to the DAQD server on the host identified by `ip' address.
 *  Returns zero if OK or the error code if failed.
 */
int
nds1_connect (daq_t *daq, const char* host, int port);

/*
 *  Disconnect from the DAQD server. Send a quit command and close the
 *  socket.
 *  Returns zero if OK or the error code if failed.
 */
int
nds1_disconnect (daq_t *daq);

/*
 *  Initialize the nds1 client
 *  Returns zero if OK or the error code if failed.
 */
int
nds1_initalize(void);

/*
 *  Receive channel data using the NDS1 protocol
 */
int
nds1_recv_channels (daq_t *daq, daq_channel_t *channel, int num_channels, 
		    int *num_channels_received);

/*  nds1_request_data(daq_t*, time_t, time_t)
 *
 *  Get requested channel data for the specified interval.
 */
int
nds1_request_data(daq_t* daq, time_t start, time_t dt);

/*  nds1_receive_reconfigure(daq_t* daq, long block_len)
 *
 *  Receive a reconfigure block. receive_reconfigure is received after the 
 *  block header has been read in. The block length does not include the
 *  header length.
 */
int 
nds1_receive_reconfigure(daq_t* daq, size_t block_len);

/*
 *  Initialize the nds1 client
 *  Returns zero if OK or the error code if failed.
 */
int
nds1_startup(void);

/*
 *  Supported NDS1 protocols
 */
#define MAX_NDS1_PROTOCOL_VERSION 12
#define MIN_NDS1_PROTOCOL_VERSION 11

#endif  /* !defined(DAQC_NDS1_H) */
