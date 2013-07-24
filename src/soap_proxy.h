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
 * @file soap_proxy.h
 *
 */

#ifndef SOAPPROXY_H_INCLUDED
#define SOAPPROXY_H_INCLUDED

#include "sp_constants.h"
#include "sp_svc.h"
#include "sp_props.h"
#include <stdarg.h>

/**
 * WCS Version identifiers (int)
 */
enum sp_wcs_version_ids
{
	SP_WCS_V200 = 200,
	SP_EOWCS_V100 = 1100
};

/* ------------------------------- http header related ---------------*/

/**
* max allowable len of http boundary separator is 70 chars.
*/
#define SP_HTTP_BOUNDLEN  70

/**
* Indices into httpHeaderVal_struct.values and rp_httpHeaderKeys
*/
#define SP_HH_CONTENTTYPE  0
#define SP_HH_DESCRIPTION  1
#define SP_HH_ID           2
#define SP_HH_XFERENCODING 3

#define SP_HH_NKEYS        4


// Holds values of the headers, i.e. the rest of each line following
// the header keyword.
//
struct httpHeaderVal_struct
{
    char *values[SP_HH_NKEYS];
};

typedef struct httpHeaderVal_struct hh_values;

/* -----------  support for axiom_xml_reader_create_for_io() ----------*/
struct rp_cb_ctx_struct {

  const axutil_env_t *env;

  // file to read from - only one of fp or st is used.
  FILE *fp;

  // stream to read from, if file is not used.
  axutil_stream_t *st;

  // boundary string
  const char *bound;

  // 0 if more input is available
  int  done;

  // Used to hold read-ahead input between invocations of rp_fill_buff_CB.
  // This will occur when a few characters of input are the same as the
  // start of the boundary string, but in the end do not match the boundary
  // string.  Such situations should occur rarely.
  char buf[SP_HTTP_BOUNDLEN+4];   // MAXLEN + '--' + '\n\0'
};

typedef struct rp_cb_ctx_struct Rp_cb_ctx;


/* -------------------------- Name-value pairs ----------*/
struct name_value_struct {
  const axis2_char_t *name;
  const axis2_char_t *value;
};

typedef struct name_value_struct Name_value;


/* -------------------------- Fault Generation ----------*/

/**
  * Soap Proxy Errors.
  */
#define SP_ERROR(env,code) \
	AXIS2_ERROR_SET(env->error, code, AXIS2_FAILURE)

enum sp_error_codes
{
	 SP_USER_ERR_NO_INPUT = USER_ERROR_CODES_START,
	 SP_USER_ERR_BAD_OP,
	 SP_USER_ERR_BAD_REQ,
	 SP_USER_ERR_IMAGE_FAILED,
	 SP_USER_ERR_DATA_LOAD,
	 SP_USER_ERR_E0_PARSE_MS_OUT,
	 SP_USER_ERR_E1_PARSE_MS_OUT,
	 SP_USER_ERR_EMPTY_XML,
	 SP_USER_ERR_NO_HASHMATCH,
	 SP_USER_ERR_CONTENTTYPE,
	 SP_USER_ERR_CONTENTHEADERS,

	 SP_SYS_ERR_INTERNAL,
	 SP_SYS_ERR_MS_EXEC,
	 SP_SYS_ERR_MS_OUT_PROCESSING,
	 SP_SYS_ERR_PROPSLOAD,
	 SP_SYS_ERR_NOT_IMPLEMENTED
};

/* ---------------------- forward / external declarations ----------*/
axiom_node_t *rp_error_elem(
    const axutil_env_t * env,
    const axis2_char_t * errorText);

int rp_log_error(
    const axutil_env_t *env,
    const char         *format,
    ...);

void sp_dump_bad_content(
    const axutil_env_t *env,
    char               *contentTypeStr,
    axutil_stream_t    *st,
    char               *header_blob);

int rp_get_contentType(char *str);
int rp_content_is_text_type(char *str);

axiom_node_t *
sp_process_xml_st(
    const axutil_env_t *env,
    axutil_stream_t    *st,
    const char         *boundId);

axiom_node_t *
rp_process_xml(
    const axutil_env_t * env,
    FILE *fp,
    const char *boundId);

int sp_execMs_dashV(
    const axutil_env_t *env,
    const axis2_char_t *msexec);

axutil_stream_t *sp_execMapserv(
    const axutil_env_t * env,
    const sp_props     *props,
    const axis2_char_t *req,
    const axis2_char_t *mapfile);

axutil_stream_t *sp_backend_socket(
    const axutil_env_t *env,
    const sp_props     *props,
    const axis2_char_t *req,
    const axis2_char_t *mapfile);

void sp_stream_cleanup(
    const axutil_env_t *env,
    axutil_stream_t    *sstream);

char *skipChars(
     char * str,
     const char *chars);

char *skipBlanks(
    char * str);

char *getNextToken(
    int *len,
    char *dest,
    char *str);

void sp_initHttpHeaderStruct(
    hh_values *hh);

void sp_freeHttpHeaders(
    const axutil_env_t *env,
    hh_values *hh);

void sp_parseHttpHeaders_fp(
    const axutil_env_t *env,
    hh_values          *hh,
    FILE               *fp);

void sp_parseHttpHeaders_buf(
    const axutil_env_t *env,
    hh_values          *hh,
    char               *buf);

void sp_printHttpHeaders(
    FILE      *fp,
    hh_values *hh);

int seek_to_boundary(
    FILE *fp,
    char *boundId);

int check_end_bound(
    FILE *fp);

char * rp_read_bin_mime_image(
    const axutil_env_t * env,
    FILE       *fp,
    const char *boundId,
    int        *len);

char *sp_load_binary_file(
    const axutil_env_t *env,
    char               *header_blob,
    axutil_stream_t    *st,
    int                *len);

char * rp_load_binary_file(
    const axutil_env_t *env,
    FILE *fp,
    int *len);

const axis2_char_t *sp_get_text_el(
		axiom_node_t       *el_node,
		const axutil_env_t *env);

const axis2_char_t *sp_get_text_text(
		axiom_node_t       *text_node,
		const axutil_env_t *env);

axiom_node_t *
sp_get_last_text_node(
    axiom_node_t       *node,
    const axutil_env_t *env);

axiom_node_t *rp_find_named_node(
    const axutil_env_t *env,
    axiom_node_t       *root_node,
    const axis2_char_t *local_name,
    int                 recurse);

axiom_node_t *rp_find_named_child(
    const axutil_env_t *env,
    axiom_node_t       *root_node,
    const axis2_char_t *local_name,
    int                 recurse);

void rp_delete_named_child(
	const axutil_env_t *env,
	axiom_node_t       *root_node,
	const axis2_char_t *local_name);

axiom_namespace_t *sp_find_or_create_ns(
    const axutil_env_t *env,
    axiom_node_t *node,
    const axis2_char_t *uri,
    const axis2_char_t *prefix);

axiom_namespace_t * rp_get_namespace(
    const axutil_env_t *env,
    axiom_node_t *node);

axiom_node_t *rp_add_sibbling(
    const axutil_env_t *env,
    axiom_node_t       *root_node,
    Name_value         *node_id,
    Name_value         *attribute,
    const axis2_char_t *whitespace);

axiom_node_t *rp_add_child(
    const axutil_env_t *env,
    axiom_node_t       *root_node,
    Name_value         *node_id,
    Name_value         *attribute,
    const axis2_char_t *whitespace);

axiom_node_t *rp_add_child_el(
    const axutil_env_t *env,
    axiom_node_t       *root_node,
    const axis2_char_t *element_name,
    const axis2_char_t *whitespace);

int sp_func_at_nodes(
    const axutil_env_t * env,
    axiom_node_t *root_node,
    const axis2_char_t *local_name,
    int (* func)(const axutil_env_t * env, axiom_node_t *node, void *arg3),
    void *func_arg
    );

void sp_add_whspace(
	const axutil_env_t *env,
    axiom_node_t       *root_node,
    const axis2_char_t *whitespace);

axiom_node_t *sp_latest_named(
    const axutil_env_t *env,
    axiom_node_t       *root_node,
    const axis2_char_t *local_name,
    time_t             *node_time);

axis2_char_t *rp_get_ref_href(
    const axutil_env_t   *env,
    axiom_node_t         *node);

axiom_node_t *
sp_build_response20(
    const axutil_env_t * env,
    const sp_props     *props, 
    axutil_stream_t    *st);

void rp_inject_soap_cap20(
    const axutil_env_t * env,
    const sp_props     *props,
    axiom_node_t *r_node);

void sp_add_soapurl(
    const axutil_env_t * env,
    const sp_props     *props, 
    axiom_node_t *r_node);

void rp_delete_nonsoap(
    const axutil_env_t * env,
    axiom_node_t *r_node);

void sp_update_lineage(
    const axutil_env_t * env,
    const sp_props     *props,
    axiom_node_t *return_node,
    axiom_node_t *request_node,
    time_t request_time);

void rp_print_coverage_hash_entries(
    const axutil_env_t *env,
    FILE               *fp,
    axutil_hash_t      *ch);

char* sp_stream_getline(
	axutil_stream_t    *st,
	const axutil_env_t *env,
	char               *buf,
	unsigned int       size,
	const int          delete_cr);

time_t sp_parse_time_str(const axis2_char_t *time_str);

#endif
