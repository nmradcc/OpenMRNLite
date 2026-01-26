/** \copyright
 * Copyright (c) 2018, Balazs Racz
 * Copyright (c) 2025 - ThreadX adaptation
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
 * \file OpenMRNLite.h
 *
 * Main include file for the OpenMRNLite library - ThreadX-only version
 * Simplified API for STM32 with ThreadX RTOS.
 *
 * @author Balazs Racz
 * @date 24 July 2018
 */

#ifndef _OPENMRNLITE_H_
#define _OPENMRNLITE_H_

#include "CDIXMLGenerator.hxx"
#include "executor/Notifiable.hxx"
#include "openlcb/SimpleStack.hxx"
#include "utils/FileUtils.hxx"
#include "utils/GridConnectHub.hxx"
#include "utils/logging.h"
#include "utils/Uninitialized.hxx"

namespace openmrn_arduino
{

/// Main class to declare the OpenMRN stack for ThreadX.
/// Create one instance of this in your application.
/// Use with FdCanBridge for CAN connectivity (see FdCanBridge.h)
class OpenMRN : private Executable
{
public:
    /// Constructor - requires Node ID
    OpenMRN(openlcb::NodeID node_id);

    /// @return pointer to the OpenMRN stack
    openlcb::SimpleCanStack *stack()
    {
        return stack_.operator->();
    }

    /// Call this function after all subsystems have been initialized
    void begin()
    {
        stack_->start_stack(false);
    }

    /// Call this function periodically to process OpenMRN events
    /// Typically called from a ThreadX thread's main loop
    void loop()
    {
        for (auto *e : loopMembers_)
        {
            e->run();
        }
    }

    /// Starts a ThreadX thread for the Executor used by OpenMRN.
    /// @param name Thread name
    /// @param priority ThreadX thread priority (0-31, lower number = higher priority)
    /// @param stack_size Stack size in bytes
    void start_executor_thread(const char *name, int priority, size_t stack_size)
    {
        haveExecutorThread_ = true;
        stack_->executor()->start_thread(name, priority, stack_size);
    }

    /// Donates the calling thread to the Executor.
    /// Note: this method will not return until the Executor has shutdown.
    void loop_executor()
    {
        haveExecutorThread_ = true;
        stack_->executor()->thread_body();
    }

#if defined(HAVE_FILESYSTEM)
    /// Creates the XML representation of the configuration structure and saves
    /// it to a file on the filesystem.
    /// @param cfg is the global configuration instance (usually called cfg).
    /// @param filename is where the xml file can be stored on the filesystem.
    /// @returns true if the cdi.xml was updated, false otherwise.
    template <class ConfigDef>
    bool create_config_descriptor_xml(
        const ConfigDef &config, const char *filename)
    {
        return CDIXMLGenerator::create_config_descriptor_xml(
            config, filename, stack());
    }
#endif // HAVE_FILESYSTEM

private:
    /// Callback from the loop() method. Internally called.
    void run() override
    {
        if (!haveExecutorThread_)
        {
            stack_->executor()->loop_some();
        }
    }

    /// Storage space for the OpenLCB stack.
    uninitialized<openlcb::SimpleCanStack> stack_;

    /// List of objects we need to call in each loop iteration.
    vector<Executable *> loopMembers_{{this}};

    /// True if there is a separate thread running the executor.
    bool haveExecutorThread_{false};
};

} // namespace openmrn_arduino

using openmrn_arduino::OpenMRN;

#endif // _OPENMRNLITE_H_
