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
                 core::IAllocator& allocator)
    : allocator_(allocator)
    , started_(false)
    , connected_(false)
    , loop_initialized_(false)
    , stop_sem_initialized_(false)
    , handle_initialized_(false)
    , dst_addr_(dst_addr)
    , type_str_(type_str)
    , cond_(mutex_) {
    if (int err = uv_loop_init(&loop_)) {
        roc_log(LogError, "tcp conn (%s): uv_loop_init(): [%s] %s", type_str_,
                uv_err_name(err), uv_strerror(err));
        return;
    }
    loop_initialized_ = true;

    if (int err = uv_async_init(&loop_, &stop_sem_, stop_sem_cb_)) {
        roc_log(LogError, "tcp conn (%s): uv_async_init(): [%s] %s", type_str_,
                uv_err_name(err), uv_strerror(err));
        return;
    }
    stop_sem_.data = this;
    stop_sem_initialized_ = true;

    if (int err = uv_async_init(&loop_, &task_sem_, task_sem_cb_)) {
        roc_log(LogError, "tcp conn (%s): uv_async_init(): [%s] %s", type_str_,
                uv_err_name(err), uv_strerror(err));
        return;
    }
    task_sem_.data = this;
    task_sem_initialized_ = true;

    if (int err = uv_tcp_init(&loop_, &handle_)) {
        roc_log(LogError, "tcp conn (%s): uv_tcp_init(): [%s] %s", type_str_,
                uv_err_name(err), uv_strerror(err));
        return;
    }

    handle_.data = this;
    handle_initialized_ = true;

    started_ = Thread::start();
}

TCPConn::~TCPConn() {
    if (started_) {
        if (int err = uv_async_send(&stop_sem_)) {
            roc_panic("tcp conn (%s): uv_async_send(): [%s] %s", type_str_,
                      uv_err_name(err), uv_strerror(err));
        }
    } else {
        close_();
    }

    if (loop_initialized_) {
        if (started_) {
            Thread::join();
        } else {
            // If the thread was never started we should manually run the loop to
            // wait all opened handles to be closed. Otherwise, uv_loop_close()
            // will fail with EBUSY.
            TCPConn::run(); // non-virtual call from dtor
        }

        if (int err = uv_loop_close(&loop_)) {
            roc_panic("tcp conn (%s): uv_loop_close(): [%s] %s", type_str_,
                      uv_err_name(err), uv_strerror(err));
        }
    }
}

const address::SocketAddr& TCPConn::source() const {
    return src_addr_;
}

const address::SocketAddr& TCPConn::destination() const {
    return dst_addr_;
}

bool TCPConn::valid() const {
    return started_;
}

bool TCPConn::accept(uv_stream_t* stream) {
    if (!valid()) {
        roc_panic("tcp conn: can't use invalid tcp conn");
    }

    roc_panic_if_not(stream);

    Task task;
    task.fn = &TCPConn::accept_;
    task.stream = stream;

    run_task_(task);

    return task.result;
}

bool TCPConn::async_connect(uv_connect_t& req, void (*connect_cb)(uv_connect_t*, int)) {
    roc_panic_if_not(handle_initialized_);

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

    // TODO: extract from lock
    conn_notifier_->notify_connected();
}

bool TCPConn::connected() const {
    core::Mutex::Lock lock(mutex_);

    return connected_;
}

void TCPConn::task_sem_cb_(uv_async_t* handle) {
    roc_panic_if_not(handle);
    roc_panic_if_not(handle->data);

    TCPConn& self = *(TCPConn*)handle->data;
    self.process_tasks_();
}

void TCPConn::stop_sem_cb_(uv_async_t* handle) {
    roc_panic_if_not(handle);
    roc_panic_if_not(handle->data);

    TCPConn& self = *(TCPConn*)handle->data;
    self.close_();
}

void TCPConn::run() {
    roc_log(LogDebug, "tcp conn (%s): starting event loop", type_str_);

    int err = uv_run(&loop_, UV_RUN_DEFAULT);
    if (err != 0) {
        roc_log(LogInfo, "tcp conn (%s): uv_run() returned non-zero", type_str_);
    }

    roc_log(LogDebug, "tcp conn (%s): finishing event loop", type_str_);
}

void TCPConn::process_tasks_() {
    core::Mutex::Lock lock(mutex_);

    while (Task* task = tasks_.front()) {
        tasks_.remove(*task);

        task->result = (this->*(task->fn))(*task);
        task->done = true;
    }

    cond_.broadcast();
}

void TCPConn::run_task_(Task& task) {
    core::Mutex::Lock lock(mutex_);

    tasks_.push_back(task);

    if (int err = uv_async_send(&task_sem_)) {
        roc_panic("tcp conn (%s): uv_async_send(): [%s] %s", type_str_, uv_err_name(err),
                  uv_strerror(err));
    }

    while (!task.done) {
        cond_.wait();
    }
}

bool TCPConn::accept_(Task& task) {
    if (int err = uv_accept(task.stream, (uv_stream_t*)&handle_)) {
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

void TCPConn::close_() {
    if (task_sem_initialized_) {
        uv_close((uv_handle_t*)&task_sem_, NULL);
        task_sem_initialized_ = false;
    }

    if (stop_sem_initialized_) {
        uv_close((uv_handle_t*)&stop_sem_, NULL);
        stop_sem_initialized_ = false;
    }

    if (handle_initialized_) {
        uv_close((uv_handle_t*)&handle_, NULL);
        handle_initialized_ = false;
    }
}

} // namespace netio
} // namespace roc
