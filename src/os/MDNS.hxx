/** @copyright
 * Copyright (c) 2017, Stuart W Baker
 * All rights reserved
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
 * @file MDNS.hxx
 *
 * A simple abstraction to publish/lookup mDNS sevices.
 *
 * @author Stuart Baker
 * @date 30 January 2017
 */

#ifndef _OS_MDNS_HXX_
#define _OS_MDNS_HXX_

#include <stdint.h>
#include <netdb.h>

struct addrinfo;

#include "utils/macros.h"


/** MDNS abstraction object.
 */
class MDNS
{
public:
    /** Constructor.
     */
    MDNS()
    {
    }

    /** Destructor.
     */
    ~MDNS()
    {
    }

    /** Publish an mDNS name.
     * @param name local "username" or "nodename" of the service
     * @param service service name, example: "_openlcb._tcp"
     * @param port port number
     */
    void publish(const char *name, const char *service, uint16_t port);

    /** Unpublish an mDNS name.
     * @param name local "username" or "nodename" of the service
     * @param service service name, example: "_openlcb._tcp"
     */
    void unpublish(const char *name, const char *service);
    
    /** Commit the mDNS publisher.
     */
    void commit()
    {
    }

    /** Lookup an mDNS name.
     * @param service servicename to lookup
     * @param hints hints about limiting the types of services that will respond
     * @param addrinfo structure containing one or more service addressess that
     *                 match the enquery, else nullptr if no matching service
     *                 found
     * @return 0 upon success, or appropriate EAI_* error on failure, use
     *         ::freeaddrinfo() to free up memory allocated to the non nullptr
     *         *addr returned
     */
    static int lookup(const char *service, struct addrinfo *hints,
                      struct addrinfo **addr);

    /** Start continuous scan for mDNS service name.
     * @param service servicename to scan
     */
    static void scan(const char *service);

private:
    DISALLOW_COPY_AND_ASSIGN(MDNS);
};

#endif /* _OS_MDNS_HXX_ */
