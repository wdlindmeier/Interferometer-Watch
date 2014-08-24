/* -*- mode: c; c-basic-offset: 4; -*- */
/*
 *  Data acquisition daemon access
 */
#ifndef DAQC_H
#define DAQC_H

#if __cplusplus
extern "C"
{
#endif /* __cplusplus */

#if WIN32 || WIN64
#define DLL_EXPORT __declspec(dllexport)
#else
#define DLL_EXPORT
#endif /* WIN32 || WIN64 */

#include "channel.h"

typedef          int  int4_type;
typedef unsigned int uint4_type;

#define DAQD_PROTOCOL_VERSION 12
#define DAQD_PROTOCOL_REVISION 0

#define DAQD_PORT 31200

struct sockaddr_in;

struct signal_conv1;

struct daq_private_;

/**  \mainpage NDS2 Application Program Interface
  *  This module defines the client interface to both versions of the Network
  *  Data Server (classic NDS and NDS2). It is based on the "daq_" function 
  *  set originally defined by Alex Ivanov for the classic NDS. The C-functions
  *  provide a low level interface to the server. In general, these must be
  *  used to implement the client server protocol defined elsewhere, e.g.
  *  <a href="http://www.ligo.caltech.edu/~jzweizig/dmt/IO/group__IO__daqs">
  *  here</a>. A new set of client functions maintain a list of channels to
  *  be requested, properly formats the NDS server commands and help retrieve
  *  the requested data.
  *
  *  The following code snippet demonstrates how to use the nds2 client:
  *  \verbatim
     -- Initialize
     --
     int rc = daq_startup();
     if (rc) {
         printf("global initialization failed\n");
	 return 1;
     }

     -- Connect to server
     --
     const char* server = "ldas-pcdev1.ligo.caltech.edu";
     int           port = 31200;
     daq_t daqd;
     rc = daq_connect(&daqd, server, port, nds_v2);
     if (rc) {
         printf("Connection failed with error: %s\n", daq_strerror(rc));
	 return 2;
     }

     -- Specify channels
     --
     const char* channel[4] = {
         "H1:DMT-STRAIN", 
	 "L1:DMT-STRAIN", 
	 "V1:h16384Hz",
	 0
     }

     int i;
     for (i=0; i<4; i++) {
         rc = daq_request_channel(&daqd, channel+i, 0, 0);
     }

     --  Request data
     --
     time_t start_gps = 876543210;
     time_t end_gps   = 876543366;
     time_t delta     = 32;
     rc = daq_request_data(&daqd, start_gps, end_gps, delta);
     if (rc) {
         printf("Data request failed with error: %s\n", daq_strerror(rc));
	 return 3;
     }

     --  Read data blocks
     --
     time_t t;
     for (t=start_gps; t<end_gps; t+=delta) {
         rc = daq_recv_next(&daqd);
	 if (rc) {
             printf("Receive data failed with error: %s\n", daq_strerror(rc));
	     return 4;
	 }

	 --  Get data
         --
	 chan_req_t* stat = daq_get_channel_status(&daqd, channel[1]);
	 if (!stat || stat->status <= 0) break;
	 
     }

     --  Disconnect from server
     --
     daq_disconnect(&daqd);
     \endverbatim
  *
  *  The API is subdivided into the following modules:
  *  - \ref daq2_api Access functions and control/status structures.
  *  - \ref daq2_listener Listener thread (deprecated)
  *  - \ref daq2_access_rc Return codes used by the functions.
  *  - \ref daq2_internal Internal and obsolete stuff.
  *  \{
  */

/**  \defgroup daq2_api NDS2 access functions.
  *  This module defines the NDS2 API. The C-functions provide a backward-
  *  compatible low level interface that may be used with either version of
  *  the NDS. These may used to implement the client server protocol defined 
  *  elsewhere, e.g.
  *  <a href="http://www.ligo.caltech.edu/~jzweizig/dmt/IO/group__IO__daqs">
  *  classic NDS protocol</a> or NDS protocol.
  *  A second high level interface is new to the API. This allows the user
  *  to make the server requests to either version of the NDS and without
  *  knowledge of the server protocol.
  *  \{
  */

/** NDS version enumerator
  */
enum nds_version {

    /** Try nds2 first, revert to nds1 on failure */
    nds_try = 0,

    /** nds1 server protocol */
    nds_v1 = 1,

    /** nds2 server protocol */
    nds_v2 = 2
};

/* pieces of the communication protocol */

/** Block header structure length.
  */
#define HEADER_LEN 16

/** Block header data type
  */
typedef struct daq_block_header daq_block_t;

/**  The signal_conv_t structure contains the linear transformation
  *  coefficients needed to convert the signals to engineering units.
  *  \brief Data unit conversion structure.
  */
struct signal_conv_ {
    /**  Overall multiplicative factor.
      */
    float signal_gain;

    /**  Value of readout unit ??? in engineering units.
      */
    float signal_slope;

    /**  Value of readout units ??? for zero engineering units.
      */
    float signal_offset;

    /** Null terminated name ??? of engineering units.
      */
    char signal_units [MAX_SIGNAL_UNIT_LENGTH]; /* Engineering units  */  
};

typedef struct signal_conv_ signal_conv_t;

/**  The channel request entry contains the information about the requested
  *  channels and current status of the request. A list of channels is 
  *  prepared by the high level interface and updated as part of the channel
  *  NDS2 protocol.
  *  \brief Channel request/status entry.
  */
struct chan_req_ {
    /**  Null terminated channel name string */
    char*  name;

    /**  Requested channel type. */
    enum chantype type;

    /**  Requested rate. If the rate is not specified, \c rate is set to zero.
 `    */
    double rate;

    /**  Channel data type. This field is filled by the NDS2 server or by 
      *  the user either by hand or from the daq_request_channel_from_list.
      *  Data types used by the NDS1 server are:
      *  - 1: short
      *  - 4: float
      */
    daq_data_t data_type;

    /**  Offset of data into the buffer.
`     */
    uint4_type offset;

    /**  Status of data request for this channel. If data has been read
      *  successfully, status is the data length. If an error occurred, the
      *  status code is negative and indicates the status condition.
      *  \brief status or data length.
      */
    int    status;

    /**  Linear transform for signal unit scaling.
      */
    signal_conv_t s;

};

typedef struct chan_req_ chan_req_t;


/**  The daq_t structure contains all the status and control information 
  *  for the NDS client. It is used by each of the daq_access functions.
  *  \brief NDS1/2 client status structure
  */
struct daq_ {
    /**  NDS server type (NDS1=1 or NDS2=2)
      *  \brief NDS server type
      */
    enum nds_version nds_versn;

    /**  Number of channels requested.
      */
    uint4_type num_chan_request;

    /**  Number of channels request structures allocated.
      */
    uint4_type num_chan_alloc;

    /** Channel request list.
     */
    chan_req_t* chan_req_list;

    /**  Block count??? zero for on-line transmission; 
      *  positive for off-line
      */
    int blocks;

    /**  transmission block; dynamically allocated and reallocated as 
      *  needed; freed in daq_recv_shutdown()
      */
    daq_block_t *tb;

    /**  size of the above malloced data 
      */
    size_t tb_size;

    /**  signal conversion data; dynamically allocated and reallocated 
      *	 as needed; freed in daq_recv_shutdown()
      */
    struct signal_conv1 *s;

    /**  size of the above malloced data in elements 
      *  (sizeof(signal_conv_t1) each)
      */
    int s_size;

    /**  NDS1 server protocol version received by NDS1 'version' command 
      */
    int nds1_ver;

    /**  NDS1 server protocol revision received by NDS1 'revision' command 
      */
    int nds1_rev;

    /** authentication context
      */
    void* auth_ctx;

    /**  Alternate error number.
      */
    int err_num;

    /** Handle to internal data
     */
    struct daq_private_*	conceal;
};

typedef struct daq_ daq_t;

/**  The channel descriptor contains information about the channels known to
  *  the NDS server. These data are transferred to the client as a list of 
  *  such structures.
  *  \brief Channel description structure
  */
struct daq_channel_ {
    /**  Channel name as a null-terminated character string.
      */
    char name [MAX_LONG_CHANNEL_NAME_LENGTH + 1]; /* Channel name */

    /**  Sample rate. The NDS1 protocol had an integer rate which is not 
      *  consistent with minute trend requests.
      */
    double rate;

    /**  Test point number; 0 for normal channels
      */
    int tpnum;

    /**  Channel type code. This field replace the group number used in 
      *  the NDS1 protocol. It resolves cases where there are different
      *  channels with the same name, or where a single named channel may 
      *  several sources. See the chantype enum for more details.
      */
    enum chantype type;

    /** Bytes per sample
      */
    int bps;

    /**  channel number.
      */
    int chNum;

    /**  Sample data type
      */
    daq_data_t data_type;

    /**  Conversion to engineering units.
      */
    signal_conv_t s;
};

typedef struct daq_channel_  daq_channel_t;

/*                   Channel Group interface (Version 1 Only)
 */

/**  Structure containing the name and number of a channel group.
  *  \brief Channel group structure
  *  \deprecated The channel type code replaceses the channel group concept.
  */
struct daq_channel_group_ {
    /**  Channel group number
     */
    int group_num;
    /**  Channel group name.
      */
    char name [MAX_CHANNEL_NAME_LENGTH + 1];
};

typedef struct daq_channel_group_ daq_channel_group_t;

/**  Zero the number of requested channels. The number of allocates channel
  *  requests is not affected.
  *  \brief Clear the channel list.
  *  \param daq Pointer to client status structure.
  *  \return The id or -1.
  *  
  */
DLL_EXPORT int
daq_clear_channel_list(daq_t* daq);

/**  Connect to the DAQD server on the host identified by `ip' address.
  *  A socket is created using a socket() function call. The host address
  *  is found using the gethostbyname() function and a connection is 
  *  established. The server is then asked for the protocol version and 
  *  revision with "version;" and "revision;" commands. The version/revision 
  *  are saved in the client structure.
  *  \brief Make a connection to the NDS1 server.
  *  \param daq Client status structure/
  *  \param host Server host name string.
  *  \param port Server port number.
  *  \param version NDS version (nds_v1 or nds_v2).
  *  \return zero if successful or DAQD status code.
  */
DLL_EXPORT int 
daq_connect(daq_t* daq, const char* host, int port, enum nds_version version);

/**  Disconnect from the server by sending a "quit;" command and close 
  *  the socket file descriptor.
  *  \brief Disconnect from the server.
  *  \param daq Pointer to client status structure.
  *  \return DAQD status code.
  */
DLL_EXPORT int 
daq_disconnect (daq_t* daq);

/**  Get a pointer to the data block from a specified channel name. Note
  *  that the returned address is in the daqd temporary buffer. It is 
  *  likely that the data will be replaced during the next call to the 
  *  daq_recv_next and possible that the data for the requested channel
  *  will be in a different location after the next request. If an error
  *  occurred in fetching the data, the returned pointer is NULL and the 
  *  error code is stored in the daq->err_num.
  *  \brief Get a pointer to the data for the specified channel.
  *  \param daq Pointer to client status structure.
  *  \param channel Pointer to null terminated channel name string.
  *  \return Pointer to channel data or NULL.
  */
DLL_EXPORT const char*
daq_get_channel_addr(daq_t *daq, const char* channel);

/**  Copy the data from a specified channel name to the target address. The 
  *  target buffer must be large enough for the full data record. If the
  *  requested data are not available a NULL pointer is returned and the 
  *  error code is stored in daq->err_num.
  *  \brief Copy channel data.
  *  \param daq Pointer to client status structure.
  *  \param channel Pointer to null terminated channel name string.
  *  \param data pointer to a buffer to receive channel data.
  *  \return Pointer to channel data buffer or NULL.
  */
DLL_EXPORT char*
daq_get_channel_data(daq_t *daq, const char* channel, char* data);

/**  Get the number of bytes of data retrieved for the specified channel.
  *  \brief Get the channel data length.
  *  \param daq Pointer to client status structure.
  *  \param channel Pointer to null terminated channel name string.
  *  \return Number of data bytes or 0 if channel not found.
  */
DLL_EXPORT int
daq_get_data_length(daq_t *daq, const char* channel);

/**  Get a pointer specified channel request/status block.
  *  \brief Get a pointer to channel data.
  *  \param daq Pointer to client status structure.
  *  \param channel Pointer to null terminated channel name string.
  *  \return Pointer to channel request/status structure.
  */
DLL_EXPORT chan_req_t*
daq_get_channel_status(daq_t *daq, const char* channel);

/**  Copy the data from a specified channel name to the target address.
  *  The data are converted to floats and scaled according to the linear
  *  transform specified by the NDS server. Complex data are not copied
  *  converted or scaled. The number of output data words is returned. It 
  *  is up  to the user to insure that the output array is large enough to
  *  accommodate the data.
  *  \brief Calibrate and copy channel data.
  *  \param daq Pointer to client status structure.
  *  \param channel Pointer to null terminated channel name string.
  *  \param data pointer to a buffer to receive calibrated channel data.
  *  \return Number of data words copied.
  */
DLL_EXPORT int
daq_get_scaled_data(daq_t *daq, const char* channel, float* data);

/**  Initialize a daq_channel_t structure with the specified data. If the
  *  channel type is \c _unknown and the name contains a trend channel 
  *  suffix (\a e.g. \c .mean ) the type will be reset to a trend type 
  *  in accordance with the specified rate. Trend channels are assigned
  *  a rate and data_type appropriate to the trend type. If a trend-type
  *  channel does not have an appropriate suffix, ".mean" is appended to 
  *  the channel name.
  *  \brief Initialize a \c daq_channel_t structure.
  *  \param chan  Pointer to structure to be initialized
  *  \param name  Channel name
  *  \param ctype Channel type code
  *  \param rate  Sample rate
  *  \param dtype Data type.
  */
DLL_EXPORT void
daq_init_channel(daq_channel_t* chan, const char* name, enum chantype ctype,
		 double rate, daq_data_t dtype);

/**  Receive a data block number or zero for online data.
  *  \brief Receive block number.
  *  \param daq Pointer to client status structure.
  *  \return Block number or -1 on error.
  */
DLL_EXPORT int
daq_recv_block_num (daq_t* daq);

/**  Receive one data block (data channel samples). A transmission buffer is 
  *  allocated as needed and its pointer is assigned to `daq->tb'. 
  *  The block size is assigned to `daq->tb_size'. A zero length block 
  *  consists of the block header and no data. It is sent by the server when 
  *  it fails to find the data for the GPS second, specified in the block 
  *  header (Note: this can only happen for the off-line data request). If 
  *  channel reconfiguration has occurred, the server sends a special 
  *  reconfiguration data block. For the client it means he needs to reread 
  *  channel data conversion variables and status from the *daq structure.
  *  \brief Receive a data block.
  *  \param daq Pointer to client status structure.
  *  \return <ul> 
  *    <li>&gt;0: Number bytes of sample data read</li>
  *    <li>0: Zero length data block is received</li>
  *    <li>-1: Error</li>
  *    <li>-2: Channel reconfiguration</li>
  *  </ul>
  */
DLL_EXPORT int 
daq_recv_block (daq_t* daq);

/**  Receive one data block, handle all additional protocol, e.g. channel
  *  reconfiguration, and swap bytes according to channel list. Data are 
  *  stored in the internal buffer. For NDS2 requests, the request list is 
  *  updated with data length, offset, data type and status of each channel.
  *  For NDS1 the request list is updated with the inferred lengths and 
  *  offsets.
  *  \brief Receive a data block.
  *  \param daq Pointer to client status structure.
  *  \return DAQD status code.
  */
DLL_EXPORT int
daq_recv_next (daq_t* daq);

/**  Close the data connection socket and free the allocated buffers 
  *  (conversion list and transmission buffer). 
  *  \brief Close the client connection.
  *  \param daq Pointer to client status structure.
  *  \return DAQD status code.
  */
DLL_EXPORT int
daq_recv_shutdown (daq_t* daq);

/**  Get a list of online channel names and information. \c daq_recv_channels 
  *  requests the channel information as appropriate for the server version. 
  *  All channel data are then copied into the list. This function is included
  *  to allow backward compatibility with the classic nds interface and is 
  *  implemented with a call to \c daq_recv_channel_list. See the documentation
  *  for \c daq_recv_channel_list for further details.
  *  \brief Get a list of online channels.
  *  \param daq Pointer to client status structure.
  *  \param channel List of daq_channel structures to receive information.
  *  \param num_channels Number of channel structures allocated.
  *  \param num_channels_received Number of channels defined.
  *  \return DAQD status code.
  */
DLL_EXPORT int 
daq_recv_channels (daq_t* daq, daq_channel_t* channel, int num_channels, 
		   int* num_channels_received);

/**  Get a list of channel names and information. A list of all channels 
  *  available at the specified time and of the specified type is returned.
  *  A \c GPS time of zero indicates currently available channels. A type
  *  code of \c cUnknown (0) gives all channel types available at the specified 
  *  gps time. Note that this will produce a list with multiple channel entries 
  *  with identical names. The number of channels returned in
  *  \c *num_channels_received is the total number of channels available from
  *  the server and may exceed the number of channels for which space has been
  *  allocated. In fact, an efficient way to find out how much space is needed
  *  for the channel list is to call daq_recv_channel_list with \c num_channels
  *  set to zero and the channel list pointer set to NULL.
  *  \remarks
  *  This function only works correctly with the NDS2 server. If a non-zero
  *  GPS or a data type other than \c cOnline is requested, the function 
  *  returns \c DAQD_VERSION_MISMATCH and \c *num_channels_received is set 
  *  to zero.
  *  \brief Get a list of channels.
  *  \param daq Pointer to client status structure.
  *  \param channel List of daq_channel structures to receive information.
  *  \param num_channels Number of channel structures allocated.
  *  \param num_channels_received Pointer to integer to receive number of 
  *                               channels defined on server (may exceed 
  *                               \c num_channels ).
  *  \param gps validity time (or zero for current channels)
  *  \param type Limit list to specified type.
  *  \return DAQD status code.
  */
DLL_EXPORT int 
daq_recv_channel_list(daq_t* daq, daq_channel_t* channel, int num_channels, 
		   int* num_channels_received, time_t gps, enum chantype type);

/**  Receive an eight-digit hex ID.
  *  \brief Receive an ID.
  *  \param daq Pointer to client status structure.
  *  \return The id or -1
  */
DLL_EXPORT long 
daq_recv_id (daq_t *daq);

/**  Get a list of channel sources for the requested channels at the specified
  *  gps time. A \c GPS time of zero indicates currently available channels. 
  *  \remarks
  *  This function only works with the NDS2 server.
  *  \brief Get a list of source frames.
  *  \param daq Pointer to client status structure.
  *  \param sources List of daq_channel structures to receive information.
  *  \param max_len Pre-allocated string length.
  *  \param gps validity time (or zero for current channels)
  *  \param str_len Output string length pointer.
  *  \return DAQD status code.
  */
DLL_EXPORT int 
daq_recv_source_list(daq_t* daq, char* sources, size_t max_len, 
		     time_t gps, long* str_len);

/**  Add the specified channel to the request list. If the type is set to 
  *  the default value (\c cUnknown or 0), the channel type will be inferred 
  *  from the channel name and sample rate as described in the documentation of 
  *  \c daq_init_channel. A zero sample rate results in the full-bandwidth 
  *  data of the specified type.
  *  \brief Add a channel to the request list.
  *  \param daq  Pointer to client status structure.
  *  \param name Pointer to null terminated name string.
  *  \param type Channel type.
  *  \param rate Requested sampling rate (in Hz).
  *  \return DAQD status code.
  */
DLL_EXPORT int
daq_request_channel(daq_t* daq, const char* name, enum chantype type,
		    double rate);

/**  Add the specified channel to the request list. The channel name data
  *  type, sample rate, etc are copied from the channel list entry.
  *  \brief Add a channel to the request list.
  *  \param daq     Pointer to client status structure.
  *  \param channel Pointer channel list entry.
  *  \return DAQD status code.
  */
DLL_EXPORT int
daq_request_channel_from_chanlist(daq_t* daq, daq_channel_t* channel);

/**  Request data for all channels added to the list for the specified 
  *  interval and stride width. Data are recived from the start time to 
  *  the end time in strides of \c dt seconds. An online data request is 
  *  made if the start time is zero and only online channels are specified.
  *  Online request end times may be specified either as an absolute GPS 
  *  or as a total data length in seconds (if end is greater than the 
  *  current time).
  *  Note that the NDS1 protocol ignores the stride length specifier.
  *  \brief Get requested data.
  *  \param daq   Pointer to client status structure.
  *  \param start Start time (GPS) or 0 for online data.
  *  \param end   End time (GPS).
  *  \param dt    Time stride.
  *  \return Server response code.
  */
DLL_EXPORT int
daq_request_data(daq_t* daq, time_t start, time_t end, time_t dt);

/**  Send a null-terminated command string to the server. Then read and 
  *  return a response code.
  *  \brief send a command string.
  *  \param daq Pointer to client status structure.
  *  \param command Null terminated command text string.
  *  \return DAQD status code.
  */
DLL_EXPORT int 
daq_send (daq_t* daq, const char* command);

/**  Set the default epoch for this transaction.
  *  \brief Set the default epoch.
  *  \return Error code or 0 on success.
  */
DLL_EXPORT int
daq_set_epoch(daq_t* daq, const char* epoch);

/**  System initialization.
  *  \brief Initialize nds1/nds2 client subsystems.
  *  \return Error code or 0 on success.
  */
DLL_EXPORT int
daq_startup(void);

/**  Return string equivalent to a return code.
  *  \brief determine English equivalent of return code.
  *  \param errornum error return code.
  *  \return pointer to static error message string.
  */
DLL_EXPORT const char*
daq_strerror(int errornum);

/** Return the number of secons spanned by the data in
 * the block.
 */
DLL_EXPORT uint4_type
daq_get_block_secs( daq_t* daq );

/** Return time stamp second field of the first
 *  sample in the block
 */
DLL_EXPORT uint4_type
daq_get_block_gps( daq_t* daq );

/** Return time stamp nano-second field of the first
 *  sample in the block
 */
DLL_EXPORT uint4_type
daq_get_block_gpsn( daq_t* daq );

/** Return the block sequence number; shows if any blocks
 *  were dropped.
 */
DLL_EXPORT uint4_type
daq_get_block_seq_num( daq_t* daq );

/** Return starting address of the data in the block.
 */
DLL_EXPORT char*
daq_get_block_data( daq_t* daq );

/** \}
  */

/** \}
  */


#if __cplusplus
}
#endif /* __cplusplus */

#endif /* DAQC_H */
