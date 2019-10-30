/*
 * Copyright (c) 2019 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_netio/target_libuv/roc_netio/tcp_client_port.h
//! @brief TODO.

#ifndef ROC_NETIO_TCP_CLIENT_PORT_H_
#define ROC_NETIO_TCP_CLIENT_PORT_H_

#include <uv.h>

#include "roc_address/socket_addr.h"
#include "roc_core/iallocator.h"
#include "roc_netio/basic_port.h"
#include "roc_netio/iclose_handler.h"
#include "roc_netio/iconn_acceptor.h"
#include "roc_netio/tcp_conn.h"

namespace roc {
namespace netio {

/* class TCPClientPort : public TCPConn { */
/* public: */
/*     //! Initialize. */
/*     TCPClientPort(const address::SocketAddr& address, */
/*                   uv_loop_t& event_loop, */
/*                   ICloseHandler& close_handler, */
/*                   IConnNotifier& conn_notifier, */
/*                   core::IAllocator& allocator); */

/*     //! Open TCP client connection. */
/*     virtual bool open(); */

/* private: */
/*     static void connect_cb_(uv_connect_t* conn, int status); */

/*     uv_connect_t request_; */
/*     IConnNotifier& conn_notifier_; */
/* }; */

} // namespace netio
} // namespace roc

#endif // ROC_NETIO_TCP_CLIENT_PORT_H_
