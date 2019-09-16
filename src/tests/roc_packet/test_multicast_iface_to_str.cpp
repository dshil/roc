/*
 * Copyright (c) 2019 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_packet/address.h"
#include "roc_packet/multicast_iface_to_str.h"

namespace roc {
namespace packet {

TEST_GROUP(multicast_iface_to_str) {};

TEST(multicast_iface_to_str, invalid_address) {
    Address addr;
    CHECK(!addr.valid());

    STRCMP_EQUAL("none", multicast_iface_to_str(addr).c_str());
}

TEST(multicast_iface_to_str, ipv4_address) {
    Address addr;

    CHECK(addr.set_ipv4("225.1.2.3", 123));
    CHECK(addr.set_multicast_iface_v4("0.0.0.0"));
    CHECK(addr.valid());

    STRCMP_EQUAL("0.0.0.0", multicast_iface_to_str(addr).c_str());
}

TEST(multicast_iface_to_str, ipv6_address) {
    Address addr;

    CHECK(addr.set_ipv6("ff00::", 123));
    CHECK(addr.set_multicast_iface_v6("::"));
    CHECK(addr.valid());

    STRCMP_EQUAL("::", multicast_iface_to_str(addr).c_str());
}

} // packet namespace
} // roc namespace
