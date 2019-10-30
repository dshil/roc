/*
 * Copyright (c) 2019 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_netio/tcp_server_port.h"
#include "roc_address/socket_addr_to_str.h"
#include "roc_core/log.h"

namespace roc {
namespace netio {

TCPServerPort::TCPServerPort(const address::SocketAddr& address,
                             uv_loop_t& loop,
                             ICloseHandler& close_handler,
                             IConnAcceptor& conn_acceptor,
                             core::IAllocator& allocator)
    : BasicPort(allocator)
    , close_handler_(close_handler)
    , conn_acceptor_(conn_acceptor)
    , loop_(loop)
    , handle_initialized_(false)
    , closing_(false)
    , closed_(false)
    , address_(address) {
}

TCPServerPort::~TCPServerPort() {
    roc_panic_if(closing_);
    roc_panic_if_not(closed_);
    roc_panic_if(handle_initialized_);
    roc_panic_if(open_ports_.size());
    roc_panic_if(closing_ports_.size());
}

const address::SocketAddr& TCPServerPort::address() const {
    return address_;
}

bool TCPServerPort::open() {
    if (int err = uv_tcp_init(&loop_, &handle_)) {
        roc_log(LogError, "tcp server: uv_tcp_init(): [%s] %s", uv_err_name(err),
                uv_strerror(err));
        return false;
    }

    handle_.data = this;
    handle_initialized_ = true;

    unsigned flags = 0;

    int bind_err = UV_EINVAL;
    if (address_.version() == 6) {
        bind_err = uv_tcp_bind(&handle_, address_.saddr(), flags | UV_TCP_IPV6ONLY);
    }
    if (bind_err == UV_EINVAL || bind_err == UV_ENOTSUP) {
        bind_err = uv_tcp_bind(&handle_, address_.saddr(), flags);
    }
    if (bind_err != 0) {
        roc_log(LogError, "tcp server: uv_tcp_bind(): [%s] %s", uv_err_name(bind_err),
                uv_strerror(bind_err));
        return false;
    }

    int addrlen = (int)address_.slen();
    if (int err = uv_tcp_getsockname(&handle_, address_.saddr(), &addrlen)) {
        roc_log(LogError, "tcp server: uv_tcp_getsockname(): [%s] %s", uv_err_name(err),
                uv_strerror(err));
        return false;
    }

    if (addrlen != (int)address_.slen()) {
        roc_log(LogError,
                "tcp server: uv_tcp_getsockname(): unexpected len: got=%lu expected=%lu",
                (unsigned long)addrlen, (unsigned long)address_.slen());
        return false;
    }

    if (int err = uv_listen((uv_stream_t*)&handle_, Backlog, listen_cb_)) {
        roc_log(LogError, "tcp server: uv_listen(): [%s] %s", uv_err_name(err),
                uv_strerror(err));
        return false;
    }

    roc_log(LogInfo, "tcp server: opened port %s",
            address::socket_addr_to_str(address_).c_str());

    return true;
}

void TCPServerPort::async_close() {
    if (closed_) {
        return; // handle_closed() was already called
    }

    if (!handle_initialized_) {
        closed_ = true;
        close_handler_.handle_closed(*this);

        return;
    }

    if (closing_) {
        return;
    }
    closing_ = true;

    /* roc_log(LogInfo, "tcp server: closing port %s", */
    /*         address::socket_addr_to_str(address_).c_str()); */

    /* if (open_ports_.size()) { */
    /*     async_close_ports_(); */

    /*     return; */
    /* } */

    /* if (closing_ports_.size()) { */
    /*     return; */
    /* } */

    /* close_(); */
}

void TCPServerPort::close_cb_(uv_handle_t* handle) {
    roc_panic_if_not(handle);

    TCPServerPort& self = *(TCPServerPort*)handle->data;

    self.closed_ = true;
    self.closing_ = false;
    self.handle_initialized_ = false;

    roc_log(LogInfo, "tcp server: closed port %s",
            address::socket_addr_to_str(self.address_).c_str());

    self.close_handler_.handle_closed(self);
}

void TCPServerPort::listen_cb_(uv_stream_t* stream, int status) {
    if (status < 0) {
        // TODO: do we still interested in failed port?

        roc_log(LogDebug, "tcp server: failed to connect: [%s] %s", uv_err_name(status),
                uv_strerror(status));

        return;
    }

    roc_panic_if_not(stream);
    roc_panic_if_not(stream->data);

    TCPServerPort& self = *(TCPServerPort*)stream->data;

    /* core::SharedPtr<TCPConn> cp = new (self.allocator_) */
    /*     TCPConn(self.address_, "server", self, self.loop_, self.allocator_); */
    /* if (!cp) { */
    /*     roc_log(LogError, "tcp server: can't allocate connection"); */

    /*     return; */
    /* } */

    /* if (!cp->open()) { */
    /*     self.closing_ports_.push_back(*cp); */
    /*     cp->async_close(); */

    /*     return; */
    /* } */

    /* if (!cp->accept(stream)) { */
    /*     self.closing_ports_.push_back(*cp); */
    /*     cp->async_close(); */

    /*     return; */
    /* } */

    /* IConnNotifier* conn_notifier = self.conn_acceptor_.accept(*cp); */
    /* if (!conn_notifier) { */
    /*     roc_log(LogError, "tcp server: accept failed: src=%s dst=%s", */
    /*             address::socket_addr_to_str(cp->address()).c_str(), */
    /*             address::socket_addr_to_str(cp->destination()).c_str()); */

    /*     self.closing_ports_.push_back(*cp); */
    /*     cp->async_close(); */

    /*     return; */
    /* } */

    /* self.open_ports_.push_back(*cp); */
    /* cp->set_connected(*conn_notifier); */

    /* roc_log(LogInfo, "tcp server: accepted: src=%s dst=%s", */
    /*         address::socket_addr_to_str(cp->address()).c_str(), */
    /*         address::socket_addr_to_str(cp->destination()).c_str()); */
}

void TCPServerPort::handle_closed(BasicPort& port) {
    /* core::SharedPtr<BasicPort> pp = get_closing_port_(port); */
    /* roc_panic_if_not(pp); */

    /* closing_ports_.remove(*pp); */

    /* roc_log(LogDebug, "tcp server: asynchronous close finished: port %s", */
    /*         address::socket_addr_to_str(port.address()).c_str()); */

    /* if (!closing_) { */
    /*     return; */
    /* } */

    /* if (closing_ports_.size() || open_ports_.size()) { */
    /*     return; */
    /* } */

    /* close_(); */
}

core::SharedPtr<BasicPort> TCPServerPort::get_closing_port_(BasicPort& port) {
    /* for (core::SharedPtr<BasicPort> pp = closing_ports_.front(); pp; */
    /*      pp = closing_ports_.nextof(*pp)) { */
    /*     if (pp.get() != &port) { */
    /*         continue; */
    /*     } */

    /*     return pp; */
    /* } */

    return NULL;
}

void TCPServerPort::close_() {
    /* uv_close((uv_handle_t*)&handle_, close_cb_); */
}

void TCPServerPort::async_close_ports_() {
    /* while (core::SharedPtr<BasicPort> port = open_ports_.front()) { */
    /*     open_ports_.remove(*port); */
    /*     closing_ports_.push_back(*port); */

    /*     port->async_close(); */
    /* } */
}

} // namespace netio
} // namespace roc
