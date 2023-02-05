/*******************************************************************************
** Copyright (c) 2020 v.arteev

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
#include <arpa/inet.h>
#include <errno.h>  // for 'errno'
#include <fcntl.h>
#include <inttypes.h>  // for 'PRIu32'
#include <limits.h>    // for 'PATH_MAX'
#include <signal.h>    // for 'cntl+C'
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "logger/logger.h"
#include "protocol/internal_types.h"
#include "protocol/parser.h"
#include "protocol/protocol_rxs.h"
#include "protocol/protocol_rxs_server.h"
#include "protocol/version.h"

#if defined(__i386__)
#define _FILE_OFFSET_BITS 64
#endif

int sockfd_listening = -1;
volatile int is_exit = 0;
void handler_cntl_c(int val) {
  (void)val;
  is_exit = 1;
  close(sockfd_listening);
  close(get_socket_connected());
  // fprintf(stderr, "Pressed cntl-c. Exit.\n");
}
void clean(int val) { waitpid(-1, 0, WNOHANG); }

int show_help(char* path_to_app) {
  if (!path_to_app) return -1;
  fprintf(stderr,
          "Sorry, try again.\n\
Help: use these commands in next format:\n\
 rxsd 'mode' 'address' 'port' 'allowed access addresses' 'file with budget' 'pid'\n\
 %s --mode=daemon --addr_rxs=192.168.56.51 --port_rxs=1301 --addr_allowed=192.168.56.0,193.168.56.5 --file_users=/etc/rxs/rxs_users --pid=0\n\
 RXS rev.%s\n",
          path_to_app, git_version);
  return 0;
}
// Main
int main(int argc, char* argv[]) {
  ////////////////////////////////////////////////////////////////////////////
  // The format of the log messages is:
  // DATE TIME MACHINE-NAME PROGRAM-NAME[PID]: MESSAGE
  ////////////////////////////////////////////////////////////////////////////
  char mod[FILENAME_MAX] = {0};
  snprintf(mod, sizeof(mod), "%s %s", "RXSd", git_version);
  openlog(mod, LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL4);  // LOG_LOCAL0

  log_msg(INFO, 0, "RXSd is trying to start ...");
  if (argc < 6) {
    if ((argc > 1) && ((strcmp("-v", argv[1]) == 0) || (strcmp("--version", argv[1]) == 0))) {
      // show version
      fprintf(stdout, "RXS rev.%s\n", git_version);
      fflush(stdout);
      // Close logger
      closelog();
      return 0;
    }
    show_help(argv[0]);
    // Close logger
    closelog();
    exit(EXIT_FAILURE);
  }

  int daemon_mode = 0;                 // Daemon mode
  uint32_t this_side_addr_n = 0;       // Address this side
  uint16_t this_side_port_h = 0;       // Port this side
  dlist_t* allowed_addr_t_lst = NULL;  // Allowed address
  char file_users[PATH_MAX] = {0};     // Path to file 'rxs_users'
  uint8_t encoder_mode = 0;            // Encoder mode (0 - plain mode; 1 - encoder mode)
  //////////////////////////////////////////////////////////////////////////////////
  // CAUTION: If pid is 0, sig shall be sent to all processes (excluding an unspecified set of system processes)
  // whose process group ID is equal to the process group ID of the sender, and for which the process has permission to
  // send a signal. see link: https://linux.die.net/man/3/kill
  //////////////////////////////////////////////////////////////////////////////////
  pid_t pid_m = 0;

  if (parse_args(argc, argv, &this_side_addr_n, &this_side_port_h, &allowed_addr_t_lst, &daemon_mode, file_users,
                 &pid_m, &encoder_mode) != 0) {
    // Free memory
    list_clear(&allowed_addr_t_lst, free_addr_t);
    log_msg(ERRN, 4);
    // Close logger
    closelog();
    exit(EXIT_FAILURE);
  }
  //////////////////////////////////////////////////////////////////////////////////
  // Encoder mode
  //////////////////////////////////////////////////////////////////////////////////
  set_encoder_mode(encoder_mode);
  if (is_encoder_mode())
    log_msg(INFO, 54, "rxsd", "encoded");
  else
    log_msg(INFO, 54, "rxsd", "plain");
  //////////////////////////////////////////////////////////////////////////////////
  // Daemon mode
  //////////////////////////////////////////////////////////////////////////////////
  if (daemon_mode) {
    if (fork()) exit(0);  // Exit from parent process
    setsid();
    if (fork()) return 0;  // Exit from parent process
  }

  //////////////////////////////////////////////////////////////////////////////////
  // Add allowed addresses
  //////////////////////////////////////////////////////////////////////////////////
  while (!list_empty(allowed_addr_t_lst)) {
    dlist_t* node = list_front(allowed_addr_t_lst);
    if (node) {
      addr_t* addr = (addr_t*)node->data;

      // Set allowed address
      if (set_allow_address(addr->addr_n) != 0) {
        // Free memory
        list_clear(&allowed_addr_t_lst, free_addr_t);
        log_msg(ERRN, 6, "set_allow_address", "");
        // Close logger
        closelog();
        exit(EXIT_FAILURE);
      }
    }
    list_pop_front(&allowed_addr_t_lst, free_addr_t);
  }
  ////////////////////////////////////////////////////////////////////////////
  // Set users file path
  ////////////////////////////////////////////////////////////////////////////
  if (set_path_users(file_users, strlen(file_users)) != 0) {
    // Free memory
    list_clear(&allowed_addr_t_lst, free_addr_t);
    // Close logger
    closelog();
    exit(EXIT_FAILURE);
  }
  //////////////////////////////////////////////////////////////////////////////////
  // Set handler for pressed 'Cntl-C.'
  //////////////////////////////////////////////////////////////////////////////////
  signal(SIGINT, handler_cntl_c);

  struct sockaddr_in addr_this, addr_other;
  // CAUTION: See link: https://tangentsoft.net/wskfaq/advanced.html#backlog
  // You will note that SYN attacks are dangerous for systems with both very short and very long backlog queues. The
  // point is that a middle ground is the best course if you expect your server to withstand SYN attacks. Either use
  // Microsoft’s dynamic backlog feature, or pick a value somewhere in the 20-200 range and tune it as required.
  int backlog = 30;  // SOMAXCONN;
  sockfd_listening = socket(AF_INET, SOCK_STREAM, 0);
  if (-1 == sockfd_listening) {
    log_msg(ERRN, 6, "socket", strerror(errno));
    // Close logger
    closelog();
    exit(EXIT_FAILURE);
  }

  // Allow multiple listeners on the broadcast address
  int so_reuseaddr = 1;
  if (setsockopt(sockfd_listening, SOL_SOCKET, SO_REUSEADDR, &so_reuseaddr, sizeof(so_reuseaddr))) {
    log_msg(ERRN, 59, "SO_REUSEADDR", strerror(errno));
    close(sockfd_listening);
    // Close logger
    closelog();
    exit(EXIT_FAILURE);
  }
  memset(&addr_this, 0, sizeof(addr_this));
  addr_this.sin_family = AF_INET;
  addr_this.sin_port = htons(this_side_port_h);
  addr_this.sin_addr.s_addr = this_side_addr_n;

  socklen_t addr_this_len = sizeof(addr_this);

  // For information string output
  char this_addr_p[INET_ADDRSTRLEN] = {0};
  uint32_t this_addr_n = ((struct sockaddr_in*)&addr_this)->sin_addr.s_addr;
  if (!inet_ntop(AF_INET, &this_addr_n, this_addr_p, sizeof(this_addr_p))) {
    log_msg(ERRN, 3, strerror(errno));
    close(sockfd_listening);
    // Close logger
    closelog();
    exit(EXIT_FAILURE);
  }
  // Bind socket
  if (bind(sockfd_listening, (struct sockaddr*)&addr_this, addr_this_len) == -1) {
    log_msg(ERRN, 6, "bind", strerror(errno));
    close(sockfd_listening);
    // Close logger
    closelog();
    exit(EXIT_FAILURE);
  }
  log_msg(STATUS, 1, git_version);
  // Listen socket
  if (listen(sockfd_listening, backlog) == -1) {
    log_msg(ERRN, 6, "listen", strerror(errno));
    close(sockfd_listening);
    // Close logger
    closelog();
    exit(EXIT_FAILURE);
  }
  //////////////////////////////////////////////////////////////////////////////////////////////////
  // Send signal to main process about condition that RXSd is ready
  //////////////////////////////////////////////////////////////////////////////////////////////////
  if (pid_m != 0) {
    if (kill(pid_m, SIGUSR1) != 0) log_msg(ERRN, 6, "kill", strerror(errno));
  }
  // Accept connection
  memset(&addr_other, 0, sizeof(addr_other));
  socklen_t addr_other_len = sizeof(addr_other);

  int pid = -1;
  for (;;) {
    // Check condition for exit
    if (1 == is_exit) {
      log_msg(INFO, 7);
      break;
    }
    log_msg(STATUS, 8, this_addr_p, this_side_port_h);

    int sockfd_connected = accept(sockfd_listening, (struct sockaddr*)&addr_other, &addr_other_len);
    set_socket_connected(sockfd_connected);
    if (-1 == sockfd_connected) {
      // perror("accept");
      continue;
    }
    if ((pid = fork()) == -1) {
      close(get_socket_connected());
      close(sockfd_listening);
      log_msg(ERRN, 6, "fork", strerror(errno));
      // Close logger
      closelog();
      exit(EXIT_FAILURE);
    }
    // Parent
    else if (pid > 0) {
      // Close connected socket for child
      close(get_socket_connected());
      // set_socket_connected(-1);
      // CAUTION: to avoid of zombie processes need use ignoring the SIGCHLD signal
      signal(SIGCHLD, SIG_IGN);
      // I'll just get back to listening state into the loop
      continue;
    }
    // Child
    else if (0 == pid) {
      // Close listening socket for parent
      close(sockfd_listening);
      set_client_addr(&addr_other);
      //////////////////////////////////////////////////////////////////////////////////////////////////
      // Connection info
      //////////////////////////////////////////////////////////////////////////////////////////////////
      char other_addr_p[INET_ADDRSTRLEN] = {0};
      uint32_t other_addr_n = ((struct sockaddr_in*)&addr_other)->sin_addr.s_addr;
      if (!inet_ntop(AF_INET, &other_addr_n, other_addr_p, sizeof(other_addr_p))) {
        log_msg(ERRN, 3, strerror(errno));
        close(get_socket_connected());
        //////////////////////////////////////////////////////////////////////////////////
        // Exit from child
        //////////////////////////////////////////////////////////////////////////////////
        // CAUTION: How to kill zombie process? (see link:
        // https://kerneltalks.com/howto/everything-need-know-zombie-process/) You simple cant! Because its Zombie! Its
        // already dead! Maximum you can do is to inform its parent process that its child is dead and now you can
        // initiate wait system call. This can be achieved by sending SIGCHLD signal to parent PID using below command:
        kill(getppid(), SIGCHLD);
        exit(0);
      }
      uint16_t other_port_h = ntohs((((struct sockaddr_in*)&addr_other)->sin_port));
      //////////////////////////////////////////////////////////////////////////////////////////////////
      // CAUTION: Drop forbidden ip connections
      //////////////////////////////////////////////////////////////////////////////////////////////////
      if (is_allow_address(other_addr_n) == 0) {
        // Drop connection
        log_msg(STATUS, 9, other_addr_p, other_port_h);
        close(get_socket_connected());
        //////////////////////////////////////////////////////////////////////////////////
        // Exit from child
        //////////////////////////////////////////////////////////////////////////////////
        // CAUTION: How to kill zombie process? (see link:
        // https://kerneltalks.com/howto/everything-need-know-zombie-process/) You simple cant! Because its Zombie! Its
        // already dead! Maximum you can do is to inform its parent process that its child is dead and now you can
        // initiate wait system call. This can be achieved by sending SIGCHLD signal to parent PID using below command:
        kill(getppid(), SIGCHLD);
        exit(0);
      }
      log_msg(STATUS, 10, other_addr_p, other_port_h);
      //////////////////////////////////////////////////////////////////////////////////////////////////
      // Processing incoming data
      //////////////////////////////////////////////////////////////////////////////////////////////////
      uint32_t buf_recv_sz = 1024 * 128;
      uint8_t* buf_recv = (uint8_t*)calloc(buf_recv_sz, sizeof(uint8_t));
      if (!buf_recv) {
        log_msg(ERRN, 6, "calloc", strerror(errno));
        close(get_socket_connected());
        //////////////////////////////////////////////////////////////////////////////////
        // Exit from child
        //////////////////////////////////////////////////////////////////////////////////
        // CAUTION: How to kill zombie process? (see link:
        // https://kerneltalks.com/howto/everything-need-know-zombie-process/) You simple cant! Because its Zombie! Its
        // already dead! Maximum you can do is to inform its parent process that its child is dead and now you can
        // initiate wait system call. This can be achieved by sending SIGCHLD signal to parent PID using below command:
        kill(getppid(), SIGCHLD);
        exit(0);
      }
      for (;;) {
        // Check condition for exit
        if (1 == is_exit) {
          log_msg(INFO, 7);
          break;
        }
        ssize_t impl_recv_sz = rxs_recv_data_x(get_socket_connected(), buf_recv, buf_recv_sz);
        if (impl_recv_sz > 0) {
          //////////////////////////////////////////////////////////////////////////////////
          // Main block. Begin
          //////////////////////////////////////////////////////////////////////////////////
          // Compose packet
          packet_rxs_t packet_rxs_recv;
          init_packet_rxs_t(&packet_rxs_recv);
          if (deserialize_packet_rxs_t(buf_recv, (size_t)impl_recv_sz, &packet_rxs_recv) != 0) {
            log_msg(ERRN, 6, "deserialize_packet_rxs_t", "");
            break;
          }
          // Execute command
          packet_rxs_t packet_rxs_send;
          init_packet_rxs_t(&packet_rxs_send);
          if (run_operation(packet_rxs_recv.operation, &packet_rxs_recv, &packet_rxs_send) != 0) {
            log_msg(ERRN, 5, packet_rxs_recv.operation);
            // Close socket
            close(get_socket_connected());
            // Free memory
            dinit_packet_rxs_t(&packet_rxs_recv);
            dinit_packet_rxs_t(&packet_rxs_send);
            break;
          }

          // Send packet
          if (packet_rxs_send.operation > 0) {
            uint8_t authentication_failed = 0;
            if ((operation_authorization == packet_rxs_send.operation) && (SC_B1 == packet_rxs_send.type)) {
              authentication_failed = 1;
            }
            if (rxs_send_packet(get_socket_connected(), &packet_rxs_send) < 0) {
              log_msg(ERRN, 6, "rxs_send_packet", strerror(errno));
              // Close socket
              close(get_socket_connected());
              // Free memory
              dinit_packet_rxs_t(&packet_rxs_recv);
              dinit_packet_rxs_t(&packet_rxs_send);
              break;
            }
            //////////////////////////////////////////////////////////////////////////////////
            // Authorization failed: need close connection
            //////////////////////////////////////////////////////////////////////////////////
            if (1 == authentication_failed) {
              log_msg(STATUS, 11, other_addr_p, other_port_h);
              close(get_socket_connected());
              // Free memory
              dinit_packet_rxs_t(&packet_rxs_recv);
              dinit_packet_rxs_t(&packet_rxs_send);
              break;
            }
          }
          // Free memory
          dinit_packet_rxs_t(&packet_rxs_recv);
          dinit_packet_rxs_t(&packet_rxs_send);
          //////////////////////////////////////////////////////////////////////////////////
          // Main block. End
          //////////////////////////////////////////////////////////////////////////////////
        }
        // CAUTION: The other side has closed connection
        else {
          log_msg(STATUS, 12, other_addr_p, other_port_h);
          close(get_socket_connected());
          // set_socket_connected(-1);
          break;
        }
      }
      // Free memory
      free(buf_recv);
      buf_recv = NULL;
      //////////////////////////////////////////////////////////////////////////////////
      // Exit from child
      //////////////////////////////////////////////////////////////////////////////////
      // CAUTION: How to kill zombie process? (see link:
      // https://kerneltalks.com/howto/everything-need-know-zombie-process/) You simple cant! Because its Zombie! Its
      // already dead! Maximum you can do is to inform its parent process that its child is dead and now you can
      // initiate wait system call. This can be achieved by sending SIGCHLD signal to parent PID using below command:
      kill(getppid(), SIGCHLD);
      exit(0);
    }
  }
  // Close file handlers
  clear_file_handlers_lst();
  // Clear user info list
  clear_user_info_lst();
  // Clear allow address list
  clear_allow_address_lst();
  // Clear deny address list
  clear_deny_address_lst();
  // Close listening socket
  close(sockfd_listening);

  log_msg(STATUS, 13);
  // Close logger
  closelog();
  exit(EXIT_SUCCESS);
}
