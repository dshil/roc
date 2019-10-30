/*
 * Copyright (c) 2019 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_core/cond.h"
#include "roc_core/mutex.h"
#include "roc_netio/iconn_notifier.h"

namespace roc {
namespace netio {

class TestConnNotifier : public IConnNotifier {
public:
    TestConnNotifier()
        : cond_(mutex_)
        , connected_(false) {
    }

    virtual void notify_connected() {
        core::Mutex::Lock lock(mutex_);

        connected_ = true;
        cond_.broadcast();
    }

    virtual void notify_readable() {
    }

    virtual void notify_writable() {
    }

    void wait_connected() {
        core::Mutex::Lock lock(mutex_);

        while (!connected_) {
            cond_.wait();
        }
    }

private:
    core::Mutex mutex_;
    core::Cond cond_;

    bool connected_;
};

} // namespace netio
} // namespace roc
