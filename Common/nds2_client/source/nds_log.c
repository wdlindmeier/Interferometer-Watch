/* -*- mode: c; c-basic-offset: 4; -*- */
#if HAVE_CONFIG_H
#include "daq_config.h"
#endif /*HAVE_CONFIG_H */

#include <stdarg.h>
#include <stdio.h>

#include "nds_log.h"

void
nds_flush( )
{
  fflush( stderr );
}

void
nds_log( const char* Message )
{

    fprintf( stderr, "%s", Message );
}
