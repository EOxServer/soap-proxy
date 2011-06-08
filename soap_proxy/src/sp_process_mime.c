/*
 * Soap proxy.
 *
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
 * @file sp_process_mime.c
 *
 */

/*
 *  XXX TODO:
 *      axiom_document_build_all(..) needs to be called twice to
 *  get the full document.  It really should not be required at all.
 *  The question is if twice is enough? Maybe with more nested levels
 *  of elements in the xml, we may need to call it more than twice.
 *  To implement a full work-around for this apparent bug, we should likely
 *  keep calling it in a loop until the resulting document stops changing.
 *
 */

#include <stdio.h>
#include <string.h>
#include <axutil_linked_list.h>

#include "soap_proxy.h"

//  ===== call-backs and support for axiom_xml_reader_create_for_io() ========
//-----------------------------------------------------------------------------
void init_rp_cb_ctx(
  Rp_cb_ctx *ctx)
{
    ctx->fp       = NULL;
    ctx->bound    = NULL;
    ctx->done     = 0;
    ctx->buf[0]   = '\0';
}


//
//   According to the spec RFC-1314,
//    section 7.2 The Multipart Content-Type
//  [http://www.w3.org/Protocols/rfc1341/7_2_Multipart.html],
//  the boundary always starts with a newline.
//
//  The current patch of mapserver erroneously omits the newline
// after the image.

#define SP_MS_BOUNDARIES_BUG

//-----------------------------------------------------------------------------
int rp_fill_buff_CB(
    char *buffer,
    int size,
    void *ctx)
{
    Rp_cb_ctx *rpctx = (Rp_cb_ctx *) ctx;

    if (size <= 0)  return 0;

    if ( NULL == rpctx->bound || '\0' == *(rpctx->bound) )
    {
        return fread(buffer, 1, size, rpctx->fp);
    }


    // Implementation notes:
    //
    // The basic principle is that reading from the file generally starts
    // after a new line, with some exceptions:
    //   -- one exception is if the input lines in the file are longer
    //   than the requested 'size' input parameter,
    //   -- another case is that after a boundary, fp is left pointing
    //   at the char immediately following the boundary.  This is
    //   done so as to enable the caller to recognize the end-boundary which
    //   is a bound with a trailing '--'.

    if ( 1 == rpctx->done)
    {
        return 0;
    }

    int         n_to_read   = size;
    int         n_read_line = 0;
    int         bound_len   = strlen(rpctx->bound);
    const char  first_bc    = *(rpctx->bound);
    char       *b           = buffer;

    if (n_to_read > 0 && *rpctx->buf != '\0')
    {
        // there is something left over in the read-ahead buffer.

        int left_over_len = strlen(rpctx->buf);

        if (left_over_len > SP_HTTP_BOUNDLEN+3)
        {
            // error - something is not initialised; the buf_len
            // should never be more than this
            return 0;
        }

        if (n_to_read <= left_over_len)
        {
            memcpy(b, rpctx->buf, n_to_read);
            *rpctx->buf = '\0';
            return n_to_read;
        }
        else
        {
            memcpy(b, rpctx->buf, left_over_len);
            *rpctx->buf = '\0';
            b         += left_over_len;
            n_to_read =- left_over_len;
        }
    }


    while (n_to_read > 0)
    {
        // keep reading chars (or lines if possible) until we reach 'size',
        // checking for the boundary as we go.
        char *ret_val = NULL;
        char next_c   = fgetc(rpctx->fp);

        if (feof(rpctx->fp) || ferror(rpctx->fp))
        {
            rpctx->done    = 1;
            *rpctx->buf = '\0';
            return size - n_to_read;
        }

        if (next_c == first_bc)
        {
            // candidate for a boundary

            rpctx->buf[0] = next_c;

            ret_val = fgets(&(rpctx->buf[1]), bound_len, rpctx->fp);

            if ( NULL == ret_val )
            {
                // We hit EOF  - done.
                rpctx->done    = 1;
                *rpctx->buf = '\0';
                return size - n_to_read;
            }

            if ( 0 == strncmp(rpctx->buf, rpctx->bound, bound_len) )
            {
                // We matched the bound string - done with this section
                // of input, but must take care of a possible \n in the
                // busp_process_mime.cer that should have been part of the boundary.
#ifndef SP_MS_BOUNDARIES_BUG
                n_to_read++;
#endif
                rpctx->done    = 1;
                *rpctx->buf = '\0';
                return size - n_to_read;
            }
            else
            {
                // It was not a boundary after all

                if ( n_to_read <= bound_len )
                {
                    memcpy(b, rpctx->buf, n_to_read);
                    b += n_to_read;
                    n_to_read = 0;

                    // move the remaining data up to the start
                    // of rpctx-> to be ready for the next call
                    memmove(rpctx->buf,
                            rpctx->buf+n_to_read,
                            strlen(rpctx->buf+n_to_read) + 1);
                }
                else
                {
                    memcpy(b, rpctx->buf, bound_len);
                    *rpctx->buf = '\0';
                    b += bound_len;
                    n_to_read -= bound_len;
                }
            }
        }
        else
        {
            *b++ = next_c;
            n_to_read--;

#ifndef SP_MS_BOUNDARIES_BUG

            if (n_to_read <= 1) continue;

            if (NULL == fgets(b, n_to_read, rpctx->fp))
            {
                rpctx->done    = 1;
                *rpctx->buf = '\0';
                return 0;
            }

            n_read_line = strlen(b);
            n_to_read   -= n_read_line;
            b           += n_read_line;

#endif

        }

    } // while

    return size - n_to_read;
}

//-----------------------------------------------------------------------------
int rp_close_CB(
    void *ctx)
{
    return 0;
}

//  ==== end call-backs and support for axiom_xml_reader_create_for_io() =====


//-----------------------------------------------------------------------------
axiom_node_t *
rp_process_xml(
    const axutil_env_t * env,
    FILE *fp,
    const char *boundId)
{
    axiom_xml_reader_t        *xml_reader    = NULL;
    axiom_stax_builder_t      *om_builder    = NULL;
    axiom_document_t          *document      = NULL;
    axiom_node_t              *resp_om_node  = NULL;

    // parse the response, creating an om_element.

    Rp_cb_ctx cbctx;
    init_rp_cb_ctx(&cbctx);
    cbctx.fp    = fp;
    cbctx.bound = boundId;

    xml_reader = axiom_xml_reader_create_for_io(
        env, rp_fill_buff_CB, rp_close_CB, &cbctx, NULL);
    om_builder = axiom_stax_builder_create(env, xml_reader);

    int success = 1;

    if (!om_builder)
    {
        axiom_xml_reader_free(xml_reader, env);
        success = 0;
    }
    document = axiom_stax_builder_get_document(om_builder, env);
    if (!document)
    {
        axiom_stax_builder_free(om_builder, env);
        if (success) axiom_xml_reader_free(xml_reader, env);
        success = 0;
    }

    // Not sure why it is necessary to call build_all twice -
    //  surely a bug in axiom?
    axiom_document_build_all(document, env);
    axiom_document_build_all(document, env);

    resp_om_node = axiom_document_get_root_element(document, env);
    if (!resp_om_node)
    {
        if (success)
        {
            axiom_stax_builder_free(om_builder, env);
            axiom_xml_reader_free(xml_reader, env);
        }
        success = 0;
    }


    if (success)
    {
        // XXX TODO: should we free om_builder and/or xml_reader?
    }
    else
    {
    	SP_ERROR(env, SP_USER_ERR_E0_PARSE_MS_OUT);
    	fprintf(stderr,
    			"*** S2P(%s:%d): Error parsing mapserver output.\n",
    			__FILE__, __LINE__ );
    	fflush(stderr);
    }

    return resp_om_node;
}

