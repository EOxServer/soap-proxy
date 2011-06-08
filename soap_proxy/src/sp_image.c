/*
 * Soap Proxy.
 *
 * File:   sp_image.c
 * Date:   April 2011
 * Author: Milan Novacek, ANF DATA
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
 * Image-reading utilities.
 *
 */

/**
 * @file sp_image.c
 * 
 */

// uncomment this to enable asserts
//#define NDEBUG

#include <assert.h>
#include "soap_proxy.h"

#include <axutil_linked_list.h>

struct tmp_store
{
  int  size;
  char buf[SP_IMG_BUF_SIZE+3];
};
typedef struct tmp_store TmpStore;

//-----------------------------------------------------------------------------
// Builds the complete image from pieces stored in the linked list ll.
//
static char * compose_image(
    const axutil_env_t * env,
    int image_size,
    axutil_linked_list_t *ll)
{
    char     *image = NULL;
    char     *ip    = NULL;

    TmpStore *ts     = NULL;

    int check = 0;
    if (axutil_linked_list_size(ll, env) > 0)
    {
        ip = image = (char *)AXIS2_MALLOC(env->allocator, image_size);
        entry_t *le = axutil_linked_list_get_entry(ll, env, 0);
        while( le != NULL)
        {
            ts = (TmpStore *)le->data;
            if (NULL != ts)
            {
                memcpy(ip, ts->buf, ts->size);
                ip += ts->size;
                AXIS2_FREE(env->allocator, ts);
                le->data = NULL;
                check += ts->size;
            }
            le = le->next;
        }
    }

    assert( check==image_size );
    return image;
}

//-----------------------------------------------------------------------------
// Reads a binary image from the file fp.
//  fp is positioned at the start of the image.
//
char * rp_load_binary_file(
    const axutil_env_t * env,
    FILE *fp,
    int *len)
{
    char     *image_binary   = NULL;
    char     *ip             = NULL;

    TmpStore             *ts = NULL;
    axutil_linked_list_t *ll = axutil_linked_list_create(env);

    int n_read  = 0;
    *len = 0;
    while ( ! ( feof(fp) || ferror(fp) ) )
    {
        ts = (TmpStore *)AXIS2_MALLOC(env->allocator, sizeof(TmpStore));
        n_read = fread(ts->buf, 1, SP_IMG_BUF_SIZE, fp);
        if (0 == n_read) 
        {
            AXIS2_FREE(env->allocator, ts);
            break;
        }

        ts->size = n_read;
        *len    += n_read;
        axutil_linked_list_add (ll, env, (void *)ts);
    }

    image_binary = compose_image(env, *len, ll);
    axutil_linked_list_free(ll, env);
    return image_binary;
}

//-----------------------------------------------------------------------------
// Reads a binary image from the file fp.
// The file should be composed of HTTP mime messages as received in the form of
// an HTTP response. fp is already positioned at the start of the image.
// The boundary is given by boundId.
//
char * rp_read_bin_mime_image(
    const axutil_env_t * env,
    FILE *fp,
    const char *boundId,
    int *len)
{
    char     *image_binary   = NULL;

    int      actual_filled   = 0;

    TmpStore             *ts = NULL;
    axutil_linked_list_t *ll = axutil_linked_list_create(env);

    *len = 0;

    Rp_cb_ctx fill_ctx;
    init_rp_cb_ctx(&fill_ctx);
    fill_ctx.fp    = fp;
    fill_ctx.bound = boundId;

    while (!fill_ctx.done)
    {
        ts = (TmpStore *)AXIS2_MALLOC(env->allocator, sizeof(TmpStore));
        actual_filled = rp_fill_buff_CB(ts->buf, SP_IMG_BUF_SIZE, &fill_ctx);
        if (0 == actual_filled)
        {
            AXIS2_FREE(env->allocator, ts);
            break;
        }
        ts->size = actual_filled;
        *len    += actual_filled;
        axutil_linked_list_add (ll, env, (void *)ts);
    }

    image_binary = compose_image(env, *len, ll);
    axutil_linked_list_free(ll, env);
    return image_binary;
}
