/*---------------------------------------------------------------------------*/
/* File: FrIO.c                               Last update:     Apr 12, 2011  */
/*                                                                           */
/* This file contains all the I/O low level function used by the Frame       */
/* Library. To use special IO devices you just need to change this file      */
/*                                                                           */
/* Copyright (C) 2002, B. Mours, R. Willams.                                 */
/* For the licensing terms see the LICENSE file.                             */
/* For the list of contributors see the history section of the documentation */
/*---------------------------------------------------------------------------*/
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include "FrIO.h"

/*------------------------------------ this is need for Windows NT --*/
#if !defined(O_BINARY)
#define O_BINARY 0
#endif

/*--------------------------------------------------------------------*/
int FrIOmkdir(char *dirName)
/*--------------------------------------------------------------------*/
{
  int err;
  DIR *dirp;

  dirp  = opendir(dirName);
  if(dirp != NULL) { /*------- test if the directory exist----*/
    closedir(dirp);
    return(0);}

  err = mkdir(dirName, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
     
  return(err);}

/*--------------------------------------------------------------------*/
struct FrIO *FrIOOpenR(char* filename)
/*--------------------------------------------------------------------*/
{struct FrIO *frfd;
	
 frfd = (struct FrIO *)malloc(sizeof(struct FrIO));
 if(frfd == NULL) return(NULL);

 frfd->position   = FRIOBSIZE; 
 frfd->nBytesRead = FRIOBSIZE;  
#ifdef FRIOCFILE
 if(strcmp(filename,"STDIN") == 0)
      frfd->fp = stdin;
 else frfd->fp = fopen(filename, "rb");

 if(frfd->fp == NULL) 
   {free(frfd);
    return (NULL);}
#else
 if(strcmp(filename,"STDIN") == 0)
       frfd->fd = dup(0);
 else  frfd->fd = open(filename, O_RDONLY|O_BINARY, 0);

 if(frfd->fd < 0)
   {free(frfd);
    return (NULL);}
#endif

 else  {return (frfd);}
}

/*--------------------------------------------------------------------*/
struct FrIO *FrIOOpenW(char* filename)
/*--------------------------------------------------------------------*/
{struct FrIO *frfd;
	
 frfd = (struct FrIO *)malloc(sizeof(struct FrIO));
 if(frfd == NULL) return(NULL);

#ifdef FRIOCFILE
 frfd->fp = fopen(filename, "w");
 if(frfd->fp == NULL) 
   {free(frfd);
    return (NULL);}
#else
 unlink(filename);
 frfd->fd = open(filename,  O_CREAT|O_WRONLY|O_BINARY,0644);
 if(frfd->fd < 0) 
   {free(frfd);
    return (NULL);}
#endif

 else             {return (frfd);}
}
/*--------------------------------------------------------------------*/
int FrIOClose(struct FrIO *frfd)
/*--------------------------------------------------------------------*/
{int ret;

#ifdef FRIOCFILE
 ret = fclose(frfd->fp);
#else
 ret = close(frfd->fd);
#endif

 free(frfd);
 return ret;
}

/*--------------------------------------------------------------------*/
FRLONG FrIOWrite(struct FrIO *frfd, char* buf, FRLONG nbyte)
/*--------------------------------------------------------------------*/
{
#ifdef FRIOCFILE
 return fwrite(buf, 1, nbyte, frfd->fp);
#else
 return write(frfd->fd, buf, nbyte);
#endif
}

/*--------------------------------------------------------------------*/
FRLONG FrIORead(struct FrIO *frfd, char* buf, FRLONG nbyte)
/*--------------------------------------------------------------------*/
{FRLONG nbytes;

 if(frfd->position >= frfd->nBytesRead) 
   {if(nbyte >= FRIOBSIZE)  /*--- direct buffer read for large block--*/
#ifdef FRIOCFILE
       {nbytes = (fread(buf, 1, nbyte, frfd->fp));
#else
       {nbytes = read(frfd->fd, buf, nbyte);
#endif
       return(nbytes);}
           
            /*--------- in this case we use a local buffer -----------*/

    else 
#ifdef FRIOCFILE
      {nbytes = (fread(frfd->buffer, 1, FRIOBSIZE, frfd->fp));
#else
      {nbytes = read(frfd->fd, frfd->buffer, FRIOBSIZE);
#endif
       if(nbytes <= 0) return(0);
       frfd->nBytesRead = nbytes;
       frfd->position = 0;}}
    
 if(frfd->nBytesRead -frfd->position < nbyte) 
      {nbytes = frfd->nBytesRead  - frfd->position;}
 else {nbytes = nbyte;}

 memcpy(buf, frfd->buffer + frfd->position, nbytes);
 frfd->position += nbytes;

 return(nbytes);
}

/*--------------------------------------------------------------------*/
FRLONG FrIOSet(struct FrIO *frfd, FRLONG offset)
/*--------------------------------------------------------------------*/
/* Set the file frfd->position (in bytes) from the file beggining
   returns -1 in case of error       */
{FRLONG off;

#ifdef FRIOCFILE
 off = fseek(frfd->fp, offset, SEEK_SET);
#else
 off = lseek(frfd->fd, offset, SEEK_SET);
#endif
 frfd->position = frfd->nBytesRead;

 return(off);}

/*--------------------------------------------------------------------*/
FRLONG FrIOSetFromEnd(struct FrIO *frfd, FRLONG offset)
/*--------------------------------------------------------------------*/
/* Set the file frfd->position (in bytes) from the end of file
   returns -1 in case of error       */
{FRLONG off;

#ifdef FRIOCFILE
 off = fseek(frfd->fp, offset, SEEK_END);
#else
 off = lseek(frfd->fd, offset, SEEK_END);
#endif
 frfd->position = FRIOBSIZE;

 return(off);}

/*--------------------------------------------------------------------*/
FRLONG FrIOSetFromCur(struct FrIO *frfd, FRLONG offset)
/*--------------------------------------------------------------------*/
/* Set the file frfd->position (in bytes) from the end of file
   returns -1 in case of error       */
{FRLONG off;

  offset -= (frfd->nBytesRead  - frfd->position);
#ifdef FRIOCFILE
 off = fseek(frfd->fp, offset, SEEK_CUR);
#else
 off = lseek(frfd->fd, offset, SEEK_CUR);
#endif
 frfd->position = frfd->nBytesRead;

 return(off);}

/*--------------------------------------------------------------------*/
FRLONG FrIOTell(struct FrIO *frfd)
/*--------------------------------------------------------------------*/
/* return the file frfd->position (in bytes)                                */
{FRLONG pos;

#ifdef FRIOCFILE
 pos = ftell(frfd->fp);
#else
 pos = lseek(frfd->fd, 0, SEEK_CUR);
#endif
 pos -= (frfd->nBytesRead  - frfd->position);
 
 return(pos);}






