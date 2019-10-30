/*
 * Copyright (c) 2019 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_core/atomic.h"
#include "roc_netio/iconn_acceptor.h"
#include "roc_netio/iconn_notifier.h"
#include "roc_netio/test_conn_notifier.h"

namespace roc {
namespace netio {

class TestConnAcceptor : public IConnAcceptor {
public:
    TestConnAcceptor()
        : size_(0) {
    }

    virtual IConnNotifier* accept(TCPConn&) {
        ++size_;

        return &conn_notifier_;
    }

    size_t size() const {
        return (size_t)size_;
    }

private:
    core::Atomic size_;
    TestConnNotifier conn_notifier_;
};
