/*******************************************************************************
** Copyright (c) 2012 - 2023 v.arteev

** Permission is hereby granted, free of charge, to any person obtaining a copy
** of this software and associated documentation files (the "Software"), to deal
** in the Software without restriction, including without limitation the rights
** to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
** copies of the Software, and to permit persons to whom the Software is
** furnished to do so, subject to the following conditions:

** The above copyright notice and this permission notice shall be included in
all
** copies or substantial portions of the Software.

** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
** IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
** FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
** AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
** LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
** OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
** SOFTWARE.
*******************************************************************************/
#include <arpa/inet.h>  // for 'inet_ntop()', 'htonl()'
#include <errno.h>      // for 'errno'
#include <fcntl.h>      // for 'fstat'
#include <string.h>     // for 'memcpy'
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>    // for 'nanosleep'
#include <unistd.h>  // for 'sync()'
#ifndef __QNXNTO__
#include <linux/limits.h>  // for 'PATH_MAX'
#if defined(__APPLE__) || defined(__FreeBSD__)
#include <copyfile.h>
#else
#include <limits.h>
#include <sys/sendfile.h>
#endif
#endif
#include <stdio.h>   // for 'FILE'
#include <stdlib.h>  // for 'calloc()'

#include "protocol/generic.h"

/////////////////////////////////////////////////////////////////////////////////////
// htond, htohd
/////////////////////////////////////////////////////////////////////////////////////
double htond(double val) {
  uint64_t val1 = 0;
  double val2 = 0;
  memcpy(&val1, &val, sizeof(val));
  val1 = htonll(val1);
  memcpy(&val2, &val1, sizeof(val1));
  return val2;
}
double ntohd(double val) {
  uint64_t val1 = 0;
  double val2 = 0;
  memcpy(&val1, &val, sizeof(val));
  val1 = ntohll(val1);
  memcpy(&val2, &val1, sizeof(val1));
  return val2;
}
/////////////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////////////
// htonll (uint64_t)
uint64_t htonll(uint64_t val) {
  return (((uint64_t)htonl((uint32_t)((val)&0xFFFFFFFFLL)) << 32) | htonl((uint32_t)((val) >> 32)));
}
// ntohll (uint64_t)
uint64_t ntohll(uint64_t val) {
  return (((uint64_t)ntohl((uint32_t)((val)&0xFFFFFFFFLL)) << 32) | ntohl((uint32_t)((val) >> 32)));
}
// convert Internet host address to an Internet dot address
char* inet_ntoa_x(uint32_t val) {
  // See more: https://linux.die.net/man/3/inet_ntoa
  struct in_addr addr = {0};
  addr.s_addr = val;
  return inet_ntoa(addr);
}
// Clear iface from exceeded symbol ':'
ssize_t iface_clean(const char* iface, size_t iface_sz, char* dev, size_t dev_sz) {
  if (!iface || !dev) return -1;

  const char* sym = strchr(iface, ':');
  if (sym) {
    if (dev_sz >= (size_t)(sym - iface))
      memcpy(dev, iface, sym - iface);
    else
      return -1;
  } else {
    if (dev_sz >= iface_sz)
      memcpy(dev, iface, iface_sz);
    else
      return -1;
  }
  return 0;
}

// CRC16
static int crc16_table_ready = 0;
static uint32_t crc16_table[256];
// CRC32
static int crc32_table_ready = 0;
static uint32_t crc32_table[256];

/////////////////////////////////////////////////////////////////////////////////////
// CRC16
/////////////////////////////////////////////////////////////////////////////////////
uint16_t calc_crc16(uint16_t crc16, const unsigned char* data, size_t data_len) {
  if (!crc16_table_ready) {
    uint16_t r;
    int i = 0;
    for (i = 0; i < 256; i++) {
      r = ((uint16_t)i) << 8;
      uint8_t j = 0;
      for (j = 0; j < 8; j++) {
        if (r & (1 << 15))
          r = (r << 1) ^ 0x8005;
        else
          r = r << 1;
      }
      crc16_table[i] = r;
    }
    crc16_table_ready = 1;
  }

  crc16 = crc16 ^ 0xFFFF;
  while (data_len-- > 0) {
    crc16 = crc16_table[((crc16 >> 8) ^ *data++) & 0xFF] ^ (crc16 << 8);
  }
  return crc16 ^ 0xFFFF;
}

/////////////////////////////////////////////////////////////////////////////////////
// CRC32
/////////////////////////////////////////////////////////////////////////////////////
uint32_t calc_crc32(uint32_t crc32, const unsigned char* data, size_t data_len) {
  if (!crc32_table_ready) {
    uint32_t i, j;
    uint32_t h = 1;
    crc32_table[0] = 0;
    for (i = 128; i; i >>= 1) {
      h = (h >> 1) ^ (((h & 1) ? 0xEDB88320 : 0));
      for (j = 0; j < 256; j += 2 * i) {
        crc32_table[i + j] = crc32_table[j] ^ h;
      }
    }
    crc32_table_ready = 1;
  }

  crc32 = crc32 ^ 0xFFFFFFFF;
  while (data_len-- > 0) {
    crc32 = (crc32 >> 8) ^ crc32_table[(crc32 ^ *data++) & 0xFF];
  }
  return crc32 ^ 0xFFFFFFFF;
}

//////////////////////////////////////////////////////////////////////////////////
// Serialize/Deserialize
//////////////////////////////////////////////////////////////////////////////////
uint8_t* serialize_uint8_t(uint8_t* data, uint8_t value) {
  if (!data) return NULL;

  memcpy(data, &value, sizeof(value));
  return data + sizeof(value);
}
const uint8_t* deserialize_uint8_t(const uint8_t* data, uint8_t* value) {
  if (!data || !value) return NULL;

  memcpy(value, data, sizeof(*value));
  return data + sizeof(*value);
}
uint8_t* serialize_uint16_t(uint8_t* data, uint16_t value) {
  if (!data) return NULL;
  memcpy(data, &value, sizeof(value));
  return data + sizeof(value);
}

const uint8_t* deserialize_uint16_t(const uint8_t* data, uint16_t* value) {
  if (!data || !value) return NULL;

  memcpy(value, data, sizeof(*value));
  return data + sizeof(*value);
}

uint8_t* serialize_int32_t(uint8_t* data, int32_t value) {
  if (!data) return NULL;

  memcpy(data, &value, sizeof(value));
  return data + sizeof(value);
}

const uint8_t* deserialize_int32_t(const uint8_t* data, int32_t* value) {
  if (!data || !value) return NULL;

  memcpy(value, data, sizeof(*value));
  return data + sizeof(*value);
}

uint8_t* serialize_uint32_t(uint8_t* data, uint32_t value) {
  if (!data) return NULL;

  memcpy(data, &value, sizeof(value));
  return data + sizeof(value);
}

const uint8_t* deserialize_uint32_t(const uint8_t* data, uint32_t* value) {
  if (!data || !value) return NULL;

  memcpy(value, data, sizeof(*value));
  return data + sizeof(*value);
}
uint8_t* serialize_uint64_t(uint8_t* data, uint64_t value) {
  if (!data) return NULL;

  memcpy(data, &value, sizeof(value));
  return data + sizeof(value);
}
const uint8_t* deserialize_uint64_t(const uint8_t* data, uint64_t* value) {
  if (!data || !value) return NULL;

  memcpy(value, data, sizeof(*value));
  return data + sizeof(*value);
}
uint8_t* serialize_double_t(uint8_t* data, double value) {
  if (!data) return NULL;

  memcpy(data, &value, sizeof(value));
  return data + sizeof(value);
}
const uint8_t* deserialize_double_t(const uint8_t* data, double* value) {
  if (!data || !value) return NULL;

  memcpy(value, data, sizeof(*value));
  return data + sizeof(*value);
}
uint8_t* serialize_uint8_t_x(uint8_t* data, size_t* data_sz, size_t number_elements, ...) {
  va_list vl;
  va_start(vl, number_elements);
  uint8_t val = 0;
  size_t i = 0;
  for (i = 0; i < number_elements; ++i) {
    // CAUTION!: According to the C language specification, usual arithmetic
    // conversions are applied to the unnamed arguments of variadic
    // functions. Hence, any integer type shorter than int (e. g. bool, char
    // and short) are implicitly converted int; float is promoted to double,
    // etc.
    val = (uint8_t)va_arg(vl, int);
    // Check data size constraint
    if (sizeof(val) > *data_sz) {
      va_end(vl);
      return NULL;
    }
    // Val
    memcpy(data, &val, sizeof(val));
    data += sizeof(val);
    *data_sz -= sizeof(val);
  }
  va_end(vl);
  return data;
}
uint8_t* serialize_uint16_t_x(uint8_t* data, size_t* data_sz, size_t number_elements, ...) {
  va_list vl;
  va_start(vl, number_elements);
  uint16_t val = 0;
  size_t i = 0;
  for (i = 0; i < number_elements; ++i) {
    // CAUTION!: According to the C language specification, usual arithmetic
    // conversions are applied to the unnamed arguments of variadic
    // functions. Hence, any integer type shorter than int (e. g. bool, char
    // and short) are implicitly converted int; float is promoted to double,
    // etc.
    val = (uint16_t)va_arg(vl, int);
    // Check data size constraint
    if (sizeof(val) > *data_sz) {
      va_end(vl);
      return NULL;
    }
    // Val
    memcpy(data, &val, sizeof(val));
    data += sizeof(val);
    *data_sz -= sizeof(val);
  }
  va_end(vl);
  return data;
}
uint8_t* serialize_uint32_t_x(uint8_t* data, size_t* data_sz, size_t number_elements, ...) {
  va_list vl;
  va_start(vl, number_elements);
  uint32_t val = 0;
  size_t i = 0;
  for (i = 0; i < number_elements; ++i) {
    // CAUTION!: According to the C language specification, usual arithmetic
    // conversions are applied to the unnamed arguments of variadic
    // functions. Hence, any integer type shorter than int (e. g. bool, char
    // and short) are implicitly converted int; float is promoted to double,
    // etc.
    val = (uint32_t)va_arg(vl, uint32_t);
    // Check data size constraint
    if (sizeof(val) > *data_sz) {
      va_end(vl);
      return NULL;
    }
    // Val
    memcpy(data, &val, sizeof(val));
    data += sizeof(val);
    *data_sz -= sizeof(val);
  }
  va_end(vl);
  return data;
}
uint8_t* serialize_uint64_t_x(uint8_t* data, size_t* data_sz, size_t number_elements, ...) {
  va_list vl;
  va_start(vl, number_elements);

  uint64_t val = 0;
  size_t i = 0;
  for (i = 0; i < number_elements; ++i) {
    // CAUTION!: According to the C language specification, usual arithmetic
    // conversions are applied to the unnamed arguments of variadic
    // functions. Hence, any integer type shorter than int (e. g. bool, char
    // and short) are implicitly converted int; float is promoted to double,
    // etc.
    val = (uint64_t)va_arg(vl, uint64_t);
    // Check data size constraint
    if (sizeof(val) > *data_sz) {
      va_end(vl);
      return NULL;
    }
    // Val
    memcpy(data, &val, sizeof(val));
    data += sizeof(val);
    *data_sz -= sizeof(val);
  }
  va_end(vl);
  return data;
}
const uint8_t* deserialize_uint8_t_x(const uint8_t* data, size_t* data_sz, size_t number_elements, ...) {
  if (!data || !data_sz) return NULL;

  va_list vl;
  va_start(vl, number_elements);
  uint8_t* val = NULL;
  size_t i = 0;
  for (i = 0; i < number_elements; ++i) {
    val = va_arg(vl, uint8_t*);
    // Check data size constraint
    if (sizeof(*val) > *data_sz) {
      va_end(vl);
      return NULL;
    }
    data = deserialize_uint8_t(data, val);
    *data_sz -= sizeof(*val);
  }
  va_end(vl);
  return data;
}
const uint8_t* deserialize_uint16_t_x(const uint8_t* data, size_t* data_sz, size_t number_elements, ...) {
  if (!data || !data_sz) return NULL;

  va_list vl;
  va_start(vl, number_elements);
  uint16_t* val = NULL;
  size_t i = 0;
  for (i = 0; i < number_elements; ++i) {
    val = va_arg(vl, uint16_t*);
    // Check data size constraint
    if (sizeof(*val) > *data_sz) {
      va_end(vl);
      return NULL;
    }
    data = deserialize_uint16_t(data, val);
    *data_sz -= sizeof(*val);
  }
  va_end(vl);
  return data;
}
const uint8_t* deserialize_uint16_t_h_x(const uint8_t* data, size_t* data_sz, size_t number_elements, ...) {
  if (!data || !data_sz) return NULL;

  va_list vl;
  va_start(vl, number_elements);
  uint16_t* val = NULL;
  size_t i = 0;
  for (i = 0; i < number_elements; ++i) {
    val = va_arg(vl, uint16_t*);
    // Check data size constraint
    if (sizeof(*val) > *data_sz) {
      va_end(vl);
      return NULL;
    }
    data = deserialize_uint16_t(data, val);
    *val = ntohs(*val);
    *data_sz -= sizeof(*val);
  }
  va_end(vl);
  return data;
}
const uint8_t* deserialize_uint32_t_x(const uint8_t* data, size_t* data_sz, size_t number_elements, ...) {
  if (!data || !data_sz) return NULL;

  va_list vl;
  va_start(vl, number_elements);
  uint32_t* val = NULL;
  size_t i = 0;
  for (i = 0; i < number_elements; ++i) {
    val = va_arg(vl, uint32_t*);
    // Check data size constraint
    if (sizeof(*val) > *data_sz) {
      va_end(vl);
      return NULL;
    }
    data = deserialize_uint32_t(data, val);
    *data_sz -= sizeof(*val);
  }
  va_end(vl);
  return data;
}
const uint8_t* deserialize_uint32_t_h_x(const uint8_t* data, size_t* data_sz, size_t number_elements, ...) {
  if (!data || !data_sz) return NULL;

  va_list vl;
  va_start(vl, number_elements);
  uint32_t* val = NULL;
  size_t i = 0;
  for (i = 0; i < number_elements; ++i) {
    val = va_arg(vl, uint32_t*);
    // Check data size constraint
    if (sizeof(*val) > *data_sz) {
      va_end(vl);
      return NULL;
    }
    data = deserialize_uint32_t(data, val);
    *val = ntohl(*val);
    *data_sz -= sizeof(*val);
  }
  va_end(vl);
  return data;
}
const uint8_t* deserialize_uint64_t_x(const uint8_t* data, size_t* data_sz, size_t number_elements, ...) {
  if (!data || !data_sz) return NULL;

  va_list vl;
  va_start(vl, number_elements);
  uint64_t* val = NULL;
  size_t i = 0;
  for (i = 0; i < number_elements; ++i) {
    val = va_arg(vl, uint64_t*);
    // Check data size constraint
    if (sizeof(*val) > *data_sz) {
      va_end(vl);
      return NULL;
    }
    data = deserialize_uint64_t(data, val);
    *data_sz -= sizeof(*val);
  }
  va_end(vl);
  return data;
}
const uint8_t* deserialize_uint64_t_h_x(const uint8_t* data, size_t* data_sz, size_t number_elements, ...) {
  if (!data || !data_sz) return NULL;

  va_list vl;
  va_start(vl, number_elements);
  uint64_t* val = NULL;
  size_t i = 0;
  for (i = 0; i < number_elements; ++i) {
    val = va_arg(vl, uint64_t*);
    // Check data size constraint
    if (sizeof(*val) > *data_sz) {
      va_end(vl);
      return NULL;
    }
    data = deserialize_uint64_t(data, val);
    *val = ntohll(*val);
    *data_sz -= sizeof(*val);
  }
  va_end(vl);
  return data;
}
const uint8_t* deserialize_double_t_x(const uint8_t* data, size_t* data_sz, size_t number_elements, ...) {
  if (!data || !data_sz) return NULL;

  va_list vl;
  va_start(vl, number_elements);
  double* val = NULL;
  size_t i = 0;
  for (i = 0; i < number_elements; ++i) {
    val = va_arg(vl, double*);
    // Check data size constraint
    if (sizeof(*val) > *data_sz) {
      va_end(vl);
      return NULL;
    }
    data = deserialize_double_t(data, val);
    *data_sz -= sizeof(*val);
  }
  va_end(vl);
  return data;
}
const uint8_t* deserialize_double_t_h_x(const uint8_t* data, size_t* data_sz, size_t number_elements, ...) {
  if (!data || !data_sz) return NULL;

  va_list vl;
  va_start(vl, number_elements);
  double* val = NULL;
  size_t i = 0;
  for (i = 0; i < number_elements; ++i) {
    val = va_arg(vl, double*);
    // Check data size constraint
    if (sizeof(*val) > *data_sz) {
      va_end(vl);
      return NULL;
    }
    data = deserialize_double_t(data, val);
    *val = ntohd(*val);
    *data_sz -= sizeof(*val);
  }
  va_end(vl);
  return data;
}
const uint8_t* deserialize_array_x(const uint8_t* data, size_t* data_sz, size_t number_elements, ...) {
  if (!data || !data_sz) return NULL;

  va_list vl;
  va_start(vl, number_elements);
  void* val = NULL;
  size_t val_sz = 0;
  size_t i = 0;
  for (i = 0; i < number_elements / 2; ++i) {
    val = va_arg(vl, void*);
    val_sz = va_arg(vl, size_t);
    // Check data size constraint
    if (val_sz > *data_sz) {
      va_end(vl);
      return NULL;
    }
    memcpy(val, data, val_sz);
    data += val_sz;
    *data_sz -= val_sz;
  }
  va_end(vl);
  return data;
}
//
uint8_t* new_serialize_uint8_t_x(uint8_t** data, size_t* data_sz, size_t number_elements, ...) {
  if (!data || !data_sz) return NULL;

  va_list vl;
  va_start(vl, number_elements);
  uint8_t val = 0;
  size_t i = 0;
  for (i = 0; i < number_elements; ++i) {
    // CAUTION!: According to the C language specification, usual arithmetic
    // conversions are applied to the unnamed arguments of variadic
    // functions. Hence, any integer type shorter than int (e. g. bool, char
    // and short) are implicitly converted int; float is promoted to double,
    // etc.
    val = (uint8_t)va_arg(vl, int);
    uint8_t* ptr = (uint8_t*)realloc(*data, *data_sz + sizeof(val));
    if (!ptr) {
      // Free memory (that already exists)
      free(*data);
      *data = NULL;
      va_end(vl);
      return NULL;
    }
    *data = ptr;
    // Val
    memcpy(*data + *data_sz, &val, sizeof(val));
    *data_sz += sizeof(val);
  }
  va_end(vl);

  return *data;
}
uint8_t* new_serialize_uint16_t_x(uint8_t** data, size_t* data_sz, size_t number_elements, ...) {
  if (!data || !data_sz) return NULL;

  va_list vl;
  va_start(vl, number_elements);
  uint16_t val = 0;
  size_t i = 0;
  for (i = 0; i < number_elements; ++i) {
    // CAUTION!: According to the C language specification, usual arithmetic
    // conversions are applied to the unnamed arguments of variadic
    // functions. Hence, any integer type shorter than int (e. g. bool, char
    // and short) are implicitly converted int; float is promoted to double,
    // etc.
    val = (uint16_t)va_arg(vl, int);
    uint8_t* ptr = (uint8_t*)realloc(*data, *data_sz + sizeof(val));
    if (!ptr) {
      // Free memory (that already exists)
      free(*data);
      *data = NULL;
      va_end(vl);
      return NULL;
    }
    *data = ptr;
    // Val
    memcpy(*data + *data_sz, &val, sizeof(val));
    *data_sz += sizeof(val);
  }
  va_end(vl);

  return *data;
}
uint8_t* new_serialize_uint32_t_x(uint8_t** data, size_t* data_sz, size_t number_elements, ...) {
  if (!data || !data_sz) return NULL;

  va_list vl;
  va_start(vl, number_elements);
  uint32_t val = 0;
  size_t i = 0;
  for (i = 0; i < number_elements; ++i) {
    val = (uint32_t)va_arg(vl, uint32_t);
    uint8_t* ptr = (uint8_t*)realloc(*data, *data_sz + sizeof(val));
    if (!ptr) {
      // Free memory (that already exists)
      free(*data);
      *data = NULL;
      va_end(vl);
      return NULL;
    }
    *data = ptr;
    // Val
    memcpy(*data + *data_sz, &val, sizeof(val));
    *data_sz += sizeof(val);
  }
  va_end(vl);

  return *data;
}
uint8_t* new_serialize_uint64_t_x(uint8_t** data, size_t* data_sz, size_t number_elements, ...) {
  if (!data || !data_sz) return NULL;

  va_list vl;
  va_start(vl, number_elements);
  uint64_t val = 0;
  size_t i = 0;
  for (i = 0; i < number_elements; ++i) {
    val = (uint64_t)va_arg(vl, uint64_t);
    uint8_t* ptr = (uint8_t*)realloc(*data, *data_sz + sizeof(val));
    if (!ptr) {
      // Free memory (that already exists)
      free(*data);
      *data = NULL;
      va_end(vl);
      return NULL;
    }
    *data = ptr;
    // Val
    memcpy(*data + *data_sz, &val, sizeof(val));
    *data_sz += sizeof(val);
  }
  va_end(vl);

  return *data;
}
uint8_t* new_serialize_double_t_x(uint8_t** data, size_t* data_sz, size_t number_elements, ...) {
  if (!data || !data_sz) return NULL;

  va_list vl;
  va_start(vl, number_elements);
  double val = 0;
  size_t i = 0;
  for (i = 0; i < number_elements; ++i) {
    val = (double)va_arg(vl, double);
    uint8_t* ptr = (uint8_t*)realloc(*data, *data_sz + sizeof(val));
    if (!ptr) {
      // Free memory (that already exists)
      free(*data);
      *data = NULL;
      va_end(vl);
      return NULL;
    }
    *data = ptr;
    // Val
    memcpy(*data + *data_sz, &val, sizeof(val));
    *data_sz += sizeof(val);
  }
  va_end(vl);

  return *data;
}
uint8_t* new_serialize_array_x(uint8_t** data, size_t* data_sz, size_t number_elements, ...) {
  if (!data || !data_sz) return NULL;

  va_list vl;
  va_start(vl, number_elements);
  const void* val = NULL;
  size_t val_sz = 0;
  size_t i = 0;
  // NOTE: total count of elements need divide to 2, by reason that series of
  // elements need meant as pair <array elements:size array>, <array
  // elements:size array> ... where first element 'array elements' contains
  // array, second element 'size array' contains size of array.
  for (i = 0; i < number_elements / 2; ++i) {
    // get the pointer to array elements
    val = (const void*)va_arg(vl, const void*);
    // get the count of elements array
    val_sz = (size_t)va_arg(vl, size_t);
    uint8_t* ptr = (uint8_t*)realloc(*data, *data_sz + val_sz);
    if (!ptr) {
      // Free memory (that already exists)
      free(*data);
      *data = NULL;
      va_end(vl);
      return NULL;
    }
    *data = ptr;
    // Val1
    memcpy(*data + *data_sz, val, val_sz);
    *data_sz += val_sz;
  }
  va_end(vl);

  return *data;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// Sleep
//////////////////////////////////////////////////////////////////////////////////////////////////
int sleep_x(time_t secs, long nanosecs) {
  ///////////////////////////////////////////////////////////////////////
  // TIMER #1 C99 + glibc2.16
  ///////////////////////////////////////////////////////////////////////
  // usleep(); // microseconds
  ///////////////////////////////////////////////////////////////////////
  // TIMER #2 C11 + glibc2.28 + <threads.h> see link:
  // https://stackoverflow.com/questions/22875545/c11-gcc-threads-h-not-found
  ///////////////////////////////////////////////////////////////////////
  // struct timespec rqtp= {secs, nanosecs};
  // struct timespec rmtp= {0};
  // thrd_sleep(&rqtp, &rmtp); // nanoseconds
  ///////////////////////////////////////////////////////////////////////
  // TIMER #3 C99 + glibc2.16
  ///////////////////////////////////////////////////////////////////////
  // CAUTION: 1 ms (millisecond) == 1000 (microseconds) == 1 000 000 ns
  // (nanoseconds)
  struct timespec tmx = {secs, nanosecs};
  while (nanosleep(&tmx, &tmx)) {
    ;  // fprintf(stdout, "ERRN: nanosleep() cause:'%s'\n",
       // strerror(errno));
       // fflush(stdout);
  }
  return 0;
}
//////////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////////
int memcmp_x(void* val1, void* val2, size_t val2_sz) {
  if (strlen((const char*)val1) < val2_sz) return -1;
  if (memcmp(val1, val2, val2_sz) == 0) return 0;

  return -1;
}
int memcpy_x(void* dst, size_t dst_sz, const void* src, size_t src_sz) {
  // Failure to observe the requirement that the memory areas do not
  // overlap has been the source of real bugs.  (POSIX and the C standards
  // are explicit that employing memcpy() with overlapping areas produces
  // undefined behavior.)
  // See link: http://man7.org/linux/man-pages/man3/memcpy.3.html

  if (!dst) return -1;

  memset(dst, 0, dst_sz);
  if (src) {
    if (dst_sz >= src_sz)
      memcpy(dst, src, src_sz);
    else
      return -1;
  }
  return 0;
}
// Converting string to uint8_t
int str_to_uint8_t(const char* lexeme, size_t lexeme_sz, uint8_t* val) {
  if (!lexeme || !val) return -1;

  char buf[lexeme_sz + 1];
  memset(&buf, 0, sizeof(buf));
  if (memcpy_x(buf, sizeof(buf), lexeme, lexeme_sz) != 0) return -1;
  // set errno 0
  errno = 0;
  *val = (uint8_t)strtoul(buf, NULL, 10);
  if (ERANGE == errno) return -1;
  return 0;
}
// Converting string to uint16_t
int str_to_uint16_t(const char* lexeme, size_t lexeme_sz, uint16_t* val) {
  if (!lexeme || !val) return -1;

  char buf[lexeme_sz + 1];
  memset(&buf, 0, sizeof(buf));
  if (memcpy_x(buf, sizeof(buf), lexeme, lexeme_sz) != 0) return -1;
  // set errno 0
  errno = 0;
  *val = (uint16_t)strtoul(buf, NULL, 10);
  if (ERANGE == errno) return -1;
  return 0;
}
// Converting string to uint32_t
int str_to_uint32_t(const char* lexeme, size_t lexeme_sz, uint32_t* val) {
  if (!lexeme || !val) return -1;

  char buf[lexeme_sz + 1];
  memset(&buf, 0, sizeof(buf));
  if (memcpy_x(buf, sizeof(buf), lexeme, lexeme_sz) != 0) return -1;
  // set errno 0
  errno = 0;
  *val = (uint32_t)strtoul(buf, NULL, 10);
  if (ERANGE == errno) return -1;
  return 0;
}
// Converting string to int_t
int str_to_int_t(const char* lexeme, size_t lexeme_sz, int* val) {
  if (!lexeme || !val) return -1;

  char buf[lexeme_sz + 1];
  if (memcpy_x(buf, sizeof(buf), lexeme, lexeme_sz) != 0) return -1;
  // set errno 0
  errno = 0;
  *val = strtol(buf, NULL, 10);
  if (ERANGE == errno) return -1;
  return 0;
}
// Copy file
int copy_file(const char* source, const char* destination) {
#ifdef __QNXNTO__
  return -1;
#else
  int input = -1;
  int output = -1;
  if ((input = open(source, O_RDONLY)) == -1) {
    return -1;
  }
  // Mode for output file
  mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
  if ((output = open(destination, O_RDWR | O_CREAT, mode)) == -1) {
    close(input);
    return -1;
  }
  // Here we use kernel-space copying for performance reasons
#if defined(__APPLE__) || defined(__FreeBSD__)
  // CAUTION: 'fcopyfile' works on FreeBSD and OS X 10.5+
  int result = fcopyfile(input, output, 0, COPYFILE_ALL);
#else
  // CAUTION: 'sendfile' will work with non-socket output (i.e. regular file)
  // on Linux 2.6.33+ In Linux kernels before 2.6.33, out_fd must refer to a
  // socket. Since Linux 2.6.33 it can be any file. If it is a regular file,
  // then sendfile() changes the file offset appropriately.
  off_t bytesCopied = 0;
  struct stat fileinfo = {0};
  fstat(input, &fileinfo);
  // IMPORTANT: sendfile() copies data between one file descriptor and
  // another. Because this copying is done within the kernel, sendfile() is
  // more efficient than the combination of read(2) and write(2), which would
  // require transferring data to and from user space.
  int result = sendfile(output, input, &bytesCopied, fileinfo.st_size);
#endif
  close(input);
  close(output);

  return result;
#endif
}
// Copy file
ssize_t copy_file_x(const char* src_path, const char* src_file, const char* dst_path, const char* dst_file) {
  char src[PATH_MAX] = {0};
  snprintf(src, sizeof(src), "%s/%s", src_path, src_file);
  char dst[PATH_MAX] = {0};
  snprintf(dst, sizeof(dst), "%s/%s", dst_path, dst_file);
  if (copy_file(src, dst) == -1) return -1;
  return 0;
}
// File size
long file_size(const char* filename) {
  struct stat st;
  int ret = stat(filename, &st);
  if (ret < 0) return -1;
  return st.st_size;
}
// Read text file
int read_text_file(const char* filename, char** buf, long* file_sz_out) {
  if (!filename || !buf || !file_sz_out) return -1;

  ///////////////////////////////////////////////////////////////
  // Open and load file
  ///////////////////////////////////////////////////////////////
  FILE* pFile = fopen(filename, "rb");
  if (!pFile) {
    return -1;
  }
  long file_sz = file_size(filename);
  //
  size_t data_sz = 0;
  if (file_sz > 0)
    data_sz = (size_t)file_sz;
  else {
    fprintf(stderr, "INFO: file have '%s' zero size\n", filename);
    // Close file
    fclose(pFile);
    return -1;
  }

  // Read text file to buffer
  *buf = (char*)calloc(data_sz + 1, sizeof(char));
  if (!*buf) {
    // Close file
    fclose(pFile);
    return -1;
  }
  // Read to buffer
  if (fread(*buf, 1, data_sz, pFile) != data_sz) {
    free(*buf);
    *buf = NULL;
    // Close file
    fclose(pFile);
    return -1;
  }
  // Close file
  fclose(pFile);
  // Set value
  *file_sz_out = file_sz;
  return 0;
}
// Write file
int write_file(const char* filename, const char* buf, size_t buf_sz, const char* mode) {
  if (!buf || !filename) return -1;

  // Open file
  FILE* fhandle = fopen(filename, mode);
  if (!fhandle) {
    return -1;
  }
  size_t write_bytes = fwrite(buf, sizeof(buf[0]), buf_sz, fhandle);

  // Close handler
  fflush(fhandle);
  fclose(fhandle);

  if (write_bytes != buf_sz) {
    return -1;
  }
  // first commits inodes to buffers, and then buffers to disk.
  sync();
  return 0;
}
// Remove file
int remove_file(const char* dir, const char* file) {
  if (!dir || !file) return -1;
  char path[PATH_MAX] = {0};
  snprintf(path, sizeof(path), "%s/%s", dir, file);
  int res = unlink(path);
  // first commits inodes to buffers, and then buffers to disk.
  sync();
  return res;
}
// Truncate file
int truncate_file(const char* dir, const char* file) {
  if (!dir || !file) return -1;
  char path[PATH_MAX] = {0};
  snprintf(path, sizeof(path), "%s/%s", dir, file);
  int res = truncate(path, 0);
  // first commits inodes to buffers, and then buffers to disk.
  sync();
  return res;
}
// Is dir
int is_dir(uint8_t* data, int* status, uint32_t* err_no) {
  if (!data || !status || !err_no) return -1;

  struct stat st;
  *status = stat((char*)data, &st);
  *err_no = errno;
  if (0 == *status)
    return ((st.st_mode & S_IFMT) == S_IFDIR) ? (1) : (0);
  else
    return -1;
}
// Make dir with nested dirs. Like '$ mkdir -p'
int mkdir_ex(const char* data, uint32_t mode, int* status, uint32_t* err_no) {
  if (!data || !status || !err_no) return -1;

  // "/home/cnd/mod1", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH

  char separator = '/';
  char tmp[PATH_MAX] = {0};
  char* p = NULL;
  size_t len;

  snprintf(tmp, sizeof(tmp), "%s", (char*)data);
  len = strlen(tmp);

  if (tmp[len - 1] == separator) tmp[len - 1] = 0;
  for (p = tmp + 1; *p; p++)
    if (*p == separator) {
      *p = 0;
      *status = mkdir(tmp, mode);
      *p = separator;
    }
  *status = mkdir(tmp, mode);

  *err_no = errno;
  return (0 == *status) ? 0 : -1;
}
ssize_t wait_while_equal(volatile uint8_t* val1, volatile uint8_t* val2, uint32_t secs) {
  if (!val1 || !val2) return -1;

  uint32_t delay_msec = 1;
  uint32_t count = 0;
  uint32_t count_max = secs * 1000;
  while (*val1 == *val2) {
    if (++count >= count_max) return -1;
    sleep_x(0, delay_msec * 1000 * 1000);  // 1 msec
  }
  return 0;
}
ssize_t wait_while_greater(volatile uint8_t* val1, volatile uint8_t* val2, uint32_t secs) {
  if (!val1 || !val2) return -1;
  uint32_t delay_msec = 1;
  uint32_t count = 0;
  uint32_t count_max = secs * 1000;
  while (*val1 > *val2) {
    if (++count >= count_max) return -1;
    sleep_x(0, delay_msec * 1000 * 1000);  // 1 msec
  }
  return 0;
}
ssize_t put_log(uint8_t use_tar, const char* dir_src, const char* file_src, const char* dir_dst, const char* file_dst,
                const char* log_lst) {
  if (!dir_src || !file_src || !dir_dst || !file_dst) return -1;

  char path_src[PATH_MAX] = {0};
  snprintf(path_src, sizeof(path_src), "%s/%s", dir_src, file_src);
  char path_dst[PATH_MAX] = {0};
  snprintf(path_dst, sizeof(path_dst), "%s/%s", dir_dst, file_dst);

  ////////////////////////////////////////////////////////////////////////////
  // Check path destination folder
  ////////////////////////////////////////////////////////////////////////////
  if (access(dir_dst, F_OK) == -1) {
    int status = 0;
    uint32_t err_no = 0;
    mkdir_ex(dir_dst, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH, &status, &err_no);
  }
  if (use_tar) {
    char cmd[] = "tar -czf";
    char content[(PATH_MAX * 2) + sizeof(cmd)] = {0};
    snprintf(content, sizeof(content), "%s %s %s", cmd, path_dst, path_src);
    int ret = system(content);
    if (!WIFEXITED(ret)) return -1;
  } else {
    if (copy_file(path_src, path_dst) == -1) return -1;
  }
  // If list files needed
  if (log_lst) {
    char log_list_dst[PATH_MAX] = {0};
    snprintf(log_list_dst, sizeof(log_list_dst), "%s/%s", dir_dst, log_lst);
    if (write_file(log_list_dst, file_dst, strlen(file_dst), "a") == -1) return -1;
    if (write_file(log_list_dst, "\n", strlen("\n"), "a") == -1) return -1;
  }
  return 0;
}
// Rotate log
volatile uint8_t lock_rotatelog = 0;
void rotate_log(char* dir_src, char* file_src, char* dir_dst, const long file_size_max) {
  if (!dir_src || !file_src || !dir_dst) return;

  if (lock_rotatelog) return;
  // lock
  lock_rotatelog = 1;

  /////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  // Compose full path
  // CAUTION: it's not recommended to use the functions: basename() and
  // dirname(), because they have many errors! See link:
  // http://man7.org/linux/man-pages/man3/basename.3.html
  /////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  char path_src[PATH_MAX] = {0};
  snprintf(path_src, sizeof(path_src), "%s/%s", dir_src, file_src);

  // Get file size
  struct stat st;
  stat(path_src, &st);
  long file_size = st.st_size;
  // Compare sizes
  if (file_size >= file_size_max) {
    ////////////////////////////////////////////////////////////////////////////
    // Compose archive name
    ////////////////////////////////////////////////////////////////////////////
    static int file_part_cnt = 0;  // counter for file parts
    char file_dst[FILENAME_MAX] = {0};
    int res = snprintf(file_dst, sizeof(file_dst), "%s_%d.tgz", file_src, file_part_cnt++);
    if (res < 0) {
      // unlock
      lock_rotatelog = 0;
      return;
    }
    ////////////////////////////////////////////////////////////////////////////
    // Compress log
    ////////////////////////////////////////////////////////////////////////////
    char log_list[] = "loglist";
    if (put_log(1, dir_src, file_src, dir_dst, file_dst, log_list) != 0) {
      // unlock
      lock_rotatelog = 0;
      return;
    }
    ////////////////////////////////////////////////////////////////////////////
    // Clear log
    ////////////////////////////////////////////////////////////////////////////
    char cmd[PATH_MAX] = {0};
    res = snprintf(cmd, sizeof(cmd), "echo -n \'\' > %s", path_src);
    if (res < 0) {
      // unlock
      lock_rotatelog = 0;
      return;
    }
    // Execute command
    res = system(cmd);
    if (!WIFEXITED(res)) {
      // unlock
      lock_rotatelog = 0;
      return;
    }
  }
  // unlock
  lock_rotatelog = 0;
  return;
}

// Progress-bar
int show_progress_bar(long total_sz, long part_sz, uint8_t* percent_last, char* lexeme) {
  if (!percent_last) return -1;

  if (total_sz) {
    double read_sz = part_sz;
    uint8_t percent = (uint8_t)((read_sz / total_sz) * 100);
    if (*percent_last != percent) {
      *percent_last = percent;
      fprintf(stdout, "%s%d%%", ((lexeme) ? (lexeme) : ("")), percent);
      fprintf(stdout, "\r");
      fflush(stdout);
    }
  }
  return 0;
}
