/*
 * Copyright (c) 2019 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_packet/target_stdio/roc_packet/ip_to_str.h
//! @brief Convert ip address to string.

#ifndef ROC_PACKET_IP_TO_STR_H_
#define ROC_PACKET_IP_TO_STR_H_

#include "roc_core/noncopyable.h"
#include "roc_packet/address.h"

namespace roc {
namespace packet {

//! Format ip string of network address.
class ip_to_str : public core::NonCopyable<> {
public:
    //! Construct.
    explicit ip_to_str(const Address&);

    //! Get formatted ip string.
    const char* c_str() const;

private:
    char buffer_[256];
};

} // namespace packet
} // namespace roc

#endif // ROC_PACKET_IP_TO_STR_H_
