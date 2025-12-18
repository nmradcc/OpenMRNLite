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
    if (os_thread_self() == selectHelper_.main_thread())
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
    selectHelper_.lock_to_thread();
    /* wait for messages to process */
    for (; /* forever */;)
    {
        Executable *msg = nullptr;
        unsigned priority = UINT_MAX;
        if (!selectPrescaler_ || ((msg = next(&priority)) == nullptr))
        {
            long long wait_length = activeTimers_.get_next_timeout();
            wait_with_select(wait_length);
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

void ExecutorBase::select(Selectable *job)
{
    fd_set *s = get_select_set(job->type());
    int fd = job->fd_;
    if (FD_ISSET(fd, s))
    {
        LOG(FATAL,
            "Multiple Selectables are waiting for the same fd %d type %u", fd,
            job->selectType_);
    }
    FD_SET(fd, s);
    if (fd >= selectNFds_)
    {
        selectNFds_ = fd + 1;
    }
    HASSERT(!job->next);
    // Inserts the job into the select queue.
    selectables_.push_front(job);
}

bool ExecutorBase::is_selected(Selectable *job)
{
    fd_set *s = get_select_set(job->type());
    int fd = job->fd_;
    return FD_ISSET(fd, s);
}

void ExecutorBase::unselect(Selectable *job)
{
    fd_set *s = get_select_set(job->type());
    int fd = job->fd_;
    if (!FD_ISSET(fd, s))
    {
        LOG(FATAL, "Tried to remove a non-active selectable: fd %d type %u", fd,
            job->selectType_);
    }
    FD_CLR((unsigned)fd, s);
    auto it = selectables_.begin();
    unsigned max_fd = 0;
    while (it != selectables_.end())
    {
        if (&*it == job)
        {
            selectables_.erase(it);
            continue;
        }
        max_fd = std::max(max_fd, it->fd_ + 1U);
        ++it;
    }
    selectNFds_ = max_fd;
}

void ExecutorBase::wait_with_select(long long wait_length)
{
    fd_set fd_r(selectRead_);
    fd_set fd_w(selectWrite_);
    fd_set fd_x(selectExcept_);
    // We will check the queue for any prior wakeups after this call. If we
    // already processed the executables, the wakeup is not necessary. Without
    // this clear, there would always be two select() iterations happening when
    // we are done with work and can go to sleep.
    selectHelper_.clear_wakeup();
    if (!empty())
    {
        wait_length = 0;
    }
    long long max_sleep = MSEC_TO_NSEC(config_executor_max_sleep_msec());
    if (wait_length > max_sleep)
    {
        wait_length = max_sleep;
    }
    int ret = selectHelper_.select(selectNFds_, &fd_r, &fd_w, &fd_x, wait_length);
    if (ret <= 0) {
        return; // nothing to do
    }
    unsigned max_fd = 0;
    for (auto it = selectables_.begin(); it != selectables_.end();) {
        fd_set* s = nullptr;
        fd_set* os = get_select_set(it->type());
        switch(it->type()) {
        case Selectable::READ: s = &fd_r; break;
        case Selectable::WRITE: s = &fd_w; break;
        case Selectable::EXCEPT: s = &fd_x; break;
        }
        if (FD_ISSET(it->fd_, s)) {
            add(it->wakeup_, it->priority_);
            FD_CLR(it->fd_, os);
            selectables_.erase(it);
            continue;
        }
        max_fd = std::max(max_fd, it->fd_ + 1U);
        ++it;
    }
    selectNFds_ = max_fd;
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

ExecutorBase::~ExecutorBase()
{
    if (!done_)
    {
        shutdown();
    }
}
