/*---------------------------------------------------------------------------*/
/* File: FrDump.c                             Last update:    Apr 13, 2011   */
/*                                                                           */
/* Copyright (C) 2002, B. Mours, D. Verkindt.                                */
/* For the licensing terms see the LICENSE file.                             */
/* For the list of contributors see the history section of the documentation */
/*---------------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include "FrameL.h"

void Help()
{
 printf(" \n" 
     " This program reads frames from one or more input files, and print   \n"
    "  the requested information.                                          \n"
     "-------------------------------------------------------------------- \n"
     " Syntax is: FrDump  -i <input file>                                  \n"
     "                    -f <first frame: (run # frame #) or (GPS time)>  \n" 
     "                    -l <last  frame: (run # frame #) or (GPS time)>  \n" 
     "                        or length in second                          \n"
     "                    -t <list of tag channels>                        \n" 
     "                    -d <debug level>                                 \n"
     "                    -c -1  to leave the data uncompressed            \n"
     "                    -top <number of ADC in the hit-parade>           \n"
     "                    -h (to get this help)                            \n"
     "  If one of the next option is there, we do only a partial frame dump\n"
     "                    -adc   to dump only the FrAdcData information    \n"
     "                    -sms   to dump only the FrSerData information    \n"
     "                    -proc  to dump only the FrProcData information   \n"
     "                    -sim   to dump only the FrSimData information    \n"
     "                    -sum   to dump only the FrSummary information    \n"
     "                    -stat  to dump only the static information       \n"
     "                    -raw   to dump only the raw data information     \n"
     "                    -event to dump only the FrEvent and FrSimEvent   \n"
     "-------------------------------------------------------------------- \n"
     " Remarks:                                                            \n"
     "  -i : Argument can be one or more file names                        \n"
     "  -t : Tag is a channel list with wild cards like: ADC122 ADC5*      \n"
     "        If a name start by - it is interpreted as an anti-tag        \n"
     " -f -l:These options works only with the debug level > 2             \n"
     " -d 0: One line per file based on the TOC information                \n"
     "       First frame time, file length, time of the first/last event,  \n"
     " -d 1: A few lines per file based on the TOC information             \n"
     "       First/last frame time, number of frame in file, missing frame \n"
     "       Count the number of frame elements like ADC, Proc, Sim, Event \n"
     "       Gives the name of few of them and the typical event amplitude \n"
     " -d 2: More lines per file based on the TOC information              \n"
     "       Same as -d 1 plus the full list of channels, event            \n"
     " -d 3: Read the frame to print information                           \n"
     "       Print the list of all frame (time, length, quality word)      \n"
     " -d 4: Read the frame to print information with more info            \n"
     "       Same as -d 3 plus summary information for all channels        \n"
     "       Print frame history                                           \n"
     " -d 5: Read the frame to print information with more info            \n"
     "        Same as -d 4 plus a full dump of the channel content...      \n"
     "-------------------------------------------------------------------- \n"
     "\n");
}

struct Parameters{
  char *name;                   /* input file name                  */
  char *tag;                    /* output tag                       */
  int frun;                     /* first run to copy                */
  int fframe;                   /* first frame to copy              */
  int lrun;                     /* last run to copy                 */
  int lframe;                   /* last frame to copy               */
  double ftime;                 /* time for the first frame         */
  double ltime;                 /* time for the last frame          */
  int dType;                    /* type of dump                     */
  int debug;                    /* debug level                      */
  int comp;                     /* compression level                */
  int nTop;                     /* number of ADC for the hit parade */
};

enum Type {INPUT, OUTPUT, TAG, FIRST, LAST, DEBUG, COMPRESS, TOP};
#define UNSET -999
#define ADC 0x1
#define SMS 0x2
#define SIM 0x4
#define STAT  0x8
#define EVENT 0x10
#define SUM   0x20
#define RAW   0x40
#define PROC  0x80

void ReadParameters(int argc, char **argv);
void StrCat(char **old, char *more);

struct Parameters Par;

/*--------------------------------------------------------- Main ------------*/
int main(int argc, char **argv)
/*---------------------------------------------------------------------------*/
{FrFile *file;         
 FrameH *frame;
 FrAdcData *adc;
 FrSerData *sms;
 FrProcData *proc;
 FrSimEvent *simE;
 FrEvent *evt;
 FrSimData *sim;
 FrSummary *sum;
 double tStart, tEnd, gtime, start, len;
 
           /*----------------- Read input arguments -------------------------*/

 ReadParameters(argc, argv);

           /*---------------------------- Open the input file(s) ------------*/

 FrLibSetLvl(Par.debug-3);
 file = FrFileINew(Par.name);
 if(file == NULL)
    {fprintf(stderr,"Cannot open input file %s\n %s",
                    Par.name, FrErrorGetHistory()); 
     return(0);}
 file->compress = Par.comp;
 file->chkSumFrFlag = FR_YES;

          /*------------- Table Of Content dump if debugLevel <2 ------------*/

 if(Par.debug < 3)
   {FrFileIDumpT(file, stdout, Par.debug+1, Par.tag, Par.ftime, Par.ltime );
    return(0);}
 Par.debug -= 2;

           /*-------------- set the input file(s) to the first frames -------*/

 if(Par.ftime != 0)
     {frame = FrameReadT(file, Par.ftime);
      if(frame == NULL) /* search for the next frame in the requested range */
        {tStart = FrFileITNextFrame(file, Par.ftime);
         if(Par.ltime > 100000000) tEnd  = Par.ltime;
         else           tEnd = Par.ftime + Par.ltime;
         if(tStart < tEnd) frame = FrameReadT(file, tStart);}}
 else if((Par.frun > 0) || (Par.fframe > 0))
     {frame = FrameReadN(file, Par.frun, Par.fframe);}
 else{frame = FrameRead(file);}
 if(frame == NULL) fprintf(stderr,
      "  Cannot read a frame\n %s", FrErrorGetHistory());

           /*----------------Start the Main loop ----------------------------*/

 while(frame != NULL) {
      
           /*-------------- Check that we are in the limits -----------------*/

    if(frame->GTimeS+1.e-9*frame->GTimeN > Par.ltime) break;
    if(frame->run > Par.lrun) break;
    if((frame->run == Par.lrun ) && (frame->frame > Par.lframe)) break;

           /*--------------- Output frame with some tag ---------------------*/
 
     if(Par.tag  != NULL) FrameTag(frame, Par.tag);
       
          /*---------------- Zoom the vectors if needed ---------------------*/

     if(Par.debug > 2) 
       {gtime = frame->GTimeS+1.e-9*frame->GTimeN;
        if(Par.ftime > gtime || Par.ltime < gtime+frame->dt)
          {if(Par.ftime > gtime) start = Par.ftime - gtime;
           else                  start = 0;
           if(Par.ltime < gtime+frame->dt) len = Par.ltime - gtime - start;
           else                            len = frame->dt - start;
           if(frame->rawData != NULL) 
             {for(adc = frame->rawData->firstAdc; adc != NULL; adc=adc->next)
                {FrVectZoomIn(adc->data, start, len);}}
           for(proc = frame->procData; proc != NULL; proc = proc->next)
                {FrVectZoomIn(proc->data, start, len);}
           for(sim = frame->simData; sim != NULL; sim = sim->next)
                {FrVectZoomIn(sim->data, start, len);}}}

          /*---------------- Dump the data ----------------------------------*/
 
     if(Par.dType == 0)
       {FrameDump(frame, stdout, Par.debug);
        if(Par.comp != 0) FrameCompDump(frame, stdout, Par.debug);}
     else
       {printf("Frame %d/%d GTimeS=%d\n",frame->run,frame->frame,frame->GTimeS);
        if(((Par.dType & ADC) != 0) && (frame->rawData != NULL))
          {for(adc = frame->rawData->firstAdc; adc != NULL; adc=adc->next) 
	    {FrAdcDataDump( adc,  stdout, Par.debug);}}
        if(((Par.dType & SMS) != 0) && (frame->rawData != NULL))
          {for(sms = frame->rawData->firstSer; sms != NULL; sms = sms->next) 
	    {printf(" SMS: %s Data:%30s\n", sms->name, sms->data);}}
        if((Par.dType & STAT) != 0)
          {if(frame->detectProc != NULL)
              {printf("Detector used for reconstruction:\n");
               FrDetectorDump(frame->detectProc, stdout, Par.debug);}
           if(frame->detectSim != NULL)
              {printf("Detector used for simulation:\n");
               FrDetectorDump(frame->detectSim, stdout, Par.debug);}}
        if((Par.dType & SIM) != 0)
          {for(sim = frame->simData; sim != NULL; sim = sim->next) 
              {FrSimDataDump(sim, stdout, Par.debug);}}
        if((Par.dType & EVENT) != 0)
          {for(simE = frame->simEvent; simE != NULL; simE = simE->next)
              {FrSimEventDump(simE, stdout,Par.debug);}
           for(evt = frame->event; evt != NULL; evt = evt->next) 
              {FrEventDump(evt, stdout, Par.debug);}}
        if((Par.dType & PROC) != 0)
           {for(proc = frame->procData; proc != NULL; proc = proc->next) 
              {printf("Reconstructed Data: %s ", proc->name);
               FrVectDump( proc->data,  stdout, Par.debug); 
               FrVectDump( proc->aux,   stdout, Par.debug);}}
        if((Par.dType & SUM) != 0)   
           {for(sum = frame->summaryData; sum != NULL; sum = sum->next)
               {printf("Summary: %s %s \n",sum->name, sum->comment); 
                FrVectDump( sum->moments, stdout, Par.debug);}}
        if((Par.dType & RAW) != 0)
           {FrRawDataDump(frame->rawData, stdout, Par.debug);}}

     if(Par.debug > 1) FrameStat(frame, stdout);
     
     if(Par.nTop != -2) FrameDumpTopADC(frame, stdout, Par.nTop, Par.comp);

     FrameFree(frame);

           /*---------------------- read the next frame --------------------*/

     frame = FrameRead(file);

           /*------------------End the Main loop ----------------------------*/
  }

 FrFileIStat(file, stdout);

 exit(0);
}


/*------------------------------------------------------ SetParameters ------*/
void ReadParameters(int argc, char **argv)
/*---------------------------------------------------------------------------*/
{int type, i;

 if (argc == 1)
    {Help();
     exit(0);}

           /*-------------------- Default values ----------------------------*/

 Par.name   = NULL;
 Par.tag    = NULL;
 Par.debug  = 1;
 Par.comp   = 0;
 Par.ftime  = UNSET;
 Par.ltime  = UNSET;
 Par.frun   = UNSET;
 Par.lrun   = UNSET;
 Par.fframe = UNSET;
 Par.lframe = UNSET;
 Par.nTop   = -2;

           /*------------------- Loop over the parameters -------------------*/

 type = UNSET;

 for (i=1; i<argc; i++)
    {if     (strcmp(argv[i],"-h") == 0) 
       {Help(); 
        exit(0);}
    else if (strcmp(argv[i],"-i") == 0) {type = INPUT;}
    else if (strcmp(argv[i],"-t") == 0) {type = TAG;}
    else if (strcmp(argv[i],"-f") == 0) {type = FIRST;}
    else if (strcmp(argv[i],"-l") == 0) {type = LAST;}
    else if (strcmp(argv[i],"-d") == 0) {type = DEBUG;}
    else if (strcmp(argv[i],"-c") == 0) {type = COMPRESS;}
    else if (strcmp(argv[i],"-top") == 0) {type = TOP;}
    else if (strcmp(argv[i],"-adc")   == 0) {Par.dType = Par.dType | ADC;}
    else if (strcmp(argv[i],"-sms")   == 0) {Par.dType = Par.dType | SMS;}
    else if (strcmp(argv[i],"-stat")  == 0) {Par.dType = Par.dType | STAT;}
    else if (strcmp(argv[i],"-sim")   == 0) {Par.dType = Par.dType | SIM;}
    else if (strcmp(argv[i],"-event") == 0) {Par.dType = Par.dType | EVENT;}
    else if (strcmp(argv[i],"-proc")  == 0) {Par.dType = Par.dType | PROC;}
    else if (strcmp(argv[i],"-sum")   == 0) {Par.dType = Par.dType | SUM;}
    else if (strcmp(argv[i],"-raw")   == 0) {Par.dType = Par.dType | RAW;}
    else
      {if(type == INPUT)   {StrCat(&(Par.name),argv[i]);} 
       if(type == TAG)     {StrCat(&(Par.tag),argv[i]);} 
       if(type == FIRST)   
             {if(Par.ftime == UNSET) 
                   {Par.ftime = atof(argv[i]);}
              else {Par.frun = Par.ftime;
                    Par.fframe = atoi(argv[i]);}}
       if(type == LAST)   
             {if(Par.ltime == UNSET) 
                   {Par.ltime = atof(argv[i]);}
              else {Par.lrun = Par.ltime;
                    Par.lframe = atoi(argv[i]);}}
       if(type == DEBUG)     {Par.debug = atoi(argv[i]);} 
       if(type == COMPRESS)  {Par.comp  = atoi(argv[i]);} 
       if(type == TOP)       {Par.nTop  = atoi(argv[i]);} 
      }}

           /*------------------ Check start/stop parameters------------------*/

 if(Par.fframe != UNSET) Par.ftime = 0.;
 if(Par.lframe != UNSET) Par.ltime = 1.e+12;
 if(Par.frun   == UNSET) Par.frun   = 0;
 if(Par.lrun   == UNSET) Par.lrun   = INT_MAX;
 if(Par.fframe == UNSET) Par.fframe = 0;
 if(Par.lframe == UNSET) Par.lframe = INT_MAX;
 if(Par.ftime  == UNSET) Par.ftime  = 0;
 if(Par.ltime  == UNSET) Par.ltime  = INT_MAX;

 if(Par.name == NULL) 
      {fprintf(stderr," Please provide at least one input file\n");
       Help();
       exit(0);}

           /*-------------------------- Dump parameters ---------------------*/

 if(Par.ltime < Par.ftime) Par.ltime += Par.ftime -1.e-6;

 if (Par.debug>0)
   {printf("-----------Parameters used--------------\n");
    printf("  Input  Files: %s\n", Par.name);
    if(Par.tag != NULL) printf("  Tag          : %s\n",Par.tag);
    printf("  First frame  : %d %d (GPS=%.1f)\n",
                               Par.frun,Par.fframe,Par.ftime);
    if(Par.ltime < 100000000)
         printf("  Lenght       : %.1fs\n", Par.ltime);
    else printf("  Last  frame  : %d %d (GPS=%.1f)\n",
                              Par.lrun,Par.lframe,Par.ltime);
    printf("  Debug level  : %d\n", Par.debug);
    if((Par.dType & ADC)   != 0) printf(" Dump adc info\n");
    if((Par.dType & SMS)   != 0) printf(" Dump sms info\n");
    if((Par.dType & STAT)  != 0) printf(" Dump Stat info\n");
    if((Par.dType & SIM)   != 0) printf(" Dump sim info\n");
    if((Par.dType & EVENT) != 0) printf(" Dump event info\n");
    if((Par.dType & PROC)  != 0) printf(" Dump proc info\n");
    if((Par.dType & SUM)   != 0) printf(" Dump summary info\n");
    if((Par.dType & RAW)   != 0) printf(" Dump raw data info\n");
    if( Par.dType          == 0) printf(" Dump all Frame info\n");
    printf("----------------------------------------\n");}

 return;
}
/*----------------------------------------------------------------- StrCat --*/
void StrCat(char **old, char *more)
/*---------------------------------------------------------------------------*/
{int lenMore;
 char *new;

 lenMore = strlen(more);

 if(*old == NULL)
   {*old =  malloc(strlen(more) + 1);
    strcpy(*old, more); 
    return;}

 new = malloc( strlen(*old) + lenMore + 2);
 sprintf(new,"%s %s",*old, more);
 free(*old);
 *old = new;

 return;}

