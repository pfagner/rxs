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

#ifndef GENERIC_H
#define GENERIC_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdarg.h>  // for vararg
#include <stdint.h>
#include <stdlib.h>

#include "container/list.h"

/////////////////////////////////////////////////////////////////////////////////////
// htond
/////////////////////////////////////////////////////////////////////////////////////
double htond(double val);
double ntohd(double val);
/////////////////////////////////////////////////////////////////////////////////////
// htonll
/////////////////////////////////////////////////////////////////////////////////////
uint64_t htonll(uint64_t val);
uint64_t ntohll(uint64_t val);
// convert Internet host address to an Internet dot address
char* inet_ntoa_x(uint32_t val);

// Clear iface from exceeded symbol ':'
ssize_t iface_clean(const char* iface, size_t iface_sz, char* dev, size_t dev_sz);
/////////////////////////////////////////////////////////////////////////////////////
// CRC
/////////////////////////////////////////////////////////////////////////////////////
uint16_t calc_crc16(uint16_t crc16, const unsigned char* data, size_t data_len);
uint32_t calc_crc32(uint32_t crc32, const unsigned char* data, size_t data_len);

//////////////////////////////////////////////////////////////////////////////////
// Serialize/Deserialize
//////////////////////////////////////////////////////////////////////////////////
uint8_t* serialize_uint8_t(uint8_t* data, uint8_t value);
const uint8_t* deserialize_uint8_t(const uint8_t* data, uint8_t* value);
uint8_t* serialize_uint16_t(uint8_t* data, uint16_t value);
const uint8_t* deserialize_uint16_t(const uint8_t* data, uint16_t* value);
uint8_t* serialize_int32_t(uint8_t* data, int32_t value);
const uint8_t* deserialize_int32_t(const uint8_t* data, int32_t* value);
uint8_t* serialize_uint32_t(uint8_t* data, uint32_t value);
const uint8_t* deserialize_uint32_t(const uint8_t* data, uint32_t* value);
uint8_t* serialize_uint64_t(uint8_t* data, uint64_t value);
const uint8_t* deserialize_uint64_t(const uint8_t* data, uint64_t* value);
uint8_t* serialize_double_t(uint8_t* data, double value);
const uint8_t* deserialize_double_t(const uint8_t* data, double* value);

uint8_t* serialize_uint8_t_x(uint8_t* data, size_t* data_sz, size_t number_elements, ...);
uint8_t* serialize_uint16_t_x(uint8_t* data, size_t* data_sz, size_t number_elements, ...);
uint8_t* serialize_uint32_t_x(uint8_t* data, size_t* data_sz, size_t number_elements, ...);
uint8_t* serialize_uint64_t_x(uint8_t* data, size_t* data_sz, size_t number_elements, ...);
const uint8_t* deserialize_uint8_t_x(const uint8_t* data, size_t* data_sz, size_t number_elements, ...);
const uint8_t* deserialize_uint16_t_x(const uint8_t* data, size_t* data_sz, size_t number_elements, ...);
const uint8_t* deserialize_uint16_t_h_x(const uint8_t* data, size_t* data_sz, size_t number_elements, ...);
const uint8_t* deserialize_uint32_t_x(const uint8_t* data, size_t* data_sz, size_t number_elements, ...);
const uint8_t* deserialize_uint32_t_h_x(const uint8_t* data, size_t* data_sz, size_t number_elements, ...);
const uint8_t* deserialize_uint64_t_x(const uint8_t* data, size_t* data_sz, size_t number_elements, ...);
const uint8_t* deserialize_uint64_t_h_x(const uint8_t* data, size_t* data_sz, size_t number_elements, ...);
const uint8_t* deserialize_double_t_x(const uint8_t* data, size_t* data_sz, size_t number_elements, ...);
const uint8_t* deserialize_double_t_h_x(const uint8_t* data, size_t* data_sz, size_t number_elements, ...);
const uint8_t* deserialize_array_x(const uint8_t* data, size_t* data_sz, size_t number_elements, ...);
// Create new object with serialize data
uint8_t* new_serialize_uint8_t_x(uint8_t** data, size_t* data_sz, size_t number_elements, ...);
uint8_t* new_serialize_uint16_t_x(uint8_t** data, size_t* data_sz, size_t number_elements, ...);
uint8_t* new_serialize_uint32_t_x(uint8_t** data, size_t* data_sz, size_t number_elements, ...);
uint8_t* new_serialize_uint64_t_x(uint8_t** data, size_t* data_sz, size_t number_elements, ...);
uint8_t* new_serialize_double_t_x(uint8_t** data, size_t* data_sz, size_t number_elements, ...);
uint8_t* new_serialize_array_x(uint8_t** data, size_t* data_sz, size_t number_elements, ...);

int sleep_x(time_t secs, long nanosecs);
int memcmp_x(void* val1, void* val2, size_t val2_sz);
int memcpy_x(void* dst, size_t dst_sz, const void* src, size_t src_sz);
// Copy file
int copy_file(const char* src, const char* dst);
// Copy file
ssize_t copy_file_x(const char* src_path, const char* src_file, const char* dst_path, const char* dst_file);
// File size
long file_size(const char* filename);
// Read text file
int read_text_file(const char* filename, char** buf, long* file_sz);
// Write file
int write_file(const char* filename, const char* buf, size_t buf_sz, const char* mode);
// Remove file
int remove_file(const char* dir, const char* file);
// Truncate file
int truncate_file(const char* dir, const char* file);
// Is dir
int is_dir(uint8_t* data, int* status, uint32_t* err_no);
// Make dir with nested dirs. Like '$ mkdir -p'
int mkdir_ex(const char* data, uint32_t mode, int* status, uint32_t* err_no);
//
ssize_t wait_while_equal(volatile uint8_t* val1, volatile uint8_t* val2, uint32_t secs);
ssize_t wait_while_greater(volatile uint8_t* val1, volatile uint8_t* val2, uint32_t secs);
// Converting string to uint8_t
int str_to_uint8_t(const char* lexeme, size_t lexeme_sz, uint8_t* val);
// Converting string to uint16_t
int str_to_uint16_t(const char* lexeme, size_t lexeme_sz, uint16_t* val);
// Converting string to uint32_t
int str_to_uint32_t(const char* lexeme, size_t lexeme_sz, uint32_t* val);
// Converting string to int_t
int str_to_int_t(const char* lexeme, size_t lexeme_sz, int* val);
// Put log
ssize_t put_log(uint8_t use_tar, const char* dir_src, const char* file_src, const char* dir_dst, const char* file_dst,
                const char* log_lst);
// Rotate log
void rotate_log(char* dir_src, char* file_src, char* dir_dst, const long file_size_max);
// Progress-bar
int show_progress_bar(long total_sz, long part_sz, uint8_t* percent_last, char* lexeme);
#ifdef __cplusplus
}
#endif
#endif  // GENERIC_H
