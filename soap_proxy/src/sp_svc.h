
/*
 * Soap Proxy.
 * Common parts of the soap service handling.
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
 */

/**
 * @file sp_svc.h
 *
 * Based on the 'hello service' template from the
 *  axis2/c tutorial.
 *
 * The axis2/c tutorial is Copyright 2004,2005 The Apache Software Foundation,
 * and is licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */


#ifndef OOOSSVC_H_INCLUDED
#define OOOSSVC_H_INCLUDED

/* ------------------------------------ includes -----------------------*/
#include <axis2_svc_skeleton.h>
#include <axis2_svc.h>
#include <axutil_log_default.h>
#include <axutil_error_default.h>
#include <axutil_array_list.h>
#include <axutil_hash.h>
#include <axiom_xml_reader.h>
#include <axiom_stax_builder.h>
#include <axiom_text.h>
#include <axiom_node.h>
#include <axiom_element.h>

/* ------------------------------------ forward declarations -----------*/
int AXIS2_CALL rpSvc_free(
    axis2_svc_skeleton_t * svc_skeleton,
    const axutil_env_t * env);

axiom_node_t *AXIS2_CALL rpSvc_invoke(
    axis2_svc_skeleton_t * svc_skeleton,
    const axutil_env_t * env,
    axiom_node_t * node,
    axis2_msg_ctx_t * msg_ctx);

int AXIS2_CALL rpSvc_init(
    axis2_svc_skeleton_t * svc_skeleton,
    const axutil_env_t * env);

axiom_node_t *AXIS2_CALL rpSvc_on_fault(
    axis2_svc_skeleton_t * svc_skeli,
    const axutil_env_t * env,
    axiom_node_t * node);

axiom_node_t *rp_error_elem(
    const axutil_env_t * env,
    axis2_char_t * errorText);

#endif
