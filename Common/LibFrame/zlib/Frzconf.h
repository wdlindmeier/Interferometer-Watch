/* zconf.h -- configuration of the zlib compression library
 * Copyright (C) 1995-1996 Jean-loup Gailly.
 * For conditions of distribution and use, see copyright notice in zlib.h 
 */

/* $Id: Frzconf.h,v 1.1.1.26 2013-05-24 14:59:10 mours Exp $ */

#ifndef _ZCONF_H
#define _ZCONF_H

/*
 * If you *really* need a unique prefix for all types and library functions,
 * compile with -DZ_PREFIX. The "standard" zlib should be compiled without it.
 */

#define deflateInit_            Frz_deflateInit_
#define deflate                 Frz_deflate
#define deflateEnd              Frz_deflateEnd
#define inflateInit_            Frz_inflateInit_
#define inflate                 Frz_inflate
#define inflateEnd              Frz_inflateEnd
#define deflateInit2_           Frz_deflateInit2_
#define deflateSetDictionary    Frz_deflateSetDictionary
#define deflateCopy             Frz_deflateCopy
#define deflateReset            Frz_deflateReset
#define deflateParams           Frz_deflateParams
#define inflateInit2_           Frz_inflateInit2_
#define inflateSetDictionary    Frz_inflateSetDictionary
#define inflateSync             Frz_inflateSync
#define inflateSyncPoint        Frz_inflateSyncPoint
#define inflateReset            Frz_inflateReset
#define compress                Frz_compress
#define compress2               Frz_compress2
#define uncompress              Frz_uncompress
#define adler32                 Frz_adler32
#define crc32                   Frz_crc32
#define get_crc_table           Frz_get_crc_table

#define Byte                    Frz_Byte
#define uInt                    Frz_uInt
#define uLong                   Frz_uLong
#define Bytef                   Frz_Bytef
#define charf                   Frz_charf
#define intf                    Frz_intf
#define uIntf                   Frz_uIntf
#define uLongf                  Frz_uLongf
#define voidpf                  Frz_voidpf
#define voidp                   Frz_voidp
#define _tr_align               Frz__tr_align
#define _tr_flush_block         Frz__tr_flush_block
#define _tr_init                Frz__tr_init
#define _tr_stored_block        Frz__tr_stored_block
#define _tr_tally               Frz__tr_tally
#define deflate_copyright       Frz_deflate_copyright
#define inflate_blocks          Frz_inflate_blocks 
#define inflate_blocks_free     Frz_inflate_blocks_free
#define inflate_blocks_new      Frz_inflate_blocks_new
#define inflate_blocks_reset    Frz_inflate_blocks_reset
#define inflate_codes           Frz_inflate_codes
#define inflate_codes_free      Frz_inflate_codes_free
#define inflate_codes_new       Frz_inflate_codes_new
#define inflate_copyright       Frz_inflate_copyright
#define inflate_fast            Frz_inflate_fast
#define inflate_flush           Frz_inflate_flush
#define inflate_mask            Frz_inflate_mask
#define inflate_set_dictionary  Frz_inflate_set_dictionary
#define inflate_trees_bits      Frz_inflate_trees_bits
#define inflate_trees_dynamic   Frz_inflate_trees_dynamic
#define inflate_trees_fixed     Frz_inflate_trees_fixed
#define z_errmsg                Frz_z_errmsg
#define zlibVersion             Frz_zlibVersion

#define zcalloc                 Frz_zcalloc
#define zcfree                  Frz_zcfree

/*------------------------ end use of prefix ----*/

#if (defined(_WIN32) || defined(__WIN32__)) && !defined(WIN32)
#  define WIN32
#endif
#if defined(__GNUC__) || defined(WIN32) || defined(__386__) || defined(i386)
#  ifndef __32BIT__
#    define __32BIT__
#  endif
#endif
#if defined(__MSDOS__) && !defined(MSDOS)
#  define MSDOS
#endif

/*
 * Compile with -DMAXSEG_64K if the alloc function cannot allocate more
 * than 64k bytes at a time (needed on systems with 16-bit int).
 */
#if defined(MSDOS) && !defined(__32BIT__)
#  define MAXSEG_64K
#endif
#ifdef MSDOS
#  define UNALIGNED_OK
#endif

#if (defined(MSDOS) || defined(_WINDOWS) || defined(WIN32))  && !defined(STDC)
#  define STDC
#endif
#if (defined(__STDC__) || defined(__cplusplus)) && !defined(STDC)
#  define STDC
#endif

#ifndef STDC
#  ifndef const /* cannot use !defined(STDC) && !defined(const) on Mac */
#    define const
#  endif
#endif

/* Some Mac compilers merge all .h files incorrectly: */
#if defined(__MWERKS__) || defined(applec) ||defined(THINK_C) ||defined(__SC__)
#  define NO_DUMMY_DECL
#endif

/* Maximum value for memLevel in deflateInit2 */
#ifndef MAX_MEM_LEVEL
#  ifdef MAXSEG_64K
#    define MAX_MEM_LEVEL 8
#  else
#    define MAX_MEM_LEVEL 9
#  endif
#endif

/* Maximum value for windowBits in deflateInit2 and inflateInit2 */
#ifndef MAX_WBITS
#  define MAX_WBITS   15 /* 32K LZ77 window */
#endif

/* The memory requirements for deflate are (in bytes):
            1 << (windowBits+2)   +  1 << (memLevel+9)
 that is: 128K for windowBits=15  +  128K for memLevel = 8  (default values)
 plus a few kilobytes for small objects. For example, if you want to reduce
 the default memory requirements from 256K to 128K, compile with
     make CFLAGS="-O -DMAX_WBITS=14 -DMAX_MEM_LEVEL=7"
 Of course this will generally degrade compression (there's no free lunch).

   The memory requirements for inflate are (in bytes) 1 << windowBits
 that is, 32K for windowBits=15 (default value) plus a few kilobytes
 for small objects.
*/

                        /* Type declarations */

#ifndef OF /* function prototypes */
#  ifdef STDC
#    define OF(args)  args
#  else
#    define OF(args)  ()
#  endif
#endif

/* The following definitions for FAR are needed only for MSDOS mixed
 * model programming (small or medium model with some far allocations).
 * This was tested only with MSC; for other MSDOS compilers you may have
 * to define NO_MEMCPY in zutil.h.  If you don't need the mixed model,
 * just define FAR to be empty.
 */
#if (defined(M_I86SM) || defined(M_I86MM)) && !defined(__32BIT__)
   /* MSC small or medium model */
#  define SMALL_MEDIUM
#  ifdef _MSC_VER
#    define FAR __far
#  else
#    define FAR far
#  endif
#endif
#if defined(__BORLANDC__) && (defined(__SMALL__) || defined(__MEDIUM__))
#  ifndef __32BIT__
#    define SMALL_MEDIUM
#    define FAR __far
#  endif
#endif
#ifndef FAR
#   define FAR
#endif

typedef unsigned char  Byte;  /* 8 bits */
typedef unsigned int   uInt;  /* 16 bits or more */
typedef unsigned long  uLong; /* 32 bits or more */

#if defined(__BORLANDC__) && defined(SMALL_MEDIUM)
   /* Borland C/C++ ignores FAR inside typedef */
#  define Bytef Byte FAR
#else
   typedef Byte  FAR Bytef;
#endif
typedef char  FAR charf;
typedef int   FAR intf;
typedef uInt  FAR uIntf;
typedef uLong FAR uLongf;

#ifdef STDC
   typedef void FAR *voidpf;
   typedef void     *voidp;
#else
   typedef Byte FAR *voidpf;
   typedef Byte     *voidp;
#endif


/* Compile with -DZLIB_DLL for Windows DLL support */
#if (defined(_WINDOWS) || defined(WINDOWS)) && defined(ZLIB_DLL)
#  include <windows.h>
#  define EXPORT  WINAPI
#else
#  define EXPORT
#endif

#endif /* _ZCONF_H */
