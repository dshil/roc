/*
 * Copyright (c) 2019 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_packet/ip_to_str.h"
#include "roc_core/log.h"

namespace roc {
namespace packet {

ip_to_str::ip_to_str(const Address& addr) {
    buffer_[0] = '\0';

    if (!addr.valid()) {
        strcpy(buffer_, "none");

        return;
    }

    if (!addr.get_ip(buffer_, sizeof(buffer_))) {
        roc_log(LogError, "ip to str: can't format ip");
    }
}

const char* ip_to_str::c_str() const {
    return buffer_;
}

} // namespace packet
} // namespace roc
