#ifndef NDS_LOG_H
#define NDS_LOG_H

#ifndef DLLEXPORT
#if WIN32 || WIN64
#define DLL_EXPORT __declspec(dllexport)
#else
#define DLL_EXPORT
#endif /* WIN32 || WIN64 */
#endif /* DLLEXPORT */

DLL_EXPORT void nds_flush( );

DLL_EXPORT void nds_log( const char* Message );

#endif /* NDS_LOG_H */
