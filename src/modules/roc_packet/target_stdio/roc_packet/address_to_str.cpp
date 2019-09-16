/*
 * Copyright (c) 2015 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <stdio.h>

#include "roc_core/log.h"
#include "roc_packet/address_to_str.h"
#include "roc_packet/ip_to_str.h"
#include "roc_packet/multicast_iface_to_str.h"

namespace roc {
namespace packet {

address_to_str::address_to_str(const Address& addr) {
    buffer_[0] = '\0';

    switch (addr.version()) {
    case 4: {
        if (addr.has_multicast_iface()) {
            if (snprintf(buffer_, sizeof(buffer_), "%s@%s:%d", ip_to_str(addr).c_str(),
                         multicast_iface_to_str(addr).c_str(), addr.port())
                < 0) {
                roc_log(LogError, "address to str: can't format address");
            }
        } else {
            if (snprintf(buffer_, sizeof(buffer_), "%s:%d", ip_to_str(addr).c_str(),
                         addr.port())
                < 0) {
                roc_log(LogError, "address to str: can't format address");
            }
        }

        break;
    }
    case 6: {
        if (addr.has_multicast_iface()) {
            if (snprintf(buffer_, sizeof(buffer_), "[%s]@[%s]:%d",
                         ip_to_str(addr).c_str(), multicast_iface_to_str(addr).c_str(),
                         addr.port())
                < 0) {
                roc_log(LogError, "address to str: can't format address");
            }
        } else {
            if (snprintf(buffer_, sizeof(buffer_), "[%s]:%d", ip_to_str(addr).c_str(),
                         addr.port())
                < 0) {
                roc_log(LogError, "address to str: can't format address");
            }
        }

        break;
    }
    default:
        strcpy(buffer_, "none");
        break;
    }
}

} // namespace packet
} // namespace roc
