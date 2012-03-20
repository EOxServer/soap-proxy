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
 * See sp_svc.c for general comments.
 *
 */

/**
 * @file sp_constants.h
 *
 */

#ifndef SPCONSTANTS_H_INCLUDED
#define SPCONSTANTS_H_INCLUDED

#include "sp_svc.h"

#include <stdarg.h>

/**
 * limit the operation name len to prevent buffer overrrun-type attacks
 */
#define SP_MAX_OP_LEN 300

/**
 * Whitespace indent for xml formatting
 */
#define SP_DEFAULT_WHSPACE 4

/**
 * limit the max length of incoming requests
 */
#define SP_MAX_REQ_LEN 536870910

/**
 * limit the max length of the mapfile mapserv path strings
 */
#define SP_MAX_MPATHS_LEN 4096

#define SP_IMG_BUF_SIZE 4096

#define SP_MAX_LOCAL_STR_LEN 512

#define MAPSERV_ID_STR "mapserv"

#define SP_WCS_SOAP_EXTENSION \
	"http://www.opengis.net/spec/WCS_protocol-binding_soap/1.0"

#define SP_OWS_NAMESPACE_STR \
	"http://www.opengis.net/ows/2.0"

#define SP_GML_NAMESPACE_STR \
	"http://www.opengis.net/gml/3.2"

#define SP_XLINK_NAMESPACE_STR \
	"http://www.w3.org/1999/xlink"

#define SP_WCSPROXY_NAMESPACE_STR \
	"http://www.eoxserver.org/soap_proxy/wcsProxy"

#define SP_EO_WCS_SOAP_PROFILE \
	"http://www.opengis.net/spec/WCS_application-profile_earth-observation/1.0/conf/eowcs_soap"

#define SP_EO_WCS_PROFILE_ROOT \
	"http://www.opengis.net/spec/WCS_application-profile_earth-observation"

#define SP_BUF_READSIZE   4096

// Max acceptable time diff in seconds.  If greater, lineage is not changed.
#define SP_LINEAGE_TIME_DIFF 360

/* -------------------------------misc constants ----------------------*/
#define SP_RESP_XML_TYPE       0
#define SP_RESP_MIXED_TYPE     2
#define SP_RESP_TIFF_TYPE      3
#define SP_RESP_APP_SEXML_TYPE 4
#define SP_RESP_UNKNOWN_TYPE  -1


#endif
