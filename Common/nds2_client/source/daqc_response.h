
#ifndef DAQC_RESPONSE_H
#define DAQC_RESPONSE_H

/* response codes */

/**  \defgroup daq2_access_rc NDS2 access error codes.
  *  \{
  */

/** Successful completion */
#define DAQD_OK 0x0000

/** Generic error code */
#define DAQD_ERROR 0x0001

/**  One or more fields in the daqd structure are not initialized.
  *  Usually daq_connect was not called or failed.
  *  \brief Daqd is not configured 
  */
#define DAQD_NOT_CONFIGURED 0x0002

/**  Attempt to get host ip address from hostname failed.
  *  \brief Invalid IP address 
  */
#define DAQD_INVALID_IP_ADDRESS 0x0003

/** Invalid channel name */
#define DAQD_INVALID_CHANNEL_NAME 0x0004

/** Client failed to get socket */
#define DAQD_SOCKET 0x0005

/** Unable to set client socket options */
#define DAQD_SETSOCKOPT 0x0006

/**  Attempt to connect to the specified port failed. The server 
  *  address or port may be incorrectly specified or there may be
  *  no path to the server.
  *  \brief Unable to connect to server 
  */
#define DAQD_CONNECT 0x0007

/** NDS server is overloaded */
#define DAQD_BUSY 0x0008

/** Insufficient memory for allocation */
#define DAQD_MALLOC 0x0009

/** Error occurred trying to write to socket */
#define DAQD_WRITE 0x000a

/** Communication protocol version mismatch */
#define DAQD_VERSION_MISMATCH 0x000b

/** No such net writer (nds1) */
#define DAQD_NO_SUCH_NET_WRITER 0x000c

/** Requested data were not found */
#define DAQD_NOT_FOUND 0x000d

/** Could not get client's IP address */
#define DAQD_GETPEERNAME 0x000e

/** Error in dup() (obsolete) */
#define DAQD_DUP 0x000f

/** Requested data rate is invalid for channel */
#define DAQD_INVALID_CHANNEL_DATA_RATE 0x0010

/** Shutdown request failed. */
#define DAQD_SHUTDOWN 0x0011

/** Trend data are not available (nds1) */
#define DAQD_NO_TRENDER 0x0012

/** Full channel data are not available (nds1) */
#define DAQD_NO_MAIN 0x0013

/** No offline data (nds1) */
#define DAQD_NO_OFFLINE 0x0014

/** Unable to create thread (obsolete) */
#define DAQD_THREAD_CREATE 0x0015

/** Too many channels or too much data requested */
#define DAQD_TOO_MANY_CHANNELS 0x0016

/** Command syntax error */
#define DAQD_COMMAND_SYNTAX 0x0017

/** Request sasl authentication protocol (nds2 only) */
#define DAQD_SASL 0x0018

/* IMPORTANT: leave NOT_SUPORTED msg the last */
/** Requested feature is not supported */
#define DAQD_NOT_SUPPORTED 0x0019

/** \}
  */

#endif
