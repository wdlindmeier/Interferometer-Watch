#ifndef DAQC_NET_H
#define DAQC_NET_H

#include <sys/types.h>
#if HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#else
#if HAVE_WS2DEF_H
#include <Ws2def.h>
#else
#if HAVE_WINSOCK2_H
#include <Winsock2.h>
#endif /* HAVE_WINSOCK2_H */
#endif /* HAVE_WS2DEF_H */
#endif /* HAVE_SYS_SOCKET_H */
#if HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif /* HAVE_NETINET_IN_H */
#if HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif /* HAVE_ARPA_INET_H */
#if HAVE_NETDB_H
#include <netdb.h>
#endif /* HAVE_NETDB_H */

#endif /* DAQC_NET_H */
