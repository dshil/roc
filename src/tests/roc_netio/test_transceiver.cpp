/*
 * Copyright (c) 2015 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_address/socket_addr.h"
#include "roc_core/atomic.h"
#include "roc_core/buffer_pool.h"
#include "roc_core/cond.h"
#include "roc_core/heap_allocator.h"
#include "roc_core/mutex.h"
#include "roc_core/shared_ptr.h"
#include "roc_netio/iconn_acceptor.h"
#include "roc_netio/iconn_notifier.h"
#include "roc_netio/tcp_conn.h"
#include "roc_netio/transceiver.h"
#include "roc_packet/concurrent_queue.h"
#include "roc_packet/packet_pool.h"

namespace roc {
namespace netio {

namespace {

enum { MaxBufSize = 500 };

core::HeapAllocator allocator;
core::BufferPool<uint8_t> buffer_pool(allocator, MaxBufSize, true);
packet::PacketPool packet_pool(allocator, true);

address::SocketAddr make_address(const char* ip, int port) {
    address::SocketAddr addr;
    CHECK(addr.set_host_port_ipv4(ip, port));
    return addr;
}

class TestConnNotifier : public IConnNotifier {
public:
    TestConnNotifier()
        : cond_(mutex_)
        , connected_(false) {
    }

    virtual void notify_connected() {
        core::Mutex::Lock lock(mutex_);

        connected_ = true;
        cond_.broadcast();
    }

    virtual void notify_readable() {
    }

    virtual void notify_writable() {
    }

    void wait_connected() {
        core::Mutex::Lock lock(mutex_);

        while (!connected_) {
            cond_.wait();
        }
    }

private:
    core::Mutex mutex_;
    core::Cond cond_;

    bool connected_;
};

class TestConnAcceptor : public IConnAcceptor {
public:
    TestConnAcceptor()
        : size_(0) {
    }

    virtual IConnNotifier* accept(TCPConn&) {
        ++size_;

        return &conn_notifier_;
    }

    size_t size() const {
        return (size_t)size_;
    }

private:
    core::Atomic size_;
    TestConnNotifier conn_notifier_;
};

} // namespace

TEST_GROUP(transceiver) {};

TEST(transceiver, init) {
    Transceiver trx(packet_pool, buffer_pool, allocator);

    CHECK(trx.valid());
}

TEST(transceiver, udp_bind_any) {
    packet::ConcurrentQueue queue;

    Transceiver trx(packet_pool, buffer_pool, allocator);

    CHECK(trx.valid());

    address::SocketAddr tx_addr = make_address("0.0.0.0", 0);
    address::SocketAddr rx_addr = make_address("0.0.0.0", 0);

    CHECK(trx.add_udp_sender(tx_addr));
    CHECK(trx.add_udp_receiver(rx_addr, queue));

    trx.remove_port(tx_addr);
    trx.remove_port(rx_addr);
}

TEST(transceiver, udp_bind_lo) {
    packet::ConcurrentQueue queue;

    Transceiver trx(packet_pool, buffer_pool, allocator);

    CHECK(trx.valid());

    address::SocketAddr tx_addr = make_address("127.0.0.1", 0);
    address::SocketAddr rx_addr = make_address("127.0.0.1", 0);

    CHECK(trx.add_udp_sender(tx_addr));
    CHECK(trx.add_udp_receiver(rx_addr, queue));

    trx.remove_port(tx_addr);
    trx.remove_port(rx_addr);
}

TEST(transceiver, udp_bind_addrinuse) {
    packet::ConcurrentQueue queue;

    Transceiver trx1(packet_pool, buffer_pool, allocator);
    CHECK(trx1.valid());

    address::SocketAddr tx_addr = make_address("127.0.0.1", 0);
    address::SocketAddr rx_addr = make_address("127.0.0.1", 0);

    CHECK(trx1.add_udp_sender(tx_addr));
    CHECK(trx1.add_udp_receiver(rx_addr, queue));

    Transceiver trx2(packet_pool, buffer_pool, allocator);
    CHECK(trx2.valid());

    CHECK(!trx2.add_udp_sender(tx_addr));
    CHECK(!trx2.add_udp_receiver(rx_addr, queue));
}

TEST(transceiver, udp_add) {
    packet::ConcurrentQueue queue;

    Transceiver trx(packet_pool, buffer_pool, allocator);

    CHECK(trx.valid());

    address::SocketAddr tx_addr = make_address("0.0.0.0", 0);
    address::SocketAddr rx_addr = make_address("0.0.0.0", 0);

    CHECK(trx.add_udp_sender(tx_addr));
    CHECK(trx.add_udp_receiver(rx_addr, queue));
}

TEST(transceiver, udp_add_remove) {
    packet::ConcurrentQueue queue;

    Transceiver trx(packet_pool, buffer_pool, allocator);

    CHECK(trx.valid());

    address::SocketAddr tx_addr = make_address("0.0.0.0", 0);
    address::SocketAddr rx_addr = make_address("0.0.0.0", 0);

    UNSIGNED_LONGS_EQUAL(0, trx.num_ports());

    CHECK(trx.add_udp_sender(tx_addr));
    UNSIGNED_LONGS_EQUAL(1, trx.num_ports());

    CHECK(trx.add_udp_receiver(rx_addr, queue));
    UNSIGNED_LONGS_EQUAL(2, trx.num_ports());

    trx.remove_port(tx_addr);
    UNSIGNED_LONGS_EQUAL(1, trx.num_ports());

    trx.remove_port(rx_addr);
    UNSIGNED_LONGS_EQUAL(0, trx.num_ports());
}

TEST(transceiver, udp_add_remove_add) {
    Transceiver trx(packet_pool, buffer_pool, allocator);

    CHECK(trx.valid());

    address::SocketAddr tx_addr = make_address("0.0.0.0", 0);

    CHECK(trx.add_udp_sender(tx_addr));
    trx.remove_port(tx_addr);
    CHECK(trx.add_udp_sender(tx_addr));
}

TEST(transceiver, udp_add_duplicate) {
    packet::ConcurrentQueue queue;

    Transceiver trx(packet_pool, buffer_pool, allocator);

    CHECK(trx.valid());

    address::SocketAddr tx_addr = make_address("0.0.0.0", 0);
    address::SocketAddr rx_addr = make_address("0.0.0.0", 0);

    CHECK(trx.add_udp_sender(tx_addr));
    UNSIGNED_LONGS_EQUAL(1, trx.num_ports());

    CHECK(!trx.add_udp_sender(tx_addr));
    UNSIGNED_LONGS_EQUAL(1, trx.num_ports());

    CHECK(!trx.add_udp_receiver(tx_addr, queue));
    UNSIGNED_LONGS_EQUAL(1, trx.num_ports());

    CHECK(trx.add_udp_receiver(rx_addr, queue));
    UNSIGNED_LONGS_EQUAL(2, trx.num_ports());

    CHECK(!trx.add_udp_sender(rx_addr));
    UNSIGNED_LONGS_EQUAL(2, trx.num_ports());

    CHECK(!trx.add_udp_receiver(rx_addr, queue));
    UNSIGNED_LONGS_EQUAL(2, trx.num_ports());

    trx.remove_port(tx_addr);
    UNSIGNED_LONGS_EQUAL(1, trx.num_ports());

    trx.remove_port(rx_addr);
    UNSIGNED_LONGS_EQUAL(0, trx.num_ports());
}

TEST(transceiver, tcp_add_server) {
    Transceiver trx(packet_pool, buffer_pool, allocator);

    CHECK(trx.valid());

    address::SocketAddr addr = make_address("0.0.0.0", 0);
    TestConnAcceptor conn_acceptor;

    CHECK(trx.add_tcp_server(addr, conn_acceptor));
}

TEST(transceiver, tcp_add_client_wait_connected) {
    Transceiver trx(packet_pool, buffer_pool, allocator);

    CHECK(trx.valid());

    address::SocketAddr server_address = make_address("0.0.0.0", 0);

    TestConnAcceptor conn_acceptor;
    CHECK(trx.add_tcp_server(server_address, conn_acceptor));

    TestConnNotifier conn_notifier;

    TCPConn* conn = trx.add_tcp_client(server_address, conn_notifier);
    CHECK(conn);

    conn_notifier.wait_connected();
    CHECK(conn_acceptor.size() == 1);

    CHECK(conn->connected());
    CHECK(conn->address() != server_address);
    CHECK(conn->destination() == server_address);
}

} // namespace netio
} // namespace roc
