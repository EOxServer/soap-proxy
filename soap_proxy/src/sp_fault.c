/*
 *
 * Soap Proxy.
 *
 * Fork, and execute mapserver.
 *
 * Milan Novacek, ANF DATA, Apr. 2011.
 *
 * Copyright (c) 2011, ANF DATA Spol. s r.o.
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
 * @file sp_fault.c
 * 
 */

#include <stdarg.h>
#include <axutil_error.h>
#include "soap_proxy.h"

#define SP_SELF_ID_STRING "soap_proxy"

static int rp_errors_initialized = 0;

extern const axis2_char_t* axutil_error_messages[];

void rp_init_errors()
{
    if (rp_errors_initialized) return;

	axutil_error_messages[SP_USER_ERR_NO_INPUT] =
			"S2P service ERROR: No input found.";
	axutil_error_messages[SP_USER_ERR_BAD_OP] =
			"Unrecognized Operation";
	axutil_error_messages[SP_USER_ERR_BAD_REQ] =
			"Malformed Request";
	axutil_error_messages[SP_USER_ERR_IMAGE_FAILED] =
			"Internal Error: Failed to read image";
	axutil_error_messages[SP_USER_ERR_DATA_LOAD] =
			"Internal Error: Failed to load response.";
	axutil_error_messages[SP_USER_ERR_E0_PARSE_MS_OUT] =
			"Error E0 parsing mapserver output.";
	axutil_error_messages[SP_USER_ERR_E1_PARSE_MS_OUT] =
			"Error E1 parsing mapserver output (multipart/mixed).";
	axutil_error_messages[SP_USER_ERR_EMPTY_XML] =
			"No XML found when expected, or bad XML in multipart/mixed response part.";
	axutil_error_messages[SP_USER_ERR_NO_HASHMATCH] =
			"Internal Error: No hash match. See error log.";
	axutil_error_messages[SP_USER_ERR_CONTENTTYPE] =
			"Unrecognised Content-type. See error log.";
	axutil_error_messages[SP_USER_ERR_CONTENTHEADERS] =
			"Error parsing Mapserver response headers";

	axutil_error_messages[SP_SYS_ERR_INTERNAL] =
			"Internal Processing Error";
	axutil_error_messages[SP_SYS_ERR_MS_EXEC] =
			"Failed to execute Mapserver";
	axutil_error_messages[SP_SYS_ERR_MS_OUT_PROCESSING] =
			"Unexpected error processing Mapserver output";
	axutil_error_messages[SP_SYS_ERR_PROPSLOAD] =
			"Failed to load required properties.";
	axutil_error_messages[SP_SYS_ERR_NOT_IMPLEMENTED] =
			"Not Implemented.";

	rp_errors_initialized = 1;
}

//-----------------------------------------------------------------------------
axiom_node_t *
rp_error_elem(
    const axutil_env_t *env,
    axis2_char_t       *errorText)
{
    axiom_node_t    *resp_om_node  = NULL;
    axiom_element_t *resp_om_ele   = NULL;

    resp_om_ele = axiom_element_create(
        env, NULL, "errorResponse", NULL, &resp_om_node);

    axiom_element_set_text(resp_om_ele, env, errorText, resp_om_node);

    return resp_om_node;
}

//-----------------------------------------------------------------------------
axiom_node_t *rp_fault_elem(
    const axutil_env_t *env)
{
    axiom_node_t    *resp_om_node  = NULL;
    axiom_element_t *resp_om_ele   = NULL;

    resp_om_ele = axiom_element_create(
        env, NULL, "errorResponse", NULL, &resp_om_node);

    return resp_om_node;
}

//-----------------------------------------------------------------------------
int rp_log_error(
    const axutil_env_t *env,
    const axis2_char_t *format,
    ...)
{
	va_list args;
	int ret = 0;
	axis2_char_t *buf = NULL;

	int buf_len = strlen(format) + strlen(SP_SELF_ID_STRING) + 9;
	buf = (axis2_char_t *) AXIS2_MALLOC(env->allocator, buf_len);
	sprintf(buf," *** %s: %s", SP_SELF_ID_STRING, format);

	va_start (args, format);
	ret = vfprintf(stderr, (char *)buf, args);
	va_end (args);
	fflush(stderr);

	AXIS2_FREE(env->allocator, buf);
	return ret;
}

