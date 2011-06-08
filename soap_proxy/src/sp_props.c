/*
 * Soap Proxy.
 *
 * Milan Novacek, ANF DATA, Nov. 2010.
 *
 * Copyright (c) 2010, ANF DATA Spol. s r.o.
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
 *
 */

/**
 * @file sp_props.c
 *
 * WCS-SOAP-To-POST specific properties are configured in the same file
 *  as the service properties and options: services.xml in the service
 *  directory, using <parameter/> elements.
 *  The syntax is:
 *     <parameter name="NNN">VVV</parameter>
 *  where NNN is the name and VVV is the value of the parameter.
 *
 *  Two parameters are required for the WCS SOAP proxy:
 *    MapFile  - abs path to the mapserver configuration file
 *    MapServ  - abs path to the mapserver executable
 *
 */

#include "soap_proxy.h"
#include "sp_svc.h"

#include <axutil_param.h>

/**
 * Names of the parameters in in the config file
 */
#define SP_MAPFILE   "MapFile"
#define SP_MAPSERVER "MapServ"

//-----------------------------------------------------------------------------
static int rp_props_loaded = 0;
static axis2_char_t rp_mapfile[SP_MAX_MPATHS_LEN];
static axis2_char_t rp_mapserv[SP_MAX_MPATHS_LEN];

// =========================  local functions = ===============================
//-----------------------------------------------------------------------------
static int rp_load_prop(
	    const axutil_env_t    *env,
	    const axis2_msg_ctx_t *msg_ctx,
	    axis2_char_t          *dest,
	    const axis2_char_t    *name
)
{
    axutil_param_t *param = NULL;

    param = axis2_msg_ctx_get_parameter(msg_ctx, env, name);
    if (NULL == param)
    {
    	return 1;
    }
    const axis2_char_t *val = axutil_param_get_value(param, env);
    strncpy(dest, val, SP_MAX_MPATHS_LEN);

    return 0;
}

//-----------------------------------------------------------------------------
static int rp_set_props_loaded()
{
    rp_props_loaded = 1;
    return 0;
}

// =========================  public functions = ===============================
//-----------------------------------------------------------------------------
/** Get mapfile path.
 * @return pointer to the mapfile path as a static string, not a copy.
 */
const axis2_char_t *rp_getMapfile()
{
    return rp_get_prop_s(SP_MAPFILE_I);
}

//-----------------------------------------------------------------------------
/** Get mapserver executable path.
 * @return pointer to the mapserver executable path as a static string,
 * not a copy.
 */
const axis2_char_t *rp_getMapserverExec()
{
	return rp_get_prop_s(SP_MAPSERVER_I);
}

//-----------------------------------------------------------------------------
/** Get property by index.
 * @param i
 *   0: mapfile path,
 *   1: mapserver executable path
 * @return pointer to a static string, not a copy.
 */
const axis2_char_t *rp_get_prop_s(
    int i)
{
	return (0==i) ? rp_mapfile : rp_mapserv ;
}

//-----------------------------------------------------------------------------
/**  Caches the WCS-SOAP-To-POST specific properties, which have been read
 * by the axis2 framework on start-up from one of the config files
 * (e.g. 'services.xml' in the service dir).
 * @param env
 * @param msg_ctx
 * @return 0 on success, non-zero on failure.
 */
int rp_load_props(
    const axutil_env_t    *env,
    const axis2_msg_ctx_t *msg_ctx)
{
    if (rp_props_loaded) return 0;

    return
    		rp_load_prop(env, msg_ctx, rp_mapfile, SP_MAPFILE)   ||
    		rp_load_prop(env, msg_ctx, rp_mapserv, SP_MAPSERVER) ||
    		rp_set_props_loaded();
}



