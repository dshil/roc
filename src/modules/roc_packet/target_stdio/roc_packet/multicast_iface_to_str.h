/*
 * Copyright (c) 2019 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_packet/target_stdio/roc_packet/multicast_iface_to_str.h
//! @brief Convert multicast interface address to string.

#ifndef ROC_PACKET_MULTICAST_IFACE_TO_STR_H_
#define ROC_PACKET_MULTICAST_IFACE_TO_STR_H_

#include "roc_core/noncopyable.h"
#include "roc_packet/address.h"

namespace roc {
namespace packet {

//! Format multicast interface address.
class multicast_iface_to_str : public core::NonCopyable<> {
public:
    //! Construct.
    explicit multicast_iface_to_str(const Address&);

    //! Get formatted multicast interface string.
    const char* c_str() const;

private:
    char buffer_[256];
};

} // namespace packet
} // namespace roc

#endif // ROC_PACKET_MULTICAST_IFACE_TO_STR_H_
