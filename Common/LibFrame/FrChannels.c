/*=============================================================================
FrChannels - Dump the list of channels and sampling rates in a frame file
Written 18 Aug 2003 by Peter Shawhan
Updated 13 Jul 2007 by Peter Shawhan to be able to read multiple frames in file
Example command for compiling:
gcc -static FrChannels.c -I$LIGOTOOLS/include -L$LIGOTOOLS/lib -lFrame -lm -oFrChannels
=============================================================================*/

#include <stdlib.h>
#include <stdio.h>
#include "FrameL.h"
#include <math.h>

/*===========================================================================*/
void PrintUsage( void )
{
   fprintf( stderr, "\nUsage: FrChannels <file> [-a] [-r] [-p] [-s] [-v] [-m]\n\n" );
   fprintf( stderr, "This utility lists the channels in a frame file, one per line, followed by the\n" );
   fprintf( stderr, "sampling rate in Hz.\n\n" );
   fprintf( stderr, "The frame format supports three different types of channels: ADC (also referred\n" );
   fprintf( stderr, "to as raw), processed, and simulated.  By default, the output from this utility\n" );
   fprintf( stderr, "includes all three types; however, you can restrict the output to a particular\n" );
   fprintf( stderr, "type by giving the option -a (or equivalently -r) for raw ADC data, -p for\n" );
   fprintf( stderr, "processed data, or -s for simulated data.  Options can be combined after one\n" );
   fprintf( stderr, "hyphen, e.g. '-ap' to list raw and processed channels.  If none of these\n" );
   fprintf( stderr, "options is given, then all three types will be included in the output.\n\n" );
   fprintf( stderr, "The -v (verbose) option causes the channel type ('Adc', 'Proc' or 'Sim') to be\n" );
   fprintf( stderr, "displayed after the channel name and sampling rate.\n\n" );
   fprintf( stderr, "The -m (multiple frames) option causes the program to report the channels in\n" );
   fprintf( stderr, "each frame in the file.  Otherwise, only the first frame is read to get the\n" );
   fprintf( stderr, "channel list.\n\n" );

   return;
}

/*===========================================================================*/
int main( int argc, char *argv[] )
{
  char *filename=NULL;
  struct FrFile *file;
  FrameH *frame;
  FrAdcData *adc;
  FrProcData *proc;
  FrSimData *sim;
  FrVect *vect;
  double rate;
  int iarg, show_all=1, show_adc=0, show_proc=0, show_sim=0, verbose=0;
  int multi=0, iframe;
  char *arg;
  char *cptr;

  /*-- Check number of command-line arguments --*/

  if ( argc < 2 ) {
    PrintUsage(); return 0;
  }

  /*-- Parse command-line arguments --*/
  for ( iarg=1; iarg<argc; iarg++ ) {
    arg = argv[iarg];
    if ( strlen(arg) == 0 ) { continue; }

    /*-- See whether this is an option flag (or more than one) --*/
    if ( arg[0] == '-' && arg[1] != '\0' ) {
      cptr = arg + 1;

      while ( *cptr != '\0' ) {
	switch ( *cptr ) {
	case 'a':
	case 'r':
	  show_all = 0;
	  show_adc = 1;
	  break;
	case 'p':
	  show_all = 0;
	  show_proc = 1;
	  break;
	case 's':
	  show_all = 0;
	  show_sim = 1;
	  break;
	case 'v':
	  verbose = 1;
	  break;
	case 'm':
	  multi = 1;
	  break;
	default:
	  fprintf( stderr, "*** Invalid option flag -%c\n", *cptr );
	  PrintUsage(); return 1;
	}

	cptr++;
      } 

    } else {
      /*-- Positional argument; should be the filename --*/
      if ( filename ) {
	fprintf( stderr, "*** Extra argument: %s\n", arg );
	PrintUsage(); return 1;
      }
      filename = arg;
    }
  }

  /*-- Make sure the filename was specified --*/
  if ( filename == NULL ) {
    fprintf( stderr, "*** Filename must be specified\n" );
    PrintUsage(); return 1;
  }

  /*-- Open the file --*/
  file = FrFileINew( filename );
  if ( file == NULL ) {
    fprintf( stderr, "*** Cannot open file %s\n %s",
	     filename, FrErrorGetHistory() );
    return 1;
  }

  /*-- Read one or more frames from the file, depending on the 'multi' flag -*/
  iframe = 0;

  while ( iframe==0 || multi ) {

    frame = FrameRead( file );
    if ( frame == NULL ) {
      if ( iframe == 0 ) {
	/*-- It's an error if we're not able to read even one frame --*/
	fprintf( stderr, "*** Error reading frame from file %s\n %s",
		 filename, FrErrorGetHistory() );
	FrFileIEnd( file );
	return 2;
      } else {
	/*-- This is not an error if we've read at least one frame before --*/
	break;
      }
    }

    iframe++;

    if ( multi ) {
      /*-- Report the frame header info --*/
      printf("# Frame at GPS %10d.%09d  (Run %d, Frame number %d)\n",
	     frame->GTimeS, frame->GTimeN, frame->run, frame->frame );
    }

    /*-- Loop over raw ADC channels --*/
    if ( frame->rawData && (show_all || show_adc) ) {
      for ( adc = frame->rawData->firstAdc; adc != NULL; adc = adc->next ) {
	rate = adc->sampleRate;
	if ( rate >= 1.0 ) {
	  printf( "%s %d", adc->name, (int) (rate+0.5) );
	} else {
	  printf( "%s %.7f", adc->name, rate );
	}
	if ( verbose ) {
	  printf( " Adc\n" );
	} else {
	  printf( "\n" );
	}
      }
    }

    /*-- Loop over processed channels --*/
    if ( show_all || show_proc ) {
      for ( proc = frame->procData; proc != NULL; proc = proc->next ) {
	/*-- Have to look into the FrVect to get the sample rate --*/
	rate = 1.0 / proc->data->dx[0];
	if ( rate >= 1.0 ) {
	  printf( "%s %d", proc->name, (int) (rate+0.5) );
	} else {
	  printf( "%s %.7f", proc->name, rate );
	}
	if ( verbose ) {
	  printf( " Proc\n" );
	} else {
	  printf( "\n" );
	}
      }
    }

    /*-- Loop over simulated data records --*/
    if ( show_all || show_sim ) {
      for ( sim = frame->simData; sim != NULL; sim = sim->next ) {
	rate = sim->sampleRate;
	if ( rate >= 1.0 ) {
	  printf( "%s %d", sim->name, (int) (rate+0.5) );
	} else {
	  printf( "%s %.7f", sim->name, rate );
	}
	if ( verbose ) {
	  printf( " Sim\n" );
	} else {
	  printf( "\n" );
	}
      }
    }

  } /*-- End loop over frames in file --*/

  /*-- Close the file --*/
  FrFileIEnd( file );

  return 0;
}
