/* -*- mode: c; c-basic-offset: 4; -*- */
#ifndef DAQC_INTERNAL_H
#define DAQC_INTERNAL_H

#if HAVE_CONFIG_H
#include "daq_config.h"
#endif /* HAVE_CONFIG_H */

#include "daqc.h"
#include "daqc_private.h"

#ifndef DLL_EXPORT
#if WIN32 || WIN64
#define DLL_EXPORT __declspec(dllexport)
#else
#define DLL_EXPORT
#endif /* WIN32 || WIN64 */
#endif /* DLL_EXPORT */

/**  \defgroup daq2_internal Internal and obsolete interfaces.
  *
  *  The internal and obsolete interface group contains the aspects of the  
  *  classic NDS interfadce that have been superceded and the items that 
  *  are not used directly by the client API.
  *  \{
  */

/**  Data block header structure contains the header of the data received 
  *  from the NDS. 
  *  \brief Received data header.
  */
struct daq_block_header {
    /**  Number of second spanned by the data in this block.
      */
    uint4_type secs;

    /**  GPS time-stamp \{seconds, nsec\} applies to the first sample 
      *  in this block. GPS second field.
      */
    uint4_type gps;

    /** Time stamp nano-second field.
      */
    uint4_type gpsn;

    /** Block sequence number; shows if any blocks were dropped 
      */
    uint4_type seq_num;

    /** first byte of data 
      */
    char data [1];
};

/*                   Signal_conv_t1 (internal??)
 */

/**  Signal conversion structure. It's not clear how this is used in the
  *  NDS1 protocol.
  */
typedef struct signal_conv1 {
    /** Signal calibration slope
      */
    float signal_slope;
    /** Signal calibration offset
      */
    float signal_offset;
    /** Signal status code.
      */
    uint4_type   signal_status;
} signal_conv_t1;


/**  Get a list of defined channel groups. daq_recv_channel_groups requests
  *  a list of channel groups from the server and the fills in the list
  *  with the channel group data.
  *  \brief Receive a list of channel groups.
  *  \deprecated Channel groups are replaced by pre-defined channel types.
  *  \param daq Pointer to client status structure.
  *  \param group Pointer to an allocated list of group information.
  *  \param num_groups Maximum number of groups to read (size of list).
  *  \param num_channel_groups_received Number of groups defined.
  *  \return DAQD status code.
  */
DLL_EXPORT int
daq_recv_channel_groups (daq_t* daq, daq_channel_group_t* group, 
			 int num_groups, int* num_channel_groups_received);


/*                   Internal NDS1/2 functions
 */

/**  Look up the server address specified by the host string and port number.
  *  The resulting ip address is stored in \c daq->srvr_addr.
  *  \brief Set the server address.
  *  \param daq  Pointer to the client status structure
  *  \param host Server node name string pointer.
  *  \param port Server port number
  *  \return DAQD status code.
  */
DLL_EXPORT int
daq_set_server(daq_t *daq, const char* host, int port);


/*                   Internal NDS1 functions
 */

/**  Get a 4 hex digit integer data word response from the server.
  *  \brief Get a response code (internal function).
  *  \param fd Opened socked file descriptor.
  *  \return Server response code or -1 if an error occurred.
  */
DLL_EXPORT long
read_server_response(nds_socket_type fd);

/**  Get a 4 hex digit integer data word response from the server.
  *  \brief Get a response integer (internal function).
  *  \param fd  Opened socket file descriptor.
  *  \param wt  Maximum wait time in tenths of a second. Defaults to 10s if 0
  *             is passed.
  *  \return Server response code, or -1 if an error occurred.
  */
DLL_EXPORT long
read_server_response_wait(nds_socket_type fd, int wt);

/**  Swap bytes of data words according to the specified word size from
  *  network (big-endian) byte ordering to the local host byte ordering.
  *  If \c gran<2 no swapping is performed and the number of words
  *  returned is \c nBytes. If the local host uses network byte-ordering
  *  no swapping is performed and the number of words returned is
  *  \c nBytes/gran.
  *  \brief Swap data bytes.
  *  \param gran   Granularity (word size in bytes).
  *  \param nBytes Number of data bytes to be swapped.
  *  \param data   Pointer to the first data byte.
  *  \return Number of words swapped.
  */
DLL_EXPORT size_t
swap_data(size_t gran, size_t nBytes, char* data);

/**  Convert an nds group number to an nds2 channel type code.
  *  \brief Convert group code to channel type.
  *  \param group_num Group code to be converted.
  *  \return Channel type corresponding to the argument group code.
  */
DLL_EXPORT enum chantype
cvt_group_chantype(int group_num);


/**  Convert a null terminated hex string to a long integer.
 *  \param str Pointer to hexidecimal string.
 *  \return Value of hex string or -1 if syntax error.
 */
DLL_EXPORT long
dca_strtol (const char *str);

/**  Wait for data to arrive on the specified socket up to the maximum 
  *  specified time.
  *  \brief Wait for data to arrive.
  *  \param fd File descriptor of the socket to be tested.
  *  \param wt Maximumwait time in seconds.
  *  \return values:
  *   1 Data available
  *   0 Timeout.
  *  -1 Error
  */
DLL_EXPORT int
_daq_wait_data(nds_socket_type fd, int wt);

/**  Read a specified number of bytes from a specified socket.
  *  \brief Read a `numb' bytes.
  *  \param fd   File descriptor to be read from.
  *  \param cptr buffer for read data.
  *  \param numb Number of bytes to be read.
  *  \return Number of bytes read or 0 on error or EOF.
  */
DLL_EXPORT int
read_bytes (nds_socket_type fd, char *cptr, size_t numb);

/**  Read a float from the indicated socket and swap bytes from network
  *  byte ordering (big-endian) to host byte ordering.
  *  \brief Read a float data word.
  *  \param fd   Socket file descriptor.
  *  \param data pointer to the output float word.
  *  \return DAQD error code.
  */
DLL_EXPORT int
read_float(nds_socket_type fd, float* data);

/**  Read an unsigned integer from the indicated socket and swap bytes from 
  *  network byte ordering (big-endian) to host byte ordering.
  *  \brief Read a 4-byte integer data word.
  *  \param fd   Socket file descriptor.
  *  \param data pointer to the output float word.
  *  \return DAQD error code.
  */
DLL_EXPORT int
read_uint4 (nds_socket_type fd, uint4_type* data);

/**  Read an NDS null terminated string from the specified socket.
  *  The string is 
  *  copied to the specified buffer and a null character is added at the 
  *  end of the string if there is room in the buffer.
  *  \brief read an nds string.
  *  \param fd     Socket file descriptor
  *  \param maxlen Length of output buffer.
  *  \param buf    String buffer.
  *  \return String length or -1.
  */
DLL_EXPORT int
_daq_read_cstring(nds_socket_type fd, size_t maxlen, char* buf);

/**  Read an NDS2 byte-counted string from the specified socket. The string 
  *  is transferred as an integer byte-count followed by text. The string is 
  *  copied to the specified buffer and a null character is added at the 
  *  end of the string if there is room in the buffer.
  *  \brief read an nds2 string.
  *  \param fd     Socket file descriptor
  *  \param maxlen Length of output buffer.
  *  \param buf    String buffer.
  *  \return String length or -1.
  */
DLL_EXPORT int
_daq_read_string(nds_socket_type fd, size_t maxlen, char* buf);

/**  Replace the character after the last non-whitespace character in a 
  *  string with a NULL. If the entire string is white-space the first 
  *  character is set to NULL. Whitespace is defined by the isspace() 
  *  function.
  *  \brief Add a NULL after the last non-blank in a string. 
  *  \param p   Pointer to the initial string.
  *  \param len Length of the initial string.
  *  \return New string length (position of the NULL).
  */
DLL_EXPORT int
null_term(char* p, int len);

/**  Copy a data string to the specified memory location. Any leading blanks 
  *  are skipped. The non-blank characters are then copied until a blank is 
  *  found, the end of the input string is reached or the maximum byte count 
  *  is reached. A NULL is appended on the output string if space is available.
  *  The number of bytes copied is returned and the input pointer is updated.
  *  \brief Copy a blank or null terminated string to an output buffer.
  *  \param p    Input string pointer address.
  *  \param end  End of input string.
  *  \param out  Output string.
  *  \param lmax Output buffer length.
  *  \return Number of bytes in output string.
  */
DLL_EXPORT size_t 
_daq_get_string(const char** p, const char* end, char* out, size_t lmax);

/**  Convert the specified integer to a string
  *  \brief Convert an integer to a string.
  *  \param in  Integer to be converted.
  *  \param out Output string.
  *  \return number of digits in the string.
  */
DLL_EXPORT int
_daq_cvt_string(unsigned long in, char* out);

/**  Convert the specified integerlong long to a string
  *  \brief Convert an integer to a string.
  *  \param in  Integer to be converted.
  *  \param out Output string.
  *  \return number of digits in the string.
  */
DLL_EXPORT int
_daq_cvt_ll_string(unsigned long long in, char* out);

/** \}
  */

#if _WIN32
DLL_EXPORT void
nds_perror( const char* Message );
#endif /* _WIN32 */

#if SIZEOF_TIME_T == SIZEOF_LONG
#define daq_cvt_timet_string(In,Out) _daq_cvt_string((unsigned long)In,Out)
#elif SIZEOF_TIME_T == SIZEOF_LONG_LONG
#define daq_cvt_timet_string(In,Out) _daq_cvt_ll_string((unsigned long long)In,Out)
#endif

#endif /* !defined(DAQC_INTERNAL_H) */
