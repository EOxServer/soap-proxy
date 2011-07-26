/*
 * Soap Proxy Implementation.
 *
 * Common parts of the soap service handling.
 *
 * Author: Milan Novacek, ANF DATA, Nov. 2010.
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
 */

/**
 * @file sp_svc.c
 *
 * Limitations:
 *
 *  - Handles only WCS 2.0  Some code to handle WCS 1.1 exists, but it is not
 *     complete and as implemented the output for WCS 1.1 may not be fully
 *     conforming to the OGC WCS standard.
 *
 *  - Assumes linux line terminators ('\n').
 *    If used in an environment where mapserver produces other line terminators
 *    it may be necessary to convert these to linux when invoking mapserver.
 *
 * Licensing notes:
 *
 * Based on the 'hello service' template from the
 *  axis2/c tutorial.
 *
 * The axis2/c tutorial is Copyright 2004,2005 The Apache Software Foundation,
 * and is licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 * 
 */

#include <stdio.h>
#include <axis2_options.h>
#include <axis2_conf_ctx.h>
#include "sp_svc.h"
#include "soap_proxy.h"
#include <axis2_svc_skeleton.h>

void rp_init_errors();
int  rp_load_props(
		const axutil_env_t    *env,
	    const axis2_msg_ctx_t *msg_ctx);

axiom_node_t *rp_dispatch_op(
    const axutil_env_t *env,
    axis2_char_t       *op_name,
    axiom_node_t       *node,
    const int           protocol);


//-----------------------------------------------------------------------------
static const axis2_svc_skeleton_ops_t rpSvc_svc_skeleton_ops_var = {
    rpSvc_init,
    rpSvc_invoke,
    rpSvc_on_fault,
    rpSvc_free
};

//-----------------------------------------------------------------------------
axis2_svc_skeleton_t *
axis2_rpSvc_create(
    const axutil_env_t * env)
{
    axis2_svc_skeleton_t *svc_skeleton = NULL;
    svc_skeleton = AXIS2_MALLOC(env->allocator, sizeof(axis2_svc_skeleton_t));

    svc_skeleton->ops = &rpSvc_svc_skeleton_ops_var;

    svc_skeleton->func_array = NULL;

    return svc_skeleton;
}

//-----------------------------------------------------------------------------
int AXIS2_CALL
rpSvc_init(
    axis2_svc_skeleton_t * svc_skeleton,
    const axutil_env_t * env)
{
    svc_skeleton->func_array = axutil_array_list_create(env, 0);
    axutil_array_list_add(svc_skeleton->func_array, env, "Post-To-Soap");

    return AXIS2_SUCCESS;
}

//-----------------------------------------------------------------------------
/**
 * This method invokes the right service method
 */
axiom_node_t *AXIS2_CALL
rpSvc_invoke(
		axis2_svc_skeleton_t * svc_skeleton,
		const axutil_env_t * env,
		axiom_node_t * node,
		axis2_msg_ctx_t * msg_ctx)
{
	axiom_node_t *rt_node = NULL;

    rp_init_errors();
    if (rp_load_props(env, msg_ctx))
    {
    	SP_ERROR(env, SP_SYS_ERR_PROPSLOAD);
    	rp_log_error(env, "*** S2P: Failed to load properties.\n");
    	return NULL;
    }

	if (node)
	{

		if (axiom_node_get_node_type(node, env) == AXIOM_ELEMENT)
		{
	    	const int  protocol = sp_glean_protocol(env, node);
	    	SP_WCS_V200;
	    	/* WCS-EO:
	    	 * Requirement 30 /req/eowcs/getCapabilities-response-conformance-class-in-profile:
	         * A WCS service implementing this extension shall include the following URI in a Profile
	         * element in the ServiceIdentification in a GetCapabilities response:
	         *    http://www.opengis.net/spec/WCS_profile_earth-observation/1.0/conf/eowcs
	    	 *
	    	 */

			axiom_element_t *el =
					(axiom_element_t *) axiom_node_get_data_element(node, env);
			if (el)
			{
				axis2_char_t *op_name = axiom_element_get_localname(el, env);
				rt_node = rp_dispatch_op(env, op_name, node, protocol);
				return rt_node;
			}
		}
		else
		{
			rp_log_error(env, "*** S2P: invalid XML in request\n");
			SP_ERROR(env, AXIS2_ERROR_SVC_SKEL_INVALID_XML_FORMAT_IN_REQUEST);
			return NULL;
		}
	}

	// else
	SP_ERROR(env, SP_USER_ERR_NO_INPUT);
	return NULL;

}

//-----------------------------------------------------------------------------
axiom_node_t *AXIS2_CALL
rpSvc_on_fault(
    axis2_svc_skeleton_t * svc_skeli,
    const axutil_env_t * env,
    axiom_node_t * node)
{
    axiom_node_t    *error_node = NULL;
    axiom_element_t *error_ele  = NULL;
    error_ele = axiom_element_create(env, node, "S2PServiceError", NULL,
    		&error_node);
    axiom_element_set_text(error_ele, env, "Soap-to-post service failed",
    		error_node);
    return error_node;
}

//-----------------------------------------------------------------------------
int AXIS2_CALL
rpSvc_free(
    axis2_svc_skeleton_t * svc_skeleton,
    const axutil_env_t * env)
{
    if (svc_skeleton->func_array)
    {
        axutil_array_list_free(svc_skeleton->func_array, env);
        svc_skeleton->func_array = NULL;
    }

    if (svc_skeleton)
    {
        AXIS2_FREE(env->allocator, svc_skeleton);
        svc_skeleton = NULL;
    }

    return AXIS2_SUCCESS;
}

//-----------------------------------------------------------------------------
AXIS2_EXPORT int
axis2_get_instance(
    axis2_svc_skeleton_t ** inst,
    const axutil_env_t * env)
{
    *inst = axis2_rpSvc_create(env);
    if (!(*inst))
    {
        return AXIS2_FAILURE;
    }

    return AXIS2_SUCCESS;
}

//-----------------------------------------------------------------------------
AXIS2_EXPORT int
axis2_remove_instance(
    axis2_svc_skeleton_t * inst,
    const axutil_env_t * env)
{
    axis2_status_t status = AXIS2_FAILURE;
    if (inst)
    {
        status = AXIS2_SVC_SKELETON_FREE(inst, env);
    }
    return status;
}
