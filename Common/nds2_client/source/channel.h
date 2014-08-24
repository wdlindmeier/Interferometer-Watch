/* -*- mode: c; c-basic-offset: 4; -*- */
#ifndef CHANNEL_H
#define CHANNEL_H

#include <stddef.h>
#include <sys/types.h>

#ifndef DLL_EXPORT
#if WIN32 || WIN64
#define DLL_EXPORT __declspec(dllexport)
#else
#define DLL_EXPORT
#endif /* WIN32 || WIN64 */
#endif /* DLL_EXPORT */

/** \ingroup daq2_api
  * \{
  */

/**  Define channel types. The channel types are used to distinguish 
  *  the requested source of the data. 
  *  \remarks
  *  This expands on and replaces the channel group code in the NDS1 API
  *  which seems to have a very few values (0=normal fast channel, 1000=dmt 
  *  trend channel, 1001=obsolete dmt channel).
  *  \brief Channel type code enumerator.
  */
typedef enum chantype {

    /**  Unknown or unspecified default type. */
    cUnknown,

    /**  Online channel */
    cOnline,

    /**  Archived raw data channel */
    cRaw,

    /** Processed/RDS data channel */
    cRDS,

    /**  Second trend data */
    cSTrend,

    /**  Minute trend data */
    cMTrend,

    /**  test point data */
    cTestPoint,

    /**  FrStat calibration data */
    cStatic

} chantype_t;

/** Allowed maximum length for DMT channels */
#define MAX_LONG_CHANNEL_NAME_LENGTH 255

/** Allowed maximum length for DAQ channels */
#define MAX_CHANNEL_NAME_LENGTH 64

/** Allowed maximum length for signal units */
#define MAX_SIGNAL_UNIT_LENGTH 40

/**  Data type enumerator.
  *  \remarks numbering must be contiguous 
  */
typedef enum {
    /** The data type is not defined. */
    _undefined = 0,

    /** 16-bit (short) integer data. */
    _16bit_integer = 1,

    /** 32-bit (int) integer data. */
    _32bit_integer = 2,

    /** 64-bit (long) integer data. */
    _64bit_integer = 3,

    /** 32-bit (float) floating point data. */
    _32bit_float = 4,

    /** 64-bit (double) floating point data. */
    _64bit_double = 5,

    /** Complex data from two 32-bit floats {re, im}. */
    _32bit_complex = 6
} daq_data_t;

/** should be equal to the first data type
  */
#define MIN_DATA_TYPE _16bit_integer

/** should be equal to the last data type
  */
#define MAX_DATA_TYPE _32bit_complex

/**  Convert a channel type string to a channel type code
 */
DLL_EXPORT chantype_t cvt_str_chantype(const char* str);

/**  Convert a channel type code to a string.
 */
DLL_EXPORT const char* cvt_chantype_str(chantype_t code);

/** Function to relate data types to size.
  */
DLL_EXPORT size_t data_type_size(daq_data_t dtype);

/** Function to relate data types to maximum value.
  */
DLL_EXPORT double data_type_max(daq_data_t dtype);

/** Function to return data type name.
  */
DLL_EXPORT const char* data_type_name(daq_data_t dtype);

/** Function to return data type name.
  */
DLL_EXPORT daq_data_t data_type_code(const char* name);

/**  Inline function to return word length in bytes. This number gives the
  *  granularity in byte for use when swapping data bytes. It differs from
  *  the size attribute for complex numbers.
  */
DLL_EXPORT size_t data_type_word(daq_data_t dtype);

/** \}
  */

#endif
