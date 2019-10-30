/*
 * Copyright (c) 2019 Roc authors
 *
 * This src_addr Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_netio/tcp_client_port.h"
#include "roc_address/socket_addr_to_str.h"
#include "roc_core/log.h"

namespace roc {
namespace netio {

/* TCPClientPort::TCPClientPort(const address::SocketAddr& server_address, */
/*                              uv_loop_t& event_loop, */
/*                              ICloseHandler& close_handler, */
/*                              IConnNotifier& conn_notifier, */
/*                              core::IAllocator& allocator) */
/*     : TCPConn(server_address, "client", close_handler, event_loop, allocator) */
/*     , conn_notifier_(conn_notifier) { */
/*     request_.data = this; */
/* } */

/* bool TCPClientPort::open() { */
/*     if (!TCPConn::open()) { */
/*         return false; */
/*     } */

/*     if (!TCPConn::async_connect(request_, connect_cb_)) { */
/*         return false; */
/*     } */

/*     roc_log(LogInfo, "tcp client: connecting: src=%s dst=%s", */
/*             address::socket_addr_to_str(TCPConn::address()).c_str(), */
/*             address::socket_addr_to_str(TCPConn::destination()).c_str()); */

/*     return true; */
/* } */

/* void TCPClientPort::connect_cb_(uv_connect_t* req, int status) { */
/*     roc_panic_if_not(req); */
/*     roc_panic_if_not(req->data); */

/*     TCPClientPort& self = *(TCPClientPort*)req->data; */

/*     if (status < 0) { */
/*         roc_log(LogDebug, "tcp client: failed to connect: src=%s dst=%s: [%s] %s", */
/*                 address::socket_addr_to_str(self.TCPConn::address()).c_str(), */
/*                 address::socket_addr_to_str(self.TCPConn::destination()).c_str(), */
/*                 uv_err_name(status), uv_strerror(status)); */

/*         // TODO: what to do with failed connection? */
/*         return; */
/*     } */

/*     self.TCPConn::set_connected(self.conn_notifier_); */

/*     roc_log(LogInfo, "tcp client: connected: src=%s dst=%s", */
/*             address::socket_addr_to_str(self.TCPConn::address()).c_str(), */
/*             address::socket_addr_to_str(self.TCPConn::destination()).c_str()); */
/* } */

/* bool TCPConn::async_connect(uv_connect_t& req, void (*connect_cb)(uv_connect_t*, int))
 * { */
/*     roc_panic_if_not(handle_initialized_); */

/*     if (int err = uv_tcp_connect(&req, &handle_, dst_addr_.saddr(), connect_cb)) { */
/*         roc_log(LogError, "tcp conn (%s): uv_tcp_connect(): [%s] %s", type_str_, */
/*                 uv_err_name(err), uv_strerror(err)); */
/*         return false; */
/*     } */

/*     int addrlen = (int)dst_addr_.slen(); */
/*     if (int err = uv_tcp_getsockname(&handle_, src_addr_.saddr(), &addrlen)) { */
/*         roc_log(LogError, "tcp conn (%s): uv_tcp_getsockname(): [%s] %s", type_str_, */
/*                 uv_err_name(err), uv_strerror(err)); */
/*         return false; */
/*     } */
/*     if (addrlen != (int)src_addr_.slen()) { */
/*         roc_log( */
/*             LogError, */
/*             "tcp conn (%s): uv_tcp_getsockname(): unexpected len: got=%lu expected =
 * %lu", */
/*             type_str_, (unsigned long)addrlen, (unsigned long)src_addr_.slen()); */
/*         return false; */
/*     } */

/*     return true; */
/* } */

} // namespace netio
} // namespace roc
