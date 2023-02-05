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
#ifndef _RXS_PARSER_H
#define _RXS_PARSER_H

#include <inttypes.h>       // for print "PRIu32" type 'uint32_t'
#include "container/list.h"

#if defined(_cplusplus)
extern "C" {
#endif
// Parse address list
ssize_t parse_addr(char* optarg, dlist_t** addr_allowed_lst);
// Parse args
ssize_t parse_args(int cnt, char* val[], uint32_t* addr_rxs, uint16_t* port_rxs, dlist_t** addr_allowed,
                   int* mode_running, char* file_users, int* pid_host, uint8_t* encoder_mode);
// Parse user info file
ssize_t parse_user_info(const char* filename, dlist_t** user_info_t_lst);
// Parse format: username:password@address:port
ssize_t parse_login(char* buf, char* login, uint16_t login_sz, char* pass, uint16_t pass_sz, char* addr,
                    uint16_t addr_sz, char* port, uint16_t port_sz);
#if defined(_cplusplus)
}
#endif

#endif  // _RXS_PARSER_H
