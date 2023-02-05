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
#include <inttypes.h>  // for 'PRIu32'
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "protocol/generic.h"
#include "protocol/internal_types.h"
//////////////////////////////////////////////////////////////////////////////////////////////////
// Internal types
//////////////////////////////////////////////////////////////////////////////////////////////////
// file_handlers_t
void* new_file_handlers_t(size_t count) { return calloc(count, sizeof(file_handlers_t)); }
ssize_t init_file_handlers_t(file_handlers_t* file_handlers, uint32_t fhandle_key, FILE* fhandle) {
  if (!file_handlers) return -1;

  file_handlers->fhandle_key = fhandle_key;
  file_handlers->fhandle_val = fhandle;
  return 0;
}
ssize_t dinit_file_handlers_t(file_handlers_t* file_handlers) {
  if (!file_handlers) return -1;

  file_handlers->fhandle_key = 0;
  if (file_handlers->fhandle_val) fclose(file_handlers->fhandle_val);
  file_handlers->fhandle_val = NULL;
  return 0;
}
void free_file_handlers_t(void* file_handlers) {
  dinit_file_handlers_t(file_handlers);
  free(file_handlers);
  return;
}
// addr_t
void* new_addr_t(size_t count) { return calloc(count, sizeof(addr_t)); }
ssize_t init_addr_t(addr_t* addr, uint32_t addr_n) {
  if (!addr) return -1;

  addr->addr_n = addr_n;
  return 0;
}
ssize_t dinit_addr_t(addr_t* addr) {
  if (!addr) return -1;

  addr->addr_n = 0;
  return 0;
}
void free_addr_t(void* addr) {
  dinit_addr_t(addr);
  free(addr);
  addr = NULL;
  return;
}
void* ctor_addr_t(uint32_t addr_n) {
  void* data = new_addr_t(1);
  if (init_addr_t(data, addr_n) != 0) {
    free_addr_t(data);
    data = NULL;
  }
  return data;
}
ssize_t copy_addr_t(const addr_t* src, addr_t* dst) {
  if (!src || !dst) return -1;

  dst->addr_n = src->addr_n;
  return 0;
}
ssize_t copy_addr_t_lst(const dlist_t* src_lst, dlist_t** dst_lst) {
  if (!dst_lst) return -1;

  if (list_copy(src_lst, dst_lst, new_addr_t, free_addr_t, (void*)copy_addr_t) != 0) return -1;
  return 0;
}
//////////////////////////////////////////////////////////////////////////////////////////////////
// user_info_t
//////////////////////////////////////////////////////////////////////////////////////////////////
void* new_user_info_t(size_t count) { return calloc(count, sizeof(user_info_t)); }
ssize_t init_user_info_t(user_info_t* user_info, const char* name, char name_sz, const char* pass, char pass_sz,
                         char* group, char group_sz, char* home_dir, char home_dir_sz) {
  if (!user_info) return -1;

  if (memcpy_x(user_info->name, sizeof(user_info->name), name, name_sz) != 0) return -1;
  if (memcpy_x(user_info->pass, sizeof(user_info->pass), pass, pass_sz) != 0) return -1;
  if (memcpy_x(user_info->group, sizeof(user_info->group), group, group_sz) != 0) return -1;
  if (memcpy_x(user_info->home_dir, sizeof(user_info->home_dir), home_dir, home_dir_sz) != 0) return -1;
  return 0;
}
void* ctor_user_info_t(const char* name, char name_sz, const char* pass, char pass_sz, char* group, char group_sz,
                       char* home_dir, char home_dir_sz) {
  void* data = new_user_info_t(1);
  if (init_user_info_t(data, name, name_sz, pass, pass_sz, group, group_sz, home_dir, home_dir_sz) != 0) {
    free_user_info_t(data);
    data = NULL;
  }
  return data;
}
void* ctor_copy_user_info_t(user_info_t* user_info) {
  void* data = new_user_info_t(1);
  if (copy_user_info_t(user_info, data) != 0) {
    free_user_info_t(data);
    data = NULL;
  }
  return data;
}
ssize_t dinit_user_info_t(user_info_t* user_info) {
  if (!user_info) return -1;

  memset(user_info->name, 0, sizeof(user_info->name));
  memset(user_info->pass, 0, sizeof(user_info->pass));
  memset(user_info->group, 0, sizeof(user_info->group));
  memset(user_info->home_dir, 0, sizeof(user_info->home_dir));

  return 0;
}
void free_user_info_t(void* user_info) {
  free(user_info);
  user_info = NULL;
  return;
}
ssize_t copy_user_info_t(const user_info_t* src, user_info_t* dst) {
  if (!dst || !src) return -1;

  if (memcpy_x(dst->name, sizeof(dst->name), src->name, sizeof(src->name)) != 0) return -1;
  if (memcpy_x(dst->pass, sizeof(dst->pass), src->pass, sizeof(src->pass)) != 0) return -1;
  if (memcpy_x(dst->group, sizeof(dst->group), src->group, sizeof(src->group)) != 0) return -1;
  if (memcpy_x(dst->home_dir, sizeof(dst->home_dir), src->home_dir, sizeof(src->home_dir)) != 0) return -1;
  return 0;
}
