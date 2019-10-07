/*
 * Copyright (c) 2018 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include <string.h>

#include "roc/address.h"

namespace roc {

TEST_GROUP(address) {};

TEST(address, ipv4) {
    char buf[16];
    memset(buf, 0xff, sizeof(buf));

    roc_address addr;
    LONGS_EQUAL(0, roc_address_init(&addr, ROC_AF_IPv4, "1.2.3.4", 123));

    LONGS_EQUAL(ROC_AF_IPv4, roc_address_family(&addr));
    STRCMP_EQUAL("1.2.3.4", roc_address_ip(&addr, buf, sizeof(buf)));
    STRCMP_EQUAL("1.2.3.4", buf);
    LONGS_EQUAL(123, roc_address_port(&addr));
}

TEST(address, ipv6) {
    char buf[16];
    memset(buf, 0xff, sizeof(buf));

    roc_address addr;
    LONGS_EQUAL(0, roc_address_init(&addr, ROC_AF_IPv6, "2001:db8::1", 123));

    LONGS_EQUAL(ROC_AF_IPv6, roc_address_family(&addr));
    STRCMP_EQUAL("2001:db8::1", roc_address_ip(&addr, buf, sizeof(buf)));
    STRCMP_EQUAL("2001:db8::1", buf);
    LONGS_EQUAL(123, roc_address_port(&addr));
}

TEST(address, detect) {
    roc_address addr;

    LONGS_EQUAL(0, roc_address_init(&addr, ROC_AF_AUTO, "1.2.3.4", 123));
    LONGS_EQUAL(ROC_AF_IPv4, roc_address_family(&addr));

    LONGS_EQUAL(0, roc_address_init(&addr, ROC_AF_AUTO, "2001:db8::1", 123));
    LONGS_EQUAL(ROC_AF_IPv6, roc_address_family(&addr));
}

TEST(address, bad_args) {
    char buf[16];

    roc_address good_addr;
    LONGS_EQUAL(0, roc_address_init(&good_addr, ROC_AF_AUTO, "1.2.3.4", 123));

    roc_address bad_addr;
    memset(&bad_addr, 0, sizeof(bad_addr));

    LONGS_EQUAL(-1, roc_address_init(NULL, ROC_AF_AUTO, "1.2.3.4", 123));
    LONGS_EQUAL(-1, roc_address_init(&bad_addr, ROC_AF_INVALID, "1.2.3.4", 123));
    LONGS_EQUAL(-1, roc_address_init(&bad_addr, ROC_AF_AUTO, NULL, 123));
    LONGS_EQUAL(-1, roc_address_init(&bad_addr, ROC_AF_AUTO, "bad", 123));
    LONGS_EQUAL(-1, roc_address_init(&bad_addr, ROC_AF_AUTO, "1.2.3.4", -1));
    LONGS_EQUAL(-1, roc_address_init(&bad_addr, ROC_AF_AUTO, "1.2.3.4", 65536));
    LONGS_EQUAL(-1, roc_address_init(&bad_addr, ROC_AF_IPv4, "2001:db8::1", 123));
    LONGS_EQUAL(-1, roc_address_init(&bad_addr, ROC_AF_IPv6, "1.2.3.4", 123));

    LONGS_EQUAL(ROC_AF_INVALID, roc_address_family(NULL));
    LONGS_EQUAL(ROC_AF_INVALID, roc_address_family(&bad_addr));

    CHECK(!roc_address_ip(NULL, buf, 8));
    CHECK(!roc_address_ip(&good_addr, NULL, 8));
    CHECK(!roc_address_ip(&good_addr, buf, 7));
    CHECK(roc_address_ip(&good_addr, buf, 8));

    LONGS_EQUAL(-1, roc_address_port(NULL));
    LONGS_EQUAL(-1, roc_address_port(&bad_addr));
}

TEST(address, multicast_ipv4) {
    char buf[16];
    roc_address addr;

    LONGS_EQUAL(0, roc_address_init(&addr, ROC_AF_IPv4, "225.1.2.3", 123));

    CHECK(!roc_address_multicast_interface(&addr, buf, sizeof(buf)));

    LONGS_EQUAL(0, roc_address_set_multicast_interface(&addr, ROC_AF_IPv4, "0.0.0.0"));
    STRCMP_EQUAL("0.0.0.0", roc_address_multicast_interface(&addr, buf, sizeof(buf)));
}

TEST(address, multicast_ipv6) {
    char buf[128];
    roc_address addr;

    LONGS_EQUAL(0, roc_address_init(&addr, ROC_AF_IPv6, "ffee::", 123));

    CHECK(!roc_address_multicast_interface(&addr, buf, sizeof(buf)));

    LONGS_EQUAL(0, roc_address_set_multicast_interface(&addr, ROC_AF_IPv6, "::"));
    STRCMP_EQUAL("::", roc_address_multicast_interface(&addr, buf, sizeof(buf)));
}

TEST(address, bad_multicast) {
    { // set NULL multicast interface to NULL address
        LONGS_EQUAL(-1, roc_address_set_multicast_interface(NULL, ROC_AF_IPv4, NULL));
    }

    { // set multicast interface to NULL address
        LONGS_EQUAL(-1,
                    roc_address_set_multicast_interface(NULL, ROC_AF_IPv4, "1.2.3.4"));
    }
    { // set NULL multicast interface
        roc_address addr;
        LONGS_EQUAL(0, roc_address_init(&addr, ROC_AF_IPv4, "1.2.3.4", 123));
        LONGS_EQUAL(-1, roc_address_set_multicast_interface(&addr, ROC_AF_IPv4, NULL));
    }
    { // set IPv4 multicast interface to non-multicast address
        roc_address addr;
        LONGS_EQUAL(0, roc_address_init(&addr, ROC_AF_IPv4, "1.2.3.4", 123));
        LONGS_EQUAL(-1,
                    roc_address_set_multicast_interface(&addr, ROC_AF_IPv4, "0.0.0.0"));
    }
    { // set IPv6 multicast interface to non-multicast address
        roc_address addr;
        LONGS_EQUAL(0, roc_address_init(&addr, ROC_AF_IPv6, "2001:db8::1", 123));
        LONGS_EQUAL(-1, roc_address_set_multicast_interface(&addr, ROC_AF_IPv6, "::"));
    }
    { // set IPv4 multicast interface to IPv6 network address
        roc_address addr;
        LONGS_EQUAL(0, roc_address_init(&addr, ROC_AF_IPv6, "ff00::", 123));
        LONGS_EQUAL(-1,
                    roc_address_set_multicast_interface(&addr, ROC_AF_IPv4, "0.0.0.0"));
    }
    { // set IPv6 multicast interface to IPv4 network address
        roc_address addr;
        LONGS_EQUAL(0, roc_address_init(&addr, ROC_AF_IPv4, "225.1.2.3", 123));
        LONGS_EQUAL(-1, roc_address_set_multicast_interface(&addr, ROC_AF_IPv4, "::"));
    }
}

} // namespace roc
