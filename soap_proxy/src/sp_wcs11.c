/*
 * SOAP-to-POST proxy
 * Stub for WCS 1.1.  Not fully implemented.
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
 * @file sp_wcs11.c
 * 
 */

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <axutil_linked_list.h>

#include "soap_proxy.h"

// TODO: WCS 1.1 support
#ifdef wcs11
//-----------------------------------------------------------------------------
// TBD: WCS   1.* is not supported.
void rp_populate_coverage_hash(
    const axutil_env_t *env,
    axutil_hash_t      *coverage_refs,
    axiom_node_t       *root_node)
{
    // First find the node Coverages,
    // then for all nodes with localname 'Coverage' get the contents of
    // the 'xlink:href' attribute.
    const axis2_char_t *coveragesStr  = "Coverages";
    const axis2_char_t *covStr        = "Coverage";
    const axis2_char_t *referenceStr  = "Reference";

    axiom_node_t *top_node =
      rp_find_named_node(env, root_node, coveragesStr, 0);
    if (NULL == top_node) return;

    axiom_node_t *child_node = axiom_node_get_first_child (top_node, env);

    axiom_children_iterator_t *chit =
      axiom_children_iterator_create (env, child_node);

    if (NULL != chit)
    {
        axiom_node_t *curr_node = NULL;
        while (axiom_children_iterator_has_next(chit, env))
        {
            curr_node = axiom_children_iterator_next(chit, env);
            if (axiom_node_get_node_type(curr_node, env) == AXIOM_ELEMENT)
            {
                axiom_element_t *el = (axiom_element_t *)
                  axiom_node_get_data_element (curr_node, env);
                axis2_char_t *el_name = axiom_element_get_localname(el, env);
                if ( 0 == strncasecmp(covStr, el_name, strlen(covStr)) )
                {
                    axiom_node_t *ref_node =
                      rp_find_named_node
                      (env,
                       axiom_node_get_first_child(curr_node,env),
                       referenceStr,
                       0);
                    axis2_char_t *a_val = rp_get_ref_href (env, ref_node);
                    if (a_val != NULL && 0 == strncmp(a_val, "cid:", 4))
                    {
                      axutil_hash_set(coverage_refs,
                                      a_val+4,
                                      AXIS2_HASH_KEY_STRING,
                                      (void *) ref_node);
                    }
                }
            }  // if
        } // while

        axiom_children_iterator_free (chit, env);
    } // if

}
#endif

//-----------------------------------------------------------------------------
axiom_node_t *
rp_make_img_node11(
    const axutil_env_t * env,
    FILE *fp,
    const char *boundId,
    hh_values *hh)
{
    axiom_node_t         *resp_om_node = NULL;
    axiom_element_t      *resp_om_ele  = NULL;

    axiom_data_handler_t *data_handler = NULL;
    axiom_node_t         *data_om_node = NULL;
    axiom_text_t            *data_text = NULL;

    int img_len = 0;
    char *image_binary =
      rp_read_bin_mime_image(env, fp, boundId, &img_len);

    if (NULL == image_binary)
    {
    	SP_ERROR(env, SP_USER_ERR_IMAGE_FAILED);
    }
    else
    {
        resp_om_ele =
          axiom_element_create (env, NULL, "CoverageData", NULL, &resp_om_node);

        data_handler =
          axiom_data_handler_create(env, NULL, hh->values[SP_HH_CONTENTTYPE]);
        axiom_data_handler_set_binary_data
          (data_handler, env, image_binary, img_len);
        data_text =
          axiom_text_create_with_data_handler
          (env, resp_om_node, data_handler, &data_om_node);
        axiom_text_set_optimize(data_text, env, AXIS2_TRUE);

        // Note:  The buffer 'image_binary' gets freed when
        //        axiom_data_handler_free is called, and it in turn gets called
        //        from axiom_text_free.
        //        That should get called when the resp_om_node gets freed.
        //        Hopefully the service framework does this at some point.
    }

    return resp_om_node;
}

