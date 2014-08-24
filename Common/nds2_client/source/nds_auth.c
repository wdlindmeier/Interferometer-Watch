/* -*- mode: c; c-basic-offset: 4; -*- */

#if HAVE_CONFIG_H
#include "daq_config.h"
#endif /* HAVE_CONFIG_H */

#if HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif /* HAVE_SYS_SOCKET_H */

#if HAVE_ERRNO_H
#include <errno.h>
#endif /* HAVE_ERRNO_H */
#include <stdio.h>
#if HAVE_STDLIB_H
#include <stdlib.h>
#endif /* HAVE_STDLIB_H */
#if HAVE_STRING_H
#include <string.h>
#endif /* HAVE_STRING_H */

#undef HAVE_STDLIB_H
#undef HAVE_STRING_H

#include "nds_auth.h"
#include "nds_logging.h"
#include "daqc_private.h"
#include "daqc_internal.h"
#include "daqc_response.h"

#if _WIN32
#define strtok_r	strtok_s

typedef int recv_size_t;
#else /* _WIN32 */
typedef ssize_t recv_size_t;
#endif /* _WIN32 */

#if HAVE_SASL
#include <sasl/sasl.h>
#include <sasl/saslutil.h>
#elif HAVE_GSSAPI
#include <gssapi/gssapi.h>

#define CHAR64(c)  (((c) < 0 || (c) > 127) ? -1 : index_64[(c)])

static unsigned char basis_64[] =
   "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static char index_64[128] = {
    -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
    -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
    -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,62, -1,-1,-1,63,
    52,53,54,55, 56,57,58,59, 60,61,-1,-1, -1,-1,-1,-1,
    -1, 0, 1, 2,  3, 4, 5, 6,  7, 8, 9,10, 11,12,13,14,
    15,16,17,18, 19,20,21,22, 23,24,25,-1, -1,-1,-1,-1,
    -1,26,27,28, 29,30,31,32, 33,34,35,36, 37,38,39,40,
    41,42,43,44, 45,46,47,48, 49,50,51,-1, -1,-1,-1,-1
};

/* base64 encode
 *  in      -- input data
 *  inlen   -- input data length
 *  out     -- output buffer (will be NUL terminated)
 *  outmax  -- max size of output buffer
 * result:
 *  outlen  -- gets actual length of output buffer (optional)
 * 
 * Returns 0 on success, -3 if result won't fit
 */

static int
sasl_encode64(const char *_in, unsigned inlen, char *_out, 
	      unsigned outmax, unsigned *outlen)
{
    const unsigned char *in = (const unsigned char *)_in;
    unsigned char *out = (unsigned char *)_out;
    unsigned char oval;
    unsigned olen;

    /* check params */
    if ((inlen >0) && (in == NULL)) return -7;
    
    /* Will it fit? */
    olen = (inlen + 2) / 3 * 4;
    if (outlen) {
      *outlen = olen;
    }
    if (outmax <= olen) {
      return -3;
    }

    /* Do the work... */
    while (inlen >= 3) {
      /* user provided max buffer size; make sure we don't go over it */
        *out++ = basis_64[in[0] >> 2];
        *out++ = basis_64[((in[0] << 4) & 0x30) | (in[1] >> 4)];
        *out++ = basis_64[((in[1] << 2) & 0x3c) | (in[2] >> 6)];
        *out++ = basis_64[in[2] & 0x3f];
        in += 3;
        inlen -= 3;
    }
    if (inlen > 0) {
      /* user provided max buffer size; make sure we don't go over it */
        *out++ = basis_64[in[0] >> 2];
        oval = (in[0] << 4) & 0x30;
        if (inlen > 1) oval |= in[1] >> 4;
        *out++ = basis_64[oval];
        *out++ =(unsigned char) ((inlen < 2) ? '=' 
				 : basis_64[(in[1] << 2) & 0x3c]);
        *out++ = '=';
    }

    *out = '\0';
    
    return 0;
}

/* base64 decode
 *  in     -- input data
 *  inlen  -- length of input data
 *  out    -- output data (may be same as in, must have enough space)
 *  outmax  -- max size of output buffer
 * result:
 *  outlen -- actual output length
 *
 * returns:
 * SASL_BADPROT on bad base64,
 * SASL_BUFOVER if result won't fit,
 * SASL_CONTINUE on a partial block,
 * SASL_OK on success
 */

static int
sasl_decode64(const char *in, unsigned inlen, char *out,
	      unsigned outmax, unsigned *outlen)
{
    unsigned len = 0;
    unsigned j;
    int c[4];
    int saw_equal = 0;

    /* check parameters */
    if (out == NULL) return -1;

    if (inlen > 0 && *in == '\r') return -1;

    while (inlen > 3) {
        /* No data is valid after an '=' character */
        if (saw_equal) {
            return -5;
        }

	for (j = 0; j < 4; j++) {
	    c[j] = in[0];
	    in++;
	    inlen--;
	}

        if (CHAR64(c[0]) == -1 || CHAR64(c[1]) == -1) return -5;
        if (c[2] != '=' && CHAR64(c[2]) == -1) return -5;
        if (c[3] != '=' && CHAR64(c[3]) == -1) return -5;
        /* No data is valid after a '=' character, unless it is another '=' */
        if (c[2] == '=' && c[3] != '=') return -5;
        if (c[2] == '=' || c[3] == '=') {
            saw_equal = 1;
        }

        *out++ = (char) ((CHAR64(c[0]) << 2) | (CHAR64(c[1]) >> 4));
        if (++len >= outmax) return -3;
        if (c[2] != '=') {
            *out++ = (char)(((CHAR64(c[1]) << 4) & 0xf0) | (CHAR64(c[2]) >> 2));
            if (++len >= outmax) return -3;
            if (c[3] != '=') {
                *out++ = (char) (((CHAR64(c[2]) << 6) & 0xc0) | CHAR64(c[3]));
                if (++len >= outmax) return -3;
            }
        }
    }

    if (inlen != 0) {
        if (saw_equal) {
            /* Unless there is CRLF at the end? */
            return -5;
        } else {
	    return 1;
        }
    }

    *out = '\0'; /* NUL terminate the output string */

    if (outlen) *outlen = len;

    return 0;
}
#endif

#if HAVE_SASL || HAVE_GSSAPI

/*------------------------------------------------------------------------
 *
 *   Receive a radix-60 encoded string
 *
 *------------------------------------------------------------------------*/
static int
nds_gets(nds_socket_type fd, char* buf, size_t buflen) {
    static const char* METHOD = "nds_gets";
    int flags=0;
    size_t l = 0;
    int   rc = 0;

    while (l < buflen) {
#if _WIN32
	rc = recv(fd, buf+l, (int)( buflen-l ), flags);
#else _WIN32
	rc = recv(fd, buf+l, buflen-l, flags);
#endif /* _WIN32 */
	if (rc == 0) {
	    printf("nds_gets: Unexpected EOF\n");
	    break;
	} else if (rc < 0) {
	    if ( nds_logging_check( NDS_LOG_GROUP_VERBOSE_ERRORS, 10 ) )
	    {
		nds_logging_print_errno( METHOD );
	    }
	    perror("nds_gets error");
	    break;
	}
	l += (unsigned) rc;
	if (!buf[l-1] || buf[l-1] == '\n') {
	    buf[--l] = 0;
	    break;
	}
    }
    if (l > 0) {
	unsigned int len = 0;
	sasl_decode64(buf, (unsigned) l, buf, (unsigned) buflen, &len);
	l = len;
    } else {
	printf("nds_gets: Error receiving string.\n");
    }
    return (int)l;
}

/*   nds_puts converts the specified string to radix 64 and sends it to the
 *   specified server socket. This function should be used only for sending
 *   sasl protocol messages. The function returns the number of bytes sent 
 *   or -1 if an error occurred. Note that the number of bytes sent is NOT 
 *   the same as the length of the string sent.
 */

#if _WIN32
typedef int nds_puts_size_t;
#else /* _WIN32 */
typedef size_t nds_puts_size_t;
#endif /* _WIN32 */

static int
nds_puts(nds_socket_type fd, const char* buf, nds_puts_size_t len) {
    int rc;
    size_t ltmp = ((len / 3) + 1) * 4 + 2;
    char*  tbuf = malloc(ltmp);
    unsigned int ldat = 0;
    sasl_encode64(buf, (unsigned)( len ), tbuf, (unsigned) ltmp, &ldat);
    tbuf[ldat++] = '\n';
    rc = (int)( send(fd, tbuf, (size_t) ldat, 0) );
    free(tbuf);
    return rc;
}
#endif

/*------------------------------------------------------------------------
 *
 *   SASL-Based user authentication.
 *
 *------------------------------------------------------------------------*/
int
nds_authenticate(daq_t *daq, const char* server) {
    static const char* METHOD = "nds_authenticate";
#if ! HAVE_SASL && ! HAVE_GSSAPI
    static int prt_once=0;
#endif /* ! HAVE_SASL */
    int result = 0;
    daq->auth_ctx = NULL;

#if HAVE_SASL
    int nch;
    sasl_conn_t* conn = 0;
    const char *out, *mechusing;
    unsigned int outlen, lmech;

    /*  Allocate input buffer   */
    size_t buflen = 2048;
    char* buf  = malloc(buflen);

    /*  Get mechanism list      */
    nch = nds_gets(daq->conceal->sockfd, buf, buflen);
    if (nch < 0) {
	free(buf);
	return DAQD_ERROR;
    }

    /* client new connection */
    result=sasl_client_new("nds2",     /* The service we are using */
			   server,     /* The fully qualified domain
					  name of the server we're
					  connecting to */
			   NULL, NULL, /* Local and remote IP
					  address strings
					  (NULL disables mechanisms
					  which require this info)*/
			   NULL,       /* connection-specific
					  callbacks */
			   0,          /* security flags */
			   &conn       /* alloc on success */
			   );
    /* check to see if that worked */
    if (result != SASL_OK) {
	printf("sasl_client_new failed, rc = %i\n", result);
	free(buf);
	return DAQD_ERROR;
    }
    daq->auth_ctx = conn;

    /*  Start authentication interchange - decide on a mechanism */
    result=sasl_client_start(daq->auth_ctx,  /* the context from above */ 
			     buf,            /* the list of mechanisms
						from the server */
			     NULL,      /* filled in if an
					   interaction is needed */
			     &out,      /* filled in on success */
			     &outlen,   /* filled in on success */
			     &mechusing
			     );
    if (result != SASL_OK && result != SASL_CONTINUE) {
	printf("error detail: %s\n", sasl_errdetail(daq->auth_ctx));
	free(buf);
	return DAQD_ERROR;	
    }

    /* Send the mechanism */
    lmech = (unsigned int)( strlen(mechusing) );
    nch = nds_puts(daq->conceal->sockfd, mechusing,
		   ( nds_puts_size_t)( lmech) );

    if ( nds_logging_check( NDS_LOG_GROUP_VERBOSE_ERRORS,
			    20 ) )
    {
	nds_logging_printf ( "INFO: %s: mechanism: %s\n",
			     METHOD,
			     mechusing );
    }
    if (nch < 0) {
 	perror("nds_authenticate: Error sending mechanism");
 	free(buf);
 	return DAQD_ERROR;
    }

    /** interact **/
    while (result == SASL_CONTINUE) {
	/*-------------------  Send the line formatted by SASL */
	nch = nds_puts(daq->conceal->sockfd, out,
		       (nds_puts_size_t)( outlen ) );
	if (nch < 0) {
	    perror("nds_authenticate: Error sending client string");
	    break;
	}

	/*-------------------  Get a response from the server  */
	nch = nds_gets(daq->conceal->sockfd, buf, buflen);
	if (nch < 0) {
	    perror("nds_authenticate: Error reading server string");
	    break;
	}

	/*-------------------  Turn the crank                   */
	result=sasl_client_step(daq->auth_ctx,   /* our context */
				buf,    /* data from the server */
				(unsigned) nch,   /* its length */
				NULL,   /* Client interactions? */
				&out,   /* filled in on success */
				&outlen /* filled in on success */
				);
 	if (result != SASL_OK && result != SASL_CONTINUE) {
 	    printf("nds_authenticate: Error stepping client: %s\n", 
 		   sasl_errdetail(daq->auth_ctx));
	}
    }

    /*-----------------------  See how it all went              */
    if (result != SASL_OK) {
	result = DAQD_ERROR;
    }

    /*-----------------------  Send the last output line        */
    else {
	nds_puts(daq->conceal->sockfd, out,
		 (nds_puts_size_t)( outlen ) );
	result = DAQD_OK;
    }
    free(buf);
#elif HAVE_GSSAPI
    {
	/*  Allocate input buffer   */
	const char* auth_mech = "GSSAPI";
	size_t buflen = 2048;
	char* buf  = malloc(buflen);
	OM_uint32 major_status, minor_status, junk, ret_flags;
	gss_buffer_desc namebuffer;
	gss_buffer_desc inbuffer, outbuffer;
	gss_name_t aname;
	gss_ctx_id_t ctx = GSS_C_NO_CONTEXT;

	/*  Get mechanism list      */
	int nch = nds_gets(daq->conceal->sockfd, buf, buflen);
	int found = 0;

	if ( nch > 0 )
	{
	    char*	pos = buf;
	    char*	token = (char*)NULL;

	    do
	    {
		token = strtok_r( pos, " \t", &pos );
		if ( token && ( strcmp( auth_mech, token ) == 0 ) )
		{
		    found = 1;
		    break;
		}
	    } while( token );

	}
	if ( ! found ) {
	    printf("nds_authenticate: Valid mechanism is not %s\n", auth_mech);
	    free(buf);
	    return DAQD_ERROR;
	}

	nds_puts(daq->conceal->sockfd, auth_mech, (nds_puts_size_t)( strlen(auth_mech)+1 ));

	/*
	 * Build a GSS-API host service name (service@hostname) and pass
	 * it into gss_import_name().
	 */
	if ( nds_logging_check( NDS_LOG_GROUP_VERBOSE_ERRORS,
				20 ) )
	{
	    nds_logging_printf ( "INFO: %s: sizeof( buf ):%d %s - %d\n",
				 METHOD,
				 sizeof( buf ),
				 __FILE__,
				 __LINE__ );
	}
#if HAVE_SPRINTF_S
	sprintf_s(buf, buflen, "nds2@%s", server);
#else /* HAVE_SPRINTF_S */
	sprintf(buf, "nds2@%s", server);
#endif /* HAVE_SPRINTF_S */
	namebuffer.value = buf;
	namebuffer.length = strlen(buf);
    
	major_status = gss_import_name(&minor_status, 
				       &namebuffer,
				       GSS_C_NT_HOSTBASED_SERVICE, 
				       &aname);
	if (major_status != GSS_S_COMPLETE) {
	    printf("error in gss_import_name\n");
	    free(buf);
	    return DAQD_ERROR;
	}

	/*
	 * Do the GSS-API context building loop continue will
	 * GSS_S_CONTINUE_NEEDED is set and no error is returned. When
	 * done GSS_S_COMPLETE is returned.
	 */
	inbuffer.value = NULL;
	inbuffer.length = 0;

	do {
	    int nch;

	    outbuffer.value = NULL;
	    outbuffer.length = 0;

	    major_status = gss_init_sec_context(&minor_status,
						/* use default credential */
						GSS_C_NO_CREDENTIAL,
						&ctx,
						aname,
						GSS_C_NO_OID,
						GSS_C_MUTUAL_FLAG|GSS_C_REPLAY_FLAG|
						GSS_C_CONF_FLAG|GSS_C_INTEG_FLAG,
						GSS_C_INDEFINITE,
						GSS_C_NO_CHANNEL_BINDINGS,
						&inbuffer,
						/* No preferred mechanism */
						NULL,
						&outbuffer,
						&ret_flags,
						NULL);
	    /*
	     * Even in case of an error, if there is an output token, send
	     * it off to the server. The mechanism might want to tell the
	     * acceptor why it failed.
	     */
	    if (outbuffer.value) {
		nds_puts(daq->conceal->sockfd, outbuffer.value,
			 (nds_puts_size_t)( outbuffer.length ) );
		gss_release_buffer(&junk, &outbuffer);
	    }

	    /*  Release input buffer later.
	     */
	    inbuffer.value = NULL;
	    inbuffer.length = 0;

	    /* In case of error, print error and fail */
	    if (GSS_ERROR(major_status)) {
		printf("error in gss_init_sec_context");
		gss_delete_sec_context(&junk, &ctx, NULL);
		free(buf);
		return DAQD_ERROR;
	    }

	    /* If we are not done yet, wait for another token from the server */
	    if ((major_status & GSS_S_CONTINUE_NEEDED) != 0) {
		nch =  nds_gets(daq->conceal->sockfd, buf, buflen);
		if (nch < 0) break;
		inbuffer.length = (size_t) nch;
		inbuffer.value  = buf;
	    }
	} while (major_status != GSS_S_COMPLETE);

	/* If there was a failure building the context, fail */
	gss_release_name(&minor_status, &aname);

	/* If there was a failure building the context, fail */
	if (major_status != GSS_S_COMPLETE) {
	    printf("error in gss_accept_sec_context\n");
	    gss_delete_sec_context(&junk, &ctx, NULL);
	    free(buf);
	    return DAQD_ERROR;
	}

	/*
	 * check that context flags are what we expect them to be, with
	 * confidentiality and integrity protected
	 */
	if ((ret_flags & GSS_C_CONF_FLAG) == 0 ||
	    (ret_flags & GSS_C_INTEG_FLAG) == 0) {
	    printf("confidentiality or integrity missing from context");
	    gss_delete_sec_context(&junk, &ctx, NULL);
	    free(buf);
	    return DAQD_ERROR;
	}

	nds_puts(daq->conceal->sockfd, "(NULL)", 0);

	inbuffer.length = (size_t) nds_gets(daq->conceal->sockfd, buf, buflen);
	inbuffer.value  = buf;
	major_status = gss_unwrap(&minor_status, ctx, &inbuffer, &outbuffer,
				  NULL, NULL);

	gss_release_buffer(&junk, &outbuffer);

	inbuffer.value = "\01\0\0\0";
	inbuffer.length = 4;
	major_status = gss_wrap(&minor_status, ctx, 0, GSS_C_QOP_DEFAULT, 
				&inbuffer, NULL, &outbuffer);

	nds_puts(daq->conceal->sockfd, outbuffer.value,
		 (nds_puts_size_t)( outbuffer.length ) );
	gss_release_buffer(&junk, &outbuffer);

	/*-----------------------  It worked!                      */
	daq->auth_ctx = ctx;
	free(buf);
    }

#else /* HAVE_SASL */
    if (!prt_once) {
 	printf("\n******************************************************\n");
 	printf("Warning - This nds2-client installation has been built\n");
 	printf("          without SASL authentication.\n");
 	printf("          NDS2 client requests will fail.\n");
 	printf("******************************************************\n");
 	prt_once = 1;
    }
    result = DAQD_SASL;
#endif /* HAVE_SASL */
    return result;
}

/*    Disconnect socket authentication context.
 */
int
nds_auth_disconnect(daq_t* daq) {
    int rc = DAQD_OK;
#if HAVE_SASL
    sasl_conn_t* conn = (sasl_conn_t*)daq->auth_ctx;
    if (conn) sasl_dispose(&conn);
#elif HAVE_GSSAPI
    OM_uint32		major;
    gss_ctx_id_t	context_handle = (gss_ctx_id_t)daq->auth_ctx;
    OM_uint32		minor_status;
    gss_buffer_desc	output_token;

    output_token.length = 0;
    output_token.value  = NULL;
    major = gss_delete_sec_context (&minor_status,
				    &context_handle,
				    &output_token);
    if (!major) rc = DAQD_ERROR;
    gss_release_buffer(&minor_status, &output_token);
#endif
    daq->auth_ctx = NULL;
    return rc;
}

/*  Initialize global context.
 */
int
nds_auth_startup(void) {
    int rc = 0;
#ifdef HAVE_SASL
    static int once=0;
    if (!once) {
	rc = sasl_client_init(0);
	once = 1;
    }
#endif
    return rc;
}
