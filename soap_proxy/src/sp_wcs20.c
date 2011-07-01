/*
 * SOAP-to-POST proxy
 *
 * Milan Novacek, ANF DATA, June 2011.
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
 */

/**
 * @file sp_wcs20.c
 * 
 */

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <axutil_linked_list.h>

#include "soap_proxy.h"

//-----------------------------------------------------------------------------
/** f_add_PostEncodingSOAP
 * Add the element
 *      <ns:Value>SOAP</ns:Value>
 *  at all nodes
 *    Constraint[@name="PostEncoding"]/AllowedValues
 *  (See OGC 09-148r1 and OGC 09-149r1).
 *
 * This function is invoked via rp_func_at_nodes() from rp_inject_soap_cap20()
 */
static int f_add_PostEncodingSOAP(
    const axutil_env_t * env,
    axiom_node_t *target_node,
    void *arg2
 )
{
    const axis2_char_t *constrIdStr = "Constraint";

    axiom_node_t *constraint_node =
      rp_find_named_node(env,
                         axiom_node_get_first_child (target_node, env),
                         constrIdStr,
                         1);
    if (NULL == constraint_node)
    {
    	fprintf(stderr, "*** S2P(%s:%d): %s node not found.\n",
    			__FILE__, __LINE__,  constrIdStr);
    	fflush(stderr);
    	return 1;
    }

    // check that constr_node is "PostEncoding", otherwise try to find it

    axiom_element_t *el = axiom_node_get_data_element(constraint_node, env);
    const axis2_char_t *nameAttr = axiom_element_get_attribute_value_by_name
    		(el, env, "name");
    while (nameAttr == NULL ||  axutil_strncmp(nameAttr, "PostEncoding", 12) )
    {
    	// loop through siblings trying find the "PostEncoding" constraint
    	constraint_node = axiom_node_get_next_sibling (constraint_node, env);
    	el = axiom_node_get_data_element(constraint_node, env);
    	if (axiom_node_get_node_type(constraint_node, env) == AXIOM_ELEMENT)
    	{
    		nameAttr =
    				axiom_element_get_attribute_value_by_name(el, env, "name");
    	}
    }

    if (nameAttr == NULL ||  axutil_strncmp(nameAttr, "PostEncoding", 12) )
    {
    	// Did not find a "PostEncoding" constraint, bail out.
    	// TODO: Future enhancement: could add a "PostEncoding" constraint
    	//   element.

    	fprintf(stderr,
    			"*** S2P(%s:%d): 'PostEncoding' constraint not found.\n",
    			__FILE__, __LINE__,  constrIdStr);
    	fflush(stderr);
    	return 1;
    }

    axiom_node_t *allowedValues_node =
      rp_find_named_node(env,
                         axiom_node_get_first_child (constraint_node, env),
                         "AllowedValues",
                         1);
    if (NULL == allowedValues_node)
    {
    	// TODO: Future enhancement: insert an "AllowedValues" node.
    	fprintf(stderr,
    			"*** S2P(%s:%d): 'AllowedValues' node not found.\n",
    			__FILE__, __LINE__,  constrIdStr);
    	fflush(stderr);
    	return 1;
    }

    // Insert <ns:Value>SOAP</ns:Value>

    Name_value value_nv_n;
    value_nv_n.name  = "Value";
    value_nv_n.value = "SOAP";
    rp_add_child (env, allowedValues_node, &value_nv_n, NULL);

    return 0;

}

//-----------------------------------------------------------------------------
void rp_inject_soap_cap20(
    const axutil_env_t * env,
    axiom_node_t *r_node)
{

    // First find the node Capabilities/ServiceIdentification
    const axis2_char_t *capIdStr  = "Capabilities";
    const axis2_char_t *svcIdStr  = "ServiceIdentification";
    const axis2_char_t *profIdStr = "Profile";

    axiom_node_t *cap_node = rp_find_named_node(
    		env, r_node, capIdStr, 0);
    if (NULL == cap_node) return;

    axiom_node_t *svc_node =
      rp_find_named_node(env,
    		  axiom_node_get_first_child (cap_node, env),
    		  svcIdStr,
    		  0);
    if (NULL == svc_node)
    {
    	fprintf(stderr, "*** S2P(%s:%d): %s node not found.\n",
    			__FILE__, __LINE__,  svcIdStr);
    	fflush(stderr);
    	return;
    }

    // see if there is a 'profile' node:
    //  if yes, insert the soap extension URI there.
    Name_value ext_nv;
    ext_nv.name  = "Profile";
    ext_nv.value = SP_WCS_SOAP_EXTENSION;
    axiom_node_t *target_node = rp_find_named_node(
    		env,
    		axiom_node_get_first_child (svc_node, env),
    		profIdStr,
    		0);
    (NULL == target_node) ?
    		rp_add_child    (env, svc_node,    &ext_nv, NULL) :
    		rp_add_sibbling (env, target_node, &ext_nv, NULL);

    //
    // Next, for all Operation children of OperationsMetadata find the path
    //   Constraint[@name="PostEncoding"]/AllowedValues
    //   and add
    //      <ns:Value>SOAP</ns:Value>
    //   where ns is the namespace of the enclosing AllowedValues element.
    //

    axiom_node_t *ops_node =
      rp_find_named_node(env,
    		  axiom_node_get_first_child (r_node, env),
    		  "OperationsMetadata",
    		  1);
    if (NULL == ops_node)
    {
    	fprintf(stderr, "*** S2P(%s:%d): %s node not found.\n",
    			__FILE__, __LINE__,  "OperationsMetadata");
    	fflush(stderr);
    	return;
    }

    rp_func_at_nodes(env,
                     axiom_node_get_first_child(ops_node, env),
                     "Operation",
                     &f_add_PostEncodingSOAP,
                     NULL);

}

//-----------------------------------------------------------------------------
axiom_node_t *
sp_make_MTOM_node20(
    const axutil_env_t *env,
    axutil_stream_t    *st,
    char               *header_blob,
    axis2_char_t       *el_name,
    axis2_char_t       *content_type,
    axis2_char_t       *ns_prefix,
    axis2_char_t       *ns_uri)
{
	axiom_node_t         *resp_om_node = NULL;

    int data_len = 0;
    char *bin_data = sp_load_binary_file(env, header_blob, st,  &data_len);

    if (NULL == bin_data)
    {
    	SP_ERROR(env, SP_USER_ERR_DATA_LOAD);
    }
    else
    {
    	axiom_element_t      *resp_om_ele  = NULL;
        axiom_data_handler_t *data_handler = NULL;
        axiom_node_t         *data_om_node = NULL;
        axiom_text_t            *data_text = NULL;

    	axiom_namespace_t *ns = axiom_namespace_create(env, ns_uri, ns_prefix);
    	resp_om_ele =
    			axiom_element_create (env, NULL, el_name, ns, &resp_om_node);
        data_handler =
        		axiom_data_handler_create(env, NULL, content_type);
        axiom_data_handler_set_binary_data
            (data_handler, env, bin_data, data_len);
        data_text =
          axiom_text_create_with_data_handler
          (env, resp_om_node, data_handler, &data_om_node);
        axiom_text_set_optimize(data_text, env, AXIS2_TRUE);
    }

    // Note:  The buffer 'bin_data' gets freed when
    //        axiom_data_handler_free is called, and it in turn gets called
    //        from axiom_text_free.
    //        That should get called when the resp_om_node gets freed.
    //        Hopefully the service framework does this at some point.

    return resp_om_node;
}

//-----------------------------------------------------------------------------
/*
 *   RCF 9 calls for including the entire response as a 'binary'
 *   attachment according to the schema wcsSoapCoverage, which reads:-
 *
    <schema ... xmlns:wcs="http://www.opengis.net/wcs/2.0"  ... >
    <import namespace="http://www.w3.org/2004/08/xop/include"
            schemaLocation="http://www.w3.org/2004/08/xop/include"/>
    <element name="Coverage" type="wcs:SoapCoverageType"/>
    <complexType name="SoapCoverageType">
      <sequence>
	     <element ref="xop:Include"/>
      </sequence>
    </complexType>
    </schema>

 * -------- Snip.
 * This means the entire response should look like this:

    <wcs:Coverage>
      <"xop:Include" xmlns:xop="http://www.w3.org/2004/08/xop/include" .../>
    </wcs:Coverage>
 TODO:  To read the entire file contents into an in-memory buffer might
 not be optimal for large data sets.  Need to investigate how to arrange
 for the axis2 framework to read the file, and delete it after it is done,
 or better yet how to use the open file descriptor and close it once it
 is done.  Perhaps using AXIOM_DATA_HANDLER_TYPE_CALLBACK might help?
 */
//-----------------------------------------------------------------------------
axiom_node_t *
sp_process_coverage20(
    const axutil_env_t *env,
    char               *header_blob,
    axutil_stream_t    *st)
{
    return sp_make_MTOM_node20(
    		env,
    		st,
    		header_blob,
    		"Coverage",
    		"application/coverage",
    		"wcs",
    		"http://www.opengis.net/wcs/2.0");

}
/*
//-----------------------------------------------------------------------------
axiom_node_t *
rp_process_coverage20(
    const axutil_env_t * env,
    char *contentLine,
    FILE *fp)
{
	rewind(fp);
    return rp_make_MTOM_node20(
    		env,
    		fp,
    		"Coverage",
    		"application/coverage",
    		"wcs",
    		"http://www.opengis.net/wcs/2.0");

}
*/
//-----------------------------------------------------------------------------
axiom_node_t *
sp_process_tiff20(
    const axutil_env_t *env,
    axutil_stream_t    *st,
    hh_values *hh)
{
    return sp_make_MTOM_node20(
    		env,
    		st,
    		NULL,
    		"Coverage",
    		hh->values[SP_HH_CONTENTTYPE],
    		"wcs",
    		"http://www.opengis.net/wcs/2.0");
}

//-----------------------------------------------------------------------------
axiom_node_t *
sp_build_response20(
    const axutil_env_t *env,
    axutil_stream_t    *st)
{
    char tmpBuf[255];
    axiom_node_t *return_node = NULL;

    hh_values hh;
    rp_initHttpHeaderStruct(&hh);

    char header_buf[2560];
    if (sp_load_header_blob(env, st, header_buf, 2560) < 0)
    {
    	// TODO:  Could allocate a bigger buffer, copy chars, etc.
    	// In practice we never expect the headers to be that big, or if
    	//  it does come up then something is wrong.
        SP_ERROR(env, SP_USER_ERR_CONTENTHEADERS);
        return NULL;
    }
    sp_parseHttpHeaders_buf(env, &hh, header_buf);

    char *contentTypeStr = hh.values[SP_HH_CONTENTTYPE];
    if ( NULL == contentTypeStr)
    {
        rp_freeHttpHeaders(env, &hh);
        SP_ERROR(env, SP_USER_ERR_CONTENTHEADERS);
        return NULL;
    }
    switch(rp_get_contentType(contentTypeStr))
    {
    case SP_RESP_XML_TYPE:
    case SP_RESP_APP_SEXML_TYPE:
        return_node =  sp_process_xml_st(env, st, NULL);
        break;

    case SP_RESP_MIXED_TYPE:
    	// A mixed type response generally signifies a coverage response.
    	// TODO:  check that we really do have a coverage!
        return_node =  sp_process_coverage20(env, contentTypeStr, st);
        break;

    case SP_RESP_TIFF_TYPE:
        return_node =  sp_process_tiff20(env, st, &hh);
        break;

    default:
    	SP_ERROR(env, SP_USER_ERR_CONTENTTYPE);
    	fprintf(stderr,
    			"*** S2P(%s:%d): Unrecognised Content-type: %s\n",
    			__FILE__, __LINE__,
    			contentTypeStr);
    }

    rp_freeHttpHeaders(env, &hh);
    return return_node;

}

