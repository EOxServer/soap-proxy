/*
 *  
 * Soap Proxy.
 *
 * Fork, and execute mapserver.
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
 *
 */

/**
 * @file sp_exec_ms.c
 * 
 */

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

#include <sys/types.h>
#include <sys/stat.h>

#ifndef __USE_GNU
#  define __USE_GNU
#endif
#include <fcntl.h>


#include "soap_proxy.h"

#include <axis2_svc.h>


void do_child(
    const axutil_env_t *env,
    const int          reqLen,
    const axis2_char_t *mapfile,
    const axis2_char_t *msexec,
    int rrpipe[],
    int wwpipe[]);

/**
 * Executes "mapserv -v", to get version info.
 * The response from mapserv is stored in a temp file.
 * The file is already unlinked on return - it can be used until
 * closed,  It is the responsibility of the caller to close the file.
 * In case of error the method returns -1.
 *
 * @return filedescriptor of response file, or -1 on error.
 */
int
rp_execMs_dashV(
    const axutil_env_t *env,
    const axis2_char_t *msexec)
{
    char tmpFname[32];
    char exec_str[SP_MAX_MPATHS_LEN+32+8];

    int  tmp_fd = -1;

    if (strlen(msexec) > SP_MAX_MPATHS_LEN)
    {
        fprintf(stderr, "Msexec string too long (%d)\n", strlen(msexec));
        return -1;
    }

    strcpy(tmpFname, "/tmp/msvtXXXXXX");
    tmp_fd = mkostemp(tmpFname, O_CLOEXEC);
    if (tmp_fd < 0)
    {
        perror("mkstemp in rp_execMs_dashV");
        return -1;
    }

    sprintf(exec_str, "%s -v >%s", msexec, tmpFname);
    int retval=system(exec_str);
    if (retval < 0)
    {
        fprintf(stderr, "system call failed.\n");
        unlink(tmpFname);
        return -1;
    }
    unlink(tmpFname);
    return tmp_fd;
}

/**
 * Executes mapserv.
 * The response from mapserv is stored in a temp file.
 * The file is already unlinked on return - it can be used until
 * closed,  It is the responsibility of the caller to close the file.
 * In case of error the method returns -1.
 *
 * @return filedescriptor of response file, or -1 on error.
 */
int
rp_execMapserv(
    const axutil_env_t *env,
    const axis2_char_t *req,
    const axis2_char_t *mapfile,
    const axis2_char_t *msexec)
{
    int rrpipe[2];
    int wwpipe[2];
    pid_t cpid;
    int reqLen = strlen(req);

    if (reqLen > SP_MAX_REQ_LEN)
    {
        fprintf(stderr, "Request too long (%d)\n", reqLen);
        return -1;
    }

    if (0 == reqLen)
    {
        fprintf(stderr, "Zero length request\n");
        return -1;
    }

    if (strlen(msexec) > SP_MAX_MPATHS_LEN)
    {
        fprintf(stderr, "Msexec string too long (%d)\n", strlen(msexec));
        return -1;
    }

    if (strlen(msexec) < 3)
    {
        fprintf(stderr, "Msexec string('%s') too short (%d)\n", msexec, strlen(msexec));
        return -1;
    }
    if (strlen(mapfile) > SP_MAX_MPATHS_LEN)
    {
        fprintf(stderr, "Mapfile string too long (%d)\n", strlen(mapfile));
        return -1;
    }


    if (pipe(rrpipe) == -1 ||
        pipe(wwpipe)== -1 )
    {
        perror("pipe");
        return -1;
    }

    if (rrpipe[0] < 3 ||
        rrpipe[1] < 3 ||
        wwpipe[0] < 3 ||
        wwpipe[1] < 3)
    {
        perror("pipe is < 3");
        return -1;
    }

    cpid = fork();
    if (-1 == cpid)
    {
      perror("fork");
      exit(3);
    }

    if (cpid == 0)
    {
        do_child(env, reqLen, mapfile, msexec, rrpipe, wwpipe);
    }
    else
    {
      /* ---- parent -------*/

      close(rrpipe[0]);          /* Close unused read end */
      close(wwpipe[1]);

      /* send the request to mapserver's stdin */
      write(rrpipe[1], req, reqLen);
      close(rrpipe[1]);          /* Reader will see EOF */
      
      axis2_char_t *tempBuf = (axis2_char_t *)AXIS2_MALLOC(env->allocator, SP_BUF_READSIZE);

      unsigned long response_size = 0;

      char tmpFname[32];
      int  tmp_fd = -1;
      strcpy(tmpFname, "/tmp/msttXXXXXX");
      tmp_fd = mkostemp(tmpFname, O_CLOEXEC);
      if (tmp_fd < 0)
      {
          perror("mkstemp in rp_execMapserv--parent");
          close(wwpipe[0]);
          wait(NULL);
          return -1;
      }

      ssize_t nRead  = read(wwpipe[0], tempBuf, SP_BUF_READSIZE);
      while (nRead < 0)
      {
          // wait and try again a few times
          int ntries = 8;
          int wait_increment = 20111000;
          int wait_interval  = 20111000;
          while (ntries > 0 && nRead < 0)
          {
              struct timespec tt;
              tt.tv_sec  = 0;
              tt.tv_nsec = wait_interval;
              nanosleep(&tt, NULL);
              ntries--;
              wait_interval += wait_increment;
              nRead  = read(wwpipe[0], tempBuf, SP_BUF_READSIZE);
          }
      }
      while (nRead > 0)
      {
          response_size += nRead;
          write(tmp_fd, tempBuf, nRead);
          nRead  = read(wwpipe[0], tempBuf, SP_BUF_READSIZE);
      }

      close(wwpipe[0]);
      AXIS2_FREE(env->allocator, tempBuf);

      lseek(tmp_fd, 0, SEEK_SET);        // return to start of file

      unlink(tmpFname);

      wait(NULL);                // Wait for child
      return tmp_fd;
      
    }  // else --- end parent ---
}

//-----------------------------------------------------------------------------
void do_child(
    const axutil_env_t * env,
    const int reqLen,
    const axis2_char_t *mapfile,
    const axis2_char_t *msexec,
    int rrpipe[],
    int wwpipe[])
{
    /*
     * To invoke mapserver in POST mode, set the following env vars:
     *     REQUEST_METHOD=POST
     *     CONTENT_LENGTH=<size-of-request>
     *     MS_MAPFILE=<path-to-mapfile>
     *
     *  Then execute mapserver, sending input via stdin, e.g.:
     *     cat <request-text> | $MAPSERVER_BINARY
     */

	/* Close unused write & read ends */
    close(rrpipe[1]);
    close(wwpipe[0]);

    dup2(rrpipe[0],  STDIN_FILENO);
    close(rrpipe[0]);

    dup2(wwpipe[1], STDOUT_FILENO);
    close(wwpipe[1]);

    /*  -- set up env for mapserver -- */

    int contBufSize = strlen ("CONTENT_LENGTH=") + 12;
    char *contStr   = (char*) AXIS2_MALLOC(env->allocator, contBufSize);
    snprintf(contStr, contBufSize, "CONTENT_LENGTH=%d", reqLen);

    int mapfBufSize = strlen ("MS_MAPFILE=") + strlen(mapfile) + 1;
    char *mapfStr   = (char*) AXIS2_MALLOC(env->allocator, mapfBufSize);
    snprintf(mapfStr, mapfBufSize, "MS_MAPFILE=%s", mapfile);

    char *msenviron[] = {
        "REQUEST_METHOD=POST",
        contStr,
        mapfStr,
        NULL
    };

    char *msargv[] = { NULL, MAPSERV_ID_STR, NULL };
    execve(msexec, msargv, msenviron);
    perror("do_child() execve");   /* execve() only returns on error */
    fprintf(stderr,"%s:%d  msexec='%s'\n", __FILE__, __LINE__, msexec);
    fflush(stderr);
    exit(EXIT_FAILURE);

}
