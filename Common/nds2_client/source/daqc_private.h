#ifndef DAQC_PRIVATE_H
#define DAQC_PRIVATE_H

#include "daqc_net.h"

#if defined(WIN32) || defined(WIN64)
typedef SOCKET nds_socket_type;
#else /* WIN32 || WIN64 */
typedef int nds_socket_type;
#endif /* WIN32 || WIN64 */

#if __cplusplus
extern "C" {
#endif /* __cplusplus */

  typedef struct sockaddr_in socket_addr_t;

  enum socket_states {
    NDS_SOCKET_OK,
    NDS_SOCKET_FAILURE,
    NDS_SOCKET_TRANSIENT_FAILURE
  };

  struct daq_private_
  {
    /**  Socket to which the connection request is made. By default this 
     *  is 8088.
     *  \brief DAQD server socket
     */
    nds_socket_type sockfd;

    /**  DAQD (NDS1) server  IP address
     */
    socket_addr_t* srvr_addr;

    /** Data connection socket 
     */
    nds_socket_type datafd;
  };

  typedef struct daq_private_ daq_private_t;

  void daq_private_create( daq_private_t** Private );

  int daq_private_data_close( daq_private_t* Private );

  void daq_private_delete( daq_private_t** Private );

  int daq_private_srvr_nonblocking( daq_private_t* Private,
				    int NonBlocking );

  void daq_private_srvr_close( daq_private_t* Private );

  int daq_private_srvr_connect( daq_private_t* Private );

  void daq_private_srvr_disconnect( daq_private_t* Prvate );

  int daq_private_srvr_open( daq_private_t* Private );

#if __cplusplus
}
#endif /* __cplusplus */

#endif /* DAQC_PRIVATE_H */
