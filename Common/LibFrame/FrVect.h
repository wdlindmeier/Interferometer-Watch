/*---------------------------------------------------------------------------*/
/* File: FrVect.h                             Last update:     Dec 09, 2003  */
/*                                                                           */
/* Copyright (C) 2003, B. Mours.                                             */
/* For the licensing terms see the LICENSE file.                             */
/* For the list of contributors see the history section of the documentation */
/*---------------------------------------------------------------------------*/
#if !defined(FRVECT_DEFINED)
#define FRVECT_DEFINED

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned short  FRVECTTYPES; 
             
enum     {FR_VECT_C,     /* vector of char                           */
          FR_VECT_2S,    /* vector of short                          */
          FR_VECT_8R,    /* vector of double                         */
          FR_VECT_4R,    /* vector of float                          */
          FR_VECT_4S,    /* vector of int                            */
          FR_VECT_8S,    /* vector of long                           */
          FR_VECT_8C,    /* vector of complex float                  */
          FR_VECT_16C,   /* vector of complex double                 */
          FR_VECT_STRING,/* vector of string                         */
          FR_VECT_2U,    /* vector of unsigned short                 */
          FR_VECT_4U,    /* vector of unsigned int                   */
          FR_VECT_8U,    /* vector of unsigned long                  */
          FR_VECT_1U,    /* vector of unsigned char                  */
          FR_VECT_8H,    /* halfcomplex vectors (float) (FFTW order) */
          FR_VECT_16H,   /* halfcomplex vectors (double)(FFTW order) */
          FR_VECT_END};  /* vector of unsigned char                  */

#define FR_VECT_C8  FR_VECT_8C
#define FR_VECT_C16 FR_VECT_16C
#define FR_VECT_H8  FR_VECT_8H
#define FR_VECT_H16 FR_VECT_16H

/*---------------------------------------------------------------------------*/

struct FrVect {
  FrSH *classe;                  /* class information (internal structure)   */
  char          *name;           /* vector name                              */
  unsigned short compress;       /* 0 = no compression; 1 = gzip             */
  FRVECTTYPES    type;           /* vector type  (see bellow)                */
  FRULONG        nData;          /* number of elements=nx[0].nx[1]..nx[nDim] */
  FRULONG        nBytes;         /* number of bytes                          */
  char          *data;           /* pointer to the data area.                */
  unsigned int   nDim;           /* number of dimension                      */
  FRULONG *nx;                   /* number of element for this dimension     */
  double *dx;                    /* step size value (express in above unit)  */
  double *startX;                /* offset for the first x value             */
  char   **unitX;                /* unit name for (used for printout)        */
  char    *unitY;                /* unit name for (used for printout)        */
  FrVect *next;                  /* hook for additional data                 */
                                 /* ------- end_of_SIO parameters -----------*/
  short  *dataS;                 /* pointer to the data area (same as *data) */
  int    *dataI;                 /* pointer to the data area (same as *data) */
  FRLONG *dataL;                 /* pointer to the data area (same as *data) */
  float  *dataF;                 /* pointer to the data area (same as *data) */
  double *dataD;                 /* pointer to the data area (same as *data) */
  unsigned char  *dataU;         /* pointer to the data area (same as *data) */
  unsigned short *dataUS;        /* pointer to the data area (same as *data) */
  unsigned int   *dataUI;        /* pointer to the data area (same as *data) */
  FRULONG        *dataUL;        /* pointer to the data area (same as *data) */
  char  **dataQ;                 /* pointer to the data area (same as *data) */
  int  wSize;                    /* size of one word                         */
  FRULONG space;                 /* variable for internal use                */
                                 /* the following variables are only fill    */
                                 /* when a vector is used in stand alonemode */
  double GTime;                  /* vector GPS time origin(second)           */
  unsigned short ULeapS;         /* leap seconds between GPS and UTC         */
  int localTime;                 /* Time offset = Local time - UTC (sec)     */
  char    *dataUnzoomed;         /* initial (before zoom) pointer to data    */
  FRULONG nDataUnzoomed;         /* initial (before zoom) nData              */
  double startXUnzoomed;         /* initial (before zoom) startX             */
};

#ifdef __cplusplus
}
#endif

#endif
















