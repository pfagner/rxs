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
#ifndef _RXS_ERRNO_H
#define _RXS_ERRNO_H
// Error RXS constants.  RXS specific version.
// Errno RXS
typedef enum rxs_errno_t {
  RXS_CLI_NONE = 0,  // None error
  // Client side errors
  RXS_CLI_EPERM,            // Operation not permitted
  RXS_CLI_ENOENT,           // No such file or directory
  RXS_CLI_ESRCH,            // No such process
  RXS_CLI_EINTR,            // Interrupted system call
  RXS_CLI_EIO,              // I/O error
  RXS_CLI_ENXIO,            // No such device or address
  RXS_CLI_E2BIG,            // Argument list too long
  RXS_CLI_ENOEXEC,          // Exec format error
  RXS_CLI_EBADF,            // Bad file number
  RXS_CLI_ECHILD,           // No child processes
  RXS_CLI_EAGAIN,           // Try again
  RXS_CLI_ENOMEM,           // Out of memory
  RXS_CLI_EACCES,           // Permission denied
  RXS_CLI_EFAULT,           // Bad address
  RXS_CLI_ENOTBLK,          // Block device required
  RXS_CLI_EBUSY,            // Device or resource busy
  RXS_CLI_EEXIST,           // File exists
  RXS_CLI_EXDEV,            // Cross-device link
  RXS_CLI_ENODEV,           // No such device
  RXS_CLI_ENOTDIR,          // Not a directory
  RXS_CLI_EISDIR,           // Is a directory
  RXS_CLI_EINVAL,           // Invalid argument
  RXS_CLI_ENFILE,           // File table overflow
  RXS_CLI_EMFILE,           // Too many open files
  RXS_CLI_ENOTTY,           // Not a typewriter
  RXS_CLI_ETXTBSY,          // Text file busy
  RXS_CLI_EFBIG,            // File too large
  RXS_CLI_ENOSPC,           // No space left on device
  RXS_CLI_ESPIPE,           // Illegal seek
  RXS_CLI_EROFS,            // Read-only file system
  RXS_CLI_EMLINK,           // Too many links
  RXS_CLI_EPIPE,            // Broken pipe
  RXS_CLI_EDOM,             // Math argument out of domain of func
  RXS_CLI_ERANGE,           // Math result not representable
  RXS_CLI_EDEADLK,          // Resource deadlock would occur
  RXS_CLI_ENAMETOOLONG,     // File name too long
  RXS_CLI_ENOLCK,           // No record locks available
  RXS_CLI_ENOSYS,           // Function not implemented
  RXS_CLI_ENOTEMPTY,        // Directory not empty
  RXS_CLI_ELOOP,            // Too many symbolic links encountered
  RXS_CLI_EWOULDBLOCK,      // Operation would block
  RXS_CLI_ENOMSG,           // No message of desired type
  RXS_CLI_EIDRM,            // Identifier removed
  RXS_CLI_ECHRNG,           // Channel number out of range
  RXS_CLI_EL2NSYNC,         // Level 2 not synchronized
  RXS_CLI_EL3HLT,           // Level 3 halted
  RXS_CLI_EL3RST,           // Level 3 reset
  RXS_CLI_ELNRNG,           // Link number out of range
  RXS_CLI_EUNATCH,          // Protocol driver not attached
  RXS_CLI_ENOCSI,           // No CSI structure available
  RXS_CLI_EL2HLT,           // Level 2 halted
  RXS_CLI_EBADE,            // Invalid exchange
  RXS_CLI_EBADR,            // Invalid request descriptor
  RXS_CLI_EXFULL,           // Exchange full
  RXS_CLI_ENOANO,           // No anode
  RXS_CLI_EBADRQC,          // Invalid request code
  RXS_CLI_EBADSLT,          // Invalid slot
  RXS_CLI_EDEADLOCK,        //
  RXS_CLI_EBFONT,           // Bad font file format
  RXS_CLI_ENOSTR,           // Device not a stream
  RXS_CLI_ENODATA,          // No data available
  RXS_CLI_ETIME,            // Timer expired
  RXS_CLI_ENOSR,            // Out of streams resources
  RXS_CLI_ENONET,           // Machine is not on the network
  RXS_CLI_ENOPKG,           // Package not installed
  RXS_CLI_EREMOTE,          // Object is remote
  RXS_CLI_ENOLINK,          // Link has been severed
  RXS_CLI_EADV,             // Advertise error
  RXS_CLI_ESRMNT,           // Srmount error
  RXS_CLI_ECOMM,            // Communication error on send
  RXS_CLI_EPROTO,           // Protocol error
  RXS_CLI_EMULTIHOP,        // Multihop attempted
  RXS_CLI_EDOTDOT,          // RXS specific error
  RXS_CLI_EBADMSG,          // Not a data message
  RXS_CLI_EOVERFLOW,        // Value too large for defined data type
  RXS_CLI_ENOTUNIQ,         // Name not unique on network
  RXS_CLI_EBADFD,           // File descriptor in bad state
  RXS_CLI_EREMCHG,          // Remote address changed
  RXS_CLI_ELIBACC,          // Can not access a needed shared library
  RXS_CLI_ELIBBAD,          // Accessing a corrupted shared library
  RXS_CLI_ELIBSCN,          // .lib section in a.out corrupted
  RXS_CLI_ELIBMAX,          // Attempting to link in too many shared libraries
  RXS_CLI_ELIBEXEC,         // Cannot exec a shared library directly
  RXS_CLI_EILSEQ,           // Illegal byte sequence
  RXS_CLI_ERESTART,         // Interrupted system call should be restarted
  RXS_CLI_ESTRPIPE,         // Streams pipe error
  RXS_CLI_EUSERS,           // Too many users
  RXS_CLI_ENOTSOCK,         // Socket operation on non-socket
  RXS_CLI_EDESTADDRREQ,     // Destination address required
  RXS_CLI_EMSGSIZE,         // Message too long
  RXS_CLI_EPROTOTYPE,       // Protocol wrong type for socket
  RXS_CLI_ENOPROTOOPT,      // Protocol not available
  RXS_CLI_EPROTONOSUPPORT,  // Protocol not supported
  RXS_CLI_ESOCKTNOSUPPORT,  // Socket type not supported
  RXS_CLI_EOPNOTSUPP,       // Operation not supported on transport endpoint
  RXS_CLI_EPFNOSUPPORT,     // Protocol family not supported
  RXS_CLI_EAFNOSUPPORT,     // Address family not supported by protocol
  RXS_CLI_EADDRINUSE,       // Address already in use
  RXS_CLI_EADDRNOTAVAIL,    // Cannot assign requested address
  RXS_CLI_ENETDOWN,         // Network is down
  RXS_CLI_ENETUNREACH,      // Network is unreachable
  RXS_CLI_ENETRESET,        // Network dropped connection because of reset
  RXS_CLI_ECONNABORTED,     // Software caused connection abort
  RXS_CLI_ECONNRESET,       // Connection reset by peer
  RXS_CLI_ENOBUFS,          // No buffer space available
  RXS_CLI_EISCONN,          // Transport endpoint is already connected
  RXS_CLI_ENOTCONN,         // Transport endpoint is not connected
  RXS_CLI_ESHUTDOWN,        // Cannot send after transport endpoint shutdown
  RXS_CLI_ETOOMANYREFS,     // Too many references: cannot splice
  RXS_CLI_ETIMEDOUT,        // Connection timed out
  RXS_CLI_ECONNREFUSED,     // Connection refused
  RXS_CLI_EHOSTDOWN,        // Host is down
  RXS_CLI_EHOSTUNREACH,     // No route to host
  RXS_CLI_EALREADY,         // Operation already in progress
  RXS_CLI_EINPROGRESS,      // Operation now in progress
  RXS_CLI_ESTALE,           // Stale file handle
  RXS_CLI_EUCLEAN,          // Structure needs cleaning
  RXS_CLI_ENOTNAM,          // Not a XENIX named type file
  RXS_CLI_ENAVAIL,          // No XENIX semaphores available
  RXS_CLI_EISNAM,           // Is a named type file
  RXS_CLI_EREMOTEIO,        // Remote I/O error
  RXS_CLI_EDQUOT,           // Quota exceeded
  RXS_CLI_ENOMEDIUM,        // No medium found
  RXS_CLI_EMEDIUMTYPE,      // Wrong medium type
  RXS_CLI_ECANCELED,        // Operation Canceled
  RXS_CLI_ENOKEY,           // Required key not available
  RXS_CLI_EKEYEXPIRED,      // Key has expired
  RXS_CLI_EKEYREVOKED,      // Key has been revoked
  RXS_CLI_EKEYREJECTED,     // Key was rejected by service
  RXS_CLI_EOWNERDEAD,       // Owner died
  RXS_CLI_ENOTRECOVERABLE,  // State not recoverable
  RXS_CLI_ERFKILL,          // Operation not possible due to RF-kill
  RXS_CLI_EHWPOISON,        // Memory page has hardware error
  RXS_CLI_INTERNAL,         // Internal error
  // Server side errors
  RXS_SRV_NONE = 200,       // None error
  RXS_SRV_EPERM,            // Operation not permitted
  RXS_SRV_ENOENT,           // No such file or directory
  RXS_SRV_ESRCH,            // No such process
  RXS_SRV_EINTR,            // Interrupted system call
  RXS_SRV_EIO,              // I/O error
  RXS_SRV_ENXIO,            // No such device or address
  RXS_SRV_E2BIG,            // Argument list too long
  RXS_SRV_ENOEXEC,          // Exec format error
  RXS_SRV_EBADF,            // Bad file number
  RXS_SRV_ECHILD,           // No child processes
  RXS_SRV_EAGAIN,           // Try again
  RXS_SRV_ENOMEM,           // Out of memory
  RXS_SRV_EACCES,           // Permission denied
  RXS_SRV_EFAULT,           // Bad address
  RXS_SRV_ENOTBLK,          // Block device required
  RXS_SRV_EBUSY,            // Device or resource busy
  RXS_SRV_EEXIST,           // File exists
  RXS_SRV_EXDEV,            // Cross-device link
  RXS_SRV_ENODEV,           // No such device
  RXS_SRV_ENOTDIR,          // Not a directory
  RXS_SRV_EISDIR,           // Is a directory
  RXS_SRV_EINVAL,           // Invalid argument
  RXS_SRV_ENFILE,           // File table overflow
  RXS_SRV_EMFILE,           // Too many open files
  RXS_SRV_ENOTTY,           // Not a typewriter
  RXS_SRV_ETXTBSY,          // Text file busy
  RXS_SRV_EFBIG,            // File too large
  RXS_SRV_ENOSPC,           // No space left on device
  RXS_SRV_ESPIPE,           // Illegal seek
  RXS_SRV_EROFS,            // Read-only file system
  RXS_SRV_EMLINK,           // Too many links
  RXS_SRV_EPIPE,            // Broken pipe
  RXS_SRV_EDOM,             // Math argument out of domain of func
  RXS_SRV_ERANGE,           // Math result not representable
  RXS_SRV_EDEADLK,          // Resource deadlock would occur
  RXS_SRV_ENAMETOOLONG,     // File name too long
  RXS_SRV_ENOLCK,           // No record locks available
  RXS_SRV_ENOSYS,           // Function not implemented
  RXS_SRV_ENOTEMPTY,        // Directory not empty
  RXS_SRV_ELOOP,            // Too many symbolic links encountered
  RXS_SRV_EWOULDBLOCK,      // Operation would block
  RXS_SRV_ENOMSG,           // No message of desired type
  RXS_SRV_EIDRM,            // Identifier removed
  RXS_SRV_ECHRNG,           // Channel number out of range
  RXS_SRV_EL2NSYNC,         // Level 2 not synchronized
  RXS_SRV_EL3HLT,           // Level 3 halted
  RXS_SRV_EL3RST,           // Level 3 reset
  RXS_SRV_ELNRNG,           // Link number out of range
  RXS_SRV_EUNATCH,          // Protocol driver not attached
  RXS_SRV_ENOCSI,           // No CSI structure available
  RXS_SRV_EL2HLT,           // Level 2 halted
  RXS_SRV_EBADE,            // Invalid exchange
  RXS_SRV_EBADR,            // Invalid request descriptor
  RXS_SRV_EXFULL,           // Exchange full
  RXS_SRV_ENOANO,           // No anode
  RXS_SRV_EBADRQC,          // Invalid request code
  RXS_SRV_EBADSLT,          // Invalid slot
  RXS_SRV_EDEADLOCK,        //
  RXS_SRV_EBFONT,           // Bad font file format
  RXS_SRV_ENOSTR,           // Device not a stream
  RXS_SRV_ENODATA,          // No data available
  RXS_SRV_ETIME,            // Timer expired
  RXS_SRV_ENOSR,            // Out of streams resources
  RXS_SRV_ENONET,           // Machine is not on the network
  RXS_SRV_ENOPKG,           // Package not installed
  RXS_SRV_EREMOTE,          // Object is remote
  RXS_SRV_ENOLINK,          // Link has been severed
  RXS_SRV_EADV,             // Advertise error
  RXS_SRV_ESRMNT,           // Srmount error
  RXS_SRV_ECOMM,            // Communication error on send
  RXS_SRV_EPROTO,           // Protocol error
  RXS_SRV_EMULTIHOP,        // Multihop attempted
  RXS_SRV_EDOTDOT,          // RXS specific error
  RXS_SRV_EBADMSG,          // Not a data message
  RXS_SRV_EOVERFLOW,        // Value too large for defined data type
  RXS_SRV_ENOTUNIQ,         // Name not unique on network
  RXS_SRV_EBADFD,           // File descriptor in bad state
  RXS_SRV_EREMCHG,          // Remote address changed
  RXS_SRV_ELIBACC,          // Can not access a needed shared library
  RXS_SRV_ELIBBAD,          // Accessing a corrupted shared library
  RXS_SRV_ELIBSCN,          // .lib section in a.out corrupted
  RXS_SRV_ELIBMAX,          // Attempting to link in too many shared libraries
  RXS_SRV_ELIBEXEC,         // Cannot exec a shared library directly
  RXS_SRV_EILSEQ,           // Illegal byte sequence
  RXS_SRV_ERESTART,         // Interrupted system call should be restarted
  RXS_SRV_ESTRPIPE,         // Streams pipe error
  RXS_SRV_EUSERS,           // Too many users
  RXS_SRV_ENOTSOCK,         // Socket operation on non-socket
  RXS_SRV_EDESTADDRREQ,     // Destination address required
  RXS_SRV_EMSGSIZE,         // Message too long
  RXS_SRV_EPROTOTYPE,       // Protocol wrong type for socket
  RXS_SRV_ENOPROTOOPT,      // Protocol not available
  RXS_SRV_EPROTONOSUPPORT,  // Protocol not supported
  RXS_SRV_ESOCKTNOSUPPORT,  // Socket type not supported
  RXS_SRV_EOPNOTSUPP,       // Operation not supported on transport endpoint
  RXS_SRV_EPFNOSUPPORT,     // Protocol family not supported
  RXS_SRV_EAFNOSUPPORT,     // Address family not supported by protocol
  RXS_SRV_EADDRINUSE,       // Address already in use
  RXS_SRV_EADDRNOTAVAIL,    // Cannot assign requested address
  RXS_SRV_ENETDOWN,         // Network is down
  RXS_SRV_ENETUNREACH,      // Network is unreachable
  RXS_SRV_ENETRESET,        // Network dropped connection because of reset
  RXS_SRV_ECONNABORTED,     // Software caused connection abort
  RXS_SRV_ECONNRESET,       // Connection reset by peer
  RXS_SRV_ENOBUFS,          // No buffer space available
  RXS_SRV_EISCONN,          // Transport endpoint is already connected
  RXS_SRV_ENOTCONN,         // Transport endpoint is not connected
  RXS_SRV_ESHUTDOWN,        // Cannot send after transport endpoint shutdown
  RXS_SRV_ETOOMANYREFS,     // Too many references: cannot splice
  RXS_SRV_ETIMEDOUT,        // Connection timed out
  RXS_SRV_ECONNREFUSED,     // Connection refused
  RXS_SRV_EHOSTDOWN,        // Host is down
  RXS_SRV_EHOSTUNREACH,     // No route to host
  RXS_SRV_EALREADY,         // Operation already in progress
  RXS_SRV_EINPROGRESS,      // Operation now in progress
  RXS_SRV_ESTALE,           // Stale file handle
  RXS_SRV_EUCLEAN,          // Structure needs cleaning
  RXS_SRV_ENOTNAM,          // Not a XENIX named type file
  RXS_SRV_ENAVAIL,          // No XENIX semaphores available
  RXS_SRV_EISNAM,           // Is a named type file
  RXS_SRV_EREMOTEIO,        // Remote I/O error
  RXS_SRV_EDQUOT,           // Quota exceeded
  RXS_SRV_ENOMEDIUM,        // No medium found
  RXS_SRV_EMEDIUMTYPE,      // Wrong medium type
  RXS_SRV_ECANCELED,        // Operation Canceled
  RXS_SRV_ENOKEY,           // Required key not available
  RXS_SRV_EKEYEXPIRED,      // Key has expired
  RXS_SRV_EKEYREVOKED,      // Key has been revoked
  RXS_SRV_EKEYREJECTED,     // Key was rejected by service
  RXS_SRV_EOWNERDEAD,       // Owner died
  RXS_SRV_ENOTRECOVERABLE,  // State not recoverable
  RXS_SRV_ERFKILL,          // Operation not possible due to RF-kill
  RXS_SRV_EHWPOISON,        // Memory page has hardware error
  RXS_SRV_INTERNAL          // Internal error
} rxs_errno_t;

#endif  // _RXS_ERRNO_H
