/** \copyright
 * Copyright (c) 2023, Balazs Racz
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are  permitted provided that the following conditions are met:
 *
 *  - Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 *  - Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * \file FdUtils.cxx
 *
 * Helper functions for dealing with posix fds.
 *
 * @author Balazs Racz
 * @date 29 Dec 2023
 */

#include "FdUtils.hxx"

#include "nmranet_config.h"

/// Performs a system call on an fd. If an error is returned, prints the error
/// using the log mechanism, but otherwise ignores it.
/// @param where user-readable text printed with the error, e.g. "setsockopt"
/// @param callfn system call, like ::setsockopt
/// @param fd file descriptor (int)
/// @param args... all other arguments to callfn
#define PCALL_LOGERR(where, callfn, fd, args...)                               \
    do                                                                         \
    {                                                                          \
        int ret = callfn(fd, args);                                            \
        if (ret < 0)                                                           \
        {                                                                      \
            char buf[256];                                                     \
            strerror_r(errno, buf, sizeof(buf));                               \
            LOG_ERROR("fd %d %s: %s", fd, where, buf);                         \
        }                                                                      \
    } while (0)

/// Optimizes the kernel settings like socket and TCP options for an fd
/// that is an outgoing TCP socket.
/// @param fd socket file descriptor.
void FdUtils::optimize_socket_fd(int fd)
{
}

/// Sets the kernel settings like queuing and terminal settings for an fd
/// that is an outgoing tty.
/// @param fd tty file descriptor.
void FdUtils::optimize_tty_fd(int fd)
{
}

/// For an fd that is an outgoing link, detects what kind of file
/// descriptor this is and calls the appropriate optimize call for it.
void FdUtils::optimize_fd(int fd)
{
}
