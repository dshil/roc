/*
 * Copyright (c) 2019 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_netio/target_libuv/roc_netio/tcp_conn.h
//! @brief TCP connection.

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
#include "roc_netio/basic_port.h"
#include "roc_netio/iclose_handler.h"
#include "roc_netio/iconn_notifier.h"
#include "roc_netio/stream.h"

namespace roc {
namespace netio {

//! TCP connection.
class TCPConn : public BasicPort {
public:
    //! Connection status.
    enum ConnectStatus { Status_None, Status_Connected, Status_Error };

    //! Initialize.
    TCPConn(const address::SocketAddr& dst_addr,
            const char* type_str,
            uv_loop_t& event_loop,
            ICloseHandler& close_handler,
            core::IAllocator& allocator);

    //! Destroy.
    ~TCPConn();

    //! Return source address of the connection.
    virtual const address::SocketAddr& address() const;

    //! Open TCP connection.
    //!
    //! @remarks
    //!  Should be called from the event loop thread.
    virtual bool open();

    //! Asynchronously close TCP connection.
    //!
    //! @remarks
    //!  Should be called from the event loop thread.
    virtual void async_close();

    //! Check if the connection was established.
    bool connected() const;

    //! Return destination address of the connection.
    const address::SocketAddr& destination_address() const;

    //! Accept TCP connection.
    //!
    //! @remarks
    //!  Should be called from the event loop thread.
    bool accept(uv_stream_t* stream, IConnNotifier& conn_notifier);

    //! Asynchronously connect to the destination address.
    //!
    //! @remarks
    //!  Should be called from the event loop thread.
    bool connect(IConnNotifier& conn_notifier);

    //! Write @p data of size @p len to TCP connection.
    //!
    //! @remarks
    //!  - @p data should not be NULL.
    //!  - @p data should not be zero-terminated.
    //!
    //! @returns
    //!  true if the data was written completely or false if an error occurred.
    bool write(const char* data, size_t len);

    //! Read @p len bytes from the TCP connection to @p buf.
    //!
    //! @remarks
    //!  - @p buf should not be NULL.
    //!  - @p buf should have size at least of @p len bytes.
    //!
    //! @returns
    //!  the number of bytes read or -1 if some error occurred.
    ssize_t read(char* buf, size_t len);

private:
    struct Task : core::ListNode {
        bool (TCPConn::*fn)(Task&);

        uv_buf_t* write_buf;
        uv_write_t* write_req;

        bool result;
        bool done;

        Task()
            : fn(NULL)
            , write_buf(NULL)
            , write_req(NULL)
            , result(false)
            , done(false) {
        }
    };

    static void close_cb_(uv_handle_t* handle);
    static void task_sem_cb_(uv_async_t* handle);
    static void connect_cb_(uv_connect_t* req, int status);
    static void write_cb_(uv_write_t* req, int status);
    static void alloc_cb_(uv_handle_t* handle, size_t size, uv_buf_t* buf);
    static void read_cb_(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf);

    void close_();

    void process_tasks_();
    void run_task_(Task&);

    bool connect_(Task& task);
    void set_connect_status_(ConnectStatus);
    void wait_connect_status_();

    bool write_(Task& task);

    uv_loop_t& loop_;

    uv_async_t task_sem_;
    bool task_sem_initialized_;

    uv_tcp_t handle_;
    bool handle_initialized_;

    uv_connect_t connect_req_;

    ICloseHandler& close_handler_;
    IConnNotifier* conn_notifier_;

    address::SocketAddr src_addr_;
    address::SocketAddr dst_addr_;

    bool closed_;
    bool stopped_;
    ConnectStatus connect_status_;

    const char* type_str_;

    core::List<Task, core::NoOwnership> tasks_;

    Stream stream_;

    core::Mutex mutex_;
    core::Cond cond_;
};

} // namespace netio
} // namespace roc

#endif // ROC_NETIO_TCP_CONN_H_
