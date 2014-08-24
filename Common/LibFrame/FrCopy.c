/*---------------------------------------------------------------------------*/
/* File: FrCopy.c                             Last update:     Apr 12, 2011  */
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
     " This program reads frames from one or more input files, merge them  \n"
    "  if requested and write them  to one or more output file, using or   \n"
     " not data compression.                                               \n"
     "-------------------------------------------------------------------- \n"
     " Syntax is: FrCopy  -i <input file>                                  \n"
     "                    -o <output file>                                 \n"
     "                    -f <first frame: (run # frame #) or (GPS time)>  \n" 
     "                    -l <last  frame: (run # frame #) or (GPS time)>  \n"
     "                       or length to copy if < 100000000              \n" 
     "                    -c <compression type>                            \n"
     "                    -a <list of input adc channels>                  \n" 
     "                    -t <list of tagged channels>                     \n"
     "                    -r <new frame length in second>                  \n" 
     "                    -decimate <number of sample to average>          \n"
     "                    -max <maximum output file length in second>      \n"
     "                    -d <debug level>                                 \n"
     "                    -h (to get this help)                            \n"
     "                    -noTOCts to not write TOC for time series        \n"
     "                    -noChkSum to not compute checksum when writting  \n"
     "                    -s <segment list> (part of the frame in the list)\n"
     "                    -S <segment list> (the full frame in the list)   \n"
     "-------------------------------------------------------------------- \n"
     " Remarks:                                                            \n"
     "  -i : If more than one files is given after the keyword -i they will\n"
     "       be read in sequence. If several input stream are defined      \n"
     "       (several -i followed by name(s)), then the frame content will \n"
     "       be merged;                                                    \n"
     "  -c : Compression types are -1 (same compression as input file),    \n"
     "       0 (no compression), 1 (gzip), 3 (differenciation+gzip),       \n"
     "       5 (zero supress for int only)                                 \n"
     "       6 (gzip for float+zero supress for int). The default is -1.   \n"
     "  -t : Tag is a channel list with wild cards like: ADC122 ADC5*      \n"
     "        If a name start by - it is interpreted as an anti-tag        \n"
     "       The -t option should come after the -o option                 \n"
     "  -a:  When input tag is used, random access are performed to read   \n"
     "       only the requested channels (adc, ser, proc, sim).            \n" 
     "       The Frame header is returned in addition to the channel data. \n"
     "       Additional information like the history is not returned.      \n"
     "       The -a option should come after the -i option.                \n"
     "  -r:  The reshape option works only with one output file.           \n"
     "       It assumes that the length of the input frame is a integer    \n"
     "       number of second. The starting PGS time ouf the output frame  \n"
     "       will be a multiple of the frame length.                       \n"
     "       The requested length should be larger than the input f. length\n"
     "  -decimate: decimation is done on all selected channel by doing     \n"
     "       a simple data averageing.                                     \n"
     "  -noTOCts: By default there a Table Of Content for time series (ADC,\n"
     "        sim, proc data) is written.                                  \n"
     "  -f : If this option is used, the Table Of Content is mandatory and \n"
     "       all frames will be read by increasing time                    \n"
     "  -max:If this option is used the output file is split in several    \n"
     "       file, each of them lasting up to <max> second. The name of    \n"
     "       these files is no more just <output file> but                 \n"
     "       '<output file>-GPS-maxLength.gwf'                             \n"
     "       (like V-R-730123000-100.gwf if <output file> = 'V-R').        \n"
     "  -s:  A segment list is an ASCII text file that contain the list of \n"
     "       start and end GPS time.                                       \n"
     "-------------------------------------------------------------------- \n"
     "\n");
}

struct inputStream{
  char *name;                   /* input file name                  */
  FrFile *file;                 /* Fr file pointer                  */
  FrameH *frame;                /* last frame read                  */
  double gtime;                 /* time for the next frame to read  */
  char *itag;                   /* tag flag used for random access  */
  FrSegList *segListL;          /* segment list (lose)              */
  FrSegList *segListS;          /* segment list (Strict)            */
  struct inputStream *next;     /* next struture in the list        */
};

struct outputStream{
  char *name;                   /* output file name                 */
  char *tag;                    /* output tag                       */
  FrFile *file;                 /* Fr file pointer                  */
  struct outputStream *next;    /* next structure in the list       */
};

struct Parameters{
  struct inputStream *input;    /* input file or directory to copy  */
  struct outputStream *output;  /* output file of the copy          */
  int frun;                     /* first run to copy                */
  int fframe;                   /* first frame to copy              */
  int lrun;                     /* last run to copy                 */
  int lframe;                   /* last frame to copy               */
  double ftime;                 /* time for the first frame         */
  double ltime;                 /* time for the last frame          */
  int ctype;                    /* type of compression              */
  int debug;                    /* debug level                      */
  int fLength;                  /* output frame length (0=undefined)*/
  int decimate;                 /* decimation to apply              */ 
  int split;                    /* maximum file length (if not 0)   */
  FRBOOL noTOCts;               /* noTOCts flag for the output file */
  FRBOOL noChkSum;              /* flag to suppress checksum        */
};

enum Type {INPUT, OUTPUT, TAG, FIRST, LAST, DEBUG, 
           COMPRESS, ADC, RESHAPE, DECIMATE, SPLIT};
#define UNSET -999
void ReadParameters(int argc, char **argv);
void StrCat(char **old, char *more);
void NamesFromStdi(struct inputStream *input);

struct Parameters Par;

/*--------------------------------------------------------- Main ------------*/
int main(int argc, char **argv)
/*---------------------------------------------------------------------------*/
{struct inputStream *input, *inputFirstFrame;
 struct outputStream *output;
 double minTime, gtime;
 FrameH *frame, *newFrame=NULL;
 FrAdcData *adc;
 long nBytes, nFrames;
 int position, nFrame, tEnd = 0;

           /*----------------- Read input arguments -------------------------*/

 ReadParameters(argc, argv);

           /*---------------------------- Open the input file(s) ------------*/

 for(input = Par.input; input != NULL; input=input->next)
   {input->file = FrFileINew(input->name);
    if(input->file == NULL)
       {fprintf(stderr,"Cannot open input file %s\n %s",
                       input->name, FrErrorGetHistory());
       return(0);}

   /*-- do not expand vector to speed up copy if no debug and no decimation--*/

    if(Par.debug < 2 &&  Par.decimate == 1) input->file->compress = -1;}

          /*--compute end time if the length to copy is the input parameter--*/

 if(Par.ltime < 100000000) 
   {if(Par.ftime != 0) Par.ltime += Par.ftime;
    else               Par.ltime += FrFileITStart(Par.input->file);}

           /*----------------- open the output file(s)  ---------------------*/

 for(output = Par.output; output != NULL; output = output->next)
   {if(Par.split == 0)
         output->file = FrFileONewH(output->name,Par.ctype,"FrCopy"); 
    else output->file = FrFileONewM(output->name,Par.ctype,"FrCopy",Par.split);
    if(output->file == NULL)
       {fprintf(stderr,"Cannot open output file %s\n %s",
                      output->name, FrErrorGetHistory());
       return(0);}
    output->file->noTOCts = Par.noTOCts;
    if(Par.noChkSum == FR_YES)          
        {output->file->chkSumFiFlag = FR_NO;
	 output->file->chkSumFrFlag = FR_NO;}}

            /*------- check for segment list and set the input file(s) 
                                           to the first requested frames-----*/

 for(input = Par.input; input != NULL; input=input->next)
   {if(input->segListS != NULL) 
       FrFileIAddSegList(input->file, input->segListS);

    if(Par.ftime            != 0    || 
       input->itag          != NULL || 
       input->file->segList != NULL)
      {input->gtime = FrFileITStart(input->file);
       if(input->gtime < Par.ftime) input->gtime = Par.ftime;}
    else if((Par.frun > 0) || (Par.fframe > 0))
        {input->frame = FrameReadN(input->file, Par.frun, Par.fframe);}}

           /*----------------Start the Main loop ----------------------------*/

 while(1) {
           /*-------- read at least one frame for each input stream ---------*/

    minTime = 1.e+20;
    frame   = NULL;
    inputFirstFrame = NULL;

    for(input= Par.input; input != NULL; input = input->next)
      {if(input->gtime < 0) continue;
       if(input->frame == NULL) 
	 {if(input->itag != NULL) 
             {input->frame = FrameReadTChnl(input->file,
                                            input->gtime, input->itag);
	      input->gtime = FrFileITNextFrame(input->file, input->gtime);
              if((input->frame == NULL) && (input->gtime < Par.ltime)) 
               {input->frame = FrameReadTChnl(input->file,
                                              input->gtime, input->itag);
	        input->gtime = FrFileITNextFrame(input->file, input->gtime);}}
          else if(Par.ftime != 0 || input->file->segList != NULL)
             {input->frame = FrameReadT(input->file, input->gtime);
              input->gtime = FrFileITNextFrame(input->file, input->gtime);
              if((input->frame == NULL) && (input->gtime < Par.ltime)) 
               {input->frame = FrameReadT(input->file, input->gtime);
                input->gtime = FrFileITNextFrame(input->file, input->gtime);}}
          else{input->frame = FrameRead (input->file);}}

           /*----------------- get the first frame --------------------------*/

       if(input->frame != NULL) 
           {gtime = input->frame->GTimeS + 1.e-9*input->frame->GTimeN;
            if(gtime < minTime)
               {minTime = gtime;
	        frame = input->frame;
                inputFirstFrame = input;}}}

    if(frame == NULL) break; /*---- no more frame to read ------------------*/

    inputFirstFrame->frame = NULL;

           /*-------- merge the frame with the rigth time (minimal time)----*/
           /*---Remark: the nanosecond field could be different, 
             FrameMerge copy this information in the relevent adc channel --*/
  
  for(input= Par.input; input != NULL; input = input->next)
      {if(input->frame == NULL) continue; 
       if(input->frame->GTimeS != frame->GTimeS) continue;
       if(input->frame != frame) FrameMerge(frame, input->frame);
       input->frame = NULL;}
      
           /*-------------- Check that we are in the limits -----------------*/

    if(frame->GTimeS+1.e-9*frame->GTimeN >= Par.ltime) break;
    if(frame->run > Par.lrun) break;
    if((frame->run == Par.lrun ) && (frame->frame > Par.lframe)) break;

           /*------------------------- Some debug ---------------------------*/

    if(Par.debug > 0) printf("Copy frame GPS=%d\n", frame->GTimeS);

          /*------------------ Decimate channel if requested ----------------*/

    if(Par.decimate > 1)
      {if(frame->rawData != NULL)
        {for(adc = frame->rawData->firstAdc; adc != NULL; adc = adc->next)
	  {FrAdcDataDecimate(adc, -Par.decimate);}}}

           /*--------------- Output frame with some tag ---------------------*/
 
     for(output=Par.output; output != NULL; output= output->next)
       {if(output->tag     != NULL) FrameTag(frame, output->tag);
         
        if(Par.debug > 1) FrameDump(frame, stdout, Par.debug);

           /*-------------- change frame length if requested ----------------*/

	if(Par.fLength > (frame->dt+1.e-6))
 
	        /*---- case when the new frame was not closed --------------*/
   
           {if((frame->GTimeS >= tEnd) && (newFrame != NULL))
	      {FrameReshapeEnd(newFrame);
               if(FrameWrite(newFrame, output->file) != FR_OK)
                 {fprintf(stderr,"Write error\n%s",FrErrorGetHistory());
                  exit(0);}
               FrameFree(newFrame);
               newFrame = NULL;}

	   position = (frame->GTimeS % Par.fLength)/frame->dt;
	   nFrame = (Par.fLength+1.e-6)/frame->dt;

           if(newFrame == NULL)
	     {newFrame = FrameReshapeNew(frame, nFrame, position);
              tEnd = Par.fLength*(1+frame->GTimeS/Par.fLength);}
           else
             {FrameReshapeAdd(newFrame, frame);}
           frame = NULL;

           if(position == nFrame-1) 
              {FrameReshapeEnd(newFrame);
	       frame = newFrame;
               newFrame = NULL;}}

           /*---------------------------- write frame -----------------------*/

        if(frame != NULL)
           {if(FrameWrite(frame, output->file) != FR_OK)
             {fprintf(stderr,"Write error\n%s",FrErrorGetHistory());
	     exit(0);}}}

      FrameFree(frame);  

           /*------------------End the Main loop ----------------------------*/
  }
	   /*--------------- flush last frame if using reshaping-------------*/
   

 if(newFrame != NULL)
   {FrameReshapeEnd(newFrame);
    if(FrameWrite(newFrame, Par.output->file) != FR_OK)
        {fprintf(stderr,"Write error\n%s",FrErrorGetHistory());
         exit(0);}
    FrameFree(newFrame);}

           /*---------------------- Close files -----------------------------*/

 if(Par.debug > 0) printf("\n--------------File Summary------------\n"); 
 for(input=Par.input; input != NULL; input= input->next)
    {FrFileIEnd(input->file);}

 for(output=Par.output; output != NULL; output= output->next)
    {nFrames = output->file->nFrames;
     nBytes = FrFileOEnd(output->file);
     if(Par.debug > 0) printf(" %ld frames and %ld Bytes written on file %s\n",
                                 nFrames,nBytes,output->name);}

 exit(0);
}


/*------------------------------------------------------ SetParameters ------*/
void ReadParameters(int argc, char **argv)
/*---------------------------------------------------------------------------*/
{struct inputStream *input;
 struct outputStream *output;
 int type, i;

 if (argc == 1)
    {Help();
     exit(0);}

           /*-------------------- Default values ----------------------------*/

 Par.input    = NULL;
 Par.output   = NULL;
 Par.ctype    = -1;
 Par.debug    = 1;
 Par.fLength  = 0;
 Par.decimate = 1;
 Par.ftime    = UNSET;
 Par.ltime    = UNSET;
 Par.frun     = UNSET;
 Par.lrun     = UNSET;
 Par.fframe   = UNSET;
 Par.lframe   = UNSET;
 Par.noTOCts  = FR_NO;
 Par.noChkSum = FR_NO;
 Par.split    = 0;
 input = NULL;

           /*------------------- Loop over the parameters -------------------*/

 type = UNSET;

 for (i=1; i<argc; i++)
    {if     (strcmp(argv[i],"-h") == 0) 
       {Help(); 
        exit(0);}
    else if (strcmp(argv[i],"-i") == 0) 
       {type = INPUT;
        input = (struct inputStream *) calloc(1, sizeof(struct inputStream));
        if(input == NULL) exit(0);
        input->next = Par.input;
        Par.input = input;
        input->gtime = 0.;}
    else if (strcmp(argv[i],"-o") == 0)
       {type = OUTPUT;
        output = (struct outputStream *) calloc(1, sizeof(struct outputStream));
        if(output == NULL) exit(0);
        output->next = Par.output;
        Par.output = output;}
    else if (strcmp(argv[i],"-t") == 0) {type = TAG;}
    else if (strcmp(argv[i],"-a") == 0) {type = ADC;}
    else if (strcmp(argv[i],"-f") == 0) {type = FIRST;}
    else if (strcmp(argv[i],"-l") == 0) {type = LAST;}
    else if (strcmp(argv[i],"-d") == 0) {type = DEBUG;}
    else if (strcmp(argv[i],"-r") == 0) {type = RESHAPE;}
    else if (strcmp(argv[i],"-c") == 0) {type = COMPRESS;}
    else if (strcmp(argv[i],"-max") == 0) {type = SPLIT;}
    else if (strcmp(argv[i],"-decimate") == 0) {type = DECIMATE;}
    else if (strcmp(argv[i],"-noTOCts") == 0)  {Par.noTOCts  = FR_YES;}
    else if (strcmp(argv[i],"-noChkSum") == 0) {Par.noChkSum = FR_YES;}
    else if (strcmp(argv[i],"-S") == 0) 
      {if(input == NULL) 
            {fprintf(stderr,"Define the input file before the segment list");
             exit(0);}
       input->segListS = FrSegListRead(argv[i+1]);
       if(input->segListS == NULL) 
	  {fprintf(stderr," Could not read segment list -s %s\n",argv[i+1]);
	   exit(0);}
       i++;}
    else
      {if(type == INPUT)   {StrCat(&(Par.input->name), argv[i]);} 
       if(type == OUTPUT)  {StrCat(&(Par.output->name),argv[i]);} 
       if(type == ADC)     
          {if(Par.input == NULL) 
              {fprintf(stderr," Please define the input file before the input channels\n");
               exit(0);}
           StrCat(&(Par.input->itag), argv[i]);}
       if(type == TAG)    
          {if(Par.output == NULL) 
              {fprintf(stderr," Please define the output file before the tag\n");
               exit(0);}
           StrCat(&(Par.output->tag),argv[i]);} 
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
       if(type == DECIMATE) {Par.decimate= atoi(argv[i]);} 
       if(type == RESHAPE)  {Par.fLength = atoi(argv[i]);} 
       if(type == DEBUG)    {Par.debug   = atoi(argv[i]);} 
       if(type == COMPRESS) {Par.ctype   = atoi(argv[i]);}
       if(type == SPLIT)    {Par.split   = atoi(argv[i]);}}
    }

           /*------------------ Check start/stop parameters------------------*/

 if(Par.fframe != UNSET) Par.ftime = 0.;
 if(Par.lframe != UNSET) Par.ltime = 1.e+12;
 if(Par.frun   == UNSET) Par.frun   = 0;
 if(Par.lrun   == UNSET) Par.lrun   = INT_MAX;
 if(Par.fframe == UNSET) Par.fframe = 0;
 if(Par.lframe == UNSET) Par.lframe = INT_MAX;
 if(Par.ftime  == UNSET) Par.ftime  = 0;
 if(Par.ltime  == UNSET) Par.ltime  = INT_MAX;

 if(Par.input == NULL) 
      {fprintf(stderr," Please provide at least one input file\n");
       exit(0);}

           /*-------------------------- Dump parameters ---------------------*/

 if (Par.debug>0)
   {printf("-----------Parameters used--------------\n");
    for(input = Par.input; input != NULL; input=input->next)
      {printf("  Input  Stream: %s\n", input->name);
       if(input->itag != NULL)  printf("  Input  tag   : %s\n", input->itag);}
    for(output=Par.output; output != NULL; output= output->next)
      {printf("  Output Stream: %s\n", output->name);
       if(output->tag   != NULL) printf("  Tag          : %s\n",output->tag);}
    printf("  Compression  : %d\n",Par.ctype);
    printf("  First frame  : %d %d (GPS=%.1f)\n",
                               Par.frun,Par.fframe,Par.ftime);
    if(Par.ltime < 100000000) 
         printf("  Lenght       : %.1fs\n", Par.ltime);
    else printf("  Last  frame  : %d %d (GPS=%.1f)\n",
                              Par.lrun,Par.lframe,Par.ltime);
    printf("  Debug level  : %d\n", Par.debug);
    if(Par.decimate == 1)
         printf("  Decimation   : NO decimation\n");
    else printf("  Decimation   : reduce the number of bin by %d\n",Par.decimate);
    if(Par.fLength == 0)
         printf("  Frame length : Unchanged\n");
    else printf("  Frame Length : set to %d(s)\n",Par.fLength);
    if(Par.split == 0)
         printf("  File length  : Unchanged\n");
    else printf("  File length  : maximum is %d secondes\n",Par.split);
    if(Par.noChkSum == FR_YES) 
         printf("  Checksum     : is NOT computed\n");
    else printf("  Checksum     : is computed\n");
    printf("----------------------------------------\n");}
    
          /*----------- Build input file list from stdinput ----------------*/

    for(input = Par.input; input != NULL; input=input->next)
      {if (strcmp(input->name,"STDINLIST") == 0) NamesFromStdi(input);}

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

/*--------------------------------------------------------------- NextFile --*/
void NamesFromStdi(struct inputStream *input )
/*---------------------------------------------------------------------------*/
{
  char *ptr, *filename, temp[256];
  int  size, maxSize, len;

  size = 0;
  maxSize = 1000;
  input->name = malloc(maxSize);

  while(fgets( temp, 256, stdin ) != NULL) {
    temp[strlen(temp)-1] = '\0';

    if ( strncmp(temp, "x ", 2) == 0 ) {
                                   /* This is an information line from "tar" */
       ptr = strchr( temp, ',' );
       if ( ptr == NULL ) return;
       *ptr = '\0';
       filename = temp+2;}
    else {
       filename = temp;}

    if ( strcmp(filename,"ENDOFLIST") == 0 ) return;

    len = strlen(filename);
    if(size+len+2 > maxSize) 
        {maxSize = 2*maxSize;
         input->name = realloc(input->name, maxSize);}

    size += sprintf(input->name+size," %s",filename);}

    return;}
