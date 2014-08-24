/*---------------------------------------------------------------------------*/
/* File: FrDiff.c                             Last update:    Nov 28, 2006   */
/*                                                                           */
/* By  Greg Mendell (from LIGOtools)                                         */ 
/* Based on FrDump.c by B. Mours, D. Verkindt                                */
/* and Frame_Compare_Channels.c by Ben Johnson                               */
/*---------------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/* #include <limits.h> */
#include "FrameL.h"

void Help()
{
    printf(" \n"
           " This program looks for differences in the data between two frame files.      \n"
           "                                                                              \n"
           " Usage: FrDiff -i1 <file1> -i2 <file2> [-t <channelName>] [-d <debug_level>]  \n"
           "              [-m <minSampleRate>] [-l]                                       \n"
           "                                                                              \n"
           " The -i1 <file1> -i2 <file2> arguments must be given.                         \n"
           "                                                                              \n"
           " Optional arguments:                                                          \n"
           "                                                                              \n"
           "  -d <debug_level>   -- give this option to get more verbose output.          \n"
           "  -t <channelName>   -- the diff is done on the data for channelName only;    \n"
           "                        otherwise all channels are checked and the channels   \n"
           "                        must appear in the same order in each file.           \n"
           "  -m <minSampleRate> -- check channels with sampleRate >= minSampleRate only. \n"
           "  -l                 -- ignore length difference between frames; compare      \n"
           "                        data for the length of the shorter frame only.        \n"
           "                                                                              \n"
           " Only channel data is checked; metadata in the frames are ignored.            \n"
           " The code exits after the first difference is found.                          \n"
           "                                                                              \n"
           " (Special dump mode: use file1 == file2 and -d -1 to print data to stdout.)   \n"
           "\n");
}

void ErrExit()
{
  fprintf(stderr,"\nFrame files differ!\n\n"); exit(1);
}

enum Type {INPUT1, INPUT2, TAG, DEBUG, MINSAMPLERATE};
#define UNSET -999
#define ADC 0x1
#define SMS 0x2
#define SIM 0x4
#define STAT  0x8
#define EVENT 0x10
#define SUM   0x20
#define RAW   0x40
#define PROC  0x80

void StrCat(char **old, char *more);
int FrDiffVectCmp(FrVect *vect0, FrVect *vect1, char* name1, char* name2, int debugLvl, double minSampleRate, int useMinLength);

/*---------------------------------- Main -----------------------------------*/
int main(int argc, char **argv)
{
    char *fileName1 = NULL;
    char *fileName2 = NULL;    
    char *tag       = NULL;
    int debug = 0;
    int type, i, frameCount;
    double minSampleRate;
    int useMinLength = 0;

    FrFile *file1 = NULL;
    FrameH *frame1 = NULL;
    FrAdcData *adc1 = NULL;
    FrSerData *ser1 = NULL;
    FrProcData *proc1 = NULL;
    FrSimEvent *simE1 = NULL;
    FrEvent *evt1 = NULL;
    FrSimData *sim1 = NULL;
    FrSummary *sum1 = NULL;

    FrFile *file2 = NULL;
    FrameH *frame2 = NULL;
    FrAdcData *adc2 = NULL;
    FrSerData *ser2 = NULL;
    FrProcData *proc2 = NULL;
    FrSimEvent *simE2 = NULL;
    FrEvent *evt2 = NULL;
    FrSimData *sim2 = NULL;
    FrSummary *sum2 = NULL;

    FrVect *vect1 = NULL; 
    FrVect *vect2 = NULL;

    /*----------------- Read input arguments -------------------------*/
    useMinLength = 0;    /* default value */
    minSampleRate = 0.0; /* default value */
    if (argc < 2) {
      Help();
      exit(0);
    } 
    type = UNSET;
    for (i=1; i<argc; i++) {
      if (strcmp(argv[i],"-h") == 0) {
        Help();
        exit(0);
      } else if (strcmp(argv[i],"-i1") == 0) {
        type = INPUT1;
      } else if (strcmp(argv[i],"-i2") == 0) {
        type = INPUT2;
      } else if (strcmp(argv[i],"-t") == 0) {
        type = TAG; /* Channel name */
      } else if (strcmp(argv[i],"-d") == 0) {
        type = DEBUG;
      } else if (strcmp(argv[i],"-m") == 0) {
        type = MINSAMPLERATE;
      } else if (strcmp(argv[i],"-l") == 0) {
        useMinLength = 1;
      } else {
        if (type == INPUT1) {
           StrCat(&(fileName1),argv[i]);
        } else if (type == INPUT2) {
           StrCat(&(fileName2),argv[i]);
        } else if (type == TAG) {
           StrCat(&(tag),argv[i]);
        } else if (type == DEBUG) {
           debug = atoi(argv[i]);
        } else if (type == MINSAMPLERATE) {
           minSampleRate = (double)atof(argv[i]);
        } /* END if (type == INPUT1) else */
        type = UNSET;
      } /* END if (strcmp(argv[i],"-h") == 0) else */
    } /* END for (i=1; i<argc; i++) */

    if (debug > 0) fprintf(stdout,"\nChecking for differences between %s and %s \n\n",fileName1,fileName2);

    /*---------------------------- Open the input file(s) ------------*/

    FrLibSetLvl(0);
    file1 = FrFileINew(fileName1);
    if(file1 == NULL)
    {
        fprintf(stderr,"Cannot open input file %s\n", fileName1);
        exit(1);
    }
    file2 = FrFileINew(fileName2);
    if(file2 == NULL)
    {
        fprintf(stderr,"Cannot open input file %s\n", fileName2);
        exit(1);
    }

    file1->compress = 0;
    file2->compress = 0;

    /*----------------Start the Main loop ----------------------------*/
    frame1 = FrameRead(file1);
    frame2 = FrameRead(file2);
    frameCount = 0;
    vect1 = NULL;
    vect2 = NULL;
    while( (frame1 != NULL) && (frame2 != NULL) )
    {
        frameCount++;
        
        /*--------------- Output frame with some tag ---------------------*/        
        if (tag != NULL) FrameTag(frame1, tag);
        if (tag != NULL) FrameTag(frame2, tag);

        if (debug > 0) {
           fprintf(stdout,"File1 frame #%i %d/%d GTimeS=%d\n",frameCount,frame1->run,frame1->frame,frame1->GTimeS);
           fprintf(stdout,"File2 frame #%i %d/%d GTimeS=%d\n\n",frameCount,frame2->run,frame2->frame,frame2->GTimeS);
        }
        if (debug > 1) {
           fprintf(stdout,"File1 info:\n"); FrFileIStat(file1, stdout);
           fprintf(stdout,"File2 info:\n"); FrFileIStat(file2, stdout);
        }
        if (debug > 4) {
            FrameDump(frame1, stdout, debug);
            FrameDump(frame2, stdout, debug);
        }

        if (tag != NULL)
        {

            adc1 = FrAdcDataReadT(file1,tag,frame1->GTimeS);
            if(adc1 != NULL) {
              vect1 = adc1->data;
              if (debug > 0) fprintf(stdout,"Found %s in %s adc struct.\n",tag,fileName1);
            } else {
              proc1 = FrProcDataReadT(file1,tag,frame1->GTimeS);
              if(proc1 != NULL) {
                 vect1 = proc1->data;
                 if (debug > 0) fprintf(stdout,"Found %s in %s proc struct.\n",tag,fileName1);
              } else {
                 ser1 = FrSerDataReadT(file1,tag,frame1->GTimeS);
                 if(ser1 != NULL) {
                    vect1 = ser1->serial;
                    if (debug > 0) fprintf(stdout,"Found %s in %s ser struct.\n",tag,fileName1);
                 } else {
                    sim1 = FrSimDataReadT(file1,tag,frame1->GTimeS);
                    if(sim1 != NULL) {
                       vect1 = sim1->data;
                       if (debug > 0) fprintf(stdout,"Found %s in %s sim struct.\n",tag,fileName1);
                    } else {
                       fprintf(stderr,"Could not find channel %s in %s\n",tag, fileName1);
                       exit(1);
                    }
                 }
              }
            }
    
            adc2 = FrAdcDataReadT(file2,tag,frame2->GTimeS);
            if(adc2 != NULL) {
              vect2 = adc2->data;
              if (debug > 0) fprintf(stdout,"Found %s in %s adc struct.\n",tag,fileName2);
            } else {
              proc2 = FrProcDataReadT(file2,tag,frame2->GTimeS);
              if(proc2 != NULL) {
                 vect2 = proc2->data;
                 if (debug > 0) fprintf(stdout,"Found %s in %s proc struct.\n",tag,fileName2);
              } else {
                 ser2 = FrSerDataReadT(file2,tag,frame2->GTimeS);
                 if(ser2 != NULL) {
                    vect2 = ser2->serial;
                    if (debug > 0) fprintf(stdout,"Found %s in %s ser struct.\n",tag,fileName2);
                 } else {
                    sim2 = FrSimDataReadT(file2,tag,frame2->GTimeS);
                    if(sim2 != NULL) {
                       vect2 = sim2->data;
                       if (debug > 0) fprintf(stdout,"Found %s in %s sim struct.\n",tag,fileName2);
                    } else {
                       fprintf(stderr,"Could not find channel %s in %s\n",tag, fileName2);
                       exit(1);
                    }
                 }
              }
            }

            FrDiffVectCmp(vect1, vect2, tag, tag, debug, minSampleRate, useMinLength);

            if (adc1  != NULL) { FrAdcDataFree(adc1); adc1 = NULL; }
            if (proc1 != NULL) { FrProcDataFree(proc1); proc1 = NULL; }
            if (ser1  != NULL) { FrSerDataFree(ser1); ser1 = NULL; }
            if (sim1  != NULL) { FrSimDataFree(sim1); sim1 = NULL; }

            if (adc2  != NULL) { FrAdcDataFree(adc2); adc2 = NULL; }
            if (proc2 != NULL) { FrProcDataFree(proc2); proc2 = NULL; }
            if (ser2  != NULL) { FrSerDataFree(ser2); ser2 = NULL; }
            if (sim2  != NULL) { FrSimDataFree(sim2); sim2 = NULL; }

        } 
        else
        {
                adc2 = frame2->rawData->firstAdc;
                for(adc1 = frame1->rawData->firstAdc; adc1 != NULL; adc1=adc1->next)
                {
                    if (debug > 0) fprintf(stdout,"adc1->name = %s\n",adc1->name);
                    if (adc2 != NULL) {
                       if (debug > 0) fprintf(stdout,"adc2->name = %s\n",adc2->name);
                       if ( strcmp(adc1->name,adc2->name) != 0 ) {
                          fprintf(stderr,"The adc channels, %s and %s, have different names.\n",adc1->name,adc2->name);
                          ErrExit();
                       } else {
                          FrDiffVectCmp(adc1->data, adc2->data, adc1->name, adc2->name, debug, minSampleRate, useMinLength);
                       }
                       adc2=adc2->next;
                    } else {
                       /* adc data end occurred in fileName2 before end in fileName1 */
                       fprintf(stderr,"Missing adc channel %s in %s\n",adc1->name, fileName2);
                       ErrExit();
                    }
                }              
                if (adc2 != NULL) {
                       /* adc data end occurred in fileName1 before end in fileName2 */
                       fprintf(stderr,"Missing adc channel %s in %s\n",adc2->name, fileName1);
                       ErrExit();
                }

                proc2 = frame2->procData;
                for(proc1 = frame1->procData; proc1 != NULL; proc1 = proc1->next)
                {
                    if (debug > 0) fprintf(stdout,"proc1->name = %s\n",proc1->name);
                    if (proc2 != NULL) {
                       if (debug > 0) fprintf(stdout,"proc2->name = %s\n",proc2->name);
                       if ( strcmp(proc1->name,proc2->name) != 0 ) {
                          fprintf(stderr,"The proc channels, %s and %s, have different names.\n",proc1->name,proc2->name);
                          ErrExit();
                       } else {
                          FrDiffVectCmp(proc1->data, proc2->data, proc1->name, proc2->name, debug, minSampleRate, useMinLength);
                       }
                       proc2=proc2->next;
                    } else {
                       /* proc data end occurred in fileName2 before end in fileName1 */
                       fprintf(stderr,"Missing proc channel %s in %s\n",proc1->name, fileName2);
                       ErrExit();
                    }
                    
                }
                if (proc2 != NULL) {
                       /* proc data end occurred in fileName1 before end in fileName2 */
                       fprintf(stderr,"Missing proc channel %s in %s\n",proc2->name, fileName1);
                       ErrExit();
                }
                
                ser2 = frame2->rawData->firstSer;
                for(ser1 = frame1->rawData->firstSer; ser1 != NULL; ser1 = ser1->next)
                {
                    if (debug > 0) fprintf(stdout,"ser1->name = %s\n",ser1->name);
                    if (ser2 != NULL) {
                       if (debug > 0) fprintf(stdout,"ser2->name = %s\n",ser2->name);
                       if ( strcmp(ser1->name,ser2->name) != 0 ) {
                          fprintf(stderr,"The ser channels, %s and %s, have different names.\n",ser1->name,ser2->name);
                          ErrExit();
                       } else {
                          FrDiffVectCmp(ser1->serial, ser2->serial, ser1->name, ser2->name, debug, minSampleRate, useMinLength);
                       }
                       ser2=ser2->next;
                    } else {
                       /* ser data end occurred in fileName2 before end in fileName1 */
                       fprintf(stderr,"Missing ser channel %s in %s\n",ser1->name, fileName2);
                       ErrExit();
                    }
                    
                }
                if (ser2 != NULL) {
                       /* ser data end occurred in fileName1 before end in fileName2 */
                       fprintf(stderr,"Missing ser channel %s in %s\n",ser2->name, fileName1);
                       ErrExit();
                }

                sim2 = frame2->simData;
                for(sim1 = frame1->simData; sim1 != NULL; sim1 = sim1->next)
                {
                    if (debug > 0) fprintf(stdout,"sim1->name = %s\n",sim1->name);
                    if (sim2 != NULL) {
                       if (debug > 0) fprintf(stdout,"sim2->name = %s\n",sim2->name);
                       if ( strcmp(sim1->name,sim2->name) != 0 ) {
                          fprintf(stderr,"The sim channels, %s and %s, have different names.\n",sim1->name,sim2->name);
                          ErrExit();
                       } else {
                          FrDiffVectCmp(sim1->data, sim2->data, sim1->name, sim2->name, debug, minSampleRate, useMinLength);
                       }
                       sim2=sim2->next;
                    } else {
                       /* sim data end occurred in fileName2 before end in fileName1 */
                       fprintf(stderr,"Missing sim channel %s in %s\n",sim1->name, fileName2);
                       ErrExit();
                    }
                }
                if (sim2 != NULL) {
                       /* sim data end occurred in fileName1 before end in fileName2 */
                       fprintf(stderr,"Missing sim channel %s in %s\n",sim2->name, fileName1);
                       ErrExit();
                }

                simE2 = frame2->simEvent;
                for(simE1 = frame1->simEvent; simE1 != NULL; simE1 = simE1->next)
                {
                    if (debug > 0) fprintf(stdout,"simE1->name = %s\n",simE1->name);
                    if (simE2 != NULL) {
                       if (debug > 0) fprintf(stdout,"simE2->name = %s\n",simE2->name);
                       if ( strcmp(simE1->name,simE2->name) != 0 ) {
                          fprintf(stderr,"The simE channels, %s and %s, have different names.\n",simE1->name,simE2->name);
                          ErrExit();
                       } else {
                          FrDiffVectCmp(simE1->data, simE2->data, simE1->name, simE2->name, debug, minSampleRate, useMinLength);
                       }
                       simE2=simE2->next;
                    } else {
                       /* simE data end occurred in fileName2 before end in fileName1 */
                       fprintf(stderr,"Missing simE channel %s in %s\n",simE1->name, fileName2);
                       ErrExit();
                    }
                }
                if (simE2 != NULL) {
                       /* simE data end occurred in fileName1 before end in fileName2 */
                       fprintf(stderr,"Missing simE channel %s in %s\n",simE2->name, fileName1);
                       ErrExit();
                }

                evt2 = frame2->event;
                for(evt1 = frame1->event; evt1 != NULL; evt1 = evt1->next)
                {
                    if (debug > 0) fprintf(stdout,"evt1->name = %s\n",evt1->name);
                    if (evt2 != NULL) {
                       if (debug > 0) fprintf(stdout,"evt2->name = %s\n",evt2->name);
                       if ( strcmp(evt1->name,evt2->name) != 0 ) {
                          fprintf(stderr,"The evt channels, %s and %s, have different names.\n",evt1->name,evt2->name);
                          ErrExit();
                       } else {
                          FrDiffVectCmp(evt1->data, evt2->data, evt1->name, evt2->name, debug, minSampleRate, useMinLength);
                       }
                       evt2=evt2->next;
                    } else {
                       /* evt data end occurred in fileName2 before end in fileName1 */
                       fprintf(stderr,"Missing evt channel %s in %s\n",evt1->name, fileName2);
                       ErrExit();
                    }
                }
                if (evt2 != NULL) {
                       /* evt data end occurred in fileName1 before end in fileName2 */
                       fprintf(stderr,"Missing evt channel %s in %s\n",evt2->name, fileName1);
                       ErrExit();
                }

                sum2 = frame2->summaryData;
                for(sum1 = frame1->summaryData; sum1 != NULL; sum1 = sum1->next)
                {
                    if (debug > 0) fprintf(stdout,"sum1->name = %s\n",sum1->name);
                    if (sum2 != NULL) {
                       if (debug > 0) fprintf(stdout,"sum2->name = %s\n",sum2->name);
                       if ( strcmp(sum1->name,sum2->name) != 0 ) {
                          fprintf(stderr,"The sum channels, %s and %s, have different names.\n",sum1->name,sum2->name);
                          ErrExit();
                       } else {
                          /* FrDiffVectCmp(sum1->data, sum2->data, sum1->name, sum2->name, debug, minSampleRate, useMinLength); */
                          /* sum structure has no member named data */
                       }
                       sum2=sum2->next;
                    } else {
                       /* sum data end occurred in fileName2 before end in fileName1 */
                       fprintf(stderr,"Missing sum channel %s in %s\n",sum1->name, fileName2);
                       ErrExit();
                    }
                }
                if (sum2 != NULL) {
                       /* sum data end occurred in fileName1 before end in fileName2 */
                       fprintf(stderr,"Missing sum channel %s in %s\n",sum2->name, fileName1);
                       ErrExit();
                }

        } /* END if (tag != NULL) */

        FrameFree(frame1);
        FrameFree(frame2);

        /*---------------------- read the next frame --------------------*/
        frame1 = FrameRead(file1);
        frame2 = FrameRead(file2);

        /*------------------End the Main loop ----------------------------*/
    }
    if ( (frame1 != NULL) && (frame2 != NULL) ) {
       fprintf(stderr,"Files had different number of frames.");
       ErrExit();    
    }
    
    if (debug > 0) fprintf(stdout,"\nNo differences found!\n\n");
 
    exit(0);
}

/*----------------------------------------------------------------- StrCat --*/
void StrCat(char **old, char *more)
/*---------------------------------------------------------------------------*/
{
    int lenMore;
    char *new;

    lenMore = strlen(more);

    if(*old == NULL)
    {
        *old =  malloc(strlen(more) + 1);
        strcpy(*old, more);
        return;
    }

    new = malloc( strlen(*old) + lenMore + 2);
    sprintf(new,"%s %s",*old, more);
    free(*old);
    *old = new;

    return;
}

/* Based on MyFrVectCmp from Frame_Compare_Channels.c by Ben Johnson */
int FrDiffVectCmp(FrVect *vect0, FrVect *vect1, char* name1, char* name2, int debugLvl, double minSampleRate, int useMinLength)
{
 FRULONG i, nData;
 char   *dC0,*dC1;
 short  *dS0,*dS1;
 int    *dI0,*dI1;
 FRLONG *dL0,*dL1;
 float  *dF0,*dF1;
 double *dD0,*dD1;
 unsigned char   *dU0,*dU1;
 unsigned short  *dUS0,*dUS1;
 unsigned int    *dUI0,*dUI1;
 double mytime = 0;
 FRULONG *dUL0,*dUL1;

 if (vect0 == NULL) {
   fprintf(stderr,"Null pointer to data for channel %s in file1.\n",name1);
   exit(1);
 }
 if (vect1 == NULL) {
   fprintf(stderr,"Null pointer to data for channel %s in file2.\n",name2);
   exit(1);
 }

 if (minSampleRate > 0.0) {
    if (vect0->dx[0] > 1.0/minSampleRate) {
       if (debugLvl > 1) fprintf(stdout,"Ignoring channel %s in file1 with sample rate %0.9lf < %0.9lf .\n",name1,1.0/vect0->dx[0],minSampleRate);
       return 0;
    }
    if (vect1->dx[0] > 1.0/minSampleRate) {
       if (debugLvl > 1) fprintf(stdout,"Ignoring channel %s in file2 with sample rate %0.9lf < %0.9lf .\n",name1,1.0/vect1->dx[0],minSampleRate);
       return 0;
    }    
 }

 if (debugLvl > 2) fprintf(stderr,"Data for channel %s have sample rates in file1 and file2: %0.9lf %0.9lf respectively.\n",name1,1.0/vect0->dx[0],1.0/vect1->dx[0]);

 if (vect0->dx[0] != vect1->dx[0]) {
   fprintf(stderr,"Data for channel %s have different sample rates in file1 and file2: %0.9lf %0.9lf respectively.\n",name1,1.0/vect0->dx[0],1.0/vect1->dx[0]);
   ErrExit(); 
 }

 if (useMinLength > 0) {
    /* only compare data for the length of the shorter frame */
    if (vect1->nData < vect0->nData) {
       nData = vect1->nData;
    } else {
       nData = vect0->nData;
    }
 } else {
   if (vect0->nData != vect1->nData) {
     fprintf(stderr,"Data for channel %s have different lengths in file1 and file2: %li %li respectively.\n",name1,vect0->nData,vect1->nData);
     ErrExit();
   } else {
     nData = vect0->nData;
   }
 }

 if(vect0->GTime != 0) mytime = vect0->GTime;
 
 if (debugLvl > 1) fprintf(stdout,"Checking %i data points for channel %s.\n",nData,name1);
 
 /*-------------------------------------- data part ---------------*/

 if(vect0->compress == 0) {
   if (vect0->type == FR_VECT_4R) {
     dF0 = (float *) vect0->data;
     dF1 = (float *) vect1->data;

     for(i=0; i<nData; i++) {
        if (dF0[i] - dF1[i]) {
           fprintf(stderr,"Data for channel %s differ: \n",name1);
           fprintf(stderr,"for GPS time: %0.9lf file1 value = %g, file2 value = %g .\n",mytime,dF0[i],dF1[i]);
           ErrExit();
        }
        if (debugLvl == -1) fprintf(stdout,"%0.9lf %g\n",mytime,dF0[i]);
        mytime += vect0->dx[0];
     }

   } else if (vect0->type == FR_VECT_8R) {
     dD0 = (double *) vect0->data;
     dD1 = (double *) vect1->data;

     for(i=0; i<nData; i++) {
        if (dD0[i] - dD1[i]) {
           fprintf(stderr,"Data for channel %s differ: \n",name1);
           fprintf(stderr,"for GPS time: %0.9lf file1 value = %g, file2 value = %g .\n",mytime,dD0[i],dD1[i]);
           ErrExit();
        }
        if (debugLvl == -1) fprintf(stdout,"%0.9lf %g\n",mytime,dD0[i]);
        mytime += vect0->dx[0];
     }

   } else if(vect0->type == FR_VECT_C) {
     dC0 = (char   *)vect0->data;
     dC1 = (char   *)vect1->data;
     
     for(i=0; i<nData; i++) {
        if (dC0[i] - dC1[i]) {
           fprintf(stderr,"Data for channel %s differ: \n",name1);
           fprintf(stderr,"for GPS time: %0.9lf file1 value = %c, file2 value = %c .\n",mytime,dC0[i],dC1[i]);
           ErrExit();
        }
        if (debugLvl == -1) fprintf(stdout,"%0.9lf %c\n",mytime,dC0[i]);
        mytime += vect0->dx[0];
     }

   } else if(vect0->type == FR_VECT_2S) {
     dS0 = (short *) vect0->data;
     dS1 = (short *) vect1->data;
     
     for(i=0; i<nData; i++) {
        if (dS0[i] - dS1[i]) {
           fprintf(stderr,"Data for channel %s differ: \n",name1);
           fprintf(stderr,"for GPS time: %0.9lf file1 value = %i, file2 value = %i .\n",mytime,dS0[i],dS1[i]);
           ErrExit();
        }
        if (debugLvl == -1) fprintf(stdout,"%0.9lf %i\n",mytime,dS0[i]);
        mytime += vect0->dx[0];
     }

   } else if(vect0->type == FR_VECT_8S) {
     dL0 = (FRLONG *) vect0->data;
     dL1 = (FRLONG *) vect1->data;

     for(i=0; i<nData; i++) {
        if (dL0[i] - dL1[i]) {
           fprintf(stderr,"Data for channel %s differ: \n",name1);
           fprintf(stderr,"for GPS time: %0.9lf file1 value = %ld, file2 value = %ld .\n",mytime,dL0[i],dL1[i]);
           ErrExit();
        }
        if (debugLvl == -1) fprintf(stdout,"%0.9lf %ld\n",mytime,dL0[i]);
        mytime += vect0->dx[0];
     }

   } else if(vect0->type == FR_VECT_4S) {
     dI0 = (int *) vect0->data;
     dI1 = (int *) vect1->data;

     for(i=0; i<nData; i++) {
        if (dI0[i] - dI1[i]) {
           fprintf(stderr,"Data for channel %s differ: \n",name1);
           fprintf(stderr,"for GPS time: %0.9lf file1 value = %i, file2 value = %i .\n",mytime,dI0[i],dI1[i]);
           ErrExit();
        }
        if (debugLvl == -1) fprintf(stdout,"%0.9lf %i\n",mytime,dI0[i]);
        mytime += vect0->dx[0];
     }

   } else if(vect0->type == FR_VECT_1U) {
     dU0 = (unsigned char *)vect0->data;
     dU1 = (unsigned char *)vect1->data;

     for(i=0; i<nData; i++) {
        if (dU0[i] - dU1[i]) {
           fprintf(stderr,"Data for channel %s differ: \n",name1);
           fprintf(stderr,"for GPS time: %0.9lf file1 value = %c, file2 value = %c .\n",mytime,dU0[i],dU1[i]);
           ErrExit();
        }
        if (debugLvl == -1) fprintf(stdout,"%0.9lf %c\n",mytime,dU0[i]);
        mytime += vect0->dx[0];
     }

   } else if(vect0->type == FR_VECT_2U) {
     dUS0 = (unsigned short *) vect0->data;
     dUS1 = (unsigned short *) vect1->data;

     for(i=0; i<nData; i++) {
        if (dUS0[i] - dUS1[i]) {
           fprintf(stderr,"Data for channel %s differ: \n",name1);
           fprintf(stderr,"for GPS time: %0.9lf file1 value = %i, file2 value = %i .\n",mytime,dUS0[i],dUS1[i]);
           ErrExit();
        }
        if (debugLvl == -1) fprintf(stdout,"%0.9lf %i\n",mytime,dUS0[i]);
        mytime += vect0->dx[0];
     }

   } else if(vect0->type == FR_VECT_8U) {
     dUL0 = (FRULONG *) vect0->data;
     dUL1 = (FRULONG *) vect1->data;

     for(i=0; i<nData; i++) {
        if (dUL0[i] - dUL1[i]) {
           fprintf(stderr,"Data for channel %s differ: \n",name1);
           fprintf(stderr,"for GPS time: %0.9lf file1 value = %ld, file2 value = %ld .\n",mytime,dUL0[i],dUL1[i]);
           ErrExit();
        }
        if (debugLvl == -1) fprintf(stdout,"%0.9lf %ld\n",mytime,dUL0[i]);
        mytime += vect0->dx[0];
     }

   } else if(vect0->type == FR_VECT_4U) {
     dUI0 = (unsigned int *) vect0->data;
     dUI1 = (unsigned int *) vect1->data;

     for(i=0; i<nData; i++) {
        if (dUI0[i] - dUI1[i]) {
           fprintf(stderr,"Data for channel %s differ: \n",name1);
           fprintf(stderr,"for GPS time: %0.9lf file1 value = %i, file2 value = %i .\n",mytime,dUI0[i],dUI1[i]);
           ErrExit();
        }
        if (debugLvl == -1) fprintf(stdout,"%0.9lf %i\n",mytime,dUI0[i]);
        mytime += vect0->dx[0];
     }

   } else if(vect0->type == FR_VECT_8C){
     /*------------- complex float ------*/
     dF0 = (float *) vect0->data;
     dF1 = (float *) vect1->data;

     for(i=0; i<nData; i++) {
        if (dF0[2*i] - dF1[2*i]) {
           fprintf(stderr,"Data for channel %s differ: \n",name1);
           fprintf(stderr,"for GPS time: %0.9lf file1 value = %g, file2 value = %g .\n",mytime,dF0[2*i],dF1[2*i]);
           ErrExit();
        }
        if (dF0[2*i+1] - dF1[2*i+1]) {
           fprintf(stderr,"Data for channel %s differ: \n",name1);
           fprintf(stderr,"for GPS time: %0.9lf file1 value = %g, file2 value = %g .\n",mytime,dF0[2*i+1],dF1[2*i+1]);
           ErrExit();
        }
        if (debugLvl == -1) fprintf(stdout,"%0.9lf %g %g\n",mytime,dF0[2*i],dF0[2*i+1]);
        mytime += vect0->dx[0];
     }

   } else if(vect0->type == FR_VECT_16C) {
     dD0 = (double *) vect0->data;
     dD1 = (double *) vect1->data;

     for(i=0; i<nData; i++) {
        if (dD0[2*i] - dD1[2*i]) {
           fprintf(stderr,"Data for channel %s differ: \n",name1);
           fprintf(stderr,"for GPS time: %0.9lf file1 value = %g, file2 value = %g .\n",mytime,dD0[2*i],dD1[2*i]);
           ErrExit();
        }
        if (dF0[2*i+1] - dF1[2*i+1]) {
           fprintf(stderr,"Data for channel %s differ: \n",name1);
           fprintf(stderr,"for GPS time: %0.9lf file1 value = %g, file2 value = %g .\n",mytime,dD0[2*i+1],dD1[2*i+1]);
           ErrExit();
        }
        if (debugLvl == -1) fprintf(stdout,"%0.9lf %g %g\n",mytime,dD0[2*i],dD0[2*i+1]);
        mytime += vect0->dx[0];
     }

   } else if(vect0->type == FR_VECT_8H)  {
        dF0 = (float *) vect0->data;
        dF1 = (float *) vect1->data;

        for(i=0; i<nData/2; i++)
        {
            if(dF0[i] - dF1[i]) {
              fprintf(stderr,"Data for channel %s differ: \n",name1);
              fprintf(stderr,"for GPS time: %0.9lf file1 value = %g, file2 value = %g .\n",mytime,dF0[i],dF1[i]);
              ErrExit();
            } 
            if(dF0[nData - i - 1] - dF1[nData - i - 1]) {
              fprintf(stderr,"Data for channel %s differ: \n",name1);
              fprintf(stderr,"for GPS time: %0.9lf file1 value = %g, file2 value = %g .\n",mytime,dF0[nData - i - 1],dF1[nData - i - 1]);
              ErrExit();
            }
            if (debugLvl == -1) fprintf(stdout,"%0.9lf %g %g\n",mytime,dF0[i],dF0[nData - i - 1]);
            mytime += vect0->dx[0];
        }

   } else if(vect0->type == FR_VECT_16H) {
      /*----half complex double -------*/
        dD0 = (double *) vect0->data;
        dD1 = (double *) vect1->data;

        for(i=0; i<nData/2; i++)
        {
            if(dD0[i] - dD1[i]) {
              fprintf(stderr,"Data for channel %s differ: \n",name1);
              fprintf(stderr,"for GPS time: %0.9lf file1 value = %g, file2 value = %g .\n",mytime,dD0[i],dD1[i]);
              ErrExit();
            } 
            if(dD0[nData - i - 1] - dD1[nData - i - 1]) {
              fprintf(stderr,"Data for channel %s differ: \n",name1);
              fprintf(stderr,"for GPS time: %0.9lf file1 value = %g, file2 value = %g .\n",mytime,dD0[nData - i - 1],dD1[nData - i - 1]);
              ErrExit();
            }
            if (debugLvl == -1) fprintf(stdout,"%0.9lf %g %g\n",mytime,dD0[i],dD0[nData - i - 1]);
            mytime += vect0->dx[0];
        }

   } else if(vect0->type == FR_VECT_STRING) {
     dC0 = (char   *)vect0->data;
     dC1 = (char   *)vect1->data;
     if ( strcmp(dC0,dC1) != 0) {
        fprintf(stderr,"String data for channel %s differ: \n",name1);
        fprintf(stderr,"for GPS time: %0.9lf file1 value = %s, file2 value = %s .\n",mytime,dC0,dC1);
        ErrExit();
     }
     if (debugLvl == -1) fprintf(stdout,"%0.9lf %s\n",mytime,dC0);
   } else {
      fprintf(stderr,"Unknown type: %d \n",vect0->type );
      ErrExit();
   }
 } else {
    fprintf(stderr,"Data for channel %s is compressed and cannot be checked.\n",name1);
    exit(1);
 } /* END if (vect0->compress == 0) else */

 return 0;
} /* END int FrDiffVectCmp(FrVect *vect0, FrVect *vect1, char* name1, char* name2, int debugLvl) */

