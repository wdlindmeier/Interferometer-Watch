#ifndef NDS_OS_H
#define NDS_OS_H

#include "daqc.h"
#include "daqc_private.h"

/*
 * If you desire not to use the Windows _s functions, then add the following lines to the source file:
 *
 * #if _WIN32
 * #define _CRT_SECURE_NO_WARNINGS 1
 * #endif
 *
 */
void socket_io_non_blocking( nds_socket_type Socket );
void socket_io_default ( nds_socket_type Socket );

void pstrerror_r( char* StrErrBuf, size_t StrErrBufLen );

#if _WIN32
typedef int socklen_t;

#define close(fd) closesocket(fd)
#define dup(oldfd) _dup(oldfd)
#define fdopen(path,mode) _fdopen(path,mode)
#define read(fd,buf,len) recv(fd,buf,len,0)
#define sleep(Seconds) Sleep(Seconds*1000)
#define strdup(str) _strdup(str)
#define usleep(MicroSeconds) Sleep(MicroSeconds/1000)
#define write(fd,buf,count) send(fd,buf,count,0)
#define perror( message ) nds_perror( message )

#define strerror_r(a,b,c)	strerror_s(b,c,a)

#else /* _WIN32 */
#define strncpy_s(dest,dest_len,src,count) \
  strncpy(dest,src,count)
#define strcpy_s(dest,dest_len,src) \
  strcpy(dest,src)
#endif /* _WIN32 */

#endif /* NDS_OS_H */
