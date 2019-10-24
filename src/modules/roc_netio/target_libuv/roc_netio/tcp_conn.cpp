/*
 * Copyright (c) 2019 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_netio/tcp_conn.h"
#include "roc_address/socket_addr_to_str.h"

namespace roc {
namespace netio {

TCPConn::TCPConn(const address::SocketAddr& dst_addr,
                 const char* type_str,
                 ICloseHandler& close_handler,
                 uv_loop_t& event_loop,
                 core::IAllocator& allocator)
    : BasicPort(allocator)
    , loop_(event_loop)
    , handle_initialized_(false)
    , close_handler_(close_handler)
    , conn_notifier_(NULL)
    , dst_addr_(dst_addr)
    , closed_(false)
    , connected_(false)
    , type_str_(type_str) {
}

TCPConn::~TCPConn() {
    roc_panic_if_not(closed_);
}

const address::SocketAddr& TCPConn::address() const {
    return src_addr_;
}

bool TCPConn::open() {
    if (int err = uv_tcp_init(&loop_, &handle_)) {
        roc_log(LogError, "tcp conn (%s): uv_tcp_init(): [%s] %s", type_str_,
                uv_err_name(err), uv_strerror(err));
        return false;
    }

    handle_.data = this;
    handle_initialized_ = true;

    return true;
}

void TCPConn::async_close() {
    if (closed_) {
        return; // handle_closed() was already called
    }

    if (!handle_initialized_) {
        closed_ = true;
        close_handler_.handle_closed(*this);

        return;
    }

    roc_log(LogInfo, "tcp conn (%s): closing: src=%s dst=%s", type_str_,
            address::socket_addr_to_str(src_addr_).c_str(),
            address::socket_addr_to_str(dst_addr_).c_str());

    if (!uv_is_closing((uv_handle_t*)&handle_)) {
        uv_close((uv_handle_t*)&handle_, close_cb_);
    }
}

const address::SocketAddr& TCPConn::destination() const {
    return dst_addr_;
}

bool TCPConn::accept(uv_stream_t* stream) {
    roc_panic_if_not(stream);

    if (int err = uv_accept(stream, (uv_stream_t*)&handle_)) {
        roc_log(LogError,
                "tcp conn (%s): can't accept connection: uv_tcp_accept(): [%s] %s",
                type_str_, uv_err_name(err), uv_strerror(err));

        return false;
    }

    int addrlen = (int)dst_addr_.slen();
    if (int err = uv_tcp_getpeername(&handle_, src_addr_.saddr(), &addrlen)) {
        roc_log(LogError,
                "tcp conn (%s): can't accept connection: uv_tcp_getpeername(): [%s] %s",
                type_str_, uv_err_name(err), uv_strerror(err));

        return false;
    }

    if (addrlen != (int)src_addr_.slen()) {
        roc_log(
            LogError,
            "tcp conn (%s): uv_tcp_getpeername(): unexpected len: got=%lu expected=%lu",
            type_str_, (unsigned long)addrlen, (unsigned long)src_addr_.slen());

        return false;
    }

    return true;
}

bool TCPConn::async_connect(uv_connect_t& req, void (*connect_cb)(uv_connect_t*, int)) {
    if (int err = uv_tcp_connect(&req, &handle_, dst_addr_.saddr(), connect_cb)) {
        roc_log(LogError, "tcp conn (%s): uv_tcp_connect(): [%s] %s", type_str_,
                uv_err_name(err), uv_strerror(err));
        return false;
    }

    int addrlen = (int)dst_addr_.slen();
    if (int err = uv_tcp_getsockname(&handle_, src_addr_.saddr(), &addrlen)) {
        roc_log(LogError, "tcp conn (%s): uv_tcp_getsockname(): [%s] %s", type_str_,
                uv_err_name(err), uv_strerror(err));
        return false;
    }
    if (addrlen != (int)src_addr_.slen()) {
        roc_log(
            LogError,
            "tcp conn (%s): uv_tcp_getsockname(): unexpected len: got=%lu expected = %lu",
            type_str_, (unsigned long)addrlen, (unsigned long)src_addr_.slen());
        return false;
    }

    return true;
}

void TCPConn::set_connected(IConnNotifier& conn_notifier) {
    core::Mutex::Lock lock(mutex_);

    if (connected_) {
        return;
    }

    connected_ = true;
    conn_notifier_ = &conn_notifier;

    conn_notifier_->notify_connected();
}

bool TCPConn::connected() const {
    core::Mutex::Lock lock(mutex_);

    return connected_;
}

void TCPConn::close_cb_(uv_handle_t* handle) {
    roc_panic_if_not(handle);

    TCPConn& self = *(TCPConn*)handle->data;

    self.closed_ = true;
    self.handle_initialized_ = false;

    roc_log(LogInfo, "tcp conn (%s): closed: src=%s dst=%s", self.type_str_,
            address::socket_addr_to_str(self.src_addr_).c_str(),
            address::socket_addr_to_str(self.dst_addr_).c_str());

    self.close_handler_.handle_closed(self);
}

} // namespace netio
} // namespace roc
