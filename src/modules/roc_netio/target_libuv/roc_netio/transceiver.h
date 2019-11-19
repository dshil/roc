/*
 * Copyright (c) 2015 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_netio/target_libuv/roc_netio/transceiver.h
//! @brief Network sender/receiver.

#ifndef ROC_NETIO_TRANSCEIVER_H_
#define ROC_NETIO_TRANSCEIVER_H_

#include <uv.h>

#include "roc_address/socket_addr.h"
#include "roc_core/buffer_pool.h"
#include "roc_core/cond.h"
#include "roc_core/iallocator.h"
#include "roc_core/list.h"
#include "roc_core/list_node.h"
#include "roc_core/mutex.h"
#include "roc_core/thread.h"
#include "roc_netio/basic_port.h"
#include "roc_netio/iclose_handler.h"
#include "roc_netio/iconn_acceptor.h"
#include "roc_netio/iconn_notifier.h"
#include "roc_netio/tcp_conn.h"
#include "roc_netio/udp_receiver_port.h"
#include "roc_netio/udp_sender_port.h"
#include "roc_packet/iwriter.h"
#include "roc_packet/packet_pool.h"

namespace roc {
namespace netio {

//! Network sender/receiver.
class Transceiver : private ICloseHandler, private core::Thread {
public:
    //! Initialize.
    //!
    //! @remarks
    //!  Start background thread if the object was successfully constructed.
    Transceiver(packet::PacketPool& packet_pool,
                core::BufferPool<uint8_t>& buffer_pool,
                core::IAllocator& allocator);

    //! Destroy. Stop all receivers and senders.
    //!
    //! @remarks
    //!  Wait until background thread finishes.
    virtual ~Transceiver();

    //! Check if transceiver was successfully constructed.
    bool valid() const;

    //! Get number of receiver and sender ports.
    size_t num_ports() const;

    //! Add UDP datagram receiver port.
    //!
    //! Creates a new UDP receiver and bind it to @p bind_address. The receiver
    //! will pass packets to @p writer. Writer will be called from the network
    //! thread. It should not block.
    //!
    //! If IP is zero, INADDR_ANY is used, i.e. the socket is bound to all network
    //! interfaces. If port is zero, a random free port is selected and written
    //! back to @p bind_address.
    //!
    //! @returns
    //!  true on success or false if error occurred
    bool add_udp_receiver(address::SocketAddr& bind_address, packet::IWriter& writer);

    //! Add UDP datagram sender port.
    //!
    //! Creates a new UDP sender, bind to @p bind_address, and returns a writer
    //! that may be used to send packets from this address. Writer may be called
    //! from any thread. It will not block the caller.
    //!
    //! If IP is zero, INADDR_ANY is used, i.e. the socket is bound to all network
    //! interfaces. If port is zero, a random free port is selected and written
    //! back to @p bind_address.
    //!
    //! @returns
    //!  a new packet writer on success or null if error occurred
    packet::IWriter* add_udp_sender(address::SocketAddr& bind_address);

    //! Add TCP server port.
    //!
    //! Creates a new TCP server, bind to @p bind_address, start listening on
    //! @p bind_address. For every new incoming connection IConnAcceptor::accept()
    //! is called on the event loop thread.
    //!
    //! If IP is zero, INADDR_ANY is used, i.e. the socket is bound to all network
    //! interfaces. If port is zero, a random free port is selected and written
    //! back to @p bind_address.
    //!
    //! @returns
    //!  true on success or false if error occurred.
    bool add_tcp_server(address::SocketAddr& bind_address, IConnAcceptor& conn_acceptor);

    //! Add TCP client port.
    //!
    //! Creates a new TCP client, connect to @p serv_addr. Provided @p conn_notifier
    //! will be notified when connection becomes readable or writable.
    //!
    //! @returns
    //!  a new TCP connection on success or NULL if error occured.
    TCPConn* add_tcp_client(address::SocketAddr serv_addr, IConnNotifier& conn_notifier);

    //! Remove port. Wait until port will be removed.
    void remove_port(address::SocketAddr bind_address);

private:
    struct Task : core::ListNode {
        bool (Transceiver::*fn)(Task&);

        address::SocketAddr* address;
        packet::IWriter* writer;
        BasicPort* port;
        TCPConn* conn;
        IConnAcceptor* conn_acceptor;
        IConnNotifier* conn_notifier;

        bool result;
        bool done;

        Task()
            : fn(NULL)
            , address(NULL)
            , writer(NULL)
            , port(NULL)
            , conn(NULL)
            , conn_acceptor(NULL)
            , result(false)
            , done(false) {
        }
    };

    static void task_sem_cb_(uv_async_t* handle);
    static void stop_sem_cb_(uv_async_t* handle);

    virtual void handle_closed(BasicPort&);
    virtual void run();

    void close_sems_();
    void async_close_ports_();

    void process_tasks_();
    void run_task_(Task&);

    bool add_udp_receiver_(Task&);
    bool add_udp_sender_(Task&);
    bool add_tcp_server_(Task&);
    bool add_tcp_client_(Task&);

    bool remove_port_(Task&);
    void wait_port_closed_(const BasicPort& port);
    bool port_is_closing_(const BasicPort& port);

    packet::PacketPool& packet_pool_;
    core::BufferPool<uint8_t>& buffer_pool_;
    core::IAllocator& allocator_;

    bool started_;

    uv_loop_t loop_;
    bool loop_initialized_;

    uv_async_t stop_sem_;
    bool stop_sem_initialized_;

    uv_async_t task_sem_;
    bool task_sem_initialized_;

    core::List<Task, core::NoOwnership> tasks_;

    core::List<BasicPort> open_ports_;
    core::List<BasicPort> closing_ports_;

    core::Mutex mutex_;
    core::Cond cond_;
};

} // namespace netio
} // namespace roc

#endif // ROC_NETIO_TRANSCEIVER_H_
