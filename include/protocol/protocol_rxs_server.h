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
#ifndef _RXS_CLIENT_PROTOCOL_H
#define _RXS_CLIENT_PROTOCOL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <netinet/tcp.h>  //TCP_MAXSEG TCP_NODELAY
#include <stdint.h>
#include <sys/stat.h>  // for 'mode_t'
#include <unistd.h>

#include "container/list.h"
#include "protocol/protocol_rxs.h"

//////////////////////////////////////////////////////////////////////////////////////////////////
// RXS Remote eXchange System: Allows editing files and perform commands on remote file systems.
// Implemented system calls 'rxs_handler_xxx()' refer to 'Standard C Library Functions'. See more:
// https://www.ibm.com/docs/en/i/7.4?topic=extensions-standard-c-library-functions-table-by-name
//////////////////////////////////////////////////////////////////////////////////////////////////

// create access point to the remote system (socket data transfer)
// return value: in success to returns 0; else returns the error code
size_t rxs_data_point_create_client(uint16_t port);
// close access piynt to remote systen (socket data transfer)
// return value: in success to returns 0; else to returns error code
size_t rxs_data_point_close();

//////////////////////////////////////////////////////////////////////////////////////////////////
// Internal functions
//////////////////////////////////////////////////////////////////////////////////////////////////
// Set/get socket descriptor
int set_socket_connected(int sockfd);
int get_socket_connected();
void set_client_addr(struct sockaddr_in* addr);
int set_socket_data(int sockfd);
int get_socket_data();
// Close file handlers
ssize_t clear_file_handlers_lst();
//////////////////////////////////////////////////////////////////////////////////////////////////
// Policy functions
//////////////////////////////////////////////////////////////////////////////////////////////////
ssize_t set_encoder_mode(uint8_t encoder_mode);
ssize_t is_encoder_mode();

ssize_t set_path_users(const char* path_users, size_t path_users_sz);
ssize_t set_user_info(const char* name, char name_sz, const char* pass, char pass_sz, char* group, char group_sz,
                      char* home_dir, char home_dir_sz);
ssize_t clear_user_info_lst();

ssize_t set_allow_address(uint32_t addr_n);
ssize_t set_allow_address_lst(const dlist_t* addr_t_lst);
ssize_t is_allow_address(uint32_t addr_n);
ssize_t clear_allow_address_lst();

ssize_t set_deny_address(uint32_t addr_n);
ssize_t set_deny_address_lst(const dlist_t* addr_t_lst);
ssize_t is_deny_address(uint32_t addr_n);
ssize_t clear_deny_address_lst();

//////////////////////////////////////////////////////////////////////////////////////////////////
// Handlers
//////////////////////////////////////////////////////////////////////////////////////////////////
ssize_t rxs_handler_authorization(const uint8_t* data1, uint32_t data1_sz, const uint8_t* data2, uint32_t data2_sz,
                                  int* status, uint8_t encoder, uint32_t* err_no);
ssize_t rxs_handler_mkdir(uint8_t* data, uint32_t mode, int* status, uint32_t* err_no);
ssize_t rxs_handler_mkdir_ex(uint8_t* data, uint32_t mode, int* status, uint32_t* err_no);
ssize_t rxs_handler_rmdir(uint8_t* data, int* status, uint32_t* err_no);
ssize_t rxs_handler_rmdir_ex(uint8_t* data, int* status, uint32_t* err_no);

ssize_t rxs_handler_chdir(uint8_t* data, int* status, uint32_t* err_no);
ssize_t rxs_handler_unlink(uint8_t* data, int* status, uint32_t* err_no);
ssize_t rxs_handler_rename(uint8_t* data1, uint8_t* data2, int* status, uint32_t* err_no);
ssize_t rxs_handler_filesize(uint8_t* data, long* status, uint32_t* err_no);

ssize_t rxs_handler_fopen(uint8_t* data1, uint8_t* data2, uint32_t* fhandle_key, uint32_t* err_no);
ssize_t rxs_handler_fread(uint32_t key, uint8_t** data, uint32_t* len, uint32_t* err_no);
ssize_t rxs_handler_fwrite(uint32_t key, uint8_t* data, uint32_t len, uint32_t* err_no);
ssize_t rxs_handler_fflush(uint32_t key, int* status, uint32_t* err_no);
ssize_t rxs_handler_fclose(uint32_t key, int* status, uint32_t* err_no);
ssize_t rxs_handler_fseek(uint32_t key, uint32_t val2, uint32_t val3, int* status, uint32_t* err_no);
ssize_t rxs_handler_ftell(uint32_t key, long* status, uint32_t* err_no);
ssize_t rxs_handler_rewind(uint32_t key, int* status, uint32_t* err_no);
ssize_t rxs_handler_is_file(uint8_t* data, int* status, uint32_t* err_no);
ssize_t rxs_handler_is_dir(uint8_t* data, int* status, uint32_t* err_no);
// Run operation
ssize_t run_operation(rxs_operation_t operation, packet_rxs_t* packet_rxs_recv, packet_rxs_t* packet_rxs_send);

#ifdef __cplusplus
}
#endif

#endif  // _RXS_CLIENT_PROTOCOL_H
