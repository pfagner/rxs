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
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#ifndef __QNXNTO__
#include <linux/limits.h>  // for 'PATH_MAX'
#else
#include <limits.h>
#include <netinet/tcp.h>  // for TCP_MAXSEG
#endif
#include <inttypes.h>  // for 'PRIu32'
#include <pthread.h>   // for 'pthread'

#include "logger/logger.h"
#include "protocol/generic.h"  // for 'write_file()'
#include "protocol/protocol_rxs_client.h"
#include "protocol/rxs_errno.h"

int sockfd_conn = -1;
int sockfd_data = -1;
int sockfd_data_client = -1;
uint8_t have_encoder = 0;
// struct sockaddr_storage hisctladdr;
int errno_both_sides = 0;

static int set_socket_connected(int sockfd) { return (sockfd_conn = sockfd); }
static int get_socket_connected() { return sockfd_conn; }
static int set_socket_data(int sockfd) { return (sockfd_data = sockfd); }
static int get_socket_data() { return sockfd_data; }
static int set_socket_data_client(int sockfd) { return (sockfd_data_client = sockfd); }
static int get_socket_data_client() { return sockfd_data_client; }
//////////////////////////////////////////////////////////////////////////////////////////////////
// Data receiver
//////////////////////////////////////////////////////////////////////////////////////////////////
typedef struct data_exchange_t {
  int sockfd;
  RXS_HANDLE stream;
  void* buf;
  size_t buf_sz;
  size_t total_impl_channel_sz;
  size_t total_impl_sz;
  int complete;
} data_exchange_t;

pthread_t data_receiver_thr;
void* data_receiver(void* arg) {
  if (!arg) pthread_exit(NULL);

  data_exchange_t* val = (data_exchange_t*)arg;

  size_t block_sz = ((have_encoder > 0) ? (crypt_packet_sz()) : (0));
  size_t buf_recv_sz = ((have_encoder > 0) ? (crypt_packet_sz()) : (crypt_data_sz()));
  uint8_t* buf_recv = calloc(buf_recv_sz, sizeof(uint8_t));
  if (!buf_recv) {
    log_msg(ERRN, 6, "calloc", strerror(errno));
    pthread_exit(NULL);
  }
  while (val->total_impl_channel_sz < val->buf_sz) {
    if ((val->buf_sz - val->total_impl_channel_sz) < buf_recv_sz) break;

    ssize_t impl_channel_sz = rxs_recv_block_x(val->sockfd, buf_recv, buf_recv_sz, block_sz);
    if (impl_channel_sz < 0) break;
    // printf("THR DATA BUF:%d ch:%d rl:%d impl:%d sz:%d df:%d\n", val->buf_sz, val->total_impl_channel_sz,
    // val->total_impl_sz, impl_channel_sz, buf_recv_sz, ( val->buf_sz - val->total_impl_sz) );
    size_t dec_bytes =
        decompose_data(have_encoder, val->buf, buf_recv, buf_recv_sz, val->total_impl_sz, (size_t)impl_channel_sz);
    val->total_impl_sz += dec_bytes;
    val->total_impl_channel_sz += (size_t)impl_channel_sz;
  }
  val->complete = 1;

  free(buf_recv);
  buf_recv = NULL;

  pthread_exit(NULL);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// CAUTION: declaration of internal functions for create/close access point to remote side
//////////////////////////////////////////////////////////////////////////////////////////////////
// create access point to remote side (socket data transfer).
// Return value: on successful returns 0, otherwise -1
size_t rxs_data_point_create_client(RXS_HANDLE stream);
// close access poit to remote side (socket data transfer)
// Return value: on successful returns 0, otherwise -1
size_t rxs_data_point_close();

//////////////////////////////////////////////////////////////////////////////////////////////////
// Implementation of interna/extrnal clients functions
//////////////////////////////////////////////////////////////////////////////////////////////////
size_t rxs_data_point_create_client(RXS_HANDLE stream) {
  int sock_data = socket(PF_INET, SOCK_STREAM, 0);
  if (sock_data < 0) {
    errno_both_sides = errno;
    log_msg(ERRN, 6, "socket, data connection", strerror(errno));
    rxs_data_point_close();
    return -1;
  }
  struct sockaddr_in data_addr;
  memset(&data_addr, 0, sizeof(data_addr));
  // memcpy(&data_addr, &hisctladdr, sizeof(data_addr));
  socklen_t addr_len = sizeof(data_addr);

  int reuseaddr = 1;
  if (setsockopt(sock_data, SOL_SOCKET, SO_REUSEADDR, &reuseaddr, sizeof(reuseaddr)) < 0) {
    errno_both_sides = errno;
    log_msg(ERRN, 59, "SO_REUSEADDR", strerror(errno));
    rxs_data_point_close();
    return -1;
  }

  data_addr.sin_port = 0;
  data_addr.sin_family = AF_INET;
  data_addr.sin_addr.s_addr = INADDR_ANY;
  if (bind(sock_data, (struct sockaddr*)&data_addr, addr_len) < 0) {
    char ipAddress[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(data_addr.sin_addr), ipAddress, INET_ADDRSTRLEN);
    errno_both_sides = errno;
    log_msg(ERRN, 56, strerror(errno));
    rxs_data_point_close();
    return -1;
  }
  if (getsockname(sock_data, (struct sockaddr*)&data_addr, &addr_len) < 0) {
    errno_both_sides = errno;
    log_msg(ERRN, 6, "getsockname", strerror(errno));
    rxs_data_point_close();
    return -1;
  }
  /*
      if(set_socket_mode(sock_data, have_encoder) !=0)
      {
          errno_both_sides = errno;
          log_msg(ERRN, 6, "set_socket_mode #1", strerror(errno));
          rxs_data_point_close();
          return -1;
      }
  */
  uint16_t port_h = ntohs(data_addr.sin_port);
  // CAUTION: backlog. See link: https://tangentsoft.net/wskfaq/advanced.html#backlog
  // You will note that SYN attacks are dangerous for systems with both very short and very long backlog queues. The
  // point is that a middle ground is the best course if you expect your server to withstand SYN attacks. Either use
  // Microsoft’s dynamic backlog feature, or pick a value somewhere in the 20-200 range and tune it as required.
  int backlog = 30;  // SOMAXCONN;
  if (listen(sock_data, backlog) < 0) {
    errno_both_sides = errno;
    log_msg(ERRN, 6, "listen", strerror(errno));
    rxs_data_point_close();
    return -1;
  }

  ssize_t res =
      rqst_x05_resp_x00(get_socket_connected(), CS_A0, operation_port, stream, port_h, NULL, 0, &errno_both_sides);
  if (errno_both_sides) errno_both_sides = RXS_SRV_NONE + errno_both_sides;
  if (res < 0) {
    // Set errno
    if (!errno_both_sides) errno_both_sides = EIO;
    rxs_data_point_close();
    // log_msg(ERRN, 6, "rqst_x05_resp_x00", strerror(errno));
    return -1;
  }
  // Set socket data
  set_socket_data(sock_data);

  struct pollfd sockfd_poll[1] = {{-1}};
  memset(sockfd_poll, -1, sizeof(sockfd_poll));
  sockfd_poll[0].fd = sock_data;
  sockfd_poll[0].events = POLLIN;
  int ret_code = poll(sockfd_poll, 1, POLL_TIMEOUT_CONNECT_MSEC);
  if (ret_code > 0) {
    if ((sockfd_poll[0].revents & POLLIN) && (sockfd_poll[0].fd == sock_data)) {
      int sock_data_client = accept(sock_data, (struct sockaddr*)&data_addr, &addr_len);
      if (sock_data_client < 0) {
        errno_both_sides = errno;
        log_msg(ERRN, 6, "accept() sock_data", strerror(errno));
        rxs_data_point_close();
        return -1;
      }
      if (set_socket_mode(sock_data_client, have_encoder) != 0) {
        errno_both_sides = errno;
        log_msg(ERRN, 6, "set_socket_mode #4", strerror(errno));
        rxs_data_point_close();
        return -1;
      }
      // Set socket data client
      set_socket_data_client(sock_data_client);
      return 0;
    } else {
      errno_both_sides = errno;
      log_msg(ERRN, 6, "poll catch unexpexted event", strerror(errno));
      rxs_data_point_close();
      return -1;
    }
  }
  // Timeout or error
  else {
    if (ret_code == 0) log_msg(WARN, 6, "poll() timeout #1", strerror(errno));
    if (ret_code < 0) log_msg(ERRN, 6, "poll() error", strerror(errno));
    errno_both_sides = errno;
    rxs_data_point_close();
    return -1;
  }
  return -1;
}
size_t rxs_point_create(const char* host_p, uint16_t port_h, const char* username, const char* password, int encoder) {
  if (!host_p || !username || !password) {
    return -1;
  }
  // Set errno
  errno_both_sides = 0;
  have_encoder = encoder;
  int sockfd = socket(PF_INET, SOCK_STREAM, 0);
  if (0 > sockfd) {
    // Set errno
    errno_both_sides = errno;
    log_msg(ERRN, 6, "socket", strerror(errno));
    rxs_point_close();
    return -1;
  }
  struct sockaddr_in addr_other;
  memset(&addr_other, 0, sizeof(addr_other));
  addr_other.sin_family = AF_INET;
  addr_other.sin_port = htons(port_h);
  addr_other.sin_addr.s_addr = inet_addr(host_p);
  socklen_t addr_other_len = sizeof(addr_other);

  /*
      if(set_socket_mode(sockfd, have_encoder) !=0)
      {
          errno_both_sides = errno;
          log_msg(ERRN, 6, "set_socket_mode#2", strerror(errno));
          rxs_point_close();
          return -1;
      }
  */
  //////////////////////////////////////////////////////////////////////////////////
  // Connect request
  //////////////////////////////////////////////////////////////////////////////////
  if (connect(sockfd, (const struct sockaddr*)&addr_other, addr_other_len) == -1) {
    // Set errno
    errno_both_sides = errno;
    log_msg(ERRN, 6, "connect", strerror(errno));
    log_msg(ERRN, 9, host_p, port_h);
    rxs_point_close();
    return -1;
  }
  if (set_socket_mode(sockfd, have_encoder) != 0) {
    errno_both_sides = errno;
    log_msg(ERRN, 6, "set_socket_mode#3", strerror(errno));
    rxs_point_close();
    return -1;
  }
  //////////////////////////////////////////////////////////////////////////////////
  // Authorization request
  //////////////////////////////////////////////////////////////////////////////////
  ssize_t res = rqst_x02_resp_x00(sockfd, CS_A0, operation_authorization, username, strlen(username), password,
                                  strlen(password), encoder, NULL, 0, &errno_both_sides);
  if (res < 0) {
    // Set errno
    if (!errno_both_sides) errno_both_sides = errno;
    // log_msg(ERRN, 6, "rqst_x02_resp_x00", strerror(errno));
    rxs_point_close();
    return -1;
  }

  if (EACCES == errno_both_sides) {
    log_msg(ERRN, 29);
    rxs_point_close();
    return -1;
  }
  // Set socket connected
  set_socket_connected(sockfd);
  // Set errno
  if (errno_both_sides) errno_both_sides = RXS_SRV_NONE + errno_both_sides;
  return (!errno_both_sides) ? (0) : (-1);
}
size_t rxs_point_connected() {
  if (get_socket_connected() >= 0) {
    int err_code;
    socklen_t err_code_sz = sizeof(err_code);
    int res = getsockopt(get_socket_connected(), SOL_SOCKET, SO_ERROR, &err_code, &err_code_sz);
    if (!res) {
      // Broken pipe
      if (EPIPE == err_code) return 0;
    }
    return 1;
  }
  return 0;
}
size_t rxs_point_close() {
  if (get_socket_connected() != -1) {
    close(get_socket_connected());
    set_socket_connected(-1);
    return 0;
  }
  // Set errno
  errno_both_sides = errno;
  return -1;
}
size_t rxs_data_point_close() {
  if (get_socket_data() != -1) {
    close(get_socket_data());
    set_socket_data(-1);
  }
  if (get_socket_data_client() != -1) {
    close(get_socket_data_client());
    set_socket_data_client(-1);
  }
  return 0;
}
int rxs_errno() { return errno_both_sides; }
char* rxs_strerror() {
  // srv
  if (rxs_errno() >= RXS_SRV_NONE) {
    // sh command not found
    if (rxs_errno() - RXS_SRV_NONE == 127)
      return "command not found";
    else
      return strerror(rxs_errno() - RXS_SRV_NONE);
  }
  // cli
  else {
    return strerror(rxs_errno());
  }
}
size_t rxs_ls(const char* path, void* path_file, size_t size) {
  if (!path || !path_file) return -1;
  ////////////////////////////////////////////
  // Get path of remote file
  ////////////////////////////////////////////
  char file_remote[PATH_MAX] = {0};
  // Set errno
  errno_both_sides = 0;
  ssize_t res = rqst_x01_resp_x01(get_socket_connected(), CS_A0, operation_ls, path, strlen(path), file_remote,
                                  sizeof(file_remote), &errno_both_sides);
  // Set errno
  if (errno_both_sides) {
    errno_both_sides = RXS_SRV_NONE + errno_both_sides;
    return -1;
  }
  if (res < 0) {
    // Set errno
    errno_both_sides = errno;
    // log_msg(ERRN, 6, "rqst_x00_resp_x00", strerror(errno));
    return -1;
  }
  ////////////////////////////////////////////
  // Get remote file by path
  ////////////////////////////////////////////
  // Open the remote file
  RXS_HANDLE handle_file_remote = rxs_fopen(file_remote, "rb");
  if (0 == handle_file_remote) {
    log_msg(ERRN, 43, file_remote);
    return -1;
  }
  ////////////////////////////////////////////
  // Truncate local file to zero length
  ////////////////////////////////////////////
  char file_local[PATH_MAX] = "/tmp/output_incoming.dat";
  FILE* handle_file_local = fopen(file_local, "wb");
  if (!handle_file_local) {
    rxs_fclose(handle_file_remote);
    // Remove remote file
    rxs_unlink(file_remote);
    return -1;
  }
  // Close remote file
  fclose(handle_file_local);
  ////////////////////////////////////////////
  uint32_t buf_recv_sz = (((have_encoder > 0) ? (crypt_packet_sz()) : (crypt_data_sz())) + 1);
  char* buf_recv = (char*)calloc(buf_recv_sz, sizeof(char));
  if (!buf_recv) {
    log_msg(ERRN, 44, buf_recv_sz);
    rxs_fclose(handle_file_remote);
    // Remove remote file
    rxs_unlink(file_remote);
    return -1;
  }
  //////////////////////////////////////////////////////////////////////////////////////////////////
  // Progress bar. Values
  //////////////////////////////////////////////////////////////////////////////////////////////////
  long file_sz = rxs_filesize(file_remote);
  uint8_t percent_last = 0;
  char lexeme[] = "Wait...";
  //////////////////////////////////////////////////////////////////////////////////////////////////
  // Read remote file by portions
  //////////////////////////////////////////////////////////////////////////////////////////////////
  long read_bytes_total = 0;
  //////////////////////////////////////////////////////////////////////////////////////////////////
  // Create RXS data point
  //////////////////////////////////////////////////////////////////////////////////////////////////
  while (1) {
    memset(buf_recv, 0, buf_recv_sz);
    // Read the remote file
    size_t read_bytes = rxs_fread(buf_recv, buf_recv_sz, sizeof(char), handle_file_remote);
    read_bytes_total += read_bytes;
    if ((rxs_errno() != 0) && (rxs_errno() != RXS_EOF)) {
      log_msg(ERRN, 45, file_remote);
      // Free memory
      free(buf_recv);
      buf_recv = NULL;
      rxs_fclose(handle_file_remote);
      // Remove remote file
      rxs_unlink(file_remote);
      return -1;
    }

    // Progress bar
    show_progress_bar(file_sz, read_bytes_total, &percent_last, lexeme);

    // Write to local file
    if (write_file(file_local, buf_recv, read_bytes, "ab") != 0) {
      log_msg(ERRN, 46, file_local);
      // Free memory
      free(buf_recv);
      buf_recv = NULL;
      rxs_fclose(handle_file_remote);
      // Remove remote file
      rxs_unlink(file_remote);
      return -1;
    }
    if (rxs_errno() == RXS_EOF) break;
  }
  // Close the remote file
  res = rxs_fclose(handle_file_remote);
  if (res) {
    log_msg(ERRN, 47, res, rxs_errno());
  }
  // Free memory
  free(buf_recv);
  buf_recv = NULL;
  // Remove remote file
  rxs_unlink(file_remote);
  // Copy path to local file
  strncpy(path_file, file_local, strlen(file_local));

  return (!errno_both_sides) ? (0) : (-1);
}
int rxs_mkdir(const char* path, mode_t mode) {
  // Set errno
  errno_both_sides = 0;
  ssize_t res = rqst_x03_resp_x00(get_socket_connected(), CS_A0, operation_mkdir, path, strlen(path), mode, NULL, 0,
                                  &errno_both_sides);
  // Set errno
  if (errno_both_sides) errno_both_sides = RXS_SRV_NONE + errno_both_sides;
  if (res < 0) {
    // Set errno
    if (!errno_both_sides) errno_both_sides = errno;
    // log_msg(ERRN, 6, "rqst_x03_resp_x00", strerror(errno));
    return -1;
  }
  return (!errno_both_sides) ? (0) : (-1);
}
int rxs_mkdir_ex(const char* path, mode_t mode) {
  // Set errno
  errno_both_sides = 0;
  ssize_t res = rqst_x03_resp_x00(get_socket_connected(), CS_A0, operation_mkdir_ex, path, strlen(path), mode, NULL, 0,
                                  &errno_both_sides);
  // Set errno
  if (errno_both_sides) errno_both_sides = RXS_SRV_NONE + errno_both_sides;
  if (res < 0) {
    // Set errno
    if (!errno_both_sides) errno_both_sides = errno;
    // log_msg(ERRN, 6, "rqst_x03_resp_x00", strerror(errno));
    return -1;
  }
  return (!errno_both_sides) ? (0) : (-1);
}
int rxs_rmdir(const char* path) {
  // Set errno
  errno_both_sides = 0;
  ssize_t res =
      rqst_x01_resp_x00(get_socket_connected(), CS_A0, operation_rmdir, path, strlen(path), NULL, 0, &errno_both_sides);
  // Set errno
  if (errno_both_sides) errno_both_sides = RXS_SRV_NONE + errno_both_sides;
  if (res < 0) {
    // Set errno
    if (!errno_both_sides) errno_both_sides = errno;
    // log_msg(ERRN, 6, "rqst_x01_resp_x00", strerror(errno));
    return -1;
  }
  return (!errno_both_sides) ? (0) : (-1);
}
char* rxs_getcwd(char* buf, size_t size) {
  // Set errno
  errno_both_sides = 0;
  ssize_t res = rqst_x00_resp_x00(get_socket_connected(), CS_A0, operation_getcwd, size, buf, size, &errno_both_sides);
  // Set errno
  if (errno_both_sides) errno_both_sides = RXS_SRV_NONE + errno_both_sides;
  if (res < 0) {
    // Set errno
    if (!errno_both_sides) errno_both_sides = errno;
    // log_msg(ERRN, 6, "rqst_x00_resp_x00", strerror(errno));
    return NULL;
  }
  return (!errno_both_sides) ? (buf) : (NULL);
}
int rxs_chdir(const char* path) {
  // Set errno
  errno_both_sides = 0;
  ssize_t res =
      rqst_x01_resp_x00(get_socket_connected(), CS_A0, operation_chdir, path, strlen(path), NULL, 0, &errno_both_sides);
  // Set errno
  if (errno_both_sides) errno_both_sides = RXS_SRV_NONE + errno_both_sides;
  if (res < 0) {
    // Set errno
    if (!errno_both_sides) errno_both_sides = errno;
    // log_msg(ERRN, 6, "rqst_x01_resp_x00", strerror(errno));
    return -1;
  }
  return (!errno_both_sides) ? (0) : (-1);
}
int rxs_unlink(const char* path) {
  // Set errno
  errno_both_sides = 0;
  ssize_t res = rqst_x01_resp_x00(get_socket_connected(), CS_A0, operation_unlink, path, strlen(path), NULL, 0,
                                  &errno_both_sides);
  // Set errno
  if (errno_both_sides) errno_both_sides = RXS_SRV_NONE + errno_both_sides;
  if (res < 0) {
    // Set errno
    if (!errno_both_sides) errno_both_sides = errno;
    // log_msg(ERRN, 6, "rqst_x01_resp_x00", strerror(errno));
    return -1;
  }
  return (!errno_both_sides) ? (0) : (-1);
}
int rxs_rename(const char* oldname, const char* newname) {
  // Set errno
  errno_both_sides = 0;
  ssize_t res = rqst_x02_resp_x00(get_socket_connected(), CS_A0, operation_rename, oldname, strlen(oldname), newname,
                                  strlen(newname), have_encoder, NULL, 0, &errno_both_sides);
  // Set errno
  if (errno_both_sides) errno_both_sides = RXS_SRV_NONE + errno_both_sides;
  if (res < 0) {
    // Set errno
    if (!errno_both_sides) errno_both_sides = errno;
    // log_msg(ERRN, 6, "rqst_x02_resp_x00", strerror(errno));
    return -1;
  }
  return (!errno_both_sides) ? (0) : (-1);
}
long rxs_filesize(const char* fname) {
  // Set errno
  errno_both_sides = 0;
  ssize_t res = rqst_x01_resp_x00(get_socket_connected(), CS_A0, operation_filesize, fname, strlen(fname), NULL, 0,
                                  &errno_both_sides);
  // Set errno
  if (errno_both_sides) errno_both_sides = RXS_SRV_NONE + errno_both_sides;
  if (res < 0) {
    // Set errno
    if (!errno_both_sides) errno_both_sides = errno;
    // log_msg(ERRN, 6, "rqst_x01_resp_x00", strerror(errno));
    return -1;
  }
  return (!errno_both_sides) ? ((long)res) : (-1);
}
RXS_HANDLE rxs_fopen(const char* fname, const char* mode) {
  // Set errno
  errno_both_sides = 0;
  ssize_t res = rqst_x02_resp_x00(get_socket_connected(), CS_A0, operation_fopen, fname, strlen(fname), mode,
                                  strlen(mode), have_encoder, NULL, 0, &errno_both_sides);
  // Set errno
  if (errno_both_sides) errno_both_sides = RXS_SRV_NONE + errno_both_sides;
  if (-1 == res) {
    // Set errno
    if (!errno_both_sides) errno_both_sides = errno;
    // log_msg(ERRN, 6, "rqst_x02_resp_x00", strerror(errno));
    return -1;
  }
  // return (!errno_both_sides)?(res):(0);
  //////////////////////////////////////////////////////////////////////////////////////////////////
  // Create RXS data point
  //////////////////////////////////////////////////////////////////////////////////////////////////
  // All Okay
  if (!errno_both_sides) {
    if (rxs_data_point_create_client(res) != 0) {
      rxs_point_close();
      return 0;
    }
    return res;
  }
  // Error
  else {
    return 0;
  }
}
size_t rxs_fread(void* buf, size_t size, size_t count, RXS_HANDLE stream) {
  if (!buf) {
    errno_both_sides = EINVAL;
    return 0;
  }
  size_t buf_sz = size * count;
  memset(buf, 0, buf_sz);
  if (get_socket_data_client() < 0) {
    errno_both_sides = EINVAL;
    return 0;
  }
  // Set errno
  errno_both_sides = 0;
  //////////////////////////////////////////////////////////////////////////////////
  // CAUTION: Zero data size don't send to avoid overload of empty/redundant operations on the server
  //////////////////////////////////////////////////////////////////////////////////
  if (buf_sz > LONG_MAX) {
    errno_both_sides = EINVAL;
    log_msg(ERRN, 50, buf_sz, LONG_MAX);
    return 0;
  }
  //////////////////////////////////////////////////////////////////////////////////
  // Send to other side size of receiver buffer
  //////////////////////////////////////////////////////////////////////////////////
  if (rxs_send_packet_x04(get_socket_connected(), CS_A0, operation_fread, stream, buf_sz, 0) < 0) {
    if (!errno_both_sides) errno_both_sides = EIO;
    // log_msg(ERRN, 6, "rxs_send_packet_x04", strerror(errno));
    return 0;
  }
  //////////////////////////////////////////////////////////////////////////////////
  // Start data receiver thread
  //////////////////////////////////////////////////////////////////////////////////
  data_exchange_t data_exchange;
  memset(&data_exchange, 0, sizeof(data_exchange));
  data_exchange.sockfd = get_socket_data_client();
  data_exchange.stream = stream;
  data_exchange.buf = buf;
  data_exchange.buf_sz = buf_sz;
  data_exchange.total_impl_channel_sz = 0;
  data_exchange.total_impl_sz = 0;
  data_exchange.complete = 0;

  int err_no = 0;
  if ((err_no = pthread_create(&data_receiver_thr, NULL, data_receiver, (void*)&data_exchange)) != 0) {
    log_msg(ERRN, 6, "pthread_create", strerror(err_no));
    return 0;
  }
  //////////////////////////////////////////////////////////////////////////////////
  // Wait whole portion data or EOF from other side
  //////////////////////////////////////////////////////////////////////////////////
  rxs_type_t type;
  uint32_t other_side_stream = 0;
  uint32_t other_side_data_sz = 0;
  uint16_t other_side_eof = 0;
  ssize_t res = rxs_recv_packet_x04(get_socket_connected(), &type, operation_fread, &other_side_stream,
                                    &other_side_data_sz, &other_side_eof);
  if ((res < 0) || (type != SC_B0) || (stream != other_side_stream)) {
    if (!errno_both_sides) errno_both_sides = EIO;
    // log_msg(ERRN, 6, "rxs_recv_packet_x04", strerror(errno));
    return 0;
  }

  if (other_side_eof || other_side_data_sz) {
    //////////////////////////////////////////////////////////////////////////////////
    // Send confirm to other side for to close operation
    //////////////////////////////////////////////////////////////////////////////////
    if (other_side_eof) {
      if (rxs_send_packet_x04(get_socket_connected(), CS_A0, operation_fread, stream,
                              data_exchange.total_impl_channel_sz, 0) < 0) {
        if (!errno_both_sides) errno_both_sides = EIO;
        // log_msg(ERRN, 6, "rxs_send_packet_x04", strerror(errno));
        return 0;
      }
    }

    while ((!other_side_eof && !data_exchange.complete) ||
           (other_side_eof && (other_side_data_sz > data_exchange.total_impl_channel_sz)))
      sleep_x(0, 100000);  // 100 microseconds

    //////////////////////////////////////////////////////////////////////////////////
    // Close data receiver thread
    //////////////////////////////////////////////////////////////////////////////////
    pthread_cancel(data_receiver_thr);
    int err_no = 0;
    void* res = NULL;
    if ((err_no = pthread_join(data_receiver_thr, &res)) != 0) log_msg(ERRN, 6, "pthread_join", strerror(err_no));

    errno_both_sides = other_side_eof;
  }
  return data_exchange.total_impl_sz;
}
size_t rxs_fwrite(const void* buf, size_t size, size_t count, RXS_HANDLE stream) {
  size_t buf_sz = size * count;
  if (get_socket_data_client() < 0) {
    errno_both_sides = EINVAL;
    return -1;
  }
  if (0 == buf_sz) {
    // Set errno
    errno_both_sides = EINVAL;
    log_msg(ERRN, 52, buf_sz);
    return 0;
  }
  if (buf_sz > LONG_MAX) {
    // Set errno
    errno_both_sides = EINVAL;
    log_msg(ERRN, 50, buf_sz, LONG_MAX);
    return 0;
  }
  //////////////////////////////////////////////////////////////////////////////////
  // Send to other side size of transmitted data
  //////////////////////////////////////////////////////////////////////////////////
  if (rxs_send_packet_x04(get_socket_connected(), CS_A0, operation_fwrite, stream, buf_sz, 0) < 0) {
    // Set errno
    if (!errno_both_sides) errno_both_sides = EIO;
    // log_msg(ERRN, 6, "rxs_send_packet_x04_1", strerror(errno));
    return 0;
  }
  size_t data_sz = ((have_encoder > 0) ? (crypt_packet_sz()) : (crypt_data_sz()));
  uint8_t* buf_send = calloc(data_sz, sizeof(uint8_t));
  if (!buf_send) {
    // Set errno
    errno_both_sides = EINVAL;
    log_msg(ERRN, 6, "calloc", strerror(errno));
    return 0;
  }
  size_t total_impl_sz = 0;
  // Set errno
  errno_both_sides = 0;
  while (total_impl_sz < buf_sz) {
    size_t data_regular_sz = portion_sz(buf_sz, total_impl_sz);
    uint16_t buf_send_sz = compose_data(have_encoder, buf, buf_send, data_sz, total_impl_sz, data_regular_sz);
    if (0 == buf_send_sz) {
      // Free memory
      free(buf_send);
      buf_send = NULL;
      // Set errno
      if (!errno_both_sides) errno_both_sides = EIO;
      log_msg(ERRN, 6, "compose_data", strerror(errno));
      return 0;
    }
    ssize_t impl_channel_sz = rxs_send_x(get_socket_data_client(), buf_send, buf_send_sz);
    if ((uint16_t)impl_channel_sz != buf_send_sz) {
      // Free memory
      free(buf_send);
      buf_send = NULL;
      // Set errno
      if (!errno_both_sides) errno_both_sides = EIO;
      log_msg(ERRN, 6, "rxs_send_x", strerror(errno));
      return 0;
    }
    total_impl_sz += data_regular_sz;
  }
  // Free memory
  free(buf_send);
  buf_send = NULL;
  //////////////////////////////////////////////////////////////////////////////////
  // Receive confirm from other side
  //////////////////////////////////////////////////////////////////////////////////
  rxs_type_t type;
  uint32_t other_side_stream = 0;
  uint32_t other_side_data_sz = 0;
  uint16_t other_side_eof = 0;
  ssize_t res = rxs_recv_packet_x04(get_socket_connected(), &type, operation_fwrite, &other_side_stream,
                                    &other_side_data_sz, &other_side_eof);
  if ((res < 0) || (type == SC_B1) || (other_side_stream != stream) || (total_impl_sz != other_side_data_sz)) {
    // Set errno
    if (!errno_both_sides) errno_both_sides = EIO;
    // log_msg(ERRN, 6, "rxs_recv_packet_x04", strerror(errno));
    return 0;
  }
  return total_impl_sz;
}
int rxs_fflush(RXS_HANDLE stream) {
  // Set errno
  errno_both_sides = 0;
  ssize_t res = rqst_x00_resp_x00(get_socket_connected(), CS_A0, operation_fflush, stream, NULL, 0, &errno_both_sides);
  // Set errno
  if (errno_both_sides) errno_both_sides = RXS_SRV_NONE + errno_both_sides;
  if (res < 0) {
    // Set errno
    if (!errno_both_sides) errno_both_sides = errno;
    // log_msg(ERRN, 6, "rqst_x00_resp_x00", strerror(errno));
    return -1;
  }

  return (!errno_both_sides) ? (0) : (-1);
}
int rxs_fclose(RXS_HANDLE stream) {
  // Set errno
  errno_both_sides = 0;
  rxs_data_point_close();
  ssize_t res = rqst_x00_resp_x00(get_socket_connected(), CS_A0, operation_fclose, stream, NULL, 0, &errno_both_sides);
  // Set errno
  if (errno_both_sides) errno_both_sides = RXS_SRV_NONE + errno_both_sides;
  if (res < 0) {
    // Set errno
    if (!errno_both_sides) errno_both_sides = errno;
    // log_msg(ERRN, 6, "rqst_x00_resp_x00", strerror(errno));
    return -1;
  }
  return (!errno_both_sides) ? (0) : (-1);
}
int rxs_fseek(RXS_HANDLE stream, long offset, int whence) {
  // Set errno
  errno_both_sides = 0;
  return -1;
  /*
  ssize_t res = rqst_x04_resp_x00(get_socket_connected(), CS_A0, operation_fseek, stream, offset, whence, NULL, 0,
  NULL, 0, &errno_both_sides); if(res < 0) { log_msg(ERRN, 6, "rqst_x04_resp_x00", strerror(errno)); return -1;
  }
  return (!errno_both_sides)?(0):(-1);
  */
}
long rxs_ftell(RXS_HANDLE stream) {
  // Set errno
  errno_both_sides = 0;
  return -1;
  /*
  ssize_t res = rqst_x00_resp_x00(get_socket_connected(), CS_A0, operation_ftell, stream, NULL, 0, &errno_both_sides);
  if(res < 0) {
      log_msg(ERRN, 6, "rqst_x00_resp_x00", strerror(errno));
      return -1;
  }
  return (!errno_both_sides)?((long)res):(-1);
  */
}
void rxs_rewind(RXS_HANDLE stream) {
  // Set errno
  errno_both_sides = 0;
  return;
  /*
  ssize_t res = rqst_x00_resp_x00(get_socket_connected(), CS_A0, operation_rewind, stream, NULL, 0,
  &errno_both_sides); if(res < 0) { log_msg(ERRN, 6, "rqst_x00_resp_x00", strerror(errno)); return ;
  }
  return;
  */
}
int rxs_file_exist(const char* path_name) {
  // Set errno
  errno_both_sides = 0;
  ssize_t res = rqst_x01_resp_x00(get_socket_connected(), CS_A0, operation_file_exist, path_name, strlen(path_name),
                                  NULL, 0, &errno_both_sides);
  // Set errno
  if (errno_both_sides) errno_both_sides = RXS_SRV_NONE + errno_both_sides;
  if (res < 0) {
    // Set errno
    if (!errno_both_sides) errno_both_sides = errno;
    // log_msg(ERRN, 6, "rqst_x01_resp_x00", strerror(errno));
    return -1;
  }
  return (!errno_both_sides) ? (res) : (-1);
}
int rxs_dir_exist(const char* path_name) {
  // Set errno
  errno_both_sides = 0;
  ssize_t res = rqst_x01_resp_x00(get_socket_connected(), CS_A0, operation_dir_exist, path_name, strlen(path_name),
                                  NULL, 0, &errno_both_sides);
  // Set errno
  if (errno_both_sides) errno_both_sides = RXS_SRV_NONE + errno_both_sides;
  if (res < 0) {
    // Set errno
    if (!errno_both_sides) errno_both_sides = errno;
    // log_msg(ERRN, 6, "rqst_x01_resp_x00", strerror(errno));
    return -1;
  }
  return (!errno_both_sides) ? (res) : (-1);
}
