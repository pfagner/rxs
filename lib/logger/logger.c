#include <inttypes.h>  // for 'PRIu32'
#include <stdint.h>    // for uint32_t
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef __QNXNTO__
#include <linux/limits.h>  // for 'PATH_MAX'
#else
#include <limits.h>
#endif
#include <syslog.h>  // for 'syslog'

#include "logger/logger.h"

const char* MSGS[] = {"%s",  // 0
                      "RXS server is ready. GIT version %s",
                      "settings file '%s' has been loaded",
                      "invalid address %s",
                      "parsing arguments has been failed",
                      "operation #:%" PRIu16 " has been failed",
                      "call '%s' failed (%s)",  // 6
                      "there is occurred a condition for exit",
                      "RXS waiting connection on %s:%" PRIu16 "...",
                      "RXS remote host %s:%" PRIu16 " is not allowed",
                      "RXS remote host %s:%" PRIu16 " established connection",  // 10
                      "RXS remote host %s:%" PRIu16 " authentication failed",
                      "RXS remote host %s:%" PRIu16 " closed connection",
                      "RXS server bye",
                      "parameter is null",
                      "incoming data '%" PRIu32 "' B overflow buffer size '%zu' B",  // 15
                      "incoming data '%zu' B overflow buffer size '%zu' B",
                      "incoming data '%" PRIu32 "' B overflow buffer size '%" PRIu32 "' B",
                      "buffer capacity is not enough. Buffer will be rewind to start",
                      "socket has been closed the other side",
                      "unexpected error",  // 20
                      "CRC32 is invalid",
                      "CRC32 specified '%x' is not equal calculated '%x' (pos:%" PRIu32 ")",
                      "data size '%zu' less that header size %zu",
                      "the address '%s' is not host address",
                      "%s too long",  // 25
                      "user authentication:'%s' sz:%d B pass:'%s' sz:%d B...",
                      "authentication is OK. working directory %s",
                      "cannot change working directory '%s'",
                      "user and/or password are invalid",
                      "cmd 'fread' parameter size '%" PRIu32 "' B is incorrect",  // 30
                      "operation '%zu' do not have permissions",
                      "cmd '%s' result:%d",
                      "cmd '%s' result:%s",
                      "cmd '%s' result:%x",
                      "cmd '%s' result:%zu",  // 35
                      "cmd '%s' handler:%x result:%d",
                      "cmd '%s' handler:%x result:%ld",
                      "unknown operation '%d'",
                      "Pressed cntl-c. Exit",
                      "file operation '%s' is not supported",  // 40
                      "cannot to create RXS client point",
                      "remote file '%s' is not exist",
                      "cannot open remote file '%s'",
                      "cannot allocate memory for '%d' B",
                      "error occurred while reading the remote file '%s'",  // 45
                      "error occurred while writing the local file '%s'",
                      "cannot close RXS file handler result:%d errno:%d",
                      "error occurred while writting the remote file '%s'",
                      "authorization failed",
                      "size of data '%zu' B is exceeded of max value '%ld'",  // 50
                      "data receiving has been failed",
                      "size of data '%zu' B is zero size",
                      "cannot flush",
                      "%s use: %s data",
                      "%s changed: %s data",
                      "bind() error in data connection %s",
                      "%s file not specified",  // 57
                      "",
                      "setsockopt(%s) error:%s",
                      ""};  //

void log_msg(int severity, int number_msg, ...) {
  const char severity_INFO[] = "INFO:";
  const char severity_WARN[] = "WARN:";
  const char severity_ERRN[] = "ERRN:";
  const char severity_STATUS[] = "";
  const char* ptr_severity = NULL;
  if (severity == INFO) ptr_severity = severity_INFO;
  if (severity == WARN) ptr_severity = severity_WARN;
  if (severity == ERRN) ptr_severity = severity_ERRN;
  if (severity == STATUS) ptr_severity = severity_STATUS;

  char buf_msg[PATH_MAX] = {0};
  char buf[PATH_MAX * 2] = {0};

  const char* ptr_msg = NULL;
  int count = sizeof(MSGS) / sizeof(MSGS[0]);
  if (number_msg < count) ptr_msg = MSGS[number_msg];
  if (!ptr_msg) return;
  va_list args;
  va_start(args, number_msg);
  vsprintf(buf_msg, ptr_msg, args);
  va_end(args);

  if (ptr_severity) {
    if (strlen(ptr_severity) != 0)
      snprintf(buf, sizeof(buf), "%s %s", ptr_severity, buf_msg);
    else
      snprintf(buf, sizeof(buf), "%s", buf_msg);
  }
  // Output to screen
  fprintf(stderr, "%s\n", buf);
  // Output to log
  syslog(LOG_WARNING, "'%s'", buf);

  return;
}
