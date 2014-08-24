/*---------------------------------------------------------------------------*/
/* File: FrFilter.c                             Last update:   Aug 03, 2004  */
/*                                                                           */
/* Copyright (C) 2004, B. Mours.                                             */
/* For the licensing terms see the LICENSE file.                             */
/* For the list of contributors see the history section of the documentation */
/*---------------------------------------------------------------------------*/
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "FrameL.h"

#define FRFILTER_XLABEL "FrFilter, coef: a0, a1... b0..., reg..., alpha..., beta..."

/*---------------------------------------------------------------------------*/
void FrFilterFree(FrFilter* filter)
/*---------------------------------------------------------------------------*/
{
 if(filter->next != NULL) FrFilterFree(filter->next);

 free(filter->name);
 free(filter->a);
 free(filter->b);
 free(filter->alpha);
 free(filter->beta);
 free(filter->reg);

 free(filter);

 return;}
/*---------------------------------------------------------------------------*/
FrFilter* FrFilterNew(char* name, 
                      double fs, 
                      double gain, 
                      int ntaps,
                      ...)   /* ntaps "a" values followed by ntaps "b" values*/
/*---------------------------------------------------------------------------*/
{va_list ap;
 FrFilter* f;
 int i;

 f = calloc(1, sizeof(FrFilter));
 if(f == NULL) return(NULL);
 FrStrCpy(&(f->name),name);
 
 f->fs    = fs;
 f->gain  = gain;
 f->ntaps = ntaps;
 f->a     = calloc(ntaps,sizeof(double));
 f->b     = calloc(ntaps,sizeof(double));
 f->alpha = calloc(ntaps,sizeof(double));
 f->beta  = calloc(ntaps,sizeof(double));
 f->reg   = calloc(ntaps,sizeof(double));
 if(f->reg == NULL) return(NULL);

 va_start(ap,ntaps);
 for(i=0; i<ntaps; i++) {f->a[i] = va_arg(ap,double);}
 for(i=0; i<ntaps; i++) {f->a[i] = va_arg(ap,double);}
 va_end(ap);

 return(f);}

/*---------------------------------------------------------------------------*/
void FrFilterDump(FrFilter *f, 
                  FILE *fp, 
                  int debugLvl)
/*---------------------------------------------------------------------------*/
{int i;

 if(f == NULL) return;

 fprintf(fp," Filter: %s frequency=%g gain=%g ntaps=%d\n     a :", 
                f->name, f->fs, f->gain, f->ntaps);
 for(i=0; i<f->ntaps; i++) {fprintf(fp,"\t%g,",f->a[i]);}
 fprintf(fp,"\n     b :");
 for(i=0; i<f->ntaps; i++) {fprintf(fp,"\t%g,",f->b[i]);}
 if(debugLvl < 3) 
   {fprintf(fp,"\n");
    return;}

 fprintf(fp,"\n  reg  :");
 for(i=0; i<f->ntaps; i++) {fprintf(fp,"\t%g,",f->reg[i]);}
 fprintf(fp,"\n  alpha:");
 for(i=0; i<f->ntaps; i++) {fprintf(fp,"\t%g,",f->alpha[i]);}
 fprintf(fp,"\n  beta :");
 for(i=0; i<f->ntaps; i++) {fprintf(fp,"\t%g,",f->beta[i]);}
 fprintf(fp,"\n");

 return;}

/*---------------------------------------------------------------------------*/
FrVect* FrFilterPackToVect(FrFilter *filter)
/*---------------------------------------------------------------------------*/
{FrVect* vect;
 int i;

 if(filter == NULL) return(NULL);

 vect = FrVectNewTS(filter->name, filter->fs, filter->ntaps*5,-64);
 vect->dx[0]     = filter->fs;
 vect->startX[0] = filter->gain;

 for(i=0; i<filter->ntaps; i++)
   {vect->dataD[i]                 = filter->a[i];
    vect->dataD[i+  filter->ntaps] = filter->b[i];
    vect->dataD[i+2*filter->ntaps] = filter->reg[i];
    vect->dataD[i+3*filter->ntaps] = filter->alpha[i];
    vect->dataD[i+4*filter->ntaps] = filter->beta[i];}

 FrVectSetUnitX(vect,FRFILTER_XLABEL);
 FrVectSetUnitY(vect,"v0");

 return(vect);}
/*---------------------------------------------------------------------------*/
FrFilter* FrFilterGetFromVect(FrVect *vect)
/*---------------------------------------------------------------------------*/
{FrFilter* f;
 int i;

 if(vect == NULL) return(NULL);
 if(strcmp(vect->unitX[0],FRFILTER_XLABEL) != 0) return(NULL);

 f = calloc(1, sizeof(FrFilter));
 if(f == NULL) return(NULL);
 FrStrCpy(&(f->name),vect->name);
 
 f->fs    = vect->dx[0];
 f->gain  = vect->startX[0];
 f->ntaps = vect->nData/5;

 f->a     = calloc(f->ntaps,sizeof(double));
 f->b     = calloc(f->ntaps,sizeof(double));
 f->reg   = calloc(f->ntaps,sizeof(double));
 f->alpha = calloc(f->ntaps,sizeof(double));
 f->beta  = calloc(f->ntaps,sizeof(double));
 if(f->beta == NULL) return(NULL);

 for(i=0; i<f->ntaps; i++)
   {f->a[i]     = vect->dataD[i];
    f->b[i]     = vect->dataD[i+  f->ntaps];
    f->reg[i]   = vect->dataD[i+2*f->ntaps];
    f->alpha[i] = vect->dataD[i+3*f->ntaps];
    f->beta[i]  = vect->dataD[i+4*f->ntaps];}

 return(f);}


/*---------------------------------------------------------------------------*/
void FrProcDataAddFilter(FrProcData *proc,  
                         FrFilter *filter)
/*---------------------------------------------------------------------------*/
{FrVect *vect;

 vect = FrFilterPackToVect(filter);
 FrProcDataAttachVect(proc, vect);
 
 return;}
/*---------------------------------------------------------------------------*/
FrFilter* FrProcDataGetFilter(FrProcData *proc,  
                              char *name)
/*---------------------------------------------------------------------------*/
{FrVect *vect;
 FrFilter *filter;

 vect = FrProcDataFindVect(proc, name);
 filter = FrFilterGetFromVect(vect);
 
 return(filter);}

/*---------------------------------------------------------------------------*/
void FrStatDataAddFilter(FrStatData *stat,  
                            FrFilter *filter)
/*---------------------------------------------------------------------------*/
/* Attached the filter information to this static data.                      */ 
/* The filter is untouched and still need to be free by the user.            */
/*---------------------------------------------------------------------------*/
{FrVect *vect;

 vect = FrFilterPackToVect(filter);
 FrStatDataAddVect(stat, vect);
 
 return;}


/*---------------------------------------------------------------------------*/
FrStatData* FrameAddStatFilter(FrameH* frame,
                       char* detectorName,
                       char* statDataName,
		       unsigned int tStart,
		       unsigned int tEnd,
		       unsigned int version,
                       FrFilter *filter)
/*---------------------------------------------------------------------------*/
{FrVect *vect;
 FrStatData *sData;
 
 vect  = FrFilterPackToVect(filter);
 sData = FrameAddStatVector(frame, detectorName, statDataName, 
                              tStart, tEnd, version, vect);

 return(sData);}



/*----------------------------------------------------------------------------*/
FrFilter *FrameGetStatFilter(FrameH *frame,
		          char *detectorName,
			  char *statDataName,
                          char *filterName, 
                          int gpsTime)
/*---------------------------------------------------------------------------*/
{FrFilter *filter;
 FrVect *vect;

 vect = FrameGetStatVect(frame, detectorName, statDataName, filterName, gpsTime);

 filter = FrFilterGetFromVect(vect);

 return(filter);}
