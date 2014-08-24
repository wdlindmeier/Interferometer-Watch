/*---------------------------------------------------------------------------*/
/* File: FrFileChk.c                          Last update:     Apr 11, 2011  */
/*                                                                           */
/* Copyright (C) 2002, B. Mours.                                             */
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
     " This program reads and check all the frames in the file.            \n"
     " It returns a error flag in case of error or zero in case of success.\n"
     " It set the environement variable FRCHECK_NFRAME to the number of    \n"
     " found in the file.                                                  \n"
     "-------------------------------------------------------------------- \n"
     " Syntax is: FrFileCheck  -i <input file>                             \n"
     "         -d <debug level>                                            \n"
     "         -t to scan the file using only the TOC (check structures)   \n"
     "         -s to scan the file using only sequential read(file checksum)\n"
     "         -f GPS time of the first frame to scan (default=0)          \n"
     "         -l GPS time of the last frame to scan (default : 2147483647)\n"
     "         -c to check the data compression (if any)                   \n"
     "         -h to get this help                                         \n"
     "-------------------------------------------------------------------- \n"
     " Remarks:                                                            \n"
     "  By default, FrCheck do first a sequential read to check the file   \n"
     "  checksum, then do a random access read to check the frame checksum.\n" 
     "  -i : Argument could be one or several files (including ffl)        \n"
     "  -f and -t option works only when using the TOC                     \n"
     "-------------------------------------------------------------------- \n"
     "\n");
}

struct Parameters{
  char *name;                   /* input file name                     */
  int debug;                    /* debug level                         */
  int compress;                 /* compression level requested         */
  double ftime;                 /* time for the first frame            */
  double ltime;                 /* time for the last frame             */
  FRBOOL seqRead;               /* if YES, perform a sequential read   */
  FRBOOL ranRead;               /* if YES, perform a random read(TOC)  */
};

void ReadParameters(int argc, char **argv);

struct Parameters Par;

/*-----------------------------------------------------------------scanFile--*/
int scanFile(FrFile *file)
/*---------------------------------------------------------------------------*/
{FrameH *frame = NULL;
 int nFrame = 0, nPrint = 1;
 
 if (Par.debug >0) printf("\n  Sequential file reading. Please wait....\n");
 file->chkSumFiFlag = FR_YES;
 file->chkSumFrFlag = FR_NO;

 while((frame = FrameReadRecycle(file, frame)) != NULL) 
   {nFrame++;
    if(nFrame != nPrint) continue;
    nPrint *= 2;
    if(Par.debug) printf("%9d frames read\n",nFrame);}
 
 if(Par.debug > 0) printf("   %d frame(s) in file.",nFrame);
 if(file->error != FR_OK)
    {fprintf(stderr," Read Error. Last errors are:\n%s\n",FrErrorGetHistory());
     printf("\n");
     exit(4);}
 else {if(Par.debug > 0) printf(" No read error.");}
  
 if(file->chkTypeFiRead == 0)
   {if(Par.debug > 0) printf(" This file does not have a file checksum\n");}
 else
   {if(file->chkSumFiRead == file->chkSumFi)
     {if(Par.debug > 0) printf(" File Checksum OK (%x)\n",file->chkSumFiRead);}
    else
     {if(Par.debug > 0) printf(" File Cheksum error:\n       read = %x\n"
                                                 "   computed = %x\n",
                                file->chkSumFiRead, file->chkSumFi);
      exit(5);}}
 
 return(nFrame);
}

/*-----------------------------------------------------------------seekFile--*/
int seekFile(FrFile *file)
/*---------------------------------------------------------------------------*/
{FrameH *frame = NULL;
 int nFrame = 0, nPrint = 1;
 double gTimeS;

 if (Par.debug >0) printf("\n  Reading using the TOC. Please wait....\n");
 file->chkSumFrFlag = FR_YES;
 file->chkSumFiFlag = FR_NO;

 if(Par.debug > 0)
    printf("   Requested time range: %9.0f - %9.0f\n",Par.ftime, Par.ltime);

 gTimeS = FrFileITStart(file);
 if(gTimeS < 0) 
   {if(Par.debug > 0) printf(" No TOC available\n");
    exit(2);}
 if(Par.debug > 0) printf("        File time range: %9.0f - %9.0f\n",
         gTimeS, FrFileITEnd(file));
 if(gTimeS < Par.ftime) gTimeS = Par.ftime;

 while((frame = FrameReadT(file, gTimeS)) != NULL) 
   {nFrame++;
    if(nFrame == nPrint)
      {nPrint *= 2;
       if(Par.debug) printf("%9d frames read\n",nFrame);}
    gTimeS = FrFileITNextFrame(file, gTimeS);
    if(gTimeS >= Par.ltime || gTimeS < 0.) break;
    FrameFree(frame);}
 
 if(Par.debug > 0) printf("   %d frame(s) in file.",nFrame);
 if(file->error != FR_OK)
      {fprintf(stderr," *** Read Error for GPS: %.0f\n Last errors are:\n%s",
            gTimeS, FrErrorGetHistory());
       exit(3);}
 else {if(Par.debug > 0) printf(" No read error.");}
  
 if(Par.debug > 0) 
   {if(file->chkTypeFrRead == 0)
         {printf(" This file does not have strcture checksums\n");}
    else {
      if(file->fmtVersion < 8) printf(" Frame Checksums OK\n");
      else                     printf(" Structure Checksums OK\n");}} 
 return(nFrame);
}

/*--------------------------------------------------------- Main ------------*/
int main(int argc, char **argv)
/*---------------------------------------------------------------------------*/
{FrFile *file;
 int nFrame = 0;
 char env[128];

           /*----------------- Read input arguments -------------------------*/

 ReadParameters(argc, argv);
 putenv("FRCHECK_NFRAME=0");

           /*---------------------------- Open the input file(s) ------------*/

 FrLibSetLvl(Par.debug-1);
 file = FrFileINew(Par.name);
 if(file == NULL)
    {fprintf(stderr,"Cannot open input file %s\n %s", 
                  Par.name, FrErrorGetHistory());
     exit(1);}
 file->compress = Par.compress;

           /*----------------Start the Main loop ----------------------------*/

 if(Par.seqRead == FR_YES) 
   {nFrame = scanFile(file);
    FrFileIRewind(file);}
 if(Par.ranRead == FR_YES) nFrame = seekFile(file);

 sprintf(env,"FRCHECK_NFRAME=%d",nFrame);
 putenv(env);
 
 exit(0);
}


/*------------------------------------------------------ SetParameters ------*/
void ReadParameters(int argc, char **argv)
/*---------------------------------------------------------------------------*/
{int i;

 if (argc == 1)
    {Help();
     exit(0);}

 Par.name    = NULL;
 Par.debug   = 1; 
 Par.compress= -1;
 Par.ftime   = 0;
 Par.ltime   = 2147483647;
 Par.seqRead = FR_YES;
 Par.ranRead = FR_YES;

           /*------------------- Loop over the parameters -------------------*/

 for (i=1; i<argc; i++)
    {if     (strcmp(argv[i],"-h") == 0) 
       {Help(); 
        exit(0);}
    else if (strcmp(argv[i],"-i") == 0) {FrStrCpy(&(Par.name),argv[i+1]);}
    else if (strcmp(argv[i],"-d") == 0) {Par.debug = atoi(argv[i+1]);}
    else if (strcmp(argv[i],"-t") == 0) {Par.seqRead = FR_NO;}
    else if (strcmp(argv[i],"-s") == 0) {Par.ranRead = FR_NO;}
    else if (strcmp(argv[i],"-f") == 0) {Par.ftime = atof(argv[i+1]);}
    else if (strcmp(argv[i],"-l") == 0) {Par.ltime = atof(argv[i+1]);}
    else if (strcmp(argv[i],"-c") == 0) {Par.compress = 0;}}

           /*-------------------------- Check parameters---------------------*/

 if(Par.name == NULL) 
      {printf(" Please provide at least one input file\n");
       Help();
       exit(0);}

 if (Par.debug >0) 
   {printf(" Checking file %s\n", Par.name);
    if(Par.compress == 0) printf(" Compression will be checked\n");
    else                  printf(" Compression will NOT be checked\n");}

 return;
}
