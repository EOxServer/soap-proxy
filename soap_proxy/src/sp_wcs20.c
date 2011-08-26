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
#include <time.h>

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
* This function may be invoked via sp_func_at_nodes() if needed.
 *
 */
static int f_add_PostEncodingSOAP(
    const axutil_env_t * env,
    axiom_node_t *target_node,
    void *arg3
 )
{
	//
    // Find the path
    //   Constraint[@name="PostEncoding"]/AllowedValues
    //   and add
    //      <ns:Value>SOAP</ns:Value>
    //   where ns is the namespace of the enclosing AllowedValues element.
	//

    const axis2_char_t *constrIdStr = "Constraint";

    axiom_node_t *constraint_node =
    		rp_find_named_child(env, target_node, constrIdStr, 1);
    if (NULL == constraint_node)
    {
    	axiom_element_t *el      = NULL;
    	axis2_char_t    *el_name = NULL;
    	axis2_char_t    *op_name = NULL;
        if (axiom_node_get_node_type(target_node, env) == AXIOM_ELEMENT)
        {
            el = (axiom_element_t *)
              axiom_node_get_data_element (target_node, env);
            el_name = el ?
            		axiom_element_get_localname( el, env) :
            		NULL;
    		op_name = el ?
    				axiom_element_get_attribute_value_by_name(el, env, "name"):
            		NULL;
        }
    	rp_log_error(env,
    			"*** S2P(%s:%d): %s node not found under %s/name='%s'.\n",
    			__FILE__, __LINE__,  constrIdStr,
    			el_name ? el_name : "<UNKNOWN>",
    			op_name ? op_name : "<UNKNOWN>");
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

    	rp_log_error(env,
    			"*** S2P(%s:%d): 'PostEncoding' constraint not found.\n",
    			__FILE__, __LINE__,  constrIdStr);
    	return 1;
    }

    axiom_node_t *allowedValues_node =
      rp_find_named_child(env, constraint_node, "AllowedValues", 1);
    if (NULL == allowedValues_node)
    {
    	// TODO: Future enhancement: insert an "AllowedValues" node.
    	rp_log_error(env,
    			"*** S2P(%s:%d): 'AllowedValues' node not found.\n",
    			__FILE__, __LINE__,  constrIdStr);
    	return 1;
    }

    // Insert <ns:Value>SOAP</ns:Value>

    Name_value value_nv_n;
    value_nv_n.name  = "Value";
    value_nv_n.value = "SOAP";
    rp_add_child (env, allowedValues_node, &value_nv_n, NULL, NULL);

    return 0;
}

//-----------------------------------------------------------------------------
// This function is invoked via sp_func_at_nodes() from rp_delete_nonsoap()
static int f_delete_nonsoap(
	const axutil_env_t * env,
	axiom_node_t *target_node,
	void *arg3)
{
	int n_done = 0;
    axiom_node_t *top_node = rp_find_named_child(env, target_node, "HTTP", 1);
    if (NULL == top_node) return 0;

    rp_delete_named_child(env, top_node, "Get");
    rp_delete_named_child(env, top_node, "Post");

    return 1;
}

//-----------------------------------------------------------------------------
/*
static int rewrite_url_attr(
	const axutil_env_t *env,
	axiom_node_t       *top_node,
	axis2_char_t       *node_name
	)
{
	axiom_node_t *node =  rp_find_named_child(env, top_node,  node_name, 1);
	if (NULL == node) return 0;

    axiom_attribute_t *href_attr = NULL;
    axiom_element_t          *el = NULL;
    axutil_qname_t           *qn = axutil_qname_create
    		(env, "href", "http://www.w3.org/1999/xlink", "xlink");
	if (axiom_node_get_node_type(node, env) == AXIOM_ELEMENT)
	{
		el        = axiom_node_get_data_element(node, env);
		href_attr = axiom_element_get_attribute (el, env, qn);
	}

    if (NULL != href_attr)
    {
    	axiom_attribute_set_value (href_attr, env, rp_getRewriteURL());
    }

    return 0;
}
*/

//-----------------------------------------------------------------------------
// This function is invoked via sp_func_at_nodes() from sp_add_soapurl()
static int f_add_soapurl(
	const axutil_env_t * env,
	axiom_node_t *target_node,
	void *arg3)
{
    //
    // Find the paths
    //   HTTP
    //   and add the SOAP capability.
    // If none found it  is not considered an error.
	//
/*
    <ows:Post xlink:type="simple" xlink:href="http://SERVER_UNDEFINED/service">
      <ows:Constraint name="PostEncoding">
        <ows:AllowedValues>
          <ows:Value>XML</ows:Value>
          <ows:Value>SOAP</ows:Value>
        </ows:AllowedValues>
      </ows:Constraint>
  */

    axiom_node_t *top_node = rp_find_named_child(env, target_node, "HTTP", 1);
    if (NULL == top_node) return 0;

    axiom_namespace_t * root_ns = rp_get_namespace (env, top_node);

    axiom_node_t    *post_node = axiom_node_create(env);
    axiom_element_t *post_ele  =
      axiom_element_create(env, top_node, "Post", root_ns, &post_node);

	axiom_namespace_t *xlink_ns = axiom_namespace_create(
			env, "http://www.w3.org/1999/xlink", "xlink");

    axiom_attribute_t *attr =
      axiom_attribute_create (env, "type", "simple", xlink_ns);
    axiom_element_add_attribute (post_ele, env, attr, post_node);

    attr = axiom_attribute_create (env, "href", rp_getSoapOpsURL(), xlink_ns);
    axiom_element_add_attribute (post_ele, env, attr, post_node);

    Name_value c_nv;  c_nv.name = "Constraint";  c_nv.value = NULL;
    Name_value p_nv;  p_nv.name = "name";        p_nv.value = "PostEncoding";
    axiom_node_t *c_node = rp_add_child(env, post_node, &c_nv, &p_nv, NULL);

    axiom_node_t *a_node = rp_add_child_el(env, c_node, "AllowedValues", NULL);

    c_nv.name  = "Value";
    c_nv.value = "SOAP";
    rp_add_child(env, a_node, &c_nv, NULL, NULL);

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

    axiom_node_t *cap_node = rp_find_named_node (env, r_node, capIdStr, 0);
    if (NULL == cap_node) return;

    axiom_node_t *svc_node = rp_find_named_child(env, cap_node, svcIdStr, 0);
    if (NULL == svc_node)
    {
    	rp_log_error(env, "*** S2P(%s:%d): %s node not found.\n",
    			__FILE__, __LINE__,  svcIdStr);
    	return;
    }

    // see if there is a 'profile' node:
    //  if yes, insert the soap extension URI there.
    axiom_node_t *svc_child_node = axiom_node_get_first_child (svc_node, env);
    axiom_node_t *profile_node =
    		rp_find_named_node(env, svc_child_node, profIdStr, 0);

    Name_value ext_nv;
    ext_nv.name  = "Profile";
    ext_nv.value = SP_WCS_SOAP_EXTENSION;
    axiom_node_t *ext_node = (NULL == profile_node) ?
    		rp_add_child    (env, svc_node,    &ext_nv, NULL, NULL) :
    		rp_add_sibbling (env, profile_node, &ext_nv, NULL, NULL);

    // Add the EO WCS Application Profile for SOAP, but only
    // if another EO WCS profile is already present
    if ( rp_find_node_with_text
    		(env, svc_child_node, profIdStr, 0, SP_EO_WCS_PROFILE_ROOT)
    )
    {
    	ext_nv.value = SP_EO_WCS_SOAP_PROFILE;
    	rp_add_sibbling (env, ext_node, &ext_nv, NULL, NULL);
    }

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
    sp_initHttpHeaderStruct(&hh);

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
        sp_freeHttpHeaders(env, &hh);
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
    	rp_log_error(env,
    			"*** S2P(%s:%d): Unrecognised Content-type: %s\n",
    			__FILE__, __LINE__,
    			contentTypeStr);
    	if (rp_getDebugMode())
    	{
    		sp_dump_bad_content(env, contentTypeStr, st, header_buf);
    	}
    }

    sp_freeHttpHeaders(env, &hh);
    return return_node;

}


//-----------------------------------------------------------------------------
void rp_delete_nonsoap(
    const axutil_env_t * env,
    axiom_node_t *r_node)
{
    //
    // For all Operation children of OperationsMetadata find the paths
    //   HTTP/Get
    //   HTTP/Post
    //   and for each delete the node.
    //

    axiom_node_t *ops_node =
    		rp_find_named_child(env, r_node, "OperationsMetadata", 1);
    if (NULL == ops_node)
    {
    	rp_log_error(env, "*** S2P(%s:%d): %s node not found.\n",
    			__FILE__, __LINE__,  "OperationsMetadata");
    	return;
    }

    sp_func_at_nodes(env,
                     axiom_node_get_first_child(ops_node, env),
                     "Operation",
                     &f_delete_nonsoap,
                     NULL);
}

//-----------------------------------------------------------------------------
void sp_add_soapurl(
    const axutil_env_t * env,
    axiom_node_t *r_node)
{
    //
    // For all Operation children of OperationsMetadata find the path
    //   HTTP/Post
    //   and add our soap-url there.
    //

    axiom_node_t *ops_node =
    		rp_find_named_child(env, r_node, "OperationsMetadata", 1);
    if (NULL == ops_node)
    {
    	rp_log_error(env, "*** S2P(%s:%d): %s node not found.\n",
    			__FILE__, __LINE__,  "OperationsMetadata");
    	return;
    }

    sp_func_at_nodes(env,
                     axiom_node_get_first_child(ops_node, env),
                     "Operation",
                     &f_add_soapurl,
                     NULL);

}

//-----------------------------------------------------------------------------
void
sp_update_lineage(
    const axutil_env_t * env,
    axiom_node_t *return_node,
    axiom_node_t *request_node,
    time_t        request_time)
{
    axiom_node_t *eom_node =
    		rp_find_named_child(env, return_node, "EOMetadata", 1);
    if (NULL == eom_node)
    {
    	rp_log_error(env, "*Warning S2P(%s:%d): %s node not found.\n",
    			__FILE__, __LINE__,  "EOMetadata");
    	return;
    }

    time_t lineage_time = 0;
    axiom_node_t *curr_lineage =
    		sp_latest_named(env, eom_node, "lineage", &lineage_time);

    // TODO: should improve handling of insignificant whitespace.

    // grab some whitespace for future use
    axiom_node_t *lin_whsp_node = sp_get_last_text_node(curr_lineage, env);
    axiom_node_t *eom_whsp_node = sp_get_last_text_node(eom_node,     env);

    const axis2_char_t *lin_whsp_str = sp_get_text_text(lin_whsp_node, env);
    const axis2_char_t *eom_whsp_str = sp_get_text_text(eom_whsp_node, env);
    int whspace_indent = SP_DEFAULT_WHSPACE;
    if (NULL != lin_whsp_str && NULL != eom_whsp_str)
    {
    	whspace_indent =
    			axutil_strlen(lin_whsp_str) - axutil_strlen(eom_whsp_str);
    	if (whspace_indent < 0 || whspace_indent >12)
    	{
    		rp_log_error(env,
    				"*Warning S2P: funny whitespace indent (%d) calculated.\n",
    				whspace_indent);
    		whspace_indent = SP_DEFAULT_WHSPACE;
    	}
    }

    axis2_char_t curr_whspace[SP_MAX_LOCAL_STR_LEN];
    strncpy(curr_whspace, lin_whsp_str, SP_MAX_LOCAL_STR_LEN-1);
    curr_whspace[SP_MAX_LOCAL_STR_LEN-1] = '\0';

    // The most recent Lineage data is deleted only if it has been added
    // in the time period since we started processing this request.
    fprintf(stderr,"l_lt=%lld\nrq_t=%lld\n", lineage_time, request_time);
    if (NULL != curr_lineage && lineage_time >= request_time)
    {
    	// OK to delete lineage
    	axiom_node_detach    (curr_lineage, env);
		axiom_node_free_tree (curr_lineage, env);
		curr_lineage  = NULL;
		lin_whsp_node = NULL;
		lin_whsp_str  = NULL;
    }
    else
    {
    	sp_add_whspace(env, eom_node, curr_whspace);
    }
    // Add a new lineage.
    /*  here is an example:
            <wcseo:lineage>
                <!-- GetCoverage request via POST -->
                <wcseo:referenceGetCoverage>
                    <ows:ServiceReference xlink:href="http://www.someWCS.org">
                        <ows:RequestMessage>
                            <wcs:GetCoverage
                                xmlns:wcs="http://www.opengis.net/wcs/2.0"
                                xmlns:gml="http://www.opengis.net/gml/3.2"
                                xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
                                xsi:schemaLocation="http://www.opengis.net/wcs/2.0 http://schemas.opengis.net/wcs/2.0/wcsAll.xsd"
                                service="WCS" version="2.0.0">
                                <wcs:format>application/gml+xml</wcs:format>
                                <wcs:CoverageId>someEOCoverage1</wcs:CoverageId>
                            </wcs:GetCoverage>
                        </ows:RequestMessage>
                    </ows:ServiceReference>
                </wcseo:referenceGetCoverage>
                <gml:timePosition>2011-08-24T14:18:52Z</gml:timePosition>
            </wcseo:lineage>

     */

    // <wcseo:lineage>
    axiom_node_t *lineage_node =
    		rp_add_child_el(env, eom_node, "lineage", NULL);

    sp_inc_whspace(curr_whspace, whspace_indent);
    sp_add_whspace(env, lineage_node, curr_whspace);

    // comment
    axiom_node_t *comment  = axiom_node_create(env);
    axiom_comment_create (
    		env,
    		lineage_node,
    		"POST GetCoverage request added by SOAP-TO-POST proxy.",
    		&comment);

	//<wcseo:referenceGetCoverage>
	axiom_node_t *ref_g1_node =
			rp_add_child_el(env, lineage_node, "referenceGetCoverage", curr_whspace);

	// <ows:ServiceReference xlink:href="http://www.someWCS.org">
    sp_inc_whspace(curr_whspace, whspace_indent);
    sp_add_whspace(env, ref_g1_node, curr_whspace);
	axiom_namespace_t *ows_ns =
			sp_find_or_create_ns(env, return_node, SP_OWS_NAMESPACE_STR, "ows");

    axiom_node_t    *service_ref_node = axiom_node_create(env);
    axiom_element_t *service_ref_el =
      axiom_element_create(
    		  env, ref_g1_node, "ServiceReference", ows_ns, &service_ref_node);

	axiom_namespace_t *xlink_ns = sp_find_or_create_ns(
			env, return_node, "http://www.w3.org/1999/xlink", "xlink");

    axiom_attribute_t *attr =
      axiom_attribute_create (env, "type", "simple", xlink_ns);
    axiom_element_add_attribute (service_ref_el, env, attr, service_ref_node);

    attr = axiom_attribute_create (env, "href", rp_getSoapOpsURL(), xlink_ns);
    axiom_element_add_attribute (service_ref_el, env, attr, service_ref_node);

    //<ows:RequestMessage>
    axiom_node_t *req_msg_node =
    		rp_add_child_el(env,
    				service_ref_node, "RequestMessage", curr_whspace);
    sp_inc_whspace(curr_whspace, whspace_indent);


    // Detach the request GetCoverage element from its parent
    //  and attach it here.
    axiom_node_t *gc_node =
    		rp_find_named_node(env, request_node, "GetCoverage", 1);
    if (gc_node)
    {
    	// TODO - pretty-up the the indentation to match current.
        sp_add_whspace(env, req_msg_node, curr_whspace);
    	axiom_node_detach    (gc_node, env);
    	axiom_node_add_child (req_msg_node, env, gc_node);
    }

    // Now adjust the indentation of all the closing tags
    sp_inc_whspace(curr_whspace, -whspace_indent);
    sp_add_whspace(env, req_msg_node, curr_whspace);
    sp_inc_whspace(curr_whspace, -whspace_indent);
    sp_add_whspace(env, service_ref_node, curr_whspace);
    sp_inc_whspace(curr_whspace, -whspace_indent);
    sp_add_whspace(env, ref_g1_node, curr_whspace);
    sp_inc_whspace(curr_whspace, -whspace_indent);
    sp_add_whspace(env, lineage_node, curr_whspace);

    // Adjust whitespace before the element
    // that follows our new lineage element, whatever it may be.
    sp_add_whspace(env, eom_node, eom_whsp_str);

    // Delete the whitespace before any our insertions.
	axiom_node_detach    (eom_whsp_node, env);
	axiom_node_free_tree (eom_whsp_node, env);
}
