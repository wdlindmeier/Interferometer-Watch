#if HAVE_CONFIG_H
#include "daq_config.h"
#endif /* HAVE_CONFIG_H */

#if HAVE_FCNTL_H
#include <fcntl.h>
#endif /* HAVE_FCNTL_H */

#include "nds_os.h"

void
socket_io_non_blocking( nds_socket_type Socket )
{
#ifdef O_NONBLOCK
    fcntl( Socket, F_SETFL, O_NONBLOCK );
#else
#endif
}

void
socket_io_default ( nds_socket_type Socket )
{
#ifdef O_NONBLCK
    fcntl( Socket, F_SETFL, 0);
#else
#endif
}

#if 0
#if ! HAVE_SLEEP
unsigned int
sleep( unsigned int Seconds )
{
  DWORD	duration = Seconds * 1000;

  Sleep( duration );
}
#endif /* HAVE_SLEEP */

#if ! HAVE_USLEEP
void
usleep( unsigned long MicroSeconds )
{
  DWORD	duration = MicroSeconds / 1000;

  Sleep( duration );
}
#endif /* HAVE_USLEEP */
#endif /* 0 */

