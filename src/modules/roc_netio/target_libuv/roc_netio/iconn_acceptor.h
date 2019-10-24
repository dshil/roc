/*
 * Copyright (c) 2019 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_netio/target_libuv/roc_netio/iconn_acceptor.h
//! @brief TODO.

#ifndef ROC_NETIO_ICONN_ACCEPTOR_H_
#define ROC_NETIO_ICONN_ACCEPTOR_H_

#include "roc_netio/iconn_notifier.h"
#include "roc_netio/tcp_conn.h"

namespace roc {
namespace netio {

class IConnAcceptor {
public:
    virtual ~IConnAcceptor();

    virtual IConnNotifier* accept(TCPConn&) = 0;
};

} // namespace netio
} // namespace roc

#endif // ROC_NETIO_ICONN_ACCEPTOR_H_
