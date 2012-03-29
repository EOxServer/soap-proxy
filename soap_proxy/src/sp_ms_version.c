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
 * @file sp_ms_version.c
 * 
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>
#include "soap_proxy.h"

#define SP_TMSTR_LEN 28
#define SP_SVNVSTR_LEN 20
#define SP_VERSSTR_LEN 1024
#define SP_TOTAL_RESP_STR_LEN \
	SP_TMSTR_LEN+SP_SVNVSTR_LEN+SP_VERSSTR_LEN

//-----------------------------------------------------------------------------
axiom_node_t *
rp_getMsVers(
    const axutil_env_t * env,
    const sp_props     *props)
{
    axiom_node_t *return_node = NULL;

    if (rp_getUrlMode(env, props))
    {
    	SP_ERROR(env, SP_SYS_ERR_NOT_IMPLEMENTED);
    	rp_log_error(env, "getMsVersion not implemented with backend_url.\n");
    	return NULL;
    }

    const axis2_char_t *msexec = rp_getMapserverExec(env, props);

    int resp_fd = rp_execMs_dashV(env, msexec);
    if (resp_fd < 0)
    {
    	SP_ERROR(env, SP_SYS_ERR_MS_EXEC);
    	rp_log_error(env, "%s:%d: Mapserver -v exec fail - msexec='%s'\n",
    			__FILE__, __LINE__, msexec);
    }
    else
    {
    	char vers_str [SP_VERSSTR_LEN];
    	char mtime    [SP_TMSTR_LEN  ];
    	char svnvers  [SP_SVNVSTR_LEN];
    	char resp_str [SP_TOTAL_RESP_STR_LEN];

    	vers_str[0] = mtime[0] = svnvers[0] = resp_str[0] = '\0';

    	int n_read = read(resp_fd, vers_str, SP_VERSSTR_LEN);
    	vers_str[n_read] = '\0';
    	close(resp_fd);

    	// get path component of msexec
    	char mspath[SP_MAX_MPATHS_LEN];
    	strncpy(mspath, msexec, SP_MAX_MPATHS_LEN);
    	char *xx = rindex(mspath, '/');
    	*xx = '\0';

    	// Get svn version.
    	// Optimistically assume that the mapserver executable is still
    	// located in the same svn directory as its corresponding source.
    	char svncmd[SP_MAX_MPATHS_LEN];
    	snprintf(svncmd, SP_MAX_MPATHS_LEN, "%s %s", "svnversion", mspath);
    	FILE *pp = popen(svncmd, "r");
    	if (pp)
    	{
    		fgets(svnvers, SP_SVNVSTR_LEN, pp);
    		pclose(pp);
    	}

    	struct stat buf;
        if (0 == stat(msexec, &buf))
        {
        	ctime_r(&buf.st_mtime, mtime);
        }

        snprintf(resp_str, SP_TOTAL_RESP_STR_LEN, "%s%s%s",
        		svnvers, mtime, vers_str);

        axiom_namespace_t * ns =
        		axiom_namespace_create (env, SP_WCSPROXY_NAMESPACE_STR, "sopr");
        axiom_element_t *resp_om_ele =  axiom_element_create(
            env, NULL, "MapServerVersion", ns, &return_node);

        axiom_element_set_text(resp_om_ele, env, resp_str, return_node);
    }
    return return_node;
}
