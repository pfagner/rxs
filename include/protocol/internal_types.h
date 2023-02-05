/*******************************************************************************
** Copyright (c) 2012 - 2023 v.arteev

** Permission is hereby granted, free of charge, to any person obtaining a copy
** of this software and associated documentation files (the "Software"), to deal
** in the Software without restriction, including without limitation the rights
** to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
** copies of the Software, and to permit persons to whom the Software is
** furnished to do so, subject to the following conditions:

** The above copyright notice and this permission notice shall be included in all
** copies or substantial portions of the Software.

** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
** IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
** FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
** AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
** LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
** OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
** SOFTWARE.
*******************************************************************************/

#ifndef _RXS_INTERNAL_TYPES_H
#define _RXS_INTERNAL_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

#include <inttypes.h>  // for 'PRIu32'
#include <stdio.h>     // for 'FILE'
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#ifndef __QNXNTO__
#include <linux/limits.h>  // for 'PATH_MAX'
#else
#include <limits.h>
#endif

#include "container/list.h"

//////////////////////////////////////////////////////////////////////////////////////////////////
// Internal types
//////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////
// file_handlers_t
//////////////////////////////////////////////////////////////////////////////////////////////////
typedef struct file_handlers_t {
  uint32_t fhandle_key;
  FILE* fhandle_val;
} file_handlers_t;

void* new_file_handlers_t(size_t count);
ssize_t init_file_handlers_t(file_handlers_t* file_handlers, uint32_t fhandle_key, FILE* fhandle);
ssize_t dinit_file_handlers_t(file_handlers_t* file_handlers);
void free_file_handlers_t(void* file_handlers);
//////////////////////////////////////////////////////////////////////////////////////////////////
// addr_t
//////////////////////////////////////////////////////////////////////////////////////////////////
typedef struct addr_t {
  uint32_t addr_n;
} addr_t;

void* new_addr_t(size_t count);
ssize_t init_addr_t(addr_t* addr, uint32_t addr_n);
void* ctor_addr_t(uint32_t addr_n);
ssize_t dinit_addr_t(addr_t* addr);
void free_addr_t(void* addr);
ssize_t copy_addr_t(const addr_t* src, addr_t* dst);
ssize_t copy_addr_t_lst(const dlist_t* src_addr_t_lst, dlist_t** dst_addr_t_lst);
//////////////////////////////////////////////////////////////////////////////////////////////////
// user_info_t
//////////////////////////////////////////////////////////////////////////////////////////////////
typedef struct user_info_t {
  char name[PATH_MAX];
  char pass[PATH_MAX];
  char group[PATH_MAX];
  char home_dir[PATH_MAX];
} user_info_t;

void* new_user_info_t(size_t count);
ssize_t init_user_info_t(user_info_t* user_info, const char* name, char name_sz, const char* pass, char pass_sz,
                         char* group, char group_sz, char* home_dir, char home_dir_sz);
void* ctor_user_info_t(const char* name, char name_sz, const char* pass, char pass_sz, char* group, char group_sz,
                       char* home_dir, char home_dir_sz);
void* ctor_copy_user_info_t(user_info_t* user_info);
ssize_t dinit_user_info_t(user_info_t* user_info);
void free_user_info_t(void* user_info);
ssize_t copy_user_info_t(const user_info_t* src, user_info_t* dst);

#ifdef __cplusplus
}
#endif

#endif  // _RXS_INTERNAL_TYPES_H
