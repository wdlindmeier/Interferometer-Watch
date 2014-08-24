/* -*- mode: c; c-basic-offset: 4; -*- */
#ifndef DAQC_NDS2_H
#define DAQC_NDS2_H

/*
 *  Connect to the NDS2 server on the host identified by `ip' address.
 *  Returns zero if OK or the error code if failed.
 */
int
nds2_connect (daq_t *daq, const char* host, int port);

/*
 *  Receive channel data using the NDS2 protocol
 */
int 
nds2_recv_channel_list(daq_t *daq, daq_channel_t *channel, int num_channels, 
		       int *num_channels_received, time_t gps, 
		       enum chantype type);

/*
 *  Receive source list using the NDS2 protocol
 */
int 
nds2_recv_source_list(daq_t *daq, char* list, size_t max_len, 
		      time_t gps, long* str_len);

/*
 *  Disconnect from the NDS2 server. Send a quit command, delete the sasl
 *  context and close the socket.
 *  Returns zero if OK or the error code if failed.
 */
int
nds2_disconnect (daq_t *daq);

/*  nds2_request_data(daq_t*, time_t, time_t)
 *
 *  Get requested channel data for the specified interval.
 */
int nds2_request_data(daq_t* daq, time_t start, time_t end, time_t dt);

/*  nds2_receive_reconfigure(daq_t* daq, long block_len)
 *
 *  Receive a reconfigure block. receive_reconfigure is received after the 
 *  block header has been read in. The block length does not include the
 *  header length.
 */
int 
nds2_receive_reconfigure(daq_t* daq, size_t block_len);

/*
 *  Set the default epoch for this session.
 */
int
nds2_set_epoch(daq_t* daq, const char* epoch);

/*
 *  Initialize the nds2 client
 *  Returns zero if OK or the error code if failed.
 */
int
nds2_startup(void);

#endif
