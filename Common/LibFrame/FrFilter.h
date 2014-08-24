/*---------------------------------------------------------------------------*/
/* File: FrFilter.h                             Last update:   Aug 03, 2004  */
/*                                                                           */
/* Copyright (C) 2004, B. Mours.                                             */
/* For the licensing terms see the LICENSE file.                             */
/* For the list of contributors see the history section of the documentation */
/*---------------------------------------------------------------------------*/
#if !defined(FRFILTER_DEFINED)
#define FRFILTER_DEFINED

#ifdef __cplusplus
extern "C" {
#endif
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

typedef struct FrFilter FrFilter;

struct FrFilter
{
  char*   name;           /* the name of the filter.                         */
  double  fs;             /* the sample rate this filter is designed for.    */
  int     ntaps;          /* the number of filter taps.                      */
  double* a;              /* the numerator coefficients.                     */
  double* b;              /* the denominator coefficients.                   */
  double  gain;           /* the gain to be applied.                         */
  double* reg;            /* internal registers                              */
  double* alpha;          /* internal coefficients                           */
  double* beta;           /* internal coefficients                           */
  int     nCall;          /* number of call for the algorithms               */
  FrVect* output;         /* vector to hold the last data chunk              */
  FrFilter* next;         /* to be able to build linked list                 */
};

void FrFilterFree(FrFilter* filter);
FrFilter* FrFilterNew(char* name,  double fs, double gain, int ntaps, ...);
/* constructor: the additional parameters are ntaps "a" values 
   followed by ntaps "b" values */

void      FrFilterDump(FrFilter *f, FILE *fp,  int debugLvl);
/* this function dump on file fp (like 'stdout') the filters parameters */

FrVect*   FrFilterPackToVect(FrFilter *filter);
/* this function create a vector containing the content of the filter */

FrFilter* FrFilterGetFromVect(FrVect *vect);
/* this function create a filter which was previously packed in a vector */

void      FrProcDataAddFilter(FrProcData *proc, FrFilter *Filter);
/* this function pack a filter into a vector and attach it to the proc data.
   The filter structure is untouched */

FrFilter* FrProcDataGetFilter(FrProcData *proc, char *name);
/* this function create a FrFilter structure according to the parameters
   of the filter called "name" and attached to the proc data */

void        FrStatDataAddFilter(FrStatData *stat, FrFilter *filter);
FrStatData* FrameAddStatFilter(FrameH* frame, char* detectorName,char* statDataName,
	     unsigned int tStart, unsigned int tEnd, unsigned int version, FrFilter *filter);

FrFilter*   FrameGetStatFilter(FrameH *frame, char *detectorName,
			  char *statDataName, char *filterName, int gpsTime);

#ifdef __cplusplus
}
#endif

#endif

