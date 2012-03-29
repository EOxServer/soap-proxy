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
 *  For Eoxserver At lest the 'BackendURL' parameters is required.
 *  
 *  Alternatively, for direct use with mapserver, the following
 *  two parameters may be set instead.  Note if BackendURL is set,
 *  then these two are ignored.  If no BackendURL is set, then
 *  both of these are required.
 *    MapFile  - abs path to the mapserver configuration file
 *    MapServ  - abs path to the mapserver executable
 *
 */

#include "soap_proxy.h"
#include "sp_props.h"
#include "sp_svc.h"

#include <axutil_param.h>

// =========================  local functions = ===============================

//-----------------------------------------------------------------------------
/** Load a property.
 * @param env
 * @param msg_ctx
 * @param dest
 * @param name
 * @return 0 on success, 1 on failure.
 */
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
/** Load a property with a boolean value.
 * @param env
 * @param msg_ctx
 * @param name
 * @return 1 on success and if property==true,
 *         0 on failure, non-existent property, or anything other than 'true'.
 */
static int rp_load_boolean(
	    const axutil_env_t    *env,
	    const axis2_msg_ctx_t *msg_ctx,
	    const axis2_char_t    *name
)
{
    axutil_param_t *param = NULL;

    param = axis2_msg_ctx_get_parameter(msg_ctx, env, name);
    if (NULL == param)
    {
    	return 0;
    }
    const axis2_char_t *val = axutil_param_get_value(param, env);
    return  ! axutil_strcasecmp(val, "true");

}

//-----------------------------------------------------------------------------
// copy src to dst, and free src.
static int rp_load_axis_str(
	axis2_char_t        *dst,
	axis2_char_t        *src,
	const int           max_chars,
	const axutil_env_t  *env)
{
    strncpy(dst, src, max_chars);
    AXIS2_FREE(env->allocator, src);
    return 0;
}

// =========================  public functions = ===============================
//-----------------------------------------------------------------------------
/** init props
 * @param props
 * 
 */
void  rp_init_props(sp_props *props)
{
    props->msg_ctx = NULL;

    props->url_mode         = 0;
    props->deleting_nonsoap = 0;
    props->debug_mode       = 0;
    props->backend_port     = -1;

    props->mapfile         [0] = '\0';
    props->mapserv         [0] = '\0';
    props->backend_url_str [0] = '\0';
    props->soapops_url_str [0] = '\0';
    props->backend_host    [0] = '\0';
    props->backend_path    [0] = '\0';
}

//-----------------------------------------------------------------------------
/** Get url mode.
 * @param env
 * @param props
 * @return false (0): not URL-mode, use exec of mapserver binary,
 *         true  (1): communicate with URL via a socket connection.
 */
const int rp_getUrlMode( const axutil_env_t *env, const sp_props *props )
{
    return props->url_mode;
}

//-----------------------------------------------------------------------------
/** Get Debug mode.
 * @param env
 * @param props
 * @return false (0): debug off.
 *         true  (1): debug on.
 */
const int rp_getDebugMode(const axutil_env_t *env, const sp_props *props )
{
    return props->debug_mode;
}

//-----------------------------------------------------------------------------
/** Get DeleteNonSoapURLs mode.
 * @param env
 * @param props
 * @return false (0): to keep all GET & POST capabilities as advertised by the
 *  backend,
 *         true  (1): delete GET & POST capabilities coming from the backend.
 */
const int rp_getDeletingNonSoap( const axutil_env_t *env, const sp_props *props )
{
    return props->deleting_nonsoap;
}

//-----------------------------------------------------------------------------
/** Get mapfile path.
 * @param env
 * @param props
 * @return pointer to the mapfile path as a static string, not a copy.
 */
const axis2_char_t *rp_getMapfile( const axutil_env_t *env, const sp_props *props )
{
    return props->mapfile;
}

//-----------------------------------------------------------------------------
/** Get mapserver executable path.
 * @param env
 * @param props
 * @return pointer to the mapserver executable path as a static string,
 * not a copy.
 */
const axis2_char_t *rp_getMapserverExec( const axutil_env_t *env, const sp_props *props )
{
	return props->mapserv;
}

//-----------------------------------------------------------------------------
/** Get SOAPOperationsURL string.
 * @param env
 * @param props
 * @return pointer to a static string, not a copy.
 */
const axis2_char_t *rp_getSoapOpsURL( const axutil_env_t *env, const sp_props *props )
{
        return props->soapops_url_str;
}

//-----------------------------------------------------------------------------
/** Get backend URL string.
 * @param env
 * @param props
 * @return pointer to a string, not a copy.
 */
const axis2_char_t *rp_getBackendURL( const axutil_env_t *env, const sp_props *props )
{
        return props->backend_url_str;
}

//-----------------------------------------------------------------------------
/** Get backend path.
 * @param env
 * @param props
 * @return pointer to a string, not a copy.
 */
const axis2_char_t *rp_getBackendPath( const axutil_env_t *env, const sp_props *props )
{
	return props->backend_path;
}

//-----------------------------------------------------------------------------
/** Get backend port.
 * @param env
 * @param props
 * @return pointer to a string, not a copy.
 */
const int rp_getBackendPort( const axutil_env_t *env, const sp_props *props )
{
	return props->backend_port;
}

//-----------------------------------------------------------------------------
/** Get backend host.
 * @param env
 * @param props
 * @return pointer to a string, not a copy.
 */
const axis2_char_t *rp_getBackendHost( const axutil_env_t *env, const sp_props *props )
{
	return props->backend_host;
}

//-----------------------------------------------------------------------------
/**  Loads the WCS-SOAP-To-POST specific properties, which have been read
 * by the axis2 framework on start-up from one of the config files
 * (e.g. 'services.xml' in the service dir).
 * @param props
 * @param env
 * @param msg_ctx
 * @return 0 on success, non-zero on failure.
 */
int rp_load_props(
    sp_props *props,
    const axutil_env_t    *env,
    const axis2_msg_ctx_t *msg_ctx)
{

    props->msg_ctx = msg_ctx;

    props->debug_mode       = rp_load_boolean(env, msg_ctx, SP_DEBUG_STR);
    props->deleting_nonsoap = rp_load_boolean(env, msg_ctx, SP_DELNONSOAP_STR);

    if ( rp_load_prop(env, msg_ctx, props->soapops_url_str, SP_SOAPOPSURL_STR) )
    {
        // Try get the endpoint URL.
        // Not sure why this in the 'from' rather than the 'to'. (TODO)

        axis2_endpoint_ref_t *xaddr = axis2_msg_ctx_get_from (msg_ctx, env);
        if (NULL==xaddr)
        {
            rp_log_error(env,
                    " SP: **WARNING: NULL==xaddr."
                    " Could not determine URL of service.\n");
            strcpy(props->soapops_url_str, "ERROR: URL-UNKNOWN");
        }
        else
        {
            if (strlen(axis2_endpoint_ref_get_address(xaddr, env))
                    > SP_MAX_MPATHS_LEN)
            {
                rp_log_error(env,
                        " SP: **WARNING: xaddr exceeds %d \n",
                        SP_MAX_MPATHS_LEN);
            }
            strncpy(props->soapops_url_str,
                    axis2_endpoint_ref_get_address(xaddr, env),
                    SP_MAX_MPATHS_LEN);
        }
    }

    // Must load at least one of BACKENDURL or MAPSERVER.
    // If loading MAPSERVER then must also load MAPFILE.
    // In case of BACKENDURL we don't know if we're running mapserver
    // or eoxserver, so the MAPFILE is not mandatory - but things will
    // fail downstream if the user attempts to send requests to mapserver
    // without a mapfile.

    int mapfile_loaded = ! rp_load_prop(env, msg_ctx, props->mapfile, SP_MAPFILE_STR);

    if ( ! rp_load_prop(env, msg_ctx, props->backend_url_str, SP_BACKENDURL_STR))
    {
        props->url_mode = 0;

        axutil_url_t *backend_url = axutil_url_parse_string(env, props->backend_url_str);
        if (!backend_url)
        {
            rp_log_error(env, "Malformed " SP_BACKENDURL_STR ".");
            return -1;
        }

        props->backend_port = axutil_url_get_port(backend_url, env);
        rp_load_axis_str(
                props->backend_host,
                axutil_url_get_host(backend_url, env),
                SP_MAX_MPATHS_LEN,
                env);
        rp_load_axis_str(
                props->backend_path,
                axutil_url_get_path(backend_url, env),
                SP_MAX_MPATHS_LEN,
                env);

        axutil_url_free(backend_url, env);

        props->url_mode = 1;
        return 0;
    }
    else
    {
        props->url_mode = 0;
        if ( ! mapfile_loaded ) return -1;
        return rp_load_prop(env, msg_ctx, props->mapserv, SP_MAPSERVER_STR);
    }
}



