/* -*- mode: c; c-basic-offset: 4; -*- */
#ifndef TRENCH_H
#define TRENCH_H
#include "channel.h"
#include "sys/types.h"

#ifndef DLL_EXPORT
#if WIN32 || WIN64
#define DLL_EXPORT __declspec(dllexport)
#else
#define DLL_EXPORT
#endif /* WIN32 || WIN64 */
#endif /* DLL_EXPORT */

/**  Trend sub-channel types.
  */
enum trench_type {
    trch_base,
    trch_mean,
    trch_rms,
    trch_min,
    trch_max,
    trch_n
};

/**  The trench package (structure + functions) implement a standard api to 
  *  specify and match channel names used by nds2. The channel names may 
  *  include a trend subchannel name, a sample rate or an nds2 channel 
  *  type. The general format for channel name to be parsed is:
  *  \verbatim
     <base-name>[.<subchannel>][,<chan-type>][,<rate>]
     \endverbatim
  *  The package is used by 
  *  - Create and initialize a \c trench_struct structure 
  *  - Preset any known parameters.
  *  - Parse the channel name
  *  - Extract any needed information from the structure.
  *
  *  A usage example is shown below:
  *
  *  \verbatim
  --  Initialize the trench structure
       trench_struct tch;
       trench_init(&tch);
  --  Preset default channel properties (e.g. channel type, rate, data type)
  --  here.
       tch.ctype = cMTrend;
  --  Parse channel specifier into structure.
       trench_parse(&tch, chanString);
     \endverbatim
  *  \brief NDS2 trend channel specification.
  *  \author John. Zweizig
  *  \version 1.0; Last modified May 3, 2010
  */
struct trench_struct {
    /** Full channel name string.               */
    char*        str;

    /** Length of channel base name.            */
    size_t       len;

    /** Trend channel subtype (statistic).      */
    enum trench_type styp;

    /** NDS2 channel type.                      */
    chantype_t   ctype;

    /**  The sample rate is valid only if it is specified explicitly or 
      *  inferred from the channel type. Othewise rate = 0;
      *  \brief Sample rate
      */
    double       rate;

    /**  The data type field defaults to -undefined and may be inferred from
      *  the channel subtype if the channel is a trend
      *  \brief The type of the channel data.
      */
    daq_data_t   dtype;
};

/**  Compare the base string of a parsed channel name to the specified 
  *  string. If the parsed channel name is a simple (non-trend) channel,
  *  the argument string is compared o the full channel name. If the parsed
  *  channel name has a sub-channel suffix (i.e the channel is a trend), the
  *  argument string is compared to the base channel name.
  *  \brief Compare string to channel base name.
  *  \param t Parsed channel name structure pointer.
  *  \param s Pointer to comparison string..
  *  \return An integer greater, equal or less than zer depending on 
  *  whether th parsed base string is greater, equal or less than s.
  */
DLL_EXPORT int
trench_cmp_base(struct trench_struct* t, const char* s);

/**  Release internal storage for a parsed channel name.
  *  \brief Release parsed data storage.
  *  \param t Parsed channel name structure pointer.
  */
DLL_EXPORT void
trench_destroy(struct trench_struct* t);

/**  Infer the data type from the the channel type and sub-channel
  *  ID.
  *  \param t Parsed channel name structure pointer.
  *  \param rawtype Data type of raw channel.
  *  \return Inferred data type.
  */
DLL_EXPORT daq_data_t
trench_dtype(struct trench_struct* t, daq_data_t rawtype);

/**  Infer channel information from the (parsed) channel name and the 
  *  specified channel type, rate and data type. 
  *  \param t Parsed channel name structure pointer.
  *  \param ctype   Default channel type.
  *  \param rate    Raw data type.
  *  \param rawtype Data type of raw channel.
  */
DLL_EXPORT void
trench_infer_chan_info(struct trench_struct* t, enum chantype ctype,
		       double rate, daq_data_t rawtype);

/**  Initialize the trench structure. THe name string is set to NULL,
  *  the channel and data types are set to unkown, the channel
  *  sub-type field is set to trch_base and the rate is set to zero.
  *  \param t Parsed channel name structure pointer.
  */
DLL_EXPORT void 
trench_init(struct trench_struct* t);

/**  Parse a channel specifier.
  *  \brief Parse a channel name.
  *  \param tch Parsed channel name structure pointer.
  *  \param str String to be parsed.
  */
DLL_EXPORT void 
trench_parse(struct trench_struct* tch, const char* str);

#endif /* defined(TRENCH_H) */
