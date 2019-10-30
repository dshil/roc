/*
 * Copyright (c) 2019 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_address/socket_addr.h"
#include "roc_core/heap_allocator.h"
#include "roc_netio/tcp_conn.h"

namespace roc {
namespace netio {

namespace {

address::SocketAddr make_address(const char* ip, int port) {
    address::SocketAddr addr;
    CHECK(addr.set_host_port_ipv4(ip, port));

    return addr;
}

core::HeapAllocator allocator;

} // namespace

TEST_GROUP(tcp_conn) {};

TEST(tcp_conn, init) {
    address::SocketAddr dst_addr = make_address("0.0.0.0", 0);

    TCPConn conn(dst_addr, "test", allocator);

    CHECK(conn.valid());
    CHECK(conn.destination() == dst_addr);
    CHECK(conn.source() != conn.destination());
}

} // namespace netio
} // namespace roc
