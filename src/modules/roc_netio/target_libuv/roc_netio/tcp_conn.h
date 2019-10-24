/*
 * Copyright (c) 2019 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_netio/target_libuv/roc_netio/tcp_conn.h
//! @brief TODO.

#ifndef ROC_NETIO_TCP_CONN_H_
#define ROC_NETIO_TCP_CONN_H_

#include <uv.h>

#include "roc_address/socket_addr.h"
#include "roc_core/iallocator.h"
#include "roc_core/log.h"
#include "roc_core/mutex.h"
#include "roc_netio/basic_port.h"
#include "roc_netio/iclose_handler.h"
#include "roc_netio/iconn_notifier.h"

namespace roc {
namespace netio {

class TCPConn : public BasicPort {
public:
    //! Initialize.
    TCPConn(const address::SocketAddr& dst_addr,
            const char* type_str,
            ICloseHandler& close_handler,
            uv_loop_t& event_loop,
            core::IAllocator& allocator);

    //! Destroy.
    ~TCPConn();

    //! Get connection address()
    virtual const address::SocketAddr& address() const;

    //! Open TCP connection.
    virtual bool open();

    //! Asynchronously close TCP connection.
    virtual void async_close();

    //! Check if connection is established.
    bool connected() const;

    //! Return destination address of the connection.
    const address::SocketAddr& destination() const;

    //! Accept TCP connection.
    //!
    //! @remarks
    //!  Should be called from the event loop thread.
    bool accept(uv_stream_t* stream);

    //! Start asynchronous TCP connection.
    bool async_connect(uv_connect_t&, void (*)(uv_connect_t*, int));

    //! Mark connection as connected.
    void set_connected(IConnNotifier& conn_notifier);

private:
    static void close_cb_(uv_handle_t* handle);

    core::Mutex mutex_;

    uv_loop_t& loop_;
    uv_tcp_t handle_;
    bool handle_initialized_;

    ICloseHandler& close_handler_;
    IConnNotifier* conn_notifier_;

    address::SocketAddr src_addr_;
    address::SocketAddr dst_addr_;

    bool closed_;
    bool connected_;

    const char* type_str_;
};

} // namespace netio
} // namespace roc

#endif // ROC_NETIO_TCP_CONN_H_
