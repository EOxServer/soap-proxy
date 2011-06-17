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
 * General utilities, e.g.
 *    String handling / parsing 
 *
 */

/**
 * @file sp_util.c
 * 
 */

#include <stdio.h>
#include <string.h>

#include "soap_proxy.h"
#include "sp_svc.h"

//-----------------------------------------------------------------------------
// Characters which are legal in the boundary separator string 
static const char rp_hh_boundChars[] = 
{
    "0123456789"
    "abcdefghijklmnopqrstuvwxyz"
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "'()+_ ,-./:=?"
};


//-----------------------------------------------------------------------------
/**
 * Scans through str until a char not in chars is found.
 *
 * @param str  Input string, NULL terminated.
 * @param chars array of chars to skip.
 * @return pointer at the first char not in chars, or else at the
 *   trailing '\0'.
 */

char *skipChars(
     char * str,
     const char *chars)
{
    while (str != '\0')
    {
        register char *c;
        int found = 0;
        for ( c=(char*)chars; *c!='\0'; c++ )
        {
            if (*str == *c)
            {
                found = 1;
                break;
            }
        }
        if ( ! found ) return str;
        str++;
    } // while
    return str;
}

//-----------------------------------------------------------------------------
/**
 * Skip blanks in str.
 * Blanks are composed of blank, tab, newline and CR ('\r').
 * @param str Input string.
 * @return ptr to the next non-blank char in str.
 */
char *skipBlanks(
     char * str)
{
    while (*str == ' ' || *str == '\t' || *str == '\n' || *str == '\r') str++;
    return str;
}

//-----------------------------------------------------------------------------
/**
 * Find and optionally copy the next token from a string.
 * The next token is composed of characters from rp_hh_boudChars. Copies the
 * token into dest if not NULL.
 *
 * @param len On entry, *len indicates the max length of a token (without the
 *   terminating \0), on exit it is set to the actual length.
 * @param dest destination for the token.  May be null.  If not null
 *   it is the responsibility of the caller to provide a buffer of at least
 *   len chars.
 * @param str The source string where to get the token from.
 * @return Returns the start of the token, and sets *len to its actual
 * length.
 */
char *
getNextToken(
    int *len,
    char *dest,
    char *str)
{
    int max_len = *len;
    char *end = NULL;
    str = skipBlanks(str);
    
    end =  ('\0' == *str) ? str : skipChars(str, rp_hh_boundChars);
    *len = end - str;

    if (*len > max_len) *len = max_len;
    if (NULL != dest)
    {
        if ( end != str)
        {
            strncpy(dest, str, *len);
        }
        *(dest+*len) = '\0';
    }
    return str;
}

//-----------------------------------------------------------------------------
/**
 * Seek forward in fp until a mime boundary or EOF is found.
 * Assume the input boundId already has '--' pre-pended.
 *
 * @param fp FILE where to seek
 * @param boundId the string representing the mime boundary.
 * @return returns 0 when boundary found, 1 if end-boundary found,
 *    -1 if nothing found.
 */
int seek_to_boundary(
    FILE *fp,
    char *boundId)
{
    char tmpBuf[512];
    int found = -1;
    while (fgets(tmpBuf, 512, fp))
    {
        if ('-' == tmpBuf[0] && '-' == tmpBuf[1])
        {
            // Potential candidate for a match

            int buflen = strlen(tmpBuf);
            if ('\n' == tmpBuf[buflen-1])
            {
                tmpBuf[buflen-1] = '\0';
                if ( 0 == strcmp(tmpBuf, boundId) )
                {
                    found = 0;
                    break;
                }
                if ( buflen-3 == strlen(boundId)  &&
                     '-' == tmpBuf[buflen-2] && '-' == tmpBuf[buflen-3]  &&
                     0 == strncmp(tmpBuf, boundId, buflen-3 ) )
                {
                    found = 1;
                    break;
                }
            }
        }
    }
    return found;
}

//-----------------------------------------------------------------------------
/**
 * Check that fp points at the end of a mime boundary.  It should be called
 * immediately after scanning a mime boundary (without the '\n' or the '--'
 * that forms the end of a mime boundary.
 * When called, fp should point at a newline in the file,
 * or else '--' (the tail of the end boundary).
 * @param fp FILE where to check.
 * @return 0 if newline,
 *         1 if '--'  (meaning end of input found)
 *        -1 otherwise (error - found something unexpected).
 */
int check_end_bound(FILE *fp)
{
    char c;
    c = fgetc(fp);
    if ('\n' == c) return 0;

    if ('-'  == c && '-' == fgetc(fp) )
    {
        return  1;  // ok - finished
    }
    else
    {
        return -1;  // error
    }
}


//-----------------------------------------------------------------------------
//====================== HTTP Headers related  ================================

// Headers of interest to us
static const char *rp_httpHeaderKeys[] =
{
    "Content-type:",
    "Content-Description:",
    "Content-ID:",
    "Content-Transfer-Encoding:"
};

// Indices into httpHeaderVal_struct.values and rp_httpHeaderKeys
#define SP_HH_CONTENTTYPE  0
#define SP_HH_DESCRIPTION  1
#define SP_HH_ID           2
#define SP_HH_XFERENCODING 3

#define SP_HH_NKEYS        4

//-----------------------------------------------------------------------------
/** Initialise hh
 * @param hh
 */
void rp_initHttpHeaderStruct(hh_values *hh) 
{
    int i;
    for ( i=0; i<SP_HH_NKEYS; i++)
    {
        hh->values[i] = NULL;
    }
}


//-----------------------------------------------------------------------------
/**
 * @param env
 * @param hh
 */
void rp_freeHttpHeaders(
    const axutil_env_t * env,
    hh_values *hh) 
{
    int i;
    for ( i=0; i<SP_HH_NKEYS; i++)
    {
        if (hh->values[i]) AXIS2_FREE(env->allocator, hh->values[i]);
    }
}

//-----------------------------------------------------------------------------
/**
 * @param fp
 * @param hh
 */
void rp_printHttpHeaders(
    FILE *fp,
    hh_values *hh)
{
    int i;
    for ( i=0; i<SP_HH_NKEYS; i++)
    {
      if (hh->values[i]) fprintf(fp, "%33s : %s\n",
                                 rp_httpHeaderKeys[i], hh->values[i]);
    }
  
}

//-----------------------------------------------------------------------------

/**
 * On entry, assume hh is initialised to all NULL pointers.
 * @param env
 * @param hh
 * @param fp
 */
void rp_parseHttpHeaders(
    const axutil_env_t * env,
    hh_values *hh,
    FILE *fp) 
{
    if (NULL == hh || NULL == fp) return;

    char tmpBuf[512];
    int i;

    while (fgets(tmpBuf, 512, fp))
    {
        if ('\n' == *tmpBuf || '\0' == *tmpBuf) break;

        for ( i=0; i<SP_HH_NKEYS; i++)
        {
            const char *key = rp_httpHeaderKeys[i];
            if ( 0 == strncasecmp (key, tmpBuf, strlen(key)) )
            {
                char *val     = tmpBuf+strlen(key);
                hh->values[i] = axutil_strtrim(env, val, " \t\r\n");
                break;
            }
        }
    }
}

//-----------------------------------------------------------------------------
/**
 *  scan_boundId
 *  Reads the boundary ID string from contentline.  Returns the boundary
 *  string in boundId, with '--' pre-pended.
 *  In case of error sets boundID to an empty string (i.e. not even the
 *  leading '--' is generated).
 *
 * @param boundId Return parameter: destination where the boundary is copied.
 *  Must be a  buffer long enough to hold the maximum allowable mime boundary
 *  including a leading '--' and a trailing '\0'.
 * @param contentLine Input, a HTTP header line containing the boundary in the
 *   form 'boundary=<str>', where <str> is the mime boundary ID string.  May be
 *   preceded by one other key/value pair terminated with a ';'
 */
void scan_boundId(
  char *boundId,
  const char *contentLine)
{
    const char boundaryKeywd[] = "boundary=";
    char * currToken = NULL;

    char * tmp = index(contentLine, ';');
    currToken =  (char *)(tmp ? tmp : contentLine);

    tmp = strstr(currToken, boundaryKeywd);

    // TODO: this should be extended to handle an arbitrary number of
    //   'key=value;' pairs preceding 'boundary=<str>'
    if (!tmp)
    {
        *boundId = '\0';
    }
    else
    {
        currToken = tmp + strlen(boundaryKeywd);

        int tokLen = SP_HTTP_BOUNDLEN;
        getNextToken(&tokLen, boundId+2, currToken);
        if ('\0' == boundId[2])
        {
            *boundId = '\0';
        }
        else
        {
            boundId[0] = boundId[1] = '-';
        }
    }
}

//====================== end HTTP Headers related ============================


//-----------------------------------------------------------------------------
/**
 * @param el_node
 * @param env
 * @return text contents of el_node
 */
const axis2_char_t *rp_getText(
		axiom_node_t       *el_node,
		const axutil_env_t *env)
{
	axiom_node_t       *val_node = NULL;
	const axis2_char_t *ret_val  = NULL;

	if (el_node &&
			axiom_node_get_node_type(el_node, env) == AXIOM_ELEMENT)
	{
		val_node = axiom_node_get_first_child(el_node, env);
		if (val_node &&
				axiom_node_get_node_type(val_node, env) == AXIOM_TEXT)
		{
			axiom_text_t *val_text = NULL;
			val_text = (axiom_text_t *)
                		axiom_node_get_data_element(val_node, env);
			if (val_text && axiom_text_get_value(val_text, env))
			{
				ret_val = axiom_text_get_value(val_text, env);
			}
		}
	}

	return ret_val;
}

//-----------------------------------------------------------------------------
/**
 * Find the first child of 'root_node' with a matching 'local_name'.
 * Recurses down the document tree depth first, and returns the first matching
 * node found.  e.g looking for 'foo' given the tree:
 *      <bar>
 *        <zar><foo/></zar>
 *        <foo/>
 *      </bar>
 * will yield the node at zar/foo.
 *
 * @param env
 * @param root_node
 * @param local_name
 * @return  the first child of 'root_node' with a matching local_name, or
 *  NULL if none found.
 */
axiom_node_t *rp_find_named_node_recurse(
    const axutil_env_t * env,
    axiom_node_t       *root_node,
    const axis2_char_t *local_name)
{
   axiom_node_t *found_node = NULL;

    axiom_children_iterator_t *chit =
      axiom_children_iterator_create (env, root_node);

    if (NULL != chit)
    {
        axiom_node_t *curr_node = NULL;
        while (axiom_children_iterator_has_next(chit, env))
        {
            curr_node = axiom_children_iterator_next(chit, env);
            if (axiom_node_get_node_type(curr_node, env) == AXIOM_ELEMENT)
            {
                axiom_element_t * el = (axiom_element_t *)
                  axiom_node_get_data_element (curr_node, env);
                axis2_char_t *el_name = axiom_element_get_localname( el, env);
                if ( 0 == strncasecmp( local_name, el_name, strlen(local_name) ) )
                {
                    found_node = curr_node;
                    break;
                }
                else
                {
                	axiom_node_t *child_node =
                			axiom_node_get_first_element(curr_node, env);
                	if (NULL != child_node)
                	{
                		found_node = rp_find_named_node_recurse(
                				env, child_node, local_name);
                		if (NULL != found_node)
                		{
                			break;
                		}
                	}
                } // else
            }  // if
        } // while

        axiom_children_iterator_free (chit, env);
    }

    return found_node;
}

//-----------------------------------------------------------------------------
/**
 * Find the first child of 'root_node' with a matching 'local_name',
 * non-recursive.
 *
 * @param env
 * @param root_node
 * @param local_name
 * @return  the first child of 'root_node' with a matching local_name, or
 *  NULL if none found.
 */
axiom_node_t *rp_find_named_node_nr(
    const axutil_env_t * env,
    axiom_node_t       *root_node,
    const axis2_char_t *local_name)
{
   axiom_node_t *found_node = NULL;

    axiom_children_iterator_t *chit =
      axiom_children_iterator_create (env, root_node);

    if (NULL != chit)
    {
        axiom_node_t *curr_node = NULL;
        while (axiom_children_iterator_has_next(chit, env))
        {
            curr_node = axiom_children_iterator_next(chit, env);
            if (axiom_node_get_node_type(curr_node, env) == AXIOM_ELEMENT)
            {
                axiom_element_t * el = (axiom_element_t *)
                  axiom_node_get_data_element (curr_node, env);
                axis2_char_t *el_name = axiom_element_get_localname( el, env);
                if ( 0 == strncasecmp( local_name, el_name, strlen(local_name) ) )
                {
                    found_node = curr_node;
                    break;
                }
            }  // if
        } // while

        axiom_children_iterator_free (chit, env);
    }

    return found_node;
}

//-----------------------------------------------------------------------------
/**
 * Find the first child of 'root_node' with a matching 'local_name'.
 * @param env
 * @param root_node
 * @param local_name
 * @param recurse
 * @return the first child of 'root_node' with a matching local_name, or
 *  NULL if none found.
 */
axiom_node_t *rp_find_named_node(
    const axutil_env_t * env,
    axiom_node_t       *root_node,
    const axis2_char_t *local_name,
    int                recurse)
{
	return (recurse) ?
		rp_find_named_node_recurse(env, root_node, local_name) :
		rp_find_named_node_nr     (env, root_node, local_name);
}


//-----------------------------------------------------------------------------
/** Add a node with an element to root_node, either as child or as sibling.
 *   The element name is in the same namespace as the root node's name.
 *   The element may optionally include one attribute; if no attribute
 *   is desired then pass 'NULL'.
 *
 *  Example with attribute:
 *     node_id.name="foo";     node.value="bar";
 *     attribute.name="attr";  attribute.value="Zar";
 *  Result :  <ns:foo attr="Zar">bar</ns:foo>
 *
 * @param env
 * @param root_node
 * @param node_id
 *   node_id.name:  name of element to be added,
 *   node_id.value: text of element
 * @param attribute optional Name_value pair for setting an attribute,
 *   may be NULL.
 * @param sibling 1: add as sibling, 0: add as child.
 * @return ptr to the the newly added element.
 */
static axiom_element_t *rp_add_node(
    const axutil_env_t *env,
    axiom_node_t       *root_node,
    Name_value         *node_id,
    Name_value         *attribute,
    int                sibling
 )
{

    axiom_namespace_t * ns = rp_get_namespace (env, root_node);

    axiom_node_t    *new_node = axiom_node_create(env);
    axiom_element_t *new_ele =
      axiom_element_create(env, NULL, node_id->name, ns, &new_node);
    axiom_element_set_text(new_ele, env, node_id->value, new_node);

    if (NULL != attribute)
    {
        axiom_attribute_t *attr =
          axiom_attribute_create (env, attribute->name, attribute->value, NULL);
        axiom_element_add_attribute (new_ele, env, attr, new_node);
    }

    axis2_status_t success = (
    		sibling ?
    				axiom_node_insert_sibling_after (root_node, env, new_node) :
    				axiom_node_add_child            (root_node, env, new_node)
    );

    if (AXIS2_SUCCESS != success)
    {
    	fprintf(stderr, "*** S2P(%s:%d): Failed to add node name='%s'\n",
    			__FILE__, __LINE__, node_id->name);
    	fflush(stderr);
    }

    return new_ele;
}

//-----------------------------------------------------------------------------
/** Add a sibling node with an element after root_node.
 *   The element name is in the same namespace as the root node's name.
 *   The element may optionally include one attribute; if no attribute
 *   is desired then pass 'NULL'.
 *
 *  Example with attribute:
 *     node_id.name="foo";     node.value="bar";
 *     attribute.name="attr";  attribute.value="Zar";
 *  Result :  <ns:foo attr="Zar">bar</ns:foo>
 *
 * @param env
 * @param root_node
 * @param node_id
 *   node_id.name:  name of element to be added,
 *   node_id.value: text of element
 * @param attribute optional Name_value pair for setting an attribute,
 *   may be NULL.
 * @return ptr to the the newly added element.
 */
axiom_element_t *rp_add_sibbling(
    const axutil_env_t *env,
    axiom_node_t       *root_node,
    Name_value         *node_id,
    Name_value         *attribute
 )
{
	return rp_add_node(env, root_node, node_id, attribute, 1);
}

//-----------------------------------------------------------------------------
/** Add a child node with an element to root_node.
 *   The element name is in the same namespace as the root node's name.
 *   The element may optionally include one attribute; if no attribute
 *   is desired then pass 'NULL'.
 *
 *  Example with attribute:
 *     node_id.name="foo";     node.value="bar";
 *     attribute.name="attr";  attribute.value="Zar";
 *  Result :  <ns:foo attr="Zar">bar</ns:foo>
 *
 * @param env
 * @param root_node
 * @param node_id
 *   node_id.name:  name of element to be added,
 *   node_id.value: text of element
 * @param attribute optional Name_value pair for setting an attribute,
 *   may be NULL.
 * @return ptr to the the newly added element.
 */
axiom_element_t *rp_add_child(
    const axutil_env_t *env,
    axiom_node_t       *root_node,
    Name_value         *node_id,
    Name_value         *attribute
 )
{
	return rp_add_node(env, root_node, node_id, attribute, 0);
}

//-----------------------------------------------------------------------------
/** Get namespace.
 * @param env
 * @param node
 * @return ptr to the namespace of 'node'.
 */
axiom_namespace_t * rp_get_namespace(
    const axutil_env_t *env,
    axiom_node_t *node)
{
    axiom_element_t * root_el =
      (axiom_element_t *) axiom_node_get_data_element (node, env);
    return axiom_element_get_namespace (root_el, env, node);
}

//-----------------------------------------------------------------------------
/** Excecute 'func' on all immediate children of 'root_node' with 'local_name'.
 * @param env
 * @param root_node
 * @param local_name
 * @param func
 *   Takes 3 args: const axutil_env_t * env, axiom_node_t *node, void *arg3
 *   node is the ptr to the node matching 'local_name', arg3 is func_arg.
 *   The function should return zero if successful, non-zero otherwise.
 * @param func_arg passed as the third arg to the function.
 * @return the number of times func returned 0.
 */
int rp_func_at_nodes(
    const axutil_env_t *env,
    axiom_node_t *root_node,
    const axis2_char_t *local_name,
    int (* func)(const axutil_env_t * env, axiom_node_t *node, void *arg3),
    void *func_arg
    )
{
    int         num_executed = 0;

    axiom_children_iterator_t *chit =
      axiom_children_iterator_create (env, root_node);

    if (NULL != chit)
    {
        axiom_node_t *curr_node = NULL;
        while (axiom_children_iterator_has_next(chit, env))
        {
            curr_node = axiom_children_iterator_next(chit, env);
            if (axiom_node_get_node_type(curr_node, env) == AXIOM_ELEMENT)
            {
                axiom_element_t * el = (axiom_element_t *)
                  axiom_node_get_data_element (curr_node, env);
                axis2_char_t *el_name = axiom_element_get_localname( el, env);
                if ( 0 == strncasecmp( local_name, el_name, strlen(local_name) ) )
                {
                    if (0 == func(env, curr_node, NULL)) num_executed++;
                }
            }  // if
        } // while

        axiom_children_iterator_free (chit, env);
    }

    return num_executed;
}

//-----------------------------------------------------------------------------
/** Get the href reference string from node's xlink:href attribute.
 * @param env
 * @param node
 * @return the href string
 */
axis2_char_t *rp_get_ref_href(
    const axutil_env_t   *env,
    axiom_node_t         *node)
{
    axis2_char_t *a_val = NULL;

    if (NULL != node &&
        axiom_node_get_node_type(node, env) == AXIOM_ELEMENT)
    {
        axiom_element_t *el2 =
          (axiom_element_t *) axiom_node_get_data_element (node, env);
        a_val = axiom_element_get_attribute_value_by_name(el2,
                                                          env,
                                                          "xlink:href");
    }

    return a_val;

}

//-----------------------------------------------------------------------------
/** Print coverage hash entries.
 *   May be used for diagnostics.
 *
 * @param env
 * @param fp   where to print to
 * @param ch  the hash to print.
 */
void rp_print_coverage_hash_entries(
    const axutil_env_t *env,
    FILE               *fp,
    axutil_hash_t      *ch)
{
    axutil_hash_index_t *hi;
    const void *key;
    void *val;
    for ( hi = axutil_hash_first (ch, env);
          hi;
          hi = axutil_hash_next (env, hi) )
    {
        axutil_hash_this(hi, &key, NULL, &val);
        fprintf(fp,"%s\n",  (char *) key);
    }
}

//-----------------------------------------------------------------------------
/** Determine the WCS protocol/version and/or profile used.
 * @param env
 * @param node
 * @return one of the sp_wcs_version_ids
 */
int sp_glean_protocol(
  const axutil_env_t *env,
  axiom_node_t       *node)
{
	// TODO: sp_glean_protocol() is not implemented.
	return SP_WCS_V200;
}
