/*
 * Copyright (c) 2019 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_netio/target_libuv/roc_netio/tcp_server_port.h
//! @brief TODO.

#ifndef ROC_NETIO_TCP_SERVER_PORT_H_
#define ROC_NETIO_TCP_SERVER_PORT_H_

#include <uv.h>

#include "roc_address/socket_addr.h"
#include "roc_core/iallocator.h"
#include "roc_core/list.h"
#include "roc_core/list_node.h"
#include "roc_core/shared_ptr.h"
#include "roc_netio/basic_port.h"
#include "roc_netio/iclose_handler.h"
#include "roc_netio/iconn_acceptor.h"

namespace roc {
namespace netio {

class TCPServerPort : public BasicPort, private ICloseHandler {
public:
    //! Initialize.
    TCPServerPort(const address::SocketAddr& address,
                  uv_loop_t& loop,
                  ICloseHandler& close_handler,
                  IConnAcceptor& conn_acceptor,
                  core::IAllocator& allocator);

    //! Destroy.
    ~TCPServerPort();

    //! Get bind address.
    virtual const address::SocketAddr& address() const;

    //! Open sender.
    virtual bool open();

    //! Asynchronously close sender.
    virtual void async_close();

private:
    enum { Backlog = 128 };

    static void close_cb_(uv_handle_t* handle);
    static void listen_cb_(uv_stream_t* stream, int status);

    virtual void handle_closed(BasicPort&);

    core::SharedPtr<BasicPort> get_closing_port_(BasicPort& port);

    void close_();
    void async_close_ports_();

    ICloseHandler& close_handler_;
    IConnAcceptor& conn_acceptor_;

    uv_loop_t& loop_;

    uv_tcp_t handle_;
    bool handle_initialized_;

    core::List<BasicPort> open_ports_;
    core::List<BasicPort> closing_ports_;

    bool closing_;
    bool closed_;

    address::SocketAddr address_;
};

} // namespace netio
} // namespace roc

#endif // ROC_NETIO_TCP_SERVER_PORT_H_
