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
sp_sock_connect(
    const axutil_env_t *env,
    const axis2_char_t *host,
    int                 port);

//-----------------------------------------------------------------------------
/** Send the request to the url.
 * @param env
 * @param req
 * @param mapfile
 * @param backend_url
 * @return stream corresponding to the socket where the response should be read.
 * On error return NULL.
 */
axutil_stream_t *
sp_backend_socket(
    const axutil_env_t *env,
    const axis2_char_t *req,
    const axis2_char_t *mapfile)
{
    axutil_stream_t     *sock_stream  = NULL;
    int                  n_writ       = -1;
    int                  n_read       = -1;
    const int            backend_port = rp_getBackendPort();
    const axis2_char_t  *backend_host = rp_getBackendHost();
    const axis2_char_t  *backend_path = rp_getBackendPath();

	if (axutil_strlen(backend_host) < 3 || axutil_strlen(backend_path) < 1 )
	{
		SP_ERROR(env, SP_SYS_ERR_MS_EXEC);
		rp_log_error(env, "(%s:%d)backend_host='%s', backend_path='%s'\n",
				__FILE__, __LINE__, backend_host, backend_path);
		return NULL;
	}

    int req_len = strlen(req);

    if (req_len > SP_MAX_REQ_LEN || 0 == req_len)
    {
        rp_log_error(env, "Request too long or short (%d)\n", req_len);
        return NULL;
    }

    sock_stream = sp_sock_connect(env, backend_host, backend_port);

    if (!sock_stream)
    {
        rp_log_error(env, "error creating stream for '%s'\n",
        		rp_getBackendURL());
        return NULL;
    }

    // est. max len of fixed header strings ('POST ' ', 'Content-type:' etc.)
    const int max_fixed_len = 160;
    int max_headers_len = max_fixed_len + strlen(backend_path) + strlen(mapfile);

    char *headers = (char*) AXIS2_MALLOC(env->allocator, max_headers_len);
    snprintf(headers, max_headers_len,
    		"POST %s HTTP/1.0\n"
    		"Content-Length: %d\n"
    		"Content-Type:   %s\n"
    		"MS_MAPFILE:     %s\n"
    		"\n"
    		,
    		backend_path,
    		req_len,
    		"text/xml",
    		mapfile);

    n_writ = axutil_stream_write(sock_stream, env, headers, strlen(headers));
    if (n_writ < strlen(headers))
    {
    	rp_log_error(env, "stream write error");
    	sp_stream_cleanup(env, sock_stream);
    	return NULL;
    }

    n_writ = axutil_stream_write(sock_stream, env, req, req_len);
    if (n_writ < req_len)
    {
    	rp_log_error(env, "stream write error");
    	sp_stream_cleanup(env, sock_stream);
    	return NULL;
    }

    return sock_stream;
}

//-----------------------------------------------------------------------------
/** Open up a socket stream to host:port.
 * @param env
 * @param host
 * @param port
 * @return stream for the socket, NULL on error.
 */
axutil_stream_t *
sp_sock_connect(
    const axutil_env_t *env,
    const axis2_char_t *host,
    int                 port)
{
    // TODO see if we need to setup signal handler for SIGPIPE

    int sockfd = -1;

    if (!host || port <= 0)
    {
        rp_log_error(env, "cannot get host/port.\n");
        return NULL;
    }

    sockfd = (int)axutil_network_handler_open_socket(env, (char *) host, port);
    if (sockfd <= 0)
    {
        rp_log_error(env, "error creating socket: %s:%d\n", host, port);
        return NULL;
    }

    return axutil_stream_create_socket(env, sockfd);
}
