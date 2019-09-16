/*
 * Copyright (c) 2019 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_packet/address.h"
#include "roc_packet/ip_to_str.h"

namespace roc {
namespace packet {

TEST_GROUP(ip_to_str) {};

TEST(ip_to_str, invalid_address) {
    Address addr;
    CHECK(!addr.valid());

    STRCMP_EQUAL("none", ip_to_str(addr).c_str());
}

TEST(ip_to_str, ipv4_address) {
    Address addr;

    CHECK(addr.set_ipv4("1.2.0.255", 123));
    CHECK(addr.valid());

    STRCMP_EQUAL("1.2.0.255", ip_to_str(addr).c_str());
}

TEST(ip_to_str, ipv6_address) {
    Address addr;

    CHECK(addr.set_ipv6("2001:db8::1", 123));
    CHECK(addr.valid());

    STRCMP_EQUAL("2001:db8::1", ip_to_str(addr).c_str());
}

} // packet namespace
} // roc namespace
