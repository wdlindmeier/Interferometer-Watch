/* -*- mode: c; c-basic-offset: 4; -*- */
#ifndef NDS_AUTH_H
#define NDS_AUTH_H

#include "daqc.h"

/**  Perform authentication handshake with the server. The socket defined
  *  in the daq structure must be connected to the server.
  *  @memo Authenticate client to server.
  *  @param daq NDS connection descriptor.
  *  @param server Server name string.
  *  @return DAQD response code.
  */
int nds_authenticate(daq_t *daq, const char* server);

/**  Disconnect and delete authentication context.
  *  @memo Clean up after authentication.
  *  @param daq NDS connection descriptor.
  *  @return DAQD response code.
  */
int nds_auth_disconnect(daq_t *daq);

/**  Perform global authentication system initialization.
  *  @memo Initialize authentication mechanism.
  *  @return DAQD response code.
  */
int nds_auth_startup(void);

#endif  /* !defined(NDS_AUTH_H) */
