/*=============================================================================
FrTrend - Generate trend frame files from raw frame files
Written 9 May 2006 by Peter Shawhan
Example command lines:
gcc -static FrTrend.c -I$LIGOTOOLS/include -L$LIGOTOOLS/lib -lFrame -lm -oFrTrend
./FrTrend -s1 -p My- -d60 -f 826902688 -c "L1:ASC-QPDX_DC L1:ASC-QPDY_DC" L-R-*.gwf |& less
./FrTrend -s1 -p L-T- -d60 -f 826902688 -c "L1:LSC-AS_DC L1:ASC-QPDX_DC L1:ASC-QPDY_DC L1:IFO-SV_STATE_VECTOR L1:ASC-MASTER_OVERFLOW L1:LSC-MASTER_OVERFLOW L1:IOO-MASTER_OVERFLOW L1:SUS-MC2_MASTER_OVERFLOW L1:SUS-RM_MASTER_OVERFLOW" L-R-*.gwf |& less
=============================================================================*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "FrameL.h"

/*------ Structure definitions ------*/

struct trendData {
  char *name;      /* Channel name */
  FrAdcData *inadc;    /* Input ADC data */
  FrAdcData *nADC, *minADC, *maxADC, *meanADC, *rmsADC;
  int n;
  float min;
  float max;
  double sum;
  double sumsq;
  struct trendData *next;
};


/*===========================================================================*/
void PrintUsage( void )
{
   fprintf( stderr, "\n"
"Usage: FrTrend -s <timestep> -p <prefix> -d <outduration> -c <channels> \\\n"
"               -f <firstGPS> [-l <endGPS>] <file> [<file> ...] \n"
"\n"
"<timestep> is the time interval (in seconds) over which trend statistics\n"
"    (min, max, mean, rms, number) are calculated.\n"
"<prefix> is the prefix to use in output filenames, such as 'L-T-'.  Output\n"
"    filenames are created in the current working diretory.\n"
"<outduration> is the length of the output frame files, in seconds.\n"
"<channels> is a space-separated list of channel names (all in a single\n"
"    command-line argument) to read and trend.\n"
"<firstGPS> is the GPS time to start generating output trend frame files.\n"
"<endGPS> is the GPS time at which to stop generating output trend frame files.\n"
"    If omitted, then trend files are generated until the input runs out.\n"
"<file> is normally the name of an input frame file.  Any number\n"
"    of such files can be specified on the command line.\n"
"If either '-' or 'STDINLIST' is specified for the filename, then the program\n"
"    reads a list of filenames (delimited by whitespace) from standard input\n"
"    until it ends or until 'ENDOFLIST' is encountered.  In the list being\n"
"    read, a filename of 'x' is ignored since that is most likely part of an\n"
"    information line from 'tar'.\n"
"\n"
"LIMITATIONS (that could be overcome with a little work):\n"
"* The input files must each contain just one frame, and ffl files can't be used\n"
"* You have to specify the channel list explicitly\n"
"* The min and max trends produced by FrTrend are always floats, whereas the\n"
"    CDS frame builder writes out min and max trends as ints if the raw channel\n"
"    has type int.\n"
	    );
  return;
}

/*===========================================================================*/
int main( int argc, char *argv[] )
{
  int timestep=0;   /* Basic time step for trend time series */
  char *prefix=NULL;
  int outduration=0;
  char *channels=NULL;
  int firstGPS=0, endGPS=1999999999;
  int stepgps;
  char *arg, *argval, *inptr, *cptr;
  int iarg, len;
  char *infiles;
  int infilelen = 0;
  int infilemaxlen=1024;
  char infilename[256], outfilename[256];
  FrFile *ffi = NULL;
  FrFile *ffo;
  FrameH *outframe;
  int stepsperfile;
  struct trendData *firsttd=NULL;
  struct trendData *lasttd=NULL;
  struct trendData *thistd;
  char channel[256];
  int infilegps=0, infileduration=0, outfilegps, readgps;
  int compress=0;      /* Compression method */
  double steprate;
  FrAdcData *chanadcs, *thisadc;
  int firststep, laststep, istep;
  int firstdata, lastdata, idata;
  short *thisS;
  int *thisI;
  float *thisF;
  double *thisD;
  float value;

  /*========== Beginning of code ==========*/

  /*-- Initial memory allocation for list of input files --*/
  infiles = (char *) calloc( 1, infilemaxlen );

  /*-- Check number of command-line arguments --*/
  if ( argc < 2 ) {
    PrintUsage(); return 0;
  }

  /*========== Parse command-line arguments ==========*/

  for ( iarg=1; iarg<argc; iarg++ ) {
    arg = argv[iarg];

    /*-- See whether this is an option flag --*/
    if ( arg[0] == '-' && arg[1] != '\0' ) {
      /*-- Option flag --*/
      cptr = arg + 1;

      if ( strlen(arg) > 2 ) {
	/*-- Value follows the flag with no space --*/
	argval = cptr+1;
      } else if ( iarg < argc-1 ) {
	/*-- Value is the next argument on the command line --*/
        iarg++;
	argval = argv[iarg];
      } else {
	fprintf( stderr, "*** Missing value after %s flag\n", arg );
	PrintUsage(); return 1;
      }
	
      switch ( *cptr ) {
      case 's':
	timestep = (int) strtol( argval, &cptr, 0 );
	if ( *cptr ) {
	  fprintf( stderr, "*** timestep must be an integer\n" );
	  PrintUsage(); return 1;
	}
	break;
      case 'p':
	prefix = argval ; break;
      case 'd':
	outduration = (int) strtol( argval, &cptr, 0 );
	if ( *cptr ) {
	  fprintf( stderr, "*** output file duration must be an integer\n" );
	  PrintUsage(); return 1;
	}
	break;
      case 'c':
	channels = argval ; break;
      case 'f':
	firstGPS = (int) strtol( argval, &cptr, 0 );
	if ( *cptr ) {
	  fprintf( stderr, "*** GPS starting time must be an integer\n" );
	  PrintUsage(); return 1;
	}
	break;
      case 'l':
	endGPS = (int) strtol( argval, &cptr, 0 );
	if ( *cptr ) {
	  fprintf( stderr, "*** GPS ending time must be an integer\n" );
	  PrintUsage(); return 1;
	}
	break;
      default:
	fprintf( stderr, "*** Invalid option flag -%c\n", *cptr );
	PrintUsage(); return 1;
      }

    } else {
      /*-- Positional argument; should be an input file --*/

      if ( strcmp(arg,"STDINLIST") == 0 || strcmp(arg,"-") == 0 ) {
	/*-- Read file names from standard input --*/

	while ( scanf(" %s",infilename) > 0 ) {
	  if ( strcmp(infilename,"ENDOFLIST") == 0 ) break;
	  if ( strcmp(infilename,"x") == 0 ) continue;
	  if ( strlen(infilename) == 0 ) continue;

	  len = strlen(infilename);

	  /*-- See if we need to allocate more memory --*/
	  if ( infilelen+len+3 > infilemaxlen ) {
	    infilemaxlen *= 2;
	    infiles = realloc( infiles, infilemaxlen );
	  }

	  /*-- Append this input filename to the list --*/
	  sprintf( infiles+infilelen, "%s ", infilename );
	  infilelen += (1+len);

	}

      } else {
	/*-- Just an ordinary file name --*/

	len = strlen(arg);

	/*-- See if we need to allocate more memory --*/
	if ( infilelen+len+3 > infilemaxlen ) {
	  infilemaxlen *= 2;
	  infiles = realloc( infiles, infilemaxlen );
	}

	/*-- Append this input filename to the list --*/
	sprintf( infiles+infilelen, "%s ", arg );
	infilelen += (1+len);

      }

    }

  }  /*-- end loop over command-line arguments --*/

  /*========== Sanity checks for arguments ==========*/

  if ( timestep <= 0 ) {
    fprintf( stderr, "*** timestep must be specified\n" );
    PrintUsage(); return 1;
  }

  if ( prefix == NULL ) {
    fprintf( stderr, "*** prefix for output file names must be specified\n" );
    PrintUsage(); return 1;
  }

  if ( outduration <= 0 ) {
    fprintf( stderr, "*** output file duration must be specified\n" );
    PrintUsage(); return 1;
  }

  if ( channels == NULL ) {
    fprintf( stderr, "*** channel list must be specified\n" );
    PrintUsage(); return 1;
  }

  if ( strlen(infiles) == 0 ) {
    fprintf( stderr, "*** One or more input filenames must be specified\n" );
    PrintUsage(); return 1;
  }

  if ( firstGPS <= 0 ) {
    fprintf( stderr, "*** Starting GPS time must be specified\n" );
    PrintUsage(); return 1;
  }

  if ( firstGPS > endGPS ) {
    fprintf( stderr, "*** GPS time interval is unreasonable\n" );
    PrintUsage(); return 1;
  }

  if ( outduration % timestep != 0 ) {
    fprintf( stderr, "*** Output file duration must be a multiple of the time step\n" );
    PrintUsage(); return 1;
  }

  if ( 0 ) {
    printf("time step: %d\n", timestep);
    printf("prefix: %s\n", prefix);
    printf("GPS time range: %d - %d\n", firstGPS, endGPS );
    printf("Input file(s): %s\n", infiles );
  }

  /*========== Allocate trend data structures for channels ==========*/

  steprate = 1.0 / (double)timestep;
  stepsperfile = outduration / timestep;

  cptr = channels;
  while ( sscanf(cptr," %s ",channel) > 0 ) {

    /*-- Allocate memory for the main structure --*/
    thistd = (struct trendData *) calloc( 1, sizeof( struct trendData ) );
    if ( thistd == NULL ) {
      fprintf( stderr, "*** Error allocating memory for channel %s\n",
	       channel );
      return 1;
    }

    /*-- Initialize structure elements --*/
    thistd->name = strdup( channel );
    thistd->inadc = NULL;
    thistd->nADC = NULL;
    thistd->minADC = NULL;
    thistd->maxADC = NULL;
    thistd->meanADC = NULL;
    thistd->rmsADC = NULL;
    thistd->n = 0;
    thistd->next = NULL;

    /*-- Put this at the end of the linked list --*/
    if ( firsttd == NULL ) {
      firsttd = thistd;
      lasttd = thistd;
    } else {
      lasttd->next = thistd;
      lasttd = thistd;
    }

    /*-- Advance cptr to the beginning of the next channel name --*/
    cptr += strlen(channel);
    while ( *cptr && ( *cptr==' ' || *cptr=='\t' || *cptr=='\n' ) ) cptr++;
  }

  /*-- Initialize pointer into list of input files --*/
  inptr = infiles;
  readgps = firstGPS;

  /*========== Loop over output files ==========*/

  for ( outfilegps=firstGPS; outfilegps < endGPS; outfilegps += outduration ) {

    /*-- Create the output frame in memory --*/
    outframe = FrameHNew("LIGO");
    if ( outframe == NULL ) {
      fprintf( stderr, "*** Cannot create output frame\n" );
      return 1;
    }
    outframe->run = 0;
    outframe->frame = 0;
    outframe->dataQuality = 0;
    outframe->GTimeS = outfilegps;
    outframe->GTimeN = 0;
    outframe->ULeapS = FRGPSLEAPS;
    outframe->dt = (double)outduration;

    /*-- Create new ADC structures and initialize all of the trend channels -*/
    for ( thistd = firsttd; thistd; thistd = thistd->next ) {

      sprintf( channel, "%s.n", thistd->name );
      thistd->nADC = FrAdcDataNew( outframe, channel,
				   steprate, stepsperfile,  32 );

      sprintf( channel, "%s.min", thistd->name );
      thistd->minADC = FrAdcDataNew( outframe, channel,
				     steprate, stepsperfile, -32 );

      sprintf( channel, "%s.max", thistd->name );
      thistd->maxADC = FrAdcDataNew( outframe, channel,
				     steprate, stepsperfile, -32 );

      sprintf( channel, "%s.mean", thistd->name );
      thistd->meanADC = FrAdcDataNew( outframe, channel,
				      steprate, stepsperfile, -64 );

      sprintf( channel, "%s.rms", thistd->name );
      thistd->rmsADC = FrAdcDataNew( outframe, channel,
				     steprate, stepsperfile, -64 );

      /*-- Initialize counters for this channel --*/
      thistd->n = 0;

    } /*-- end loop over channels --*/

    /*========= Read input files needed to create this output file ==========*/

    while ( infilegps < outfilegps+outduration ) {

      /*-- If the current file has data we need, use it now --*/
      if ( infilegps+infileduration > outfilegps ) {

	/*-- Loop over channels --*/
	for ( thistd = firsttd; thistd; thistd = thistd->next ) {
	  thisadc = thistd->inadc;
	  switch ( thisadc->data->type ) {
	  case FR_VECT_2S:
	    thisS = thisadc->data->dataS; break;
	  case FR_VECT_4S:
	    thisI = thisadc->data->dataI; break;
	  case FR_VECT_4R:
	    thisF = thisadc->data->dataF; break;
	  case FR_VECT_8R:
	    thisD = thisadc->data->dataD; break;
	  default:
	    fprintf( stderr,
		     "*** Channel %s has type %d that FrTrend does not handle\n",
		     channel, thisadc->data->type );
	    return 1;
	  }
	    

	  /*-- Loop over time steps for which this input file provides data -*/

	  firststep = (infilegps-outfilegps) / timestep ;
	  if ( firststep < 0 ) { firststep = 0; }
	  laststep =
	    (infilegps+infileduration-outfilegps+(timestep-1)) / timestep ;
	  if ( laststep > stepsperfile-1 ) { laststep = stepsperfile-1; }

	  for ( istep=firststep; istep<=laststep; istep++ ) {

	    /*-- Calculate time series index limits for this time step --*/
	    stepgps = outfilegps + istep * timestep;
	    firstdata = (stepgps-infilegps) * thisadc->sampleRate;
	    if ( firstdata < 0 ) { firstdata = 0; }
	    lastdata = (stepgps+timestep-infilegps) * thisadc->sampleRate - 1;
	    if ( lastdata > thisadc->data->nData - 1 ) {
	      lastdata = thisadc->data->nData - 1;
	    }

	    /*-- If firstdata corresponds to the beginning of the time step,
	      then initialize counters using the first data value --*/
	    if ( firstdata == (stepgps-infilegps) * thisadc->sampleRate ) {
	      thistd->n = 1;
	      if ( thisadc->data->type == FR_VECT_8R ) {
		thistd->min = (float) thisD[firstdata];
		thistd->max = (float) thisD[firstdata];
		thistd->sum = thisD[firstdata];
		thistd->sumsq = thisD[firstdata] * thisD[firstdata];
	      } else {
		switch ( thisadc->data->type ) {
		case FR_VECT_2S:
		  value = (float) thisS[firstdata]; break;
		case FR_VECT_4S:
		  value = (float) thisI[firstdata]; break;
		case FR_VECT_4R:
		  value = thisF[firstdata]; break;
		}
		thistd->min = value;
		thistd->max = value;
		thistd->sum = value;
		thistd->sumsq = value*value;
	      }
	      /*-- Get ready to loop over the REST of the data points --*/
	      firstdata += 1;
	    }

	    /*-- Loop over data points and update counters --*/
	    switch ( thisadc->data->type ) {
	    case FR_VECT_2S:
	      for ( idata = firstdata; idata <= lastdata; idata++ ) {
		float fvalue = (float) thisS[idata];
		thistd->n++;
		if ( fvalue < thistd->min ) { thistd->min = fvalue; }
		if ( fvalue > thistd->max ) { thistd->max = fvalue; }
		thistd->sum += fvalue;
		thistd->sumsq += fvalue*fvalue;
	      }
	      break;
	    case FR_VECT_4S:
	      for ( idata = firstdata; idata <= lastdata; idata++ ) {
		float fvalue = (float) thisI[idata];
		thistd->n++;
		if ( fvalue < thistd->min ) { thistd->min = fvalue; }
		if ( fvalue > thistd->max ) { thistd->max = fvalue; }
		thistd->sum += fvalue;
		thistd->sumsq += fvalue*fvalue;
	      }
	      break;
	    case FR_VECT_4R:
	      for ( idata = firstdata; idata <= lastdata; idata++ ) {
		float fvalue = thisF[idata];
		thistd->n++;
		if ( fvalue < thistd->min ) { thistd->min = fvalue; }
		if ( fvalue > thistd->max ) { thistd->max = fvalue; }
		thistd->sum += fvalue;
		thistd->sumsq += fvalue*fvalue;
	      }
	      break;
	    case FR_VECT_8R:
	      for ( idata = firstdata; idata <= lastdata; idata++ ) {
		float fvalue = (float) thisD[idata];
		thistd->n++;
		if ( fvalue < thistd->min ) { thistd->min = fvalue; }
		if ( fvalue > thistd->max ) { thistd->max = fvalue; }
		thistd->sum += fvalue;
		thistd->sumsq += fvalue*fvalue;
	      }
	      break;
	    }

	    /*-- If we have accumulated all the data for this time step, then
	      record the trend statistics in the output ADC structures --*/
	    if ( thistd->n == timestep * thisadc->sampleRate ) {
	      thistd->nADC->data->dataUI[istep] = thistd->n;
	      thistd->minADC->data->dataF[istep] = thistd->min;
	      thistd->maxADC->data->dataF[istep] = thistd->max;
	      thistd->meanADC->data->dataD[istep] = thistd->sum / thistd->n;
	      thistd->rmsADC->data->dataD[istep] =
		      sqrt( thistd->sumsq / thistd->n );
	    }

	  } /*-- End loop over time steps --*/

	} /*-- End loop over channels --*/

      } /*-- End if-block which checks whether this file has data we need --*/

      /*-- If we have not used all the data in this input file, then break
	out of the loop without opening the next input file --*/
      if ( infilegps+infileduration > outfilegps+outduration ) break;

      /*-- Close the current input file (if any), freeing memory --*/
      if ( ffi ) {
	FrFileIEnd( ffi );
	ffi = NULL;
	if ( chanadcs ) { FrAdcDataFree( chanadcs ); chanadcs = NULL;}
	for ( thistd = firsttd; thistd; thistd = thistd->next ) {
	  thistd->inadc = NULL;
	}
      }

      /*-- Open the next input file --*/

      if ( sscanf(inptr," %s ",infilename) > 0 ) {
	/*-- An input file is available to be read --*/

	/*-- Advance inptr to the beginning of the next filename --*/
	inptr += strlen(infilename);
	while (*inptr && (*inptr==' ' || *inptr=='\t' || *inptr=='\n')) inptr++;
	
	ffi = FrFileINew( infilename );
	if ( ffi == NULL ) {
	  fprintf( stderr, "*** Cannot open input file %s\n %s",
		   infilename, FrErrorGetHistory() );
	  return 1;
	}

	/* Get time interval information from the table of contents */
	if ( ffi->toc == NULL ) FrTOCReadFull(ffi);
	if ( ffi->toc == NULL ) {
	  fprintf( stderr, "*** Cannot read table of contents from %s\n",
		   infilename );
	  return 1;
	}
	if ( ffi->toc->nFrame != 1 ) {
	  fprintf(stderr,"*** Input frame file must contain exactly one frame\n");
	  return 1;
	}
	infilegps = ffi->toc->GTimeS[0];
	infileduration = (int) ( ffi->toc->dt[0] + 0.5 );

	/*-- Make sure that the input frame data is contiguous with what we
	  had before --*/
	if ( infilegps > readgps ) {
	  fprintf(stderr,"*** Missing input data between %d and %d\n",
		  readgps, infilegps);
	  return 1;
	}
	readgps = infilegps + infileduration;

	/*-- Get pointers to ADC structs for specified channels all at once -*/
	chanadcs = FrAdcDataReadT( ffi, channels, 0.0 );
	if ( chanadcs == NULL ) {
	  fprintf( stderr, "*** Cannot get channel ADCs from file %s\n %s",
		   infilename, FrErrorGetHistory() );
	  return 1;
	}

	/*-- Associate returned channel ADCs with our trend data structures -*/
	for ( thisadc = chanadcs; thisadc; thisadc = thisadc->next ) {
	  /*-- Match up the channel name --*/
	  for ( thistd = firsttd; thistd; thistd = thistd->next ) {
	    if ( strcmp(thisadc->name,thistd->name) == 0 ) {
	      /*-- This is a match --*/
	      thistd->inadc = thisadc;
	      break;
	    }
	  }
	}

	/*-- Make sure data was read for all channels --*/
	for ( thistd = firsttd; thistd; thistd = thistd->next ) {
	  if ( thistd->inadc == NULL ) {
	    fprintf( stderr, "*** Did not find %s data in file %s\n",
		     thistd->name, infilename );
	    return 1;
	  }
	}

      } else {
	/*-- There are no more input files available to be read --*/
	infilegps = 1999999999;
      }

    } /*-- End loop over input files with data for this output file --*/

    /*========= Write out this output frame to a file ==========*/

    /*-- Construct the output file name --*/
    sprintf( outfilename, "%s%d-%d.gwf", prefix, outfilegps, outduration );

    /*-- Check whether we got all the input data we needed --*/
    if ( readgps >= outfilegps + outduration ) {

      /*-- Open the output frame file --*/
      ffo = FrFileONew( outfilename, compress );
      if ( ffo == NULL ) {
	fprintf( stderr, "*** Cannot open output frame file %s\n",outfilename);
	return 1;
      }
      FrFileOSetMsg( ffo, "Trend frame file created by FrTrend" );

      /*-- Write the output frame to the file --*/
      if( FrameWrite(outframe,ffo) != FR_OK ) {
	fprintf( stderr, "*** Error writing output frame to file %s\n",
		 outfilename );
	return 1;
      }

      /*-- Close output frame file and free output frame --*/
      FrFileOEnd( ffo );
      FrameFree( outframe );

      printf( "Created file %s\n", outfilename );

    } else {

      fprintf( stderr, "*** Not enough input data to create file %s\n",
	       outfilename );
      FrameFree( outframe );
      break;

    }


  } /*-- End loop over output frame files --*/


  /*========== Free memory ==========*/

  /*-- Close the current input file (if any), freeing memory --*/
  if ( ffi ) {
    FrFileIEnd( ffi );
    ffi = NULL;
    if ( chanadcs ) { FrAdcDataFree( chanadcs ); chanadcs = NULL;}
    for ( thistd = firsttd; thistd; thistd = thistd->next ) {
      thistd->inadc = NULL;
    }
  }

  free( infiles );

  while ( firsttd ) {
    thistd = firsttd;
    firsttd = firsttd->next;
    free( thistd );
  }
  
  return 0;
}
