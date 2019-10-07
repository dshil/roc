/*
 * Copyright (c) 2018 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "private.h"

#include "roc_core/stddefs.h"
#include "roc_packet/address.h"

using namespace roc;

namespace {

const char* address_payload(const roc_address* address) {
    return address->private_data.payload;
}

char* address_payload(roc_address* address) {
    return address->private_data.payload;
}

} // namespace

const packet::Address& get_address(const roc_address* address) {
    return *(const packet::Address*)address_payload(address);
}

packet::Address& get_address(roc_address* address) {
    return *(packet::Address*)address_payload(address);
}

int roc_address_init(roc_address* address, roc_family family, const char* ip, int port) {
    if (sizeof(roc_address) < sizeof(packet::Address)) {
        return -1;
    }

    if (!address) {
        return -1;
    }

    if (!ip) {
        return -1;
    }

    if (port < 0 || port > USHRT_MAX) {
        return -1;
    }

    packet::Address& pa = *new (address_payload(address)) packet::Address;

    if (family == ROC_AF_AUTO || family == ROC_AF_IPv4) {
        if (pa.set_ipv4(ip, port)) {
            return 0;
        }
    }

    if (family == ROC_AF_AUTO || family == ROC_AF_IPv6) {
        if (pa.set_ipv6(ip, port)) {
            return 0;
        }
    }

    return -1;
}

int roc_address_set_multicast_interface(roc_address* address,
                                        roc_family family,
                                        const char* iface) {
    if (!address) {
        return -1;
    }

    if (!iface) {
        return -1;
    }

    packet::Address& pa = get_address(address);

    if (!pa.multicast()) {
        return -1;
    }

    if (family == ROC_AF_AUTO || family == ROC_AF_IPv4) {
        if (pa.set_multicast_iface_v4(iface)) {
            if (pa.version() == 6) {
                return -1;
            }

            return 0;
        }
    }

    if (family == ROC_AF_AUTO || family == ROC_AF_IPv6) {
        if (pa.set_multicast_iface_v6(iface)) {
            if (pa.version() == 4) {
                return -1;
            }

            return 0;
        }
    }

    return -1;
}

roc_family roc_address_family(const roc_address* address) {
    if (!address) {
        return ROC_AF_INVALID;
    }

    const packet::Address& pa = get_address(address);

    switch (pa.version()) {
    case 4:
        return ROC_AF_IPv4;
    case 6:
        return ROC_AF_IPv6;
    default:
        break;
    }

    return ROC_AF_INVALID;
}

const char* roc_address_ip(const roc_address* address, char* buf, size_t bufsz) {
    if (!address) {
        return NULL;
    }

    if (!buf) {
        return NULL;
    }

    const packet::Address& pa = get_address(address);

    if (!pa.get_ip(buf, bufsz)) {
        return NULL;
    }

    return buf;
}

int roc_address_port(const roc_address* address) {
    if (!address) {
        return -1;
    }

    const packet::Address& pa = get_address(address);

    int port = pa.port();
    if (port < 0) {
        return -1;
    }

    return port;
}

const char*
roc_address_multicast_interface(const roc_address* address, char* buf, size_t bufsz) {
    if (!address) {
        return NULL;
    }

    if (!buf) {
        return NULL;
    }

    const packet::Address& pa = get_address(address);

    if (!pa.has_multicast_iface()) {
        return NULL;
    }

    if (!pa.get_multicast_iface(buf, bufsz)) {
        return NULL;
    }

    return buf;
}
