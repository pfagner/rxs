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
#define _FILE_OFFSET_BITS 64
#include <arpa/inet.h>  // for 'INET_ADDRSTRLEN'
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>  // for 'strncasecmp'
#ifndef __QNXNTO__
#include <linux/limits.h>  // for 'PATH_MAX'
#else
#include <limits.h>
#endif
#include <errno.h>   // for 'errno'
#include <signal.h>  // for 'cntl+C'

#include "logger/logger.h"
#include "protocol/generic.h"
#include "protocol/parser.h"
#include "protocol/protocol_rxs_client.h"
#include "protocol/rxs_errno.h"
#include "protocol/version.h"

volatile int is_exit = 0;
void handler_cntl_c(int val) {
  (void)val;
  log_msg(STATUS, 39);
  is_exit = 1;
  // Close logger
  closelog();
  exit(EXIT_SUCCESS);
}
int compose_prefix_cli(const char* addr_p, const char* port_p, const char* login) {
  char pwd[PATH_MAX] = {0};
  rxs_getcwd(pwd, sizeof(pwd));
  char prefix_cli[PATH_MAX * 2] = {0};
  // Output remote console
  snprintf(prefix_cli, sizeof(prefix_cli), "%s:%s@%s:%s$ ", addr_p, port_p, login, pwd);
  fprintf(stdout, "%s", prefix_cli);
  fflush(stdout);
  return 0;
}
// Clear output console
int clear_console() {
  fprintf(stdout, "                                                                               ");
  fprintf(stdout, "\r");
  fflush(stdout);
  return 0;
}
int show_help() {
  fprintf(stdout,
          "Sorry, try again.\n\
Help: use these commands in next format:\n\
 *PUT file to other side:\n\
   $/usr/sbin/rxsc put username:password@address:port ./local_file ./remote_file\n\
 *GET file from other side:\n\
   $/usr/sbin/rxsc get username:password@address:port ./local_file ./remote_file\n\
 *CLI to remote terminal:\n\
   $/usr/sbin/rxsc cli username:password@address:port\n\
 RXS rev.%s\n",
          git_version);
  return 0;
}
// Main
int main(int argc, char* argv[]) {
  const char* operation = NULL;
  const char* file_local = NULL;
  const char* file_remote = NULL;

  char other_side_addr_p[INET_ADDRSTRLEN] = {0};  // Address on other side
  char other_side_port_p[INET_ADDRSTRLEN] = {0};  // Port on other side
  char login[FILENAME_MAX] = {0};                 // Login
  char pass[FILENAME_MAX] = {0};                  // Password

  // Read arguments
  char operation_put[] = "put";
  char operation_get[] = "get";
  char operation_cli[] = "cli";
  char operation_put_e[] = "put_e";
  char operation_get_e[] = "get_e";
  char operation_cli_e[] = "cli_e";

  switch (argc) {
    case 3: {
      // CLI or CLI_E username:password@ip:port
      if ((!strncasecmp(argv[1], operation_cli, strlen(operation_cli))) ||
          (!strncasecmp(argv[1], operation_cli_e, strlen(operation_cli_e)))) {
        operation = argv[1];
        parse_login(argv[2], login, sizeof(login), pass, sizeof(pass), other_side_addr_p, sizeof(other_side_addr_p),
                    other_side_port_p, sizeof(other_side_port_p));
        break;
      }
      show_help();
      exit(RXS_CLI_EINVAL);
    }
    case 6: {
      // CLI or CLI_E: old format
      if ((!strncasecmp(argv[1], operation_cli, strlen(operation_cli))) ||
          (!strncasecmp(argv[1], operation_cli_e, strlen(operation_cli_e)))) {
        operation = argv[1];
        strncpy(other_side_addr_p, argv[2], sizeof(other_side_addr_p));  // Address on other side
        strncpy(other_side_port_p, argv[3], sizeof(other_side_port_p));  // Port on other side
        strncpy(login, argv[4], sizeof(login));
        strncpy(pass, argv[5], sizeof(pass));
        break;
      }
      show_help();
      exit(RXS_CLI_EINVAL);
    }
    case 5: {
      // (PUT or GET) or (PUT_E or GET_E): username:password@ip:port
      if ((!strncasecmp(argv[1], operation_put, strlen(operation_put))) ||
          (!strncasecmp(argv[1], operation_put_e, strlen(operation_put_e))) ||
          (!strncasecmp(argv[1], operation_get, strlen(operation_get))) ||
          (!strncasecmp(argv[1], operation_get_e, strlen(operation_get_e)))) {
        operation = argv[1];
        parse_login(argv[2], login, sizeof(login), pass, sizeof(pass), other_side_addr_p, sizeof(other_side_addr_p),
                    other_side_port_p, sizeof(other_side_port_p));
        file_local = argv[3];
        file_remote = argv[4];
        break;
      }
      show_help();
      exit(RXS_CLI_EINVAL);
    }
    case 8: {
      // (PUT or GET) or (PUT_E or GET_E): old format
      if ((!strncasecmp(argv[1], operation_put, strlen(operation_put))) ||
          (!strncasecmp(argv[1], operation_put_e, strlen(operation_put_e))) ||
          (!strncasecmp(argv[1], operation_get, strlen(operation_get))) ||
          (!strncasecmp(argv[1], operation_get_e, strlen(operation_get_e)))) {
        operation = argv[1];
        file_local = argv[2];
        file_remote = argv[3];
        strncpy(other_side_addr_p, argv[4], sizeof(other_side_addr_p));  // Address on other side
        strncpy(other_side_port_p, argv[5], sizeof(other_side_port_p));  // Port on other side
        strncpy(login, argv[6], sizeof(login));
        strncpy(pass, argv[7], sizeof(pass));
        break;
      }
      show_help();
      exit(RXS_CLI_EINVAL);
    }
    default: {
      if ((argc > 1) && ((strcmp("-v", argv[1]) == 0) || (strcmp("--version", argv[1]) == 0))) {
        // show version
        fprintf(stdout, "RXS rev.%s\n", git_version);
        fflush(stdout);
        return 0;
      }
      // no '-v' -> invalid args
      show_help();
      exit(RXS_CLI_EINVAL);
    }
  }
  ////////////////////////////////////////////////////////////////////////////
  // The format of the log messages is:
  // DATE TIME MACHINE-NAME PROGRAM-NAME[PID]: MESSAGE
  ////////////////////////////////////////////////////////////////////////////
  char mod[FILENAME_MAX] = {0};
  snprintf(mod, sizeof(mod), "%s %s", "RXSc", git_version);
  openlog(mod, LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL4);  // LOG_LOCAL0

  int have_encoder = 0;
  //////////////////////////////////////////////////////////////////////////////////////////////////
  //
  //////////////////////////////////////////////////////////////////////////////////////////////////
  uint8_t operation_is_supported = 0;

  if (strncmp(operation_put, operation, strlen(operation_put_e)) == 0) {
    have_encoder = 0;
    operation_is_supported = 1;
  }
  if (strncmp(operation_get, operation, strlen(operation_get_e)) == 0) {
    have_encoder = 0;
    operation_is_supported = 1;
  }
  if (strncmp(operation_cli, operation, strlen(operation_cli_e)) == 0) {
    have_encoder = 0;
    operation_is_supported = 1;
  }
  if (strncmp(operation_put_e, operation, strlen(operation_put_e)) == 0) {
    have_encoder = 1;
    operation_is_supported = 1;
  }
  if (strncmp(operation_get_e, operation, strlen(operation_get_e)) == 0) {
    have_encoder = 1;
    operation_is_supported = 1;
  }
  if (strncmp(operation_cli_e, operation, strlen(operation_cli_e)) == 0) {
    have_encoder = 1;
    operation_is_supported = 1;
  }

  //////////////////////////////////////////////////////////////////////////////////////////////////
  // Is supported operation?
  //////////////////////////////////////////////////////////////////////////////////////////////////
  if (0 == operation_is_supported) {
    log_msg(ERRN, 40, operation);
    // Close logger
    closelog();
    exit(RXS_CLI_EPERM);
  }
  //////////////////////////////////////////////////////////////////////////////////////////////////
  // Create RXS point
  //////////////////////////////////////////////////////////////////////////////////////////////////
  if (rxs_point_create(other_side_addr_p, atoi(other_side_port_p), login, pass, have_encoder) != 0) {
    log_msg(ERRN, 41);
    // Close logger
    closelog();
    exit(rxs_errno());
  }
  int res = -1;
  //////////////////////////////////////////////////////////////////////////////////////////////////
  // GET remote file to local side
  //////////////////////////////////////////////////////////////////////////////////////////////////
  if ((strncmp(operation_get, operation, strlen(operation)) == 0) ||
      (strncmp(operation_get_e, operation, strlen(operation)) == 0)) {
    if (!file_local) {
      log_msg(ERRN, 57, "local");
      rxs_point_close();
      // Close logger
      closelog();
      exit(RXS_CLI_ENOENT);
    }
    // Is exist remote file?
    if (rxs_file_exist(file_remote) == 0) {
      log_msg(ERRN, 42, file_remote);
      rxs_point_close();
      // Close logger
      closelog();
      exit(rxs_errno());
    }
    long file_remote_size = rxs_filesize(file_remote);
    // Open the remote file
    RXS_HANDLE handle_file_remote = rxs_fopen(file_remote, "rb");
    if (0 == handle_file_remote) {
      log_msg(ERRN, 43, file_remote);
      rxs_point_close();
      // Close logger
      closelog();
      exit(rxs_errno());
    }
    // Truncate local file to zero length
    {
      FILE* handle_file_local = fopen(file_local, "wb");
      if (!handle_file_local) {
        int errno_tmp = errno;
        rxs_point_close();
        // Close logger
        closelog();
        exit(errno_tmp);
      }
      // Close local file
      fclose(handle_file_local);
    }
    uint32_t buf_sz = 5 * 1024 * 1024;
    char* buf_file_remote = (char*)calloc(buf_sz, sizeof(char));
    if (!buf_file_remote) {
      log_msg(ERRN, 44, buf_sz);
      rxs_point_close();
      // Close logger
      closelog();
      exit(RXS_CLI_ENOMEM);
    }
    uint8_t percent_last = 0;
    char lexeme[] = "Download...";
    // Open local file
    FILE* handle_file_local = fopen(file_local, "ab");
    if (!handle_file_local) {
      int errno_tmp = errno;
      // Free memory
      free(buf_file_remote);
      buf_file_remote = NULL;
      rxs_point_close();
      // Close logger
      closelog();
      exit(errno_tmp);
    }
    //////////////////////////////////////////////////////////////////////////////////////////////////
    // Read remote file by portions
    //////////////////////////////////////////////////////////////////////////////////////////////////
    size_t read_bytes_total = 0;
    while (1) {
      memset(buf_file_remote, 0, buf_sz);
      // Read the remote file
      size_t read_bytes = rxs_fread(buf_file_remote, buf_sz, sizeof(char), handle_file_remote);
      // printf("read_bytes_total=%d read_bytes=%d\n", read_bytes_total, read_bytes);
      read_bytes_total += read_bytes;
      // Progress bar
      show_progress_bar(file_remote_size, read_bytes_total, &percent_last, lexeme);
      if ((rxs_errno() != 0) && (rxs_errno() != RXS_EOF)) {
        log_msg(ERRN, 45, file_remote);
        // Close local file
        fclose(handle_file_local);
        // Free memory
        free(buf_file_remote);
        buf_file_remote = NULL;
        rxs_point_close();
        // Close logger
        closelog();
        exit(rxs_errno());
      }
      // Write to local file
      size_t write_bytes = fwrite(buf_file_remote, sizeof(buf_file_remote[0]), read_bytes, handle_file_local);
      if (write_bytes != read_bytes) {
        int errno_tmp = errno;
        log_msg(ERRN, 46, file_local);
        // Close local file
        fclose(handle_file_local);
        // Free memory
        free(buf_file_remote);
        buf_file_remote = NULL;
        rxs_point_close();
        // Close logger
        closelog();
        exit(errno_tmp);
      }
      // Flush local file
      fflush(handle_file_local);

      if (rxs_errno() == RXS_EOF) break;
    }
    // Clear output console
    clear_console();
    // Close local file
    // Note that fclose() flushes only the user-space buffers provided by the C library.
    // To ensure that the data is physically stored on disk the kernel buffers must be flushed too, for example, with
    // sync(2) or fsync(2).
    int errno_tmp = RXS_CLI_NONE;
    if (fclose(handle_file_local)) errno_tmp = errno;
    // Sync local buffers
    sync();
    // Close remote file
    res = rxs_fclose(handle_file_remote);
    if (res != 0) log_msg(ERRN, 47, res, rxs_errno());
    // Free memory
    free(buf_file_remote);
    buf_file_remote = NULL;
    // Close RXS point
    rxs_point_close();
    // Close logger
    closelog();
    // Success
    exit(errno_tmp);
  }
  //////////////////////////////////////////////////////////////////////////////////////////////////
  // PUT local file to remote side
  //////////////////////////////////////////////////////////////////////////////////////////////////
  if ((strncmp(operation_put, operation, strlen(operation)) == 0) ||
      (strncmp(operation_put_e, operation, strlen(operation)) == 0)) {
    if (!file_local) {
      log_msg(ERRN, 57, "local");
      rxs_point_close();
      // Close logger
      closelog();
      exit(RXS_CLI_ENOENT);
    }
    long file_local_sz = file_size(file_local);
    if (-1 == file_local_sz) {
      rxs_point_close();
      // Close logger
      closelog();
      exit(errno);
    }
    uint32_t buf_sz = 5 * 1024 * 1024;
    char* buf_file_local = (char*)calloc(buf_sz, sizeof(char));
    if (!buf_file_local) {
      log_msg(ERRN, 44, buf_sz);
      rxs_point_close();
      // Close logger
      closelog();
      exit(RXS_CLI_ENOMEM);
    }
    // Open the local file
    FILE* handle_file_local = fopen(file_local, "rb");
    if (!handle_file_local) {
      // Free memory
      free(buf_file_local);
      buf_file_local = NULL;
      rxs_point_close();
      // Close logger
      closelog();
      exit(errno);
    }
    // Truncate remote file to zero length
    {
      RXS_HANDLE handle_file_remote = rxs_fopen(file_remote, "wb");
      // Close remote file
      res = rxs_fclose(handle_file_remote);
      if (res != 0) {
        log_msg(ERRN, 47, res, rxs_errno());
        // Free memory
        free(buf_file_local);
        buf_file_local = NULL;
        rxs_point_close();
        // Close logger
        closelog();
        exit(errno);
      }
    }
    // Open the remote file
    RXS_HANDLE handle_file_remote = rxs_fopen(file_remote, "ab");
    if (0 == handle_file_remote) {
      log_msg(ERRN, 43, file_remote);
      // Close local file
      fclose(handle_file_local);
      // Free memory
      free(buf_file_local);
      buf_file_local = NULL;
      rxs_point_close();
      // Close logger
      closelog();
      exit(rxs_errno());
    }
    uint8_t percent_last = 0;
    char lexeme[] = "Upload...";
    //////////////////////////////////////////////////////////////////////////////////////////////////
    // Write to remote file by portions
    //////////////////////////////////////////////////////////////////////////////////////////////////
    size_t read_bytes_total = 0;
    size_t write_bytes_total = 0;
    while (write_bytes_total < file_local_sz) {
      memset(buf_file_local, 0, buf_sz);
      // Shift file handler to new position
      fseek(handle_file_local, read_bytes_total, SEEK_SET);
      // Read local file
      size_t read_bytes = fread(buf_file_local, sizeof(char), buf_sz, handle_file_local);
      read_bytes_total += read_bytes;
      // Progress bar
      show_progress_bar(file_local_sz, write_bytes_total, &percent_last, lexeme);
      // Examine local handle file on error
      if (ferror(handle_file_local) != 0) {
        // Close local file
        fclose(handle_file_local);
        // Close remote file
        res = rxs_fclose(handle_file_remote);
        if (res != 0) {
          log_msg(ERRN, 47, res, rxs_errno());
          // Free memory
          free(buf_file_local);
          buf_file_local = NULL;
          rxs_point_close();
          // Close logger
          closelog();
          exit(rxs_errno());
        }
        // Free memory
        free(buf_file_local);
        buf_file_local = NULL;
        rxs_point_close();
        // Close logger
        closelog();
        exit(rxs_errno());
      }
      // Write to the remote file
      size_t write_bytes = rxs_fwrite(buf_file_local, read_bytes, sizeof(char), handle_file_remote);
      write_bytes_total += write_bytes;
      if (rxs_errno() != 0) {
        log_msg(ERRN, 48, file_remote);
        // Close local file
        fclose(handle_file_local);
        // Free memory
        free(buf_file_local);
        buf_file_local = NULL;
        rxs_point_close();
        // Close logger
        closelog();
        exit(rxs_errno());
      }
      // Flush remote file
      res = rxs_fflush(handle_file_remote);
      if (rxs_errno() != 0) {
        log_msg(ERRN, 53);
        // Close local file
        fclose(handle_file_local);
        // Free memory
        free(buf_file_local);
        buf_file_local = NULL;
        rxs_point_close();
        // Close logger
        closelog();
        exit(rxs_errno());
      }
    }
    // Clear output console
    clear_console();

    // Free memory
    free(buf_file_local);
    buf_file_local = NULL;
    // Close local file
    fclose(handle_file_local);
    // Close remote file
    res = rxs_fclose(handle_file_remote);
    if (res != 0) {
      log_msg(ERRN, 47, res, rxs_errno());
      // Success
      rxs_point_close();
      // Close logger
      closelog();
      exit(rxs_errno());
    }
    // Success
    rxs_point_close();
    closelog();
    exit(RXS_CLI_NONE);
  }

  //////////////////////////////////////////////////////////////////////////////////////////////////
  // CLI remote system call
  //////////////////////////////////////////////////////////////////////////////////////////////////
  if ((strncmp(operation_cli, operation, strlen(operation)) == 0) ||
      (strncmp(operation_cli_e, operation, strlen(operation)) == 0)) {
    fprintf(stdout,
            "  Welcome to remote console (rxsc rev.%s)\n  Input command and press 'ENTER', for exit press 'Ctrl+C'\n",
            git_version);
    // Print CLI prefix
    compose_prefix_cli(other_side_addr_p, other_side_port_p, login);

    char input[PATH_MAX] = {0};
    while (fgets(input, sizeof(input), stdin)) {
      // Remove last symbol '\n' - it is key ENTER
      size_t len = strlen(input);
      if (len > 0 && input[len - 1] == '\n') input[--len] = 0;
      // Get remote file
      char path_remote_file[PATH_MAX] = {0};
      size_t res = rxs_ls(input, path_remote_file, sizeof(path_remote_file));
      if (res) {
        if (!((rxs_errno() == 0) || ((rxs_errno() - RXS_SRV_NONE) == 0)))
          fprintf(stdout, "error: '%s'\n", rxs_strerror());
        // Print CLI prefix
        compose_prefix_cli(other_side_addr_p, other_side_port_p, login);
        continue;
      }
      FILE* handle_file_local = NULL;
      if ((handle_file_local = fopen(path_remote_file, "r")) == NULL) {
        rxs_point_close();
        // Close logger
        closelog();
        exit(errno);
      }
      // Clear output console
      clear_console();
      // Print all symbols that contain received file
      char symbol;
      while (EOF != fscanf(handle_file_local, "%c", &symbol)) fprintf(stdout, "%c", symbol);

      fflush(stdout);
      fclose(handle_file_local);
      // Remove file
      unlink(path_remote_file);
      // Print CLI prefix
      compose_prefix_cli(other_side_addr_p, other_side_port_p, login);
    }
  }
  //////////////////////////////////////////////////////////////////////////////////////////////////
  // закрытие точки доступа к удаленной системе
  //////////////////////////////////////////////////////////////////////////////////////////////////
  rxs_point_close();
  // Close logger
  closelog();
  exit(RXS_CLI_INTERNAL);
}
