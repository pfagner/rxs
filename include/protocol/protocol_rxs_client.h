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
#ifndef _RXS_CLIENT_PROTOCOL_H
#define _RXS_CLIENT_PROTOCOL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <netinet/tcp.h>  //TCP_MAXSEG TCP_NODELAY
#include <stdint.h>
#include <sys/stat.h>
#include <sys/types.h>  // for 'mode_t'
#include <unistd.h>

#include "protocol/protocol_rxs.h"

//////////////////////////////////////////////////////////////////////////////////////////////////
// RXS Remote eXchange System: Allows editing files and perform commands on remote file systems
//////////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////////////
// Functions for a create, close, existatnce of a connection to remote side
//////////////////////////////////////////////////////////////////////////////////////////////////
// create access point to remote side
// Return value: on successful returns 0, otherwise -1
size_t rxs_point_create(const char* host_p, uint16_t port_h, const char* username, const char* password, int encoder);

// check for existance of a connection
// Return value: on successful returns 1, otherwise 0
size_t rxs_point_connected();

// close access poit to remote side
// Return value: on successful returns 0, otherwise -1
size_t rxs_point_close();

// Retun value: number of last error
int rxs_errno();

// Retun value: number of last error as string (with indication client or server side)
char* rxs_strerror();

//////////////////////////////////////////////////////////////////////////////////////////////////
// Functions for operations with file, directories, commands
//////////////////////////////////////////////////////////////////////////////////////////////////
// attepmts to execute command
// Return value:
size_t rxs_ls(const char* path, void* buf, size_t count);

// attempts to create a directory named pathname
// Return value: returns zero on success, or -1 if an error occurred (in which case, errno is set appropriately).
int rxs_mkdir(const char* path, mode_t mode);

// attempts to create a nested directories named pathname
// Return value: returns zero on success, or -1 if an error occurred (in which case, errno is set appropriately).
int rxs_mkdir_ex(const char* path, mode_t mode);

// deletes a directory, which must be empty.
// Return value: On success, zero is returned. On error, -1 is returned, and errno is set appropriately.
int rxs_rmdir(const char* path);

// get current working directory
// Return value: On success, these functions return a pointer to a string containing the pathname of the current working
// directory. On failure, these functions return NULL, and errno is set to indicate the error. The contents of the array
// pointed to by buf are undefined on error.
char* rxs_getcwd(char* buf, size_t size);

// change working directory
// Return Value: On success, zero is returned. On error, -1 is returned, and errno is set appropriately.
int rxs_chdir(const char* path);

// delete a name and possibly the file it refers to
// Return value: On success, zero is returned. On error, -1 is returned, and errno is set appropriately.
int rxs_unlink(const char* path);

// change the name or location of a file
// Return value: On success, zero is returned. On error, -1 is returned, and errno is set appropriately.
int rxs_rename(const char* oldname, const char* newname);

// returns a file size
// Return value: On success, file size is returned. On error, -1 is returned, and errno is set appropriately.
long rxs_filesize(const char* fname);

// stream open functions
// Return value: Upon successful completion return a file handler. Otherwise, zero is returned and errno is set to
// indicate the error.
RXS_HANDLE rxs_fopen(const char* fname, const char* mode);

// binary stream input
// Return value: On success, return the number of items read. This number equals the number of bytes transferred only
// when size is 1. If an error occurs, or the end of the file is reached, the return value is a short item count (or
// zero). fread() does not distinguish between end-of-file and error, and callers must use feof(3) and ferror(3) to
// determine which occurred.
size_t rxs_fread(void* buf, size_t size, size_t count, RXS_HANDLE stream);

// binary stream output
// Return value: On success, return the number of items written. This number equals the number of bytes transferred only
// when size is 1. If an error occurs, or the end of the file is reached, the return value is a short item count (or
// zero).
size_t rxs_fwrite(const void* buf, size_t size, size_t count, RXS_HANDLE stream);

// flush a stream
// Return value: Upon successful completion 0 is returned. Otherwise, EOF is returned and errno is set to indicate the
// error.
int rxs_fflush(RXS_HANDLE stream);

// close a stream
// Return value: Upon successful completion 0 is returned. Otherwise, EOF is returned and errno is set to indicate the
// error. In either case any further access (including another call to fclose()) to the stream results in undefined
// behavior.
int rxs_fclose(RXS_HANDLE stream);

// reposition a stream
// Upon successful completion return 0. Otherwise, -1 is returned and errno is set to indicate the error.
int rxs_fseek(RXS_HANDLE stream, long offset, int whence);

// reposition a stream
// Return value: Upon successful completion returns the current offset. Otherwise, -1 is returned and errno is set to
// indicate the error.
long rxs_ftell(RXS_HANDLE stream);

// reposition a stream
// Return value: returns no value.
void rxs_rewind(RXS_HANDLE stream);

// check for the existense of a file
// Return value: on successful returns 1, if not exist 0; otherwise -1
int rxs_file_exist(const char* path_file);

// check for the existense of a directory
// Return value: on successful returns 1, if not exist 0; otherwise -1
int rxs_dir_exist(const char* path_dir);

#ifdef __cplusplus
}
#endif

#endif  // _RXS_CLIENT_PROTOCOL_H
