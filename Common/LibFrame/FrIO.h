/*---------------------------------------------------------------------------*/
/* File: FrIO.h                               Last update:     Jun 26, 2009  */
/*                                                                           */
/* This file contains all the I/O low level function definition used         */
/* by the Frame Library.                                                     */
/*                                                                           */
/* Copyright (C) 2002, B. Mours,                                             */
/* For the licensing terms see the LICENSE file.                             */
/* For the list of contributors see the history section of the documentation */
/*---------------------------------------------------------------------------*/
#if !defined(FRIO_DEFINED)
#define FRIO_DEFINED

#ifdef FR_LONG_LONG /*----- to use 8 bytes long ----------------------*/
typedef            long  long FRLONG;
typedef   unsigned long  long FRULONG;
#define FRLLD "lld"
#else            /*--------- default regular long---------------- ----*/
typedef            long FRLONG;
typedef   unsigned long FRULONG;
#define FRLLD "ld"
#endif

#ifdef FRRFIO         /*---- this is for HPSS-------------------------*/
#include <shift.h>
#endif

#define FRIOBSIZE 16384 /*--- local buffer to speed up the read process ---*/

struct FrIO{
#ifdef FRIOCFILE
  FILE *fp;
#else
  int fd;
#endif
  char buffer[FRIOBSIZE];
  int  nBytesRead;        /* last number of bytes read */ /**** ici***/
  int  position;          /*---------- position in the local buffer, 
                         if position = BSIZE when the buffer is empty */
};

typedef struct FrIO FrIO;

FrIO  *FrIOOpenR(char* filename);
FrIO  *FrIOOpenW(char* filename);
int    FrIOClose     (FrIO *frfd);
FRLONG FrIOWrite     (FrIO *frfd, char* buf, FRLONG nbyte);
FRLONG FrIORead      (FrIO *frfd, char* buf, FRLONG nbyte);
FRLONG FrIOSet       (FrIO *frfd, FRLONG offset);
FRLONG FrIOSetFromEnd(FrIO *frfd, FRLONG offset);
FRLONG FrIOSetFromCur(FrIO *frfd, FRLONG offset);
FRLONG FrIOTell(struct FrIO *frfd);
int    FrIOmkdir(char *dirName);

#endif
