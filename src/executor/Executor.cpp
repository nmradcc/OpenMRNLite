/** \copyright
 * Copyright (c) 2013, Stuart W Baker and Balazs Racz
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
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
 * \file Executor.cxx
 *
 * Class to control execution of tasks that get pulled of an input queue.  This
 * is based off of work started by Balazs on 5 August 2013.
 *
 * @author Stuart W Baker and Balazs Racz
 * @date 26 October 2013
 */

#define _DEFAULT_SOURCE

#include "executor/Executor.hxx"

#include "openmrn_features.h"
#include <unistd.h>

#include <sys/select.h>

#include "executor/Service.hxx"
#include "nmranet_config.h"

void __attribute__((weak,noinline)) Executable::test_deletion() {} 

Executable::~Executable() {
    test_deletion();
}


/** Constructor.
 */
ExecutorBase::ExecutorBase()
    : name_(NULL) /** @todo (Stuart Baker) is "name" still in use? */
    , activeTimers_(this)
    , done_(0)
    , started_(0)
    , selectPrescaler_(0)
{
    FD_ZERO(&selectRead_);
    FD_ZERO(&selectWrite_);
    FD_ZERO(&selectExcept_);
    selectNFds_ = 0;
}

/** Lookup an executor by its name.
 * @param name name of executor to lookup
 * @return pointer to executor upon success, else NULL if not found
 */
ExecutorBase *ExecutorBase::by_name(const char *name, bool wait)
{
    /** @todo (Stuart Baker) we need a locking mechanism here to protect
     *  the list.
     */
    for (; /* forever */;)
    {
        {
            AtomicHolder hld(head_mu());
            ExecutorBase *current = head_;
            while (current)
            {
                if (!strcmp(name, current->name_))
                {
                    return current;
                }
                current = current->link_next();
            }
        }
        if (wait)
        {
            sleep(1);
        }
        else
        {
            return NULL;
        }
    }
}

/// An Executable that runs a callback on the executor and returns once the run
/// is complete. Must not be created against the local executor (because that
/// would deterministically deadlock).
///
/// Usage:
///
/// SyncExecutable(stack.executor(), [] { DoFooBar(); });
class SyncExecutable : public Executable
{
public:
    /// @param e is the executor on which to execute the callback
    /// @param fn is the callback to execute. Caller should use std::move to
    /// get the callback in here.
    SyncExecutable(ExecutorBase *e, std::function<void()>&& fn)
        : fn_(std::move(fn))
    {
        e->add(this);
        n_.wait_for_notification();
    }

    void run() OVERRIDE
    {
        fn_();
        n_.notify();
    }
    /// Callback to run.
    std::function<void()> fn_;
    /// Blocks the calling thread until the callback is done running.
    SyncNotifiable n_;
};

void ExecutorBase::sync_run(std::function<void()> fn)
{
    if (os_thread_self() == thread_handle())
    {
        // run inline.
        fn();
    }
    else
    {
        // run externally and block.
        SyncExecutable(this, std::move(fn));
    }
}

bool ExecutorBase::loop_once()
{
    ScopedSetThreadHandle h(this);
    unsigned priority;
    activeTimers_.get_next_timeout();
    Executable* msg = next(&priority);
    if (!msg)
    {
        return false;
    }
    if (msg == this)
    {
        // exit closure
        done_ = 1;
        return false;
    }
    current_ = msg;
    msg->run();
    current_ = nullptr;
    return true;
}

long long ICACHE_FLASH_ATTR  ExecutorBase::loop_some() {
    ScopedSetThreadHandle h(this);
    for (int i = 12; i > 0; --i) {
        Executable *msg = nullptr;
        unsigned priority = UINT_MAX;
        long long wait_length = activeTimers_.get_next_timeout();
        if (empty()) {
            return wait_length;
        }
        msg = next(&priority);
        if (msg == this)
        {
            // exit closure
            done_ = 1;
            return INT64_MAX;
        }
        if (msg != NULL)
        {
            current_ = msg;
            msg->run();
            current_ = nullptr;
        }
    }
    // Still stuff pending to run.
    return 0;
}

/** Thread entry point.
 * @return Should never return
 */
void *ExecutorBase::entry()
{
    started_ = 1;
    sequence_ = 0;
    printf("[EXECUTOR] Entry function started, thread handle = %p\r\n", (void*)OSThread::get_handle());
    uint32_t loop_count = 0;
    fflush(stdout);
    /* wait for messages to process */
    for (; /* forever */;)
    {
        Executable *msg = nullptr;
        unsigned priority = UINT_MAX;
        if (!selectPrescaler_ || ((msg = next(&priority)) == nullptr))
        {
            long long wait_length = activeTimers_.get_next_timeout();
            if (++loop_count % 50 == 0)
            {
                printf("[EXECUTOR] Loop %lu: seq=%d, timers_empty=%d, timeout=%lld ns\r\n", 
                       (unsigned long)loop_count, sequence_, (int)activeTimers_.empty(), wait_length);
                fflush(stdout);
            }
            sleep_with_timeout(wait_length);
            selectPrescaler_ = config_executor_select_prescaler();
            msg = next(&priority);
        }
        else
        {
            --selectPrescaler_;
        }
        if (msg == this)
        {
            // exit closure
            done_ = 1;
            return NULL;
        }
        if (msg != NULL)
        {
            ++sequence_;
            current_ = msg;
            msg->run();
            current_ = nullptr;
        }
    }

    return NULL;
}

void ExecutorBase::sleep_with_timeout(long long wait_length)
{
    // Convert nanoseconds to milliseconds for ThreadX sleep
    if (wait_length < 0) {
        // Negative value means sleep indefinitely
        wait_length = OPENMRN_OS_WAIT_FOREVER;
    } else {
        wait_length = NSEC_TO_MSEC(wait_length);
        long long max_sleep = config_executor_max_sleep_msec();
        if (wait_length > max_sleep)
        {
            wait_length = max_sleep;
        }
    }
    
    // ThreadX-based sleep: just yield to allow other threads to run
    // Timers are handled by the main loop checking activeTimers_
    if (!empty()) {
        // If there's work to do, don't sleep
        return;
    }
    
    // Sleep for the specified timeout
    if (wait_length == OPENMRN_OS_WAIT_FOREVER) {
        sleep(1000);  // Sleep for 1 second at a time to be responsive
    } else if (wait_length > 0) {
        usleep(wait_length * 1000);  // Convert ms to microseconds
    }
}

void ExecutorBase::shutdown()
{
    if (!started_) return;
    add(this);
    
    while (!done_)
    {
        usleep(100);
    }
}

/** Adds a Selectable to the executor's select set.
 * @param s the Selectable to add to the select set.
 */
void ExecutorBase::select(Selectable *s)
{
    HASSERT(s != nullptr);
    HASSERT(s->fd() <= Selectable::MAX_FD);
    
    int fd = s->fd();
    if (fd >= selectNFds_)
    {
        selectNFds_ = fd + 1;
    }
    
    switch (s->type())
    {
        case Selectable::READ:
            FD_SET(fd, &selectRead_);
            break;
        case Selectable::WRITE:
            FD_SET(fd, &selectWrite_);
            break;
        case Selectable::EXCEPT:
            FD_SET(fd, &selectExcept_);
            break;
        default:
            break;
    }
}

/** Checks if a Selectable is currently in the executor's select set.
 * @param s the Selectable to check.
 * @return true if the Selectable is in the select set.
 */
bool ExecutorBase::is_selected(Selectable *s)
{
    HASSERT(s != nullptr);
    
    int fd = s->fd();
    if (fd >= selectNFds_)
    {
        return false;
    }
    
    switch (s->type())
    {
        case Selectable::READ:
            return FD_ISSET(fd, &selectRead_) ? true : false;
        case Selectable::WRITE:
            return FD_ISSET(fd, &selectWrite_) ? true : false;
        case Selectable::EXCEPT:
            return FD_ISSET(fd, &selectExcept_) ? true : false;
        default:
            return false;
    }
}

/** Removes a Selectable from the executor's select set.
 * @param s the Selectable to remove from the select set.
 */
void ExecutorBase::unselect(Selectable *s)
{
    HASSERT(s != nullptr);
    
    int fd = s->fd();
    if (fd >= selectNFds_)
    {
        return;  // Not in set
    }
    
    switch (s->type())
    {
        case Selectable::READ:
            FD_CLR(fd, &selectRead_);
            break;
        case Selectable::WRITE:
            FD_CLR(fd, &selectWrite_);
            break;
        case Selectable::EXCEPT:
            FD_CLR(fd, &selectExcept_);
            break;
        default:
            break;
    }
}

ExecutorBase::~ExecutorBase()
{
    if (!done_)
    {
        shutdown();
    }
}
