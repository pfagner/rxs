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
#ifndef _RXS_PROTOCOL_H
#define _RXS_PROTOCOL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>    // for 'uint32_t'
#include <sys/stat.h>  // for 'mode_t'
#include <unistd.h>

#define MAX_PORTION_DATA_BYTES 982  // Size of data payload is equal about MTU size
#define CRYPT_DATA_KEY_SIZE 8
#define CRYPT_DATA_LEN_SIZE 2
#define CRYPT_DATA_IMIT_SIZE 8

typedef int RXS_HANDLE;       // Type handler
typedef int RXS_DATA_HANDLE;  // Type data handler
extern const uint16_t TCP_SEGMENT_SIZE;
extern const uint16_t TIME_INTERVAL_SEND_DATA_MSEC;  // Time interval for separating packets each from other
extern const uint16_t TCP_COMMAND_SEGMENT_SIZE;
extern const uint16_t TCP_FLAG_SIZE;
extern const uint32_t POLL_TIMEOUT_DATA_MSEC;
extern const uint32_t POLL_TIMEOUT_CONNECT_MSEC;
extern const uint32_t RXS_DATA_PORT;
extern const char RXS_SEPARATOR;  // Packet's RXS separator
extern const uint16_t RXS_EOF;
//////////////////////////////////////////////////////////////////////////////////////////////////
// RXS Remote eXchange System: Allows editing files and perform commands on remote file systems
//////////////////////////////////////////////////////////////////////////////////////////////////
// type request/response
typedef enum rxs_type_t {
  TYPE_UNDEF = 0,
  CS_A0,  // request
  SC_B0,  // response: OK
  SC_B1   // response: FAIL
} rxs_type_t;
// commands type for file operations
typedef enum rxs_operation_t {
  operation_undef = 0,
  operation_fopen = 1,
  operation_fread = 2,
  operation_fwrite = 3,
  operation_fflush = 4,
  operation_fclose = 5,
  operation_fseek = 6,
  operation_ftell = 7,
  operation_rewind = 8,
  operation_point_create = 9,
  operation_point_close = 10,
  operation_authorization = 11,
  operation_ls = 12,
  operation_mkdir = 13,
  operation_mkdir_ex = 14,
  operation_rmdir = 15,
  operation_getcwd = 16,
  operation_chdir = 17,
  operation_unlink = 18,
  operation_rename = 19,
  operation_filesize = 20,
  operation_file_exist = 21,
  operation_dir_exist = 22,
  operation_port = 23,
  operation_max = 24,
} rxs_operation_t;
//////////////////////////////////////////////////////////////////////////////////////////////////
// Packet RXS
//////////////////////////////////////////////////////////////////////////////////////////////////
// -------------------------------------------------------------------------------
// | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10| 11| 12| 13| 14| 15| 16| 17| 18| ... |
// -------------------------------------------------------------------------------
// | A | A | B | B | B | B | C | D | D | D | D | E | E | E | E | F | F | G | G.. |
// -------------------------------------------------------------------------------
// A - 2 - separator
// B - 4 - packet size
// C - 1 - type of interaction (Request: CS_A0; Respond: SC_B0, SC_B1)
// D - 4 - UID packet (In request: unique; In response: as request)
// E - 4 - CRC32 of packet
// F - 2 - type RXS operation
// G - * - data (packeted in slot0x_t description see below)

typedef struct packet_rxs_t {
  uint8_t sep1;        // A
  uint8_t sep2;        // A
  uint32_t sz;         // B
  uint8_t type;        // C
  uint32_t uid;        // D
  uint32_t crc32;      // E
  uint16_t operation;  // F
  uint8_t* data;       // G
} packet_rxs_t;

ssize_t init_packet_rxs_t(packet_rxs_t* packet_rxs);
ssize_t dinit_packet_rxs_t(packet_rxs_t* packet_rxs);
// header size
size_t hdr_packet_rxs_t_sz();
// slot size slot04_t
size_t hdr_slot04_t_sz();
// Counter UID packet
uint32_t create_uid_pkt();
// data packet size
uint16_t crypt_data_sz();
uint16_t crypt_packet_sz();
//////////////////////////////////////////////////////////////////////////////////////////////////
// Slots
//////////////////////////////////////////////////////////////////////////////////////////////////
typedef struct slot00_t {
  uint32_t val;
} slot00_t;

ssize_t init_slot00_t(slot00_t* slot00);
ssize_t dinit_slot00_t(slot00_t* slot00);

typedef struct slot01_t {
  uint32_t data_sz;
  uint8_t* data;
} slot01_t;

ssize_t init_slot01_t(slot01_t* slot01);
ssize_t dinit_slot01_t(slot01_t* slot01);

typedef struct slot02_t {
  uint32_t data1_sz;
  uint8_t* data1;
  uint32_t data2_sz;
  uint8_t* data2;
  uint8_t encoder;
} slot02_t;

ssize_t init_slot02_t(slot02_t* slot02);
ssize_t dinit_slot02_t(slot02_t* slot02);

typedef struct slot03_t {
  uint32_t data_sz;
  uint8_t* data;
  uint32_t val;
} slot03_t;

ssize_t init_slot03_t(slot03_t* slot03);
ssize_t dinit_slot03_t(slot03_t* slot03);

typedef struct slot04_t {
  uint32_t val1;  // stream_id
  uint32_t data_sz;
  uint16_t eof;
} slot04_t;

ssize_t init_slot04_t(slot04_t* slot04);
ssize_t dinit_slot04_t(slot04_t* slot04);

typedef struct slot05_t {
  uint32_t stream_id;  // stream_id
  uint16_t port;       // Port number
} slot05_t;

ssize_t init_slot05_t(slot05_t* slot05);
ssize_t dinit_slot05_t(slot05_t* slot05);

typedef struct crypt_data_t {
  uint8_t key_info[CRYPT_DATA_KEY_SIZE];
  uint16_t len;
  uint8_t data[MAX_PORTION_DATA_BYTES];
  uint8_t imit[CRYPT_DATA_IMIT_SIZE];
} crypt_data_t;

ssize_t init_crypt_data_t(crypt_data_t* crypt_data);
ssize_t dinit_crypt_data_t(crypt_data_t* crypt_data);

typedef struct rcv_slot0x_t {
  int sockfd;
  void* buf;
  size_t size;
  void* slot0x;
  int* errno_other_side;
  ssize_t impl_bytes;
} rcv_slot0x_t;

//////////////////////////////////////////////////////////////////////////////////////////////////
// request/response interaction format | RQST/RESP
//////////////////////////////////////////////////////////////////////////////////////////////////
// FCNT: ls(const char *path, void *buf, size_t buf_sz);
// RQST: path_sz, path_data, buf_sz | use: slot03_t
// RESP:
// RESP B1: errno | use: slot00_t

// FCNT: mkdir(const char *path, mode_t mode);
// RQST: path_sz, path_data, mode | use: slot03_t
// RESP B0: none
// RESP B1: errno | use: slot00_t

// FCNT: rmdir(const char *path);
// RQST: path_sz, path_data | use: slot01_t
// RESP B0: none
// RESP B1: errno | use: slot00_t

// FCNT: getcwd(char *buf, size_t size);
// RQST: buf_sz | use: slot00_t
// RESP B0: size, data | use: slot01_t
// RESP B1: errno | use: slot00_t

// FCNT: chdir(const char *path);
// RQST: path_sz, path_data | use: slot01_t
// RESP B0: none
// RESP B1: errno | use: slot00_t

// FCNT: unlink(const char *path);
// RQST: path_sz, path_data | use: slot01_t
// RESP B0: none
// RESP B1: errno | use: slot00_t

// FCNT: filesize(const char *path);
// RQST: path_sz, path_data | use: slot01_t
// RESP B0: none | use: slot00_t
// RESP B1: errno | use: slot00_t

// FCNT: rename(const char *old, const char *new);
// RQST: old_sz, old_data, new_sz, new_data | use: slot02_t
// RESP B0: none
// RESP B1: errno | use: slot00_t

// FCNT: fopen(const char *fname, const char *mode);
// RQST: fname_sz, fname_data, mode_sz, mode_data | use: slot02_t
// RESP B0: stream_id | use: slot00_t
// RESP B1: errno | use: slot00_t

// FCNT: fread(void *buf, size_t size, size_t count, RXS_HANDLE stream);
// RQST: stream_id, size, count | use: slot04_t
// RESP B0 #N: stream_id, data_total_sz, data_portion_sz, data | use: slot04_t
// RESP B1: errno | use: slot00_t

// FCNT: fwrite(const void *buf, size_t size, size_t count, RXS_HANDLE stream);
// RQST #N: stream_id, data_total_sz, data_portion_sz, data | use: slot04_t
// RESP B0: count bytes | use: slot00_t
// RESP B1: errno | use: slot00_t

// FCNT: fflush(RXS_HANDLE stream);
// RQST: stream_id | use: slot00_t
// RESP B0: none
// RESP B1: errno | use: slot00_t

// FCNT: fclose(RXS_HANDLE stream);
// RQST: stream_id | use: slot00_t
// RESP B0: none
// RESP B1: errno | use: slot00_t

// FCNT: fseek(RXS_HANDLE stream, long offset, int whence)
// RQST: stream_id | use: slot04_t
// RESP B0: none
// RESP B1: errno | use: slot00_t

// FCNT: ftell(RXS_HANDLE stream)
// RQST: stream_id | use: slot00_t
// RESP B0: none
// RESP B1: errno | use: slot00_t

// FCNT: rewind(RXS_HANDLE stream)
// RQST: stream_id | use: slot00_t
// RESP B0: none
// RESP B1: errno | use: slot00_t

// FCNT: port(RXS_HANDLE stream)
// RQST: stream_id | use: slot00_t
// RESP B0: port_number | use: slot05_t
// RESP B1: errno | use: slot00_t

//////////////////////////////////////////////////////////////////////////////////
// exchange data functions
//////////////////////////////////////////////////////////////////////////////////
ssize_t rxs_send_x(int sockfd, void* buf, size_t buf_sz);
ssize_t rxs_recv_x(int sockfd, void* buf, size_t buf_sz);
ssize_t rxs_recv_block_x(int sockfd, void* buf, size_t buf_sz, size_t block_sz);
ssize_t rxs_send_packet(int sockfd, packet_rxs_t* packet_rxs);
ssize_t rxs_send_packet_x04(int sockfd, rxs_type_t type, rxs_operation_t operation, uint32_t stream, uint32_t data_sz,
                            uint16_t eof);
ssize_t rxs_recv_packet_x04(int sockfd, rxs_type_t* type, rxs_operation_t operation, uint32_t* stream,
                            uint32_t* data_sz, uint16_t* eof);
ssize_t rxs_recv_slot0x(int sockfd, void* buf, size_t buf_sz, void* slot0x, int* errno_other_side);
ssize_t rxs_recv_data_x(int sockfd, uint8_t* data, uint32_t data_sz);
//////////////////////////////////////////////////////////////////////////////////
// Serialization/Deserialization
//////////////////////////////////////////////////////////////////////////////////
// Serialization data
ssize_t serialize_slot00_t(void* slot0x, uint8_t** data, size_t* data_sz);
ssize_t deserialize_slot00_t(uint8_t* data, size_t data_sz, slot00_t* slot00);
ssize_t serialize_slot01_t(void* slot01, uint8_t** data, size_t* data_sz);
ssize_t deserialize_slot01_t(uint8_t* data, size_t data_sz, slot01_t* slot01);
ssize_t serialize_slot02_t(void* slot02, uint8_t** data, size_t* data_sz);
ssize_t deserialize_slot02_t(uint8_t* data, size_t data_sz, slot02_t* slot02);
ssize_t serialize_slot03_t(void* slot03, uint8_t** data, size_t* data_sz);
ssize_t deserialize_slot03_t(uint8_t* data, size_t data_sz, slot03_t* slot03);
ssize_t serialize_slot04_t(void* slot04, uint8_t** data, size_t* data_sz);
ssize_t deserialize_slot04_t(uint8_t* data, size_t data_sz, slot04_t* slot04);
ssize_t serialize_slot05_t(void* slot0x, uint8_t** data, size_t* data_sz);
ssize_t deserialize_slot05_t(uint8_t* data, size_t data_sz, slot05_t* slot05);
// Serialization encrypted header
ssize_t serialize_crypt_data_t(void* crypt_data_x, uint8_t** data);
ssize_t deserialize_crypt_data_t(uint8_t* data, crypt_data_t* crypt_data);

// Serialization RXS packet
ssize_t serialize_packet_rxs_t(packet_rxs_t* packet_rxs, uint8_t** data, size_t* data_sz);
ssize_t deserialize_packet_rxs_t(const uint8_t* data, size_t data_sz, packet_rxs_t* packet_rxs);
// Create data slot
ssize_t compose_slot00_t(uint32_t val, slot00_t* slot00);
ssize_t compose_slot01_t(const char* data, size_t data_sz, slot01_t* slot01);
ssize_t compose_slot02_t(const char* data1, size_t data1_sz, const char* data2, size_t data2_sz, uint8_t encoder,
                         slot02_t* slot02);
ssize_t compose_slot03_t(const char* data, size_t data_sz, uint32_t val, slot03_t* slot03);
ssize_t compose_slot04_t(uint32_t stream, uint32_t data_sz, uint16_t eof, slot04_t* slot04);
ssize_t compose_slot05_t(uint32_t stream, uint16_t port, slot05_t* slot05);
// Create crypted data
ssize_t compose_crypt_data_t(const uint8_t* key_info, uint16_t len, const uint8_t* data, const uint8_t* imit,
                             crypt_data_t* crypt_data);
//////////////////////////////////////////////////////////////////////////////////
// Find packet
//////////////////////////////////////////////////////////////////////////////////
ssize_t find_header_packet_rxs(uint8_t* buffer, uint32_t buffer_size, uint32_t* pos_out, uint32_t* packet_size_out);
// Create RXS packet
ssize_t compose_packet_rxs(rxs_type_t type, rxs_operation_t operation, uint8_t* data, size_t data_sz,
                           packet_rxs_t* packet_rxs);

ssize_t compose_packet_rxs_0x(rxs_type_t type, rxs_operation_t operation,
                              ssize_t (*serialize_slot0x_t)(void* slot, uint8_t** data, size_t* data_sz), void* slot,
                              packet_rxs_t* packet_rxs);
ssize_t compose_packet_rxs_x00(rxs_type_t type, rxs_operation_t operation, uint32_t val, packet_rxs_t* packet_rxs);
ssize_t compose_packet_rxs_x01(rxs_type_t type, rxs_operation_t operation, const char* data, size_t data_sz,
                               packet_rxs_t* packet_rxs);
ssize_t compose_packet_rxs_x02(rxs_type_t type, rxs_operation_t operation, const char* data1, size_t data1_sz,
                               const char* data2, size_t data2_sz, uint8_t encoder, packet_rxs_t* packet_rxs);
ssize_t compose_packet_rxs_x03(rxs_type_t type, rxs_operation_t operation, const char* data, size_t data_sz,
                               uint32_t val, packet_rxs_t* packet_rxs);
ssize_t compose_packet_rxs_x04(rxs_type_t type, rxs_operation_t operation, uint32_t stream, uint32_t data_sz,
                               uint16_t eof, packet_rxs_t* packet_rxs);
ssize_t compose_packet_rxs_x05(rxs_type_t type, rxs_operation_t operation, uint32_t stream, uint16_t port,
                               packet_rxs_t* packet_rxs);
//////////////////////////////////////////////////////////////////////////////////
// send/receive data slot functions
//////////////////////////////////////////////////////////////////////////////////
ssize_t rqst_x00_resp_x00(int sockfd_conn, rxs_type_t type, rxs_operation_t operation, uint32_t val, void* buf,
                          size_t buf_size, int* errno_other_side);
ssize_t rqst_x01_resp_x00(int sockfd_conn, rxs_type_t type, rxs_operation_t operation, const char* data, size_t data_sz,
                          void* buf, size_t buf_size, int* errno_other_side);
ssize_t rqst_x01_resp_x01(int sockfd_conn, rxs_type_t type, rxs_operation_t operation, const char* data, size_t data_sz,
                          void* buf, size_t buf_size, int* errno_other_side);
ssize_t rqst_x02_resp_x00(int sockfd_conn, rxs_type_t type, rxs_operation_t operation, const char* data1,
                          size_t data1_sz, const char* data2, size_t data2_sz, uint8_t encoder, void* buf,
                          size_t buf_size, int* errno_other_side);
ssize_t rqst_x03_resp_x00(int sockfd_conn, rxs_type_t type, rxs_operation_t operation, const char* data, size_t data_sz,
                          uint32_t val, void* buf, size_t buf_size, int* errno_other_side);
ssize_t rqst_x05_resp_x00(int sockfd_conn, rxs_type_t type, rxs_operation_t operation, uint32_t stream, uint16_t port,
                          void* buf, size_t buf_size, int* errno_other_side);

//////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////
size_t portion_sz(size_t total_rqst_bytes, size_t total_impl_bytes);
size_t compose_data(uint8_t encoder, const void* buf_src, uint8_t* buf_dst, size_t buf_dst_sz, size_t total_impl_bytes,
                    size_t portion);
size_t decompose_data(uint8_t encoder, void* buf_dst, uint8_t* buf_src, size_t buf_src_sz, size_t total_impl_bytes,
                      size_t impl_bytes);
// Write regular file to encrypted format
ssize_t write_encrypted_file(char* path_regular_file, char* path_encrypted_file, size_t data_encrypted_sz);
//////////////////////////////////////////////////////////////////////////////////
// Socket settings
//////////////////////////////////////////////////////////////////////////////////
// Set socket option mode
ssize_t set_socket_mode(int sockfd, int encoder);
// Set sock option
int setsockopt_x(int sockfd, int level, int optname, int optval);
// Get sock option
int getsockopt_x(int sockfd, int level, int optname, int* optval);

#ifdef __cplusplus
}
#endif

#endif  // _RXS_PROTOCOL_H
