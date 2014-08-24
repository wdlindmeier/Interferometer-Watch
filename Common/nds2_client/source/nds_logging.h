#ifndef NDS_LOGGING_H
#define NDS_LOGGING_H

#ifdef __cplusplus
extern "C" {
  /* } */
#endif

#ifndef DLLEXPORT
#if WIN32 || WIN64
#define DLL_EXPORT __declspec(dllexport)
#else
#define DLL_EXPORT
#endif /* WIN32 || WIN64 */
#endif /* DLLEXPORT */

#define NDS_LOG_BAD_GROUP	 	-1
#define NDS_LOG_GROUP_CONNECTION 	0
#define NDS_LOG_GROUP_VERBOSE_ERRORS	1
#define NDS_LOG_GROUP_TRACE_ENTRY_EXIT	2
#define NDS_LOG_GROUP_STATUS_UPDATE	3
#define NDS_LOG_GROUP_USER		4
#define NDS_LOG_GROUP_SIZE_MAX		5

#define NDS_LOGGING_HELPER_ENTRY	nds_logging_helper_init
#define STRING_IT(x) #x
#define XSTRING_IT(x) STRING_IT(x)
#define NDS_LOGGING_HELPER_ENTRY_STRING XSTRING_IT(NDS_LOGGING_HELPER_ENTRY)

typedef void (*nds_flush_function_type)( );

typedef void (*nds_logging_function_type)( const char* Message );

  typedef void (*nds_logging_helper_entry_type)( );

DLL_EXPORT void nds_function_flush( nds_flush_function_type Func );

DLL_EXPORT void nds_function_logging( nds_logging_function_type Func );

DLL_EXPORT int nds_logging_check( int Group, int Level );

DLL_EXPORT void nds_logging_enable( int Group );

DLL_EXPORT void nds_logging_disable( int Group );

DLL_EXPORT int nds_logging_debug_level( int Group, int Level );

DLL_EXPORT void nds_logging_flush( );

DLL_EXPORT int nds_logging_group_from_string( const char* GroupString );

DLL_EXPORT void nds_logging_init( );

DLL_EXPORT void nds_logging_printf( const char* MessageFormat, ... );

DLL_EXPORT void nds_logging_print_errno( const char* Leader );

#ifdef __cplusplus
/* { */
}
#endif

#endif /* NDS_LOGGING_H */
