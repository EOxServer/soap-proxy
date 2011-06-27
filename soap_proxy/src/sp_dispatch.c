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
 * @file sp_dispatch.c
 * 
 */

#include <errno.h>
#include <stdio.h>
#include <string.h>
// XXX #include <axutil_linked_list.h>
#include <axutil_url.h>

#include "soap_proxy.h"
#include "sp_svc.h"

//  ==================== Forward declarations ================================
axiom_node_t *rp_getMsVers(
    const axutil_env_t * env);

axiom_node_t *rp_invokeBackend(
    const axutil_env_t *env,
    axiom_node_t       *node,
    const int          wcs_version);

//-----------------------------------------------------------------------------
axiom_node_t *
rp_dispatch_op(
    const axutil_env_t *env,
    axis2_char_t       *op_name,
    axiom_node_t       *node,
    const int          protocol
    )
{
    axiom_node_t *return_node = NULL;

    if (op_name && axutil_strlen(op_name) < SP_MAX_OP_LEN)
    {
        if (axutil_strcmp(op_name, "DescribeCoverage") == 0 ||
            axutil_strcmp(op_name, "GetCoverage"     ) == 0 )
        {
        	return_node = rp_invokeBackend(env, node, protocol);
        }
        else if ( axutil_strcmp(op_name, "GetCapabilities" ) == 0 )
        {
            return_node = rp_invokeBackend(env, node, protocol);
            rp_inject_soap_cap20(env, return_node);
        }
        /* TODO
        else if ( axutil_strcmp(op_name, "DescribeEOCoverageSet" ) == 0 )
        {
            return_node = rp_invokeBackend(env, node, protocol);
        }
        */
        else if ( axutil_strcmp(op_name, "GetMsVersion" ) == 0 )
        {
        	return_node = rp_getMsVers(env);
        }
        else
        {
        	SP_ERROR(env, SP_USER_ERR_BAD_OP);
        	// return_node remains as NULL
        }
    }
    else
    {
    	SP_ERROR(env, SP_USER_ERR_BAD_REQ);
    	// return_node remains as NULL
    }

    return return_node;
}

//-----------------------------------------------------------------------------
axiom_node_t *
rp_invokeBackend(
    const axutil_env_t *env,
    axiom_node_t       *node,
    const int           wcs_version)
{
    AXIS2_ENV_CHECK(env, NULL);

    axiom_node_t *return_node  = NULL;
    axis2_char_t *req_string   = axiom_node_to_string(node, env);
    unsigned long resp_len     = 0;
    axis2_char_t *response_buf = NULL;
    int           resp_fd      = -1;

	const axis2_char_t *mapfile     = NULL;
	const axis2_char_t *msexec      = NULL;
	const axutil_url_t *backend_url = NULL;

    if (rp_getUrlMode())
    {
    	backend_url = rp_getBackendURL();
    	if (NULL == backend_url)
    	{
    		SP_ERROR(env, SP_SYS_ERR_MS_EXEC);
    		rp_log_error(env, "(%s:%d)rp_invokeBackend-0, backendUrl=NULL\n",
    				__FILE__, __LINE__);
    	}
    	else
    	{
    		resp_fd = rp_backend_socket(env, req_string, backend_url);
    	}
    }
    else
    {
    	mapfile = rp_getMapfile();
    	msexec  = rp_getMapserverExec();

    	if (NULL == msexec || '\0' == msexec[0])
    	{
    		SP_ERROR(env, SP_SYS_ERR_MS_EXEC);
    		rp_log_error(env,
    				"(%s:%d)rp_invokeBackend-1, msexec=NULL or empty\n",
    				__FILE__, __LINE__);
    	}
    	else
    	{
    		resp_fd = rp_execMapserv(env, req_string, mapfile, msexec);
    	}
    }

    if (resp_fd < 0)
    {
    	SP_ERROR(env, SP_SYS_ERR_MS_EXEC);
    	if (rp_getUrlMode())
    	{
    		rp_log_error(env, " (%s:%d) rp_invokeBackend-2u, url='%s'\n",
    				__FILE__, __LINE__, rp_get_prop_s(SP_BACKENDURL_ID));
    	}
    	else
    	{
    		rp_log_error(env,
    				"(%s:%d)rp_invokeBackend-2e, msexec ='%s'\n"
    				"                            mapfile='%s'\n",
    				__FILE__, __LINE__, msexec, mapfile);


    	}
    }
    else
    {
        FILE *fp = fdopen(resp_fd, "r");
        if (NULL == fp)
        {
        	int e = errno;
        	SP_ERROR(env, SP_SYS_ERR_MS_OUT_PROCESSING);
        	rp_log_error(env,
        			"(%s:%d)Unexpected error processing Mapserver output,"
            		" fdopen() error %d\n",
            		__FILE__, __LINE__, e);
        }
        else
        {
        	switch(wcs_version)
        	{
        	case SP_WCS_V200:
        		return_node = rp_build_response20(env, fp);
        		break;
        	default:
        		return_node = NULL;
            	SP_ERROR(env, SP_SYS_ERR_INTERNAL);
            	rp_log_error(env,
                		"(%s:%d)Unexpected wcs_version (%d) in switch.\n"
                		__FILE__, __LINE__, wcs_version);
        	}

        }
        fclose(fp);
        if (rp_getUrlMode()) rp_close_sock(resp_fd);
    }

    AXIS2_FREE(env->allocator, req_string);

    return return_node;
}
