/*
 * Soap Proxy- properties header
 *
 * Milan Novacek, ANF DATA, Feb 2012
 *
 * Copyright (c) 2012, ANF DATA Spol. s r.o.
 *
 ******************************************************************************
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the “Software”),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies of this Software or works derived from this Software.
 *
 * THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 ******************************************************************************
 *
 * See sp_svc.c for general comments.
 *
 */

/**
 * @file sp_props.h
 *
 */

#ifndef SPPROPS_H_INCLUDED
#define SPPROPS_H_INCLUDED

#include <axis2_conf_ctx.h>

#include "sp_constants.h"

/**
 * Names of the parameters in in the config file
 */
#define SP_MAPFILE_STR    "MapFile"
#define SP_MAPSERVER_STR  "MapServ"
#define SP_BACKENDURL_STR "BackendURL"
#define SP_SOAPOPSURL_STR "SOAPOperationsURL"
#define SP_DELNONSOAP_STR "DeleteNonSoapURLs"
#define SP_DEBUG_STR      "DebugSoapProxy"

// 
//  WCS-SOAP-To-POST specific properties.
//
struct sp_props_struct
{
    const axis2_msg_ctx_t * msg_ctx;

    int deleting_nonsoap;
    int debug_mode;

    axis2_char_t mapfile         [SP_MAX_MPATHS_LEN];
    axis2_char_t mapserv         [SP_MAX_MPATHS_LEN];
    axis2_char_t backend_url_str [SP_MAX_MPATHS_LEN];
    axis2_char_t soapops_url_str [SP_MAX_MPATHS_LEN];

    // Derived values.

    int          url_mode;
    int          backend_port;
    axis2_char_t backend_host[SP_MAX_MPATHS_LEN];
    axis2_char_t backend_path[SP_MAX_MPATHS_LEN];

};

typedef struct sp_props_struct sp_props;

const int           rp_getDebugMode      (const axutil_env_t *env, const sp_props *props);
const int           rp_getUrlMode        (const axutil_env_t *env, const sp_props *props);
const int           rp_getDeletingNonSoap(const axutil_env_t *env, const sp_props *props);
const axis2_char_t *rp_getMapfile        (const axutil_env_t *env, const sp_props *props);
const axis2_char_t *rp_getMapserverExec  (const axutil_env_t *env, const sp_props *props);
const axis2_char_t *rp_getSoapOpsURL     (const axutil_env_t *env, const sp_props *props);
const axis2_char_t *rp_getBackendURL     (const axutil_env_t *env, const sp_props *props);
const axis2_char_t *rp_getBackendPath    (const axutil_env_t *env, const sp_props *props);
const int           rp_getBackendPort    (const axutil_env_t *env, const sp_props *props);
const axis2_char_t *rp_getBackendHost    (const axutil_env_t *env, const sp_props *props);


#endif
