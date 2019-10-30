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
#include "roc_core/cond.h"
#include "roc_core/iallocator.h"
#include "roc_core/list.h"
#include "roc_core/list_node.h"
#include "roc_core/log.h"
#include "roc_core/mutex.h"
#include "roc_core/ownership.h"
#include "roc_core/thread.h"
#include "roc_netio/basic_port.h"
#include "roc_netio/iclose_handler.h"
#include "roc_netio/iconn_notifier.h"

namespace roc {
namespace netio {

// Bidirectinal TCP connection.
class TCPConn : private core::Thread {
public:
    //! Initialize.
    TCPConn(const address::SocketAddr& dst_addr,
            const char* type_str,
            core::IAllocator& allocator);

    //! Destroy.
    virtual ~TCPConn();

    //! Check if the connection was established.
    bool connected() const;

    //! Check if connection was successfully constructed.
    bool valid() const;

    //! Return source address of the connection.
    const address::SocketAddr& source() const;

    //! Return destination address of the connection.
    const address::SocketAddr& destination() const;

    //! Accept TCP connection.
    //!
    //! @remarks
    //!  Should be called from the event loop thread.
    bool accept(uv_stream_t* stream);

    //! Start asynchronous TCP connection and call @p connect_cb when
    //! the connection is established.
    //!
    //! @remarks
    //!  Should be called from the event loop thread.
    bool async_connect(uv_connect_t& req, void (*connect_cb)(uv_connect_t*, int));

    //! Mark connection as connected.
    //!
    //! @remarks
    //!  Can be called from any thread.
    void set_connected(IConnNotifier& conn_notifier);

private:
    struct Task : core::ListNode {
        bool (TCPConn::*fn)(Task&);

        uv_stream_t* stream;

        bool result;
        bool done;

        Task()
            : fn(NULL)
            , stream(NULL)
            , result(false)
            , done(false) {
        }
    };

    static void stop_sem_cb_(uv_async_t* handle);
    static void task_sem_cb_(uv_async_t* handle);

    virtual void run();

    void process_tasks_();
    void run_task_(Task&);

    bool accept_(Task& task);

    void close_();

    core::IAllocator& allocator_;

    bool started_;
    bool connected_;

    uv_loop_t loop_;
    bool loop_initialized_;

    uv_async_t stop_sem_;
    bool stop_sem_initialized_;

    uv_async_t task_sem_;
    bool task_sem_initialized_;

    uv_tcp_t handle_;
    bool handle_initialized_;

    IConnNotifier* conn_notifier_;

    address::SocketAddr src_addr_;
    address::SocketAddr dst_addr_;

    const char* type_str_;

    core::List<Task, core::NoOwnership> tasks_;

    core::Mutex mutex_;
    core::Cond cond_;
};

} // namespace netio
} // namespace roc

#endif // ROC_NETIO_TCP_CONN_H_
