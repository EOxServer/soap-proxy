/*
 * sp_time_util.c
 *
 *  Created on: Aug 23, 2011
 *      Author: Milan Novacek, ANF DATA
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
 */

/**
 * @file sp_time_util.c
 *
 */


#define _XOPEN_SOURCE
#include <time.h>
#include <stdio.h>

#include <axiom_text.h>

//-----------------------------------------------------------------------------
// Date stings are expected in ISO 8601 format in UTC, example:
//   2011-02-04T15:45:52Z
time_t
sp_parse_time_str(
    const axis2_char_t       *time_str)
{
	int success = 0;
	struct tm tm_s;
	memset(&tm_s, 0, sizeof(tm_s));

	char * parsed =
			strptime(time_str, "%Y-%m-%dT%H:%M:%S", &tm_s);
	if (NULL == parsed)
	{
		success = 0;
	}
	else
	{
		if ('.' == *parsed)
		{
			char tz;
			sscanf(parsed+1, "%d*%c", &tz);
			if ('z'==tz || 'Z' == tz) success = 1;
			else success = 0;
		}
		else if ('Z' == *parsed || 'z' == *parsed)
		{
			success = 1;
		}
		else
		{
			success = 0;
		}
	}

	if (success)
	{
		return timegm(&tm_s);
	}
	else
	{
		return 0;
	}
}
