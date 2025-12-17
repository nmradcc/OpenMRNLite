/** \copyright
* Copyright (c) 2015, Balazs Racz
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
* \file OSSelectWakeup.cxx
* Helper class for portable wakeup of a thread blocked in a select call.
*
* @author Balazs Racz
* @date 10 Apr 2015
*/

#include "os/OSSelectWakeup.hxx"
#include "utils/logging.h"

void empty_signal_handler(int)
{
}

int OSSelectWakeup::select(int nfds, fd_set *readfds,
                           fd_set *writefds, fd_set *exceptfds,
                           long long deadline_nsec)
{
    {
        AtomicHolder l(this);
        inSelect_ = true;
        if (pendingWakeup_)
        {
            deadline_nsec = 0;
        }
        else
        {
#if OPENMRN_FEATURE_DEVICE_SELECT
            // It is important that we do this select_clear() in the same
            // critical section as checking pendingWakeup above. This ensures
            // tht all wakeups before this clear cause sleep to be zero, and
            // all wakeups after this clear will cause the event group bit to
            // be set.
            Device::select_clear();
#endif
        }
    }
#if OPENMRN_FEATURE_DEVICE_SELECT
    int ret =
        Device::select(nfds, readfds, writefds, exceptfds, deadline_nsec);
    if (!ret && pendingWakeup_)
    {
        ret = -1;
        errno = EINTR;
    }
#elif OPENMRN_HAVE_PSELECT
    struct timespec timeout;
    timeout.tv_sec = deadline_nsec / 1000000000;
    timeout.tv_nsec = deadline_nsec % 1000000000;
    int ret =
        ::pselect(nfds, readfds, writefds, exceptfds, &timeout, &origMask_);
#elif OPENMRN_HAVE_SELECT
    struct timeval timeout;
    timeout.tv_sec = (deadline_nsec / 1000) / 1000000LL;
    timeout.tv_usec = (deadline_nsec / 1000) % 1000000LL;
    int ret =
        ::select(nfds, readfds, writefds, exceptfds, &timeout);
#elif !defined(OPENMRN_FEATURE_SINGLE_THREADED)
    #error no select implementation in multi threaded OS.
#else
    // Single threaded OS: nothing to wake up.
    int ret = 0;
#endif
    {
        AtomicHolder l(this);
        pendingWakeup_ = false;
        inSelect_ = false;
    }
    return ret;
}
