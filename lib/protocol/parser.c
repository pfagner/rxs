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
#include <arpa/inet.h>  // for 'inet_ntop()', 'htonl()'
#include <errno.h>      // for 'errno'
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>      // for 'stderr'
#include <stdlib.h>     // for 'calloc()'
#include <string.h>     // for 'strlen()'
#include <time.h>       // for 'strptime()’
#ifdef __QNXNTO__
#include <sys/socket.h>
#include <unistd.h>
#define __THROW
#else
#include <getopt.h>     // for 'getopt()'
#endif
#include <inttypes.h>   // for print "PRIu32" type 'uint32_t'
#include <limits.h>     // for 'UINT_MAX'

#include "container/list.h"
#include "protocol/generic.h"
#include "protocol/internal_types.h"
#include "protocol/parser.h"

// extern int have_encoder;
///////////////////////////////////////////////////////////////
// Common variables
///////////////////////////////////////////////////////////////
// NOTE: From the header, 'snprintf' is only available if you are compiling C99, BSD, or UNIX98 code.
// Since ANSI C (pedantic or not) isn't any of those, it doesn't declare 'snprintf'.
extern int snprintf(char* __restrict __s, size_t __maxlen, __const char* __restrict __format, ...) __THROW;
static size_t int2size_t(int val) { return (val < 0) ? UINT_MAX : (size_t)((unsigned)val); }
// Compose type user_info_t to type void (object)
static int compose_user_info_t_to_object(const char* lexeme, size_t lexeme_sz, size_t counter, void* object) {
  if (!lexeme || !lexeme_sz || !object) return -1;

  user_info_t* user_info = (user_info_t*)object;
  switch (counter) {
    // user
    case 0:
      if (memcpy_x(user_info->name, sizeof(user_info->name), lexeme, lexeme_sz) != 0) return -1;
      break;
    // pass
    case 1:
      if (memcpy_x(user_info->pass, sizeof(user_info->pass), lexeme, lexeme_sz) != 0) return -1;
      break;
    // group
    case 2:
      if (memcpy_x(user_info->group, sizeof(user_info->group), lexeme, lexeme_sz) != 0) return -1;
      break;
    // home dir
    case 3:
      if (memcpy_x(user_info->home_dir, sizeof(user_info->home_dir), lexeme, lexeme_sz) != 0) return -1;
      break;
  }
  return 0;
}
// Find group
static int find_group(const char* group_beg, const char* delimeter_group, int* is_last_group, uint32_t* group_sz,
                      char** group_end) {
  if (!group_beg || !delimeter_group || !is_last_group || !group_sz || !group_end) return -1;

  // Find separator
  *group_end = strstr(group_beg, delimeter_group);
  *group_sz = 0;
  // Reassign for last
  if (!*group_end) {
    *is_last_group = 1;
    *group_sz = strlen(group_beg);
    if (!*group_sz) return -1;
  } else {
    *group_sz = (*group_end - group_beg);
  }
  return 0;
}
// Parse group, e.g.: 'zzz,yyy,aaa,bbb' Explanation: zzz - is group, symbol ',' - is delimeter group
static int parse_group_01(const char* lexeme, size_t lexeme_sz, const char* delimeter_group,
                          int (*compose_01)(const char* lexeme, size_t lexeme_sz, size_t counter, uint32_t* val1,
                                            uint32_t* val2, dlist_t** dst_lst),
                          uint32_t* val1, uint32_t* val2, dlist_t** dst_lst,
                          int (*compose_02)(const char* lexeme, size_t lexeme_sz, size_t counter, void* object_02),
                          void* object_02) {
  if (!lexeme || !lexeme_sz || !delimeter_group) return -1;

  char buf[lexeme_sz + 1];
  if (memcpy_x(buf, sizeof(buf), lexeme, lexeme_sz) != 0) {
    fprintf(stderr, "ERRN: copy has been failed\n");
    return -1;
  }
  ////////////////////////////////////////////////////////////////////////////
  // Parse address
  ////////////////////////////////////////////////////////////////////////////
  char* group_beg = buf;
  char* group_end = NULL;
  uint32_t i = 0;
  int is_last_group = 0;
  while (!is_last_group && group_beg) {
    // Find separator
    uint32_t group_sz = 0;
    if (find_group(group_beg, delimeter_group, &is_last_group, &group_sz, &group_end) != 0) {
      fprintf(stderr, "ERRN: find group has been failed\n");
      return -1;
    }
    ////////////////////////////////////////////////////////////////////////////
    //
    ////////////////////////////////////////////////////////////////////////////
    if (compose_01) {
      if (compose_01(group_beg, int2size_t(group_sz), i, val1, val2, dst_lst) != 0) {
        fprintf(stderr, "ERRN: compose 01 has been failed\n");
        return -1;
      }
    } else if (compose_02) {
      if (compose_02(group_beg, int2size_t(group_sz), i, object_02) != 0) {
        fprintf(stderr, "ERRN: compose 02 has been failed\n");
        return -1;
      }
    }
    ////////////////////////////////////////////////////////////////////////////
    // Set next
    ////////////////////////////////////////////////////////////////////////////
    group_beg = (group_end) ? (group_end + strlen(delimeter_group)) : (NULL);
    // Set counter
    i++;
  }
  return 0;
}
// Parse address list
ssize_t parse_addr(char* optarg, dlist_t** addr_allowed_lst) {
  char* saveptr = NULL;
  char* lexeme = strtok_r(optarg, ",", &saveptr);

  while (lexeme) {
    uint32_t addr_n = 0;
    if (inet_pton(AF_INET, lexeme, &addr_n) == 0) {
      printf("invalid address %s\n", lexeme);
      return -1;
    }
    void* data = ctor_addr_t(addr_n);
    if (!data) return -1;
    if (list_push_back(addr_allowed_lst, data) == NULL) {
      return -1;
    }
    lexeme = strtok_r(NULL, ",", &saveptr);
  }
  return 0;
}
// Parse cmd's arguments
ssize_t parse_args(int cnt, char* val[], uint32_t* addr_rxs, uint16_t* port_rxs, dlist_t** addr_allowed_lst, int* mode_running, char* file_users, int* pid_host, uint8_t * encoder_mode)
{
#ifdef __QNXNTO__
  return 0;
#else
  // Check parameters
  if(!val || !addr_rxs || !port_rxs || !addr_allowed_lst || !mode_running)
    return -1;

  int c = 0;
  int digit_optind = 0;
  // Parameter's table
  while (1)
  {
    int this_option_optind = optind ? optind : 1;
    int option_index = 0;
    struct option long_options[] =
    {
        // Common keys
        {"addr_rxs",               required_argument,  0,  'g' },
        {"port_rxs",               required_argument,  0,  'h' },
        {"addr_allowed",           required_argument,  0,  'i' },
        {"mode",                   required_argument,  0,  'l' },
        {"file_users",             required_argument,  0,  'f' },
        {"pid",                    required_argument,  0,  'p' },
        {"encoder",                no_argument,        0,  'e' },
        {0, 0,  0,  0 }
    };

    c = getopt_long(cnt, val, "abc:d:012", long_options, &option_index);
    if (c == -1)
    {
      break;
    }
    switch (c)
    {
      case 0:
      if (optarg)
        printf(" with arg %s", optarg);
      printf("\n");
      break;
      case '0':
      case '1':
      case '2':
      if (digit_optind != 0 && digit_optind != this_option_optind)
        printf("digits occur in two different argv-elements.\n");
      digit_optind = this_option_optind;
      printf("option %c\n", c);
      break;
      // addr_rxs
      case 'g':
      {
        if(inet_pton(AF_INET, optarg, addr_rxs) == 0)
        {
          fprintf(stderr, "ERRN: invalid address %s\n", optarg);
          return -1;
        }
        break;
      }
      // port_rxs
      case 'h':
      {
        if(str_to_uint16_t(optarg, strlen(optarg), port_rxs ) != 0)
        {
          fprintf(stderr, "ERRN: invalid value %s\n", optarg);
          return -1;
        }
        break;
      }
      // addr_allowed
      case 'i':
      {
        if(parse_addr(optarg, addr_allowed_lst) != 0)
        {
          return -1;
        }
        break;
      }
      // daemon
      case 'l':
      {
        if(memcmp_x(optarg, "daemon", strlen("daemon")) == 0)
          *mode_running = 1;
        break;
      }
      // file
      case 'f':
        strcpy(file_users, optarg );
      break;
      // have_encoder
      case 'e':
        *encoder_mode = 1;
      break;
      case 'p':
      {
        if(str_to_int_t(optarg, strlen(optarg), pid_host ) != 0)
        {
          fprintf(stderr, "ERRN: invalid value %s\n", optarg);
          return -1;
        }
        break;
      }
      case '?':
        printf("ERRN: unknown option '%s' \n", val[optind-1]);
        return -1;
      default:
        printf("?? getopt returned character code 0%o ??\n", c);
      }
    }

    if(optind < cnt)
    {
      printf("non-option ARGV-elements: ");
      while(optind < cnt)
        printf("%s ", val[optind++]);
      printf("\n");

      return -1;
    }
    return 0;
#endif
}
// Parse user info file
ssize_t parse_user_info(const char* filename, dlist_t** user_info_t_lst) {
  if (!filename || !user_info_t_lst) {
    fprintf(stderr, "ERRN: parameters is null\n");
    return -1;
  }

  ///////////////////////////////////////////////////////////////
  // Open and load file
  ///////////////////////////////////////////////////////////////
  char* buf = NULL;
  long file_sz = 0;
  if (read_text_file(filename, &buf, &file_sz) != 0) {
    fprintf(stderr, "ERRN: cannot load file '%s'\n", filename);
    // Free memory
    free(buf);
    buf = NULL;
    return -1;
  }
  ///////////////////////////////////////////////////////////////
  //
  ///////////////////////////////////////////////////////////////
  char pattern_delim_1[] = " ";
  char pattern_delimeters[] = "=;{}\n";
  char* saveptr = NULL;
  char* lexeme = strtok_r(buf, pattern_delimeters, &saveptr);
  ///////////////////////////////////////////////////////////////
  // Parse string
  ///////////////////////////////////////////////////////////////
  while (lexeme) {
    user_info_t* user_info = ctor_user_info_t(NULL, 0, NULL, 0, NULL, 0, NULL, 0);
    if (parse_group_01(lexeme, strlen(lexeme), pattern_delim_1, NULL, NULL, NULL, NULL, compose_user_info_t_to_object,
                       user_info) != 0) {
      fprintf(stderr, "ERRN: parse user info has been failed\n");
      // Free memory
      free_user_info_t(user_info);
      goto alarm_syntax;
    }
    // Add to list
    if (!list_push_back(user_info_t_lst, user_info)) {
      list_clear(user_info_t_lst, free_user_info_t);
      goto alarm_syntax;
    }
    ///////////////////////////////////////////////////////////////
    // Break lexeme
    ///////////////////////////////////////////////////////////////
    lexeme = strtok_r(NULL, pattern_delimeters, &saveptr);
  }

  // Free memory
  free(buf);
  buf = NULL;
  return 0;
// Syntax error
alarm_syntax:
  fprintf(stderr, "ERRN: syntax error\n");
  ///////////////////////////////////////////////////////////////
  // Free memory element
  ///////////////////////////////////////////////////////////////
  ///////////////////////////////////////////////////////////////
  // Free memory list
  ///////////////////////////////////////////////////////////////
  return -1;
}
// Parse format: username:password@address:port
ssize_t parse_login(char* buf, char* login, uint16_t login_sz, char* pass, uint16_t pass_sz, char* addr,
                    uint16_t addr_sz, char* port, uint16_t port_sz) {
  if (!buf || !login || !pass || !addr || !port) return -1;
  ///////////////////////////////////////////////////////////////
  //
  ///////////////////////////////////////////////////////////////
  char pattern_delimeters[] = ":@\n";
  char* saveptr = NULL;
  char* lexeme = strtok_r(buf, pattern_delimeters, &saveptr);
  uint16_t counter = 0;
  ///////////////////////////////////////////////////////////////
  // Parse string
  ///////////////////////////////////////////////////////////////
  while (lexeme) {
    // Position: username:password@address:port
    switch (++counter) {
      case 1:
        strncpy(login, lexeme, login_sz);
        break;
      case 2:
        strncpy(pass, lexeme, pass_sz);
        break;
      case 3:
        strncpy(addr, lexeme, addr_sz);
        break;
      case 4:
        strncpy(port, lexeme, port_sz);
        break;
    }
    ///////////////////////////////////////////////////////////////
    // Break lexeme
    ///////////////////////////////////////////////////////////////
    lexeme = strtok_r(NULL, pattern_delimeters, &saveptr);
  }

  return 0;
}
