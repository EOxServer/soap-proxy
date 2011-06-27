/*
 * sp_backend_sock.c
 *
 *  Created on: Jun 21, 2011
 *      Author: novacek
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
 *
 */
#include <sys/types.h>
#include <sys/socket.h>

#include "soap_proxy.h"

#define SP_MIN_URL_LEN 6

//--------------------------- forward declarations ----------------------------
axutil_stream_t *
rp_sock_connect(
    const axutil_env_t *env,
    const axis2_char_t *host,
    int                 port);

//-----------------------------------------------------------------------------
/** Send the request to the url.
 * @param env
 * @param req
 * @param backend_url
 * @return filedescriptor of the socket where the response should be read. On
 * error return -1.
 */
int
rp_backend_socket(
    const axutil_env_t *env,
    const axis2_char_t *req,
    axutil_url_t       *backend_url)
{
	return -1;
    axutil_stream_t     *sock_stream;
    int                  n_writ = -1;
    int                  n_read = -1;

    axis2_char_t buff[1];

    int reqLen = strlen(req);

    if (reqLen > SP_MAX_REQ_LEN || 0 == reqLen)
    {
        rp_log_error(env, "Request too long or short (%d)\n", reqLen);
        return -1;
    }

    sock_stream = rp_sock_connect(
    		env,
    		axutil_url_get_host(backend_url, env),
    		axutil_url_get_port(backend_url, env));


    if (!sock_stream)
    {
        rp_log_error(env, "error creating stream.");
        return -1;
    }

    // XXX not implemented
    return -1;
}

//-----------------------------------------------------------------------------
/** Close down the socket and perform any cleanup.
 * @param sstream socket stream
 */
void
rp_sock_close(
    const axutil_env_t *env,
    axutil_stream_t    *sstream)
{
	// XXX not implemented

}

//-----------------------------------------------------------------------------
/** Open up a socket stream to host:port.
 * @param host
 * @param port
 * @return filedescriptor of the socket, -1 on error.
 */
axutil_stream_t *
rp_sock_connect(
    const axutil_env_t *env,
    const axis2_char_t *host,
    int                 port)
{
    // TODO see if we need to setup signal handler for SIGPIPE

    int sockfd = -1;

    if (!host || port <= 0)
    {
        rp_log_error(env, "cannot get host/port.");
        return NULL;
    }

    sockfd = (int)axutil_network_handler_open_socket(env, (char *) host, port);
    if (sockfd <= 0)
    {
        rp_log_error(env, "error creating socket.");
        return NULL;
    }

    return axutil_stream_create_socket(env, sockfd);
}
