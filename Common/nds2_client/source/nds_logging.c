#include "daq_config.h"

#if HAVE_ERRNO_H
#include <errno.h>
#endif /* HAVE_ERRNO_H */
#if HAVE_STDLIB_H
#include <stdlib.h>
#endif
#if HAVE_STDIO_H
#include <stdio.h>
#endif
#if HAVE_STDARG_H
#include <stdarg.h>
#endif
#if HAVE_STRING_H
#include <string.h>
#endif
#if HAVE_DLFCN_H
#include <dlfcn.h>
#endif

#include "nds_log.h"
#include "nds_logging.h"

#if _WIN32
static char* strsep(char** stringp, const char* delim);
#endif /* _WIN32 */

#if DEBUG
#define DEFAULT_VERBOSE_DEBUGGING 1
#else
#define DEFAULT_VERBOSE_DEBUGGING 0
#endif

struct nds_log_ {
  int	s_group_mask;
  int	s_group_debug_level[ NDS_LOG_GROUP_SIZE_MAX ];
  nds_logging_function_type	s_logging_func;
  nds_flush_function_type	s_flush_func;
};

typedef struct nds_log_ nds_log_t;

static nds_log_t
log_info = {
  /* --------------------------------------------------------------------
   * Set bit mask of things that should be debugged by
   * default.
   * ----------------------------------------------------------------- */
#if DEFAULT_VERBOSE_DEBUGGING
  ( 1 << NDS_LOG_GROUP_CONNECTION )
  | ( 1 << NDS_LOG_GROUP_VERBOSE_ERRORS )
  | ( 1 << NDS_LOG_GROUP_TRACE_ENTRY_EXIT )
  | ( 1 << NDS_LOG_GROUP_STATUS_UPDATE )
#else	/* DEFAULT_VERBOSE_DEBUGGING */
  0
#endif /* DEFAULT_VERBOSE_DEBUGGING */
  ,
  /* --------------------------------------------------------------------
   * Setting of the default logging levels
   * ----------------------------------------------------------------- */
  {
    30,	/* NDS_LOG_GROUP_CONNECTION */
    30, /* NDS_LOG_GROUP_VERBOSE_ERRORS */
    20, /* NDS_LOG_GROUP_TRACE_ENTRY_EXIT */
    20, /* NDS_LOG_GROUP_STATUS_UPDATE */
    20, /* NDS_LOG_GROUP_USER */
  },
  (nds_logging_function_type)nds_log,
  (nds_flush_function_type)nds_flush
};


void
nds_function_flush( nds_flush_function_type Func )
{
  log_info.s_flush_func = Func;
}

void
nds_function_logging( nds_logging_function_type Func )
{
  log_info.s_logging_func = Func;
}

int
nds_logging_check( int Group, int Level )
{
  int retval = 0;

  if ( ( ( 1 << Group ) & log_info.s_group_mask )
       && ( log_info.s_group_debug_level[ Group ] >= Level ) )
  {
    retval = 1;
  }

  return retval;
}

void
nds_logging_enable( int Group )
{
  if ( ( Group < 0 )
       || ( Group >= NDS_LOG_GROUP_SIZE_MAX ) )
  {
    return;
  }
  log_info.s_group_mask |= ( 1<<Group );
}

void
nds_logging_disable( int Group )
{
  if ( ( Group < 0 )
       || ( Group >= NDS_LOG_GROUP_SIZE_MAX ) )
  {
    return;
  }
  log_info.s_group_mask &= ~( 1<<Group );
}

int
nds_logging_debug_level( int Group, int Level )
{
  int retval = 0;
  if ( ( Group >= 0 )
       && ( Group < NDS_LOG_GROUP_SIZE_MAX ) )
  {
    retval = log_info.s_group_debug_level[ Group ];
    log_info.s_group_debug_level[ Group ] = Level;
  }
  return retval;
}

void
nds_logging_flush( )
{
  if ( log_info.s_flush_func )
  {
    (*log_info.s_flush_func)( );
  }
}

int
nds_logging_group_from_string( const char* GroupString )
{
  int retval = NDS_LOG_BAD_GROUP;

  if ( strcmp( GroupString, "CONNECTION" ) == 0 )
  {
    retval = NDS_LOG_GROUP_CONNECTION;
  }
  else if ( strcmp( GroupString, "VERBOSE_ERRORS" ) == 0 )
  {
    retval = NDS_LOG_GROUP_VERBOSE_ERRORS;
  }
  else if ( strcmp( GroupString, "TRACE_ENTRY_EXIT" ) == 0 )
  {
    retval = NDS_LOG_GROUP_TRACE_ENTRY_EXIT;
  }
  else if ( strcmp( GroupString, "STATUS_UPDATE" ) == 0 )
  {
    retval = NDS_LOG_GROUP_STATUS_UPDATE;
  }
  else if ( strcmp( GroupString, "USER" ) == 0 )
  {
    retval = NDS_LOG_GROUP_USER;
  }
  return retval;
}

void
nds_logging_init( )
{
  static int uninitialized = 1;

  nds_logging_printf("nds_logging_init: Entry");
  if ( uninitialized == 1 )
  {
    char *library = /* getenv("NDS_LOGGING_LIBRARY") */ NULL;
    char *info = getenv("NDS_LOGGING");

    if ( library )
    {
      /*
       * Open up a dynamic library and call the initialization
       *   function to setup custom logging routines.
       */
#if _WIN32
      /*
       * This is how it is done under Windows
       */
      nds_logging_helper_entry_type
	dll_init_func = (nds_logging_helper_entry_type)
	GetProcAddress( GetModuleHandle(library), 
			NDS_LOGGING_HELPER_ENTRY_STRING );
      if( NULL != dll_init_func )
      {
	dll_init_func( );
      }

#else /* _WIN32 */
      /*
       * This is how the rest of world does it.
       */
      void* dlhandle = (void*)NULL;

      dlhandle = dlopen( library, RTLD_NOW );
      if ( dlhandle )
      {
	void* var = (void*)NULL;

	var = dlsym( dlhandle, NDS_LOGGING_HELPER_ENTRY_STRING );

	if ( var )
	{
	  nds_logging_helper_entry_type func
	    = (nds_logging_helper_entry_type)var;

	  /*
	   * Initialize the NDS Logging Helper library
	   */
	  func();
	}

	dlclose( dlhandle);
	dlhandle = (void*)NULL;
      }
#endif /* _WIN32 */
      
    }

    if ( info )
    {
      char *tok_name;
      char *tok_value;
      tok_name = strsep(&info, ":");
      while ( tok_name )
      {
	int nds_log_group = 0;
	int nds_log_level = -2;

	char *tmp;
	tmp = strsep(&tok_name, "=");
	if ( tmp )
	{
	  tok_value = tok_name;
	}
	tok_name = tmp;
	/*
	 * Determine what debugging information is being requested
	 */
	if ( ( nds_log_group = nds_logging_group_from_string( tok_name ) )
	     == NDS_LOG_BAD_GROUP )
	{
	  tok_name = strsep(&info, ":");
	  continue;
	}
	/*
	 * Determine quantity of output
	 */
	if ( tok_value )
	{
	  nds_log_level = atoi( tok_value );
	  if ( nds_log_level < -2 )
	  {
	    tok_name = strsep(&info, ":");
	    continue;
	  }
	}
	switch ( nds_log_level )
	{
	case -2:
	  nds_logging_enable( nds_log_group );
	  break;
	case -1:
	  nds_logging_disable( nds_log_group );
	  break;
	default:
	  nds_logging_debug_level( nds_log_group,
				   nds_log_level );
	  nds_logging_enable( nds_log_group );
	  break;
	}
	tok_name = strsep(&info, ":");
      }
    }
    /*
     * Initialization is complete
     */
    uninitialized = 0;
  }
  nds_logging_printf("nds_logging_init: Exit");
}

void
nds_logging_printf( const char* MessageFormat, ... )
{
  if ( log_info.s_logging_func )
  {
    char buf[2048];
    va_list	ap;

    va_start( ap, MessageFormat );

#if HAVE_VSNPRINTF_S
    vsnprintf_s
#else /* HAVE_VSNPRINTF_S */
    vsnprintf
#endif /* HAVE_VSNPRINTF_S */
      ( buf, sizeof( buf ),
#if HAVE_VSNPRINTF_S
	sizeof( buf ) - 1,
#endif /* HAVE_VSNPRINTF_S */
	MessageFormat, ap );

    (*log_info.s_logging_func)( buf );

    va_end( ap );
  }
}

void
nds_logging_print_errno( const char* Leader )
{
  char perror_buffer[ 1024 ];

  pstrerror_r( perror_buffer,
	       sizeof( perror_buffer ) );
  if ( Leader )
  {
    nds_logging_printf( "ERRNO: %s: errno: %d - %s",
			Leader,
			errno,
			perror_buffer );
  }
  else
  {
    nds_logging_printf( "ERRNO: errno: %d - %s",
			errno,
			perror_buffer );
  }
}

#if _WIN32
static char*
strsep(char** stringp, const char* delim)
{
  char* start = *stringp;
  char* p;

  p = (start != NULL) ? strpbrk(start, delim) : NULL;

  if (p == NULL)
  {
    *stringp = NULL;
  }
  else
  {
    *p = '\0';
    *stringp = p + 1;
  }

  return start;
}
#endif /* _WIN32 */
