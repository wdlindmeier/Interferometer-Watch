/* -*- mode: c; c-basic-offset: 4; -*- */
#if HAVE_CONFIG_H
#include "daq_config.h"
#endif /* HAVE_CONFIG_H */

#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#if HAVE_UNISTD_H
#include <unistd.h>
#else  /* HAVE_UNISTD_H */
#include <io.h>
#endif /* HAVE_UNISTD_H */

#include "daqc.h"
#include "trench.h"
#include "nds_os.h"

/*======================================  Identify the channel type  */
void 
trench_init(struct trench_struct* t) {
    t->str   = NULL;
    t->len   = 0;
    t->styp  = trch_base;
    t->ctype = cUnknown;
    t->rate  = 0.0;
    t->dtype = _undefined;
}

/*======================================  Infer channel information    */
void
trench_infer_chan_info(struct trench_struct* t, enum chantype ctype,
		       double rate, daq_data_t rawtype) {

    /*----------------------------------  Base channel name            */
    if (t->styp == trch_base) {
	if (t->ctype == cUnknown) t->ctype = ctype;
	if (t->rate  == 0)        t->rate  = rate;
	t->dtype = rawtype;
    }

    /*----------------------------------  Trend sub-channel            */
    else {
	/*------------------------------  Force trend channel types    */
	if (t->ctype != cSTrend && t->ctype != cMTrend) t->ctype = ctype;
	if (t->ctype != cSTrend && t->ctype != cMTrend) {
	    if (t->rate > 0.02) t->ctype = cSTrend;
	    else 	        t->ctype = cMTrend;
	}

	/*------------------------------  Make sure rate is correct    */
	if (t->ctype == cSTrend) t->rate = 1.0; 
	if (t->ctype == cMTrend) t->rate = (double) 1.0/60.0;

	/*------------------------------  Guess dtype from raw data type */
	t->dtype = trench_dtype(t, rawtype);
    }
}

/*======================================  Identify the channel type  */
void 
trench_parse(struct trench_struct* t, const char* s) {
    char * pterm;

    /*----------------------------------  Copy string, find the first ' '    */
    t->str = strdup(s);
    pterm = strchr(t->str, ' ');
    if (pterm) *pterm = 0;

    /*----------------------------------  Look for c-type or rate at end     */
    while ((pterm = strrchr(t->str, ',')) != NULL) {
	*pterm++ = 0;
	if (*pterm >= '0' && *pterm <= '9') {
	    t->rate = strtod(pterm, 0);
	} else {
	    t->ctype = cvt_str_chantype(pterm);
	}
    }

    /*----------------------------------  Look for a trend subchannel suffix */
    t->styp = trch_base;
    {
	const char* pdot = strrchr(t->str, '.');
	if (pdot) {
	    size_t ldot = strlen(pdot);
	    if (ldot == 2 && !strcmp(pdot, ".n")) {
		t->styp = trch_n;
	    } else if (ldot == 4) {
		if (!strcmp(pdot, ".max")) {
		    t->styp = trch_max;
		} else if (!strcmp(pdot, ".min")) {
		    t->styp = trch_min;
		} else if (!strcmp(pdot, ".rms")) {
		    t->styp = trch_rms;
		}
	    } else if (ldot == 5 && !strcmp(pdot, ".mean")) {
		t->styp = trch_mean;
	    }
	}
	if (t->styp == trch_base) t->len = strlen(t->str);
	else                      t->len = (size_t)( pdot - t->str );
    }
}

/*======================================  Compare the base name  */
int
trench_cmp_base(struct trench_struct* t, const char* s) {
    int rc = -1;
    if (t->styp == trch_base) rc = strcmp (t->str, s);
    else                      rc = strncmp(t->str, s, t->len);
    return rc;
}

/*======================================  Get data type         */
void
trench_destroy(struct trench_struct* t) {
    if (t->str) free(t->str);
    t->str = 0;
}

/*======================================  Get data type         */
daq_data_t
trench_dtype(struct trench_struct* t, daq_data_t rawtype) {
    daq_data_t dtype;
    switch (t->styp) {
    case trch_mean:
    case trch_rms:
	dtype = _64bit_double;
	break;
    case trch_min:
    case trch_max:
	if      (rawtype == _16bit_integer) dtype = _32bit_integer;
	else if (rawtype == _32bit_integer) dtype = _32bit_integer;
	else                                dtype = _32bit_float;
	break;
    case trch_n:
	dtype = _32bit_integer;
	break;
    default:
	dtype = rawtype;
    }
    return dtype;
}
