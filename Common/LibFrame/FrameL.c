
#define _POSIX_SOURCE 
/*---------------------------------------------------------------------------*/
/* File: FrameL.c                                                            */
/*                                                                           */
/* Copyright (C) 2003, B. Mours.                                             */
/* For the licensing terms see the LICENSE file.                             */
/* For the list of contributors see the history section of the documentation */
/*---------------------------------------------------------------------------*/
char FrVersion[] =                 "8.21 (Apr 24, 14)";

#include <math.h>
#include <stdarg.h>
#include <ctype.h>
#include <errno.h>
#include "FrameL.h"
#include <sys/time.h>

/*------------------------------------ buffer space used in several places --*/
#define FRBUFSIZE 100000
char FrBuf[FRBUFSIZE];     
int FrBufSize = FRBUFSIZE;
char FrErrMsg[1024];
#define FRMAXTAG 1000
/*--------------------------------------------------miscellaneous variables--*/
int    FrSshort, FrSint, FrSlong, FrSfloat, FrSdouble;
FILE  *FrFOut = NULL;
int    FrDebugLvl = 0;
char  *FrErrorMsg[] = {"OK",
              "No Frame Error",
              "No file Error",
              "malloc failed error",
              "Buffer overflow error",
              "Write Error",
              "Read Error",
              "FRSET Error",
              "Wrong character length",
              "Bad end of frame Error",
              "File Open Error",
	      "Cheksum Error",
              "Too long string"};
char *FrFlipPrefix = NULL;      /*------------- until v8r18 it was: "V1:";---*/
int FrFormatVersion = 8;
/*---------------------------------------Header for each class of structure--*/
FrSH *FrSHRoot[FRMAPMAXSH];
int FrnSH = 0;
/*-----------------------------static variable used by the FrError functions-*/
void (*FrErrorHandler)() = FrErrorDefHandler;
/*----------------------------------------------------- Define for the zlib--*/
int Frz_compress   (unsigned char *dest, unsigned long *destLen,
	                   char *source, unsigned long sourceLen, int level);
int Frz_uncompress (unsigned char *dest, unsigned long *destLen,
		           char *source, unsigned long sourceLen);
/*---------------------------------define to use FFTW malloc/free functions--*/
#ifdef FFTW_MALLOC
void *fftw_free  (void *p);
void *fftw_malloc(size_t n);
void *FrCalloc(size_t nobj, size_t size)
{void *mem;
 mem = fftw_malloc(nobj*size);
 if(mem == NULL) return(NULL);
 memset(mem,0,nobj*size);
 return(mem);};

#define malloc fftw_malloc
#define calloc FrCalloc
#define free   fftw_free
#endif
/*---------------------------------------------------------------------------*/


/*---------------------------------------------------------------- FrLibIni -*/
FILE *FrLibIni(char *outFile, 
               FILE *fOut, 
               int dbglvl)
                 /*if not outFile == NULL, we use fOut for output file*/
/*---------------------------------------------------------------------------*/
{static int icall = 0;
 
  icall++;
  if(icall > 1) 
    {printf(" Warning, FrLibIni is call twice\n");
     return(FrFOut);}

  if(outFile == NULL)
    {FrFOut = fOut;
     if(FrFOut == NULL)  {FrFOut = stdout;}}
  else
    {printf("FrLibIni: output will be sent on file:%s\n",outFile);
     FrFOut = fopen(outFile,"w");
     if(FrFOut == NULL)
         {printf("\n !! Opening file error, output stay on screen\n");
          FrFOut = stdout;}}
  
  FrDebugLvl = dbglvl;

  FrLibVersion(FrFOut);

  FrLibShortIni();

  return(FrFOut);}

/*----------------------------------------------------------- FrLibShortIni -*/
void FrLibShortIni()
/*---------------------------------------------------------------------------*/
{static int icall = 0;
 
  icall++;
  if(icall > 1) return;
                                    /*----- i/o package initialization ------*/
	
  FrSshort  = sizeof(short);
  FrSint    = sizeof(int);
  FrSlong   = sizeof(FRLONG);
  FrSfloat  = sizeof(float);
  FrSdouble = sizeof(double);

    /*--- check the size of words. Unpredictible behevior will results
              from the use of non standard values ---------------------------*/

  if(FrSshort != 2)
     {FrError(3,"FrLibShortIni","Warning: sizeof(short) != 2 !!!!!");}
  if(FrSint != 4)
     {FrError(3,"FrLibShortIni","Warning: sizeof(int) != 4 !!!!!!!");}
  if(FrSlong != 4 && FrSlong != 8 )
     {FrError(3,"FrLibShortIni","Warning: sizeof(long) != 4 or 8 !");}
  if(FrSfloat != 4)
     {FrError(3,"FrLibShortIni","Warning: sizeof(float) != 4 !!!!!");}
  if(FrSdouble != 8)
     {FrError(3,"FrLibShortIni","Warning: sizeof(double) != 8 !!!!");}
  if(sizeof(long long) != 8)
     {FrError(3,"FrLibShortIni","Warning: sizeof(long long) != 8 !!!!!");}

                              /*----- declare the standard structures -------*/

  if(FrObjectDef()     == NULL) return;
  if(FrSHDef()         == NULL) return;
  if(FrSEDef()         == NULL) return;

  if(FrameHDef()       == NULL) return;
  if(FrAdcDataDef()    == NULL) return;
  if(FrDetectorDef()   == NULL) return;
  if(FrEndOfFrameDef() == NULL) return;
  if(FrEventDef()      == NULL) return;
  if(FrMsgDef()        == NULL) return;
  if(FrHistoryDef()    == NULL) return;
  if(FrRawDataDef()    == NULL) return;
  if(FrProcDataDef()   == NULL) return;
  if(FrSimDataDef()    == NULL) return;
  if(FrSimEventDef()   == NULL) return;
  if(FrSerDataDef()    == NULL) return;
  if(FrStatDataDef()   == NULL) return;
  if(FrSummaryDef()    == NULL) return;
  if(FrTableDef()      == NULL) return;
  if(FrTOCDef()        == NULL) return;

  if(FrVectDef()       == NULL) return;
  
  if(FrEndOfFileDef()  == NULL) return;
 
  return;}

/*------------------------------------------------------------- FrLibSetLvl -*/
void FrLibSetLvl(int dbglvl)
/*---------------------------------------------------------------------------*/
{ 
  FrDebugLvl = dbglvl;
  if(FrFOut == NULL) FrFOut = stdout;
  
  return;}

/*-------------------------------------------------------FrLibSetFlipPrefix--*/
void FrLibSetFlipPrefix(char *prefix)
/*---------------------------------------------------------------------------*/
{ 
  FrStrCpy(&FrFlipPrefix, prefix);
  
  return;}

/*----------------------------------------------------------FrLibSetVersion--*/
void FrLibSetVersion(int formatVersion)
/*---------------------------------------------------------------------------*/
{ 
  FrFormatVersion = formatVersion;
  
  return;}

/*------------------------------------------------------------ FrLibVersion -*/
float FrLibVersion(FILE *fOut)
                              /*if fOut != NULL, we use print version on fOut*/
/*---------------------------------------------------------------------------*/
{float version;

 sscanf(FrVersion,"%f",&version);

 if(fOut != NULL) fprintf(fOut,"  FrameL Version:%s(Compiled: %s %s)\n",
	                       FrVersion,__DATE__,__TIME__);
  
 return(version);}

/*----------------------------------------------------------- FrLibVersionF -*/
char *FrLibVersionF()
/*---------------------------------------------------------------------------*/
{static char version[256];

 sprintf(version,"Fr     Version:%s(Compiled: %s %s)\n",
                               FrVersion,__DATE__,__TIME__);

 return(version);}


/*--------------------------------------------------------FrAdcDataCopy------*/
FrAdcData *FrAdcDataCopy(FrAdcData *in,
                         FrameH *frame)
/*---------------------------------------------------------------------------*/
{FrAdcData *out;

  if(in == NULL) return(NULL); 
  out = (FrAdcData *) calloc(1,sizeof(FrAdcData));
  if(out == NULL) {return(NULL);} 

  memcpy(out, in, sizeof(FrAdcData));
	 
  if(FrStrCpy(&out->name,   in->name)    == NULL) return(NULL);
  if(in->comment != NULL)
     {if(FrStrCpy(&out->comment,in->comment) == NULL) return(NULL);}
  if(in->units != NULL)
     {if(FrStrCpy(&out->units,  in->units)   == NULL) return(NULL);}
 
  if(in->data != NULL) 
     {out->data = FrVectCopy(in->data);
      if(out->data == NULL) return(NULL);}
  if(in->aux != NULL)
     {out->aux  = FrVectCopy(in->aux);
      if(out->aux == NULL) return(NULL);}

  out->next = NULL;
                           /*----- now store it in the Frame structures -----*/

  if(frame != NULL) FrameAddAdc(frame, out);
  
  return(out);} 

/*------------------------------------------------------- FrAdcDataDecimate--*/
int FrAdcDataDecimate(FrAdcData *adc,
                      int nGroup)
/*---------------------------------------------------------------------------*/
{int i, j, sum, group;
 float  *dataF;
 double *dataD, sumD, coef;
 short  *dataS;
 int    *dataI;

 if(adc == NULL) return(1);
 if(nGroup == 0) return(2);

 if(adc->data->compress != 0) FrVectExpand(adc->data);

              /*------ this version works only for short, int and float -----*/

 if((adc->data->type != FR_VECT_2S) &&
    (adc->data->type != FR_VECT_4S) &&
    (adc->data->type != FR_VECT_4R) &&
    (adc->data->type != FR_VECT_8R)) return(4);

 if(nGroup < 4)         {group = 0;}
 else if(nGroup < 16)   {group = 1;}
 else if(nGroup < 64)   {group = 2;}
 else if(nGroup < 256)  {group = 3;}
 else if(nGroup < 1024) {group = 4;}
 else                   {group = 5;}
 if (adc->data->type == FR_VECT_4R) group = 0;

              /*---- if ngroup is negative, we do not change the resolution--*/

 if(nGroup < 0) nGroup = -nGroup;
 if(adc->data->nData % nGroup != 0) return(3);

 coef = (1.+group)/nGroup;

 adc->slope       = adc->slope * (1+group);
 adc->nBits       = adc->nBits + group;
 adc->sampleRate  = adc->sampleRate  / nGroup;
 adc->data->nData = adc->data->nData / nGroup;
 adc->data->nx[0] = adc->data->nx[0] / nGroup;
 adc->data->dx[0] = adc->data->dx[0] * nGroup;
 adc->data->nBytes= adc->data->nBytes /nGroup;

 if(adc->data->type == FR_VECT_2S)
   {if(adc->nBits > 16) adc->data->type = FR_VECT_4S;

    dataS = adc->data->dataS;
    for(i=0; i < adc->data->nData; i++)
       {sum = 0.;
        for(j=0; j<nGroup; j++) {sum += dataS[j];}
        if(adc->nBits < 17)
             {adc->data->dataS[i] = sum*coef;}
        else {adc->data->dataI[i] = sum*coef;}
        dataS = dataS + nGroup;}}

 else if(adc->data->type == FR_VECT_4S)
   {dataI = adc->data->dataI;
    for(i=0; i < adc->data->nData; i++)
       {sum = 0.;
        for(j=0; j<nGroup; j++) {sum += dataI[j];}
        adc->data->dataI[i] = sum*coef;
        dataI = dataI + nGroup;}}
    
 else if(adc->data->type == FR_VECT_4R)
   {dataF = adc->data->dataF;
    for(i=0; i < adc->data->nData; i++)
       {sumD = 0.;
        for(j=0; j<nGroup; j++) {sumD += dataF[j];}
        adc->data->dataF[i] = sumD*coef;
        dataF = dataF + nGroup;}}
    
 else if(adc->data->type == FR_VECT_8R)
   {dataD = adc->data->dataD;
    for(i=0; i < adc->data->nData; i++)
       {sumD = 0.;
        for(j=0; j<nGroup; j++) {sumD += dataD[j];}
        adc->data->dataD[i] = sumD*coef;
        dataD = dataD + nGroup;}}

 adc->data->data = realloc(adc->data->data, adc->data->nBytes);
 FrVectMap(adc->data);
    
 return(0);}

/*------------------------------------------------------------ FrAdcDataDef--*/
FrSH *FrAdcDataDef()
/*---------------------------------------------------------------------------*/
{static FrSH *classe = NULL;

  if(classe != NULL) return(classe);
  classe = FrDicAddS("FrAdcData", (void (*)())FrAdcDataRead);

  FrSENew(classe, "STRING", "name","-");
  FrSENew(classe, "STRING", "comment","-");
  FrSENew(classe, "INT_4U", "channelGroup","-");
  FrSENew(classe, "INT_4U", "channelNumber","-");
  FrSENew(classe, "INT_4U", "nBits","-");
  FrSENew(classe, "REAL_4", "bias","-");
  FrSENew(classe, "REAL_4", "slope","-");
  FrSENew(classe, "STRING", "units","-");
  FrSENew(classe, "REAL_8", "sampleRate","-");
  FrSENew(classe, "REAL_8", "timeOffset","-");
  FrSENew(classe, "REAL_8", "fShift","-");
  FrSENew(classe, "REAL_4", "phase","-");
  FrSENew(classe, "INT_2U", "dataValid","-");
  FrSENew(classe, "PTR_STRUCT(FrVect *)",   "data","-");
  FrSENew(classe, "PTR_STRUCT(FrVect *)",   "aux","-");
  FrSENew(classe, "PTR_STRUCT(FrAdcData *)", "next","-"); 
  if(FrFormatVersion >= 8) FrSENew(classe, "INT_4U", "chkSum","-");

  return(classe);}
  
/*----------------------------------------------------------- FrAdcDataDump--*/
void FrAdcDataDump(FrAdcData *adc, 
                   FILE *fp, 
                   int debugLvl)
/*---------------------------------------------------------------------------*/
{
 if(adc == NULL) return;

 fprintf(fp," ADC:    %s channel(G/N):%d-%d nBits=%d bias=%g slope=%g", 
     adc->name, adc->channelGroup, adc->channelNumber, 
     adc->nBits, adc->bias, adc->slope);
 if(adc->units != NULL) fprintf(fp," units=%s",adc->units);
 if(adc->fShift != 0 || adc->phase != 0)   
   fprintf(fp," fShift=%g phase=%g",adc->fShift, adc->phase);
 fprintf(fp,"\n");

 if(adc->dataValid != 0)
  {fprintf(fp,"   DataValid=%x (",adc->dataValid);
   if((adc->dataValid & 1) != 0)fprintf(fp,"Non valid floating point;");
   if((adc->dataValid & 2) != 0)fprintf(fp,"Missing data at known position;");
   if((adc->dataValid & 4) != 0)fprintf(fp,"Missing data at unknown position");
   if((adc->dataValid & 8) != 0)fprintf(fp,"Hardware parity error;");
   if((adc->dataValid & 16)!= 0)fprintf(fp,"Too slow DAQ (like FIFO full)");
   if((adc->dataValid & 32)!= 0)fprintf(fp,"Invalid front end format");
   printf(")\n");}

 if(adc->comment != NULL) 
   {if(strlen(adc->comment) > 2) fprintf(fp,"   comment:%s\n", adc->comment);}
    
 FrVectDump( adc->data,  fp, debugLvl);

 if(adc->aux != NULL) 
   {fprintf(fp,"  Auxiliary");
    FrVectDump(adc->aux, fp, debugLvl);}

 return;}

/*------------------------------------------------------------FrAdcDataFind--*/
FrAdcData *FrAdcDataFind(FrameH *frame, 
                         char *adcName)
/*---------------------------------------------------------------------------*/
/* Return a pointer to the adc structure or NULL if not found                */
{FrAdcData *adc;

 if(frame == NULL)             return (NULL);
 if(frame->rawData == NULL)    return (NULL);
 if(adcName == NULL)           return (NULL);

 adc = (FrAdcData*) FrameFindBasic((FrBasic*) frame->rawData->firstAdc, adcName);
 if(adc == NULL) return(NULL);

 if(adc->data->compress != 0) FrVectExpandF(adc->data);
 if(adc->aux != NULL)         FrVectExpandF(adc->aux);

 adc->data->GTime     = frame->GTimeS +
                        frame->GTimeN * 1.e-9 + adc->timeOffset;
 adc->data->ULeapS    = frame->ULeapS;
 adc->data->localTime = FrameGetLTOffset(frame,adcName);
 
 return (adc);}

/*------------------------------------------------------------FrAdcDataFree--*/
void FrAdcDataFree(FrAdcData *adcData)
/*---------------------------------------------------------------------------*/
{
 while(adcData != NULL) {adcData = FrAdcDataFreeOne(adcData);}

 return;}

/*---------------------------------------------------------FrAdcDataFreeOne--*/
FrAdcData* FrAdcDataFreeOne(FrAdcData *adcData)
/*---------------------------------------------------------------------------*/
{
 FrAdcData* next;

 if(adcData == NULL) return(NULL);
 next = adcData->next;

 if(adcData->name    != NULL) free (adcData->name);
 if(adcData->comment != NULL) free (adcData->comment);
 if(adcData->units   != NULL) free (adcData->units);
 if(adcData->data    != NULL) FrVectFree (adcData->data);
 if(adcData->aux     != NULL) FrVectFree (adcData->aux);

 free(adcData);

 return(next);}

/*---------------------------------------------------------FrAdcDataGetSize--*/
FRLONG FrAdcDataGetSize(FrAdcData *adc)
/*---------------------------------------------------------------------------*/
{FRLONG size;

 if(adc == NULL) return(0);

 size = 69;
 if(adc->name    != NULL) size += strlen(adc->name);
 if(adc->comment != NULL) size += strlen(adc->comment);
 if(adc->units   != NULL) size += strlen(adc->units);
 size += FrVectGetTotSize(adc->data);
 size += FrVectGetTotSize(adc->aux);

 return(size);}

/*--------------------------------WARNING:OBSOLETE FUNCTION---FrAdcDataGetV--*/
FrVect *FrAdcDataGetV(FrameH *frame, char *name)
/*---------------------------------------------------------------------------*/
{return(FrameFindAdcVect(frame,name));}

/*-------------------------------------------------------------FrAdcDataNew--*/
FrAdcData *FrAdcDataNew(FrameH *frame,  
                        char *name, 
                        double sampleRate, 
                        FRLONG nData, 
                        int nBits)
/*---------------------------------------------------------------------------*/
{FrAdcData *adcData;

 adcData = FrAdcDataNewF(frame, name, NULL, 0, 0,
                          nBits, 0, 1., NULL, sampleRate, nData); 
  
 return(adcData);} 

/*------------------------------------------------------------FrAdcDataNewF--*/
FrAdcData *FrAdcDataNewF(FrameH *frame,  
                         char *name,
                         char *comment,
                         unsigned int channelGroup,
                         unsigned int channelNumber, 
                         int nBits,
                         float bias,
                         float slope,
                         char *units,
                         double sampleRate, 
                         FRLONG nData)
/*---------------------------------------------------------------------------*/
{FrAdcData *adcData;
 double dx;
 int type;
 char *aUnits = "Counts.";
 
  adcData = (FrAdcData *) calloc(1,sizeof(FrAdcData));
  if(adcData == NULL)   return(NULL);
  adcData->classe = FrAdcDataDef();
	 
  if(units == NULL) units = aUnits;
  if(FrStrCpy(&adcData->name,name)       == NULL) return(NULL);
  if(FrStrCpy(&adcData->comment,comment) == NULL) return(NULL);
  if(FrStrCpy(&adcData->units,units)     == NULL) return(NULL);

  adcData->channelGroup  = channelGroup;
  adcData->channelNumber = channelNumber;
  if(nBits > 0) adcData->nBits =   nBits;
  else          adcData->nBits =  -nBits;
  adcData->bias  = bias;
  adcData->slope = slope;
  adcData->sampleRate = sampleRate;
  adcData->timeOffset = 0;
  adcData->fShift     = 0;
  adcData->phase      = 0;
  adcData->dataValid  = 0;
  adcData->aux        = NULL;

  if(nBits >16)       {type = FR_VECT_4S;}
  else if(nBits >  8) {type = FR_VECT_2S;}
  else if(nBits >  0) {type = FR_VECT_C;}
  else if(nBits >-33) {type = FR_VECT_4R;}
  else                {type = FR_VECT_8R;}

  if(sampleRate == 0.)
       {dx = 0.;}
  else {dx = 1./sampleRate;}

  adcData->data = FrVectNew1D(name, -type, nData,dx, "Time (sec)", units);
  if(adcData->data == NULL) return(NULL);
  
  if(frame != NULL) FrameAddAdc(frame, adcData);
  
  return(adcData);} 

/*------------------------------------------------------------FrAdcDataRead--*/
FrAdcData *FrAdcDataRead(FrFile *iFile)
/*---------------------------------------------------------------------------*/
{FrAdcData *adcData;
 unsigned int timeOffsetS, timeOffsetN;
 char message[128];

  adcData = (FrAdcData *) calloc(1,sizeof(FrAdcData));
  if(adcData == NULL) 
       {iFile->error = FR_ERROR_MALLOC_FAILED;
        return(NULL);}
  adcData->classe = FrAdcDataDef();

  FrReadHeader(iFile,  adcData);
  FrReadSChar (iFile, &adcData->name); 
  FrReadSChar (iFile, &adcData->comment);
  FrReadIntU  (iFile, &adcData->channelGroup);
  FrReadIntU  (iFile, &adcData->channelNumber);
  FrReadIntU  (iFile, &adcData->nBits);
  FrReadFloat (iFile, &adcData->bias);
  FrReadFloat (iFile, &adcData->slope);
  FrReadSChar (iFile, &adcData->units);
  FrReadDouble(iFile, &adcData->sampleRate);
  if(iFile->fmtVersion > 5)
       {FrReadDouble(iFile, &adcData->timeOffset);}
  else {FrReadIntU  (iFile, &timeOffsetS);
        FrReadIntU  (iFile, &timeOffsetN);
        adcData->timeOffset = timeOffsetS + 1.e-9*timeOffsetN;}
  FrReadDouble(iFile, &adcData->fShift);
  if(iFile->fmtVersion > 4)
       {FrReadFloat (iFile, &adcData->phase);}
  FrReadShortU(iFile, &adcData->dataValid);
  FrReadStruct(iFile, &adcData->data);
  iFile->vectInstance = iFile->instance;
  FrReadStruct(iFile, &adcData->aux);
  FrReadStruct(iFile, &adcData->next);
  if(iFile->fmtVersion >=8) FrReadStructChksum(iFile);

  if(iFile->nBytes != iFile->nBytesR + iFile->length)
     {sprintf(message,": Record length error: "
         "nBytes=%"FRLLD" nBytesR=%"FRLLD" length=%"FRLLD"\n",
          iFile->nBytes, iFile->nBytesR, iFile->length);
      FrError(3,"FrAdcDataRead",message);
      return(NULL);}     

  if(FrDebugLvl > 2) fprintf(FrFOut," %s\n", adcData->name);
  
return(adcData);}

/*---------------------------------------------------------- FrAdcDataReadI--*/
FrAdcData *FrAdcDataReadI(FrFile *iFile,
                          char *name,
                          int index)
/*---------------------------------------------------------------------------*/
{FrAdcData *adc;
 FrTOCts *ts;
 FrTOC *toc;
 double gtime;

 toc = FrTOCReadFull(iFile);
 if(toc == NULL)          return(NULL);
 if(index == -1)          return(NULL);
 if(index >= toc->nFrame) return(NULL);

                                /*------ find TOC info for this ADC ---------*/

 for(ts = toc->adc; ts != NULL; ts = ts->next)
   {if(strcmp(name, ts->name) == 0)  break;}
 if(ts == NULL) return(NULL); 

                        /*---- set the file pointer and read the data -------*/

 if(FrTOCSetPos(iFile, ts->position[index]) != 0) {return (NULL);}
 adc = FrAdcDataRead(iFile);
 if(adc == NULL) return(NULL);

 gtime = toc->GTimeS[index] + 1.e-9 *toc->GTimeN[index] + adc->timeOffset;
 adc->data = FrVectReadNext(iFile, gtime, adc->name);

 return(adc);}

/*---------------------------------------------------------- FrAdcDataReadT--*/
FrAdcData *FrAdcDataReadT(FrFile *iFile,
                          char *name,
                          double gtime)
/*---------------------------------------------------------------------------*/
{FrAdcData  **current, *adc, *root;
 FrTOCts *ts;
 FrTag *frtag;
 int index;
          /*------- find frame(it will read the FrTOC if needed) ------------*/

 index = FrTOCFrameFindT(iFile, gtime);
 if(index <0) return(NULL);

 if(name == NULL) return(NULL);              /*----- build tag object -------*/
 frtag = FrTagNew(name);
 if(frtag == NULL) return(NULL);
                                   
 root = NULL;                           /*------ find the ADC in the TOC ----*/
 current = &root;

 for(ts = iFile->toc->adc; ts != NULL; ts = ts->next)

                          /*-------- check if we need to copy this adc ?-----*/

   {if(FrTagMatch(frtag, ts->name) == FR_NO) continue; 

                       /*---- set the file pointer and read the data -------*/

    if(FrTOCSetPos(iFile, ts->position[index]) != 0) continue;
    adc = FrAdcDataRead(iFile);
    if(adc == NULL) continue;
                                 /*--- check that we read the right ADC-----*/

    if(FrTagMatch(frtag, adc->name) == FR_NO) 
        {FrError(3,"FrAdcDataReadT","name missmatch");
         return(NULL);}

    gtime = iFile->toc->GTimeS[index] + 1.e-9 *
            iFile->toc->GTimeN[index] + adc->timeOffset;
    adc->data = FrVectReadNext(iFile, gtime, adc->name);

    *current = adc;
    current = &(adc->next);}

 FrTagFree(frtag); 

 return(root);}

/*--------------------------------------------------------- FrAdcDataSetAux--*/
void FrAdcDataSetAux(FrAdcData *adc, 
                     FrVect *aux)
/*---------------------------------------------------------------------------*/
{
 if(adc == NULL) return;
 if(aux == NULL) return;

 aux->next = adc->aux;
 adc->aux = aux;

 return;}

/*--------------------------------------------------- FrAdcDataSetDataValid--*/
void FrAdcDataSetDataValid(FrAdcData *adc, 
                           unsigned short dataValid)
/*---------------------------------------------------------------------------*/
{
 if(adc != NULL) adc->dataValid = dataValid;

 return;}

/*----------------------------------------------------- FrAdcDataSetTOffset--*/
void FrAdcDataSetFShift(FrAdcData *adc, 
                        double fShift,
                        float phase)
/*---------------------------------------------------------------------------*/
{
 if(adc == NULL) return;

 adc->fShift = fShift;
 adc->phase  = phase;

 return;}

/*----------------------------------------------------- FrAdcDataSetTOffset--*/
void FrAdcDataSetTOffset(FrAdcData *adc, 
                         double timeOffset)
/*---------------------------------------------------------------------------*/
{
 if(adc != NULL) adc->timeOffset = timeOffset;

 return;}

/*-----------------------------------------------------------FrAdcDataWrite--*/
void FrAdcDataWrite(FrAdcData *adcData,
                    FrFile *oFile)
/*---------------------------------------------------------------------------*/
{
  FrPutNewRecord(oFile, adcData, FR_YES);
  FrPutSChar (oFile, adcData->name);
  FrPutSChar (oFile, adcData->comment);
  FrPutIntU  (oFile, adcData->channelGroup);
  FrPutIntU  (oFile, adcData->channelNumber);
  FrPutIntU  (oFile, adcData->nBits);
  FrPutFloat (oFile, adcData->bias);
  FrPutFloat (oFile, adcData->slope);
  FrPutSChar (oFile, adcData->units);
  FrPutDouble(oFile, adcData->sampleRate);
  FrPutDouble(oFile, adcData->timeOffset);
  FrPutDouble(oFile, adcData->fShift);
  FrPutFloat (oFile, adcData->phase);
  FrPutShortU(oFile, adcData->dataValid);
  FrPutStruct(oFile, adcData->data);
  FrPutStruct(oFile, adcData->aux);
  FrPutStruct(oFile, adcData->next);

  if(oFile->toc != NULL) FrTOCtsMark(oFile, &oFile->toc->adcH, 
          adcData->name, adcData->channelGroup, adcData->channelNumber);

  FrPutWriteRecord(oFile, FR_NO);

  FrVectWrite(adcData->data, oFile);

  if(adcData->aux != NULL) FrVectWrite(adcData->aux, oFile);
   
 return;}

/*--------------------------------------------------------------FrameAddAdc--*/
void FrameAddAdc(FrameH *frame,  FrAdcData* adc)
/*---------------------------------------------------------------------------*/
{
  FrRawData *rawData;
 
  if(frame == NULL) return;
  if(adc   == NULL) return;

  if(frame->rawData == NULL)  FrRawDataNew(frame);
  rawData = frame->rawData;
  if(rawData == NULL) return;

  adc->next = rawData->firstAdc;
  rawData->firstAdc = adc;
 
  if(rawData->firstAdcOld != NULL) {    /*---------- if tagging is used---*/
    adc->nextOld = rawData->firstAdcOld;
    rawData->firstAdcOld = adc;}
  
  return;
} 
/*------------------------------------------------------------FrameAddEvent--*/
void FrameAddEvent(FrameH *frame,  FrEvent* event)
/*---------------------------------------------------------------------------*/
{
  FrEvent **last;

  if(frame == NULL) return;
  if(event == NULL) return;

  /*-------------------------------add event at the end of the linked list---*/
  last = &frame->event;
  while((*last) != NULL) {last = &((*last)->next);}
  (*last) = event;

  if(frame->eventOld != NULL) {/*if tagging is used the order does not matter*/
    event->nextOld = frame->eventOld;
    frame->eventOld = event;}

  return;
} 
/*-------------------------------------------------------------FrameAddProc--*/
void FrameAddProc(FrameH *frame,  FrProcData* proc)
/*---------------------------------------------------------------------------*/
{
  if(frame == NULL) return;
  if(proc  == NULL) return;

  proc->next = frame->procData;
  frame->procData = proc;
 
  if(frame->procDataOld != NULL) {    /*---------- if tagging is used---*/
    proc->nextOld = frame->procDataOld;
    frame->procDataOld = proc;}
  
  return;
} 
/*--------------------------------------------------------------FrameAddSer--*/
void FrameAddSer(FrameH *frame,  FrSerData* ser)
/*---------------------------------------------------------------------------*/
{
  FrRawData *rawData;
 
  if(frame == NULL) return;
  if(ser   == NULL) return;

  if(frame->rawData == NULL)  FrRawDataNew(frame);
  rawData = frame->rawData;
  if(rawData == NULL) return;

  ser->next = rawData->firstSer;
  rawData->firstSer = ser;
 
  if(rawData->firstSerOld != NULL) {    /*---------- if tagging is used---*/
    ser->nextOld = rawData->firstSerOld;
    rawData->firstSerOld = ser;}
  
  return;
} 
/*----------------------------------------------------------FrameAddSimData--*/
void FrameAddSimData(FrameH *frame,  FrSimData* sim)
/*---------------------------------------------------------------------------*/
{
  if(frame == NULL) return;
  if(sim   == NULL) return;

  sim->next = frame->simData;
  frame->simData = sim;
 
  if(frame->simDataOld != NULL) {    /*---------- if tagging is used---*/
    sim->nextOld = frame->simDataOld;
    frame->simDataOld = sim;}
  
  return;
} 
/*---------------------------------------------------------FrameAddSimEvent--*/
void FrameAddSimEvent(FrameH *frame,  FrSimEvent* event)
/*---------------------------------------------------------------------------*/
{
  if(frame == NULL) return;
  if(event == NULL) return;

  event->next = frame->simEvent;
  frame->simEvent = event;
 
  if(frame->simEventOld != NULL) {    /*---------- if tagging is used---*/
    event->nextOld = frame->simEventOld;
    frame->simEventOld = event;}
  
  return;
} 
/*---------------------------------------------------------------------------*/
int FrameAddStatData(FrameH* frame,
                      char* detectorName,
                      FrStatData *stat)
/*---------------------------------------------------------------------------*/
/* Attached a static data to a detector structure belonging to this frame.   */
/* If no detector exist for this name, a new one is created.                 */
/* If name is NULL, it is attached to the first detector or to a new detector*/
/* called "Default" is there is no detector structure.                       */
/* Any new detector structure is attached to the frame->detectProc list.     */
/*---------------------------------------------------------------------------*/
{FrDetector *detector;

 if(frame == NULL) return(1);
 if(stat  == NULL) return(2);

 if(detectorName == NULL) detector = frame->detectProc;
 else                     detector = FrameFindDetector(frame, detectorName);

 if(detector == NULL)
   {if(detectorName == NULL) detector = FrDetectorNew("Dummy");
    else                     detector = FrDetectorNew(detectorName);
    if(detector == NULL) return(3);
    detector->next = frame->detectProc;
    frame->detectProc = detector;}

 FrDetectorAddStatData(detector, stat);
 
 return(0);}
/*---------------------------------------------------------------------------*/
FrStatData* FrameAddStatVector(FrameH* frame,
                       char* detectorName,
                       char* statDataName,
		       unsigned int tStart,
		       unsigned int tEnd,
		       unsigned int version,
                       FrVect* vect)
/*---------------------------------------------------------------------------*/
/* Attached a vector to a static data to a detector structure belonging to this frame.   */
/* If no detector exist for this name, a new one is created.                 */
/* If name is NULL, it is attached to the first detector or to a new detector*/
/* called "Default" is there is no detector structure.                       */
/* Any new detector structure is attached to the frame->detectProc list.     */
/*---------------------------------------------------------------------------*/
{FrStatData *sData;

 if(frame == NULL) return(NULL);
 if(vect  == NULL) return(NULL);

 sData = FrameFindStatData(frame, detectorName, statDataName, (tStart+tEnd)/2);
 
 if(sData == NULL)
   {sData = FrStatDataNew(statDataName, NULL, NULL, 
                         tStart, tEnd, version, NULL, NULL); 
    FrameAddStatData(frame, detectorName, sData);}

 FrStatDataAddVect(sData, vect);

 return(sData);}
/*--------------------------------------------------------------FrameAddSum--*/
void FrameAddSum(FrameH *frame,  FrSummary* sum)
/*---------------------------------------------------------------------------*/
{
  if(frame == NULL) return;
  if(sum   == NULL) return;

  sum->next = frame->summaryData;
  frame->summaryData = sum;
 
  if(frame->summaryDataOld != NULL) {    /*---------- if tagging is used---*/
    sum->nextOld = frame->summaryDataOld;
    frame->summaryDataOld = sum;}
  
  return;
} 
/*------------------------------------------------------------FrameCompDump--*/
void FrameCompDump(FrameH *frame,
                   FILE *fp,
                   int debugLvl)
/*---------------------------------------------------------------------------*/
{int nAdc, nComp;
 double compFactor,nBytes, nBytesTot;
 FrAdcData *adc;

 if(frame   == NULL) return;
 if(fp      == NULL) return;
 if(debugLvl < 1)    return;

 nAdc      = 0;
 nComp     = 0;
 nBytes    = 0;
 nBytesTot = 0;

 if(frame->rawData != NULL)
  {for(adc = frame->rawData->firstAdc; adc != NULL; adc = adc->next) 
    {nAdc++;
     if(adc->data->compress != 0) nComp ++;
     nBytes    += adc->data->nBytes;
     nBytesTot += adc->data->nData*adc->data->wSize;}}

 if(nBytes != 0) compFactor = nBytesTot/nBytes;
 else            compFactor = -1.;

 fprintf(fp," %d Adc channels (%d compressed); %.0f uncompressed bytes; "
            "mean compression factor=%.2f;\n",
              nAdc, nComp, nBytesTot, compFactor);

 return;}

/*------------------------------------------------------------ FrameCompress-*/
void FrameCompress(FrameH *frameH,
                   int compress,
                   int gzipLevel)
/*---------------------------------------------------------------------------*/
{FrDetector *det;
 FrAdcData  *adc;
 FrProcData *proc;
 FrSimData  *sim;
 FrSimEvent *simEvt;
 FrSerData  *ser;
 FrEvent    *evt;
 FrSummary  *sum;

 if(frameH == NULL) return;

 FrVectCompress(frameH->type, compress, gzipLevel);
 FrVectCompress(frameH->user, compress, gzipLevel);
 FrVectCompress(frameH->auxData, compress, gzipLevel);
 FrTableCompress(frameH->auxTable, compress, gzipLevel);

 for(det = frameH->detectProc; det != NULL; det = det->next)
   {FrVectCompress(det->aux, compress, gzipLevel);
    FrTableCompress(det->table, compress, gzipLevel);}

 for(det = frameH->detectSim; det != NULL; det = det->next)
   {FrVectCompress(det->aux, compress, gzipLevel);
    FrTableCompress(det->table, compress, gzipLevel);}
                                /*-------------- Uncompress the raw data ----*/
     
 if(frameH->rawData != NULL) 
   {for(adc = frameH->rawData->firstAdc; adc != NULL; adc = adc->next)
       {FrVectCompress(adc->data, compress, gzipLevel);
        FrVectCompress(adc->aux, compress, gzipLevel);}
    for(ser = frameH->rawData->firstSer; ser != NULL; ser = ser->next)
       {FrVectCompress(ser->serial, compress, gzipLevel);
        FrTableCompress(ser->table, compress, gzipLevel);}
    FrTableCompress(frameH->rawData->firstTable, compress, gzipLevel);
    FrVectCompress(frameH->rawData->more, compress, gzipLevel);}

                                /*----------- Uncompress the other data ----*/

 for(proc = frameH->procData; proc != NULL; proc = proc->next)
       {FrVectCompress(proc->data, compress, gzipLevel);
        FrVectCompress(proc->aux, compress, gzipLevel);
        FrTableCompress(proc->table, compress, gzipLevel);}

 for(sim = frameH->simData; sim != NULL; sim = sim->next)
      {FrVectCompress(sim->data, compress, gzipLevel);
       FrVectCompress(sim->input, compress, gzipLevel);
       FrTableCompress(sim->table, compress, gzipLevel);}  

 for(simEvt = frameH->simEvent; simEvt != NULL; simEvt = simEvt->next)
   {FrVectCompress(simEvt->data, compress, gzipLevel);
    FrTableCompress(simEvt->table, compress, gzipLevel);}

 for(evt = frameH->event; evt != NULL; evt = evt->next)
      {FrVectCompress(evt->data, compress, gzipLevel);
       FrTableCompress(evt->table, compress, gzipLevel);}

 for(sum = frameH->summaryData; sum != NULL; sum = sum->next)
      {FrVectCompress(sum->moments, compress, gzipLevel);
       FrTableCompress(sum->table, compress, gzipLevel);}

 return;}

/*-------------------------------------------------------------- FrameCopy---*/
FrameH *FrameCopy(FrameH *frame)
/*---------------------------------------------------------------------------*/
{FrameH *out;
 int nBytes;
 static int size = 0;
 static char *buf;
 char comment[100];

 if(frame == NULL) return(NULL);
 
 if(size == 0)
     {size = 200000;
      buf = (char *) malloc(size);}

 nBytes = 0;

 while(nBytes == 0)
    {if(buf == NULL) return(NULL);
     nBytes = FrameWriteToBuf(frame, -1, buf, size);
     if(nBytes == 0)
         {size = size*2;
          free(buf);
          buf = malloc(size);}}

 out = FrameReadFromBuf(buf,size,-1);

            /*----- update the history record added by the frameWrite call --*/

 free(out->history->comment);
 sprintf(comment,"FrameCopy %s",FrVersion);
 FrStrCpy(&out->history->comment,comment);

 return(out);}
/*---------------------------------------------------------------------------*/
FrameH* FrameCopyPart(FrameH* frameIn, double start, double duration)
/*---------------------------------------------------------------------------*/
/* Returns a copy of frameIn starting at xStart and lasting duration*/
/*---------------------------------------------------------------------------*/
{
  FrProcData *p, *proc;
  FrAdcData  *a, *adc;
  FrameH *frame;
  FrSerData *ser;
  FrEvent *event;
  FrSimEvent *simEvt;
  FrVect *vect;
  double tFrame, tSer, deltaT, startX;
  FRLONG nBytes;
  char message[256];
  int  i, sum;

  if(frameIn == NULL) return(NULL);

  /*------------------- copy only the intersaction with the current frame---*/
  if(start < 0) start = 0;
  if(start > frameIn->dt) start = frameIn->dt;
  if(start+duration > frameIn->dt) duration = frameIn->dt - start;

  if(duration <= 0) return(NULL);

  /*--------------------------------------------------- copy frame header---*/
  frame = FrameHCopy(frameIn);
  if(frame == NULL) return(NULL);

  frame->dt = duration;
  frame->GTimeS = (int)(frameIn->GTimeS + start);
  frame->GTimeN = (int)(1.e9*(frameIn->GTimeS + start - frame->GTimeS));
  tFrame = (double) frame->GTimeS + 1.e-9*(double) frame->GTimeN;

  frame->history = FrHistoryCopy(frameIn->history);

  sprintf(message,"FrameCopyPart:%g (%gs->%gs start=%gs)",
	  FRAMELIB_VERSION, frameIn->dt, frame->dt, start);
  FrHistoryAdd(frame, message);

  /*-------------------------------------------------------- copy procData---*/
  for(proc = frameIn->procData; proc != NULL; proc = proc->next) {
    if(proc->data == NULL) continue;

    /*-----------------for the time series we copy only the zoomed part---*/
    if(proc->type <= 1) { 
      vect = proc->data;
      if(vect == NULL) continue;
      
      startX = vect->startX[0];
      FrVectZoomIn(vect, start, duration);

      sum = 1;
      if(vect->next != NULL) {
	sum = 0;
	for(i=0; i<vect->next->nData; i++) {sum += vect->next->data[i];}}
	
      if(sum > 0 && vect->nData > 0){/*copy vector only if data are available*/
	nBytes = vect->nBytes;
	vect->nBytes = vect->nData * vect->wSize;
	if((p = FrProcDataCopy(proc, frame)) == NULL) return(NULL);
	p->data->startX[0] = startX;
	vect->nBytes = nBytes;}

      FrVectZoomOut(vect);}

    /*---for the other types like FFT we copy them only for the first frame---*/
    else if(start == 0) {
      if((p = FrProcDataCopy(proc, frame)) == NULL) return(NULL);}
  }

  /*-------------------------------------------------------------copy ADC---*/
  if(frameIn->rawData != NULL) {
    for(adc = frameIn->rawData->firstAdc; adc != NULL; adc = adc->next) {
      vect = adc->data;
      if(vect == NULL) continue;

      startX = vect->startX[0];
      FrVectZoomIn(vect, start, duration);

      sum = 1;
      if(vect->next != NULL) {
	sum = 0;
	for(i=0; i<vect->next->nData; i++) {sum += vect->next->data[i];}}
	
      
      if(sum > 0 && vect->nData > 0){/*copy vector only if data are available*/
	nBytes = vect->nBytes;
	vect->nBytes = vect->nData * vect->wSize;
	if((a = FrAdcDataCopy(adc, frame)) == NULL) return(NULL);
	a->data->startX[0] = startX;
	vect->nBytes = nBytes;}

      FrVectZoomOut(vect);}

    /*------------------- copy only the serData which belong to this frame--*/
    /* ser data with a wrong GTimeS will not be copied--*/
    for(ser = frameIn->rawData->firstSer; ser != NULL; ser = ser->next) {
      tSer = (double) ser->timeSec + 1.e-9*(double) ser->timeNsec;
      if(tSer < tFrame)          continue;
      if(tSer > tFrame+duration) continue;

      FrSerDataNew(frame, ser->name, frame->GTimeS, ser->data, ser->sampleRate);}}

  /*------------------------------------------------------------copy events----*/
  for(event = frameIn->event; event != NULL; event = event->next) {
    deltaT = (event->GTimeS - frame->GTimeS) +
             (event->GTimeS - frame->GTimeS) * 1.e-9;
    if(deltaT < 0)        continue;
    if(deltaT > duration) continue;
    FrameAddEvent(frame, FrEventCopy(event));}

  /*----------------------------------------------------------copy simEvents----*/
  for(simEvt = frameIn->simEvent; simEvt != NULL; simEvt = simEvt->next) {
    deltaT = (simEvt->GTimeS - frame->GTimeS) + 
             (simEvt->GTimeS - frame->GTimeS) * 1.e-9;
    if(deltaT < 0)        continue;
    if(deltaT > duration) continue;
    FrameAddSimEvent(frame, FrSimEventCopy(simEvt));}

  return(frame);}

/*-------------------------------------------------------------- FrameDump---*/
void FrameDump(FrameH *frameH,
               FILE *fp, 
               int debugLvl)
/*---------------------------------------------------------------------------*/
{FrProcData *proc;
 FrSimData  *simData;
 FrSimEvent *simEvent;
 FrHistory  *history; 
 FrEvent    *event;
 FrSummary  *sum;
 FrDetector *det;
 unsigned int local, UTC;

 if(fp     == NULL) return;
 if(frameH == NULL) return;

 if(debugLvl < 1) return;

 if(debugLvl > 1) fprintf(fp,"\n");
 UTC = frameH->GTimeS - frameH->ULeapS + FRGPSTAI;
 fprintf(fp,"Dump frame %s", frameH->name);
 fprintf(fp," Starting GPS time: %d s and %d nsec", frameH->GTimeS, frameH->GTimeN);
 fprintf(fp,"  Length:%.3f sec",frameH->dt);
 fprintf(fp,"  Quality=%x\n",frameH->dataQuality);
 fprintf(fp,"      Starting UTC time: %s", FrStrGTime(UTC));
 fprintf(fp," ULeapS:%d", frameH->ULeapS);
 fprintf(fp," Run#:%d Frame#:%d\n", frameH->run, frameH->frame);

 if(debugLvl < 2) return;

 for(det = frameH->detectProc; det != NULL; det = det->next)
   {local = UTC + det->localTime;
    fprintf(fp,"             Local time: %s  for detector %s\n", 
            FrStrGTime(local), det->name);}

                                /*------------------history------------------*/
 fprintf(fp,"History records:\n");
 for(history = frameH->history; history != NULL; history = history->next)
   {fprintf(fp," %s %s\n", FrStrGTime(history->time), history->comment);}

                                /*------------detector and static data-------*/

 for(det = frameH->detectProc; det != NULL; det = det->next)
    {fprintf(fp,"Detector used for reconstruction:\n");
     FrDetectorDump(det, fp, debugLvl);}
 for(det = frameH->detectSim; det != NULL; det = det->next)
    {fprintf(fp,"Detector used for simulation:\n");
     FrDetectorDump(det, fp, debugLvl);}

                                /*------------ dump simulated data ----------*/

 for(simData = frameH->simData; simData != NULL; simData = simData->next) 
    {FrSimDataDump(simData, fp, debugLvl);}
  
 for(simEvent = frameH->simEvent; simEvent != NULL; simEvent = simEvent->next)
    {FrSimEventDump(simEvent, fp,debugLvl);}

                                /*-------- dump reconstructed data ----------*/
 if(frameH->procData != NULL)
  {fprintf(fp,"Processed Data:\n");
   for(proc = frameH->procData; proc != NULL; proc = proc->next) 
     {FrProcDataDump(proc,   fp, debugLvl);}}

                                /*------------- dump trigger data -----------*/
     
 for(event = frameH->event; event != NULL; event = event->next) 
    {FrEventDump(event, fp, debugLvl);}

                                /*------------- dump summary data -----------*/
     
 for(sum = frameH->summaryData; sum != NULL; sum = sum->next)
    {FrSummaryDump(sum,   fp, debugLvl);}

                                /*---------------------- dump user data -----*/

 if(frameH->type != NULL) 
   {fprintf(fp,"Event type:\n");
    FrVectDump(frameH->type, fp, debugLvl);}
 if(frameH->user != NULL) 
   {fprintf(fp,"User Data:\n");
    FrVectDump(frameH->user, fp, debugLvl);}
 if(frameH->auxData != NULL) 
   {fprintf(fp,"Auxiliary Data:\n");
    FrVectDump(frameH->auxData, fp, debugLvl);}
 if(frameH->auxTable != NULL) 
   {fprintf(fp,"Auxiliary Table:\n");
    FrTableDump(frameH->auxTable, fp, debugLvl);}

                               /*------------------ dump raw data -----------*/

 if(frameH->rawData != NULL) FrRawDataDump(frameH->rawData, fp, debugLvl);

 return;}

/*------------------------------------------------------ FrameDumpToBuf------*/
char *FrameDumpToBuf(FrameH *frame,
                     int debugLvl, 
                     char *buf, 
                     FRLONG bufSize)
/*---------------------------------------------------------------------------*/
{FILE *tmp;
 int  nMissingWords;
 FRLONG warningSize = 75;

 if(debugLvl < 1)    return(NULL);
 if(frame == NULL)   return(NULL);
 if(buf   == NULL)   return(NULL);
 if(bufSize<warningSize) 
      {FrError(3,"FrameDumpBuf"," too short buffer");
       return(NULL);}

 tmp = tmpfile();
 if(tmp == NULL) 
      {FrError(3,"FrameDumpBuf"," cannot open tempory file");
       return(NULL);}

 FrameDump(frame, tmp, debugLvl);
 nMissingWords = ftell(tmp) - bufSize + warningSize;
 rewind(tmp);

 fread(buf, 1, bufSize, tmp);
 fclose(tmp);

 if(nMissingWords > 0) sprintf(buf+bufSize-warningSize, "...\n "
     ">>>> Warning: FrameDump Buffer to short by %8d characters <<<<",
    nMissingWords);
 
 return(buf);}
/*----------------------------------------------------------FrameDumpTopADC--*/
void FrameDumpTopADC(FrameH *frame, FILE *fp, int nTop, int comp)
/*---------------------------------------------------------------------------*/
{static int size, maxSize, nVect, i, bSize;
 double totSize, curSize, dt;
 char *buf, *name;
 FrAdcData *adc;
 FrProcData *proc, *nProc;
 FrVect *vect, *bVect;
 FrFile *file;

 if(frame == NULL) return;
 if(frame->rawData == NULL) return;

         /*----- first get the maximum adc size for the temporary buffer ---*/

 dt = frame->dt;
 maxSize = 10000;
 for(adc = frame->rawData->firstAdc; adc != NULL; adc = adc->next)
   {size = adc->data->nData*adc->data->wSize + 10000;
    if(size > maxSize) maxSize = size;}
 for(proc = frame->procData; proc != NULL; proc = proc->next)
   {size = proc->data->nData*proc->data->wSize + 10000;
    if(size > maxSize) maxSize = size;}

                                      /*---- create the temporary file -----*/
 buf = (char *) malloc(maxSize);
 if(buf == NULL) return;

 file = FrFileNew(NULL, comp, buf, maxSize);
 if(file == NULL) return;

                    /*--open the file and write all ADC to get their size---*/
 FrFileOOpen(file);
 FrPutStruct(file, frame->rawData->firstAdc);
 nVect = 0;
 totSize = 0;
 for(adc = frame->rawData->firstAdc; adc != NULL; adc = adc->next)
    {file->p = buf;
     FrAdcDataWrite( adc, file);
     vect = adc->data;
     vect->nDataUnzoomed = file->p-buf;
     totSize +=  vect->nDataUnzoomed;
     nVect ++;}
 printf(" ADC Vect channels list by size(Bytes); (%d channels "
         "for %.0f kBytes);\n", nVect, totSize/1024.);

 for(proc = frame->procData; proc != NULL; proc = proc->next)
    {file->p = buf;
     nProc = proc->next; proc->next = NULL;
     FrPutStruct(file, proc );
     FrProcDataWrite(proc, file);
     vect = proc->data;
     vect->nDataUnzoomed = file->p-buf;
     proc->next = nProc;
     totSize +=  vect->nDataUnzoomed;
     nVect ++;}

                   /*--------------------------------- print the summary ---*/
 curSize = 0;
 printf(" ADC and Proc Vect channels list by size(Bytes); (%d channels "
        "for %.0f kBytes);\n", nVect, totSize/1024.);
 printf("   id;   Kbytes;      %%;  int. %%; name; Sampling rate(Hz); Vector type, Compress Algo;\n");
 if(nVect > nTop) nVect = nTop;
 for( i= 0; i<nVect; i++) {
   bVect = NULL;
   bSize = 0; name = frame->rawData->firstAdc->name;
   for(adc = frame->rawData->firstAdc; adc != NULL; adc = adc->next) {
     vect  = adc->data;
     if( vect->nDataUnzoomed > bSize ){
       name = adc->name;
       bSize = vect->nDataUnzoomed;
       bVect = vect;
     }
   }

   for(proc = frame->procData; proc != NULL; proc = proc->next) {
     vect  = proc->data;
     if( vect->nDataUnzoomed > bSize ){
       name = proc->name;
       bSize = vect->nDataUnzoomed;
       bVect = vect;
     }
   }
   curSize += bSize;
   printf("%5d; %8d; %4.4f; %4.4f; %s; %.2f; %2d; %2d;\n",i, bSize,
          100.*bSize/totSize,100.*curSize/totSize, name, bVect->nData/dt,
          bVect->type,
         (bVect->compress > 255 ? bVect->compress - 256 : bVect->compress));
   bVect->nDataUnzoomed = 0;
 }

 FrFileOEnd(file);

 free(buf);

 return;}

/*------------------------------------------------------------ FrameExpand---*/
void FrameExpand(FrameH *frameH)
/*---------------------------------------------------------------------------*/
{FrDetector *det;
 FrAdcData  *adc;
 FrProcData *proc;
 FrSimData  *sim;
 FrSimEvent *simEvt;
 FrSerData  *ser;
 FrEvent    *evt;
 FrSummary  *sum;

 if(frameH == NULL) return;

 FrVectExpandF(frameH->type);
 FrVectExpandF(frameH->user);
 FrVectExpandF(frameH->auxData);
 FrTableExpand(frameH->auxTable);

 for(det = frameH->detectProc; det != NULL; det = det->next)
   {FrVectExpandF(det->aux);
    FrTableExpand(det->table);}

 for(det = frameH->detectSim; det != NULL; det = det->next)
   {FrVectExpandF(det->aux);
    FrTableExpand(det->table);}
                                /*-------------- Uncompress the raw data ----*/
     
 if(frameH->rawData != NULL) 
   {for(adc = frameH->rawData->firstAdc; adc != NULL; adc = adc->next)
       {FrVectExpandF(adc->data);
        FrVectExpandF(adc->aux);}
    for(ser = frameH->rawData->firstSer; ser != NULL; ser = ser->next)
       {FrVectExpandF(ser->serial);
        FrTableExpand(ser->table);}
    FrTableExpand(frameH->rawData->firstTable);
    FrVectExpandF(frameH->rawData->more);}

                                /*----------- Uncompress the other data ----*/

 for(proc = frameH->procData; proc != NULL; proc = proc->next)
       {FrVectExpandF(proc->data);
        FrVectExpandF(proc->aux);
        FrTableExpand(proc->table);}

 for(sim = frameH->simData; sim != NULL; sim = sim->next)
      {FrVectExpandF(sim->data);
       FrVectExpandF(sim->input);
       FrTableExpand(sim->table);}  

 for(simEvt = frameH->simEvent; simEvt != NULL; simEvt = simEvt->next)
   {FrVectExpandF(simEvt->data);
    FrTableExpand(simEvt->table);}

 for(evt = frameH->event; evt != NULL; evt = evt->next)
      {FrVectExpandF(evt->data);
       FrTableExpand(evt->table);}

 for(sum = frameH->summaryData; sum != NULL; sum = sum->next)
      {FrVectExpandF(sum->moments);
       FrTableExpand(sum->table);}

 return;}

/*------------------------------------------------------------FrameFree------*/
void FrameFree(FrameH *frameH)
/*---------------------------------------------------------------------------*/
{
 if(frameH == NULL) return;

 FrameUntag(frameH);
                     /*------ free all structures connected to this frame ---*/

 if(frameH->type        != NULL) FrVectFree    (frameH->type);   
 if(frameH->user        != NULL) FrVectFree    (frameH->user);   
 if(frameH->detectSim   != NULL) FrDetectorFree(frameH->detectSim);
 if(frameH->detectProc  != NULL) FrDetectorFree(frameH->detectProc);
 if(frameH->history     != NULL) FrHistoryFree (frameH->history);
 if(frameH->rawData     != NULL) FrRawDataFree (frameH->rawData);
 if(frameH->procData    != NULL) FrProcDataFree(frameH->procData);   
 if(frameH->simData     != NULL) FrSimDataFree (frameH->simData);   
 if(frameH->event       != NULL) FrEventFree   (frameH->event);   
 if(frameH->simEvent    != NULL) FrSimEventFree(frameH->simEvent);   
 if(frameH->summaryData != NULL) FrSummaryFree (frameH->summaryData);   
 if(frameH->auxData     != NULL) FrVectFree    (frameH->auxData);   
 if(frameH->auxTable    != NULL) FrTableFree   (frameH->auxTable);   

 free(frameH->name);
 free(frameH);

 return;}

/*----------------------------------------------------------FrameFindBasic--*/
FrBasic* FrameFindBasic(FrBasic*root,
                        char *name)
/*---------------------------------------------------------------------------*/
{FrBasic *adc;

 if(root == NULL) return(NULL);
 if(name == NULL) return(NULL);
                             /*----- first try to find the correct name ----*/

 for(adc = root; adc != NULL; adc = adc->next)
   {if(FrStrcmpAndPrefix(adc->name, name) == 0) return(adc);}

 return(NULL);}

/*------------------------------------------------------------FrameFindVect--*/
FrVect *FrameFindVect(FrameH *frame,
                      char *name)
/*---------------------------------------------------------------------------*/
{FrVect *vect;

 if((vect = FrameFindAdcVect (frame, name)) != NULL) return(vect); 
 if((vect = FrameFindProcVect(frame, name)) != NULL) return(vect);
 if((vect = FrameFindSumVect (frame, name)) != NULL) return(vect);
 if((vect = FrameFindStatVect(frame, name)) != NULL) return(vect);

 return(FrameFindSimVect(frame, name));}

/*---------------------------------------------------------FrameFindAdcVect--*/
FrVect *FrameFindAdcVect(FrameH *frame, 
                         char *adcName)
/*---------------------------------------------------------------------------*/
{FrAdcData *adc;

 adc = FrAdcDataFind(frame, adcName);
 if(adc != NULL) return(adc->data);

 return (NULL);}

/*--------------------------------------------------------FrameFindDetector--*/
FrDetector *FrameFindDetector(FrameH *frame,
                              char *detNameOrPrefix)
/*---------------------------------------------------------------------------*/
{FrDetector *det;
 int len;

 if(frame == NULL)             return (NULL);
 if(detNameOrPrefix == NULL)   return (NULL);

 len = strlen(detNameOrPrefix);

 for(det = frame->detectProc; det != NULL; det = det->next)
   {if(strcmp(det->name, detNameOrPrefix) == 0) return(det);
    if(len != 2) continue;
    if((det->prefix[0] == detNameOrPrefix[0]) &&
       (det->prefix[1] == detNameOrPrefix[1])) return(det);}

 for(det = frame->detectSim; det != NULL; det = det->next)
   {if(strcmp(det->name, detNameOrPrefix) == 0) return(det);
    if(len != 2) continue;
    if((det->prefix[0] == detNameOrPrefix[0]) &&
       (det->prefix[1] == detNameOrPrefix[1])) return(det);}
  
 return (NULL);}

/*--------------------------------------------------------FrameFindProcVect--*/
FrVect *FrameFindProcVect(FrameH *frame,
                          char *name)
/*---------------------------------------------------------------------------*/
{FrProcData *proc;

 proc = FrProcDataFind(frame, name);
 if(proc != NULL) return(proc->data);
  
 return (NULL);}
/*---------------------------------------------------------FrameFindSimVect--*/
FrVect *FrameFindSimVect(FrameH *frame,
                         char *name)
/*---------------------------------------------------------------------------*/
{FrSimData *sim;

 sim = FrSimDataFind(frame, name);
 if(sim != NULL) return(sim->data);
  
 return (NULL);}

/*--------------------------------------------------------FrameFindStatVect--*/
FrVect *FrameFindStatVect(FrameH *frame, 
                          char * name)
/*---------------------------------------------------------------------------*/
{FrStatData *stat;

 if(frame == NULL) return(NULL);

 stat = FrStatDataFind(frame->detectProc, name, 0); 
 if(stat == NULL) return(NULL);

 return(stat->data);}
/*---------------------------------------------------------FrameFindSumVect--*/
FrVect *FrameFindSumVect(FrameH *frame, 
                         char * name)
/*---------------------------------------------------------------------------*/
{FrSummary *sum;

 sum = FrSummaryFind(frame, name); 
 if(sum == NULL) return(NULL);

 return(sum->moments);}

/*--------------------------------------------------------- FrameGetAdcSize--*/
FRLONG FrameGetAdcSize(FrameH *frame)
/*---------------------------------------------------------------------------*/
{FrAdcData *adc;
 FRLONG size;

 if(frame == NULL) return(0);
 if(frame->rawData == NULL) return(0);
 size = 0;

 for(adc = frame->rawData->firstAdc; adc != NULL; adc = adc->next)
    {size += FrAdcDataGetSize(adc);}

 return(size);}

/*---------------------------------------------------------FrameGetLTOffset--*/
int FrameGetLTOffset(FrameH *frame, 
                     char *channelName)
/*---------------------------------------------------------------------------*/
{FrDetector *det;
 int localTimeOffset = 99999;   /* default invalid value */

 if(frame == NULL)       return (localTimeOffset);
 if(channelName == NULL) return (localTimeOffset);

 for(det = frame->detectProc; det != NULL; det = det->next)
   {if(det->name[0] != channelName[0]) continue;
    return(det->localTime);}

    /*----- special case for virgo, since the channel name are incompleted--*/

 if(frame->detectProc != NULL)
  {if(frame->detectProc->name[0] == 'V') return(frame->detectProc->localTime);}

 return (localTimeOffset);}
 
/*----------------------------------------------------------------------------*/
FrStatData *FrameFindStatData(FrameH *frame,
	   	          char *detectorName,
			  char *statDataName,
                          int gpsTime)
/*---------------------------------------------------------------------------*/
{FrDetector *det;
 FrStatData *stat;

 if(detectorName != NULL)
     {det = FrameFindDetector(frame, detectorName);}
 else{det = frame->detectProc;}
 if(det == NULL) return(NULL);

 stat = FrDetectorFindStatData(det, statDataName, gpsTime);

 return(stat);}

/*----------------------------------------------------------------------------*/
FrVect *FrameGetStatVect(FrameH *frame,
		          char *detectorName,
			  char *statDataName,
                          char *vectorName, 
                          int gpsTime)
/*---------------------------------------------------------------------------*/
{FrStatData *stat;
 FrVect *vect;

 if(statDataName == NULL) statDataName = vectorName;

 stat = FrameFindStatData(frame, detectorName, statDataName, gpsTime);
 if(stat == NULL) return(NULL);

 if(vectorName   == NULL) return(stat->data);

 for(vect = stat->data; vect != NULL; vect = vect->next)
    {if(strcmp(vect->name, vectorName) != 0) continue;
     vect = FrVectCopy(vect);
     break;}

 return(vect);}
 
/*------------------------------------WARNING:OBSOLETE FUNCTION---FrameGetV--*/
FrVect *FrameGetV(FrameH *frame,char *name)
/*---------------------------------------------------------------------------*/
{
  return(FrameFindVect(frame,name));
}
/*------------------------------------------------------------FrameLatency---*/
double FrameLatency (FrameH *frame)
/*---------------------------------------------------------------------------*/
{
 double latency;

 if (frame == NULL) return(-1.);

 latency = FrGetCurrentGPS() - frame->GTimeS - frame->GTimeN * 1.0e-9;

 return(latency);
}
/*-------------------------------------------------------------- FrameMerge--*/
void FrameMerge(FrameH *frame1, 
                FrameH *frame2)
/*---------------------------------------------------------------------------*/
{FrAdcData  **adc, *ad;
 FrHistory  **history;
 FrTable    **table;
 FrSerData  **ser, *se;
 FrEvent    **event;
 FrMsg      **msg, *ms;
 FrSimData  **sim;
 FrProcData **proc, *pr;
 FrSimEvent **simE;
 FrSummary  **sum;
 FrVect     **aux;
 FrDetector *det1, *det2, *next;
 double     offset;

 if (frame1 == NULL) return;
 if (frame2 == NULL) return;

                   /*------------- Propagate timing information if needed ---*/

 if(frame1->rawData != NULL)
   {for(ms = frame1->rawData->logMsg; ms != NULL; ms = ms->next)
     {if(ms->GTimeS > 1000) continue;
      ms->GTimeS += frame1->GTimeS;
      ms->GTimeN += frame1->GTimeN;}}
 if(frame2->rawData != NULL)
   {for(ms = frame2->rawData->logMsg; ms != NULL; ms = ms->next)
     {if(ms->GTimeS > 1000) continue;
      ms->GTimeS += frame2->GTimeS;
      ms->GTimeN += frame2->GTimeN;}}

                  /*----------------------------------- Merge header info ---*/

 frame1->dataQuality = frame1->dataQuality | frame2->dataQuality;

                  /*-------------------- merge detector info if different ---*/
 
 for(det2 = frame2->detectProc; det2 != NULL; det2 = next)
   {next = det2->next;
    for(det1 = frame1->detectProc; det1 != NULL; det1 = det1->next)
      {if(strcmp(det1->name, det2->name) == 0) break;}
    if(det1 == NULL)                /*-new detector: move it to frame1 liste-*/
        {det2->next = frame1->detectProc;
         frame1->detectProc = det2;}
    else{FrDetectorMerge(det1,det2);}} /*-same detectors: merge info --------*/
 frame2->detectProc = NULL;

 for(det2 = frame2->detectSim; det2 != NULL; det2 = next)
   {next = det2->next;
    for(det1 = frame1->detectSim; det1 != NULL; det1 = det1->next)
      {if(strcmp(det1->name, det2->name) == 0) break;}
    if(det1 == NULL)                /*-new detector: move it to frame1 liste-*/
        {det2->next = frame1->detectSim;
         frame1->detectSim = det2;}
    else{FrDetectorMerge(det1,det2);}} /*-same detectors: merge info --------*/
  frame2->detectSim = NULL;

                  /*---------- transport to time series the frame offset ----*/

 offset = (frame2->GTimeS - frame1->GTimeS) + 1.e-9 *
          (frame2->GTimeN - frame1->GTimeN);

                  /*-- if there is a discrepency for ULeapS, take the largest*/
 /* this is usefull for Virgo DAQ when ULeapS is changeing on jan 1st for instance*/

 if(frame1->ULeapS > frame2->ULeapS) frame2->ULeapS = frame1->ULeapS;
 if(frame2->ULeapS > frame1->ULeapS) frame1->ULeapS = frame2->ULeapS;

                  /*-- if no timing is available for one frame, 
                                      use the values from the other one  ----*/

 if(frame2->GTimeS == 0)  offset = 0.;
 if(frame1->GTimeS == 0) {offset = 0.;
                          frame1->GTimeS = frame2->GTimeS;
                          frame1->GTimeN = frame2->GTimeN;}
 if(offset != 0) {
   if (frame2->rawData != NULL)
     {for(se = frame2->rawData->firstSer; se != NULL; se = se->next) 
         {se->timeNsec += offset;}
      for(ad = frame2->rawData->firstAdc; ad != NULL; ad = ad->next) 
         {ad->timeOffset += offset;}}

   for(pr = frame2->procData; pr != NULL; pr = pr->next) 
      {pr->timeOffset += offset;}}

                  /*---------- merge the raw data part ----------------------*/

 if (frame2->rawData != NULL)

   {if (frame1->rawData == NULL) 
        frame1->rawData = FrRawDataNew(NULL);

    ser = &(frame1->rawData->firstSer);
    while(*ser != NULL)  {ser = &((*ser)->next);}
    *ser = frame2->rawData->firstSer;
    frame2->rawData->firstSer = NULL;

    adc = &(frame1->rawData->firstAdc);
    while(*adc != NULL) {adc = &((*adc)->next);}
    *adc = frame2->rawData->firstAdc;
    frame2->rawData->firstAdc = NULL;

    table = &(frame1->rawData->firstTable);
    while(*table != NULL) {table = &((*table)->next);}
    *table = frame2->rawData->firstTable;
    frame2->rawData->firstTable = NULL;

    msg = &(frame1->rawData->logMsg);
    while(*msg != NULL) {msg = &((*msg)->next);}
    *msg = frame2->rawData->logMsg;
    frame2->rawData->logMsg = NULL;}

                  /*-------------- Merge the top structures -----------------*/

 history = &(frame1->history);
 while(*history != NULL) {history = &((*history)->next);}
 *history = frame2->history;
 frame2->history = NULL;

 proc = &(frame1->procData);
 while(*proc != NULL) {proc = &((*proc)->next);}
 *proc = frame2->procData;
 frame2->procData = NULL;

 sim = &(frame1->simData);
 while(*sim != NULL) {sim = &((*sim)->next);}
 *sim = frame2->simData;
 frame2->simData = NULL;

 event = &(frame1->event);
 while(*event != NULL) {event = &((*event)->next);}
 *event = frame2->event;
 frame2->event = NULL;

 simE = &(frame1->simEvent);
 while(*simE != NULL) {simE = &((*simE)->next);}
 *simE = frame2->simEvent;
 frame2->simEvent = NULL;

 sum = &(frame1->summaryData);
 while(*sum != NULL) {sum = &((*sum)->next);}
 *sum = frame2->summaryData;
 frame2->summaryData = NULL;

 aux = &(frame1->auxData);
 while(*aux != NULL) {aux = &((*aux)->next);}
 *aux = frame2->auxData;
 frame2->auxData = NULL;

 table = &(frame1->auxTable);
 while(*table != NULL) {table = &((*table)->next);}
 *table = frame2->auxTable;
 frame2->auxTable = NULL;

                  /*---------------- delete the remaining of frame 2---------*/

 FrameFree(frame2);

 return;
}
/*-----------------------------------------------------------------FrameNew--*/
FrameH *FrameNew(char *name)
/*---------------------------------------------------------------------------*/
{FrameH *frame;

 frame = FrameHNew(name);
 if(frame == NULL) return(NULL);

                 /*---------  Fill timing information ----------------------*/

 frame->GTimeS = FrGetCurrentGPS();
 frame->ULeapS = FrGetLeapS(frame->GTimeS);

                     /*------Add detector information -----------------------*/

 frame->detectProc = FrDetectorNew(name);

 return(frame);}
/*----------------------------------------------------------------FrameRead--*/
FrameH *FrameRead(FrFile *iFile)
/*---------------------------------------------------------------------------*/
{FrStatData *next, *sData;
 FrDetector *det;
 FrameH *frame;
 char msg[1024];

 if(iFile == NULL) return(NULL);
 if(iFile->frfd == NULL  &&
    iFile->inMemory == FR_NO) FrFileIOpen(iFile);  /*--Open file if needed --*/

 FrSetIni(iFile);             /*----- reset address relocation mechanism ----*/
 
 iFile->curFrame = NULL;
 iFile->endOfFrame = FR_NO;
 iFile->sDataCur = NULL;

 /*----- Read each structure of the frame starting by the length and type ---*/
 while((iFile->error == FR_OK) && (iFile->endOfFrame == FR_NO)) {
   if(iFile->curFrame == NULL) {
     iFile->nBytesF  = iFile->nBytes; /*--reset checksum before FrameH--*/
     iFile->chkSumFr = 0;}

   FrReadStructHeader(iFile);
   if(iFile->error != FR_OK) break;
          
   if(FrDebugLvl > 2) {
     fprintf(FrFOut," length:%10"FRLLD" type=%4d ",
	     iFile->length,iFile->type);
     if(iFile->type > 2 && iFile->type < iFile->maxSH)
       {if(iFile->sh[iFile->type] != NULL)
	   {fprintf(FrFOut,"(%12s)",iFile->sh[iFile->type]->name);}}}
                             
   if((iFile->type >= iFile->maxSH) || 
      (iFile->sh[iFile->type] == NULL)) {
     FrError(3,"FrameRead"," missiging dictionary");
     iFile->error = FR_ERROR_READ_ERROR;
     break;}
 
   iFile->sh[iFile->type]->nBytes += iFile->length; 
   iFile->sh[iFile->type]->nInstances ++;
   iFile->sh[iFile->type]->objRead(iFile);}

 if(iFile->error != FR_OK) {
   sprintf(msg,"%s for %s",FrErrorMsg[iFile->error], iFile->current->fileName);
   FrError(3,"FrameRead",msg);
   return(NULL);}

 frame = iFile->curFrame; 
 if(frame == NULL) {return(NULL);}
           
 /*-------------------------------------------------- pointer reallocation--- */
 FrSetAll(iFile);   

 /*--------------------------- attach static data to the file linked list-----*/
 for(sData = iFile->sDataCur; sData != NULL; sData = next) {
   next = sData->next;

   for(det = frame->detectProc; det != NULL; det = det->next) {
     if(det != sData->detector) continue;
     FrStrCpy(&(sData->detName), det->name); 
     FrStatDataAddR(&(iFile->sDataProc), sData);
     sData = NULL;
     break;}

   if(sData == NULL) continue;
   for(det = frame->detectSim; det != NULL; det = det->next) {
     if(det != sData->detector) continue;
     if(sData->detName != NULL) free(sData->detName);
     FrStrCpy(&(sData->detName), det->name); 
     FrStatDataAddR(&(iFile->sDataSim),  sData);
     sData = NULL;
     break;}
 
   if(sData != NULL) FrError(3,"FrameRead","Orphan FrStatData");}

                        /*------------clean up the static data information --*/
  
  FrStatDataChkT(&(iFile->sDataProc), frame->GTimeS, 
                                      frame->GTimeS + frame->dt);
  FrStatDataChkT(&(iFile->sDataSim),  frame->GTimeS, 
                                      frame->GTimeS + frame->dt);
       
                        /*-------------- copy the stat data to this frame ---*/

  for(det = frame->detectSim; det != NULL; det = det->next)
    {for(sData = iFile->sDataSim; sData != NULL; sData = sData->next)
       {if(strcmp(sData->detName, det->name) == 0) FrStatDataCopy(sData,det);}}

  for(det = frame->detectProc; det != NULL; det = det->next)
    {for(sData = iFile->sDataProc; sData != NULL; sData = sData->next)
       {if(strcmp(sData->detName, det->name) == 0) FrStatDataCopy(sData,det);}}
      
  return(iFile->curFrame);}

/*---------------------------------------------------------FrameReadFromBuf--*/
FrameH *FrameReadFromBuf(char *buf, 
                         FRLONG nBytes,
                         int comp)
/*---------------------------------------------------------------------------*/
{FrFile *iFile;
 FrameH *frame;
 
 if(buf == NULL) return(NULL);
 if(nBytes <= 0) return(NULL);
  
 iFile = FrFileNew(NULL, comp, buf, nBytes);
 if(iFile == NULL) return (NULL);

 iFile->chkSumFiFlag = FR_NO; /* do not compute checksum for in memory buffer*/
 iFile->chkSumFrFlag = FR_NO; 

 FrFileIOpen(iFile);
 if(iFile->error != FR_OK) return(NULL);

 frame = FrameRead(iFile);

 FrFileIEnd(iFile);

 return(frame);}

/*-------------------------------------------------------------- FrameReadN--*/
FrameH *FrameReadN(FrFile *iFile,
                   int rNumber,
                   int fNumber)
/*---------------------------------------------------------------------------*/
{FrameH *frame;
 int i;

 i = FrTOCFrameFindN(iFile, rNumber, fNumber);

                 /*---TOC information available but no frame for this time---*/

 if(i == -2) return(NULL);

                 /*---- the frame has been found in the TOC: direct read-----*/
 if(i != -1)
   {if(FrFileIOSet(iFile, iFile->toc->positionH[i])==-1) {return (NULL);}
    
    frame = FrameRead(iFile);

    FrTOCStatDGet(iFile, frame);}

                /*----- no TOC information available, scan the file ---------*/

 else
   while(1)
     {frame = FrameRead(iFile);
      if(frame == NULL) break;
      if((rNumber == frame->run) && (fNumber == frame->frame)) break;
      FrameFree(frame);}
  
 return(frame);}

/*---------------------------------------------------------FrameReadRecycle--*/
FrameH *FrameReadRecycle(FrFile *iFile, 
                         FrameH *frame)
/*---------------------------------------------------------------------------*/
{

 FrameFree(frame);
 frame = FrameRead(iFile);

 return(frame);}

/*-------------------------------------------------------------- FrameReadT--*/
FrameH *FrameReadT(FrFile *iFile,
                   double gtime)
/*---------------------------------------------------------------------------*/
{FrameH *frame = NULL;
 int i;
 double ftime;

 i = FrTOCFrameFindT(iFile, gtime);

                 /*---- the frame has been found in the TOC: direct read-----*/

 if(i >= 0)
   {if(FrFileIOSet(iFile, iFile->toc->positionH[i])==-1) {return (NULL);}
    
    frame = FrameRead(iFile);

    FrTOCStatDGet(iFile, frame);}

                /*----- no TOC information available, scan the file ---------*/

 if(i == -2) 
   {while((frame = FrameReadRecycle(iFile,frame)) != NULL)
     {ftime = frame->GTimeS + 1.e-9 * frame->GTimeN;
      if((ftime <= gtime) && (gtime<= ftime+frame->dt)) break;}}
 
 return(frame);}

/*------------------------------------------------------------FrameReadTAdc--*/
FrameH *FrameReadTAdc(FrFile *iFile,
                      double gtime,
                      char *tag)
/*---------------------------------------------------------------------------*/
{FrameH *frame;

 frame = FrameHReadT(iFile,  gtime);
 if(frame == NULL) return(NULL);

 FrRawDataNew(frame);
 if(frame->rawData != NULL)
    frame->rawData->firstAdc = FrAdcDataReadT(iFile, tag, gtime);
    
 return(frame);
}
/*-----------------------------------------------------------FrameReadTChnl--*/
FrameH *FrameReadTChnl(FrFile *iFile,
                       double gtime,
                       char *tag)
/*---------------------------------------------------------------------------*/
{FrameH *frame;
 FrVect *snr, *gps;
 int nData, i, index;
 FrTag* word, *listOfWord;
 char star[] = "*";
 char *name;
 double sampleRate;

 frame = FrameHReadT(iFile,  gtime);
 if(frame == NULL) return(NULL);

 FrRawDataNew(frame);
 if(frame->rawData != NULL) 
   {frame->rawData->firstSer = FrSerDataReadT(iFile, tag, gtime);
    frame->rawData->firstAdc = FrAdcDataReadT(iFile, tag, gtime);}

 frame->procData = FrProcDataReadT(iFile, tag, gtime);    
 frame->simData  = FrSimDataReadT (iFile, tag, gtime);
    
 /*-------------- build pseudo procData channel for the events ----*/

 if(strstr(tag,"EVENT_SNR:") == 0) return(frame);

 listOfWord = FrTagNew(tag);

 for(word = listOfWord; word != NULL; word = word->next)
   {if(strncmp(word->start,"EVENT_SNR:",10) == 0) 
       {name = word->start+10;
	 if(strncmp(name,"ALL",3) == 0) name = star;
        gps = FrFileIGetEventInfo(iFile, name, gtime, frame->dt, 0., 1.e37);}
    else if(strncmp(word->start,"SIM_EVENT_SNR:",14) == 0) 
       {name = word->start+14;
	 if(strncmp(name,"ALL",3) == 0) name = star;
        gps = FrFileIGetSimEventInfo(iFile, name, gtime, frame->dt, 0, 1.e37);}
    else {continue;}

    sampleRate = 20000.;
    nData = frame->dt*sampleRate + 0.1;
    snr = FrVectNewTS(word->start, sampleRate, nData, -32);
    if(snr == NULL) return(frame);
    snr->GTime = gtime;

  if(gps != NULL)
    {for(i=0; i<gps->nData; i++)
      {index = FrVectGetIndex(snr, gps->dataD[i]-gtime);
       snr->dataF[index] += gps->next->dataF[i];}}

     FrProcDataNewV(frame, snr);
     FrVectFree(gps);}

 FrTagFree(listOfWord);

 return(frame);
}
/*-----------------------------------------------------------FrameReadTProc--*/
FrameH *FrameReadTProc(FrFile *iFile,
                       double gtime,
                       char *tag)
/*---------------------------------------------------------------------------*/
{FrameH *frame;

 frame = FrameHReadT(iFile,  gtime);
 if(frame == NULL) return(NULL);

 frame->procData = FrProcDataReadT(iFile, tag, gtime);
    
 return(frame);
}
/*------------------------------------------------------------FrameReadTSer--*/
FrameH *FrameReadTSer(FrFile *iFile,
                      double gtime,
                      char *tag)
/*---------------------------------------------------------------------------*/
{FrameH *frame;

 frame = FrameHReadT(iFile,  gtime);
 if(frame == NULL) return(NULL);

 FrRawDataNew(frame);
 if(frame->rawData != NULL) 
    frame->rawData->firstSer = FrSerDataReadT(iFile, tag, gtime);
    
 return(frame);
}
/*------------------------------------------------------------FrameReadTSim--*/
FrameH *FrameReadTSim(FrFile *iFile,
                      double gtime,
                      char *tag)
/*---------------------------------------------------------------------------*/
{FrameH *frame;

 frame = FrameHReadT(iFile,  gtime);
 if(frame == NULL) return(NULL);

 frame->simData = FrSimDataReadT(iFile, tag, gtime);
    
 return(frame);
}

/*------------------------------------------------------FrameRemoveUntagged--*/
void FrameRemoveUntagged(FrameH *frame)
/*---------------------------------------------------------------------------*/
{
 if(frame == NULL) return;

 FrameRemoveUntaggedAdc   (frame);
 FrameRemoveUntaggedEvent (frame);
 FrameRemoveUntaggedProc  (frame);
 FrameRemoveUntaggedSer   (frame);
 FrameRemoveUntaggedSim   (frame);
 FrameRemoveUntaggedSimEvt(frame);
 FrameRemoveUntaggedStat  (frame);
 FrameRemoveUntaggedSum   (frame);

 return;}
/*--------------------------------------------------FrameRemoveUntaggedData--*/
void FrameRemoveUntaggedData(FrameH *frame)
/*---------------------------------------------------------------------------*/
{
 FrameRemoveUntagged(frame);

 return;}
/*--------------------------------------------------FrameRemoveUntaggedAdc--*/
void FrameRemoveUntaggedAdc(FrameH *frame)
/*---------------------------------------------------------------------------*/
{FrAdcData *adc,   *adcOK,   *nextAdc;

 if(frame == NULL) return; 
 if(frame->rawData == NULL) return;
 
 adcOK = frame->rawData->firstAdc;
 for(adc = frame->rawData->firstAdcOld; adc != NULL; adc = nextAdc)
    {nextAdc = adc->nextOld;
     if(adc == adcOK)
	  {adcOK = adcOK->next;}
     else {FrAdcDataFreeOne(adc);}}
 frame->rawData->firstAdcOld = NULL;

 return;}
/*-------------------------------------------------FrameRemoveUntaggedEvent--*/
void FrameRemoveUntaggedEvent(FrameH *frame)
/*---------------------------------------------------------------------------*/
{FrEvent   *event, *eventOK, *nextEvent;

 if(frame == NULL) return; 
 eventOK = frame->event; 
 for(event = frame->eventOld; event != NULL; event = nextEvent)
   {nextEvent = event->nextOld;
    if(event == eventOK)
         {eventOK = event->nextOld;}
    else {event->next = NULL;
          FrEventFree(event);}}
 frame->eventOld = NULL;

 return;}
/*--------------------------------------------------FrameRemoveUntaggedProc--*/
void FrameRemoveUntaggedProc(FrameH *frame)
/*---------------------------------------------------------------------------*/
{FrProcData *proc, *procOK,  *nextProc;
 
 if(frame == NULL) return; 
 procOK = frame->procData; 
 for(proc = frame->procDataOld; proc != NULL; proc = nextProc)
    {nextProc = proc->nextOld;
     if(proc == procOK)
          {procOK = proc->nextOld;}
     else {proc->next = NULL;
          FrProcDataFree(proc);}}
 frame->procDataOld = NULL;

 return;}
/*--------------------------------------------------FrameRemoveUntaggedSer--*/
void FrameRemoveUntaggedSer(FrameH *frame)
/*---------------------------------------------------------------------------*/
{FrSerData *ser,   *serOK,   *nextSer;
 
 if(frame == NULL) return; 
 if(frame->rawData == NULL) return;
 
 serOK = frame->rawData->firstSer;
 for(ser = frame->rawData->firstSerOld; ser != NULL; ser = nextSer)
    {nextSer = ser->nextOld;
     if(ser == serOK)
          {serOK = serOK->next;}
     else {ser->next = NULL;
           FrSerDataFree(ser);}
 frame->rawData->firstSerOld = NULL;}

 return;}
/*--------------------------------------------------FrameRemoveUntaggedSim--*/
void FrameRemoveUntaggedSim(FrameH *frame)
/*---------------------------------------------------------------------------*/
{FrSimData *sim,   *simOK,   *nextSim;
 
 if(frame == NULL) return; 
 simOK = frame->simData; 
 for(sim = frame->simDataOld; sim != NULL; sim = nextSim)
    {nextSim = sim->nextOld;
     if(sim == simOK)
          {simOK = simOK->next;}
     else {sim->next = NULL;
           FrSimDataFree(sim);}}
 frame->simDataOld = NULL;

 return;}
/*------------------------------------------------FrameRemoveUntaggedSimEvt--*/
void FrameRemoveUntaggedSimEvt(FrameH *frame)
/*---------------------------------------------------------------------------*/
{FrSimEvent   *event, *eventOK, *nextEvent;

 if(frame == NULL) return; 
 eventOK = frame->simEvent; 
 for(event = frame->simEventOld; event != NULL; event = nextEvent)
   {nextEvent = event->nextOld;
    if(event == eventOK)
         {eventOK = event->nextOld;}
    else {event->next = NULL;
          FrSimEventFree(event);}}
 frame->simEventOld = NULL;

 return;}

/*------------------------------------------------FrameRemoveUntaggedStat--*/
void FrameRemoveUntaggedStat(FrameH *frame)
/*---------------------------------------------------------------------------*/
{FrStatData *stat, *statOK, *nextStat;

 if(frame->detectProc != NULL)
   {statOK = frame->detectProc->sData; 
    for(stat = frame->detectProc->sDataOld; stat != NULL; stat = nextStat)
      {nextStat = stat->nextOld;
       if(stat == statOK)
            {statOK = stat->nextOld;}
       else {stat->next = NULL;
             FrStatDataFree(stat);}}
    frame->detectProc->sDataOld = NULL;}

 if(frame->detectSim != NULL)
   {statOK = frame->detectSim->sData; 
    for(stat = frame->detectSim->sDataOld; stat != NULL; stat = nextStat)
      {nextStat = stat->nextOld;
       if(stat == statOK)
            {statOK = stat->nextOld;}
       else {stat->next = NULL;
             FrStatDataFree(stat);}}
    frame->detectSim->sDataOld = NULL;}

 return;}

/*---------------------------------------------------FrameRemoveUntaggedSum--*/
void FrameRemoveUntaggedSum(FrameH *frame)
/*---------------------------------------------------------------------------*/
{FrSummary *sum,   *sumOK,   *nextSum;
 
 if(frame == NULL) return; 
 sumOK = frame->summaryDataOld; 
 for(sum = frame->summaryDataOld; sum != NULL; sum = nextSum)
    {nextSum = sum->nextOld;
     if(sum == sum->nextOld)
          {sumOK = sum->nextOld;}
     else {sum->next = NULL;
           FrSummaryFree(sum);}}
 frame->summaryDataOld = NULL;

 return;}

/*-------------------------------------------------FrameRemoveDuplicatedADC--*/
int FrameRemoveDuplicatedADC(FrameH *frame)
/*---------------------------------------------------------------------------*/
{
  FrAdcData *adcI, *adcJ, *adc, *root;
  FrCList *list;
  FrVect *available;
  int i, j, k, nDuplicated;

  if(frame                    == NULL) return(0);
  if(frame->rawData           == NULL) return(0);
  if(frame->rawData->firstAdc == NULL) return(0);

  /*------------------------------------uncompressed the auxiliary vectors---*/
  for(adc = frame->rawData->firstAdc; adc != NULL; adc = adc->next) {
    available = adc->data->next;
    if(available == NULL) continue;
    if(available->compress != 0) FrVectExpand(available);
    for(k=0; k< available->nData; k++) {
      if(available->data[k] == 0) break;}
    if(k ==  available->nData) {
      FrVectFree(available);
      adc->data->next = NULL;}}

  /*-------------------------------------create a sorted list of channels---*/
  list = FrCListBldAdc(frame);
  root = NULL;
  nDuplicated = 0;

  /*------scan the list to see if there are channels with the same name
          since the list is sorted such channel must be next to each other---*/
  for(i=0; i<list->nChannels; ) {
    adcI = (FrAdcData*) FrCListGetElement(list, i);

    for(j=i+1; j<list->nChannels; j++) {
      adcJ = (FrAdcData*) FrCListGetElement(list, j);

      if(strcmp(adcI->name, adcJ->name) != 0) break; 

      /*-------- channels are identical; keep the one without missing part---*/
      nDuplicated++;

      if(adcI->data->next == NULL) {
        adcJ->next = NULL;
        FrAdcDataFree(adcJ);
        continue;}
      if(adcJ->data->next == NULL) {
        adcI->next = NULL;
        FrAdcDataFree(adcI);
        adcI = adcJ;
        continue;}
      else { }} /*--if both channel miss data we should do something smarter-*/

    adcI->next = root;
    root = adcI;
    i = j;}

  frame->rawData->firstAdc = root;
  FrCListFree(list);
 
  return(nDuplicated);
}
/*------------------------------------------------------------FrFrameReshape---*/
FrameH* FrameReshape(FrameH* frame,
                     FrameH** resizedFrame,
                     int newFrameLength)
/*-----------------------------------------------------------------------------*/
/* Put several frames together to make longer frames of duration newFrameLength*/ 
/* resizedFrame is a pointer to get the intermetidate working frame.           */
/* This function returns the new frame or NULL if it is incomplet.             */
/*  Work only for frames with an length equal to an integer number of seconds  */
/*-----------------------------------------------------------------------------*/
{
  FrameH* output;
  int position, nFrames, resizedEnd;

  if(frame == NULL) return(NULL);

  output   = NULL;
  position = (frame->GTimeS % newFrameLength)/frame->dt;
  nFrames  = (newFrameLength+1.e-6)/frame->dt;

  /*------- first case: create a new output frame which could be the last... --*/
  if(*resizedFrame == NULL) {
    *resizedFrame = FrameReshapeNew(frame, nFrames, position);
    if(position == nFrames-1)
      {FrameReshapeEnd(*resizedFrame);
        output = *resizedFrame;
        *resizedFrame = NULL;}
    return(output);}

  /*-------------- second case, add the frame to the existing resized frame ---*/
  resizedEnd = newFrameLength + (*resizedFrame)->GTimeS;
  if(frame->GTimeS <= resizedEnd) {
    FrameReshapeAdd(*resizedFrame, frame);
    if(position == nFrames-1)
      {FrameReshapeEnd(*resizedFrame);
        output = *resizedFrame;
        *resizedFrame = NULL;}
    return(output);}

  /*----------------------- last case, add a frame to the next resized frame --*/
  FrameReshapeEnd(*resizedFrame);
  output = *resizedFrame;
  *resizedFrame = FrameReshapeNew(frame, nFrames, position);

  return(output);}

/*--------------------------------------------------------- FrameReshapeAdd--*/
int FrameReshapeAdd(FrameH *frame1, 
                    FrameH *frame2)
/*---------------------------------------------------------------------------*/
/* This function add frame 2 to frame 1. unsued part of frame2 are free.     */
/*---------------------------------------------------------------------------*/
{
  FrAdcData  *adc1, *adc2, *adcN;
  FrSimData  *sim1, *sim2, *simN;
  FrProcData *proc1, *proc2, *procN;
  FrCList *list;
  double t1,t2;

  if (frame1 == NULL) return(1);
  if (frame2 == NULL) return(1);

  FrameRemoveUntaggedData(frame1);
  FrameRemoveUntaggedData(frame2);

  /*----------------------check that the added frame is in the frame limit---*/
  t1 = frame1->GTimeS + 1.e-9*frame1->GTimeN;
  t2 = frame2->GTimeS + 1.e-9*frame2->GTimeN;
  if(t2+1.e-6 < t1)      return(2);
  if(t2 > t1+frame1->dt) return(3);

  /*---------------------------------------------------- merge header info---*/
  frame1->dataQuality = frame1->dataQuality & frame2->dataQuality;
  frame2->dataQuality = frame1->dataQuality;

  list = FrCListBldChannel(frame1);

  /*---------------------------------------------------- move the raw data---*/
  if (frame2->rawData != NULL) {
    if (frame1->rawData == NULL) frame1->rawData = FrRawDataNew(NULL);

    /*------------------------------------ convert SerData to ADC channels---*/
    FrameReshapeConvertSer(frame1, frame2, frame1->dt);

    /*---------------------------------------------- move the ADC channels---*/
    for(adc2 = frame2->rawData->firstAdc; adc2 != NULL; adc2 = adcN) {
      adc2->data->GTime = t2;
      adcN = adc2->next;
      adc1 = (FrAdcData *) FrCListFind(list, adc2->name); /*---match ADC's---*/

      /*-------------if the Adc is already in the list, move only the data---*/
      if(adc1 != NULL) {
	if(adc2->data->next == NULL) adc2->data->next = adc1->data;
	else                   adc2->data->next->next = adc1->data;
	adc1->data = adc2->data;
	adc2->data = NULL;
	FrAdcDataFreeOne(adc2);}
      else {                         /*---if not move the full adc channel---*/
	adc2->next = frame1->rawData->firstAdc;
	frame1->rawData->firstAdc = adc2;}

      if(adc1 != NULL) adc1 = adc1->next;}

    frame2->rawData->firstAdc = NULL;}

  /*------------------- Now move the FrProcData vectors like for the ADC's---*/
  for(proc2 = frame2->procData; proc2 != NULL; proc2 = procN) {
    proc2->data->GTime = t2;
    procN = proc2->next;
    proc1 = (FrProcData *) FrCListFind(list, proc2->name); /*--match PROC's-*/

    /*-------------if the Proc is already in the list, move only the data---*/
    if(proc1 != NULL) {
      if(proc2->data->next == NULL) proc2->data->next = proc1->data;
      else                    proc2->data->next->next = proc1->data;
      proc1->data = proc2->data;
      proc2->data = NULL;
      FrProcDataFreeOne(proc2);}
    else {                         /*---if not move the full proc channel---*/
      proc2->next = frame1->procData;
      frame1->procData = proc2;}

    if(proc1 != NULL) proc1 = proc1->next;}

  frame2->procData = NULL;

  /*--------------------- Now move the FrSimData vectors like for the ADC's--*/
  for(sim2 = frame2->simData; sim2 != NULL; sim2 = simN) {
    sim2->data->GTime = t2;
    simN = sim2->next;
    sim1 = (FrSimData *) FrCListFind(list, sim2->name); /*------match SIM's-*/

    /*--------------if the Sim is already in the list, move only the data---*/
    if(sim1 != NULL) {
      if(sim2->data->next == NULL) sim2->data->next = sim1->data;
      else                    sim2->data->next->next = sim1->data;
      sim1->data = sim2->data;
      sim2->data = NULL;
      FrSimDataFreeOne(sim2);}
    else {                         /*---if not move the full sim channel---*/
      sim2->next = frame1->simData;
      frame1->simData = sim2;}

    if(sim1 != NULL) sim1 = sim1->next;}

  frame2->simData = NULL;

  FrCListFree(list);

  FrHistoryFree(frame2->history);  /*--- we keep only the first frame history-*/
  frame2->history = NULL;

  /*------------------------------------------------ Copy the other channels--*/
  FrameMerge(frame1,frame2);

  return(0);
}

/*---------------------------------------------------------------------------*/
FrameH *FrameReshapeConvertSer(FrameH *trend,
                               FrameH *frame,
                               int trendPeriod)
/*---------------------------------------------------------------------------*/
/* This function build trend frame (i.e. convert all FrSerData from frame to 
   FrAdcData) of size trendPeriod (in seconds) starting from the frame *frame*/
/*---------------------------------------------------------------------------*/
{FrSerData *sms;
 FrAdcData *adc;
 FrVect  *isThere;
 FrCList *listTrend;
 int nData, nBits, i, index, period, nRead, nWhile, j, k;
 char *data, *start, word1[256], word2[256], word3[256], word4[256],  
      channelName[256], *units;
 float bias, slope;

 if (frame == NULL) return(NULL);
 if (frame->rawData == NULL) return(NULL);

 if(trendPeriod < 1) trendPeriod = 1;
 index = frame->GTimeS % trendPeriod;

 listTrend = FrCListBldAdc(trend);

               /*------------------------convert sms data to adc vectors ---*/

 for(sms = frame->rawData->firstSer; sms != NULL; sms = sms->next)
   {if (sms->sampleRate <= 0) sms->sampleRate = 1.;
    period = (0.5 + 1. / sms->sampleRate);
    i      = index / period;
    nData  = trendPeriod / period;
    if(nData < 1) nData = 1;

                /*-------------------- check if it is an old SMS format -----*/

    data = sms->data;
    if((start = strstr(data, " ALL ")) != NULL)
       {nRead = sscanf(start,"%s %s %s %s",word1, word2, word3, word4);
        if(nRead != 4) continue;
	data = strstr(start, word4);}

                /*------------------------- decode each individual blocks ---*/

    nWhile = 0;
    units  = NULL;
    bias   = 0.;
    slope  = 1.;
    while ((nRead = sscanf(data,"%s %s %s",word1, word2, word3)) >= 2) {
  
                /*-------------------------- save calibration info if any ---*/

      if     (strcmp(word1,"bias")  == 0)  {sscanf (word2,  "%e", &bias);} 
      else if(strcmp(word1,"slope") == 0)  {sscanf (word2,  "%e", &slope);} 
      else if(strcmp(word1,"units") == 0)  {
        if(units != NULL) free(units);
        FrStrCpy(&(units),word2);}

                /*---------------------------- do not stored delta values ---*/      

      else if(strcmp(word1,"delta") != 0)  {

                /*-------------- search the adc channel in the trend frame---*/

        sprintf(channelName,"%s_%s",sms->name, word1);
        adc = (FrAdcData *) FrCListFind(listTrend,channelName);

                /*----check that we get the rigth FrAdcData channel---*/

        while(adc != NULL) 
          {if((adc->sampleRate == sms->sampleRate) &&
              (adc->data->next != NULL)) break;
           sprintf(channelName,"%s_%gHz",channelName, sms->sampleRate);
           adc = (FrAdcData *) FrCListFind(listTrend,channelName);}

                /*------ the channel is not in the trend frame. Create it ---*/

        if (adc == NULL)
	  {if (word2[0] == 'x')                nBits = 32;
           else if(strncmp(word2,"0x",2) == 0) nBits = 32;
           else                                nBits =-32;

           adc = FrAdcDataNew(trend, channelName, sms->sampleRate, nData, nBits);
           if (adc == NULL) return(NULL);

	   for(j=0; j<adc->data->nData; j++) {adc->data->dataI[j] = 0;}

	   /*---------------add auxiliary vector -------------*/

           isThere = FrVectNew1D("Available_data",  FR_VECT_C,  nData, 
              adc->data->dx[0], adc->data->unitX[0], "1 if data is there");
           if(isThere == NULL) return(NULL);
           for(k=0; k<isThere->nData; k++) {isThere->data[k] = 0.;}
           adc->data->next = isThere;

	         /*------------------------------- copy calibration data ---*/

           if(units != NULL)
	     {adc->bias  = bias;
	      adc->slope = slope;
	      FrStrCpy(&(adc->units),units);
	      if(slope == 1) FrVectSetUnitY(adc->data, units);}}

                /*------------------------------------------- fill data ---*/

        if (i >= adc->data->nData) i = adc->data->nData-1;/*vector overflow.*/

	adc->data->next->data[i] ++;
        if (word2[0] == 'x')             
           sscanf (word2+1,"%x", &(adc->data->dataUI[i]));
        else if(strncmp(word2,"0x",2) == 0) 
           sscanf (word2,  "%x", &(adc->data->dataUI[i]));
        else if (isalpha((int) word2[0]) == 0) 
           sscanf (word2,  "%e", &(adc->data->dataF[i]));
        else                            
           sscanf (word2+1,"%e", &(adc->data->dataF[i]));}

      if (nRead != 3) break;
      data = strstr(data+strlen(word1)+strlen(word2), word3);}

    if(units != NULL) free(units);}

 FrCListFree(listTrend);

 FrSerDataFree(frame->rawData->firstSer);
 frame->rawData->firstSer = NULL;

 return(trend);
}
/*--------------------------------------------------------- FrameReshapeEnd--*/
int FrameReshapeEnd(FrameH *frame)
/*---------------------------------------------------------------------------*/
/* This function reformat all time series vector of the frame                */
/*---------------------------------------------------------------------------*/
{FrAdcData  *adc;
 FrSimData  *sim;
 FrProcData *proc;
 FrVect *isThere;
 double tStart;
 FRBOOL remove;
 int i;

 if (frame == NULL) return(1);

 tStart = frame->GTimeS + 1.e-9*frame->GTimeN;

 if (frame->rawData != NULL)
   {for(adc = frame->rawData->firstAdc; adc != NULL; adc = adc->next)
      {if(adc->data->next == NULL) continue;

                     /*--- first the case of SerData converted to ADC ---*/
       if(strncmp(adc->data->next->name,"Available_data",14) == 0) 
	  {isThere = adc->data->next;
           remove = FR_YES;
           for(i=0; i<isThere->nData; i++) 
              {if(isThere->data[i] != 1) remove = FR_NO;}
           if(remove == FR_YES)
              {FrVectFree(isThere);
               adc->data->next = NULL;}
           continue;}
                                              /*---- then regular ADCs ---*/
       adc->data = FrVectConcat(adc->data, tStart, frame->dt);
       if(adc->data == NULL) return(2);
       if(adc->data->next != NULL) adc->dataValid = adc->dataValid | 2;}}

 for(proc = frame->procData; proc != NULL; proc = proc->next) {
   if(proc->type < 2)     /*-- unknow types are assumed to be time series-*/
    {proc->data = FrVectConcat(proc->data, tStart, frame->dt);
     if(proc->data == NULL) return(3);}
   else   
    {proc->timeOffset = proc->data->GTime - tStart;}}

 for(sim = frame->simData; sim != NULL; sim = sim->next) {
   sim->data = FrVectConcat(sim->data, tStart, frame->dt);
   if(sim->data == NULL) return(4);}

 return(0);
}
/*----------------------------------------------------------FrameReshapeNew--*/
FrameH *FrameReshapeNew(FrameH *frame,
                        int nFrame,
                        int position)
/*---------------------------------------------------------------------------*/
{FrAdcData *adc;
 FrProcData *proc;
 FrSimData *sim;
 double start, gtime;
 char message[128];
 int GTimeN;

 if(nFrame  <= 0)      return(NULL);
 if(position < 0)      return(NULL);
 if(position > nFrame) return(NULL);

 sprintf(message,"FrameReshape (%.0fs -> %.0fs). "
     "Keep only the first frame history", 
      frame->dt,nFrame*frame->dt);
 FrHistoryAdd(frame, message);

    /*----Copy time information and remove auxiliary vector in all channels--*/

 gtime = frame->GTimeS + 1.e-9 * frame->GTimeN;

 if(frame->rawData != NULL) {
   for(adc = frame->rawData->firstAdc; adc != NULL; adc = adc->next) {
     adc->data->GTime = gtime;
     if(adc->data->next == NULL) continue;
     FrVectFree(adc->data->next);
     adc->data->next = NULL;}}

 for(proc = frame->procData; proc != NULL; proc = proc->next) {
     proc->data->GTime = gtime;
     if(proc->data->next == NULL) continue;
     FrVectFree(proc->data->next);
     proc->data->next = NULL;}

 for(sim = frame->simData; sim != NULL; sim = sim->next) {
     sim->data->GTime = gtime;
     if(sim->data->next == NULL) continue;
     FrVectFree(sim->data->next);
     sim->data->next = NULL;}
                                          /*-------- Update frame header ----*/
 if(position != 0)
  {start = gtime - position*frame->dt;
   frame->GTimeS = start;
   GTimeN = 1.e+9*(start - (double) frame->GTimeS);
   if(GTimeN < 0) GTimeN = 0;
   frame->GTimeN = GTimeN;}
 
 frame->dt = nFrame*frame->dt;
                                                  /*----- convert SerData ---*/
 FrameReshapeConvertSer(frame, frame, frame->dt);

 return(frame);}

/*--------------------------------------------------------- FrameSetPrefix---*/
void FrameSetPrefix(FrameH *frame, char *prefix) 
/*---------------------------------------------------------------------------*/
{FrAdcData *adc;
 FrSerData *sms;
 FrProcData *proc;

 if(prefix         == NULL) return;
 if(frame          == NULL) return;

 if(frame->rawData != NULL)
   {for(adc = frame->rawData->firstAdc; adc != NULL; adc = adc->next) 
     {FrStrSetPrefix(&(adc->name),prefix);
      if(adc->data != NULL)
        FrStrSetPrefix(&(adc->data->name),prefix);}

    for(sms = frame->rawData->firstSer; sms != NULL; sms = sms->next) 
      {FrStrSetPrefix(&(sms->name),prefix);}}

 for(proc = frame->procData; proc != NULL; proc = proc->next) 
    {FrStrSetPrefix(&(proc->name),prefix);
     if(proc->data != NULL) 
       FrStrSetPrefix(&(proc->data->name),prefix);}

 return;}

/*----------------------------------------------------------------FrameStat--*/
int FrameStat(FrameH *frame, FILE *fp)
/*---------------------------------------------------------------------------*/
{FrAdcData *adc;
#define MAXTYPE 100
 int nType, nSample[MAXTYPE],storage[MAXTYPE],nAdc[MAXTYPE];
 FRLONG totSize,i, size;

 if(frame == NULL) return(0);
 if(frame->rawData == NULL) return(0);

 nType = 0;
 for(adc = frame->rawData->firstAdc; adc != NULL; adc = adc->next)
   {if(adc->data == NULL) continue;
    for(i=0; i<nType; i++)
       {if((nSample[i] == adc->data->nData) && 
           (storage[i] == adc->data->wSize)) break;}

    if(i >= nType)
      {if(i == MAXTYPE) continue;
       nSample[i] = adc->data->nData;
       storage[i] = adc->data->wSize;
       nAdc   [i] = 0;
       nType++;}

    nAdc[i] ++;}

 if (fp == NULL) return(0);
 totSize = 0;
 for(i=0; i<nType; i++) {totSize += nSample[i]*nAdc[i]*storage[i];}

 fprintf(fp," Frame statistic: Total ADC data size: %"FRLLD" bytes\n",totSize);
 for(i=0; i<nType; i++)
  {size = nSample[i]*nAdc[i]*storage[i];
   fprintf(fp, " %6d ADC with%7d samples of %d bytes."
          " Total size=%8"FRLLD" (%5.2f%%)\n",
	  nAdc[i],nSample[i],storage[i],size,100.*size/totSize);}

 return(totSize);}

/*-----------------------------------------------------------------FrameTag--*/
void FrameTag(FrameH *frame,
              char *tag)
/*---------------------------------------------------------------------------*/
{
  char *subTag;

  subTag = FrTagGetSubTag("#ADC",tag);
  FrameTagAdc   (frame, subTag); 
  if(subTag != NULL) free(subTag);

  subTag = FrTagGetSubTag("#MSG",tag);
  FrameTagMsg   (frame, subTag);
  if(subTag != NULL) free(subTag);

  subTag = FrTagGetSubTag("#SER",tag);
  FrameTagSer   (frame, subTag);
  if(subTag != NULL) free(subTag);

  subTag = FrTagGetSubTag("#SIM",tag);
  FrameTagSim   (frame, subTag);
  if(subTag != NULL) free(subTag);

  subTag = FrTagGetSubTag("#SIMEVENT",tag);
  FrameTagSimEvt(frame, subTag);
  if(subTag != NULL) free(subTag);

  subTag = FrTagGetSubTag("#PROC",tag);
  FrameTagProc  (frame, subTag);
  if(subTag != NULL) free(subTag);

  subTag = FrTagGetSubTag("#EVENT",tag);
  FrameTagEvent (frame, subTag);
  if(subTag != NULL) free(subTag);

  subTag = FrTagGetSubTag("#SUM",tag);
  FrameTagSum   (frame, subTag);
  if(subTag != NULL) free(subTag);

  subTag = FrTagGetSubTag("#STAT",tag);
  FrameTagStat  (frame, subTag);
  if(subTag != NULL) free(subTag);

  return;}

/*--------------------------------------------------------------FrameTagAdc--*/
void FrameTagAdc(FrameH *frame,
                 char *tag)
/*---------------------------------------------------------------------------*/
{
 if(frame == NULL) return;
 if(frame->rawData == NULL) return;

 FrameTagBasic((FrBasic**) &frame->rawData->firstAdc,
               (FrBasic**) &frame->rawData->firstAdcOld, tag);

 return;}

/*------------------------------------------------------------FrameTagBasic--*/
void FrameTagBasic(FrBasic**root,
                   FrBasic**rootOld,
                   char *tag)
/*---------------------------------------------------------------------------*/
{FrBasic **current, *adc;
 FrTag *frtag;

 if(tag == NULL) return;

 if(*rootOld != NULL) FrameUntagBasic(root, rootOld);

 if(*root == NULL) return;

              /*------------------- save the current link list --------------*/

 *rootOld = *root;
 adc = *root;
 while(adc != NULL)
    {adc->nextOld = adc->next;
     adc = adc->next;}

             /*--------- keep in the link list only the requested object ----*/

 if(strcmp("NONE", tag) == 0)
   {*root = NULL;
    return;}

 frtag = FrTagNew(tag);
 if(frtag == NULL) return;

 current = root;
 while(*current != NULL)
   {if(FrTagMatch(frtag,(*current)->name)  == FR_NO)
        {*current = (*current)->next;}
    else{current = &((*current)->next);}}

 FrTagFree(frtag); 

 return;}
/*------------------------------------------------------------FrameTagEvent--*/
void FrameTagEvent(FrameH *frame,
                   char *tag)
/*---------------------------------------------------------------------------*/
{
 if(frame == NULL) return;

 FrameTagBasic((FrBasic**) &frame->event,
               (FrBasic**) &frame->eventOld, tag);

 return;}

/*--------------------------------------------------------------FrameTagMsg--*/
void FrameTagMsg(FrameH *frame,
                 char *tag)
/*---------------------------------------------------------------------------*/
{
 if(frame == NULL) return;
 if(frame->rawData == NULL) return;

 FrameTagBasic((FrBasic**) &frame->rawData->logMsg,
               (FrBasic**) &frame->rawData->logMsgOld, tag);

 return;}

/*-------------------------------------------------------------FrameTagProc--*/
void FrameTagProc(FrameH *frame,
                  char *tag)
/*---------------------------------------------------------------------------*/
{
 if(frame == NULL) return;

 FrameTagBasic((FrBasic**) &frame->procData,
               (FrBasic**) &frame->procDataOld, tag);

 return;}

/*--------------------------------------------------------------FrameTagSer--*/
void FrameTagSer(FrameH *frame,
                 char *tag)
/*---------------------------------------------------------------------------*/
{
 if(frame == NULL) return;
 if(frame->rawData == NULL) return;

 FrameTagBasic((FrBasic**) &frame->rawData->firstSer,
               (FrBasic**) &frame->rawData->firstSerOld, tag);

 return;}

/*--------------------------------------------------------------FrameTagSim--*/
void FrameTagSim(FrameH *frame,
                 char *tag)
/*---------------------------------------------------------------------------*/
{
 if(frame == NULL) return;

 FrameTagBasic((FrBasic**) &frame->simData,
               (FrBasic**) &frame->simDataOld, tag);

 return;}

/*-----------------------------------------------------------FrameTagSimEvt--*/
void FrameTagSimEvt(FrameH *frame,
                    char *tag)
/*---------------------------------------------------------------------------*/
{
 if(frame == NULL) return;

 FrameTagBasic((FrBasic**) &frame->simEvent,
               (FrBasic**) &frame->simEventOld, tag);

 return;}

/*--------------------------------------------------------------FrameTagStat-*/
void FrameTagStat(FrameH *frame,
                  char *tag)
/*---------------------------------------------------------------------------*/
{FrDetector *det;

 if(frame == NULL) return;

 for(det = frame->detectProc; det != NULL; det = det->next)
   {FrameTagBasic((FrBasic**) &det->sData,
                  (FrBasic**) &det->sDataOld, tag);}

 for(det = frame->detectSim; det != NULL; det = det->next)
   {FrameTagBasic((FrBasic**) &det->sData,
                  (FrBasic**) &det->sDataOld, tag);}

 return;}

/*--------------------------------------------------------------FrameTagSum--*/
void FrameTagSum(FrameH *frame,
                 char *tag)
/*---------------------------------------------------------------------------*/
{
 if(frame == NULL) return;

 FrameTagBasic((FrBasic**) &frame->summaryData,
               (FrBasic**) &frame->summaryDataOld, tag);

 return;}

/*---------------------------------------------------------------FrameUntag--*/
void FrameUntag(FrameH *frame)
/*---------------------------------------------------------------------------*/
{
 if(frame == NULL) return;

 FrameUntagAdc   (frame);
 FrameUntagMsg   (frame);
 FrameUntagEvent (frame);
 FrameUntagProc  (frame);
 FrameUntagSer   (frame);
 FrameUntagSim   (frame);
 FrameUntagSimEvt(frame);
 FrameUntagSum   (frame);
 FrameUntagStat  (frame);

 return;}
/*------------------------------------------------------------FrameUntagAdc--*/
void FrameUntagAdc(FrameH *frame)
/*---------------------------------------------------------------------------*/
{
 if(frame == NULL) return;
 if(frame->rawData != NULL)  FrRawDataUntagAdc(frame->rawData);

 return;}

/*----------------------------------------------------------FrameUntagBasic--*/
void FrameUntagBasic(FrBasic **root, 
                     FrBasic **rootOld)
/*---------------------------------------------------------------------------*/
{FrBasic *adc;

 if(*rootOld == NULL) return;

 *root = *rootOld;
 *rootOld = NULL;

 adc = *root;
 while(adc != NULL)
    {adc->next = adc->nextOld;
     adc = adc->next;}

 return;}

/*----------------------------------------------------------FrameUntagEvent--*/
void FrameUntagEvent(FrameH *frame)
/*---------------------------------------------------------------------------*/
{
 if(frame == NULL) return;
 FrameUntagBasic((FrBasic **) &frame->event,
                 (FrBasic **) &frame->eventOld);

 return;}

/*------------------------------------------------------------FrameUntagMsg--*/
void FrameUntagMsg(FrameH *frame)
/*---------------------------------------------------------------------------*/
{
 if(frame == NULL) return;
 if(frame->rawData != NULL)  FrRawDataUntagMsg(frame->rawData);

 return;}
/*-----------------------------------------------------------FrameUntagProc--*/
void FrameUntagProc(FrameH *frame)
/*---------------------------------------------------------------------------*/
{
 if(frame == NULL) return;
 FrameUntagBasic((FrBasic **) &frame->procData, 
                 (FrBasic **) &frame->procDataOld );

 return;}

/*------------------------------------------------------------FrameUntagSer--*/
void FrameUntagSer(FrameH *frame)
/*---------------------------------------------------------------------------*/
{
 if(frame == NULL) return;
 if(frame->rawData != NULL) FrRawDataUntagSer(frame->rawData);

 return;}

/*------------------------------------------------------------FrameUntagSim--*/
void FrameUntagSim(FrameH *frame)
/*---------------------------------------------------------------------------*/
{
 if(frame == NULL) return;
 FrameUntagBasic((FrBasic **) &frame->simData,
                 (FrBasic **) &frame->simDataOld);

 return;}

/*---------------------------------------------------------FrameUntagSimEvt--*/
void FrameUntagSimEvt(FrameH *frame)
/*---------------------------------------------------------------------------*/
{
 if(frame == NULL) return;
 FrameUntagBasic((FrBasic **) &frame->simEvent,
                 (FrBasic **) &frame->simEventOld);

 return;}

/*------------------------------------------------------------FrameUntagStat-*/
void FrameUntagStat(FrameH *frame)
/*---------------------------------------------------------------------------*/
{FrDetector *det;

 if(frame == NULL) return;

 for(det = frame->detectProc; det != NULL; det = det->next)
   {FrDetectorUntagStat(det);}

 for(det = frame->detectSim; det != NULL; det = det->next)
   {FrDetectorUntagStat(det);}

 return;}

/*------------------------------------------------------------FrameUntagSum--*/
void FrameUntagSum(FrameH *frame)
/*---------------------------------------------------------------------------*/
{
 if(frame == NULL) return;
 FrameUntagBasic((FrBasic **) &frame->summaryData,
                 (FrBasic **) &frame->summaryDataOld);

 return;}

/*---------------------------------------------------------------FrameWrite--*/
int FrameWrite(FrameH *frameH,
               FrFile *oFile)
/*---------------------------------------------------------------------------*/
{FrHistory *history;
 
 if(oFile == NULL) 
     {return(FR_ERROR_NO_FILE);}
 if(frameH == NULL) 
     {oFile->error = FR_ERROR_NO_FRAME;
      return(oFile->error);}
 oFile->error = FR_OK;

 if(FrDebugLvl > 1)
    {fprintf(FrFOut,"Output frame\n  Run:%d Frame:%d \n",
         frameH->run,frameH->frame);}

                  /*---- should we reopen the file (multiple output files)?--*/
 
 if(oFile->fLength > 0) FrFileOReopen(oFile, frameH->GTimeS);
 if(oFile->error != FR_OK) return( oFile->error);
 oFile->currentEndTime = frameH->GTimeS+(1.e-9*frameH->GTimeN+frameH->dt);

                  /*---------------------- write the file header if needed---*/

 if(oFile->pStart == NULL) FrFileOOpen(oFile);

                  /*----------------------------------- add history record --*/

 if(oFile->historyMsg != NULL) 
      history = FrHistoryAdd(frameH, oFile->historyMsg);
 else history = NULL;

                  /*---------------------------------reset instance numbers--*/
 FrDicIni(oFile);
                  /*----------------------------------- write FrameH struct--*/

 FrameHWrite(frameH, oFile);

                 /*---------------------------------------- write user info--*/

 if(frameH->type != NULL) FrVectWrite (frameH->type, oFile);
 if(frameH->user != NULL) FrVectWrite (frameH->user, oFile);
 
                /*-------------------------- write the detector information--*/
 
 if(frameH->detectSim != NULL) FrDetectorWrite(frameH->detectSim, oFile);
 if(frameH->detectProc!= NULL) FrDetectorWrite(frameH->detectProc,oFile);

                /*--------------------------------------- write the history--*/
 
 FrHistoryWrite(frameH->history, oFile);

                /*---------------------------------------write the raw data--*/
 
 if(frameH->rawData != NULL) FrRawDataWrite(frameH->rawData, oFile);

                /*---------------------------- write the reconstructed data--*/

 if(frameH->procData != NULL) FrProcDataWrite(frameH->procData, oFile);   

               /*--------------------------------- write the simulated data--*/

 if(frameH->simData != NULL) FrSimDataWrite(frameH->simData, oFile);   

               /*-------------------------- write experiment dependant data--*/

 if(frameH->event      != NULL) FrEventWrite   (frameH->event,      oFile);
 if(frameH->simEvent   != NULL) FrSimEventWrite(frameH->simEvent,   oFile);
 if(frameH->summaryData!= NULL) FrSummaryWrite (frameH->summaryData,oFile);
 if(frameH->auxData    != NULL) FrVectWrite    (frameH->auxData,    oFile);
 if(frameH->auxTable   != NULL) FrTableWrite   (frameH->auxTable,   oFile);

               /*---------------------------- write the end of frame record--*/

 FrEndOfFrameWrite(frameH, oFile);

               /*--------------------------- delete the history we just add--*/

 if(history != NULL) 
   {history = frameH->history->next;
    frameH->history->next = NULL;
    FrHistoryFree(frameH->history);
    frameH->history = history;}

 oFile->nFrames++;

                  /*---- should we close the file (multiple output files)?--*/
 if(oFile->fLength > 0) {
   if(oFile->closingTime <= oFile->currentEndTime) FrFileOReopen(oFile, -1);}

 if(oFile->error   != FR_OK) FrError(3,"FrameWrite",FrErrorMsg[oFile->error]);

 return(oFile->error);}

/*----------------------------------------------------------FrameWriteToBuf--*/
FRLONG FrameWriteToBuf(FrameH *frame,
                       int  comp,
                       char *buf, 
                       FRLONG nBytes)
/*---------------------------------------------------------------------------*/
{FrFile *file;
 FRLONG size, status;
 FRBOOL checksum;

 if(buf == NULL) return(0);
 if(nBytes <= 0) return(0);

 if(comp < -1) { /*------ if comp is negatif; checksum are computed---*/
   checksum = FR_YES;
   comp = -comp;}
 else {
   checksum = FR_NO;}

 file = FrFileNew(NULL, comp, buf, nBytes); /*-- create a temporary FrFile---*/
 if(file == NULL) return(0);

 file->chkSumFiFlag = checksum;
 file->chkSumFrFlag = checksum;  
	 
 status = FrameWrite(frame, file);         /*------------ write frame -------*/
 if(status != FR_OK) return (0);

 size = FrFileOEnd(file);
 if(status != FR_OK) return (0);
 
 return(size);}

/*---------------------------------------------------------------FrameHCopy--*/
FrameH *FrameHCopy(FrameH *in)
/*---------------------------------------------------------------------------*/
{FrameH *out;

  if(in == NULL) return(NULL); 
  out = FrameHNew(in->name);
  if(out == NULL) 
       {FrError(3,"FrameHCopy","malloc failed");
        return (NULL);} 

  out->run         = in->run;
  out->frame       = in->frame;
  out->dataQuality = in ->dataQuality;
  out->GTimeS      = in->GTimeS;
  out->GTimeN      = in->GTimeN;
  out->ULeapS      = in->ULeapS;
  out->dt          = in->dt;

  return(out);} 

/*----------------------------------------------------------------FrameHDef--*/
FrSH *FrameHDef()    
/*---------------------------------------------------------------------------*/
{static FrSH *classe = NULL;

  if(classe != NULL) return(classe);
  classe = FrDicAddS("FrameH", (void (*)())FrameHRead);

  FrSENew(classe, "STRING", "name","-");
  FrSENew(classe, "INT_4S", "run","-");
  FrSENew(classe, "INT_4U", "frame","-");
  FrSENew(classe, "INT_4U", "dataQuality","-");
  FrSENew(classe, "INT_4U", "GTimeS","-");
  FrSENew(classe, "INT_4U", "GTimeN","-");
  FrSENew(classe, "INT_2U", "ULeapS","-");
  FrSENew(classe, "REAL_8", "dt","-");
  FrSENew(classe, "PTR_STRUCT(FrVect *)", "type","-");
  FrSENew(classe, "PTR_STRUCT(FrVect *)", "user","-");
  FrSENew(classe, "PTR_STRUCT(FrDetector *)", "detectSim",  "-");
  FrSENew(classe, "PTR_STRUCT(FrDetector *)", "detectProc", "-");
  FrSENew(classe, "PTR_STRUCT(FrHistory *)",  "history",    "-");
  FrSENew(classe, "PTR_STRUCT(FrRawData *)",  "rawData",    "-");
  FrSENew(classe, "PTR_STRUCT(FrProcData *)", "procData",   "-");
  FrSENew(classe, "PTR_STRUCT(FrSimData *)",  "simData",    "-");
  FrSENew(classe, "PTR_STRUCT(FrEvent *)",    "event",      "-");
  FrSENew(classe, "PTR_STRUCT(FrSimEvent *)", "simEvent",   "-");
  FrSENew(classe, "PTR_STRUCT(FrSummary *)",  "summaryData","-");
  FrSENew(classe, "PTR_STRUCT(FrVect *)",     "auxData",    "-");
  FrSENew(classe, "PTR_STRUCT(FrTable *)",    "auxTable",   "-");
  if(FrFormatVersion >= 8) FrSENew(classe, "INT_4U", "chkSum","-");

  return(classe);}
  
/*--------------------------------------------------------------- FrameHNew--*/
FrameH *FrameHNew(char *name)
/*---------------------------------------------------------------------------*/
{FrameH *frame;
 
  frame = (FrameH *) calloc(1,sizeof(FrameH));
  if(frame == NULL)   return (NULL); 
  frame->classe = FrameHDef(); 

  FrStrCpy(&frame->name,name);

  frame->type        = NULL;
  frame->user        = NULL;
  frame->detectProc  = NULL;
  frame->detectSim   = NULL;
  frame->history     = NULL;
  frame->rawData     = NULL;
  frame->procData    = NULL;
  frame->simData     = NULL;
  frame->event       = NULL;
  frame->simEvent    = NULL;
  frame->summaryData = NULL;
  frame->auxData     = NULL;
  frame->auxTable    = NULL;

  return(frame);} 

/*-------------------------------------------------------------- FrameHRead--*/
FrameH *FrameHRead(FrFile *iFile)
/*---------------------------------------------------------------------------*/
{FrameH *frameH;
 unsigned int   id4;
 unsigned short id2;
 int dummyStrain;

 if(iFile->fmtVersion == 3) return(FrBack3FrameHRead(iFile));

  frameH = (FrameH *) calloc(1,sizeof(FrameH));
  if(frameH == NULL) 
       {iFile->error = FR_ERROR_MALLOC_FAILED;
        return(NULL);}
  frameH->classe = FrameHDef(); 
  
  if(iFile->fmtVersion > 5)
       {FrReadIntU  (iFile, &id4);}   /*frameH are not referenced by pointer*/
  else {FrReadShortU(iFile, &id2);}
  FrReadSChar (iFile, &frameH->name); 
  FrReadInt   (iFile, &frameH->run);
  FrReadIntU  (iFile, &frameH->frame);
  FrReadIntU  (iFile, &frameH->dataQuality);
  FrReadIntU  (iFile, &frameH->GTimeS);
  FrReadIntU  (iFile, &frameH->GTimeN);
  FrReadShortU(iFile, &frameH->ULeapS);
  if(iFile->fmtVersion < 5)
    {FrReadInt(iFile, &iFile->oldLocalTime);}
  FrReadDouble(iFile, &frameH->dt);
  FrReadStruct(iFile, &frameH->type);
  FrReadStruct(iFile, &frameH->user);
  FrReadStruct(iFile, &frameH->detectSim);
  FrReadStruct(iFile, &frameH->detectProc);
  FrReadStruct(iFile, &frameH->history);
  FrReadStruct(iFile, &frameH->rawData);
  FrReadStruct(iFile, &frameH->procData);
  if(iFile->fmtVersion < 5)
    {FrReadInt(iFile, &dummyStrain);}
  FrReadStruct(iFile, &frameH->simData);
  FrReadStruct(iFile, &frameH->event);
  FrReadStruct(iFile, &frameH->simEvent);
  FrReadStruct(iFile, &frameH->summaryData);
  FrReadStruct(iFile, &frameH->auxData);
  FrReadStruct(iFile, &frameH->auxTable);
  if(iFile->fmtVersion >=8) FrReadStructChksum(iFile);

  if(FrDebugLvl > 0) fprintf(FrFOut," FrameHRead: Run:%d Frame:%d GTimeS:%d\n",
                            frameH->run, frameH->frame, frameH->GTimeS);

  iFile->curFrame = frameH;

  return(frameH);}
/*--------------------------------------------------------------FrameHReadN--*/
FrameH *FrameHReadN(FrFile *iFile,
                    int rNumber,
                    int fNumber)
/*---------------------------------------------------------------------------*/
{int i;

 i = FrTOCFrameFindN(iFile, rNumber, fNumber);

                 /*----------Could not access frame using the TOC------------*/

 if(i < 0) return(NULL);

                 /*------------------ We just read the frameH structure -----*/

 if(FrTOCSetPos(iFile, iFile->toc->positionH[i]) != 0) return (NULL);
    
 FrameHRead(iFile);
 
 return(iFile->curFrame);}

/*--------------------------------------------------------------FrameHReadT--*/
FrameH *FrameHReadT(FrFile *iFile,
                    double gtime)
/*---------------------------------------------------------------------------*/
{int i;

 i = FrTOCFrameFindT(iFile, gtime);

                 /*----------Could not access frame using the TOC------------*/

 if(i <0) return(NULL);

                 /*------------------ We just read the frameH structure -----*/

 if(FrTOCSetPos(iFile, iFile->toc->positionH[i]) != 0) return (NULL);
    
 FrameHRead(iFile);
 
 return(iFile->curFrame);}

/*--------------------------------------------------------------FrameHWrite--*/
void FrameHWrite(FrameH *frameH,
                 FrFile *oFile)
/*---------------------------------------------------------------------------*/
{

 FrTOCFrame(oFile,frameH);

 FrPutNewRecord(oFile, frameH, FR_NO);

 FrPutSChar( oFile, frameH->name);
 FrPutInt(   oFile, frameH->run);
 FrPutIntU(  oFile, frameH->frame);
 FrPutIntU(  oFile, frameH->dataQuality);
 FrPutIntU(  oFile, frameH->GTimeS);
 FrPutIntU(  oFile, frameH->GTimeN);
 FrPutShortU(oFile, frameH->ULeapS);
 FrPutDouble(oFile, frameH->dt);          
 FrPutStruct(oFile, frameH->type);          
 FrPutStruct(oFile, frameH->user);          
 FrPutStruct(oFile, frameH->detectSim);
 FrPutStruct(oFile, frameH->detectProc);
 FrPutStruct(oFile, frameH->history);
 FrPutStruct(oFile, frameH->rawData);
 FrPutStruct(oFile, frameH->procData);
 FrPutStruct(oFile, frameH->simData);
 FrPutStruct(oFile, frameH->event);
 FrPutStruct(oFile, frameH->simEvent);
 FrPutStruct(oFile, frameH->summaryData);
 FrPutStruct(oFile, frameH->auxData);
 FrPutStruct(oFile, frameH->auxTable);

 oFile->nBytesF  = oFile->nBytes;
 oFile->chkSumFr = 0;

 FrPutWriteRecord(oFile, FR_NO);

 return;}

/*---------------------------------------------------------------------------*/
unsigned int FrChkSum(FRSCHAR *buf, int nBytes, int start)
/*---------------------------------------------------------------------------*/
/* This function compute the checksum for the buffer buf which has a size
(in bytes) of nBytes. The start variable is used to tell the alignment of buf
-----------------------------------------------------------------------------*/
{unsigned int i, n, shift, first, chksum;

 chksum = 0;
 first = start%4;

          /*------- deal with the first bytes to be 4 bytes aligned --------*/

 if(first != 0) {
   n = 4-first;
   if(n > nBytes) n = nBytes;
   shift = 8*first;

   for(i=0; i<n; i++) {
      chksum += buf[i]<<shift;
      shift = (shift+8)%32;}

   nBytes -= n;
   buf    += n;}

          /*-------- deal with the nicely aligned words (this could be 
             a plain sum on a computer with the right bytes order)-----------*/

 n = nBytes - nBytes%4;
 for(i=0; i<n; i=i+4) {
     chksum += buf[i];
     chksum += buf[i+1]<<8;
     chksum += buf[i+2]<<16;
     chksum += buf[i+3]<<24;}
 nBytes -= n;
 buf    += n;

        /*------------ deal with the extra non aligned bytes ----------------*/

 if(nBytes > 0) {chksum += buf[0];}
 if(nBytes > 1) {chksum += buf[1]<<8;}
 if(nBytes > 2) {chksum += buf[2]<<16;}

 return(chksum);
}
/*---------------------------------------------------------------FrCksumGnu--*/
void FrCksumGnu (char *buf, FRLONG nBytes, unsigned int *crc)
/*---------------------------------------------------------------------------*/
/* This function compute the checksum as defined by the GNU cksum tool       */
/* buf != 1 is used to feed nBytes of data from the input buffer buf.        */
/* buf == 0 is used for the last call.                                       */
/*                  nBytes is then the total number of bytes                 */
/*---------------------------------------------------------------------------*/
/* This test program should give the same result as the GNU cksum tool
int main (int argc, char **argv)
{ register FILE *fp;
# define BUFLEN 1024
  char buf[BUFLEN];
  unsigned int crc = 0, nBytes = 0, bytes_read;

  if ((fp = fopen (argv[1], "r")) == NULL) exit(0);
 
  while ((bytes_read = fread (buf, 1, BUFLEN, fp)) > 0)
    {FrCksumGnu(buf, bytes_read, &crc);
     nBytes += bytes_read;}

  FrCksumGnu(NULL, nBytes, &crc);

  printf ("crc: %lu length: %ld file:%s\n", crc, nBytes, argv[1]);

  exit (0);
-----------------------------------------------------------------------------*/
{unsigned char *cp = (unsigned char *)buf;
 register unsigned int mycrc; /* needs to be 4 bytes */
 FRULONG n;
 unsigned long const crctab[256] =
{
  0x0,
  0x04C11DB7, 0x09823B6E, 0x0D4326D9, 0x130476DC, 0x17C56B6B,
  0x1A864DB2, 0x1E475005, 0x2608EDB8, 0x22C9F00F, 0x2F8AD6D6,
  0x2B4BCB61, 0x350C9B64, 0x31CD86D3, 0x3C8EA00A, 0x384FBDBD,
  0x4C11DB70, 0x48D0C6C7, 0x4593E01E, 0x4152FDA9, 0x5F15ADAC,
  0x5BD4B01B, 0x569796C2, 0x52568B75, 0x6A1936C8, 0x6ED82B7F,
  0x639B0DA6, 0x675A1011, 0x791D4014, 0x7DDC5DA3, 0x709F7B7A,
  0x745E66CD, 0x9823B6E0, 0x9CE2AB57, 0x91A18D8E, 0x95609039,
  0x8B27C03C, 0x8FE6DD8B, 0x82A5FB52, 0x8664E6E5, 0xBE2B5B58,
  0xBAEA46EF, 0xB7A96036, 0xB3687D81, 0xAD2F2D84, 0xA9EE3033,
  0xA4AD16EA, 0xA06C0B5D, 0xD4326D90, 0xD0F37027, 0xDDB056FE,
  0xD9714B49, 0xC7361B4C, 0xC3F706FB, 0xCEB42022, 0xCA753D95,
  0xF23A8028, 0xF6FB9D9F, 0xFBB8BB46, 0xFF79A6F1, 0xE13EF6F4,
  0xE5FFEB43, 0xE8BCCD9A, 0xEC7DD02D, 0x34867077, 0x30476DC0,
  0x3D044B19, 0x39C556AE, 0x278206AB, 0x23431B1C, 0x2E003DC5,
  0x2AC12072, 0x128E9DCF, 0x164F8078, 0x1B0CA6A1, 0x1FCDBB16,
  0x018AEB13, 0x054BF6A4, 0x0808D07D, 0x0CC9CDCA, 0x7897AB07,
  0x7C56B6B0, 0x71159069, 0x75D48DDE, 0x6B93DDDB, 0x6F52C06C,
  0x6211E6B5, 0x66D0FB02, 0x5E9F46BF, 0x5A5E5B08, 0x571D7DD1,
  0x53DC6066, 0x4D9B3063, 0x495A2DD4, 0x44190B0D, 0x40D816BA,
  0xACA5C697, 0xA864DB20, 0xA527FDF9, 0xA1E6E04E, 0xBFA1B04B,
  0xBB60ADFC, 0xB6238B25, 0xB2E29692, 0x8AAD2B2F, 0x8E6C3698,
  0x832F1041, 0x87EE0DF6, 0x99A95DF3, 0x9D684044, 0x902B669D,
  0x94EA7B2A, 0xE0B41DE7, 0xE4750050, 0xE9362689, 0xEDF73B3E,
  0xF3B06B3B, 0xF771768C, 0xFA325055, 0xFEF34DE2, 0xC6BCF05F,
  0xC27DEDE8, 0xCF3ECB31, 0xCBFFD686, 0xD5B88683, 0xD1799B34,
  0xDC3ABDED, 0xD8FBA05A, 0x690CE0EE, 0x6DCDFD59, 0x608EDB80,
  0x644FC637, 0x7A089632, 0x7EC98B85, 0x738AAD5C, 0x774BB0EB,
  0x4F040D56, 0x4BC510E1, 0x46863638, 0x42472B8F, 0x5C007B8A,
  0x58C1663D, 0x558240E4, 0x51435D53, 0x251D3B9E, 0x21DC2629,
  0x2C9F00F0, 0x285E1D47, 0x36194D42, 0x32D850F5, 0x3F9B762C,
  0x3B5A6B9B, 0x0315D626, 0x07D4CB91, 0x0A97ED48, 0x0E56F0FF,
  0x1011A0FA, 0x14D0BD4D, 0x19939B94, 0x1D528623, 0xF12F560E,
  0xF5EE4BB9, 0xF8AD6D60, 0xFC6C70D7, 0xE22B20D2, 0xE6EA3D65,
  0xEBA91BBC, 0xEF68060B, 0xD727BBB6, 0xD3E6A601, 0xDEA580D8,
  0xDA649D6F, 0xC423CD6A, 0xC0E2D0DD, 0xCDA1F604, 0xC960EBB3,
  0xBD3E8D7E, 0xB9FF90C9, 0xB4BCB610, 0xB07DABA7, 0xAE3AFBA2,
  0xAAFBE615, 0xA7B8C0CC, 0xA379DD7B, 0x9B3660C6, 0x9FF77D71,
  0x92B45BA8, 0x9675461F, 0x8832161A, 0x8CF30BAD, 0x81B02D74,
  0x857130C3, 0x5D8A9099, 0x594B8D2E, 0x5408ABF7, 0x50C9B640,
  0x4E8EE645, 0x4A4FFBF2, 0x470CDD2B, 0x43CDC09C, 0x7B827D21,
  0x7F436096, 0x7200464F, 0x76C15BF8, 0x68860BFD, 0x6C47164A,
  0x61043093, 0x65C52D24, 0x119B4BE9, 0x155A565E, 0x18197087,
  0x1CD86D30, 0x029F3D35, 0x065E2082, 0x0B1D065B, 0x0FDC1BEC,
  0x3793A651, 0x3352BBE6, 0x3E119D3F, 0x3AD08088, 0x2497D08D,
  0x2056CD3A, 0x2D15EBE3, 0x29D4F654, 0xC5A92679, 0xC1683BCE,
  0xCC2B1D17, 0xC8EA00A0, 0xD6AD50A5, 0xD26C4D12, 0xDF2F6BCB,
  0xDBEE767C, 0xE3A1CBC1, 0xE760D676, 0xEA23F0AF, 0xEEE2ED18,
  0xF0A5BD1D, 0xF464A0AA, 0xF9278673, 0xFDE69BC4, 0x89B8FD09,
  0x8D79E0BE, 0x803AC667, 0x84FBDBD0, 0x9ABC8BD5, 0x9E7D9662,
  0x933EB0BB, 0x97FFAD0C, 0xAFB010B1, 0xAB710D06, 0xA6322BDF,
  0xA2F33668, 0xBCB4666D, 0xB8757BDA, 0xB5365D03, 0xB1F740B4
};

 if(nBytes <= 0) return;

 mycrc = *crc;
 /* implement Duff's deivce to unroll the loop */
 if(buf != NULL)
   {n = (nBytes + 15) / 16;
    switch( nBytes % 16 ) {
         case 0:    do { mycrc = (mycrc << 8) ^ crctab[(((mycrc) >> 24) ^ *(cp++))];
         case 15:         mycrc = (mycrc << 8) ^ crctab[(((mycrc) >> 24) ^ *(cp++))];
         case 14:         mycrc = (mycrc << 8) ^ crctab[(((mycrc) >> 24) ^ *(cp++))];
         case 13:         mycrc = (mycrc << 8) ^ crctab[(((mycrc) >> 24) ^ *(cp++))];
         case 12:         mycrc = (mycrc << 8) ^ crctab[(((mycrc) >> 24) ^ *(cp++))];
         case 11:         mycrc = (mycrc << 8) ^ crctab[(((mycrc) >> 24) ^ *(cp++))];
         case 10:         mycrc = (mycrc << 8) ^ crctab[(((mycrc) >> 24) ^ *(cp++))];
         case 9:         mycrc = (mycrc << 8) ^ crctab[(((mycrc) >> 24) ^ *(cp++))];
         case 8:         mycrc = (mycrc << 8) ^ crctab[(((mycrc) >> 24) ^ *(cp++))];
         case 7:         mycrc = (mycrc << 8) ^ crctab[(((mycrc) >> 24) ^ *(cp++))];
         case 6:         mycrc = (mycrc << 8) ^ crctab[(((mycrc) >> 24) ^ *(cp++))];
         case 5:         mycrc = (mycrc << 8) ^ crctab[(((mycrc) >> 24) ^ *(cp++))];
         case 4:         mycrc = (mycrc << 8) ^ crctab[(((mycrc) >> 24) ^ *(cp++))];
         case 3:         mycrc = (mycrc << 8) ^ crctab[(((mycrc) >> 24) ^ *(cp++))];
         case 2:         mycrc = (mycrc << 8) ^ crctab[(((mycrc) >> 24) ^ *(cp++))];
         case 1:         mycrc = (mycrc << 8) ^ crctab[(((mycrc) >> 24) ^ *(cp++))];
     } while (--n > 0);} }

 else
 {while (nBytes > 0)
    {mycrc = ((mycrc) << 8) ^ crctab[(((mycrc) >> 24) ^ nBytes) & 0xFF];
     nBytes >>= 8;}
  mycrc = ~(mycrc) & 0xFFFFFFFF;}
 
  *crc = mycrc;

 return;
}

/*---------------------------------------------------------------------------*/
FrCList *FrCListBldAdc(FrameH *frame)
/*---------------------------------------------------------------------------*/
/* This function build an alphabetic sorted list of Adc channels.            */
/*---------------------------------------------------------------------------*/
{FrAdcData *adc, **adcField;
 FrCList *list;
 int len, maxL, pSize;
 void *current;

 if (frame == NULL)                    return(NULL);
 if (frame->rawData == NULL)           return(NULL);
 if (frame->rawData->firstAdc == NULL) return(NULL);

 list = (FrCList *) malloc(sizeof(FrCList));
 if(list == NULL)                    return (NULL);

                                       /*-------- compute the table size ----*/
 list->nChannels = 0;
 maxL = 0;
 for (adc = frame->rawData->firstAdc; adc != NULL; adc = adc->next)
    {(list->nChannels)++;
     len = strlen(adc->name)+1;
     if(len > maxL) maxL = len;}

 pSize = sizeof(FrAdcData *);
 list->nameSize = pSize*((maxL+pSize-1)/pSize);     /*-- align the address---*/
 list->rowSize  = pSize + list->nameSize;

                                       /*----------------- Allocate space ---*/

 list->table = calloc(1, list->nChannels * list->rowSize);
 if(list->table == NULL) return(NULL);

                                       /*------------------ Fill the table---*/
 current = list->table;
 for (adc = frame->rawData->firstAdc; adc != NULL; adc = adc->next)
    {memcpy(current, adc->name, strlen(adc->name)+1);
     adcField = (FrAdcData **)((long)current + list->nameSize);
     *adcField = adc;
     current =  (void *)((long)current + list->rowSize);}
                                       /*---------------- sort the table ----*/

 qsort(list->table, list->nChannels, list->rowSize, 
                (int (*) (const void*, const void*)) strcmp);

 return(list);
}
/*---------------------------------------------------------------------------*/
FrCList *FrCListBldChannel(FrameH *frame)
/*---------------------------------------------------------------------------*/
/* This function build an alphabetic sorted list of all channels             */
/* (ADC, Proc, Ser) */
/*---------------------------------------------------------------------------*/
{
  FrAdcData *adc, **adcField;
  FrProcData *proc, **procField;
  FrSerData *ser;
  FrCList *list;
  int len, maxL, pSize;
  void *current;
  char *data, word1[512], word2[512], word3[512];
  int nRead;

  if (frame == NULL)  return(NULL);

  list = (FrCList *) malloc(sizeof(FrCList));
  if(list == NULL) return (NULL);

  /*--------------------------------------------- compute the table size ----*/
  list->nChannels = 0;
  maxL = 0;
  if (frame->rawData != NULL) {
    for (adc = frame->rawData->firstAdc; adc != NULL; adc = adc->next) {
      list->nChannels++;
      len = strlen(adc->name)+1;
      if(len > maxL) maxL = len;}
   
    for(ser = frame->rawData->firstSer; ser != NULL; ser = ser->next) {
      data = ser->data;
      while ((nRead = sscanf(data,"%s %s %s",word1, word2, word3)) >= 2) {
      if ((strcmp(word1,"bias")  != 0) && 
         (strcmp(word1,"slope") != 0) &&
         (strcmp(word1,"units") != 0)) {
        list->nChannels++;
        len = strlen(ser->name)+strlen(word1)+2;
        if(len > maxL) maxL = len;}
      if (nRead != 3) break;
      data = strstr(data+strlen(word1)+strlen(word2), word3);}}}

  for (proc = frame->procData; proc != NULL; proc = proc->next) {
    list->nChannels++;
    len = strlen(proc->name)+1;
    if(len > maxL) maxL = len;}

  pSize = sizeof(FrAdcData *);
  list->nameSize = pSize*((maxL+pSize-1)/pSize);    /*-- align the address---*/
  list->rowSize  = pSize + list->nameSize;

  /*------------------------------------------------------ Allocate space ---*/
  list->table = calloc(1, list->nChannels * list->rowSize);
  if(list->table == NULL) return(NULL);

  /*------------------------------------------------------- Fill the table---*/
  current = list->table;
  if (frame->rawData != NULL) {
    for (adc = frame->rawData->firstAdc; adc != NULL; adc = adc->next) {
      memcpy(current, adc->name, strlen(adc->name)+1);
      adcField = (FrAdcData **)((long)current + list->nameSize);
      *adcField = adc;
      current =  (void *)((long)current + (long)list->rowSize);}

    for(ser = frame->rawData->firstSer; ser != NULL; ser = ser->next) {
      data = ser->data;
      while ((nRead = sscanf(data,"%s %s %s",word1, word2, word3)) >= 2) {
	if  ((strcmp(word1,"bias")  != 0) && 
	     (strcmp(word1,"slope") != 0) && 
	     (strcmp(word1,"units") != 0)) {
	  sprintf(current,"%s_%s",ser->name,word1);
	  current =  (void *)((long)current + (long)list->rowSize);}
	if (nRead != 3) break;
	data = strstr(data+strlen(word1)+strlen(word2), word3);}}}

  for (proc = frame->procData; proc != NULL; proc = proc->next) {
    memcpy(current, proc->name, strlen(proc->name)+1);
    procField = (FrProcData **)((long)current + list->nameSize);
    *procField = proc;
    current =  (void *)((long)current + (long)list->rowSize);}

  /*------------------------------------------------ sort the table ----*/
  qsort(list->table, list->nChannels, list->rowSize, 
	(int (*) (const void*, const void*)) strcmp);

  return(list);
}

/*---------------------------------------------------------------------------*/
int FrCListFindDuplicate(FrCList *list, char *duplNames, int duplNSize)
/*---------------------------------------------------------------------------*/
/* This function compute the number of duplicated channels and print their   */
/* list if space is available                                                */
/*---------------------------------------------------------------------------*/
{
  void **channel;
  char *name, *duplNamesEnd;
  int i, nDuplicate, len;

  if(list == NULL) return(-1);

  nDuplicate = 0;
  duplNamesEnd = duplNames + duplNSize;

  for(i=0; i<list->nChannels-1; i++) {
    name = list->table + i*list->rowSize;
    channel = bsearch(name, name+list->rowSize, 
		      list->nChannels-i-1, list->rowSize,
		      (int (*) (const void*, const void*)) strcmp);
 
    if(channel == NULL) continue;
    nDuplicate++;

    if((duplNSize > 0) && (duplNames != NULL)) {
      len = strlen(name)+1;
      if(duplNames+len+3 < duplNamesEnd) {
	if(nDuplicate != 1) {
	  sprintf(duplNames," ,");
	  duplNames +=2;}
	sprintf(duplNames,"%s",name);
	duplNames += len-1;}
      else if(duplNames+3 < duplNamesEnd) {
	sprintf(duplNames,"...");}}}

  return(nDuplicate);}

/*---------------------------------------------------------------------------*/
FrCList *FrCListBldSer(FrameH *frame)
/*---------------------------------------------------------------------------*/
/* This function build an alphabetic sorted list of Ser channels.            */
/*---------------------------------------------------------------------------*/
{FrSerData *ser, **serField;
 FrCList *list;
 int len, maxL, pSize;
 void *current;

 if (frame == NULL)                    return(NULL);
 if (frame->rawData == NULL)           return(NULL);
 if (frame->rawData->firstSer == NULL) return(NULL);

 list = (FrCList *) malloc(sizeof(FrCList));
 if(list == NULL)                    return (NULL);

                                       /*-------- compute the table size ----*/
 list->nChannels = 0;
 maxL = 0;
 for (ser = frame->rawData->firstSer; ser != NULL; ser = ser->next)
    {(list->nChannels)++;
     len = strlen(ser->name)+1;
     if(len > maxL) maxL = len;}

 pSize = sizeof(FrSerData *);
 list->nameSize = pSize*((maxL+pSize-1)/pSize);     /*-- align the address---*/
 list->rowSize  = pSize + list->nameSize;

                                       /*----------------- Allocate space ---*/

 list->table = calloc(1, list->nChannels * list->rowSize);
 if(list->table == NULL) return(NULL);

                                       /*------------------ Fill the table---*/
 current = list->table;
 for (ser = frame->rawData->firstSer; ser != NULL; ser = ser->next)
    {memcpy(current, ser->name, strlen(ser->name)+1);
     serField = (FrSerData **)((long)current + list->nameSize);
     *serField = ser;
     current = (void *)((long)current + list->rowSize);}
                                       /*---------------- sort the table ----*/

 qsort(list->table, list->nChannels, list->rowSize, 
                (int (*) (const void*, const void*)) strcmp);

 return(list);
}

/*---------------------------------------------------------------------------*/
void *FrCListFind(FrCList *list, char *name)
/*---------------------------------------------------------------------------*/
/* This function an Adc channel in the list.                                 */
/*---------------------------------------------------------------------------*/
{void **channel;

 if(name == NULL) return(NULL);
 if(list == NULL) return(NULL);

 channel = bsearch(name, list->table, list->nChannels, list->rowSize, 
                             (int (*) (const void*, const void*)) strcmp);
 
 if(channel == NULL) return(NULL);
 
 channel = (void **)((long)channel + list->nameSize);
 return(*channel);
}

/*---------------------------------------------------------------------------*/
void FrCListFree(FrCList *list)
/*---------------------------------------------------------------------------*/
/* This function free the space associated to an channel list                */
/*---------------------------------------------------------------------------*/
{
 if(list == NULL) return;

 free(list->table);
 free(list);

 return;
}
/*---------------------------------------------------------------------------*/
void *FrCListGetElement(FrCList *list, int index)
/*---------------------------------------------------------------------------*/
/* This function returns the pointer to the element in the list.             */
/*---------------------------------------------------------------------------*/
{void **channel;

 if(list == NULL)             return(NULL);
 if(index < 0)                return(NULL);
 if(index >= list->nChannels) return(NULL);

 channel = (void **)((long) list->table + index * (long)list->rowSize +
                      list->nameSize);

 return(*channel);
}
/*---------------------------------------------------------------------------*/
int FrDetectorAddStatData(FrDetector* detector,
                          FrStatData *sData)
/*---------------------------------------------------------------------------*/
/* Attached a static data to a detector structure.                           */
/*---------------------------------------------------------------------------*/
{
 if(detector == NULL) return(1);
 if(sData    == NULL) return(2);

 FrStatDataAdd(detector, sData);

 return(0);}

/*------------------------------------------------------------FrDetectorDef--*/
FrSH *FrDetectorDef()
/*---------------------------------------------------------------------------*/
{static FrSH *classe = NULL;

  if(classe != NULL) return(classe);
  classe = FrDicAddS("FrDetector",(void (*)())FrDetectorRead);

  FrSENew(classe, "STRING", "name","-");
  FrSENew(classe, "CHAR[2]", "prefix","-");
  FrSENew(classe, "REAL_8", "longitude","-"); 
  FrSENew(classe, "REAL_8", "latitude","-"); 
  FrSENew(classe, "REAL_4", "elevation","-"); 
  FrSENew(classe, "REAL_4", "armXazimuth","-"); 
  FrSENew(classe, "REAL_4", "armYazimuth","-"); 
  FrSENew(classe, "REAL_4", "armXaltitude","-"); 
  FrSENew(classe, "REAL_4", "armYaltitude","-"); 
  FrSENew(classe, "REAL_4", "armXmidpoint","-"); 
  FrSENew(classe, "REAL_4", "armYmidpoint","-");
  FrSENew(classe, "INT_4S", "localTime","-"); 
  FrSENew(classe, "PTR_STRUCT(FrVect *)", "aux","-"); 
  FrSENew(classe, "PTR_STRUCT(FrTable *)", "table","-"); 
  FrSENew(classe, "PTR_STRUCT(FrDetector *)", "next","-"); 
  if(FrFormatVersion >= 8) FrSENew(classe, "INT_4U", "chkSum","-");

  return(classe);}
/*-----------------------------------------------------------FrDetectorDump--*/
void FrDetectorDump(FrDetector *detector, 
                    FILE *fp, 
                    int debugLvl)
/*---------------------------------------------------------------------------*/
{FrStatData *sData;
 char p0, p1;

 if(debugLvl < 1)     return;
 if(detector == NULL) return;
 if(fp       == NULL) return;

 p0 = detector->prefix[0];
 p1 = detector->prefix[1];
 if(p0 == '\0') p0 = ' ';
 if(p1 == '\0') p1 = ' ';

 fprintf(fp,"Detector: %s (prefix:%c%c) localTime=%d",
	 detector->name, p0, p1, detector->localTime);
 fprintf(fp," longitude =%.8f latitude =%.8f\n",
	 detector->longitude, detector->latitude);
 fprintf(fp,"  Arm Azimuth: X=%.4f Y=%.4f",
	 detector->armXazimuth, detector->armYazimuth);
 fprintf(fp," Altitude: X=%.4f Y=%.4f", 
         detector->armXaltitude, detector->armYaltitude);
 fprintf(fp," midPoint: X=%.1f Y=%.1f\n",
         detector->armXmidpoint, detector->armYmidpoint);

                                      /*----------static data----------------*/

 for(sData = detector->sData; sData != NULL; sData = sData->next)
   {FrStatDataDump(sData, fp, debugLvl);}

 return;}
 
/*----------------------------------------------------------------------------*/
FrStatData *FrDetectorFindStatData(FrDetector *det,
  		 	           char *statDataName,
                                   int gpsTime)
/*---------------------------------------------------------------------------*/
{FrStatData *stat;

 stat = FrStatDataFind(det, statDataName, gpsTime);

 return(stat);}

/*-----------------------------------------------------------FrDetectorFree--*/
void FrDetectorFree(FrDetector *detector)
/*---------------------------------------------------------------------------*/
{
 if(detector == NULL) return;

 if(detector->next != NULL) FrDetectorFree(detector->next);

 if(detector->sDataOld != NULL) FrDetectorUntagStat(detector);
 if(detector->sData    != NULL) FrStatDataFree(detector->sData);

 if(detector->name  != NULL) free(detector->name);
 if(detector->aux   != NULL) FrVectFree(detector->aux);
 if(detector->table != NULL) FrTableFree(detector->table);

 free(detector);

 return;}

/*--------------------------------------------------------- FrDetectorMerge--*/
FrDetector *FrDetectorMerge(FrDetector *det1, FrDetector *det2)
/*---------------------------------------------------------------------------*/
{FrDetector *next;
 FrVect **vect;
 FrTable **table;
 FrStatData *stat1, *stat2, *statNext;

 if(det1 == NULL) return(NULL);
 if(det2 == NULL) return(NULL);
 next = det2->next;

 for(vect = &(det1->aux); *vect != NULL; vect = &((*vect)->next)) {;}
 *vect = det2->aux;

 for(table = &(det1->table); *table != NULL; table = &((*table)->next)) {;}
 *table = det2->table;

                      /*------ merge static data if they are different ----*/

 for(stat2 = det2->sData; stat2 != NULL; stat2 = statNext)
   {statNext = stat2->next;

    for(stat1 = det1->sData; stat1 != NULL; stat1 = stat1->next)
      {if(strcmp(stat1->name, stat2->name) != 0) continue;
       if(stat1->timeStart !=  stat2->timeStart) continue;
       if(stat1->timeEnd   !=  stat2->timeEnd)   continue;
       if(stat1->version   !=  stat2->version)   continue;
       break;}

    if(stat1 == NULL)        /*--------move stat data from det2 to det1 ---*/
      {stat2->detector = det1;
       stat2->next = det1->sData;
       det1->sData = stat2;}
    else                     /*--------- duplicate stat data: free stat2---*/
      {stat2->next = NULL;
      FrStatDataFree(stat2);}}
  
               /*----- free detector 2 since everything has been moved -----*/

 if(det2->name != NULL) free(det2->name);
 free(det2);

 return(next);}

/*----------------------------------------------------------- FrDetectorNew--*/
FrDetector *FrDetectorNew(char *name)
/*---------------------------------------------------------------------------*/
{FrDetector *detector;
 time_t utc;
 
  detector = (FrDetector *) calloc(1,sizeof(FrDetector));
  if(detector == NULL) return(NULL);
  detector->classe = FrDetectorDef(); 

  FrStrCpy(&detector->name, name);

  if(strcmp(name,"Virgo") == 0) {
    detector->prefix[0] = 'V';
    detector->prefix[1] = '1';}

  utc = time(NULL);
  detector->localTime = mktime(localtime(&utc)) - mktime(gmtime(&utc));

  return(detector);}
/*--------------------------------------------------------- FrDetectorRead---*/
FrDetector  *FrDetectorRead(FrFile *iFile)
/*---------------------------------------------------------------------------*/
{FrDetector *detector;
 char *qaList, message[128];
 int dataQuality;
 
 if(iFile->fmtVersion == 3) return(FrBack3DetectorRead(iFile));
 if(iFile->fmtVersion == 4) return(FrBack4DetectorRead(iFile));

  iFile->detectorType = iFile->type;

  detector = (FrDetector *) calloc(1,sizeof(FrDetector));
  if(detector == NULL) 
       {iFile->error = FR_ERROR_MALLOC_FAILED;
        return(NULL);}
  detector->classe = FrDetectorDef(); 
  
  FrReadHeader(iFile,  detector);
  FrReadSChar (iFile, &detector->name);
  if(iFile->fmtVersion > 5)
       {FrRead(iFile, (char *)&detector->prefix, 2);}
  else {detector->prefix[0] = '*';
        detector->prefix[1] = '*';}
  FrReadDouble(iFile, &detector->longitude); 
  FrReadDouble(iFile, &detector->latitude); 
  FrReadFloat (iFile, &detector->elevation); 
  FrReadFloat (iFile, &detector->armXazimuth); 
  FrReadFloat (iFile, &detector->armYazimuth); 
  FrReadFloat (iFile, &detector->armXaltitude); 
  FrReadFloat (iFile, &detector->armYaltitude); 
  FrReadFloat (iFile, &detector->armXmidpoint); 
  FrReadFloat (iFile, &detector->armYmidpoint); 
  FrReadInt   (iFile, &detector->localTime);
  if(iFile->fmtVersion == 5) 
   {FrReadInt (iFile, &dataQuality);
    FrReadSChar(iFile, &qaList);
    if(qaList != NULL) free(qaList);}
  FrReadStruct(iFile, &detector->aux); 
  FrReadStruct(iFile, &detector->table);
  FrReadStruct(iFile, &detector->next);
  if(iFile->fmtVersion >=8) FrReadStructChksum(iFile);

  if(iFile->nBytes != iFile->nBytesR + iFile->length)
     {sprintf(message,": Record length error: "
         "nBytes=%"FRLLD" nBytesR=%"FRLLD" length=%"FRLLD"\n",
          iFile->nBytes, iFile->nBytesR, iFile->length);
      FrError(3,"FrDetectorRead",message);
      return(NULL);}     

  if(FrDebugLvl > 2) fprintf(FrFOut," %s\n", detector->name);

  return(detector);}
 
/*-------------------------------------------------------FrDetectorUntagStat-*/
void FrDetectorUntagStat(FrDetector*det)
/*---------------------------------------------------------------------------*/
{
 if(det == NULL) return;

 FrameUntagBasic((FrBasic**) &det->sData,
                 (FrBasic**) &det->sDataOld);

 return;}

/*----------------------------------------------------------FrDetectorWrite--*/
void FrDetectorWrite(FrDetector *detector, 
                     FrFile *oFile)
/*---------------------------------------------------------------------------*/
{FrStatData *sData;

 oFile->detectorType = FrDetectorDef()->id;

 if(detector == NULL) return;
 	 
 if(FrDebugLvl > 1)
    {fprintf(FrFOut,"  Output FrDetector %s\n",detector->name);}
         
 FrTOCdetMark(oFile, detector->name);

 FrPutNewRecord(oFile, detector, FR_YES);
 FrPutSChar (oFile, detector->name);
 FrPutVC    (oFile, detector->prefix, 2);
 FrPutDouble(oFile, detector->longitude); 
 FrPutDouble(oFile, detector->latitude); 
 FrPutFloat (oFile, detector->elevation); 
 FrPutFloat (oFile, detector->armXazimuth); 
 FrPutFloat (oFile, detector->armYazimuth); 
 FrPutFloat (oFile, detector->armXaltitude); 
 FrPutFloat (oFile, detector->armYaltitude); 
 FrPutFloat (oFile, detector->armXmidpoint); 
 FrPutFloat (oFile, detector->armYmidpoint); 
 FrPutInt   (oFile, detector->localTime);
 FrPutStruct(oFile, detector->aux); 
 FrPutStruct(oFile, detector->table);
 FrPutStruct(oFile, detector->next);
 FrPutWriteRecord(oFile, FR_NO);
 
              /*----------------------- write associated data ---------------*/
 
 for(sData = detector->sData; sData != NULL; sData = sData->next) 
   {if(FrTOCStatD(oFile, sData) == 0) FrStatDataWrite(sData, oFile);}
 
 if(detector->aux   != NULL) FrVectWrite    (detector->aux,   oFile);
 if(detector->table != NULL) FrTableWrite   (detector->table, oFile);
 if(detector->next  != NULL) FrDetectorWrite(detector->next,  oFile);

 return;}

/*----------------------------------------------------------------FrDicAddS--*/
FrSH *FrDicAddS(char *type, 
                void  (*funRead)())
/*---------------------------------------------------------------------------*/
{FrSH *structH;

  structH = FrSHNew(type, "");
  if(structH == NULL)  {return(NULL);}
  FrSHRoot[FrnSH] = structH;
  FrnSH++;

  structH->objRead   = funRead;
  
  return(structH);}
  
/*----------------------------------------------------------------FrDicFree--*/
void FrDicFree(FrFile *file)
/*---------------------------------------------------------------------------*/
{FrSH *sh;
 FrSE *nextSE;
 int j;

 for(j=3; j<file->maxSH; j++)
   {sh = file->sh[j];
    if(sh == NULL) continue;

    while(sh->firstE != NULL)
        {nextSE = sh->firstE->next;
         if(sh->firstE->comment != NULL) free(sh->firstE->comment); 
         free(sh->firstE->type);
         free(sh->firstE->name);
         free(sh->firstE); 
         sh->firstE = nextSE;}

    free(sh->name);
    if(sh->comment != NULL) free(sh->comment);
    free(sh);

    file->sh[j] = NULL;}

 return;} 
/*-------------------------------------------------------------- FrDicGetId--*/
int FrDicGetId(FrFile *file, 
               unsigned short type, 
               void *address)             
/*---------------------------------------------------------------------------*/
/* This function return the instance number assigned by FrDicAssignId        */
/* It removed the item from the list except for FrDetector                   */
/* since several structures could point to it.                               */
/*---------------------------------------------------------------------------*/
{int i;

 for(i=0; i<file->nRef; i++)
   {if((file->refType[i]  == type) && (file->address[i] == address))
       {if(type != file->detectorType) file->refType[i] = -1;
        return(file->refInstance[i]);}}

  FrError(3,"FrDicGetId","bank not referenced");
  file->error = FR_ERROR_FRSET;
 
  return(0);}

/*------------------------------------------------------------FrDicAssignId--*/
int FrDicAssignId(FrFile *file,
                  unsigned short type, 
                  void *address)                
/*---------------------------------------------------------------------------*/
/* This function assigns an instance number to be used by FrDicGetId         */
/*---------------------------------------------------------------------------*/
{unsigned short instance;
 int i;

       /*------ first search for FrDetector if an id is already assigned ----*/

 if(type == file->detectorType)
   {for(i=0; i<file->nRef; i++)
      {if((file->refType[i]  == type) && (file->address[i] == address))
	{return(file->refInstance[i]);}}}
   
      /*------------------- then standard case: assign a new id -------------*/

 instance = file->lastInstance[type];
 file->lastInstance[type]++;

 for(i=0; i<file->nRef; i++)
   {if(file->refType[i]  == -1) break;}

 if(i == file->nRef) 
    {if(file->nRef > FRMAXREF) 
       {FrError(3,"FrDicAssignId", "FRMAXREF need to be increased");
        file->error = FR_ERROR_FRSET;
        return(0);}
     file->nRef++;}

 file->refType[i]     = type;
 file->refInstance[i] = instance;
 file->address[i]     = address;

 return(instance);}
  
/*---------------------------------------------------------------- FrDicIni--*/
void FrDicIni(FrFile *file)
/*---------------------------------------------------------------------------*/
{int i;

 for(i=0; i<FrnSH; i++) {file->lastInstance[i] = 0;}

 file->nRef = 0;

 return;} 

/*---------------------------------------------------------- FrEndOfFileDef--*/
FrSH *FrEndOfFileDef()
/*---------------------------------------------------------------------------*/
{static FrSH *classe = NULL;

  if(classe != NULL) return(classe);
  classe = FrDicAddS("FrEndOfFile",FrEndOfFileRead);

  FrSENew(classe, "INT_4U", "nFrames","-");
  FrSENew(classe, "INT_8U", "nBytes","-");

  if(FrFormatVersion < 8) {
    FrSENew(classe, "INT_4U", "chkType","-");
    FrSENew(classe, "INT_4U", "chkSum","-");
    FrSENew(classe, "INT_8U", "seekTOC","-");}
  else {
    FrSENew(classe, "INT_8U", "seekTOC","-");
    FrSENew(classe, "INT_4U", "chkSumFrHeader","-");
    FrSENew(classe, "INT_4U", "chkSum","-");
    FrSENew(classe, "INT_4U", "chkSumFile","-");}

return(classe);}

/*----------------------------------------------------------FrEndOfFileRead--*/
void FrEndOfFileRead(FrFile *iFile)
/*---------------------------------------------------------------------------*/
{
  unsigned int nFrames, nBytes4, seekTOC4, instance, zero = 0; 
  unsigned int chkSumFi,chkSumFrHeader;
  FRLONG nBytes, seekTOC;
  unsigned short instance2;
  FRBOOL chkSumFlag;

  chkSumFrHeader = 0;
  if(iFile->fmtVersion == 4) { /*------------- for frame format version 4 ---*/
    FrReadShortU(iFile, &instance2);
    instance = instance2;
    FrReadIntU(iFile, &nFrames);
    FrReadIntU(iFile, &nBytes4);
    nBytes = nBytes4;

    chkSumFlag = iFile->chkSumFiFlag;  
    iFile->chkSumFiFlag = FR_NO;
    FrReadIntU(iFile, &iFile->chkTypeFiRead);
    FrReadIntU(iFile, &iFile->chkSumFiRead);
    iFile->chkSumFiFlag = chkSumFlag;
    FrReadIntU(iFile, &seekTOC4);
    seekTOC = seekTOC4;}

  else if(iFile->fmtVersion < 8) { /*---------- for frame format version 6---*/
    FrReadIntU(iFile, &instance);
    FrReadIntU(iFile, &nFrames);
    FrReadLong(iFile, &nBytes);

    chkSumFlag = iFile->chkSumFiFlag;  
    FrReadIntU  (iFile, &iFile->chkTypeFiRead);
    iFile->chkSumFiFlag = FR_NO;
    FrCksumGnu((char *) &zero, FrSint, &(iFile->chkSumFi));
    FrReadIntU  (iFile, &iFile->chkSumFiRead);
    iFile->chkSumFiFlag = chkSumFlag;

    FrReadLong  (iFile, &seekTOC);
    FrCksumGnu( NULL, nBytes, &(iFile->chkSumFi));}

  else {  /*----------------------------------- for frame format version 8---*/
    FrReadIntU(iFile, &instance);
    FrReadIntU(iFile, &nFrames);
    FrReadLong(iFile, &nBytes);

    FrReadLong(iFile, &seekTOC);
    FrReadIntU(iFile, &chkSumFrHeader);
    FrReadStructChksum(iFile);      /*---- this is the structure checksum --*/
 
    FrCksumGnu(NULL, nBytes-4, &(iFile->chkSumFi)); /*-- the file checksum--*/
    chkSumFi = iFile->chkSumFi;
    FrReadIntU(iFile, &iFile->chkSumFiRead);
    iFile->chkSumFi = chkSumFi;}


  if(FrDebugLvl > 0) {
    fprintf(FrFOut, "End of File Reached; Frame format version %d\n",
	    iFile->fmtVersion);
    fprintf(FrFOut, "  nFrames=%d nBytes=%"FRLLD" chkSumFile=%x(flag=%d) ",
	    nFrames, nBytes, iFile->chkSumFiRead, iFile->chkTypeFiRead);
    fprintf(FrFOut, " chkSumFrHeader=%x", chkSumFrHeader);
    fprintf(FrFOut, " seekTOC=%"FRLLD"\n", seekTOC);}

  /*--------------------------------------------check File Header checksum---*/

  if(iFile->fmtVersion >= 8) { 
    if(chkSumFrHeader == iFile->chkSumFrHeader) {
      if(FrDebugLvl > 0) fprintf(FrFOut, "  File Header checksum OK\n");}
    else {
      sprintf(FrErrMsg, "File Header checksum ERROR; read=%x",chkSumFrHeader);
      FrError(3,"FrEndOfFileRead", FrErrMsg);
      iFile->error = FR_ERROR_CHECKSUM;}}

  /*---------------------------------------------- check the file checksum---*/

  if(iFile->chkTypeFiRead == 0) {
    if(FrDebugLvl >0) fprintf(FrFOut,"  File checksum not available\n");}
  else if(iFile->chkSumFiFlag == FR_NO) {
    if(FrDebugLvl >0) fprintf(FrFOut,"  File checksum not asked for check\n");}
  else if(iFile->chkSumFiRead  == iFile->chkSumFi) {
    if(FrDebugLvl >0) fprintf(FrFOut, "  File checksum OK\n");}
  else {
    if(iFile->fmtVersion > 5) 
      sprintf(FrErrMsg, "File CRC Checksum error");
    else   sprintf(FrErrMsg, "File XOR Checksum error");                     
    sprintf(FrErrMsg+23, ": read=%x computed=%x file:%s",
	    iFile->chkSumFiRead, iFile->chkSumFi, iFile->current->fileName);
    FrError(3,"FrEndOfFileRead", FrErrMsg);
    iFile->error = FR_ERROR_CHECKSUM;
    return;}
                          
  /*--------------------------- we try to open the next file in the list ----*/

  if(FrFileINext(iFile, 0., 0., NULL, FR_NO) != 0) {
    iFile->endOfFrame = FR_YES;
    return;}
 
  return;}

/*-------------------------------------------------------- FrEndOfFileWrite--*/
void FrEndOfFileWrite(FrFile *oFile)
/*---------------------------------------------------------------------------*/
{static FrEndOfFile *eof = NULL;
 FRLONG nBytes, seekTOC, eofLength; /*end of file bloc size*/
          
 if(eof == NULL) {
   eof = (FrEndOfFile *) calloc(1,sizeof(FrEndOfFile));
   if(eof == NULL) {
     oFile->error = FR_ERROR_MALLOC_FAILED;
     return;}
   eof->classe = FrEndOfFileDef();}

 FrPutNewRecord(oFile, eof, FR_NO);

 if(FrFormatVersion >=8) eofLength = 46;
 else                    eofLength = 42;

 nBytes = oFile->nBytes + eofLength; 
 if(oFile->toc != NULL)
     {seekTOC = nBytes - oFile->toc->position;}
 else{seekTOC = 0;}

 FrPutIntU (oFile, oFile->nFrames);
 FrPutLong (oFile, nBytes);

 /*--------------- before frame format version 8 ---*/

 if(FrFormatVersion < 8) {
   FrPutIntU (oFile, oFile->chkSumFiFlag);
   FrPutIntU (oFile, 0);
   FrPutLong (oFile, seekTOC);}

 /*--- for format v8 (checksums are added by the PutWriteRecord fcnt)------*/

 else {
   FrPutLong (oFile, seekTOC);
   FrPutIntU (oFile, oFile->chkSumFrHeader);}

 /*------------- common to all format -------*/

 FrPutWriteRecord(oFile, FR_YES);

 if(FrDebugLvl > 0) {
   fprintf(FrFOut, "write End of File after "
           "%d frames; %"FRLLD" Bytes checksum = %x seekTOC=%"FRLLD"\n",
           oFile->nFrames, nBytes, oFile->chkSumFiFlag, seekTOC);
   fprintf(FrFOut, "  chkSumFrHeader=%x\n", oFile->chkSumFrHeader);}

 return;}

/*--------------------------------------------------------- FrEndOfFrameDef--*/
FrSH *FrEndOfFrameDef()
/*---------------------------------------------------------------------------*/
{static FrSH *classe = NULL;

  if(classe != NULL) return(classe);
  classe = FrDicAddS("FrEndOfFrame", (void (*)())FrEndOfFrameRead);

  FrSENew(classe, "INT_4S", "run","-");
  FrSENew(classe, "INT_4U", "frame","-");
  if(FrFormatVersion < 8) {
    FrSENew(classe, "INT_4U", "chkType","-");
    FrSENew(classe, "INT_4U", "chkSum","-");}
  else {
    FrSENew(classe, "INT_4U", "GTimeS","-");
    FrSENew(classe, "INT_4U", "GTimeN","-");
    FrSENew(classe, "INT_4U", "chkSum","-");}

  return(classe);}
/*------------------------------------------------------- FrEndOfFrameRead---*/
void FrEndOfFrameRead(FrFile *iFile)
/*---------------------------------------------------------------------------*/
{unsigned int run, frameNumber, instance, gTimeS, gTimeN, chkSumFlag, zero = 0;
 FRULONG nBytes;
 unsigned short instance2;

 if(iFile->fmtVersion == 4) 
    {FrBack4EndOfFrameRead(iFile);
     return;}

 if(iFile->fmtVersion > 5) 
      FrReadIntU  (iFile, &instance);
 else FrReadShortU(iFile, &instance2);
 FrReadIntU(iFile, &run);
 FrReadIntU(iFile, &frameNumber);

                    /*--------- checksum test for version lower than 8 ---*/

 if(iFile->fmtVersion < 8) {
   FrReadIntU(iFile, &iFile->chkTypeFrRead);

   chkSumFlag = iFile->chkSumFrFlag;       
   if(iFile->chkTypeFrRead == 1) { /*-- chkSum is set to 0 when computing it--*/
     iFile->chkSumFrFlag = FR_NO;
     FrCksumGnu((char *) &zero, FrSint, &(iFile->chkSumFr));}
   FrReadIntU(iFile, &iFile->chkSumFrRead);
   iFile->chkSumFrFlag = chkSumFlag;

   nBytes = iFile->nBytes - iFile->nBytesF;

   if(FrDebugLvl > 0) fprintf(FrFOut, " FrEndOfFrameRead: run=%d frame=%d "
			      "nBytes =%"FRLLD" chkType=%d chkSum=%x", run, frameNumber, 
			      nBytes, iFile->chkTypeFrRead, iFile->chkSumFrRead);

   /*--------- check checksum ----------------------*/

   if((iFile->chkTypeFrRead == 1) && (iFile->chkSumFrFlag == FR_YES))
     {FrCksumGnu( NULL, nBytes, &(iFile->chkSumFr));
       if(iFile->chkSumFrRead  != iFile->chkSumFr)
	 {sprintf(FrErrMsg, "Frame Checksum error: read=%x computed=%x"
		  " GPS=%d file:%s",  iFile->chkSumFrRead,iFile->chkSumFr, 
		  iFile->curFrame->GTimeS, iFile->current->fileName);
	   FrError(3,"FrEndOfFrameRead", FrErrMsg);
	   iFile->error = FR_ERROR_CHECKSUM;
	   return;}
       else {if(FrDebugLvl > 0) fprintf(FrFOut,"(OK)");}}
   if(FrDebugLvl > 0) fprintf(FrFOut,"\n");}

 /*-------------------- checksum for frame format v8 and above ---*/

 else {
   FrReadIntU(iFile, &gTimeS);
   FrReadIntU(iFile, &gTimeN);
   FrReadStructChksum(iFile);}

                             /*---- check that the frame is not corrupted ---*/

 if(iFile->curFrame != NULL)
   {if(run != iFile->curFrame->run)
     {FrError(3,"FrEndOfFrameRead","run number missmatch");
      iFile->error = FR_ERROR_BAD_END_OF_FRAME;
      return;}

    if(frameNumber != iFile->curFrame->frame)
     {FrError(3,"FrEndOfFrameRead","frame number missmatch");
      iFile->error = FR_ERROR_BAD_END_OF_FRAME;
      return;}}

 iFile->endOfFrame = FR_YES;

 return;}

/*------------------------------------------------------ FrEndOfFrameWrite---*/
void FrEndOfFrameWrite(FrameH *frame, 
                       FrFile *oFile)
/*---------------------------------------------------------------------------*/
{static FrEndOfFrame *eof = NULL;
 unsigned int chkSumFlag, chkSum; 
 FRULONG nBytes, eofLength = 30; /*end of file bloc size*/
 short ref;
 char *swap;
 int dummy[2];
 
 if(eof == NULL) 
    {eof = (FrEndOfFrame *) calloc(1,sizeof(FrEndOfFrame));
     if(eof == NULL) 
       {oFile->error = FR_ERROR_MALLOC_FAILED;
        return;}
     eof->classe = FrEndOfFrameDef();}
 
 FrPutNewRecord(oFile, eof, FR_NO);
 FrPutIntU (oFile, frame->run);
 FrPutIntU (oFile, frame->frame);

 if(FrFormatVersion >= 8) {
   FrPutIntU (oFile, frame->GTimeS);
   FrPutIntU (oFile, frame->GTimeN);}

 else {
   FrPutIntU (oFile, 0);
   FrPutIntU (oFile, 0);

   /*---------------------------------------- Put checksum info--------*/

   nBytes = oFile->nBytes - oFile->nBytesF + eofLength;

   if(oFile->chkSumFrFlag == FR_YES)
     {if(FrSlong == 8)
	 {memcpy(oFile->pStart, &eofLength, FrSlong);}
       else {
	 swap = (char *)&ref;
	 ref = 0x1234;
	 if(swap[0] == 0x12) {
	   dummy[0] = 0;
	   dummy[1] = eofLength;}
	 else {
	   dummy[0] = eofLength;
	   dummy[1] = 0;}
	 memcpy(oFile->pStart, dummy, 8);}

       chkSumFlag = 1;
       memcpy(oFile->pStart+22, &chkSumFlag, FrSint);
       FrCksumGnu((FRSCHAR*) oFile->pStart, eofLength, &(oFile->chkSumFr));
       FrCksumGnu( NULL, nBytes, &(oFile->chkSumFr));
       chkSum = oFile->chkSumFr;
       memcpy(oFile->pStart+26, &(oFile->chkSumFr), FrSint);}}

 FrPutWriteRecord(oFile, FR_NO);

 if(FrDebugLvl > 1) {fprintf(FrFOut,
   "Write FrEndOfFrame for GPS %d \n",frame->GTimeS);}

 oFile->nSH = 0;
 oFile->nSE = 0;

 return;}

/*------------------------------------------------------------ FrError ------*/
char *FrError(int level, 
              char *function, 
              char *message)
/*---------------------------------------------------------------------------*/
/* handle FrameLib error acording the level of severity.                     */
/* input: function: name of the function calling for an error                */
/*        message:  message to by reported                                   */
/*        level:    2 = warning,                                             */
/*                  3 = fatal error: requested action could not be           */
/*                                   completed                               */
/* return: a pointer to the string which contains the history of the         */
/*         error messages                                                    */
/*---------------------------------------------------------------------------*/
{static int errorMsgL = 0;
#define  FRERRORMSGSIZE  500           /* this is used only in this function */
 static char errorMsg[FRERRORMSGSIZE];
 
 if(level == -1) return(errorMsg);

 if(level < 2)
     {sprintf(FrBuf," *** FrInfo: in %s %s\n",function,message);}
 else if(level < 3)
     {sprintf(FrBuf," *** FrWarning: in %s %s\n",function,message);}
 else{sprintf(FrBuf," *** FrError: in %s %s\n",function,message);}
   
 if(errorMsgL < FRERRORMSGSIZE)
   {strncpy(&errorMsg[errorMsgL], FrBuf, FRERRORMSGSIZE-errorMsgL);
    errorMsgL = strlen(FrBuf) + errorMsgL;}
 
 if(FrErrorHandler != NULL) 
   {FrErrorHandler(level, FrBuf);}
 
  return(errorMsg);}

/*--------------------------------------------------------FrErrorGetHistory--*/
char *FrErrorGetHistory()
/*---------------------------------------------------------------------------*/
/* return: a pointer to the string which contains the history of the         */
/*         error messages                                                    */
/*---------------------------------------------------------------------------*/
{static char *errorMsg = NULL;

 if(errorMsg == NULL)
   {errorMsg = FrError(-1,NULL,NULL);}
 
  return(errorMsg);}
/*--------------------------------------------------------FrErrorDefHandler--*/
void FrErrorDefHandler(int level, 
                       char *lastMessage)
/*---------------------------------------------------------------------------*/
/* default handler for the FrameLib error.                                   */
/* input parameters:                                                         */
/*   lastMessage: the string which contain the last generated message        */
/*   level:  2 = warning,                                                    */
/*           3 = fatal error: requested action could not be  completed       */
/*---------------------------------------------------------------------------*/
{
    if(FrDebugLvl > 0)
       {fprintf(FrFOut,"%s",lastMessage);
        fprintf(stderr,"%s",lastMessage);}
 
    return;}

/*--------------------------------------------------------FrErrorSetHandler--*/
void FrErrorSetHandler(void  (*handler)(int, char *))
/*---------------------------------------------------------------------------*/
{
 FrErrorHandler = handler;

 return;}

/*----------------------------------------------------------FrEventAddParam--*/
FrEvent *FrEventAddParam(FrEvent *event,  
                         char *name, 
                         double value)
/*---------------------------------------------------------------------------*/
{
 if(event == NULL) return(NULL);

 if(event->nParam > 65534) return(NULL);  /*--overflow the storage capacity--*/

 event->nParam++;

 if(event->nParam == 1)
   {event->parameters     = (double *)malloc(sizeof(double));
    event->parameterNames = (char **) malloc(sizeof(char *));}
 else   
   {event->parameters     = (double *) realloc(event->parameters, 
                                               event->nParam* sizeof(double));
    event->parameterNames = (char **) realloc(event->parameterNames,
                                              event->nParam* sizeof(char *));}

 if(event->parameters     == NULL) return(NULL);
 if(event->parameterNames == NULL) return(NULL);

 if(FrStrCpy(&(event->parameterNames[event->nParam - 1]),name) == NULL) 
                                             return(NULL);
 event->parameters[event->nParam - 1] = value;

 return(event);} 

/*-----------------------------------------------------------FrEventAddVect--*/
int FrEventAddVect(FrEvent *event,  
                   FrVect *vect,
                   char* newName)
/*---------------------------------------------------------------------------*/
{FrVect *copy, *last;

  if(event == NULL) return(1);
  if(vect  == NULL) return(2);
  
  copy = FrVectCopy(vect);
  if(copy == NULL) return(3);
  if(newName != NULL) FrVectSetName(copy, newName);
  copy->startX[0] = vect->GTime + vect->startX[0] 
                - (event->GTimeS + 1.e-9*event->GTimeN);
  
  if(event->data == NULL)
    {event->data = copy;}
  else
    {last = event->data;
     while(last->next != NULL) {last = last->next;}
     last->next = copy;}

 return(0);}

/*----------------------------------------------------------FrEventAddVectF--*/
int FrEventAddVectF(FrEvent *event,  
                    FrVect *vect,
                    char* newName)
/*---------------------------------------------------------------------------*/
{FrVect *copy, *last;

  if(event == NULL) return(1);
  if(vect  == NULL) return(2);
  
  copy= FrVectCopyToF(vect, 1., newName);
  if(copy == NULL) return(3);
  copy->startX[0] = vect->GTime + vect->startX[0] 
                - (event->GTimeS + 1.e-9*event->GTimeN);
  
  if(event->data == NULL)
    {event->data = copy;}
  else
    {last = event->data;
     while(last->next != NULL) {last = last->next;}
     last->next = copy;}

 return(0);}

/*--------------------------------------------------------------FrEventCopy--*/
FrEvent *FrEventCopy(FrEvent *event)
/*---------------------------------------------------------------------------*/
{FrEvent *copy;
 int i;

 if(event == NULL) return(NULL);

 copy = (FrEvent *) calloc(1,sizeof(FrEvent));
 if(copy == NULL) return(NULL);
 copy->classe = FrEventDef();

 copy->GTimeS     = event->GTimeS; 
 copy->GTimeN     = event->GTimeN; 
 copy->timeBefore = event->timeBefore; 
 copy->timeAfter  = event->timeAfter; 
 copy->eventStatus= event->eventStatus; 
 copy->amplitude  = event->amplitude; 
 copy->probability= event->probability; 
 
 if(FrStrCpy(&copy->name,       event->name)       == NULL) return(NULL);
 if(FrStrCpy(&copy->comment,    event->comment)    == NULL) return(NULL);
 if(FrStrCpy(&copy->inputs,     event->inputs)     == NULL) return(NULL);
 if(FrStrCpy(&copy->statistics, event->statistics) == NULL) return(NULL);
 
 copy->data  = FrVectCopy(event->data);
 copy->table = FrTableCopy(event->table);
 copy->next  = NULL;

 if(event->nParam > 0)
   {copy->parameters     = (double *)calloc(event->nParam, sizeof(double));
    copy->parameterNames = (char **) calloc(event->nParam, sizeof(char *));

    if(copy->parameters     == NULL) return(NULL);
    if(copy->parameterNames == NULL) return(NULL);

     for(i=0; i<event->nParam; i++)
       {if(event->parameterNames[i] != NULL)
         {if(FrStrCpy(&(copy->parameterNames[i]),
                        event->parameterNames[i]) == NULL) return(NULL);}
        copy->parameters[i] = event->parameters[i];}}

 copy->nParam = event->nParam;

 return(copy);} 

/*--------------------------------------------------------------FrEventDef--*/
FrSH *FrEventDef()
/*--------------------------------------------------------------------------*/
{static FrSH *classe = NULL;

  if(classe != NULL) return(classe);
  classe = FrDicAddS("FrEvent",(void (*)())FrEventRead);

  FrSENew(classe, "STRING", "name","-");
  FrSENew(classe, "STRING", "comment","-");
  FrSENew(classe, "STRING", "inputs","-");
  FrSENew(classe, "INT_4U", "GTimeS","-");
  FrSENew(classe, "INT_4U", "GTimeN","-");
  FrSENew(classe, "REAL_4", "timeBefore","-");
  FrSENew(classe, "REAL_4", "timeAfter","-");
  FrSENew(classe, "INT_4U", "eventStatus","-");
  FrSENew(classe, "REAL_4", "amplitude","-");
  FrSENew(classe, "REAL_4", "probability","-");
  FrSENew(classe, "STRING", "statistics","-");
  FrSENew(classe, "INT_2U", "nParam", "-");
  if(FrFormatVersion == 6)
       FrSENew(classe, "REAL_4[nParam]", "parameters","-");
  else FrSENew(classe, "REAL_8[nParam]", "parameters","-");
  FrSENew(classe, "STRING[nParam]", "parameterNames","-");
  FrSENew(classe, "PTR_STRUCT(FrVect *)",  "data","-");
  FrSENew(classe, "PTR_STRUCT(FrTable *)", "table","-");
  FrSENew(classe, "PTR_STRUCT(FrEvent *)", "next","-"); 
  if(FrFormatVersion >= 8) FrSENew(classe, "INT_4U", "chkSum","-");

  return(classe);}

/*--------------------------------------------------------------FrEventDump--*/
void FrEventDump(FrEvent *evt,
                 FILE *fp,
                 int debugLvl)
/*---------------------------------------------------------------------------*/
{int i;

 if(fp  == NULL) return;
 if(evt == NULL) return;
 if(debugLvl < 1) return;

 fprintf(fp,"Event:%s amplitude=%10.4e time=%.5f s",
           evt->name,       evt->amplitude,  evt->GTimeS+1.e-9*evt->GTimeN);

 if(debugLvl > 1)
  {fprintf(fp," (before=%.5f after=%.5fs)\n",evt->timeBefore, evt->timeAfter);

   if(evt->comment   != NULL) fprintf(fp,"   comment: %s\n", evt->comment);
   if(evt->inputs    != NULL) fprintf(fp,"   inputs: %s\n", evt->inputs);
   fprintf(fp,"   probability: %g  ", evt->probability);
   if(evt->statistics!= NULL) fprintf(fp,"(statistics: %s)",evt->statistics);
   fprintf(fp,"status:%x\n", evt->eventStatus);}

 if(evt->nParam>0) 
   {fprintf(fp,"   parameters:");
    for(i=0; i< evt->nParam; i++)
      {fprintf(fp,"\t%s=%g",evt->parameterNames[i],evt->parameters[i]);
       if(debugLvl > 1 && i%4==3 && i != evt->nParam-1) fprintf(fp,"\n");}
    if(debugLvl > 1) fprintf(fp,"\n");}
 
 if(evt->nParam>0 || debugLvl <= 1) fprintf(fp,"\n");

 if(debugLvl < 2) return;
 if(evt->table != NULL){FrTableDump(evt->table, fp, debugLvl);}
 if(evt->data  != NULL){FrVectDump (evt->data,  fp, debugLvl);
                        fprintf(fp,"\n");}

 return;}

/*-------------------------------------------------------------FrEventFind--*/
FrEvent *FrEventFind(FrameH *frameH, 
                     char *name,
                     FrEvent *event)
/*--------------------------------------------------------------------------*/
{
 if(frameH == NULL) return(NULL);
 if(name   == NULL) return(NULL);

 if(event == NULL) 
      event = frameH->event;
 else event = event->next;

 for(; event != NULL;  event = event->next)
   {if(strcmp(event->name, name) != 0) continue;

    if(event->data  != NULL) FrVectExpandF(event->data);
    if(event->table != NULL) FrTableExpand(event->table);

    return (event);}

 return (NULL);}

/*---------------------------------------------------------FrEventFindVect--*/
FrVect* FrEventFindVect(FrEvent *event, 
                       char *name)
/*--------------------------------------------------------------------------*/
{FrVect *vect;

 if(event == NULL) return(NULL);
 if(name  == NULL) return(NULL);

 vect = event->data;
 while(vect != NULL)
   {if(strcmp(vect->name, name) == 0) return(vect);
   vect = vect->next;}

 return (NULL);}

/*-------------------------------------------------------------FrEventFree--*/
void FrEventFree(FrEvent *event)
/*--------------------------------------------------------------------------*/
{int i;

 if(event == NULL) return;

 if(event->next != NULL) FrEventFree(event->next);

 if(event->name       != NULL) free (event->name);
 if(event->comment    != NULL) free (event->comment);
 if(event->inputs     != NULL) free (event->inputs);
 if(event->statistics != NULL) free (event->statistics);

 if(event->nParam > 0)
   {for(i= 0; i<event->nParam; i++) {free(event->parameterNames[i]);}
    free(event->parameters);
    free(event->parameterNames);}

 if(event->data       != NULL) FrVectFree (event->data);
 if(event->table      != NULL) FrTableFree(event->table);
 
 free(event);

 return;}

/*---------------------------------------------------------FrEventGetParam--*/
double FrEventGetParam(FrEvent *event, 
                       char *name)
/*--------------------------------------------------------------------------*/
{int id;

 id = FrEventGetParamId(event, name);

 if(id < 0) return(-1);
 
 return(event->parameters[id]);}

/*-------------------------------------------------------FrEventGetParamId--*/
int FrEventGetParamId(FrEvent *event, 
                      char *name)
/*--------------------------------------------------------------------------*/
{int i;

 if(event == NULL) return(-1);
 if(name  == NULL) return(-1);

 for(i=0; i<event->nParam; i++)
   {if(strcmp(event->parameterNames[i], name) == 0) return(i);}

 return (-1);}

/*---------------------------------------------------------FrEventGetVectD--*/
FrVect* FrEventGetVectD(FrEvent *event, 
                        char *name)
/*--------------------------------------------------------------------------*/
{FrVect *vect;

 vect = FrEventFindVect(event, name);
 vect = FrVectCopyToD(vect, 1., NULL);

 return (vect);}

/*---------------------------------------------------------FrEventGetVectF--*/
FrVect* FrEventGetVectF(FrEvent *event, 
                        char *name)
/*--------------------------------------------------------------------------*/
{FrVect *vect;

 vect = FrEventFindVect(event, name);
 vect = FrVectCopyToF(vect, 1., NULL);

 return (vect);}

/*------------------------------------------------------------FrEventNew--*/
FrEvent *FrEventNew(FrameH *frameH,  
                    char *name, 
                    char *comment, 
                    char *inputs, 
                    double gtime,
                    float  timeBefore,
                    float  timeAfter,
                    float  amplitude,
                    float  probability,
                    char   *stat,
                    FrVect *data,
                    int nParam, ...)
/*---------------------------------------------------------------------------*/
{FrEvent *event;
 char  *pName;
 int GTimeN, i;
 va_list ap;

 event = (FrEvent *) calloc(1,sizeof(FrEvent));
 if(event == NULL) return(NULL);
 event->classe = FrEventDef();
 
 if(FrStrCpy(&event->name,name)       == NULL) return(NULL);
 if(FrStrCpy(&event->comment,comment) == NULL) return(NULL);
 if(FrStrCpy(&event->inputs,inputs)   == NULL) return(NULL);
 if(FrStrCpy(&event->statistics,stat) == NULL) return(NULL);

 event->GTimeS = gtime;
 GTimeN = (1.e9*(gtime - (double) event->GTimeS));
 if(GTimeN < 0) GTimeN = 0;
 event->GTimeN = GTimeN;
 event->timeBefore  = timeBefore;
 event->timeAfter   = timeAfter;
 event->amplitude   = amplitude;
 event->probability = probability;
 event->data   = data;
 event->nParam = nParam;

 if(nParam > 0)
   {event->parameters     = (double *)calloc(nParam, sizeof(double));
    event->parameterNames = (char **) calloc(nParam, sizeof(char *));

    if(event->parameters     == NULL) return (NULL);
    if(event->parameterNames == NULL) return(NULL);

    va_start(ap,nParam);
    for(i=0; i<nParam; i++)
       {pName = va_arg(ap, char *);
        if(pName != NULL)
         {if(FrStrCpy(&(event->parameterNames[i]),pName) == NULL) return(NULL);}
        event->parameters[i] = va_arg(ap, double);}
    va_end(ap);}

                            /*------- now store it in the Frame structures --*/

 if(frameH != NULL) FrameAddEvent(frameH, event);

return(event);} 

/*--------------------------------------------------------------FrEventRead--*/
FrEvent *FrEventRead(FrFile *iFile)
/*---------------------------------------------------------------------------*/
{FrEvent *event;
 char message[128];

 if(iFile->fmtVersion == 3) return(FrBack3EventRead(iFile));
 if(iFile->fmtVersion == 4) return(FrBack4EventRead(iFile));

 event = (FrEvent *) calloc(1,sizeof(FrEvent));
 if(event == NULL) 
      {iFile->error = FR_ERROR_MALLOC_FAILED;
       return(NULL);}
 event->classe = FrEventDef();

 FrReadHeader(iFile,  event);
 FrReadSChar (iFile, &event->name); 
 FrReadSChar (iFile, &event->comment);
 FrReadSChar (iFile, &event->inputs);
 FrReadIntU  (iFile, &event->GTimeS);
 FrReadIntU  (iFile, &event->GTimeN);
 FrReadFloat (iFile, &event->timeBefore);
 FrReadFloat (iFile, &event->timeAfter);
 FrReadIntU  (iFile, &event->eventStatus);
 FrReadFloat (iFile, &event->amplitude);
 FrReadFloat (iFile, &event->probability);
 FrReadSChar (iFile, &event->statistics);
 FrReadShortU(iFile, &event->nParam);
 if(event->nParam > 0)  
   {if(iFile->fmtVersion == 6) 
          FrReadVFD(iFile, &event->parameters,     event->nParam);
    else  FrReadVD (iFile, &event->parameters,     event->nParam);
    FrReadVQ (iFile, &event->parameterNames, event->nParam);}
 FrReadStruct(iFile, &event->data);
 iFile->vectInstance = iFile->instance;
 FrReadStruct(iFile, &event->table);
 FrReadStruct(iFile, &event->next);
  if(iFile->fmtVersion >=8) FrReadStructChksum(iFile);

  if(iFile->nBytes != iFile->nBytesR + iFile->length)
     {sprintf(message,": Record length error: "
         "nBytes=%"FRLLD" nBytesR=%"FRLLD" length=%"FRLLD"\n",
          iFile->nBytes, iFile->nBytesR, iFile->length);
      FrError(3,"FrEventRead",message);
      return(NULL);}     

 if(FrDebugLvl > 2) fprintf(FrFOut," %s\n", event->name);

 return(event);}
/*--------------------------------------------------------- FrEventReadData--*/
int FrEventReadData(FrFile *file,
                    FrEvent *event)
/*---------------------------------------------------------------------------*/
{FrEvent *tmp, *match;
 double dt=1.e-7;
 int error = 0;

 if(file  == NULL) return(3);

 for(;event != NULL; event = event->next)
   {tmp = FrEventReadTF1(file, event->name, 
			 event->GTimeS + 1.e-9*event->GTimeN - dt, 2*dt, 1, 
               "amplitude", event->amplitude*.99999, event->amplitude*1.0001);
    if(tmp == NULL) 
        {error = 1;
         continue;}
    
    for(match = tmp; match != NULL; match = match->next)
       {if(match->amplitude != event->amplitude) continue;
        if(match->GTimeS    != event->GTimeS)    continue;
        if(match->GTimeN    != event->GTimeN)    continue;
        if(event->data != NULL) FrVectFree(event->data);
        event->data = match->data;
        match->data = NULL;
        break;}
	
    FrEventFree(tmp);
    if(match == NULL) error = 2;}

 return(error);}
/*--------------------------------------------------FrFileIGetNextEventTime--*/
double FrFileIGetNextEventTime(FrFile *iFile,
                               char *name,
                               double tStart,
                               double aMin)
/*---------------------------------------------------------------------------*/
/* Return the next event with amplitude above ampMin                         */
/*---------------------------------------------------------------------------*/
{FrTOCevt *tocEvt;
 FrTag    *frtag;
 FrFileH  *firstFileH;
 double tEvent, tNext;
 int i;
 
 if(iFile == NULL) return(0);                 /*----- check input file ---*/

 FrTOCFrameFindNT(iFile, tStart);

 firstFileH = iFile->current;
 iFile->relocation = FR_NO;            /*--- to avoid relloaction problems---*/

 tNext = 1.e37;
 frtag = FrTagNew(name);
 if(frtag == NULL) return(0);
                                                /*--- Loop over all files--*/
 do
   {if(iFile->toc == NULL) FrTOCReadFull(iFile); /*--------- get the TOC----*/
    if(iFile->toc == NULL) return(0);

                  /*---------Find name in the channel list for this file----*/

    for(tocEvt = iFile->toc->event; tocEvt != NULL; tocEvt = tocEvt->next) 
      {if(FrTagMatch(frtag, tocEvt->name) == FR_NO) continue; 

       for(i=0; i<tocEvt->nEvent; i++)
         {if(tocEvt->amplitude[i] < aMin) continue;
          tEvent = tocEvt->GTimeS[i]+1.e-9*tocEvt->GTimeN[i];
          if(tEvent <= tStart) continue;
          if(tEvent < tNext) tNext = tEvent;}}

    if(tNext < 1.e13) break;}

 while (FrFileINext(iFile, 0, 0, firstFileH, FR_YES) == 0);

 FrTagFree(frtag); 
 if(tNext > 1.e36) tNext = 0.;
 
 return(tNext);}

/*------------------------------------------------------------ FrEventReadT--*/
FrEvent *FrEventReadT(FrFile *iFile,
                      char *name,
                      double tStart,
                      double length,
                      double amplitudeMin,
                      double amplitudeMax)
/*---------------------------------------------------------------------------*/
{
 return(FrEventReadTF(iFile, name, tStart, length, 0, 1, 
		      "amplitude", amplitudeMin, amplitudeMax));}

/*---------------------------------------------------------- FrEventReadTF--*/
FrEvent *FrEventReadTF(FrFile *iFile,
                       char *name,
                       double tStart,
                       double length,
                       int readData,       /* 0= no vector; 1= read vectors */
                       int nParam,
                       ...)
/*---------------------------------------------------------------------------*/
/* Find all events in this time window. Do not read associated table         */
/*---------------------------------------------------------------------------*/
{FrEvent  *root, *event, *last;
 FrTOCevt *tocEvt;
 FrTag    *frtag;
 FrFileH  *firstFileH;
 double tEvent, tEnd;
 float *pMin, *pMax, aMin, aMax;
 char **pNames, *pName;
 va_list ap;
 FRBOOL aCheck, select;
 int i, j, k;
                                          /*------- extract parameters ----*/
 if(nParam > 0)
   {pMin   = (float *) calloc(nParam, sizeof(float));
    pMax   = (float *) calloc(nParam, sizeof(float));
    pNames = (char **) calloc(nParam, sizeof(char *));
    if(pMin   == NULL) return (NULL);
    if(pMax   == NULL) return (NULL);
    if(pNames == NULL) return(NULL);

    va_start(ap,nParam);
    for(i=0; i<nParam; i++)
       {pName = va_arg(ap, char *);
        if(pName != NULL)
         {if(FrStrCpy(&(pNames[i]),pName) == NULL) return(NULL);}
        pMin[i] = va_arg(ap, double);
        pMax[i] = va_arg(ap, double);}
    va_end(ap);}
 else
   {pMin = NULL;
    pMax = NULL; 
    pNames = NULL;}
	                      /*------- should we check the amplitude? ---*/
 aMin = 0.;
 aMax = 0.;
 aCheck = FR_NO; 
 for(i=0; i<nParam; i++)
   {if(strcmp(pNames[i],"amplitude") != 0) continue; 
    aMin = pMin[i];
    aMax = pMax[i];
    aCheck = FR_YES;}  
 
 if(iFile == NULL) return(NULL);                 /*----- check input file ---*/
 firstFileH = iFile->current;
 iFile->relocation = FR_NO;            /*--- to avoid relloaction problems---*/

 tEnd = tStart + length;

 frtag = FrTagNew(name);
 if(frtag == NULL) return(NULL);
                                                 /*--- Loop over all files--*/
 root = NULL;
 last = NULL;
 do
   {if(iFile->toc == NULL) FrTOCReadFull(iFile); /*--------- get the TOC----*/
    if(iFile->toc == NULL) return(NULL);

                  /*---------Find name in the channel list for this file----*/

    for(tocEvt = iFile->toc->event; tocEvt != NULL; tocEvt = tocEvt->next) 
      {if(FrTagMatch(frtag, tocEvt->name) == FR_NO) continue; 

       for(i=0; i<tocEvt->nEvent; i++)
         {tEvent = tocEvt->GTimeS[i]+1.e-9*tocEvt->GTimeN[i];
          if(tEvent < tStart) continue;
          if(tEvent > tEnd)   continue;
          if(aCheck == FR_YES && tocEvt->amplitude[i] < aMin) continue;
          if(aCheck == FR_YES && tocEvt->amplitude[i] > aMax) continue;
          if(FrTOCSetPos(iFile, tocEvt->position[i]) != 0) continue;
          event = FrEventRead(iFile);

          select = FR_YES;           /*----- additional selection criteria --*/
          for(j=0; j<nParam; j++)
             {if(strcmp(pNames[j],"amplitude") == 0) continue;
              if(strcmp(pNames[j],"timeBefore") == 0)
               {if(event->timeBefore < pMin[j]) select = FR_NO;  
                if(event->timeBefore > pMax[j]) select = FR_NO;}  
              if(strcmp(pNames[j],"timeAfter") == 0)
               {if(event->timeAfter < pMin[j]) select = FR_NO;  
                if(event->timeAfter > pMax[j]) select = FR_NO;}  
              for(k=0; k<event->nParam; k++)
                {if(strcmp(pNames[j],event->parameterNames[k]) == 0)
                  {if(event->parameters[k] < pMin[j]) select = FR_NO;  
                   if(event->parameters[k] > pMax[j]) select = FR_NO;}}}  
          if(select == FR_NO) 
             {FrEventFree(event);
              continue;}

          if(readData == 1)
            {event->data = FrVectReadNext(iFile, tEvent, event->name);}
 
          if(root == NULL) root = event;
          else last->next = event;
          last = event;}}}

    while (FrFileINext(iFile, tStart, length, firstFileH, FR_YES) == 0);

                            /*--------------------- free working space -----*/
 FrTagFree(frtag); 
                                          
 if(nParam > 0)
   {for(i=0; i<nParam; i++) {if(pNames[i] != NULL) free(pNames[i]);}
    free(pMin);
    free(pMax);
    free(pNames);}

 return(root);}

/*--------------------------------------------------------- FrEventReadTF1--*/
FrEvent *FrEventReadTF1(FrFile *iFile,  char *name,
                        double tStart,  double length, int readData,  
                        char *pName1, double min1, double max1)
/*---------------------------------------------------------------------------*/
{
 return(FrEventReadTF(iFile, name, tStart, length, readData, 1, 
                      pName1, min1, max1));}

/*--------------------------------------------------------- FrEventReadTF2--*/
FrEvent *FrEventReadTF2(FrFile *iFile,  char *name,
                        double tStart,  double length, int readData,  
                        char *pName1, double min1, double max1,
                        char *pName2, double min2, double max2)
/*---------------------------------------------------------------------------*/
{
 return(FrEventReadTF(iFile, name, tStart, length, readData, 2, 
                      pName1, min1, max1,
                      pName2, min2, max2));}

/*--------------------------------------------------------- FrEventReadTF3--*/
FrEvent *FrEventReadTF3(FrFile *iFile,  char *name,
                        double tStart,  double length, int readData,  
                        char *pName1, double min1, double max1,
                        char *pName2, double min2, double max2,
                        char *pName3, double min3, double max3)
/*---------------------------------------------------------------------------*/
{
 return(FrEventReadTF(iFile, name, tStart, length, readData, 3, 
                      pName1, min1, max1,
                      pName2, min2, max2,
                      pName3, min3, max3));}

/*--------------------------------------------------------- FrEventReadTF4--*/
FrEvent *FrEventReadTF4(FrFile *iFile,  char *name,
                        double tStart,  double length, int readData,  
                        char *pName1, double min1, double max1,
                        char *pName2, double min2, double max2,
                        char *pName3, double min3, double max3,
                        char *pName4, double min4, double max4)
/*---------------------------------------------------------------------------*/
{
 return(FrEventReadTF(iFile, name, tStart, length, readData, 4, 
                      pName1, min1, max1,
                      pName2, min2, max2,
                      pName3, min3, max3,
                      pName4, min4, max4));}

/*--------------------------------------------------------- FrEventReadTF5--*/
FrEvent *FrEventReadTF5(FrFile *iFile,  char *name,
                        double tStart,  double length, int readData,  
                        char *pName1, double min1, double max1,
                        char *pName2, double min2, double max2,
                        char *pName3, double min3, double max3,
                        char *pName4, double min4, double max4,
                        char *pName5, double min5, double max5)
/*---------------------------------------------------------------------------*/
{
 return(FrEventReadTF(iFile, name, tStart, length, readData, 5, 
                      pName1, min1, max1,
                      pName2, min2, max2,
                      pName3, min3, max3,
                      pName4, min4, max4,
                      pName5, min5, max5));}

/*--------------------------------------------------------FrEventSaveOnFile--*/
int FrEventSaveOnFile(FrEvent* event, FrFile* oFile)
/*---------------------------------------------------------------------------*/
{FrameH *frame;
 FrEvent *nextEvent;

 if(oFile == NULL) return(-2);
 if(event == NULL) return(-3);

  frame = FrameNew("Event_Hook");
  if(frame == NULL) return(-1);

  frame->dt = 0;
  frame->GTimeS = event->GTimeS;
  frame->GTimeN = event->GTimeN;
  frame->event = event;

  nextEvent = event->next;
  event->next = NULL;

  FrameWrite(frame, oFile);

  frame->event  = NULL;
  event->next = nextEvent;

  FrameFree(frame);

  return(0);
}

/*------------------------------------------------------------FrEventWrite--*/
void FrEventWrite(FrEvent *event,
                  FrFile *oFile)
/*--------------------------------------------------------------------------*/
{
 if(oFile->toc != NULL) FrTOCevtMark(oFile, &(oFile->toc->event), 
               event->name, event->GTimeS, event->GTimeN, event->amplitude);

 FrPutNewRecord(oFile, event, FR_YES);
 FrPutSChar (oFile, event->name);
 FrPutSChar (oFile, event->comment);
 FrPutSChar (oFile, event->inputs);
 FrPutIntU  (oFile, event->GTimeS);
 FrPutIntU  (oFile, event->GTimeN);
 FrPutFloat (oFile, event->timeBefore);
 FrPutFloat (oFile, event->timeAfter);
 FrPutIntU  (oFile, event->eventStatus);
 FrPutFloat (oFile, event->amplitude);
 FrPutFloat (oFile, event->probability);
 FrPutSChar (oFile, event->statistics);
 FrPutShortU(oFile, event->nParam);
 if(event->nParam > 0)   
   {if(FrFormatVersion == 6)
         FrPutVFD(oFile, event->parameters, event->nParam);
    else FrPutVD (oFile, event->parameters, event->nParam);
    FrPutVQ (oFile, event->parameterNames, event->nParam);}
 FrPutStruct(oFile, event->data);
 FrPutStruct(oFile, event->table);
 FrPutStruct(oFile, event->next);
 FrPutWriteRecord(oFile, FR_NO);

 if(event->data != NULL) FrVectWrite (event->data, oFile);
 if(event->table!= NULL) FrTableWrite(event->table, oFile);

 if(event->next != NULL) FrEventWrite(event->next, oFile);
  
 return;}

/*-------------------------------------------------------------FrCompareGPS--*/
int FrCompareGPS(const void *g1, const void *g2)
/*---------------------------------------------------------------------------*/
/* This function compare two GPS time saved in void* pointers                */
/*---------------------------------------------------------------------------*/
{
  long delta, *gps1, *gps2;

  gps1 = (long*) g1;
  gps2 = (long*) g2;
  delta = *gps1 - *gps2;
  return((int)delta);
}
/*----------------------------------------------------------FrFileSortByGPS--*/
FrFileH* FrFileSortByGPS(FrFileH *fileHRoot)
/*---------------------------------------------------------------------------*/
/* This function check that the files are ordered by increasing GPS time.    */
/* If not, it reorder the linked list and returns the new root pointer or    */
/* NULL in case of error                                                     */
/*---------------------------------------------------------------------------*/
{
  FrFileH *fileH;
  int nBad, nFiles, lastGTime, i;
  void **base;

  /*---------------------------------- first check the order ---*/
  lastGTime = -1;
  nFiles = 0;
  nBad = 0;
  for(fileH = fileHRoot; fileH != NULL; fileH = fileH->next) {
    nFiles++;
    if(lastGTime > fileH->tStart) nBad++;
    lastGTime = fileH->tStart;}

  if(nBad == 0)  return(fileHRoot);

  /*--------------- files are not properly ordered. Try to fix it ---*/
  base = malloc(nFiles*2*sizeof(void*));
  if(base == NULL) {
    sprintf(FrErrMsg,"Bad time ordering for file %s; enable to fix it",
	    fileHRoot->fileName);
    FrError(3,"FrFileBreakName",FrErrMsg);
    return(NULL);}

  /*--------------------------------------------- fill the table---*/
  i = 0;
  for(fileH = fileHRoot; fileH != NULL; fileH = fileH->next) {
    base[i] = (void *) ((long)fileH->tStart);
    i++;
    base[i] = (void *) fileH;
    i++;}

  qsort(base, nFiles, 2*sizeof(void*), FrCompareGPS);

  /*-------------------- rebuild the linked list---*/
  fileHRoot = (FrFileH*) base[1];

  for(i=0; i<nFiles-1; i++) {
    fileH = (FrFileH*) base[2*i+1];
    fileH->next =      base[2*i+3];}

  fileH = (FrFileH*) base[2*nFiles-1];
  fileH->next = NULL;

  free(base);

  return(fileHRoot);}

/*----------------------------------------------------------FrFileBreakName--*/
FrFileH* FrFileBreakName(char   *fullName,
                         FrFileH *next,
                         FrSegList** segList)
/*---------------------------------------------------------------------------*/
/* This function is looking for space character to breaks a string in words. */
/* It returns a link list of FrFileHeader structures.                        */
/* We assume that the file names start with a letter                         */
/*---------------------------------------------------------------------------*/
{FrFileH **root, *fileH, *fileHRoot;
 char  *token;
 FRBOOL ffl;
 FILE  *fp=NULL;
 FrTag *list=NULL, *tag=NULL;
 int length;

 if(fullName == NULL) return(NULL);

 if(strstr(fullName, ".cache") != NULL) return(FrFileOpenCacheFile(fullName));

 root = &fileHRoot;

 if(fullName == NULL) return(NULL);

                /*----- if the extension is .ffl (frame file list) 
                                          we read the list from  a file ----*/

 if(strstr(fullName, ".ffl") == NULL)
    {ffl = FR_NO;
     list = FrTagNew(fullName);
     tag = list;
     if(tag == NULL) 
          {token = NULL;}
     else {token = tag->start;}}
 else 
   {ffl = FR_YES;
    fp = fopen(fullName,"r");
    if(fp == NULL) 
       {FrError(3,"FrFileBreakName","open ffl file failed");
        return(NULL);}
    token = FrBuf;
    if(fscanf(fp,"%s",FrBuf) != 1) token = NULL;}

               /*----------------------------- start the loop on each file --*/

 while(token != NULL)

               /*--first check if we have a segment list(works only for ffl)-*/
   {if(token[0] == 'S' && ffl == FR_YES)  
       {if(strcmp(token,"SEGMENT_LIST_START") == 0 && segList != NULL)
	   {*segList = FrSegListReadFP(fp,"ffl");
            if(fscanf(fp,"%s",FrBuf) != 1) token = NULL;
	    continue;}}

    fileH = (FrFileH *) malloc(sizeof(FrFileH));
    if(fileH == NULL) return(NULL);

    if(ffl == FR_NO) 
         length = tag->length;
    else length = strlen(FrBuf);
    fileH->fileName = malloc(length+1);
    if(fileH->fileName == NULL) break;
    
    strncpy(fileH->fileName, token, length);
    fileH->fileName[length] = '\0';

    fileH->tStart = -1.;
    fileH->length = 0;
    fileH->tFirstEvt = -1.;
    fileH->tLastEvt  = -1;
    fileH->next   = next;
    *root = fileH;
    root = &(fileH->next);

               /*----------------------------- get the starting time if any-*/

    if(ffl == FR_NO)
         {if((tag = tag->next) == NULL) break;
          token = tag->start;}
    else {if(fscanf(fp,"%s",FrBuf) != 1) token = NULL;}
    if(token == NULL) break;

    if(token[0] != '-' && isdigit((int)token[0]) == 0) continue;
    fileH->tStart = atof(token);
    if(fileH->tStart == -1) fileH->tStart = -2;

               /*------------------------------------get the end time if any-*/

    if(ffl == FR_NO)
         {if((tag = tag->next) == NULL) break;
          token = tag->start;}
    else {if(fscanf(fp,"%s",FrBuf) != 1) token = NULL;}
    if(token == NULL) break;

    if(isdigit((int)token[0]) == 0) 
      {fileH->tStart = -1.;
       continue;}
    fileH->length = atof(token);

               /*---------------------------- get the first event time if any-*/

    if(ffl == FR_NO)
         {if((tag = tag->next) == NULL) break;
          token = tag->start;}
    else {if(fscanf(fp,"%s",FrBuf) != 1) token = NULL;}
    if(token == NULL) break;

    if(token[0] != '-' && isdigit((int)token[0]) == 0) continue;
    fileH->tFirstEvt =atof(token);

               /*----------------------------------get the event range if any-*/

    if(ffl == FR_NO)
         {if((tag = tag->next) == NULL) break;
          token = tag->start;}
    else {if(fscanf(fp,"%s",FrBuf) != 1) token = NULL;}
    if(token == NULL) break;

    if(token[0] != '-' && isdigit((int)token[0]) == 0) 
      {fileH->tFirstEvt = -1.;
       continue;}
    fileH->tLastEvt = atof(token);

              /*------------------------------------ get the next file name --*/

    if(ffl == FR_NO)
         {if((tag = tag->next) == NULL) break;
          token = tag->start;}
    else {if(fscanf(fp,"%s",FrBuf) != 1) token = NULL;}}

               /*------------------------------ a little bit of cleaning ------*/

 if(ffl == FR_NO) 
      {FrTagFree(list);}
 else {fclose(fp);}

               /*---drop files with tStart = -1 since they are unreadable ---*/

 if(ffl == FR_YES)
   {for(root = &fileHRoot; *root != NULL;)
     {if((*root)->tStart != -2) 
        {root = &((*root)->next);}
      else
        {fileH = (*root)->next;
         free((*root)->fileName);
         free((*root));
         *root = fileH;}}

                        /*-----------------------read sub-ffl if needed------*/
   for(root = &fileHRoot; *root != NULL;)
     {if(strstr((*root)->fileName, ".ffl") == NULL) 
        {root = &((*root)->next);}
      else
        {fileH = FrFileBreakName((*root)->fileName, (*root)->next, NULL);
         free((*root)->fileName);
         free((*root));
         *root = fileH;}}

                     /*-----check the time ordering for the top linked list -*/
   fileHRoot = FrFileSortByGPS(fileHRoot);}

 return(fileHRoot);}

/*--------------------------------------------------------------- FrFileDbg -*/
void FrFileDbg(FrFile *file)
/*---------------------------------------------------------------------------*/
{int i,size;
  char *b;
  unsigned c;
 
  if(FrDebugLvl < 1) return;
  if(file->inMemory == FR_YES)
    {size = file->p - file->buf;
     fprintf(FrFOut," \n Buffer size = %d\n Buffer:\n\n",size);
     b = file->buf;
     for(i=0; i<size; i=i+16)
           {fprintf(FrFOut," %10d  ",i); 
            c = b[i+0]; fprintf(FrFOut,"%2.2x ",c);
            c = b[i+1]; fprintf(FrFOut,"%2.2x ",c);
            c = b[i+2]; fprintf(FrFOut,"%2.2x ",c);
            c = b[i+3]; fprintf(FrFOut,"%2.2x  ",c);
            c = b[i+4]; fprintf(FrFOut,"%2.2x ",c);
            c = b[i+5]; fprintf(FrFOut,"%2.2x ",c);
            c = b[i+6]; fprintf(FrFOut,"%2.2x ",c);
            c = b[i+7]; fprintf(FrFOut,"%2.2x   ",c);
            c = b[i+8]; fprintf(FrFOut,"%2.2x ",c);
            c = b[i+9]; fprintf(FrFOut,"%2.2x ",c);
            c = b[i+10]; fprintf(FrFOut,"%2.2x ",c);
            c = b[i+11]; fprintf(FrFOut,"%2.2x  ",c);
            c = b[i+12]; fprintf(FrFOut,"%2.2x ",c);
            c = b[i+13]; fprintf(FrFOut,"%2.2x ",c);
            c = b[i+14]; fprintf(FrFOut,"%2.2x ",c);
            c = b[i+15]; fprintf(FrFOut,"%2.2x\n",b[i+15]);}} 
  
  return;}

/*-------------------------------------------------------------- FrFileFree--*/
void FrFileFree(FrFile *file)
/*---------------------------------------------------------------------------*/
{FrFileH *fileH, *next;

  fileH = file->fileH;
  while(fileH != NULL)
    {free(fileH->fileName);
     next = fileH->next;
     free(fileH);
     fileH = next;}

  FrDicFree(file);

  FrTOCFree(file->toc);

  if(file->sh != NULL) free(file->sh);

  if(file->historyMsg != NULL) free(file->historyMsg);
  if(file->path       != NULL) free(file->path);

  free(file);

  return;
}

/*--------------------------------------------------------- FrFileHeader-----*/
void FrFileHeader(unsigned char *buf)
/*---------------------------------------------- define the format version --*/
/* format version 3 is valid since FrameLib version 3.0 (July 97)            */
/* format version 2 is valid since FrameLib version 2.3 (May 97)             */
/* format version 1 was valid up to the FrameLib version 2.24                */
/*---------------------------------------------------------------------------*/
{unsigned short swapS;
 unsigned int swapI,i, swapL1, swapL2;
 double dPi;
 float fPi;
 int formatVersion;
 int libMinorVersion;
 char *swapC;

 sscanf(FrVersion,"%d.%d",&formatVersion,&libMinorVersion);

 for(i=0; i<40; i++) {buf[i] = 0;}

 strcpy((char *) buf,"IGWD");
 buf[5]  = FrFormatVersion;
 buf[6]  = libMinorVersion;
 buf[7]  = FrSshort;
 buf[8]  = FrSint;
 buf[9]  = 8;
 buf[10] = FrSfloat;
 buf[11] = FrSdouble;
 swapS = 0x1234;         
 swapI = 0x12345678; 
 swapL1 = 0;
 swapL2 = 0;
 for (i=0; i<8; i++)
    {swapL1 = swapL1*16 + i;   
     swapL2 = swapL2*16 + i+8;}
 fPi = 3.1415926535897932384626; 
 dPi = 3.1415926535897932384626;
 memcpy(&buf[12],&swapS,2);
 memcpy(&buf[14],&swapI,FrSint);
 swapC = (char *) &swapS;
 if(*swapC == 0x12)
     {memcpy(&buf[18],&swapL1,4);
      memcpy(&buf[22],&swapL2,4);}
 else{memcpy(&buf[22],&swapL1,4);
      memcpy(&buf[18],&swapL2,4);}
 memcpy(&buf[26],&fPi,FrSfloat);
 memcpy(&buf[30],&dPi,FrSdouble);
 buf[38] = 'A';
 buf[39] = 'Z';

 if(FrFormatVersion == 8) 
   {buf[38] = 1;
    buf[39] = 0;}

   
 return;}
/*------------------------------------------------------------- FrFileIncSH--*/
void FrFileIncSH(FrFile *iFile,
                 unsigned short id)
/*---------------------------------------------------------------------------*/
{FrSH **old;
 int i;

          /*--- check the space for the direct acces table ----*/
          
 if(id < iFile->maxSH) return;
 
 old = iFile->sh;
 iFile->sh = calloc(id+10, sizeof(FrSH*));
 if(iFile->sh == NULL)
                {iFile->error = FR_ERROR_MALLOC_FAILED;
                 return;} 
 for(i=0; i<iFile->maxSH; i++)
               {iFile->sh[i] = old[i];}
 iFile->maxSH = id+10;
 free(old);

 return;}

/*----------------------------------------------------------------FrFileNew--*/
FrFile *FrFileNew(char *fileName, 
                  int   compress, 
                  char *buf, 
                  FRLONG  bufSize)
/*---------------------------------------------------------------------------*/
{FrFile *file;
 FrFileH *fileH;
 int i;
   
  FrLibShortIni();                       /*---- do a short init if needed ---*/
 
  file = (FrFile *) calloc(1,sizeof(FrFile));
  if(file == NULL) 
    {FrError(3,"FrFileNew","malloc struct file failed");
     return(NULL);}

  file->inMemory = FR_YES;

  if(fileName != NULL) 
    {file->fileH   = FrFileBreakName(fileName, NULL, &(file->segList));
     if(file->fileH == NULL) 
       {FrError(3,"FrFileNew","break name failed");
        return(NULL);}
     FrFileISegListMatch(file);
     if(file->fileH == NULL) 
       {FrError(3,"FrFileNew","No file left after segment list selection");
        return(NULL);}
     file->current = file->fileH;
     if(strcmp(file->fileH->fileName,"STRING") != 0) file->inMemory = FR_NO;
     if(FrDebugLvl > 5)
       {fprintf(FrFOut,"FrFileNew: (start/stop time and names)");
        for(fileH = file->fileH; fileH != NULL; fileH = fileH->next)
              {fprintf(FrFOut,"%s\t%f\t%f\n",  fileH->fileName, 
                                fileH->tStart, fileH->length);}
        FrSegListDump(file->segList, stdout, 2);}}

  file->compress = compress;
  file->gzipLevel = 1;
  file->run = -1;
  file->frfd= NULL;
  file->p   = NULL;
  file->pMax = file->p + 99999999;
  file->bufSize = 0;
  file->buf       = NULL;
  file->sDataSim  = NULL;
  file->sDataProc = NULL;
  file->error   = 0;
  file->nBytes  = 0;
  file->nFrames = 0;
  file->relocation   = FR_YES;
  file->noTOCts      = FR_NO; 
  file->chkSumFrFlag = FR_YES;
  file->chkSumFiFlag = FR_YES;
  file->chkSumFr = 0;
  file->chkSumFi = 0;
  file->convertH = FR_NO;
  file->aligned  = FR_YES;

  file->maxSH = 30; 
  file->sh = calloc(file->maxSH, sizeof(FrSH*));
  if(file->sh == NULL)
    {FrError(3,"FrFileNew","malloc struct sh failed");
     return(NULL);}

  file->sh[1] = FrSHRoot[0];
  file->sh[2] = FrSHRoot[1];     /*--- the FrSH and FrSE are always 1 and 2--*/
  
                 /*--------------------------------- check tempory space  ---*/

  file->buf = buf;
  file->bufSize = bufSize;
  file->p    = file->buf;
  file->pMax = file->p + bufSize;

                             /*----------------- default history message ----*/

  sprintf(FrBuf,"FrameLib:%s",FrVersion);
  FrStrCpy(&(file->historyMsg), FrBuf);
 
                                          /*---- reset dictionary output ----*/
  for(i=0; i<FRMAPMAXSH; i++)
     {file->dicWrite[i] = FR_NO;}

  file->detectorType = 0;

  return(file);
}
/*-------------------------------------------------------FrFileIAddSegList--*/
void FrFileIAddSegList(FrFile *iFile, FrSegList *segList)
/*--------------------------------------------------------------------------*/
{FrSegList *oldSegList;

  if(iFile   == NULL) return;
  if(segList == NULL) return;

  if(iFile->segList == NULL) 
    {iFile->segList = segList;}
  else
    {oldSegList = iFile->segList;
     iFile->segList = FrSegListIntersect(oldSegList, segList);
     FrSegListFree(oldSegList);}


  FrFileISegListMatch(iFile);     /*--- update the internal ffl -----*/

  return;}
/*-------------------->>>>>>>>>>>>>>>>obsolete function-FrFileIChannelList--*/
char *FrFileIChannelList(FrFile *iFile, int mode)
/*--------------------------------------------------------------------------*/
{
  return(FrFileIGetChannelList(iFile, 0));}

/*---------------------------------------------------FrFileIGetChannelList--*/
char *FrFileIGetChannelList(FrFile *iFile, int gtime)
/*--------------------------------------------------------------------------*/
/* This function allocate the space and returns the pointer to a string with 
the channel list and frequency information. The user should free the space. 
If gtime == 0, the channel list is return for the current file position or 
for the beginning of the file if no frame has been read                     */
/*--------------------------------------------------------------------------*/
{int i, fill, size, last, nextSize;
 FrTOCts *ts;
 FrTOC *toc;
 FrAdcData *adc;
 FrSimData *sim;
 FrSerData *ser;
 FrProcData *proc;
 FrTOCevt *event;
 FrFileH *lastF;
 char *buffer, *buf;
 double sampleRate;

 if(iFile == NULL) return(NULL);

 if(gtime > 0) FrTOCFrameFindT(iFile, gtime); /*--set ffl to the right file-*/

 else if (gtime < 0)    /* got to the last file */
   {for(lastF = iFile->fileH; lastF->next != NULL; lastF = lastF->next) {;}
    if(iFile->current != lastF)
      {FrFileIClose(iFile);
       iFile->current = lastF;}}

 if(iFile->toc == NULL)  FrTOCReadFull(iFile);

 iFile->relocation = FR_NO;            /*--- to avoid relloaction problems---*/

 toc = iFile->toc; 
 if(toc == NULL)      return(NULL);
 if(toc->nFrame == 0) return(NULL); 

                     /*----------------- create output buffer ---------------*/

 size = 500;
 buffer = malloc(size);
 if(buffer == NULL) return(NULL);
 buf = buffer;
                             /*-------------- Frame information ----------*/

 buf += sprintf(buf,"FIRSTFRAME %d %d %f %d %d\n",toc->GTimeS[0],
                toc->ULeapS, toc->dt[0],toc->frame[0],toc->runs[0]);
 last = toc->nFrame-1;
 buf += sprintf(buf,"LASTFRAME  %d %d %f %d %d\n",toc->GTimeS[last],
          toc->ULeapS, toc->dt[last],toc->frame[last],toc->runs[last]);

               /*for each ADC, find the first ADC and read it -------------*/

for(ts = toc->adc; ts != NULL; ts = ts->next)
   {for(i=0; i<toc->nFrame; i++) {if(ts->position[i] != 0) break;}

    if(FrTOCSetPos(iFile, ts->position[i]) != 0) continue;
    adc = FrAdcDataRead(iFile);
    if(adc == NULL) continue;

    fill = buf - buffer;
    nextSize = fill + 50 + strlen(adc->name);
    if(nextSize > size) 
      {size = 2*nextSize;
       if((buffer  = realloc(buffer,size))== NULL) return(NULL);
       buf = buffer+fill;}

    buf += sprintf(buf,"ADC %s\t%g 1. 0.\n", adc->name, adc->sampleRate);
    FrAdcDataFreeOne(adc);}
 
               /*for each Sim, find the first Sim and read it -------------*/

for(ts = toc->sim; ts != NULL; ts = ts->next)
   {for(i=0; i<toc->nFrame; i++) {if(ts->position[i] != 0) break;}

    if(FrTOCSetPos(iFile, ts->position[i]) != 0) continue;
    sim = FrSimDataRead(iFile);
    if(sim == NULL) continue;

    fill = buf - buffer;
    nextSize = fill + 50 + strlen(sim->name); 
    if(nextSize > size) 
      {size = 2*nextSize;
       if((buffer  = realloc(buffer,size))== NULL) return(NULL);
       buf = buffer+fill;}

    buf += sprintf(buf,"SIM %s\t%g 1. 0.\n", sim->name, sim->sampleRate);
    sim->data = NULL;
    FrSimDataFree(sim);}
 
               /*for each Proc, find the first proc and read it ------------*/

for(ts = toc->proc; ts != NULL; ts = ts->next)
   {for(i=0; i<toc->nFrame; i++) {if(ts->position[i] != 0) break;}

    if(FrTOCSetPos(iFile, ts->position[i]) != 0) continue;
    proc = FrProcDataRead(iFile);
    if(proc == NULL) continue;
    proc->data = FrVectReadNext(iFile, 0, NULL);

    fill = buf - buffer;
    nextSize = fill + 50 + strlen(proc->name);
    if(nextSize > size) 
      {size = 2*nextSize;
       if((buffer  = realloc(buffer,size))== NULL) return(NULL);
       buf = buffer+fill;}

    sampleRate = 0.;
    if(proc->data != NULL)
      {if(proc->data->dx[0] != 0.) 
        {sampleRate = 1./proc->data->dx[0];}}
    buf += sprintf(buf,"PROC %s\t%g 1. 0.\n", proc->name, sampleRate);
    FrProcDataFree(proc);}

              /*--------------------- add the list of events ------------*/

 if(toc->event != NULL)
   {if((fill = buf - buffer) > size - 120) 
      {size = 2*size;
       if((buffer  = realloc(buffer,size))== NULL) return(NULL);
       buf = buffer+fill;}

    buf += sprintf(buf,"PROC EVENT_SNR:ALL\t20000. 1. 0.\n");}

 for(event = toc->event; event != NULL; event = event->next)
   {fill = buf - buffer;
    nextSize = fill + 50 + strlen(event->name);
    if(nextSize > size) 
      {size = 2*nextSize;
       if((buffer  = realloc(buffer,size))== NULL) return(NULL);
       buf = buffer+fill;}

    buf += sprintf(buf,"PROC EVENT_SNR:%s\t20000. 1. 0.\n",event->name);}

              /*----------- add the list of simulated events ------------*/

 if(toc->simEvt != NULL)
   {if((fill = buf - buffer) > size - 120) 
      {size = 2*size;
       if((buffer  = realloc(buffer,size))== NULL) return(NULL);
       buf = buffer+fill;}

    buf += sprintf(buf,"PROC SIM_EVENT_SNR:ALL\t20000. 1. 0.\n");}

 for(event = toc->simEvt; event != NULL; event = event->next)
   {fill = buf - buffer;
    nextSize = fill + 50 + strlen(event->name);
    if(nextSize > size) 
      {size = 2*nextSize;
       if((buffer  = realloc(buffer,size))== NULL) return(NULL);
       buf = buffer+fill;}

    buf += sprintf(buf,"PROC SIM_EVENT_SNR:%s\t20000. 1. 0.\n",event->name);}

               /*for each Ser, find the first ser and read it -------------*/

for(ts = toc->ser; ts != NULL; ts = ts->next)
   {for(i=0; i<toc->nFrame; i++) {if(ts->position[i] != 0) break;}

    if(FrTOCSetPos(iFile, ts->position[i]) != 0) continue;
    ser = FrSerDataRead(iFile);
    if(ser == NULL) continue;
    if(ser->data == NULL) continue;

    fill = buf - buffer;
    nextSize = fill + 50 + strlen(ser->name) + strlen(ser->data);
    if(nextSize > size) 
      {size = 2*nextSize;
       if((buffer  = realloc(buffer,size))== NULL) return(NULL);
       buf = buffer+fill;}

    buf += sprintf(buf,"SER %s\t%f %s\n",ser->name, ser->sampleRate, ser->data);
    ser->data = NULL;
    FrSerDataFree(ser);}
                           /*----- add frameH info ------------------*/

 fill = buf - buffer;
 nextSize = fill + 250;
 if(nextSize > size) 
    {size = 2*nextSize;
     if((buffer  = realloc(buffer,size))== NULL) return(NULL);
     buf = buffer+fill;}
 buf += sprintf(buf,"SER FrameH\t%f run 0 frame 0 dataQuality 0 GTimeS 0"
                   " GTimeN 0 ULeapS 0 dt 0\n",1./toc->dt[0]);

 return(buffer);}

/*-------------------------------------------------------------FrFileIClose--*/
void FrFileIClose(FrFile *iFile)
/*---------------------------------------------------------------------------*/
/* Close the current file and free all associated space                      */
/*---------------------------------------------------------------------------*/
{
 if(iFile == NULL) return;
  
 if(iFile->frfd != NULL) 
     {FrIOClose(iFile->frfd);
      iFile->frfd = NULL;}

 FrStatDataFree(iFile->sDataSim);
 FrStatDataFree(iFile->sDataProc);
 iFile->sDataSim  = NULL;
 iFile->sDataProc = NULL;

 FrDicFree(iFile);

 FrTOCFree(iFile->toc);
 iFile->toc = NULL;

 return;}
  
/*--------------------------------------------------------------FrFileIDump--*/
void FrFileIDump(FrFile *iFile,
                 FILE *fp,
                 int debugLvl,
                 char *tag) 
/*---------------------------------------------------------------------------*/
{ 
  FrFileIDumpT(iFile, fp, debugLvl, tag, 0, 2000000000);

  return;}

/*-------------------------------------------------------------FrFileIDumpT--*/
void FrFileIDumpT(FrFile *iFile,
                  FILE *fp,
                  int debugLvl,
                  char *tag,
                  double tStart,
                  double tEnd) 
/*---------------------------------------------------------------------------*/
/* This function dump a summary of the current file                          */
/*---------------------------------------------------------------------------*/
{FrFileH *fileH;
 FrVect *vect;
 FRLONG i;
 char s1[20], s2[20], s3[20], s4[20];

 if(iFile == NULL) return;
 if(debugLvl < 1)  return; 

       /*----- This call will scan all the file and build the list of time --*/

 if(iFile->fileH->next != NULL) FrFileIRewind(iFile);
 FrTOCFFLBuild(iFile);

       /*------------debugLvl == 1 Dump all file names, start and end time --*/

 for(fileH = iFile->fileH; fileH != NULL; fileH = fileH->next)
   {if(fileH->tStart+fileH->length < tStart &&
       fileH->tLastEvt              < tStart) continue;
    if(fileH->tStart > tEnd && fileH->tFirstEvt > tEnd)   continue;
    if(fmod(fileH->tStart,   1.) == 0) sprintf(s1,"%.0f",fileH->tStart);
    else                               sprintf(s1,"%.6f",fileH->tStart);
    if(fmod(fileH->length,   1.) == 0) sprintf(s2,"%.0f",fileH->length);
    else                               sprintf(s2,"%.6f",fileH->length);
    if(fmod(fileH->tFirstEvt,1.) == 0) sprintf(s3,"%.0f",fileH->tFirstEvt);
    else                               sprintf(s3,"%.6f",fileH->tFirstEvt);
    if(fmod(fileH->tLastEvt, 1.) == 0) sprintf(s4,"%.0f",fileH->tLastEvt);
    else                               sprintf(s4,"%.6f",fileH->tLastEvt);
    fprintf(fp,"%s\t%s %s  %s %s\n", fileH->fileName, s1, s2, s3, s4);}

 if(iFile->segList != NULL) 
    {fprintf(fp,"Attached segment list:\n");
     FrSegListDump(iFile->segList, fp, debugLvl);}

 if(debugLvl < 2)  return; 

      /*-------------------------------------------- Dump a TOC summary, ----*/
 
 if(debugLvl == 2)
   {FrFileIDumpFr (iFile, fp, debugLvl, 0., 1.e10);

    vect = FrFileIGetAdcNames(iFile);
    if(vect != NULL)
      {fprintf(fp," ADC :%6"FRLLD" type of AdcData :",vect->nData);
       for(i=0; i<vect->nData; i++)
         {if(i>50) break;
          if(i%5 == 0) fprintf(fp,"\n   ");
          fprintf(fp," \t%s",vect->dataQ[i]);}
       if(vect->nData>50) fprintf(fp," \t...");
       fprintf(fp,"\n");
       FrVectFree(vect);}

    vect = FrFileIGetSerNames(iFile);
    if(vect != NULL)
      {fprintf(fp," Ser :%6"FRLLD" type of SerData :",vect->nData);
       for(i=0; i<vect->nData; i++)
         {if(i>5) break;
          fprintf(fp," %s",vect->dataQ[i]);}
       if(vect->nData>5) fprintf(fp," ...");
       fprintf(fp,"\n");
       FrVectFree(vect);}

    vect = FrFileIGetProcNames(iFile);
    if(vect != NULL)
      {fprintf(fp," Proc:%6"FRLLD" type of ProcData:",vect->nData);
       for(i=0; i<vect->nData; i++)
         {if(i>5) break;
          fprintf(fp," %s",vect->dataQ[i]);}
       if(vect->nData>5) fprintf(fp," ...");
       fprintf(fp,"\n");
       FrVectFree(vect);}

    vect = FrFileIGetSimNames(iFile);
    if(vect != NULL)
      {fprintf(fp," Sim :%6"FRLLD" type of SimData :",vect->nData);
       for(i=0; i<vect->nData; i++)
         {if(i>5) break;
          fprintf(fp," %s",vect->dataQ[i]);}
       if(vect->nData>5) fprintf(fp," ...");
       fprintf(fp,"\n");
       FrVectFree(vect);}

    vect = FrFileIGetDetectorNames(iFile);
    if(vect != NULL)
      {fprintf(fp," Detector: %"FRLLD" type of Detector:",vect->nData);
       for(i=0; i<vect->nData; i++)
         {fprintf(fp," %s",vect->dataQ[i]);}
       fprintf(fp,"\n");
       FrVectFree(vect);}

    vect = FrFileIGetStatNames(iFile);
    if(vect != NULL)
      {fprintf(fp," StatData: %"FRLLD" type of StatData:",vect->nData);
       for(i=0; i<vect->nData; i++)
         {if(i>5) break;
          fprintf(fp," %s",vect->dataQ[i]);}
       if(vect->nData>5) fprintf(fp," ...");
       fprintf(fp,"\n");
       FrVectFree(vect);}

    FrFileIDumpEvt(iFile, fp, debugLvl); 
    return;}

           /*----------------------------------------- if debugLvl >= 3 -----*/
  
 fprintf(fp," Frame time range: %17.6f to %17.6f\n"
            " Event time range: %17.6f to %17.6f\n", 
       FrFileITStart(iFile),   FrFileITEnd(iFile),
       FrFileITFirstEvt(iFile),FrFileITLastEvt(iFile));

           /*--------------------------------- Close the previous file ------*/

 if(iFile->current != iFile->fileH) FrFileIClose(iFile);

           /*------------------------------------- Loop over all files ------*/

 for(iFile->current = iFile->fileH; iFile->current != NULL;
        iFile->current = iFile->current->next)
      {fprintf(fp," For file: %s\n",iFile->current->fileName);
       if(iFile->toc == NULL) FrTOCReadFull(iFile);  
       if(iFile->toc == NULL) 
            fprintf(fp," NO TOC available");
       else FrTOCDump(iFile->toc, fp, debugLvl-1, tag);

          /*--------Close the file if we have more than one file-------------*/

      if(iFile->fileH->next != NULL) FrFileIClose(iFile);
      else break;}

 return;
 }
/*-----------------------------------------------------------FrFileIDumpEvt--*/
void FrFileIDumpEvt(FrFile *iFile,
                    FILE *fp,
                    int debugLvl) 
/*---------------------------------------------------------------------------*/
/* This function dump a summary of the current file                          */
/*---------------------------------------------------------------------------*/
{FrVect *event, *info;
 int nEvents, i, type, iMax, iEvent;
 double mean, min, max;
 float *amplitude;

 if(iFile == NULL) return;
 
 for(type = 0; type < 2; type++)
   {if(type == 0) event = FrFileIGetEventNames(iFile);
    else          event = FrFileIGetSimEventNames(iFile);
    if(event == NULL)
         {nEvents = 0;}
    else {nEvents = event->nData;}
    if(type == 1)  fprintf(fp," Simulated");
    fprintf(fp," Event   : %d Types of event in the file\n", nEvents);

    for(iEvent=0; iEvent<nEvents; iEvent++)
      {if(type == 0)  info = FrFileIGetEventInfo(iFile, event->dataQ[iEvent], 
                             0., 1.e12, 0., 1.e50);
      else  info = FrFileIGetSimEventInfo(iFile, event->dataQ[iEvent], 
                             0., 1.e12, 0., 1.e50);
       if(info == NULL) continue;
       amplitude = info->next->dataF;
       mean = amplitude[0];
       min  = amplitude[0];
       max  = amplitude[0];
       iMax = 0;
       for(i=1; i<info->nData; i++)
         {mean += amplitude[i];
          if(amplitude[i] < min) min = amplitude[i];
          if(amplitude[i] < max) continue;
          iMax = i;
	  max = amplitude[i];}

       fprintf(fp," %8"FRLLD" events of type:%s \tAmplitude min=%10.3e"
	       " mean=%12g max=%12g at t=%.3fs.\n",info->nData, 
        event->dataQ[iEvent],min, mean/info->nData, max, info->dataD[iMax]);
       FrVectFree(info);}

    FrVectFree(event);}

 return;}

/*----------------------------------------------------------- FrFileIDumpFr--*/
void FrFileIDumpFr(FrFile *iFile,
                   FILE *fp,
                   int debugLvl,
                   double tStart,
                   double length) 
/*---------------------------------------------------------------------------*/
/* This function dump a summary of the current file                          */
/*---------------------------------------------------------------------------*/
{FrVect *gtime;
 double end, delta, *gtimeD, *dtD;
 int nFrames, i;

 if(iFile == NULL) return;
    
 gtime = FrFileIGetFrameInfo(iFile, tStart, length);
 if(gtime == NULL)
      {nFrames = 0;}
 else {nFrames = gtime->nData;}
 fprintf(fp,"File(s) summary:\n %d Frames in the requested time range "
        "(%.0f to %.0f (GPS))\n", nFrames, tStart, tStart+length);
 if(nFrames == 0) return;

          /*-------------------------- debugLvl == 2 Dump a TOC summary, ----*/

 gtimeD = gtime->dataD;
 dtD    = gtime->next->dataD;
 fprintf(fp,"     First frame start at:%.0f (UTC:%s) length=%.2fs.\n",
         gtimeD[0],FrStrUTC(gtime->dataD[0], 0),dtD[0]);

 for(i=1; i<nFrames; i++)
   {delta = gtimeD[i] - (gtimeD[i-1] + dtD[i-1]);
    if(delta < -1.e-3)
      {fprintf(fp,"       Duplicate frame at:%.0f (UTC:%s) "
           "(%.3fs. of overlapp data)\n",
	      gtimeD[i],FrStrUTC(gtimeD[i], 0), -delta);}
    else if(delta > 1.e-3)
      {fprintf(fp,"Missing frame starting at:%.0f (UTC:%s) "
           "(%.3fs. of missing data)\n",
          gtimeD[i-1]+dtD[i-1],FrStrUTC(gtimeD[i-1]+dtD[i-1], 0),delta);}}

 end = gtimeD[nFrames-1]+dtD[nFrames-1];
 fprintf(fp,"        Last frame end at:%.0f (UTC:%s) length=%.2fs.\n", 
             end, FrStrUTC(end, 0),dtD[nFrames-1]);

 FrVectFree(gtime);
 return;
 }
/*---------------------------------------------------------------FrFileIEnd--*/
void FrFileIEnd(FrFile *iFile)
/*---------------------------------------------------------------------------*/
{
 if(iFile == NULL) return;
  
 FrFileIClose(iFile);

 FrFileFree(iFile);
 
 return;}

/*------------------------------------------------------FrFileIGetEventInfo--*/
FrVect *FrFileIGetEventInfo(FrFile *iFile,
                            char *tag,
                            double tStart,
                            double length,
                            double aMin,
                            double aMax)
/*---------------------------------------------------------------------------*/
{FrVect **vect;
 FrFileH *current;
 int nFile;

 if(iFile == NULL) return(NULL);
 
                  /*----count the number of files  and allocate work space--*/

 nFile = 0;
 for(current = iFile->fileH; current != NULL; current = current->next)
   {nFile++;}

 if(nFile == 0) return(NULL);
 if(nFile == 1) 
   {if(iFile->toc == NULL) FrTOCReadFull(iFile);
    if(iFile->toc == NULL) return(NULL);
    return(FrTOCevtGetEventInfo(iFile->toc->event, tag, 
                                tStart, length, aMin, aMax));}

 if((vect = (FrVect **) malloc(nFile*sizeof(void*))) == NULL) return(NULL);

                    /*---------------get the information from all files ----*/

 nFile = 0;
 FrFileIClose(iFile);
 
 for(current = iFile->fileH; current != NULL; current = current->next)

         /*------ we add/subtract a small offset to avoid rounding errors----*/

   {if(current->tFirstEvt >= 0.)
      {if(current->tFirstEvt > tStart+length+1.e-5) continue;
       if(current->tLastEvt  < tStart-1.e-5) continue;}

    iFile->error = FR_OK;  
    iFile->current = current;
    FrFileIOpen(iFile);
    if(iFile->toc == NULL) FrTOCReadFull(iFile);
    if(iFile->toc == NULL) return(NULL);
    
    vect[nFile] = FrTOCevtGetEventInfo(iFile->toc->event, tag, 
                                tStart, length, aMin, aMax);
    if(vect[nFile] != NULL) nFile++;

    FrFileIClose(iFile);}
                             /*------------------------ merge the vectors ---*/

 return(FrVectMergeT(vect, nFile));}

/*-----------------------------------------------------FrFileIGetEventNames--*/
FrVect *FrFileIGetEventNames(FrFile *iFile)
/*---------------------------------------------------------------------------*/
{FrVect *vect;
 FrFileH *current;
 FrTOCevt *tocEvt;

 if(iFile == NULL) return(NULL);

 vect = FrVectNew1D("Event_name",FR_VECT_STRING,0, 1.,"a.u.","name");
 if(vect == NULL) return(NULL);

                    /*---------------get the information from all files ----*/
 
 for(current = iFile->fileH; current != NULL; current = current->next)
   {if(iFile->current != current) 
      {FrFileIClose(iFile);
       iFile->error = FR_OK;  
       iFile->current = current;
       FrFileIOpen(iFile);}

    if(iFile->toc == NULL) FrTOCReadFull(iFile);
    if(iFile->toc == NULL) return(NULL);
 
    for(tocEvt = iFile->toc->event; tocEvt != NULL; tocEvt = tocEvt->next) 
       {if(current != iFile->fileH)
	 {if(FrVectFindQ(vect,tocEvt->name) >= 0) continue;}
        FrVectFillQ(vect, tocEvt->name);}}

 return(vect);}

/*-------------------------------------------------------FrFileIGetAdcNames--*/
FrVect *FrFileIGetAdcNames(FrFile *iFile)
/*---------------------------------------------------------------------------*/
{FrVect *vect;
 FrFileH *current;
 FrTOCts *adc;

 if(iFile == NULL) return(NULL);

 vect = FrVectNew1D("ADC_name",FR_VECT_STRING,0, 1.,"a.u.","name");
 if(vect == NULL) return(NULL);

 for(current = iFile->fileH; current != NULL; current = current->next)
   {if(iFile->current != current) 
      {FrFileIClose(iFile);
       iFile->error = FR_OK;  
       iFile->current = current;
       FrFileIOpen(iFile);}

    if(iFile->toc == NULL) FrTOCReadFull(iFile);
    if(iFile->toc == NULL) return(NULL);
 
    for(adc = iFile->toc->adc; adc != NULL; adc = adc->next) 
       {if(current != iFile->fileH)
	 {if(FrVectFindQ(vect,adc->name) >= 0) continue;}
        FrVectFillQ(vect, adc->name);}}

 return(vect);}
/*--------------------------------------------------FrFileIGetDetectorNames--*/
FrVect *FrFileIGetDetectorNames(FrFile *iFile)
/*---------------------------------------------------------------------------*/
{FrVect *vect;
 FrFileH *current;
 FrTOCdet *det;

 if(iFile == NULL) return(NULL);

 vect = FrVectNew1D("Detector_name",FR_VECT_STRING,0, 1.,"a.u.","name");
 if(vect == NULL) return(NULL);

 for(current = iFile->fileH; current != NULL; current = current->next)
   {if(iFile->current != current) 
      {FrFileIClose(iFile);
       iFile->error = FR_OK;  
       iFile->current = current;
       FrFileIOpen(iFile);}

    if(iFile->toc == NULL) FrTOCReadFull(iFile);
    if(iFile->toc == NULL) return(NULL);
 
    for(det = iFile->toc->detector; det != NULL; det = det->next) 
       {if(current != iFile->fileH)
	  {if(FrVectFindQ(vect,det->name) >= 0) continue;}
        FrVectFillQ(vect, det->name);}}

 return(vect);}

/*------------------------------------------------------FrFileIGetFrameInfo--*/
FrVect *FrFileIGetFrameInfo(FrFile *iFile,
                            double tStart,
                            double length)
/*---------------------------------------------------------------------------*/
{FrVect **vect;
 FrFileH *current;
 int nFile;

 if(iFile == NULL) return(NULL);
 
                  /*----count the number of files  and allocate work space--*/

 nFile = 0;
 for(current = iFile->fileH; current != NULL; current = current->next)
   {nFile++;}

 if(nFile == 0) return(NULL);
 if(nFile == 1) return(FrFileIGetFrameInfo1(iFile, tStart, length));

 if((vect = (FrVect **) malloc(nFile*sizeof(void*))) == NULL) return(NULL);

                    /*---------------get the information from all files ----*/

 nFile = 0;
 FrFileIClose(iFile);
 
 for(current = iFile->fileH; current != NULL; current = current->next)

         /*------ we add/subtract a small offset to avoid rounding errors----*/

   {if(current->tStart > 0)
      {if(current->tStart          > tStart+length+1.e-5) continue;
       if(current->tStart+current->length < tStart-1.e-5) continue;}

    iFile->error = FR_OK;  
    iFile->current = current;
    FrFileIOpen(iFile);

    vect[nFile] = FrFileIGetFrameInfo1(iFile, tStart, length);
    if(vect[nFile] != NULL) nFile++;

    FrFileIClose(iFile);}
                             /*------------------------ merge the vectors ---*/

 return(FrVectMergeT(vect, nFile));}

/*-----------------------------------------------------FrFileIGetFrameInfo1--*/
FrVect *FrFileIGetFrameInfo1(FrFile *iFile,
                             double tStart,
                             double length)
/*---------------------------------------------------------------------------*/
{FrVect *gtime, *dt, *qc;
 long i, l, first, last, nFrame;
 FrTOC *toc;
 double frTime;

 if(iFile->toc == NULL)  FrTOCReadFull(iFile);
 if((toc = iFile->toc) == NULL) return(NULL);
 if(toc->nFrame == 0) return(NULL);

 l = toc->nFrame-1;
 if((toc->GTimeS[l] + 1.e-9 * toc->GTimeN[l]) < tStart-1.e-6) return(NULL);
 if((toc->GTimeS[0] + 1.e-9 * toc->GTimeN[0]) > tStart+length
                                                      +1.e-6) return(NULL);
 last  = 0;
 first = 0;
 for(i=0; i< toc->nFrame; i++)
   {frTime = toc->GTimeS[i] + 1.e-9 * toc->GTimeN[i];
   if(frTime < tStart+1.e-5) first = i;
   if(frTime > tStart+length-1.e-5) break;
   last = i;}

 nFrame = last - first + 1;
 gtime = FrVectNewTS("Frame_time",       toc->dt[first],nFrame, -64);
 dt    = FrVectNewTS("Frame_dt",         toc->dt[first],nFrame, -64);
 qc    = FrVectNewTS("Frame_dataQuality",toc->dt[first],nFrame, 32);
 if(gtime == NULL || dt == NULL || qc == NULL) return(NULL);

 for(i=first; i<last+1; i++)
   {gtime->dataD[i-first] = toc->GTimeS[i] + 1.e-9 * toc->GTimeN[i];
    dt   ->dataD[i-first] = toc->dt[i];
    qc   ->dataI[i-first] = toc->dataQuality[i];}

 gtime->next = dt;
 dt   ->next = qc;

 return(gtime);}

/*------------------------------------------------------FrFileIGetProcNames--*/
FrVect *FrFileIGetProcNames(FrFile *iFile)
/*---------------------------------------------------------------------------*/
{FrVect *vect;
 FrFileH *current;
 FrTOCts *proc;

 if(iFile == NULL) return(NULL);

 vect = FrVectNew1D("Proc_name",FR_VECT_STRING,0, 1.,"a.u.","name");
 if(vect == NULL) return(NULL);

 for(current = iFile->fileH; current != NULL; current = current->next)
   {if(iFile->current != current) 
      {FrFileIClose(iFile);
       iFile->error = FR_OK;  
       iFile->current = current;
       FrFileIOpen(iFile);}

    if(iFile->toc == NULL) FrTOCReadFull(iFile);
    if(iFile->toc == NULL) return(NULL);
 
    for(proc = iFile->toc->proc; proc != NULL; proc = proc->next) 
       {if(current != iFile->fileH)
	 {if(FrVectFindQ(vect,proc->name) >= 0) continue;}
        FrVectFillQ(vect, proc->name);}}

 return(vect);}

/*-------------------------------------------------------FrFileIGetSerNames--*/
FrVect *FrFileIGetSerNames(FrFile *iFile)
/*---------------------------------------------------------------------------*/
{FrVect *vect;
 FrFileH *current;
 FrTOCts *ser;

 if(iFile == NULL) return(NULL);

 vect = FrVectNew1D("Ser_name",FR_VECT_STRING,0, 1.,"a.u.","name");
 if(vect == NULL) return(NULL);

 for(current = iFile->fileH; current != NULL; current = current->next)
   {if(iFile->current != current) 
      {FrFileIClose(iFile);
       iFile->error = FR_OK;  
       iFile->current = current;
       FrFileIOpen(iFile);}

    if(iFile->toc == NULL) FrTOCReadFull(iFile);
    if(iFile->toc == NULL) return(NULL);
 
    for(ser = iFile->toc->ser; ser != NULL; ser = ser->next) 
       {if(FrVectFindQ(vect,ser->name) >= 0) continue;
        FrVectFillQ(vect, ser->name);}}

 return(vect);}

/*---------------------------------------------------FrFileIGetSimEventInfo--*/
FrVect *FrFileIGetSimEventInfo(FrFile *iFile,
                            char *tag,
                            double tStart,
                            double length,
                            double aMin,
                            double aMax)
/*---------------------------------------------------------------------------*/
{FrVect **vect;
 FrFileH *current;
 int nFile;

 if(iFile == NULL) return(NULL);
 
                  /*----count the number of files  and allocate work space--*/

 nFile = 0;
 for(current = iFile->fileH; current != NULL; current = current->next)
   {nFile++;}

 if(nFile == 0) return(NULL);
 if(nFile == 1) 
   {if(iFile->toc == NULL) FrTOCReadFull(iFile);
    if(iFile->toc == NULL) return(NULL);
    return(FrTOCevtGetEventInfo(iFile->toc->simEvt, tag, 
                                tStart, length, aMin, aMax));}

 if((vect = (FrVect **) malloc(nFile*sizeof(void*))) == NULL) return(NULL);

                    /*---------------get the information from all files ----*/

 nFile = 0;
 FrFileIClose(iFile);
 
 for(current = iFile->fileH; current != NULL; current = current->next)

         /*------ we add/subtract a small offset to avoid rounding errors----*/

   {if(current->tFirstEvt >= 0.)
      {if(current->tFirstEvt > tStart+length+1.e-5) continue;
       if(current->tLastEvt  < tStart-1.e-5) continue;}

    iFile->error = FR_OK;  
    iFile->current = current;
    FrFileIOpen(iFile);
    if(iFile->toc == NULL) FrTOCReadFull(iFile);
    if(iFile->toc == NULL) return(NULL);
    
    vect[nFile] = FrTOCevtGetEventInfo(iFile->toc->simEvt, tag, 
                                tStart, length, aMin, aMax);
    if(vect[nFile] != NULL) nFile++;

    FrFileIClose(iFile);}
                             /*------------------------ merge the vectors ---*/

 return(FrVectMergeT(vect, nFile));}
/*--------------------------------------------------FrFileIGetSimEventNames--*/
FrVect *FrFileIGetSimEventNames(FrFile *iFile)
/*---------------------------------------------------------------------------*/
{FrVect *vect;
 FrFileH *current;
 FrTOCevt *tocEvt;

 if(iFile == NULL) return(NULL);

 vect = FrVectNew1D("SimEvent_name",FR_VECT_STRING,0, 1.,"a.u.","name");
 if(vect == NULL) return(NULL);

                    /*---------------get the information from all files ----*/
 
 for(current = iFile->fileH; current != NULL; current = current->next)
   {if(iFile->current != current) 
      {FrFileIClose(iFile);
       iFile->error = FR_OK;  
       iFile->current = current;
       FrFileIOpen(iFile);}

    if(iFile->toc == NULL) FrTOCReadFull(iFile);
    if(iFile->toc == NULL) return(NULL);
 
    for(tocEvt = iFile->toc->simEvt; tocEvt != NULL; tocEvt = tocEvt->next) 
       {if(current != iFile->fileH)
	 {if(FrVectFindQ(vect,tocEvt->name) >= 0) continue;}
        FrVectFillQ(vect, tocEvt->name);}}

 return(vect);}
/*-------------------------------------------------------FrFileIGetSimNames--*/
FrVect *FrFileIGetSimNames(FrFile *iFile)
/*---------------------------------------------------------------------------*/
{FrVect *vect;
 FrFileH *current;
 FrTOCts *sim;

 if(iFile == NULL) return(NULL);

 vect = FrVectNew1D("Sim_name",FR_VECT_STRING,0, 1.,"a.u.","name");
 if(vect == NULL) return(NULL);

 for(current = iFile->fileH; current != NULL; current = current->next)
   {if(iFile->current != current) 
      {FrFileIClose(iFile);
       iFile->error = FR_OK;  
       iFile->current = current;
       FrFileIOpen(iFile);}

    if(iFile->toc == NULL) FrTOCReadFull(iFile);
    if(iFile->toc == NULL) return(NULL);
 
    for(sim = iFile->toc->sim; sim != NULL; sim = sim->next) 
       {if(FrVectFindQ(vect,sim->name) >= 0) continue;
        FrVectFillQ(vect, sim->name);}}

 return(vect);}
/*------------------------------------------------------FrFileIGetStatNames--*/
FrVect *FrFileIGetStatNames(FrFile *iFile)
/*---------------------------------------------------------------------------*/
{FrVect *vect;
 FrFileH *current;
 FrTOCstat *stat;

 if(iFile == NULL) return(NULL);

 vect = FrVectNew1D("ADC_name",FR_VECT_STRING,0, 1.,"a.u.","name");
 if(vect == NULL) return(NULL);

 for(current = iFile->fileH; current != NULL; current = current->next)
   {if(iFile->current != current) 
      {FrFileIClose(iFile);
       iFile->error = FR_OK;  
       iFile->current = current;
       FrFileIOpen(iFile);}

    if(iFile->toc == NULL) FrTOCReadFull(iFile);
    if(iFile->toc == NULL) return(NULL);
 
    for(stat = iFile->toc->stat; stat != NULL; stat = stat->next) 
       {if(current != iFile->fileH)
	 {if(FrVectFindQ(vect,stat->name) >= 0) continue;}
        FrVectFillQ(vect, stat->name);}}

 return(vect);}
/*--------------------------------------------------------------FrFileIGetV--*/
FrVect *FrFileIGetV(FrFile *iFile,
                    char *name,
                    double tStart,
                    double len)
/*---------------------------------------------------------------------------*/
{FrVect *vect;

 vect = FrFileIGetVect(iFile, name, tStart, len);

 return(vect);}
/*-----------------------------------------------------------FrFileIGetVect--*/
FrVect *FrFileIGetVect(FrFile *iFile,
                       char *name,
                       double tStart,
                       double len)
/*---------------------------------------------------------------------------*/
{FrVect *vect;

 vect = FrFileIGetVType(iFile, name, tStart, len, 1, FRTSADC);
 if(vect != NULL) return(vect);

 vect = FrFileIGetVType(iFile, name, tStart, len, 1, FRTSSIM);
 if(vect != NULL) return(vect);

 vect = FrFileIGetVType(iFile, name, tStart, len, 1, FRTSPROC);
 return(vect);}
/*----------------------------------------------------------FrFileIGetVectD--*/
FrVect *FrFileIGetVectD(FrFile *iFile,
                        char *name,
                        double tStart,
                        double len)
/*---------------------------------------------------------------------------*/
{FrVect *vect, *vectD, *next;

 vect = FrFileIGetVect(iFile, name, tStart, len);
 if(vect == NULL) return NULL;
 if(vect->type == FR_VECT_8R) return(vect);

 next = vect->next;
 vect->next = NULL;
 vectD = FrVectCopyToD(vect, 1., NULL);
 FrVectFree(vect);
 vectD->next = next;

 return(vectD);}
/*---------------------------------------------------------FrFileIGetVectDN--*/
FrVect *FrFileIGetVectDN(FrFile *iFile,
                         char *name,
                         double tStart,
                         double len)
/*---------------------------------------------------------------------------*/
{FrVect *vect;
 int i;

 vect = FrFileIGetVectD(iFile, name, tStart, len);
 if(vect == NULL) return NULL;
                                    /*------------ normalized ADC content ---*/
 if(iFile->lastUnits != NULL)
   {FrVectSetUnitY(vect, iFile->lastUnits);
    for(i=0; i<vect->nData; i++)
      {vect->dataD[i] = iFile->lastSlope * vect->dataD[i] + iFile->lastBias;}}

 return(vect);}
/*----------------------------------------------------------FrFileIGetVectF--*/
FrVect *FrFileIGetVectF(FrFile *iFile,
                        char *name,
                        double tStart,
                        double len)
/*---------------------------------------------------------------------------*/
{FrVect *vect, *vectF, *next;

 vect = FrFileIGetVect(iFile, name, tStart, len);
 if(vect == NULL) return NULL;
 if(vect->type == FR_VECT_4R) return(vect);
 
 next = vect->next;
 vect->next = NULL;
 vectF = FrVectCopyToF(vect, 1., NULL);
 FrVectFree(vect);
 vectF->next = next;

 return(vectF);}

/*---------------------------------------------------------FrFileIGetVectFN--*/
FrVect *FrFileIGetVectFN(FrFile *iFile,
                         char *name,
                         double tStart,
                         double len)
/*---------------------------------------------------------------------------*/
{FrVect *vect;
 int i;

 vect = FrFileIGetVectF(iFile, name, tStart, len);
 if(vect == NULL) return NULL;
                                    /*------------ normalized ADC content ---*/
 if(iFile->lastUnits != NULL)
   {FrVectSetUnitY(vect, iFile->lastUnits);
    for(i=0; i<vect->nData; i++)
      {vect->dataF[i] = iFile->lastSlope * vect->dataF[i] + iFile->lastBias;}}

 return(vect);}

/*-----------------------------------------------------------FrFileIGetVAdc--*/
FrVect *FrFileIGetVAdc(FrFile *iFile,
                        char *name,
                        double tStart,
                        double len,
                        int    group)
/*---------------------------------------------------------------------------*/
{
  return(FrFileIGetVType(iFile, name, tStart, len, group, FRTSADC));
}
/*----------------------------------------------------------FrFileIGetVProc--*/
FrVect *FrFileIGetVProc(FrFile *iFile,
                        char *name,
                        double tStart,
                        double len,
                        int    group)
/*---------------------------------------------------------------------------*/
{
  return(FrFileIGetVType(iFile, name, tStart, len, group, FRTSPROC));
}
/*-----------------------------------------------------------FrFileIGetVSim--*/
FrVect *FrFileIGetVSim(FrFile *iFile,
                        char *name,
                        double tStart,
                        double len,
                        int    group)
/*---------------------------------------------------------------------------*/
{
  return(FrFileIGetVType(iFile, name, tStart, len, group, FRTSSIM));
}
/*----------------------------------------------------------FrFileIGetVType--*/
FrVect *FrFileIGetVType(FrFile *iFile,
                        char *name,
                        double tStartIn,
                        double len,
                        int    group,
                        FRTSTYPE type)
/*---------------------------------------------------------------------------*/
{FrAdcData *adc=NULL;
 FrSimData *sim=NULL;
 FrProcData *proc=NULL;
 FrTOCts *ts, *firstTs=NULL;
 FrVect *vect, *vectRoot, *fullVect, *vLast;
 FrFileH *firstFileH;
 int i;
 double tEnd, fStart, fEnd, tStart;
 char msg[350], *fName;

 if(iFile == NULL) return(NULL);

 i = FrTOCFrameFindNT(iFile, tStartIn); /*-set the pointer to the right file-*/
 if(i< 0) return(NULL);

 firstFileH = iFile->current;

 iFile->relocation = FR_NO;            /*--- to avoid relloaction problems---*/

                  /*-------- if tStart=0, return the first frames ----------*/
 if(tStartIn < 1.)
    {tStartIn = iFile->toc->GTimeS[0] + 
                iFile->toc->GTimeN[0] * 1.e-9 + 1.e-6;}

 tEnd = tStartIn + len - 1.e-6;/*sub a small number to avoid round off errors*/
 tStart = tStartIn   + 1.e-6; /* sub a small number to avoid round off errors*/

 vectRoot = NULL;

      /*--- First step, we extract all vectors contained in the time window--*/

 do
   {fName = iFile->current->fileName;
    if(iFile->toc == NULL) FrTOCReadFull(iFile);       /*---- get the TOC----*/
    if(iFile->toc == NULL) 
      {sprintf(msg,"Could not read TOC for %s", fName);
       FrError(3,"FrFileIGetVType",msg);
       return(NULL);}
 
                   /*---------Find name in the channel list for this file----*/

    if     (type == FRTSADC)  firstTs = iFile->toc->adc;
    else if(type == FRTSSIM)  firstTs = iFile->toc->sim;
    else if(type == FRTSPROC) firstTs = iFile->toc->proc;
      
    for(ts = firstTs; ts != NULL; ts = ts->next)
      {if(FrStrcmpAndPrefix(name, ts->name) == 0) break;}

    if(ts == NULL) continue;

                   /*---------------- read vectors contained in one file ----*/

    for(i=0; i<iFile->toc->nFrame; i++)
      {fStart = iFile->toc->GTimeS[i] + 
	        iFile->toc->GTimeN[i] * 1.e-9;
       if(fStart > tEnd)   continue;
       fEnd   = fStart+iFile->toc->dt[i];
       if(fEnd   < tStart) continue;

       if(ts->position[i] == 0) continue;
       if(FrTOCSetPos(iFile, ts->position[i]) != 0) 
          {sprintf(msg,"seek error for %s at GPS=%.1f",fName, fStart);
           FrError(3,"FrFileIGetVType",msg);
	   return(NULL);}

       if     (type == FRTSADC)
         {if((adc = FrAdcDataRead (iFile)) == NULL) 
            {sprintf(msg,"ADC read error for %s at GPS=%.1f",fName, fStart);
             FrError(3,"FrFileIGetVType",msg);
             return(NULL);}}
       else if(type == FRTSSIM) 
         {if((sim = FrSimDataRead (iFile)) == NULL) 
            {sprintf(msg,"Sim read error for %s at GPS=%.1f",fName, fStart);
             FrError(3,"FrFileIGetVType",msg);
	     return(NULL);}}
       else if(type == FRTSPROC) 
         {if((proc = FrProcDataRead(iFile)) == NULL)
            {sprintf(msg,"Proc read error for %s at GPS=%.1f",fName, fStart);
             FrError(3,"FrFileIGetVType",msg);
             return(NULL);}}
         
        /*-Remark: The next check on GTimeS is just to fix a VIRGO DAQ bug -*/

       if(type == FRTSADC && fStart > 700000000) {fStart += adc->timeOffset;}
       else if(type == FRTSSIM)                  {fStart += sim->timeOffset;}
       else if(type == FRTSPROC)                 {fStart += proc->timeOffset;}
       vect = FrVectReadNext(iFile, fStart, name);
       
       if(vect == NULL) 
          {FrVectFree(vectRoot);
           sprintf(msg,"Vector read error for %s at GPS=%.1f",fName, fStart);
           FrError(3,"FrFileIGetVType",msg);
           return(NULL);}
	   
       if(vect->compress != 0) FrVectExpand(vect);

       if((group != 1) && (group != 0)) FrVectDecimate(vect, group, vect);

       /*--------------record the last normalization factors for ADC's ---*/

       if(iFile->lastUnits != NULL) free(iFile->lastUnits);
       iFile->lastUnits = NULL;
       if(type == FRTSADC)
	   {iFile->lastBias  = adc->bias;
	    iFile->lastSlope = adc->slope;
	    FrStrCpy(&iFile->lastUnits, adc->units);}

       /*------------ build a linked list, including the auxilary vectors---*/
       for(vLast = vect; vLast->next != NULL; vLast = vect->next) {;}
       vLast->next = vectRoot;  
       vectRoot = vect;

       if     (type == FRTSADC)  FrAdcDataFree(adc);
       else if(type == FRTSSIM)  FrSimDataFree(sim);
       else if(type == FRTSPROC) FrProcDataFree(proc);}}

  while (FrFileINext(iFile, tStart, len, firstFileH, FR_NO) == 0);  

                      /*--------------------End of the extract vector loop--*/

 if(vectRoot == NULL) return(NULL);

       /*------------- in case of read error (checksum error) return NULL --*/

 if(iFile->error != FR_OK) {
   FrVectFree(vectRoot);
   return(NULL);}
                     /*-------------------- build the output vector --------*/

 fullVect = FrVectConcat(vectRoot, tStartIn, len);

 return(fullVect);}

/*----------------------------------------------------FrFileIGoToNextRecord--*/
int FrFileIGoToNextRecord(FrFile *iFile)
/*---------------------------------------------------------------------------*/
/* this function set the file pointer position to the next real record       */
/*---------------------------------------------------------------------------*/
{
  FRLONG toBeSkiped;

  if(iFile == NULL) return(-1);

  while(iFile->error == FR_OK) {
    FrReadStructHeader(iFile);
    if(iFile->error != FR_OK) return(3);

    if(FrDebugLvl > 2) fprintf(FrFOut, " length:%10"FRLLD" type=%4d",
			       iFile->length,iFile->type);
                             
    if(iFile->type > 2) return(iFile->type);

    if(iFile->fmtVersion > 5) {toBeSkiped = iFile->length - 10;}
    else                      {toBeSkiped = iFile->length - 6;}
    FrFileIOSetFromCur(iFile, toBeSkiped);}

 return(-2);}

/*---------------------------------------------------------------FrFileINew--*/
FrFile *FrFileINew(char *fileName) 
/*---------------------------------------------------------------------------*/
{FrFile *file;
 char *buf;
 long nBytes;

 /*---- is it a file stored in memory? The name should give the addresse---*/
 if(fileName == NULL) return(NULL);
 if(fileName[0] == '0' &&
     fileName[1] == 'x') {
    if(sscanf(fileName,"%p-%lx.fim",&buf, &nBytes) == 2) {
      if(buf == NULL) return(NULL);
      if(nBytes <= 0) return(NULL);
  
      file = FrFileNew(NULL, -1, buf, nBytes);
      if(file == NULL) return (NULL);

      file->chkSumFiFlag = FR_NO; /*don't compute checksum for memory buffer*/
      file->chkSumFrFlag = FR_NO; 

      FrFileIOpen(file);
      if(file->error != FR_OK) return(NULL);
      else return(file);}}

 /*--------------------------------------regular .gwf or .ffl file opening---*/
 file = FrFileNew(fileName, 0, NULL, 0);
 if(file == NULL) return (NULL);
                                        /*----- open single file (not ffl)---*/
 if(file->fileH == NULL) return(NULL);
 if(file->fileH->next == NULL)
   {FrFileIOpen(file);
    if(file->error != FR_OK) 
       {FrFileIEnd(file);
        return(NULL);}}

 file->chkSumFiFlag = FR_NO;    /*-- no checksum to speedup standard read ---*/
 file->chkSumFrFlag = FR_NO;

 return(file);
}
/*-------------------------------------------------------------FrFileINewFd--*/
FrFile *FrFileINewFd(FrIO *frfd)
/*---------------------------------------------------------------------------*/
{FrFile *file;
 
 file = FrFileNew("open_with_fd", 0, NULL, 0);
 if(file == NULL) return (NULL);

 file->frfd = frfd;
                                        /*----- open single file (not ffl)---*/
 if(file->fileH == NULL) return(NULL);
 if(file->fileH->next == NULL)
   {FrFileIOpen(file);
    if(file->error != FR_OK) 
       {FrFileIEnd(file);
        return(NULL);}}

 file->chkSumFiFlag = FR_NO;    /*-- no checksum to speedup standard read ---*/
 file->chkSumFrFlag = FR_NO;

 return(file);
}
/*------------------------------------------------------------- FrFileINext--*/
int FrFileINext(FrFile *iFile,
                double tStart,
                double len,
                FrFileH *firstFileH,
                FRBOOL event) 
/*---------------------------------------------------------------------------*/
/* This function open the next file in the list if available. Returns 0 if OK*/
/* IF tStart is not equal to 0, it skeep all file which do not overlap       */
/* tSTart and tStart+len.                                                    */
/* If event == YES, the match is search for the event time boundaries.       */
/*---------------------------------------------------------------------------*/
{
 if(iFile == NULL)              return(-1);
 if(iFile->fileH->next == NULL) return(-2);     /*--only one single file ---*/
 if(iFile->current == NULL)     return(-3);

 FrFileIClose(iFile);               /*-------- Close the previous file ------*/

           /*-----------Search for the next file in the list which 
                                              matchs the time conditions ----*/
 for(;;)
   {iFile->current = iFile->current->next;
    if(tStart < 1) break;                   /*----no timing requested -------*/

    if(iFile->current == NULL) iFile->current = iFile->fileH;
    if(iFile->current == firstFileH) return(-1);

                     /*--------------- search for frame overlapping time ----*/
    if(event == FR_NO)
      {if(iFile->current->tStart == -1.) break;/*-no timing info available --*/
    
      if(tStart < iFile->current->tStart)
          {if(tStart+len > iFile->current->tStart) break;}
      else if(tStart < iFile->current->tStart+iFile->current->length) break;}

                     /*--------------- search for event overlapping time ----*/
    else
      {if(iFile->current->tFirstEvt == -1.) break;
    
       if(tStart < iFile->current->tFirstEvt)
	   {if(tStart+len > iFile->current->tFirstEvt) break;}
       else if(tStart     < iFile->current->tLastEvt)  break;}}

           /*--------------------------- Open the next file ----------------*/

 if(iFile->current == NULL) return(-1);

 iFile->error = FR_OK;   /*--- reset the error flag for the next file ------*/
 FrFileIOpen(iFile);

 if(iFile->error != FR_OK) return(-1); 

 return(0);
 }
/*------------------------------------------------------------- FrFileIOpen--*/
void FrFileIOpen(FrFile *file)
/*---------------------------------------------------------------------------*/
{int i, hLen = 40;
 unsigned char *h, l[40];
 FRBOOL chkSumFlag;
 FRLONG length;
 short type;
 
           /*------- reset error flags (the previous file may be corrupted)--*/

 file->error = FR_OK;

           /*----------------------------------------- read the first byte --*/
      
 if(file->frfd != NULL)
      {if(FrDebugLvl > 0) fprintf(FrFOut,
           "FrFileIOpen: open file using existing file descriptor\n");}
 else if(file->inMemory == FR_NO)
      {if(file->current == NULL)  
         {FrError(3,"FrFileIOpen"," No current file");
          file->error = FR_ERROR_OPEN_ERROR;
          return;}

       if(FrDebugLvl > 0) fprintf(FrFOut,
           "FrFileIOpen: open file:%s\n",file->current->fileName); 

       file->frfd = FrIOOpenR(file->current->fileName);
       if(file->frfd == NULL)  
           {sprintf(FrErrMsg,"Open file error (%s) for file %s", 
		    strerror(errno), file->current->fileName);
            FrError(3,"FrFileIOpen",FrErrMsg);
            file->error = FR_ERROR_OPEN_ERROR;
            return;}}
  
 else
     {if(FrDebugLvl > 0) fprintf(FrFOut,
            "FrFileIOpen: input data from string");} 
 
      /*---------- read the first bytes to determine the format.
                chkSum is turn off since we dont know yet the file version --*/
      
 chkSumFlag = file->chkSumFiFlag;
 file->chkSumFiFlag = FR_NO;
 file->chkSumFi = 0;
 
 i = FrRead(file, (char *) file->header, 1);
 if(i != 1)  
     {FrError(3,"FrFileIOpen"," Cannot read first byte");
      file->error = FR_ERROR_READ_ERROR;
      return;}
 
 if(file->header[0] != 'I')	 
      {if(FrDebugLvl > 0) 
           fprintf(FrFOut,"    unknown format: %d\n",file->header[0]);
       FrError(3,"FrFileIOpen","unknown format"); 
       file->error = FR_ERROR_OPEN_ERROR;
       return;}
 
      /*---------------------------- read header ----------------------------*/

 if(FrRead(file, (char *) file->header+1, hLen-1) != hLen-1)  
      {FrError(3,"FrFileIOpen","read failled"); 
       file->error = FR_ERROR_READ_ERROR;
       return;}

 file->fmtVersion = file->header[5];
  
       /*------------------- Check byte swaping and word lenght-------------*/

 FrFileHeader(l);
 l[9] = FrSlong;   /*-- Remarks: we may be on a 4 bytes long computer-------*/

 h = file->header;
 h[0] = file->header[0];
 if(FrDebugLvl > 0)
       {fprintf(FrFOut," Header(local): Originator:%.5s(%.5s)",h,l);
        fprintf(FrFOut," Format version:%d(%d)",h[5],l[5]);
        fprintf(FrFOut," Library version:%d(%d)\n",h[6],l[6]);
        fprintf(FrFOut,
          "  Sizeof: s:%d(%d) i:%d(%d) l:%d(%d) f:%d(%d) d:%d(%d)\n",
              h[7],l[7],h[8],l[8],h[9],l[9],h[10],l[10],h[11],l[11]);
        fprintf(FrFOut,
          "  SwapS:%3x%2x(%2x%2x)  \n",
              h[12],h[13],l[12],l[13]);
        fprintf(FrFOut,
          "  SwapI:%3x%2x%2x%2x(%2x%2x%2x%2x)\n",
              h[14],h[15],h[16],h[17],
              l[14],l[15],l[16],l[17]);
        fprintf(FrFOut,
          "  SwapL:%3x%2x%2x%2x%2x%2x%2x%2x(%2x%2x%2x%2x%2x%2x%2x%2x)\n",
              h[18],h[19],h[20],h[21],h[22],h[23],h[24],h[25],
              l[18],l[19],l[20],l[21],l[22],l[23],l[24],l[25]);
        fprintf(FrFOut,"  FPi:%3x%2x%2x%2x(%2x%2x%2x%2x)\n",
              h[26],h[27],h[28],h[29],
              l[26],l[27],l[28],l[29]);
        fprintf(FrFOut,"  DPi:%3x%2x%2x%2x%2x%2x%2x%2x"
                            "(%2x%2x%2x%2x%2x%2x%2x%2x)\n",
              h[30],h[31],h[32],h[33],h[34],h[35],h[36],h[37],
              l[30],l[31],l[32],l[33],l[34],l[35],l[36],l[37]);}

 file->fmType = FRNATIVE;
 file->fmLong = FRNATIVE;
 if(memcmp(&h[7],&l[7],19) == 0)               
     {if(FrDebugLvl > 0) fprintf(FrFOut," No format conversion is needed\n");}
 else{if(FrDebugLvl > 0) fprintf(FrFOut," Format conversion is needed\n");
      if(h[12] == l[13])
         {file->fmType = FRSWAP;
          file->fmLong = FRSWAP;
          if(FrDebugLvl > 0) fprintf(FrFOut," We will swap bytes\n");}
      if(FrSlong == 8 && h[9] == 4) 
         {if(file->fmLong == FRSWAP) 
              {file->fmLong = FREXPAND_AND_SWAP;}
          else{file->fmLong = FREXPAND;}
          if(FrDebugLvl > 0) fprintf(FrFOut,
                " We will convert long from 4 bytes word to 8 bytes\n");}
      if(FrSlong == 4 && h[9] == 8) 
         {if(file->fmLong == FRSWAP) 
              {file->fmLong = FRCONTRACT_AND_SWAP;}
          else{file->fmLong = FRCONTRACT;}
          if(FrDebugLvl > 0) fprintf(FrFOut,
                " We will convert long from 8 bytes word to 4 bytes\n");}
      if(file->fmLong == FRNATIVE) 
        {if(FrDebugLvl > 0) fprintf(FrFOut," Strange format\n");
         FrError(3,"FrFileIOpen","Strange format");}
     }

       /*---- the following code is to handle the confused version 5. 
           Frame produced with Fr/v5r00 are read as version 6.
           In the FrameLib code version 5 is LIGO intermediate version ------*/

 if(file->fmtVersion == 5)
   {FrReadLong(file, &length);
    FrReadShort(file, &type);
    if(FrDebugLvl > 0) fprintf(FrFOut," Try to read FrSH type:%d\n",type);
    if(type == 1) 
       {file->fmtVersion = 6;
        if(FrDebugLvl > 0) fprintf(FrFOut,
	      	" Warning: this seems to be version 6 frames\n");}

    FrFileIOSet(file, 40);}
 
        /*---------------------------- compute header checksum --------------*/

 file->chkSumFiFlag = chkSumFlag;
 if(file->fmtVersion > 5)
      {FrCksumGnu((FRSCHAR*)file->header, hLen, &(file->chkSumFi));}
 else {file->chkSumFi += FrChkSum((FRSCHAR*)file->header, hLen, 0);}

 if(file->fmtVersion > 7) {
   file->chkSumFrHeader = file->chkSumFi;
   FrCksumGnu(NULL, hLen, &(file->chkSumFrHeader));
   file->chkTypeFiRead = h[39];
   if(FrDebugLvl > 0)  {
     fprintf(FrFOut," File checksum flag:%d \n",h[39]);
     fprintf(FrFOut," File produce by:%d (1=FrameL, 2=FrameCPP)\n",h[38]);}}

 return;}

/*---------------------------------------------------------- FrFileIRewind---*/
FrFile *FrFileIRewind(FrFile *iFile)
/*---------------------------------------------------------------------------*/
{
  if(iFile == NULL) return(NULL);
  
  FrFileIClose(iFile);             /*----------------- Close the old file ---*/

  iFile->current = iFile->fileH;   /*---- Set the pointer to the first file -*/

  FrFileIOpen(iFile);              /*----------------------- Open it again---*/

  return(iFile);
}
/*-------------------------------------------------------------- FrameReadT--*/
int FrFileISetTime(FrFile *iFile, double gtime)
/*---------------------------------------------------------------------------*/
{int i;

 i = FrTOCFrameFindNT(iFile, gtime);

 if(i< 0) return(i);

 i = FrFileIOSet(iFile, iFile->toc->positionH[i]);
 return(i);}

/*------------------------------------------------------FrFileISegListMatch--*/
void FrFileISegListMatch(FrFile *iFile)
/*---------------------------------------------------------------------------*/
{FrFileH *fileH, *next, **last;
  double coverage, ts;

 if(iFile          == NULL) return;
 if(iFile->segList == NULL) return;

 fileH = iFile->fileH;
 last = &(iFile->fileH);
 *last = NULL;

 for(; fileH != NULL; fileH = next)
  {next = fileH->next;
   ts = fileH->tStart;
   coverage = FrSegListCoverage(iFile->segList, ts, fileH->length);
   if(coverage > 0)
     {if(coverage < 1)
	 fileH->tStart = FrSegListFindFirstT(iFile->segList, ts, fileH->length);
         fileH->length = FrSegListFindLastT (iFile->segList, ts, fileH->length)
                                               - fileH->tStart;
      *last = fileH;
      last = &(fileH->next);}
   else
     {free(fileH->fileName);
      free(fileH);}}

 *last = NULL;

 return;}

/*--------------------------------------------------------------FrFileIOSet--*/
FRLONG FrFileIOSet(FrFile *iFile, FRLONG offset)
/*---------------------------------------------------------------------------*/
{
  FRLONG i;

  if(iFile->inMemory == FR_NO) {    /*------------------- read from file ---*/
    i = FrIOSet(iFile->frfd, offset);
    return(i);}

  /*--------------------------------------- read from the internal buffer---*/
  if(offset > iFile->bufSize) return(-1);

  iFile->p = iFile->buf + offset;
    
  return(0);}

/*-------------------------------------------------------FrFileIOSetFromEnd--*/
FRLONG FrFileIOSetFromEnd(FrFile *iFile, FRLONG offset)
/*---------------------------------------------------------------------------*/
{
  FRLONG i;

  if(iFile->inMemory == FR_NO) {    /*------------------- read from file ---*/
    i = FrIOSetFromEnd(iFile->frfd, offset);
    return(i);}

  /*--------------------------------------- read from the internal buffer---*/
  if(-offset > iFile->bufSize) return(-1);

  iFile->p = iFile->buf + iFile->bufSize + offset;
    
  return(0);}
/*-------------------------------------------------------FrFileIOSetFromCur--*/
FRLONG FrFileIOSetFromCur(FrFile *iFile, FRLONG offset)
/*---------------------------------------------------------------------------*/
{
  FRLONG i;

  if(iFile->inMemory == FR_NO) {    /*------------------- read from file ---*/
    i = FrIOSetFromCur(iFile->frfd, offset);
    return(i);}

  /*--------------------------------------- read from the internal buffer---*/
  if(offset + iFile->p > iFile->pMax) return(-1);

  iFile->p += offset;
    
  return(0);}

/*--------------------------------------------------------------FrFileIStat--*/
void FrFileIStat(FrFile *iFile, 
                 FILE *fp)
/*---------------------------------------------------------------------------*/
{FRULONG i, totSize = 0;

 if(iFile == NULL) return;

 for(i=1; i<iFile->maxSH; i++) 
    {if(iFile->sh[i] == NULL) continue;
     totSize += iFile->sh[i]->nBytes;}

 fprintf(fp," File summary:\n");
 for(i=1; i<iFile->maxSH; i++)
   {if(iFile->sh[i] == NULL) continue;
    if(iFile->sh[i]->nInstances == 0) continue;
    fprintf(fp, "%16s: %7d structures. Total size=%12"FRLLD
                " (%5.2f%% of the file)\n",
	   iFile->sh[i]->name, iFile->sh[i]->nInstances,
           iFile->sh[i]->nBytes,(100.*iFile->sh[i]->nBytes/(float)totSize));}
 fprintf(fp,"                                "
            "     Grand total:%12"FRLLD" bytes\n", totSize);

 return;}

/*--------------------------------------------------------------FrFileITEnd--*/
double FrFileITEnd(FrFile *iFile)
/*---------------------------------------------------------------------------*/
{FrFileH *fileH;
 double tEnd, fEnd;

 if(iFile == NULL) return(-1.);

 FrTOCFFLBuild(iFile);

           /*------------------------------------Loop on all file Heeader --*/

 tEnd = 0.;  

 for(fileH= iFile->fileH; fileH != NULL; fileH = fileH->next)
   {fEnd = fileH->tStart + fileH->length;
   if(fEnd > tEnd) tEnd = fEnd;}

 return(tEnd);}

/*---------------------------------------------------------FrFileITFirstEvt--*/
double FrFileITFirstEvt(FrFile *iFile)
/*---------------------------------------------------------------------------*/
{FrFileH *fileH;
 double tFirst;

 if(iFile == NULL) return(-1.);

 FrTOCFFLBuild(iFile);

           /*------------------------------------Loop on all file Heeader --*/

 tFirst = 1.e20;  

 for(fileH= iFile->fileH; fileH != NULL; fileH = fileH->next)
       {if(tFirst > fileH->tFirstEvt) tFirst = fileH->tFirstEvt;}

 return(tFirst);}

/*----------------------------------------------------------FrFileITLastEvt--*/
double FrFileITLastEvt(FrFile *iFile)
/*---------------------------------------------------------------------------*/
{FrFileH *fileH;
 double tLast;

 if(iFile == NULL) return(-1.);

 FrTOCFFLBuild(iFile);

           /*------------------------------------Loop on all file Heeader --*/

 tLast = 0;  

 for(fileH= iFile->fileH; fileH != NULL; fileH = fileH->next)
       {if(tLast < fileH->tLastEvt) tLast = fileH->tLastEvt;}

 return(tLast);}

/*--------------------------------------------------------FrFileITNextFrame--*/
double FrFileITNextFrame(FrFile *iFile,
                         double gtime)
/*---------------------------------------------------------------------------*/
/* This function return the GPS time for the next frame.                     */
/* It returns  a negative value if the frame could not be found              */
/*---------------------------------------------------------------------------*/
{int position;
 double GTimeS, GTimeN;

 if(iFile == NULL) return(-1.);

    /*--try to find the index for the frame corresponding at gtime ----------*/

 position = FrTOCFrameFindT(iFile, gtime);

    /*---if the frame has been found, get the start time of the next frame---*/

 if(position >= 0)
   {gtime = iFile->toc->GTimeS[position] + 1.e-9 *
            iFile->toc->GTimeN[position] +
            iFile->toc->dt[position] + 1.e-6;}

    /*--get the frame index for this (or following) updated (or not) gtime---*/

 position = FrTOCFrameFindNT(iFile, gtime);
 if(position < 0) return(-2.);
 
 if(iFile->toc == NULL) FrTOCReadFull(iFile);
 if(iFile->toc == NULL) return(-3.);

 GTimeS = iFile->toc->GTimeS[position];
 GTimeN = iFile->toc->GTimeN[position];

 return(GTimeS + 1.e-9*GTimeN);}

/*------------------------------------------------------------FrFileITStart--*/
double FrFileITStart(FrFile *iFile)
/*---------------------------------------------------------------------------*/
{FrFileH *fileH;
 double tStart;

 if(iFile == NULL) return(-1.);

 FrTOCFFLBuild(iFile);

            /*-------------------------------------Loop on all file Header --*/
 
 tStart = 1.e+30;           

 for(fileH= iFile->fileH; fileH != NULL; fileH = fileH->next)
   {if(fileH->tStart < tStart) tStart = fileH->tStart;}
    
 return(tStart);}

/*---------------------------------------------------------------FrFileOEnd--*/
int FrFileOEnd(FrFile *oFile)
/*---------------------------------------------------------------------------*/
{FRLONG nBytes;
 
  if(oFile == NULL) return(0);

  if((oFile->frfd == NULL) &&   /*--the file was not yet open (multiWrite)--*/
     (oFile->inMemory != FR_YES)) {
    FrFileFree(oFile);
    return(0);}

  if(oFile->pStart == NULL) FrFileOOpen(oFile);

  if(oFile->fLength > 0 && oFile->path != NULL) 
    {FrFileOReopen(oFile, -1);
     nBytes = oFile->nBytes;
     free(oFile->path);
     free(oFile->historyMsg);
     free(oFile);
     return(nBytes);}

  FrTOCWrite(oFile);
    
  FrEndOfFileWrite(oFile);

  if(oFile->frfd != NULL) {
    if(FrIOClose(oFile->frfd) != 0) oFile->error = FR_ERROR_WRITE_ERROR;}

  if(oFile->inMemory == FR_NO) free(oFile->buf);

  nBytes = oFile->nBytes;
  if(oFile->error != FR_OK) nBytes = 0;

  FrFileFree(oFile);
 
  return(nBytes);}

/*---------------------------------------------------------------FrFileONew--*/
FrFile *FrFileONew(char  *fileName, 
                   int  compress)
/*---------------------------------------------------------------------------*/
{FrFile *oFile;
 char  *outBuf; 
 int  bufSize;
 
 bufSize = 200000;
 outBuf = malloc(bufSize);
 if(outBuf == NULL) return(NULL);
 
 oFile = FrFileNew(fileName, compress, outBuf, bufSize);
 if(oFile == NULL) return(NULL);
                                       /*----- initialized output file ------*/

 oFile->frfd = FrIOOpenW(oFile->current->fileName);
 if(oFile->frfd == NULL) 
      {sprintf(FrErrMsg,"Open file error: %s", strerror(errno));
       FrError(3,"FrFileONew",FrErrMsg);
       oFile->error = FR_ERROR_OPEN_ERROR;
       return(NULL);}
   
 return(oFile);}

/*-------------------------------------------------------------FrFileONewFd--*/
FrFile *FrFileONewFd(FrIO *frfd, 
                     int compress)
/*---------------------------------------------------------------------------*/
{FrFile *oFile;
 char  *outBuf; 
 int  bufSize;
 
 bufSize = 200000;
 outBuf = malloc(bufSize);
 if(outBuf == NULL) return(NULL);

 oFile = FrFileNew("open_with_fd", compress, outBuf, bufSize);
 if(oFile == NULL) return(NULL);
                                       /*----- initialized output file ------*/

 oFile->frfd = frfd;
 if(oFile->frfd == NULL) 
           {FrError(3,"FrFileONewFd"," Open file error");
            oFile->error = FR_ERROR_OPEN_ERROR;
            return(NULL);}
 
 return(oFile);}
/*--------------------------------------------------------------FrFileONewH--*/
FrFile *FrFileONewH(char *fileName, 
                    int   compress,
                    char *program)
/*---------------------------------------------------------------------------*/
{FrFile *oFile;

 oFile = FrFileONew(fileName, compress);
 if(oFile == NULL) return(NULL);

 if(program != NULL) {
    free(oFile->historyMsg);
    sprintf(FrBuf,"FrameLib:%s %s",FrVersion,program);
    if(FrStrCpy(&oFile->historyMsg,  FrBuf) == NULL) return (NULL);}

 return(oFile);}

/*--------------------------------------------------------------FrFileONewM--*/
FrFile *FrFileONewM(char *path, 
                    int   compress,
                    char *program,
                    int   fLength)
/*---------------------------------------------------------------------------*/
{FrFile *oFile;

 oFile = (FrFile *) calloc(sizeof(FrFile), 1);
 if(oFile == NULL) return(NULL);

 if(FrStrCpy(&(oFile->path), path) == NULL) return(NULL);

 if(fLength <= 0) fLength = 2147483647; /*set to max value for a 32bit integer*/

 oFile->compress = compress;
 oFile->fLength  = fLength;
 oFile->aligned  = FR_YES;

 if(program != NULL) {
    sprintf(FrBuf,"FrameLib:%s %s",FrVersion,program);
    if(FrStrCpy(&oFile->historyMsg,  FrBuf) == NULL) return (NULL);}

 return(oFile);}

/*-------------------------------------------------------------FrFileONewMD--*/
FrFile *FrFileONewMD(char* path, 
		     int   compress,
		     char* program,
		     int   fLength,
		     char* filePrefix,
		     int   dirPeriod)
/*---------------------------------------------------------------------------*/
{
  FrFile *oFile;

  oFile = (FrFile *) calloc(sizeof(FrFile), 1);
  if(oFile == NULL) return(NULL);

  if(FrStrCpy(&(oFile->path), path) == NULL) return(NULL);
  if(FrStrCpy(&(oFile->prefix), filePrefix) == NULL) return(NULL);

  oFile->compress = compress;
  oFile->fLength  = fLength;
  oFile->aligned  = FR_YES;
  oFile->dirPeriod = dirPeriod;

  if(program != NULL) {
    sprintf(FrBuf,"FrameLib:%s %s",FrVersion,program);
    if(FrStrCpy(&oFile->historyMsg,  FrBuf) == NULL) return (NULL);}

  return(oFile);}

/*------------------------------------------------------------FrFileONewFdH--*/
FrFile *FrFileONewFdH(FrIO *frfd, 
                      int   compress,
                      char  *program)
/*---------------------------------------------------------------------------*/
{FrFile *oFile;

 oFile = FrFileONewFd(frfd, compress);
 if(oFile == NULL) return(NULL);

 if(program != NULL) {
    free(oFile->historyMsg);
    sprintf(FrBuf,"FrameLib:%s %s",FrVersion,program);
    if(FrStrCpy(&oFile->historyMsg,  FrBuf) == NULL) return (NULL);}

 return(oFile);}

/*--------------------------------------------------------------FrFileOOpen--*/
void FrFileOOpen(FrFile *oFile)
/*---------------------------------------------------------------------------*/
{char buf[40];
 FrSH *classe;
                                            /*---- write the file header ----*/
 FrFileHeader((unsigned char*) buf);
 if(oFile->chkSumFiFlag == FR_YES && FrFormatVersion >= 8) buf[39] = 1;
     
       /*------- compute the file header checksum (for version 8 and above---*/

 oFile->chkSumFrHeader = 0;
 FrCksumGnu((FRSCHAR*) buf, 40, &(oFile->chkSumFrHeader));
 FrCksumGnu( NULL, 40, &(oFile->chkSumFrHeader));

       /*---------------------------------------- write the header ----------*/

  if(oFile->inMemory == FR_YES)
          {memcpy(oFile->p, buf, 40);}
  else {if(FrIOWrite(oFile->frfd, buf, 40) != 40) 
                 {sprintf(FrErrMsg," write error:%s",strerror(errno));
                  FrError(3,"FrFileOOpen", FrErrMsg);
                  oFile->error = FR_ERROR_WRITE_ERROR;
                  return;}}
  oFile->p += 40;
  oFile->nBytes = 40;

  if(oFile->chkSumFiFlag == FR_YES)
    {FrCksumGnu((FRSCHAR*)buf, 40, &(oFile->chkSumFi));}

                    /*---- we do not write here the minimal dictionnary -----*/

  classe = FrSHDef();
  oFile->dicWrite[classe->id] = FR_YES;	 
  classe = FrSEDef();
  oFile->dicWrite[classe->id] = FR_YES;
   
  return;}

/*------------------------------------------------------FrFileOpenCacheFile--*/
FrFileH* FrFileOpenCacheFile(char   *fullName)
/*---------------------------------------------------------------------------*/
/* This function build the ffl structure for a "cache file"                  */
/* It returns a linked list of FrFileHeader structures.                      */
/*---------------------------------------------------------------------------*/
{
  FrFileH *fileH, *fileHFirst, *fileHLast;
  FILE  *fp;
  int length, i;

  fp = fopen(fullName,"r");
  if(fp == NULL){
    FrError(3,"FrFileOpenCacheFile","open cache file failed");
    return(NULL);}

  fileHFirst = NULL;
  fileHLast = NULL;

  while(fscanf(fp,"%s",FrBuf) == 1) {

    if(strncmp(FrBuf,"file:",5) != 0) continue; /*--search for the file name-*/

    fileH = (FrFileH *) malloc(sizeof(FrFileH));
    if(fileH == NULL) return(NULL);
    if(fileHFirst == NULL) {fileHFirst = fileH;}
    else              {fileHLast->next = fileH;}
    fileHLast = fileH;

    FrStrCpy(&fileH->fileName, FrBuf);
    if(fileH->fileName == NULL) break;

    fileH->tFirstEvt = -1.;
    fileH->tLastEvt  = -1;
    fileH->next   = NULL;

    length = strlen(FrBuf);

    /*-----------------the file length is the last number in the file name---*/
    FrBuf[length-4] = '\0';
    for(i = length-4; i > 1; i--) {
      if(FrBuf[i] == '-') break;}
    sscanf(FrBuf+i+1,"%lf",&fileH->length);

    /*-------------------------the file start is the parameter just before---*/
    FrBuf[i] = '\0';
    for(i--; i >= 0; i--) {
      if(FrBuf[i] == '-') break;}
    sscanf(FrBuf+i+1,"%lf",&fileH->tStart);}

  /*----------------------check the time ordering for the top linked list ---*/
  fileHFirst = FrFileSortByGPS(fileHFirst);

  return(fileHFirst);}

/*------------------------------------------------------------FrFileOReopen--*/
void FrFileOReopen(FrFile *oFile,
                   int  gps)
/*---------------------------------------------------------------------------*/
{char tmpName[512],name[512];
 FrFile *tmp;
 int fileDuration, ierr;
 
 if(oFile == NULL) return;
                               /*----------- close output file if needed ----*/
 if(oFile->frfd != NULL)
   {if(oFile->closingTime > gps && gps >= 0) return;

    tmp = malloc(sizeof(FrFile));
    if(tmp == NULL) return;
    memcpy(tmp, oFile, sizeof(FrFile));
    tmp->historyMsg = NULL;
    tmp->path       = NULL;

    fileDuration = oFile->currentEndTime - oFile->startTime;

    if(oFile->dirPeriod != 0) {
      sprintf(name,"%.300s/%.180s-%d-%d.gwf", oFile->dirName,
	      oFile->prefix, oFile->startTime, fileDuration);}
    else {
      sprintf(name,"%.480s-%d-%d.gwf", oFile->path, oFile->startTime, fileDuration);}

    sprintf(tmpName,"%.510s",oFile->current->fileName);

    oFile->nBytes = FrFileOEnd(tmp);
    oFile->frfd  = NULL;
    oFile->fileH = NULL;
    oFile->toc   = NULL;
    oFile->sh    = NULL;
    oFile->maxSH = 0;

    if(oFile->nBytes > 0) rename(tmpName, name);
    else oFile->error = FR_ERROR_WRITE_ERROR;}

                                /*----------- open output file if needed ----*/
 if(oFile->frfd == NULL && gps >= 0) {

   if(oFile->dirPeriod != 0) {
     if(oFile->dirName == NULL) {
       oFile->dirName = malloc(strlen(oFile->path)+20);
       if(oFile->dirName == NULL) {
	 oFile->error = FR_ERROR_OPEN_ERROR;
	 return;}}
     sprintf(oFile->dirName,"%s-%d", oFile->path, gps/oFile->dirPeriod);
     ierr = FrIOmkdir(oFile->dirName);
     if(ierr != 0) {
       oFile->error = FR_ERROR_OPEN_ERROR;
       FrError(1,"FrFileOReopen","Cannot create directory");
       return;}
     sprintf(name,"%s/%s-%d-%d.gwf_NOT_YET_CLOSED", 
	     oFile->dirName,oFile->prefix, gps, oFile->fLength);}
   else {
     sprintf(name,"%s-%d-%d.gwf_NOT_YET_CLOSED", 
	     oFile->path, gps, oFile->fLength);}
   if(FrDebugLvl > 0) fprintf(FrFOut,"Open file: %s\n", name);

    tmp = FrFileONew(name, oFile->compress);
    if(tmp == NULL) 
      {oFile->error = FR_ERROR_OPEN_ERROR;
       return;}
    free(tmp->historyMsg);
    tmp->historyMsg = oFile->historyMsg;
    tmp->fLength    = oFile->fLength;
    tmp->aligned    = oFile->aligned;
    tmp->path       = oFile->path;
    tmp->dirPeriod  = oFile->dirPeriod;
    tmp->dirName    = oFile->dirName;
    tmp->prefix     = oFile->prefix;
    memcpy(oFile, tmp, sizeof(FrFile));
    free(tmp);
    oFile->startTime   = gps;
    oFile->closingTime = gps +oFile->fLength;
    if(oFile->aligned == FR_YES) 
       oFile->closingTime -= (oFile->closingTime%oFile->fLength);}
 
 return;}

/*-----------------------------------------------------------FrFileORealloc--*/
int FrFileORealloc(FrFile *oFile, char *name, int size)
/*---------------------------------------------------------------------------*/
{int offset1, offset2;

 if(oFile->inMemory == FR_YES)
    {FrError(3,name,"Output File buffer overflow");
     oFile->error = FR_ERROR_BUFF_OVERFLOW;
     return(1);} 

 if(FrDebugLvl > 2) printf("FrFileORealloc for %s size=%d\n",name,size);
 offset1 = oFile->p      - oFile->buf;
 offset2 = oFile->pStart - oFile->buf;

 oFile->buf = realloc(oFile->buf, oFile->bufSize + size);
 if(oFile->buf == NULL)
    {FrError(3,name,"Output File buffer overflow (realloc failed)");
     oFile->error = FR_ERROR_BUFF_OVERFLOW;
     return(2);} 
   
 oFile->bufSize += size;
 oFile->p      = oFile->buf + offset1;
 oFile->pStart = oFile->buf + offset2;
 oFile->pMax   = oFile->buf + oFile->bufSize;
 
 return(0);}

/*------------------------------------------------------FrFileOSetGzipLevel--*/
void FrFileOSetGzipLevel(FrFile *oFile, unsigned short level)
/*---------------------------------------------------------------------------*/
{
 if(oFile != NULL) {oFile->gzipLevel = level;}

 return;}

/*------------------------------------------------------------FrFileOSetMsg--*/
void FrFileOSetMsg(FrFile *oFile, char *msg)
/*---------------------------------------------------------------------------*/
{
 if(oFile == NULL) return;

 free(oFile->historyMsg);
 oFile->historyMsg = NULL;

 if(msg != NULL) {
    sprintf(FrBuf,"FrameLib:%s %s",FrVersion,msg);
    FrStrCpy(&oFile->historyMsg,  FrBuf);}

 return;}

/*----------------------------------------------------------------FrGetLeapS--*/
int FrGetLeapS(unsigned int gps)
/*---------------------------------------------------------------------------*/
/* this function returns the leaps second for a given GPS time               */
/*---------------------------------------------------------------------------*/
{
 int uLeapS; 
  
  if  (gps > 1025136015)   uLeapS = FRGPSLEAPS; /* Sun Jul  1 00:00:00 2012*/
  else if(gps > 914803214) uLeapS = 34;  /* Sun Jan  1 00:00:00 2009*/
  else if(gps > 820108813) uLeapS = 33;  /* Sun Jan  1 00:00:00 2006*/
  else if(gps > 599184012) uLeapS = 32;  /* Fri Jan  1 00:00:00 1999*/
  else if(gps > 551750411) uLeapS = 31;  /* Tue Jul  1 00:00:00 1997*/
  else if(gps > 504489610) uLeapS = 30;  /* Mon Jan  1 00:00:00 1996*/
  else if(gps > 457056009) uLeapS = 29;  /* Fri Jul  1 00:00:00 1994*/
  else if(gps > 425520008) uLeapS = 28;  /* Thu Jul  1 00:00:00 1993*/
  else                     uLeapS = 0;   /* too old cases are not handled */

  return(uLeapS);
}

/*---------------------------------------------------------FrGetCurrentGPS---*/
double FrGetCurrentGPS ()
/*---------------------------------------------------------------------------*/
{
  double gps, currentTime;
  struct timeval tv;
  int leapS;

  gettimeofday(&tv, NULL);
  currentTime = tv.tv_sec + tv.tv_usec*1.0e-6;

  gps = currentTime - FRGPSOFF - FRGPSTAI;
  leapS = FrGetLeapS(gps);
  gps = gps + FrGetLeapS(gps+leapS);

  return(gps);
}

/*-------------------------------------------------------------FrHistoryAdd--*/
FrHistory *FrHistoryAdd(FrameH *frame, 
                        char *comment)
/*---------------------------------------------------------------------------*/
{FrHistory *history;
 unsigned int mytime;
  
 mytime = FrGetCurrentGPS();
 history = FrHistoryNew(NULL, mytime, comment);

                            /*----- now store it in the Frame structure -----*/
  if(frame != NULL)
    {FrStrCpy(&history->name,frame->name);
     history->next = frame->history;
     frame->history = history;}

 return(history);}

/*-----------------------------------------------------------FrHistoryCopy---*/
FrHistory *FrHistoryCopy(FrHistory *historyIn)
/*---------------------------------------------------------------------------*/
/* Copy the full linked list of history records. Return NULL in case of error*/
/*---------------------------------------------------------------------------*/
{
  FrHistory *h, *history, *root, *last;

  root = NULL;
  last = NULL;

  for(history = historyIn; history != NULL; history = history->next) {
    h = FrHistoryNew(history->name, history->time, history->comment);
    if(h == NULL) return(NULL);
    if(root == NULL) root = h;
    if(last != NULL) last->next = h;
    last = h;}

  return(root);
}
/*-------------------------------------------------------------FrHistoryDef--*/
FrSH *FrHistoryDef()    
/*---------------------------------------------------------------------------*/
{static FrSH *classe = NULL;

  if(classe != NULL) return(classe);
  classe = FrDicAddS("FrHistory",FrHistoryRead);

  FrSENew(classe, "STRING", "name","-");
  FrSENew(classe, "INT_4U", "time","-");
  FrSENew(classe, "STRING", "comment","-");
  FrSENew(classe, "PTR_STRUCT(FrHistory *)", "next","-");
  if(FrFormatVersion >= 8) FrSENew(classe, "INT_4U", "chkSum","-");

  return(classe);}
 
/*------------------------------------------------------------FrHistoryFree--*/
void FrHistoryFree(FrHistory *history)
/*---------------------------------------------------------------------------*/
{
 if(history == NULL) return;

 if(history->next != NULL) FrHistoryFree(history->next);

 if(history->comment != NULL) free(history->comment);
 if(history->name    != NULL) free(history->name);
 free(history);

 return;}

/*-------------------------------------------------------------FrHistoryNew--*/
FrHistory *FrHistoryNew(char *name,
                        unsigned int mytime, 
                        char *comment)
/*---------------------------------------------------------------------------*/
{FrHistory *history;
 
 history = (FrHistory *) calloc(1,sizeof(FrHistory));
 if(history == NULL) return(NULL);
 history->classe = FrHistoryDef();
 
 history->time = mytime;
 if(name != NULL)
   {if(FrStrCpy(&history->name,    name) == NULL) return(NULL);}
 if(FrStrCpy(&history->comment, comment) == NULL) return(NULL);

 return(history);}

/*------------------------------------------------------------FrHistoryRead--*/
void FrHistoryRead(FrFile *iFile)
/*---------------------------------------------------------------------------*/
{FrHistory *history;
 char message[128];

  history = (FrHistory *) calloc(1,sizeof(FrHistory));
  if(history == NULL)  
       {iFile->error = FR_ERROR_MALLOC_FAILED;
        return;}
  history->classe = FrHistoryDef();
  
  FrReadHeader(iFile,  history);
  FrReadSChar (iFile, &history->name); 
  FrReadIntU  (iFile, &history->time);
  FrReadSChar (iFile, &history->comment);
  FrReadStruct(iFile, &history->next);
  if(iFile->fmtVersion >=8) FrReadStructChksum(iFile);

  if(iFile->nBytes != iFile->nBytesR + iFile->length)
     {sprintf(message,": Record length error: "
         "nBytes=%"FRLLD" nBytesR=%"FRLLD" length=%"FRLLD"\n",
          iFile->nBytes, iFile->nBytesR, iFile->length);
      FrError(3,"FrHistoryRead",message);
      return;}     

 if(FrDebugLvl > 2) fprintf(FrFOut," %s\n", history->name);

return;}

/*--------------------------------------------------------- FrHistoryWrite---*/
void FrHistoryWrite(FrHistory *history, 
                    FrFile *oFile)
/*---------------------------------------------------------------------------*/
{
  FrPutNewRecord(oFile, history, FR_YES);
  FrPutSChar (oFile, history->name);
  FrPutIntU  (oFile, history->time);
  FrPutSChar (oFile, history->comment);
  FrPutStruct(oFile, history->next);
  FrPutWriteRecord(oFile, FR_NO);

  if(history->next != NULL)  {FrHistoryWrite(history->next, oFile);}

  return;}

/*--------------------------------------------------------------- FrMsgAdd --*/
FrMsg *FrMsgAdd(FrameH *frame, 
                char *alarm,
                char *message, 
                unsigned int severity)
/*---------------------------------------------------------------------------*/
{FrMsg *msg;
                                /*--- first create and fill the structure ---*/

  msg = (FrMsg *) calloc(1,sizeof(FrMsg));
  if(msg == NULL)  return(NULL);
  msg->classe = FrMsgDef();
	 
  if(FrStrCpy(&msg->message,message) == NULL) 
       {FrError(3,"FrMsgAdd","malloc failed");
        return(NULL);}
  msg->severity = severity;

  if(FrStrCpy(&msg->alarm, alarm) == NULL)
       {FrError(3,"FrMsgAdd","Cannot malloc alarm name");
        return(NULL);}
                              /*---------- then attach it to the frame  -----*/

  if(frame == NULL) return(msg);

  if(frame->rawData == NULL) {FrRawDataNew(frame);}
  if(frame->rawData == NULL) 
       {FrError(3,"FrMsgAdd","Cannot create FrRawData");
        return(NULL);}
     
  msg->next = frame->rawData->logMsg;
  frame->rawData->logMsg = msg;
  
  msg->GTimeS = frame->GTimeS;
  msg->GTimeN = frame->GTimeN;

  return(msg);} 

/*------------------------------------------------------------- FrMsgDef ----*/
FrSH *FrMsgDef()
/*---------------------------------------------------------------------------*/
{static FrSH *classe = NULL;

  if(classe != NULL) return(classe);
  classe = FrDicAddS("FrMsg", (void (*)())FrMsgRead);

  FrSENew(classe, "STRING", "alarm","-");
  FrSENew(classe, "STRING", "message","-");
  FrSENew(classe, "INT_4U", "severity","-");
  FrSENew(classe, "INT_4U", "GTimeS","-");
  FrSENew(classe, "INT_4U", "GTimeN","-");
  FrSENew(classe, "PTR_STRUCT(FrMsg *)", "next","-");
  if(FrFormatVersion >= 8) FrSENew(classe, "INT_4U", "chkSum","-");

  return(classe);}

/*-------------------------------------------------------------- FrMsgDump---*/
void FrMsgDump(FrMsg *msg,
               FILE *fp, 
               int debugLvl)
/*---------------------------------------------------------------------------*/
{
 fprintf(fp," Msg:");
 if(msg->alarm   != NULL) 
      fprintf(fp,"%s", msg->alarm);
 else fprintf(fp,"-");
 fprintf(fp,"  severity=%d GTimeS=%d N=%d\n",
         msg->severity, msg->GTimeS, msg->GTimeN); 
 if(msg->message != NULL) fprintf(fp,"  message:%s\n", msg->message);

 return;}

/*----------------------------------------------------------------FrMsgFind--*/
FrMsg *FrMsgFind(FrameH *frame,
                 char *alarm,
                 FrMsg *msg)
/*---------------------------------------------------------------------------*/
{
 if(alarm == NULL)          return (NULL);
 if(frame == NULL)          return (NULL);
 if(frame->rawData == NULL) return (NULL);

 if(msg == NULL) 
      {msg = frame->rawData->logMsg;}
 else {msg = msg->next;}

 for(; msg != NULL; msg = msg->next)
   {if(strcmp(msg->alarm, alarm) == 0) break;}
  
 return (msg);}

/*-------------------------------------------------------------FrMsgFree-----*/
void FrMsgFree(FrMsg *msg)
/*---------------------------------------------------------------------------*/
{ 
 if(msg == NULL) return;

 if(msg->next != NULL) FrMsgFree(msg->next);

 if(msg->alarm   != NULL) free (msg->alarm);
 if(msg->message != NULL) free (msg->message);
 free(msg);

 return;}

/*-------------------------------------------------------------- FrMsgRead---*/
FrMsg *FrMsgRead(FrFile *iFile)
/*---------------------------------------------------------------------------*/
{FrMsg *msg;
 char message[128];

 if(iFile->fmtVersion <= 5) return(FrBack4MsgRead(iFile));

  msg = (FrMsg *) calloc(1,sizeof(FrMsg));
  if(msg == NULL)  
       {iFile->error = FR_ERROR_MALLOC_FAILED;
        return(NULL);}
  msg->classe = FrMsgDef();

  FrReadHeader(iFile,  msg);
  FrReadSChar (iFile, &msg->alarm); 
  FrReadSChar (iFile, &msg->message);
  FrReadIntU  (iFile, &msg->severity);
  FrReadIntU  (iFile, &msg->GTimeS);
  FrReadIntU  (iFile, &msg->GTimeN);
  FrReadStruct(iFile, &msg->next);
  if(iFile->fmtVersion >=8) FrReadStructChksum(iFile);

  if(iFile->nBytes != iFile->nBytesR + iFile->length)
     {sprintf(message,": Record length error: "
         "nBytes=%"FRLLD" nBytesR=%"FRLLD" length=%"FRLLD"\n",
          iFile->nBytes, iFile->nBytesR, iFile->length);
      FrError(3,"FrMsgRead",message);
      return(NULL);}     

  if(FrDebugLvl > 2) fprintf(FrFOut," %s\n", msg->alarm);

return(msg);}

 /*------------------------------------------------------------ FrMsgWrite---*/
void FrMsgWrite(FrMsg *msg,
                FrFile *oFile)
/*---------------------------------------------------------------------------*/
{
  FrPutNewRecord(oFile, msg, FR_YES);
  FrPutSChar (oFile, msg->alarm);
  FrPutSChar (oFile, msg->message);
  FrPutIntU  (oFile, msg->severity);
  FrPutIntU  (oFile, msg->GTimeS);
  FrPutIntU  (oFile, msg->GTimeN);
  FrPutStruct(oFile, msg->next);
  FrPutWriteRecord(oFile, FR_NO);
    
 return;}
 
/*--------------------------------------------------------------FrObjectDef--*/
FrSH *FrObjectDef()
/*---------------------------------------------------------------------------*/
{static FrSH *classe = NULL;

  if(classe != NULL) return(classe);
  classe = FrDicAddS("FrObject",NULL);

  return(classe);}
/*-----------------------------------------------------FrProcDataAddHistory--*/
FrHistory *FrProcDataAddHistory(FrProcData *proc, 
                                char *comment,
                                int nPrevious,
                                ...)
/*---------------------------------------------------------------------------*/
{FrHistory *history, *old, *copy, **last;
 va_list ap;
 unsigned int i, mytime;

 if(proc == NULL) return NULL;
  
 mytime = FrGetCurrentGPS();
 history = FrHistoryNew(proc->name, mytime, comment);
         
                       /*----- now store it in the FrPrcoData structure -----*/
 if(proc != NULL)
    {FrStrCpy(&history->name, proc->name);
     history->next = proc->history;
     proc->history = history;}

            /*------ add the old history records at the end of the list -----*/

 last = &(proc->history);
 while(*last != NULL) {last = &((*last)->next);}

 va_start(ap,nPrevious);
 for(i=0; i<nPrevious; i++)
     {old = va_arg(ap,void *);
      copy = FrHistoryNew(old->name, old->time, old->comment);
      *last = copy;      
      last = &(copy->next);}

   va_end(ap);

 return(history);}

/*-------------------------------------------------------FrProcDataAddParam--*/
FrProcData *FrProcDataAddParam(FrProcData *proc,  
                               char *name, 
                               double value)
/*---------------------------------------------------------------------------*/
{
 if(proc == NULL) return(NULL);

 if(proc->nAuxParam > 65534) return(NULL); /*--overflow the storage capacity-*/

 proc->nAuxParam++;

 if(proc->nAuxParam == 1)
   {proc->auxParam      = (double *) malloc(sizeof(double));
    proc->auxParamNames = (char  **) malloc(sizeof(char *));}
 else   
   {proc->auxParam      = (double *) realloc(proc->auxParam, 
                                             proc->nAuxParam* sizeof(double));
    proc->auxParamNames = (char  **) realloc(proc->auxParamNames,
                                             proc->nAuxParam* sizeof(char *));}

 if(proc->auxParam      == NULL) return(NULL);
 if(proc->auxParamNames == NULL) return(NULL);

 if(FrStrCpy(&(proc->auxParamNames[proc->nAuxParam - 1]),name) == NULL) 
                                             return(NULL);
 proc->auxParam[proc->nAuxParam - 1] = value;

 return(proc);} 

/*---------------------------------------------------------------------------*/
void FrProcDataAttachVect(FrProcData *proc,  
                          FrVect *vect)
/*---------------------------------------------------------------------------*/
{FrVect **aux;

 if(proc == NULL) return;

 aux = &(proc->aux);
 while(*aux != NULL) {aux = &((*aux)->next);}
 
 *aux = vect;
 
 return;}

/*--------------------------------------------------------- FrProcDataCopy---*/
FrProcData *FrProcDataCopy(FrProcData *procIn, FrameH* frame)
/*---------------------------------------------------------------------------*/
/* Copy one FrProcData. Retiurns NULL in case of error                      */
/*---------------------------------------------------------------------------*/
{
  FrProcData *proc;
  int i;
 
  if(procIn == NULL) return(NULL);

  proc = FrProcDataNewV(frame, FrVectCopy(procIn->data));
  if(proc == NULL) return(NULL);  

  proc->type       = procIn->type;
  proc->subType    = procIn->subType;
  proc->timeOffset = procIn->timeOffset;
  proc->tRange     = procIn->tRange;
  proc->fShift     = procIn->fShift;
  proc->phase      = procIn->phase;
  proc->fRange     = procIn->fRange;
  proc->BW         = procIn->BW;

  if(procIn->comment != NULL) {
    FrStrCpy(&proc->comment, procIn->comment);
    if(proc->comment == NULL) return(NULL);}

  for(i=0; i<procIn->nAuxParam; i++) {
    FrProcDataAddParam(proc, procIn->auxParamNames[i], procIn->auxParam[i]);}
 
  proc->aux     = FrVectCopy( procIn->aux);
  proc->table   = FrTableCopy(procIn->table);
  proc->history = FrHistoryCopy(procIn->history);

  return(proc);}

/*----------------------------------------------------------- FrProcDataDef--*/
FrSH *FrProcDataDef()
/*---------------------------------------------------------------------------*/
{static FrSH *classe = NULL;

  if(classe != NULL) return(classe);
  classe = FrDicAddS("FrProcData",(void (*)())FrProcDataRead);

  FrSENew(classe, "STRING", "name","-");
  FrSENew(classe, "STRING", "comment","-");
  FrSENew(classe, "INT_2U", "type","-");
  FrSENew(classe, "INT_2U", "subType","-");
  FrSENew(classe, "REAL_8", "timeOffset","-");
  FrSENew(classe, "REAL_8", "tRange","-");
  FrSENew(classe, "REAL_8", "fShift","-");
  FrSENew(classe, "REAL_4", "phase", "-");
  FrSENew(classe, "REAL_8", "fRange", "-");
  FrSENew(classe, "REAL_8", "BW", "-");
  FrSENew(classe, "INT_2U", "nAuxParam", "-");
  FrSENew(classe, "REAL_8[nAuxParam]", "auxParam","-");
  FrSENew(classe, "STRING[nAuxParam]", "auxParamNames", "-");
  FrSENew(classe, "PTR_STRUCT(FrVect *)",  "data","-");
  FrSENew(classe, "PTR_STRUCT(FrVect *)",  "aux","-");
  FrSENew(classe, "PTR_STRUCT(FrTable *)", "table","-");
  FrSENew(classe, "PTR_STRUCT(FrHistory *)", "history","-");
  FrSENew(classe, "PTR_STRUCT(FrProcData *)", "next","-");
  if(FrFormatVersion >= 8) FrSENew(classe, "INT_4U", "chkSum","-");

  return(classe);}

/*---------------------------------------------------------- FrProcDataDump--*/
void FrProcDataDump(FrProcData *proc, 
                    FILE *fp, 
                    int debugLvl)
/*---------------------------------------------------------------------------*/
{FrHistory *hst;
 int i;

 if(proc == NULL) return;

 fprintf(fp," ProcData: %s type =%d", proc->name, proc->type);
 if      (proc->type == 0) fprintf(fp,"(Unknown/user defined)");
 else  if(proc->type == 1) fprintf(fp,"(Time serie)");
 else  if(proc->type == 2) fprintf(fp,"(Frequency serie)");
 else  if(proc->type == 3) fprintf(fp,"(Other serie data)");
 else  if(proc->type == 4) fprintf(fp,"(Time frequency)");
 else  if(proc->type == 5) fprintf(fp,"(Wavelets)");
 else  if(proc->type == 6) fprintf(fp,"(Multi-dimension)");
 else                      fprintf(fp,"(Undefined format)");
 fprintf(fp," subType =%d\n", proc->subType);

 if(debugLvl < 2) return;
 if(proc->comment != NULL) 
   {fprintf(fp,"  comment:%s\n", proc->comment);}

 for(hst = proc->history; hst != NULL; hst = hst->next) 
   {fprintf(fp,"  history:%s (time:%d) comment:%s\n", 
           hst->name, hst->time, hst->comment);}
 
 fprintf(fp,"  timeOffset=%.5f tRange=%.4f fShift=%g phase=%.2f "
	 "fRange=%.2f BW=%g\n",proc->timeOffset, proc->tRange, proc->fShift,
	 proc->phase, proc->fRange, proc->BW);

 if(proc->nAuxParam > 0) 
   {fprintf(fp,"  auxiliary parameters:");
    for(i=0; i<proc->nAuxParam; i++)
      {fprintf(fp," %s=%g",proc->auxParamNames[i],proc->auxParam[i]);}
    fprintf(fp,"\n");}
    
 if(proc->data  != NULL) FrVectDump(proc->data,   fp, debugLvl);
 if(proc->aux   != NULL) FrVectDump(proc->aux,    fp, debugLvl);
 if(proc->table != NULL) FrTableDump(proc->table, fp, debugLvl);

 return;}

/*-----------------------------------------------------------FrProcDataFind--*/
FrProcData *FrProcDataFind(FrameH *frame,
                           char *name)
/*---------------------------------------------------------------------------*/
{FrProcData *proc;

 if(name  == NULL)              return (NULL);
 if(frame == NULL)              return (NULL);
 if(frame->procData == NULL)    return (NULL);

 proc = (FrProcData*) FrameFindBasic((FrBasic*) frame->procData, name); 
 if(proc == NULL) return(NULL);

 if(proc->data  != NULL) FrVectExpandF(proc->data);
 if(proc->aux   != NULL) FrVectExpandF(proc->aux);
 if(proc->table != NULL) FrTableExpand(proc->table);
    
 proc->data->GTime     = frame->GTimeS + 
                         frame->GTimeN*1.e-9 + proc->timeOffset;
 proc->data->ULeapS    = frame->ULeapS;
 proc->data->localTime = FrameGetLTOffset(frame, name);
  
 return (proc);}

/*---------------------------------------------------------------------------*/
FrVect*  FrProcDataFindVect(FrProcData *proc,  
                            char *name)
/*---------------------------------------------------------------------------*/
{FrVect *vect;

 if(proc == NULL) return(NULL);

 for(vect = proc->aux; vect != NULL; vect = vect->next)
   {if(strcmp(vect->name, name) == 0) break;}

 return(vect);}
 
/*-----------------------------------------------------------FrProcDataFree--*/
void FrProcDataFree(FrProcData *procData)
/*---------------------------------------------------------------------------*/
{
 while(procData != NULL) {procData = FrProcDataFreeOne(procData);}

 return;}

/*--------------------------------------------------------FrProcDataFreeOne--*/
FrProcData* FrProcDataFreeOne(FrProcData *proc)
/*---------------------------------------------------------------------------*/
{int i;
 FrProcData *next;

 if(proc == NULL) return(NULL);

 next = proc->next;

 if(proc->name    != NULL) free(proc->name);
 if(proc->comment != NULL) free(proc->comment);

 for(i=0; i<proc->nAuxParam; i++) {free(proc->auxParamNames[i]);}
 if(proc->nAuxParam > 0) free(proc->auxParam);
 if(proc->nAuxParam > 0) free(proc->auxParamNames);

 if(proc->data    != NULL) FrVectFree(proc->data);
 if(proc->aux     != NULL) FrVectFree(proc->aux);
 if(proc->table   != NULL) FrTableFree(proc->table);
 if(proc->history != NULL) FrHistoryFree(proc->history);
 
 free(proc);

 return(next);}

/*------------------------------------------------------FrProcDataGetParam--*/
double FrProcDataGetParam(FrProcData *proc, 
                          char *name)
/*--------------------------------------------------------------------------*/
{int id;

 id = FrProcDataGetParamId(proc, name);

 if(id < 0) return(-1.);

 return(proc->auxParam[id]);}

/*----------------------------------------------------FrProcDataGetParamId--*/
int FrProcDataGetParamId(FrProcData *proc, 
                         char *name)
/*--------------------------------------------------------------------------*/
{int i;

 if(proc == NULL) return(-1);
 if(name == NULL) return(-1);

 for(i=0; i<proc->nAuxParam; i++)
   {if(strcmp(proc->auxParamNames[i], name) == 0) return(i);}

 return (-1);}

/*-------------------------------WARNING:OBSOLETE FUNCTION---FrProcDataGetV--*/
FrVect *FrProcDataGetV(FrameH *frame, char *name)
/*---------------------------------------------------------------------------*/
{return(FrameFindProcVect(frame,name));}

/*------------------------------------------------------------FrProcDataNew--*/
FrProcData *FrProcDataNew(FrameH *frame,  
                          char *name, 
                          double sampleRate, 
                          FRLONG nData, 
                          int nBits)
/*---------------------------------------------------------------------------*/
{FrProcData *procData;
 FrVect* vect;
 double dx;
 int type;
 
 if(nBits >16)       {type = FR_VECT_4S;}
 else if(nBits >  8) {type = FR_VECT_2S;}
 else if(nBits >  0) {type = FR_VECT_C;}
 else if(nBits >-33) {type = FR_VECT_4R;}
 else                {type = FR_VECT_8R;}

 if(sampleRate == 0.)
      {dx = 0.;}
 else {dx = 1./sampleRate;}

 vect = FrVectNew1D(name, type, nData, dx,"time (s)",NULL);
 if(vect == NULL) return(NULL);
  
 procData = FrProcDataNewVT(frame, vect, 1);

 return(procData);}
 
/*-----------------------------------------------------------FrProcDataNewV--*/
FrProcData *FrProcDataNewV(FrameH *frame,
                           FrVect* vect)
/*---------------------------------------------------------------------------*/
{FrProcData *procData;
 
 if(vect == NULL) return(NULL);

 procData = (FrProcData *) calloc(1,sizeof(FrProcData));
 if(procData == NULL) return(NULL); 
 procData->classe = FrProcDataDef();
	 
 if(FrStrCpy(&procData->name,vect->name) == NULL) return(NULL);
 procData->data = vect;
                           /*----- now store it in the Frame structures -----*/
 if(frame != NULL) FrameAddProc(frame, procData);
  
 return(procData);}
 
/*----------------------------------------------------------FrProcDataNewVT--*/
FrProcData* FrProcDataNewVT(FrameH* frame, FrVect *vect, int type)
/*---------------------------------------------------------------------------*/
{
  FrProcData *proc;

  proc = FrProcDataNewV(frame, vect);
  if(proc == NULL) return(NULL);

  proc->type = type;

  return(proc);
} 
/*--------------------------------------------------------- FrProcDataRead---*/
FrProcData *FrProcDataRead(FrFile *iFile)
/*---------------------------------------------------------------------------*/
{FrProcData *procData;
 char message[128];

 if(iFile->fmtVersion == 3) return(FrBack3ProcDataRead(iFile));
 if(iFile->fmtVersion == 4) return(FrBack4ProcDataRead(iFile));
 if(iFile->fmtVersion == 5) return(FrBack5ProcDataRead(iFile));

  procData = (FrProcData *) calloc(1,sizeof(FrProcData));
  if(procData == NULL)  
       {iFile->error = FR_ERROR_MALLOC_FAILED;
        return(NULL);}
  procData->classe = FrProcDataDef();
  
  FrReadHeader(iFile,  procData);
  FrReadSChar( iFile, &procData->name);
  FrReadSChar( iFile, &procData->comment);
  FrReadShortU(iFile, &procData->type);
  FrReadShortU(iFile, &procData->subType);
  FrReadDouble(iFile, &procData->timeOffset);
  FrReadDouble(iFile, &procData->tRange);
  FrReadDouble(iFile, &procData->fShift);
  FrReadFloat (iFile, &procData->phase);
  FrReadDouble(iFile, &procData->fRange);
  FrReadDouble(iFile, &procData->BW);
  FrReadShortU(iFile, &procData->nAuxParam);
  if(procData->nAuxParam > 0)  
   {FrReadVD (iFile, &procData->auxParam,      procData->nAuxParam);
    FrReadVQ (iFile, &procData->auxParamNames, procData->nAuxParam);}
  FrReadStruct(iFile, &procData->data);
  iFile->vectInstance = iFile->instance;
  FrReadStruct(iFile, &procData->aux);
  FrReadStruct(iFile, &procData->table);
  FrReadStruct(iFile, &procData->history);
  FrReadStruct(iFile, &procData->next);
  if(iFile->fmtVersion >=8) FrReadStructChksum(iFile);

  if(iFile->nBytes != iFile->nBytesR + iFile->length)
     {sprintf(message,": Record length error: "
         "nBytes=%"FRLLD" nBytesR=%"FRLLD" length=%"FRLLD"\n",
          iFile->nBytes, iFile->nBytesR, iFile->length);
      FrError(3,"FrProcDataRead",message);
      return(NULL);}     

  if(FrDebugLvl > 2) fprintf(FrFOut," %s\n", procData->name);

  return(procData);}

/*--------------------------------------------------------- FrProcDataReadT--*/
FrProcData *FrProcDataReadT(FrFile *iFile,
                            char *name,
                            double gtime)
/*---------------------------------------------------------------------------*/
{FrProcData  **current, *proc, *root;
 FrTOCts *ts;
 FrTag *frtag;
 int index;
                       /*---- find frame(it will read the FrTOC if needed) --*/

 index = FrTOCFrameFindT(iFile, gtime);
 if(index <0) return(NULL);

 if(name == NULL) return(NULL);     /*------------------ build tag object ---*/
 frtag = FrTagNew(name);
 if(frtag == NULL) return(NULL);
                                
                       /*-------------- Extract all the requested channels --*/
 root = NULL;
 current = &root;

 for(ts = iFile->toc->proc; ts != NULL; ts = ts->next)

                       /*---------- check if we need to copy this proc ?-----*/

   {if(FrTagMatch(frtag, ts->name) == FR_NO) continue; 

                       /*---- set the file pointer and read the data -------*/

    if(FrTOCSetPos(iFile, ts->position[index]) != 0) continue;
    proc = FrProcDataRead(iFile);
    if(proc == NULL) continue;

    gtime     = iFile->toc->GTimeS[index]+
                iFile->toc->GTimeN[index]*1.e-9+proc->timeOffset;
    proc->data = FrVectReadNext(iFile, gtime, proc->name);

    *current = proc;
    current = &(proc->next);}
 
 FrTagFree(frtag); 

 return(root);}

/*-------------------------------------------------------- FrProcDataWrite---*/
void FrProcDataWrite(FrProcData *procData,
                     FrFile *oFile)
/*---------------------------------------------------------------------------*/
{
  FrPutNewRecord(oFile, procData, FR_YES);
  FrPutSChar (oFile, procData->name);
  FrPutSChar (oFile, procData->comment);
  FrPutShortU(oFile, procData->type);
  FrPutShortU(oFile, procData->subType);
  FrPutDouble(oFile, procData->timeOffset);
  FrPutDouble(oFile, procData->tRange);
  FrPutDouble(oFile, procData->fShift);
  FrPutFloat (oFile, procData->phase);
  FrPutDouble(oFile, procData->fRange);
  FrPutDouble(oFile, procData->BW);
  FrPutShortU(oFile, procData->nAuxParam);
  if(procData->nAuxParam > 0)  
    {FrPutVD (oFile, procData->auxParam,      procData->nAuxParam);
     FrPutVQ (oFile, procData->auxParamNames, procData->nAuxParam);}
  FrPutStruct(oFile, procData->data);
  FrPutStruct(oFile, procData->aux);
  FrPutStruct(oFile, procData->table);
  FrPutStruct(oFile, procData->history);
  FrPutStruct(oFile, procData->next);

  if(oFile->toc != NULL) FrTOCtsMark(oFile, &oFile->toc->procH, 
                                      procData->name, 0, 0);

  FrPutWriteRecord(oFile, FR_NO);
                                    /*----------------- data write buffer ---*/

  FrVectWrite (procData->data, oFile);

  if(procData->aux    != NULL) {FrVectWrite   (procData->aux,     oFile);}
  if(procData->table  != NULL) {FrTableWrite  (procData->table,   oFile);}
  if(procData->history!= NULL) {FrHistoryWrite(procData->history, oFile);}
     
                                   /*---------- write next struct. if any ---*/

  if(procData->next != NULL)  {FrProcDataWrite(procData->next, oFile);}  

 return;}

/*----------------------------------------------------------------FrPutChar--*/
void FrPutChar(FrFile *oFile,
               char value)
/*---------------------------------------------------------------------------*/
{
 if(oFile->error != FR_OK) return;

 if(oFile->p+1 > oFile->pMax) 
      {if(FrFileORealloc(oFile, "FrPutChar", 50) != 0) return;}

 *oFile->p++ = value;
    
 return;}
/*--------------------------------------------------------------FrPutDouble--*/
void FrPutDouble(FrFile *oFile,
                 double value)
/*---------------------------------------------------------------------------*/
{
 if(oFile->error != FR_OK) return;

 if(oFile->p+FrSdouble > oFile->pMax) 
     {if(FrFileORealloc(oFile, "FrPutDouble", 50) != 0) return;}

 memcpy(oFile->p, &value, FrSdouble); 
 oFile->p += FrSdouble;
    
 return;}

/*----------------------------------------------------------------FrPutFloat-*/
void FrPutFloat(FrFile *oFile,
                float value)
/*---------------------------------------------------------------------------*/
{
 if(oFile->error != FR_OK) return;
  
 if(oFile->p+FrSfloat > oFile->pMax) 
     {if(FrFileORealloc(oFile, "FrPutFloat", 50) != 0) return;}

 memcpy(oFile->p, &value, FrSfloat);
 oFile->p += FrSfloat;
     
 return;}

/*-----------------------------------------------------------------FrPutInt--*/
void FrPutInt(FrFile *oFile,
              int value)
/*---------------------------------------------------------------------------*/
{
 if(oFile->error != FR_OK) return;
 
 if(oFile->p+FrSint > oFile->pMax) 
    {if(FrFileORealloc(oFile, "FrPutInt", 50) != 0) return;}
   
 memcpy(oFile->p, &value, FrSint);
 oFile->p += FrSint;
     
 return;}

/*----------------------------------------------------------------FrPutIntU--*/
void FrPutIntU(FrFile *oFile,
               unsigned int value)
/*---------------------------------------------------------------------------*/
{
 if(oFile->error != FR_OK) return;
 
 if(oFile->p+FrSint > oFile->pMax) 
    {if(FrFileORealloc(oFile, "FrPutIntU", 50) != 0) return;}

 memcpy(oFile->p, &value, FrSint);
 oFile->p += FrSint;
     
 return;}

/*----------------------------------------------------------------FrPutLong--*/
void FrPutLong(FrFile *oFile,
               FRLONG value)
/*---------------------------------------------------------------------------*/
{short ref;
 char *swap;
 int dummy[2];

 if(oFile->error != FR_OK) return;

 if(oFile->p+8 > oFile->pMax) 
    {if(FrFileORealloc(oFile, "FrPutLong", 50) != 0) return;}
 
 if(FrSlong == 8)   
   {memcpy(oFile->p, &value, FrSlong);}

       /*--- for computer with 4 bytes long, add 4 more bytes. 
                  There location depends of the bytes ordering---------------*/
 else
   {swap = (char *)&ref;
    ref = 0x1234;
    if(swap[0] == 0x12)  
         {dummy[0] = 0;
          dummy[1] = value;}
    else {dummy[0] = value;
          dummy[1] = 0;}
     memcpy(oFile->p, dummy, 8);}

 oFile->p += 8;
 
 return;}
 
/*----------------------------------------------------------FrPutNewRecord---*/
void FrPutNewRecord(FrFile *oFile,
                    void *instance,
                    FRBOOL withInstanceId)
/*---------------------------------------------------------------------------*/
{short type;
 int id;
 FrameH *proto; /* all structures start like FrameH */
 
 if(oFile->error != FR_OK) return;

 proto = instance;
 if(proto == NULL) {FrError(3,"FrPutNewRecord","instance == NULL");}
 if(proto->classe == NULL) {FrError(3,"FrPutNewRecord","classe == NULL");}

 if(oFile->dicWrite[proto->classe->id] == FR_NO) 
   {oFile->dicWrite[proto->classe->id] = FR_YES; 
    FrSHWrite(proto->classe, oFile);}  
    
 if(oFile->inMemory == FR_NO) {oFile->p = oFile->buf;} 

 oFile->pStart = oFile->p;

 if(oFile->p+14 > oFile->pMax)       /* 14 = sizeof(INT_8U + INT_2U + INT_4U)*/
    {if(FrFileORealloc(oFile, "FrPutNewRecord", 50) != 0) return;}

 oFile->p += 8;                     /*- the record length is not an INT_8U --*/

 type = (short) proto->classe->id;
 if(FrFormatVersion < 8) 
   {memcpy(oFile->p, &type, FrSshort);}
 else 
   {if(oFile->chkSumFrFlag == FR_YES) oFile->p[0] = 1;
    else oFile->p[0] = 0;
    oFile->p[1] = type;}
 oFile->p += 2;

 if(withInstanceId == FR_YES)
      {id = FrDicGetId(oFile, type, instance);}
 else {if(type == 1)      id = oFile->nSH++;
       else if(type == 2) id = oFile->nSE++;
       else               id = 0;}
 memcpy(oFile->p, &id, 4);
 oFile->p += 4;
     
 return;}
 
/*-------------------------------------------------------------FrPutStruct---*/
void FrPutStruct(FrFile *oFile,
                 void *address)
/*---------------------------------------------------------------------------*/
{short class;
 FrameH *example; 
 FrSH *type;
 int instance;
       
 if(oFile->error != FR_OK) return;

 if(oFile->p+50 > oFile->pMax) 
   {if(FrFileORealloc(oFile, "FrPutStruct", 50) != 0) return;}
    
 if(address == NULL)
   {memset(oFile->p, 0, 6); 
    oFile->p += 6;
    return;}

  /*--we assume that the first element of the structure is like in FrFrameH--*/
    
 example = address;
 type = example->classe;
 if(type == NULL) {FrError(3,"FrPutStruct","classe == NULL");}
       
 class = type->id; 
 memcpy(oFile->p, &class, 2); 
 oFile->p += FrSshort; 

 instance = FrDicAssignId(oFile, type->id, address);                
 memcpy(oFile->p, &instance, 4); 
 oFile->p += 4;

 return;}

/*---------------------------------------------------------------FrPutShort--*/
void FrPutShort(FrFile *oFile,
                short value)
/*---------------------------------------------------------------------------*/
{   
 if(oFile->error != FR_OK) return;

 if(oFile->p+FrSshort > oFile->pMax) 
     {if(FrFileORealloc(oFile, "FrPutShort", FrSshort) != 0) return;}

 memcpy(oFile->p, &value, FrSshort);
 oFile->p += FrSshort;
     
 return;}

/*---------------------------------------------------------------FrPutShortU-*/
void FrPutShortU(FrFile *oFile,
                 unsigned short value)
/*---------------------------------------------------------------------------*/
{   
 if(oFile->error != FR_OK) return;

 if(oFile->p+FrSshort > oFile->pMax) 
     {if(FrFileORealloc(oFile, "FrPutShortU", FrSshort) != 0) return;}
 
 memcpy(oFile->p, &value, FrSshort);
 oFile->p += FrSshort;
    
 return;}

/*---------------------------------------------------------------FrPutSChar--*/
void FrPutSChar(FrFile *oFile,
                char *value)
/*---------------------------------------------------------------------------*/
{int slen;
 char *empty="";

 if(oFile->error != FR_OK) return;

 if(value == NULL) 
   {value = empty;}

 slen = strlen(value)+1;
 if(slen > 65535) 
    {FrError(3,"FrPutSChar","too long string"); 
     oFile->error = FR_ERROR_TOO_LONG_STRING;
     return;} 

 FrPutShortU(oFile, (short) slen);

 if(slen != 0) 
    {if(oFile->p+slen > oFile->pMax) 
       {if(FrFileORealloc(oFile, "FrPutSChar", slen) != 0) return;}

     memcpy(oFile->p, value, slen);
     oFile->p += slen;}
 
 return;}

/*------------------------------------------------------------------FrPutVC--*/
void FrPutVC(FrFile *oFile,
             char *data,
             int nData)
/*---------------------------------------------------------------------------*/
{
 if(oFile->error != FR_OK) return;

 if(oFile->p+nData > oFile->pMax) 
     {if(FrFileORealloc(oFile, "FrPutVC", nData) != 0) return;}
  
 memcpy(oFile->p, data, nData);
 oFile->p += nData;

 return;}

/*------------------------------------------------------------------FrPutVD--*/
void FrPutVD(FrFile *oFile, 
             double *data, 
             int nData)
/*---------------------------------------------------------------------------*/
{int size;
	
 if(oFile->error != FR_OK) return;

 size = FrSdouble*nData;
 if(oFile->p+size > oFile->pMax) 
   {if(FrFileORealloc(oFile, "FrPutVD", size) != 0) return;}
   
 memcpy(oFile->p, data, size);
 oFile->p += size;

 return;}

/*------------------------------------------------------------------FrPutVF--*/
void FrPutVF(FrFile *oFile, 
             float *data, 
             int nData)
/*---------------------------------------------------------------------------*/
{int size;

 if(oFile->error != FR_OK) return;

 size = FrSfloat*nData;
 if(oFile->p+size > oFile->pMax) 
    {if(FrFileORealloc(oFile, "FrPutVF", size) != 0) return;}

 memcpy(oFile->p, data, size);
 oFile->p += size;
	
 return;}
/*------------------------------------------------------------------FrPutVF--*/
void FrPutVFD(FrFile *oFile, 
              double *data, 
              int nData)
/*---------------------------------------------------------------------------*/
/* This function first convert the double to float and then write it         */
{int size, i;
 float dataF;

 if(oFile->error != FR_OK) return;

 size = FrSfloat*nData;
 if(oFile->p+size > oFile->pMax) 
    {if(FrFileORealloc(oFile, "FrPutVFD", size) != 0) return;}

 for(i=0; i<nData; i++)
   {dataF = data[i];
    memcpy(oFile->p, &dataF, FrSfloat);
    oFile->p += FrSfloat;}
	
 return;}

/*------------------------------------------------------------------FrPutVI--*/
void FrPutVI(FrFile *oFile, 
             int *data, 
             int nData)
/*---------------------------------------------------------------------------*/
{int size;

 if(oFile->error != FR_OK) return;

 size = FrSint*nData;
 if(oFile->p+size > oFile->pMax) 
     {if(FrFileORealloc(oFile, "FrPutVI", size) != 0) return;}

 memcpy(oFile->p, data, size);
 oFile->p += size;
	
 return;}

/*------------------------------------------------------------------FrPutVL--*/
void FrPutVL(FrFile *oFile, 
             FRLONG *data, 
             int nData)
/*---------------------------------------------------------------------------*/
{short ref;
 char *swap;
 int size, i, j, dummy[2];

 if(oFile->error != FR_OK) return;

 size = 8*nData;
 if(oFile->p+size > oFile->pMax) 
    {if(FrFileORealloc(oFile, "FrPutVL", size) != 0) return;}

 if(FrSlong == 8)   
   {memcpy(oFile->p, data, size);
    oFile->p += size;}

       /*--- for computer with 4 bytes long, add 4 more bytes. 
                  There location depends of the bytes ordering---------------*/
 else
   {swap = (char *)&ref;
    ref = 0x1234;
    if(swap[0] == 0x12)  
         j = 1;
    else j = 0; 
 
    dummy[0] = 0;
    dummy[1] = 0;

    for(i=0; i<nData; i++)
      {dummy[j] = data[i];
       memcpy(oFile->p, dummy, 8);
       oFile->p += 8;}}
	
 return;}
/*------------------------------------------------------------------FrPutVQ--*/
void FrPutVQ(FrFile *oFile, 
             char **data, 
             int nData)
/*---------------------------------------------------------------------------*/
{int i;

 for(i=0; i<nData; i++)
    {FrPutSChar(oFile, data[i]);}

 return;}

/*------------------------------------------------------------------FrPutVS--*/
void FrPutVS(FrFile *oFile, 
             short *data, 
             int nData)
/*---------------------------------------------------------------------------*/
{int size;

 if(oFile->error != FR_OK) return;

 size = FrSshort*nData;
 if(oFile->p+size > oFile->pMax) 
    {if(FrFileORealloc(oFile, "FrPutVS", size) != 0) return;}

 memcpy(oFile->p, data, size);
 oFile->p += size;		
	
 return;}

/*---------------------------------------------------------FrPutWriteRecord--*/
void FrPutWriteRecord(FrFile *oFile, FRBOOL endOfFile)
/*---------------------------------------------------------------------------*/
{FRULONG len, extraLen=0;
 short ref;
 char *swap;
 int dummy[2];
 unsigned int chkSum;
 
 if(oFile->error != FR_OK) return;

 /*--------------------------------------------- compute the record length---*/

 len = (FRULONG) (oFile->p)  - (FRULONG)(oFile->pStart); 

 /*-------for format v8: add space the structure and endOffile checksum(s)---*/

 if(FrFormatVersion >= 8) {
   if(endOfFile == FR_NO) {
     extraLen = FrSint;}
   else {
     extraLen = 2*FrSint;}
   len += extraLen;
   if(oFile->p+extraLen > oFile->pMax) {
     if(FrFileORealloc(oFile, "FrPutIntU", 50) != 0) return;}}

 /*--------------------------------------store the record length (INT_8U) ---*/

 if(FrSlong == 8) {  /*-if running on a system where long integer = 8 bytes-*/ 
   memcpy(oFile->pStart, &len, FrSlong);}
 else {
   swap = (char *)&ref;
   ref = 0x1234;
   if(swap[0] == 0x12) {
     dummy[0] = 0;
     dummy[1] = len;}
   else {
     dummy[0] = len;
     dummy[1] = 0;}
   memcpy(oFile->pStart, dummy, 8);}

 /*---------------------------------- compute Frame/structure checksum ------*/

 if(FrFormatVersion < 8) {
   if(oFile->chkSumFrFlag == FR_YES) {
     FrCksumGnu((FRSCHAR*)oFile->pStart,len, &(oFile->chkSumFr));}}
 else {
   chkSum = 0;
   if(oFile->chkSumFrFlag == FR_YES) {
     FrCksumGnu((FRSCHAR*)oFile->pStart,len - extraLen, &chkSum);
     FrCksumGnu( NULL, len - extraLen, &chkSum);}

   if(oFile->p+FrSint > oFile->pMax) {
     if(FrFileORealloc(oFile, "FrPutWriteRecord", FrSint) != 0) return;}
   memcpy(oFile->p, &chkSum, FrSint);
   oFile->p += FrSint;}

 /*--------------------------------------------- compute file checksum ------*/

 oFile->nBytes += len;

 if(FrFormatVersion < 8) {
   if(oFile->chkSumFiFlag == FR_YES) {
     FrCksumGnu((FRSCHAR*)oFile->pStart,len, &(oFile->chkSumFi));
     if(endOfFile == FR_YES) {
       FrCksumGnu( NULL, oFile->nBytes, &(oFile->chkSumFi));
       memcpy(oFile->pStart+30, &(oFile->chkSumFi), FrSint);}}}

 else {
   if(oFile->chkSumFiFlag == FR_YES) {
     if(endOfFile == FR_NO) {
       FrCksumGnu((FRSCHAR*)oFile->pStart,len, &(oFile->chkSumFi));}
     else {
       FrCksumGnu((FRSCHAR*)oFile->pStart,len - FrSint, &(oFile->chkSumFi));
       FrCksumGnu(NULL,oFile->nBytes-FrSint, &(oFile->chkSumFi));}}

   if(oFile->p+FrSint > oFile->pMax) {
     if(FrFileORealloc(oFile, "FrPutWriteRecord", FrSint) != 0) return;}
   memcpy(oFile->p, &(oFile->chkSumFi), FrSint);
   if(endOfFile == FR_YES) oFile->p += FrSint;}

 /*--------------------------------------- write data on external device ----*/

 if(oFile->inMemory == FR_NO) 
    {if(FrIOWrite(oFile->frfd,oFile->pStart, len) != len) 
            {sprintf(FrErrMsg," write error:%s",strerror(errno));
             FrError(3,"FrPutWriteRecord",FrErrMsg);
             oFile->error = FR_ERROR_WRITE_ERROR;}}

 return;}

/*-------------------------------------------------------------FrRawDataDef--*/
FrSH *FrRawDataDef()
/*---------------------------------------------------------------------------*/
{static FrSH *classe = NULL;

  if(classe != NULL) return(classe);
  classe = FrDicAddS("FrRawData",(void (*)())FrRawDataRead);

  FrSENew(classe, "STRING", "name","-");
  FrSENew(classe, "PTR_STRUCT(FrSerData *)", "firstSer","-");
  FrSENew(classe, "PTR_STRUCT(FrAdcData *)", "firstAdc","-");
  FrSENew(classe, "PTR_STRUCT(FrTable *)", "firstTable","-");
  FrSENew(classe, "PTR_STRUCT(FrMsg *)", "logMsg","-");
  FrSENew(classe, "PTR_STRUCT(FrVect *)", "more","-");
  if(FrFormatVersion >= 8) FrSENew(classe, "INT_4U", "chkSum","-");

  return(classe);}

/*---------------------------------------------------------- FrRawDataDump---*/
void FrRawDataDump(FrRawData *rawData, 
                   FILE *fp, 
                   int debugLvl)
/*---------------------------------------------------------------------------*/
{FrAdcData *adcData;
 FrMsg     *msg;
 FrSerData *smsData;
 FrTable   *table;

 if(fp      == NULL) return;
 if(rawData == NULL) return;
 if(debugLvl < 1)    return;

 fprintf(fp,"Dump Raw data:\n");

           /*------------------------- dump logMsg data ---------------------*/

 for(msg = rawData->logMsg; msg != NULL; msg = msg->next )
    {FrMsgDump(msg, fp, debugLvl);}

           /*--------------------------- dump adc data ----------------------*/

 for(adcData = rawData->firstAdc; adcData != NULL; adcData = adcData->next) 
    {FrAdcDataDump( adcData, fp, debugLvl);}

            /*---------------------- dump Slow monitoring data --------------*/

 for(smsData = rawData->firstSer; smsData != NULL; smsData = smsData->next) 
    {FrSerDataDump(smsData, fp, debugLvl);}

           /*------------------------------- dump tables data ---------------*/

 for(table = rawData->firstTable; table != NULL; table = table->next) 
    {FrTableDump(table, fp, debugLvl);}

 return;}
  
/*------------------------------------------------------------FrRawDataFree--*/
void FrRawDataFree(FrRawData *rawData)
/*---------------------------------------------------------------------------*/
{
 if(rawData->name     != NULL) free(rawData->name);
 if(rawData->firstSer != NULL) FrSerDataFree(rawData->firstSer);

 if(rawData->firstAdcOld != NULL) FrRawDataUntagAdc(rawData);
 if(rawData->firstSerOld != NULL) FrRawDataUntagMsg(rawData);
 if(rawData->firstSerOld != NULL) FrRawDataUntagSer(rawData);

 if(rawData->firstAdc   != NULL) FrAdcDataFree(rawData->firstAdc);
 if(rawData->firstTable != NULL) FrTableFree  (rawData->firstTable);
 if(rawData->logMsg     != NULL) FrMsgFree    (rawData->logMsg);
 if(rawData->more       != NULL) FrVectFree   (rawData->more);

 free(rawData);

 return;}

/*------------------------------------------------------------FrRawDataNew--*/
FrRawData *FrRawDataNew(FrameH *frame)
/*--------------------------------------------------------------------------*/
{FrRawData *rawData;
  
 rawData = (FrRawData *) calloc(1,sizeof(FrRawData));
 if(rawData == NULL) return(NULL);
 rawData->classe = FrRawDataDef();

 FrStrCpy(&rawData->name,"rawData");
 rawData->firstAdcOld = NULL;
  
 if(frame != NULL) {frame->rawData = rawData;}
  
 return(rawData);}

/*-----------------------------------------------------------FrRawDataRead--*/
FrRawData *FrRawDataRead(FrFile *iFile)
/*--------------------------------------------------------------------------*/
{FrRawData *rawData;
 char message[128];

 if(iFile->fmtVersion == 3) return(FrBack3RawDataRead(iFile));

 rawData = (FrRawData *) calloc(1,sizeof(FrRawData));
 if(rawData == NULL)  
      {iFile->error = FR_ERROR_MALLOC_FAILED;
       return(NULL);}
 rawData->classe = FrRawDataDef();
  
 FrReadHeader(iFile,  rawData);
 FrReadSChar (iFile, &rawData->name); 
 FrReadStruct(iFile, &rawData->firstSer);
 FrReadStruct(iFile, &rawData->firstAdc);
 FrReadStruct(iFile, &rawData->firstTable);
 FrReadStruct(iFile, &rawData->logMsg);
 FrReadStruct(iFile, &rawData->more);
  if(iFile->fmtVersion >=8) FrReadStructChksum(iFile);

  if(iFile->nBytes != iFile->nBytesR + iFile->length)
     {sprintf(message,": Record length error: "
         "nBytes=%"FRLLD" nBytesR=%"FRLLD" length=%"FRLLD"\n",
          iFile->nBytes, iFile->nBytesR, iFile->length);
      FrError(3,"FrRawDataRead",message);
      return(NULL);}     

 if(FrDebugLvl > 2) fprintf(FrFOut," %s\n", rawData->name);

 return(rawData);}
  
/*-------------------------------------------------------FrRawDataUntagAdc---*/
void FrRawDataUntagAdc(FrRawData *rawData)
/*---------------------------------------------------------------------------*/
{
 if(rawData == NULL) return;
 if(rawData->firstAdcOld == NULL) return;

 FrameUntagBasic((FrBasic **)&rawData->firstAdc, 
                 (FrBasic **)&rawData->firstAdcOld);

 return;}
/*-------------------------------------------------------FrRawDataUntagMsg---*/
void FrRawDataUntagMsg(FrRawData *rawData)
/*---------------------------------------------------------------------------*/
{
 if(rawData == NULL) return;
 if(rawData->logMsgOld == NULL) return;

 FrameUntagBasic((FrBasic **)&rawData->logMsg, 
                 (FrBasic **)&rawData->logMsgOld);

 return;}
/*-------------------------------------------------------FrRawDataUntagSer---*/
void FrRawDataUntagSer(FrRawData *rawData)
/*---------------------------------------------------------------------------*/
{
 if(rawData == NULL) return;
 if(rawData->firstSerOld == NULL) return;

 FrameUntagBasic((FrBasic **)&rawData->firstSer, 
                 (FrBasic **)&rawData->firstSerOld);

 return;}

/*-----------------------------------------------------------FrRawDataWrite--*/
void FrRawDataWrite(FrRawData *rawData,
                    FrFile *oFile)
/*---------------------------------------------------------------------------*/
{FrAdcData *adcData;
 FrSerData *serData;
 FrMsg     *logMsg;

	     /*-------------------first write rawData struct-----------------*/
 
 FrPutNewRecord(oFile, rawData, FR_YES);
 FrPutSChar (oFile, rawData->name);
 FrPutStruct(oFile, rawData->firstSer);
 FrPutStruct(oFile, rawData->firstAdc);
 FrPutStruct(oFile, rawData->firstTable);
 FrPutStruct(oFile, rawData->logMsg);
 FrPutStruct(oFile, rawData->more);
 FrPutWriteRecord(oFile, FR_NO);   
 
           /*-----------------then write adc data struct--------------------*/

 if((rawData->firstAdc != NULL) && (oFile->toc != NULL))
   {oFile->toc->nFirstADC[oFile->toc->nFrame] = oFile->nBytes;}

 adcData = rawData->firstAdc;
 while(adcData != NULL) 
    {FrAdcDataWrite(adcData, oFile);
     adcData = adcData->next;}
    
            /*------------write the slow monitoring station stuff------------*/
 
 if((rawData->firstSer != NULL) && (oFile->toc != NULL))
   {oFile->toc->nFirstSer[oFile->toc->nFrame] = oFile->nBytes;}

 serData = rawData->firstSer;
 while(serData != NULL) 
    {FrSerDataWrite(serData, oFile);
     serData = serData->next;}


 if(rawData->firstTable != NULL) 
   {if(oFile->toc != NULL)
       oFile->toc->nFirstTable[oFile->toc->nFrame] = oFile->nBytes;
    FrTableWrite(rawData->firstTable, oFile);}
    
           /*---------------------write error messages-----------------------*/

 if((rawData->logMsg != NULL) && (oFile->toc != NULL))
   {oFile->toc->nFirstMsg[oFile->toc->nFrame] = oFile->nBytes;}

 logMsg = rawData->logMsg;
 while(logMsg != NULL) 
    {FrMsgWrite(logMsg, oFile);
     logMsg = logMsg->next;}

           /*-----------------finally write aditional data-------------------*/
   
 if(rawData->more != NULL) {FrVectWrite(rawData->more, oFile);}

 return;}

/*-------------------------------------------------------------------FrRead--*/
FRLONG FrRead(FrFile *iFile,
              char *buf,
              FRULONG n)
/*---------------------------------------------------------------------------*/
{FRLONG i,nb;

 if(iFile->inMemory == FR_NO) 
     {i = 0;
      do{nb = FrIORead(iFile->frfd, buf+i, n-i);
         i += nb;
        }while(nb>0 && i<n);
      if(i != n)
        {if(errno == 0 && nb == 0)
              sprintf(FrErrMsg,"FrRead error: EOF reached"); 
         else sprintf(FrErrMsg,"FrRead error:%s",strerror(errno));
         FrError(3,"FrError", FrErrMsg);}}

      /*--------- the previous logic is complicate since a read may 
                need several calls to deliver all the requested bytes -------*/
         
 else{memcpy(buf,iFile->p,n);
      iFile->p += n;
      i = n;
      if(iFile->p > iFile->pMax) 
         {i = i - (iFile->p - iFile->pMax);}}

      /*------------------------- compute checksum if requested--------------*/

 if(iFile->chkSumFiFlag == FR_YES)
   {if(iFile->fmtVersion > 5)
         {FrCksumGnu((FRSCHAR*)buf, i, &(iFile->chkSumFi));}
    else {iFile->chkSumFi += FrChkSum((FRSCHAR*)buf, i, iFile->nBytes);}}
 if(iFile->chkSumFrFlag == FR_YES)
   {FrCksumGnu((FRSCHAR*)buf, i, &(iFile->chkSumFr));}
 
 iFile->nBytes += i;

 return(i);}  

/*------------------------------------------------------------FrReadDouble---*/
void FrReadDouble(FrFile *iFile, 
                  double *value)
/*---------------------------------------------------------------------------*/
{int i;
 unsigned char *buf,local[8];
   
  if(iFile->error != FR_OK) {return;}
 
  if(iFile->fmType == FRNATIVE)
       {i = FrRead(iFile,(char *) value, FrSdouble);}
  else 
       {i = FrRead(iFile,(char *) local,FrSdouble);
        buf = (unsigned char *) value;
        buf[0] = local[7];
        buf[1] = local[6];
        buf[2] = local[5];
        buf[3] = local[4];
        buf[4] = local[3];
        buf[5] = local[2];
        buf[6] = local[1];
        buf[7] = local[0];}

  if(i == 0) 
    {FrError(3,"FrReadDouble","end of file");
     iFile->error = FR_ERROR_READ_ERROR;}

  return;}

/*--------------------------------------------------------------FrReadFloat--*/
void FrReadFloat(FrFile *iFile,
                 float *value)
/*---------------------------------------------------------------------------*/
{int i;
 unsigned char local[4], *buf;

  if(iFile->error != FR_OK) {return;}
 
  if(iFile->fmType == FRNATIVE)
       {i = FrRead(iFile, (char *) value, FrSfloat);}
  else 
       {i = FrRead(iFile,(char *) local,FrSfloat);
        buf = (unsigned char *) value;
        buf[0] = local[3];
        buf[1] = local[2];
        buf[2] = local[1];
        buf[3] = local[0];}
        
  if(i == 0) 
    {FrError(3,"FrReadFloat","end of file");
     iFile->error = FR_ERROR_READ_ERROR;} 

  return;}

/*-------------------------------------------------------------FrReadHeader--*/
void FrReadHeader(FrFile *iFile,
                  void *instance)
/*---------------------------------------------------------------------------*/
{unsigned short id2;

  if(iFile->fmtVersion > 5)
       {iFile->nBytesR = iFile->nBytes-10;
        FrReadIntU(iFile, &(iFile->instanceH));}
  else {iFile->nBytesR = iFile->nBytes-6;
        FrReadShortU(iFile, &id2);
        iFile->instanceH = id2;}
  if(iFile->error == FR_OK)   {FrSetBAT(iFile, iFile->instanceH, instance);}

  return;}
 
/*----------------------------------------------------------------FrReadInt--*/
void FrReadInt(FrFile *iFile,
               int *value)
/*---------------------------------------------------------------------------*/
{int i;
 unsigned char *buf,local[4];
   
  if(iFile->error != FR_OK) {return;}
 
  if(iFile->fmType == FRNATIVE)
       {i = FrRead(iFile,(char *) value, FrSint);}
  else 
       {i = FrRead(iFile,(char *) local,FrSint);
        buf = (unsigned char *) value;
        buf[0] = local[3];
        buf[1] = local[2];
        buf[2] = local[1];
        buf[3] = local[0];}

  if(i == 0) 
    {FrError(3,"FrReadInt","end of file");
     iFile->error = FR_ERROR_READ_ERROR;} 

  return;}

/*----------------------------------------------------------------FrReadInt--*/
void FrReadIntU(FrFile *iFile,
                unsigned int *value)
/*---------------------------------------------------------------------------*/
{int i;
 unsigned char *buf,local[4];
   
  if(iFile->error != FR_OK) {return;}
 
  if(iFile->fmType == FRNATIVE)
       {i = FrRead(iFile,(char *) value, FrSint);}
  else 
       {i = FrRead(iFile,(char *) local,FrSint);
        buf = (unsigned char *) value;
        buf[0] = local[3];
        buf[1] = local[2];
        buf[2] = local[1];
        buf[3] = local[0];}

  if(i == 0) 
    {FrError(1,"FrReadIntU","End of file detected");
     iFile->error = FR_ERROR_READ_ERROR;} 

  return;}

/*---------------------------------------------------------------FrReadLong--*/
void FrReadLong(FrFile *iFile,
                FRLONG *value)
/*---------------------------------------------------------------------------*/
{FRLONG i, *localI;
 unsigned char *buf,local[8];
 
  if(iFile->error != FR_OK) {return;}
   
  if(iFile->fmLong == FRNATIVE)
       {i = FrRead(iFile, (char *) value, FrSlong);}
  else
    {buf = (unsigned char *) value;
     i = FrRead(iFile,(char *) local, iFile->header[9]);
     if((iFile->fmLong == FRSWAP) && (FrSlong == 8))
       {buf[7] = local[0];
        buf[6] = local[1];
        buf[5] = local[2];
        buf[4] = local[3];
        buf[3] = local[4];
        buf[2] = local[5];
        buf[1] = local[6];
        buf[0] = local[7];}
     else if((iFile->fmLong == FRSWAP) && (FrSlong == 4))
       {buf[3] = local[0];
        buf[2] = local[1];
        buf[1] = local[2];
        buf[0] = local[3];}
     else if(iFile->fmLong == FREXPAND)
       {buf[7] = 0;
        buf[6] = 0;
        buf[5] = 0;
        buf[4] = 0;
        buf[3] = local[3];
        buf[2] = local[2];
        buf[1] = local[1];
        buf[0] = local[0];}
     else if(iFile->fmLong == FREXPAND_AND_SWAP)
       {buf[7] = 0;
        buf[6] = 0;
        buf[5] = 0;
        buf[4] = 0;
        buf[3] = local[0];
        buf[2] = local[1];
        buf[1] = local[2];
        buf[0] = local[3];}
     else if(iFile->fmLong == FRCONTRACT)
       {localI = (FRLONG *) local;
        *value = 0;
        if(localI[0] == 0) 
             *value = localI[1];
        else *value = localI[0];}
     else if(iFile->fmLong == FRCONTRACT_AND_SWAP)
       {localI = (FRLONG *) local;
        if(localI[0] == 0) 
           {buf[0] = local[7];
            buf[1] = local[6];
            buf[2] = local[5];
            buf[3] = local[4];}
        else
           {buf[0] = local[3];
            buf[1] = local[2];
            buf[2] = local[1];
            buf[3] = local[0];}}
     else 
       {FrError(3,"FrReadLong","unknown type");}} 

  if(i != iFile->header[9]) 
    {FrError(3,"FrReadLong","end of file");
     iFile->error = FR_ERROR_READ_ERROR;} 

  return;}

/*-------------------------------------------------------------FrReadRecord--*/
void FrReadRecord(FrFile *iFile)
/*---------------------------------------------------------------------------*/
{
              /*--- try to find a read function for this type of record------*/

 FrSHMatch(iFile);  

 if(iFile->sh[iFile->type]->objRead != FrReadRecord)
   {iFile->sh[iFile->type]->objRead(iFile);
    return;}

             /*----------- well, no match found, skip the record ------------*/

 if(FrDebugLvl > 0) fprintf(FrFOut,"    skip unknown structure. "
                    "type=%d size=%"FRLLD"\n",iFile->type,iFile->length);
 FrError(3,"FrReadRecord","unknown record");

 FrReadSkipRecord(iFile);
             
 return;}
 
/*--------------------------------------------------------------FrReadSChar--*/
void FrReadSChar(FrFile *iFile,
                 char **value)
/*---------------------------------------------------------------------------*/
{unsigned short slen;

  if(iFile->error != FR_OK) {return;}
 
  FrReadShortU(iFile, &slen); 
  if(iFile->error != FR_OK) {return;}
  if(slen == 0)
    {*value = NULL;}
  else
    {*value = malloc(slen+1); 
     if(*value == NULL) 
        {FrError(3,"FrReadSChar","malloc failed"); 
         iFile->error = FR_ERROR_MALLOC_FAILED;
         return;} 
     if(slen != FrRead(iFile,(char *)  *value, slen)) 
        {FrError(3,"FrReadSChar","error reading string");
         iFile->error = FR_ERROR_READ_ERROR;
         return;}}

 return;}

/*--------------------------------------------------------------FrReadShort--*/
void FrReadShort(FrFile *iFile,
                 short *value)
/*---------------------------------------------------------------------------*/
{int i;
 unsigned char *buf, local[2];
 
 if(iFile->error != FR_OK) {return;}
 
 if(iFile->fmType == FRNATIVE)
       {i = FrRead(iFile,(char *)  value, FrSshort);}
 else 
       {i = FrRead(iFile,(char *) local, FrSshort);
        buf = (unsigned char *) value;
        buf[0] = local[1];
        buf[1] = local[0];}

 if(i == 0) 
    {FrError(3,"FrReadShort","end of file");
     iFile->error = FR_ERROR_READ_ERROR;} 

 return;}

/*-------------------------------------------------------------FrReadShortU--*/
void FrReadShortU(FrFile *iFile,
                  unsigned short *value)
/*---------------------------------------------------------------------------*/
{int i;
 unsigned char *buf, local[2];
 
 if(iFile->error != FR_OK) {return;}
 
 if(iFile->fmType == FRNATIVE)
       {i = FrRead(iFile,(char *)  value, FrSshort);}
 else 
       {i = FrRead(iFile,(char *) local, FrSshort);
        buf = (unsigned char *) value;
        buf[0] = local[1];
        buf[1] = local[0];}

 if(i == 0) 
    {FrError(3,"FrReadShortU","end of file");
     iFile->error = FR_ERROR_READ_ERROR;} 

 return;}
/*---------------------------------------------------------FrReadSkipRecord--*/
void FrReadSkipRecord(FrFile *iFile)
/*---------------------------------------------------------------------------*/
{unsigned int l1,l2;

 if(iFile->fmtVersion > 5)
      l1 = iFile->length-10;
 else l1 = iFile->length-6;

 while(l1 > 0)
      {l2 = FrBufSize;
       if(l2 > l1) {l2 = l1;}
       if(l2 != FrRead(iFile, FrBuf,  l2)) break;
       l1 = l1-l2;}
            
 return;}
 
/*------------------------------------------------------FrReadStructChksum---*/
void FrReadStructChksum(FrFile *iFile)
/*---------------------------------------------------------------------------*/
{
  unsigned int chksum=0;
  FRULONG nBytes;

  /*------------------- compute structure checksum and then read its value---*/

  nBytes = iFile->nBytes - iFile->nBytesF;
  if(iFile->chkSumFrFlag == FR_YES){
    FrCksumGnu( NULL, nBytes, &(iFile->chkSumFr));
    chksum = iFile->chkSumFr;}

  FrReadIntU(iFile, &iFile->chkSumFrRead);
    
  /*------------------- debug and then return if there is nothing to check---*/

  if(iFile->chkSumFrFlag  == FR_NO) {
    if(FrDebugLvl > 2) 
      fprintf(FrFOut," Not asked to check structure checksums");
    return;}

  if(iFile->chkTypeFrRead == FR_NO) {
    if(FrDebugLvl > 2) {
      fprintf(FrFOut," Structure checksum not available");}
    return;}

  /*------------------------------------------------------- check checksum---*/

  if(iFile->chkSumFrRead  != chksum) {
    if(iFile->sh[iFile->type] != NULL) {
      sprintf(FrErrMsg, "\nStructure Checksum error: read=%x computed=%x"
	      " file:%s structure: %s ", iFile->chkSumFrRead, chksum, 
	      iFile->current->fileName, iFile->sh[iFile->type]->name);}
    else {
      sprintf(FrErrMsg, "\nStructure Checksum error: read=%x computed=%x"
	      " file:%s structure type: %d ", iFile->chkSumFrRead, chksum, 
	      iFile->current->fileName, iFile->type);}
    FrError(3,"Checksum structure error", FrErrMsg);
    if(FrDebugLvl > 2) fprintf(FrFOut,"%s\n nBytes=%"FRLLD"\n",FrErrMsg, nBytes);
    iFile->error = FR_ERROR_CHECKSUM;}
  else {
    if(FrDebugLvl > 2) fprintf(FrFOut," Checksum OK (%x)", chksum);}

  /*------------------------------------------ reset checksum computation ---*/

  iFile->nBytesF = iFile->nBytes;
  iFile->chkSumFr = 0.;

  return;}

/*-------------------------------------------------------------FrReadStruct--*/
void FrReadStruct(FrFile *iFile,
                  void *address)
/*---------------------------------------------------------------------------*/
{unsigned short type, instance2;

 if(iFile->error != FR_OK) {return;}
 
 FrReadShortU(iFile, &type); 
 if(iFile->fmtVersion > 5)
      {FrReadIntU  (iFile, &(iFile->instance));}
 else {FrReadShortU(iFile, &instance2);
       iFile->instance = instance2;}

 if(type > 3000) {type = 0;} /*--- this is for backward compatibility (3.10)*/

 if(iFile->error != FR_OK) {return;}
 if(type != 0) 
      {FrSetBRT(iFile, iFile->instance, type, address);} 
 else {address = NULL;}

 return;}
  
/*-------------------------------------------------------FrReadStructHeader--*/
void FrReadStructHeader(FrFile *iFile)
/*---------------------------------------------------------------------------*/
{
  unsigned char value;
  unsigned int length;

  if(iFile == NULL) return;

  if(iFile->fmtVersion >= 8) { /*----reset structure checksum computation---*/
    iFile->nBytesF = iFile->nBytes;
    iFile->chkSumFr = 0.;}

  if(iFile->fmtVersion > 5) { /*------------------- read structure length---*/
    FrReadLong(iFile, &iFile->length);}
  else {
    FrReadIntU(iFile, &length);
    iFile->length = length;}

  if(iFile->fmtVersion < 8) { /*- read struct type and checksum type if v8--*/
    FrReadShortU(iFile,&iFile->type);}
  else {
    FrRead(iFile, (char *) &value, 1);
    iFile->chkTypeFrRead = value;
    if(FrRead(iFile, (char *) &value, 1) == 0) {
      FrError(3,"FrReadStructHeader","end of file");
      iFile->error = FR_ERROR_READ_ERROR;}
    iFile->type = value;}
 
  return;}

/*-----------------------------------------------------------------FrReadVC--*/
void FrReadVC(FrFile *iFile,
              char **data, 
              FRULONG nData)                
/*---------------------------------------------------------------------------*/
{FRULONG nread;
 char *buf;
 
 if(iFile->error != FR_OK) {return;}
 
 buf = (char *) malloc(nData*sizeof(char));
 *data = buf;
 if(buf == NULL) 
        {FrError(3,"FrReadVC","malloc failed");
         iFile->error = FR_ERROR_MALLOC_FAILED;
         return;} 
 
 nread = FrRead(iFile,(char *) buf, nData);
 if(nread != nData)
        {FrError(3,"FrReadVC","error reading bank");
         iFile->error = FR_ERROR_READ_ERROR;}
          
 return;}
 
/*-----------------------------------------------------------------FrReadVD--*/
void FrReadVD(FrFile *iFile,
              double **data,
              FRULONG nData)
/*---------------------------------------------------------------------------*/
{FRULONG i,nread;
 double *buf;
 unsigned char *swap,local[4];

 if(iFile->error != FR_OK) {return;}
 
 buf = (double *) malloc(nData*sizeof(double));
 *data = buf;
 if(buf == NULL) 
      {FrError(3,"FrReadVD","malloc failed");
       iFile->error = FR_ERROR_MALLOC_FAILED;
       return;} 
 
 nread = FrRead(iFile,(char *) buf, sizeof(double)*nData);
 if(nread != nData*sizeof(double))
      {FrError(3,"FrReadVD","error reading bank");
       iFile->error = FR_ERROR_READ_ERROR;
       return;} 
   
 if(iFile->fmType != FRNATIVE)
      {swap = (unsigned char *) buf;
       for(i=0; i<nData; i++)
          {local[0] = swap[7];
           local[1] = swap[6];
           local[2] = swap[5];
           local[3] = swap[4];
           swap[7]  = swap[0];
           swap[6]  = swap[1];
           swap[5]  = swap[2];
           swap[4]  = swap[3];
           swap[3]  = local[3];
           swap[2]  = local[2];
           swap[1]  = local[1];
           swap[0]  = local[0];
           swap = swap + 8;}}

 return;}

/*-----------------------------------------------------------------FrReadVF--*/
void FrReadVF(FrFile *iFile,
              float **data,
              FRULONG nData)
/*---------------------------------------------------------------------------*/
{FRULONG i,nread;
 float *buf;
 unsigned char *swap,local[2];
 
 if(iFile->error != FR_OK) {return;}
 
 buf = (float *) malloc(nData*sizeof(float));
 *data = buf;
 if(buf == NULL) 
      {FrError(3,"FrReadVF","malloc failed");
       iFile->error = FR_ERROR_MALLOC_FAILED;
       return;}               
 
 nread = FrRead(iFile,(char *) buf, sizeof(float)*nData);
 if(nread != sizeof(float)*nData)
      {FrError(3,"FrReadVF","error reading bank");
       iFile->error = FR_ERROR_READ_ERROR;
       return;}               

 if(iFile->fmType != FRNATIVE)
      {swap = (unsigned char *) buf;
       for(i=0; i<nData; i++)
          {local[0] = swap[3];
           local[1] = swap[2];
           swap[3]  = swap[0];
           swap[2]  = swap[1];
           swap[1]  = local[1];
           swap[0]  = local[0];
           swap = swap + 4;}}
        
 return;}
/*----------------------------------------------------------------FrReadVFD--*/
void FrReadVFD(FrFile *iFile,
               double **data,
               FRULONG nData)
/*---------------------------------------------------------------------------*/
{FRULONG i;
 float  *dataF;
 double *dataD;
 
 FrReadVF(iFile, &dataF, nData);
 if(dataF == NULL)
   {*data = NULL;
     return;}

 dataD = (double *) malloc(nData*sizeof(double));
 *data = dataD;
 if(dataD == NULL) 
      {FrError(3,"FrReadVFD","malloc failed");
       iFile->error = FR_ERROR_MALLOC_FAILED;
       return;}               

 for(i=0; i<nData; i++)
   {dataD[i] = dataF[i];}

 free(dataF);
         
 return;}
 
/*-----------------------------------------------------------------FrReadVI--*/
void FrReadVI(FrFile *iFile,
              int **data,
              FRULONG nData)
/*---------------------------------------------------------------------------*/
{FRULONG i,nread;
 int *buf;
 unsigned char *swap,local;
 
 if(iFile->error != FR_OK) {return;}
 
 buf = (int *) malloc(nData*sizeof(int));
 *data = buf;
 if(buf == NULL) 
        {FrError(3,"FrReadVI","malloc failed");
         iFile->error = FR_ERROR_MALLOC_FAILED;
         return;}               
 
 nread = FrRead(iFile,(char *) buf, sizeof(int)*nData);
 if(nread != sizeof(int)*nData)
       {FrError(3,"FrReadVI","error reading bank");
        iFile->error = FR_ERROR_READ_ERROR;
        return;}               
 
 if(iFile->fmType != FRNATIVE)
      {swap = (unsigned char *) buf;
       for(i=0; i<nData; i++)
          {local   = swap[3];
           swap[3] = swap[0];
           swap[0] = local;
           local   = swap[2];
           swap[2] = swap[1];
           swap[1] = local;
           swap = swap + 4;}}
           
 return;}

/*-----------------------------------------------------------------FrReadVL--*/
void FrReadVL(FrFile *iFile,
              FRLONG **data,
              FRULONG nData)
/*---------------------------------------------------------------------------*/
{FRULONG i,nread, nBytes;
 FRLONG *buf, *out;
 unsigned char *swap, *local;
 
 if(iFile->error != FR_OK) {return;}
 
 nBytes = nData * iFile->header[9];  
 buf = (FRLONG *) malloc(nBytes);
 if(buf == NULL) {FrError(3,"FrReadVL","malloc failed");
                  iFile->error = FR_ERROR_MALLOC_FAILED;
                  return;}             

 nread = FrRead(iFile,(char *) buf, nBytes);
 if(nread != nBytes)
              {FrError(3,"FrReadVL","error reading bank");
               iFile->error = FR_ERROR_READ_ERROR;
               return;}
  
 if(iFile->fmLong == FRNATIVE)
    {*data = buf;
     return;}
                                  /*--------------- special cases -----------*/

 local = (unsigned char *) buf;
 swap  = (unsigned char *) malloc(nData*sizeof(FRLONG));
 if(swap == NULL) {FrError(3,"FrReadVL","malloc failed");
                  iFile->error = FR_ERROR_MALLOC_FAILED;
                  return;}             
 *data = (FRLONG *) swap;

 if((iFile->fmLong == FRSWAP) && (FrSlong == 8))
   {for(i=0; i<nData; i++)
          {swap[7+i*8]  = local[  i*8];
           swap[6+i*8]  = local[1+i*8];
           swap[5+i*8]  = local[2+i*8];
           swap[4+i*8]  = local[3+i*8];
           swap[3+i*8]  = local[4+i*8];
           swap[2+i*8]  = local[5+i*8];
           swap[1+i*8]  = local[6+i*8];
           swap[  i*8]  = local[7+i*8];}}
 else if((iFile->fmLong == FRSWAP) && (FrSlong == 4))
   {for(i=0; i<nData; i++)
          {swap[3+i*4]  = local[  i*4];
           swap[2+i*4]  = local[1+i*4];
           swap[1+i*4]  = local[2+i*4];
           swap[  i*4]  = local[3+i*4];}}
 else if(iFile->fmLong == FREXPAND)
    {for(i=0; i<nData; i++)
          {swap[7+i*8]  = 0;
           swap[6+i*8]  = 0;
           swap[5+i*8]  = 0;
           swap[4+i*8]  = 0;
           swap[3+i*8]  = local[3+i*4];
           swap[2+i*8]  = local[2+i*4];
           swap[1+i*8]  = local[1+i*4];
           swap[  i*8]  = local[  i*4];}}
 else if(iFile->fmLong == FREXPAND_AND_SWAP)
    {for(i=0; i<nData; i++)
          {swap[7+i*8]  = 0;
           swap[6+i*8]  = 0;
           swap[5+i*8]  = 0;
           swap[4+i*8]  = 0;
           swap[3+i*8]  = local[  i*4];
           swap[2+i*8]  = local[1+i*4];
           swap[1+i*8]  = local[2+i*4];
           swap[  i*8]  = local[3+i*4];}}
 else if(iFile->fmLong == FRCONTRACT)
    {out = *data;
     for(i=0; i<nData; i++)
          {out[i] = 0;
           if(buf[2*i] == 0)
                out[i] = buf[2*i+1];
           else out[i] = buf[2*i];}}
 else if(iFile->fmLong == FRCONTRACT_AND_SWAP)
    {out = *data;
     for(i=0; i<nData; i++)
          {if(buf[2*i] == 0)
             {swap[  i*4]  = local[7+i*8];
              swap[1+i*4]  = local[6+i*8];
              swap[2+i*4]  = local[5+i*8];
              swap[3+i*4]  = local[4+i*8];}
           else
             {swap[  i*4]  = local[3+i*8];
              swap[1+i*4]  = local[2+i*8];
              swap[2+i*4]  = local[1+i*8];
              swap[3+i*4]  = local[  i*8];}}}
 
 free(local);

 return;}

/*-----------------------------------------------------------------FrReadVQ--*/
void FrReadVQ(FrFile *iFile, 
              char  ***data,
              FRULONG nData)
/*---------------------------------------------------------------------------*/
{FRULONG i;
 char **buf;
 
 if(iFile->error != FR_OK) {return;}
 
 buf = (char **) malloc(nData*sizeof(char *));
 *data = buf; 
 if(buf == NULL) 
        {FrError(3,"FrReadVQ","malloc failed");
         iFile->error = FR_ERROR_MALLOC_FAILED;
          return;}               
  
 for(i=0; i<nData; i++)
        {FrReadSChar(iFile, &buf[i]);}

 return;}

/*-----------------------------------------------------------------FrReadVS--*/
void FrReadVS(FrFile *iFile,
              short **data,
              FRULONG nData)
/*---------------------------------------------------------------------------*/
{FRULONG i,nread;
 short *buf;
 unsigned char *swap,local;
 
 if(iFile->error != FR_OK) {return;}
 
 buf = (short *) malloc(nData*sizeof(short));
 *data = buf;
 if(buf == NULL) 
     {FrError(3,"FrReadVS","malloc failed");
      iFile->error = FR_ERROR_MALLOC_FAILED;
      return;}               

 nread = FrRead(iFile,(char *) buf, sizeof(short)*nData);
 if(nread != sizeof(short)*nData)
     {FrError(3,"FrReadVS","error reading bank");
      iFile->error = FR_ERROR_READ_ERROR;
      return;}               
 
 if(iFile->fmType != FRNATIVE)
     {swap = (unsigned char *) buf;
      for(i=0; i<nData; i++)
          {local   = swap[1];
           swap[1] = swap[0];
           swap[0] = local;
           swap = swap + 2;}}
           
 return;}

/*------------------------------------------------------------------FrSEDef--*/
FrSH *FrSEDef()
/*---------------------------------------------------------------------------*/
{static FrSH *classe = NULL;

  if(classe != NULL) return(classe);
  classe = FrDicAddS("FrSE",FrSERead);

  FrSENew(classe, "STRING", "name","-");
  FrSENew(classe, "STRING", "type","-");
  FrSENew(classe, "STRING", "comment","-");

  return(classe);}

/*------------------------------------------------------------------ FrSENew-*/
void FrSENew(FrSH *structH,
             char *type,
             char  *name,
             char  *comment)
/*---------------------------------------------------------------------------*/
{FrSE *structE, *last;
 int i;
 char buf[256];
 static char *dataTypes[] = 
   {"UNKNOWN","CHAR","CHAR_U","INT_2S","INT_2U",
    "INT_4S","INT_4U","INT_8S","INT_8U","REAL_4","REAL_8",
    "STRING","PTR_STRUCT","COMPLEX_8","COMPLEX_16","*","*INT_4U",
    "*REAL_8","*STRING", "*INT_8U","*INT_2U", "*INT_4S",
    "CHAR[2]","REAL_4[nParam]","STRING[nParam]","REAL_8[nAuxParam]",
    "STRING[nAuxParam]","*REAL_4", "STRING[nDim]", "REAL_8[nDim]",
    "INT_4U[nDim]", "CHAR[nBytes]", "STRING[nColumn]", "INT_8U[nDim]",
    "REAL_8[nParam]",
    "INT_4U[nFrame]","REAL_8[nFrame]","INT_4S[nFrame]","INT_8U[nFrame]",
    "INT_2U[nSH]", "STRING[nSH]",
    "STRING[nDetector]", "INT_8U[nDetector]",
    "INT_4U[nStatInstance]", "INT_8U[nStatInstance]",
    "STRING[nStatType]", "INT_4U[nStatType]",
    "INT_8U[nTotalStat]", "INT_4U[nTotalStat]",
    "STRING[nADC]","INT_4U[nADC]","INT_8U[nADC][nFrame]",
    "STRING[nProc]","INT_8U[nProc][nFrame]",
    "STRING[nSim]", "INT_8U[nSim][nFrame]",
    "STRING[nSer]", "INT_8U[nSer][nFrame]",
    "STRING[nSummary]", "INT_8U[nSummary][nFrame]",
    "STRING[nEventType]", "INT_4U[nEventType]", "INT_4U[nTotalEvent]",
    "REAL_4[nTotalEvent]", "INT_8U[nTotalEvent]",
    "STRING[nSimEventType]", "INT_4U[nSimEventType]","INT_4U[nTotalSEvent]",
    "INT_4U[nTotalSEvent]","REAL_4[nTotalSEvent]","INT_8U[nTotalSEvent]",
    "LAST"};

                  /*------------------------check the type validity ---------*/

  if(type == NULL) type = dataTypes[0];

  for(i=0;; i++)
    {if(strcmp (type,dataTypes[i])    == 0) break;
     if(strncmp(type,"PTR_STRUCT",10) == 0) break;
     if(strcmp(dataTypes[i],"LAST")   != 0) continue;
     sprintf(buf,"For %s.%s unknown type:%s\n", structH->name,name,type);
     FrError(3,"FrSENew",buf);
     return;}
                 /*--------------------------------- fill structure --------*/

  structE = (FrSE *) calloc(1,sizeof(FrSE));
  if(structE == NULL)   return;
  structE->classe = FrSEDef();

  if(FrStrCpy(&structE->type    ,type)    == NULL) return;
  if(FrStrCpy(&structE->name    ,name)    == NULL) return;
  if(comment == NULL)
    {structE->comment = NULL;}
  else
    {if(FrStrCpy(&structE->comment ,comment) == NULL) return;}

  structE->next = NULL;
  
  if(structH->firstE == NULL)
    {structH->firstE = structE;}
  else
   {last = structH->firstE;
    while( last->next != NULL ) {last = last->next;}
    last->next = structE;}

  structH->nSE++;

  return;}

/*-----------------------------------------------------------------FrSERead--*/
void FrSERead(FrFile *iFile)
/*---------------------------------------------------------------------------*/
{FrSE *se, *last;
 short id2;
 int id;

  se = (FrSE *) calloc(1,sizeof(FrSE));
  if(se == NULL) 
       {iFile->error = FR_ERROR_MALLOC_FAILED;
        return;}
  se->classe = FrSEDef();

  if(iFile->fmtVersion > 5) 
       {FrReadInt(iFile, &id);}
  else {FrReadShort(iFile, &id2);
        id = id2;}
  FrReadSChar(iFile, &se->name); 
  FrReadSChar(iFile, &se->type);
  FrReadSChar(iFile, &se->comment);
  if(iFile->fmtVersion >=8) FrReadStructChksum(iFile);

  if(FrDebugLvl > 2) 
    {fprintf(FrFOut,"  FrSERead: %16s",se->name);
     if(se->type    != NULL) fprintf(FrFOut," (%s)", se->type);
     if(se->comment != NULL) fprintf(FrFOut," %s"  , se->comment);
     fprintf(FrFOut,"\n");}

                            /*--- in the case of file mark, do not use SH ---*/

  if(iFile->curSH == NULL) 
    {if(se->name    != NULL) free(se->name);
     if(se->type    != NULL) free(se->type);
     if(se->comment != NULL) free(se->comment);
     free(se);
     return;}

                           /*----------- store the SH structure -------------*/

  if(iFile->curSH->firstE == NULL) 
       {iFile->curSH->firstE = se;}
  else {last = iFile->curSH->firstE;
        while(last->next != NULL) {last = last->next;}
        last->next = se;}

  iFile->curSH->nSE++;

return;}
 
/*----------------------------------------------------------------FrSEWrite--*/
void FrSEWrite(FrSE *structE,
               FrFile *oFile)
/*---------------------------------------------------------------------------*/
{
 FrPutNewRecord(oFile, structE, FR_NO);
 FrPutSChar(oFile,structE->name);
 FrPutSChar(oFile,structE->type);
 FrPutSChar(oFile,structE->comment);
 FrPutWriteRecord(oFile, FR_NO);

 return;} 

/*-----------------------------------------------------------------FrSetAll--*/
void FrSetAll(FrFile *file)                         /* set all banks address */
/*---------------------------------------------------------------------------*/
{int i, nLeft;
 
 nLeft = 0;
 for(i=0; i<file->nRef; i++)
   {if((file->refType[i] != -1) && 
       (file->refType[i] != file->detectorType)) nLeft++;}
 
 if(nLeft != 0) FrError(3,"FrSetAll","Some reference not used");

 return;}
 
/*-----------------------------------------------------------------FrSetBAT--*/
void FrSetBAT(FrFile *file, 
              unsigned int instance,
              void *address)                     /* store address for a bank */
/*---------------------------------------------------------------------------*/
{int i;
 void **reference;
                                             /*------ if using mark ---------*/
 if(file->relocation == FR_NO) return;

 for(i=0; i<file->nRef; i++)
   {if((file->refType[i]  == file->type) &&
       (file->refInstance[i] == instance))
     {reference = file->refAddress[i];
      *reference = address;
      if(file->type != file->detectorType) 
          {file->refType[i] = -1;}
      else{file->address[i]= address;}
      return;}}

  FrError(3,"FrSetBAT","bank not referencedd");
 
  return;}

/*-----------------------------------------------------------------FrSetBRT--*/
void FrSetBRT(FrFile *file,
              unsigned int instance,
              unsigned short type, 
              void *address)                 
/*---------------------------------------------------------------------------*/
{int i;
 void **reference;

 if(file->relocation == FR_NO)  return;

 if(FrDebugLvl > 3) {fprintf(FrFOut," FrSetBRT: instance=%d type=%d\n",
        instance, type);}
 
    /*------- for FrDetector, search if the structure is already declared ---*/

 if(type == file->detectorType)
   {for(i=0; i<file->nRef; i++)
      {if((file->refType[i]  == type) && (file->refInstance[i] == instance))
        {reference = address;
         *reference = file->address[i];
         return;}}}

     /*----------- regular case: store directly the information -------------*/

 for(i=0; i<file->nRef; i++)
   {if(file->refType[i]  == -1) break;}

 if(i == file->nRef) 
    {if(file->nRef > FRMAXREF) 
       {FrError(3,"FrSetBRT", "FRMAXREF need to be increased");
        return;}
     file->nRef++;}

 file->refType[i]     = type;
 file->refInstance[i] = instance;
 file->refAddress[i]  = address;

 return;}

/*---------------------------------------------------------------- FrSetIni--*/
void FrSetIni(FrFile *file)           /* init the address relocation package */
/*---------------------------------------------------------------------------*/
{
 file->nRef = 0;
 
 file->relocation = FR_YES;      

 return;}

/*------------------------------------------------------------------FrSHDef--*/
FrSH *FrSHDef()
/*---------------------------------------------------------------------------*/
{static FrSH *classe = NULL;

  if(classe != NULL) return(classe);
  classe = FrDicAddS("FrSH",FrSHRead);

  FrSENew(classe, "STRING", "name","-");
  FrSENew(classe, "INT_2U", "classe","-");
  FrSENew(classe, "STRING", "comment","-");

  return(classe);}

/*-------------------------------------------------------------- FrSHMatch---*/
void FrSHMatch(FrFile *iFile)
/*---------------------------------------------------------------------------*/
{FrSH *sh, *shRef;
 FrSE *se, *seRef;
 int i;
 unsigned short id;
 char buf[512];

 id = iFile->type;
 sh = iFile->sh[id]; 
 if(sh == NULL) return;

 if(iFile->fmtVersion <= 5)
   {if(strcmp(sh->name,"FrTrigData") == 0) FrStrCpy(&(sh->name),"FrEvent");}

                                   /*------ search the reference shRef -----*/
 shRef = NULL; 
 for(i=0; i<FrnSH; i++)
   {shRef = FrSHRoot[i];
    if(shRef->name == NULL) continue;
    if(strcmp(sh->name,shRef->name) == 0) {break;}}
 if(i == FrnSH) {shRef = NULL;}
            
 if(shRef == NULL)
      {sh->objRead = NULL;
       if(FrDebugLvl > 0) fprintf(FrFOut,
           "FrSHMatch: unknown bank:%s id=%d\n",sh->name,id);
       return;}
       
 sh->objRead = shRef->objRead;
                                   /*--------- check for the data type ------*/

 if(iFile->fmtVersion != FrFormatVersion) return;  /*----do a check only for 
                                          the current version----------------*/
 if(sh->nSE != shRef->nSE)
      {sprintf(buf,"FrSHMatch: the number of element has changed" 
             " for:%s (id=%d) (%d %d)\n",sh->name,id,sh->nSE,shRef->nSE);
       FrError(3,"FrSHMatch",buf);}
       
 se    = sh->firstE;
 seRef = shRef->firstE;
 while(se != NULL && seRef != NULL)
   {if((strcmp(se->name,seRef->name) != 0) && (FrDebugLvl > 3)) 
      {sprintf(buf,"FrSHMatch: Element name changed for: %s (id=%d)(%s %s)\n"
                   "    file type= %s FrameLib type=%s\n",
                 sh->name,id,se->name,seRef->name,se->type,seRef->type);
       FrError(3,"FrSHMatch",buf);}
    if((strcmp(se->type,seRef->type) != 0) && (FrDebugLvl > 3))
      {sprintf(buf,"FrSHMatch: Element type changed for: %s->%s:%s (ref=%s)\n",
                sh->name,se->name,se->type,seRef->type);
       FrError(3,"FrSHMatch",buf);}
    se    = se->next;
    seRef = seRef->next;}
              
 return;}

/*------------------------------------------------------------------FrSHNew--*/
FrSH *FrSHNew(char *name,
              char *comment)
/*---------------------------------------------------------------------------*/
{FrSH *sh;
 static int id = 1;

  sh = (FrSH *) calloc(1,sizeof(FrSH));
  if(sh == NULL)   {return(NULL);}

  if(name == NULL)                  {sh->classe = FrSHDef();}
  else if(strcmp(name,"FrSH") == 0) {sh->classe = sh;}
  else                              {sh->classe = FrSHDef();}

  if(name != NULL) 
    {if(FrStrCpy(&sh->name, name)       == NULL) return(NULL);}
  if(comment != NULL)
    {if(FrStrCpy(&sh->comment, comment) == NULL) return(NULL);}

  sh->objRead = FrReadRecord;
  sh->firstE = NULL;
  sh->id = id;
  id++;
  
  return(sh);}

/*-----------------------------------------------------------------FrSHRead--*/
void FrSHRead(FrFile *iFile)
/*---------------------------------------------------------------------------*/
{FrSH *sh, *shDup;
 unsigned short id,i, id2;
 unsigned int id0;
 char *name, *comment;

 if(iFile->error != FR_OK) return;

 if(iFile->fmtVersion > 5) 
      {FrReadIntU  (iFile, &id0);}
 else {FrReadShortU(iFile, &id2);
       id0 = id2;}
 FrReadSChar (iFile, &name); 
 FrReadShortU(iFile, &id); 
 FrReadSChar (iFile, &comment); 
 if(iFile->fmtVersion >=8) FrReadStructChksum(iFile);

                                  /*--- check for existing sh structure ----*/       
 for(i=0; i<iFile->maxSH; i++)
    {shDup = iFile->sh[i];
     if(shDup == NULL)       continue;
     if(shDup->name == NULL) continue;
     if(strcmp(name,shDup->name) == 0) 
       {if(id != shDup->id)
            {FrError(3,"FrSHRead","Duplicate FrSH with different id");}
        if(name    != NULL) free(name);
        if(comment != NULL) free(comment);
        iFile->curSH = NULL;
        return;}}
                                    /*----------- add it in the list --------*/
 if(id >= iFile->maxSH) 
      {FrFileIncSH(iFile, id);
       if(iFile->error != FR_OK) return;}

 sh = iFile->sh[id];
 if(sh == NULL)
   {sh = FrSHNew(name,comment);
    sh->id = id;
    iFile->sh[id] = sh;
    if(name    != NULL) free(name);
    if(comment != NULL) free(comment);}
 else
   {sh->name = name;
    sh->comment = comment;}

 iFile->curSH = sh;

 if(FrDebugLvl > 2) fprintf(FrFOut," %s\n", sh->name);

 return;}
 
/*----------------------------------------------------------------FrSHWrite--*/
void FrSHWrite(FrSH *structH,
               FrFile *oFile)
/*---------------------------------------------------------------------------*/
{FrSE *structE;

 FrPutNewRecord(oFile, structH, FR_NO);
 FrPutSChar    (oFile, structH->name);
 FrPutShortU   (oFile, structH->id);
 FrPutSChar    (oFile, structH->comment);
 FrTOCSH       (oFile, structH->name, structH->id);
 FrPutWriteRecord(oFile, FR_NO);

     /*--- write FrSE structure ------*/
	 
 structE = structH->firstE;
 while(structE != NULL )
       {FrSEWrite(structE, oFile);
        structE = structE->next;}
		 
 return;}

/*-------------------------------------------------------------FrSegListAdd--*/
int FrSegListAdd(FrSegList *segList, double tStart, double tEnd)
/*---------------------------------------------------------------------------*/
{ 
  if(segList == NULL) return(-1);
  if(tStart > tEnd)   return(-2);
  FrVectFillD(segList->tStart, tStart);
  FrVectFillD(segList->tEnd,   tEnd);
  return(0);
}
/*-------------------------------------------------------FrSegListBuildVect--*/
FrVect* FrSegListBuildVect(FrSegList *segList, 
                           double tStart, 
                           double length,
		           int nData)
/*---------------------------------------------------------------------------*/
{FrVect *vect;
  double dt;
  int i;

  if(segList == NULL) return(NULL);
  if(nData <= 0) return(NULL);
  dt = length/nData;

  vect = FrVectNew1D("Segment",FR_VECT_8R,nData,dt,"time (s)","1 means OK");
  if(vect == NULL) return(NULL);

  vect->GTime = tStart;
  for(i=0; i<nData; i++)
    {vect->dataD[i]= FrSegListCoverage(segList, tStart + i*dt, dt);}

  return(vect);
}
/*--------------------------------------------------------FrSegListCoverage--*/
double FrSegListCoverage(FrSegList *segList, double gtime, double dt)
/*---------------------------------------------------------------------------*/
/* This function compute the intersection between the segment list and the
   time interval. It returns a number from 0 to 1 (1 = 100% coverage).       */
/*---------------------------------------------------------------------------*/
{int i, iS, iE;
  double totTime, tStart, tEnd, length, use;

 if(segList == NULL) return(-1);

 iS = FrSegListFind(segList, gtime +      1.e-6);
 iE = FrSegListFind(segList, gtime + dt - 1.e-6);

 if(iS == iE)
   {if(iS >= 0) return(1.);
    else        return(0.);}

 totTime = 0;

 if(iS < 0) iS = -iS-1;
 if(iE < 0) iE = -iE-2;
 iE = iE + 1;
 if(iE > segList->tEnd->nData) iE = segList->tEnd->nData;

 for(i=iS; i<iE; i++)
    {tStart = segList->tStart->dataD[i];
     tEnd = segList->tEnd->dataD[i];
     if(tStart < gtime) tStart = gtime;
     if(tEnd > gtime+dt) tEnd = gtime+dt;
     length = tEnd - tStart;
     totTime += length;}
  
 use = totTime/dt;

 return(use);}

/*------------------------------------------------------------FrSegListDump--*/
int FrSegListDump(FrSegList *segList, FILE *fp, int debugLvl)
/*---------------------------------------------------------------------------*/
{int i, nData;
  double length, min, max, totTime, start, end, use;

  if(segList == NULL) return(-1);

  min = 1.e37;
  max = 0;
  totTime = 0;

  nData = segList->tStart->nData;
  for(i=0; i<nData; i++)
    {length = segList->tEnd->dataD[i]-segList->tStart->dataD[i];
      totTime += length;
     if(length > max) max = length;
     if(length < min) min = length;}
  
  fprintf(fp,"%8d segments;", nData);
  if(nData > 0) 
     {start = segList->tStart->dataD[0];
      end   = segList->tEnd->dataD[nData-1];
      fprintf(fp," start:%9.0f end:%9.0f",start, end);
      use = 100.*totTime/(end-start);}
  else {use = 0;}
  fprintf(fp," total time:%.3g (%.3g%%) for %s\n", totTime, use, segList->name);

  return(0);
}
/*------------------------------------------------------------FrSegListFind--*/
int FrSegListFind(FrSegList *segList, double gtime)
/*---------------------------------------------------------------------------*/
/* This function returns the segment index for gtime or a negative value if
   it is between our outside the segment list:

Segement:           |----------|       |--------|        |------|
Returned value: -1        0         -2      1       -3       2      -4       */
/*---------------------------------------------------------------------------*/
{int i, delta, nSegments;
  double *tStart, *tEnd;

 if(segList == NULL) return(-1);

 tStart = segList->tStart->dataD;
 tEnd   = segList->tEnd->dataD;
 nSegments = segList->tStart->nData;

 if(nSegments == 0)               return(-1);
 if(gtime < tStart[0])            return(-1);
 if(gtime >=   tEnd[nSegments-1]) return(-nSegments-1);
 if(gtime >= tStart[nSegments-1]) return(nSegments-1);

 delta = nSegments;
 i = delta/2;

 while(delta > 0)
   {if(i < 0) i = 0;
    if(i > nSegments-2) i = nSegments - 2;
    delta = delta-delta/2;
    
    if     (gtime < tStart[i]) {i = i - delta;}
    else if(gtime < tEnd[i])     {return(i);}
    else if(gtime < tStart[i+1]) {return(-i-2);}
    else {i = i + delta;}}

 return(-3);}

/*-------------------------------------------------------FrSegListFindFirst--*/
int FrSegListFindFirst(FrSegList *segList, double gtime, double dt)
/*---------------------------------------------------------------------------*/
/* This function check if there is a match between the segment list and the 
   time interval. It returns the id of the first segment or a negative value */
/*---------------------------------------------------------------------------*/
{int i, j;

 if(segList == NULL) return(-1);
 
 i = FrSegListFind(segList, gtime+1.e-7);
 if(i>= 0) return(i);

 j = FrSegListFind(segList, gtime+dt-1.e-7);
 if(i == j) return(-2);
 
 return(-i-1);}

/*------------------------------------------------------FrSegListFindFirstT--*/
double FrSegListFindFirstT(FrSegList *segList, double gtime, double dt)
/*---------------------------------------------------------------------------*/
/* This function returns the first time for the interesction between the list
   of segments and the time interval. It returns 0 if not found              */
/*---------------------------------------------------------------------------*/
{double tStart;
  int i;

 if(segList == NULL) return(0);

 i = FrSegListFindFirst(segList, gtime, dt);
 if(i < 0) return(0);

 tStart = segList->tStart->dataD[i];
 if(gtime > tStart) tStart = gtime;

 return(tStart);}

/*-------------------------------------------------------FrSegListFindLast--*/
int FrSegListFindLast(FrSegList *segList, double gtime, double dt)
/*---------------------------------------------------------------------------*/
/* This function check if there is a match between the segment list and the 
   time interval. It returns the id of the last segment or a negative value */
/*---------------------------------------------------------------------------*/
{int i, j;

 if(segList == NULL) return(-1);
 
 i = FrSegListFind(segList, gtime+dt-1.e-7);
 if(i>= 0) return(i);

 j = FrSegListFind(segList, gtime+1.e-7);
 if(i == j) return(-2);
 
 return(-i-2);}
/*-------------------------------------------------------FrSegListFindLstT--*/
double FrSegListFindLastT(FrSegList *segList, double gtime, double dt)
/*---------------------------------------------------------------------------*/
/* This function returns the last time for the interesction between the list
   of segments and the time interval. It returns 0 if not found              */
/*---------------------------------------------------------------------------*/
{double tEnd;
 int i;

 if(segList == NULL) return(0); 

 i = FrSegListFindLast(segList, gtime, dt);
 if(i < 0) return(0);

 tEnd = segList->tEnd->dataD[i];
 if(gtime+dt < tEnd) tEnd = gtime+dt;

 return(tEnd);}

/*------------------------------------------------------------FrSegListFree--*/
void FrSegListFree(FrSegList *segList)
/*---------------------------------------------------------------------------*/
{
 if(segList == NULL) return;
 if(segList->next != NULL) FrSegListFree(segList->next);

 FrVectFree(segList->tStart);
 FrVectFree(segList->tEnd);
 if(segList->name != NULL) free(segList->name);
 free(segList);

 return;}
/*-------------------------------------------------------FrSegListIntersect--*/
FrSegList* FrSegListIntersect(FrSegList *list1, FrSegList* list2)
/*---------------------------------------------------------------------------*/
{FrSegList *list;
  int i,j;
  double *tStart1, *tStart2, *tEnd1, *tEnd2;

  if(list1 == NULL) return(NULL);
  if(list2 == NULL) return(NULL);

  list = FrSegListNew("intersect");
     
  tStart1 = list1->tStart->dataD;
  tStart2 = list2->tStart->dataD;
  tEnd1  = list1->tEnd->dataD;
  tEnd2  = list2->tEnd->dataD;

  i = 0;
  j = 0;
  while(i<list1->tStart->nData &&
        j<list2->tStart->nData)
    {if(tEnd1[i] < tStart2[j])        /*---case: seg 1 end before seg 2 start*/
       {i++;}                         /*---case: 1111 22222 */
      else if(tEnd1[i] < tEnd2[j])    /*---case: seg 1 end before seg 2 end */
	{if(tStart1[i] < tStart2[j])  /*---case: 1111***222 */
	    {FrSegListAdd(list, tStart2[j], tEnd1[i]);
	      i++;}
	  else                        /*---case: 222****222 */
	    {FrSegListAdd(list, tStart1[i], tEnd1[i]);
	      i++;}}
      else                            /*---case: seg 1 end after seg 2*/
	{if(tStart1[i] < tStart2[j])    /*---case: 1111*****1111 */
	    {FrSegListAdd(list,tStart2[j],tEnd2[j]);
	      j++;}
	  else if(tStart1[i] < tEnd2[j]) /*---case: 222*****1111 */
	    {FrSegListAdd(list,tStart1[i],tEnd2[j]);
	      j++;}
	  else {j++;}}}                 /*---case: 2222  1111*/

  return(list);
}
/*-------------------------------------------------------------FrSegListNew--*/
FrSegList *FrSegListNew(char *name)
/*---------------------------------------------------------------------------*/
{FrSegList *segList;
 
 segList = (FrSegList *) calloc(1,sizeof(FrSegList));
 if(segList == NULL)   return(NULL);
 
 FrStrCpy(&(segList->name), name);

 segList->tStart = FrVectNew1D("start time",FR_VECT_8R,0,1.,NULL,"GPS time");
 segList->tEnd   = FrVectNew1D("start time",FR_VECT_8R,0,1.,NULL,"GPS time");
 if(segList->tEnd == NULL) return(NULL);

 return(segList);}

/*-----------------------------------------------------------FrSegListPrint--*/
int FrSegListPrint(FrSegList *segList, char *fileName)
/*---------------------------------------------------------------------------*/
{FILE *fp;
  int i, dec, nDigits;
  double *tStart, *tEnd;

  if(segList == NULL) return(-1);
  if(fileName == NULL) return(-1);

  /*---- get the needed resolution-----*/

  tStart = segList->tStart->dataD;
  tEnd   = segList->tEnd->dataD;

  nDigits = 0;
  for(i=0; i<segList->tStart->nData; i++)
    {dec = (int) 1.e6*(tStart[i]-((int)tStart[i]));
     if     (dec%1000      != 0) nDigits = 6;
     else if(dec%1000000   != 0 && nDigits < 3) nDigits = 3;
     
     dec = (int) 1.e6*(tEnd  [i]-((int)tEnd[i]));
     if     (dec%1000      != 0) nDigits = 6;
     else if(dec%1000000   != 0 && nDigits < 3) nDigits = 3;}

  fp = fopen(fileName, "w");
  if(nDigits == 0)
      fprintf(fp,"    id       Start         End      length\n");
  else if(nDigits == 3)
      fprintf(fp,"    id       Start            End         length\n");
  else 
      fprintf(fp,"    id       Start               End            length\n");

  for(i=0; i<segList->tStart->nData; i++)
    {if(nDigits == 6) fprintf(fp, "%6d %17.6f %17.6f %17.6f\n", 
			      i, tStart[i], tEnd[i], tEnd[i]-tStart[i]);
     else if(nDigits == 3)  fprintf(fp, "%6d %14.3f %14.3f %14.3f\n", 
			      i, tStart[i], tEnd[i], tEnd[i]-tStart[i]);
     else                   fprintf(fp, "%6d %11.0f %11.0f %11.0f\n",
			      i, tStart[i], tEnd[i], tEnd[i]-tStart[i]);}
  
  fclose(fp);

  return(0);
}
/*------------------------------------------------------------FrSegListRead--*/
FrSegList* FrSegListRead(char *fileName)
/*---------------------------------------------------------------------------*/
{FILE *fp;
  FrSegList *segList;

  if(fileName == NULL) return(NULL);
  fp = fopen(fileName, "r");

  segList = FrSegListReadFP(fp, fileName);
  
  fclose(fp);

  return(segList);
}
/*----------------------------------------------------------FrSegListReadFP--*/
FrSegList* FrSegListReadFP(FILE *fp, char *name)
/*---------------------------------------------------------------------------*/
{ FrSegList *segList;
  double tStart, t;
  char word[40];
  int i;

  if(fp == NULL) return(NULL);

  segList = FrSegListNew(name);
  if(segList == NULL) return(NULL);

  tStart = 0;
  while(fscanf(fp,"%39s",word) == 1)
    {if(word[0] == 'S') 
	{if(strcmp(word,"SEGMENT_LIST_END") == 0) return(segList);}

     i = sscanf(word,"%lg",&t);
      if(i != 1) continue;
      if(t > 500123123)
	{if(tStart == 0) tStart = t;
	  else 
	    {FrVectFillD(segList->tStart, tStart);
	      FrVectFillD(segList->tEnd, t);
	      tStart = 0;}}}
  
  return(segList);
}
/*----------------------------------------------------------FrSegListShrink--*/
int FrSegListShrink(FrSegList *segList, double tStart, double tEnd)
/*---------------------------------------------------------------------------*/
{FrSegList *limit, *intersect;

  if(segList == NULL) return(-1);

  limit = FrSegListNew("temporary");
  if(limit == NULL) return(-2);
  FrSegListAdd(limit, tStart, tEnd);
  intersect = FrSegListIntersect(segList, limit);
  FrVectFree(segList->tStart);
  FrVectFree(segList->tEnd);

  segList->tStart = intersect->tStart;
  segList->tEnd   = intersect->tEnd;
  free(intersect->name);
  free(intersect);

  FrSegListFree(limit);

  return(segList->tStart->nData);
}
/*-------------------------------------------------------------FrSerDataDef--*/
FrSH *FrSerDataDef()
/*---------------------------------------------------------------------------*/
{static FrSH *classe = NULL;

  if(classe != NULL) return(classe);
  classe = FrDicAddS("FrSerData",(void (*)())FrSerDataRead);

  FrSENew(classe, "STRING", "name","-");
  FrSENew(classe, "INT_4U", "timeSec","-");
  FrSENew(classe, "INT_4U", "timeNsec","-");
  if(FrFormatVersion == 6)
       FrSENew(classe, "REAL_4", "sampleRate","-");
  else FrSENew(classe, "REAL_8", "sampleRate","-");
  FrSENew(classe, "STRING", "data","-");
  FrSENew(classe, "PTR_STRUCT(FrVect *)", "serial","-");
  FrSENew(classe, "PTR_STRUCT(FrTable *)", "table","-");
  FrSENew(classe, "PTR_STRUCT(FrSerData *)", "next","-");
  if(FrFormatVersion >= 8) FrSENew(classe, "INT_4U", "chkSum","-");

  return(classe);}

/*------------------------------------------------------------FrSerDataDump--*/
void FrSerDataDump(FrSerData *serData, 
                   FILE *fp, 
                   int debugLvl)
/*---------------------------------------------------------------------------*/
{
 if(fp      == NULL) return;
 if(serData == NULL) return;
 if(debugLvl < 1)    return;

 fprintf(fp," SerData: %s GPS=%d sampleRate=%g Data:", 
           serData->name, serData->timeSec, serData->sampleRate);
 if(debugLvl == 1) fprintf(fp," %20s...\n", serData->data);
 else              fprintf(fp,    "  %s\n", serData->data);

 if(serData->serial != NULL) FrVectDump(serData->serial, fp, debugLvl);

 return;}  

/*------------------------------------------------------------FrSerDataFree--*/
void FrSerDataFree(FrSerData *serData)
/*---------------------------------------------------------------------------*/
{
 if(serData == NULL) return;

 if(serData->next   != NULL) FrSerDataFree(serData->next);

 if(serData->serial != NULL) FrVectFree(serData->serial);
 if(serData->table  != NULL) FrTableFree(serData->table);

 if(serData->name   != NULL) free(serData->name);
 if(serData->data   != NULL) free(serData->data);

 free(serData);

 return;}
/*------------------------------------------------------------FrSerDataFind--*/
FrSerData *FrSerDataFind(FrameH *frame,
                         char *smsName,
                         FrSerData *smsStart)
/*---------------------------------------------------------------------------*/
{char data[256];
 FrSerData *sms;

 if(frame == NULL)             return (NULL);
 if(frame->rawData == NULL)    return (NULL);
 if(smsName == NULL)           return (NULL);

 if(smsStart == NULL) 
      smsStart = frame->rawData->firstSer;
 else smsStart = smsStart->next;

 for(sms = smsStart; sms != NULL; sms = sms->next)
    {if(FrStrcmpAndPrefix(sms->name, smsName) == 0) break;}

                    /*------ create the FrameH SMS if it does not exist -----*/

 if(sms == NULL && strcmp(smsName,"FrameH") == 0)
   {sprintf(data,"run %d frame %d dataQuality x%x GTimeS %d GTimeN %d "
               "ULeapS %d dt %g",
             frame->run,    frame->frame,  frame->dataQuality, 
             frame->GTimeS, frame->GTimeN, frame->ULeapS, frame->dt);
    sms = FrSerDataNew(frame, smsName, frame->GTimeS, data, 1./frame->dt);}

 return (sms);}
  
/*-------------------------------------------------------------FrSerDataGet--*/
int FrSerDataGet0(FrameH *frameH,
		  char *smsName,
		  char  *smsParam,
		  double *value)
/*---------------------------------------------------------------------------*/
/* this function return:
      -2 if the sms is not found
      -1 if the parameters is not found
       0 if everything is ok, the result is put in value */

{FrSerData *sms;
 char vName[50], vValue[50], *pointer;
 int l;
 unsigned int i;

 *value = 0;
 
 sms = FrSerDataFind(frameH, smsName, NULL);
 if(sms == NULL) return(-2);
 
         /*-- We have found the sms structure, search for the first word ----*/

  strcpy(vName,smsParam);
  l = strlen(smsParam);
  vName[l] = ' ';
  if(strncmp(sms->data, vName, l+1) == 0)
     {pointer = sms->data;}
  else
     {vName[0] = ' ';
      strcpy(&vName[1],smsParam);
      l = strlen(smsParam);
      vName[l+1] = ' ';
      vName[l+2] = '\0';
 
      pointer = strstr(sms->data, vName);
      if(pointer == NULL) return (-1);}

  sscanf(pointer,"%s %s",vName, vValue);
  if(vValue[0] == 'x')
       {sscanf(vValue+1,"%x",&i);
        *value = i;}
  else if(isalpha((int) vValue[0]))
       {sscanf(vValue+1,"%lf",value);}
  else {sscanf(vValue  ,"%lf",value);}

  return (0);}
  
/*-------------------------------------------------------------FrSerDataGet--*/
int FrSerDataGet(FrameH *frame,
		 char *smsName,
		 char  *smsParam,
		 double *value)
/*---------------------------------------------------------------------------*/
{
  char* adcName;
  FrAdcData* adc;
  int err;

  err = FrSerDataGet0(frame, smsName, smsParam, value);
  if(err == 0) return(0);

  /*- if we do not found the value in the serData we search in the 1Hz 
    FrAdcDat since frame resizing may have change its type ------------------*/

  adcName = malloc(strlen(smsName)+strlen(smsParam)+2);
  if(adcName == NULL) return(err);

  sprintf(adcName,"%s_%s",smsName, smsParam);
  adc = FrAdcDataFind (frame, adcName);
  free(adcName);

  if(adc == NULL) return(err);
  if(adc->data == NULL) return(err);
  if(adc->data->nData != 1) return(err);

  *value = FrVectGetValueI (adc->data, 0);
  return(0);}

/*--------------------------------------------------------FrSerDataGetValue--*/
double FrSerDataGetValue(FrameH *frame,
                         char *smsName,
                         char  *smsParam,
                         double defaultValue)
/*---------------------------------------------------------------------------*/
{
  double value;
  int err;

  err = FrSerDataGet(frame, smsName, smsParam, &value);
  if(err == 0) return(value);

  return(defaultValue);
}
/*-------------------------------------------------------------FrSerDataNew--*/
FrSerData *FrSerDataNew(FrameH *frame,
                        char *smsName,
                        unsigned int serTime,
                        char *data,
                        double sampleRate)
/*---------------------------------------------------------------------------*/
{FrSerData *smsData;
 int slen;

 if(data != NULL)
   {slen = strlen(data)+1;
      if(slen > 65535) 
        {FrError(3,"FrSerDataNew","too long data string"); 
         return(NULL);}}

  smsData = (FrSerData *) calloc(1,sizeof(FrSerData));
  if(smsData == NULL) return(NULL);
  smsData->classe = FrSerDataDef();
	 
  if(FrStrCpy(&smsData->name, smsName) == NULL) return(NULL);
  if(FrStrCpy(&smsData->data, data)    == NULL) return(NULL);

  smsData->timeSec    = serTime;
  smsData->timeNsec   = 0;
  smsData->sampleRate = sampleRate;
  smsData->table      = NULL;

  if(frame != NULL) FrameAddSer(frame, smsData);
	   
  return(smsData);}

/*----------------------------------------------------------- FrSerDataRead--*/
FrSerData *FrSerDataRead(FrFile *iFile)
/*---------------------------------------------------------------------------*/
{FrSerData *smsData;
 char message[128];
 float rate;

  smsData = (FrSerData *) calloc(1,sizeof(FrSerData));
  if(smsData == NULL) 
       {iFile->error = FR_ERROR_MALLOC_FAILED;
        return(NULL);}
  smsData->classe = FrSerDataDef();
  
  FrReadHeader(iFile,  smsData);
  FrReadSChar( iFile, &smsData->name);
  FrReadIntU  (iFile, &smsData->timeSec);
  FrReadIntU  (iFile, &smsData->timeNsec);
  if(iFile->fmtVersion == 6)
      {FrReadFloat (iFile, &rate);
       smsData->sampleRate = rate;}
  else FrReadDouble (iFile, &smsData->sampleRate);
  FrReadSChar (iFile, &smsData->data);
  FrReadStruct(iFile, &smsData->serial);
  FrReadStruct(iFile, &smsData->table);
  FrReadStruct(iFile, &smsData->next);
  if(iFile->fmtVersion >=8) FrReadStructChksum(iFile);

  if(iFile->nBytes != iFile->nBytesR + iFile->length)
     {sprintf(message,": Record length error: "
         "nBytes=%"FRLLD" nBytesR=%"FRLLD" length=%"FRLLD"\n",
          iFile->nBytes, iFile->nBytesR, iFile->length);
      FrError(3,"FrSerDataRead",message);
      return(NULL);}     

  if(FrDebugLvl > 2) fprintf(FrFOut," %s\n", smsData->name);

 return(smsData);} 

/*--------------------------------------------------------- FrSerDataReadT--*/
FrSerData *FrSerDataReadT(FrFile *iFile,
                            char *name,
                            double gtime)
/*---------------------------------------------------------------------------*/
{FrSerData  **current, *ser, *root;
 FrTOCts *ts;
 FrTag *frtag;
 int index;
                       /*---- find frame(it will read the FrTOC if needed) --*/

 index = FrTOCFrameFindT(iFile, gtime);
 if(index <0) return(NULL);
                                
 if(name == NULL) return(NULL);    /*-------------------- build tag object---*/
 frtag = FrTagNew(name);
 if(frtag == NULL) return(NULL);
                                
                       /*-------------- Extract all the requested channels --*/
 root = NULL;
 current = &root;

 for(ts = iFile->toc->ser; ts != NULL; ts = ts->next)

                       /*---------- check if we need to copy this ser ?-----*/

   {if(FrTagMatch(frtag, ts->name) == FR_NO) continue; 

                       /*---- set the file pointer and read the data -------*/

    if(FrTOCSetPos(iFile, ts->position[index]) != 0) continue;
    ser = FrSerDataRead(iFile);
    if(ser == NULL) continue;

    *current = ser;
    current = &(ser->next);}
 
 FrTagFree(frtag); 

 return(root);}

/*-----------------------------------------------------------FrSerDataWrite--*/
void FrSerDataWrite(FrSerData *serData,
                    FrFile *oFile)
/*---------------------------------------------------------------------------*/
{
  FrPutNewRecord(oFile, serData, FR_YES);
  FrPutSChar (oFile, serData->name);
  FrPutIntU  (oFile, serData->timeSec);
  FrPutIntU  (oFile, serData->timeNsec);
  if(FrFormatVersion == 6)
       FrPutFloat (oFile, serData->sampleRate);
  else FrPutDouble(oFile, serData->sampleRate);
  FrPutSChar (oFile, serData->data);
  FrPutStruct(oFile, serData->serial);
  FrPutStruct(oFile, serData->table);
  FrPutStruct(oFile, serData->next);

  if(oFile->toc != NULL) FrTOCtsMark(oFile, &oFile->toc->serH, 
                                    serData->name, 0, 0xffffffff);

  FrPutWriteRecord(oFile, FR_NO);
    
  if(serData->serial != NULL) FrVectWrite(serData->serial, oFile);
  if(serData->table  != NULL) FrTableWrite(serData->table, oFile);

 return;} 
/*-------------------------------------------------------------FrSimDataDef--*/
FrSH *FrSimDataDef()
/*---------------------------------------------------------------------------*/
{static FrSH *classe = NULL;

  if(classe != NULL) return(classe);
  classe = FrDicAddS("FrSimData",(void (*)())FrSimDataRead);

  FrSENew(classe, "STRING", "name","-");
  FrSENew(classe, "STRING", "comment","-");
  if(FrFormatVersion == 6)
       FrSENew(classe, "REAL_4", "sampleRate","-");
  else FrSENew(classe, "REAL_8", "sampleRate","-");
  FrSENew(classe, "REAL_8", "timeOffset","-");
  FrSENew(classe, "REAL_8", "fShift","-");
  FrSENew(classe, "REAL_4", "phase","-");
  FrSENew(classe, "PTR_STRUCT(FrVect *)", "data","-");
  FrSENew(classe, "PTR_STRUCT(FrVect *)", "input","-");
  FrSENew(classe, "PTR_STRUCT(FrTable *)", "table","-");
  FrSENew(classe, "PTR_STRUCT(FrSimData *)", "next","-");
  if(FrFormatVersion >= 8) FrSENew(classe, "INT_4U", "chkSum","-");

  return(classe);}

/*------------------------------------------------------------FrSimDataDump--*/
void FrSimDataDump(FrSimData *simData,
                   FILE *fp, 
                   int debugLvl)
/*---------------------------------------------------------------------------*/
{
 if(simData == NULL) return;
 if(fp == NULL)   return;
 if(debugLvl < 0) return;

 fprintf(fp,"SimData: %s ", simData->name);
 if(simData->comment != NULL) fprintf(fp," (%s)", simData->comment);
 fprintf(fp,"  rate=%.2fHz tOffset=%.3f fShift=%g Hz\n",
	 simData->sampleRate, simData->fShift, simData->phase);

 if(simData->data  != NULL){FrVectDump (simData->data,  fp, debugLvl);}
 if(simData->table != NULL){FrTableDump(simData->table, fp, debugLvl);}

 return;}

/*------------------------------------------------------------FrSimDataFind--*/
FrSimData *FrSimDataFind(FrameH *frame,
                         char *name)
/*---------------------------------------------------------------------------*/
{FrSimData *sim;

 if(frame == NULL)             return (NULL);
 if(frame->simData == NULL)    return (NULL);
 if(name == NULL)              return (NULL);

 sim = (FrSimData*) FrameFindBasic((FrBasic*)frame->simData, name);
 if(sim == NULL) return(NULL);

 if(sim->data  != NULL) FrVectExpandF(sim->data);
 if(sim->table != NULL) FrTableExpand(sim->table);
        
 sim->data->GTime     = frame->GTimeS + 1.e-9 *
                        frame->GTimeN + sim->timeOffset;
 sim->data->ULeapS    = frame->ULeapS;
 sim->data->localTime = FrameGetLTOffset(frame, name);

 return(sim);}
  
/*------------------------------------------------------------FrSimDataFree--*/
void FrSimDataFree(FrSimData *simData)
/*---------------------------------------------------------------------------*/
{
 while(simData != NULL) {simData = FrSimDataFreeOne(simData);}

 return;}

/*---------------------------------------------------------FrSimDataFreeOne--*/
FrSimData* FrSimDataFreeOne(FrSimData *simData)
/*---------------------------------------------------------------------------*/
{
 FrSimData *next;

 if(simData == NULL) return(NULL);

 if(simData->name    != NULL) free (simData->name);
 if(simData->comment != NULL) free (simData->comment);
 if(simData->data    != NULL) FrVectFree(simData->data);
 if(simData->input   != NULL) FrVectFree(simData->input);
 if(simData->table   != NULL) FrTableFree(simData->table);

 next = simData->next;
 
 free(simData);

 return(next);}

/*--------------------------------WARNING:OBSOLETE FUNCTION---FrSimDataGetV--*/
FrVect *FrSimDataGetV(FrameH *frame, char *name)
/*---------------------------------------------------------------------------*/
{return(FrameFindSimVect(frame,name));}

/*-------------------------------------------------------------FrSimDataNew--*/
FrSimData *FrSimDataNew(FrameH *frame,
                        char   *name,
                        double  sampleRate,
                        FRLONG  nData,
                        int     nBits)
/*---------------------------------------------------------------------------*/
{FrSimData *simData;
 double dx;
 int type;
 
  simData = (FrSimData *) calloc(1,sizeof(FrSimData));
  if(simData == NULL) return(NULL); 
  simData->classe = FrSimDataDef();
	 
  if(FrStrCpy(&simData->name, name) == NULL) return(NULL); 
	   
  simData->sampleRate = sampleRate;

  if(nBits >16)       {type = FR_VECT_4S;}
  else if(nBits >  8) {type = FR_VECT_2S;}
  else if(nBits >  0) {type = FR_VECT_C;}
  else if(nBits >-33) {type = FR_VECT_4R;}
  else                {type = FR_VECT_8R;}

  if(sampleRate == 0.)
       {dx = 0.;}
  else {dx = 1./sampleRate;}

  simData->data = FrVectNew1D(name,type,nData,dx,"time (s)",NULL);
  if(simData->data == NULL)
       {FrError(3,"FrSimDataNew","cannot create vector");
        return(NULL);}

 free(simData->data->name);
 if(FrStrCpy(&simData->data->name,name) == NULL) return(NULL);
 
                           /*----- now store it in the Frame structures -----*/
  if(frame != NULL) FrameAddSimData(frame, simData);

  return(simData);} 

/*------------------------------------------------------------FrSimDataRead--*/
FrSimData *FrSimDataRead(FrFile *iFile)
/*---------------------------------------------------------------------------*/
{FrSimData *simData;
 char message[128];
 float rate;

 if(iFile->fmtVersion == 3) return(FrBack3SimDataRead(iFile));
 if(iFile->fmtVersion == 4) return(FrBack4SimDataRead(iFile));
 if(iFile->fmtVersion == 5) return(FrBack4SimDataRead(iFile));

  simData = (FrSimData *) calloc(1,sizeof(FrSimData));
  if(simData == NULL) 
       {iFile->error = FR_ERROR_MALLOC_FAILED;
        return(NULL);}
  simData->classe = FrSimDataDef();
  
  FrReadHeader(iFile,  simData);
  FrReadSChar( iFile, &simData->name);
  FrReadSChar (iFile, &simData->comment);
  if(iFile->fmtVersion == 6)
      {FrReadFloat (iFile, &rate);
       simData->sampleRate = rate;}
  else FrReadDouble(iFile, &simData->sampleRate);
  FrReadDouble(iFile, &simData->timeOffset);
  FrReadDouble(iFile, &simData->fShift);
  FrReadFloat (iFile, &simData->phase);
  FrReadStruct(iFile, &simData->data);
  iFile->vectInstance = iFile->instance;
  FrReadStruct(iFile, &simData->input);
  FrReadStruct(iFile, &simData->table);
  FrReadStruct(iFile, &simData->next);
  if(iFile->fmtVersion >=8) FrReadStructChksum(iFile);

  if(iFile->nBytes != iFile->nBytesR + iFile->length)
     {sprintf(message,": Record length error: "
         "nBytes=%"FRLLD" nBytesR=%"FRLLD" length=%"FRLLD"\n",
          iFile->nBytes, iFile->nBytesR, iFile->length);
      FrError(3,"FrSimDataRead",message);
      return(NULL);}     

  if(FrDebugLvl > 2) fprintf(FrFOut," %s\n", simData->name);

return(simData);}

/*-----------------------------------------------------------FrSimDataReadT--*/
FrSimData *FrSimDataReadT(FrFile *iFile,
                          char *name,
                          double gtime)
/*---------------------------------------------------------------------------*/
{FrSimData  **current, *sim, *root;
 FrTOCts *ts;
 FrTag *frtag;
 int index;

 if(name == NULL) return(NULL);
 frtag = FrTagNew(name);
 if(frtag == NULL) return(NULL);

                       /*---- find frame(it will read the FrTOC if needed) --*/

 index = FrTOCFrameFindT(iFile, gtime);
 if(index <0) return(NULL);
                                
                       /*-------------- Extract all the requested channels --*/
 root = NULL;
 current = &root;

 for(ts = iFile->toc->sim; ts != NULL; ts = ts->next)

                       /*---------- check if we need to copy this sim ?-----*/

   {if(FrTagMatch(frtag, ts->name) == FR_NO) continue; 

                       /*---- set the file pointer and read the data -------*/

    if(FrTOCSetPos(iFile, ts->position[index]) != 0) continue;
    sim = FrSimDataRead(iFile);
    if(sim == NULL) continue;

    gtime = iFile->toc->GTimeS[index] + 1.e-9 *
            iFile->toc->GTimeN[index] + sim->timeOffset;
    sim->data = FrVectReadNext(iFile, gtime, sim->name);

    *current = sim;
    current = &(sim->next);}
 
 FrTagFree(frtag); 

 return(root);}

/*-----------------------------------------------------------FrSimDataWrite--*/
void FrSimDataWrite(FrSimData *simData,
                    FrFile *oFile)
/*---------------------------------------------------------------------------*/
{
  FrPutNewRecord(oFile, simData, FR_YES);
  FrPutSChar (oFile, simData->name);
  FrPutSChar (oFile, simData->comment);
  if(FrFormatVersion == 6)
       FrPutFloat (oFile, simData->sampleRate);
  else FrPutDouble(oFile, simData->sampleRate);
  FrPutDouble(oFile, simData->timeOffset);
  FrPutDouble(oFile, simData->fShift);
  FrPutFloat (oFile, simData->phase);
  FrPutStruct(oFile, simData->data);
  FrPutStruct(oFile, simData->input);
  FrPutStruct(oFile, simData->table);
  FrPutStruct(oFile, simData->next);

  if(oFile->toc != NULL) FrTOCtsMark(oFile, &oFile->toc->simH, 
                                     simData->name, 0, 0);

  FrPutWriteRecord(oFile, FR_NO);

            /*-------------data write buffer---------------------------------*/

  FrVectWrite(simData->data, oFile);
     
  if(simData->input != NULL){FrVectWrite(simData->input, oFile);}
  if(simData->table != NULL){FrTableWrite(simData->table, oFile);}

  if(simData->next != NULL) {FrSimDataWrite(simData->next, oFile);}

 return;}

/*-------------------------------------------------------FrSimEventAddParam--*/
FrSimEvent *FrSimEventAddParam(FrSimEvent *event,  
                               char *name, 
                               double value)
/*---------------------------------------------------------------------------*/
{
 if(event == NULL) return(NULL);

 if(event->nParam > 65534) return(NULL);  /*--overflow the storage capacity--*/

 event->nParam++;

 if(event->nParam == 1)
   {event->parameters     = (double *)malloc(sizeof(double));
    event->parameterNames = (char **) malloc(sizeof(char *));}
 else   
   {event->parameters     = (double *)realloc(event->parameters, 
                                              event->nParam* sizeof(double));
    event->parameterNames = (char **) realloc(event->parameterNames,
                                              event->nParam* sizeof(char *));}

 if(event->parameters     == NULL) return(NULL);
 if(event->parameterNames == NULL) return(NULL);

 if(FrStrCpy(&(event->parameterNames[event->nParam - 1]),name) == NULL) 
                                             return(NULL);
 event->parameters[event->nParam - 1] = value;

 return(event);} 

/*--------------------------------------------------------FrSimEventAddVect--*/
int FrSimEventAddVect(FrSimEvent *event,  
                      FrVect *vect,
                      char* newName)
/*---------------------------------------------------------------------------*/
{FrVect *copy, *last;

  if(event == NULL) return(1);
  if(vect  == NULL) return(2);
  
  copy = FrVectCopy(vect);
  if(copy == NULL) return(3);
  if(newName != NULL) FrVectSetName(copy, newName);
  copy->startX[0] = vect->GTime + vect->startX[0] 
                - (event->GTimeS + 1.e-9*event->GTimeN);
  
  if(event->data == NULL)
    {event->data = copy;}
  else
    {last = event->data;
     while(last->next != NULL) {last = last->next;}
     last->next = copy;}

 return(0);}

/*-------------------------------------------------------FrSimEventAddVectF--*/
int FrSimEventAddVectF(FrSimEvent *event,  
                       FrVect *vect,
                       char* newName)
/*---------------------------------------------------------------------------*/
{FrVect *copy, *last;

  if(event == NULL) return(1);
  if(vect  == NULL) return(2);
  
  copy= FrVectCopyToF(vect, 1., newName);
  if(copy == NULL) return(3);
  copy->startX[0] = vect->GTime + vect->startX[0] 
                - (event->GTimeS + 1.e-9*event->GTimeN);
  
  if(event->data == NULL)
    {event->data = copy;}
  else
    {last = event->data;
     while(last->next != NULL) {last = last->next;}
     last->next = copy;}

 return(0);}

/*-----------------------------------------------------------FrSimEventCopy--*/
FrSimEvent *FrSimEventCopy(FrSimEvent *event)
/*---------------------------------------------------------------------------*/
{FrSimEvent *copy;
 int i;

 if(event == NULL) return(NULL);

 copy = (FrSimEvent *) calloc(1,sizeof(FrSimEvent));
 if(copy == NULL) return(NULL);
 copy->classe = FrSimEventDef();

 copy->GTimeS     = event->GTimeS; 
 copy->GTimeN     = event->GTimeN; 
 copy->timeBefore = event->timeBefore; 
 copy->timeAfter  = event->timeAfter; 
 copy->amplitude  = event->amplitude; 
 
 if(FrStrCpy(&copy->name,       event->name)       == NULL) return(NULL);
 if(FrStrCpy(&copy->comment,    event->comment)    == NULL) return(NULL);
 if(FrStrCpy(&copy->inputs,     event->inputs)     == NULL) return(NULL);
 
 copy->data  = FrVectCopy(event->data);
 copy->table = FrTableCopy(event->table);

 if(event->nParam > 0)
   {copy->parameters     = (double *)calloc(event->nParam, sizeof(double));
    copy->parameterNames = (char **) calloc(event->nParam, sizeof(char *));

    if(copy->parameters     == NULL) return(NULL);
    if(copy->parameterNames == NULL) return(NULL);

     for(i=0; i<event->nParam; i++)
       {if(event->parameterNames[i] != NULL)
         {if(FrStrCpy(&(copy->parameterNames[i]),
                        event->parameterNames[i]) == NULL) return(NULL);}
        copy->parameters[i] = event->parameters[i];}}

 copy->nParam = event->nParam;

 return(copy);} 

/*----------------------------------------------------------- FrSimEventDef--*/
FrSH *FrSimEventDef()
/*---------------------------------------------------------------------------*/
{static FrSH *classe = NULL;

 if(classe != NULL) return(classe);
 classe = FrDicAddS("FrSimEvent",(void (*)())FrSimEventRead);

 FrSENew(classe, "STRING", "name","-");
 FrSENew(classe, "STRING", "comment","-");
 FrSENew(classe, "STRING", "inputs","-");
 FrSENew(classe, "INT_4U", "GTimeS","-");
 FrSENew(classe, "INT_4U", "GTimeN","-");
 FrSENew(classe, "REAL_4", "timeBefore","-");
 FrSENew(classe, "REAL_4", "timeAfter","-");
 FrSENew(classe, "REAL_4", "amplitude","-");
 FrSENew(classe, "INT_2U", "nParam", "-");
 if(FrFormatVersion == 6)
      FrSENew(classe, "REAL_4[nParam]", "parameters","-");
 else FrSENew(classe, "REAL_8[nParam]", "parameters","-");
 FrSENew(classe, "STRING[nParam]", "parameterNames","-");
 FrSENew(classe, "PTR_STRUCT(FrVect *)",   "data","-");
 FrSENew(classe, "PTR_STRUCT(FrTable *)",   "table","-");
 FrSENew(classe, "PTR_STRUCT(FrSimEvent *)", "next","-"); 
 if(FrFormatVersion >= 8) FrSENew(classe, "INT_4U", "chkSum","-");

 return(classe);}

/*-----------------------------------------------------------FrSimEventDump--*/
void FrSimEventDump(FrSimEvent *evt,
                    FILE *fp, 
                    int debugLvl)
/*---------------------------------------------------------------------------*/
{int i;

 if(fp  == NULL) return;
 if(evt == NULL) return;
 if(debugLvl < 1) return;

 fprintf(fp,"SimEvent:%s amplitude=%10.4e time=%.5f s",
           evt->name,       evt->amplitude,  evt->GTimeS+1.e-9*evt->GTimeN);

 if(debugLvl > 1)
  {fprintf(fp," (before=%.5f after=%.5fs)\n",evt->timeBefore, evt->timeAfter);

   if(evt->comment    != NULL) fprintf(fp,"   comment: %s\n", evt->comment);
   if(evt->inputs     != NULL) fprintf(fp,"   inputs: %s\n", evt->inputs);}

 if(evt->nParam>0) 
   {fprintf(fp,"   parameters:");
    for(i=0; i< evt->nParam; i++)
      {fprintf(fp,"\t%s=%g",evt->parameterNames[i],evt->parameters[i]);
       if(debugLvl > 1 && i%4==3 && i != evt->nParam-1) fprintf(fp,"\n");}
    fprintf(fp,"\n");}

 if(debugLvl < 2) return;
 if(evt->data  != NULL){FrVectDump (evt->data,  fp, debugLvl);}
 if(evt->table != NULL){FrTableDump(evt->table, fp, debugLvl);}

 return;}

/*-----------------------------------------------------------FrSimEventFind--*/
FrSimEvent *FrSimEventFind(FrameH *frameH, 
                           char *name,
                           FrSimEvent *simEvt)
/*---------------------------------------------------------------------------*/
{
 if(frameH == NULL)  return(NULL);
 if(name   == NULL)  return(NULL);

 if(simEvt == NULL) 
      simEvt = frameH->simEvent;
 else simEvt = simEvt->next;

 for(; simEvt != NULL; simEvt = simEvt->next)
    {if(strcmp(simEvt->name, name) != 0) continue;

     if(simEvt->data  != NULL) FrVectExpandF(simEvt->data);
     if(simEvt->table != NULL) FrTableExpand(simEvt->table);

     return (simEvt);}
  
 return (NULL);}
/*------------------------------------------------------FrSimEventFindVect--*/
FrVect* FrSimEventFindVect(FrSimEvent *event, 
                           char *name)
/*--------------------------------------------------------------------------*/
{FrVect *vect;

 if(event == NULL) return(NULL);
 if(name  == NULL) return(NULL);

 vect = event->data;
 while(vect != NULL)
   {if(strcmp(vect->name, name) == 0) return(vect);
   vect = vect->next;}

 return (NULL);}

/*-----------------------------------------------------------FrSimEventFree--*/
void FrSimEventFree(FrSimEvent *event)
/*---------------------------------------------------------------------------*/
{int i;

 if(event == NULL) return;

 if(event->next   != NULL) FrSimEventFree(event->next);

 if(event->name    != NULL) free (event->name);
 if(event->comment != NULL) free (event->comment);
 if(event->inputs  != NULL) free (event->inputs);

 if(event->nParam > 0)
   {for(i= 0; i<event->nParam; i++) {free(event->parameterNames[i]);}
    free(event->parameters);
    free(event->parameterNames);}

 if(event->data  != NULL) FrVectFree (event->data);
 if(event->table != NULL) FrTableFree(event->table);

 free(event);

 return;}

/*------------------------------------------------------FrSimEventGetParam--*/
double FrSimEventGetParam(FrSimEvent *event, 
                          char *name)
/*--------------------------------------------------------------------------*/
{int id;

 id = FrSimEventGetParamId(event, name);

 if(id < 0) return(-1.);
 
 return(event->parameters[id]);}

/*----------------------------------------------------FrSimEventGetParamId--*/
int FrSimEventGetParamId(FrSimEvent *event, 
                         char *name)
/*--------------------------------------------------------------------------*/
{int i;

 if(event == NULL) return(-1);
 if(name  == NULL) return(-1);

 for(i=0; i<event->nParam; i++)
   {if(strcmp(event->parameterNames[i], name) == 0) return(i);}

 return (-1);}

/*------------------------------------------------------FrSimEventGetVectD--*/
FrVect* FrSimEventGetVectD(FrSimEvent *event, 
                           char *name)
/*--------------------------------------------------------------------------*/
{FrVect *vect;

 vect = FrSimEventFindVect(event, name);
 vect = FrVectCopyToD(vect, 1., NULL);

 return (vect);}

/*------------------------------------------------------FrSimEventGetVectF--*/
FrVect* FrSimEventGetVectF(FrSimEvent *event, 
                           char *name)
/*--------------------------------------------------------------------------*/
{FrVect *vect;

 vect = FrSimEventFindVect(event, name);
 vect = FrVectCopyToF(vect, 1., NULL);

 return (vect);}

/*------------------------------------------------------------FrSimEventNew--*/
FrSimEvent *FrSimEventNew(FrameH *frameH,  
                          char *name, 
                          char *comment, 
                          char *inputs, 
                          double gtime,
                          float timeBefore,
                          float timeAfter,
                          float amplitude,
                          int nParam, ...)
/*---------------------------------------------------------------------------*/
{FrSimEvent *event;
 char *pName;
 va_list ap;
 int i, GTimeN;
 
 event = (FrSimEvent *) calloc(1,sizeof(FrSimEvent));
 if(event == NULL) return(NULL); 
 event->classe = FrSimEventDef();
	 
 if(FrStrCpy(&event->name,name)       == NULL) return(NULL);
 if(FrStrCpy(&event->comment,comment) == NULL) return(NULL);
 if(FrStrCpy(&event->inputs,inputs)   == NULL) return(NULL);

 event->GTimeS     = gtime;
 GTimeN = 1.e9*(gtime - (double) event->GTimeS);
 if(GTimeN < 0) GTimeN = 0;
 event->GTimeN     = GTimeN;
 event->timeBefore = timeBefore;
 event->timeAfter  = timeAfter;
 event->amplitude  = amplitude; 
 event->nParam     = nParam;
 
 if(nParam > 0)
   {event->parameters     = (double *)malloc(nParam * sizeof(double));
    event->parameterNames = (char **) malloc(nParam * sizeof(char *));

    if(event->parameters     == NULL) return (NULL);
    if(event->parameterNames == NULL) return(NULL);

    va_start(ap,nParam);
    for(i=0; i<nParam; i++)
       {pName = va_arg(ap, char *);
        if(pName != NULL)
          {if(FrStrCpy(&(event->parameterNames[i]),pName)==NULL) return(NULL);}
       event->parameters[i] = va_arg(ap, double);}
    va_end(ap);}

       /*------------- now store it in the Frame structures ----------------*/

 if(frameH != NULL) FrameAddSimEvent(frameH, event);
  
 return(event);} 

/*-----------------------------------------------------------FrSimEventRead--*/
FrSimEvent *FrSimEventRead(FrFile *iFile)
/*---------------------------------------------------------------------------*/
{FrSimEvent *simEvent;
 char message[128];

 if(iFile->fmtVersion <= 5) return(FrBack4SimEventRead(iFile));

 simEvent = (FrSimEvent *) calloc(1,sizeof(FrSimEvent));
 if(simEvent == NULL) 
      {iFile->error = FR_ERROR_MALLOC_FAILED;
       return(NULL);}
 simEvent->classe = FrSimEventDef();

 FrReadHeader(iFile,  simEvent);
 FrReadSChar (iFile, &simEvent->name); 
 FrReadSChar (iFile, &simEvent->comment);
 FrReadSChar (iFile, &simEvent->inputs);
 FrReadIntU  (iFile, &simEvent->GTimeS);
 FrReadIntU  (iFile, &simEvent->GTimeN);
 FrReadFloat (iFile, &simEvent->timeBefore);
 FrReadFloat (iFile, &simEvent->timeAfter);
 FrReadFloat (iFile, &simEvent->amplitude);
 FrReadShortU(iFile, &simEvent->nParam);
 if(simEvent->nParam > 0)  
   {if(iFile->fmtVersion == 6)
          FrReadVFD(iFile, &simEvent->parameters,     simEvent->nParam);
     else FrReadVD (iFile, &simEvent->parameters,     simEvent->nParam);
    FrReadVQ (iFile, &simEvent->parameterNames, simEvent->nParam);}
 FrReadStruct(iFile, &simEvent->data);
 iFile->vectInstance = iFile->instance;
 FrReadStruct(iFile, &simEvent->table);
 FrReadStruct(iFile, &simEvent->next);
  if(iFile->fmtVersion >=8) FrReadStructChksum(iFile);

  if(iFile->nBytes != iFile->nBytesR + iFile->length)
     {sprintf(message,": Record length error: "
         "nBytes=%"FRLLD" nBytesR=%"FRLLD" length=%"FRLLD"\n",
          iFile->nBytes, iFile->nBytesR, iFile->length);
      FrError(3,"FrSimEventRead",message);
      return(NULL);}     

 if(FrDebugLvl > 2) fprintf(FrFOut," %s\n", simEvent->name);

 return(simEvent);}

/*------------------------------------------------------ FrSimEventReadData--*/
int FrSimEventReadData(FrFile *file,
                       FrSimEvent *event)
/*---------------------------------------------------------------------------*/
{FrSimEvent *tmp, *match;
 double dt=1.e-7;
 int error = 0;

 if(file  == NULL) return(3);

 for(;event != NULL; event = event->next)
   {tmp = FrSimEventReadTF1(file, event->name, 
		 event->GTimeS + 1.e-9*event->GTimeN - dt, 2*dt, 1, 
                  "amplitude", event->amplitude*.99, event->amplitude*1.01);
    if(tmp == NULL) 
        {error = 1;
         continue;}
 
    for(match = tmp; match != NULL; match = match->next)
       {if(match->amplitude != event->amplitude) continue;
        if(match->GTimeS    != event->GTimeS)    continue;
        if(match->GTimeN    != event->GTimeN)    continue;
        if(event->data != NULL) FrVectFree(event->data);
        event->data = match->data;
        match->data = NULL;
        break;}

    FrSimEventFree(tmp);
    if(match == NULL) error = 2;}

 return(error);}

/*--------------------------------------------------------- FrSimEventReadT--*/
FrSimEvent *FrSimEventReadT(FrFile *iFile,
                            char *name,
                            double tStart,
                            double length,
                            double amplitudeMin,
                            double amplitudeMax)
/*---------------------------------------------------------------------------*/
{
 return(FrSimEventReadTF(iFile, name, tStart, length, 0, 1, 
			 "amplitude", amplitudeMin, amplitudeMax));}
 
/*------------------------------------------------------- FrSimEventReadTF--*/
FrSimEvent *FrSimEventReadTF(FrFile *iFile,
                             char *name,
                             double tStart,
                             double length,
                             int readData, /* 0= no vector; 1= read vectors */
                             int nParam,
                             ...)
/*---------------------------------------------------------------------------*/
{FrSimEvent  *root, *event, *last;
 FrTOCevt *tocEvt;
 FrTag    *frtag;
 FrFileH  *firstFileH;
 double tEvent, tEnd;
 float *pMin, *pMax, aMin, aMax;
 char **pNames, *pName;
 va_list ap;
 FRBOOL aCheck, select;
 int i, j, k;
                                          /*------- extract parameters ----*/
 if(nParam > 0)
   {pMin   = (float *) calloc(nParam, sizeof(float));
    pMax   = (float *) calloc(nParam, sizeof(float));
    pNames = (char **) calloc(nParam, sizeof(char *));
    if(pMin   == NULL) return (NULL);
    if(pMax   == NULL) return (NULL);
    if(pNames == NULL) return(NULL);

    va_start(ap,nParam);
    for(i=0; i<nParam; i++)
       {pName = va_arg(ap, char *);
        if(pName != NULL)
         {if(FrStrCpy(&(pNames[i]),pName) == NULL) return(NULL);}
        pMin[i] = va_arg(ap, double);
        pMax[i] = va_arg(ap, double);}
    va_end(ap);}
 else
   {pMin = NULL;
    pMax = NULL; 
    pNames = NULL;}
	                      /*------- should we check the amplitude? ---*/
 aMin = 0.;
 aMax = 0.;
 aCheck = FR_NO; 
 for(i=0; i<nParam; i++)
   {if(strcmp(pNames[i],"amplitude") != 0) continue; 
    aMin = pMin[i];
    aMax = pMax[i];
    aCheck = FR_YES;}  
 
 if(iFile == NULL) return(NULL);                 /*----- check input file ---*/
 firstFileH = iFile->current;
 iFile->relocation = FR_NO;            /*--- to avoid relloaction problems---*/

 tEnd = tStart + length;

 frtag = FrTagNew(name);
 if(frtag == NULL) return(NULL);
                                                 /*--- Loop over all files--*/
 root = NULL;
 last = NULL;

 do
   {if(iFile->toc == NULL) FrTOCReadFull(iFile); /*--------- get the TOC----*/
    if(iFile->toc == NULL) return(NULL);

                  /*---------Find name in the channel list for this file----*/

    for(tocEvt = iFile->toc->simEvt; tocEvt != NULL; tocEvt = tocEvt->next) 
      {if(FrTagMatch(frtag, tocEvt->name) == FR_NO) continue; 

       for(i=0; i<tocEvt->nEvent; i++)
         {tEvent = tocEvt->GTimeS[i]+1.e-9*tocEvt->GTimeN[i];
          if(tEvent < tStart) continue;
          if(tEvent > tEnd)   continue;
          if(aCheck == FR_YES && tocEvt->amplitude[i] < aMin) continue;
          if(aCheck == FR_YES && tocEvt->amplitude[i] > aMax) continue;
          if(FrTOCSetPos(iFile, tocEvt->position[i]) != 0) continue;
          event = FrSimEventRead(iFile);

          select = FR_YES;           /*----- additional selection criteria --*/
          for(j=0; j<nParam; j++)
             {if(strcmp(pNames[j],"amplitude") == 0) continue;
              if(strcmp(pNames[j],"timeBefore") == 0)
               {if(event->timeBefore < pMin[j]) select = FR_NO;  
                if(event->timeBefore > pMax[j]) select = FR_NO;}  
              if(strcmp(pNames[j],"timeAfter") == 0)
               {if(event->timeAfter < pMin[j]) select = FR_NO;  
                if(event->timeAfter > pMax[j]) select = FR_NO;}  
              for(k=0; k<event->nParam; k++)
                {if(strcmp(pNames[j],event->parameterNames[k]) == 0)
                  {if(event->parameters[k] < pMin[j]) select = FR_NO;  
                   if(event->parameters[k] > pMax[j]) select = FR_NO;}}}  
          if(select == FR_NO) 
             {FrSimEventFree(event);
              continue;}

          if(readData == 1)
            {event->data = FrVectReadNext(iFile, tEvent, event->name);}
 
          if(root == NULL) root = event;
          else last->next = event;
          last = event;}}}

    while (FrFileINext(iFile, tStart, length, firstFileH, FR_YES) == 0);

                            /*--------------------- free working space -----*/
 FrTagFree(frtag); 
                                          
 if(nParam > 0)
   {for(i=0; i<nParam; i++) {if(pNames[i] != NULL) free(pNames[i]);}
    free(pMin);
    free(pMax);
    free(pNames);}

 return(root);}

/*------------------------------------------------------ FrSimEventReadTF1--*/
FrSimEvent *FrSimEventReadTF1(FrFile *iFile,  char *name,
                              double tStart,  double length, int readData,  
                              char *pName1, double min1, double max1)
/*---------------------------------------------------------------------------*/
{
 return(FrSimEventReadTF(iFile, name, tStart, length, readData, 1, 
                         pName1, min1, max1));}

/*------------------------------------------------------ FrSimEventReadTF2--*/
FrSimEvent *FrSimEventReadTF2(FrFile *iFile,  char *name,
                              double tStart,  double length, int readData,  
                              char *pName1, double min1, double max1,
                              char *pName2, double min2, double max2)
/*---------------------------------------------------------------------------*/
{
 return(FrSimEventReadTF(iFile, name, tStart, length, readData, 2, 
                         pName1, min1, max1,
                         pName2, min2, max2));}

/*-------------------------------------------------------FrSimEventSetParam--*/
void FrSimEventSetParam(FrSimEvent *event,  
                        char *name,  
                        double value)
/*---------------------------------------------------------------------------*/
{
  int id; 

  if(event == NULL) return; 
  if(name  == NULL) return; 

  id = FrSimEventGetParamId(event, name);

  if(id < 0) FrSimEventAddParam(event, name, value); 
  else       event->parameters[id] = value;

  return;} 

/*----------------------------------------------------------FrSimEventWrite--*/
void FrSimEventWrite(FrSimEvent *simEvent,
                     FrFile *oFile)
/*---------------------------------------------------------------------------*/
{
 if(oFile->toc != NULL) FrTOCevtMark(oFile, &(oFile->toc->simEvt), 
    simEvent->name, simEvent->GTimeS, simEvent->GTimeN, simEvent->amplitude);

 FrPutNewRecord(oFile, simEvent, FR_YES);
 FrPutSChar (oFile, simEvent->name);
 FrPutSChar (oFile, simEvent->comment);
 FrPutSChar (oFile, simEvent->inputs);
 FrPutIntU  (oFile, simEvent->GTimeS);
 FrPutIntU  (oFile, simEvent->GTimeN);
 FrPutFloat (oFile, simEvent->timeBefore);
 FrPutFloat (oFile, simEvent->timeAfter);
 FrPutFloat (oFile, simEvent->amplitude);
 FrPutShortU(oFile, simEvent->nParam);
 if(simEvent->nParam > 0)   
   {if(FrFormatVersion == 6)
          FrPutVFD(oFile, simEvent->parameters,simEvent->nParam);
     else FrPutVD (oFile, simEvent->parameters,simEvent->nParam);
    FrPutVQ (oFile, simEvent->parameterNames, simEvent->nParam);}
 FrPutStruct(oFile, simEvent->data);
 FrPutStruct(oFile, simEvent->table);
 FrPutStruct(oFile, simEvent->next);
 FrPutWriteRecord(oFile, FR_NO);
                                   /*--------------------write the data -----*/

 FrVectWrite (simEvent->data,  oFile);
 FrTableWrite(simEvent->table, oFile);

                                   /*------- write next data bloc if any ----*/

 if(simEvent->next != NULL) FrSimEventWrite(simEvent->next, oFile);
  
 return;}

/*------------------------------------------------------------FrStatDataAdd--*/
void FrStatDataAdd(FrDetector *detector,
                   FrStatData *sData)
/* There should be only one static data with a given name and 
   time boundaries in the program. 
   This function remove the existing static data structure.                  */
/*---------------------------------------------------------------------------*/
{ 	 
 if(detector  == NULL) 
     {FrStatDataFree(sData);
      return;}
      
 if(sData == NULL) {return;}
 sData->detector = detector;
 
 FrStatDataAddR(&(detector->sData), sData);

 return;}
/*------------------------------------------------------------FrStatDataAddR-*/
void FrStatDataAddR(FrStatData **root,
                    FrStatData *sData)
/* There should be only one static data with a given name and 
   time boundaries in the list starting by root. 
   This function remove the existing static data structure.                  */
/*---------------------------------------------------------------------------*/
{FrStatData **cur;
 	 
 if(sData == NULL) {return;}
 
 for(cur = root; *cur != NULL; cur = &((*cur)->next))
   {if(strcmp((*cur)->name, sData->name) != 0) continue;
    if((*cur)->timeStart != sData->timeStart)  continue;
    if((*cur)->timeEnd   != sData->timeEnd)    continue;
    if(sData->version > (*cur)->version) 
          {sData->next = FrStatDataFreeOne(*cur);
           *cur = sData;}
    else
          {FrStatDataFreeOne(sData);}
    return;}

 sData->next = *root;
 *root = sData;

 return;}

/*---------------------------------------------------------------------------*/
void FrStatDataAddVect(FrStatData *stat,  
                          FrVect *vect)
/*---------------------------------------------------------------------------*/
/* Attach the vector to this static data. The user must NOT free the vector  */
/*---------------------------------------------------------------------------*/
{FrVect **data;

 if(stat == NULL) return;

 data = &(stat->data);
 while(*data != NULL) {data = &((*data)->next);}
 
 *data = vect;
 
 return;}

/*----------------------------------------------------------FrStatDataChkT--*/
void FrStatDataChkT(FrStatData **root,
                    unsigned int timeStart,
                    unsigned int timeEnd)
/*--------------------------------------------------------------------------*/
/* This function remove all static data which are not valid for 
   requested validity range (RVR). The rule is the following:
     -remove all data with a validy range not overlapping the RVR
     -keep all data with a validy range partially overlapping the RVR 
     -keep the latest (ie with the latest starting time) data 
      with a validy range fully overlapping the RVR
   Note that timeEnd == 0 means no upper bound limit.
----------------------------------------------------------------------------*/
{FrStatData **current, **name;
 unsigned int cts,cte;
 	 
  if(root == NULL) return;
     
    /*-- we first remove all data not in the validity range ----------------*/
    /*-- and we flag all the data with full overlapp. ----------------------*/

 for(current = root; *current != NULL;)
   {cts = (*current)->timeStart;
    cte = (*current)->timeEnd;
    if(cte == 0) cte = timeEnd; 

    /*-- we first remove all data not in the validity range ----------------*/

    if     (timeEnd   < cts) {*current = FrStatDataFreeOne(*current);}
    else if(timeStart > cte) {*current = FrStatDataFreeOne(*current);}

    /*-- then we flag all the data with full overlapp. ---------------------*/

    else
      {if((timeStart > cts) && (timeEnd   < cte))
          {(*current)->overlap = 1;}
      else{(*current)->overlap = 0;}
      current = &(*current)->next;}}

      /*------ then we loop on all possible names --------------------------*/
      
 for(name = root; *name != NULL; name = &(*name)->next)
   {if((*name)->overlap != 1) continue; 

      /*------ we search for the latest fully overlapping  -----------------*/

    for(current = &((*name)->next); 
        *current != NULL; 
        current = &(*current)->next)
      {if((*current)->overlap != 1)                      continue;
       if(strcmp((*current)->name, (*name)->name) != 0)  continue;
       if((*name)->timeStart < (*current)->timeStart) 
                {(*name)->overlap = 2;}
       else  {(*current)->overlap = 2;}}}

      /*--------------- now remove the tagged blocs ------------------------*/

 for(current = root; *current != NULL;)
      {if((*current)->overlap == 2)
            {*current = FrStatDataFreeOne(*current);}
       else  {current = &(*current)->next;}} 
 
 return;}

/*----------------------------------------------------------FrStatDataCopy--*/
FrStatData *FrStatDataCopy(FrStatData *sData,
                           FrDetector *detector)
/*--------------------------------------------------------------------------*/
{FrStatData *copy;

 if(sData == NULL) return(NULL);

 copy = (FrStatData *) calloc(1,sizeof(FrStatData));
 if(copy == NULL)  return(NULL);
 copy->classe = FrStatDataDef();

 memcpy(copy, sData, sizeof(FrStatData));
 copy->detector = detector;

 if(FrStrCpy(&copy->name,    sData->name)    == NULL) return(NULL);
 if(FrStrCpy(&copy->comment, sData->comment) == NULL) return(NULL);
 if(FrStrCpy(&copy->detName, sData->detName) == NULL) return(NULL);
 if(FrStrCpy(&copy->representation, 
             sData->representation) == NULL) return(NULL);

 if(sData->data  != NULL) {copy->data  = FrVectCopy(sData->data);}
 if(sData->table != NULL) {copy->table = FrTableCopy(sData->table);}

 copy->next = detector->sData;
 detector->sData = copy;
    
 return(copy);}

/*------------------------------------------------------------FrStatDataDef--*/
FrSH *FrStatDataDef()
/*---------------------------------------------------------------------------*/
{static FrSH *classe = NULL;

 if(classe != NULL) return(classe);
 classe = FrDicAddS("FrStatData",(void (*)())FrStatDataRead);

 FrSENew(classe, "STRING", "name","-");
 FrSENew(classe, "STRING", "comment","-");
 FrSENew(classe, "STRING", "representation","-"); 
 FrSENew(classe, "INT_4U", "timeStart","-"); 
 FrSENew(classe, "INT_4U", "timeEnd","-"); 
 FrSENew(classe, "INT_4U", "version","-"); 
 FrSENew(classe, "PTR_STRUCT(FrDetector *)", "detector","-"); 
 FrSENew(classe, "PTR_STRUCT(FrVect *)", "data","-"); 
 FrSENew(classe, "PTR_STRUCT(FrTable *)", "table","-"); 
 if(FrFormatVersion >= 8) FrSENew(classe, "INT_4U", "chkSum","-");

 return(classe);}
/*-----------------------------------------------------------FrStatDataDump--*/
void FrStatDataDump(FrStatData *sData, 
                    FILE *fp, 
                    int debugLvl)
/*---------------------------------------------------------------------------*/
{
 if(debugLvl < 1)  return;
 if(sData == NULL) return;
 if(fp    == NULL) return;

 fprintf(fp," StaticData: %s ", sData->name);
 if(sData->comment != NULL) fprintf(fp," (%s)", sData->comment);
 fprintf(fp,"\n  valid from GPS=%d (UTC=%s)",
               sData->timeStart, FrStrUTC(sData->timeStart, 0));
 fprintf(fp," to %d (%s) version %d\n",
	       sData->timeEnd, FrStrUTC(sData->timeEnd, 0),sData->version);
 if(sData->data  != NULL) FrVectDump (sData->data ,fp, debugLvl);
 if(sData->table != NULL) FrTableDump(sData->table,fp, debugLvl);

 return;}

/*-----------------------------------------------------------FrStatDataFind--*/
FrStatData *FrStatDataFind(FrDetector *detector,
                           char *name, 
                           unsigned int timeNow)
/*---------------------------------------------------------------------------*/
{FrStatData *current, *latest;
 	 
 if(detector == NULL) return(NULL);
 if(name     == NULL) return(NULL);
 
 latest = NULL;
 current = detector->sData;
 while(current != NULL)
   {if(strcmp(current->name, name) == 0)
     {if(timeNow == 0) {return(current);}
      if(current->timeStart <= timeNow)
         {if(latest == NULL) {latest = current;}
          if(latest->timeStart < current->timeStart) {latest = current;}}}

    current = current->next;}
 
 return(latest);} 

/*-----------------------------------------------------------FrStatDataFree--*/
void FrStatDataFree(FrStatData *sData)
/*---------------------------------------------------------------------------*/
{
 if(sData == NULL) return;
 if(sData->next != NULL) FrStatDataFree(sData->next);

 if(sData->name           != NULL) {free(sData->name);}
 if(sData->comment        != NULL) {free(sData->comment);}
 if(sData->detName        != NULL) {free(sData->detName);}
 if(sData->representation != NULL) {free(sData->representation);}
 
 if(sData->data  != NULL) FrVectFree(sData->data);
 if(sData->table != NULL) FrTableFree(sData->table);
   
 free(sData);

 return;}

/*--------------------------------------------------------FrStatDataFreeOne--*/
FrStatData *FrStatDataFreeOne(FrStatData *sData)
/*---------------------------------------------------------------------------*/
{FrStatData *next;

 if(sData == NULL) return(NULL);
 next = sData->next;

 if(sData->name           != NULL) {free(sData->name);}
 if(sData->comment        != NULL) {free(sData->comment);}
 if(sData->representation != NULL) {free(sData->representation);}
 if(sData->detName        != NULL) {free(sData->detName);}
 
 if(sData->data  != NULL) FrVectFree(sData->data);
 if(sData->table != NULL) FrTableFree(sData->table);
   
 free(sData);

 return(next);}
/*--------------------------------WARNING:OBSOLETE FUNCTION---FrStatDataGetV--*/
FrVect *FrStatDataGetV(FrameH *frame, char *name)
/*---------------------------------------------------------------------------*/
{return(FrameFindStatVect(frame,name));}

/*------------------------------------------------------------FrStatDataNew--*/
FrStatData *FrStatDataNew(char *name,
                          char *comment,
                          char *representation,
                          unsigned int tStart,
                          unsigned int tEnd,
                          unsigned int version,
                          FrVect *data,
                          FrTable *table)
/*---------------------------------------------------------------------------*/
{FrStatData *sData;

 sData = (FrStatData *) calloc(1,sizeof(FrStatData));
 if(sData == NULL)  return(NULL);
 sData->classe = FrStatDataDef();

 if(FrStrCpy(&sData->name, name)                     == NULL) return(NULL);
 if(FrStrCpy(&sData->comment, comment)               == NULL) return(NULL);
 if(FrStrCpy(&sData->representation, representation) == NULL) return(NULL);

 sData->timeStart = tStart;
 sData->timeEnd   = tEnd;
 sData->version   = version;
 sData->data      = data;
 sData->table     = NULL;
   
 return(sData);}

/*-----------------------------------------------------------FrStatDataRead--*/
FrStatData *FrStatDataRead(FrFile *iFile)
/*---------------------------------------------------------------------------*/
{FrStatData *sData;
 unsigned int id;
 unsigned short id2;

 if(iFile->fmtVersion == 3) return(FrBack3StatDataRead(iFile));

 sData = (FrStatData *) calloc(1,sizeof(FrStatData));
 if(sData == NULL)  
      {iFile->error = FR_ERROR_MALLOC_FAILED;
       return(NULL);}
 sData->classe = FrStatDataDef();

 if(iFile->fmtVersion > 5) 
      FrReadIntU  (iFile, &id);    /*-statData are not referenced by pointer-*/
 else FrReadShortU(iFile, &id2);   /*-statData are not referenced by pointer-*/
 FrReadSChar (iFile, &sData->name);
 FrReadSChar (iFile, &sData->comment); 
 FrReadSChar (iFile, &sData->representation); 
 FrReadIntU  (iFile, &sData->timeStart); 
 FrReadIntU  (iFile, &sData->timeEnd); 
 FrReadIntU  (iFile, &sData->version); 
 FrReadStruct(iFile, &sData->detector); 
 FrReadStruct(iFile, &sData->data);
 FrReadStruct(iFile, &sData->table);
 if(iFile->fmtVersion >=8) FrReadStructChksum(iFile);

 sData->next = iFile->sDataCur;
 iFile->sDataCur = sData;

 if(FrDebugLvl > 2) fprintf(FrFOut, " %s\n", sData->name);

 return(sData);}
 
/*----------------------------------------------------------FrStatDataReadT--*/
FrStatData* FrStatDataReadT(FrFile *iFile,
                            char *name,
                            double gtime)
/*---------------------------------------------------------------------------*/
{FrStatData *sData, *root;
 FrTOCstat *sm;
 unsigned int i;
 FRLONG position;

  if(name == NULL) return(NULL);
  if(iFile->toc == NULL)  FrTOCReadFull(iFile);
  if(iFile->toc == NULL)  return(NULL);
 
  root = NULL;
  position = 0;

  for(sm = iFile->toc->stat; sm != NULL; sm=sm->next)
    {if(strcmp(sm->name, name) != 0) continue;
     for(i=0; i<sm->nStat; i++)
      {if(sm->tStart[i] > gtime) continue;
       if(sm->tEnd[i]   < gtime) continue;

       if(position == 0) position = FrIOTell(iFile->frfd);
       iFile->relocation = FR_NO;  
       if(FrFileIOSet(iFile, sm->position[i]) == -1) return(NULL);

       FrFileIGoToNextRecord(iFile);
       sData = FrStatDataRead(iFile);

       if(FrFileIGoToNextRecord(iFile) != iFile->vectorType) 
            sData->data = NULL;
       else sData->data = FrVectRead(iFile); 

       iFile->relocation = FR_YES;

       sData->next = root;
       root = sData;}}

  if(position != 0) FrFileIOSet(iFile, position);
            
  return(root);}

/*---------------------------------------------------------FrStatDataRemove--*/
void FrStatDataRemove(FrDetector *detector,
                      char *name) 
/*---------------------------------------------------------------------------*/
{FrStatData **current;

 if(detector == NULL) return;
 if(name     == NULL) return;
  
          /*-------------- search for an existing structure -----------------*/

 current = &detector->sData;
 while(*current != NULL)
   {if(strcmp((*current)->name, name) == 0)
       {*current = FrStatDataFreeOne(*current);}
    else
       {current = &(*current)->next;}}
 
 return;}

/*----------------------------------------------------------FrStatDataTouch--*/
void FrStatDataTouch(FrStatData * sData)
/*---------------------------------------------------------------------------*/
{
 sData->version++;
   
 return;}

/*----------------------------------------------------------FrStatDataWrite--*/
void FrStatDataWrite(FrStatData *sData,
                     FrFile *oFile)
/*---------------------------------------------------------------------------*/
{FrSH *classe;

 if(sData == NULL) return;
 	  
          /*--------------- write the bloc ----------------------------------*/
    
 if(FrDebugLvl > 1) fprintf(FrFOut,"  Output FrStatData %s\n",sData->name);

          /*-----------------is is to make happy the dictionary functions---*/

 classe = sData->classe;
 if(classe == NULL) {FrError(3,"FrStatDataWrite","classe == NULL");}
 else FrDicAssignId(oFile, classe->id, sData);                

            /*------------------------ copy the structure elements-----------*/

 FrPutNewRecord(oFile, sData, FR_YES);
 FrPutSChar (oFile, sData->name);
 FrPutSChar (oFile, sData->comment); 
 FrPutSChar (oFile, sData->representation); 
 FrPutIntU  (oFile, sData->timeStart); 
 FrPutIntU  (oFile, sData->timeEnd); 
 FrPutIntU  (oFile, sData->version); 
 FrPutStruct(oFile, sData->detector); 
 FrPutStruct(oFile, sData->data);
 FrPutStruct(oFile, sData->table);
 FrPutWriteRecord(oFile, FR_NO);
  
           /*-------------write the sData components ------------------------*/
 
 if(sData->data  != NULL) FrVectWrite( sData->data,  oFile);
 if(sData->table != NULL) FrTableWrite(sData->table, oFile);
  
 return;}

/*-----------------------------------------------------------------FrStrCpy--*/
char *FrStrCpy(char **copy,
               char *old)
/*---------------------------------------------------------------------------*/
{
 if(old == NULL) 
    {*copy = NULL;
     return((char *) 1);}
 
 *copy = (char *) malloc(strlen(old)+1);  
 if(*copy == NULL) 
     {FrError(3,"FrStrCpy","malloc failed at line");} 
 else{strcpy(*copy ,old);}
 
 return(*copy);}

/*---------------------------------------------------------------FrStrGTime--*/
char *FrStrGTime(unsigned int in)
/*---------------------------------------------------------------------------*/
/* this function format a GPS time                                           */
/* the GPS time origin is Sun Jan  6 00:00:00 1980                           */
/* the C function time origin is Jan 1 00:00:00 1970 (no leap seconds)       */
/* (C time - GPS time = 3600*24*3567 = 315964800 seconds = FRGPSOFF)         */
/*---------------------------------------------------------------------------*/
{static char stringTime[30];
 time_t tp;
 
 tp = in + FRGPSOFF;
 strncpy(stringTime,asctime(gmtime(&tp)),24);

 return(stringTime);}

/*--------------------------------------------------------FrStrcmpAndPrefix--*/
int FrStrcmpAndPrefix(char* name1, char *name2)
/*---------------------------------------------------------------------------*/
/* This function compared to name, with or whout prefix                      */
/*---------------------------------------------------------------------------*/
{int prefixLen;

 if(name1 == NULL) return(-1);
 if(name2 == NULL) return(-2);

 if(FrFlipPrefix == NULL) return(strcmp(name1, name2));

 if(strcmp(name1, name2) == 0) return(0);

 /*--- if the test failed, we do the test without the prefix --*/

 prefixLen = strlen(FrFlipPrefix);
  if(strncmp(name1,FrFlipPrefix, prefixLen) == 0)
   {if(strlen(name1) > prefixLen) name1 += prefixLen;}

 if(strncmp(name2,FrFlipPrefix, prefixLen) == 0)
   {if(strlen(name2) > prefixLen) name2 += prefixLen;}

 return(strcmp(name1, name2));
}

/*----------------------------------------------------------FrStrFlipPrefix--*/
char* FrStrFlipPrefix(char *name)
/*---------------------------------------------------------------------------*/
/* This function try to add a prefix or to remove if it is already there     */
/*---------------------------------------------------------------------------*/
{char *newName;
 int lenName, lenPrefix;

 if(name == NULL) return(NULL);

 lenName   = strlen(name);
 lenPrefix = strlen(FrFlipPrefix);
 newName = malloc(lenName + lenPrefix + 1);
 if(newName == NULL) return(NULL);

 if(strncmp(name, FrFlipPrefix, lenPrefix) == 0)
      {sprintf(newName,"%s",name+lenPrefix);}
 else {sprintf(newName,"%s%s", FrFlipPrefix, name);}

 return(newName);}

/*----------------------------------------------------------FrStrSetPrefix--*/
int FrStrSetPrefix(char **name, char *prefix) 
/*--------------------------------------------------------------------------*/
/* This function add the prefix "prefix" to the beginning of the name is the
   name is not yet starting with this prefix. It return 0 in case of success*/
/*--------------------------------------------------------------------------*/
{char *newName;
 int prefixLength;

 if(name   == NULL) return(1);
 if(*name  == NULL) return(1);
 if(prefix == NULL) return(1);

  prefixLength = strlen(prefix);
 if(strncmp(*name,prefix,prefixLength) == 0) return(0);

 newName = malloc(strlen(*name) + prefixLength + 1);
 if(newName == NULL) return(2);

 sprintf(newName,"%s%s", prefix, *name);
 free(*name);
 *name = newName;

 return(0);}

/*----------------------------------------------------------------FrGPS2UTC--*/
time_t FrGPS2UTC(unsigned int gps,
                 int uLeapS)
/*---------------------------------------------------------------------------*/
/* this function converts a GPS time to a UTC time                           */
/* the GPS time origin is Sun Jan  6 00:00:00 1980                           */
/* the C function time origin is Jan 1 00:00:00 1970 (no leap seconds)       */
/* (C time - GPS time = 3600*24*3567 = 315964800 seconds = FRGPSOFF)         */
/*---------------------------------------------------------------------------*/
{
 time_t utc;
 
 if(uLeapS == 0) uLeapS = FRGPSLEAPS;             /*-- current best guess ---*/

    /*---- update leap second according the table if available ----*/

 if  (gps > 1025136015);                /* Sun Jul  1 00:00:00 2012*/
 else if(gps > 914803214) uLeapS = 34;  /* Sun Jan  1 00:00:00 2009*/
 else if(gps > 820108813) uLeapS = 33;  /* Sun Jan  1 00:00:00 2006*/
 else if(gps > 599184012) uLeapS = 32;  /* Fri Jan  1 00:00:00 1999*/
 else if(gps > 551750411) uLeapS = 31;  /* Tue Jul  1 00:00:00 1997*/
 else if(gps > 504489610) uLeapS = 30;  /* Mon Jan  1 00:00:00 1996*/
 else if(gps > 457056009) uLeapS = 29;  /* Fri Jul  1 00:00:00 1994*/
 else if(gps > 425520008) uLeapS = 28;  /* Thu Jul  1 00:00:00 1993*/

 utc = gps + FRGPSTAI - uLeapS + FRGPSOFF;

 return(utc);}

/*-----------------------------------------------------------------FrStrUTC--*/
char *FrStrUTC(unsigned int gps,
               int uLeapS)
/*---------------------------------------------------------------------------*/
/* this function format a GPS time to a UTC time (string)                    */
/*---------------------------------------------------------------------------*/
{static char stringTime[30];
 time_t utc;
 
 utc = FrGPS2UTC(gps, uLeapS);
 strncpy(stringTime,asctime(gmtime(&utc)),24);

 return(stringTime);}

/*-------------------------------------------------------------FrSummaryDef--*/
FrSH *FrSummaryDef()
/*---------------------------------------------------------------------------*/
{static FrSH *classe = NULL;

 if(classe != NULL) return(classe);
 classe = FrDicAddS("FrSummary",(void (*)())FrSummaryRead);

 FrSENew(classe, "STRING", "name","-");
 FrSENew(classe, "STRING", "comment","-");
 FrSENew(classe, "STRING", "test","-");
 FrSENew(classe, "INT_4U", "GTimeS","-");
 FrSENew(classe, "INT_4U", "GTimeN","-");
 FrSENew(classe, "PTR_STRUCT(FrVect *)",  "moments","-");
 FrSENew(classe, "PTR_STRUCT(FrTable *)", "table","-");
 FrSENew(classe, "PTR_STRUCT(FrSummary *)", "next","-"); 
 if(FrFormatVersion >= 8) FrSENew(classe, "INT_4U", "chkSum","-");

 return(classe);}

/*------------------------------------------------------------FrSummaryDump--*/
void FrSummaryDump(FrSummary *summary,
                   FILE *fp, 
                   int debugLvl)
/*---------------------------------------------------------------------------*/
{
 if(summary == NULL) return;
 if(fp == NULL)   return;
 if(debugLvl < 0) return;

 fprintf(fp,"Summary: %s GTimeS=%d N=%d",  
        summary->name, summary->GTimeS, summary->GTimeN);
 if(summary->comment != NULL) fprintf(fp," (comment:%s)",summary->comment);
 fprintf(fp,"\n");
 if(summary->test    != NULL) fprintf(fp,"  test:%s\n",summary->test); 
 if(summary->moments != NULL){FrVectDump (summary->moments,  fp, debugLvl);}
 if(summary->table   != NULL){FrTableDump(summary->table,    fp, debugLvl);}

 return;}

/*------------------------------------------------------------FrSummaryFind--*/
FrSummary *FrSummaryFind(FrameH *frameH, 
                         char *name)
/*---------------------------------------------------------------------------*/
{FrSummary *sum;

 if(frameH == NULL) return(NULL);
 if(name   == NULL) return(NULL);

 for(sum = frameH->summaryData; sum != NULL; sum = sum->next)
    {if(strcmp(sum->name, name) == 0) return (sum);}
  
 return (NULL);}


/*------------------------------------------------------------FrSummaryFree--*/
void FrSummaryFree(FrSummary *summary)
/*---------------------------------------------------------------------------*/
{
 if(summary == NULL) return;

 if(summary->next != NULL) FrSummaryFree(summary->next);

 if(summary->name    != NULL) free (summary->name);
 if(summary->comment != NULL) free (summary->comment);
 if(summary->test    != NULL) free (summary->test);
 if(summary->moments != NULL) FrVectFree (summary->moments);
 if(summary->table   != NULL) FrTableFree(summary->table);

 free(summary);

 return;}
/*--------------------------------WARNING:OBSOLETE FUNCTION---FrSummaryGetV--*/
FrVect *FrSummaryGetV(FrameH *frame, char *name)
/*---------------------------------------------------------------------------*/
{return(FrameFindSumVect(frame,name));}

/*-------------------------------------------------------------FrSummaryNew--*/
FrSummary *FrSummaryNew(FrameH *frameH,  
                        char *name, 
                        char *comment, 
                        char *test, 
                        FrVect *moments,
                        FrTable *table)
/*---------------------------------------------------------------------------*/
{FrSummary *summary;
 
 summary = (FrSummary *) calloc(1,sizeof(FrSummary));
 if(summary == NULL) return(NULL); 
 summary->classe = FrSummaryDef();
	 
 if(FrStrCpy(&summary->name,name)       == NULL) return(NULL);
 if(FrStrCpy(&summary->comment,comment) == NULL) return(NULL);
 if(FrStrCpy(&summary->test,test)       == NULL) return(NULL);

 summary->moments = moments;
 summary->table   = table;
 
             /*--------------- now store it in the Frame structures ---------*/

 if(frameH != NULL) FrameAddSum(frameH, summary);
  
 return(summary);} 

/*------------------------------------------------------------FrSummaryRead--*/
FrSummary *FrSummaryRead(FrFile *iFile)
/*---------------------------------------------------------------------------*/
{FrSummary *summary;
 char message[128];

 if(iFile->fmtVersion == 3) return(FrBack3SummaryRead(iFile));
 if(iFile->fmtVersion == 4) return(FrBack4SummaryRead(iFile));

 summary = (FrSummary *) calloc(1,sizeof(FrSummary));
 if(summary == NULL) 
      {iFile->error = FR_ERROR_MALLOC_FAILED;
       return(NULL);}
 summary->classe = FrSummaryDef();

 FrReadHeader(iFile,  summary);
 FrReadSChar (iFile, &summary->name); 
 FrReadSChar (iFile, &summary->comment);
 FrReadSChar (iFile, &summary->test);
 FrReadIntU  (iFile, &summary->GTimeS);
 FrReadIntU  (iFile, &summary->GTimeN);
 FrReadStruct(iFile, &summary->moments);
 iFile->vectInstance = iFile->instance;
 FrReadStruct(iFile, &summary->table);
 FrReadStruct(iFile, &summary->next);
 if(iFile->fmtVersion >=8) FrReadStructChksum(iFile);

 if(iFile->nBytes != iFile->nBytesR + iFile->length)
     {sprintf(message,": Record length error: "
         "nBytes=%"FRLLD" nBytesR=%"FRLLD" length=%"FRLLD"\n",
          iFile->nBytes, iFile->nBytesR, iFile->length);
      FrError(3,"FrSummaryRead",message);
      return(NULL);}     

 if(FrDebugLvl > 2) fprintf(FrFOut," %s\n", summary->name);

 return(summary);}

/*---------------------------------------------------------- FrSummaryReadT--*/
FrSummary *FrSummaryReadT(FrFile *iFile,
                          char *name,
                          double gtime)
/*---------------------------------------------------------------------------*/
{FrSummary  **current, *sum, *root;
 FrTOCts *ts;
 FrTag *frtag;
 int index;

 if(name == NULL) return(NULL);
 frtag = FrTagNew(name);
 if(frtag == NULL) return(NULL);

          /*------- find frame(it will read the FrTOC if needed) ------------*/

 index = FrTOCFrameFindT(iFile, gtime);
 if(index <0) return(NULL);
                                             /*------ find sum mark ---------*/

 root = NULL;
 current = &root;

 for(ts = iFile->toc->summary; ts != NULL; ts = ts->next)

                          /*-------- check if we need to copy this sum ?-----*/

   {if(FrTagMatch(frtag, ts->name) == FR_NO) continue; 

                        /*---- set the file pointer and read the data -------*/

    if(FrTOCSetPos(iFile, ts->position[index]) != 0) {return (NULL);}
    sum = FrSummaryRead(iFile);
    if(sum == NULL) return(NULL);

    sum->moments = FrVectReadNext(iFile, sum->GTimeS + 
                                  1.e-9* sum->GTimeN, sum->name);
    *current = sum;
    current = &(sum->next);}

 FrTagFree(frtag); 

 return(root);}

/*-----------------------------------------------------------FrSummaryWrite--*/
void FrSummaryWrite(FrSummary *summary,
                    FrFile *oFile)
/*---------------------------------------------------------------------------*/
{
 FrPutNewRecord(oFile, summary, FR_YES);
 FrPutSChar (oFile, summary->name);
 FrPutSChar (oFile, summary->comment);
 FrPutSChar (oFile, summary->test);
 FrPutIntU  (oFile, summary->GTimeS);
 FrPutIntU  (oFile, summary->GTimeN);
 FrPutStruct(oFile, summary->moments);
 FrPutStruct(oFile, summary->table);
 FrPutStruct(oFile, summary->next);

 if(oFile->toc != NULL) FrTOCtsMark(oFile, &oFile->toc->summaryH,
                                             summary->name, 0, 0xffffffff);

 FrPutWriteRecord(oFile, FR_NO);

 if(summary->moments) FrVectWrite(summary->moments, oFile);
 if(summary->table)   FrTableWrite(summary->table, oFile);

 if(summary->next != NULL) FrSummaryWrite(summary->next, oFile);
 
 return;}

/*---------------------------------------------------------FrTableCompress---*/
void FrTableCompress(FrTable *table,
                    int compType,
                    int gzipLevel)
/*---------------------------------------------------------------------------*/
{
 if(table == NULL) return;

 if(table->next != NULL) FrTableCompress(table->next, compType, gzipLevel);

 FrVectCompress(table->column, compType, gzipLevel);

 return;}

/*--------------------------------------------------------------FrTableCopy--*/
FrTable *FrTableCopy(FrTable *in)
/*---------------------------------------------------------------------------*/
{FrTable *table;
 int i;
 
 if(in == NULL) return(NULL);
 table = (FrTable *) calloc(1,sizeof(FrTable));
 if(table == NULL) return(NULL); 
 table->classe = FrTableDef();

	 
 if(FrStrCpy(&table->name,      in->name)       == NULL) return(NULL);
 if(FrStrCpy(&table->comment,   in->comment)    == NULL) return(NULL);

 table->nColumn = in->nColumn;
 table->nRow    = in->nRow;

 table->columnName = (char **) calloc(in->nColumn, sizeof(char *));
 if(table->column == NULL) return(NULL);

 for(i=0; i<table->nColumn; i++)
   {if(FrStrCpy(&table->columnName[i],in->columnName[i])==NULL) return(NULL);}

 table->column = FrVectCopy(in->column);
 if(table->column == NULL) return (NULL);
  
 return(table);} 
/*-------------------------------------------------------------- FrTableDef--*/
FrSH *FrTableDef()
/*---------------------------------------------------------------------------*/
{static FrSH *classe = NULL;

 if(classe != NULL) return(classe);
 classe = FrDicAddS("FrTable",FrTableRead);

 FrSENew(classe, "STRING", "name","-");
 FrSENew(classe, "STRING", "comment","-");
 FrSENew(classe, "INT_2U", "nColumn","-");
 FrSENew(classe, "INT_4U", "nRow","-");
 FrSENew(classe, "STRING[nColumn]", "columnName", "-");
 FrSENew(classe, "PTR_STRUCT(FrVect *)",   "column","-");
 FrSENew(classe, "PTR_STRUCT(FrTable *)", "next","-"); 
 if(FrFormatVersion >= 8) FrSENew(classe, "INT_4U", "chkSum","-");

 return(classe);}
  
/*--------------------------------------------------------------FrTableDump--*/
void FrTableDump(FrTable *table,
                 FILE *fp, 
                 int debugLvl)
/*---------------------------------------------------------------------------*/
{FrVect *col;
 int i;
 char *dC, **dSt;
 short  *dS;
 int    *dI;  
 FRLONG *dL; 
 float  *dF;  
 double *dD;
 unsigned char   *dU;
 unsigned short  *dUS;
 unsigned int    *dUI;
 FRULONG *dUL;
 
 if(table == NULL) return;
 if(fp == NULL)    return;
 if(debugLvl < 0)  return;

 fprintf(fp,"  Table: %s (%s) \n   nRow=%d nColumn=%d:", 
	 table->name, table->comment, table->nRow, table->nColumn);
 for(i=0; i<table->nColumn; i++)
   {fprintf(fp," %s,",table->columnName[i]);}
 fprintf(fp,"\n");

                                     /*------ dump each column---------------*/
 col = table->column;
 while(col != NULL)
 
   {fprintf(fp,"  col:%s",col->name);
    if     (col->type == FR_VECT_4R) 
      {dF = (float *) col->data;
       fprintf(fp,"(F) %g %g %g ...\n", dF[0],dF[1],dF[2]);}
    else if(col->type == FR_VECT_8R)  
      {dD = (double *) col->data;
       fprintf(fp,"(D) %g %g %g ...\n", dD[0],dD[1],dD[2]);}
    else if(col->type == FR_VECT_C)  
      {dC = col->data;
       fprintf(fp,"(C) %d %d %d %d %d ...\n", 
                             dC[0],dC[1],dC[2],dC[3],dC[4]);}
    else if(col->type == FR_VECT_2S)  
      {dS = (short *) col->data;
       fprintf(fp,"(S) %d %d %d %d ...\n",dS[0],dS[1],dS[2],dS[3]);}
    else if(col->type == FR_VECT_8S)  
      {dL = (FRLONG *) col->data;
       fprintf(fp,"(L) %"FRLLD" %"FRLLD" %"FRLLD" ...\n", dL[0],dL[1],dL[2]);}
    else if(col->type == FR_VECT_4S)  
      {dI = (int *) col->data;
       fprintf(fp,"(I) %d %d %d ...\n", dI[0],dI[1],dI[2]);}
    else if(col->type == FR_VECT_1U)  
      {dU = (unsigned char *)col->dataU;
       fprintf(fp,"(C) %d %d %d ...\n", dU[0],dU[1],dU[2]);}
    else if(col->type == FR_VECT_2U)  
      {dUS = (unsigned short *) col->data;
       fprintf(fp,"(S) %d %d %d ...\n", dUS[0],dUS[1],dUS[2]);}
    else if(col->type == FR_VECT_8U)  
      {dUL = (FRULONG *) col->data;
       fprintf(fp,"(L) %"FRLLD" %"FRLLD" %"FRLLD"...\n", dUL[0],dUL[1],dUL[2]);}
    else if(col->type == FR_VECT_4U)  
      {dUI = (unsigned int *) col->data;
       fprintf(fp,"(I) %d %d %d ...\n", dUI[0],dUI[1],dUI[2]);}
    else if(col->type == FR_VECT_8C) 
      {dF = (float *) col->data;
       fprintf(fp,"(CF) (%g,%g) (%g,%g) ...\n", dF[0],dF[1],dF[2],dF[3]);}
    else if(col->type == FR_VECT_16C) 
      {dD = (double *) col->data;
       fprintf(fp,"(CD) (%g,%g) (%g,%g) ...\n", dD[0],dD[1],dD[2],dD[3]);}
    else if(col->type == FR_VECT_STRING) 
      {dSt = (char **) col->data;
       fprintf(fp,"(STRING) ");
       if((dSt[0] != NULL) &&
          (dSt[1] != NULL)) fprintf(fp,"%s, %s,...", dSt[0], dSt[1]);
        fprintf(fp,"\n");}
    else 
      {fprintf(fp," unknown type: %d \n",col->type );}

   col = col->next;}

 return;}

/*------------------------------------------------------------FrTableExpand--*/
void FrTableExpand(FrTable *table)
/*---------------------------------------------------------------------------*/
{
 if(table == NULL) return;

 if(table->next != NULL) FrTableExpand(table->next);

 FrVectExpandF(table->column);

 return;}

/*--------------------------------------------------------------FrTableFree--*/
void FrTableFree(FrTable *table)
/*---------------------------------------------------------------------------*/
{int i;

 if(table == NULL) return;

 if(table->next != NULL) FrTableFree(table->next);

 if(table->name       != NULL) free (table->name);
 if(table->comment    != NULL) free (table->comment);

 for(i=0; i<table->nColumn; i++)
    {if(table->columnName[i] != NULL) free(table->columnName[i]);}
 free(table->columnName);
 
 FrVectFree(table->column);

 free(table);

 return;}

/*------------------------------------------------------------FrTableGetCol--*/
FrVect* FrTableGetCol(FrTable *table,
                      char *colName)
/*---------------------------------------------------------------------------*/
{FrVect *col;
 
 if(table == NULL) return(NULL);
 if(colName == NULL) return(NULL); 

 for(col = table->column; col!= NULL; col = col->next)
   {if(strcmp(col->name,colName) == 0) return(col);}

 return(NULL);
}
/*---------------------------------------------------------------FrTableNew--*/
FrTable *FrTableNew(char *name, 
                    char *comment,
                    int  nRow,
                    int  nColumn,
                    ...)
/*---------------------------------------------------------------------------*/
{FrTable *table;
 FrVect *vect, *last;
 va_list ap;
 int type, i;
 char *colName;
 
 table = (FrTable *) calloc(1,sizeof(FrTable));
 if(table == NULL) return(NULL); 
 table->classe = FrTableDef();
 
 table->next = NULL;
 if(FrStrCpy(&table->name,name)           == NULL) return(NULL);
 if(FrStrCpy(&table->comment,comment)     == NULL) return(NULL);

 table->nColumn = nColumn;
 table->nRow    = nRow;

 table->columnName = (char **) calloc(nColumn, sizeof(char *));
 if(table->columnName == NULL) return(NULL);

 last = NULL;
 va_start(ap,nColumn);
 for(i=0; i<nColumn; i++)
    {type    = va_arg(ap, int);
     colName = va_arg(ap, char *);
     vect = FrVectNew1D(colName, type, nRow, 0., "NONE", "NONE");
     if(vect == NULL) return(NULL);
     if(colName != NULL)
       {if(FrStrCpy(&table->columnName[i],colName) == NULL) return(NULL);}
     vect->next = last;
     last = vect;}

 va_end(ap);
 table->column = last;

 return(table);} 

/*--------------------------------------------------------------FrTableRead--*/
void FrTableRead(FrFile *iFile)
/*---------------------------------------------------------------------------*/
{FrTable *table;
 char message[128];

 table = (FrTable *) calloc(1,sizeof(FrTable));
 if(table == NULL) 
      {iFile->error = FR_ERROR_MALLOC_FAILED;
       return;}
 table->classe = FrTableDef();

 FrReadHeader(iFile,  table);
 FrReadSChar (iFile, &table->name); 
 FrReadSChar (iFile, &table->comment);
 FrReadShortU(iFile, &table->nColumn);
 FrReadIntU  (iFile, &table->nRow);
 FrReadVQ    (iFile, &table->columnName, table->nColumn);
 FrReadStruct(iFile, &table->column);
 FrReadStruct(iFile, &table->next);
 if(iFile->fmtVersion >=8) FrReadStructChksum(iFile);

 if(iFile->nBytes != iFile->nBytesR + iFile->length)
     {sprintf(message,": Record length error: "
         "nBytes=%"FRLLD" nBytesR=%"FRLLD" length=%"FRLLD"\n",
          iFile->nBytes, iFile->nBytesR, iFile->length);
      FrError(3,"FrTableRead",message);
      return;}     

 if(FrDebugLvl > 2) fprintf(FrFOut," %s\n", table->name);

 return;}

/*-------------------------------------------------------------FrTableWrite--*/
void FrTableWrite(FrTable *table,
                  FrFile *oFile)
/*---------------------------------------------------------------------------*/
{
 if(table == NULL) return;

 FrPutNewRecord(oFile, table, FR_YES);
 FrPutSChar (oFile, table->name);
 FrPutSChar (oFile, table->comment);
 FrPutShortU(oFile, table->nColumn);
 FrPutIntU  (oFile, table->nRow);
 FrPutVQ    (oFile, table->columnName, table->nColumn);
 FrPutStruct(oFile, table->column);
 FrPutStruct(oFile, table->next);
 FrPutWriteRecord(oFile, FR_NO);

 FrVectWrite(table->column, oFile);

 if(table->next != NULL) FrTableWrite(table->next, oFile);
  
 return;}

/*----------------------------------------------------------------FrTagFree--*/
void FrTagFree(FrTag *tag)
/*---------------------------------------------------------------------------*/
/* This function free all the space alocated for the tag structure           */
/*---------------------------------------------------------------------------*/
{FrTag *next;

 if(tag != NULL) free(tag->start);

 while(tag != NULL)
   {next = tag->next;
    free(tag);
    tag = next;}
 
 return;}

/*-----------------------------------------------------------FrTagGetSubTag--*/
char* FrTagGetSubTag(char *type, char *tag)
/*---------------------------------------------------------------------------*/
{
  int i, slen;
  char *subTag, *p;

  if(type == NULL) return(NULL);
  if(tag  == NULL) return(NULL);

  /*---------first skip the white space ---*/
  slen = strlen(tag);
  for(i=0; i<slen-1; i++) {
    if(isspace((int)tag[i]) == 0) break;}
  tag += i;

  /*---------------- get the default value ---*/
  if(strchr(tag,'#') == NULL) {
    FrStrCpy(&subTag, tag);
    return(subTag);}

  /*----------------- no type found------*/
  if((p = strstr(tag,type)) != NULL) {
    FrStrCpy(&subTag, p+strlen(type));}
  else if((p = strstr(tag, "#DEFAULT")) != NULL) {
    FrStrCpy(&subTag, p+strlen("#DEFAULT"));}
  else if(tag[0] == '#') {
    FrStrCpy(&subTag, "NONE");}
  else {
    FrStrCpy(&subTag, tag);}

  if(subTag == NULL) return(NULL);
  if((p = strchr(subTag,'#')) != NULL) p[0] = '\0';

  return(subTag);}

/*-----------------------------------------------------------------FrTagNew--*/
FrTag *FrTagNew(char *string)
/*---------------------------------------------------------------------------*/
/* This function is looking for space character to breaks a string in words. */
/* It returns a link list of FrTag structures.                               */
/*---------------------------------------------------------------------------*/
{int i, slen, nWord;
 FrTag *root, *tag, **current;
 char *stringCopy;

 if(string == NULL) return(NULL);

 slen  = strlen(string);
 nWord = 0;
 root  = NULL;
 tag   = NULL;
 current = &root;
                                 /*---------first skip the white space ---*/
 for(i=0; i<slen; i++)
   {if(isspace((int)string[i]) == 0) break;}

 string = string + i;
 slen   = slen   - i;
 if(slen == 0) return(NULL);
                                  /*----- then copy to whole string---------*/
 stringCopy = malloc(slen+1);
 if(stringCopy == NULL) return(NULL);
 memcpy(stringCopy, string, slen);
 stringCopy[slen] = '\0';
                                 /*---------break the string in small one ---*/
 for(i=0; i<slen; i++)
   {if(isspace((int)string[i]) != 0)
       {tag = NULL;
        stringCopy[i] = '\0';}
    else
       {if(tag == NULL)
            {tag = (FrTag *) malloc(sizeof(FrTag));
             if(tag == NULL) return(NULL);
             tag->next = NULL;
             *current = tag;
             current = &(tag->next);

             tag->start = stringCopy+i;
             tag->length = 1;
             if(string[i] == '-')
                  tag->returnValue = FR_NO;
	     else tag->returnValue = FR_YES;}

        else{tag->length++;}}}

 return(root);}

/*---------------------------------------------------------------FrTagMatch--*/
FRBOOL FrTagMatch(FrTag *tag0,
                  char *name)
/*---------------------------------------------------------------------------*/
/* this function returns FR_YES if the name match the tags. (FR_NO otherwise)*/
/* the function try with and without the channel prefix                      */
/*- new logic: it scans all the tag until the end to see if a tag is OK      */
/*---------------------------------------------------------------------------*/
{
  int slen, slenF;
  FrTag *tag;
  FRBOOL match;
  char *nameF;

  if(tag0 == NULL) return(FR_YES);
  if(name == NULL) return(FR_YES);

  slen  = strlen(name);

  if(FrFlipPrefix != NULL) { 
    nameF = FrStrFlipPrefix(name);
    if(nameF == NULL) return(FR_NO);
    slenF = strlen(nameF);}
  else {
    nameF = NULL;
    slenF = 0;}

  match = FR_NO;

  for(tag = tag0; tag != NULL; tag = tag->next) {

    if(tag->returnValue == FR_YES) { /*-------------this is a positive tag---*/
      if(FrTagMatch1(name,  slen,  tag->start, tag->length) == FR_YES)
        match = FR_YES;
      if(nameF != NULL) {
        if(FrTagMatch1(nameF, slenF, tag->start, tag->length) == FR_YES) 
          match = FR_YES;}}

    else {
      if(tag->length < 2) continue; /*------------now this is the anti-tag---*/
      if(FrTagMatch1(name,  slen, tag->start+1, tag->length-1) == FR_YES) 
        match = FR_NO;
      if(nameF != NULL) {
        if(FrTagMatch1(nameF, slen, tag->start+1, tag->length-1) == FR_YES) 
          match = FR_NO;}}}

  if(nameF != NULL) free(nameF);

  return(match);}

/*--------------------------------------------------------------FrTagMatch1--*/
FRBOOL FrTagMatch1(char *name, 
                   int   nameL, 
                   char *tag, 
                   int   tagL)
/*---------------------------------------------------------------------------*/
/* this function returns FR_YES if the name match the tags. (FR_NO otherwise)*/
{int i, j, jStart, iStart;

         /*----loop over the string: we compare each character and 
                      stops the loop as soon as it founds a difference ------*/

 jStart = -1;
 iStart = -1;
 j = 0;
                            /*----- case when the tag finish by a "*" -------*/
 if(tagL == nameL+1)
   {if(tag[tagL-1] == '*')
     {if(strncmp(name, tag, tagL-1) == 0) return(FR_YES);}}

 for(i=0; i<nameL; i++)
   {if(tag[j] != '*')                   /*-------- case without wild card ---*/
      {if(name[i] != tag[j]) 
            {if(jStart >= 0)
                   {j = jStart;
                    i = iStart;}
	      else {return(FR_NO);}}
       else {j++;
             if(j >= tagL)
               {if(i == nameL-1) return(FR_YES);
                else             return(FR_NO);}}}
    
    else 
      {if(j+1 == tagL) return(FR_YES);    /*------- tag end by a '*' --------*/

       if(tag[j+1] == name[i]) {jStart = j;
                                iStart = i;
                                j++;
                                i--;}}}

 if(tagL - j != 0)                        /*---still some character in tag---*/
    {if((tagL == j+1) && (tag[j] == '*')) return(FR_YES);
     else return(FR_NO);}


 return(FR_YES);}

/*---------------------------------------------------------------- FrTOCDef--*/
FrSH *FrTOCDef()    
/*---------------------------------------------------------------------------*/
{static FrSH *classe = NULL;

  if(classe != NULL) return(classe);
  classe = FrDicAddS("FrTOC", FrReadSkipRecord);  

      /*---- remark: in standard read we always skip the FrTOC -----*/

  FrSENew(classe, "INT_2S",  "ULeapS","-");
  FrSENew(classe, "INT_4U",  "nFrame","-");

  if(FrFormatVersion < 8) {
    FrSENew(classe, "*INT_4U","dataQuality","-");
    FrSENew(classe, "*INT_4U","GTimeS","-");
    FrSENew(classe, "*INT_4U","GTimeN","-");
    FrSENew(classe, "*REAL_8","dt","-");
    FrSENew(classe, "*INT_4S","runs","-");
    FrSENew(classe, "*INT_4U","frame","-");
    FrSENew(classe, "*INT_8U","positionH","-");
    FrSENew(classe, "*INT_8U","nFirstADC","-");
    FrSENew(classe, "*INT_8U","nFirstSer","-");
    FrSENew(classe, "*INT_8U","nFirstTable","-");
    FrSENew(classe, "*INT_8U","nFirstMsg","-");

    FrSENew(classe, "INT_4U",  "nSH","-");
    FrSENew(classe, "*INT_2U","SHid","-");
    FrSENew(classe, "*STRING","SHname","-");

    FrSENew(classe, "INT_4U",  "nDetector","-");
    FrSENew(classe, "*STRING","nameDetector","-");
    FrSENew(classe, "*INT_8U","positionDetector","-");

    FrSENew(classe, "INT_4U","nStatType","-");
    FrSENew(classe, "STRING","nameStat","-");
    FrSENew(classe, "STRING","detector","-");
    FrSENew(classe, "INT_4U","nStatInstance","-");
    FrSENew(classe, "*INT_4U","tStart","-");
    FrSENew(classe, "*INT_4U","tEnd","-");
    FrSENew(classe, "*INT_4U","version","-");
    FrSENew(classe, "*INT_8U","positionStat","-");

    FrSENew(classe, "INT_4U","nADC","-");
    FrSENew(classe, "*STRING","name","-");
    FrSENew(classe, "*INT_4U","channelID","-");
    FrSENew(classe, "*INT_4U","groupID","-");
    FrSENew(classe, "*INT_8U","positionADC","-");

    FrSENew(classe, "INT_4U","nProc","-");
    FrSENew(classe, "*STRING","nameProc","-");
    FrSENew(classe, "*INT_8U","positionProc","-");

    FrSENew(classe, "INT_4U","nSim","-");
    FrSENew(classe, "*STRING","nameSim","-");
    FrSENew(classe, "*INT_8U","positionSim","-");

    FrSENew(classe, "INT_4U","nSer","-");
    FrSENew(classe, "*STRING","nameSer","-");
    FrSENew(classe, "*INT_8U","positionSer","-");

    FrSENew(classe, "INT_4U","nSummary","-");
    FrSENew(classe, "*STRING","nameSum","-");
    FrSENew(classe, "*INT_8U","positionSum","-");

    FrSENew(classe, "INT_4U",  "nEventType","-");
    FrSENew(classe, "*STRING","nameEvent","-");
    FrSENew(classe, "*INT_4U","nEvent","-");
    FrSENew(classe, "*INT_4U","GTimeSEvent","-");
    FrSENew(classe, "*INT_4U","GTimeNEvent","-");
    FrSENew(classe, "*REAL_4","amplitudeEvent","-");
    FrSENew(classe, "*INT_8U","positionEvent","-");

    FrSENew(classe, "INT_4U",  "nSimEventType","-");
    FrSENew(classe, "*STRING","nameSimEvent","-");
    FrSENew(classe, "*INT_4U","nSimEvent","-");
    FrSENew(classe, "*INT_4U","GTimeSSim","-");
    FrSENew(classe, "*INT_4U","GTimeNSim","-");
    FrSENew(classe, "*REAL_4","amplitudeSimEvent","-");
    FrSENew(classe, "*INT_8U","positionSimEvent","-");}

  /*------------------- for frame format version 8 ---------*/
  else {
    FrSENew(classe, "INT_4U[nFrame]","dataQuality","-");
    FrSENew(classe, "INT_4U[nFrame]","GTimeS","-");
    FrSENew(classe, "INT_4U[nFrame]","GTimeN","-");
    FrSENew(classe, "REAL_8[nFrame]","dt","-");
    FrSENew(classe, "INT_4S[nFrame]","runs","-");
    FrSENew(classe, "INT_4U[nFrame]","frame","-");
    FrSENew(classe, "INT_8U[nFrame]","positionH","-");
    FrSENew(classe, "INT_8U[nFrame]","nFirstADC","-");
    FrSENew(classe, "INT_8U[nFrame]","nFirstSer","-");
    FrSENew(classe, "INT_8U[nFrame]","nFirstTable","-");
    FrSENew(classe, "INT_8U[nFrame]","nFirstMsg","-");

    FrSENew(classe, "INT_4U",  "nSH","-");
    FrSENew(classe, "INT_2U[nSH]","SHid","-");
    FrSENew(classe, "STRING[nSH]","SHname","-");

    FrSENew(classe, "INT_4U",  "nDetector","-");
    FrSENew(classe, "STRING[nDetector]","nameDetector","-");
    FrSENew(classe, "INT_8U[nDetector]","positionDetector","-");

    FrSENew(classe, "INT_4U","nStatType","-");
    FrSENew(classe, "STRING[nStatType]","nameStat","-");
    FrSENew(classe, "STRING[nStatType]","detector","-");
    FrSENew(classe, "INT_4U[nStatType]","nStatInstance","-");
    FrSENew(classe, "INT_4U","nTotalStat","-");
    FrSENew(classe, "INT_4U[nTotalStat]","tStart","-");
    FrSENew(classe, "INT_4U[nTotalStat]","tEnd","-");
    FrSENew(classe, "INT_4U[nTotalStat]","version","-");
    FrSENew(classe, "INT_8U[nTotalStat]","positionStat","-");

    FrSENew(classe, "INT_4U","nADC","-");
    FrSENew(classe, "STRING[nADC]","name","-");
    FrSENew(classe, "INT_4U[nADC]","channelID","-");
    FrSENew(classe, "INT_4U[nADC]","groupID","-");
    FrSENew(classe, "INT_8U[nADC][nFrame]","positionADC","-");

    FrSENew(classe, "INT_4U","nProc","-");
    FrSENew(classe, "STRING[nProc]","nameProc","-");
    FrSENew(classe, "INT_8U[nProc][nFrame]","positionProc","-");

    FrSENew(classe, "INT_4U","nSim","-");
    FrSENew(classe, "STRING[nSim]","nameSim","-");
    FrSENew(classe, "INT_8U[nSim][nFrame]","positionSim","-");

    FrSENew(classe, "INT_4U","nSer","-");
    FrSENew(classe, "STRING[nSer]","nameSer","-");
    FrSENew(classe, "INT_8U[nSer][nFrame]","positionSer","-");

    FrSENew(classe, "INT_4U","nSummary","-");
    FrSENew(classe, "STRING[nSummary]","nameSum","-");
    FrSENew(classe, "INT_8U[nSummary][nFrame]","positionSum","-");

    FrSENew(classe, "INT_4U",  "nEventType","-");
    FrSENew(classe, "STRING[nEventType]","nameEvent","-");
    FrSENew(classe, "INT_4U[nEventType]","nEvent","-");
    FrSENew(classe, "INT_4U",  "nTotalEvent","-");
    FrSENew(classe, "INT_4U[nTotalEvent]","GTimeSEvent","-");
    FrSENew(classe, "INT_4U[nTotalEvent]","GTimeNEvent","-");
    FrSENew(classe, "REAL_4[nTotalEvent]","amplitudeEvent","-");
    FrSENew(classe, "INT_8U[nTotalEvent]","positionEvent","-");

    FrSENew(classe, "INT_4U",  "nSimEventType","-");
    FrSENew(classe, "STRING[nSimEventType]","nameSimEvent","-");
    FrSENew(classe, "INT_4U[nSimEventType]","nSimEvent","-");
    FrSENew(classe, "INT_4U",  "nTotalSEvent","-"); 
    FrSENew(classe, "INT_4U[nTotalSEvent]","GTimeSSim","-");
    FrSENew(classe, "INT_4U[nTotalSEvent]","GTimeNSim","-");
    FrSENew(classe, "REAL_4[nTotalSEvent]","amplitudeSimEvent","-");
    FrSENew(classe, "INT_8U[nTotalSEvent]","positionSimEvent","-");

    FrSENew(classe, "INT_4U","chkSum","-");}

  return(classe);}

/*-------------------------------------------------------------FrTOCdetFree--*/
void FrTOCdetFree(FrTOCdet *det)
/*---------------------------------------------------------------------------*/
{
 if(det == NULL) return;

 if(det->next != NULL) FrTOCdetFree(det->next);

 free(det->name);
 free(det);

 return;}

/*-------------------------------------------------------------FrTOCdetMark--*/
void FrTOCdetMark(FrFile *oFile,
                  char *name)
/*---------------------------------------------------------------------------*/
{FrTOCdet *det, **root;
 int test;

    /*--- search if this detector is already in the alphabetic ordered list--*/
 
 for(root = &(oFile->toc->detector); *root != NULL; root = &(*root)->next)
   {test = strcmp(name, (*root)->name);
    if(test == 0) return; 
    if(test < 0) break;}
                                             /*----- add a new detector -----*/

 det = (FrTOCdet *) malloc(sizeof(FrTOCdet));
 if(det == NULL) return;
 det->next = (*root);
 *root = det;

 FrStrCpy(&(det->name),name);
 det->position = oFile->nBytes;

 return;}

/*-------------------------------------------------------------FrTOCdetRead--*/
FrTOCdet *FrTOCdetRead(FrFile *iFile)
/*---------------------------------------------------------------------------*/
{FrTOCdet *det, *root;
 unsigned int nDetector, i;
 char **names;
 FRLONG *position;

 FrReadIntU(iFile, &nDetector);
 if(nDetector == 0) return(NULL);

 FrReadVQ (iFile, &names,    nDetector);
 FrReadVL (iFile, &position, nDetector);

                         /*---------------- convert data to a linked list----*/

 root = NULL;
 for(i=0; i<nDetector; i++)
   {det = (FrTOCdet *) malloc(sizeof(FrTOCdet));
    if(det == NULL) return(NULL);
    FrStrCpy(&(det->name), names[i]);
    free(names[i]);
    det->position  = position[i];
    det->localTime = 99999;
    det->next = root;
    root = det;}
                         /*--------------------- free the temporary buffer--*/

 free(names);
 free(position);

 return(root);}

/*-------------------------------------------------------------FrTOCdetWrite-*/
void FrTOCdetWrite(FrFile *oFile)
/*---------------------------------------------------------------------------*/
{int nDetector;
 FrTOCdet *det;

 nDetector = 0;
 for(det = oFile->toc->detector; det != NULL; det = det->next)  {nDetector ++;}
 FrPutInt(oFile, nDetector);
 if(nDetector == 0) return;

 for(det = oFile->toc->detector; det != NULL; det = det->next) 
      {FrPutSChar(oFile, det->name);}
 for(det = oFile->toc->detector; det != NULL; det = det->next) 
      {FrPutLong (oFile, det->position);}
 
 return;}

/*----------------------------------------------------------------FrTOCDump--*/
void FrTOCDump(FrTOC *toc,
               FILE *fp,
               int debugLvl,
               char *tag)
/*---------------------------------------------------------------------------*/
{int size, gts, t0, tend, i, sizeMax;
 FrTOCstat *sm;
 FrTOCevt *evt;
 FrTOCdet *det;
 double gpsMax;

 if(toc == NULL)    return;

 fprintf(fp," Dump file TOC: %d Frames; ULeaps=%d\n", toc->nFrame,toc->ULeapS);
 if(toc->nFrame == 0) return; 

                               /*-------------- Frame information ----------*/
 t0 = toc->GTimeS[0];
 tend = toc->GTimeS[toc->nFrame-1];
 fprintf(fp,"  First frame GPS=%d (UTC:%s) (t0)\n",
                                t0, FrStrUTC(t0,  toc->ULeapS));
 fprintf(fp,"  Last  frame GPS=%d (UTC:%s)\n",
                              tend, FrStrUTC(tend,toc->ULeapS));
 if(debugLvl <1) return;

 fprintf(fp,"        run# frame#  t-t0(s) dataQuality"
        " position  firstADC first ser  f. table  size(B)\n");
 size = 0;
 sizeMax = 0;
 gpsMax = 0;

 for(i=0; i<toc->nFrame; i++)
    {if(i < toc->nFrame -1)
          {size = toc->positionH[i+1] - toc->positionH[i];}
     else {size = toc->position       - toc->positionH[i];}
     if(size > sizeMax) 
        {sizeMax = size;
         gpsMax  = toc->GTimeS[i]+1.e-9*toc->GTimeN[i];}
     if((i < 5) || (i>toc->nFrame -3))
       {gts = toc->GTimeS[i];
        fprintf(fp,"%5d%7d%7d%9d%10x%10"FRLLD"%10"FRLLD"%10"FRLLD"%10"FRLLD
           " %8d\n", i, toc->runs[i],toc->frame[i],toc->GTimeS[i]-t0,
           toc->dataQuality[i], toc->positionH[i], toc->nFirstADC[i],
           toc->nFirstSer[i],   toc->nFirstTable[i], size);}
     else if(i == 5)
       {fprintf(fp," ....   ....\n");}}

 fprintf(fp," Maximum frame size: %9d bytes at GPS=%.3f\n", sizeMax, gpsMax);

                                 /*-------------------- dump detector info --*/

 for(det = toc->detector; det != NULL; det = det->next)
  {fprintf(fp," Detector:%s \tposition=%"FRLLD" localTime:%d s.\n",
        det->name, det->position, det->localTime);}

 if(debugLvl < 2) return;
                                /*--------- Static Data information ---------*/

 fprintf(fp," Number of static data blocks: %d\n",toc->nStatType);
 sm = toc->stat;
 while(sm != NULL)
   {fprintf(fp," For Static Data:%s   detector:%s    %d instances\n",
         sm->name, sm->detector, sm->nStat);
    fprintf(fp,"     TStart      TEnd      version      position\n");
    for(i=0; i<sm->nStat; i++)
      {if(i < 3 || i == sm->nStat-1) 
          {fprintf(fp,"     %10d %10d %7d %14"FRLLD"\n",sm->tStart[i],sm->tEnd[i],
                 sm->version[i],sm->position[i]);}
       else if(i == 3) {fprintf(fp,"       .......\n");}}

    sm = sm->next;}
                                 /*--------- time serie information ---------*/

 FrTOCtsDump(toc->adc,    fp,     "ADC", tag, toc->nFrame);
 FrTOCtsDump(toc->proc,   fp,    "proc", tag, toc->nFrame);
 FrTOCtsDump(toc->sim,    fp,     "sim", tag, toc->nFrame);
 FrTOCtsDump(toc->ser,    fp,     "ser", tag, toc->nFrame);
 FrTOCtsDump(toc->summary,fp, "summary", tag, toc->nFrame);

 fprintf(fp," FrEvent:\n");
 for(evt=toc->event; evt!=NULL; evt=evt->next) 
    {FrTOCevtDump(evt, fp, 4, toc->ULeapS);}

 fprintf(fp," FrSimEvent:\n");
 for(evt=toc->simEvt; evt!=NULL; evt=evt->next) 
    {FrTOCevtDump(evt, fp, 4, toc->ULeapS);}

                            /*------------ SH information -------------------*/

 if(debugLvl < 3) return; 

 fprintf(fp,"  Number of SH structure: %d (max=%d)\n",toc->nSH, toc->SHidMax);
 for(i=0; i<toc->nSH; i++)
   {fprintf(fp,"    id=%4d for:%s\n",toc->SHid[i],toc->SHname[i]);}

 return;}

/*-------------------------------------------------------------FrTOCevtDump--*/
void FrTOCevtDump(FrTOCevt *evt, 
                  FILE *fp,
                  int maxPrint,
                  int ULeapS)
/*---------------------------------------------------------------------------*/
{int i, gts;
 double gps;

 if(evt == NULL) return;
 fprintf(fp,"  %s: %d event(s): \n",
             evt->name, evt->nEvent);

 for(i=0; i<evt->nEvent; i++)
   {if(i > maxPrint) 
      {fprintf(fp,"....\n");
       break;}
    gts = evt->GTimeS[i];
    gps = evt->GTimeS[i]+1.e-9*evt->GTimeN[i];
    fprintf(fp,"%5d GPS:%16.4f UTC:%25s amplitude:%12g pos:%12"FRLLD"\n",
          i, gps, FrStrUTC(gts,ULeapS), evt->amplitude[i], evt->position[i]);}

 return;}

/*-------------------------------------------------------------FrTOCevtFree--*/
void FrTOCevtFree(FrTOCevt *evt)
/*---------------------------------------------------------------------------*/
{
 if(evt == NULL) return;

 if(evt->next != NULL) FrTOCevtFree(evt->next);

 free(evt->GTimeS);
 free(evt->GTimeN);
 free(evt->amplitude);
 free(evt->position);
 free(evt->name);

 free(evt);

 return;}


/*-----------------------------------------------------FrTOCevtGetEventInfo--*/
FrVect *FrTOCevtGetEventInfo(FrTOCevt *root,
                             char *tag,
                             double tStart,
                             double length,
                             double amplitudeMin,
                             double amplitudeMax)
/*---------------------------------------------------------------------------*/
{FrVect *gtime, *amplitude;
 int i, iEvent, nEvent;
 FrTOCevt *tocEvt;
 FrTag *frtag;
 double tEvent, tEnd;

 tEnd = tStart + length;
 if((frtag = FrTagNew(tag)) == NULL) return(NULL);

           /*------------------ loop over all event which match the name ----*/

 nEvent = 0;
 for(tocEvt = root; tocEvt != NULL; tocEvt = tocEvt->next) 
   {if(FrTagMatch(frtag, tocEvt->name) == FR_NO) continue; 

                                         /*----count the number of events----*/
    for(i=0; i<tocEvt->nEvent; i++)
       {tEvent = tocEvt->GTimeS[i]+1.e-9*tocEvt->GTimeN[i];
        if(tEvent < tStart) continue;
        if(tEvent > tEnd)   continue; 
        if(tocEvt->amplitude[i] < amplitudeMin) continue;
        if(tocEvt->amplitude[i] > amplitudeMax) continue;
        nEvent ++;}}

 if(nEvent == 0) 
   {FrTagFree(frtag);
    return(NULL);}
                                         /*------- create output vectors ----*/

 gtime     = FrVectNew1D("Event_time",    FR_VECT_8R, nEvent,1.,
                         "a.u.","GPS time");
 amplitude = FrVectNew1D("Event_amplitude",FR_VECT_4R, nEvent,1.,
                         "a.u.","event amplitude");
 if(gtime == NULL || amplitude == NULL) return(NULL);
 gtime->next = amplitude;

                   /*---------------------------------- fill information ----*/
 iEvent = 0;
 for(tocEvt = root; tocEvt != NULL; tocEvt = tocEvt->next) 
   {if(FrTagMatch(frtag, tocEvt->name) == FR_NO) continue; 

    for(i=0; i<tocEvt->nEvent; i++)
       {tEvent = tocEvt->GTimeS[i]+1.e-9*tocEvt->GTimeN[i];
        if(tEvent < tStart) continue;
        if(tEvent > tEnd)   continue; 
        if(tocEvt->amplitude[i] < amplitudeMin) continue;
        if(tocEvt->amplitude[i] > amplitudeMax) continue;
        gtime->dataD[iEvent] = tEvent;
        amplitude->dataF[iEvent] = tocEvt->amplitude[i];
        iEvent ++;}}

 FrTagFree(frtag); 
 
 return(gtime);}

/*-------------------------------------------------------------FrTOCevtMark--*/
FrTOCevt *FrTOCevtMark(FrFile *oFile,
                      FrTOCevt **root,
                  char *name,
                  unsigned int GTimeS,
                  unsigned int GTimeN,
                  float amplitude)
/*---------------------------------------------------------------------------*/
{FrTOCevt *evt;
 int test, maxEvent;

               /*--- search if this event is already in the list 
                                            (sorted by alphabetic order)-----*/
 evt = NULL;
 for(; *root != NULL; root = &(*root)->next)
   {test = strcmp(name, (*root)->name);
    if(test < 0) break;
    else if(test == 0) 
      {evt = (*root);
       break;}}
                             /*----- new type of event; add a new table -----*/
 if(evt == NULL) 
   {evt = FrTOCevtNew(name, 4);
    if(evt == NULL) return(NULL);
    evt->next = (*root);
    *root = evt;}
                             /*---------------increase event size if needed--*/

 if(evt->nEvent > evt->maxEvent - 2)
   {evt->maxEvent = 2 * evt->maxEvent;
    maxEvent = evt->maxEvent;
    evt->GTimeS =(unsigned int *)realloc(evt->GTimeS, sizeof(int)  * maxEvent);
    evt->GTimeN =(unsigned int *)realloc(evt->GTimeN, sizeof(int)  * maxEvent);
    evt->amplitude =(float *)realloc(evt->amplitude, sizeof(float) * maxEvent);
    evt->position =(FRULONG *)realloc(evt->position,sizeof(FRULONG)* maxEvent);

    if((evt->GTimeS    == NULL) || 
       (evt->GTimeN    == NULL) || 
       (evt->amplitude == NULL) || 
       (evt->position  == NULL))
      {evt->maxEvent = 0;
       return(NULL);}}
                             /*--------------- fill information--------------*/

 evt->GTimeS   [evt->nEvent] = GTimeS;
 evt->GTimeN   [evt->nEvent] = GTimeN;
 evt->amplitude[evt->nEvent] = amplitude;
 evt->position [evt->nEvent] = oFile->nBytes;

 evt->nEvent++;

 return(evt);}
/*--------------------------------------------------------------FrTOCevtNew--*/
FrTOCevt *FrTOCevtNew(char *name, int maxEvent)
/*---------------------------------------------------------------------------*/
{FrTOCevt *evt;

 evt = (FrTOCevt *) malloc(sizeof(FrTOCevt));
 if(evt == NULL)  return(NULL);

 evt->maxEvent = maxEvent;
 evt->nEvent   = 0;
 evt->next     = NULL;
 
 FrStrCpy(&(evt->name),name);
 evt->GTimeS    =(unsigned int *)malloc(sizeof(int)    * evt->maxEvent);
 evt->GTimeN    =(unsigned int *)malloc(sizeof(int)    * evt->maxEvent);
 evt->amplitude =       (float *)malloc(sizeof(float)  * evt->maxEvent);
 evt->position  =     (FRULONG *)malloc(sizeof(FRULONG)* evt->maxEvent);

 if((evt->name      == NULL) || 
    (evt->GTimeS    == NULL) || 
    (evt->GTimeN    == NULL) ||
    (evt->amplitude == NULL) ||
    (evt->position  == NULL))    {return(NULL);}

 return(evt);}

/*-------------------------------------------------------------FrTOCevtRead--*/
FrTOCevt *FrTOCevtRead(FrFile *iFile)
/*---------------------------------------------------------------------------*/
{FrTOCevt *evt, *root, *last;
  unsigned int nEventType, *nEvent, *GTimeS, *GTimeN, i, j, nTot, nTotalEvent;
  char msg[80];
 char **names;
 float *amplitude;
 FRLONG *position;

 FrReadIntU(iFile, &nEventType);
 if((nEventType == 0) || 
    (nEventType == 0xffffffff)) {
  if(iFile->fmtVersion >= 8) FrReadIntU(iFile, &nTotalEvent);
  return(NULL);}

 FrReadVQ (iFile,           &names,  nEventType);
 FrReadVI (iFile, (int **)  &nEvent, nEventType);
 if(iFile->error == FR_ERROR_MALLOC_FAILED) return(NULL);

 
 nTot = 0;
 for(i=0; i<nEventType; i++) {nTot += nEvent[i];}   
 if(FrDebugLvl > 1) fprintf(FrFOut," nEventType=%d nTot=%d\n",
			    nEventType, nTot);

 if(iFile->fmtVersion >= 8) {
   FrReadIntU(iFile, &nTotalEvent);
   if(nTot != nTotalEvent) {
     sprintf(msg,"Bad event numbers nTot=%d nTotalEvent=%d",nTot,nTotalEvent);
     FrError(3,"FrTOCFrame",msg);
     iFile->error = FR_ERROR_BAD_NEVENT;
     return(NULL);}}

 FrReadVI (iFile, (int **)   &GTimeS,   nTot);
 FrReadVI (iFile, (int **)   &GTimeN,   nTot);
 FrReadVF (iFile,           &amplitude, nTot);
 FrReadVL (iFile, (FRLONG **)&position, nTot);
 if(iFile->error == FR_ERROR_MALLOC_FAILED) return(NULL);

                         /*---------------- convert data to a linked list----*/

 j = 0;
 last = NULL;
 root = NULL;
 for(i=0; i<nEventType; i++)
   {evt = FrTOCevtNew(names[i], nEvent[i]);
    if(evt == NULL) return(NULL);
    free(names[i]);
    if(last != NULL) last->next = evt;
    else root = evt;
    last = evt;
    evt->nEvent = nEvent[i];

    memcpy(evt->GTimeS,       &(GTimeS[j]), nEvent[i]*sizeof(int));
    memcpy(evt->GTimeN,       &(GTimeN[j]), nEvent[i]*sizeof(int));
    memcpy(evt->amplitude, &(amplitude[j]), nEvent[i]*sizeof(float));
    memcpy(evt->position,   &(position[j]), nEvent[i]*sizeof(position));
    j += nEvent[i];}
                                    /*----------- free the temporary buffer--*/
 free(names);
 free(nEvent);
 free(GTimeS);
 free(GTimeN);
 free(amplitude);
 free(position);

 return(root);}

/*------------------------------------------------------FrTOCevtSegListMatch-*/
void FrTOCevtSegListMatch(FrFile *iFile,
                          FrTOCevt *root)
/*---------------------------------------------------------------------------*/
{int i,j;
 FrTOCevt *evt;
 double gtime;

 if(root == NULL) return;
 
 for(evt = root; evt != NULL; evt = evt->next) 
   {j = 0;
    for(i=0; i<evt->nEvent; i++)
       {gtime = evt->GTimeS[i] + 1.e-9*evt->GTimeN[i];
        if(FrSegListFind(iFile->segList,gtime) < 0) continue;

        evt->GTimeS   [j] = evt->GTimeS   [i];
        evt->GTimeN   [j] = evt->GTimeN   [i];
	evt->amplitude[j] = evt->amplitude[i];
	evt->position [j] = evt->position [i];
	j++;}
    evt->nEvent = j;}
 
 return;}
/*-------------------------------------------------------------FrTOCevtWrite-*/
void FrTOCevtWrite(FrFile *oFile,
                   FrTOCevt *root)
/*---------------------------------------------------------------------------*/
{int nEventType, nTotalEvent;
 FrTOCevt *evt;

 if(root == NULL) 
    {FrPutInt  (oFile, 0);
     FrPutInt  (oFile, 0);
     return;}

 nEventType = 0;
 for(evt = root; evt != NULL; evt = evt->next) 
    {nEventType ++;}
 FrPutInt(oFile, nEventType);

 for(evt = root; evt != NULL; evt = evt->next) {FrPutSChar(oFile, evt->name);}
 for(evt = root; evt != NULL; evt = evt->next) {FrPutInt(oFile, evt->nEvent);}

 if(FrFormatVersion >= 8) {
   nTotalEvent = 0;
   for(evt = root; evt != NULL; evt = evt->next) {nTotalEvent += evt->nEvent;}
   FrPutInt(oFile, nTotalEvent);}

 for(evt = root; evt != NULL; evt = evt->next) 
                   {FrPutVI (oFile, (int *)evt->GTimeS, evt->nEvent);}
 for(evt = root; evt != NULL; evt = evt->next) 
                   {FrPutVI (oFile, (int *)evt->GTimeN, evt->nEvent);}
 for(evt = root; evt != NULL; evt = evt->next) 
                   {FrPutVF (oFile, evt->amplitude, evt->nEvent);}
 for(evt = root; evt != NULL; evt = evt->next) 
                   {FrPutVL (oFile, (FRLONG *)evt->position,  evt->nEvent);}
 
 return;}

/*----------------------------------------------------------- FrTOCFFLBuild--*/
void FrTOCFFLBuild(FrFile *iFile)
/*---------------------------------------------------------------------------*/
/* This function build the FFL for all the input files                       */
/*---------------------------------------------------------------------------*/
{FrFileH *current;

 if(iFile == NULL) return;
    
 for(current = iFile->fileH; current != NULL; current = current->next)
   {if(current->tLastEvt > -0.5) continue;

    if(current != iFile->current) 
     {FrFileIClose(iFile);        
      iFile->current = current;
      iFile->error = FR_OK;  
      FrFileIOpen(iFile);}

    if(iFile->toc == NULL)  FrTOCReadFull(iFile);}

 return;}

/*----------------------------------------------------------------FrTOCFree--*/
void FrTOCFree(FrTOC *toc)
/*---------------------------------------------------------------------------*/
{int i;
 FrTOCstat *sm, *nextSm;

 if(toc == NULL)   return;

 free(toc->dataQuality);
 free(toc->GTimeS);
 free(toc->GTimeN);
 free(toc->dt);
 free(toc->runs);
 free(toc->frame);
 free(toc->positionH);
 free(toc->nFirstADC);
 free(toc->nFirstSer);
 free(toc->nFirstTable);
 free(toc->nFirstMsg);

 free(toc->SHid);
 for(i=0; i<toc->nSH; i++)
    {free(toc->SHname[i]);}
 free(toc->SHname);

 FrTOCdetFree(toc->detector);
                                /*--------- Static Data information ---------*/
 sm = toc->stat;
 while(sm != NULL)
   {nextSm = sm->next;
    free(sm->name);
    free(sm->detector);
    free(sm->tStart);
    free(sm->tEnd);
    free(sm->version);
    free(sm->position);
    free(sm);
    sm = nextSm;}
                                      /*--------- free time series  ---------*/
 FrTOCtsFree(toc->adc);
 FrTOCtsFree(toc->proc);
 FrTOCtsFree(toc->sim);
 FrTOCtsFree(toc->ser);
 FrTOCtsFree(toc->summary);

 FrTOCtsHFree(toc->adcH);
 FrTOCtsHFree(toc->procH);
 FrTOCtsHFree(toc->simH);
 FrTOCtsHFree(toc->serH);
 FrTOCtsHFree(toc->summaryH);

 FrTOCevtFree(toc->event);
 FrTOCevtFree(toc->simEvt);

 free(toc);

 return;
}
/*---------------------------------------------------------------FrTOCFrame--*/
void FrTOCFrame(FrFile *oFile,
                FrameH *frame)
/*---------------------------------------------------------------------------*/
{FrTOC *toc;
 int max;
  
 if(oFile->toc == NULL) FrTOCNew(oFile, frame);
 toc = oFile->toc; 
 if(toc == NULL) return;
 toc->nFrame++;
                              /*-------- increase space if needed -----------*/

 if(toc->nFrame+1 >= toc->maxFrame)
   {toc->maxFrame = 2 * toc->maxFrame;
    max = toc->maxFrame;

    toc->dataQuality =
                 (unsigned int *)realloc(toc->dataQuality,sizeof(int)    *max);
    toc->GTimeS =(unsigned int *)realloc(toc->GTimeS,     sizeof(int)    *max);
    toc->GTimeN =(unsigned int *)realloc(toc->GTimeN,     sizeof(int)    *max);
    toc->dt         = (double *) realloc(toc->dt,         sizeof(double) *max);
    toc->runs       =    (int *) realloc(toc->runs,       sizeof(int)    *max);
    toc->frame      =    (int *) realloc(toc->frame,      sizeof(int)    *max);
    toc->positionH  =(FRULONG *) realloc(toc->positionH,  sizeof(FRULONG)*max);
    toc->nFirstADC  =(FRULONG *) realloc(toc->nFirstADC,  sizeof(FRULONG)*max);
    toc->nFirstSer  =(FRULONG *) realloc(toc->nFirstSer,  sizeof(FRULONG)*max);
    toc->nFirstTable=(FRULONG *) realloc(toc->nFirstTable,sizeof(FRULONG)*max);
    toc->nFirstMsg  =(FRULONG *) realloc(toc->nFirstMsg,  sizeof(FRULONG)*max);

    if((toc->dataQuality == NULL) ||
       (toc->GTimeS      == NULL) ||
       (toc->GTimeN      == NULL) ||
       (toc->dt          == NULL) ||
       (toc->runs        == NULL) ||
       (toc->frame       == NULL) ||
       (toc->positionH   == NULL) ||
       (toc->nFirstADC   == NULL) ||
       (toc->nFirstSer   == NULL) ||
       (toc->nFirstTable == NULL) ||
       (toc->nFirstMsg   == NULL))
           {FrError(3,"FrTOCFrame"," realloc failed");
            oFile->error = FR_ERROR_MALLOC_FAILED;
            return;}
                             /*-------- increase storage for time series ----*/

    FrTOCtsRealloc(toc->adcH,     oFile, max/2);
    FrTOCtsRealloc(toc->procH,    oFile, max/2);
    FrTOCtsRealloc(toc->simH,     oFile, max/2);
    FrTOCtsRealloc(toc->serH,     oFile, max/2);
    FrTOCtsRealloc(toc->summaryH, oFile, max/2);}

                              /*-------------- fill information--------------*/

 toc->dataQuality[toc->nFrame] = frame->dataQuality;
 toc->GTimeS     [toc->nFrame] = frame->GTimeS;
 toc->GTimeN     [toc->nFrame] = frame->GTimeN;
 toc->dt         [toc->nFrame] = frame->dt;
 toc->runs       [toc->nFrame] = frame->run;
 toc->frame      [toc->nFrame] = frame->frame;
 toc->positionH  [toc->nFrame] = oFile->nBytes;

 toc->nFirstADC  [toc->nFrame] = 0;
 toc->nFirstSer  [toc->nFrame] = 0;
 toc->nFirstTable[toc->nFrame] = 0;
 toc->nFirstMsg  [toc->nFrame] = 0;
 
 return;}

/*--------------------------------------------------------- FrTOCFrameFindN--*/
int FrTOCFrameFindN(FrFile *iFile,
                    int run,
                    int frame)
/*---------------------------------------------------------------------------*/
/* This function search the frame index in the Table Of Content. It returns  */
/* -2 if no TOC is available, and -1 if the frame is not in the file         */
/*---------------------------------------------------------------------------*/
{FrTOC *toc;
 int i;

 if(iFile == NULL) return(-1);
 if(iFile->toc == NULL)  FrTOCReadFull(iFile);

 iFile->relocation = FR_NO;            /*--- to avoid relloaction problems---*/
 
 toc = iFile->toc;
 if(toc == NULL) return(-2);

 for(i=0; i<toc->nFrame; i++)
   {if(frame != toc->frame[i]) continue;
    if(run   != toc->runs [i]) continue;
    return(i);}

 return(-1);}

/*---------------------------------------------------------FrTOCFrameFindNT--*/
int FrTOCFrameFindNT(FrFile *iFile,
                     double gtime)
/*---------------------------------------------------------------------------*/
/* This function search the frame index in the Table Of Content for a given  */
/* GPS time. It find also the rigth file if needed.                          */
/* If no frame is found for this time it looks for the following frame.      */
/* It returns a negative value in case of error or if gtime is after the     */
/* end of the files.                                                         */
/*---------------------------------------------------------------------------*/
{int position;
 FrFileH *bestFile, *firstFile;

 if(iFile == NULL) return(-1);

 FrTOCFFLBuild(iFile); 
 if(iFile->error != FR_OK) return(-1);

 firstFile = iFile->fileH;    /*---define the starting file for the loop----*/
                              /*-check is the current file is a better start*/
 if(iFile->current != NULL)
   {if(gtime > iFile->fileH->tStart - 1.e-6) firstFile = iFile->current;}

 /*--- loop over all files to get the bestFile that contain or follow gtime-*/

 for(bestFile = firstFile; bestFile != NULL; bestFile = bestFile->next)
    {if(gtime < (bestFile->tStart + bestFile->length - 1.e-6)) break;}

 if(bestFile == NULL) return(-1);

                                  /* to protect against hole between files --*/
 if(gtime < bestFile->tStart) gtime = bestFile->tStart;

 if(bestFile != iFile->current) /*--- close and open the new file if needed--*/
   {FrFileIClose(iFile);   
    iFile->error = FR_OK; 
    iFile->current = bestFile;
    FrFileIOpen(iFile);
    if(iFile->error != FR_OK) return(-1);}

   /*---- Search the file with the time range closest to the requested time--*/
                                 /*--- then get the closest frame -----------*/

 position = FrTOCGetFrameIndexOneFile(iFile, gtime);
 if(position  < -5) position = - position - 10;
 if(position == -3 && iFile->current->tStart > gtime) position = 0;

 return(position);}

/*--------------------------------------------------------- FrTOCFrameFindT--*/
int FrTOCFrameFindT(FrFile *iFile,
                    double gtime)
/*---------------------------------------------------------------------------*/
/* This function search the frame index in the Table Of Content. It returns  */
/* -2 if no TOC is available, and -1 if the frame is not in the file         */
/*---------------------------------------------------------------------------*/
{int position;
 FrFileH *firstFileH;;

 if(iFile == NULL) return(-1);
 firstFileH = iFile->current;
 
                    /*- return the first frame in the first file if gtime = 0*/
 if(gtime == 0)
   {if(iFile->current != iFile->fileH) 
      {FrFileIClose(iFile);         
       iFile->current = iFile->fileH;
       iFile->error = FR_OK;   
       FrFileIOpen(iFile);}

    if(iFile->toc == NULL)  FrTOCReadFull(iFile);
    if(iFile->error != FR_OK) return(-1); 
    return(0);}
                    /*--- We loop on all possible files ---------------------*/

 while(1)
   {position = FrTOCGetFrameIndexOneFile(iFile, gtime);
    if(position >= 0) return(position);
    if(FrFileINext(iFile, gtime, 1.e-6, firstFileH, FR_NO) != 0) return(-3);}

 return(-4);}

/*------------------------------------------------FrTOCGetFrameIndexOneFile--*/
int FrTOCGetFrameIndexOneFile(FrFile *iFile,
                              double gtime)
/*---------------------------------------------------------------------------*/
/* Given a GPS time, this function searches the frame index in the Table Of  */
/* Content for a single file. It returns:                                    */
/*  0 or a positive value if the frame is found                              */
/*   or -1 if the current file do not include this GPS time                  */
/*   or -2 if the TOC is unavailable for this file                           */
/*   or -10-index of the next frame if no frame is available for this time   */
/*---------------------------------------------------------------------------*/
{FrTOC *toc;
 int i, delta, ilast, nLoop;
 double t1;

 if(iFile == NULL) return(-1);

 if(iFile->current != NULL)
   {if(iFile->current->tStart > -1)
     {if(iFile->current->tStart > gtime+1.e-6) return(-1);
      if(iFile->current->tStart+
         iFile->current->length < gtime+1.e-6) return(-1);}}
      
 if(iFile->toc == NULL)  FrTOCReadFull(iFile);

 iFile->relocation = FR_NO;            /*--- to avoid relloaction problems---*/
 
 toc = iFile->toc;
 if(toc == NULL) return(-2);

 if(toc->nFrame == 0) return(-1);

 if(gtime == 0.)      return(0);   /*--- return the first frame is gtime == 0*/

 gtime += 1.e-6; /* we add a small offset to avoid round problems when using
                 the starting time of the frame as requested time (for instance
                 1 may be converted to 0.999999 instead of 1.0000) ----------*/

 delta = toc->nFrame;
 i =  delta - delta/2;
 ilast = -1;

 nLoop = 0;
 while(delta > 0)
   {if(nLoop++ > 80) 
       return(-10-(i+ilast+1)/2);  /*-- the frame is probably not here
                                      return -10-index of the next frame --*/
    if(i < 0) i = 0;
    if(i > toc->nFrame-1) i = toc->nFrame-1;
    delta = delta-delta/2;

    if(ilast == i) break;
    ilast = i;

    t1 =  toc->GTimeS[i]+1.e-9*toc->GTimeN[i];
    if     (gtime < t1)              {i = i - delta;}
    else if(gtime > t1 + toc->dt[i]) {i = i + delta;}
    else {return(i);}}

 return(-3);}

/*---------------------------------------------------------FrTOCGetLTOffset--*/
int FrTOCGetLTOffset(FrFile *iFile, 
                     char *channelName)
/*---------------------------------------------------------------------------*/
{FrTOCdet *det;
 FrDetector *detector;
 int localTimeOffset = 99999;   /* default invalid value */

 if(iFile == NULL)       return (localTimeOffset);
 if(iFile->toc == NULL)  return (localTimeOffset);
 if(channelName == NULL) return (localTimeOffset);

 for(det = iFile->toc->detector; det != NULL; det = det->next)
   {
                 /*----- the first time extract localTime from the file ---*/

     if(det->localTime == 99999)
         {if(FrTOCSetPos(iFile, det->position) != 0) continue;
          detector = FrDetectorRead(iFile);
          if(detector == NULL) continue;
          det->localTime = detector->localTime;
          detector->next = NULL;
          FrDetectorFree(detector);}

    if(det->name[0] != channelName[0]) continue;
    return(det->localTime);}

    /*----- special case for virgo, since the channel name are incompleted--*/

 det = iFile->toc->detector;
 if(det != NULL)
   {if(det->name[0] == 'V') return(det->localTime);}

 return (localTimeOffset);}

/*------------------------------------------------------------FrTOCGetTimes--*/
void FrTOCGetTimes(FrFile *iFile)
/*---------------------------------------------------------------------------*/
{double tEvent, tStart, tEnd, fStart, fEnd, tFirst, tLast;
 FrTOCevt *tocEvt;
 int i;

 if(iFile->toc == NULL) return;
 if(iFile->current == NULL) return;
 
                     /*------------------------ Get time limits for frames --*/

 tStart = 1.e+20;
 tEnd   = 0.;

 for(i=0; i<iFile->toc->nFrame; i++)
   {fStart = iFile->toc->GTimeS[i] + 1.e-9*iFile->toc->GTimeN[i];
    if(fStart < tStart) tStart = fStart;
    fEnd = fStart + iFile->toc->dt[i];
    if(fEnd > tEnd) tEnd = fEnd;}

 iFile->current->tStart = tStart;
 iFile->current->length = tEnd-tStart;

                    /*---------------------------Find time limit for events--*/
 
 tFirst = 1.e+20;
 tLast  = 0.;

 for(tocEvt = iFile->toc->event; tocEvt != NULL; tocEvt = tocEvt->next) 
      {for(i=0; i<tocEvt->nEvent; i++)
         {tEvent = tocEvt->GTimeS[i]+1.e-9*tocEvt->GTimeN[i];
          if(tEvent < tFirst) tFirst = tEvent;
          if(tEvent > tLast)  tLast  = tEvent;}}

 for(tocEvt = iFile->toc->simEvt; tocEvt != NULL; tocEvt = tocEvt->next) 
      {for(i=0; i<tocEvt->nEvent; i++)
         {tEvent = tocEvt->GTimeS[i]+1.e-9*tocEvt->GTimeN[i];
          if(tEvent < tFirst) tFirst = tEvent;
          if(tEvent > tLast)  tLast  = tEvent;}}
 
 if(tFirst > 1.e19) tFirst = 0.;   /*---- there is no event in this file ----*/
 iFile->current->tFirstEvt = tFirst;
 iFile->current->tLastEvt  = tLast;

 return;}

/*-----------------------------------------------------------------FrTOCNew--*/
void FrTOCNew(FrFile *oFile, 
              FrameH *frame)
/*---------------------------------------------------------------------------*/
{FrTOC   *toc;
 int max;

 toc = malloc(sizeof(FrTOC));
 if(toc == NULL)
           {FrError(3,"FrTOCNew"," malloc failed");
            oFile->error = FR_ERROR_MALLOC_FAILED;
            return;}
 toc->classe = FrTOCDef(); 

 oFile->toc = toc;

 toc->ULeapS    = frame->ULeapS;
 toc->nFrame    = -1;
 toc->maxFrame  = 4;
 max = toc->maxFrame;

 toc->dataQuality =(unsigned int *) malloc(sizeof(int)     * max);
 toc->GTimeS      =(unsigned int *) malloc(sizeof(int)     * max);
 toc->GTimeN      =(unsigned int *) malloc(sizeof(int)     * max);
 toc->dt          =      (double *) malloc(sizeof(double)  * max);
 toc->runs        =         (int *) malloc(sizeof(int)     * max);
 toc->frame       =         (int *) malloc(sizeof(int)     * max);
 toc->positionH   =     (FRULONG *) malloc(sizeof(FRULONG) * max);
 toc->nFirstADC   =     (FRULONG *) malloc(sizeof(FRULONG) * max);
 toc->nFirstSer   =     (FRULONG *) malloc(sizeof(FRULONG) * max);
 toc->nFirstTable =     (FRULONG *) malloc(sizeof(FRULONG) * max);
 toc->nFirstMsg   =     (FRULONG *) malloc(sizeof(FRULONG) * max);

 if((toc->dataQuality == NULL) ||
    (toc->GTimeS      == NULL) ||
    (toc->GTimeN      == NULL) ||
    (toc->dt          == NULL) ||
    (toc->runs        == NULL) ||
    (toc->frame       == NULL) ||
    (toc->positionH   == NULL) ||
    (toc->nFirstADC   == NULL) ||
    (toc->nFirstSer   == NULL) ||
    (toc->nFirstTable == NULL) ||
    (toc->nFirstMsg   == NULL))
           {FrError(3,"FrTOCNew"," malloc failed");
            oFile->error = FR_ERROR_MALLOC_FAILED;
            return;}


 toc->nSH    = 0;
 toc->SHid   = (unsigned short *)malloc(FRMAPMAXSH*sizeof(short));
 toc->SHname = (char **)         malloc(FRMAPMAXSH*sizeof(char *));

 toc->detector = NULL;
 toc->nStatType = 0;
 toc->stat    = NULL;

 toc->adc     = NULL;
 toc->proc    = NULL;
 toc->sim     = NULL;
 toc->ser     = NULL;
 toc->summary = NULL;

 toc->adcH     = NULL;
 toc->procH    = NULL;
 toc->simH     = NULL;
 toc->serH     = NULL;
 toc->summaryH = NULL;

 toc->event   = NULL;
 toc->simEvt  = NULL;

 return;}

/*----------------------------------------------------------------FrTOCRead--*/
FrTOC *FrTOCRead(FrFile *iFile, 
                 int nFrame)           /* expected number of frame (if >= 0) */
/*---------------------------------------------------------------------------*/
{FrTOC *toc;
 unsigned int i, id, localTime;
 unsigned short id2;
    
 toc = (FrTOC *) calloc(1,sizeof(FrTOC));
 if(toc == NULL) 
      {iFile->error = FR_ERROR_MALLOC_FAILED;
       return(NULL);}

 iFile->nBytesR = iFile->nBytes-10;
 if(iFile->fmtVersion > 5)
      {FrReadIntU  (iFile, &id);}
 else {FrReadShortU(iFile, &id2);
       id = id2;}
 if(FrDebugLvl > 0) fprintf(FrFOut,"FrTOCRead: for id=%d\n",id);

 FrReadShort (iFile, &toc->ULeapS);
 if(iFile->fmtVersion <= 5) {FrReadIntU(iFile, &localTime);}
 FrReadInt   (iFile, &toc->nFrame);
 if(FrDebugLvl > 1) fprintf(FrFOut," nFrame=%d\n",toc->nFrame);

 if((nFrame >= 0) && (nFrame != toc->nFrame))
   {if(FrDebugLvl > 0) fprintf(FrFOut,"FrTOCRead: ERROR: # of frame missmatch"
               " (%d %d). Corrupted TOC. Do not use it\n",nFrame,toc->nFrame);
    free(toc);
    return(NULL);}

 if(toc->nFrame > 0)
  {if(iFile->fmtVersion > 5)
     {FrReadVI  (iFile, (int **)&toc->dataQuality, toc->nFrame);}
  else {toc->dataQuality = (unsigned int *) calloc(nFrame, sizeof(int));}
   FrReadVI  (iFile, (int **)&toc->GTimeS,         toc->nFrame);
   FrReadVI  (iFile, (int **)&toc->GTimeN,         toc->nFrame);
   FrReadVD  (iFile, &toc->dt,                     toc->nFrame);
   FrReadVI  (iFile, &toc->runs,                   toc->nFrame);
   FrReadVI  (iFile, (int **)&toc->frame,          toc->nFrame);
   FrReadVL  (iFile, (FRLONG **)&toc->positionH,   toc->nFrame);
   FrReadVL  (iFile, (FRLONG **)&toc->nFirstADC,   toc->nFrame);
   FrReadVL  (iFile, (FRLONG **)&toc->nFirstSer,   toc->nFrame);
   FrReadVL  (iFile, (FRLONG **)&toc->nFirstTable, toc->nFrame);
   FrReadVL  (iFile, (FRLONG **)&toc->nFirstMsg,   toc->nFrame);}

 FrReadIntU  (iFile, &toc->nSH);
 if(FrDebugLvl > 1) fprintf(FrFOut," nSH=%d\n",toc->nSH);
 FrReadVS    (iFile, (short **)&toc->SHid, toc->nSH);
 FrReadVQ    (iFile, &toc->SHname, toc->nSH);
 if(iFile->error == FR_ERROR_MALLOC_FAILED) return(NULL);

                      /*-------------- get SHid for FrVect ------------------*/

 toc->SHidMax = 0;
 for(i=0; i<toc->nSH; i++)
    {if(toc->SHid[i] > toc->SHidMax) toc->SHidMax = toc->SHid[i];
     if(strcmp(toc->SHname[i],"FrVect") ==0) iFile->vectorType=toc->SHid[i];}

 if(iFile->fmtVersion > 5) {toc->detector = FrTOCdetRead(iFile);}

                     /*--------------- get Static data ----------------------*/

 iFile->toc = toc;
 FrTOCStatRead(iFile);
 if(iFile->error != FR_OK) return(NULL);

                   /*------------------- read time series part --------------*/

 toc->adc     = FrTOCtsRead(iFile,     "ADC", toc->nFrame);
 toc->proc    = FrTOCtsRead(iFile,    "proc", toc->nFrame);
 toc->sim     = FrTOCtsRead(iFile,     "sim", toc->nFrame);
 toc->ser     = FrTOCtsRead(iFile,     "ser", toc->nFrame);
 toc->summary = FrTOCtsRead(iFile, "summary", toc->nFrame);
 if(iFile->error == FR_ERROR_MALLOC_FAILED) return(NULL);

                   /*---------------------- read event part -----------------*/

 if(iFile->fmtVersion > 5)
   {toc->event  = FrTOCevtRead(iFile);
    toc->simEvt = FrTOCevtRead(iFile);}
 else
   {toc->event  = FrBack4TOCevtRead(iFile);
    toc->simEvt = FrBack4TOCevtRead(iFile);}
 if(iFile->error == FR_ERROR_MALLOC_FAILED) return(NULL);

                  /*------------------------ checksum ----------------------*/

 if(iFile->fmtVersion >=8) FrReadStructChksum(iFile);

                  /*--------- apply segment list if available ---------------*/

 if(iFile->segList != NULL) FrTOCSegListMatch(iFile);

 if(FrDebugLvl > 0) FrTOCDump(toc, FrFOut, FrDebugLvl-1, NULL);

 return(toc);}
/*--------------------------------------------------------FrTOCSegListMatch--*/
void FrTOCSegListMatch(FrFile *iFile)
/*---------------------------------------------------------------------------*/
{FrTOC *toc;
 FrTOCts *ts;
 unsigned int i, j;
 double coverage, gtime;
    
 toc = iFile->toc;
 if(toc == NULL) return;

 if(FrDebugLvl > 0) 
    fprintf(FrFOut,"Start FrTOCSegListMatch with %d frames\n",toc->nFrame);

          /*---------------- first the event and time series pointers ------*/
 j = 0;
 for(i=0; i<toc->nFrame; i++)
   {gtime = toc->GTimeS[i] + 1.e-9*toc->GTimeN[i];
     coverage = FrSegListCoverage(iFile->segList, gtime, toc->dt[i]);
     if(coverage == 0.) continue;

     toc->dataQuality[j] = toc->dataQuality[i];
     toc->GTimeS     [j] = toc->GTimeS     [i];
     toc->GTimeN     [j] = toc->GTimeN     [i];
     toc->dt         [j] = toc->dt         [i];
     toc->runs       [j] = toc->runs       [i];
     toc->frame      [j] = toc->frame      [i];
     toc->positionH  [j] = toc->positionH  [i];
     toc->nFirstADC  [j] = toc->nFirstADC  [i];
     toc->nFirstSer  [j] = toc->nFirstSer  [i];
     toc->nFirstTable[j] = toc->nFirstTable[i];
     toc->nFirstMsg  [j] = toc->nFirstMsg  [i];
 
     for(ts = toc->adc;  ts != NULL; ts = ts->next) 
                             {ts->position[j] = ts->position[i];}
     for(ts = toc->proc; ts != NULL; ts = ts->next) 
                             {ts->position[j] = ts->position[i];}
     for(ts = toc->sim;  ts != NULL; ts = ts->next) 
                             {ts->position[j] = ts->position[i];}
     for(ts = toc->ser;  ts != NULL; ts = ts->next) 
                             {ts->position[j] = ts->position[i];}
     for(ts = toc->summary;ts!=NULL; ts = ts->next) 
                             {ts->position[j] = ts->position[i];}

    j++;}

 toc->nFrame = j;

 if(FrDebugLvl > 0) 
    fprintf(FrFOut," %d frames after selection\n",toc->nFrame);

                                   /*---------------- event part ------------*/

 FrTOCevtSegListMatch(iFile, toc->event);
 FrTOCevtSegListMatch(iFile, toc->simEvt);

 if(FrDebugLvl > 0) FrTOCDump(toc, FrFOut, FrDebugLvl-1, NULL);

 return;}

/*------------------------------------------------------------FrTOCReadFull--*/
FrTOC *FrTOCReadFull(FrFile *iFile)
/*---------------------------------------------------------------------------*/
{unsigned int i, j, type, nFrames, chkSumFlag, chkSum, nBytes4, offset4,
    chkSumFrHeader, chkSumFile;
 unsigned char value;
 FRLONG off, offset, nBytesEnd;
 FrTOC *toc;

 if(iFile->inMemory == FR_NO) {
   if(iFile->frfd == NULL) FrFileIOpen(iFile); /*--Open the file if needed --*/
   if(iFile->frfd == NULL) return(NULL);
   if(iFile->toc  != NULL) return(iFile->toc);}

                /*--do not compute file checksum when using random access ---*/

 iFile->chkSumFiFlag = FR_NO;

 /*---------------- read the FrEndOfFile data for differnet format version---*/

 if(iFile->fmtVersion < 6) {
   if(FrFileIOSetFromEnd(iFile, -20) == -1) {return(NULL);}
   FrReadIntU(iFile, &nFrames);
   FrReadIntU(iFile, &nBytes4);
   iFile->nBytesE = nBytes4;
   FrReadIntU(iFile, &chkSumFlag);
   FrReadIntU(iFile, &chkSum);
   FrReadIntU(iFile, &offset4);
   offset = offset4;
   off = offset-6;}    /*--- skip the record length(INT4) and type(INT2) */
 
 else if(iFile->fmtVersion < 8) {
   if(FrFileIOSetFromEnd(iFile, -28) == -1) {return(NULL);}
   FrReadIntU(iFile, &nFrames);
   FrReadLong(iFile, (FRLONG*) &iFile->nBytesE);
   FrReadIntU(iFile, &chkSumFlag);
   FrReadIntU(iFile, &chkSum);
   FrReadLong(iFile, &offset);
   off = offset-10;}   /*--- skip the record length(INT8) and type(INT2) */
 
 else {
   if(FrFileIOSetFromEnd(iFile, -32) == -1) {return(NULL);}
   nBytesEnd = FrIOTell(iFile->frfd)+32;
   if(nBytesEnd == FRIOBSIZE) nBytesEnd = iFile->frfd->nBytesRead;
   FrReadIntU(iFile, &nFrames);
   FrReadLong(iFile, (FRLONG*) &iFile->nBytesE);
   if(nBytesEnd != iFile->nBytesE) {
     sprintf(FrErrMsg,"FrTOCReadFull: Corrupted end of file"
       "nBytesEnd=%"FRLLD" iFile->nBytesE=%"FRLLD"\n", nBytesEnd, iFile->nBytesE);
     FrError(3,"FrTOCReadFull", FrErrMsg);
     return(NULL);}
   FrReadLong(iFile, &offset);
   FrReadIntU(iFile, &chkSumFrHeader);
   FrReadIntU(iFile, &chkSum);
   FrReadIntU(iFile, &chkSumFile);
   chkSumFlag = 0;
   off = offset;}   /*--- do not skip the record length(INT8) and type(INT2) */
  
 if(chkSumFlag > 1)                     /*---- this is a corrupted TOC-------*/
   {if(FrDebugLvl > 0) fprintf(FrFOut,"FrTOCReadFull: ERROR: Wrong"
         " chksum flag (%d). Corrupted TOC. Do not use it\n",chkSumFlag);
    return(NULL);}

 if(offset == 0) {return(NULL);}
 if(FrFileIOSetFromEnd(iFile, -off) == -1) {return(NULL);}

 /*-------- for format v8; read the structure header to check the checksum---*/

 if(iFile->fmtVersion >= 8) {
   nBytesEnd = FrIOTell(iFile->frfd);
   iFile->nBytesF  = iFile->nBytes; /*--reset for checksum computation */
   iFile->chkSumFr = 0;
   FrReadLong(iFile, &iFile->length);
   FrRead(iFile, (char *)&value, 1);
   iFile->chkTypeFrRead = value;
   FrRead(iFile, (char *)&value, 1);
   iFile->type = value;}

                                /*----------------- read the TOC itself------*/
 FrTOCRead(iFile, nFrames);

              /*---------- set SH pointer for direct read function access ---*/

 toc = iFile->toc;
 if(toc == NULL) return(NULL);

 iFile->toc->position = iFile->nBytesE - off; 

 for(i=0; i<toc->nSH; i++)
   {for(j=0; j<FrnSH; j++)
     {if(strcmp(toc->SHname[i],FrSHRoot[j]->name) == 0) break;
      if((strcmp(toc->SHname[i],"FrTrigData") == 0) &&
         (strcmp(FrSHRoot[j]->name, "FrEvent") == 0)) break;}
    if (j==FrnSH) {return(NULL);}

    type = toc->SHid[i];
    if(type >= iFile->maxSH) 
       {FrFileIncSH(iFile,type);
        if(iFile->error != FR_OK) return(NULL);}
       
    if(iFile->sh[type] != NULL) continue;

    iFile->sh[type] = FrSHNew(FrSHRoot[j]->name,NULL);
    iFile->sh[type]->id = type;
    iFile->sh[type]->objRead = FrSHRoot[j]->objRead;}

      /*----- set the pointer to the first frame, so the file could be used--*/

 if(toc->nFrame > 0) FrFileIOSet(iFile, toc->positionH[0]);

 FrTOCGetTimes(iFile);       /*------------------------Save start/end time --*/

 FrTOCSort(iFile);           /*--------------------------- check time order--*/

 return(toc);}

/*--------------------------------------------------------------FrTOCSetPos--*/
int FrTOCSetPos(FrFile *iFile,
                FRULONG position)
/*---------------------------------------------------------------------------*/
/* this function set the file pointer position to the first bytes            */
{
  FRULONG available;
  char message[256];

  if(iFile == NULL) return(1);
  if(position == 0) return(1);

  if(FrFileIOSet(iFile, position) == -1) return (2);
  available = iFile->nBytesE - position;

  while((iFile->error == FR_OK)) {
    FrReadStructHeader(iFile);
    if(iFile->error != FR_OK) return(3);

    if(FrDebugLvl > 2) fprintf(FrFOut," length:%10"FRLLD" type=%4d\n",
			       iFile->length,iFile->type);
                             
    if(iFile->length > available) {
      sprintf(message,": request to read beyond the end of "
	      "file (skip record)\n  position=%"FRLLD" length=%"
	      FRLLD" nBytesE=%"FRLLD" available=%"FRLLD"\n",
	      position, iFile->length, iFile->nBytesE,available);
      FrError(3,"FrTOCSetPos",message);
      return(5);}
    available -= iFile->length;

    if(iFile->type >= iFile->toc->SHidMax) {
      sprintf(message,": record type error: type=%d nSH=%d. Skip it\n",
	      iFile->type, iFile->toc->SHidMax);
      FrError(3,"FrTOCSetPos",message);
      return(5);}
     
    if(iFile->type > 2) return(0);

    FrReadSkipRecord(iFile);}

  return(4);}

/*------------------------------------------------------------------FrTOCSH--*/
void FrTOCSH(FrFile *oFile, 
             char *name, 
             unsigned short id)
/*---------------------------------------------------------------------------*/
{FrTOC *toc;
 
 toc = oFile->toc;
 if(toc == NULL) return;

 if(toc->nSH >= FRMAPMAXSH) return;
 toc->SHid[toc->nSH] = id;
 FrStrCpy(&toc->SHname[toc->nSH], name);
 toc->nSH ++;

 return;}
/*---------------------------------------------------------------FrTOCSort--*/
void FrTOCSort(FrFile *file)
/*--------------------------------------------------------------------------*/
/* This function sort the frame by increasing GPS time                      */
/*--------------------------------------------------------------------------*/
{FrTOC *toc;
 FrTOCts *ts;
 double previous, t, *gps, min;
 int i, j, first=0, *index, nFr;

 toc = file->toc;     
 if(toc->nFrame == 0) return;

                 /*------------ first check if we have to do something ----*/

 previous = 0.;
 for(i=0; i<toc->nFrame; i++)
   {t = toc->GTimeS[i] + 1.e-9*toc->GTimeN[i];
    if(previous >= t) break;
    previous = t;}
 if(i == toc->nFrame) return;

        /*------ We need to sort the TOC. The algorithm is not efficient, 
                             but usually the TOC is already ordered---------*/

 if(FrDebugLvl > 0) 
       printf("FrTOCSort: start sorting for nFrame=%d\n",toc->nFrame);

              /*------------------ first we get the right frame order ------*/

 index = (int *)    malloc(toc->nFrame*sizeof(int));
 gps   = (double *) malloc(toc->nFrame*sizeof(double));
 if (index == NULL || gps == NULL) return;

 for(i=0; i<toc->nFrame; i++)
   {gps[i] = toc->GTimeS[i] + 1.e-9*toc->GTimeN[i];}

 for(i=0; i<toc->nFrame; i++)
   {min = 1.e+20;
    for(j=0; j<toc->nFrame; j++)
      {if(gps[j] < min)
        {min = gps[j];
         first = j;}}
    index[i] = first;
    gps[first] = 1.e+30;}

           /*----- then we copy all the frame array with the right order ----*/

 nFr = toc->nFrame;
 j  = FrTOCSort1((char **) &(toc->dataQuality),index, nFr, sizeof(int));
 j += FrTOCSort1((char **) &(toc->GTimeS),     index, nFr, sizeof(int));
 j += FrTOCSort1((char **)&(toc->GTimeN),      index, nFr, sizeof(int));
 j += FrTOCSort1((char **)&(toc->dt),          index, nFr, sizeof(double));
 j += FrTOCSort1((char **)&(toc->runs),        index, nFr, sizeof(int));
 j += FrTOCSort1((char **)&(toc->frame),       index, nFr, sizeof(int));
 j += FrTOCSort1((char **)&(toc->positionH),   index, nFr, sizeof(FRLONG));
 j += FrTOCSort1((char **)&(toc->nFirstADC),   index, nFr, sizeof(FRLONG));
 j += FrTOCSort1((char **)&(toc->nFirstSer),   index, nFr, sizeof(FRLONG));
 j += FrTOCSort1((char **)&(toc->nFirstTable), index, nFr, sizeof(FRLONG));
 j += FrTOCSort1((char **)&(toc->nFirstMsg),   index, nFr, sizeof(FRLONG));

          /*------------------- read time series part --------------*/

 for(ts = toc->adc; ts != NULL; ts = ts->next)
   {j += FrTOCSort1((char **)&(ts->position), index, nFr, sizeof(FRLONG));}
 for(ts = toc->proc; ts != NULL; ts = ts->next)
   {j += FrTOCSort1((char **)&(ts->position), index, nFr, sizeof(FRLONG));}
 for(ts = toc->sim; ts != NULL; ts = ts->next)
   {j += FrTOCSort1((char **)&(ts->position), index, nFr, sizeof(FRLONG));}
 for(ts = toc->ser; ts != NULL; ts = ts->next)
   {j += FrTOCSort1((char **)&(ts->position), index, nFr, sizeof(FRLONG));}
 for(ts = toc->summary; ts != NULL; ts = ts->next)
   {j += FrTOCSort1((char **)&(ts->position), index, nFr, sizeof(FRLONG));}

 if(j > 0)  
   {fprintf(FrFOut,"FrTOCSort: sorting error, malloc failed\n");
    file->toc = NULL;}

 free(gps);
 free(index);

 return;}

/*--------------------------------------------------------------FrTOCSort1--*/
int FrTOCSort1(char **dataIn, int *index, int nData, int size)
/*--------------------------------------------------------------------------*/
/* This function sort on array by increasing index                          */
/*--------------------------------------------------------------------------*/
{int i;
 char *new;

 new = malloc(nData*size);
 if(new == NULL) return(1);

 for(i=0; i<nData; i++)
   {memcpy(new+i*size, (*dataIn)+index[i]*size, size);}

 free(*dataIn);
 *dataIn = new;

 return(0);}

/*-------------------------------------------------------------- FrTOCStatD--*/
int FrTOCStatD(FrFile *oFile,
               FrStatData *sData)
/*---------------------------------------------------------------------------*/
{FrTOCstat *sm;
 int i;
                               /*-------- find MarkSD structure -------------*/

 if(sData == NULL)      return(2);
 if(oFile->toc == NULL) return(2);

 sm = oFile->toc->stat;
 while(sm != NULL)
   {if((strcmp(sData->detector->name, sm->detector) == 0) &&
       (strcmp(sData->name,           sm->name) == 0)) break;
    sm = sm->next;}

                               /*----- create a new FrTOCstat if needed ----*/
 if(sm == NULL) 
   {sm = (FrTOCstat *) malloc(sizeof(FrTOCstat));
    if(sm == NULL)
       {FrError(3,"FrTOCStatD"," malloc failed");
        oFile->error = FR_ERROR_MALLOC_FAILED; 
        return(2);}
    FrStrCpy(&sm->detector, sData->detector->name);
    FrStrCpy(&sm->name, sData->name);
    sm->maxStat = 4;
    sm->nStat = 0;
    sm->version = (unsigned  int *)malloc(sizeof(int)   * sm->maxStat);
    sm->tStart  = (unsigned  int *)malloc(sizeof(int)   * sm->maxStat);
    sm->tEnd    = (unsigned  int *)malloc(sizeof(int)   * sm->maxStat);
    sm->position= (       FRLONG *)malloc(sizeof(FRLONG)* sm->maxStat);
    if((sm->detector  == NULL) ||
       (sm->name      == NULL) ||
       (sm->version   == NULL) ||
       (sm->tStart    == NULL) ||
       (sm->tEnd      == NULL) ||
       (sm->position== NULL))
         {FrError(3,"FrTOCStatD"," malloc failed");
          oFile->error = FR_ERROR_MALLOC_FAILED; 
          return(2);}
    sm->next = oFile->toc->stat;
    oFile->toc->stat = sm;
    oFile->toc->nStatType++;}

                    /*-------- check if the Static datais already there -----*/

 for(i=0; i<sm->nStat; i++)        
   {if(sm->version[i] != sData->version)   continue;
    if(sm->tStart[i]  != sData->timeStart) continue;
    if(sm->tEnd[i]    != sData->timeEnd)   continue;
    return(1);}

                              /*-------- increase space if needed -----------*/

 if(sm->nStat+1 >= sm->maxStat)
   {sm->maxStat = 2 * sm->maxStat;

    sm->version = (unsigned  int *)realloc(sm->version,
                             sizeof(int) * sm->maxStat);
    sm->tStart  = (unsigned  int *)realloc(sm->tStart,
                             sizeof(int) * sm->maxStat);
    sm->tEnd    = (unsigned  int *)realloc(sm->tEnd,
                             sizeof(int) * sm->maxStat);
    sm->position= (       FRLONG *)realloc(sm->position,
                          sizeof(FRLONG) * sm->maxStat);
    if((sm->version == NULL) ||
       (sm->tStart  == NULL) ||
       (sm->tEnd    == NULL) ||
       (sm->position== NULL))
           {FrError(3,"FrTOCStatD"," realloc failed");
            oFile->error = FR_ERROR_MALLOC_FAILED;
            return(2);}}
                              /*-------------- fill information--------------*/

 sm->version[sm->nStat] = sData->version;
 sm->tStart[sm->nStat]  = sData->timeStart;
 sm->tEnd[sm->nStat]    = sData->timeEnd;
 sm->position[sm->nStat]= oFile->nBytes;
 sm->nStat++;

 return(0);}

/*------------------------------------------------------------FrTOCStatDGet--*/
void FrTOCStatDGet(FrFile *iFile,
                   FrameH *frame)
/*---------------------------------------------------------------------------*/
/* This function read all the static data valid for this frame               */
{FrStatData *sData, *sDataFrame;
 FrTOCstat *sm;
 FrDetector *det;
 unsigned int i;
 FRLONG position;

  if(iFile == NULL) return;
  if(frame == NULL) return;
                    /*-------- search the valid static data for this frame --*/
  position = 0;
  for(sm = iFile->toc->stat; sm != NULL; sm = sm->next)

    {sDataFrame = FrameFindStatData(frame, sm->detector, sm->name, 0);

     for(i=0; i<sm->nStat; i++)
      {if(sm->tStart[i] > frame->GTimeS) continue;
       if(sm->tEnd[i]   < frame->GTimeS) continue;

       if(sDataFrame != NULL)
          {if(sDataFrame->version == sm->version[i]) continue;}

        /*-- this statData is not yet in the frame, but must be in; read it--*/

       iFile->relocation = FR_NO;
       if(position == 0) position = FrIOTell(iFile->frfd);
       if(FrFileIOSet(iFile, sm->position[i]) == -1) {return;}
       FrFileIGoToNextRecord(iFile);
       sData = FrStatDataRead(iFile);
       FrStrCpy(&(sData->detName), sm->detector);

                             /*--------read and attach the vector if any ----*/
       if(FrFileIGoToNextRecord(iFile) != iFile->vectorType) 
	    sData->data = NULL;
       else sData->data = FrVectRead(iFile); 
       iFile->relocation = FR_YES; 

                   /*--- attache it to the frame->detector and to the file---*/

      for(det = frame->detectSim; det != NULL; det = det->next)
        {if(strcmp(sm->detector, det->name) != 0) break; 
         FrStatDataCopy(sData, det);
         sData->next = iFile->sDataSim;
         iFile->sDataSim = sData;}

      for(det = frame->detectProc; det != NULL; det = det->next)
        {if(strcmp(sm->detector, det->name) != 0) break; 
         FrStatDataCopy(sData, det);
         sData->next = iFile->sDataProc;
         iFile->sDataProc = sData;}}}

  if(position != 0) FrFileIOSet(iFile, position);

                        /*------------clean up the static data information --*/
           
 for(det = frame->detectSim; det != NULL; det = det->next)
   {FrStatDataChkT(&det->sData, frame->GTimeS, frame->GTimeS + frame->dt);}
 for(det = frame->detectProc; det != NULL; det = det->next)
   {FrStatDataChkT(&det->sData, frame->GTimeS, frame->GTimeS + frame->dt);}
            
 return;}

/*------------------------------------------------------------FrTOCStatRead--*/
void FrTOCStatRead(FrFile *iFile)
/*---------------------------------------------------------------------------*/
{
  FrTOC *toc;
  FrTOCstat *sm;
  unsigned int i, nTotalStat, nTotalStatRead;
    
  toc = iFile->toc;

  /*------------- get the number of Static data and build the linked list----*/

  FrReadInt   (iFile, &toc->nStatType);
  toc->stat = NULL;
  if(FrDebugLvl > 1) fprintf(FrFOut," nStatType=%d\n",toc->nStatType);

  for(i=0; i<toc->nStatType; i++) {
    sm = (FrTOCstat *) malloc(sizeof(FrTOCstat));
    if(sm == NULL) {
      FrError(3,"FrTOCRead"," malloc failed");
      iFile->error = FR_ERROR_MALLOC_FAILED; 
      return;}
    sm->next = toc->stat;
    toc->stat = sm;}

  /*---------------------------------for frame format version less than 8 ---*/

  if(iFile->fmtVersion < 8) {
    for(sm = toc->stat; sm != NULL; sm = sm->next) {
      FrReadSChar(iFile, &sm->name);
      FrReadSChar(iFile, &sm->detector);
      FrReadInt (iFile,  &sm->nStat);
      sm->maxStat = sm->nStat;
      FrReadVI (iFile, (int **) &sm->tStart,   sm->nStat);
      FrReadVI (iFile, (int **) &sm->tEnd,     sm->nStat);
      FrReadVI (iFile, (int **) &sm->version,  sm->nStat);
      FrReadVL (iFile,          &sm->position, sm->nStat);
      if(iFile->error == FR_ERROR_MALLOC_FAILED) return;}
    return;}

  /*-------------------------------- this is for frame format 8 and above ---*/

  nTotalStatRead = 0;
  for(sm = toc->stat; sm != NULL; sm = sm->next) {
    FrReadSChar(iFile, &sm->name);}
  for(sm = toc->stat; sm != NULL; sm = sm->next) {
    FrReadSChar(iFile, &sm->detector);}
  for(sm = toc->stat; sm != NULL; sm = sm->next) {
    FrReadInt (iFile,  &sm->nStat);
    sm->maxStat = sm->nStat;
    nTotalStatRead += sm->nStat;}

  FrReadIntU(iFile,  &nTotalStat);
  if(nTotalStat != nTotalStatRead) {
    iFile->error = FR_ERROR_BAD_NSTAT;
    FrError(3,"FrTOCStatRead","nTotalStat missmatch");}

  for(sm = toc->stat; sm != NULL; sm = sm->next) {
    FrReadVI (iFile, (int **) &sm->tStart, sm->nStat);}
  for(sm = toc->stat; sm != NULL; sm = sm->next) {
    FrReadVI (iFile, (int **) &sm->tEnd, sm->nStat);}
  for(sm = toc->stat; sm != NULL; sm = sm->next) {
    FrReadVI (iFile, (int **) &sm->version, sm->nStat);}
  for(sm = toc->stat; sm != NULL; sm = sm->next) {
    FrReadVL (iFile,          &sm->position, sm->nStat);}

  return;}

/*-----------------------------------------------------------FrTOCStatWrite--*/
void FrTOCStatWrite(FrFile *oFile)
/*---------------------------------------------------------------------------*/
{
  FrTOC *toc;
  FrTOCstat *sm;
  unsigned int nTotalStat;

  toc = oFile->toc;
  FrPutInt (oFile, toc->nStatType);

  if(FrFormatVersion < 8) {
    for(sm = toc->stat; sm != NULL; sm = sm->next) {
      FrPutSChar (oFile, sm->name); 
      FrPutSChar (oFile, sm->detector); 
      FrPutInt   (oFile, sm->nStat);
      FrPutVI    (oFile, (int*) sm->tStart,   sm->nStat);
      FrPutVI    (oFile, (int*) sm->tEnd,     sm->nStat);
      FrPutVI    (oFile, (int*) sm->version,  sm->nStat);
      FrPutVL    (oFile,        sm->position, sm->nStat);}
    return;}

 /*-------------------------------- This is for format version 8 and above---*/

 nTotalStat = 0;
 for(sm = toc->stat; sm != NULL; sm = sm->next) {
   nTotalStat += sm->nStat;
   FrPutSChar(oFile, sm->name);}
 for(sm = toc->stat; sm != NULL; sm = sm->next) {
   FrPutSChar(oFile, sm->detector);}
 for(sm = toc->stat; sm != NULL; sm = sm->next) {
   FrPutInt   (oFile, sm->nStat);}

 FrPutInt (oFile, nTotalStat);
 for(sm = toc->stat; sm != NULL; sm = sm->next) {
   FrPutVI    (oFile, (int*) sm->tStart,   sm->nStat);}
 for(sm = toc->stat; sm != NULL; sm = sm->next) {
   FrPutVI    (oFile, (int*) sm->tEnd,     sm->nStat);}
 for(sm = toc->stat; sm != NULL; sm = sm->next) {
   FrPutVI    (oFile, (int*) sm->version,  sm->nStat);}
 for(sm = toc->stat; sm != NULL; sm = sm->next) {
   FrPutVL    (oFile,        sm->position, sm->nStat);}

 return;}

/*--------------------------------------------------------------FrTOCtsDump--*/
void FrTOCtsDump(FrTOCts *root,
                 FILE *fp,
                 char *type,
                 char *tag,
                 int nFrame)
/*---------------------------------------------------------------------------*/
{int i, count;
 FrTOCts *ts;
 FrTag *frtag;

 if(tag == NULL) 
      {frtag = NULL;}
 else {frtag = FrTagNew(tag);}

 if(root == NULL)    return;
 fprintf(fp," Channel list for %s:\n",type);

 for(ts = root; ts != NULL; ts = ts->next)
   {if(FrTagMatch(frtag, ts->name) == FR_NO) continue;

    if(type[0]=='A')
        {fprintf(fp,"   (channel=%d group=%d) %s:", 
	              ts->channelID, ts->groupID, ts->name);}
    else{fprintf(fp,"    %s", ts->name);}
 
    count = 0;
    for(i=0; i<nFrame; i++) {if(ts->position[i] != 0) count++;}
    fprintf(fp," in %d frames\n", count);}

 if(frtag != NULL) FrTagFree(frtag);

 return;}
/*--------------------------------------------------------------FrTOCtsFree--*/
void FrTOCtsFree(FrTOCts *ts)
/*---------------------------------------------------------------------------*/
{FrTOCts *next;

 while(ts != NULL)
   {next = ts->next;
    free(ts->name);
    free(ts->position);
    free(ts);
    ts = next;}

 return;}

/*-------------------------------------------------------------FrTOCtsHFree--*/
void FrTOCtsHFree(FrTOCtsH *tsh)
/*---------------------------------------------------------------------------*/
{FrTOCtsH *next;

 while(tsh != NULL)
   {next = tsh->next;
    FrTOCtsFree(tsh->first);
    free(tsh);
    tsh = next;}

 return;}

/*--------------------------------------------------------------FrTOCtsMark--*/
void FrTOCtsMark(FrFile *oFile,
                 FrTOCtsH **root,
                 char *name,
                 unsigned int groupID,
                 unsigned int channelID)
/*---------------------------------------------------------------------------*/
{FrTOCts  *ts, *ts2, **tsp;
 FrTOCtsH *tsh, *last;
 int threshold, th, n, nts, test;
 
 if(oFile->noTOCts == FR_YES) return;

 if(name == NULL)  return;

        /*------------- create at least one header structure-----------------*/

 if(*root == NULL)
    {tsh = (FrTOCtsH *) calloc(1, sizeof(FrTOCtsH));
     if(tsh == NULL) return;
     *root = tsh;}

       /*--------------- search the header structure which own this ts-------*/

 last = *root; 
 for(tsh = *root; tsh != NULL; tsh = tsh->next)
   {if(tsh->first == NULL) break;
    if(strcmp(tsh->first->name, name) > 0) break;
    last = tsh;}

        /*----------- search the slot where the ts should be included--------*/

 ts = NULL;
 for(tsp = &(last->first); *tsp != NULL; tsp = &((*tsp)->next))
   {test = strcmp((*tsp)->name, name);
    if(test < 0) continue; 
    if(test == 0) {ts = *tsp;}
    break;}

        /*--------- if no ts structure existe create a new one --------------*/

 if(ts == NULL)
   {ts = (FrTOCts *) malloc(sizeof(FrTOCts));
    if(ts == NULL) return;

    FrStrCpy(&ts->name, name);
    ts->channelID = channelID;
    ts->groupID   = groupID;
    ts->position  = (FRULONG *)calloc(oFile->toc->maxFrame,sizeof(FRULONG));
    if(ts->position == NULL)
        {FrError(3,"FrTOCtsMark"," malloc buffer failed");
         oFile->error = FR_ERROR_MALLOC_FAILED;
         return;}
                               /*----------- insert it in the link list -----*/
    ts->next = *tsp;
    *tsp = ts;
    last->nElement++;

         /*---------------- should be split the list in two?-----------------*/

    nts = 0;
    for(tsh = *root; tsh != NULL; tsh = tsh->next) {nts += tsh->nElement;}
    threshold = 1.5*sqrt((double)nts);
    if(threshold < 15) threshold = 15;

    if(last->nElement > threshold)
      {n = 0;
       th = threshold/2;
       for(ts2 = last->first; ts2 != NULL; ts2 = ts2->next) 
          {n++;
           if(n > th) break;}

       tsh = (FrTOCtsH *) calloc(1, sizeof(FrTOCtsH));
       if(tsh == NULL) return;

       tsh->next = last->next;
       last->next = tsh;
   
       tsh->first = ts2->next;
       ts2->next = NULL;
       tsh->nElement = last->nElement - n;  
       last->nElement = n;}}

        /*------ check that the name is unique for ADC, sim and proc data----*/

 if((ts->position[oFile->toc->nFrame] != 0) && (channelID != 0xffffffff))
   {if(FrDebugLvl > 1) fprintf(FrFOut, 
         " FrTOCtsMark: Duplicate channel: %s !!! WARNING !!!\n",name);}

        /*-------------- fill information------------------------------------*/

 ts->position[oFile->toc->nFrame]= oFile->nBytes;

 return;}

/*--------------------------------------------------------------FrTOCtsRead--*/
FrTOCts *FrTOCtsRead(FrFile *iFile,
                     char *type,
                     int nFrame)
/*---------------------------------------------------------------------------*/
{FrTOCts *ts;
 FrTOCts *root;
 unsigned int i, nInstance, nFix;

 if(iFile->error != FR_OK) return(NULL);

 root= NULL;
 FrReadIntU(iFile, &nInstance);
 if(FrDebugLvl > 1) fprintf(FrFOut," nInstance=%d for %s\n",nInstance,type);

 if(nInstance == 0xffffffff) return(NULL);

 for(i=0; i<nInstance; i++)
   {ts = (FrTOCts*)malloc(sizeof(FrTOCts));
    if(ts == NULL) 
       {FrError(3,"FrTOCtsRead"," malloc adcfailed");
        iFile->error = FR_ERROR_MALLOC_FAILED; 
        return(NULL);}
    ts->next = root;
    root = ts;}

 for(ts = root; ts != NULL; ts = ts->next) 
   {FrReadSChar(iFile, &ts->name);}

 if(type[0] == 'A')
   {for(ts = root; ts != NULL; ts = ts->next)
      {FrReadIntU(iFile, &ts->channelID);}
    for(ts = root; ts != NULL; ts = ts->next)
      {FrReadIntU(iFile, &ts->groupID);}}

 for(ts = root; ts != NULL; ts = ts->next)
   {FrReadVL (iFile, (FRLONG **)&ts->position, nFrame);}
 if(iFile->error == FR_ERROR_MALLOC_FAILED) return(NULL);

         /*----- check time that frame element are within the right frame----*/

 nFix = 0; 
 for(ts = root; ts != NULL; ts = ts->next)
   {for(i=0; i<nFrame; i++)
      {if(ts->position[i] == 0) continue;
       if(ts->position[i] < iFile->toc->positionH[i])  
         {nFix++;
          ts->position[i] = 0;}
       if(i<nFrame-1)
         {if(ts->position[i] > iFile->toc->positionH[i+1])
            {nFix++;
	     ts->position[i] = 0;}}
      else
         {if(ts->position[nFrame-1] > iFile->nBytesE)
            {nFix++;
	     ts->position[i] = 0;}}}}

 if(nFix > 0) FrError(3,"FrTOCtsRead"," TOC positions required fixes");
  
 return(root);}

/*-----------------------------------------------------------FrTOCtsRealloc--*/
void FrTOCtsRealloc(FrTOCtsH *tsh,
                    FrFile   *oFile,
                    int       oldSize)
/*---------------------------------------------------------------------------*/
{FrTOCts  *ts;
 FRULONG *newP;

 for(; tsh != NULL; tsh = tsh->next)
   {for(ts = tsh->first; ts != NULL; ts = ts->next)
     {if(FrDebugLvl > 2) printf("FrTOCtsHRealloc for %s maxFrame=%d\n",
                                       ts->name,oFile->toc->maxFrame);
      newP  = (FRULONG *)calloc(oFile->toc->maxFrame,sizeof(FRULONG));
      if(newP == NULL)
           {FrError(3,"FrTOCtsHRealloc"," realloc failed");
            oFile->error = FR_ERROR_MALLOC_FAILED;
            return;}

      memcpy(newP, ts->position, oldSize*sizeof(FRULONG)); 
      free(ts->position);
      ts->position = newP;}}

 return;}

/*-------------------------------------------------------------FrTOCtsWrite--*/
void FrTOCtsWrite(FrFile  *oFile,
                  FrTOCtsH *root,
                  char *type,
                  int nFrame)
/*---------------------------------------------------------------------------*/
{FrTOCts *ts;
 FrTOCtsH *tsh;
 unsigned int nInstance;

 if(oFile->noTOCts == FR_YES) 
    {FrPutIntU(oFile, 0xffffffff);
     return;}

 nInstance = 0;
 for(tsh = root; tsh != NULL; tsh = tsh->next) {nInstance += tsh->nElement;}

 FrPutIntU(oFile, nInstance);
 
 for(tsh = root; tsh != NULL; tsh = tsh->next)
   {for(ts = tsh->first; ts != NULL; ts = ts->next)
     {FrPutSChar(oFile, ts->name);}}

 if(type[0] == 'A')
   {for(tsh = root; tsh != NULL; tsh = tsh->next)
      {for(ts = tsh->first; ts != NULL; ts = ts->next)
	{FrPutIntU(oFile, ts->channelID);}}
    for(tsh = root; tsh != NULL; tsh = tsh->next)
      {for(ts = tsh->first; ts != NULL; ts = ts->next)
	{FrPutIntU(oFile, ts->groupID);}}}

 for(tsh = root; tsh != NULL; tsh = tsh->next)
   {for(ts = tsh->first; ts != NULL; ts = ts->next)
     {FrPutVL (oFile, (FRLONG *) ts->position, nFrame);}}
  
 return;}

/*---------------------------------------------------------------FrTOCWrite--*/
void FrTOCWrite(FrFile *oFile)
/*---------------------------------------------------------------------------*/
{FrTOC *toc;

 toc = oFile->toc;
 if(toc == NULL) return;

 if(FrDebugLvl > 1) FrTOCDump(oFile->toc, FrFOut, FrDebugLvl-1, NULL);

           /*--- write first the dictionary to get the real header position--*/

 if(oFile->dicWrite[toc->classe->id] == FR_NO) 
   {oFile->dicWrite[toc->classe->id] = FR_YES; 
    FrSHWrite(toc->classe, oFile);}  

 toc->position = oFile->nBytes;
 
            /*------------------ write now the data bloc --------------------*/

 FrPutNewRecord(oFile, toc, FR_NO);
 FrPutShortU   (oFile, toc->ULeapS);
 FrPutInt      (oFile, toc->nFrame+1);
 FrPutVI       (oFile,     (int *) toc->dataQuality,toc->nFrame+1);
 FrPutVI       (oFile,     (int *) toc->GTimeS,     toc->nFrame+1);
 FrPutVI       (oFile,     (int *) toc->GTimeN,     toc->nFrame+1);
 FrPutVD       (oFile,             toc->dt,         toc->nFrame+1);
 FrPutVI       (oFile,             toc->runs,       toc->nFrame+1);
 FrPutVI       (oFile,     (int *) toc->frame,      toc->nFrame+1);
 FrPutVL       (oFile, (FRLONG *)  toc->positionH,  toc->nFrame+1);
 FrPutVL       (oFile, (FRLONG *)  toc->nFirstADC,  toc->nFrame+1);
 FrPutVL       (oFile, (FRLONG *)  toc->nFirstSer,  toc->nFrame+1);
 FrPutVL       (oFile, (FRLONG *)  toc->nFirstTable,toc->nFrame+1);
 FrPutVL       (oFile, (FRLONG *)  toc->nFirstMsg,  toc->nFrame+1);

 FrPutInt      (oFile, toc->nSH);
 FrPutVS       (oFile, (short *) toc->SHid,   toc->nSH);
 FrPutVQ       (oFile,           toc->SHname, toc->nSH);

 FrTOCdetWrite(oFile);
                                /*------------ Static data part--------------*/
 
 FrTOCStatWrite(oFile);
                                /*------------ Time series part--------------*/
 
 FrTOCtsWrite(oFile, toc->adcH,         "ADC", toc->nFrame+1);
 FrTOCtsWrite(oFile, toc->procH,       "proc", toc->nFrame+1);
 FrTOCtsWrite(oFile, toc->simH,         "sim", toc->nFrame+1);
 FrTOCtsWrite(oFile, toc->serH,         "ser", toc->nFrame+1);
 FrTOCtsWrite(oFile, toc->summaryH, "summary", toc->nFrame+1);

 FrTOCevtWrite(oFile, toc->event);
 FrTOCevtWrite(oFile, toc->simEvt);
 
                       /*--------------- store the marks length -------------*/

 FrPutWriteRecord(oFile, FR_NO);

 return;}

/*---------------------------------------------------------------FrVectCtoH--*/
int FrVectCtoH(FrVect *vect)
/*---------------------------------------------------------------------------*/
/* Try to convert regular complex vector to Half complex vector              */
/*---------------------------------------------------------------------------*/
{double *dataD;
 float *dataF;
 FRLONG i, nData, a, end;

 if(vect == NULL) return(-1); 
 if(vect->compress != 0) FrVectExpand(vect);

 nData = vect->nData;
 a = 2*nData;
 end = (1+nData)/2;
                                /*------------- Complex number with float ---*/
 if(vect->type == FR_VECT_8C)
   {dataF = vect->dataF;              
    if(dataF[1] != 0.) return(1);
    if((nData%2 == 0) && (dataF[nData+1] != 0.)) return(2);

    for (i=1; i<end; i++) 
      {if(dataF[a-2*i]   != dataF[2*i])   return(3);
       if(dataF[a-2*i+1] !=-dataF[2*i+1]) return(4);}
                             /*--- the vector is half complex, convert it ---*/
    for (i=1; i<end; i++) {dataF[i]   = dataF[2*i];}
    if(nData%2 == 0)       dataF[end] = dataF[2*end]; 
    for (i=1; i<end; i++) {dataF[nData-i] = -dataF[a-2*i+1];} 
    vect->type = FR_VECT_8H;} 
                                /*----------------- same code with double ---*/

 else if(vect->type == FR_VECT_16C)
   {dataD = vect->dataD;              
    if(dataD[1] != 0.) return(1);
    if((nData%2 == 0) && (dataD[nData+1] != 0.)) return(2);

    for (i=1; i<end; i++) 
      {if(dataD[a-2*i]   != dataD[2*i])   return(3);
       if(dataD[a-2*i+1] !=-dataD[2*i+1]) return(4);}
                             /*--- the vector is half complex, convert it ---*/
    for (i=1; i<end; i++) {dataD[i]   = dataD[2*i];}
    if(nData%2 == 0)       dataD[end] = dataD[2*end]; 
    for (i=1; i<end; i++) {dataD[nData-i] = -dataD[a-2*i+1];}
    vect->type = FR_VECT_16H;} 

 else{return(-3);}

 vect->nBytes = vect->nBytes/2;
 vect->wSize  = vect->wSize/2;
 
 return(0);}

/*----------------------------------------------------------FrVectCompData---*/
unsigned short FrVectCompData(FrVect *vect,
                              int compType,
                              int gzipLevel,
                              unsigned char **result,
                              FRULONG *nBytes)
/*---------------------------------------------------------------------------*/
/* compType = 1 -> gzip
            = 3 -> differentiate + gzip
            = 5 -> differentiate + Zcomp for short and int
            = 6 -> differentiate + Zcomp for short and int, gzip for others
            = 7 -> differentiate + Zcomp for short, int and float to integer
            = 8 -> differentiate + Zcomp for short, int and float.
            = 255->User define compression code
  This function returns the compression code used.                           */
/*---------------------------------------------------------------------------*/
{int err, compValue, compVect, oldType;
 short ref;
 unsigned char *compr;
 unsigned long comprLenL;
 char *swap, *dataIn;
 FRULONG comprLen, comp3, comp8, nBytes3, nBytes8;
 unsigned char *data3, *data8;

 if(vect == NULL)                 return(0);

      /*---- here we try all the compression algo and keep the best ------*/

 if(compType == 9)
   {comp3 = 0;
    comp8 = 0;
                     /*---- save the vector if it is already compressed--*/
    if(vect->compress != 0)  
      {if(vect->compress == 3 || vect->compress == 256+3)
	  {nBytes3 = vect->nBytes;
	   data3 = malloc(vect->nBytes);
	   if(data3 != NULL)
	        {memcpy(data3, vect->data, vect->nBytes);
	         comp3 = vect->compress;}
           else {comp3 = 0;}}
	else if(vect->compress == 8 || vect->compress == 256+8)
	  {nBytes8 = vect->nBytes;
           data8 = malloc(vect->nBytes);
	   if(data8 != NULL)
	        {memcpy(data8, vect->data, vect->nBytes);
	         comp8 = vect->compress;}
	   else {comp8 = 0;}}
	FrVectExpand(vect);}
                                 /*----- now try both compression scheme ---*/

    if(comp3 == 0) comp3 = FrVectCompData(vect, 3, 1, &data3, &nBytes3);
    if(comp8 == 0) comp8 = FrVectCompData(vect, 8, 1, &data8, &nBytes8);
    if(comp3 == 0) nBytes3 = 10*vect->nBytes;
    if(comp8 == 0) nBytes8 = 10*vect->nBytes;
    if(nBytes3 < nBytes8)
      {if(comp8 != 0) free(data8);
       *result = data3;
       *nBytes = nBytes3;
       return(comp3);}
    else
      {if(comp3 != 0) free(data3);
       *result = data8;
       *nBytes = nBytes8;
       return(comp8);}}

      /*-------- first check if we need/can do the compression---------------*/

 if(vect->type == FR_VECT_STRING) return(0);
 if(vect->nData < 8)              return(0);

     /*-------- determine and check the type of compression requested -------*/

 if(compType == 3) 
   {if(vect->type != FR_VECT_C && vect->type != FR_VECT_1U &&
       vect->type != FR_VECT_2S&& vect->type != FR_VECT_2U &&
       vect->type != FR_VECT_4S&& vect->type != FR_VECT_4U) compType = 1;}  

 else if(compType == 4)
   {if     (vect->type == FR_VECT_2S) compType = 3;
    else if(vect->type == FR_VECT_2U) compType = 3;
    else compType = 0;}

 else if(compType == 5)
   {if((vect->type != FR_VECT_2S && 
        vect->type != FR_VECT_2U)) compType = 0;}

 else if(compType == 6)
   {if     (vect->type == FR_VECT_2S) compType = 5;
    else if(vect->type == FR_VECT_2U) compType = 5;
    else compType = 1;}

 /* else if(compType == 7)   ----- This is not part of the spec (test only)---
   {if     (vect->type == FR_VECT_2S) compType = 5;
    else if(vect->type == FR_VECT_2U) compType = 5;
    else if(vect->type == FR_VECT_4R) compType = 7;
    else compType = 0;} -----------------------------------------------------*/

 else if(compType == 8)
   {if     (vect->type == FR_VECT_2S) compType = 5;
    else if(vect->type == FR_VECT_2U) compType = 5;
    else if(vect->type == FR_VECT_4R) compType = 8;
    else if(vect->type == FR_VECT_4S) compType = 8;
    else if(vect->type == FR_VECT_4U) compType = 8;
    else compType = 0;}

 else if((compType != 0) &&(compType != 1) &&(compType != 255)) /*-- unknown compression type--*/
   {return(0);}

           /*------ determine the type of compression for this vector -------*/

 compVect = vect->compress;         /*--- this is for backward compatibility */
 if(compVect == 6)   
   {if((vect->type == FR_VECT_2S || 
        vect->type == FR_VECT_2U)) compVect = 5;
    else compVect = 1;}

 if(compType == compVect) return(0);

           /*---------- if the vector is already compress, first expand it---*/

 if(vect->compress != 0)  FrVectExpand(vect);
 if(compType == 0) return(0);

           /*-------------------- reserve the output space ------------------*/
           /*----Remark: for gzip, the  size of the destination buffer, 
                  must be at least 0.1% larger than sourceLen plus 12 bytes. */

 comprLen  = 1.01*vect->nBytes + 13;
 comprLenL = comprLen;
 compr    = malloc(comprLen);
 if(compr == NULL) 
    {FrError(3,"FrCompress","malloc failed");
     return(0);}

               /*--------------------- do compression -----------------------*/

 err = 0;

 if(compType == 1)                   /*--------------------- plain gzip -----*/
   {err = Frz_compress(compr, &comprLenL, vect->data, vect->nBytes,gzipLevel);
    if(comprLenL > vect->nBytes) err = 1;
    comprLen = comprLenL;}

 else if(compType == 3)             /*---------------------- diff + gzip ----*/
   {dataIn = FrVectDiff(vect);
    if(dataIn == NULL) return(0);
    err = Frz_compress(compr, &comprLenL, dataIn, vect->nBytes,gzipLevel);
    if(comprLenL > vect->nBytes) err = 1;
    comprLen = comprLenL;}

 else if(compType == 5)         /*---- diff + zero supress (2 bytes words)---*/
   {dataIn = FrVectDiff(vect);
    if(dataIn == NULL) return(0);
    err = FrVectZComp((unsigned short *)compr, &comprLen, 
                          (short *) dataIn, vect->nData,12);}

 /*-- else if(compType == 7)   ---- float to integer conversion (test only)--
   {err = FrVectFComp((short *)compr, &comprLen,
                      vect->dataF, vect->nData,17-gzipLevel);} --------------*/
 
 else if(compType == 8)      /*------- diff + zero supress (4 bytes words)---*/
   {oldType = vect->type;
    vect->type = FR_VECT_4S;
    dataIn = FrVectDiff(vect);
    if(dataIn == NULL) 
      {vect->type = oldType;
       return(0);}
    err = FrVectZCompI((unsigned int *)compr, &comprLen, 
	                (int *) dataIn, vect->nData,8);
    vect->type = oldType;}

 else if(compType == 255)     /*----------- user defined code (test only) ---*/
   {err = FrVectUComp(vect, compr, &comprLen, &compType);}

            /*------------- check error code --------------------------------*/

 if (err != 0) 
    {if(FrDebugLvl > 1) fprintf(FrFOut,"FrVectCompData: compression failed " 
                        "for vector:%s compType=%d\n",vect->name, compType);
     free(compr);
     return(0);}

                     /*------------ prepared the returned arguments----------*/

 *result = compr;
 *nBytes = comprLen;

 compValue = compType;

 swap = (char *)&ref;
 ref = 0x1234;
 if(swap[0] == 0x34) compValue = 256+compValue;

 return(compValue);}

/*----------------------------------------------------------FrVectCompress---*/
void FrVectCompress(FrVect *vect,
                    int compType,
                    int gzipLevel)
/*---------------------------------------------------------------------------*/
{unsigned char *data;
 unsigned int compress;
 FRULONG nBytes;

 if(vect == NULL) return;
 if(compType == vect->compress) return;

 compress = FrVectCompData(vect, compType, gzipLevel,&data, &nBytes);
    
 if(compress == 0) return;

 if(vect->dataUnzoomed == 0)
       free(vect->data);
 else  free(vect->dataUnzoomed);
 
 vect->data     = (char *) data;
 vect->nBytes   = nBytes;
 vect->compress = compress;
 FrVectMap(vect);

 return;}

/*-------------------------------------------------------------FrVectConcat--*/
FrVect *FrVectConcat(FrVect *vectIn, double tStart, double len)
/*---------------------------------------------------------------------------*/
/* This function takes a link list of 3d vectors and convert it to a single  */
/* vector starting at tStart (GPS time in second) of length len (in second). */
/* The input vector list is free in case of success                          */
/* Remark: all vectors are assumed to have the same type and same dx.        */
/* The time is suposed to be in the first dimension                          */
/*-------------------------------------------------------------------------- */
{
  FrVect *vect, *next, *output, *isThere, *firstVect, *auxVect, *mainVect;
  FRLONG iO, iS, iE;
  FRBOOL isAux;
  FRULONG nDataTime, i, type, wSize, blockSize, gtime;
  double  dt, dO, start;

  if(vectIn == NULL) return(NULL);

    /*-----first extract all the auxiliairy "Available_data" vectors.
    They are recongnise since they have no GPS time (fast detection method)
    but we also have to check the name since v8r12 since some vector may 
    have not time information-------------------*/
  mainVect = NULL;
  auxVect  = NULL;
  gtime = 0;

  for(vect = vectIn; vect != NULL; vect = next) {
    next = vect->next;

    isAux = FR_NO;  /*-------------- first to check for auxiliary vector---*/
    if(vect->type == FR_VECT_C) { 
      if(vect->name[0] == 'A') {
        if(strncmp(vect->name,"Available_data",14) == 0){
	  isAux = FR_YES;}}}

    if(isAux == FR_NO) {   /*------------this shoudd be a regular vector---*/
      vect->next = mainVect;
      mainVect = vect;
      gtime = vect->GTime;}
    else {                 /*----------------this is an auxiliary vector---*/
      vect->next = auxVect;
      vect->GTime = gtime;
      auxVect = vect;}}
  
  vectIn = mainVect;
  if(vectIn == NULL) { /*--- this should not hapen, unless there is a bug---*/
    FrVectFree(auxVect);
    return(NULL);}

/*--------------------------------------------now process the main vector---*/
  /*--find the first vector (the linked list may be stored in reverse order)--*/
  firstVect = vectIn;
  for(vect = vectIn; vect != NULL; vect = vect->next) {
    if(vect->GTime < firstVect->GTime) firstVect = vect;

    if(vect->nDim == 3) {/*backward compatibility for Virgo images before v8r05*/
      if(vect->nx[2] == 1) FrVectFixVirgoImage(vect);}}

  /*----------------------------------------- compute the exact start time---*/
  if(tStart < firstVect->GTime) {tStart = tStart - 1.e-6;}
  dt     = firstVect->dx[0];
  start  = ((tStart - firstVect->GTime) + 1.e-6 *dt)/dt;
  iS     = start;
  if(start < 0) iS--;  /*--  since 0.2 and -0.2 both convert to 0 --*/
  tStart = firstVect->GTime + iS * dt;

  /*--------------------------------------------- create the output vector---*/
  vect  = vectIn;
  nDataTime = (len+.25*dt)/dt;
  type  = vect->type;
 
  if(vect->nDim == 1) {
    output  =  FrVectNewL(type,1, nDataTime, dt, vect->unitX[0]);}
  else if(vect->nDim == 2) {
    output  =  FrVectNewL(type,2,
			  nDataTime,   dt,          vect->unitX[0],
			  vect->nx[1], vect->dx[1], vect->unitX[1]);}
  else if(vect->nDim == 3) {
    output  =  FrVectNewL(type,3,
			  nDataTime,   dt,          vect->unitX[0],
			  vect->nx[1], vect->dx[1], vect->unitX[1],
			  vect->nx[2], vect->dx[2], vect->unitX[2]);}
  else {return(NULL);}
  if(output  == NULL) return(NULL);

  /*---------------------------------------------------fill extra metadata---*/
  FrVectSetName(output, vect->name);
  FrStrCpy(&output->unitY, vect->unitY);

  output->GTime    = tStart;
  output->localTime= vect->localTime;
  output->ULeapS   = vect->ULeapS;

  output->startX[0] =  vect->startX[0];
  if(vect->nDim > 1) {output->startX[1] =  vect->startX[1];}
  if(vect->nDim > 2) {output->startX[2] =  vect->startX[2];}

  sprintf(FrBuf,"Available_data_for_%s",vect->name);
  isThere = FrVectNew1D(FrBuf,  FR_VECT_C,  nDataTime, dt,
			vect->unitX[0], "1 if data is there");
  if(isThere == NULL) return(NULL);
  isThere->GTime    = tStart;
  isThere->localTime= vect->localTime;
  isThere->ULeapS   = vect->ULeapS;
  for(i=0; i<isThere->nData; i++) {isThere->data[i] = 0.;}

  /*-------------------------------- copy each vector in the output vector---*/
  for(;vect != NULL; vect = vect->next) {
    if(vect->compress != 0) {
      FrVectExpand(vect);
      if(vect->compress != 0) {
	FrError(3,"FrVectConcat","expand failed");
	break;}}

    if(vect->dx[0] != dt) {           /*----- check vector compatibility ----*/
      FrError(3,"FrVectConcat","dt has changed");
      break;}
    if(vect->type != output->type) {
      FrError(3,"FrVectConcat","vector type has changed");
      break;}

    /*-------------------------------------------------------copy the data---*/
    iS = 0;
    dO = (vect->GTime - output->GTime + .25*dt)/dt;
    iO = dO;
    if(dO < 0) {
      iO --;
      iS = -iO;}
    iE = vect->nx[0];
    if(iO+iE > output->nx[0]) {
      iE = output->nx[0] - iO;}

    /*----- compute the "word" size. For multidimensional array, a word is
                 one block of data taken at the same time, like one image---*/ 

    wSize = vect->wSize;
    if(vect->nDim >= 2) wSize *= vect->nx[1];
    if(vect->nDim >= 3) wSize *= vect->nx[2];

    blockSize = wSize*(iE-iS);
    if(blockSize <= 0) continue;
    memcpy(output->data+(iS+iO)*wSize, vect->data+iS*wSize, blockSize);

    for(i=iS;i<iE;i++) {++(isThere->data[i+iO]);}}

  /*----if we did not reach the end of the linked list: there was an error---*/
  if(vect != NULL) {
    FrVectFree(output);
    FrVectFree(isThere);
    return(NULL);}

  /*-----------------------------------------succes, free the input vector---*/
  FrVectFree(vectIn);

  /*-----------------------------------------------copy the auxiliary data---*/
  for(vect = auxVect; vect != NULL; vect = vect->next){
    iS = 0;
    dO = (vect->GTime - output->GTime + .25*dt)/dt;
    iO = dO;
    if(dO < 0) {
      iO --;
      iS = -iO;}
    iE = vect->nx[0];
    if(iO+iE > output->nx[0]) {
      iE = output->nx[0] - iO;}
  
    wSize = vect->wSize;
    blockSize = wSize*(iE-iS);
    if(blockSize <= 0) continue;
    memcpy(isThere->data+(iS+iO)*wSize, vect->data+iS*wSize, blockSize);}

  /*----------------------clean the auxiliary vector if needed or build it---*/
  if(auxVect != NULL) FrVectFree(auxVect);
  
  for(i=0; i<isThere->nData; i++) {if(isThere->data[i] == 0) break;}
  if(i == isThere->nData) {FrVectFree(isThere);}
  else {output->next = isThere;}
  
  return(output);}

/*---------------------------------------------------------------FrVectCopy--*/
FrVect *FrVectCopy(FrVect *vectin)
/*---------------------------------------------------------------------------*/
{FrVect *vect;
 int i;

 if(vectin == NULL) return(NULL);

   vect = (FrVect *) calloc(1,sizeof(FrVect));
   if(vect == NULL) return(NULL);
   vect->classe = FrVectDef();

   if(FrStrCpy(&vect->name, vectin->name) == NULL) return(NULL); 

   vect->compress = vectin->compress;
   vect->type     = vectin->type;
   vect->nData    = vectin->nData;
   vect->nBytes   = vectin->nBytes;
   vect->nDim     = vectin->nDim;
   vect->nx    = malloc(vectin->nDim*sizeof(FRULONG));
   vect->unitX = malloc(vectin->nDim*sizeof(char *));
   vect->dx    = malloc(vectin->nDim*sizeof(double));
   vect->startX= malloc(vectin->nDim*sizeof(double));
   if(vect->nx    == NULL || 
      vect->unitX == NULL || 
      vect->dx    == NULL || 
      vect->startX== NULL) return(NULL);

   for(i=0; i<vectin->nDim; i++)
     {vect->nx[i]     = vectin->nx[i];
      vect->dx[i]     = vectin->dx[i];
      vect->startX[i] = vectin->startX[i];
      FrStrCpy(&vect->unitX[i], vectin->unitX[i]);
      if(vect->unitX[i] == NULL && vectin->unitX[i] != NULL) return(NULL);}


   FrStrCpy(&vect->unitY, vectin->unitY);

   vect->wSize = vectin->wSize;
   vect->space = vectin->space;
   vect->data = malloc(vect->nBytes);
   if(vect->data == NULL) 
      {FrError(3,"FrVectCopy","malloc failed");
       return(NULL);}

   if(vect->type == FR_VECT_STRING)
     {vect->dataQ = (char **) vect->data;
      for(i=0; i<vect->nData; i++) 
        {if(FrStrCpy(&vect->dataQ[i],vectin->dataQ[i]) == NULL) return(NULL);}}
   else{memcpy(vect->data, vectin->data, vect->nBytes);}

   FrVectMap(vect);

   if(vectin->next != NULL) 
        {vect->next = FrVectCopy(vectin->next);}
   else {vect->next = NULL;}

   vect->GTime     = vectin->GTime;
   vect->ULeapS    = vectin->ULeapS;
   vect->localTime = vectin->localTime;

   return(vect);}
/*----------------------------------------------------------FrVectCopyPartI--*/
FrVect *FrVectCopyPartI(FrVect *vect,
                        int iStart, 
			int nTot)
/*---------------------------------------------------------------------------*/
{FrVect *copy;
 
  FrVectZoomInI(vect, iStart, nTot);
  
  copy = FrVectCopy(vect);
  
  FrVectZoomOut(vect);
  
  return(copy);}
  
/*----------------------------------------------------------FrVectCopyPartX--*/
FrVect *FrVectCopyPartX(FrVect *vect,
                        double xStart, 
		        double length)
/*---------------------------------------------------------------------------*/
{FrVect *copy;
 
  FrVectZoomIn(vect, xStart, length);
  
  copy = FrVectCopy(vect);
  
  FrVectZoomOut(vect);
  
  return(copy);}
  
/*-------------------------------------------------------------FrVectCopyTo--*/
FrVect *FrVectCopyTo(FrVect *vIn,
                     double scale,
                     FrVect *copy)
/*---------------------------------------------------------------------------*/
/* This function copy the data from the vector vIn to the vector copy using  */
/* the scale factor 'scale' and casted to the vector copy type.              */
/* It return NULL in case of error (malloc failed, no input vectors).        */
/*---------------------------------------------------------------------------*/
{int i;

 if(vIn  == NULL)              return(NULL);
 if(copy == NULL)              return(NULL);
 if(copy->nData != vIn->nData) return(NULL);

 if(vIn->compress != 0) FrVectExpand(vIn);
 
 for(i=0; i<vIn->nDim; i++) {copy->startX[i] = vIn->startX[i];}
 copy->GTime = vIn->GTime;

                   /*----------------------- copy to D ----------------------*/

 if(copy->type == FR_VECT_8R)  
  {if(scale == 1.)
    {if(vIn->type == FR_VECT_8R)
      {memcpy(copy->dataD, vIn->dataD, vIn->nData*sizeof(double));}
     else if(vIn->type == FR_VECT_C)
      {for(i=0; i<copy->nData; i++)  {copy->dataD[i] = vIn->data[i];}}
     else if(vIn->type == FR_VECT_2S)
      {for(i=0; i<copy->nData; i++)  {copy->dataD[i] = vIn->dataS[i];}}
     else if(vIn->type == FR_VECT_8R)
      {for(i=0; i<copy->nData; i++)  {copy->dataD[i] = vIn->dataD[i];}}
     else if(vIn->type == FR_VECT_4R)
      {for(i=0; i<copy->nData; i++)  {copy->dataD[i] = vIn->dataF[i];}}
     else if(vIn->type == FR_VECT_4S)
      {for(i=0; i<copy->nData; i++)  {copy->dataD[i] = vIn->dataI[i];}}
     else if(vIn->type == FR_VECT_8S)
      {for(i=0; i<copy->nData; i++)  {copy->dataD[i] = vIn->dataL[i];}}
     else if(vIn->type == FR_VECT_2U)
      {for(i=0; i<copy->nData; i++)  {copy->dataD[i] = vIn->dataUS[i];}}
     else if(vIn->type == FR_VECT_4U)
      {for(i=0; i<copy->nData; i++)  {copy->dataD[i] = vIn->dataUI[i];}}
     else if(vIn->type == FR_VECT_8U)
      {for(i=0; i<copy->nData; i++)  {copy->dataD[i] = vIn->dataUL[i];}}
     else if(vIn->type == FR_VECT_1U)
      {for(i=0; i<copy->nData; i++)  {copy->dataD[i] = vIn->dataU[i];}}
     else {return(NULL);}}
   else
    {if(vIn->type == FR_VECT_8R) 
      {for(i=0; i<copy->nData; i++)  {copy->dataD[i] = scale*vIn->dataD[i];}}
     else if(vIn->type == FR_VECT_C)
      {for(i=0; i<copy->nData; i++)  {copy->dataD[i] = scale*vIn->data[i];}}
     else if(vIn->type == FR_VECT_2S)
      {for(i=0; i<copy->nData; i++)  {copy->dataD[i] = scale*vIn->dataS[i];}}
     else if(vIn->type == FR_VECT_8R)
      {for(i=0; i<copy->nData; i++)  {copy->dataD[i] = scale*vIn->dataD[i];}}
     else if(vIn->type == FR_VECT_4R)
      {for(i=0; i<copy->nData; i++)  {copy->dataD[i] = scale*vIn->dataF[i];}}
     else if(vIn->type == FR_VECT_4S)
      {for(i=0; i<copy->nData; i++)  {copy->dataD[i] = scale*vIn->dataI[i];}}
     else if(vIn->type == FR_VECT_8S)
      {for(i=0; i<copy->nData; i++)  {copy->dataD[i] = scale*vIn->dataL[i];}}
     else if(vIn->type == FR_VECT_2U)
      {for(i=0; i<copy->nData; i++)  {copy->dataD[i] = scale*vIn->dataUS[i];}}
     else if(vIn->type == FR_VECT_4U)
      {for(i=0; i<copy->nData; i++)  {copy->dataD[i] = scale*vIn->dataUI[i];}}
     else if(vIn->type == FR_VECT_8U)
      {for(i=0; i<copy->nData; i++)  {copy->dataD[i] = scale*vIn->dataUL[i];}}
     else if(vIn->type == FR_VECT_1U)
      {for(i=0; i<copy->nData; i++)  {copy->dataD[i] = scale*vIn->dataU[i];}}
     else {return(NULL);}}}

                   /*----------------------- copy to F ----------------------*/

 else if(copy->type == FR_VECT_4R)  
  {if(scale == 1.)
    {if(vIn->type == FR_VECT_4R)
     {memcpy(copy->dataF, vIn->dataF, copy->nData * sizeof(float));}
     else if(vIn->type == FR_VECT_C)
      {for(i=0; i<copy->nData; i++)  {copy->dataF[i] = vIn->data[i];}}
     else if(vIn->type == FR_VECT_2S)
      {for(i=0; i<copy->nData; i++)  {copy->dataF[i] = vIn->dataS[i];}}
     else if(vIn->type == FR_VECT_8R)
      {for(i=0; i<copy->nData; i++)  {copy->dataF[i] = vIn->dataD[i];}}
     else if(vIn->type == FR_VECT_4S)
      {for(i=0; i<copy->nData; i++)  {copy->dataF[i] = vIn->dataI[i];}}
     else if(vIn->type == FR_VECT_8S)
      {for(i=0; i<copy->nData; i++)  {copy->dataF[i] = vIn->dataL[i];}}
     else if(vIn->type == FR_VECT_2U)
      {for(i=0; i<copy->nData; i++)  {copy->dataF[i] = vIn->dataUS[i];}}
     else if(vIn->type == FR_VECT_4U)
      {for(i=0; i<copy->nData; i++)  {copy->dataF[i] = vIn->dataUI[i];}}
     else if(vIn->type == FR_VECT_8U)
      {for(i=0; i<copy->nData; i++)  {copy->dataF[i] = vIn->dataUL[i];}}
     else if(vIn->type == FR_VECT_1U)
      {for(i=0; i<copy->nData; i++)  {copy->dataF[i] = vIn->dataU[i];}}
     else {return(NULL);}}
   else
     {if(vIn->type == FR_VECT_4R)
      {for(i=0; i<copy->nData; i++)  {copy->dataF[i] = scale*vIn->dataF[i];}}
     else if(vIn->type == FR_VECT_C)
      {for(i=0; i<copy->nData; i++)  {copy->dataF[i] = scale*vIn->data[i];}}
     else if(vIn->type == FR_VECT_2S)
      {for(i=0; i<copy->nData; i++)  {copy->dataF[i] = scale*vIn->dataS[i];}}
     else if(vIn->type == FR_VECT_8R)
      {for(i=0; i<copy->nData; i++)  {copy->dataF[i] = scale*vIn->dataD[i];}}
     else if(vIn->type == FR_VECT_4S)
      {for(i=0; i<copy->nData; i++)  {copy->dataF[i] = scale*vIn->dataI[i];}}
     else if(vIn->type == FR_VECT_8S)
      {for(i=0; i<copy->nData; i++)  {copy->dataF[i] = scale*vIn->dataL[i];}}
     else if(vIn->type == FR_VECT_2U)
      {for(i=0; i<copy->nData; i++)  {copy->dataF[i] = scale*vIn->dataUS[i];}}
     else if(vIn->type == FR_VECT_4U)
      {for(i=0; i<copy->nData; i++)  {copy->dataF[i] = scale*vIn->dataUI[i];}}
     else if(vIn->type == FR_VECT_8U)
      {for(i=0; i<copy->nData; i++)  {copy->dataF[i] = scale*vIn->dataUL[i];}}
     else if(vIn->type == FR_VECT_1U)
      {for(i=0; i<copy->nData; i++)  {copy->dataF[i] = scale*vIn->dataU[i];}}
     else {return(NULL);}}}

                   /*----------------------- copy to I ----------------------*/

 else if(copy->type == FR_VECT_4S)  
  {if(scale == 1.)
    {if(vIn->type == FR_VECT_4S)
      {memcpy(copy->data, vIn->data, copy->nData *sizeof(int));}
     else if(vIn->type == FR_VECT_C)
      {for(i=0; i<copy->nData; i++)  {copy->dataI[i] = vIn->data[i];}}
     else if(vIn->type == FR_VECT_2S)
      {for(i=0; i<copy->nData; i++)  {copy->dataI[i] = vIn->dataS[i];}}
     else if(vIn->type == FR_VECT_8R)
      {for(i=0; i<copy->nData; i++)  {copy->dataI[i] = floor(.5+vIn->dataD[i]);}}
     else if(vIn->type == FR_VECT_4R)
      {for(i=0; i<copy->nData; i++)  {copy->dataI[i] = floor(.5+vIn->dataF[i]);}}
     else if(vIn->type == FR_VECT_8S)
      {for(i=0; i<copy->nData; i++)  {copy->dataI[i] = vIn->dataL[i];}}
     else if(vIn->type == FR_VECT_2U)
      {for(i=0; i<copy->nData; i++)  {copy->dataI[i] = vIn->dataUS[i];}}
     else if(vIn->type == FR_VECT_4U)
      {for(i=0; i<copy->nData; i++)  {copy->dataI[i] = vIn->dataUI[i];}}
     else if(vIn->type == FR_VECT_8U)
      {for(i=0; i<copy->nData; i++)  {copy->dataI[i] = vIn->dataUL[i];}}
     else if(vIn->type == FR_VECT_1U)
      {for(i=0; i<copy->nData; i++)  {copy->dataI[i] = vIn->dataU[i];}}
     else {return(NULL);}}
   else
    {if(vIn->type == FR_VECT_4S)
      {for(i=0; i<copy->nData; i++)  {copy->dataI[i] = scale*vIn->dataI[i];}}
     else if(vIn->type == FR_VECT_C)
      {for(i=0; i<copy->nData; i++)  {copy->dataI[i] = scale*vIn->data[i];}}
     else if(vIn->type == FR_VECT_2S)
      {for(i=0; i<copy->nData; i++)  {copy->dataI[i] = scale*vIn->dataS[i];}}
     else if(vIn->type == FR_VECT_8R)
      {for(i=0; i<copy->nData; i++)  {copy->dataI[i] = floor(.5+scale*vIn->dataD[i]);}}
     else if(vIn->type == FR_VECT_4R)
      {for(i=0; i<copy->nData; i++)  {copy->dataI[i] = floor(.5+scale*vIn->dataF[i]);}}
     else if(vIn->type == FR_VECT_8S)
      {for(i=0; i<copy->nData; i++)  {copy->dataI[i] = scale*vIn->dataL[i];}}
     else if(vIn->type == FR_VECT_2U)
      {for(i=0; i<copy->nData; i++)  {copy->dataI[i] = scale*vIn->dataUS[i];}}
     else if(vIn->type == FR_VECT_4U)
      {for(i=0; i<copy->nData; i++)  {copy->dataI[i] = scale*vIn->dataUI[i];}}
     else if(vIn->type == FR_VECT_8U)
      {for(i=0; i<copy->nData; i++)  {copy->dataI[i] = scale*vIn->dataUL[i];}}
     else if(vIn->type == FR_VECT_1U)
      {for(i=0; i<copy->nData; i++)  {copy->dataI[i] = scale*vIn->dataU[i];}}
     else {return(NULL);}}}

                   /*----------------------- copy to S ----------------------*/

 else if(copy->type == FR_VECT_2S)  
  {if(scale == 1.)
    {if(vIn->type == FR_VECT_2S)
      {memcpy(copy->dataS, vIn->dataS, copy->nData*sizeof(short));}
     else if(vIn->type == FR_VECT_C)
      {for(i=0; i<copy->nData; i++)  {copy->dataS[i] = vIn->data[i];}}
     else if(vIn->type == FR_VECT_8R)
      {for(i=0; i<copy->nData; i++)  {copy->dataS[i] = floor(.5+vIn->dataD[i]);}}
     else if(vIn->type == FR_VECT_4R)
      {for(i=0; i<copy->nData; i++)  {copy->dataS[i] = floor(.5+vIn->dataF[i]);}}
     else if(vIn->type == FR_VECT_4S)
      {for(i=0; i<copy->nData; i++)  {copy->dataS[i] = vIn->dataI[i];}}
     else if(vIn->type == FR_VECT_8S)
      {for(i=0; i<copy->nData; i++)  {copy->dataS[i] = vIn->dataL[i];}}
     else if(vIn->type == FR_VECT_2U)
      {for(i=0; i<copy->nData; i++)  {copy->dataS[i] = vIn->dataUS[i];}}
     else if(vIn->type == FR_VECT_4U)
      {for(i=0; i<copy->nData; i++)  {copy->dataS[i] = vIn->dataUI[i];}}
     else if(vIn->type == FR_VECT_8U)
      {for(i=0; i<copy->nData; i++)  {copy->dataS[i] = vIn->dataUL[i];}}
     else if(vIn->type == FR_VECT_1U)
      {for(i=0; i<copy->nData; i++)  {copy->dataS[i] = vIn->dataU[i];}}
     else {return(NULL);}}
   else
    {if(vIn->type == FR_VECT_2S)
      {for(i=0; i<copy->nData; i++)  {copy->dataS[i] = scale*vIn->dataS[i];}}
     else if(vIn->type == FR_VECT_C)
      {for(i=0; i<copy->nData; i++)  {copy->dataS[i] = scale*vIn->data[i];}}
     else if(vIn->type == FR_VECT_8R)
      {for(i=0; i<copy->nData; i++)  {copy->dataS[i] = floor(.5+scale*vIn->dataD[i]);}}
     else if(vIn->type == FR_VECT_4R)
      {for(i=0; i<copy->nData; i++)  {copy->dataS[i] = floor(.5+scale*vIn->dataF[i]);}}
     else if(vIn->type == FR_VECT_4S)
      {for(i=0; i<copy->nData; i++)  {copy->dataS[i] = scale*vIn->dataI[i];}}
     else if(vIn->type == FR_VECT_8S)
      {for(i=0; i<copy->nData; i++)  {copy->dataS[i] = scale*vIn->dataL[i];}}
     else if(vIn->type == FR_VECT_2U)
      {for(i=0; i<copy->nData; i++)  {copy->dataS[i] = scale*vIn->dataUS[i];}}
     else if(vIn->type == FR_VECT_4U)
      {for(i=0; i<copy->nData; i++)  {copy->dataS[i] = scale*vIn->dataUI[i];}}
     else if(vIn->type == FR_VECT_8U)
      {for(i=0; i<copy->nData; i++)  {copy->dataS[i] = scale*vIn->dataUL[i];}}
     else if(vIn->type == FR_VECT_1U)
      {for(i=0; i<copy->nData; i++)  {copy->dataS[i] = scale*vIn->dataU[i];}}
     else {return(NULL);}}}

                   /*----------------------- copy to 8H ---------------------*/

 else if(copy->type == FR_VECT_8H)  
  {if(vIn->type == copy->type)
    {if(scale == 1.)
      {memcpy(copy->data, vIn->data, copy->nData*sizeof(float));}
     else 
      {for(i=0; i<copy->nData; i++)  {copy->dataF[i] = scale*vIn->dataF[i];}}}
   else if(vIn->type == FR_VECT_16H)
      {for(i=0; i<copy->nData; i++)  {copy->dataF[i] = scale*vIn->dataD[i];}}
   else {return(NULL);}}

                   /*----------------------- copy to 16H --------------------*/

 else if(copy->type == FR_VECT_16H)  
  {if(vIn->type == copy->type)
    {if(scale == 1.)
      {memcpy(copy->data, vIn->data, copy->nData*sizeof(double));}
     else 
      {for(i=0; i<copy->nData; i++)  {copy->dataD[i] = scale*vIn->dataD[i];}}}
   else if(vIn->type == FR_VECT_8H)
      {for(i=0; i<copy->nData; i++)  {copy->dataD[i] = scale*vIn->dataF[i];}}
   else {return(NULL);}}

                   /*----------------------- copy to 8C ---------------------*/

 else if(copy->type == FR_VECT_8C)  
  {if(vIn->type == copy->type)
    {if(scale == 1.)
      {memcpy(copy->data, vIn->data, 2*copy->nData*sizeof(float));}
     else 
      {for(i=0; i<2*copy->nData; i++)  {copy->dataF[i] = scale*vIn->dataF[i];}}}
   else if(vIn->type == FR_VECT_16C)
      {for(i=0; i<2*copy->nData; i++)  {copy->dataF[i] = scale*vIn->dataD[i];}}
   else {return(NULL);}}

                   /*----------------------- copy to 16C --------------------*/

 else if(copy->type == FR_VECT_16C)  
  {if(vIn->type == copy->type)
    {if(scale == 1.)
      {memcpy(copy->data, vIn->data, 2*copy->nData*sizeof(double));}
     else 
      {for(i=0; i<2*copy->nData; i++)  {copy->dataD[i] = scale*vIn->dataD[i];}}}
   else if(vIn->type == FR_VECT_8C)
      {for(i=0; i<2*copy->nData; i++)  {copy->dataD[i] = scale*vIn->dataF[i];}}
   else {return(NULL);}}

                     /*------------------ unsuported type -------------------*/

 else
   {return(NULL);}

 return(copy);
}
/*------------------------------------------------------------FrVectCopyToD--*/
FrVect *FrVectCopyToD(FrVect *vIn,
                      double scale,
                      char *newName)
/*---------------------------------------------------------------------------*/
/* This function creates a new vector of type double. The data are copy using*/
/* the scale factor 'scale' and casted to the proper type. The new vector    */
/* has the same name as the original one except if a newName is provided     */
/*---------------------------------------------------------------------------*/
{FrVect *vect;

 if(vIn == NULL) return(NULL);

 if(newName == NULL) newName = vIn->name;

 vect = FrVectNew1D(newName, -FR_VECT_8R, vIn->nData, vIn->dx[0], 
                    vIn->unitX[0], vIn->unitY);
 vect = FrVectCopyTo(vIn, scale, vect);

 return(vect);
}
/*------------------------------------------------------------FrVectCopyToF--*/
FrVect *FrVectCopyToF(FrVect *vIn,
                      double scale,
                      char *newName)
/*---------------------------------------------------------------------------*/
/* This function creates a new vector of type float. The data are copy using */
/* the scale factor 'scale' and casted to the proper type. The new vector    */
/* has the same name as the original one except if a newName is provided     */
/*---------------------------------------------------------------------------*/
{FrVect *vect;

 if(vIn == NULL) return(NULL);

 if(newName == NULL) newName = vIn->name;

 vect = FrVectNew1D(newName, -FR_VECT_4R, vIn->nData, vIn->dx[0], 
                    vIn->unitX[0], vIn->unitY);
 vect = FrVectCopyTo(vIn, scale, vect);

 return(vect);
}
/*------------------------------------------------------------FrVectCopyToI--*/
FrVect *FrVectCopyToI(FrVect *vIn,
                      double scale,
                      char *newName)
/*---------------------------------------------------------------------------*/
/* This function creates a new vector of type int. The data are copy using   */
/* the scale factor 'scale' and casted to the proper type. The new vector    */
/* has the same name as the original one except if a newName is provided     */
/*---------------------------------------------------------------------------*/
{FrVect *vect;

 if(vIn == NULL) return(NULL);

 if(newName == NULL) newName = vIn->name;

 vect = FrVectNew1D(newName, -FR_VECT_4S, vIn->nData, vIn->dx[0], 
                    vIn->unitX[0], vIn->unitY);
 vect = FrVectCopyTo(vIn, scale, vect);

 return(vect);
}
/*-----------------------------------------------------------FrVectCopyToS--*/
FrVect *FrVectCopyToS(FrVect *vIn,
                      double scale,
                      char *newName)
/*---------------------------------------------------------------------------*/
/* This function creates a new vector of type short. The data are copy using */
/* the scale factor 'scale' and casted to the proper type. The new vector    */
/* has the same name as the original one except if a newName is provided     */
/*---------------------------------------------------------------------------*/
{FrVect *vect;

 if(vIn == NULL) return(NULL);

 if(newName == NULL) newName = vIn->name;

 vect = FrVectNew1D(newName, -FR_VECT_2S, vIn->nData, vIn->dx[0], 
                    vIn->unitX[0], vIn->unitY);
 vect = FrVectCopyTo(vIn, scale, vect);

 return(vect);
}


/*---------------------------------------------------------- FrVectDecimate--*/
FrVect *FrVectDecimate(FrVect *vect,
                       int nGroup,
                       FrVect *outVect)
/*---------------------------------------------------------------------------*/
/* This function decimates the data from the vector vect by averaging nGroup */
/* values togehter. The result is put in the vector outVect. The size of     */
/* outVect should be nGroup time smaller than the size of vect.              */
/*---------------------------------------------------------------------------*/
{int i, j, off, nData;
 double sum, coef;
 FRBOOL average;

 if(vect == NULL) return(NULL);
 if(nGroup == 0)  return(NULL);
 if(nGroup < 0)
     {nGroup = -nGroup;
      average = FR_NO;}
 else{average = FR_YES;}

 if(vect->nData % nGroup != 0) return(NULL);

 if(outVect == NULL) outVect = vect;

           /*-- check that there is enough output space for the output data--*/
   
 if(outVect->nData < vect->nData/nGroup) return(NULL);

           /*------------ adjust parameter for the output vector ------------*/

 outVect->nData  = vect->nData/nGroup;
 outVect->space  = vect->space/nGroup;
 outVect->nx[0]  = vect->nx[0]/nGroup;
 outVect->nBytes = outVect->wSize * outVect->nData;
 outVect->dx[0]  = vect->dx[0]*nGroup;

           /*------------------ do simple decimation without averaging-------*/

 if(average == FR_NO)
  {if(outVect->wSize != vect->wSize) return(NULL);
   nData = outVect->nData;
   if(vect->wSize == 1)
     {for(i=0; i<nData; i++) {outVect->data[i] = vect->data[(i+1)*nGroup-1];}}
   else if(vect->wSize == 2)
     {for(i=0; i<nData; i++) {outVect->dataS[i]=vect->dataS[(i+1)*nGroup-1];}}
   else if(vect->wSize == 4)
     {for(i=0; i<nData; i++) {outVect->dataI[i]=vect->dataI[(i+1)*nGroup-1];}}
   else if(vect->wSize == 8)
     {for(i=0; i<nData; i++) {outVect->dataD[i]=vect->dataD[(i+1)*nGroup-1];}}}

           /*------------------ do decimation with averaging-----------------*/

 else
  {coef = 1./nGroup;
   off = 0;

   for(i=0; i < outVect->nData; i++)
    {sum = 0.;

     if     (vect->type == FR_VECT_2S) 
              for(j=off; j<off+nGroup; j++) {sum += vect->dataS[j];}
     else if(vect->type == FR_VECT_4S)
              for(j=off; j<off+nGroup; j++) {sum += vect->dataI[j];}
     else if(vect->type == FR_VECT_4R)
              for(j=off; j<off+nGroup; j++) {sum += vect->dataF[j];}
     else if(vect->type == FR_VECT_8R)
              for(j=off; j<off+nGroup; j++) {sum += vect->dataD[j];}   
     else if(vect->type == FR_VECT_C)
              for(j=off; j<off+nGroup; j++) {sum += vect->data[j];}
     else if(vect->type == FR_VECT_8S)
              for(j=off; j<off+nGroup; j++) {sum += vect->dataL[j];}
     else if(vect->type == FR_VECT_2U)
              for(j=off; j<off+nGroup; j++) {sum += vect->dataUS[j];}
     else if(vect->type == FR_VECT_4U)
              for(j=off; j<off+nGroup; j++) {sum += vect->dataUI[j];}
     else if(vect->type == FR_VECT_8U)
              for(j=off; j<off+nGroup; j++) {sum += vect->dataUL[j];}
       
     if     (outVect->type == FR_VECT_4R) outVect->dataF[i] = sum*coef;
     else if(outVect->type == FR_VECT_8R) outVect->dataD[i] = sum*coef;
     else if(outVect->type == FR_VECT_2S) outVect->dataS[i] = sum*coef;
     else if(outVect->type == FR_VECT_4S) outVect->dataI[i] = sum*coef;
     else if(outVect->type == FR_VECT_8S) outVect->dataL[i] = sum*coef;
     else if(outVect->type == FR_VECT_C)  outVect->data[i]  = sum*coef;
     else if(outVect->type == FR_VECT_2U) outVect->dataUS[i]= sum*coef;
     else if(outVect->type == FR_VECT_4U) outVect->dataUI[i]= sum*coef;
     else if(outVect->type == FR_VECT_8U) outVect->dataUL[i]= sum*coef;

     off += nGroup;}}
                        /*------------- free the unused space ---------------*/

 if(outVect == vect && vect->dataUnzoomed == NULL) 
    {vect->data = realloc(vect->data, vect->nBytes);
     FrVectMap(vect);}

 return(outVect);}

/*--------------------------------------------------------FrVectDecimateMax--*/
void FrVectDecimateMax(FrVect *vect,
                      int nGroup)
/*---------------------------------------------------------------------------*/
{int    *dataI, maxI, i, j;
 float  *dataF, maxF;
 double *dataD, maxD;
 short  *dataS, maxS;

 if(vect == NULL) return;
 if(nGroup <= 0)  return;

 vect->nData = vect->nData / nGroup;
 vect->nx[0] = vect->nx[0] / nGroup;
 vect->dx[0] = vect->dx[0] * nGroup;
 vect->nBytes= vect->nBytes /nGroup;

 if(vect->type == FR_VECT_2S)
   {dataS = vect->dataS;
    for(i=0; i < vect->nData; i++)
       {maxS = dataS[0];
        for(j=0; j<nGroup; j++) {if(dataS[j] > maxS) maxS = dataS[j];}
        vect->dataS[i] = maxS;
        dataS = dataS + nGroup;}}

 else if(vect->type == FR_VECT_4S)
   {dataI = vect->dataI;
    for(i=0; i < vect->nData; i++)
       {maxI = dataI[0];
        for(j=0; j<nGroup; j++) {if(dataI[j] > maxI) maxI = dataI[j];}
        vect->dataI[i] = maxI;
        dataI = dataI + nGroup;}}

 else if(vect->type == FR_VECT_4R)
   {dataF = vect->dataF;
    for(i=0; i < vect->nData; i++)
       {maxF = dataF[0];
        for(j=0; j<nGroup; j++) {if(dataF[j] > maxF) maxF = dataF[j];}
        vect->dataF[i] = maxF;
        dataF = dataF + nGroup;}}

 else if(vect->type == FR_VECT_8R)
   {dataD = vect->dataD;
    for(i=0; i < vect->nData; i++)
       {maxD = dataD[0];
        for(j=0; j<nGroup; j++) {if(dataD[j] > maxD) maxD = dataD[j];}
        vect->dataD[i] = maxD;
        dataD = dataD + nGroup;}}

 vect->data = realloc(vect->data, vect->nBytes);
 FrVectMap(vect);

 return;}

/*--------------------------------------------------------FrVectDecimateMin--*/
void FrVectDecimateMin(FrVect *vect,
                       int nGroup)
/*---------------------------------------------------------------------------*/
{int    *dataI, minI, i, j;
 float  *dataF, minF;
 double *dataD, minD;
 short  *dataS, minS;

 if(vect == NULL) return;
 if(nGroup <= 0)  return;

 vect->nData = vect->nData / nGroup;
 vect->nx[0] = vect->nx[0] / nGroup;
 vect->dx[0] = vect->dx[0] * nGroup;
 vect->nBytes= vect->nBytes /nGroup;

 if(vect->type == FR_VECT_2S)
   {dataS = vect->dataS;
    for(i=0; i < vect->nData; i++)
       {minS = dataS[0];
        for(j=0; j<nGroup; j++) {if(dataS[j] < minS) minS = dataS[j];}
        vect->dataS[i] = minS;
        dataS = dataS + nGroup;}}

 else if(vect->type == FR_VECT_4S)
   {dataI = vect->dataI;
    for(i=0; i < vect->nData; i++)
       {minI = dataI[0];
        for(j=0; j<nGroup; j++) {if(dataI[j] < minI) minI = dataI[j];}
        vect->dataI[i] = minI;
        dataI = dataI + nGroup;}}

 else if(vect->type == FR_VECT_4R)
   {dataF = vect->dataF;
    for(i=0; i < vect->nData; i++)
       {minF = dataF[0];
        for(j=0; j<nGroup; j++) {if(dataF[j] < minF) minF = dataF[j];}
        vect->dataF[i] = minF;
        dataF = dataF + nGroup;}}

 else if(vect->type == FR_VECT_8R)
   {dataD = vect->dataD;
    for(i=0; i < vect->nData; i++)
       {minD = dataD[0];
        for(j=0; j<nGroup; j++) {if(dataD[j] < minD) minD = dataD[j];}
        vect->dataD[i] = minD;
        dataD = dataD + nGroup;}}

 vect->data = realloc(vect->data, vect->nBytes);
 FrVectMap(vect);

 return;}

/*----------------------------------------------------------------FrVectDef--*/
FrSH *FrVectDef()
/*---------------------------------------------------------------------------*/
{static FrSH *classe = NULL;

  if(classe != NULL) return(classe);
  classe = FrDicAddS("FrVect",(void (*)())FrVectRead);

  FrSENew(classe, "STRING",       "name","-");
  FrSENew(classe, "INT_2U",       "compress","-");
  FrSENew(classe, "INT_2U",       "type","-");
  FrSENew(classe, "INT_8U",       "nData","-");
  FrSENew(classe, "INT_8U",       "nBytes","-");
  FrSENew(classe, "CHAR[nBytes]", "data","-");
  FrSENew(classe, "INT_4U",       "nDim","-");
  FrSENew(classe, "INT_8U[nDim]", "nx","-");
  FrSENew(classe, "REAL_8[nDim]", "dx","-");
  FrSENew(classe, "REAL_8[nDim]", "startX","-");
  FrSENew(classe, "STRING[nDim]", "unitX", NULL);
  FrSENew(classe, "STRING",       "unitY", NULL);
  FrSENew(classe, "PTR_STRUCT(FrVect *)", "next", NULL);
  if(FrFormatVersion >= 8) FrSENew(classe, "INT_4U", "chkSum","-");

  return(classe);}

/*---------------------------------------------------------------FrVectDiff--*/
char *FrVectDiff(FrVect *vect)
/*---------------------------------------------------------------------------*/
/* This function return a pointer to the array of differented values         */
/* The input vector is unchanged                                             */
{int i,err;
 static char  *data, last;
 static short *dataS, lastS;
 static int  *dataI, lastI, localBytes = 0;
 static unsigned char  *dataU,  lastU;
 static unsigned short *dataUS, lastUS;
 static unsigned int   *dataUI, lastUI;

 if(vect == NULL) return(NULL);
 if(vect->nBytes > localBytes)
    {if(localBytes != 0) free(data);
     localBytes = vect->nBytes;
     data = (char *) malloc(localBytes);
     if(data == NULL) 
        {localBytes = 0;
         return(NULL);}

     dataS  = (short          *)data;
     dataI  = (int            *)data;
     dataU  = (unsigned char  *)data;
     dataUS = (unsigned short *)data;
     dataUI = (unsigned int   *)data;}

            /*----- differentiate and check ----*/
 err = 0;
 if(vect->type == FR_VECT_C)
   {data[0] = vect->data[0];
    last    =       data[0];
    for(i=1; i<vect->nData; i++)
      {data[i] = vect->data[i] - vect->data[i-1];
       last   +=       data[i];
       if(last != vect->data[i]) err = 1;}}

 else if(vect->type == FR_VECT_2S)
   {dataS[0] = vect->dataS[0];
    lastS    =       dataS[0];
    for(i=1; i<vect->nData; i++)
      {dataS[i] = vect->dataS[i] - vect->dataS[i-1];
       lastS   +=       dataS[i];
       if(lastS != vect->dataS[i]) err = 1;}}

 else if(vect->type == FR_VECT_4S) 
   {dataI[0] = vect->dataI[0];
    lastI    =       dataI[0];
    for(i=1; i<vect->nData; i++)
      {dataI[i] = vect->dataI[i] - vect->dataI[i-1];
       lastI   +=       dataI[i];
       if(lastI != vect->dataI[i]) err = 1;}}

 else if(vect->type == FR_VECT_1U) 
   {dataU[0] = vect->dataU[0];
    lastU    =       dataU[0];
    for(i=1; i<vect->nData; i++)
      {dataU[i] = vect->dataU[i] - vect->dataU[i-1];
       lastU   +=       dataU[i];
       if(lastU != vect->dataU[i]) err = 1;}}

 else if(vect->type == FR_VECT_2U)
   {dataUS[0] = vect->dataUS[0];
    lastUS    =       dataUS[0];
    for(i=1; i<vect->nData; i++)
      {dataUS[i] = vect->dataUS[i] - vect->dataUS[i-1];
       lastUS   +=       dataUS[i];
       if(lastUS != vect->dataUS[i]) err = 1;}}

 else if(vect->type == FR_VECT_4U) 
   {dataUI[0] = vect->dataUI[0];
    lastUI    =       dataUI[0];
    for(i=1; i<vect->nData; i++)
      {dataUI[i] = vect->dataUI[i] - vect->dataUI[i-1];
       lastUI   +=       dataUI[i];
       if(lastUI != vect->dataUI[i]) err = 1;}}

 else  {err = 2;}
                                  /*------ set the compress flag if ok ------*/
 if(err == 0)  {return (data);}

return(NULL);}

/*---------------------------------------------------------------FrVectDump--*/
void FrVectDump(FrVect *vect,
                FILE *fp,
                int debugLvl)
/*---------------------------------------------------------------------------*/
{FRULONG i, nData, inValid;
 char *dC, **dSt;
 short  *dS;
 int    *dI;  
 FRLONG *dL; 
 float  *dF, ratio;  
 double *dD;
 unsigned char   *dU;
 unsigned short  *dUS;
 unsigned int    *dUI;
 FRULONG *dUL;

 if(fp   == NULL) return;
 if(vect == NULL) return;
 if(debugLvl < 1) return;

 nData = vect->nData;
 if(vect->name == NULL)
      {fprintf(fp,"  Vector:- ndata=%"FRLLD,              nData);}
 else {fprintf(fp,"  Vector:%s ndata=%"FRLLD, vect->name, nData);}

 if(vect->GTime != 0) fprintf(fp,"  GTime=%.5f",vect->GTime);

 if(vect->unitY != NULL) 
         {fprintf(fp," unitY=%s", vect->unitY);}
 
 if(vect->nDim == 1)
   {if(vect->unitX[0] != NULL) fprintf(fp," unitX=%s", vect->unitX[0]);
    fprintf(fp,"  startX=%g dx=%g\n", vect->startX[0], vect->dx[0]);}
 else
   {fprintf(fp," nDim=%d\n", vect->nDim);
    for(i=0; i<vect->nDim; i++)
      {fprintf(fp,"      Dimension=%"FRLLD" nx=%10"FRLLD" startX=%.2g dx=%g",
	       i,vect->nx[i], vect->startX[i], vect->dx[i]);
       if(vect->unitX[i] != NULL) fprintf(fp," unit=%s\n", vect->unitX[i]);
       else fprintf(fp,"\n");}}

           /*-------------------------------------- data part ---------------*/

 fprintf(fp,"   Data");
 if(vect->compress == 0)
   {if     (vect->type == FR_VECT_4R)  /*----------------- float-------------*/
      {dF = (float *) vect->data;
       dI = (int *) vect->data;
       fprintf(fp,"(float) %s\n",FrVectStat(vect));
       if(debugLvl > 2)
         {inValid = FrVectIsValid(vect); 
          for(i=0; i<nData; i++)
            {if(i%10 == 0) 
                {if(i > inValid && inValid != 0) fprintf(fp,
		     "\n   HEX:%12x%12x%12x%12x%12x%12x%12x%12x%12x%12x",
		       dI[i-10],dI[i-9],dI[i-8],dI[i-7],dI[i-6],
		       dI[i- 5],dI[i-4],dI[i-3],dI[i-2],dI[i-1]);
 		 fprintf(fp,"\n%6"FRLLD":",i);}
             fprintf(fp," %11.5g",dF[i]);}
          fprintf(fp,"\n");}}

    else if(vect->type == FR_VECT_8R)  /*---------------- double ------------*/
      {dD = (double *) vect->data;
       fprintf(fp,"(double) %s\n",FrVectStat(vect));
       if(debugLvl >2)
         {for(i=0; i<nData; i++)
            {if(i%10 == 0) fprintf(fp,"\n%6"FRLLD":",i);
             fprintf(fp," %11.5g",dD[i]);}
          fprintf(fp,"\n");}}

    else if(vect->type == FR_VECT_C)   /*---------------one byte integer-----*/
      {dC = vect->data;
       fprintf(fp,"(byte) %s\n",FrVectStat(vect));
       if(debugLvl > 2)
         {for(i=0; i<nData; i++)
            {if(i%20 == 0) fprintf(fp,"\n%6"FRLLD":",i);
             fprintf(fp,"%4d",dC[i]);}
          fprintf(fp,"\n");}}

    else if(vect->type == FR_VECT_2S)  /*-------------- short integer -------*/
      {dS = (short *) vect->data;
       fprintf(fp,"(short) %s\n",FrVectStat(vect));
       if(debugLvl > 2)
         {for(i=0; i<nData; i++)
            {if(i%10 == 0) fprintf(fp,"\n%6"FRLLD":",i);
             fprintf(fp," %6d",dS[i]);}
          fprintf(fp,"\n");}}

    else if(vect->type == FR_VECT_8S)  /*---------------long integer---------*/
      {dL = (FRLONG *) vect->data;
       fprintf(fp,"(8S) %s\n",FrVectStat(vect));
       if(debugLvl > 2)
         {for(i=0; i<nData; i++)
            {if(i%10 == 0) fprintf(fp,"\n%6"FRLLD":",i);
             fprintf(fp," %9"FRLLD"",dL[i]);}
          fprintf(fp,"\n");}}

    else if(vect->type == FR_VECT_4S)  /*------------- signed integer -------*/
      {dI = (int *) vect->data;
       fprintf(fp,"(int) %s\n",FrVectStat(vect));
       if(debugLvl > 2)
         {for(i=0; i<nData; i++)
            {if(i%10 == 0) fprintf(fp,"\n%6"FRLLD":",i);
             fprintf(fp," %8d",dI[i]);}
          fprintf(fp,"\n");}}

    else if(vect->type == FR_VECT_1U)    /*--------- unsigned  character----*/
      {dU = (unsigned char *)vect->dataU;
       fprintf(fp,"(1U) %s\n",FrVectStat(vect));
       if(debugLvl > 2)
         {for(i=0; i<nData; i++)
            {if(i%20 == 0) fprintf(fp,"\n%6"FRLLD":",i);
             fprintf(fp," %4d",dU[i]);}
          fprintf(fp,"\n");}}

    else if(vect->type == FR_VECT_2U)    /*-------------unsigned short -----*/
      {dUS = (unsigned short *) vect->data;
       fprintf(fp,"(2U) %s\n",FrVectStat(vect));
       if(debugLvl > 2)
         {for(i=0; i<nData; i++)
            {if(i%10 == 0) fprintf(fp,"\n%6"FRLLD":",i);
             fprintf(fp," %6d",dUS[i]);}
          fprintf(fp,"\n");}}

    else if(vect->type == FR_VECT_8U)  
      {dUL = (FRULONG *) vect->data;
       fprintf(fp,"(8U) %s\n",FrVectStat(vect));
       if(debugLvl > 2)
         {for(i=0; i<nData; i++)
            {if(i%10 == 0) fprintf(fp,"\n%6"FRLLD":",i);
             fprintf(fp," %"FRLLD"",dUL[i]);}
          fprintf(fp,"\n");}}

    else if(vect->type == FR_VECT_4U)  
      {dUI = (unsigned int *) vect->data;
       fprintf(fp,"(4U) %s\n",FrVectStat(vect));
       if(debugLvl > 2)
         {for(i=0; i<nData; i++)
            {if(i%10 == 0) fprintf(fp,"\n%6"FRLLD":",i);
             fprintf(fp," %12d",dUI[i]);}
          fprintf(fp,"\n");}}

    else if(vect->type == FR_VECT_8C)    /*------------- complex float ------*/
      {dF = (float *) vect->data;
       fprintf(fp,"(8C) %s\n", FrVectStat(vect));
       if(debugLvl <3)
         {fprintf(fp,"   (%g,%g)", dF[0],dF[1]);
          if(nData > 1) fprintf(fp," (%g,%g) ...", dF[2],dF[3]);
          fprintf(fp,"\n");}
       else
         {for(i=0; i<nData; i++)
            {if(i%4 == 0) fprintf(fp,"\n%6"FRLLD":",i);
             fprintf(fp,"(%12g %12g)",dF[2*i],dF[2*i+1]);}
          fprintf(fp,"\n");}}

    else if(vect->type == FR_VECT_16C)  /*------------- complex double ------*/
      {dD = (double *) vect->data;
       fprintf(fp,"(16C) %s\n", FrVectStat(vect));
       if(debugLvl <3)
         {fprintf(fp,"   (%g,%g)",dD[0],dD[1]);
          if(nData > 1) fprintf(fp," (%g,%g) ...",dD[2],dD[3]);
          fprintf(fp,"\n");}
       else
         {for(i=0; i<nData; i++)
            {if(i%4 == 0) fprintf(fp,"\n%6"FRLLD":",i);
             fprintf(fp,"(%12g %12g)",dD[2*i],dD[2*i+1]);}
          fprintf(fp,"\n");}}

    else if(vect->type == FR_VECT_8H)     /*-------half complex float -------*/
      {dF = (float *) vect->data;
       fprintf(fp,"(8H) %s\n", FrVectStat(vect));
       if(debugLvl <3)
         {fprintf(fp,"   %g ", dF[0]);
          if(nData > 1) fprintf(fp,", (%g,%g) ...", dF[1],dF[nData-1]);
          fprintf(fp,"\n");}
       else
         {fprintf(fp,"%6d:(%12g %12g)", 0, dF[0], 0.);
          for(i=1; i<nData/2; i++)
            {if(i%4 == 0) fprintf(fp,"\n%6"FRLLD":",i);
             fprintf(fp,"(%12g %12g)",dF[i],dF[nData-i]);}
          if(nData%2 == 0) fprintf(fp,"(%12g 0.)",dF[nData/2]);
          fprintf(fp,"\n");}}

    else if(vect->type == FR_VECT_16H)      /*----half complex double -------*/
      {dD = (double *) vect->data;
       fprintf(fp,"(16H) %s\n", FrVectStat(vect));
       if(debugLvl <3)
         {fprintf(fp,"   %g",dD[0]);
          if(nData > 1) fprintf(fp,", (%g,%g) ...\n",dD[1],dD[nData-1]);
          fprintf(fp,"\n");}
       else
         {fprintf(fp,"%6d:(%12g %12g)", 0, dD[0], 0.);
          for(i=1; i<nData/2; i++)
            {if(i%4 == 0) fprintf(fp,"\n%6"FRLLD":",i);
             fprintf(fp,"(%12g %12g)",dD[i],dD[nData-i]);}
          if(nData%2 == 0) fprintf(fp,"(%12g 0.)",dD[nData/2]);
          fprintf(fp,"\n");}}

    else if(vect->type == FR_VECT_STRING) 
      {dSt = (char **) vect->data;
       fprintf(fp,"(STRING)");
       if(dSt[0] != NULL) fprintf(fp," \"%s\"", dSt[0]);
       if(dSt[1] != NULL) fprintf(fp," \"%s\"", dSt[1]);
       fprintf(fp,"...\n");}
    else 
      {fprintf(fp," unknown type: %d \n",vect->type );}}
  else 
    {ratio = (nData*vect->wSize)/(float) vect->nBytes;
     fprintf(fp,"\n     the vector is %.2f compressed (%x)"
                " nBytes=%"FRLLD" wSize=%d\n",
            ratio,vect->compress,vect->nBytes, vect->wSize);}

 if(vect->next != NULL)
   {fprintf(fp,"  Attached information:\n");
    FrVectDump(vect->next, fp, debugLvl) ;}

 return;}

/*-------------------------------------------------------------FrVectExpand--*/
void FrVectExpand(FrVect *vect)
/*---------------------------------------------------------------------------*/
{int err;
 unsigned int i, compress, oldType;
 unsigned char *buf, local0, local1, local2, local3, *uncompr;
 unsigned long uncomprLenL;
 FRULONG uncomprLen;
 char *refC;
 short ref, swap;
 FRULONG nData;

 if(vect == NULL) return;
 if(vect->compress == 0) return;
                                     /*---- do we have to swap the bytes ?---*/
 refC = (char *)&ref;
 ref = 0x1234;

 swap = (refC[0] == 0x34 && vect->compress < 255) ||
        (refC[0] == 0x12 && vect->compress > 255);
 compress = vect->compress & 0xff;
 if(compress == 6) compress = 5;

                          /*----------------------- first expand the data ---*/

 uncomprLen = vect->nData * vect->wSize;
 uncompr    = (unsigned char*)calloc(uncomprLen, 1);
 if(uncompr == NULL) 
    {FrError(3,"FrVectExpand","malloc failed");
     return;}

 if(compress == 1 || compress == 3)
    {uncomprLenL = uncomprLen;
     err = Frz_uncompress(uncompr, &uncomprLenL, vect->data, vect->nBytes);
     if(err != 0) 
          {FrError(3,"FrVectExpand","uncompress failed");
	   return;}}

 else if(compress == 5)
      {if(swap)
          {buf = vect->dataU;
           for(i=0; i<vect->nBytes-1; i=i+2)
              {local0   = buf[i];
               buf[i]   = buf[i+1];
               buf[i+1] = local0;}}
       FrVectZExpand((short *) uncompr,
                     (unsigned short *)vect->data, vect->nData);}

 else if(compress == 7)
      {if(swap)
          {buf = vect->dataU;
	   local0 = buf[3];
           buf[3] = buf[0];
	   buf[0] = local0;
           local0 = buf[2];
           buf[2] = buf[1];
	   buf[1] = local0;
           local0 = buf[7];
           buf[7] = buf[4];
	   buf[4] = local0;
           local0 = buf[6];
           buf[6] = buf[5];
	   buf[5] = local0;
           for(i=8; i<vect->nBytes-1; i=i+2)
              {local0   = buf[i];
               buf[i]   = buf[i+1];
               buf[i+1] = local0;}}
      
       FrVectFExpand((float *) uncompr, (short *)vect->data, vect->nData);}

 else if(compress == 8)
      {if(swap)
          {buf = vect->dataU;
           for(i=0; i<vect->nBytes-3; i=i+4)
              {local0   = buf[i+3];
               local1   = buf[i+2];
               buf[i+3] = buf[i];
               buf[i+2] = buf[i+1];
               buf[i+1] = local1;
               buf[i]   = local0;}}
       FrVectZExpandI((int *)uncompr,(unsigned int *)vect->data,vect->nData);}

 else if(compress == 10)
      {if(swap)
          {buf = vect->dataU;
           for(i=0; i<vect->nBytes-7; i=i+4)
           {local0 = buf[i];
            local1 = buf[i+1];
            local2 = buf[i+2];
            local3 = buf[i+3];
            buf[i]   = buf[i+7];
            buf[i+1] = buf[i+6];
            buf[i+2] = buf[i+5];
            buf[i+3] = buf[i+4];
            buf[i+4] = local3;
            buf[i+5] = local2;
            buf[i+6] = local1;
            buf[i+7] = local0;}}
       FrVectZExpandL((long long *)uncompr,(unsigned long long *)vect->data,vect->nData);}

 else if(compress == 255)
      {FrVectUExpand(vect, uncompr);}

 else
    {FrError(3,"FrVectExpand","unknown compression level");
     return;}

 free(vect->data);
 vect->data = (char *) uncompr;
 FrVectMap(vect);

 vect->nBytes =  vect->nData * vect->wSize;

                       /*---------------------- swap bytes if needed --------*/
       
 if(swap && (compress == 1 || compress == 3))
   {nData = vect->nData;
    if(vect->type == FR_VECT_8C)  nData = 2*nData;
    if(vect->type == FR_VECT_16C) nData = 2*nData;
    buf = vect->dataU;

    if((vect->type == FR_VECT_2S) ||
       (vect->type == FR_VECT_2U))
          {for(i=0; i<2*nData; i=i+2)
              {local0   = buf[i];
               buf[i]   = buf[i+1];
               buf[i+1] = local0;}}

    else if((vect->type == FR_VECT_4S) ||
         (vect->type == FR_VECT_4U) ||
         (vect->type == FR_VECT_4R) ||
         (vect->type == FR_VECT_8C))
       {for(i=0; i<4*nData; i=i+4)
           {local0 = buf[i];
            local1 = buf[i+1];
            buf[i]   = buf[i+3];
            buf[i+1] = buf[i+2];
            buf[i+2] = local1;
            buf[i+3] = local0;}} 

    else if((vect->type == FR_VECT_8S) ||
         (vect->type == FR_VECT_8U) ||
         (vect->type == FR_VECT_8R) ||
         (vect->type == FR_VECT_16C))
       {for(i=0; i<8*nData; i=i+8)
           {local0 = buf[i];
            local1 = buf[i+1];
            local2 = buf[i+2];
            local3 = buf[i+3];
            buf[i]   = buf[i+7];
            buf[i+1] = buf[i+6];
            buf[i+2] = buf[i+5];
            buf[i+3] = buf[i+4];
            buf[i+4] = local3;
            buf[i+5] = local2;
            buf[i+6] = local1;
            buf[i+7] = local0;}}}

                             /*------------integrate the data if needed -----*/
 oldType = vect->type;
 if(compress == 8)  vect->type = FR_VECT_4S;
 if(compress == 10) vect->type = FR_VECT_8S;
 if(compress > 1 && compress < 255) FrVectInt(vect);
 vect->type = oldType;

 vect->compress = 0;

 return;}

/*------------------------------------------------------------FrVectExpandF--*/
void FrVectExpandF(FrVect *vect)
/*---------------------------------------------------------------------------*/
{
  if(vect == NULL) return;
  FrVectExpand(vect);

  if(vect->next != NULL) FrVectExpandF(vect->next);

  return;}

/*---------------------------------------------------------- FrVectExtand--*/
FrVect *FrVectExtend(FrVect *vect,
                     int nTimes,
                     FrVect *outVect,
                     char *newName)
/*---------------------------------------------------------------------------*/
/* This function extend the data from the vector vect by duplicate nTimes    */
/* each values. The result is put in the vector outVect. The size of         */
/* outVect should be nTime larger than the size of vect.                     */
/* If outVect is NULL, the output vector is created and named "newName" or as*/
/* the original vector is newName = NULL.                                    */
/*---------------------------------------------------------------------------*/
{int i, j, nData;

 if(vect == NULL) return(NULL);
 if(nTimes == 0)  return(NULL);

 if(outVect == NULL)
   {if(newName == NULL) newName = vect->name;
    outVect = FrVectNew1D(newName, -vect->type, vect->nData*nTimes,
                     vect->dx[0]/nTimes, vect->unitX[0], vect->unitY);
    if(outVect == NULL) return(NULL);
    outVect->startX[0] = vect->startX[0];
    outVect->GTime     = vect->GTime;
    outVect->localTime = vect->localTime;
    outVect->ULeapS    = vect->ULeapS;}


           /*-- check that there is enough output space for the output data--*/

 if(outVect->nData < vect->nData*nTimes) return(NULL);

           /*------------------------------------------- fill the data part--*/

 if(outVect->wSize != vect->wSize) return(NULL);
 nData = vect->nData;
 if(vect->wSize == 1)
     {for(i=0; i<nData; i++)
        for(j=0; j<nTimes; j++) {outVect->data[i*nTimes+j] = vect->data[i];}}
 else if(vect->wSize == 2)
     {for(i=0; i<nData; i++)
        for(j=0; j<nTimes; j++) {outVect->dataS[i*nTimes+j]= vect->dataS[i];}}
 else if(vect->wSize == 4)
     {for(i=0; i<nData; i++)
        for(j=0; j<nTimes; j++) {outVect->dataI[i*nTimes+j]=vect->dataI[i];}}
 else if(vect->type == FR_VECT_8C)
     {for(i=0; i<nData; i++)
        for(j=0; j<nTimes; j++) 
           {outVect->dataF[2*(i*nTimes+j)  ] = vect->dataF[2*i];
            outVect->dataF[2*(i*nTimes+j)+1] = vect->dataF[2*i+1];}}
 else if(vect->type == FR_VECT_16C)
     {for(i=0; i<nData; i++)
        for(j=0; j<nTimes; j++) 
           {outVect->dataD[2*(i*nTimes+j)  ] = vect->dataD[2*i];
            outVect->dataD[2*(i*nTimes+j)+1] = vect->dataD[2*i+1];}}
 else if(vect->wSize == 8)
     {for(i=0; i<nData; i++)
        for(j=0; j<nTimes; j++) {outVect->dataD[i*nTimes+j]=vect->dataD[i];}}

 return(outVect);}

/*--------------------------------------------------------------FrVectFillC--*/
int FrVectFillC(FrVect *vect,
                char value)
/*---------------------------------------------------------------------------*/
{
   if(vect->type != FR_VECT_C) 
      {FrError(3,"FrVectFillC","type missmatch");
       return(1);}
   if(vect->compress != 0) FrVectExpand(vect);
         
   if(vect->nData >= vect->space)
      {vect->space = 2*vect->space;
       if(vect->space == 0) vect->space = 16;
       vect->data = realloc(vect->data, vect->space*sizeof(char));
       if(vect->data == NULL) return(2);
       FrVectMap(vect);}

   vect->data[vect->nData] = value;
   vect->nData++;
   vect->nx[0]++;
   vect->nBytes += vect->wSize;

   return(0);}

/*--------------------------------------------------------------FrVectFillD--*/
int FrVectFillD(FrVect *vect,
                double value)
/*---------------------------------------------------------------------------*/
{
   if(vect->type != FR_VECT_8R) 
      {FrError(3,"FrVectFillD","type missmatch");
       return(1);}
   if(vect->compress != 0) FrVectExpand(vect);
         
   if(vect->nData >= vect->space)
      {vect->space = 2*vect->space;
       if(vect->space == 0) vect->space = 16;
       vect->data = realloc(vect->data, vect->space*sizeof(double));;
       if(vect->data == NULL) return(2);
       FrVectMap(vect);}

   vect->dataD[vect->nData] = value;
   vect->nData++;
   vect->nx[0]++;
   vect->nBytes += vect->wSize;

   return(0);}

/*--------------------------------------------------------------FrVectFillF--*/
int FrVectFillF(FrVect *vect,
                float value)
/*---------------------------------------------------------------------------*/
{
   if(vect->type != FR_VECT_4R) 
      {FrError(3,"FrVectFillF","type missmatch");
       return(1);}
   if(vect->compress != 0) FrVectExpand(vect);
         
   if(vect->nData >= vect->space)
      {vect->space = 2*vect->space;
       if(vect->space == 0) vect->space = 16;
       vect->data = realloc(vect->data, vect->space*sizeof(float));;
       if(vect->data == NULL) return(2);
       FrVectMap(vect);}

   vect->dataF[vect->nData] = value;
   vect->nData++;
   vect->nx[0]++;
   vect->nBytes += vect->wSize;

   return(0);}

/*--------------------------------------------------------------FrVectFillI--*/
int FrVectFillI(FrVect *vect,
                int value)
/*---------------------------------------------------------------------------*/
{
   if(vect->type != FR_VECT_4S) 
      {FrError(3,"FrVectFillI","type missmatch");
       return(1);}
   if(vect->compress != 0) FrVectExpand(vect);
          
   if(vect->nData >= vect->space)
      {vect->space = 2*vect->space;
       if(vect->space == 0) vect->space = 16;
       vect->data = realloc(vect->data, vect->space*sizeof(int));
       if(vect->data == NULL) return(2);
       FrVectMap(vect);}

   vect->dataI[vect->nData] = value;
   vect->nData++;
   vect->nx[0]++;
   vect->nBytes += vect->wSize;

   return(0);}

/*--------------------------------------------------------------FrVectFillQ--*/
int FrVectFillQ(FrVect *vect,
                char *value)
/*---------------------------------------------------------------------------*/
{
   if(vect->type != FR_VECT_STRING) 
      {FrError(3,"FrVectFillQ","type missmatch");
       return(1);}
         
   if(vect->nData >= vect->space)
      {vect->space = 2*vect->space;
       if(vect->space == 0) vect->space = 16;
       vect->data = realloc(vect->data, vect->space*sizeof(char *));
       if(vect->data == NULL) return(2);
       FrVectMap(vect);}

   FrStrCpy(&(vect->dataQ[vect->nData]), value);
   vect->nData++;
   vect->nx[0]++;
   vect->nBytes += vect->wSize+strlen(value)+1;

   return(0);}
/*--------------------------------------------------------------FrVectFillS--*/
int FrVectFillS(FrVect *vect,
                short value)
/*---------------------------------------------------------------------------*/
{
   if(vect->type != FR_VECT_2S) 
      {FrError(3,"FrVectFillS","type missmatch");
       return(1);}
   if(vect->compress != 0) FrVectExpand(vect);
         
   if(vect->nData >= vect->space)
      {vect->space = 2*vect->space;
       if(vect->space == 0) vect->space = 16;
       vect->data = realloc(vect->data, vect->space*sizeof(short));
       if(vect->data == NULL) return(2);
       FrVectMap(vect);}

   vect->dataS[vect->nData] = value;
   vect->nData++;
   vect->nx[0]++;
   vect->nBytes += vect->wSize;

   return(0);}
/*--------------------------------------------------------------FrVectFindQ--*/
int FrVectFindQ(FrVect *vect,
		char* name)
/*---------------------------------------------------------------------------*/
{int i;

 if(vect == NULL) return(-3);
 if(vect->type != FR_VECT_STRING) return (-2);

 for(i=0; i<vect->nData; i++)
    {if(strcmp(vect->dataQ[i], name) == 0) return(i);}

 return(-1);
}
/*---------------------------------------------------------- FrVectFixNAN----*/
int FrVectFixNAN(FrVect *vect)
/*---------------------------------------------------------------------------*/
/* This function set to 0 all NAN, INF or denormalized numbers               */
/*---------------------------------------------------------------------------*/
{
 unsigned int  nData, i, *dI, maskF = 0x7f800000, maskD = 0x7ff00000, swap; 
 char *swapC;
 int nFix, proj;

 if(vect == NULL) return(1);

 if(vect->compress != 0) FrVectExpand(vect);

 dI = vect->dataUI;
 nFix = 0;

 nData = vect->nData;
 if((vect->type == FR_VECT_8C)  || 
    (vect->type == FR_VECT_16C))  nData = 2*nData;

 if((vect->type == FR_VECT_4R) || 
    (vect->type == FR_VECT_8C) || 
    (vect->type == FR_VECT_8H))
   {for(i=0; i<  nData; i++) {if(dI[i] == 0)          continue;
                              if(dI[i] == 0x80000000) continue;
                              proj = dI[i] & maskF;
                              if((proj != maskF) && (proj != 0)) continue;
                              dI[i] = 0; 
                              nFix++;}}

 else if((vect->type == FR_VECT_8R)  ||
         (vect->type == FR_VECT_16C) ||
         (vect->type == FR_VECT_16H))
  {nData = 2*nData; 
   swap = 1;
   swapC= (char *) &swap;

   if(swapC[0] == 1)
       {for(i=0; i<nData; i=i+2) 
            {if(dI[i+1] == 0          && dI[i] == 0) continue;
             if(dI[i+1] == 0x80000000 && dI[i] == 0) continue;
             proj = dI[i+1] & maskD;
             if((proj != maskD) && (proj != 0)) continue;
             dI[i]   = 0; 
             dI[i+1] = 0;
             nFix++;}}
   else{for(i=0; i<nData; i=i+2) 
            {if(dI[i] == 0          && dI[i+1] == 0) continue;
             if(dI[i] == 0x80000000 && dI[i+1] == 0) continue;
             proj = dI[i] & maskD;
             if((proj != maskD) && (proj != 0)) continue; 
             dI[i]   = 0;
             dI[i+1] = 0; 
             nFix++;}}}

 return(nFix);}

/*------------------------------------------------------FrVectFixVirgoImage--*/
void FrVectFixVirgoImage(FrVect *vect)
/*---------------------------------------------------------------------------*/
/* This function is for backward compatibility for Virgo images before 
   May 09 ( before v8r05) when the time was store as the third dimension. 
   Only single images were produced: no "movie"                           */
/*-------------------------------------------------------------------------- */
{
  double dt, startX;

  if(vect == NULL) return;
  if(vect->nDim != 3) return;
  if(vect->nx[2] != 1) return;

  vect->nx[2] = vect->nx[1];
  vect->nx[1] = vect->nx[0];
  vect->nx[0] = 1;

  dt = vect->dx[2];
  vect->dx[2] = vect->dx[1];
  vect->dx[1] = vect->dx[0];
  vect->dx[0] = dt;

  startX = vect->startX[2];
  vect->startX[2] = vect->startX[1];
  vect->startX[1] = vect->startX[0];
  vect->startX[0] = startX;

  free(vect->unitX[2]);
  vect->unitX[2] = vect->unitX[1];
  vect->unitX[1] = vect->unitX[0];
  FrStrCpy(&vect->unitX[0],"Time [sec]");

  return;}

/*---------------------------------------------------------------FrVectFree--*/
void FrVectFree(FrVect *vect)
/*---------------------------------------------------------------------------*/
{int i;

 if(vect == NULL) return;

 if(vect->next != NULL) FrVectFree(vect->next);

 if(vect->dataUnzoomed != NULL) FrVectZoomOut(vect);

 if(vect->data != NULL) 
      {if(vect->type == FR_VECT_STRING)
	{for(i=0; i<vect->nData; i++) 
           {if(vect->dataQ[i] != NULL) free(vect->dataQ[i]);}}
       free(vect->data);}

 free(vect->nx);
 free(vect->dx);
 if(vect->unitY != NULL) {free(vect->unitY);}
 if(vect->unitX != NULL)
     {for(i=0; i<vect->nDim; i++)
       {if(vect->unitX[i] != NULL) free(vect->unitX[i]);}
      free(vect->unitX);}
 if(vect->name != NULL) free(vect->name);
 free(vect->startX);
 free(vect);

 return;}
/*----------------------------------------------------------FrVectGetIndex--*/
FRLONG FrVectGetIndex(FrVect *vect, 
                      double x)
/*--------------------------------------------------------------------------*/
{FRLONG i;

 if(vect == NULL)     return(-1);
 if(vect->dx[0] == 0) return(-2);
 
 i = ((x-vect->startX[0])+1.e-8*vect->dx[0])/vect->dx[0];
 if(i < vect->nData-1 && 
    (x-vect->startX[0]-i*vect->dx[0] > 0.5*vect->dx[0])) i++;

 if(i < 0)            return(-3);
 if(i >= vect->nData) return(-4);

 return(i);}

/*--------------------------------------------------------- FrameGetTotSize--*/
FRLONG FrVectGetTotSize(FrVect *vect)
/*---------------------------------------------------------------------------*/
{FRLONG size;
 int i;

 if(vect == NULL) return(0);

 size = 54 + vect->nDim*(30)+vect->nData*vect->wSize;
 if(vect->name  != NULL) size += strlen(vect->name);
 if(vect->unitY != NULL) size += strlen(vect->unitY);
 for(i=0; i<vect->nDim; i++)
     {if(vect->unitX[i] != NULL) size += strlen(vect->unitX[i]);}

 size += FrVectGetTotSize(vect->next);

 return(size);}

/*--------------------------------------------------------------FrVectGetV--*/
double FrVectGetV(FrVect *vect, 
                  FRULONG i)
/*--------------------------------------------------------------------------*/
{ return(FrVectGetValueI(vect,i));}
/*---------------------------------------------------------FrVectGetValueI--*/
double FrVectGetValueI(FrVect *vect, 
                       FRULONG i)
/*--------------------------------------------------------------------------*/
{
 if(vect == NULL)     return(0.);
 if(i >= vect->nData) return(0.);

 if(vect->compress != 0) FrVectExpand(vect); 

 if     (vect->type == FR_VECT_C)   {return((double) vect->data [i]);}
 else if(vect->type == FR_VECT_2S)  {return((double) vect->dataS[i]);}
 else if(vect->type == FR_VECT_8R)  {return(         vect->dataD[i]);}
 else if(vect->type == FR_VECT_4R)  {return((double) vect->dataF[i]);}
 else if(vect->type == FR_VECT_4S)  {return((double) vect->dataI[i]);}
 else if(vect->type == FR_VECT_8S)  {return((double) vect->dataL[i]);}
 else if(vect->type == FR_VECT_2U)  {return((double) vect->dataUS[i]);}
 else if(vect->type == FR_VECT_4U)  {return((double) vect->dataUI[i]);}
 else if(vect->type == FR_VECT_8U)  {return((double) vect->dataUL[i]);}
 else if(vect->type == FR_VECT_1U)  {return((double) vect->dataU[i]);}

 return(0.);}
/*---------------------------------------------------------FrVectGetValueX--*/
double FrVectGetValueX(FrVect *vect, 
                       double x)
/*--------------------------------------------------------------------------*/
{FRLONG i;

 i = FrVectGetIndex(vect, x);
 if(i<0)     return(0.);

 return(FrVectGetValueI(vect,i));}

/*-------------------------------------------------------FrVectGetValueGPS--*/
double FrVectGetValueGPS(FrVect *vect, 
                         double gps)
/*--------------------------------------------------------------------------*/
{FRLONG i;

 i = FrVectGetIndex(vect, gps - vect->GTime);
 if(i<0)     return(0.);

 return(FrVectGetValueI(vect,i));}

/*---------------------------------------------------------------FrVectHtoC--*/
int FrVectHtoC(FrVect *vect)
/*---------------------------------------------------------------------------*/
/* Convert an Half complex vector to a regular complex vector                */
/*---------------------------------------------------------------------------*/
{double *outD, *dataD;
 float *outF, *dataF;
 FRLONG i, nData, a, end;

 if(vect == NULL) return(-1); 
 if(vect->compress != 0) FrVectExpand(vect);

 nData = vect->nData;
 a = 2*nData;
 end = nData/2+1;

 if(vect->type == FR_VECT_8H)
   {outF = (float *) malloc(2*nData*sizeof(float));
    if(outF == NULL) return(-2);
    vect->type = FR_VECT_8C;

    dataF = vect->dataF;              
    outF[0] = dataF[0];          /*---------------------------- DC part------*/
    outF[1] = 0.;

    for (i=1; i<end; i++) 
      {outF[  2*i]   = dataF[i];
       outF[  2*i+1] = dataF[nData-i];
       outF[a-2*i]   = outF[2*i];
       outF[a-2*i+1] =-outF[2*i+1];}

    free(vect->data);
    vect->data = (char *) outF;

    if(nData%2 == 0) outF[nData+1] = 0.;} /*------------Nyquiest frequency---*/

 else if(vect->type == FR_VECT_16H)
   {outD = (double *) malloc(2*nData*sizeof(double));
    if(outD == NULL) return(-2);
    vect->type = FR_VECT_16C;

    dataD = vect->dataD;              
    outD[0] = dataD[0];         
    outD[1] = 0.;

    for (i=1; i<end; i++) 
      {outD[  2*i]   = dataD[i];
       outD[  2*i+1] = dataD[nData-i];
       outD[a-2*i]   = outD[2*i];
       outD[a-2*i+1] =-outD[2*i+1];}

    free(vect->data);
    vect->data = (char *) outD;

    if(nData%2 == 0) outD[nData+1] = 0.;}

 else{return(-3);}

 vect->nBytes = 2*vect->nBytes;
 vect->wSize  = 2*vect->wSize;

 FrVectMap(vect);
 
 return(0);}

/*----------------------------------------------------------------FrVectInt--*/
void FrVectInt(FrVect *vect)
/*---------------------------------------------------------------------------*/
{int i;
 long long* dataL;

 if(vect == NULL) return;

 else if(vect->type == FR_VECT_C)
     {for(i=1; i<vect->nData; i++)
          {vect->data[i] = vect->data[i] + vect->data[i-1];}}

 else if(vect->type == FR_VECT_2S)
     {for(i=1; i<vect->nData; i++)
          {vect->dataS[i] = vect->dataS[i] + vect->dataS[i-1];}}

 else if(vect->type == FR_VECT_4S)
     {for(i=1; i<vect->nData; i++)
          {vect->dataI[i] = vect->dataI[i] + vect->dataI[i-1];}}

 else if(vect->type == FR_VECT_8S)
   {dataL = (long long*) vect->data;
    for(i=1; i<vect->nData; i++) {dataL[i] = dataL[i] + dataL[i-1];}}

 else if(vect->type == FR_VECT_1U) 
     {for(i=1; i<vect->nData; i++)
          {vect->dataU[i] = vect->data[i] + vect->data[i-1];}}

 else if(vect->type == FR_VECT_2U)
     {for(i=1; i<vect->nData; i++)
          {vect->dataUS[i] = vect->dataS[i] + vect->dataS[i-1];}}

 else if(vect->type == FR_VECT_4U) 
     {for(i=1; i<vect->nData; i++)
          {vect->dataUI[i] = vect->dataI[i] + vect->dataI[i-1];}}

return;}
/*--------------------------------------------------------- FrVectIsValid----*/
int FrVectIsValid(FrVect *vect)
/*---------------------------------------------------------------------------*/
/* returns 0 if the vector do not contains NaN or INF numbers                */
/*                  denormalized numbers or OK since v8r14                   */
/* returns 0 if the vector is not an integer number                          */
/* returns 1 in the other cases                                              */
/*---------------------------------------------------------------------------*/
{
 unsigned int  nData, i, *dI, maskF = 0x7f800000, maskD = 0x7ff00000, swap; 
 char *swapC;

 if(vect == NULL) return(1);

 if(vect->compress != 0) FrVectExpand(vect);

 dI = vect->dataUI;

 nData = vect->nData;
 if((vect->type == FR_VECT_8C)  || 
    (vect->type == FR_VECT_16C))  nData = 2*nData;

 if((vect->type == FR_VECT_4R) || 
    (vect->type == FR_VECT_8C) || 
    (vect->type == FR_VECT_8H))
   {for(i=0; i<  nData; i++) {if(dI[i] == 0)          continue;
                              if(dI[i] == 0x80000000) continue;
                              if((dI[i] & maskF) == maskF) 
                                {FrVectIsValidFillAux(vect);
                                 return(i+1);}}}

 else if((vect->type == FR_VECT_8R)  ||
         (vect->type == FR_VECT_16C) ||
         (vect->type == FR_VECT_16H))
  {nData = 2*nData; 
   swap = 1;
   swapC= (char *) &swap;

   if(swapC[0] == 1)
       {for(i=0; i<nData; i=i+2) 
            {if(dI[i+1] == 0          && dI[i] == 0) continue;
             if(dI[i+1] == 0x80000000 && dI[i] == 0) continue;
             if((dI[i+1] & maskD) == maskD) 
               {FrVectIsValidFillAux(vect);
                return(i/2+1);}}}
   else{for(i=0; i<nData; i=i+2) 
            {if(dI[i] == 0          && dI[i+1] == 0) continue;
             if(dI[i] == 0x80000000 && dI[i+1] == 0) continue;
             if((dI[i] & maskD) == maskD) 
               {FrVectIsValidFillAux(vect);
                return(i/2+1);}}}}

 return(0);}

/*-------------------------------------------------- FrVectIsValidFillAux----*/
void FrVectIsValidFillAux(FrVect *vect)
/*---------------------------------------------------------------------------*/
/* returns 0 if the vector do not contains NaN or INF numbers                */
/*                  denormalized numbers or OK since v8r14                   */
/* returns 0 if the vector is not an integer number                          */
/* returns 1 in the other cases                                              */
/*---------------------------------------------------------------------------*/
{
 unsigned int  nData, i, *dI, maskF = 0x7f800000, maskD = 0x7ff00000, swap; 
 char *swapC, *a;
 FrVect *aux;

 if(vect->next != NULL) {
   if(strncmp(vect->next->name,"Available_data",14) != 0) return;
   a = vect->next->data;}
 else
   {aux = FrVectNew1D("Available_data",  FR_VECT_C,  vect->nData,
            vect->dx[0], vect->unitX[0], "1 if data is there; -1 for NaN/INF");
    if(aux == NULL) return;
    vect->next = aux;
    a = aux->data;
    for(i=0; i<aux->nData; i++) {a[i] = 1;}}
 
 dI = vect->dataUI;

 nData = vect->nData;
 if((vect->type == FR_VECT_8C)  || 
    (vect->type == FR_VECT_16C))  nData = 2*nData;

 if((vect->type == FR_VECT_4R) || 
    (vect->type == FR_VECT_8C) || 
    (vect->type == FR_VECT_8H))
   {for(i=0; i<  nData; i++) {if(dI[i] == 0)          continue;
                              if(dI[i] == 0x80000000) continue;
                              if((dI[i] & maskF) == maskF) a[i] = -1;}}

 else if((vect->type == FR_VECT_8R)  ||
         (vect->type == FR_VECT_16C) ||
         (vect->type == FR_VECT_16H))
  {nData = 2*nData; 
   swap = 1;
   swapC= (char *) &swap;

   if(swapC[0] == 1)
       {for(i=0; i<nData; i=i+2) 
            {if(dI[i+1] == 0          && dI[i] == 0) continue;
             if(dI[i+1] == 0x80000000 && dI[i] == 0) continue;
             if((dI[i+1] & maskD) == maskD) a[i/2] = -1;}}
   else{for(i=0; i<nData; i=i+2) 
            {if(dI[i] == 0          && dI[i+1] == 0) continue;
             if(dI[i] == 0x80000000 && dI[i+1] == 0) continue;
             if((dI[i] & maskD) == maskD) a[i/2] = -1;}}}

 return;}

/*-----------------------------------------------------FrVectIsValidStrict---*/
int FrVectIsValidStrict(FrVect *vect)
/*---------------------------------------------------------------------------*/
/* returns 0 if the vector do not contains NaN, INF or denormalized numbers  */
/* returns 0 if the vector is not an integer number                          */
/* returns 1 in the other cases                                              */
/*---------------------------------------------------------------------------*/
{
 unsigned int  nData, i, *dI, maskF = 0x7f800000, maskD = 0x7ff00000, swap; 
 char *swapC;

 if(vect == NULL) return(1);

 if(vect->compress != 0) FrVectExpand(vect);

 dI = vect->dataUI;

 nData = vect->nData;
 if((vect->type == FR_VECT_8C)  || 
    (vect->type == FR_VECT_16C))  nData = 2*nData;

 if((vect->type == FR_VECT_4R) || 
    (vect->type == FR_VECT_8C) || 
    (vect->type == FR_VECT_8H))
   {for(i=0; i<  nData; i++) {if(dI[i] == 0)          continue;
                              if(dI[i] == 0x80000000) continue;
                              if((dI[i] & maskF) == maskF) return(i+1);
                              if((dI[i] & maskF) == 0)     return(i+1);}}

 else if((vect->type == FR_VECT_8R)  ||
         (vect->type == FR_VECT_16C) ||
         (vect->type == FR_VECT_16H))
  {nData = 2*nData; 
   swap = 1;
   swapC= (char *) &swap;

   if(swapC[0] == 1)
       {for(i=0; i<nData; i=i+2) 
            {if(dI[i+1] == 0          && dI[i] == 0) continue;
             if(dI[i+1] == 0x80000000 && dI[i] == 0) continue;
             if((dI[i+1] & maskD) == maskD) return(i/2+1);
             if((dI[i+1] & maskD) == 0)     return(i/2+1);}}
   else{for(i=0; i<nData; i=i+2) 
            {if(dI[i] == 0          && dI[i+1] == 0) continue;
             if(dI[i] == 0x80000000 && dI[i+1] == 0) continue;
             if((dI[i] & maskD) == maskD) return(i/2+1);
             if((dI[i] & maskD) == 0)     return(i/2+1);}}}

 return(0);}

/*---------------------------------------------------------------------------*/
FrVect* FrVectLoad(char *fileName)
/*---------------------------------------------------------------------------*/
{FrVect *vect;
 FrFile *iFile;
 FrameH *frame;

 iFile = FrFileINew(fileName);
 if(iFile == NULL) return(NULL);

 frame = FrameRead(iFile);
 if(frame           == NULL) return(NULL);
 if(frame->procData == NULL) return(NULL);

 vect = frame->procData->data;
 vect->GTime = frame->GTimeS + 1.e-9 * frame->GTimeN;

 frame->procData->data = NULL;
 FrameFree(frame);
 FrFileIEnd(iFile);

 return(vect);}

/*----------------------------------------------------------------FrVectMap--*/
void FrVectMap(FrVect *vect)
/*---------------------------------------------------------------------------*/
{
   if(vect == NULL) return;

   vect->dataS  = (short          *) vect->data;
   vect->dataI  = (int            *) vect->data;
   vect->dataL  = (FRLONG         *) vect->data;
   vect->dataF  = (float          *) vect->data;
   vect->dataD  = (double         *) vect->data;
   vect->dataU  = (unsigned char  *) vect->data;
   vect->dataUS = (unsigned short *) vect->data;
   vect->dataUI = (unsigned int   *) vect->data;
   vect->dataUL = (FRULONG        *) vect->data;
   vect->dataQ  = (char          **) vect->data;

   return;}

/*-------------------------------------------------------------FrVectMergeT--*/
FrVect *FrVectMergeT(FrVect **vectIn,
                     int nVect)
/*---------------------------------------------------------------------------*/
/* This function take an array of nVect vector and merge them into a single
   vector sorted according the to value. The vector could have linked vector.
   The first vector must contain GPS time. 
   The inputs vectors could have time overlap.
   This function is not intendent for general use                            */
/*---------------------------------------------------------------------------*/
{FrVect *v, *vIn, *vOut;
 FRLONG i, nTot, iVect, iData, nData, iLast, *iStart, nCopy;
 double *tStart, tFirst, tNext;
                               /*--------------------  single file case -----*/

 if(nVect == 0) {
   if(vectIn != NULL) free(vectIn);
   return(NULL);}

 if(nVect == 1) {
   vOut = vectIn[0];
   free(vectIn);
   return(vOut);}

                               /*-------------------- create work arrays ----*/

 if((iStart = (FRLONG*) malloc(nVect*sizeof(FRLONG))) == NULL) return(NULL);
 if((tStart = (double*) malloc(nVect*sizeof(double))) == NULL) return(NULL);

                               /*----------- create the output vector(s) ----*/
 nTot = 0;
 for(i=0; i<nVect; i++) 
   {nTot += vectIn[i]->nData;
    iStart[i] = 0;
    tStart[i] = vectIn[i]->dataD[0];}

 vOut = FrVectNew1D(vectIn[0]->name,vectIn[0]->type,nTot,1.,NULL,NULL);
 if(vOut == NULL) return(NULL);
 v = vOut;
 for(vIn = vectIn[0]->next; vIn != NULL; vIn = vIn->next)
   {v->next = FrVectNew1D(vIn->name,vIn->type,nTot,1.,NULL,NULL);
    if(v->next == NULL) return (NULL);
    v = v->next;}

                               /*----- move the data by bloc of GPS time----*/
 iData = 0;
 while(iData < nTot)
   {tFirst = 1.e20;            /*-the vector to copy has the lowest GPS time*/
    iVect = 0;
    for(i = 0; i<nVect; i++)
       {if(tStart[i] > tFirst) continue;
        iVect = i;
        tFirst = tStart[i];}
    if(tFirst > 1.e15) break;

    tNext = 1.e20;            /*-find the starting time of the next vector*/
    for(i = 0; i<nVect; i++)
       {if(i==iVect) continue;
        if(tStart[i] > tNext) continue;
        tNext = tStart[i];}
                                 /*----what is the last element to copy? ---*/
    nData = vectIn[iVect]->nData;
    for(i=nData-1; i>iStart[iVect]-1; i--)
	{if(vectIn[iVect]->dataD[i] > tNext) continue;
         break;}
    iLast = i;
                                  /*----------------------- copy the data ---*/

    nCopy = iLast - iStart[iVect] + 1;
    vIn = vectIn[iVect];
    for(v = vOut; v != NULL; v = v->next) 
      {memcpy(v->data+v->wSize*iData,
            vIn->data+v->wSize*iStart[iVect], nCopy * v->wSize);
       vIn = vIn->next;}
                                 /*----------- update the work array info ---*/
    iData         += nCopy;
    iStart[iVect] += nCopy;
    if(iLast == nData-1) 
          {tStart[iVect] = 1.e20;}
     else {tStart[iVect] = vectIn[iVect]->dataD[iLast+1];}}

                                 /*---------------- free temporary space ----*/

 for(i=0; i<nVect; i++) {FrVectFree(vectIn[i]);}   
 free(vectIn);
 free(iStart);
 free(tStart);

 return(vOut);}

/*---------------------------------------------------------------FrVectMean--*/
double FrVectMean(FrVect *vect)
/*---------------------------------------------------------------------------*/
/* This function returns the mean value of the input vector vect or 0 in case*/
/* of problem (vector not found.                                             */
/*---------------------------------------------------------------------------*/
{int i;
 double mean,sum;

 if(vect == NULL)     return(0);
 if(vect->nData == 0) return(0);

 sum = 0.;
 if(vect->type == FR_VECT_C)
    {for(i=0; i<vect->nData; i++) {sum += vect->data[i];}}
 else if(vect->type == FR_VECT_2S)
    {for(i=0; i<vect->nData; i++) {sum += vect->dataS[i];}}
 else if(vect->type == FR_VECT_8R)
    {for(i=0; i<vect->nData; i++) {sum += vect->dataD[i];}}
 else if(vect->type == FR_VECT_4R)
    {for(i=0; i<vect->nData; i++) {sum += vect->dataF[i];}}
 else if(vect->type == FR_VECT_4S)
    {for(i=0; i<vect->nData; i++) {sum += vect->dataI[i];}}
 else if(vect->type == FR_VECT_8S)
    {for(i=0; i<vect->nData; i++) {sum += vect->dataL[i];}}
 else if(vect->type == FR_VECT_8C)
    {return(0);}
 else if(vect->type == FR_VECT_16C)
    {return(0);}
 else if(vect->type == FR_VECT_2U)
    {for(i=0; i<vect->nData; i++) {sum += vect->dataUS[i];}}
 else if(vect->type == FR_VECT_4U)
    {for(i=0; i<vect->nData; i++) {sum += vect->dataUI[i];}}
 else if(vect->type == FR_VECT_8U)
    {for(i=0; i<vect->nData; i++) {sum += vect->dataUL[i];}}
 else if(vect->type == FR_VECT_1U)
    {for(i=0; i<vect->nData; i++) {sum += vect->dataU[i];}}

 mean = sum/vect->nData;

 return(mean);}


/*-------------------------------------------------------------FrVectMinMax--*/
int FrVectMinMax(FrVect *vect, double *min, double *max)
/*---------------------------------------------------------------------------*/
/* This function computes the min and max value of the input vector vect.    */
/* It returns 1 in case of failure or 0 in case of success.                  */
/*---------------------------------------------------------------------------*/
{int i;
 double value;

 if(vect == NULL)     return(1);
 if(vect->nData == 0) return(1);
 
 *min = 1.e+37;
 *max =-1.e+37;
 if(FrVectIsValid(vect) != 0) return(2); 

 if(vect->type == FR_VECT_C)
    {for(i=0; i<vect->nData; i++) {value = vect->data[i];
                                   if(value > *max) *max = value;
                                   if(value < *min) *min = value;}}
 else if(vect->type == FR_VECT_2S)
    {for(i=0; i<vect->nData; i++) {value = vect->dataS[i];
                                   if(value > *max) *max = value;
                                   if(value < *min) *min = value;}}
 else if(vect->type == FR_VECT_8R)
    {for(i=0; i<vect->nData; i++) {value = vect->dataD[i];
                                   if(value > *max) *max = value;
                                   if(value < *min) *min = value;}}
 else if(vect->type == FR_VECT_4R)
    {for(i=0; i<vect->nData; i++) {value = vect->dataF[i];
                                   if(value > *max) *max = value;
                                   if(value < *min) *min = value;}}
 else if(vect->type == FR_VECT_4S)
    {for(i=0; i<vect->nData; i++) {value = vect->dataI[i];
                                   if(value > *max) *max = value;
                                   if(value < *min) *min = value;}}
 else if(vect->type == FR_VECT_8S)
    {for(i=0; i<vect->nData; i++) {value = vect->dataL[i];
                                   if(value > *max) *max = value;
                                   if(value < *min) *min = value;}}
 else if(vect->type == FR_VECT_8C)
    {return(0);}
 else if(vect->type == FR_VECT_16C)
    {return(0);}
 else if(vect->type == FR_VECT_2U)
    {for(i=0; i<vect->nData; i++) {value = vect->dataUS[i];
                                   if(value > *max) *max = value;
                                   if(value < *min) *min = value;}}
 else if(vect->type == FR_VECT_4U)
    {for(i=0; i<vect->nData; i++) {value = vect->dataUI[i];
                                   if(value > *max) *max = value;
                                   if(value < *min) *min = value;}}
 else if(vect->type == FR_VECT_8U)
    {for(i=0; i<vect->nData; i++) {value = vect->dataUL[i];
                                   if(value > *max) *max = value;
                                   if(value < *min) *min = value;}}
 else if(vect->type == FR_VECT_1U)
    {for(i=0; i<vect->nData; i++) {value = vect->dataU[i];
                                   if(value > *max) *max = value;
                                   if(value < *min) *min = value;}}

 return(0);}

/*----------------------------------------------------------------FrVectNew--*/
FrVect *FrVectNew(int type,
                  int nDim,
                  ...)
/*---------------------------------------------------------------------------*/
{FrVect *vect;
 va_list ap;
 char *unit;
 static int i, first = 0, wSize[FR_VECT_END];

 if(first == 0)
    {first = 1;
     wSize[FR_VECT_4R]  = sizeof(float);
     wSize[FR_VECT_8R]  = sizeof(double);
     wSize[FR_VECT_C]   = sizeof(char);
     wSize[FR_VECT_1U]  = sizeof(char);
     wSize[FR_VECT_2S]  = sizeof(short);
     wSize[FR_VECT_2U]  = sizeof(short);
     wSize[FR_VECT_4S]  = sizeof(int);
     wSize[FR_VECT_4U]  = sizeof(int);
     wSize[FR_VECT_8S]  = sizeof(FRLONG);
     wSize[FR_VECT_8U]  = sizeof(FRLONG);
     wSize[FR_VECT_STRING] = sizeof(char *);
     wSize[FR_VECT_8C]   = 2*sizeof(float);
     wSize[FR_VECT_16C]  = 2*sizeof(double);
     wSize[FR_VECT_8H]   = sizeof(float);
     wSize[FR_VECT_16H]  = sizeof(double);}

   vect = (FrVect *) calloc(1,sizeof(FrVect));
   if(vect == NULL) return(NULL);
   vect->classe = FrVectDef();

   vect->compress = 0;
   if(type < 0) 
         vect->type = -type;
   else  vect->type =  type;
   vect->nDim = nDim;
   vect->nData = 1;
   vect->nx    = malloc(nDim*sizeof(FRULONG));
   vect->unitX = malloc(nDim*sizeof(char *));
   vect->dx    = malloc(nDim*sizeof(double));
   vect->startX= malloc(nDim*sizeof(double));
   if(vect->nx    == NULL || 
      vect->unitX == NULL || 
      vect->dx    == NULL || 
      vect->startX== NULL) 
       {FrError(3,"FrVectNew","malloc failed");
        return(NULL);}

   va_start(ap,nDim);
   for(i=0; i<nDim; i++)
     {vect->nx[i] = va_arg(ap,int);
      vect->dx[i] = va_arg(ap,double);
      unit = va_arg(ap,char *);
      FrStrCpy(&vect->unitX[i], unit);
      vect->nData = vect->nData*vect->nx[i];
      vect->startX[i] = 0.;}
   va_end(ap);

   vect->wSize  = wSize[vect->type];
   vect->space  = vect->nData;
   vect->nBytes = vect->nData*vect->wSize;
   
   if(vect->nData > 0)
       {if(type >= 0)
             {vect->data = calloc(vect->nData, vect->wSize);}
        else {vect->data = malloc(vect->nBytes);}
        if(vect->data == NULL) 
           {FrError(3,"FrVectNew","calloc failed");
            return(NULL);}}
   else if(vect->nData <0)
        {FrError(3,"FrVectNew","nData is too big");
         return(NULL);}
   else {vect->data = NULL;}

   FrVectMap(vect);
 
   vect->unitY = NULL;
   vect->next  = NULL;

   return(vect);
}
/*---------------------------------------------------------------FrVectNewL--*/
FrVect *FrVectNewL(int type,
                   int nDim,
                   ...)
/*---------------------------------------------------------------------------*/
{FrVect *vect;
 va_list ap;
 char *unit;
 static int i, first = 0, wSize[FR_VECT_END];

 if(first == 0)
    {first = 1;
     wSize[FR_VECT_4R]  = sizeof(float);
     wSize[FR_VECT_8R]  = sizeof(double);
     wSize[FR_VECT_C]   = sizeof(char);
     wSize[FR_VECT_1U]  = sizeof(char);
     wSize[FR_VECT_2S]  = sizeof(short);
     wSize[FR_VECT_2U]  = sizeof(short);
     wSize[FR_VECT_4S]  = sizeof(int);
     wSize[FR_VECT_4U]  = sizeof(int);
     wSize[FR_VECT_8S]  = sizeof(FRLONG);
     wSize[FR_VECT_8U]  = sizeof(FRLONG);
     wSize[FR_VECT_STRING] = sizeof(char *);
     wSize[FR_VECT_8C]   = 2*sizeof(float);
     wSize[FR_VECT_16C]  = 2*sizeof(double);
     wSize[FR_VECT_8H]   = sizeof(float);
     wSize[FR_VECT_16H]  = sizeof(double);}

   vect = (FrVect *) calloc(1,sizeof(FrVect));
   if(vect == NULL) return(NULL);
   vect->classe = FrVectDef();

   vect->compress = 0;
   if(type < 0) 
         vect->type = -type;
   else  vect->type =  type;
   vect->nDim = nDim;
   vect->nData = 1;
   vect->nx    = malloc(nDim*sizeof(FRLONG));
   vect->unitX = malloc(nDim*sizeof(char *));
   vect->dx    = malloc(nDim*sizeof(double));
   vect->startX= malloc(nDim*sizeof(double));
   if(vect->nx    == NULL || 
      vect->unitX == NULL || 
      vect->dx    == NULL || 
      vect->startX== NULL) 
       {FrError(3,"FrVectNew","malloc failed");
        return(NULL);}

   va_start(ap,nDim);
   for(i=0; i<nDim; i++)
     {vect->nx[i] = va_arg(ap,FRLONG);
      vect->dx[i] = va_arg(ap,double);
      unit = va_arg(ap,char *);
      FrStrCpy(&vect->unitX[i], unit);
      vect->nData = vect->nData*vect->nx[i];
      vect->startX[i] = 0.;}
   va_end(ap);

   vect->wSize  = wSize[vect->type];
   vect->space  = vect->nData;
   vect->nBytes = vect->nData*vect->wSize;
   
   if(vect->nData > 0)
       {if(type >= 0)
             {vect->data = calloc(vect->nData, vect->wSize);}
        else {vect->data = malloc(vect->nBytes);}
        if(vect->data == NULL) 
           {FrError(3,"FrVectNew","calloc failed");
            return(NULL);}}
   else if(vect->nData <0)
        {FrError(3,"FrVectNew","nData is too big");
         return(NULL);}
   else {vect->data = NULL;}

   FrVectMap(vect);
 
   vect->unitY = NULL;
   vect->next  = NULL;

   return(vect);
}
/*--------------------------------------------------------------FrVectNewTS--*/
FrVect *FrVectNewTS(char *name, 
                    double sampleRate, 
                    FRLONG nData, 
                    int nBits)
/*---------------------------------------------------------------------------*/
{FrVect *vect;
 double dx;
 int type;
 
  if(nBits >16)       {type = FR_VECT_4S;}
  else if(nBits >  8) {type = FR_VECT_2S;}
  else if(nBits >  0) {type = FR_VECT_C;}
  else if(nBits >-33) {type = FR_VECT_4R;}
  else                {type = FR_VECT_8R;}

  if(sampleRate == 0.)
       {dx = 0.;}
  else {dx = 1./sampleRate;}

  vect = FrVectNew1D(name,type,nData,dx,"time (s)",NULL);
  
  return(vect);} 
/*-------------------------------------------------------------FrVectNew1D--*/
FrVect *FrVectNew1D(char *name, 
                    int type, 
                    FRLONG nData, 
                    double dx,
                    char *unitx,
                    char *unity)
/*--------------------------------------------------------------------------*/
{FrVect *vect;
 
 vect = FrVectNewL(type,1,nData,dx,unitx);
 if(vect == NULL) return(NULL);

 if(name != NULL)
   {if(FrStrCpy(&vect->name,name) == NULL) return(NULL);}

 if(unity != NULL)
   {if(FrStrCpy(&vect->unitY,unity) == NULL) return(NULL);}
  
 return(vect);} 

/*--------------------------------------------------------------FrVectRead--*/
FrVect *FrVectRead(FrFile *iFile)
/*--------------------------------------------------------------------------*/
{FrVect *v;
 static int first=0, wSize[FR_VECT_END];
 unsigned short type;
 unsigned int i, nBytes, nData, nx;
 char message[128];

 if(iFile->fmtVersion == 3) return(FrBack3VectRead(iFile));

 if(first == 0)
    {first = 1;
     wSize[FR_VECT_4R]  = sizeof(float);
     wSize[FR_VECT_8R]  = sizeof(double);
     wSize[FR_VECT_C]   = sizeof(char);
     wSize[FR_VECT_1U]  = sizeof(char);
     wSize[FR_VECT_2S]  = sizeof(short);
     wSize[FR_VECT_2U]  = sizeof(short);
     wSize[FR_VECT_4S]  = sizeof(int);
     wSize[FR_VECT_4U]  = sizeof(int);
     wSize[FR_VECT_8S]  = sizeof(FRLONG);
     wSize[FR_VECT_8U]  = sizeof(FRLONG);
     wSize[FR_VECT_8C]  = 2*sizeof(float);
     wSize[FR_VECT_16C] = 2*sizeof(double);
     wSize[FR_VECT_8H]  = sizeof(float);
     wSize[FR_VECT_16H] = sizeof(double);}
 
 v = (FrVect *) calloc(1,sizeof(FrVect));
 if(v == NULL)  
      {iFile->error = FR_ERROR_MALLOC_FAILED;
       return(NULL);}
 v->classe = FrVectDef();
 
 FrReadHeader(iFile,  v);
 FrReadSChar( iFile, &v->name);
 FrReadShortU(iFile, &v->compress);
 if (v->compress == 256) v->compress = 0;/*we will swap bytes at reading time*/

 FrReadShortU(iFile, &type);
 v->type = type;
 if(iFile->fmtVersion > 5)
      {FrReadLong  (iFile, (FRLONG *)&v->nData);
       FrReadLong  (iFile, (FRLONG *)&v->nBytes);}
 else {FrReadIntU  (iFile, &nData);
       FrReadIntU  (iFile, &nBytes);
       v->nData  = nData;
       v->nBytes = nBytes;}
 v->space = v->nData;

 if     (v->compress != 0)  
        {FrReadVC (iFile, &v->data, v->nBytes);}

 else if((v->type == FR_VECT_C ) || (v->type == FR_VECT_1U)) 
        {FrReadVC (iFile, &v->data, v->nData);}
 else if((v->type == FR_VECT_2S) || (v->type == FR_VECT_2U))
        {FrReadVS (iFile, &v->dataS, v->nData);
	 v->data = (char *)v->dataS;}
 else if((v->type == FR_VECT_8R) || (v->type == FR_VECT_16H))
        {FrReadVD (iFile, &v->dataD, v->nData);
	 v->data = (char *)v->dataD;}
 else if((v->type == FR_VECT_4R) || (v->type == FR_VECT_8H))
        {FrReadVF (iFile, &v->dataF, v->nData);
	 v->data = (char *)v->dataF;}
 else if((v->type == FR_VECT_4S) || (v->type == FR_VECT_4U))
        {FrReadVI (iFile, &v->dataI, v->nData);
	 v->data = (char *)v->dataI;}
 else if((v->type == FR_VECT_8S) || (v->type == FR_VECT_8U)) 
        {FrReadVL (iFile, &v->dataL, v->nData);
	 v->data = (char *)v->dataL;}
 else if(v->type == FR_VECT_8C) 
        {FrReadVF (iFile, &v->dataF, v->nData*2);
	 v->data = (char *)v->dataF;}
 else if(v->type == FR_VECT_16C) 
        {FrReadVD (iFile, &v->dataD, v->nData*2);
 	 v->data = (char *)v->dataD;}
 else if(v->type == FR_VECT_STRING) 
        {FrReadVQ (iFile, &v->dataQ, v->nData);
 	 v->data = (char *)v->dataQ;}
 else {iFile->error = FR_ERROR_READ_ERROR;
       return(NULL);}

 if(iFile->error == FR_ERROR_MALLOC_FAILED) return(NULL);

 FrVectMap(v);

 FrReadIntU  (iFile, &v->nDim);
 if(iFile->fmtVersion > 5)
      {FrReadVL    (iFile, (FRLONG**)&v->nx, v->nDim);}
 else {v->nx = malloc(v->nDim*sizeof(FRULONG));
       for(i=0; i<v->nDim; i++)
         {FrReadIntU (iFile, &nx);
          v->nx[i] = nx;}}
 FrReadVD    (iFile, &v->dx,           v->nDim);
 FrReadVD    (iFile, &v->startX,       v->nDim);
 FrReadVQ    (iFile, &v->unitX,        v->nDim);
 FrReadSChar (iFile, &v->unitY);
 FrReadStruct(iFile, &v->next);
 if(iFile->fmtVersion >=8) FrReadStructChksum(iFile);

 v->wSize= wSize[v->type];

 if(iFile->nBytes != iFile->nBytesR + iFile->length)
     {sprintf(message,": Record length error: "
         "nBytes=%"FRLLD" nBytesR=%"FRLLD" length=%"FRLLD"\n",
          iFile->nBytes, iFile->nBytesR, iFile->length);
      FrError(3,"FrVectRead",message);
      return(NULL);}     

 if(FrDebugLvl > 2)
    {if(v->name != NULL) fprintf(FrFOut," %-24s ", v->name);
     fprintf(FrFOut," nBytes=%"FRLLD"\n", v->nBytes);}

 if(iFile->compress == 0 && v->compress != 0) {FrVectExpand(v);}
 
 return(v);}
/*---------------------------------------------------------- FrVectReadNext--*/
FrVect *FrVectReadNext(FrFile *iFile, double gtime, char *name)
/*---------------------------------------------------------------------------*/
/* This function search for FrVect (skiping FrSE and FrSH  records)          */
/*---------------------------------------------------------------------------*/
{FrVect *vect;

 if(FrFileIGoToNextRecord(iFile) != iFile->vectorType) return(NULL);

 vect = FrVectRead(iFile);
 if(iFile->instanceH != iFile->vectInstance)
   {FrError(3,"FrVectReadNext","Vector instance missmatch");
    return(NULL);}

 if(vect == NULL) return(NULL);

 iFile->vectInstance = iFile->instance;
 if(iFile->instance != 0) vect->next = FrVectReadNext(iFile, gtime, name);

 vect->GTime     = gtime;
 vect->ULeapS    = iFile->toc->ULeapS;
 vect->localTime = FrTOCGetLTOffset(iFile, name);

 return(vect);}
/*----------------------------------------------------------------FrVectRMS--*/
double FrVectRMS(FrVect *vect)
/*---------------------------------------------------------------------------*/
/* This function computes the RMS of the input vector vect.                  */
/* It returns -1 in case of error.                                           */
/*---------------------------------------------------------------------------*/
{int i;
 double sum, sum2, val, rms, rms2, mean;

 rms = -1.;
 if(vect == NULL)     return(-1);
 if(vect->nData == 0) return(-1);

 sum  = 0.;
 sum2 = 0.;

 if(vect->type == FR_VECT_C)
    {for(i=0; i<vect->nData; i++) {val = vect->data[i];
                                   sum  += val;
                                   sum2 += val*val;}}
 else if(vect->type == FR_VECT_2S)
    {for(i=0; i<vect->nData; i++) {val = vect->dataS[i];
                                   sum  += val;
                                   sum2 += val*val;}}
 else if(vect->type == FR_VECT_8R)
    {for(i=0; i<vect->nData; i++) {val = vect->dataD[i];
                                   sum  += val;
                                   sum2 += val*val;}}
 else if(vect->type == FR_VECT_4R)
    {for(i=0; i<vect->nData; i++) {val = vect->dataF[i];
                                   sum  += val;
                                   sum2 += val*val;}}
 else if(vect->type == FR_VECT_4S)
    {for(i=0; i<vect->nData; i++) {val = vect->dataI[i];
                                   sum  += val;
                                   sum2 += val*val;}}
 else if(vect->type == FR_VECT_8S)
    {for(i=0; i<vect->nData; i++) {val = vect->dataL[i];
                                   sum  += val;
                                   sum2 += val*val;}}
 else if(vect->type == FR_VECT_C8)
    {return(-1.);}
 else if(vect->type == FR_VECT_C16)
    {return(-1.);}
 else if(vect->type == FR_VECT_2U)
    {for(i=0; i<vect->nData; i++) {val = vect->dataUS[i];
                                   sum  += val;
                                   sum2 += val*val;}}
 else if(vect->type == FR_VECT_4U)
    {for(i=0; i<vect->nData; i++) {val = vect->dataUI[i];
                                   sum  += val;
                                   sum2 += val*val;}}
 else if(vect->type == FR_VECT_8U)
    {for(i=0; i<vect->nData; i++) {val = vect->dataUL[i];
                                   sum  += val;
                                   sum2 += val*val;}}
 else if(vect->type == FR_VECT_1U)
    {for(i=0; i<vect->nData; i++) {val = vect->dataU[i];
                                   sum  += val;
                                   sum2 += val*val;}}

 mean = sum/vect->nData;
 rms2 = sum2/vect->nData - (mean*mean);
 rms = sqrt(rms2);

 return(rms);}

/*---------------------------------------------------------------------------*/
int FrVectSave(FrVect *vect, char *fileName)
/*---------------------------------------------------------------------------*/
{
  FrFile *oFile;
  char *fName;
  FrProcData *proc;
  FrameH* frame;

  if(vect == NULL) return(1);
  if(vect->name == NULL) return(1);

  /*----------------------------- build file name if needed and open file---*/ 
 if(fileName == NULL) {
    fName = malloc(strlen(vect->name)+20);
    if(vect->name == NULL) return(2);
    sprintf(fName,"%s_%.0f.vect",vect->name, vect->GTime);}
  else{fName = fileName;}

 oFile = FrFileONew(fName, 8); 
 if(oFile == NULL) return(2);

 frame = FrameNew(vect->name);
 if(frame == NULL) return(3);

 /*------------------------------------- store timing info and add vector ---*/
 frame->GTimeS = vect->GTime;
 frame->GTimeN = 1.e9*(vect->GTime - frame->GTimeS);
 frame->dt = vect->dx[0]*vect->nData;

 proc = FrProcDataNewV(frame, vect);
 if(proc == NULL) return(4);

 /*--------------------------------------- write frame and then free space---*/
 FrameWrite(frame, oFile);

 proc->data = NULL;
 FrameFree(frame);

 FrFileOEnd(oFile);
 if(fileName == NULL) free(fName);

 return(0);}
 
/*----------------------------------------------------------FrVectReadZoom--*/
FrVect * FrVectReadZoom( FrFile *iFile, double xStart, double length  )
/*--------------------------------------------------------------------------*/
{
   FrVect *v;
   static int first=0, wSize[FR_VECT_END];
   unsigned short type;
   FRULONG cur;
   FRULONG pos;

   if (first == 0)
   {
     first = 1;
     wSize[FR_VECT_4R]  = sizeof(float);
     wSize[FR_VECT_8R]  = sizeof(double);
     wSize[FR_VECT_C]   = sizeof(char);
     wSize[FR_VECT_1U]  = sizeof(char);
     wSize[FR_VECT_2S]  = sizeof(short);
     wSize[FR_VECT_2U]  = sizeof(short);
     wSize[FR_VECT_4S]  = sizeof(int);
     wSize[FR_VECT_4U]  = sizeof(int);
     wSize[FR_VECT_8S]  = sizeof(FRLONG);
     wSize[FR_VECT_8U]  = sizeof(FRLONG);
     wSize[FR_VECT_8C]  = 2*sizeof(float);
     wSize[FR_VECT_16C] = 2*sizeof(double);
     wSize[FR_VECT_8H]  = sizeof(float);
     wSize[FR_VECT_16H] = sizeof(double);
   }

   if ( FrFileIGoToNextRecord( iFile ) != iFile->vectorType )
     return NULL;

   v = calloc( 1, sizeof( FrVect ) );
   if ( ! v )
   {
     iFile->error = FR_ERROR_MALLOC_FAILED;
     return NULL;
   }
   v->classe = FrVectDef();

   FrReadHeader( iFile, v );
   FrReadSChar( iFile, &v->name );
   FrReadShortU( iFile, &v->compress );
   if ( v->compress == 256 )
     v->compress = 0; /* we will swap bytes at reading time */
   FrReadShortU( iFile, &type );
   v->type = type;
   v->wSize = wSize[v->type];
   FrReadLong( iFile, (FRLONG *)&v->nData );
   FrReadLong( iFile, (FRLONG *)&v->nBytes );
   v->space = v->nData;

   /* skip the data */
   cur = FrIOTell( iFile->frfd );
   pos = cur + v->nBytes;

   /* read the vect info */
   FrFileIOSet( iFile, pos );
   FrReadIntU( iFile, &v->nDim );
   FrReadVL( iFile, (FRLONG**)&v->nx, v->nDim );
   FrReadVD( iFile, &v->dx, v->nDim );
   FrReadVD( iFile, &v->startX, v->nDim );
   FrReadVQ( iFile, &v->unitX, v->nDim );
   FrReadSChar( iFile, &v->unitY );
   FrReadStruct( iFile, &v->next );

   /* go back to read the required data */
   pos = cur;
   cur = FrIOTell( iFile->frfd ); /* keep current position */
   FrFileIOSet( iFile, pos );

   if ( v->nDim != 1 || v->dx[0] == 0
       || v->type == FR_VECT_8H || v->type == FR_VECT_16H )
   { /* problem here... */
     free( v );
     return NULL;
   }

   if ( v->compress )
   {
     FrReadVC( iFile, &v->data, v->nBytes );
     if ( iFile->error == FR_ERROR_MALLOC_FAILED )
     {
       free( v );
       return NULL;
     }
     if ( FrVectZoomIn( v, xStart, length ) )
     { /* problem here... */
       free( v );
       return NULL;
     }
   }
   else
   { /* this duplicates the zoom in process */
     int iFirst, nBin;
     iFirst = ( xStart - v->startX[0] + 1e-4*v->dx[0] ) / v->dx[0];
     if ( iFirst < 0 )
       iFirst = 0;
     if ( iFirst >= (int)v->nData )
       return NULL;
     nBin = ( length + 1e-4*v->dx[0] ) / v->dx[0];
     if ( nBin + iFirst > (int)v->nData )
       nBin = v->nData - iFirst;
     v->nData = nBin;
     v->nx[0] = nBin;
     v->startX[0] += iFirst * v->dx[0];

     /* go to the correct location and read the data */
     pos += iFirst * v->wSize;
     FrFileIOSet( iFile, pos );
     switch ( v->type )
     {
       case FR_VECT_C:
       case FR_VECT_1U:
         FrReadVC( iFile, &v->data, v->nData );
         break;
       case FR_VECT_2S:
       case FR_VECT_2U:
         FrReadVS( iFile, &v->dataS, v->nData );
         v->data = (char *)v->dataS;
         break;
       case FR_VECT_8R:
         FrReadVD( iFile, &v->dataD, v->nData );
         v->data = (char *)v->dataD;
         break;
       case FR_VECT_4R:
         FrReadVF( iFile, &v->dataF, v->nData );
         v->data = (char *)v->dataF;
         break;
       case FR_VECT_4S:
       case FR_VECT_4U:
         FrReadVI( iFile, &v->dataI, v->nData );
         v->data = (char *)v->dataI;
         break;
       case FR_VECT_8S:
       case FR_VECT_8U:
         FrReadVL( iFile, &v->dataL, v->nData );
         v->data = (char *)v->dataL;
         break;
       case FR_VECT_8C:
         FrReadVF( iFile, &v->dataF, 2 * v->nData );
         v->data = (char *)v->dataF;
         break;
       case FR_VECT_16C:
         FrReadVD( iFile, &v->dataD, 2 * v->nData );
         v->data = (char *)v->dataD;
         break;
       case FR_VECT_STRING:
         FrReadVQ( iFile, &v->dataQ, v->nData );
         v->data = (char *)v->dataQ;
         break;
       default:
         iFile->error = FR_ERROR_READ_ERROR;
         free( v );
         return NULL;
     }
     if ( iFile->error == FR_ERROR_MALLOC_FAILED )
     {
       free( v );
       return NULL;
     }
     FrVectMap( v );
   }

   /* reset position and return */
   FrFileIOSet( iFile, cur );
   return v;
}
/*--------------------------------------------------------- FrVectResample--*/
FrVect *FrVectResample(FrVect *vect,
                       int nDataNew,
                       FrVect *outVect,
                       char *newName)
/*---------------------------------------------------------------------------*/
/* This function resample the dData data from the vector vect to nDataNew    */
/* values. It returns the resampled vector. The result is put in the vector  */
/* outVect that must have the right size (but could have diffrent type).     */
/* If outVect is NULL, the output vector is created and named "newName" or as*/
/* the original vector is newName = NULL.                                    */
/*---------------------------------------------------------------------------*/
{int i, j, jFirst, jLast, type;
 double scale, wFirst, wLast, sum;

 if(vect == NULL)  return(NULL);
 if(nDataNew <= 0) return(NULL);
 if(vect->type == FR_VECT_8C ||
    vect->type == FR_VECT_16C) return(NULL);

             /*----------------------- create the output vector if needed ---*/

 scale = ((double) vect->nData)/nDataNew;
 if(outVect == NULL)
   {if(newName == NULL) newName = vect->name;
    outVect = FrVectNew1D(newName, -vect->type, nDataNew,
                     vect->dx[0]*scale, vect->unitX[0], vect->unitY);
    if(outVect == NULL) return(NULL);}
 if(outVect->nData != nDataNew) return(NULL);

           /*------------------------------------------- fill the data part--*/

 type = outVect->type;
 for(i=0; i<nDataNew; i++)
   {jFirst = scale*i;
    jLast  = scale*(i+1);
    if(jLast >= vect->nData) jLast = vect->nData-1;

    wFirst = jFirst + 1 - scale*i;
    wLast  = scale*(i+1) - jLast;
    if(jFirst == jLast) wLast -= 1.;
 
    sum  = wFirst * FrVectGetValueI(vect, jFirst);
    sum += wLast  * FrVectGetValueI(vect, jLast);
    for(j=jFirst+1; j< jLast; j++) {sum +=FrVectGetValueI(vect, j);}

    if     (type == FR_VECT_C)   {outVect->data [i] = sum/scale;}
    else if(type == FR_VECT_2S)  {outVect->dataS[i] = sum/scale;}
    else if(type == FR_VECT_8R)  {outVect->dataD[i] = sum/scale;}
    else if(type == FR_VECT_4R)  {outVect->dataF[i] = sum/scale;}
    else if(type == FR_VECT_4S)  {outVect->dataI[i] = sum/scale;}
    else if(type == FR_VECT_8S)  {outVect->dataL[i] = sum/scale;}
    else if(type == FR_VECT_2U)  {outVect->dataS[i] = sum/scale;}
    else if(type == FR_VECT_4U)  {outVect->dataI[i] = sum/scale;}
    else if(type == FR_VECT_8U)  {outVect->dataL[i] = sum/scale;}
    else if(type == FR_VECT_1U)  {outVect->data [i] = sum/scale;}}
            
 return(outVect);}
/*---------------------------------------------------------FrVectReshapeAdd--*/
int FrVectReshapeAdd(FrVect *vect, 
                     FrVect *add)
/*---------------------------------------------------------------------------*/
{long nData, iVect, i;
 double rounding = 1.e-6;            /*----- used to avoid rounding problems */

 if(vect       == NULL) return(1);
 if(vect->next == NULL) return(2);

 if(add  == NULL) return(3);
 if(add->compress != 0) FrVectExpand(add);
 if(add->compress != 0)           return(4);
 if(add->wSize    != vect->wSize) return(5);
 if(add->dx[0]    != vect->dx[0]) return(6);

 iVect = (add->GTime - vect->GTime + rounding)/vect->dx[0];
 if(iVect < 0 || iVect > vect->nData) 
   {printf("FrVectReshapeAdd: Boundary Error, iVect=%ld nData=%"FRLLD"\n"  
             " time:Vect=%f add=%f\n",
        iVect, vect->nData, vect->GTime, add->GTime);
    return(7);}

 nData = add->nData;
 if(iVect+nData > vect->nData) nData = vect->nData - iVect;

 memcpy(vect->data + iVect*vect->wSize, add->data, vect->wSize*nData);
 for(i=iVect; i<iVect+nData; i++) 
   {++(vect->next->data[i]);}

 return(0);}

/*---------------------------------------------------------FrVectReshapeEnd--*/
void FrVectReshapeEnd(FrVect *vect)
/*---------------------------------------------------------------------------*/
{FRULONG i, nValues, type;
 double sum;
 char *isThere;

 if(vect ==  NULL) return;
                             /*------ Search for drop out, if no, return-----*/

 isThere = vect->next->data;
 for(i=0; i<vect->nData; i++) 
   {if(isThere[i] != 1) break;}

 if(i == vect->nData)
   {FrVectFree(vect->next);
    vect->next = NULL;
    return;}
 
       /*--------------------------------- Fill gap with the mean value-----*/

 sum    = 0.;
 nValues = 0;
 type = vect->type;

 for(i=0; i<vect->nData; i++) 
   {if(isThere[i] == 0) continue;
    nValues ++;
    if(type == FR_VECT_C)  sum += vect->data[i];
    if(type == FR_VECT_2S) sum += vect->dataS[i];
    if(type == FR_VECT_8R) sum += vect->dataD[i];
    if(type == FR_VECT_4R) sum += vect->dataF[i];
    if(type == FR_VECT_4S) sum += vect->dataI[i];
    if(type == FR_VECT_8S) sum += vect->dataL[i];
    if(type == FR_VECT_2U) sum += vect->dataUS[i];
    if(type == FR_VECT_4U) sum += vect->dataUI[i];
    if(type == FR_VECT_8U) sum += vect->dataUL[i];
    if(type == FR_VECT_1U) sum += vect->dataU[i];}

 if(nValues > 0) sum =sum/nValues;

 for(i=0; i<vect->nData; i++) 
   {if(isThere[i] != 0) continue;
    if(type == FR_VECT_C)  vect->data[i]   = sum;
    if(type == FR_VECT_2S) vect->dataS[i]  = sum;
    if(type == FR_VECT_8R) vect->dataD[i]  = sum;
    if(type == FR_VECT_4R) vect->dataF[i]  = sum;
    if(type == FR_VECT_4S) vect->dataI[i]  = sum;
    if(type == FR_VECT_8S) vect->dataL[i]  = sum;
    if(type == FR_VECT_2U) vect->dataUS[i] = sum;
    if(type == FR_VECT_4U) vect->dataUI[i] = sum;
    if(type == FR_VECT_8U) vect->dataUL[i] = sum;
    if(type == FR_VECT_1U) vect->dataU[i]  = sum;}

 return;}

/*---------------------------------------------------------FrVectReshapeNew--*/
FrVect *FrVectReshapeNew(FrVect *vect,
                     double tStart,
                     double len)
/*---------------------------------------------------------------------------*/
{FrVect *output, *isThere;
 FRULONG nData, i;
 double rounding = 1.e-6;            /*----- used to avoid rounding problems */

           /*-------------------- create the output vectors -----------------*/

 nData  = (len+rounding)/vect->dx[0];
 output  = FrVectNew1D(vect->name,     vect->type, nData, vect->dx[0], 
                       vect->unitX[0], vect->unitY);
 if(output  == NULL) return(NULL);
 output->GTime    = tStart;
 output->localTime= vect->localTime;
 output->ULeapS   = vect->ULeapS;

 sprintf(FrBuf,"Available_data_for_%s",vect->name);
 isThere = FrVectNew1D(FrBuf,  FR_VECT_C,  nData, vect->dx[0],
                       vect->unitX[0], "1 if data is there");
 if(isThere == NULL) return(NULL);
 output->next  = isThere;
 for(i=0; i<nData; i++) {isThere->data[i] = 0.;}
 
 FrVectReshapeAdd(output, vect);

 return(output);}
/*---------------------------------------------------FrVectSetMissingValues--*/
int FrVectSetMissingValues(FrVect *vect,
                           double def)
/*---------------------------------------------------------------------------*/
/* This function set the missing values do "default" for a vector provided   */
/*  by FrFileIGetVect. It returns the number of sample set.                  */
/*-------------------------------------------------------------------------- */
{FrVect *isThere;
 int iVect, nSet;
 FRLONG i, iStart, iEnd, type;

 if(vect == NULL) return(-1);

 isThere = vect->next; 
 if(isThere == NULL) return(0);
 if(isThere->type != FR_VECT_C) return(-2);
 if(strncmp(isThere->name,"Available_data",14) != 0) return(-3);

 type   = vect->type;
 nSet   = 0;

 for(iVect=0; iVect<isThere->nData; iVect++) 
   {if(isThere->data[iVect] != 0) continue;
    iStart = FrVectGetIndex(vect, (iVect+  1.e-6)*isThere->dx[0]);
    iEnd   = FrVectGetIndex(vect, (iVect+1+1.e-6)*isThere->dx[0]);
    if(iStart < 0) iStart = 0;
    if(iEnd   < 0) iEnd   = vect->nData;
    nSet  += iEnd - iStart;
    if(type == FR_VECT_C)  for(i=iStart;i<iEnd;i++) {vect->data[i]   = def;}
    if(type == FR_VECT_2S) for(i=iStart;i<iEnd;i++) {vect->dataS[i]  = def;}
    if(type == FR_VECT_8R) for(i=iStart;i<iEnd;i++) {vect->dataD[i]  = def;}
    if(type == FR_VECT_4R) for(i=iStart;i<iEnd;i++) {vect->dataF[i]  = def;}
    if(type == FR_VECT_4S) for(i=iStart;i<iEnd;i++) {vect->dataI[i]  = def;}
    if(type == FR_VECT_8S) for(i=iStart;i<iEnd;i++) {vect->dataL[i]  = def;}
    if(type == FR_VECT_2U) for(i=iStart;i<iEnd;i++) {vect->dataUS[i] = def;}
    if(type == FR_VECT_4U) for(i=iStart;i<iEnd;i++) {vect->dataUI[i] = def;}
    if(type == FR_VECT_8U) for(i=iStart;i<iEnd;i++) {vect->dataUL[i] = def;}
    if(type == FR_VECT_1U) for(i=iStart;i<iEnd;i++) {vect->dataU[i]  = def;}}

 return(nSet);}

/*-----------------------------------------------------------FrVectSetName--*/
void FrVectSetName(FrVect *vect, char *name) 
/*--------------------------------------------------------------------------*/
{
 if(vect == NULL) return;
 if(name == NULL) return;

 if(vect->name != NULL) free(vect->name);
 FrStrCpy(&vect->name,name);

 return;}
 
/*----------------------------------------------------------FrVectSetUnitX--*/
void FrVectSetUnitX(FrVect *vect, char *unitX) 
/*--------------------------------------------------------------------------*/
{
 if(vect  == NULL) return;
 if(unitX == NULL) return;

 if(vect->unitX[0] != NULL) free(vect->unitX[0]);
 FrStrCpy(&vect->unitX[0],unitX);
  
 return;} 
/*----------------------------------------------------------FrVectSetUnitX--*/
void FrVectSetUnitX1(FrVect *vect, char *unitX) 
/*--------------------------------------------------------------------------*/
{
 if(vect  == NULL) return;
 if(unitX == NULL) return;
 if(vect->nDim < 2)return;

 if(vect->unitX[1] != NULL) free(vect->unitX[1]);
 FrStrCpy(&vect->unitX[1],unitX);
  
 return;} 
/*----------------------------------------------------------FrVectSetUnitY--*/
void FrVectSetUnitY(FrVect *vect, char *unitY) 
/*--------------------------------------------------------------------------*/
{
 if(vect  == NULL) return;
 if(unitY == NULL) return;

 if(vect->unitY != NULL) free(vect->unitY);
 FrStrCpy(&vect->unitY,unitY);
  
 return;} 

/*---------------------------------------------------------------FrVectStat--*/
char *FrVectStat(FrVect *vect)
/*---------------------------------------------------------------------------*/
{static char buf[256];
 int i, minI, maxI, valueI, pastI, deltaI, deltaMI, *dataI;
 int imax, nDataH, real, imag;
 double mean, mean2, valueD, minD, maxD, pastD, deltaD, deltaMD, *dataD;
 double amplitude;
 float *dataF;
 short *dataS;
 char *dataC;

 sprintf(buf,"cannot compute statistique");

 if(vect == NULL)      return(buf);
 if(vect->nData == 0)  return(buf);

 if(FrVectIsValid(vect) != 0) 
   {sprintf(buf,"No stat: vector contains NAN or INF data");
    return(buf);}

 mean  = 0.;
 mean2 = 0.;
 imax  = 0;
   
 if(vect->type == FR_VECT_C)
     {dataC = vect->data;
      minI    = dataC[0];
      maxI    = dataC[0];
      pastI   = dataC[0];
      deltaMI = 0.;

      for (i= 0; i<vect->nData; i++)
       {valueI = dataC[i];
        if (valueI > maxI) {imax = i;
                            maxI = valueI;}
        if (valueI < minI)  minI = valueI;
        mean  += valueI;
        mean2 += valueI*valueI;
        deltaI  = pastI - valueI;
        if(deltaI < 0) deltaI = -deltaI;
        if(deltaI > deltaMI) deltaMI = deltaI;
        pastI   = valueI;}

      mean  = mean  / (double)vect->nData;
      mean2 = mean2/vect->nData-mean*mean;
      if(mean2 < 0) mean2 = 0.;
      sprintf(buf,"min %d mean %.2f max %d (for bin %d) delta %d rms %.2f",
          minI, mean, maxI, imax, deltaMI, sqrt(mean2));}

 if(vect->type == FR_VECT_2S)
     {dataS = vect->dataS;
      minI    = dataS[0];
      maxI    = dataS[0];
      pastI   = dataS[0];
      deltaMI = 0.;

      for (i= 0; i<vect->nData; i++)
       {valueI = dataS[i];
        if (valueI > maxI) {imax = i;
                            maxI = valueI;}
        if (valueI < minI)  minI = valueI;
        mean  += valueI;
        mean2 += valueI*valueI;
        deltaI  = pastI - valueI;
        if(deltaI < 0) deltaI = -deltaI;
        if(deltaI > deltaMI) deltaMI = deltaI;
        pastI   = valueI;}

      mean  = mean  / (double)vect->nData;
      mean2 = mean2/vect->nData-mean*mean;
      if(mean2 < 0) mean2 = 0.;
      sprintf(buf,"min %d mean %.2f max %d (for bin %d) delta %d rms %.2f",
          minI, mean, maxI, imax, deltaMI, sqrt(mean2));}

 else if(vect->type == FR_VECT_4S)
     {dataI = vect->dataI;
      minI    = dataI[0];
      maxI    = dataI[0];
      pastI   = dataI[0];
      deltaMI = 0.;

      for (i= 0; i<vect->nData; i++)
       {valueI = dataI[i];
        if (valueI > maxI) {imax = i;
                            maxI = valueI;}
        if (valueI < minI)  minI = valueI;
        valueD = valueI;
        mean  += valueD;
        mean2 += valueD*valueD;
        deltaI  = pastI - valueI;
        if(deltaI < 0) deltaI = -deltaI;
        if(deltaI > deltaMI) deltaMI = deltaI;
        pastI   = valueI;}

      mean  = mean  / (double)vect->nData;
      mean2 = mean2/vect->nData-mean*mean;
      if(mean2 < 0) mean2 = 0.;
      sprintf(buf,"min %d mean %.2f max %d (for bin %d) delta %d rms %.2f",
          minI, mean, maxI, imax, deltaMI, sqrt(mean2));}

 else if(vect->type == FR_VECT_4R)
     {dataF = vect->dataF;
      minD    = dataF[0];
      maxD    = dataF[0];
      pastD   = dataF[0];
      deltaMD = 0.;

      for (i= 0; i<vect->nData; i++)
       {valueD = dataF[i];
        if (valueD > maxD) {imax = i;
                            maxD = valueD;}
        if (valueD < minD)  minD = valueD;
        mean  += valueD;
        mean2 += valueD*valueD;
        deltaD  = pastD - valueD;
        if(deltaD < 0) deltaD = -deltaD;
        if(deltaD > deltaMD) deltaMD = deltaD;
        pastD   = valueD;}

      mean  = mean  / (double)vect->nData;
      mean2 = mean2/vect->nData-mean*mean;
      if(mean2 < 0) mean2 = 0.;
      sprintf(buf,"min %g mean %g max %g (for bin %d) delta %g rms %g",
          minD, mean, maxD, imax, deltaMD, sqrt(mean2));}

 else if(vect->type == FR_VECT_8R)
     {dataD = vect->dataD;
      minD    = dataD[0];
      maxD    = dataD[0];
      pastD   = dataD[0];
      deltaMD = 0.;

      for (i= 0; i<vect->nData; i++)
       {valueD = dataD[i];
        if (valueD > maxD) {imax = i;
                            maxD = valueD;}
        if (valueD < minD)  minD = valueD;
        mean  += valueD;
        if((valueD > -1.e+100) && 
           (valueD <  1.e+100)) mean2 += valueD*valueD;
        deltaD  = pastD - valueD;
        if(deltaD < 0) deltaD = -deltaD;
        if(deltaD > deltaMD) deltaMD = deltaD;
        pastD   = valueD;}

      mean  = mean  / (double)vect->nData;
      mean2 = mean2/vect->nData-mean*mean;
      if(mean2 < 0) mean2 = 0.;
      sprintf(buf,"min %g mean %g max %g (for bin %d) rms %g",
          minD, mean, maxD, imax, sqrt(mean2));}

 else if(vect->type == FR_VECT_8C)
     {dataF  = vect->dataF;
      minD = 1.e38;
      maxD = 0.;

      for (i=0; i<vect->nData; i++) 
       {real = 2*i;
        imag = real+1;
        amplitude =  sqrt(dataF[real]*dataF[real] + dataF[imag]*dataF[imag]);
        mean += amplitude;
        if(amplitude < minD) minD = amplitude;
        if(amplitude > maxD)
	  {maxD = amplitude;
           imax = i;}}

      mean = mean/vect->nData;
      sprintf(buf,"amplitude: min=%g mean=%g max=%g (%g,%g) for bin %d",
          minD, mean, maxD,dataF[2*imax],dataF[2*imax+1],imax);}

 else if(vect->type == FR_VECT_16C)
     {dataD  = vect->dataD;
      minD = 1.e38;
      maxD = 0.;

      for (i=0; i<vect->nData; i++) 
       {real = 2*i;
        imag = real+1;
        amplitude =  sqrt(dataD[real]*dataD[real] + dataD[imag]*dataD[imag]);
        mean += amplitude;
        if(amplitude < minD) minD = amplitude;
        if(amplitude > maxD)
	  {maxD = amplitude;
           imax = i;}}

      mean = mean/vect->nData;
      sprintf(buf,"amplitude: min=%g mean=%g max=%g (%g,%g) for bin %d",
          minD, mean, maxD,dataD[2*imax],dataD[2*imax+1],imax);}

 else if(vect->type == FR_VECT_8H)
     {nDataH = (vect->nData+1)/2;
      dataF  = vect->dataF;
      minD = 1.e38;
      maxD = 0.;
      imax = 0;

      for (i=1; i<nDataH; i++) 
       {real = i;
        imag = vect->nData-i;
        amplitude =  sqrt(dataF[real]*dataF[real] + dataF[imag]*dataF[imag]);
        mean += amplitude;
        if(amplitude < minD) minD = amplitude;
        if(amplitude > maxD)
	  {maxD = amplitude;
           imax = i;}}

      if(nDataH > 1) mean = mean/(nDataH-1);
      sprintf(buf,"amplitude: min=%g mean=%g max=%g (%g,%g) for bin %d",
          minD, mean, maxD,dataF[imax],dataF[vect->nData-imax],imax);}

 else if(vect->type == FR_VECT_16H)
     {nDataH = (vect->nData+1)/2;
      dataD  = vect->dataD;
      minD = 1.e38;
      maxD = 0.;

      for (i=1; i<nDataH; i++) 
       {real = i;
        imag = vect->nData-i;
        amplitude =  sqrt(dataD[real]*dataD[real] + dataD[imag]*dataD[imag]);
        mean += amplitude;
        if(amplitude < minD) minD = amplitude;
        if(amplitude > maxD)
	  {maxD = amplitude;
           imax = i;}}

      if(nDataH > 1) mean = mean/(nDataH-1);
      sprintf(buf,"amplitude: min=%g mean=%g max=%g (%g,%g) for bin %d",
          minD, mean, maxD,dataD[imax],dataD[vect->nData-imax],imax);}

 else{return(buf);}

 return(buf);
}
/*-------------------------------------------------------------FrVectToAudio-*/
void FrVectToAudio(FrVect* vect, char *fileName, char *option)
/*---------------------------------------------------------------------------*/
 {int i;
  int header[7], headerBE[7];
  FILE *fp;
  unsigned char *h1, *h2, tmp;
  char *fullName;
  double scale, min, max, sampleRate;
  FrVect* vectRescaled;

  if(vect     == NULL) return;
  if(fileName == NULL) return;

  /*---- first rescale the vector to put in in the 16 bit dyna,ic */

  FrVectMinMax(vect, &min, &max);
  min = fabs(min);
  max = fabs(max);
  if(min > max) max = min;
  if(max > 0) scale =32000./max;
  else        scale = 0.;
  vectRescaled = FrVectCopyToS(vect, scale, NULL);
 
  /*----- build header ------*/

  if(vect->dx[0] < 1) sampleRate = 1./vect->dx[0];
  else sampleRate = 200000;

  header[0] = 0x2e736E64;     /* audio file magic number */
  header[1] = 28;             /* header size in bytes    */
  header[2] = vect->nData*2;  /* length of data */
  header[3] = 3;              /* for 16 bits PCM encoding format */
  header[4] = sampleRate;     /* samples per second */
  header[5] = 1;              /* number of interleaved channels */
  header[6] = 0;              /* ?? */

  /*--------- swap bytes if needed ------*/

  h1 = (unsigned char*) &(header[5]);
  if(h1[0] == 1)
   {for(i=0; i<7; i++)
     {h1 = ((unsigned char *)header)   + i*4;
      h2 = ((unsigned char *)headerBE) + i*4;
      h2[0] = h1[3];
      h2[1] = h1[2];
      h2[2] = h1[1];
      h2[3] = h1[0];}

     for (i=0;i<vect->nData; i++) 
      {h1 = ((unsigned char*)vectRescaled->data)+i*2;
       h2 = h1;
       tmp = h1[0];
       h1[0] = h2[1];
       h1[1] = tmp;}}

       /*------------------------ write file ----*/

  fullName = malloc(strlen(fileName)+4);
  sprintf(fullName,"%s.au",fileName);
  fp = fopen(fullName,"w");
  fwrite(headerBE,7,4,fp);
  fwrite(vectRescaled->data,vect->nData,2,fp);
  fclose(fp);

  FrVectFree(vectRescaled);

  return;
}

/*--------------------------------------------------------------FrVectWrite--*/
void FrVectWrite(FrVect *v, 
                 FrFile *oFile)
/*---------------------------------------------------------------------------*/
{unsigned short compress;
 unsigned char *data;
 FRULONG nBytes, i;
 short ref;
 char *swap;

 if(v == NULL) return;

               /*------------------ change the type for hermitian vectors ---*/

 if(oFile->convertH == FR_YES)
  {if((v->type == FR_VECT_8H) || 
      (v->type == FR_VECT_16H)) FrVectHtoC(v);}

               /*----- compress only if the vector is not properly compress -*/
 compress = 0;
 if((oFile->compress != v->compress) &&
    (oFile->compress != -1) && (v->type != FR_VECT_STRING)) 
     {compress = FrVectCompData(v, oFile->compress, 
                                   oFile->gzipLevel,&data, &nBytes);}
 if(compress == 0)
    {compress = v->compress;
     data     = v->dataU;
     nBytes   = v->nBytes;}

 if(compress == 0)
    {swap = (char *)&ref;
     ref = 0x1234;
     if(swap[0] != 0x12) compress = 256;}

 if( v->type == FR_VECT_STRING)
   {nBytes = 0;
    for(i=0; i<v->nData; i++) 
       {nBytes += 3;
        if(v->dataQ[i] != NULL) nBytes += strlen(v->dataQ[i]);}}

 FrPutNewRecord(oFile, v, FR_YES);
 FrPutSChar (oFile, v->name);
 FrPutShortU(oFile, compress);
 FrPutShortU(oFile, v->type);
 FrPutLong  (oFile, v->nData);
 FrPutLong  (oFile, nBytes);
 if(compress != 0 && compress != 256)             
     {FrPutVC (oFile, (char *)data, nBytes);
      if(data != v->dataU) free(data);}
 else if(v->type == FR_VECT_C)     {FrPutVC (oFile, v->data,  v->nData);}
 else if(v->type == FR_VECT_2S)    {FrPutVS (oFile, v->dataS, v->nData);}
 else if(v->type == FR_VECT_8R)    {FrPutVD (oFile, v->dataD, v->nData);}
 else if(v->type == FR_VECT_4R)    {FrPutVF (oFile, v->dataF, v->nData);}
 else if(v->type == FR_VECT_4S)    {FrPutVI (oFile, v->dataI, v->nData);}
 else if(v->type == FR_VECT_8S)    {FrPutVL (oFile, v->dataL, v->nData);}
 else if(v->type == FR_VECT_8C)    {FrPutVF (oFile, v->dataF, v->nData*2);}
 else if(v->type == FR_VECT_16C)   {FrPutVD (oFile, v->dataD, v->nData*2);}
 else if(v->type == FR_VECT_8H)    {FrPutVF (oFile, v->dataF, v->nData);}
 else if(v->type == FR_VECT_16H)   {FrPutVD (oFile, v->dataD, v->nData);}
 else if(v->type == FR_VECT_STRING){FrPutVQ (oFile, v->dataQ, v->nData);}
 else if(v->type == FR_VECT_2U)    {FrPutVS (oFile, v->dataS, v->nData);}
 else if(v->type == FR_VECT_4U)    {FrPutVI (oFile, v->dataI, v->nData);}
 else if(v->type == FR_VECT_8U)    {FrPutVL (oFile, v->dataL, v->nData);}
 else if(v->type == FR_VECT_1U)    {FrPutVC (oFile, v->data,  v->nData);}
 else     {oFile->error = FR_ERROR_WRITE_ERROR;
           return;}
 FrPutIntU(oFile, v->nDim);
 FrPutVL (oFile,(FRLONG*) v->nx,     v->nDim);
 FrPutVD (oFile,          v->dx,     v->nDim);
 FrPutVD (oFile,          v->startX, v->nDim);
 FrPutVQ (oFile,          v->unitX,  v->nDim);
 FrPutSChar(oFile,  v->unitY);
 FrPutStruct(oFile, v->next);
 FrPutWriteRecord(oFile, FR_NO); 
	
 if(v->next != NULL) FrVectWrite(v->next, oFile);

 return;}

/*--------------------------------------------------------------FrVectFComp--*/
int FrVectFComp(short *out, 
                FRULONG *compL,
                float *data,
                FRULONG nData,
                int bSize)
/*---------------------------------------------------------------------------*/
{float *outF, delta, deltaMax, deltaMin, resolution, scale, halfScale,
   factor, rebuild, new;
 float resol[] = {1.,1.,2.,4.,6.5,14.5,30.5,62.,126.,254.,510.,1022.,2046.,
                   4094.,8190.,16382.,32766.,65532.};
 FRLONG i, step;
                       /*-------------------------- store the bloc size -----*/
 outF = (float *) out;
 outF[0] = data[0];
                      /*----------------------- first compute the step size--*/

 deltaMax = 0.;
 deltaMin = 0.;
 for(i=0; i<nData-1; i++)  
    {delta = data[i+1] - data[i];
     if(delta < deltaMin) deltaMin = delta;
     if(delta > deltaMax) deltaMax = delta;}
 if(-deltaMin > deltaMax) deltaMax = -deltaMin;
 if(deltaMax == 0.) 
   {outF[1] = 0;
    *compL = 8;
    return(0);}

 resolution = resol[bSize];
 factor = resolution/deltaMax;
 scale  = 1./factor;
 halfScale = .5*scale;
 outF[1] = scale; 

                     /*------------------------------- then store the data---*/
 rebuild   = data[0];

 for(i=1; i<nData; i++)
   {step = factor*(data[i] - data[i-1]);
    new = scale*step + rebuild;
    if(new > data[i])
         {step -= (int)(factor*( halfScale + new - data[i]));}
    else {step -= (int)(factor*(-halfScale + new - data[i]));}
    rebuild += scale*step;
    out[i+4] = step;}

out[4]  = bSize;

 *compL = 8 + 2*nData;
   
return(0);}

/*------------------------------------------------------------FrVectFExpand--*/
void FrVectFExpand(float *out, 
                   short  *data, 
                   FRULONG nData)
/*---------------------------------------------------------------------------*/
{FRLONG  i;
 float *dataF, scale;

 dataF = (float *) data;
 out[0] = dataF[0];
 scale  = dataF[1];

 if(scale == 0.)
      {for(i=1; i<nData; i++) {out[i] = 0.;}}
 else {for(i=1; i<nData; i++) {out[i] = out[i-1] + scale*data[4+i];}}

return;}

#if !defined(FR_USER_COMPRESSION)
/*---------------------------------------------------------------------------*/
/* The next two function are designec to provide a hook for user 
   defined compression algorithms. Just recompile the FrameLib with the
   flag FR_USER_COMPRESSION and provide the two following functions          */
/*-------------------------------------------------------------FrVectUComp---*/
int FrVectUComp(FrVect *vect,
                unsigned char *out, 
                FRULONG *compL,
                int  *compType)
/*---------------------------------------------------------------------------*/
{   
return(0);}

/*------------------------------------------------------------FrVectUExpand--*/
void FrVectUExpand(FrVect *vect,
                   unsigned char *out)
/*---------------------------------------------------------------------------*/
{
return;}
#endif

/*--------------------------------------------------------------FrVectZComp--*/
int FrVectZComp(unsigned short *out, 
                FRULONG *compL,
                short *data,
                FRULONG nData,
                int bSize)
/*---------------------------------------------------------------------------*/
{unsigned short nBits, uData, limit;
 short max, pOut;
 int  iIn, iOut, i, maxOut;
 short wMax[] = {0,0,1,3,7,0xf,0x1f,0x3f,0x7f,0xff,0x1ff,
                   0x3ff,0x7ff,0xfff,0x1fff,0x3fff,0x7fff};

  maxOut = *compL/2;
                        /*------------------------- store the bloc size -----*/
 out[0] = bSize;
 iOut = 0;
 pOut = 16;
                        /*--------------------------------- store the data --*/
 iIn = 0;
 while(iIn<nData)
                         /*------------- tune the size of the last bloc -----*/

    {if(iIn + bSize > nData) {bSize = nData - iIn;}

                         /*--------- get the maximum amplitude --------------*/
      
     max = 0;
     for(i=0; i<bSize; i++)  
       {if(data[iIn+i] == -32768) return(-1);/*the positive value is outside
                                             the 16 bits integer range ------*/
           if(data[iIn+i] > 0)
                {max = max |  data[iIn + i];}
           else {max = max | -data[iIn + i];}}

     /*----- determine the number of bits needed -( (2*(max-1)  < 2**nBits)) */

     if(max > 127) 
       {if(max > 2047) 
          {if(max > 8191)
               {if(max > 16383) {nBits = 16;}
                else            {nBits = 15;}}
           else{if(max > 4095)  {nBits = 14;}
                else            {nBits = 13;}}}
        else
          {if(max > 511)
               {if(max > 1023)  {nBits = 12;}
                else            {nBits = 11;}}
           else{if(max > 255)   {nBits = 10;}
                else            {nBits = 9;}}}}
     else
       {if(max > 7) 
          {if(max > 31)
               {if(max > 63)    {nBits = 8;}
                else            {nBits = 7;}}
           else{if(max > 15)    {nBits = 6;}
                else            {nBits = 5;}}}
        else
          {if(max > 1)
               {if(max > 3)     {nBits = 4;}
                else            {nBits = 3;}}
           else{if(max > 0)     {nBits = 2;}
                else            {nBits = 1;}}}}

                /*---- encode the data size - we store nBits - 1 in 4 bits---*/

     out[iOut] = out[iOut] | ((nBits-1) << pOut);
     if(pOut > 12)
        {iOut ++;
         if(iOut >= maxOut) return(-1);
        pOut = pOut - 16;
        out[iOut] = (nBits-1) >> - pOut;}
     pOut  = pOut +4;

                     /*----------------------- encode the data itself ------*/
     if(nBits > 1)
       {limit = 16 - nBits;
        for(i = 0; i<bSize; i++)
          {uData = (unsigned short) (data[iIn+i] + wMax[nBits]);

           out[iOut] = out[iOut] | (uData << pOut);
           if(pOut > limit)
             {iOut ++;
              if(iOut >= maxOut) return(-1);
              pOut = pOut - 16;
              out[iOut] = uData >> -pOut;}
           pOut  = pOut + nBits;
        }  } 
                     /*----------------------------- increase pointer -------*/
  
     iIn = iIn + bSize;}

  *compL = 2*(iOut+1);

return(0);}

/*-------------------------------------------------------------FrVectZCompI--*/
int FrVectZCompI(unsigned int *out, 
                FRULONG *compL,
                int *data,
                FRULONG nData,
                int bSize)
/*---------------------------------------------------------------------------*/
{unsigned int nBits, uData, limit;
 unsigned int max, pOut;
 int  iIn, iOut, i, maxOut;
 int wMax[] = {0,0,1,3,7,
                 0xf      ,0x1f      ,0x3f      ,0x7f,
                 0xff     ,0x1ff     ,0x3ff     ,0x7ff,
                 0xfff    ,0x1fff    ,0x3fff    ,0x7fff,
                 0xffff   ,0x1ffff   ,0x3ffff   ,0x7ffff,
                 0xfffff  ,0x1fffff  ,0x3fffff  ,0x7fffff,
                 0xffffff ,0x1ffffff ,0x3ffffff ,0x7ffffff,
                 0xfffffff,0x1fffffff,0x3fffffff,0x7fffffff};

  maxOut = *compL/4;
                          /*----------------------------store the bloc size--*/
 out[0] = bSize;
 iOut = 0;
 pOut = 16;
                          /*------------------------------- store the data --*/
 iIn = 0;
 while(iIn<nData)
                          /*------------ tune the size of the last bloc -----*/

    {if(iIn + bSize > nData) {bSize = nData - iIn;}

                          /*-------- get the maximum amplitude --------------*/
     max = 0;
     
     for(i=0; i<bSize; i++)  
       {if(data[iIn+i]==0x80000000) return(-1);/*the positive value is outside
                                             the 32 bits integer range ------*/
           if(data[iIn+i] > 0)
                {max = max |  data[iIn + i];}
           else {max = max | -data[iIn + i];}}

     /*----- determine the number of bits needed - (2*(max-1)  < 2**nBits)   */
     
     if(max > 0x7fff)
       {if(max > 0x7fffff) 
          {if(max > 0x7ffffff) 
             {if(max > 0x1fffffff)
                  {if(max > 0x3fffffff) {nBits = 32;}
                   else                 {nBits = 31;}}
              else{if(max > 0xfffffff)  {nBits = 30;}
                   else                 {nBits = 29;}}}
           else
             {if(max > 0x1ffffff)
                  {if(max > 0x3ffffff)  {nBits = 28;}
                   else                 {nBits = 27;}}
              else{if(max > 0xffffff)   {nBits = 26;}
                   else                 {nBits = 25;}}}}
        else
          {if(max > 0x7ffff) 
             {if(max > 0x1fffff)
                  {if(max > 0x3fffff)   {nBits = 24;}
                   else                 {nBits = 23;}}
              else{if(max > 0xfffff)    {nBits = 22;}
                   else                 {nBits = 21;}}}
           else
             {if(max > 0x1ffff)
                  {if(max > 0x3ffff)    {nBits = 20;}
                   else                 {nBits = 19;}}
              else{if(max > 0xffff)     {nBits = 18;}
 	           else                 {nBits = 17;}}}}}
     else
       {if(max > 0x7f) 
          {if(max > 0x7ff) 
             {if(max > 0x1fff)
                  {if(max > 0x3fff) {nBits = 16;}
                   else             {nBits = 15;}}
              else{if(max > 0xfff)  {nBits = 14;}
                   else             {nBits = 13;}}}
           else
             {if(max > 0x1ff)
                  {if(max > 0x3ff)  {nBits = 12;}
                   else             {nBits = 11;}}
              else{if(max > 0xff)   {nBits = 10;}
                   else             {nBits = 9;}}}}
        else
          {if(max > 0x7) 
             {if(max > 0x1f)
                  {if(max > 0x3f)   {nBits = 8;}
                   else             {nBits = 7;}}
              else{if(max > 0xf)    {nBits = 6;}
                   else             {nBits = 5;}}}
           else
             {if(max > 1)
                  {if(max > 3)    {nBits = 4;}
                   else           {nBits = 3;}}
              else{if(max > 0)    {nBits = 2;}
 	           else           {nBits = 1;}}}}}
     
             /*---- encode the data size ---- we store nBits - 1 in 5 bits---*/

     if(pOut != 32) out[iOut] = out[iOut] | ((nBits-1) << pOut);
     if(pOut > 27)
        {iOut ++;
         if(iOut >= maxOut) return(-1);
        pOut = pOut - 32;
        out[iOut] = (nBits-1) >> - pOut;}
     pOut  = pOut + 5;
                                    /*--------- encode the data itself ------*/
     if(nBits > 1)
       {limit = 32 - nBits;
        for(i = 0; i<bSize; i++)
          {uData = (unsigned int) (data[iIn+i] + wMax[nBits]);

           if(pOut != 32) out[iOut] = out[iOut] | (uData << pOut);
           if(pOut > limit)
             {iOut ++;
              if(iOut >= maxOut) return(-1);
              pOut = pOut - 32;
              out[iOut] = uData >> -pOut;
            }
           pOut  = pOut + nBits;
        }  } 
                         /*------------------------- increase pointer -------*/
  
     iIn = iIn + bSize;}

  *compL = 4*(iOut+1);

return(0);}

/*------------------------------------------------------------FrVectZExpand--*/
void FrVectZExpand(short *out, 
                   unsigned short *data, 
                   FRULONG nData)
/*---------------------------------------------------------------------------*/
{unsigned short nBits, pIn, uData, *buf, *alpha;
 unsigned short wMax[] = {0,0,1,3,7,0xf,0x1f,0x3f,0x7f,0xff,0x1ff,
                      0x3ff,0x7ff,0xfff,0x1fff,0x3fff,0x7fff};
 unsigned short mask[] = {0,1,3,7,0xf,0x1f,0x3f,0x7f,0xff,0x1ff,
                      0x3ff,0x7ff,0xfff,0x1fff,0x3fff,0x7fff,0xffff};
 unsigned int iBuf, iAlpha;
 int  i, iIn, iOut, bSize;

                            /*------------------- check which indian --------*/
 iAlpha = 0x12345678;
 alpha = (unsigned short *) &iAlpha;

                            /*------------------ retrieve the bloc size -----*/
 bSize = data[0];
 iIn = 1;
 pIn = 0;
                            /*------------- retrieve the data  --------------*/

 buf = (unsigned short *) &iBuf; 
 iOut = 0;
 do
    {/*-------- extract nBits ---(we check if data is in 1 or 2 words) ------*/
        
    if(pIn <= 12)
       {uData = data[iIn] >> pIn;
        pIn = pIn + 4;}
     else
       {uData = (data[iIn] >> pIn) | (data[iIn+1] << (16-pIn) ) ;
        iIn++;
	pIn = pIn - 12;}

    nBits = 1 + (mask[4] & uData);
    if(nBits == 1) nBits = 0;

                        /*----------------------------extract data ----------*/

     for(i=0; i<bSize; i++)
       {if(iOut >= nData) break;

        if(pIn + nBits <= 16)
          {uData = data[iIn] >> pIn;
           pIn = pIn + nBits;}
        else
          {uData = (data[iIn] >> pIn) | (data[iIn+1] << (16-pIn) ) ;
           iIn++;
	   pIn = pIn + nBits - 16;}

        out[iOut] = (mask[nBits] & uData) - wMax[nBits];
        iOut++;}}
 while(iOut<nData);

return;}

/*-----------------------------------------------------------FrVectZExpandI--*/
void FrVectZExpandI(int *out, 
                    unsigned int *data, 
                    FRULONG nData)
/*---------------------------------------------------------------------------*/
{unsigned short *alpha;
 unsigned int nBits, pIn, uData, *buf;
 unsigned int wMax[] = {0,0,1,3,7,
                 0xf      ,0x1f      ,0x3f      ,0x7f,
                 0xff     ,0x1ff     ,0x3ff     ,0x7ff,
                 0xfff    ,0x1fff    ,0x3fff    ,0x7fff,
                 0xffff   ,0x1ffff   ,0x3ffff   ,0x7ffff,
                 0xfffff  ,0x1fffff  ,0x3fffff  ,0x7fffff,
                 0xffffff ,0x1ffffff ,0x3ffffff ,0x7ffffff,
                 0xfffffff,0x1fffffff,0x3fffffff,0x7fffffff};
 unsigned int mask[] = {0,1,3,      7,       0xf,
              0x1f      ,0x3f      ,0x7f,      0xff,
              0x1ff     ,0x3ff     ,0x7ff,     0xfff,
              0x1fff    ,0x3fff    ,0x7fff,    0xffff,
              0x1ffff   ,0x3ffff   ,0x7ffff,   0xfffff,
              0x1fffff  ,0x3fffff  ,0x7fffff,  0xffffff,
              0x1ffffff ,0x3ffffff ,0x7ffffff, 0xfffffff,
              0x1fffffff,0x3fffffff,0x7fffffff,0xffffffff};
unsigned int iBuf, iAlpha;
 int  i, iIn, iOut, bSize;
                              /*----------------- check which indian --------*/
 iAlpha = 0x12345678;
 alpha = (unsigned short *) &iAlpha;

                              /*---------------- retrieve the bloc size -----*/
 bSize = data[0] & 0xffff;
 iIn = 0;
 pIn = 16;
                              /*------------------ retrieve the data---------*/

 buf = (unsigned int *) &iBuf; 
 iOut = 0;
 do
    {  /*-------- extract nBits -(we check if data is in 1 or 2 words) ------*/
        
    if(pIn <= 27)
       {uData = data[iIn] >> pIn;
        pIn = pIn + 5;}
     else
       {uData = (data[iIn] >> pIn) & mask[32-pIn];
        iIn++;
        uData += data[iIn] << (32-pIn);
        pIn = pIn - 27;}

    nBits = 1 + (0x1f & uData);
    if(nBits == 1) nBits = 0;

                              /*----------------------extract data ----------*/
     for(i=0; i<bSize; i++)
       {if(iOut >= nData) break;

        if(pIn + nBits <= 32)
          {uData = data[iIn] >> pIn;
           pIn = pIn + nBits;}
        else
          {uData = (data[iIn] >> pIn) & mask[32-pIn];
           iIn++;
           uData += data[iIn] << (32-pIn);
           pIn = pIn + nBits - 32;}

        out[iOut] = (mask[nBits] & uData) - wMax[nBits];
        iOut++;}}
 while(iOut<nData);

return;}
/*-----------------------------------------------------------FrVectZExpandL--*/
void FrVectZExpandL(long long *out, 
                    unsigned long long *data, 
                    FRULONG nData)
/*---------------------------------------------------------------------------*/
{
  unsigned long long nBits, pIn, uData, *buf, iBuf, i, iIn, iOut, bSize;
  static unsigned long long wMax[65],  mask[65], iFirst = 0;
          
  /*-----------------------------------------------------initialize arrays---*/
  if(iFirst == 0) {
    iFirst = 1;
    mask[0] = 0;
    mask[1] = 1;
    wMax[0] = 0;
    wMax[1] = 0;
    for(i=2; i< 65; i++) {
      mask[i] = (mask[i-1]<< 1) + 1;
      wMax[i] = (wMax[i-1]<< 1) + 1;}}
 
  /*-------------------------------------------- retrieve the bloc size -----*/
  bSize = data[0] & 0xffff;
  iIn = 0;
  pIn = 16;

  /*----------------------------------------------------retrieve the data---*/
  buf = (unsigned long long *) &iBuf; 
  iOut = 0;
  do
    {  /*-------- extract nBits -(we check if data is in 1 or 2 words) ------*/
        
      if(pIn <= (64-6))
       {uData = data[iIn] >> pIn;
        pIn = pIn + 6;}
     else
       {uData = (data[iIn] >> pIn) & mask[64-pIn];
        iIn++;
        uData += data[iIn] << (64-pIn);
        pIn = pIn - (64-6);}

    nBits = 1 + (0x3f & uData);
    if(nBits == 1) nBits = 0;
                               /*----------------------extract data ----------*/
     for(i=0; i<bSize; i++)
       {if(iOut >= nData) break;

        if(pIn + nBits <= 64)
          {uData = data[iIn] >> pIn;
           pIn = pIn + nBits;}
        else
          {uData = (data[iIn] >> pIn) & mask[64-pIn];
           iIn++;
           uData += data[iIn] << (64-pIn);
           pIn = pIn + nBits - 64;}

        out[iOut] = (mask[nBits] & uData) - wMax[nBits];
       iOut++;}}
 while(iOut<nData);

return;}
/*-------------------------------------------------------------FrVectZoomIn--*/
int FrVectZoomIn(FrVect *vect,
                 double xStart,
                 double length)
/*---------------------------------------------------------------------------*/
{FRLONG iFirst, nBin, err;
 double dx;

 if(vect == NULL)    return(1);
 if(vect->dx[0] == 0)          return(4);
 if(length <= 0.)              return(5);
 
               /*--- we had a little something to avoid rounding problems----*/

 dx = xStart - vect->startX[0] + 1.e-6;
 iFirst = dx/vect->dx[0];
 nBin = (length + 1.e-6)/vect->dx[0];

 if(nBin == 0) { /*-------this is the case with length smaller than a bin---*/
   if(((iFirst+1)*length - dx) > xStart) nBin = 1;}
 
 err = FrVectZoomInI(vect, iFirst, nBin);
 
 return(err);}
 /*-----------------------------------------------------------FrVectZoomInI--*/
int FrVectZoomInI(FrVect *vect,
                  FRLONG iFirst,
                  FRLONG nBin)
/*---------------------------------------------------------------------------*/
/* The zoom is performed on the first dimension                              */
{
  FRLONG wSize;

 if(vect == NULL)    return(1);
 if(vect->compress != 0) FrVectExpand(vect);
 if(vect->compress != 0)       return(3);
 if(vect->dx[0] == 0)          return(4);
 if(vect->type == FR_VECT_8H)  return(6);
 if(vect->type == FR_VECT_16H) return(7); 

 /*--------------- zoom the auxiliary vector if present---*/
 if(vect->next != NULL) {
   FrVectZoomInI(vect->next, 
		 (iFirst * vect->next->nData)/vect->nData,
		 (nBin   * vect->next->nData)/vect->nData);}

   /*------------------- zoom the vector itself---*/
 if(vect->dataUnzoomed == NULL)
   {vect->dataUnzoomed  = vect->data;
    vect->nDataUnzoomed = vect->nData;
    vect->startXUnzoomed = vect->startX[0];}
 
 if(iFirst < 0) iFirst = 0;
 if(iFirst > vect->nData-1) iFirst = vect->nData-1;

 if(nBin+iFirst > vect->nData) nBin = vect->nData-iFirst;

 vect->nx[0] = nBin;
 vect->startX[0] += iFirst*vect->dx[0];

 vect->nData = nBin;
 wSize = vect->wSize;
 if(vect->nDim > 1) {
   wSize       *= vect->nx[1]; /*-- to handle time-frequency or images/movies*/
   vect->nData *= vect->nx[1];}
 if(vect->nDim > 2) {
   wSize       *= vect->nx[2];
   vect->nData *= vect->nx[2];}

 vect->data  = vect->data + iFirst*wSize;
 FrVectMap(vect);

 return(0);}
 
/*------------------------------------------------------------FrVectZoomOut--*/
int FrVectZoomOut(FrVect *vect)
/*---------------------------------------------------------------------------*/
{
 if(vect == NULL)               return(1);
 if(vect->dataUnzoomed == NULL) return(2);
  
 vect->nData     = vect->nDataUnzoomed;
 vect->nBytes    = vect->nData*vect->wSize;
 vect->nx[0]     = vect->nData;
 vect->startX[0] = vect->startXUnzoomed;
 vect->data      = vect->dataUnzoomed;
 vect->dataUnzoomed = NULL;

 FrVectMap(vect);

 /*----------------------- zoom out the auxiliary data availability vector---*/
 if(vect->next != NULL) FrVectZoomOut(vect->next); 

 return(0);}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
/*  Here is a set of function used for backward compatibility                */
/*    FrBackNxxx is used to read file written with format N                  */
/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
/*------------------------------------------------------FrBack3DetectorRead--*/
FrDetector *FrBack3DetectorRead(FrFile *iFile)
/*---------------------------------------------------------------------------*/
{FrDetector *detector;
 short lonD, lonM, latD, latM;
 float lonS, latS, armLength;
 
  detector = (FrDetector *) calloc(1,sizeof(FrDetector));
  if(detector == NULL) 
       {iFile->error = FR_ERROR_MALLOC_FAILED;
        return(NULL);}
  detector->classe = FrDetectorDef(); 
  
  FrReadHeader(iFile,  detector);
  FrReadSChar (iFile, &detector->name); 
  detector->prefix[0] = '*';
  detector->prefix[1] = '*';
  FrReadShort (iFile, &lonD); 
  FrReadShort (iFile, &lonM); 
  FrReadFloat (iFile, &lonS);
  FrReadShort (iFile, &latD); 
  FrReadShort (iFile, &latM); 
  FrReadFloat (iFile, &latS);
  FrReadFloat (iFile, &detector->elevation); 
  FrReadFloat (iFile, &detector->armXazimuth); 
  FrReadFloat (iFile, &detector->armYazimuth); 
  FrReadFloat (iFile, &armLength); 
  FrReadStruct(iFile, &detector->aux); 

                               /*------------ Convert coordinates-----------*/
  if(lonD >= 0) {detector->longitude = FRPI/180.*(lonD +lonM/60. +lonS/3600.);}
  else          {detector->longitude = FRPI/180.*(lonD -lonM/60. -lonS/3600.);}
  if(latD >= 0) {detector->latitude  = FRPI/180.*(latD +latM/60. +latS/3600.);}
  else          {detector->latitude  = FRPI/180.*(latD -latM/60. -latS/3600.);}

                      /*-- Convert azimuth values to clockwise-from-north --*/
  detector->armXazimuth = FRPI/2. - detector->armXazimuth;
  if (detector->armXazimuth < 0.)  {detector->armXazimuth += 2.*FRPI;}
  detector->armYazimuth = FRPI/2. - detector->armYazimuth;
  if (detector->armYazimuth < 0.)  {detector->armYazimuth += 2.*FRPI;}

  detector->armXmidpoint = .5*armLength;
  detector->armYmidpoint = .5*armLength;
  
   if(FrDebugLvl > 2) fprintf(FrFOut," %s\n", detector->name);
 
  return(detector);}
 
/*-------------------------------------------------------FrBack3FrameHRead---*/
FrameH *FrBack3FrameHRead(FrFile *iFile)
/*---------------------------------------------------------------------------*/
{FrameH *frameH;
 unsigned short id;
 int dummyStrain, localTime;

  frameH = (FrameH *) calloc(1,sizeof(FrameH));
  if(frameH == NULL) 
       {iFile->error = FR_ERROR_MALLOC_FAILED;
        return(NULL);}
  frameH->classe = FrameHDef(); 
  
  FrReadShortU(iFile, &id);            /*frameH are not referenced by pointer*/
  FrReadSChar (iFile, &frameH->name); 
  FrReadInt   (iFile, &frameH->run);
  FrReadIntU  (iFile, &frameH->frame);
  FrReadIntU  (iFile, &frameH->GTimeS);
  FrReadIntU  (iFile, &frameH->GTimeN);
  FrReadShortU(iFile, &frameH->ULeapS); /*------------only since version 3.30*/
  FrReadInt   (iFile, &localTime);
  FrReadDouble(iFile, &frameH->dt);
  FrReadStruct(iFile, &frameH->type);
  FrReadStruct(iFile, &frameH->user);
  FrReadStruct(iFile, &frameH->detectSim);
  FrReadStruct(iFile, &frameH->detectProc);
  FrReadStruct(iFile, &frameH->history);
  FrReadStruct(iFile, &frameH->rawData);
  FrReadStruct(iFile, &frameH->procData);
  FrReadInt   (iFile, &dummyStrain);
  FrReadStruct(iFile, &frameH->simData);
  FrReadStruct(iFile, &frameH->event);
  FrReadStruct(iFile, &frameH->summaryData);
  FrReadStruct(iFile, &frameH->auxData);

  if(FrDebugLvl > 0) fprintf(FrFOut, 
          "FrBack3FrameHRead:  Run:%d Frame:%d\n",frameH->run,frameH->frame);

  iFile->curFrame = frameH;
   
  return(NULL);}
/*------------------------------------------------------FrBack3ProcDataRead--*/
FrProcData *FrBack3ProcDataRead(FrFile *iFile)
/*---------------------------------------------------------------------------*/
{FrProcData *procData;
 int timeOffsetS, timeOffsetN;
 double sampleRate;

  procData = (FrProcData *) calloc(1,sizeof(FrProcData));
  if(procData == NULL)  
       {iFile->error = FR_ERROR_MALLOC_FAILED;
        return(NULL);}
  procData->classe = FrProcDataDef();
  
  FrReadHeader(iFile,  procData);
  FrReadSChar (iFile, &procData->name);
  FrReadSChar (iFile, &procData->comment);
  FrReadDouble(iFile, &sampleRate);
  FrReadInt   (iFile, &timeOffsetS);
  FrReadInt   (iFile, &timeOffsetN);
  procData->timeOffset = timeOffsetS + 1.e-9 * timeOffsetN;
  FrReadDouble(iFile, &procData->fShift);
  FrReadStruct(iFile, &procData->data);
  FrReadStruct(iFile, &procData->aux);
  FrReadStruct(iFile, &procData->next);

  if(FrDebugLvl > 2) fprintf(FrFOut," %s\n", procData->name);

  return(procData);}
/*-------------------------------------------------------FrBack3RawDataRead--*/
FrRawData *FrBack3RawDataRead(FrFile *iFile)
/*---------------------------------------------------------------------------*/
{FrRawData *rawData;

 rawData = (FrRawData *) calloc(1,sizeof(FrRawData));
 if(rawData == NULL)  
      {iFile->error = FR_ERROR_MALLOC_FAILED;
       return(NULL);}
 rawData->classe = FrRawDataDef();
  
 FrReadHeader(iFile,  rawData);
 FrReadSChar (iFile, &rawData->name); 
 FrReadStruct(iFile, &rawData->firstSer);
 FrReadStruct(iFile, &rawData->firstAdc);
 rawData->firstTable = NULL;
 FrReadStruct(iFile, &rawData->logMsg);
 FrReadStruct(iFile, &rawData->more);
  
 if(FrDebugLvl > 2) fprintf(FrFOut," %s\n", rawData->name);

 return(rawData);}

/*-------------------------------------------------------FrBack3SimDataRead--*/
FrSimData *FrBack3SimDataRead(FrFile *iFile)
/*---------------------------------------------------------------------------*/
{FrSimData *simData;
  float rate;

  simData = (FrSimData *) calloc(1,sizeof(FrSimData));
  if(simData == NULL) 
       {iFile->error = FR_ERROR_MALLOC_FAILED;
        return(NULL);}
  simData->classe = FrSimDataDef();
  
  FrReadHeader(iFile,  simData);
  FrReadSChar( iFile, &simData->name);
  FrReadSChar (iFile, &simData->comment);
  FrReadFloat (iFile, &rate);
  simData->sampleRate = rate;
  FrReadStruct(iFile, &simData->data);
  FrReadStruct(iFile, &simData->input);
  FrReadStruct(iFile, &simData->next);
    
  if(FrDebugLvl > 2) fprintf(FrFOut," %s\n", simData->name);

return(simData);}
/*------------------------------------------------------FrBack3StatDataRead--*/
FrStatData *FrBack3StatDataRead(FrFile *iFile)
/*---------------------------------------------------------------------------*/
{FrStatData *sData;
 unsigned short id;
 
  sData = (FrStatData *) calloc(1,sizeof(FrStatData));
  if(sData == NULL)  
       {iFile->error = FR_ERROR_MALLOC_FAILED;
        return(NULL);}
  sData->classe = FrStatDataDef();

  FrReadShortU(iFile, &id);  /*--------statData are not referenced by pointer*/
  FrReadSChar( iFile, &sData->name);
  FrReadSChar (iFile, &sData->comment); 
  FrReadIntU  (iFile, &sData->timeStart); 
  FrReadIntU  (iFile, &sData->timeEnd); 
  FrReadIntU  (iFile, &sData->version); 
  FrReadStruct(iFile, &sData->detector); 
  FrReadStruct(iFile, &sData->data); 

  sData->next = iFile->sDataCur;
  iFile->sDataCur = sData;

  if(FrDebugLvl > 2) fprintf(FrFOut," %s\n", sData->name);

  return(sData);}
/*-------------------------------------------------------FrBack3SummaryRead--*/
FrSummary *FrBack3SummaryRead(FrFile *iFile)
/*---------------------------------------------------------------------------*/
{FrSummary *summary;

 summary = (FrSummary *) calloc(1,sizeof(FrSummary));
 if(summary == NULL) 
      {iFile->error = FR_ERROR_MALLOC_FAILED;
       return(NULL);}
 summary->classe = FrSummaryDef();

 FrReadHeader(iFile,  summary);
 FrReadSChar (iFile, &summary->name); 
 FrReadSChar (iFile, &summary->comment);
 FrReadSChar (iFile, &summary->test);
 FrReadStruct(iFile, &summary->moments);
 FrReadStruct(iFile, &summary->next);
    
 if(FrDebugLvl > 2) fprintf(FrFOut," %s\n", summary->name);

 return(summary);}

/*---------------------------------------------------------FrBack3EventRead--*/
FrEvent *FrBack3EventRead(FrFile *iFile)
/*---------------------------------------------------------------------------*/
{FrEvent *event;

 event = (FrEvent *) calloc(1,sizeof(FrEvent));
 if(event == NULL) 
      {iFile->error = FR_ERROR_MALLOC_FAILED;
       return(NULL);}
 event->classe = FrEventDef();

 FrReadHeader(iFile,  event);
 FrReadSChar (iFile, &event->name); 
 FrReadSChar (iFile, &event->comment);
 FrReadSChar (iFile, &event->inputs);
 FrReadIntU  (iFile, &event->GTimeS);
 FrReadIntU  (iFile, &event->GTimeN);
 FrReadIntU  (iFile, &event->eventStatus);
 FrReadFloat (iFile, &event->amplitude);
 FrReadFloat (iFile, &event->probability);
 FrReadSChar (iFile, &event->statistics);
 FrReadStruct(iFile, &event->data);
 FrReadStruct(iFile, &event->next);
    
 if(FrDebugLvl > 2) fprintf(FrFOut," %s\n", event->name);

 return(event);}

/*----------------------------------------------------------FrBack3VectRead--*/
FrVect *FrBack3VectRead(FrFile *iFile)
/*---------------------------------------------------------------------------*/
{FrVect *v;
 static int first=0, wSize[FR_VECT_END];
 unsigned int i, nData, nBytes, nx;
 unsigned short type;

 if(first == 0)
    {first = 1;
     wSize[FR_VECT_4R]  = sizeof(float);
     wSize[FR_VECT_8R]  = sizeof(double);
     wSize[FR_VECT_C]   = sizeof(char);
     wSize[FR_VECT_1U]  = sizeof(char);
     wSize[FR_VECT_2S]  = sizeof(short);
     wSize[FR_VECT_2U]  = sizeof(short);
     wSize[FR_VECT_4S]  = sizeof(int);
     wSize[FR_VECT_4U]  = sizeof(int);
     wSize[FR_VECT_8S]  = sizeof(FRLONG);
     wSize[FR_VECT_8U]  = sizeof(FRLONG);
     wSize[FR_VECT_8C]  = 2*sizeof(float);
     wSize[FR_VECT_16C] = 2*sizeof(double);}
 
 v = (FrVect *) calloc(1,sizeof(FrVect));
 if(v == NULL)  
      {iFile->error = FR_ERROR_MALLOC_FAILED;
       return(NULL);}
 v->classe = FrVectDef();
 
 FrReadHeader(iFile,  v);
 FrReadSChar( iFile, &v->name);
 FrReadShortU(iFile, &v->compress);
 FrReadShortU(iFile, &type);
 FrReadIntU  (iFile, &nData);
 FrReadIntU  (iFile, &nBytes);
 v->type   = type;
 v->nData  = nData;
 v->nBytes = nBytes;
 v->space  = v->nData;

 if     (v->compress != 0)  
        {FrReadVC (iFile, &v->data, v->nBytes);}
 else if(v->type == FR_VECT_4R)  
        {FrReadVF (iFile, &v->dataF, v->nData);
	 v->data = (char *)v->dataF;}
 else if(v->type == FR_VECT_8R)  
        {FrReadVD (iFile, &v->dataD, v->nData);
	 v->data = (char *)v->dataD;}
 else if((v->type == FR_VECT_C ) || (v->type == FR_VECT_1U)) 
        {FrReadVC (iFile, &v->data, v->nData);}
 else if((v->type == FR_VECT_2S) || (v->type == FR_VECT_2U))
        {FrReadVS (iFile, &v->dataS, v->nData);
	 v->data = (char *)v->dataS;}
 else if((v->type == FR_VECT_4S) || (v->type == FR_VECT_4U))
        {FrReadVI (iFile, &v->dataI, v->nData);
	 v->data = (char *)v->dataI;}
 else if((v->type == FR_VECT_8S) || (v->type == FR_VECT_8U)) 
        {FrReadVL (iFile, &v->dataL, v->nData);
	 v->data = (char *)v->dataL;}
 else if(v->type == FR_VECT_8C) 
        {FrReadVF (iFile, &v->dataF, v->nData*2);
	 v->data = (char *)v->dataF;}
 else 
        {FrReadVD (iFile, &v->dataD, v->nData*2);
	 v->data = (char *)v->dataD;}

 if(iFile->error == FR_ERROR_MALLOC_FAILED) return(NULL);

 FrVectMap(v);

 FrReadIntU  (iFile, &v->nDim);
 v->nx = malloc(v->nDim*sizeof(FRULONG));
 for(i=0; i<v->nDim; i++)
   {FrReadIntU (iFile, &nx);
    v->nx[i] = nx;}
 FrReadVD (iFile, &v->dx,    v->nDim);
 FrReadVQ (iFile, &v->unitX, v->nDim);
 FrReadSChar (iFile, &v->unitY);
 FrReadStruct(iFile, &v->next);
 
 v->wSize= wSize[v->type];

 v->startX= malloc(v->nDim*sizeof(double));
 if(v->startX== NULL) 
       {FrError(3,"FrVectRead","malloc failed");
        return(NULL);}
 for(i=0; i<v->nDim; i++) {v->startX[i] = 0.;}

                          /*--------------- fix bug in compression flags-----*/
 if(v->compress >0xff)
   {v->compress  = 256 + (v->compress/0x100);}
 if(iFile->compress == 0 && v->compress != 0) {FrVectExpand(v);}
 
 if(FrDebugLvl > 2) fprintf(FrFOut," %s\n", v->name);

 return(NULL);}

/*---------------------------------------------------- FrBack4DetectorRead---*/
FrDetector *FrBack4DetectorRead(FrFile *iFile)
/*---------------------------------------------------------------------------*/
{FrDetector *detector;
 short lonD, lonM, latD, latM;
 float lonS, latS;

  iFile->detectorType = iFile->type;

  detector = (FrDetector *) calloc(1,sizeof(FrDetector));
  if(detector == NULL) 
       {iFile->error = FR_ERROR_MALLOC_FAILED;
        return(NULL);}
  detector->classe = FrDetectorDef(); 
  
  FrReadHeader(iFile,  detector);
  FrReadSChar (iFile, &detector->name); 
  detector->prefix[0] = '*';
  detector->prefix[1] = '*';
  FrReadShort (iFile, &lonD); 
  FrReadShort (iFile, &lonM); 
  FrReadFloat (iFile, &lonS);
  FrReadShort (iFile, &latD); 
  FrReadShort (iFile, &latM); 
  FrReadFloat (iFile, &latS);
  FrReadFloat (iFile, &detector->elevation); 
  FrReadFloat (iFile, &detector->armXazimuth); 
  FrReadFloat (iFile, &detector->armYazimuth); 
  FrReadStruct(iFile, &detector->aux); 
  FrReadStruct(iFile, &detector->table);

                               /*------------ Convert coordinates-----------*/
  if(lonD >= 0) {detector->longitude = FRPI/180.*(lonD +lonM/60. +lonS/3600.);}
  else          {detector->longitude = FRPI/180.*(lonD -lonM/60. -lonS/3600.);}
  if(latD >= 0) {detector->latitude  = FRPI/180.*(latD +latM/60. +latS/3600.);}
  else          {detector->latitude  = FRPI/180.*(latD -latM/60. -latS/3600.);}

                      /*-- Convert azimuth values to clockwise-from-north --*/
  detector->armXazimuth = FRPI/2. - detector->armXazimuth;
  if (detector->armXazimuth < 0.)  {detector->armXazimuth += 2.*FRPI;}
  detector->armYazimuth = FRPI/2. - detector->armYazimuth;
  if (detector->armYazimuth < 0.)  {detector->armYazimuth += 2.*FRPI;}

  if(FrDebugLvl > 2) fprintf(FrFOut," %s\n", detector->name);
  detector->localTime = iFile->oldLocalTime;

  return(detector);}
/*-------------------------------------------------- FrBack4EndOfFrameRead---*/
void FrBack4EndOfFrameRead(FrFile *iFile)
/*---------------------------------------------------------------------------*/
{unsigned int run,frameNumber;
 unsigned short instance;

  FrReadShortU(iFile, &instance);
  FrReadIntU(iFile, &run);
  FrReadIntU(iFile, &frameNumber);

  if(FrDebugLvl > 0) fprintf(FrFOut, " FrEndOfFrameRead: "
                     " run=%d frame=%d\n", run, frameNumber);

                         /*------ check that the frame is not corrupted -----*/

  if(run != iFile->curFrame->run)
     {FrError(3,"FrEndOfFrameRead","run number missmatch");
      iFile->error = FR_ERROR_BAD_END_OF_FRAME;
      return;}

  if(frameNumber != iFile->curFrame->frame)
     {FrError(3,"FrEndOfFrameRead","frame number missmatch");
      iFile->error = FR_ERROR_BAD_END_OF_FRAME;
      return;}

  iFile->endOfFrame = FR_YES;

  return;}
 
/*---------------------------------------------------------FrBack4EventRead--*/
FrEvent *FrBack4EventRead(FrFile *iFile)
/*---------------------------------------------------------------------------*/
{FrEvent *event;

 event = (FrEvent *) calloc(1,sizeof(FrEvent));
 if(event == NULL) 
      {iFile->error = FR_ERROR_MALLOC_FAILED;
       return(NULL);}
 event->classe = FrEventDef();

 FrReadHeader(iFile,  event);
 FrReadSChar (iFile, &event->name); 
 FrReadSChar (iFile, &event->comment);
 FrReadSChar (iFile, &event->inputs);
 FrReadIntU  (iFile, &event->GTimeS);
 FrReadIntU  (iFile, &event->GTimeN);
 FrReadFloat (iFile, &event->timeBefore);
 FrReadFloat (iFile, &event->timeAfter);
 FrReadIntU  (iFile, &event->eventStatus);
 FrReadFloat (iFile, &event->amplitude);
 FrReadFloat (iFile, &event->probability);
 FrReadSChar (iFile, &event->statistics);
 FrReadStruct(iFile, &event->data);
 FrReadStruct(iFile, &event->table);
 FrReadStruct(iFile, &event->next);
    
 if(FrDebugLvl > 2) fprintf(FrFOut," %s\n", event->name);

 return(event);}

/*--------------------------------------------------------- FrBack4MsgRead---*/
FrMsg *FrBack4MsgRead(FrFile *iFile)
/*---------------------------------------------------------------------------*/
{FrMsg *msg;

  msg = (FrMsg *) calloc(1,sizeof(FrMsg));
  if(msg == NULL)  
       {iFile->error = FR_ERROR_MALLOC_FAILED;
        return(NULL);}
  msg->classe = FrMsgDef();

  FrReadHeader(iFile,  msg);
  FrReadSChar (iFile, &msg->alarm); 
  FrReadSChar (iFile, &msg->message);
  FrReadIntU  (iFile, &msg->severity);
  FrReadStruct(iFile, &msg->next);
    
  if(FrDebugLvl > 2) fprintf(FrFOut," %s\n", msg->alarm);

return(msg);}

/*---------------------------------------------------- FrBack4ProcDataRead---*/
FrProcData *FrBack4ProcDataRead(FrFile *iFile)
/*---------------------------------------------------------------------------*/
{FrProcData *procData;
 unsigned int timeOffsetN, timeOffsetS;
 double sampleRate;

  procData = (FrProcData *) calloc(1,sizeof(FrProcData));
  if(procData == NULL)  
       {iFile->error = FR_ERROR_MALLOC_FAILED;
        return(NULL);}
  procData->classe = FrProcDataDef();
  
  FrReadHeader(iFile,  procData);
  FrReadSChar( iFile, &procData->name);
  FrReadSChar( iFile, &procData->comment);
  FrReadDouble(iFile, &sampleRate);
  FrReadIntU  (iFile, &timeOffsetS);
  FrReadIntU  (iFile, &timeOffsetN);
  procData->timeOffset = timeOffsetS + 1.e-9 * timeOffsetN;
  FrReadDouble(iFile, &procData->fShift);
  FrReadStruct(iFile, &procData->data);
  iFile->vectInstance = iFile->instance;
  FrReadStruct(iFile, &procData->aux);
  FrReadStruct(iFile, &procData->table);
  FrReadStruct(iFile, &procData->next);

  if(FrDebugLvl > 2) fprintf(FrFOut," %s\n", procData->name);

  return(procData);}

/*-------------------------------------------------------FrBack4SimDataRead--*/
FrSimData *FrBack4SimDataRead(FrFile *iFile)
/*---------------------------------------------------------------------------*/
{FrSimData *simData;
 float rate;

  simData = (FrSimData *) calloc(1,sizeof(FrSimData));
  if(simData == NULL) 
       {iFile->error = FR_ERROR_MALLOC_FAILED;
        return(NULL);}
  simData->classe = FrSimDataDef();
  
  FrReadHeader(iFile,  simData);
  FrReadSChar( iFile, &simData->name);
  FrReadSChar (iFile, &simData->comment);
  FrReadFloat (iFile, &rate);
  simData->sampleRate = rate;
  FrReadStruct(iFile, &simData->data);
  iFile->vectInstance = iFile->instance;
  FrReadStruct(iFile, &simData->input);
  FrReadStruct(iFile, &simData->table);
  FrReadStruct(iFile, &simData->next);
    
  if(FrDebugLvl > 2) fprintf(FrFOut," %s\n", simData->name);

return(simData);}
/*------------------------------------------------------FrBack4SimEventRead--*/
FrSimEvent *FrBack4SimEventRead(FrFile *iFile)
/*---------------------------------------------------------------------------*/
{FrSimEvent *simEvent;

 simEvent = (FrSimEvent *) calloc(1,sizeof(FrSimEvent));
 if(simEvent == NULL) 
      {iFile->error = FR_ERROR_MALLOC_FAILED;
       return(NULL);}
 simEvent->classe = FrSimEventDef();

 FrReadHeader(iFile,  simEvent);
 FrReadSChar (iFile, &simEvent->name); 
 FrReadSChar (iFile, &simEvent->comment);
 FrReadSChar (iFile, &simEvent->inputs);
 FrReadIntU  (iFile, &simEvent->GTimeS);
 FrReadIntU  (iFile, &simEvent->GTimeN);
 FrReadFloat (iFile, &simEvent->timeBefore);
 FrReadFloat (iFile, &simEvent->timeAfter);
 FrReadFloat (iFile, &simEvent->amplitude);
 FrReadStruct(iFile, &simEvent->data);
 FrReadStruct(iFile, &simEvent->table);
 FrReadStruct(iFile, &simEvent->next);
    
 if(FrDebugLvl > 2) fprintf(FrFOut," %s\n", simEvent->name);

 return(simEvent);}

/*-------------------------------------------------------FrBack4SummaryRead--*/
FrSummary *FrBack4SummaryRead(FrFile *iFile)
/*---------------------------------------------------------------------------*/
{FrSummary *summary;

 summary = (FrSummary *) calloc(1,sizeof(FrSummary));
 if(summary == NULL) 
      {iFile->error = FR_ERROR_MALLOC_FAILED;
       return(NULL);}
 summary->classe = FrSummaryDef();

 FrReadHeader(iFile,  summary);
 FrReadSChar (iFile, &summary->name); 
 FrReadSChar (iFile, &summary->comment);
 FrReadSChar (iFile, &summary->test);
 FrReadStruct(iFile, &summary->moments);
 FrReadStruct(iFile, &summary->table);
 FrReadStruct(iFile, &summary->next);
    
 if(FrDebugLvl > 2) fprintf(FrFOut," %s\n", summary->name);

 return(summary);}

/*--------------------------------------------------------FrBack4TOCevtRead--*/
FrTOCevt *FrBack4TOCevtRead(FrFile *iFile)
/*---------------------------------------------------------------------------*/
{FrTOCevt *root, *evt;
 unsigned int *GTimeS, *GTimeN, i, nTot;
 char **names;
 FRLONG *position;
 
 FrReadIntU(iFile, &nTot);
 if(nTot == 0xffffffff) return(NULL);
 if(nTot == 0) return (NULL);

 if(FrDebugLvl > 1) fprintf(FrFOut," nTot=%d\n",nTot);
 FrReadVQ (iFile,            &names,    nTot);
 FrReadVI (iFile, (int **)   &GTimeS,   nTot);
 FrReadVI (iFile, (int **)   &GTimeN,   nTot);
 FrReadVL (iFile, (FRLONG **)&position, nTot);
 
                         /*---------------- convert data to a linked list----*/

 root = NULL;
 for(i=0; i<nTot; i++)
  {evt = FrTOCevtMark(iFile, &root, names[i], GTimeS[i], GTimeN[i], 0);
   evt->position[evt->nEvent-1] = position[i];}

                         /*---------------------- free the temporary buffer--*/

 free(names);
 free(GTimeS);
 free(GTimeN);
 free(position);

 return(root);}

/*---------------------------------------------------- FrBack5ProcDataRead---*/
FrProcData *FrBack5ProcDataRead(FrFile *iFile)
/*---------------------------------------------------------------------------*/
{FrProcData *procData;
 unsigned int timeOffsetN, timeOffsetS;
 double sampleRate;

  procData = (FrProcData *) calloc(1,sizeof(FrProcData));
  if(procData == NULL)  
       {iFile->error = FR_ERROR_MALLOC_FAILED;
        return(NULL);}
  procData->classe = FrProcDataDef();
  
  FrReadHeader(iFile,  procData);
  FrReadSChar( iFile, &procData->name);
  FrReadSChar( iFile, &procData->comment);
  FrReadDouble(iFile, &sampleRate);
  FrReadIntU  (iFile, &timeOffsetS);
  FrReadIntU  (iFile, &timeOffsetN);
  procData->timeOffset = timeOffsetS + 1.e-9 * timeOffsetN;
  FrReadDouble(iFile, &procData->fShift);
  FrReadFloat(iFile, &procData->phase);
  FrReadStruct(iFile, &procData->data);
  iFile->vectInstance = iFile->instance;
  FrReadStruct(iFile, &procData->aux);
  FrReadStruct(iFile, &procData->table);
  FrReadStruct(iFile, &procData->next);

  if(FrDebugLvl > 2) fprintf(FrFOut," %s\n", procData->name);

  return(procData);}
