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
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef __QNXNTO__
#include <linux/limits.h>  // for 'PATH_MAX'
#else
#include <limits.h>
#include <netdb.h>        // for getprotobyname
#include <netinet/tcp.h>  // for TCP_MAXSEG
#endif
#include <arpa/inet.h>
#include <dirent.h>
#include <errno.h>     // for 'errno'
#include <inttypes.h>  // for 'PRIu32'
#include <signal.h>    // for 'signal()'
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/stat.h>  // for 'stat'
#include <sys/types.h>
#include <sys/wait.h>  // for 'WIFEXITED'
#include <time.h>
//#include <math.h>       // for 'floor(), ceil()'

#include "container/list.h"
#include "logger/logger.h"
#include "protocol/generic.h"
#include "protocol/internal_types.h"
#include "protocol/parser.h"
#include "protocol/protocol_rxs_server.h"

#define USERNAME_SZ 255
#define PASSWORD_SZ 255

char path_output[PATH_MAX] = {0};
char file_users[PATH_MAX] = {0};  // Path to file 'rxs_users'
dlist_t* user_info_lst = NULL;
dlist_t* allow_addr_lst = NULL;
dlist_t* deny_addr_lst = NULL;
int sockfd_connect = -1;
int sockfd_data = -1;
uint8_t have_encoder = 0;
struct sockaddr_in client_addr;
uint32_t this_side_addr_n = 0;
uint8_t access_granted = 0;
// File handlers list
dlist_t* file_handlers_lst = NULL;

static float ceil_x(float f) {
  unsigned input;
  memcpy(&input, &f, sizeof(f));
  int exponent = ((input >> 23) & 255) - 127;
  if (exponent < 0) return (f > 0);
  // small numbers get rounded to 0 or 1, depending on their sign

  int fractional_bits = 23 - exponent;
  if (fractional_bits <= 0) return f;
  // numbers without fractional bits are mapped to themselves

  unsigned integral_mask = 0xffffffff << fractional_bits;
  unsigned output = input & integral_mask;
  // round the number down by masking out the fractional bits

  memcpy(&f, &output, sizeof(output));
  if (f > 0 && output != input) ++f;
  // positive numbers need to be rounded up, not down

  return f;
}
// Build unique name
static int build_unique_name(char* buf, size_t buf_size, char* lexem) {
  time_t rawtime;
  struct tm* info;
  char buffer[FILENAME_MAX] = {0};

  time(&rawtime);
  info = localtime(&rawtime);
  strftime(buffer, sizeof(buffer), "%Y%m%d_%I%M%S%p", info);
  snprintf(buf, buf_size, "%s%s", buffer, lexem);

  return 0;
}

size_t rxs_data_point_close() {
  if (get_socket_data() != -1) {
    close(get_socket_data());
    set_socket_data(-1);
    return 0;
  }
  return -1;
}

size_t rxs_data_point_create_server(uint16_t port_h) {
  struct sockaddr_in local;
  struct sockaddr_in data;

  int socketfd = socket(AF_INET, SOCK_STREAM, 0);
  if (socketfd < 0) {
    log_msg(ERRN, 6, "socket, data connection", strerror(errno));
    rxs_data_point_close();
    return -1;
  }
  memset(&local, 0, sizeof(local));
  local.sin_addr.s_addr = htonl(this_side_addr_n);
  local.sin_family = AF_INET;
  local.sin_port = htons(RXS_DATA_PORT);

  memset(&data, 0, sizeof(data));
  memcpy(&data, &client_addr, sizeof(client_addr));
  data.sin_family = AF_INET;
  data.sin_port = htons(port_h);

  if (set_socket_mode(socketfd, have_encoder) != 0) {
    rxs_data_point_close();
    return -1;
  }
  int reuseaddr = 1;
  if (setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, &reuseaddr, sizeof(reuseaddr)) < 0) {
    log_msg(ERRN, 59, "SO_REUSEADDR", strerror(errno));
    rxs_data_point_close();
    return -1;
  }
  if (bind(socketfd, (struct sockaddr*)&local, sizeof(local)) < 0) {
    log_msg(ERRN, 56, strerror(errno));
    rxs_data_point_close();
    return 1;
  }
  //////////////////////////////////////////////////////////////////////////////////
  // Connect request
  //////////////////////////////////////////////////////////////////////////////////
  if (connect(socketfd, (const struct sockaddr*)&data, (socklen_t)sizeof(data)) == -1) {
    // Set errno
    log_msg(ERRN, 6, "connect", strerror(errno));
    log_msg(ERRN, 9, inet_ntoa((struct in_addr)data.sin_addr), port_h);
    rxs_data_point_close();
    return -1;
  }
  //////////////////////////////////////////////////////////////////////////////////
  //
  //////////////////////////////////////////////////////////////////////////////////
  set_socket_data(socketfd);

  return 0;
}
//////////////////////////////////////////////////////////////////////////////////////////////////
// Various internal functions
//////////////////////////////////////////////////////////////////////////////////////////////////
// Convert netmask to network prefix length. e.g. '255.255.255.0'->'24'
static int inet_mask_len_n(int mask_n) {
  int i = 0;
  while (mask_n > 0) {
    mask_n = mask_n >> 1;
    i++;
  }
  return i;
}
// Get netmask prefix
static uint32_t get_network_prefix(uint32_t addr_n, uint32_t mask_n) {
  uint32_t shift = 32 - inet_mask_len_n(mask_n);
  return (addr_n << shift) >> shift;
}
// Is host address
static int is_host_address(uint32_t addr_n) {
  char addr_p[INET_ADDRSTRLEN] = {0};
  if (!inet_ntop(AF_INET, &addr_n, addr_p, sizeof(addr_p))) {
    log_msg(ERRN, 3, strerror(errno));
    return 0;
  }

  uint8_t last_byte = (addr_n >> 24);
  if ((addr_n == 0) || (last_byte == 0)) {
    log_msg(INFO, 24, addr_p);
    return 0;
  }
  return 1;
}
//////////////////////////////////////////////////////////////////////////////////////////////////
// Comparators
//////////////////////////////////////////////////////////////////////////////////////////////////
static int cmp_file_handlers_t_uint32_t(const void* data1, const void* data2) {
  if ((!data1) || (!data2)) {
    log_msg(ERRN, 14);
    return -1;
  }

  uint32_t val1 = ((file_handlers_t*)data1)->fhandle_key;
  uint32_t val2 = *((uint32_t*)data2);

  return (val1 == val2) ? 0 : ((val1 > val2) ? 1 : -1);
}
static int cmp_addr_t_uint32_t(const void* data1, const void* data2) {
  if (!data1 || !data2) {
    return -2;
  }

  uint32_t val1 = ((addr_t*)data1)->addr_n;
  uint32_t val2 = *((uint32_t*)data2);

  return (val1 == val2) ? 0 : ((val1 > val2) ? 1 : -1);
}
static int cmp_user_info_t_user_info_t(const void* data1, const void* data2) {
  if (!data1 || !data2) {
    return -2;
  }

  user_info_t* user1 = (user_info_t*)data1;
  user_info_t* user2 = (user_info_t*)data2;
  return (((memcmp(user1->name, user2->name, strlen(user2->name)) == 0) &&
           (strlen(user1->name) == strlen(user2->name)) &&
           (memcmp(user1->pass, user2->pass, strlen(user2->pass)) == 0) && (strlen(user1->pass) == strlen(user2->pass)))
              ? (0)
              : ((memcmp(user1->name, user2->name, strlen(user2->name)) > 0) ? 1 : -1));
}
//////////////////////////////////////////////////////////////////////////////////////////////////
// Internal functions
//////////////////////////////////////////////////////////////////////////////////////////////////
int set_socket_connected(int sockfd) { return (sockfd_connect = sockfd); }
int get_socket_connected() { return sockfd_connect; }
void set_client_addr(struct sockaddr_in* addr) {
  memset(&client_addr, 0, sizeof(client_addr));
  memcpy(&client_addr, addr, sizeof(client_addr));
}
int set_socket_data(int sockfd) { return (sockfd_data = sockfd); }
int get_socket_data() { return sockfd_data; }

// Close file handlers
ssize_t clear_file_handlers_lst() {
  // Close file handler and clear list
  list_clear(&file_handlers_lst, (void*)free_file_handlers_t);
  return 0;
}
//////////////////////////////////////////////////////////////////////////////////////////////////
// Policy functions
//////////////////////////////////////////////////////////////////////////////////////////////////
ssize_t set_encoder_mode(uint8_t encoder_mode) {
  have_encoder = encoder_mode;
  return 0;
}
ssize_t is_encoder_mode() { return (have_encoder) ? 1 : 0; }
ssize_t set_path_users(const char* path_users, size_t path_users_sz) {
  if (memcpy_x(file_users, sizeof(file_users), path_users, path_users_sz) != 0) {
    log_msg(ERRN, 25, "path");
    return -1;
  }
  return 0;
}
ssize_t set_user_info(const char* name, char name_sz, const char* pass, char pass_sz, char* group, char group_sz,
                      char* home_dir, char home_dir_sz) {
  if (!list_push_back(&user_info_lst,
                      ctor_user_info_t(name, name_sz, pass, pass_sz, group, group_sz, home_dir, home_dir_sz)))
    return -1;
  return 0;
}
ssize_t clear_user_info_lst() {
  list_clear(&user_info_lst, (void*)free_user_info_t);
  return 0;
}

ssize_t set_allow_address(uint32_t addr_n) {
  if (!list_push_back(&allow_addr_lst, ctor_addr_t(addr_n))) return -1;
  return 0;
}
ssize_t set_allow_address_lst(const dlist_t* addr_t_lst) {
  copy_addr_t_lst(addr_t_lst, &allow_addr_lst);
  return 0;
}
ssize_t is_allow_address(uint32_t addr_n) {
  if (!is_host_address(addr_n)) return 0;
  uint32_t network_addr_n = get_network_prefix(addr_n, inet_addr("255.255.255.0"));
  /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  // CAUTION: addresses are allowed for access, if the list is not specified or a concrete address/addresses is
  // specified
  /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  return ((list_empty(allow_addr_lst)) || (list_find(allow_addr_lst, &addr_n, cmp_addr_t_uint32_t)) ||
          (list_find(allow_addr_lst, &network_addr_n, cmp_addr_t_uint32_t)))
             ? 1
             : 0;
}
ssize_t clear_allow_address_lst() {
  list_clear(&allow_addr_lst, (void*)free_addr_t);
  return 0;
}
ssize_t set_deny_address(uint32_t addr_n) {
  if (!list_push_back(&deny_addr_lst, ctor_addr_t(addr_n))) return -1;
  return 0;
}
ssize_t set_deny_address_lst(const dlist_t* addr_t_lst) {
  copy_addr_t_lst(addr_t_lst, &deny_addr_lst);
  return 0;
}
ssize_t is_deny_address(uint32_t addr_n) { return (list_find(deny_addr_lst, &addr_n, cmp_addr_t_uint32_t)) ? 1 : 0; }
ssize_t clear_deny_address_lst() {
  list_clear(&deny_addr_lst, (void*)free_addr_t);
  return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// Handlers for commands
//////////////////////////////////////////////////////////////////////////////////////////////////
ssize_t rxs_handler_authorization(const uint8_t* user, uint32_t user_sz, const uint8_t* pass, uint32_t pass_sz,
                                  int* status, uint8_t encoder, uint32_t* err_no) {
  if (!user || !pass || !status || !err_no) return -1;
  ////////////////////////////////////////////////////////////////////////////
  // Print user & password
  ////////////////////////////////////////////////////////////////////////////
  char user_tmp[USERNAME_SZ + 1] = {0};
  char pass_tmp[PASSWORD_SZ + 1] = {0};
  if (memcpy_x(user_tmp, sizeof(user_tmp), user, user_sz) != 0) {
    log_msg(ERRN, 25, "name");
    *err_no = E2BIG;
    return -1;
  }
  if (memcpy_x(pass_tmp, sizeof(pass_tmp), pass, pass_sz) != 0) {
    log_msg(ERRN, 25, "pass");
    *err_no = E2BIG;
    return -1;
  }
  log_msg(INFO, 26, user_tmp, user_sz, pass_tmp, pass_sz);
  ////////////////////////////////////////////////////////////////////////////
  // CAUTION: Clear current user info list
  ////////////////////////////////////////////////////////////////////////////
  list_clear(&user_info_lst, free_user_info_t);
  ////////////////////////////////////////////////////////////////////////////
  // Read file user info
  ////////////////////////////////////////////////////////////////////////////
  dlist_t* user_info_t_lst = NULL;
  if (parse_user_info(file_users, &user_info_t_lst) != 0) {
    // Free memory
    list_clear(&user_info_t_lst, (void*)free_user_info_t);
    log_msg(ERRN, 4);
    *err_no = EIO;
    return -1;
  }
  ////////////////////////////////////////////////////////////////////////////
  // Add user info to list
  ////////////////////////////////////////////////////////////////////////////
  while (!list_empty(user_info_t_lst)) {
    dlist_t* node = list_front(user_info_t_lst);
    if (node) {
      user_info_t* user_info = (user_info_t*)node->data;
      if (set_user_info(user_info->name, strlen(user_info->name), user_info->pass, strlen(user_info->pass),
                        user_info->group, strlen(user_info->group), user_info->home_dir,
                        strlen(user_info->home_dir)) != 0) {
        // Free memory
        list_clear(&user_info_t_lst, (void*)free_user_info_t);
        log_msg(ERRN, 6, "set_user_info", "");
        *err_no = EIO;
        return -1;
      }
    }
    list_pop_front(&user_info_t_lst, (void*)free_user_info_t);
  }
  ////////////////////////////////////////////////////////////////////////////
  // Find user & password
  ////////////////////////////////////////////////////////////////////////////
  user_info_t* user_info =
      ctor_user_info_t((const char*)user, (char)user_sz, (const char*)pass, (char)pass_sz, NULL, 0, NULL, 0);
  if (!user_info) {
    *err_no = E2BIG;
    return -1;
  }
  dlist_t* node = list_find(user_info_lst, user_info, cmp_user_info_t_user_info_t);
  // Free memory
  free_user_info_t(user_info);
  if (node) {
    user_info_t* user_info = (user_info_t*)node->data;
    // Set default directory
    if (chdir(user_info->home_dir) == 0) {
      ////////////////////////////////////////////////////////////////////////////
      // Create tmp folder
      ////////////////////////////////////////////////////////////////////////////
      memcpy_x(path_output, sizeof(path_output), user_info->home_dir, strlen(user_info->home_dir));
      char folder_tmp[] = "/tmp/";
      memcpy_x(path_output + strlen(path_output), sizeof(path_output) - strlen(path_output), folder_tmp,
               strlen(folder_tmp));

      if (!rxs_handler_mkdir_ex((uint8_t*)&path_output[0], (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH), status, err_no)) {
        char file_output[PATH_MAX] = {0};
        build_unique_name(file_output, sizeof(file_output), "_output.dat");
        memcpy_x(path_output + strlen(path_output), sizeof(path_output) - strlen(path_output), file_output,
                 strlen(file_output));

        *status = 0;
        *err_no = 0;
        log_msg(INFO, 27, user_info->home_dir);
        have_encoder = encoder;

        if (is_encoder_mode())
          log_msg(INFO, 55, "rxsd", "encoded");
        else
          log_msg(INFO, 55, "rxsd", "plain");
        return 0;
      } else {
        // Folder are exist. It's Okay
        if ((EEXIST == *err_no) && (1 == rxs_handler_is_dir((uint8_t*)&path_output[0], status, err_no))) {
          char file_output[PATH_MAX] = {0};
          build_unique_name(file_output, sizeof(file_output), "_output.dat");
          memcpy_x(path_output + strlen(path_output), sizeof(path_output) - strlen(path_output), file_output,
                   strlen(file_output));

          *status = 0;
          *err_no = 0;
          log_msg(INFO, 27, user_info->home_dir);
          have_encoder = encoder;

          if (is_encoder_mode())
            log_msg(INFO, 55, "rxsd", "encoded");
          else
            log_msg(INFO, 55, "rxsd", "plain");
          return 0;
        }
        log_msg(ERRN, 28, user_info->home_dir);
        *err_no = errno;
        return -1;
      }
    } else {
      log_msg(ERRN, 28, user_info->home_dir);
      *err_no = errno;
      return -1;
    }
  } else {
    log_msg(ERRN, 29);
    *err_no = EACCES;
    return -1;
  }

  return -1;
}
ssize_t rxs_handler_ls(uint8_t* data, char* path_output, int* status, uint32_t* err_no) {
  if (!data || !path_output || !status || !err_no) return -1;

  char lexem_prefix[] = "bash -c ";
  char lexem_greater[] = " >";
  char lexem_comma[] = "'";
  size_t cmd_size = strlen(lexem_prefix) + strlen(lexem_comma) + strlen((const char*)data) + strlen(lexem_comma) +
                    strlen(lexem_greater) + strlen(path_output) + 1;
  char* cmd = (char*)calloc(cmd_size, sizeof(char));
  if (!cmd) {
    *err_no = errno;
    return -1;
  }
  // CAUTION: set permission for return status from child process
  signal(SIGCHLD, SIG_DFL);

  snprintf(cmd, cmd_size, "%s%s%s%s%s%s", lexem_prefix, lexem_comma, data, lexem_comma, lexem_greater, path_output);

  *status = system(cmd);
  *err_no = errno;
  // Free memory
  free(cmd);
  cmd = NULL;
  // NOTE: return code needed to analyze by next macros:
  // WIFEXITED(stat_val) -- Evaluates to a non-zero value if status was returned for a child process that terminated
  // normally. WEXITSTATUS(stat_val) -- If the value of WIFEXITED(stat_val) is non-zero, this macro evaluates to the
  // low-order 8 bits of the status argument that the child process passed to _exit() or exit(), or the value the
  // child process returned from main().
  if (WIFEXITED(*status)) return (*status = WEXITSTATUS(*status));

  return -1;
}

ssize_t rxs_handler_mkdir(uint8_t* data, uint32_t mode, int* status, uint32_t* err_no) {
  if (!data || !status || !err_no) return -1;

  *status = mkdir((char*)data, mode);
  *err_no = errno;
  return ((0 == *status) ? 0 : -1);
}
ssize_t rxs_handler_mkdir_ex(uint8_t* data, uint32_t mode, int* status, uint32_t* err_no) {
  if (!data || !status || !err_no) return -1;

  char separator = '/';
  char tmp[PATH_MAX] = {0};
  char* p = NULL;

  snprintf(tmp, sizeof(tmp), "%s", (char*)data);
  size_t len = strlen(tmp);

  if (tmp[len - 1] == separator) tmp[len - 1] = 0;
  for (p = tmp + 1; *p; p++)
    if (*p == separator) {
      *p = 0;
      *status = mkdir(tmp, mode);
      *p = separator;
    }
  *status = mkdir(tmp, mode);

  *err_no = errno;
  return ((0 == *status) ? 0 : -1);
}
ssize_t rxs_handler_rmdir(uint8_t* data, int* status, uint32_t* err_no) {
  if (!data || !status || !err_no) return -1;

  *status = rmdir((char*)data);
  *err_no = errno;
  return ((0 == *status) ? 0 : -1);
}
ssize_t rxs_handler_rmdir_ex(uint8_t* data, int* status, uint32_t* err_no) {
  struct dirent* entry = NULL;
  *status = chdir((char*)data);

  DIR* directory = opendir(".");
  while ((entry = readdir(directory)) != NULL) {
    if (strcmp(entry->d_name, ".") && strcmp(entry->d_name, "..")) *status = unlink(entry->d_name);  // maybe unlinkat ?
  }
  closedir(directory);
  *err_no = errno;
  return ((0 == *status) ? 0 : -1);
}
ssize_t rxs_handler_chdir(uint8_t* data, int* status, uint32_t* err_no) {
  if (!data || !status || !err_no) return -1;

  *status = chdir((char*)data);
  *err_no = errno;
  return ((0 == *status) ? 0 : -1);
}
ssize_t rxs_handler_unlink(uint8_t* data, int* status, uint32_t* err_no) {
  if (!data || !status || !err_no) return -1;

  *status = unlink((char*)data);
  *err_no = errno;
  return ((0 == *status) ? 0 : -1);
}

ssize_t rxs_handler_rename(uint8_t* data1, uint8_t* data2, int* status, uint32_t* err_no) {
  if (!data1 || !data2 || !status || !err_no) return -1;

  *status = rename((char*)data1, (char*)data2);
  *err_no = errno;
  return ((0 == *status) ? 0 : -1);
}
ssize_t rxs_handler_filesize(uint8_t* data, long* status, uint32_t* err_no) {
  if (!data || !status || !err_no) return -1;
  //////////////////////////////////////////////////////////////////////////////////
  // Get file size
  //////////////////////////////////////////////////////////////////////////////////
  struct stat st;
  int res = stat((char*)data, &st);

  *status = st.st_size;
  *err_no = errno;
  return (!res) ? 0 : -1;
}
ssize_t rxs_handler_fopen(uint8_t* data1, uint8_t* data2, uint32_t* fhandle_key, uint32_t* err_no) {
  if (!data1 || !data2 || !fhandle_key || !err_no) return -1;

  // Open file
  FILE* fhandle = fopen((char*)data1, (char*)data2);
  if (fhandle) {
    //////////////////////////////////////////////////////////////////////////////////
    // Associate file handler with value and put it into the map/list
    //////////////////////////////////////////////////////////////////////////////////
    char buf[FILENAME_MAX] = {0};
    snprintf(buf, sizeof(buf), "%p", fhandle);
    // NOTE: 32-bit address is look like as '0xFFFFFFFF'
    uint32_t offset = strlen("0x") + (sizeof(uint32_t) * 2);
    memset(buf + offset, 0, sizeof(buf) - offset);
    *fhandle_key = strtoul(buf, NULL, 16);

    file_handlers_t* file_handlers = new_file_handlers_t(1);
    if (!file_handlers) {
      log_msg(ERRN, 6, "new_file_handlers_t", "");
      // Close handler
      fclose(fhandle);
      return -1;
    }
    init_file_handlers_t(file_handlers, *fhandle_key, fhandle);

    list_push_back(&file_handlers_lst, file_handlers);

    return 0;
  } else {
    *err_no = errno;
    return -1;
  }
}
ssize_t rxs_handler_fread(uint32_t key, uint8_t** buf, uint32_t* len, uint32_t* err_no) {
  if (!buf || !err_no || !len) return -1;
  //////////////////////////////////////////////////////////////////////////////////
  // Looking FILE handler by address view into the map/list
  //////////////////////////////////////////////////////////////////////////////////
  size_t impl_bytes = 0;
  const dlist_t* node = list_cfind(file_handlers_lst, &key, cmp_file_handlers_t_uint32_t);
  if (node && key) {
    file_handlers_t* file_handlers = (file_handlers_t*)node->data;
    size_t buf_sz = ((have_encoder > 0) ? (crypt_packet_sz()) : (crypt_data_sz()));
    *buf = calloc(buf_sz, sizeof(uint8_t));
    if (!*buf) {
      log_msg(ERRN, 6, "calloc", strerror(errno));
      return -1;
    }
    impl_bytes = fread(*buf, sizeof(uint8_t), buf_sz, file_handlers->fhandle_val);
    *len = ((have_encoder > 0) ? ((impl_bytes) ? buf_sz : 0) : (impl_bytes));
    // Examine error
    *err_no = (uint32_t)ferror(file_handlers->fhandle_val);
    // No error
    if (0 == *err_no) {
      if (feof(file_handlers->fhandle_val) != 0) return RXS_EOF;
      return 0;
    } else {
      // Free memory
      free(*buf);
      *buf = NULL;
      return -1;
    }
  }
  // File not found
  else {
    *err_no = ENOENT;
    return -1;
  }
}
ssize_t rxs_handler_fwrite(uint32_t key, uint8_t* data, uint32_t len, uint32_t* err_no) {
  if (!data || !err_no) return -1;

  const dlist_t* node = list_cfind(file_handlers_lst, &key, cmp_file_handlers_t_uint32_t);
  if (node && key) {
    size_t data_sz = ((have_encoder > 0) ? (crypt_packet_sz()) : (len));
    file_handlers_t* file_handlers = (file_handlers_t*)node->data;
    size_t impl_bytes = fwrite(data, sizeof(uint8_t), data_sz, file_handlers->fhandle_val);
    if (impl_bytes != data_sz) log_msg(ERRN, 6, "fwrite", strerror(errno));
    *err_no = errno;
    return 0;
  } else {
    *err_no = ENOENT;
    return -1;
  }
}
ssize_t rxs_handler_fflush(uint32_t key, int* status, uint32_t* err_no) {
  if (!status || !err_no) return -1;

  const dlist_t* node = list_cfind(file_handlers_lst, &key, cmp_file_handlers_t_uint32_t);
  if (node && key) {
    file_handlers_t* file_handlers = (file_handlers_t*)node->data;
    *status = fflush(file_handlers->fhandle_val);
    *err_no = errno;
    return 0;
  } else {
    *err_no = ENOENT;
    return -1;
  }
}
ssize_t rxs_handler_fclose(uint32_t key, int* status, uint32_t* err_no) {
  if (!status || !err_no) return -1;

  const dlist_t* node = list_cfind(file_handlers_lst, &key, cmp_file_handlers_t_uint32_t);
  if (node && key) {
    file_handlers_t* file_handlers = (file_handlers_t*)node->data;
    *status = fclose(file_handlers->fhandle_val);
    *err_no = errno;
    // Reset values
    file_handlers->fhandle_val = NULL;
    // file_handlers->fhandle_key = 0;
    //////////////////////////////////////////////////////////////////////////////////
    // Delete FILE handler into the map/list
    //////////////////////////////////////////////////////////////////////////////////
    list_remove_if(&file_handlers_lst, &key, cmp_file_handlers_t_uint32_t, free_file_handlers_t);

    return 0;
  } else {
    *err_no = ENOENT;
    return -1;
  }
}
ssize_t rxs_handler_fseek(uint32_t key, uint32_t val2, uint32_t val3, int* status, uint32_t* err_no) {
  if (!status || !err_no) return -1;

  const dlist_t* node = list_cfind(file_handlers_lst, &key, cmp_file_handlers_t_uint32_t);
  if (node && key) {
    file_handlers_t* file_handlers = (file_handlers_t*)node->data;
    *status = fseek(file_handlers->fhandle_val, val2, val3);
    *err_no = errno;
    return 0;
  } else {
    *err_no = ENOENT;
    return -1;
  }
}
ssize_t rxs_handler_ftell(uint32_t key, long* status, uint32_t* err_no) {
  if (!status || !err_no) return -1;

  const dlist_t* node = list_cfind(file_handlers_lst, &key, cmp_file_handlers_t_uint32_t);
  if (node && key) {
    file_handlers_t* file_handlers = (file_handlers_t*)node->data;
    *status = ftell(file_handlers->fhandle_val);
    *err_no = errno;
    return 0;
  } else {
    *err_no = ENOENT;
    return -1;
  }
}
ssize_t rxs_handler_rewind(uint32_t key, int* status, uint32_t* err_no) {
  if (!status || !err_no) return -1;

  const dlist_t* node = list_cfind(file_handlers_lst, &key, cmp_file_handlers_t_uint32_t);
  if (node && key) {
    file_handlers_t* file_handlers = (file_handlers_t*)node->data;
    rewind(file_handlers->fhandle_val);

    *status = 0;
    *err_no = errno;

    return 0;
  } else {
    *err_no = ENOENT;
    return -1;
  }
}
ssize_t rxs_handler_is_file(uint8_t* data, int* status, uint32_t* err_no) {
  if (!data || !status || !err_no) return -1;

  struct stat st;
  *status = stat((char*)data, &st);
  *err_no = errno;
  if (0 == *status)
    return ((st.st_mode & S_IFMT) == S_IFREG) ? (1) : (0);
  else
    return -1;
}
ssize_t rxs_handler_is_dir(uint8_t* data, int* status, uint32_t* err_no) {
  if (!data || !status || !err_no) return -1;

  struct stat st;
  *status = stat((char*)data, &st);
  *err_no = errno;
  if (0 == *status)
    return ((st.st_mode & S_IFMT) == S_IFDIR) ? (1) : (0);
  else
    return -1;
}
//
ssize_t run_operation(rxs_operation_t operation, packet_rxs_t* packet_rxs_recv, packet_rxs_t* packet_rxs_send) {
  //////////////////////////////////////////////////////////////////////////////////
  // Check access granted
  //////////////////////////////////////////////////////////////////////////////////
  if ((0 == access_granted) && (operation_authorization != operation)) {
    log_msg(ERRN, 31, (size_t)operation);
    if (compose_packet_rxs_x00(SC_B1, operation, EACCES, packet_rxs_send) < 0) {
      log_msg(ERRN, 6, "compose_packet_rxs_x00", "");
      return -1;
    }
    return 0;
  }
  switch (operation) {
    case operation_authorization: {
      slot02_t slot02;
      init_slot02_t(&slot02);
      if (deserialize_slot02_t(packet_rxs_recv->data, (packet_rxs_recv->sz - hdr_packet_rxs_t_sz()), &slot02) < 0) {
        log_msg(ERRN, 6, "deserialize_slot02_t", "");
        // Free memory
        dinit_slot02_t(&slot02);
        return -1;
      }

      int status = -1;
      uint32_t err_no = 0;
      ssize_t result = rxs_handler_authorization(slot02.data1, slot02.data1_sz, slot02.data2, slot02.data2_sz, &status,
                                                 slot02.encoder, &err_no);
      log_msg(INFO, 32, "authorization", status);
      // Free memory
      dinit_slot02_t(&slot02);

      //////////////////////////////////////////////////////////////////////////////////
      // Set access granted
      //////////////////////////////////////////////////////////////////////////////////
      access_granted = (0 == result) ? (1) : (0);

      //////////////////////////////////////////////////////////////////////////////////
      // RESP
      //////////////////////////////////////////////////////////////////////////////////
      if (compose_packet_rxs_x00((!status) ? (SC_B0) : (SC_B1), operation, (!status) ? (status) : (err_no),
                                 packet_rxs_send) < 0) {
        log_msg(ERRN, 6, "compose_packet_rxs_x00", "");
        return -1;
      }
      return 0;
    }
    case operation_ls: {
      slot01_t slot01;
      init_slot01_t(&slot01);
      if (deserialize_slot01_t(packet_rxs_recv->data, (packet_rxs_recv->sz - hdr_packet_rxs_t_sz()), &slot01) < 0) {
        // log_msg(ERRN, 6, "deserialize_slot01_t", "");
        // Free memory
        dinit_slot01_t(&slot01);
        return -1;
      }

      int status = -1;
      uint32_t err_no = 0;
      ssize_t result = rxs_handler_ls(slot01.data, path_output, &status, &err_no);
      //////////////////////////////////////////////////////////////////////////////////
      // CAUTION: do not write the log to disk, because we can record a new image!
      //////////////////////////////////////////////////////////////////////////////////
      // log_msg(INFO, 32, "ls", status);
      // Free memory
      dinit_slot01_t(&slot01);
      if (-1 == result) {
      }
      //////////////////////////////////////////////////////////////////////////////////
      // ToDo: Compress regular file to tar format
      //////////////////////////////////////////////////////////////////////////////////

      //////////////////////////////////////////////////////////////////////////////////
      // In encrypted mode, need rewrite regular file to encrypted format
      //////////////////////////////////////////////////////////////////////////////////
      if (have_encoder > 0 && (file_size(path_output) > 0)) {
        if (write_encrypted_file(path_output, path_output, crypt_packet_sz()) < 0)
          status = EIO;  // Standard error maybe not set
      }
      //////////////////////////////////////////////////////////////////////////////////
      // RESP
      //////////////////////////////////////////////////////////////////////////////////
      if (status) {
        if (compose_packet_rxs_x00(SC_B1, operation, status, packet_rxs_send) < 0) return -1;
      } else {
        if (compose_packet_rxs_x01(SC_B0, operation, path_output, strlen(path_output), packet_rxs_send) < 0) return -1;
      }
      return 0;
    }
    case operation_mkdir: {
      slot03_t slot03;
      init_slot03_t(&slot03);
      if (deserialize_slot03_t(packet_rxs_recv->data, (packet_rxs_recv->sz - hdr_packet_rxs_t_sz()), &slot03) < 0) {
        log_msg(ERRN, 6, "deserialize_slot03_t", "");
        // Free memory
        dinit_slot03_t(&slot03);
        return -1;
      }

      int status = -1;
      uint32_t err_no = 0;
      ssize_t result = rxs_handler_mkdir(slot03.data, slot03.val, &status, &err_no);
      log_msg(INFO, 32, "mkdir", status);
      // Free memory
      dinit_slot03_t(&slot03);
      if (-1 == result) log_msg(ERRN, 6, "rxs_handler_mkdir", strerror(err_no));
      //////////////////////////////////////////////////////////////////////////////////
      // RESP
      //////////////////////////////////////////////////////////////////////////////////
      if (compose_packet_rxs_x00((!status) ? (SC_B0) : (SC_B1), operation, (!status) ? (status) : (err_no),
                                 packet_rxs_send) < 0) {
        log_msg(ERRN, 6, "compose_packet_rxs_x00", "");
        return -1;
      }

      return 0;
    }
    case operation_mkdir_ex: {
      slot03_t slot03;
      init_slot03_t(&slot03);
      if (deserialize_slot03_t(packet_rxs_recv->data, (packet_rxs_recv->sz - hdr_packet_rxs_t_sz()), &slot03) < 0) {
        log_msg(ERRN, 6, "deserialize_slot03_t", "");
        // Free memory
        dinit_slot03_t(&slot03);
        return -1;
      }

      int status = -1;
      uint32_t err_no = 0;
      ssize_t result = rxs_handler_mkdir_ex(slot03.data, slot03.val, &status, &err_no);
      log_msg(INFO, 32, "mkdir_ex", status);
      // Free memory
      dinit_slot03_t(&slot03);
      if (-1 == result) log_msg(ERRN, 6, "rxs_handler_mkdir_ex", strerror(err_no));
      //////////////////////////////////////////////////////////////////////////////////
      // RESP
      //////////////////////////////////////////////////////////////////////////////////
      if (compose_packet_rxs_x00((!status) ? (SC_B0) : (SC_B1), operation, (!status) ? (status) : (err_no),
                                 packet_rxs_send) < 0) {
        log_msg(ERRN, 6, "compose_packet_rxs_x00", "");
        return -1;
      }

      return 0;
    }
    case operation_rmdir: {
      slot01_t slot01;
      init_slot01_t(&slot01);
      if (deserialize_slot01_t(packet_rxs_recv->data, (packet_rxs_recv->sz - hdr_packet_rxs_t_sz()), &slot01) < 0) {
        log_msg(ERRN, 6, "deserialize_slot01_t", "");
        // Free memory
        dinit_slot01_t(&slot01);
        return -1;
      }

      int status = -1;
      uint32_t err_no = 0;
      ssize_t result = rxs_handler_rmdir(slot01.data, &status, &err_no);
      log_msg(INFO, 32, "rmdir", status);
      // Free memory
      dinit_slot01_t(&slot01);
      if (-1 == result) log_msg(ERRN, 6, "rxs_handler_rmdir", strerror(err_no));
      //////////////////////////////////////////////////////////////////////////////////
      // RESP
      //////////////////////////////////////////////////////////////////////////////////
      if (compose_packet_rxs_x00((!status) ? (SC_B0) : (SC_B1), operation, (!status) ? (status) : (err_no),
                                 packet_rxs_send) < 0) {
        log_msg(ERRN, 6, "compose_packet_rxs_x00", "");
        return -1;
      }
      return 0;
    }
    case operation_getcwd: {
      slot00_t slot00;
      init_slot00_t(&slot00);
      if (deserialize_slot00_t(packet_rxs_recv->data, (packet_rxs_recv->sz - hdr_packet_rxs_t_sz()), &slot00) < 0) {
        log_msg(ERRN, 6, "deserialize_slot00_t", "");
        // Free memory
        dinit_slot00_t(&slot00);
        return -1;
      }

      //
      char buf[slot00.val + 1];
      memset(buf, 0, sizeof(buf));
      char* status = getcwd(buf, sizeof(buf));
      log_msg(INFO, 33, "getcwd", status);
      // Free memory
      dinit_slot00_t(&slot00);
      //////////////////////////////////////////////////////////////////////////////////
      // RESP
      //////////////////////////////////////////////////////////////////////////////////
      if (!status) {
        if (compose_packet_rxs_x00(SC_B1, operation, errno, packet_rxs_send) < 0) {
          log_msg(ERRN, 6, "compose_packet_rxs_x00", "");
          return -1;
        }
      } else {
        if (compose_packet_rxs_x01(SC_B0, operation, buf, strlen(buf), packet_rxs_send) < 0) {
          return -1;
        }
      }
      return 0;
    }
    case operation_chdir: {
      slot01_t slot01;
      init_slot01_t(&slot01);
      if (deserialize_slot01_t(packet_rxs_recv->data, (packet_rxs_recv->sz - hdr_packet_rxs_t_sz()), &slot01) < 0) {
        log_msg(ERRN, 6, "deserialize_slot01_t", "");
        // Free memory
        dinit_slot01_t(&slot01);
        return -1;
      }

      int status = -1;
      uint32_t err_no = 0;
      ssize_t result = rxs_handler_chdir(slot01.data, &status, &err_no);
      log_msg(INFO, 32, "chdir", status);
      // Free memory
      dinit_slot01_t(&slot01);
      if (-1 == result) log_msg(ERRN, 6, "rxs_handler_chdir", strerror(err_no));
      //////////////////////////////////////////////////////////////////////////////////
      // RESP
      //////////////////////////////////////////////////////////////////////////////////
      if (compose_packet_rxs_x00((!status) ? (SC_B0) : (SC_B1), operation, (!status) ? (status) : (err_no),
                                 packet_rxs_send) < 0) {
        log_msg(ERRN, 6, "compose_packet_rxs_x00", "");
        return -1;
      }

      return 0;
    }
    case operation_unlink: {
      slot01_t slot01;
      init_slot01_t(&slot01);
      if (deserialize_slot01_t(packet_rxs_recv->data, (packet_rxs_recv->sz - hdr_packet_rxs_t_sz()), &slot01) < 0) {
        log_msg(ERRN, 6, "deserialize_slot01_t", "");
        // Free memory
        dinit_slot01_t(&slot01);
        return -1;
      }

      int status = -1;
      uint32_t err_no = 0;
      ssize_t result = rxs_handler_unlink(slot01.data, &status, &err_no);
      log_msg(INFO, 32, "unlink", status);
      // Free memory
      dinit_slot01_t(&slot01);
      if (-1 == result) log_msg(ERRN, 6, "rxs_handler_unlink", strerror(err_no));
      //////////////////////////////////////////////////////////////////////////////////
      // RESP
      //////////////////////////////////////////////////////////////////////////////////
      if (compose_packet_rxs_x00((!status) ? (SC_B0) : (SC_B1), operation, (!status) ? (status) : (err_no),
                                 packet_rxs_send) < 0) {
        log_msg(ERRN, 6, "compose_packet_rxs_x00", "");
        return -1;
      }

      return 0;
    }
    case operation_rename: {
      slot02_t slot02;
      init_slot02_t(&slot02);
      if (deserialize_slot02_t(packet_rxs_recv->data, (packet_rxs_recv->sz - hdr_packet_rxs_t_sz()), &slot02) < 0) {
        log_msg(ERRN, 6, "deserialize_slot02_t", "");
        // Free memory
        dinit_slot02_t(&slot02);
        return -1;
      }

      int status = -1;
      uint32_t err_no = 0;
      ssize_t result = rxs_handler_rename(slot02.data1, slot02.data2, &status, &err_no);
      log_msg(INFO, 32, "rename", status);
      // Free memory
      dinit_slot02_t(&slot02);
      if (-1 == result) log_msg(ERRN, 6, "rxs_handler_rename", strerror(err_no));
      //////////////////////////////////////////////////////////////////////////////////
      // RESP
      //////////////////////////////////////////////////////////////////////////////////
      if (compose_packet_rxs_x00((!status) ? (SC_B0) : (SC_B1), operation, (!status) ? (status) : (err_no),
                                 packet_rxs_send) < 0) {
        log_msg(ERRN, 6, "compose_packet_rxs_x00", "");
        return -1;
      }

      return 0;
    }
    case operation_filesize: {
      slot01_t slot01;
      init_slot01_t(&slot01);
      if (deserialize_slot01_t(packet_rxs_recv->data, (packet_rxs_recv->sz - hdr_packet_rxs_t_sz()), &slot01) < 0) {
        log_msg(ERRN, 6, "deserialize_slot01_t", "");
        // Free memory
        dinit_slot01_t(&slot01);
        return -1;
      }

      long status = -1;
      uint32_t err_no = 0;
      ssize_t result = rxs_handler_filesize(slot01.data, &status, &err_no);
      log_msg(INFO, 32, "filesize", status);
      // Free memory
      dinit_slot01_t(&slot01);
      if (-1 == result) log_msg(ERRN, 6, "rxs_handler_filesize", strerror(err_no));
      //////////////////////////////////////////////////////////////////////////////////
      // RESP
      //////////////////////////////////////////////////////////////////////////////////
      if (compose_packet_rxs_x00((status >= 0) ? (SC_B0) : (SC_B1), operation, (status >= 0) ? (status) : (err_no),
                                 packet_rxs_send) < 0) {
        log_msg(ERRN, 6, "compose_packet_rxs_x00", "");
        return -1;
      }

      return 0;
    }
    case operation_port: {
      slot05_t slot05;
      uint16_t port = 0;
      init_slot05_t(&slot05);
      if (deserialize_slot05_t(packet_rxs_recv->data, (packet_rxs_recv->sz - hdr_packet_rxs_t_sz()), &slot05) < 0) {
        log_msg(ERRN, 6, "deserialize_slot05_t", "");
        // Free memory
        dinit_slot05_t(&slot05);
        return -1;
      }
      port = ntohs(slot05.port);
      ssize_t status = rxs_data_point_create_server(port);
      //////////////////////////////////////////////////////////////////////////////////
      // RESP
      //////////////////////////////////////////////////////////////////////////////////
      if (compose_packet_rxs_x00((!status) ? (SC_B0) : (SC_B1), operation, status, packet_rxs_send) < 0) {
        log_msg(ERRN, 6, "compose_packet_rxs_x00", "");
        return -1;
      }
      return 0;
    }
    case operation_fopen: {
      slot02_t slot02;
      init_slot02_t(&slot02);
      if (deserialize_slot02_t(packet_rxs_recv->data, (packet_rxs_recv->sz - hdr_packet_rxs_t_sz()), &slot02) < 0) {
        log_msg(ERRN, 6, "deserialize_slot02_t", "");
        // Free memory
        dinit_slot02_t(&slot02);
        return -1;
      }

      uint32_t fhandle_key = 0;
      uint32_t err_no = 0;
      ssize_t status = rxs_handler_fopen(slot02.data1, slot02.data2, &fhandle_key, &err_no);
      log_msg(INFO, 34, "fopen", fhandle_key);
      // Free memory
      dinit_slot02_t(&slot02);
      if (-1 == status) log_msg(ERRN, 6, "rxs_handler_fopen", strerror(err_no));
      //////////////////////////////////////////////////////////////////////////////////
      // RESP
      //////////////////////////////////////////////////////////////////////////////////
      if (compose_packet_rxs_x00((fhandle_key) ? (SC_B0) : (SC_B1), operation, (fhandle_key) ? (fhandle_key) : err_no,
                                 packet_rxs_send) < 0) {
        log_msg(ERRN, 6, "compose_packet_rxs_x00", "");
        return -1;
      }
      return 0;
    }
    case operation_fread: {
      slot04_t slot04;
      init_slot04_t(&slot04);
      if (deserialize_slot04_t(packet_rxs_recv->data, (packet_rxs_recv->sz - hdr_packet_rxs_t_sz()), &slot04) < 0) {
        log_msg(ERRN, 6, "deserialize_slot04_t", "");
        // Free memory
        dinit_slot04_t(&slot04);
        return -1;
      }
      uint32_t buf_sz = slot04.data_sz;
      uint32_t stream = slot04.val1;
      dinit_slot04_t(&slot04);

      uint32_t read_data_bytes = 0;
      size_t block_sz = ((have_encoder > 0) ? (crypt_packet_sz()) : (crypt_data_sz()));

      size_t total_impl_sz = 0;
      size_t total_impl_channel_sz = 0;
      while (total_impl_channel_sz < buf_sz) {
        // Check total size
        if ((buf_sz - total_impl_channel_sz) < block_sz) break;

        uint8_t* data = NULL;
        uint32_t err_no = 0;
        ssize_t result = rxs_handler_fread(stream, &data, &read_data_bytes, &err_no);
        if ((0 == result) || (RXS_EOF == result)) {
          ssize_t impl_bytes = rxs_send_x(get_socket_data(), data, read_data_bytes);
          if (data) {
            free(data);
            data = NULL;
          }
          if (impl_bytes < 0) {
            log_msg(ERRN, 6, "rxs_send_x", strerror(errno));
            rxs_data_point_close();
            return -1;
          }
          total_impl_channel_sz += (size_t)impl_bytes;
          total_impl_sz += read_data_bytes;

          if (RXS_EOF == result) {
            if (rxs_send_packet_x04(get_socket_connected(), SC_B0, operation_fread, stream, total_impl_channel_sz,
                                    RXS_EOF) < 0) {
              log_msg(ERRN, 6, "rxs_send_packet_x04", strerror(errno));
              rxs_data_point_close();
              return -1;
            }
            //////////////////////////////////////////////////////////////////////////////////
            // Wait confirm to other side for to close operation 'fread'
            //////////////////////////////////////////////////////////////////////////////////
            rxs_type_t type;
            uint32_t other_side_stream = 0;
            uint32_t other_side_data_sz = 0;
            uint16_t other_side_eof = 0;
            if (rxs_recv_packet_x04(get_socket_connected(), &type, operation_fread, &other_side_stream,
                                    &other_side_data_sz, &other_side_eof) == 0) {
              if ((type == CS_A0) || (stream == other_side_stream)) {
                return 0;
              } else {
                rxs_data_point_close();
                return -1;
              }
            }
          }
        } else {
          log_msg(ERRN, 6, "rxs_handler_fread", strerror(errno));
          if (data) {
            free(data);
            data = NULL;
          }
          rxs_data_point_close();
          return -1;
        }
      }
      //////////////////////////////////////////////////////////////////////////////////
      // Send confirm to other side
      //////////////////////////////////////////////////////////////////////////////////
      rxs_send_packet_x04(get_socket_connected(), SC_B0, operation_fread, stream, total_impl_channel_sz, 0);
      // printf("DBG: ALL buf:%d | ch:%d tot:%d \n", buf_sz, total_impl_channel_sz, total_impl_sz);
      return 0;
    }
    case operation_fwrite: {
      slot04_t slot04;
      init_slot04_t(&slot04);
      if (deserialize_slot04_t(packet_rxs_recv->data, (packet_rxs_recv->sz - hdr_packet_rxs_t_sz()), &slot04) < 0) {
        log_msg(ERRN, 6, "deserialize_slot04_t", "");
        // Free memory
        dinit_slot04_t(&slot04);
        return -1;
      }
      uint32_t data_sz = slot04.data_sz;
      uint32_t stream = slot04.val1;
      dinit_slot04_t(&slot04);

      float data_only_sz = data_sz;
      uint32_t number_block = 0;
      uint32_t number_block_total = (uint32_t)ceil_x(data_only_sz / MAX_PORTION_DATA_BYTES);

      size_t buf_sz = ((have_encoder > 0) ? (crypt_packet_sz()) : (crypt_data_sz()));
      uint8_t* recv_buf = calloc(buf_sz, sizeof(uint8_t));
      if (!recv_buf) {
        log_msg(ERRN, 6, "calloc", strerror(errno));
        rxs_data_point_close();
        rxs_send_packet_x04(get_socket_connected(), SC_B1, operation_fwrite, stream, 0, 0);
        return -1;
      }

      size_t total_impl_bytes = 0;
      uint8_t is_full = 0;
      while (!is_full) {
        // memset(recv_buf, 0, buf_sz);
        errno = 0;
        ssize_t impl_bytes = rxs_recv_x(get_socket_data(), recv_buf, buf_sz);
        if ((impl_bytes < 0)) {
          log_msg(ERRN, 6, "rxs_recv_x", strerror(errno));
          // Free memory
          free(recv_buf);
          recv_buf = NULL;
          rxs_data_point_close();
          rxs_send_packet_x04(get_socket_connected(), SC_B1, operation_fwrite, stream, 0, 0);
          return -1;
        }
        total_impl_bytes += (size_t)impl_bytes;
        ++number_block;
        // Success
        uint32_t err_no;
        if (rxs_handler_fwrite(stream, recv_buf, (size_t)impl_bytes, &err_no) < 0) {
          // Free memory
          free(recv_buf);
          recv_buf = NULL;
          rxs_data_point_close();
          rxs_send_packet_x04(get_socket_connected(), SC_B1, operation_fwrite, stream, (uint32_t)impl_bytes, 0);
          return -1;
        }
        // It's end of whole data
        if ((!have_encoder && (total_impl_bytes == data_sz)) || (have_encoder && (number_block == number_block_total)))
          is_full = 1;
      }
      free(recv_buf);
      recv_buf = NULL;
      //////////////////////////////////////////////////////////////////////////////////
      // Send confirm to other side
      //////////////////////////////////////////////////////////////////////////////////
      rxs_send_packet_x04(get_socket_connected(), SC_B0, operation_fwrite, stream, data_sz, 0);
      return 0;
    }
    case operation_fflush: {
      slot00_t slot00;
      init_slot00_t(&slot00);
      if (deserialize_slot00_t(packet_rxs_recv->data, (packet_rxs_recv->sz - hdr_packet_rxs_t_sz()), &slot00) < 0) {
        log_msg(ERRN, 6, "deserialize_slot00_t", "");
        // Free memory
        dinit_slot00_t(&slot00);
        return -1;
      }
      int status = -1;
      uint32_t err_no = 0;
      status = rxs_handler_fflush(slot00.val, &status, &err_no);
      // log_msg(INFO, 32, "fflush", status);
      if (-1 == status) log_msg(ERRN, 6, "rxs_handler_fflush", strerror(err_no));
      // Free memory
      dinit_slot00_t(&slot00);
      //////////////////////////////////////////////////////////////////////////////////
      // RESP
      //////////////////////////////////////////////////////////////////////////////////
      if (compose_packet_rxs_x00((!status) ? (SC_B0) : (SC_B1), operation, (!status) ? (status) : (err_no),
                                 packet_rxs_send) < 0) {
        log_msg(ERRN, 6, "compose_packet_rxs_x00", "");
        return -1;
      }
      return 0;
    }
    case operation_fclose: {
      slot00_t slot00;
      init_slot00_t(&slot00);
      if (deserialize_slot00_t(packet_rxs_recv->data, (packet_rxs_recv->sz - hdr_packet_rxs_t_sz()), &slot00) < 0) {
        log_msg(ERRN, 6, "deserialize_slot00_t", "");
        // Free memory
        dinit_slot00_t(&slot00);
        return -1;
      }
      int status = -1;
      uint32_t err_no = 0;
      status = rxs_handler_fclose(slot00.val, &status, &err_no);
      log_msg(INFO, 34, "fclose", slot00.val);
      if (-1 == status) log_msg(ERRN, 6, "operation_fclose", strerror(err_no));
      // Free memory
      dinit_slot00_t(&slot00);
      //
      rxs_data_point_close();
      //////////////////////////////////////////////////////////////////////////////////
      // RESP
      //////////////////////////////////////////////////////////////////////////////////
      if (compose_packet_rxs_x00((!status) ? (SC_B0) : (SC_B1), operation, (!status) ? (status) : (err_no),
                                 packet_rxs_send) < 0) {
        log_msg(ERRN, 6, "compose_packet_rxs_x00", "");
        return -1;
      }

      return 0;
    }
    /*    case operation_fseek: {
            slot04_t slot04;
            init_slot04_t(&slot04);
            if(deserialize_slot04_t(packet_rxs_recv->data, (packet_rxs_recv->sz - hdr_packet_rxs_t_sz()),
       &slot04)<0) { log_msg(ERRN, 6, "deserialize_slot04_t", "");
                // Free memory
                dinit_slot04_t(&slot04);
                return -1;
            }

            int status = -1;
            uint32_t err_no = 0;
            ssize_t result = -1;
            result = rxs_handler_fseek(slot04.val1, slot04.val2, slot04.val3, &status, &err_no);
            log_msg(INFO, 36, "fseek", slot04.val1, status);
            // Free memory
            dinit_slot04_t(&slot04);
            if(-1 == result) {

            }
            //////////////////////////////////////////////////////////////////////////////////
            // RESP
            //////////////////////////////////////////////////////////////////////////////////
            if(compose_packet_rxs_x00((!status)?(SC_B0):(SC_B1), operation, (!status)?(status):(err_no),
       packet_rxs_send)<0) { log_msg(ERRN, 6, "compose_packet_rxs_x00", ""); return -1;
            }

            return 0;
        }
    */
    case operation_ftell: {
      slot00_t slot00;
      init_slot00_t(&slot00);
      if (deserialize_slot00_t(packet_rxs_recv->data, (packet_rxs_recv->sz - hdr_packet_rxs_t_sz()), &slot00) < 0) {
        log_msg(ERRN, 6, "deserialize_slot00_t", "");
        // Free memory
        dinit_slot00_t(&slot00);
        return -1;
      }

      long status = -1;
      uint32_t err_no = 0;
      ssize_t result = -1;
      result = rxs_handler_ftell(slot00.val, &status, &err_no);
      log_msg(INFO, 37, "ftell", slot00.val, status);
      // Free memory
      dinit_slot00_t(&slot00);
      if (-1 == result) {
      }
      //////////////////////////////////////////////////////////////////////////////////
      // RESP
      //////////////////////////////////////////////////////////////////////////////////
      if (compose_packet_rxs_x00((status >= 0) ? (SC_B0) : (SC_B1), operation, (status >= 0) ? (status) : (err_no),
                                 packet_rxs_send) < 0) {
        log_msg(ERRN, 6, "compose_packet_rxs_x00", "");
        return -1;
      }

      return 0;
    }
    case operation_rewind: {
      slot00_t slot00;
      init_slot00_t(&slot00);
      if (deserialize_slot00_t(packet_rxs_recv->data, (packet_rxs_recv->sz - hdr_packet_rxs_t_sz()), &slot00) < 0) {
        log_msg(ERRN, 6, "deserialize_slot00_t", "");
        // Free memory
        dinit_slot00_t(&slot00);
        return -1;
      }
      int status = -1;
      uint32_t err_no = 0;
      ssize_t result = -1;
      result = rxs_handler_rewind(slot00.val, &status, &err_no);
      log_msg(INFO, 36, "rewind", slot00.val, status);
      // Free memory
      dinit_slot00_t(&slot00);
      if (-1 == result) {
      }
      //////////////////////////////////////////////////////////////////////////////////
      // RESP
      //////////////////////////////////////////////////////////////////////////////////
      if (compose_packet_rxs_x00((!status) ? (SC_B0) : (SC_B1), operation, (!status) ? (status) : (err_no),
                                 packet_rxs_send) < 0) {
        log_msg(ERRN, 6, "compose_packet_rxs_x00", "");
        return -1;
      }

      return 0;
    }
    case operation_file_exist: {
      slot01_t slot01;
      init_slot01_t(&slot01);
      if (deserialize_slot01_t(packet_rxs_recv->data, (packet_rxs_recv->sz - hdr_packet_rxs_t_sz()), &slot01) < 0) {
        log_msg(ERRN, 6, "deserialize_slot01_t", "");
        // Free memory
        dinit_slot01_t(&slot01);
        return -1;
      }

      int status = -1;
      uint32_t err_no = 0;
      ssize_t result = -1;
      result = rxs_handler_is_file(slot01.data, &status, &err_no);
      log_msg(INFO, 32, "file_exist", status);
      // Free memory
      dinit_slot01_t(&slot01);
      if (-1 == result) {
      }
      //////////////////////////////////////////////////////////////////////////////////
      // RESP
      //////////////////////////////////////////////////////////////////////////////////
      if (compose_packet_rxs_x00((!status) ? (SC_B0) : (SC_B1), operation, (!status) ? (result) : (err_no),
                                 packet_rxs_send) < 0) {
        log_msg(ERRN, 6, "compose_packet_rxs_x00", "");
        return -1;
      }

      return 0;
    }
    case operation_dir_exist: {
      slot01_t slot01;
      init_slot01_t(&slot01);
      if (deserialize_slot01_t(packet_rxs_recv->data, (packet_rxs_recv->sz - hdr_packet_rxs_t_sz()), &slot01) < 0) {
        log_msg(ERRN, 6, "deserialize_slot01_t", "");
        // Free memory
        dinit_slot01_t(&slot01);
        return -1;
      }

      int status = -1;
      uint32_t err_no = 0;
      ssize_t result = -1;
      result = rxs_handler_is_dir(slot01.data, &status, &err_no);
      log_msg(INFO, 32, "dir_exist", status);
      // Free memory
      dinit_slot01_t(&slot01);
      if (-1 == result) {
      }
      //////////////////////////////////////////////////////////////////////////////////
      // RESP
      //////////////////////////////////////////////////////////////////////////////////
      if (compose_packet_rxs_x00((!status) ? (SC_B0) : (SC_B1), operation, (!status) ? (result) : (err_no),
                                 packet_rxs_send) < 0) {
        log_msg(ERRN, 6, "compose_packet_rxs_x00", "");
        return -1;
      }

      return 0;
    }
    default: {
      log_msg(ERRN, 38, operation);
    }
  }
  //////////////////////////////////////////////////////////////////////////////////
  // RESP by default
  //////////////////////////////////////////////////////////////////////////////////
  if (compose_packet_rxs_x00(SC_B1, operation, 0, packet_rxs_send) < 0) {
    log_msg(ERRN, 6, "compose_packet_rxs_x00", "");
    return -1;
  }
  return 0;
}
