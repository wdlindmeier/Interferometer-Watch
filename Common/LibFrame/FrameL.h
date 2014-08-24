/*---------------------------------------------------------------------------*/
/* File: FrameL.h                             Last update:     Feb 20, 2010  */
/*                                                                           */
/* Copyright (C) 2002, B. Mours.                                             */
/* For the licensing terms see the LICENSE file.                             */
/* For the list of contributors see the history section of the documentation */
/*---------------------------------------------------------------------------*/
#if !defined(FRAMELIB_DEFINED)
#define FRAMELIB_DEFINED

#define FRAMELIB_VERSION  8.21
#define FR_VERS           8021
#define FR_MINOR_VERSION    21
#define FRPI              3.14159265358979
#define FRTWOPI           6.2831853071795864769




#ifdef __cplusplus
extern "C" {
#endif

#include <time.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include "FrIO.h"

#ifdef MEMWATCH
#include <memwatch.h>
#endif

#if CHAR_MAX == 127
#define FRSCHAR char
#else
#define FRSCHAR signed char     
#endif

typedef enum  {FR_NO, FR_YES} FRBOOL;  /*this set FR_NO=0, FR_YES=1 */

typedef  enum {FRNATIVE,
               FRSWAP,
               FRCONTRACT,
               FREXPAND,
               FRCONTRACT_AND_SWAP,
               FREXPAND_AND_SWAP} FRFORMAT;

typedef  enum {FRTSADC,
               FRTSSIM,
               FRTSPROC} FRTSTYPE;

typedef enum {FR_OK,
              FR_ERROR_NO_FRAME,
              FR_ERROR_NO_FILE,
              FR_ERROR_MALLOC_FAILED,
              FR_ERROR_BUFF_OVERFLOW,
              FR_ERROR_WRITE_ERROR,
              FR_ERROR_READ_ERROR,
              FR_ERROR_FRSET,
              FR_ERROR_WRONG_CHAR_LENGTH,
              FR_ERROR_BAD_END_OF_FRAME,
              FR_ERROR_OPEN_ERROR,
              FR_ERROR_CHECKSUM,
	      FR_ERROR_BAD_NEVENT,
	      FR_ERROR_BAD_NSTAT,
              FR_ERROR_TOO_LONG_STRING} FRERROR;

#define FRMAPMAXSH 100
#define FRMAPMAXREF 4
#define FRMAPMAXINSTANCE 500
/*--------------------------- time define ----------------------------------*/
#define FRGPSOFF 315964800
#define FRGPSDELTA  16
#define FRGPSTAI    19  /* in case of change, update the function FrGPS2UTC */
#define FRGPSLEAPS (FRGPSTAI+FRGPSDELTA)
/* Just some rules about time .......                                       */
/* the GPS time origin is Sun Jan  6 00:00:00 1980                          */
/* the C function time origin is Jan 1 00:00:00 1970 (no leap seconds)      */
/*       (C time - GPS time = 3600*24*3567 = 315964800 seconds)             */
/*  TAI is ahead of GPS by 19 seconds                                       */
/*  TAI is ahead of UTC by 32 seconds (July 2001)                           */
 /*---- GPS time = UTC - 3600*24*3657 + delta ------------------------------*/
/*from 1997 01 Jul, UTC:   delta = +12s                                     */
/*from 1999 01 Jan, UTC:   delta = +13s                                     */
/*from 2006 01 Jan, UTC:   delta = +14s                                     */
/*from 2009 01 Jan, UTC:   delta = +15s                                     */
/*from 2012 01 Jul, UTC:   delta = +16s                                     */
/*See http://tycho.usno.navy.mil/time.html or http://hpiers.obspm.fr/eop-pc */
/* See also the function FrGetLeapS                                         */
/*--------------------------------------------------------------------------*/

struct FrameH;
struct FrAdcData;
struct FrBasic;
struct FrDetector;
struct FrEndOfFrame;
struct FrEndOfFile;
struct FrEvent;
struct FrMsg;
struct FrFile;
struct FrFileH;
struct FrHistory;
struct FrObject;
struct FrRawData;
struct FrProcData;
struct FrSE;
struct FrSH;
struct FrSimData;
struct FrSegList;
struct FrSerData;
struct FrStatData;
struct FrSummary;
struct FrSimEvent;
struct FrTable;
struct FrTag;
struct FrTOC;
struct FrTOCdet;
struct FrTOCevt;
struct FrTOCstat;
struct FrTOCts;
struct FrTOCtsH;
struct FrVect;

typedef struct FrAdcData    FrAdcData;
typedef struct FrameH       FrameH;
typedef struct FrBasic      FrBasic;
typedef struct FrCList      FrCList;
typedef struct FrDetector   FrDetector;
typedef struct FrEndOfFrame FrEndOfFrame;
typedef struct FrEndOfFile  FrEndOfFile;
typedef struct FrEvent      FrEvent;
typedef struct FrMsg        FrMsg;
typedef struct FrFile       FrFile;
typedef struct FrFileH      FrFileH;
typedef struct FrHistory    FrHistory;
typedef struct FrProcData   FrProcData;
typedef struct FrRawData    FrRawData;
typedef struct FrSegList    FrSegList;
typedef struct FrSerData    FrSerData;
typedef struct FrSE         FrSE;
typedef struct FrSH         FrSH;
typedef struct FrSimData    FrSimData;
typedef struct FrSimEvent   FrSimEvent;
typedef struct FrStatData   FrStatData;
typedef struct FrSummary    FrSummary;
typedef struct FrTable      FrTable;
typedef struct FrTag        FrTag;
typedef struct FrTOC        FrTOC;  
typedef struct FrTOCdet     FrTOCdet;
typedef struct FrTOCevt     FrTOCevt;
typedef struct FrTOCstat   FrTOCstat;
typedef struct FrTOCts      FrTOCts;
typedef struct FrTOCtsH     FrTOCtsH;
typedef struct FrVect       FrVect;


/*---------------------------------------------------------------------------*/
struct FrameH {              /* Frame header structure                       */
  FrSH *classe;              /* class information (internal structure)       */
  char *name;                /* frame name (experiment name)                 */
  int   run;                 /* run number                                   */
  unsigned int frame;        /* frame number                                 */
  unsigned int dataQuality;  /* data quality word                            */
  unsigned int GTimeS;       /* frame starting Time(GPS:s.since 6/1/80)      */
  unsigned int GTimeN;       /* frame starting Time(nsec modulo 1 sec)       */
  unsigned short ULeapS;     /* leap seconds between GPS and UTC             */
  double dt;                 /* frame length (sec)                           */
  FrVect *type;              /* Array used to store general info like trigger*/
                             /* type. Their meaning will  be define later.   */
  FrVect *user;              /*    Array used to store user data             */
  FrDetector *detectSim;     /* detector used for simulation                 */
  FrDetector *detectProc;    /* detector used for reconstruction             */
                             /* (reconstruction = off line processing)       */
  FrHistory  *history;       /* pointer to the history bank                  */
  FrRawData  *rawData;       /* pointer to the raw data structure            */
  FrProcData *procData;      /* pointer to the reconstructed data            */
  FrSimData  *simData;       /* pointer to the simulated data buffers        */
  FrEvent    *event;         /* pointer to the first event structure         */
  FrSimEvent *simEvent;      /* pointer to the simulated events              */
  FrSummary  *summaryData;   /* pointer to statistical summary data          */
  FrVect     *auxData;       /* pointer to the auxiliary data                */
  FrTable    *auxTable;      /* pointer to the auxiliary table               */
                             /* ------ end_of_SIO parameters ----------------*/
  FrProcData *procDataOld;   /* pointer to the proc data        (used by tag)*/
  FrSimData  *simDataOld;    /* pointer to the simulated data   (used by tag)*/
  FrEvent    *eventOld;      /* pointer to the event data       (used by tag)*/
  FrSimEvent *simEventOld;   /* pointer to the sim event        (used by tag)*/
  FrSummary  *summaryDataOld;/* pointer to statistical summary  (used by tag)*/
};

void    FrameAddAdc     (FrameH *frame, FrAdcData* adc);
void    FrameAddEvent   (FrameH *frame, FrEvent* event);
void    FrameAddProc    (FrameH *frame, FrProcData* proc);
void    FrameAddSer     (FrameH *frame, FrSerData* ser);
void    FrameAddSimData (FrameH *frame, FrSimData* sim);
void    FrameAddSimEvent(FrameH *frame, FrSimEvent *event);
int     FrameAddStatData(FrameH* frame, char* detectorName, FrStatData *stat);
FrStatData* FrameAddStatVector(FrameH* frame, char* detectorName, char* statDataName,
	     unsigned int tStart, unsigned int tEnd, unsigned int version, FrVect* vect);
void    FrameAddSum     (FrameH *frame, FrSummary* sum);

void    FrameCompress   (FrameH *frame, int compress, int gzipLevel);
void    FrameCompDump   (FrameH *frame, FILE *fp, int debugLvl);
FrameH *FrameCopy       (FrameH *frame);
FrameH* FrameCopyPart(FrameH* frameIn, double start, double duration);
void    FrameDump       (FrameH *frame, FILE *fp, int debugLvl);
char   *FrameDumpToBuf  (FrameH *frame, int dbgLvl, char *buf, FRLONG bufSize);
void    FrameDumpTopADC (FrameH *frame, FILE *fp, int nTop, int comp);
void    FrameExpand     (FrameH *frame);
FrBasic*    FrameFindBasic  (FrBasic*root, char *name);
FrDetector* FrameFindDetector(FrameH *frame,char *detNameOrPrefix);
FrVect *FrameFindVect   (FrameH *frame, char *vectName);
FrVect *FrameFindAdcVect(FrameH *frame, char  *adcName);
FrVect *FrameFindProcVect(FrameH *frame,char *procName);
FrVect *FrameFindSimVect(FrameH *frame, char  *simName);
FrVect *FrameFindStatVect(FrameH *frame,char *statName);
FrVect *FrameFindSumVect(FrameH *frame, char  *sumName);
void    FrameFree       (FrameH *frame);
FRLONG  FrameGetAdcSize (FrameH *frame);
int     FrameGetLTOffset(FrameH *frame, char *channelName);
FrStatData *FrameFindStatData(FrameH *frame, char *detectorName,
			  char *statDataName, int gpsTime);
FrVect* FrameGetStatVect(FrameH *frame, char *detectorName, char *statDataName,
                          char *vectorName,  int gpsTime);
void    FrameMerge      (FrameH *frame1, FrameH *frame2);
double  FrameLatency (FrameH *frame);
FrameH *FrameNew   (char *name);
FrameH *FrameRead       (FrFile *iFile);
FrameH *FrameReadN      (FrFile *iFile, int rNumber, int fNumber);
FrameH *FrameReadRecycle(FrFile *iFile, FrameH *frame);
FrameH *FrameReadT      (FrFile *iFile, double gtime);
FrameH *FrameReadTAdc   (FrFile *iFile, double gtime, char *tag);
FrameH *FrameReadTChnl  (FrFile *iFile, double gtime, char *tag);
FrameH *FrameReadTProc  (FrFile *iFile, double gtime, char *tag);
FrameH *FrameReadTSer   (FrFile *iFile, double gtime, char *tag);
FrameH *FrameReadTSim   (FrFile *iFile, double gtime, char *tag);
FrameH *FrameReadFromBuf(char *buf, FRLONG nBytes, int comp);
void    FrameRemoveUntagged(FrameH *frame);
void    FrameRemoveUntaggedData(FrameH *frame);
void    FrameRemoveUntaggedAdc(FrameH *frame);
void    FrameRemoveUntaggedEvent(FrameH *frame);
void    FrameRemoveUntaggedProc(FrameH *frame);
void    FrameRemoveUntaggedSer(FrameH *frame);
void    FrameRemoveUntaggedSim(FrameH *frame);
void    FrameRemoveUntaggedSimEvt(FrameH *frame);
void    FrameRemoveUntaggedStat(FrameH *frame);
void    FrameRemoveUntaggedSum(FrameH *frame);
int     FrameRemoveDuplicatedADC(FrameH *frame);
FrameH* FrameReshape(FrameH* frame, FrameH** workSpace, int newFrameLength);
int     FrameReshapeAdd(FrameH *frame1, FrameH *frame2);
FrameH *FrameReshapeConvertSer(FrameH *trend, FrameH *frame, int trendPeriod);
int     FrameReshapeEnd(FrameH *frame);
FrameH *FrameReshapeNew(FrameH *frame, int nFrame, int position);
void    FrameSetPrefix(FrameH *frame, char *prefix);
int     FrameStat    (FrameH *frame, FILE *fp);
void    FrameTag     (FrameH *frame, char *tag);
void    FrameTagAdc  (FrameH *frame, char *tag);
void    FrameTagBasic(FrBasic**root, FrBasic**rootOld,char *tag);
void    FrameTagEvent(FrameH *frame, char *tag);
void    FrameTagMsg  (FrameH *frame, char *tag);
void    FrameTagProc (FrameH *frame, char *tag);
void    FrameTagSer  (FrameH *frame, char *tag);
void    FrameTagSim  (FrameH *frame, char *tag);
void    FrameTagSimEvt(FrameH *frame, char *tag);
void    FrameTagStat (FrameH *frame, char *tag);
void    FrameTagSum  (FrameH *frame, char *tag);
void    FrameUntag   (FrameH *frame);
void    FrameUntagAdc(FrameH *frame);
void    FrameUntagBasic(FrBasic **root, FrBasic **rootOld);
void    FrameUntagEvent(FrameH *frame);
void    FrameUntagMsg  (FrameH *frame);
void    FrameUntagProc (FrameH *frame);
void    FrameUntagSer  (FrameH *frame);
void    FrameUntagSim  (FrameH *frame);
void    FrameUntagSimEvt(FrameH *frame);
void    FrameUntagStat (FrameH *frame);
void    FrameUntagSum  (FrameH *frame);
int     FrameWrite     (FrameH *frame, FrFile *oFile);
FRLONG  FrameWriteToBuf(FrameH *frame, int comp, char *buf, FRLONG nBytes);
/*---------------------------------------------------------------------------*/
FrameH *FrameHCopy(FrameH *in);
FrSH   *FrameHDef(void);    
FrameH *FrameHRead (FrFile *iFile);
FrameH *FrameHReadN(FrFile *iFile,int rNumber, int fNumber);
FrameH *FrameHReadT(FrFile *iFile,double gtime);
FrameH *FrameHNew(char *name);
void    FrameHWrite(FrameH *frameH, FrFile *oFile);
/*---------------------------------------------------------------------------*/
struct FrAdcData {               /* hold ADC data (or image)                 */
  FrSH *classe;                  /* class information (internal structure)   */
  char   *name;                  /* signal name                              */
  FrVect *data;                  /* array of data                            */
  FrVect *aux;                   /* array of auxiliary data if needed.       */
  FrAdcData *next;               /* next adc structure                       */
  FrAdcData *nextOld;            /* used by the tag/untag functions          */
                                 /*------- enf of the FrBasic structure------*/
  char   *comment;               /* signal description;  Adc units are 
                                      stored in adc->data->unit[0]           */
  unsigned int channelGroup;     /* crate number                             */
  unsigned int channelNumber;    /* channel number within a crate            */
  unsigned int nBits;            /* number of bits                           */
  float bias;                    /* DC bias on channel.  the step 
                                     size is stored in adc->data->dx;        */
  float slope;                   /* ADC calibration: input units/count       */
  char *units;                   /* ADC calibration: input units for slope   */
  double sampleRate;             /* sampling frequency (unit = Hz)           */
  double timeOffset;             /* offset from the frame starting time (sec)*/
  double fShift;                 /* frequency shift if the signal was 
                                         heterodyne (Hz) (0->no shift)       */
  float  phase;                  /* phase of heterodyning signal.            */
  unsigned short dataValid;      /* flag used to specify if the ADC input was 
                                   out of range (0=ok) during this frame     */
}; 

FrAdcData *FrAdcDataCopy(FrAdcData *adc, FrameH *frame);
int        FrAdcDataDecimate(FrAdcData *adc, int nGroup);
FrSH      *FrAdcDataDef(void);
void       FrAdcDataDump(FrAdcData *adc, FILE *fp, int debugLvl);
FrAdcData *FrAdcDataFind (FrameH *frame, char *adcName); 
void       FrAdcDataFree(FrAdcData *adc);
FrAdcData* FrAdcDataFreeOne(FrAdcData *adcData);
FRLONG     FrAdcDataGetSize(FrAdcData *adc);
FrAdcData *FrAdcDataNew (FrameH *frame, char *name, 
                         double sampleRate, FRLONG nData, int nBits);
FrAdcData *FrAdcDataNewF(FrameH *frame, char *name,char *comment,
                 unsigned int channelGroup, unsigned int channelNumber, 
                 int nBits, float bias, float slope, char *units,
		 double sampleRate, FRLONG nData);
FrAdcData *FrAdcDataRead (FrFile *iFile);
FrAdcData *FrAdcDataReadT(FrFile *iFile, char *name, double gtime);
void       FrAdcDataSetAux      (FrAdcData *adc, FrVect *aux);
void       FrAdcDataSetDataValid(FrAdcData *adc, unsigned short dataValid);
void       FrAdcDataSetFShift   (FrAdcData *adc, double fShift, float phase);
void       FrAdcDataSetTOffset  (FrAdcData *adc, double timeOffset);
void       FrAdcDataWrite       (FrAdcData *adc, FrFile *oFile);

/*---------------------------------------------------------------------------*/
struct FrCList {       /* channel list                                       */
  int nChannels;       /* number of channel in the list                      */
  int nameSize;        /* space reserved for the name                        */
  int rowSize;         /* size (bytes) of one row = nameSize + sizeof(void *)*/
  void *table;         /* Array of size nChannels * rowSize                  */
};

FrCList *FrCListBldAdc(FrameH *frame);
FrCList *FrCListBldChannel(FrameH *frame);
FrCList *FrCListBldSer(FrameH *frame);
void    *FrCListFind(FrCList *list, char *name);
int      FrCListFindDuplicate(FrCList *list, char *duplNames, int duplNSize);
void     FrCListFree(FrCList *list);
void*    FrCListGetElement(FrCList *list, int index);
/*---------------------------------------------------------------------------*/
struct FrDetector { 
  FrSH   *classe;           /* class information (internal structure)        */
  char   *name;             /* detector name                                 */
  char   prefix[2];         /* channel prefix for this detector
				              "**" means prefix are not used */
  double longitude;         /* longitude (east of greenwich) in radians      */
  double latitude;          /* latitude (north of equator) in radians        */
  float  elevation;         /* detector altitude (meter)                     */
  float  armXazimuth;       /* orientation of X arm in radians CW from North */
  float  armYazimuth;       /* orientation of Y arm in radians CW from North */
                            /* Azimuth values should be in the range 0 to 2pi*/
  float  armXaltitude;      /* altitude (pitch) of the X arm                 */
  float  armYaltitude;      /* altitude (pitch) of the Y arm                 */
  float  armXmidpoint;      /* vertex to  middle of the X arm distance       */
  float  armYmidpoint;      /* vertex to  middle of the Y arm distance       */
  int    localTime;         /* local time - UTC time (second)                */
  FrVect *aux;              /* more data for this detector                   */
  FrTable *table;           /* additional table                              */
  FrDetector *next;         /* next detector structure                       */
                            /* ------ end_of_SIO parameters -----------------*/
  FrStatData *sData;        /* first static data bloc                        */
  FrStatData *sDataOld;     /* used by the tag/untag function                */
};

int         FrDetectorAddStatData(FrDetector* detector, FrStatData *stat);
FrSH       *FrDetectorDef(void);    
void        FrDetectorDump(FrDetector *detector, FILE *fp, int debugLvl);
FrDetector *FrDetectorMerge(FrDetector *det1, FrDetector *det2);
FrDetector *FrDetectorNew(char *name);
void        FrDetectorFree (FrDetector *detector);
FrStatData* FrDetectorFindStatData(FrDetector *det, char *statDataName, int gpsTime);
FrDetector *FrDetectorRead(FrFile *iFile);
void        FrDetectorUntagStat(FrDetector*det);
void        FrDetectorWrite(FrDetector *detector, FrFile *oFile);
/*---------------------------------------------------------------------------*/
/* template class for FrAdcData, FrProcData, FrSerData, FrSimData,           */
/*                FrSimEvent, FrStatData, FrSummary and FrEvent              */
/*---------------------------------------------------------------------------*/
struct FrBasic {                                                             
  FrSH    *classe;              /* class information (internal structure)    */
  char    *name;                /* object name                               */
  FrVect  *data;                /* vector of data                            */
  FrVect  *aux;                 /* could be an FrVect or FrTable             */
  FrBasic *next;                /* next structure                            */
  FrBasic *nextOld;             /* used by the tag/untag functions           */
}; 

/*---------------------------------------------------------------------------*/
struct FrEndOfFrame {            /* End of Frame Marker                      */
  FrSH *classe;                  /* class information (internal structure)   */
  int run;                       /* run number                               */
  unsigned int frame;            /* frame number                             */
  unsigned int chkType;          /* checkSum type                            */
  unsigned int chkSum;           /* checkSum value                           */
                                 /* ------------- end_of_SIO parameters ---- */
};
FrSH *FrEndOfFrameDef(void);    
void  FrEndOfFrameRead (FrFile *iFile);
void  FrEndOfFrameWrite(FrameH *frameH, FrFile *oFile);
/*---------------------------------------------------------------------------*/
struct FrEndOfFile {             /* End of File Marker                       */
  FrSH *classe;                  /* class information (internal structure)   */
  unsigned int  nFrames;         /* number of frame                          */
  FRULONG       nBytes;          /* number of bytes                          */
  unsigned int  chkType;         /* flag for checksum                        */
  unsigned int  chkSum;          /* checksum                                 */
  FRULONG       seekTOC;         /* seek for the TOC                         */
                                 /* ------------- end_of_SIO parameters ---- */
};
FrSH *FrEndOfFileDef(void);
void  FrEndOfFileRead (FrFile *iFile);
void  FrEndOfFileWrite(FrFile *oFile);

/*---------------------------------------------------------------------------*/
struct FrEvent {     
  FrSH *classe;                  /* class information (internal structure)   */
  char   *name;                  /* event name                               */
  FrVect *data;                  /* vector containing additional event info  */
  FrTable *table;                /* table for additional info                */
  FrEvent *next;                 /* next FrEvent structure                   */
  FrEvent *nextOld;              /* used by the tag/untag functions          */
                                 /*------- enf of the FrBasic structure------*/
  char   *comment;               /* comment                                  */
  char   *inputs;                /* input channels                           */
  unsigned int GTimeS;           /* Event occurrence time (second) (GPS time)*/
  unsigned int GTimeN;           /* Event occurrence time (nano second)      */
  float timeBefore;              /* signal duration before GTime             */
  float timeAfter;               /* signal duration after GTime              */
  unsigned int eventStatus;      /* defined by event                         */
  float  amplitude;              /* value returned by the trigger            */
  float  probability;            /* lieklihood estimate of event             */
  char *statistics;              /* Statisrical description of event         */
  unsigned short nParam;         /* number of additional parameters          */
  double  *parameters;           /* array of events parameters (size nParam) */
  char   **parameterNames;       /* array of parameters names  (size nParam) */
                                 /* -------- end_of_SIO parameters ----------*/
}; 
FrEvent *FrEventAddParam(FrEvent *event,  char *name,  double value);
int      FrEventAddVect (FrEvent *event,  FrVect *vect, char* newName);
int      FrEventAddVectF(FrEvent *event,  FrVect *vect, char* newName);

FrEvent *FrEventCopy(FrEvent *event);
FrSH    *FrEventDef(void);
void     FrEventDump(FrEvent *event, FILE *fp, int debugLvl);
FrEvent* FrEventFind(FrameH *frameH, char *name, FrEvent *last);
FrVect*  FrEventFindVect(FrEvent *event, char *vectName);
void     FrEventFree(FrEvent *event);
double   FrEventGetParam  (FrEvent *event, char *paramName);
int      FrEventGetParamId(FrEvent *event, char *paramName);
FrVect*  FrEventGetVectD(FrEvent *event, char *vectName);
FrVect*  FrEventGetVectF(FrEvent *event, char *vectName);
FrEvent *FrEventNew (FrameH *frameH,  
                 char *name,  char *comment, char *inputs, 
                 double gime, float  timeBefore, float  timeAfter,
                 float  amplitude,  float  probability, char   *stat,
                 FrVect *data, int nParam, ...);
FrEvent *FrEventRead    (FrFile *iFile);
int      FrEventReadData(FrFile *iFile, FrEvent *event);
FrEvent *FrEventReadT   (FrFile *iFile, char *name, double tStart, 
                  double length, double amplitudeMin, double amplitudeMax);
FrEvent *FrEventReadTF  (FrFile *iFile, char *name, double tStart, 
                  double length, int readData, int nParam, ...);
FrEvent *FrEventReadTF1(FrFile *iFile, char *name, double tStart,
                        double length, int readData,  
                        char *pName1, double min1, double max1);
FrEvent *FrEventReadTF2(FrFile *iFile, char *name, double tStart,
                        double length, int readData,  
                        char *pName1, double min1, double max1,
                        char *pName2, double min2, double max2);
FrEvent *FrEventReadTF3(FrFile *iFile, char *name, double tStart,
                        double length, int readData,  
                        char *pName1, double min1, double max1,
                        char *pName2, double min2, double max2,
                        char *pName3, double min3, double max3);
FrEvent *FrEventReadTF4(FrFile *iFile, char *name, double tStart,
                        double length, int readData,  
                        char *pName1, double min1, double max1,
                        char *pName2, double min2, double max2,
                        char *pName3, double min3, double max3,
                        char *pName4, double min4, double max4);
FrEvent *FrEventReadTF5(FrFile *iFile, char *name, double tStart,
                        double length, int readData,  
                        char *pName1, double min1, double max1,
                        char *pName2, double min2, double max2,
                        char *pName3, double min3, double max3,
                        char *pName4, double min4, double max4,
                        char *pName5, double min5, double max5);
int      FrEventSaveOnFile(FrEvent* event, FrFile* oFile);

void     FrEventWrite(FrEvent *event, FrFile *oFile);

#define FRMAXREF 50
/*---------------------------------------------------------------------------*/
struct FrFile{                   /* standard header for i/o file             */
  FrFileH *fileH;                /* fileHeader linked list                   */
  FrFileH *current;              /* current fileHeader pointer               */
  FRBOOL inMemory;               /* if YES file output is put in memory      */
  int  compress;                 /* type of compression used for this file   */
  unsigned short gzipLevel;      /* compression level used by gzip           */
  FrSH **sh;                     /* pointer to the sh struct. Used for read  */
  FrSH *firstSH ;
  FrSH *curSH;
  int maxSH;                     /* size of FrSHlist vector                  */
  FRFORMAT fmType;               /* give format conversion type              */
  FRFORMAT fmLong;               /* give format conversion type for long     */
  FRBOOL endOfFrame;             /* tell if we reach the end of frame ord    */
  FrameH *curFrame;              /* current frameH pointer for this file     */
  int fmtVersion;                /* format version                           */
  unsigned char header[40];      /* header for format type                   */
  int run;                       /* current run number on file               */
  unsigned int nFrames;          /* total number of frames                   */
  FRULONG nBytes;                /* total number of bytes (current value)    */
  FRULONG nBytesE;               /* total number of bytes (fron FrEndOfFile) */
  FRULONG nBytesF;               /* number of bytes at the frame begining    */
  FRULONG nBytesR;               /* number of bytes at the record begining   */
  FrIO *frfd;                    /* pseudo file descriptor                   */
  FRERROR error;                 /* error flag                               */
  char *buf;                     /* pointer to the output sting. The current
                                    buffer length = FrFile->buf - FrFile->p  */
  char *p;                       /* current pointer to the output string     */
  char *pStart;                  /* pointer to the begining of a ord         */
  char *pMax;                    /* maximum value for the output string p.   */
  FRLONG  bufSize;               /* output buffer size                       */
  FrStatData *sDataSim;          /* static data                              */
  FrStatData *sDataProc;         /* static data                              */
  FrStatData *sDataCur;          /* all static data for the current frame    */
  FRBOOL dicWrite[FRMAPMAXSH];   /* tell if the dict. info has been written  */
  int oldLocalTime;              /* Old Time offset (for frame file verion 4)*/
  FRLONG length;                 /* current record length read               */
  unsigned short vectorType;     /* SH id for FrVect structure when using TOC*/
  unsigned short detectorType;   /* type for the FrDetector structures       */
  unsigned short type;           /* current record type read                 */
  unsigned int instance;         /* current record instance read             */
  unsigned int instanceH;        /* current instance header read             */
  unsigned int vectInstance;     /* vector instance used by random read      */
  int nSH;                       /* FrSH struct count for the current frame  */
  int nSE;                       /* FrSE struct count for the current frame  */
  FrTOC *toc;                    /* file table of content                    */
  FRBOOL noTOCts;                /* tell if we dont write TOC for time serie */
  char *historyMsg;              /* history message to be added at output    */
  int  fLength;                  /* file length if multiple files is used    */
  int  dirPeriod;                /* directory period if multiple directory   */
  int  startTime;                /* file GPS starting time                   */
  int  closingTime;              /* file closing time (for multiples files)  */
  int  currentEndTime;           /* end time of the last frame written       */
  char *path;                    /* path for multiple files                  */
  char *prefix;                  /* file prefix if using multiple directory  */
  char *dirName;                 /* directory name if multiple directory     */
  FrSegList *segList;            /* segment list if available                */
  FRBOOL  convertH;              /* if yes, hermitian are  converted to type */
  FRBOOL  aligned;               /* if no; output file is not aligned on GPS */
                                 /* ------- checksum variables---------------*/
  FRBOOL       chkSumFiFlag;     /* if = YES compute file cksum              */
  FRBOOL       chkSumFrFlag;     /* if = YES compute frame cksum             */
  unsigned int chkSumFi;         /* computed file checksum                   */
  unsigned int chkSumFr;         /* computed frame checksum                  */
  unsigned int chkSumFrHeader;   /* computed file header checksum            */
  unsigned int chkSumFiRead;     /* file checksum read                       */
  unsigned int chkSumFrRead;     /* frame checksum read                      */
  unsigned int chkTypeFiRead;    /* file checksum flag read                  */
  unsigned int chkTypeFrRead;    /* frame checksum flag read                 */
                                 /*------- data used for address relocation--*/
  FRBOOL relocation;             /* tell if we skip relocation mechanism     */
  int lastInstance[FRMAPMAXSH];  /* last instance assigned for an FrSH type  */
  int    refType    [FRMAXREF];  /* temporary array of FrSH type             */
  int    refInstance[FRMAXREF];  /* temporary array of instance values       */
  void  *address    [FRMAXREF];  /* temporary array of address               */
  void **refAddress [FRMAXREF];  /* temporary array of referenced address    */
  int nRef;                      /* size of the 4 previous vectors           */
                                 /*------------- data use by FrFileIGetVect--*/
  double lastSlope;              /* last normalization factor(ADC only)      */
  double lastBias;               /* last pedestal value (ADC only)           */
  char *lastUnits;               /* last ADC units                           */
};

FrFileH* FrFileBreakName(char *fullName, FrFileH *next, FrSegList **segList);
void    FrFileDbg      (FrFile *file);
void    FrFileFree     (FrFile *file);
FrFile *FrFileNew(char *fileName, int compress, char *buf, FRLONG bufSize);

void    FrFileIAddSegList(FrFile *iFile, FrSegList *segList);
void    FrFileIClose   (FrFile *iFile);
void    FrFileIDump    (FrFile *iFile, FILE *fp, int debugLvl, char *tag);
void    FrFileIDumpT   (FrFile *iFile, FILE *fp, int debugLvl, char *tag, 
                                                 double tStart, double tEnd); 
void    FrFileIDumpEvt (FrFile *iFile, FILE *fp, int debugLvl); 
void    FrFileIDumpFr  (FrFile *iFile, FILE *fp, int debugLvl,
			                    double tStart, double length);
void    FrFileIEnd             (FrFile *iFile);
FrVect *FrFileIGetAdcNames     (FrFile *iFile);
char   *FrFileIGetChannelList  (FrFile *iFile, int gpsTime);
FrVect *FrFileIGetDetectorNames(FrFile *iFile);
FrVect *FrFileIGetEventInfo (FrFile *iFile,char *tag,
                           double tStart,double len, double aMin, double aMax);
FrVect *FrFileIGetEventNames(FrFile *iFile);
FrVect *FrFileIGetFrameInfo (FrFile *iFile, double tStart,double length);
FrVect *FrFileIGetFrameInfo1(FrFile *iFile, double tStart,double length);
FrVect *FrFileIGetProcNames (FrFile *iFile);
FrVect *FrFileIGetSerNames  (FrFile *iFile);
FrVect *FrFileIGetSimEventInfo(FrFile *iFile,char *tag,
                           double tStart,double len, double aMin, double aMax);
FrVect *FrFileIGetSimEventNames(FrFile *iFile);
FrVect *FrFileIGetSimNames  (FrFile *iFile);
FrVect *FrFileIGetStatNames (FrFile *iFile);

FrVect *FrFileIGetV     (FrFile *iFile, char *name, double tStart, double len);
FrVect *FrFileIGetVect  (FrFile *iFile, char *name, double tStart, double len);
FrVect *FrFileIGetVectD (FrFile *iFile, char *name, double tStart, double len);
FrVect *FrFileIGetVectDN(FrFile *iFile, char *name, double tStart, double len);
FrVect *FrFileIGetVectF (FrFile *iFile, char *name, double tStart, double len);
FrVect *FrFileIGetVectFN(FrFile *iFile, char *name, double tStart, double len);

FrVect *FrFileIGetVAdc (FrFile *iFile, char *name, double tStart, double len, 
                                       int   group);
FrVect *FrFileIGetVProc(FrFile *iFile, char *name, double tStart, double len,
                                       int   group);
FrVect *FrFileIGetVSim (FrFile *iFile, char *name, double tStart, double len,
		                       int   group);
FrVect *FrFileIGetVType(FrFile *iFile, char *name, double tStart, double len,
                                       int   group, FRTSTYPE type);
double FrFileIGetNextEventTime(FrFile *iFile, char *name, double tStart,
                                  double amplitudeMin);
int     FrFileIGoToNextRecord(FrFile *iFile);
FrFile *FrFileINew  (char *fileName);
FrFile *FrFileINewFd(FrIO *frfd);
int     FrFileINext  (FrFile *iFile,double tStart,double len,
                                FrFileH *firstFH, FRBOOL event);
void    FrFileIncSH  (FrFile *iFile, unsigned short id);
void    FrFileIOpen  (FrFile *file);
FRLONG  FrFileIOSet(FrFile *iFile, FRLONG offset);
FRLONG  FrFileIOSetFromEnd(FrFile *iFile, FRLONG offset);
FRLONG  FrFileIOSetFromCur(FrFile *iFile, FRLONG offset);
FrFile *FrFileIRewind(FrFile *iFile);
void    FrFileISegListMatch(FrFile *iFile);
int     FrFileISetTime(FrFile *iFile, double gtime);
void    FrFileIStat  (FrFile *iFile, FILE *fp);
double  FrFileITEnd      (FrFile *iFile);
double  FrFileITFirstEvt (FrFile *iFile);
double  FrFileITLastEvt  (FrFile *iFile);
double  FrFileITNextFrame(FrFile *iFile,double gtime);
double  FrFileITStart    (FrFile *iFile);

int     FrFileOEnd(FrFile *file);
FrFile *FrFileONew (char *fileName, int compress);
FrFile *FrFileONewH(char *fileName, int compress, char *program);
FrFile *FrFileONewFd   (FrIO *frfd, int compress);
FrFile *FrFileONewFdH  (FrIO *frfd, int compress, char *program);
FrFile *FrFileONewM(char *path,     int compress, char *program, int length);
FrFile *FrFileONewMD(char* path,    int compress, char* program, int fLength,
		     char* filePrefix, int   dirPeriod);

FrFileH* FrFileOpenCacheFile(char   *fullName);
void    FrFileOOpen        (FrFile *oFile);
int     FrFileORealloc     (FrFile *oFile, char *name, int size);
void    FrFileOReopen      (FrFile *oFile, int  gps);
void    FrFileOSetMsg      (FrFile *oFile, char *msg);
void    FrFileOSetGzipLevel(FrFile *oFile, unsigned short level);

/*---------------------------------------------------------------------------*/
struct FrFileH{                  /* header for each file                     */
  char *fileName;                /* file name                                */
  double tStart;                 /* starting GPS time for this file          */
  double length;                 /* time length for this file                */
  double tFirstEvt;              /* GPS time for earliest event in file      */
  double tLastEvt;               /* GPS time for the latest event in file    */
  FrFileH* next;                 /* next FrFileH in the linked list          */
};

int    FrGetLeapS(unsigned int gps);
double FrGetCurrentGPS (void);

/*---------------------------------------------------------------------------*/
struct FrHistory {               /* Describes data history info.             */
  FrSH *classe;                  /* class information (internal structure)   */
  char *name;                    /* name of history ord                      */
  unsigned int time;             /* time of reprocessing(GPS)                */
  char *comment;                 /* program name and comment                 */
  FrHistory *next;               /* pointer to the next history struct       */
                                 /* ------------- end_of_SIO parameters -----*/
};

FrHistory *FrHistoryAdd(FrameH *frame, char *comment);
FrHistory *FrHistoryCopy(FrHistory *historyIn);
FrSH      *FrHistoryDef(void);    
void       FrHistoryFree(FrHistory *history);
FrHistory *FrHistoryNew(char *name, unsigned int mytime, char *comment);
void       FrHistoryRead(FrFile *iFile);
void       FrHistoryWrite(FrHistory *history, FrFile *oFile);

/*---------------------------------------------------------------------------*/
void   FrSetAll(FrFile *file);
void   FrSetBAT(FrFile *file, unsigned int   instance, void *add);
void   FrSetBRT(FrFile *file, unsigned int   instance, 
                              unsigned short type, void *add);
void   FrSetIni(FrFile *file);
/*---------------------------------------------------------------------------*/
struct FrMsg {                   /* hold  message      information           */
  FrSH *classe;                  /* class information (internal structure)   */
  char   *alarm;                 /* data name                                */
  FrVect *dummy1;                /* unused FrBasic element                   */
  FrVect *dummy2;                /* unsued FrBasic element                   */
  FrMsg  *next;                  /* next bloc of onstructed data             */
  FrMsg  *nextOld;               /* used by the tag/untag functions          */
                                 /*------- enf of the FrBasic structure------*/
  char   *message;               /* error message                            */
  unsigned int severity;         /* severity (to be defined)                 */
  unsigned int GTimeS;           /* message occurrence time (GPS seconds)    */
                                 /* if GTimeS=0, the FrameH value is be used */
  unsigned int GTimeN;           /* message occurrence time (GPS nano second)*/
                                 /* -------- end_of_SIO parameters ----------*/
}; 
FrMsg *FrMsgAdd(FrameH *frame, char *alarm, char *message, 
                               unsigned int severity);
FrSH  *FrMsgDef(void);
void   FrMsgDump(FrMsg *msg,FILE *fp, int debugLvl);
FrMsg *FrMsgFind(FrameH *frame, char *alarm, FrMsg *msg);
void   FrMsgFree(FrMsg *msg);
FrMsg *FrMsgRead(FrFile *iFile);
void   FrMsgWrite(FrMsg *msg, FrFile *oFile);
/*---------------------------------------------------------------------------*/
struct FrObject {                /* prototype of FrameLib structure          */
  FrSH *classe;                  /* class information (internal structure)   */
  char *name;                    /* name for this element                    */
}; 

FrSH *FrObjectDef(void);
/*---------------------------------------------------------------------------*/
struct FrProcData {              /* process and reconstructed data           */
  FrSH *classe;                  /* class information (internal structure)   */
  char   *name;                  /* data name                                */
  FrVect *data;                  /* main data like the metric perturbation(h)*/
  FrVect *aux;                   /* auxillary data like for h: a vector of 6
                                    values: the detector equatorial position 
                                   (x,y,z) and speed (vx,vy,vz) at the first 
                                    sampling.                                */
  FrProcData *next;              /* next bloc of onstructed data             */
  FrProcData *nextOld;           /* used by the tag/untag functions          */
                                 /*------- enf of the FrBasic structure------*/
  unsigned short type;           /* vector type                              */
  unsigned short subType;        /* vector subType                           */
  char   *comment;               /* comment                                  */
  double timeOffset;             /* offset from the frame starting time (sec)*/
  double tRange;                 /* duration of sampled data (tStop-tStart)  */
  double fShift;                 /* frequency shift if the signal was 
                                             heterodyne (Hz) (0->no shift)   */
  float  phase;                  /* phase for heterodyning signal            */
  double fRange;                 /* duration of sampled data (tStop-tStart)  */
  double BW;                     /* resolution BW                            */
  unsigned short nAuxParam;      /* number of auxiliary parameters           */
  double *auxParam;              /* auxiliary parameters values              */
  char  **auxParamNames;         /* auxiliary parameters names               */
  FrTable *table;                /* auxiallary data                          */
  FrHistory *history;            /* history for this proc data               */
                                 /* ------ end_of_SIO parameters ------------*/
};

FrHistory  *FrProcDataAddHistory(FrProcData *proc,char *comment,int nPrev,...);
FrProcData *FrProcDataAddParam  (FrProcData *proc,char *name, double value);
void        FrProcDataAttachVect(FrProcData *proc, FrVect *vect);
  FrProcData *FrProcDataCopy(FrProcData *procIn, FrameH* frame);
FrSH       *FrProcDataDef(void);
void        FrProcDataDump(FrProcData *proc, FILE *fp,  int debugLvl);
FrProcData *FrProcDataFind (FrameH *frame, char *name);
FrVect*     FrProcDataFindVect(FrProcData *proc, char *name);
void        FrProcDataFree      (FrProcData *procData);
FrProcData* FrProcDataFreeOne   (FrProcData *proc);
double      FrProcDataGetParam  (FrProcData *proc, char *name);
int         FrProcDataGetParamId(FrProcData *proc, char *name);
FrProcData *FrProcDataNew (FrameH *frame, char *name, 
                           double sampleRate, FRLONG nData, int nBits);
FrProcData *FrProcDataNewV(FrameH *frame, FrVect* vect);
FrProcData* FrProcDataNewVT(FrameH* frame, FrVect *vect, int type);
FrProcData *FrProcDataRead (FrFile *iFile);
FrProcData *FrProcDataReadT(FrFile *iFile, char *name, double gtime);
void        FrProcDataWrite(FrProcData *procData, FrFile *oFile);
/*---------------------------------------------------------------------------*/
struct FrRawData {               /* Hold raw data                            */
  FrSH *classe;                  /* class information (internal structure)   */
  char   *name;                  /* name                                     */
  FrSerData *firstSer;           /* pointer to the first slow mon. station   */
  FrAdcData *firstAdc;           /* pointer to the first adc structure       */
  FrTable *firstTable;           /* pointer to the first table               */
  FrMsg *logMsg;                 /*pointer to the error message structure    */
  FrVect *more;                  /*pointer aditional data                    */
                                 /* ------------- end_of_SIO parameters -----*/
  FrAdcData *firstAdcOld;        /* used by the tag/untag function           */
  FrSerData *firstSerOld;        /* used by the tag/untag function           */
  FrMsg     *logMsgOld;          /* used by the tag/untag function           */
};

FrSH      *FrRawDataDef(void);    
void       FrRawDataDump(FrRawData *rawData, FILE *fp, int debugLvl);
void       FrRawDataFree(FrRawData *rawData);
FrRawData *FrRawDataNew(FrameH *frame);
FrRawData *FrRawDataRead(FrFile *iFile);
void       FrRawDataUntagAdc(FrRawData *rawData);
void       FrRawDataUntagMsg(FrRawData *rawData);
void       FrRawDataUntagSer(FrRawData *rawData);
void       FrRawDataWrite   (FrRawData *rawData, FrFile *oFile);
/*---------------------------------------------------------------------------*/
struct FrSE {                    /* hold structure information(one per line) */
  FrSH *classe;                  /* class information (internal structure)   */
  char        *name;             /* name for this element                    */
  char        *type;             /* element type                             */
  char        *comment;          /* comment                                  */
                                 /* ------------ end_of_SIO parameters ------*/
  FrSE *next;                    /* next struct E                            */
};
FrSH *FrSEDef(void);
void  FrSENew(FrSH *structH, char *type, char *name, char *comment);
void  FrSERead(FrFile *iFile);
void  FrSEWrite(FrSE *structE, FrFile *oFile);
/*---------------------------------------------------------------------------*/
struct FrSegList {
  char *name;        /* segment list name (file name usually)                */
  FrVect *tStart;    /* vector of starting time                              */
  FrVect *tEnd;      /* vector of end time                                   */
  FrSegList *next;   /* to easily build a linked list                        */
};

int        FrSegListAdd       (FrSegList *segList, double tStart, double tEnd);
FrVect*    FrSegListBuildVect (FrSegList *segList, double tStart, 
                                                   double length, int nData);
double     FrSegListCoverage  (FrSegList *segList, double gtime, double dt);
int        FrSegListDump      (FrSegList *segList, FILE *fp, int debugLvl);
int        FrSegListFind      (FrSegList *segList, double gtime);
int        FrSegListFindFirst (FrSegList *segList, double gtime, double dt);
double     FrSegListFindFirstT(FrSegList *segList, double gtime, double dt);
double     FrSegListFindLastT (FrSegList *segList, double gtime, double dt);
void       FrSegListFree      (FrSegList *segList);
FrSegList* FrSegListIntersect (FrSegList *list1, FrSegList* list2);
FrSegList *FrSegListNew(char *name);
int        FrSegListPrint     (FrSegList *segList, char *fileName);
FrSegList* FrSegListRead(char *fileName);
  FrSegList* FrSegListReadFP(FILE *fp, char* name);
int        FrSegListShrink    (FrSegList *segList, double tStart, double tEnd);

/*---------------------------------------------------------------------------*/
struct FrSerData {               /* slow monitoring station data             */
  FrSH *classe;                  /* class information (internal structure)   */
  char *name;                    /* slow monitoring station name             */
  FrVect *serial;                /* additional s.m. data for this station    */
  FrTable *table;                /* additional s.m. data for this station    */
  FrSerData *next;               /* next slow monitoring station             */
  FrSerData *nextOld;            /* used by the tag/untag functions          */
                                 /*------- enf of the FrBasic structure------*/
  unsigned int timeSec;          /* F.B. collection time (GPS)               */
  unsigned int timeNsec;         /* F.B. collection time (GPS)               */
  double sampleRate;             /* sample/s.                                */
  char *data;                    /* pointer to the data block                */
};

FrSH      *FrSerDataDef(void);  
void       FrSerDataDump(FrSerData *serData, FILE *fp, int debugLvl);
FrSerData *FrSerDataFind(FrameH *frameH,char *smsName, FrSerData *sms);
void       FrSerDataFree(FrSerData *smsData);
int        FrSerDataGet (FrameH *FrameH, char *smsName, 
                             char *smsParam, double *value); 
double     FrSerDataGetValue(FrameH *frame,  char *smsName,
                            char  *smsParam, double defaultValue);
FrSerData *FrSerDataNew (FrameH *frame, char *smsName, 
                         unsigned int serTime, char *data, double sampleRate);
FrSerData *FrSerDataRead (FrFile *iFile);
FrSerData *FrSerDataReadT(FrFile *iFile, char *name,double gtime);
void       FrSerDataWrite(FrSerData *smsData, FrFile *oFile);
/*---------------------------------------------------------------------------*/
struct FrSH {                    /* hold info for this type of struct        */
  FrSH *classe;                  /* class information (internal structure)   */
  char   *name;                  /* name for this type of structure          */
  unsigned short  id;            /* is structure when written on tape        */
  char   *comment;               /* comment                                  */
  FRLONG nBytes;                 /* total number of bytes for this structure */
  int    nInstances;             /* total number of instances for this struct*/
                                 /* ------------ end_of_SIO parameters ------*/
  FrSE *firstE;                  /* first structure element                  */
  void (*objRead)(FrFile *iFile);/* function to read the object on file      */
  int nSE;                       /* number of element for this structure     */
  FrSH *shRef;                   /* reference SH struct (used for in. file)  */
};

FrSH *FrSHDef(void);
void  FrSHMatch(FrFile *iFile);
FrSH *FrSHNew(char *name, char *comment);
void  FrSHRead(FrFile *iFile);
void  FrSHWrite(FrSH *structH, FrFile *oFile);
/*---------------------------------------------------------------------------*/
struct FrSimData {               /* hold Simulated data                      */
  FrSH *classe;                  /* class information (internal structure)   */
  char   *name;                  /* name                                     */
  FrVect *data;                  /*buffer for the data                       */
  FrVect *input;                 /* input signal                             */
  FrSimData *next;               /* next structure                           */
  FrSimData *nextOld;            /* used by the tag/untag functions          */
                                 /*------- enf of the FrBasic structure------*/
  char   *comment;               /* comment                                  */
  double sampleRate;             /* sampling rate  (unit= Hz)                */
  double timeOffset;             /* offset from the frame starting time (sec)*/
  double fShift;                 /* frequency shift if the signal was 
                                         heterodyne (Hz) (0->no shift)       */
  float  phase;                  /* phase of heterodyning signal.            */
  FrTable *table;                /* data stored in table                     */
                                 /* ------ end_of_SIO parameters ------------*/
}; 

FrSH      *FrSimDataDef(void);    
void       FrSimDataDump(FrSimData *simData, FILE *fp, int debugLvl);
FrSimData *FrSimDataFind (FrameH *frame, char *name);
void       FrSimDataFree(FrSimData *simData);
FrSimData *FrSimDataFreeOne(FrSimData *simData);
FrSimData *FrSimDataNew (FrameH *frame, char *name, 
                         double sampleRate, FRLONG nData, int nBits);
FrSimData *FrSimDataRead (FrFile *iFile);
FrSimData *FrSimDataReadT(FrFile *iFile, char *name, double gtime);
void       FrSimDataWrite(FrSimData *adcData, FrFile *oFile);
/*---------------------------------------------------------------------------*/
struct FrSimEvent {     
  FrSH *classe;                  /* class information (internal structure)   */
  char   *name;                  /* name of event type                       */
  FrVect *data;                  /* vector containing additional event info  */
  FrTable *table;                /* table containing additional info.        */
  FrSimEvent *next;              /* next event structure                     */
  FrSimEvent *nextOld;           /* used by the tag/untag functions          */
                                 /*------- enf of the FrBasic structure------*/
  char   *comment;               /* comment                                  */
  char   *inputs;                /* input channels                           */
  unsigned int GTimeS;           /* Event occurrence time (second) (GPS time)*/
  unsigned int GTimeN;           /* Event occurrence time (nano second)      */
  float  timeBefore;             /* signal duration before GTimeS            */
  float  timeAfter;              /* signal duration after GTimeS             */
  float  amplitude;              /* signal amplitude                         */
  unsigned short nParam;         /* number of additional parameters          */
  double  *parameters;           /* array of events parameters (size nParam) */
  char   **parameterNames;       /* array of parameters names  (size nParam) */
                                 /* -------- end_of_SIO parameters ----------*/
}; 
FrSimEvent *FrSimEventAddParam(FrSimEvent *event,  char *name,  double value);
int         FrSimEventAddVect (FrSimEvent *event,  FrVect *vect, char* newName);
int         FrSimEventAddVectF(FrSimEvent *event,  FrVect *vect, char* newName);

FrSimEvent *FrSimEventCopy(FrSimEvent *event);
FrSH       *FrSimEventDef(void);
void        FrSimEventDump(FrSimEvent *simEvent,FILE *fp, int debugLvl);
FrSimEvent *FrSimEventFind(FrameH *frameH, char *name, FrSimEvent *last);
FrVect*     FrSimEventFindVect  (FrSimEvent *event, char *vectName);
void        FrSimEventFree      (FrSimEvent *event);
double      FrSimEventGetParam  (FrSimEvent *event, char *paramName);
int         FrSimEventGetParamId(FrSimEvent *event, char *paramName);
FrVect*     FrSimEventGetVectD  (FrSimEvent *event, char *vectName);
FrVect*     FrSimEventGetVectF  (FrSimEvent *event, char *vectName);
FrSimEvent *FrSimEventNew(FrameH *frameH, char *name, char *comment,
                 char *inputs, double time, float timeBefore, float timeAfter,
                 float amplitude, int nParam, ...);
FrSimEvent *FrSimEventRead (FrFile *iFile);
int         FrSimEventReadData(FrFile *iFile, FrSimEvent *event);
FrSimEvent *FrSimEventReadT(FrFile *iFile, char *name,
                            double gtime, double dt, double aMin, double aMax);
FrSimEvent *FrSimEventReadTF  (FrFile *iFile, char *name, double tStart, 
                        double length, int readData, int nParam, ...);
FrSimEvent *FrSimEventReadTF1(FrFile *iFile, char *name, double tStart,
                        double length, int readData,  
                        char *pName1, double min1, double max1);
FrSimEvent *FrSimEventReadTF2(FrFile *iFile, char *name, double tStart,
                        double length, int readData,  
                        char *pName1, double min1, double max1,
                        char *pName2, double min2, double max2);
void        FrSimEventSetParam(FrSimEvent *event, char *name,  double value);
void        FrSimEventWrite(FrSimEvent *simEvent, FrFile *oFile);
/*---------------------------------------------------------------------------*/
struct FrStatData { /*-- this structure is used to hold quasi permanent data. 
                This data stay has long they are valid compare to the frame 
             starting time (GPS), or as long there is not a new bloc of data 
     	          with the same name more ent or with  an higgest version 
     	           number ---------------------------------------------------*/
  FrSH *classe;                  /* class information (internal structure)   */
  char *name;                    /* static data name name                    */
  FrVect *data;                  /* vector for this stat data                */
  FrTable *table;                /* additional data block                    */
  FrStatData *next;              /* next static data bloc                    */
  FrStatData *nextOld;           /* used by the tag/untag functions          */
                                 /*------- enf of the FrBasic structure------*/
  char   *comment;               /* comment 	                             */
  char   *representation;        /* type of representation                   */
  unsigned int timeStart;        /* data validity starting time (GPS time)   */
  unsigned int timeEnd;          /* data validity end time (GPS time)        */
  unsigned int version;          /* version number                           */
  FrDetector *detector;          /* detector owning static data              */
  int  overlap;
  char *detName;                 /* name of the detector owning this statData*/
};

void        FrStatDataAdd(FrDetector *detector, FrStatData *sData);
void        FrStatDataAddR(FrStatData **root,   FrStatData *sData);
void        FrStatDataAddVect(FrStatData *stat, FrVect *vect);
void        FrStatDataChkT(FrStatData **root, 
                       unsigned int timeStart, unsigned int timeEnd);
FrStatData *FrStatDataCopy(FrStatData *sData, FrDetector *detector);
FrSH       *FrStatDataDef(void);    
void        FrStatDataDump(FrStatData *sData, FILE *fp, int debugLvl);
FrStatData *FrStatDataFind(FrDetector *detector, char *name,
                       unsigned int timeNow);
void        FrStatDataFree(FrStatData *sData);
FrStatData *FrStatDataFreeOne(FrStatData *sData);
FrStatData *FrStatDataNew(char *name, char *comment,char *represent,
                  unsigned int tStart, unsigned int tEnd,unsigned int version,
                  FrVect *data, FrTable *table);
FrStatData *FrStatDataRead(FrFile *iFile);
FrStatData* FrStatDataReadT(FrFile *iFile, char *name, double gtime);
void        FrStatDataRemove(FrDetector *detector, char *name);
void        FrStatDataTouch(FrStatData *sData);
void        FrStatDataWrite(FrStatData *sData, FrFile *oFile);
/*---------------------------------------------------------------------------*/
struct FrSummary {     
  FrSH *classe;                  /* class information (internal structure)   */
  char   *name;                  /* summary name                             */
  FrVect *moments;               /* vector containing statistical description*/
  FrTable *table;                /* data stored in table                     */
  FrSummary *next;               /* next summary structure                   */
  FrSummary *nextOld;            /* used by the tag/untag functions          */
                                 /*------- enf of the FrBasic structure------*/
  char   *comment;               /* comment                                  */
  char   *test;                  /* statistical test(s) used on raw data     */
  unsigned int GTimeS;           /* FrSummary time (GPS seconds)             */
  unsigned int GTimeN;           /* FrSummary time (GPS nano second)         */
                                 /* -------- end_of_SIO parameters ----------*/
}; 
FrSH      *FrSummaryDef(void);
void       FrSummaryDump(FrSummary *summary, FILE *fp, int debugLvl);
FrSummary *FrSummaryFind (FrameH *frame, char *name);
void       FrSummaryFree(FrSummary *summary);
FrSummary *FrSummaryNew (FrameH *frame, char *name, char *comment,  
                          char *test, FrVect *moments, FrTable *table);
FrSummary *FrSummaryRead (FrFile *iFile);
FrSummary *FrSummaryReadT(FrFile *iFile, char *name, double gtime);
void       FrSummaryWrite(FrSummary *summary, struct FrFile *oFile);
/*---------------------------------------------------------------------------*/
struct FrTable {     
  FrSH *classe;                  /* class information (internal structure)   */
  char   *name;                  /* table name                               */
  char   *comment;               /* comment                                  */
  unsigned short nColumn;        /* number of column                         */
  unsigned int nRow;             /* number of row                            */
  char   **columnName;           /* name of each column                      */
  FrVect  *column;               /* first column of the table                */
  FrTable *next;                 /* next table if any                        */
                                 /* -------- end_of_SIO parameters ----------*/
};
void     FrTableCompress(FrTable *table, int compType, int gzipLevel);
FrTable *FrTableCopy    (FrTable *table);
FrSH    *FrTableDef(void);
void     FrTableDump  (FrTable *table, FILE *fp, int debugLvl);
void     FrTableExpand(FrTable *table);
FrVect*  FrTableGetCol(FrTable *table, char *colName);
void     FrTableFree  (FrTable *table);
FrTable *FrTableNew(char *name, char *comment, int  nRow, int  nColumn, ...);
void     FrTableRead(FrFile *iFile);
void     FrTableWrite(FrTable *table, FrFile *oFile);
/*---------------------------------------------------------------------------*/
struct FrTag {                   /* data for tag match (internal structure)  */
  char  *start;                  /* start of each tag segment                */
  int    length;                 /* length of each tag segment               */
  FrTag *next;                   /* next FrTag structure                     */
  FRBOOL returnValue;            /* = FR_NO for antitag (start by '-')       */
};

void   FrTagFree (FrTag *tag);
char*  FrTagGetSubTag(char *type, char *tag);
FRBOOL FrTagMatch(FrTag *tag, char *name);
FRBOOL FrTagMatch1(char *name, int   nameL, char *tag, int   tagL);
FrTag *FrTagNew(char *tag);
 
/*---------------------------------------------------------------------------*/
struct FrTOC {                   /* File Table Of Content                    */
  struct FrSH *classe;           /* class information (internal structure)   */
  short  ULeapS; 
  int nFrame;
  unsigned int *dataQuality;
  unsigned int *GTimeS;
  unsigned int *GTimeN;
  double *dt;
  int    *runs;
  int    *frame;
  FRULONG *positionH;
  FRULONG *nFirstADC;
  FRULONG *nFirstSer;
  FRULONG *nFirstTable;
  FRULONG *nFirstMsg;
  unsigned int nSH;
  unsigned short *SHid;
  unsigned short SHidMax;
  char **SHname;
  FrTOCdet *detector;
  int nStatType;
  FrTOCstat *stat;
  FrTOCts *adc;
  FrTOCts *proc;
  FrTOCts *sim;
  FrTOCts *ser;
  FrTOCts *summary;
  FrTOCtsH *adcH;
  FrTOCtsH *procH;
  FrTOCtsH *simH;
  FrTOCtsH *serH;
  FrTOCtsH *summaryH;
  FrTOCevt *event;
  FrTOCevt *simEvt;
  int maxFrame;
  FRLONG position;
};
FrSH  *FrTOCDef(void);
void   FrTOCDump(FrTOC *toc, FILE *fp, int debugLvl, char *tag);
void   FrTOCFFLBuild(FrFile *iFile);
void   FrTOCFree(FrTOC *toc);
void   FrTOCFrame     (FrFile *oFile, FrameH *frame);
int    FrTOCFrameFindN(FrFile *iFile, int run, int frame);
int    FrTOCFrameFindNT(FrFile *iFile, double gtime);
int    FrTOCFrameFindT (FrFile *iFile, double gtime);
int    FrTOCGetFrameIndexOneFile(FrFile *iFile, double gtime);
int    FrTOCGetLTOffset(FrFile *iFile, char *channelName);
void   FrTOCGetTimes  (FrFile *iFile);
void   FrTOCNew       (FrFile *oFile, FrameH *frame);
FrTOC *FrTOCRead      (FrFile *iFile,  int nFrame);
FrTOC *FrTOCReadFull  (FrFile *iFile);
void   FrTOCSegListMatch(FrFile *iFile);
int    FrTOCSetPos    (FrFile *iFile, FRULONG position);
void   FrTOCSH        (FrFile *oFile, char *name, unsigned short id);
void   FrTOCSort      (FrFile *iFile);
int    FrTOCSort1  (char **dataIn, int *index, int nData, int size);
int    FrTOCStatD     (FrFile *oFile, FrStatData *sData);
void   FrTOCStatDGet  (FrFile *iFile, FrameH *frame);
void   FrTOCStatRead  (FrFile *iFile);
void   FrTOCWrite     (FrFile *oFile);
/*---------------------------------------------------------------------------*/
struct FrTOCdet {
  char         *name;      /* name for this class of detector                */
  int           localTime; /* local time offset for this detector            */
  FRULONG       position;  /* position position in the file                  */
  FrTOCdet      *next;     /* next FrTOC event in the lnked list             */
};
void      FrTOCdetFree(FrTOCdet *det);
void      FrTOCdetMark(FrFile *oFile,  char *name);
FrTOCdet *FrTOCdetRead(FrFile *iFile);
void      FrTOCdetWrite(FrFile *oFile);
/*---------------------------------------------------------------------------*/
struct FrTOCevt {
  char         *name;      /* name for this class of events                  */
  int           nEvent;    /* number of event of this type                   */
  unsigned int *GTimeS;    /* GPS for these events       (array size: nEvent)*/
  unsigned int *GTimeN;    /* GPS time (nanoseconds)     (array size: nEvent)*/
  float        *amplitude; /* event amplitude            (array size: nEvent)*/
  FRULONG      *position;  /* event position in the file (array size: nEvent)*/
  int           maxEvent;  /* size of the arrays                             */
  FrTOCevt      *next;     /* next FrTOC event in the lnked list             */
};
void      FrTOCevtDump(FrTOCevt *evt, FILE *fp, int maxPrint, int ULeapS);
void      FrTOCevtFree(FrTOCevt *evt);
FrVect   *FrTOCevtGetEventInfo(FrTOCevt *root,char *tag,
                               double tStart, double length,
                               double amplitudeMin,double amplitudeMax);
FrTOCevt *FrTOCevtMark(FrFile *oFile, FrTOCevt **root, char *name,
                  unsigned int GTimeS, unsigned int GTimeN, float amplitude);
FrTOCevt *FrTOCevtNew(char *name, int maxEvent);
FrTOCevt *FrTOCevtRead (FrFile *iFile);
void      FrTOCevtSegListMatch(FrFile *iFile, FrTOCevt *root);
void      FrTOCevtWrite(FrFile *oFile, FrTOCevt *evt);
/*---------------------------------------------------------------------------*/
struct FrTOCstat {            /* direct access data for FrStatData           */
  char *name;                 /* static data bloc name                       */
  char *detector;             /* detector name                               */
  int nStat;                  /* number of instance                          */
  unsigned int *tStart;       /* array (nStat) of starting time (GPS time)   */
  unsigned int *tEnd;         /* array (nStat) of validity end time(GPS time)*/
  unsigned int *version;      /* array (nStat) of version number             */
  FRLONG *position;           /* array (nStat) of position in the file       */
			      /* ------ end_of_SIO parameters ---------------*/
  int maxStat;                /* maximum number of instance                  */
  struct FrTOCstat *next;     /* next static data bloc                       */
 };
/*---------------------------------------------------------------------------*/
struct FrTOCts {                   /*table of content info for time series   */
  char           *name;
  unsigned int   channelID;          /* only for adc                         */
  unsigned int   groupID;            /* only for adc                         */
  FRULONG        *position; 
  struct FrTOCts *next;
};
void     FrTOCtsFree(FrTOCts *ts);
void     FrTOCtsMark(FrFile *oFile, FrTOCtsH **root,
                 char *name, unsigned int groupID, unsigned int channelID);
void     FrTOCtsDump(FrTOCts *root, FILE *fp, char *type,char *tag,int nFrame);
FrTOCts *FrTOCtsRead(FrFile *iFile, char *type, int nFrame);
void     FrTOCtsRealloc(FrTOCtsH *tsh, FrFile *oFile, int oldSize);
void     FrTOCtsWrite(FrFile *oFile, FrTOCtsH *root, char *type, int n);
/*---------------------------------------------------------------------------*/
struct FrTOCtsH{         /* Intermediate structure to speed up FrTOCts access*/
  int nElement;          /* number of FrTOCts strcuture attached             */
  struct FrTOCts *first; /* first FrTOCts structure                          */
  struct FrTOCtsH *next; /* next FrTOCtsH                                    */
};
void   FrTOCtsHFree(struct FrTOCtsH *tsh);

/*---------------------------------------------------------------------------*/
/* Remark: See the file FrVect.h for the definition of FrVect (since v6r10)  */
/*---------------------------------------------------------------------------*/
#include "FrVect.h"

int     FrVectCtoH   (FrVect *vect);
unsigned short FrVectCompData(FrVect *vect, int compType, int gzipLevel, 
                       unsigned char **result, FRULONG *nBytes);
void    FrVectCompress(FrVect *vect,int compress, int gzipLevel);
FrVect *FrVectConcat  (FrVect *vect, double tStart, double len);

FrVect *FrVectCopy     (FrVect *vect);
FrVect *FrVectCopyPartX(FrVect *vect, double xStart, double length);
FrVect *FrVectCopyPartI(FrVect *vect, int    iStart, int nTot);
FrVect *FrVectCopyTo   (FrVect *vect, double scale, FrVect *copy);
FrVect *FrVectCopyToD  (FrVect *vect, double scale, char *newName);
FrVect *FrVectCopyToF  (FrVect *vect, double scale, char *newName);
FrVect *FrVectCopyToI  (FrVect *vect, double scale, char *newName);
FrVect *FrVectCopyToS  (FrVect *vect, double scale, char *newName);

FrVect *FrVectDecimate(FrVect *vect, int nGroup, FrVect *outVect);
void    FrVectDecimateMax(FrVect *vect, int nGroup);
void    FrVectDecimateMin(FrVect *vect, int nGroup);
FrSH   *FrVectDef(void);    
char   *FrVectDiff   (FrVect *vect);
void    FrVectDump   (FrVect *vect, FILE *fp, int debugLvl);
void    FrVectExpand (FrVect *vect);
void    FrVectExpandF(FrVect *vect);
FrVect *FrVectExtend (FrVect *vect, int nTimes, FrVect *outVect, char *newName);

int     FrVectFillC  (FrVect *vect,  char value);
int     FrVectFillD  (FrVect *vect,double value);
int     FrVectFillF  (FrVect *vect, float value);
int     FrVectFillI  (FrVect *vect,   int value);
int     FrVectFillQ  (FrVect *vect, char *value);
int     FrVectFillS  (FrVect *vect, short value);
int     FrVectFindQ  (FrVect *vect, char *name);
int     FrVectFixNAN(FrVect *vect);
void    FrVectFixVirgoImage(FrVect *vect);
void    FrVectFree   (FrVect *vect);

FRLONG  FrVectGetIndex  (FrVect *vect, double x);
FRLONG  FrVectGetTotSize(FrVect *vect);
double  FrVectGetV      (FrVect *vect, FRULONG i); /*-- obsolete --*/
double  FrVectGetValueI (FrVect *vect, FRULONG i);
double  FrVectGetValueX (FrVect *vect, double x);
double  FrVectGetValueGPS(FrVect *vect, double gps);

int     FrVectHtoC   (FrVect *vect);
void    FrVectInt    (FrVect *vect);
int     FrVectIsValid(FrVect *vect);
void    FrVectIsValidFillAux(FrVect *vect);
int     FrVectIsValidStrict(FrVect *vect);
FrVect* FrVectLoad(char *fileName);
void    FrVectMap    (FrVect *vect);
FrVect *FrVectMergeT (FrVect **vectIn, int nVect);
double  FrVectMean   (FrVect *vect);
int     FrVectMinMax (FrVect *vect, double *min, double *max);
FrVect *FrVectNew  (int type, int nDim, ...);
FrVect *FrVectNewL (int type, int nDim, ...);
FrVect *FrVectNewTS(char *name,double sampleRate, FRLONG nData, int nBits);
FrVect *FrVectNew1D(char *name,int type, FRLONG nData, 
                           double dx, char *unitx, char *unity);
FrVect *FrVectRead    (FrFile *iFile);
FrVect *FrVectReadNext(FrFile *iFile, double gtime, char *name);
FrVect *FrVectReadZoom(FrFile *iFile, double xStart, double length);

FrVect *FrVectResample(FrVect *vect, int nDataNew, FrVect *outVect, char* newName);
int     FrVectReshapeAdd(FrVect *vect, FrVect *add);
void    FrVectReshapeEnd(FrVect *vect);
FrVect *FrVectReshapeNew(FrVect *vect,double tStart, double len);
double  FrVectRMS(FrVect *vect);
int     FrVectSave(FrVect *vect, char *fileName);

int     FrVectSetMissingValues(FrVect *vect, double def);
void    FrVectSetName    (FrVect *vect, char *name);
void    FrVectSetUnitX   (FrVect *vect, char *unitX);
void    FrVectSetUnitY   (FrVect *vect, char *unitY);

char   *FrVectStat    (FrVect *vect);
void    FrVectToAudio(FrVect* vect, char *fileName, char *option);
void    FrVectWrite   (FrVect *vect, FrFile *oFile);
int     FrVectFComp(short *out, FRULONG *compL,
                    float *data, FRULONG nData, int bSize);
void    FrVectFExpand(float *out, short  *data, FRULONG nData);
int     FrVectUComp(FrVect *vect, unsigned char *out, 
                    FRULONG *compL, int *compType);
void    FrVectUExpand(FrVect *vect, unsigned char *out);
int     FrVectZComp (unsigned short *out, FRULONG *compL,
                   short *data, FRULONG nData, int bSize);
int     FrVectZCompI(unsigned   int *out, FRULONG *compL, 
                 int *data,  FRULONG nData, int bSize);
void    FrVectZExpand(short *out, unsigned short *data, FRULONG nData);
void    FrVectZExpandI( int *out, unsigned   int *data, FRULONG nData);
  void    FrVectZExpandL(long long *out, unsigned long long *data, FRULONG nData);
int     FrVectZoomIn (FrVect *vect, double xStart, double length);
int     FrVectZoomInI(FrVect *vect, FRLONG iFirst, FRLONG nBin);
int     FrVectZoomOut(FrVect *vect);
/*--------------------------------------------------------------------------*/
/*  General functions                                                       */
/*--------------------------------------------------------------------------*/
FILE *FrLibIni(char *outFile, FILE *fOut, int dbglvl);
void  FrLibShortIni(void);
void  FrLibSetLvl(int dbglvl);
void  FrLibSetFlipPrefix(char *prefix);
void  FrLibSetVersion(int formatVersion);
float FrLibVersion(FILE *fOut);
char *FrLibVersionF(void);
/*--------------------------------------------------------------------------*/
char *FrError(int level, char *subroutine, char *message);
char *FrErrorGetHistory(void);
void  FrErrorDefHandler(int level, char *lastMessage);
void  FrErrorSetHandler(void  (*handler)(int, char *));
/*--------------------------------------------------------------------------*/
unsigned int FrChkSum(FRSCHAR *buf, int nBytes, int start);
void FrCksumGnu (char *buf, FRLONG nBytes, unsigned int *crc);
/*---------------------------------------------------------------------------*/
FrSH *FrDicAddS(char *type, void (*funRead)());         
int   FrDicAssignId(struct FrFile *file, unsigned short type, void *address);
int   FrDicGetId   (struct FrFile *file, unsigned short type, void *address);
void  FrDicIni     (struct FrFile *file);
/*---------------------------------------------------------------------------*/

char  *FrStrCpy(char **copy, char *old);
char  *FrStrGTime(unsigned int in);
int    FrStrcmpAndPrefix(char *name1, char* name2);
char*  FrStrFlipPrefix(char *name);
int    FrStrSetPrefix(char **name, char *prefix);
char  *FrStrUTC(unsigned int gps, int uLeapS);
time_t FrGPS2UTC(unsigned int gps, int uLeapS);

/*---------------------------------------------------------------------------*/

/*------------------ here we have the I/O package. ------------------------- */
   
void FrPutNewRecord(FrFile *oFile, void    *instance, FRBOOL withInstanceId);
void FrPutChar  (FrFile *oFile, char           value); 
void FrPutDouble(FrFile *oFile, double         value);
void FrPutFloat (FrFile *oFile, float          value);
void FrPutInt   (FrFile *oFile, int            value);
void FrPutIntU  (FrFile *oFile, unsigned int   value);
void FrPutLong  (FrFile *oFile, FRLONG         value);
void FrPutShort (FrFile *oFile, short           value);
void FrPutShortU(FrFile *oFile, unsigned short value);
void FrPutSChar (FrFile *oFile, char          *value);
void FrPutStruct(FrFile *oFile, void       *instance);

void FrPutVC(FrFile *oFile, char      *data, int nData);
void FrPutVD(FrFile *oFile, double    *data, int nData);
void FrPutVF(FrFile *oFile, float     *data, int nData);
void FrPutVFD(FrFile *oFile,double    *data, int nData);
void FrPutVI(FrFile *oFile, int       *data, int nData);
void FrPutVL(FrFile *oFile, FRLONG    *data, int nData);
void FrPutVQ(FrFile *oFile, char     **data, int nData);
void FrPutVS(FrFile *oFile, short     *data, int nData);

  void FrPutWriteRecord(FrFile *oFile, FRBOOL endOfFile);

/*---------------------------------------------------------------------------*/

void FrReadHeader(FrFile *iFile, void *instance);

FRLONG FrRead    (FrFile *iFile, char *buf, FRULONG n);
void FrReadDouble(FrFile *iFile,       double *value);
void FrReadFloat (FrFile *iFile,        float *value);
void FrReadInt   (FrFile *iFile,          int *value);
void FrReadIntU  (FrFile *iFile, unsigned int *value);
void FrReadLong  (FrFile *iFile,       FRLONG *value);
void FrReadRecord(FrFile *iFile);
void FrReadSChar (FrFile *iFile,  char **value);
void FrReadShort (FrFile *iFile,  short *value);
void FrReadShortU(FrFile *iFile,  unsigned short *value);
void FrReadSkipRecord(FrFile *iFile);
void FrReadStruct(FrFile *iFile, void *add);
void FrReadStructHeader(FrFile *iFile);
void FrReadStructChksum(FrFile *iFile);
void FrReadVC    (FrFile *iFile, char      **data, FRULONG nData);
void FrReadVD    (FrFile *iFile, double    **data, FRULONG nData);
void FrReadVF    (FrFile *iFile, float     **data, FRULONG nData);
void FrReadVFD   (FrFile *iFile, double    **data, FRULONG nData);
void FrReadVI    (FrFile *iFile, int       **data, FRULONG nData);
void FrReadVL    (FrFile *iFile, FRLONG    **data, FRULONG nData);
void FrReadVQ    (FrFile *iFile, char     ***data, FRULONG nData);
void FrReadVS    (FrFile *iFile, short     **data, FRULONG nData);

/*---------------------------------------------------------------------------*/
FrDetector *FrBack3DetectorRead(FrFile *iFile);
FrameH     *FrBack3FrameHRead  (FrFile *iFile);
FrProcData *FrBack3ProcDataRead(FrFile *iFile);
FrRawData  *FrBack3RawDataRead (FrFile *iFile);
FrSimData  *FrBack3SimDataRead (FrFile *iFile);
FrStatData *FrBack3StatDataRead(FrFile *iFile);
FrSummary  *FrBack3SummaryRead (FrFile *iFile);
FrEvent    *FrBack3EventRead   (FrFile *iFile);
FrVect     *FrBack3VectRead    (FrFile *iFile);

FrDetector   *FrBack4DetectorRead  (FrFile *iFile);
void          FrBack4EndOfFrameRead(FrFile *iFile);
FrEvent      *FrBack4EventRead     (FrFile *iFile);
FrMsg        *FrBack4MsgRead       (FrFile *iFile);
FrProcData   *FrBack4ProcDataRead  (FrFile *iFile);
FrSimData    *FrBack4SimDataRead   (FrFile *iFile);
FrSimEvent   *FrBack4SimEventRead  (FrFile *iFile);
FrSummary    *FrBack4SummaryRead   (FrFile *iFile);
FrTOCevt     *FrBack4TOCevtRead    (FrFile *iFile);

FrProcData  *FrBack5ProcDataRead(FrFile *iFile);

#include "FrFilter.h"

#ifdef __cplusplus
}
#endif

#endif
















