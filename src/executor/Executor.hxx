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
 * \file Executor.hxx
 *
 * Class to control execution of tasks that get pulled of an input queue.  This
 * is based off of work started by Balazs on 5 August 2013.
 *
 * @author Stuart W Baker and Balazs Racz
 * @date 26 October 2013
 */

#ifndef _EXECUTOR_EXECUTOR_HXX_
#define _EXECUTOR_EXECUTOR_HXX_

#include <functional>
#include <atomic>

#include "executor/Executable.hxx"
#include "executor/Notifiable.hxx"
#include "executor/Timer.hxx"
#include "executor/Selectable.hxx"
#include "utils/Queue.hxx"
#include "utils/SimpleQueue.hxx"
#include "utils/LinkedObject.hxx"
#include "utils/logging.h"
#include "utils/macros.h"

class ActiveTimers;

/** This class implements an execution of tasks pulled off an input queue.
 */
class ExecutorBase : protected OSThread, protected Executable, public LinkedObject<ExecutorBase>
{
public:
    /** Constructor.
     */
    ExecutorBase();

    /** Destructor.
     */
    ~ExecutorBase();

    /** Lookup an executor by its name.
     * @param name name of executor to lookup
     * @param wait wait forever for an executor to show up
     * @return pointer to executor upon success, else NULL if not found
     */
    static ExecutorBase *by_name(const char *name, bool wait);

    /** Send a message to this Executor's queue.
     * @param action Executable instance to insert into the input queue
     * @param priority priority of execution
     */
    virtual void add(Executable *action, unsigned priority = UINT_MAX) = 0;

    /** Synchronously runs a closure on this executor. Does not return until
     * the execution is completed. @param fn is the closure to run. */
    void sync_run(std::function<void()> fn);

#if OPENMRN_FEATURE_RTOS_FROM_ISR
    /** Send a message to this Executor's queue. Callable from interrupt
     * context.
     * @param action Executable instance to insert into the input queue
     * @param priority priority of execution
     */
    virtual void add_from_isr(Executable *action,
                              unsigned priority = UINT_MAX) = 0;
#endif // OPENMRN_FEATURE_RTOS_FROM_ISR

    /** Performs one loop of the execution on the calling thread. @return true
     * if there is more scheduled work to do. Returns false if the executor
     * loop would block right now. */
    bool loop_once();

    /** Performs a few loops of the executor on the calling thread.
     *
     * @return 0 if there is still pending work scheduled on the executor,
     * non-zero if there is some work to do after a sleep. The returned value
     * is the nanoseconds to sleep before calling loop_some again.
     */
    long long loop_some() ICACHE_FLASH_ATTR;

    /** @returns the list of active timers. */
    ActiveTimers* active_timers() { return &activeTimers_; }

    /** Terminates the executor thread. Waits until it is safe to delete the
     * executor. */
    void shutdown();

    /// @return true if there are no executables waiting on this thread to be
    /// executed. There could still be a current executable.
    virtual bool empty() = 0;

    /// @return the thread handle.
    os_thread_t thread_handle() { return OSThread::get_handle(); }

    OSThread& thread() { return *this; }
    
    /// Die if we are not on the current executor.
    void assert_current() { HASSERT(os_thread_self() == thread_handle()); }
    
    /// @return a number that gets incremented by one every time an executable
    /// runs.
    virtual uint32_t sequence() = 0;

    /// Helper function for debugging and tracing.
    /// @return currently running executable or nullptr if none active.
    Executable* current() { return current_; }

    /// Adds a Selectable to the executor's select set.
    /// @param s the Selectable to add to the select set.
    void select(Selectable *s);

    /// Checks if a Selectable is currently in the executor's select set.
    /// @param s the Selectable to check.
    /// @return true if the Selectable is in the select set.
    bool is_selected(Selectable *s);

    /// Removes a Selectable from the executor's select set.
    /// @param s the Selectable to remove from the select set.
    void unselect(Selectable *s);
    
protected:
    /** Thread entry point.
     * @return Should never return
     */
    void *entry() override;

    void run() override {}
    
private:
    /** Retrieve an item from the front of the queue.
     * @param priority pass back the priority of the queue pulled from
     * @return item retrieved from queue, else NULL if queue is empty.
     */
    virtual Executable *next(unsigned *priority) = 0;

    /** Sleep for the specified time until an executable becomes available or
     * timeout occurs.
     *
     * @param next_timer_nsec is the maximum time to sleep in nanoseconds. */
    void sleep_with_timeout(long long next_timer_nsec);

    /** name of this Executor */
    const char *name_;

    /** Currently executing closure. USeful for debugging crashes. */
    Executable* volatile current_;

    /** List of active timers. */
    ActiveTimers activeTimers_;

    /** Set to 1 when the executor thread has exited and it is safe to delete
     * *this. */
    std::atomic_uint_least8_t done_;
    /// 1 if the executor is already running
    std::atomic_uint_least8_t started_;
    /// How many executables we schedule blindly before calling a select() in
    /// order to find more data to read/write in the FDs being waited upon.
    unsigned selectPrescaler_ : 5;

    /// File descriptor set for select() read operations.
    fd_set selectRead_;
    /// File descriptor set for select() write operations.
    fd_set selectWrite_;
    /// File descriptor set for select() exception operations.
    fd_set selectExcept_;
    /// Number of file descriptors to monitor in select().
    int selectNFds_;

protected:
    /// Sequence number.
    volatile unsigned sequence_ : 25;
    
    /** provide access to Executor::send method. */
    friend class Service;

    DISALLOW_COPY_AND_ASSIGN(ExecutorBase);
};

/** This is an empty struct. If you give it as an argument to the executor
 * constructor, the executor will be created without a thread. The owner is
 * responsible for "donating" a thread (typically the main thread) to that
 * executor. See @ref Executor::thread_body() */
class NO_THREAD {
public:
    NO_THREAD() {}
};

/// Implementation the ExecutorBase with a specific number of priority
/// bands. The memory usage and scheduling cost is proportional to the number
/// of priority bands, so it should be kept pretty low.
template <unsigned NUM_PRIO>
class Executor : public ExecutorBase
{
public:

    /** Constructor.
     * @param name name of executor
     * @param priority thread priority
     * @param stack_size thread stack size
     */
    Executor(const char *name, int priority, size_t stack_size)
    {
        start_thread(name, priority, stack_size);
    }

    /// Constructor that does not create a thread for running the executor. The
    /// owner should later create a thread to this executor by calling the
    /// start_thread() function or donate a thread by calling thread_body()
    /// function.
    /// @param unused unused -- just here for polymorphic disalbiguation.
    explicit Executor(const NO_THREAD& unused) {}

    /// Creates a new thread for running this executor.
    ///
    /// @param name thread name (passed to OS)
    /// @param priority thread priority (0 == default prio)
    /// @param stack_size number of bytes to allocate for the thread stack; used
    /// only for FreeRTOS and ignored on linux etc.
    ///
    void start_thread(const char *name, int priority, size_t stack_size)
    {
        OSThread::start(name, priority, stack_size);
    }

    /** Destructor.
     */
    ~Executor();

    /** Send a message to this Executor's queue.
     * @param msg Executable instance to insert into the input queue
     * @param priority priority of message
     */
    void add(Executable *msg, unsigned priority = UINT_MAX) OVERRIDE
    {
        queue_.insert(
            msg, priority >= NUM_PRIO ? NUM_PRIO - 1 : priority);
    }

#if OPENMRN_FEATURE_RTOS_FROM_ISR
    /** Send a message to this Executor's queue. Callable from interrupt
     * context.
     * @param msg Executable instance to insert into the input queue
     * @param priority priority of message
     */
    void add_from_isr(Executable *msg, unsigned priority = UINT_MAX) override
    {
        queue_.insert_locked(
            msg, priority >= NUM_PRIO ? NUM_PRIO - 1 : priority);
    }
#endif // OPENMRN_FEATURE_RTOS_FROM_ISR

    /** If the executor was created with NO_THREAD, then this function needs to
     * be called to run the executor loop. It will exit when the execut gets
     * shut down. Useful for having an executor loop run in the main thread. */
    void thread_body() {
        inherit();
    }

    /// @return true if there are no executables waiting on this thread to be
    /// executed. There could still be a current executable.
    bool empty() OVERRIDE
    {
        return queue_.empty();
    }

    uint32_t sequence() OVERRIDE { return sequence_; }

private:
    /** Retrieve an item from the front of the queue.
     * @param priority pass back the priority of the queue pulled from
     * @return item retrieved from queue, else NULL if none waiting.
     */
    Executable *next(unsigned *priority) OVERRIDE
    {
        auto result = queue_.next();
        *priority = result.index;
        return static_cast<Executable*>(result.item);
    }

    /** Default Constructor.
     */
    Executor();

    DISALLOW_COPY_AND_ASSIGN(Executor);

    /// Internal queue of executables waiting to be scheduled.
    QListProtected<NUM_PRIO> queue_;
};

/** This class can be given an executor, and will notify itself when that
 *   executor is out of work. Callers can pend on the sync notifiable to wait
 *   for that. */
class ExecutorGuard : private ::Timer, public SyncNotifiable
{
public:
    /// Constructor. @param e is the executor to look for being empty.
    ExecutorGuard(ExecutorBase *e)
        : ::Timer(e->active_timers())
        , executor_(e)
    {
        // We wait on the front of the timer queue by expiring immediately.
        start();
    }

    /// Implementation of the guard functionality. Called on the executor.
    long long timeout() override {
        if (executor_->empty()) {
            SyncNotifiable::notify();
            return NONE;
        } else {
            return RESTART;  // wait more on the front of the timer queue
        }
    }

private:
    /// Parent.
    ExecutorBase* executor_;
};

template <unsigned NUM_PRIO>
/** Destructs the executor. Waits for the executor to run out of work first. */
Executor<NUM_PRIO>::~Executor()
{
    shutdown();
}

#endif /* _EXECUTOR_EXECUTOR_HXX_ */
