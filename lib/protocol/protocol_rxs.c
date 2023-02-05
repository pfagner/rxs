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
#include <errno.h>     // for 'errno'
#include <inttypes.h>  // for 'PRIu32'
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/types.h>

#ifndef __QNXNTO__
#include <linux/limits.h>  // for 'PATH_MAX'
#include <netinet/tcp.h>   // for 'SOL_TCP'
// SOL_TCP is a synonym for IPPROTO_TCP
#define SOL_TCP IPPROTO_TCP
#else
#include <limits.h>
#include <netdb.h>        // for getprotobyname
#include <netinet/tcp.h>  // for TCP_MAXSEG
#define SOL_TCP 6         // CAUTION: it is for compatible calls socket options
#endif

#include "logger/logger.h"
#include "protocol/generic.h"
#include "protocol/protocol_rxs.h"

const uint16_t TCP_SEGMENT_SIZE = 1012;
const uint16_t TIME_INTERVAL_SEND_DATA_MSEC = 5 * 1000;  // Time interval for separating packets each from other
const uint16_t TCP_COMMAND_SEGMENT_SIZE = 612;
const uint16_t TCP_FLAG_SIZE = 12;
const uint32_t POLL_TIMEOUT_DATA_MSEC = 60 * 1000;
const uint32_t POLL_TIMEOUT_CONNECT_MSEC = 60 * 1000;
const uint32_t RXS_DATA_PORT = 1502;
const char RXS_SEPARATOR = '*';  // Packet's RXS separator
const uint16_t RXS_EOF = 0xFFFF;

uint8_t buf_remain[1024 * 128];
uint32_t buf_remain_sz = 0;

// Header size
size_t hdr_packet_rxs_t_sz() {
  packet_rxs_t packet_rxs;
  return sizeof(packet_rxs.sep1) + sizeof(packet_rxs.sep2) + sizeof(packet_rxs.sz) + sizeof(packet_rxs.type) +
         sizeof(packet_rxs.uid) + sizeof(packet_rxs.crc32) + sizeof(packet_rxs.operation);
}
// Slot size slot04_t
size_t hdr_slot04_t_sz() {
  slot04_t slot04;
  return sizeof(slot04.val1) + sizeof(slot04.data_sz) + sizeof(slot04.eof);
}
// Counter UID service packet
uint32_t create_uid_pkt(void) {
  static uint32_t uid_pkt;
  return ++uid_pkt;
}

uint16_t crypt_data_sz(void) { return MAX_PORTION_DATA_BYTES; }
uint16_t crypt_packet_sz(void) {
  crypt_data_t crypt_data;
  return sizeof(crypt_data.key_info) + sizeof(crypt_data.len) + sizeof(crypt_data.data) + sizeof(crypt_data.imit);
}
//////////////////////////////////////////////////////////////////////////////////////////////////
// Packet
//////////////////////////////////////////////////////////////////////////////////////////////////
ssize_t init_packet_rxs_t(packet_rxs_t* packet_rxs) {
  if (!packet_rxs) return -1;

  packet_rxs->sep1 = 0;
  packet_rxs->sep2 = 0;
  packet_rxs->sz = 0;
  packet_rxs->type = 0;
  packet_rxs->uid = 0;
  packet_rxs->crc32 = 0;
  packet_rxs->operation = 0;
  packet_rxs->data = NULL;

  return 0;
}
ssize_t dinit_packet_rxs_t(packet_rxs_t* packet_rxs) {
  if (!packet_rxs) return -1;

  packet_rxs->sep1 = 0;
  packet_rxs->sep2 = 0;
  packet_rxs->sz = 0;
  packet_rxs->type = 0;
  packet_rxs->uid = 0;
  packet_rxs->crc32 = 0;
  packet_rxs->operation = 0;
  free(packet_rxs->data);
  packet_rxs->data = NULL;

  return 0;
}
//////////////////////////////////////////////////////////////////////////////////////////////////
// Data slots
//////////////////////////////////////////////////////////////////////////////////////////////////
ssize_t init_slot00_t(slot00_t* slot00) {
  if (!slot00) return -1;
  slot00->val = 0;
  return 0;
}
ssize_t dinit_slot00_t(slot00_t* slot00) { return init_slot00_t(slot00); }
ssize_t init_slot01_t(slot01_t* slot01) {
  if (!slot01) return -1;
  slot01->data_sz = 0;
  slot01->data = NULL;
  return 0;
}
ssize_t dinit_slot01_t(slot01_t* slot01) {
  if (!slot01) return -1;
  slot01->data_sz = 0;
  free(slot01->data);
  slot01->data = NULL;
  return 0;
}
ssize_t init_slot02_t(slot02_t* slot02) {
  if (!slot02) return -1;
  slot02->data1_sz = 0;
  slot02->data1 = NULL;
  slot02->data2_sz = 0;
  slot02->data2 = NULL;
  return 0;
}
ssize_t dinit_slot02_t(slot02_t* slot02) {
  if (!slot02) return -1;
  slot02->data1_sz = 0;
  free(slot02->data1);
  slot02->data1 = NULL;
  slot02->data2_sz = 0;
  free(slot02->data2);
  slot02->data2 = NULL;

  return 0;
}
ssize_t init_slot03_t(slot03_t* slot03) {
  if (!slot03) return -1;
  slot03->val = 0;
  slot03->data_sz = 0;
  slot03->data = NULL;
  return 0;
}
ssize_t dinit_slot03_t(slot03_t* slot03) {
  if (!slot03) return -1;
  slot03->val = 0;
  slot03->data_sz = 0;
  free(slot03->data);
  slot03->data = NULL;
  return 0;
}
ssize_t init_slot04_t(slot04_t* slot04) {
  if (!slot04) return -1;
  slot04->val1 = 0;
  slot04->data_sz = 0;
  slot04->eof = 0;
  return 0;
}
ssize_t dinit_slot04_t(slot04_t* slot04) { return init_slot04_t(slot04); }
ssize_t init_slot05_t(slot05_t* slot05) {
  if (!slot05) return -1;
  slot05->stream_id = 0;
  slot05->port = 0;
  return 0;
}
ssize_t dinit_slot05_t(slot05_t* slot05) { return init_slot05_t(slot05); }

ssize_t init_crypt_data_t(crypt_data_t* crypt_data) {
  if (!crypt_data) return -1;
  memset(crypt_data->key_info, 0, sizeof(crypt_data->key_info));
  crypt_data->len = 0;
  memset(crypt_data->data, 0, sizeof(crypt_data->data));
  memset(crypt_data->imit, 0, sizeof(crypt_data->imit));
  return 0;
}
ssize_t dinit_crypt_data_t(crypt_data_t* crypt_data) { return init_crypt_data_t(crypt_data); }

//////////////////////////////////////////////////////////////////////////////////
// data exchange functions
//////////////////////////////////////////////////////////////////////////////////
ssize_t rxs_send_x(int sockfd, void* buf, size_t buf_sz) {
  //////////////////////////////////////////////////////////////////////////////////////
  // POLL MODE
  //////////////////////////////////////////////////////////////////////////////////////
  struct pollfd sockfd_poll[1] = {{-1}};
  memset(sockfd_poll, -1, sizeof(sockfd_poll));
  sockfd_poll[0].fd = sockfd;
  sockfd_poll[0].events = POLLOUT;

  int ret_code = poll(sockfd_poll, 1, POLL_TIMEOUT_DATA_MSEC);
  if (ret_code > 0) {
    if ((sockfd_poll[0].revents & POLLOUT) && (sockfd_poll[0].fd == sockfd)) {
      ssize_t impl_send = send(sockfd, buf, buf_sz, MSG_NOSIGNAL);
      if ((impl_send < 0) || (buf_sz != (size_t)impl_send)) {
        log_msg(ERRN, 6, "send", strerror(errno));
        return -1;
      }
      return impl_send;
    }
  } else {
    if (ret_code == 0) log_msg(WARN, 6, "poll() send timeout", strerror(errno));
    if (ret_code < 0) log_msg(ERRN, 6, "poll() send error", strerror(errno));
    return -1;
  }
  return -1;

  //////////////////////////////////////////////////////////////////////////////////////
  // BLOCKED MODE
  //////////////////////////////////////////////////////////////////////////////////////
  /*
  s size_t impl_send = send(sockfd, buf, buf_sz, MSG_NOSIGNAL);
  if(impl_send < 0)
  {
      log_msg(ERRN, 6, "send", strerror(errno));
      return -1;
  }
  else
  {
      if(buf_sz != (size_t)impl_send)
      {
          log_msg(ERRN, 6, "send", strerror(errno));
          return -1;
      }
  }
  return impl_send;
  */
}
ssize_t rxs_recv_x(int sockfd, void* buf, size_t buf_sz) {
  //////////////////////////////////////////////////////////////////////////////////////
  // POLL MODE
  //////////////////////////////////////////////////////////////////////////////////////
  struct pollfd sockfd_poll[1] = {{-1}};
  memset(sockfd_poll, -1, sizeof(sockfd_poll));
  sockfd_poll[0].fd = sockfd;
  sockfd_poll[0].events = POLLIN;

  int ret_code = poll(sockfd_poll, 1, POLL_TIMEOUT_DATA_MSEC);
  if (ret_code > 0) {
    if ((sockfd_poll[0].revents & POLLIN) && (sockfd_poll[0].fd == sockfd)) {
      ssize_t impl_recv_sz = recv(sockfd, buf, buf_sz, 0);
      if (impl_recv_sz < 0) {
        log_msg(ERRN, 6, "recv", strerror(errno));
        return -1;
      }
      return impl_recv_sz;
    }
  } else {
    if (ret_code == 0) log_msg(WARN, 6, "poll() recv timeout", strerror(errno));
    if (ret_code < 0) log_msg(ERRN, 6, "poll() recv error", strerror(errno));
    return -1;
  }
  return -1;

  //////////////////////////////////////////////////////////////////////////////////////
  // BLOCKED MODE
  //////////////////////////////////////////////////////////////////////////////////////
  /*
  ssize_t impl_recv_sz = recv(sockfd, buf, buf_sz, 0);
  if(-1 == impl_recv_sz)
  {
      log_msg(ERRN, 6, "recv", strerror(errno));
      return -1;
  }
  return impl_recv_sz;
  */
}
ssize_t rxs_recv_block_x(int sockfd, void* buf, size_t buf_sz, size_t block_sz) {
  ssize_t impl_sz = 0;
  if (block_sz) {
    while (impl_sz < block_sz) {
      ssize_t chunk_sz = rxs_recv_x(sockfd, buf + impl_sz, buf_sz - impl_sz);
      impl_sz += chunk_sz;
    }
  } else {
    impl_sz = rxs_recv_x(sockfd, buf + impl_sz, buf_sz - impl_sz);
  }
  return impl_sz;
}
ssize_t rxs_send_packet(int sockfd, packet_rxs_t* packet_rxs) {
  if ((sockfd < 0) || (!packet_rxs)) return -1;
  // Serialize packet
  uint8_t* data = NULL;
  size_t data_sz = 0;
  if (serialize_packet_rxs_t(packet_rxs, &data, &data_sz) < 0) return -1;
  // Send packet
  uint32_t count_try_max = 10;
  uint32_t count_try_cur = 0;
  ssize_t impl_send = -1;
  while (count_try_cur < count_try_max) {
    impl_send = rxs_send_x(sockfd, data, data_sz);
    if (impl_send == data_sz) break;
    // Need to wait
    sleep_x(0, TIME_INTERVAL_SEND_DATA_MSEC * 100000);  // total 500 Milliseconds
    // Increase count attepts
    count_try_cur++;
  }
  // Free memory
  free(data);
  data = NULL;
  // Free memory
  dinit_packet_rxs_t(packet_rxs);
  return impl_send;
}
ssize_t rxs_send_packet_x04(int sockfd, rxs_type_t type, rxs_operation_t operation, uint32_t stream, uint32_t data_sz,
                            uint16_t eof) {
  //////////////////////////////////////////////////////////////////////////////////
  // CAUTION: this action 'send' with zero data size, only for confirm received operation from other side
  //////////////////////////////////////////////////////////////////////////////////
  // Send packet
  packet_rxs_t packet_rxs_send;
  if (compose_packet_rxs_x04(type, operation, stream, data_sz, eof, &packet_rxs_send) < 0) {
    return -1;
  }
  // Send packet
  ssize_t impl_send = rxs_send_packet(sockfd, &packet_rxs_send);
  if (impl_send < 0) {
    return -1;
  }
  return 0;
}
ssize_t rxs_recv_packet_x04(int sockfd, rxs_type_t* type, rxs_operation_t operation, uint32_t* stream,
                            uint32_t* data_sz, uint16_t* eof) {
  if (sockfd < 0) {
    return -1;
  }

  uint32_t buf_recv_sz = 1024 * 1024;
  uint8_t* buf_recv = (uint8_t*)calloc(buf_recv_sz, sizeof(uint8_t));
  packet_rxs_t packet_rxs_recv;
  slot04_t slot04;
  init_slot04_t(&slot04);
  if (!buf_recv) {
    log_msg(ERRN, 6, "calloc", strerror(errno));
    return -1;
  }
  for (;;) {
    ssize_t impl_recv_sz = rxs_recv_data_x(sockfd, buf_recv, buf_recv_sz);
    if (impl_recv_sz <= 0) {
      // log_msg(ERRN, 6, "rxs_recv_data_x", strerror(errno));
      // Free memory
      free(buf_recv);
      buf_recv = NULL;
      return -1;
    } else {
      init_packet_rxs_t(&packet_rxs_recv);
      if (deserialize_packet_rxs_t(buf_recv, (size_t)impl_recv_sz, &packet_rxs_recv) < 0) {
        // log_msg(ERRN, 6, "deserialize_packet_rxs_t", strerror(errno));
        // Free memory
        free(buf_recv);
        buf_recv = NULL;
        // Free memory
        dinit_packet_rxs_t(&packet_rxs_recv);
        return -1;
      }
    }
    if (packet_rxs_recv.operation == operation) {
      if (deserialize_slot04_t(packet_rxs_recv.data, (packet_rxs_recv.sz - hdr_packet_rxs_t_sz()), &slot04) < 0) {
        // log_msg(ERRN, 6, "deserialize_slot04_t", strerror(errno));
        // Free memory
        free(buf_recv);
        buf_recv = NULL;
        // Free memory
        dinit_packet_rxs_t(&packet_rxs_recv);
        dinit_slot04_t(&slot04);
        return -1;
      }
      *type = packet_rxs_recv.type;
      *stream = slot04.val1;
      *data_sz = slot04.data_sz;
      *eof = slot04.eof;
      free(buf_recv);
      buf_recv = NULL;
      dinit_packet_rxs_t(&packet_rxs_recv);
      dinit_slot04_t(&slot04);
      break;
    } else {
      free(buf_recv);
      buf_recv = NULL;
      dinit_packet_rxs_t(&packet_rxs_recv);
      dinit_slot04_t(&slot04);
      return -1;
    }
  }
  return 0;
}
ssize_t rxs_send_packet_x05(int sockfd, rxs_type_t type, rxs_operation_t operation, uint32_t stream, uint16_t port) {
  size_t imp_total_data_sz = 0;

  // Send packet
  packet_rxs_t packet_rxs_send;
  compose_packet_rxs_x05(type, operation, stream, port, &packet_rxs_send);
  // Send packet
  ssize_t impl_send = rxs_send_packet(sockfd, &packet_rxs_send);
  if (impl_send < 0) return -1;
  return imp_total_data_sz;
}
//
ssize_t rxs_recv_slot0x(int sockfd, void* buf, size_t size, void* slot0x, int* errno_other_side) {
  if (sockfd < 0) return -1;

  size_t count = 1;
  uint32_t buf_recv_sz = 1024 * 1024;
  uint8_t* buf_recv = (uint8_t*)calloc(buf_recv_sz, sizeof(uint8_t));
  if (!buf_recv) {
    log_msg(ERRN, 6, "calloc", strerror(errno));
    return -1;
  }
  for (;;) {
    memset(buf_recv, 0, buf_recv_sz);
    ssize_t impl_recv_sz = rxs_recv_data_x(sockfd, buf_recv, buf_recv_sz);
    if (impl_recv_sz <= 0) {
      // log_msg(ERRN, 6, "rxs_recv_data_x", strerror(errno));
      // Free memory
      free(buf_recv);
      buf_recv = NULL;
      return -1;
    } else {
      //////////////////////////////////////////////////////////////////////////////////
      // Main block. Begin
      //////////////////////////////////////////////////////////////////////////////////
      // Compose RXS packet
      packet_rxs_t packet_rxs_recv;
      init_packet_rxs_t(&packet_rxs_recv);
      if (deserialize_packet_rxs_t(buf_recv, (size_t)impl_recv_sz, &packet_rxs_recv) < 0) {
        // log_msg(ERRN, 6, "deserialize_packet_rxs_t", strerror(errno));
        // Free memory
        free(buf_recv);
        buf_recv = NULL;
        // Free memory
        dinit_packet_rxs_t(&packet_rxs_recv);
        return -1;
      }
      // code B0
      if (packet_rxs_recv.type == SC_B0) {
        switch (packet_rxs_recv.operation) {
          //////////////////////////////////////////////////////////////////////////////////
          // Those packets that are HAVE data in response.
          //////////////////////////////////////////////////////////////////////////////////
          case operation_ls:
          case operation_getcwd: {
            slot01_t slot01;
            init_slot01_t(&slot01);
            deserialize_slot01_t(packet_rxs_recv.data, (packet_rxs_recv.sz - hdr_packet_rxs_t_sz()), &slot01);
            // Check the incoming data size
            if (slot01.data_sz > (size * count)) {
              log_msg(ERRN, 15, slot01.data_sz, (size * count));
              // Free memory
              free(buf_recv);
              buf_recv = NULL;
              // Free memory
              dinit_packet_rxs_t(&packet_rxs_recv);
              dinit_slot01_t(&slot01);
              return -1;
            }
            memcpy(buf, slot01.data, slot01.data_sz);
            dinit_slot01_t(&slot01);
            break;
          }
          //////////////////////////////////////////////////////////////////////////////////
          // Those packets that are DON'T HAVE data in response.
          //////////////////////////////////////////////////////////////////////////////////
          case operation_rewind:
          case operation_fflush:
          case operation_fclose:
          case operation_ftell:
          case operation_filesize:
          case operation_authorization:
          case operation_mkdir:
          case operation_mkdir_ex:
          case operation_rmdir:
          case operation_chdir:
          case operation_unlink:
          case operation_rename:
          case operation_fopen:
          case operation_fseek:
          case operation_file_exist:
          case operation_dir_exist:
          case operation_port: {
            slot00_t* slot00 = (slot00_t*)slot0x;
            // slot00_t slot00;
            // init_slot00_t(&slot00);
            deserialize_slot00_t(packet_rxs_recv.data, (packet_rxs_recv.sz - hdr_packet_rxs_t_sz()), slot00);
            // variuos_value = slot00.val;
            // dinit_slot00_t(&slot00);
            break;
          }
        }
      }
      // code B1
      else {
        slot00_t slot00;
        init_slot00_t(&slot00);
        deserialize_slot00_t(packet_rxs_recv.data, (packet_rxs_recv.sz - hdr_packet_rxs_t_sz()), &slot00);
        // Set errno
        *errno_other_side = slot00.val;
        dinit_slot00_t(&slot00);
      }

      uint8_t pkt_type = packet_rxs_recv.type;
      uint16_t pkt_operation = packet_rxs_recv.operation;
      //////////////////////////////////////////////////////////////////////////////////////////////////
      // Free memory
      //////////////////////////////////////////////////////////////////////////////////////////////////
      dinit_packet_rxs_t(&packet_rxs_recv);

      //////////////////////////////////////////////////////////////////////////////////////////////////
      // Exit
      //////////////////////////////////////////////////////////////////////////////////////////////////
      if (pkt_type == SC_B1) {
        // Free memory
        free(buf_recv);
        buf_recv = NULL;
        return 0;
      }

      if (pkt_type == SC_B0) {
        switch (pkt_operation) {
          case operation_authorization:
          case operation_mkdir:
          case operation_mkdir_ex:
          case operation_rmdir:
          case operation_ls:
          case operation_getcwd:
          case operation_chdir:
          case operation_unlink:
          case operation_rename:
          case operation_fopen:
          case operation_fseek:
          case operation_file_exist:
          case operation_dir_exist:
          case operation_rewind:
          case operation_fflush:
          case operation_fclose:
          case operation_ftell:
          case operation_filesize:
          case operation_port: {
            // Free memory
            free(buf_recv);
            buf_recv = NULL;
            return 0;  // variuos_value
          }
        }
      }
      //////////////////////////////////////////////////////////////////////////////////
      // Main block. End
      //////////////////////////////////////////////////////////////////////////////////
    }
  }
  // Free memory
  free(buf_recv);
  buf_recv = NULL;

  return 0;
}
ssize_t rxs_recv_data_x(int sockfd, uint8_t* data, uint32_t data_sz) {
  if ((sockfd < 0) || (!data)) {
    log_msg(ERRN, 14);
    return -1;
  }
  uint32_t buf_recv_sz = 1024 * 1024;
  uint8_t* buf_recv = (uint8_t*)calloc(buf_recv_sz, sizeof(uint8_t));
  if (!buf_recv) {
    log_msg(ERRN, 6, "calloc", strerror(errno));
    return -1;
  }

  if (buf_remain_sz > 0) memcpy(buf_recv, buf_remain, buf_remain_sz);
  uint8_t* buf_read = buf_recv + buf_remain_sz;
  uint32_t buf_read_sz = buf_recv_sz - buf_remain_sz;
  uint32_t buf_read_total_sz = buf_remain_sz;
  uint8_t* buf_view = &buf_recv[0];

  uint8_t once_read_remained_data = (buf_remain_sz > 0) ? (1) : (0);
  for (;;) {
    ssize_t impl_recv_sz = 0;

    if (0 == once_read_remained_data) impl_recv_sz = rxs_recv_x(sockfd, buf_read, buf_read_sz);
    if ((impl_recv_sz > 0) || (1 == once_read_remained_data)) {
      once_read_remained_data = 0;
      buf_read_total_sz += (uint32_t)impl_recv_sz;
      uint32_t pos_out = 0;
      uint32_t packet_size_out = 0;
      ssize_t found = find_header_packet_rxs(buf_view, buf_read_total_sz, &pos_out, &packet_size_out);
      // Unexpected error
      if (-1 == found) {
        log_msg(ERRN, 20);
        //
        memset(buf_remain, 0, sizeof(buf_remain));
        buf_remain_sz = 0;
        // Free memory
        free(buf_recv);
        buf_recv = NULL;
        return -1;
      }
      // CRC32 error
      if (-2 == found) {
        log_msg(ERRN, 21);
        //
        memset(buf_remain, 0, sizeof(buf_remain));
        buf_remain_sz = 0;
        // Free memory
        free(buf_recv);
        buf_recv = NULL;
        return -1;
      }
      // All completed
      if (2 == found) {
        //////////////////////////////////////////////////////////////////////////////////
        // Main block. Begin
        //////////////////////////////////////////////////////////////////////////////////
        if (data_sz >= packet_size_out)
          memcpy(data, buf_view + pos_out, packet_size_out);
        else
          log_msg(ERRN, 17, packet_size_out, data_sz);
        //////////////////////////////////////////////////////////////////////////////////
        // Main block. End
        //////////////////////////////////////////////////////////////////////////////////
        buf_read_total_sz -= packet_size_out;
        if (buf_read_total_sz == 0) {
          buf_read = buf_recv;
          buf_read_sz = buf_recv_sz;
          buf_view = buf_read;
          //
          memset(buf_remain, 0, sizeof(buf_remain));
          buf_remain_sz = 0;
        } else {
          buf_view = buf_view + pos_out + packet_size_out;
          buf_read += buf_read_total_sz;
          buf_read_sz -= buf_read_total_sz;
          // Remain
          memset(buf_remain, 0, sizeof(buf_remain));
          memcpy(buf_remain, buf_view, buf_read_total_sz);
          buf_remain_sz = buf_read_total_sz;
        }
        // Free memory
        free(buf_recv);
        buf_recv = NULL;

        return packet_size_out;
      }
      // Header is found || Header not found
      if ((1 == found) || (0 == found)) {
        // buf_view = buf_read;
        // buf_read += buf_remain_sz;
        buf_read += impl_recv_sz;
        buf_read_sz -= impl_recv_sz;
      }
      // CAUTION! Check the buffer range
      if (buf_read >= (buf_recv + buf_recv_sz)) {
        log_msg(WARN, 18);
        buf_view = buf_recv;
        buf_read = buf_recv;
        buf_read_sz = buf_recv_sz;
        buf_read_total_sz = 0;
        //
        memset(buf_remain, 0, sizeof(buf_remain));
        buf_remain_sz = 0;
      }
    }
    // CAUTION: The other side has closed socket
    else {
      log_msg(INFO, 19);
      break;
    }
  }
  // Free memory
  free(buf_recv);
  buf_recv = NULL;
  return 0;
}
//////////////////////////////////////////////////////////////////////////////////
// Serialize/Deserialize
//////////////////////////////////////////////////////////////////////////////////
// Data serialization
ssize_t serialize_slot00_t(void* slot0x, uint8_t** data, size_t* data_sz) {
  if (!slot0x || !data || !data_sz) {
    log_msg(ERRN, 14);
    return -1;
  }
  slot00_t* slot00 = (slot00_t*)slot0x;
  size_t sz = sizeof(slot00->val);
  *data = (uint8_t*)calloc(sz, sizeof(uint8_t));
  if (!*data) {
    log_msg(ERRN, 6, "calloc", strerror(errno));
    return -1;
  }

  size_t offset = 0;
  // Set data
  serialize_uint32_t(*data + offset, htonl(slot00->val));
  offset += sizeof(slot00->val);
  // Set size
  *data_sz = offset;

  return 0;
}
ssize_t deserialize_slot00_t(uint8_t* data, size_t data_sz, slot00_t* slot00) {
  if (!slot00 || !data || !data_sz) {
    log_msg(ERRN, 14);
    return -1;
  }
  size_t offset = 0;
  // Set data
  deserialize_uint32_t(data + offset, &slot00->val);
  offset += sizeof(slot00->val);
  slot00->val = ntohl(slot00->val);

  return 0;
}
ssize_t serialize_slot01_t(void* slot0x, uint8_t** data, size_t* data_sz) {
  if (!slot0x || !data || !data_sz) {
    log_msg(ERRN, 14);
    return -1;
  }
  slot01_t* slot01 = (slot01_t*)slot0x;
  size_t sz = sizeof(slot01->data_sz) + slot01->data_sz;
  *data = (uint8_t*)calloc(sz, sizeof(uint8_t));
  if (!*data) {
    log_msg(ERRN, 6, "calloc", strerror(errno));
    return -1;
  }
  size_t offset = 0;
  // Set data
  //
  serialize_uint32_t(*data + offset, htonl(slot01->data_sz));
  offset += sizeof(slot01->data_sz);
  memcpy(*data + offset, slot01->data, slot01->data_sz);
  offset += slot01->data_sz;

  // Set size
  *data_sz = offset;

  return 0;
}
ssize_t deserialize_slot01_t(uint8_t* data, size_t data_sz, slot01_t* slot01) {
  if (!slot01 || !data || !data_sz) {
    log_msg(ERRN, 14);
    return -1;
  }
  size_t offset = 0;
  // Set data
  //
  deserialize_uint32_t(data + offset, &slot01->data_sz);
  offset += sizeof(slot01->data_sz);
  slot01->data_sz = ntohl(slot01->data_sz);
  //
  slot01->data = (uint8_t*)calloc(slot01->data_sz + 1, sizeof(uint8_t));
  if (!slot01->data) {
    log_msg(ERRN, 6, "calloc", strerror(errno));
    return -1;
  }
  memcpy(slot01->data, data + offset, slot01->data_sz);
  // offset += slot01->data_sz;

  return 0;
}
ssize_t serialize_slot02_t(void* slot0x, uint8_t** data, size_t* data_sz) {
  if (!slot0x || !data || !data_sz) {
    log_msg(ERRN, 14);
    return -1;
  }
  slot02_t* slot02 = (slot02_t*)slot0x;
  size_t sz = sizeof(slot02->data1_sz) + slot02->data1_sz + sizeof(slot02->data2_sz) + slot02->data2_sz +
              sizeof(slot02->encoder);
  *data = (uint8_t*)calloc(sz, sizeof(uint8_t));
  if (!*data) {
    log_msg(ERRN, 6, "calloc", strerror(errno));
    return -1;
  }
  size_t offset = 0;
  // Set data
  //
  serialize_uint32_t(*data + offset, htonl(slot02->data1_sz));
  offset += sizeof(slot02->data1_sz);
  //
  memcpy(*data + offset, slot02->data1, slot02->data1_sz);
  offset += slot02->data1_sz;
  //
  serialize_uint32_t(*data + offset, htonl(slot02->data2_sz));
  offset += sizeof(slot02->data2_sz);
  //
  memcpy(*data + offset, slot02->data2, slot02->data2_sz);
  offset += slot02->data2_sz;
  //
  memcpy(*data + offset, &slot02->encoder, sizeof(slot02->encoder));
  offset += sizeof(slot02->encoder);
  // Set size
  *data_sz = offset;

  return 0;
}
ssize_t deserialize_slot02_t(uint8_t* data, size_t data_sz, slot02_t* slot02) {
  if (!slot02 || !data || !data_sz) {
    log_msg(ERRN, 14);
    return -1;
  }
  size_t offset = 0;
  // Set data
  //
  deserialize_uint32_t(data + offset, &slot02->data1_sz);
  offset += sizeof(slot02->data1_sz);
  slot02->data1_sz = ntohl(slot02->data1_sz);
  //
  slot02->data1 = (uint8_t*)calloc(slot02->data1_sz + 1, sizeof(uint8_t));
  if (!slot02->data1) {
    log_msg(ERRN, 6, "calloc", strerror(errno));
    return -1;
  }
  memcpy(slot02->data1, data + offset, slot02->data1_sz);
  offset += slot02->data1_sz;
  //
  deserialize_uint32_t(data + offset, &slot02->data2_sz);
  offset += sizeof(slot02->data2_sz);
  slot02->data2_sz = ntohl(slot02->data2_sz);
  //
  slot02->data2 = (uint8_t*)calloc(slot02->data2_sz + 1, sizeof(uint8_t));
  if (!slot02->data2) {
    log_msg(ERRN, 6, "calloc", strerror(errno));
    return -1;
  }
  memcpy(slot02->data2, data + offset, slot02->data2_sz);
  offset += slot02->data2_sz;
  //
  deserialize_uint8_t(data + offset, &slot02->encoder);
  // offset += sizeof(slot02->encoder);

  return 0;
}
ssize_t serialize_slot03_t(void* slot0x, uint8_t** data, size_t* data_sz) {
  if (!slot0x || !data || !data_sz) {
    log_msg(ERRN, 14);
    return -1;
  }
  slot03_t* slot03 = (slot03_t*)slot0x;
  size_t sz = sizeof(slot03->data_sz) + slot03->data_sz + sizeof(slot03->val);
  *data = (uint8_t*)calloc(sz, sizeof(uint8_t));
  if (!*data) {
    log_msg(ERRN, 6, "calloc", strerror(errno));
    return -1;
  }
  size_t offset = 0;
  // Set data
  //
  serialize_uint32_t(*data + offset, htonl(slot03->data_sz));
  offset += sizeof(slot03->data_sz);
  //
  memcpy(*data + offset, slot03->data, slot03->data_sz);
  offset += slot03->data_sz;
  //
  serialize_uint32_t(*data + offset, htonl(slot03->val));
  offset += sizeof(slot03->val);
  // Set size
  *data_sz = offset;

  return 0;
}
ssize_t deserialize_slot03_t(uint8_t* data, size_t data_sz, slot03_t* slot03) {
  if (!slot03 || !data || !data_sz) {
    log_msg(ERRN, 14);
    return -1;
  }
  size_t offset = 0;
  // Set data
  //
  deserialize_uint32_t(data + offset, &slot03->data_sz);
  offset += sizeof(slot03->data_sz);
  slot03->data_sz = ntohl(slot03->data_sz);

  slot03->data = (uint8_t*)calloc(slot03->data_sz + 1, sizeof(uint8_t));
  if (!slot03->data) {
    log_msg(ERRN, 6, "calloc", strerror(errno));
    return -1;
  }
  memcpy(slot03->data, data + offset, slot03->data_sz);
  offset += slot03->data_sz;
  //
  deserialize_uint32_t(data + offset, &slot03->val);
  offset += sizeof(slot03->val);
  slot03->val = ntohl(slot03->val);

  return 0;
}
ssize_t serialize_slot04_t(void* slot0x, uint8_t** data, size_t* data_sz) {
  if (!slot0x) {
    log_msg(ERRN, 14);
    return -1;
  }
  slot04_t* slot04 = (slot04_t*)slot0x;
  *data = (uint8_t*)calloc(sizeof(slot04->val1) + sizeof(slot04->data_sz) + sizeof(slot04->eof), sizeof(uint8_t));
  if (!*data) {
    log_msg(ERRN, 6, "calloc", strerror(errno));
    return -1;
  }
  size_t offset = 0;
  // Set data
  //
  serialize_uint32_t(*data + offset, htonl(slot04->val1));
  offset += sizeof(slot04->val1);
  serialize_uint32_t(*data + offset, htonl(slot04->data_sz));
  offset += sizeof(slot04->data_sz);
  serialize_uint16_t(*data + offset, htons(slot04->eof));
  offset += sizeof(slot04->eof);
  // Set size
  *data_sz = offset;

  return 0;
}
ssize_t deserialize_slot04_t(uint8_t* data, size_t data_sz, slot04_t* slot04) {
  if (!slot04) {
    log_msg(ERRN, 14);
    return -1;
  }
  size_t offset = 0;
  // Set data
  //
  deserialize_uint32_t(data + offset, &slot04->val1);
  offset += sizeof(slot04->val1);
  slot04->val1 = ntohl(slot04->val1);

  deserialize_uint32_t(data + offset, &slot04->data_sz);
  offset += sizeof(slot04->data_sz);
  slot04->data_sz = ntohl(slot04->data_sz);

  deserialize_uint16_t(data + offset, &slot04->eof);
  offset += sizeof(slot04->eof);
  slot04->eof = ntohs(slot04->eof);
  return 0;
}
ssize_t serialize_slot05_t(void* slot0x, uint8_t** data, size_t* data_sz) {
  if (!slot0x || !data || !data_sz) {
    log_msg(ERRN, 14);
    return -1;
  }
  slot05_t* slot05 = (slot05_t*)slot0x;
  size_t sz = sizeof(slot05->stream_id) + sizeof(slot05->port);
  *data = (uint8_t*)calloc(sz, sizeof(uint8_t));
  if (!*data) {
    log_msg(ERRN, 6, "calloc", strerror(errno));
    return -1;
  }
  size_t offset = 0;
  // stream_id
  serialize_uint32_t(*data + offset, slot05->stream_id);
  offset += sizeof(slot05->stream_id);
  // port
  serialize_uint16_t(*data + offset, slot05->port);
  offset += sizeof(slot05->port);
  // Set size
  *data_sz = offset;

  return 0;
}
ssize_t deserialize_slot05_t(uint8_t* data, size_t data_sz, slot05_t* slot05) {
  if (!slot05 || !data) {
    log_msg(ERRN, 14);
    return -1;
  }
  size_t offset = 0;
  // stream_id
  deserialize_uint32_t(data + offset, &slot05->stream_id);
  offset += sizeof(slot05->stream_id);
  // port
  deserialize_uint16_t(data + offset, &slot05->port);
  // offset += sizeof(slot05->port);

  return 0;
}
ssize_t serialize_crypt_data_t(void* crypt_data_x, uint8_t** data) {
  if (!crypt_data_x || !data) {
    log_msg(ERRN, 14);
    return -1;
  }
  crypt_data_t* crypt_data = (crypt_data_t*)crypt_data_x;
  size_t offset = 0;
  // Set key_info
  memcpy((*data) + offset, crypt_data->key_info, sizeof(crypt_data->key_info));
  offset += sizeof(crypt_data->key_info);
  // Set len
  uint16_t be_len = htons(crypt_data->len);
  serialize_uint16_t((*data) + offset, be_len);
  offset += sizeof(crypt_data->len);
  // Set data
  memcpy((*data) + offset, crypt_data->data, sizeof(crypt_data->data));
  offset += sizeof(crypt_data->data);
  // SET imit
  memcpy((*data) + offset, crypt_data->imit, sizeof(crypt_data->imit));
  // offset += sizeof(crypt_data->imit);

  return 0;
}
ssize_t deserialize_crypt_data_t(uint8_t* data, crypt_data_t* crypt_data) {
  if (!crypt_data || !data) {
    log_msg(ERRN, 14);
    return -1;
  }

  size_t offset = 0;
  memcpy(&crypt_data->key_info, data + offset, CRYPT_DATA_KEY_SIZE);
  offset += CRYPT_DATA_KEY_SIZE;

  uint16_t be_len = 0;
  memcpy(&be_len, data + offset, sizeof(be_len));
  crypt_data->len = ntohs(be_len);
  offset += sizeof(crypt_data->len);

  if (crypt_data->len > MAX_PORTION_DATA_BYTES) {
    dinit_crypt_data_t(crypt_data);
    return -1;
  }
  memcpy(crypt_data->data, data + offset, crypt_data->len);
  offset += MAX_PORTION_DATA_BYTES;

  memcpy(crypt_data->imit, data + offset, CRYPT_DATA_IMIT_SIZE);
  // offset += CRYPT_DATA_IMIT_SIZE;
  return 0;
}
ssize_t serialize_packet_rxs_t(packet_rxs_t* packet_rxs, uint8_t** data, size_t* data_sz) {
  if (!packet_rxs || !data || !data_sz) {
    log_msg(ERRN, 14);
    return -1;
  }
  size_t sz_hdr = hdr_packet_rxs_t_sz();
  size_t sz_pkt = packet_rxs->sz;
  size_t sz_bdy = (sz_pkt - sz_hdr);
  *data = (uint8_t*)calloc(sz_pkt, sizeof(uint8_t));
  if (!*data) {
    log_msg(ERRN, 6, "calloc", strerror(errno));
    return -1;
  }
  size_t offset = 0;
  // Set data
  serialize_uint8_t(*data + offset, packet_rxs->sep1);
  offset += sizeof(packet_rxs->sep1);
  serialize_uint8_t(*data + offset, packet_rxs->sep2);
  offset += sizeof(packet_rxs->sep2);
  serialize_uint32_t(*data + offset, htonl(packet_rxs->sz));
  offset += sizeof(packet_rxs->sz);
  serialize_uint8_t(*data + offset, packet_rxs->type);
  offset += sizeof(packet_rxs->type);
  serialize_uint32_t(*data + offset, htonl(packet_rxs->uid));
  offset += sizeof(packet_rxs->uid);
  serialize_uint32_t(*data + offset, htonl(packet_rxs->crc32));
  offset += sizeof(packet_rxs->crc32);
  serialize_uint16_t(*data + offset, htons(packet_rxs->operation));
  offset += sizeof(packet_rxs->operation);
  memcpy(*data + offset, packet_rxs->data, sz_bdy);
  offset += sz_bdy;

  *data_sz = offset;

  return 0;
}
ssize_t deserialize_packet_rxs_t(const uint8_t* data, size_t data_sz, packet_rxs_t* packet_rxs) {
  if (!packet_rxs || !data || !data_sz) {
    log_msg(ERRN, 14);
    return -1;
  }

  size_t sz_hdr = hdr_packet_rxs_t_sz();
  if (data_sz < sz_hdr) {
    log_msg(ERRN, 23, data_sz, sz_hdr);
    return -1;
  }
  size_t offset = 0;
  //
  deserialize_uint8_t(data + offset, &packet_rxs->sep1);
  offset += sizeof(packet_rxs->sep1);
  deserialize_uint8_t(data + offset, &packet_rxs->sep2);
  offset += sizeof(packet_rxs->sep2);
  deserialize_uint32_t(data + offset, &packet_rxs->sz);
  offset += sizeof(packet_rxs->sz);
  deserialize_uint8_t(data + offset, &packet_rxs->type);
  offset += sizeof(packet_rxs->type);
  deserialize_uint32_t(data + offset, &packet_rxs->uid);
  offset += sizeof(packet_rxs->uid);
  deserialize_uint32_t(data + offset, &packet_rxs->crc32);
  offset += sizeof(packet_rxs->crc32);
  deserialize_uint16_t(data + offset, &packet_rxs->operation);
  offset += sizeof(packet_rxs->operation);

  packet_rxs->sz = ntohl(packet_rxs->sz);
  packet_rxs->uid = ntohl(packet_rxs->uid);
  packet_rxs->crc32 = ntohl(packet_rxs->crc32);
  packet_rxs->operation = ntohs(packet_rxs->operation);

  packet_rxs->data = (uint8_t*)calloc((packet_rxs->sz - sz_hdr), sizeof(uint8_t));
  if (!packet_rxs->data) {
    log_msg(ERRN, 6, "calloc", strerror(errno));
    return -1;
  }
  memcpy(packet_rxs->data, data + offset, (packet_rxs->sz - sz_hdr));
  // offset += (packet_rxs->sz - sz_hdr);

  return 0;
}
// Compose data slot
ssize_t compose_slot00_t(uint32_t val, slot00_t* slot00) {
  if (!slot00) return -1;

  slot00->val = val;

  return 0;
}
ssize_t compose_slot01_t(const char* data, size_t data_sz, slot01_t* slot01) {
  if (!data || !slot01) return -1;

  slot01->data_sz = data_sz;
  slot01->data = (uint8_t*)calloc(data_sz, sizeof(uint8_t));
  if (!slot01->data) {
    log_msg(ERRN, 6, "calloc", strerror(errno));
    return -1;
  }
  memcpy(slot01->data, data, data_sz);

  return 0;
}
ssize_t compose_slot02_t(const char* data1, size_t data1_sz, const char* data2, size_t data2_sz, uint8_t encoder,
                         slot02_t* slot02) {
  if (!data1 || !data2 || !slot02) return -1;

  slot02->data1_sz = data1_sz;
  slot02->data1 = (uint8_t*)calloc(data1_sz, sizeof(uint8_t));
  if (!slot02->data1) {
    log_msg(ERRN, 6, "calloc", strerror(errno));
    return -1;
  }
  memcpy(slot02->data1, data1, data1_sz);

  slot02->data2_sz = data2_sz;
  slot02->data2 = (uint8_t*)calloc(data2_sz, sizeof(uint8_t));
  if (!slot02->data2) {
    log_msg(ERRN, 6, "calloc", strerror(errno));
    return -1;
  }
  memcpy(slot02->data2, data2, data2_sz);
  slot02->encoder = encoder;

  return 0;
}
// Compose data slot
ssize_t compose_slot03_t(const char* data, size_t data_sz, uint32_t val, slot03_t* slot03) {
  if (!data || !slot03) return -1;

  slot03->data_sz = data_sz;
  slot03->data = (uint8_t*)calloc(data_sz, sizeof(uint8_t));
  if (!slot03->data) {
    log_msg(ERRN, 6, "calloc", strerror(errno));
    return -1;
  }
  memcpy(slot03->data, data, data_sz);
  slot03->val = val;

  return 0;
}
// Compose data slot
ssize_t compose_slot04_t(uint32_t val1, uint32_t data_sz, uint16_t eof, slot04_t* slot04) {
  // CAUTION! Data ptr 'data' maybe NULL
  if (!slot04) return -1;
  slot04->val1 = val1;
  slot04->data_sz = data_sz;
  slot04->eof = eof;
  return 0;
}
// Compose data slot
ssize_t compose_slot05_t(uint32_t stream, uint16_t port, slot05_t* slot05) {
  if (!slot05) return -1;
  slot05->stream_id = stream;
  slot05->port = htons(port);
  return 0;
}

// Compose crypt data slot
ssize_t compose_crypt_data_t(const uint8_t* key_info, uint16_t len, const uint8_t* data, const uint8_t* imit,
                             crypt_data_t* crypt_data) {
  if (!crypt_data) return -1;
  if (key_info != NULL) memcpy(crypt_data->key_info, key_info, sizeof(crypt_data->key_info));
  crypt_data->len = len;
  if (data != NULL) memcpy(crypt_data->data, data, sizeof(crypt_data->data));
  if (imit != NULL) memcpy(crypt_data->imit, imit, sizeof(crypt_data->imit));

  return 0;
}
ssize_t find_header_packet_rxs(uint8_t* buffer, uint32_t buffer_size, uint32_t* pos_out, uint32_t* packet_size_out) {
  if (!buffer) return -1;
  if (buffer_size < hdr_packet_rxs_t_sz()) return 0;
  if (!pos_out || !packet_size_out) return -1;

  packet_rxs_t packet_rxs;
  uint32_t offset_sep2 = sizeof(packet_rxs.sep1);
  uint32_t offset_size_packet = offset_sep2 + sizeof(packet_rxs.sep2);
  uint32_t offset_type = offset_size_packet + sizeof(packet_rxs.sz);
  uint32_t offset_uid = offset_type + sizeof(packet_rxs.type);
  uint32_t offset_crc32 = offset_uid + sizeof(packet_rxs.uid);
  uint32_t offset_operation = offset_crc32 + sizeof(packet_rxs.crc32);
  uint32_t offset_data = offset_operation + sizeof(packet_rxs.operation);

  uint32_t i = 0;
  for (i = 0; i < buffer_size; i++) {
    // Extract values
    uint32_t packet_size = 0;
    deserialize_uint32_t(&(buffer[i + offset_size_packet]), &packet_size);
    packet_size = ntohl(packet_size);
    uint16_t packet_operation = 0;
    deserialize_uint16_t(&(buffer[i + offset_operation]), &packet_operation);
    packet_operation = ntohs(packet_operation);
    uint8_t packet_type = buffer[i + offset_type];
    uint32_t packet_uid = 0;
    deserialize_uint32_t(&(buffer[i + offset_uid]), &packet_uid);
    packet_uid = ntohl(packet_uid);
    if ((buffer[i] == RXS_SEPARATOR) && (buffer[i + offset_sep2] == RXS_SEPARATOR) &&
        // SIZE
        (packet_size >= hdr_packet_rxs_t_sz()) &&
        // TYPE
        ((packet_type == CS_A0) || (packet_type == SC_B0) || (packet_type == SC_B1)) &&
        // OPERATION
        ((packet_operation != operation_undef) && (packet_operation < operation_max))) {
      // Header is found
      *pos_out = i;
      *packet_size_out = packet_size;
      // Packet is found
      if ((intptr_t)(buffer_size - i) >= (intptr_t)packet_size) {
        uint32_t crc32_calc = calc_crc32(0, &(buffer[i + offset_data]), packet_size - hdr_packet_rxs_t_sz());
        uint32_t crc32_spec = 0;
        deserialize_uint32_t(&(buffer[i + offset_crc32]), &crc32_spec);
        crc32_spec = ntohl(crc32_spec);
        if (crc32_spec != crc32_calc) {
          log_msg(ERRN, 22, crc32_spec, crc32_calc, i);
          // CRC32 is incorrect
          return -2;
        }
        // Packet is found
        return 2;
      }
      // Header is found
      return 1;
    }
  }
  // Header is not found
  return 0;
}
// Compose packet RXS
ssize_t compose_packet_rxs(rxs_type_t type, rxs_operation_t operation, uint8_t* data, size_t data_sz,
                           packet_rxs_t* packet_rxs) {
  if (!data || !packet_rxs) return -1;
  // Init packet
  init_packet_rxs_t(packet_rxs);
  packet_rxs->sep1 = RXS_SEPARATOR;
  packet_rxs->sep2 = RXS_SEPARATOR;
  packet_rxs->sz = hdr_packet_rxs_t_sz() + data_sz;
  packet_rxs->type = type;
  packet_rxs->uid = create_uid_pkt();
  packet_rxs->crc32 = calc_crc32(0, data, data_sz);
  packet_rxs->operation = operation;
  packet_rxs->data = data;

  return 0;
}

ssize_t compose_packet_rxs_0x(rxs_type_t type, rxs_operation_t operation,
                              ssize_t (*serialize_slot0x_t)(void* slot, uint8_t** data, size_t* data_sz), void* slot0x,
                              packet_rxs_t* packet_rxs) {
  if (!serialize_slot0x_t || !slot0x || !packet_rxs) return -1;
  // serialize
  uint8_t* data_slot = NULL;
  size_t data_slot_sz = 0;
  if (serialize_slot0x_t(slot0x, &data_slot, &data_slot_sz) < 0) return -1;
  // create packet
  if (compose_packet_rxs(type, operation, data_slot, data_slot_sz, packet_rxs) < 0) return -1;

  return 0;
}
ssize_t compose_packet_rxs_x00(rxs_type_t type, rxs_operation_t operation, uint32_t val, packet_rxs_t* packet_rxs) {
  if (!packet_rxs) return -1;
  slot00_t slot00;
  init_slot00_t(&slot00);
  if (compose_slot00_t(val, &slot00) < 0) {
    // Free memory
    dinit_slot00_t(&slot00);
    return -1;
  }
  if (compose_packet_rxs_0x(type, operation, serialize_slot00_t, &slot00, packet_rxs) < 0) {
    // Free memory
    dinit_slot00_t(&slot00);
    return -1;
  }
  // Free memory
  dinit_slot00_t(&slot00);

  return 0;
}
ssize_t compose_packet_rxs_x01(rxs_type_t type, rxs_operation_t operation, const char* data, size_t data_sz,
                               packet_rxs_t* packet_rxs) {
  if (!packet_rxs) return -1;

  slot01_t slot01;
  init_slot01_t(&slot01);
  if (compose_slot01_t(data, data_sz, &slot01) < 0) {
    // Free memory
    dinit_slot01_t(&slot01);
    return -1;
  }
  if (compose_packet_rxs_0x(type, operation, serialize_slot01_t, &slot01, packet_rxs) < 0) {
    // Free memory
    dinit_slot01_t(&slot01);
    return -1;
  }
  // Free memory
  dinit_slot01_t(&slot01);
  return 0;
}
ssize_t compose_packet_rxs_x02(rxs_type_t type, rxs_operation_t operation, const char* data1, size_t data1_sz,
                               const char* data2, size_t data2_sz, uint8_t encoder, packet_rxs_t* packet_rxs) {
  if (!packet_rxs) return -1;

  slot02_t slot02;
  init_slot02_t(&slot02);
  if (compose_slot02_t(data1, data1_sz, data2, data2_sz, encoder, &slot02) < 0) {
    // Free memory
    dinit_slot02_t(&slot02);
    return -1;
  }
  if (compose_packet_rxs_0x(type, operation, serialize_slot02_t, &slot02, packet_rxs) < 0) {
    // Free memory
    dinit_slot02_t(&slot02);
    return -1;
  }
  // Free memory
  dinit_slot02_t(&slot02);
  return 0;
}
ssize_t compose_packet_rxs_x03(rxs_type_t type, rxs_operation_t operation, const char* data, size_t data_sz,
                               uint32_t val, packet_rxs_t* packet_rxs) {
  if (!packet_rxs) return -1;

  slot03_t slot03;
  init_slot03_t(&slot03);
  if (compose_slot03_t(data, data_sz, val, &slot03) < 0) {
    // Free memory
    dinit_slot03_t(&slot03);
    return -1;
  }
  if (compose_packet_rxs_0x(type, operation, serialize_slot03_t, &slot03, packet_rxs) < 0) {
    // Free memory
    dinit_slot03_t(&slot03);
    return -1;
  }
  // Free memory
  dinit_slot03_t(&slot03);
  return 0;
}
ssize_t compose_packet_rxs_x04(rxs_type_t type, rxs_operation_t operation, uint32_t stream, uint32_t data_sz,
                               uint16_t eof, packet_rxs_t* packet_rxs) {
  if (!packet_rxs) return -1;

  slot04_t slot04;
  init_slot04_t(&slot04);
  if (compose_slot04_t(stream, data_sz, eof, &slot04) < 0) {
    // Free memory
    dinit_slot04_t(&slot04);
    return -1;
  }
  if (compose_packet_rxs_0x(type, operation, serialize_slot04_t, &slot04, packet_rxs) < 0) {
    // Free memory
    dinit_slot04_t(&slot04);
    return -1;
  }
  // Free memory
  dinit_slot04_t(&slot04);
  return 0;
}
ssize_t compose_packet_rxs_x05(rxs_type_t type, rxs_operation_t operation, uint32_t stream, uint16_t port,
                               packet_rxs_t* packet_rxs) {
  if (!packet_rxs) return -1;

  slot05_t slot05;
  init_slot05_t(&slot05);
  if (compose_slot05_t(stream, port, &slot05) < 0) {
    // Free memory
    dinit_slot05_t(&slot05);
    return -1;
  }
  if (compose_packet_rxs_0x(type, operation, serialize_slot05_t, &slot05, packet_rxs) < 0) {
    // Free memory
    dinit_slot05_t(&slot05);
    return -1;
  }
  // Free memory
  dinit_slot05_t(&slot05);
  return 0;
}

//////////////////////////////////////////////////////////////////////////////////
// Functions for send and receive data slot
//////////////////////////////////////////////////////////////////////////////////
ssize_t rqst_x00_resp_x00(int sockfd_conn, rxs_type_t type, rxs_operation_t operation, uint32_t val, void* buf,
                          size_t buf_size, int* errno_other_side) {
  // May have buf == NULL, buf_size == 0,
  //////////////////////////////////////////////////////////////////////////////////
  // RQST
  //////////////////////////////////////////////////////////////////////////////////
  packet_rxs_t packet_rxs_send;
  if (compose_packet_rxs_x00(type, operation, val, &packet_rxs_send) < 0) {
    dinit_packet_rxs_t(&packet_rxs_send);
    return -1;
  }
  // Send packet
  ssize_t impl_send = rxs_send_packet(sockfd_conn, &packet_rxs_send);
  dinit_packet_rxs_t(&packet_rxs_send);
  if (impl_send < 0) return -1;

  //////////////////////////////////////////////////////////////////////////////////
  // RESP: Return data via buffer, not via the slot!
  //////////////////////////////////////////////////////////////////////////////////
  slot00_t slot00;
  init_slot00_t(&slot00);
  ssize_t impl_recv = rxs_recv_slot0x(sockfd_conn, buf, buf_size, &slot00, errno_other_side);
  long res = (long)slot00.val;
  // Free memory
  dinit_slot00_t(&slot00);
  if (impl_recv < 0) return -1;
  return res;
}
ssize_t rqst_x01_resp_x00(int sockfd_conn, rxs_type_t type, rxs_operation_t operation, const char* data, size_t data_sz,
                          void* buf, size_t buf_size, int* errno_other_side) {
  //////////////////////////////////////////////////////////////////////////////////
  // RQST
  //////////////////////////////////////////////////////////////////////////////////
  packet_rxs_t packet_rxs_send;
  if (compose_packet_rxs_x01(type, operation, data, data_sz, &packet_rxs_send) < 0) {
    dinit_packet_rxs_t(&packet_rxs_send);
    return -1;
  }
  // Send packet
  ssize_t impl_send = rxs_send_packet(sockfd_conn, &packet_rxs_send);
  dinit_packet_rxs_t(&packet_rxs_send);
  if (impl_send < 0) return -1;

  //////////////////////////////////////////////////////////////////////////////////
  // RESP
  //////////////////////////////////////////////////////////////////////////////////
  slot00_t slot00;
  init_slot00_t(&slot00);
  ssize_t impl_recv = rxs_recv_slot0x(sockfd_conn, buf, buf_size, &slot00, errno_other_side);
  int res = (int)slot00.val;
  // Free memory
  dinit_slot00_t(&slot00);
  if (impl_recv < 0) {
    return -1;
  }
  return res;
}
ssize_t rqst_x01_resp_x01(int sockfd_conn, rxs_type_t type, rxs_operation_t operation, const char* data, size_t data_sz,
                          void* buf, size_t buf_size, int* errno_other_side) {
  //////////////////////////////////////////////////////////////////////////////////
  // RQST
  //////////////////////////////////////////////////////////////////////////////////
  packet_rxs_t packet_rxs_send;
  if (compose_packet_rxs_x01(type, operation, data, data_sz, &packet_rxs_send) < 0) {
    dinit_packet_rxs_t(&packet_rxs_send);
    return -1;
  }
  // Send packet
  ssize_t impl_send = rxs_send_packet(sockfd_conn, &packet_rxs_send);
  dinit_packet_rxs_t(&packet_rxs_send);
  if (impl_send < 0) return -1;

  //////////////////////////////////////////////////////////////////////////////////
  // RESP: Return data via buffer, not via the slot!
  //////////////////////////////////////////////////////////////////////////////////
  slot00_t slot00;
  init_slot00_t(&slot00);
  ssize_t impl_recv = rxs_recv_slot0x(sockfd_conn, buf, buf_size, &slot00, errno_other_side);
  long res = (long)slot00.val;
  // Free memory
  dinit_slot00_t(&slot00);
  if (impl_recv < 0) {
    return -1;
  }
  return res;
}
ssize_t rqst_x02_resp_x00(int sockfd_conn, rxs_type_t type, rxs_operation_t operation, const char* data1,
                          size_t data1_sz, const char* data2, size_t data2_sz, uint8_t encoder, void* buf,
                          size_t buf_size, int* errno_other_side) {
  //////////////////////////////////////////////////////////////////////////////////
  // RQST
  //////////////////////////////////////////////////////////////////////////////////
  packet_rxs_t packet_rxs_send;
  if (compose_packet_rxs_x02(type, operation, data1, data1_sz, data2, data2_sz, encoder, &packet_rxs_send) < 0) {
    dinit_packet_rxs_t(&packet_rxs_send);
    return -1;
  }
  // Send packet
  ssize_t impl_send = rxs_send_packet(sockfd_conn, &packet_rxs_send);
  dinit_packet_rxs_t(&packet_rxs_send);
  if (impl_send < 0) {
    return -1;
  }
  //////////////////////////////////////////////////////////////////////////////////
  // RESP
  //////////////////////////////////////////////////////////////////////////////////
  slot00_t slot00;
  init_slot00_t(&slot00);
  ssize_t impl_recv = rxs_recv_slot0x(sockfd_conn, buf, buf_size, &slot00, errno_other_side);
  size_t res = (size_t)slot00.val;
  // Free memory
  dinit_slot00_t(&slot00);
  if (impl_recv < 0) return -1;
  return res;
}
ssize_t rqst_x03_resp_x00(int sockfd_conn, rxs_type_t type, rxs_operation_t operation, const char* data, size_t data_sz,
                          uint32_t val, void* buf, size_t buf_size, int* errno_other_side) {
  //////////////////////////////////////////////////////////////////////////////////
  // RQST
  //////////////////////////////////////////////////////////////////////////////////
  packet_rxs_t packet_rxs_send;
  if (compose_packet_rxs_x03(type, operation, data, data_sz, val, &packet_rxs_send) < 0) {
    dinit_packet_rxs_t(&packet_rxs_send);
    return -1;
  }
  // Send packet
  ssize_t impl_send = rxs_send_packet(sockfd_conn, &packet_rxs_send);
  dinit_packet_rxs_t(&packet_rxs_send);
  if (impl_send < 0) return -1;

  //////////////////////////////////////////////////////////////////////////////////
  // RESP
  //////////////////////////////////////////////////////////////////////////////////
  slot00_t slot00;
  init_slot00_t(&slot00);
  ssize_t impl_recv = rxs_recv_slot0x(sockfd_conn, buf, buf_size, &slot00, errno_other_side);
  // Free memory
  dinit_slot00_t(&slot00);
  if (impl_recv < 0) {
    return -1;
  }
  return impl_recv;
}
ssize_t rqst_x05_resp_x00(int sockfd_conn, rxs_type_t type, rxs_operation_t operation, uint32_t stream, uint16_t port,
                          void* buf, size_t buf_size, int* errno_other_side) {
  //////////////////////////////////////////////////////////////////////////////////
  // RQST
  //////////////////////////////////////////////////////////////////////////////////
  packet_rxs_t packet_rxs_send;
  if (compose_packet_rxs_x05(type, operation, stream, port, &packet_rxs_send) < 0) {
    dinit_packet_rxs_t(&packet_rxs_send);
    return -1;
  }
  // Send packet
  ssize_t impl_send = rxs_send_packet(sockfd_conn, &packet_rxs_send);
  dinit_packet_rxs_t(&packet_rxs_send);
  if (impl_send < 0) return -1;

  //////////////////////////////////////////////////////////////////////////////////
  // RESP
  //////////////////////////////////////////////////////////////////////////////////
  slot00_t slot00;
  init_slot00_t(&slot00);
  ssize_t impl_recv = rxs_recv_slot0x(sockfd_conn, buf, buf_size, &slot00, errno_other_side);
  size_t res = (size_t)slot00.val;
  // Free memory
  dinit_slot00_t(&slot00);
  if (impl_recv < 0) {
    return -1;
  }
  return res;
}
size_t portion_sz(size_t total_rqst_sz, size_t total_impl_sz) {
  return (((total_rqst_sz - total_impl_sz) > MAX_PORTION_DATA_BYTES) ? MAX_PORTION_DATA_BYTES
                                                                     : (total_rqst_sz - total_impl_sz));
}
size_t compose_data(uint8_t encoder, const void* buf_src, uint8_t* buf_dst, size_t buf_dst_sz, size_t total_impl_sz,
                    size_t data_sz) {
  if (!buf_src || !buf_dst) return 0;

  uint16_t out_sz = 0;
  memset(buf_dst, 0, buf_dst_sz);

  if (encoder > 0) {
    crypt_data_t crypt_data;
    init_crypt_data_t(&crypt_data);
    memcpy(crypt_data.data, buf_src + total_impl_sz, data_sz);
    crypt_data.len = data_sz;
    if (serialize_crypt_data_t(&crypt_data, &buf_dst) != 0) return 0;
    out_sz = buf_dst_sz;
  } else {
    memcpy(buf_dst, buf_src + total_impl_sz, data_sz);
    out_sz = data_sz;
  }
  return out_sz;
}
size_t decompose_data(uint8_t encoder, void* buf_dst, uint8_t* buf_src, size_t buf_src_sz, size_t total_impl_sz,
                      size_t impl_sz) {
  if (!buf_dst || !buf_src) return 0;

  uint16_t out_sz = 0;
  if (encoder > 0) {
    crypt_data_t crypt_data;
    init_crypt_data_t(&crypt_data);
    if (deserialize_crypt_data_t(buf_src, &crypt_data) < 0) return 0;
    memcpy(buf_dst + total_impl_sz, crypt_data.data, crypt_data.len);
    out_sz = crypt_data.len;
  } else {
    memcpy(buf_dst + total_impl_sz, buf_src, impl_sz);
    out_sz = impl_sz;
  }
  memset(buf_src, 0, buf_src_sz);
  return out_sz;
}
// Write regular file to encrypted format
ssize_t write_encrypted_file(char* path_regular_file, char* path_encrypted_file, size_t data_encrypted_sz) {
  if (!path_regular_file || !path_encrypted_file) return -1;

  // Read text file
  char* buf = NULL;
  long total_rqst_sz = 0;
  if (read_text_file(path_regular_file, &buf, &total_rqst_sz) < 0) {
    return -1;
  }
  size_t total_impl_sz = 0;
  char* buf_send = calloc(data_encrypted_sz, sizeof(uint8_t));
  if (!buf_send) {
    free(buf);
    buf = NULL;
    return -1;
  }
  unlink(path_encrypted_file);
  while (total_impl_sz < total_rqst_sz) {
    size_t portion = portion_sz(total_rqst_sz, total_impl_sz);
    uint16_t buf_send_sz = compose_data(1, (void*)buf, (uint8_t*)buf_send, data_encrypted_sz, total_impl_sz, portion);
    if (write_file(path_encrypted_file, buf_send, buf_send_sz, "ab") < 0) {
      free(buf);
      buf = NULL;
      free(buf_send);
      buf_send = NULL;
      return -1;
    }
    total_impl_sz += portion;
  }
  free(buf);
  buf = NULL;
  free(buf_send);
  buf_send = NULL;
  return 0;
}

//////////////////////////////////////////////////////////////////////////////////
// Socket settings
//////////////////////////////////////////////////////////////////////////////////
// Set socket option
ssize_t set_socket_mode(int sockfd, int encoder) {
  // Set TCP_NODELAY
  if (setsockopt_x(sockfd, SOL_TCP, TCP_NODELAY, 1) < 0) {
    log_msg(ERRN, 59, "TCP_NODELAY", strerror(errno));
    return -1;
  }

  if (encoder) {
    // CAUTION: TCP_MAXSEG -- The maximum segment size for outgoing TCP packets.
    // In Linux 2.2 and earlier, and in Linux 2.6.28 and later, if this option is set before connection establishment,
    // it also changes the MSS value announced to the other end in the initial packet. NOTE: Values greater than the
    // (eventual) interface MTU have no effect. TCP will also impose its minimum and maximum bounds over the value
    // provided. Set TCP_MAXSEG
    if (setsockopt_x(sockfd, SOL_TCP, TCP_MAXSEG, TCP_SEGMENT_SIZE) < 0) {
      log_msg(ERRN, 59, "TCP_MAXSEG", strerror(errno));
      return -1;
    }

    //////////////////////////////////////////////////////////////////////////////////////
    //
    //////////////////////////////////////////////////////////////////////////////////////
    /*
    {
        int seg_size = 0;
        if( getsockopt_x(sockfd, SOL_TCP, TCP_MAXSEG, &seg_size) < 0)
        {
            log_msg(ERRN, 59, "TCP_MAXSEG", strerror(errno));
            return -1;
        }
        printf("DBG: TCP_MAXSEG:%d \n", seg_size);
    }
    */
    /*
    {
        int max_buf_snd = 0;
        socklen_t socklen = sizeof(max_buf_snd);
        if(getsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, &max_buf_snd, &socklen) < 0)
        {
            log_msg(ERRN, 59, "SO_SNDBUF", strerror(errno));
            return -1;
        }
        printf("DBG: default buf_snd:%d \n", max_buf_snd);
    }
    {
        int max_buf_rcv = 0;
        socklen_t socklen = sizeof(max_buf_rcv);
        if(getsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &max_buf_rcv, &socklen) < 0)
        {
            log_msg(ERRN, 59, "SO_RCVBUF", strerror(errno));
            return -1;
        }
        printf("DBG: default buf_rcv:%d \n", max_buf_rcv);
    }
    */
    /*
    // Set SO_SNDBUF, SO_RCVBUF
    int max_buf_snd = seg_size;
    if(setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, &max_buf_snd, sizeof(max_buf_snd)) < 0)
    {
        log_msg(ERRN, 59, "SO_SNDBUF", strerror(errno));
        return -1;
    }
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // CAUTION: The minimum multiplier value (4) is due to how the TCP fast recovery algorithm works.
    // Sender uses three double acknowledgments to find the lost packet (RFC 2581 [4]).
    // The receiver sends a double acknowledgment for every segment received after the one that was skipped.
    // If the window size is less than four segments, there will be no three double acknowledgments and the fast
    recovery algorithm will not work.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    int max_buf_rcv = 4*seg_size;
    if(setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &max_buf_rcv, sizeof(max_buf_rcv)) < 0)
    {
        log_msg(ERRN, 59, "SO_RCVBUF", strerror(errno));
        return -1;
    }
    */
  }
  return 0;
}
// Set sock option
int setsockopt_x(int sockfd, int level, int optname, int optval) {
  int level_sol = level;
  if (SOL_TCP == level) {
#ifndef __QNXNTO__
    level_sol = SOL_TCP;
#else
    // QNX
    struct protoent* prt = getprotobyname("tcp");
    if (!prt) return -1;
    level_sol = prt->p_proto;
#endif
  }

  int param = optval;
  if (setsockopt(sockfd, level_sol, optname, &param, sizeof(param)) < 0) return -1;
  return 0;
}
// Get sock option
int getsockopt_x(int sockfd, int level, int optname, int* optval) {
  int level_sol = level;
  if (SOL_TCP == level) {
#ifndef __QNXNTO__
    level_sol = SOL_TCP;
#else
    // QNX
    struct protoent* prt = getprotobyname("tcp");
    if (!prt) return -1;
    level_sol = prt->p_proto;
#endif
  }
  socklen_t socklen = sizeof(*optval);
  if (getsockopt(sockfd, level_sol, optname, optval, &socklen) < 0) return -1;
  return 0;
}
