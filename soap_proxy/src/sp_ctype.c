/*
 * Soap Proxy.
 *
 * File:   sp_ctype.c
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
 */

/**
 * @file sp_ctype.c
 * 
 */

#include "soap_proxy.h"

#//-----------------------------------------------------------------------------
// Const data for rp_get_contentType

const int idTypes[] = {
        SP_RESP_XML_TYPE,
        SP_RESP_MIXED_TYPE,
        SP_RESP_TIFF_TYPE,
        SP_RESP_APP_SEXML_TYPE,
        SP_RESP_UNKNOWN_TYPE
};

const char *idStrings[] = {
        "text/xml",
        "multipart/mixed",
        "image/tiff",
        "application/vnd.ogc.se_xml"
};

#define SP_CTYPE_NSTRINGS 4

//-----------------------------------------------------------------------------
int rp_get_contentType(
    char *str)
{

    str = skipBlanks(str);

    int i = 0;

    for(i = 0; i<SP_CTYPE_NSTRINGS; i++)
    {
        const char * xmlIdStr = idStrings[i];
        int idStrLen = strlen(xmlIdStr);
        if ( 0 == strncmp (xmlIdStr, str, idStrLen) )
        {
            return idTypes[i];
        }
    }

    return SP_RESP_UNKNOWN_TYPE;
}

