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
#ifndef RXS_LOGGER_H
#define RXS_LOGGER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdarg.h>  // for vararg
#include <stdint.h>
#include <stdlib.h>
#include <syslog.h>  // for 'syslog'

/////////////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////////////
typedef enum { INFO = 0, WARN, ERRN, STATUS } severity_t;
/////////////////////////////////////////////////////////////////////////////////////
// Log message
/////////////////////////////////////////////////////////////////////////////////////
void log_msg(int severity, int number_msg, ...);

#ifdef __cplusplus
}
#endif
#endif  // RXS_LOGGER_H
