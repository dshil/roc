/*
 * Copyright (c) 2019 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_netio/tcp_conn.h"
#include "roc_address/socket_addr_to_str.h"
#include "roc_core/shared_ptr.h"

namespace roc {
namespace netio {

TCPConn::TCPConn(const address::SocketAddr& dst_addr,
                 const char* type_str,
                 uv_loop_t& event_loop,
                 ICloseHandler& close_handler,
                 core::IAllocator& allocator)
    : BasicPort(allocator)
    , loop_(event_loop)
    , task_sem_initialized_(false)
    , handle_initialized_(false)
    , close_handler_(close_handler)
    , conn_notifier_(NULL)
    , dst_addr_(dst_addr)
    , closed_(false)
    , stopped_(true)
    , connect_status_(Status_None)
    , type_str_(type_str)
    , cond_(mutex_) {
}

TCPConn::~TCPConn() {
    roc_panic_if(tasks_.size());
    roc_panic_if(handle_initialized_);
    roc_panic_if(task_sem_initialized_);
}

const address::SocketAddr& TCPConn::address() const {
    return src_addr_;
}

bool TCPConn::open() {
    if (int err = uv_async_init(&loop_, &task_sem_, task_sem_cb_)) {
        roc_log(LogError, "tcp conn (%s): uv_async_init(): [%s] %s", type_str_,
                uv_err_name(err), uv_strerror(err));
        return false;
    }
    task_sem_.data = this;
    task_sem_initialized_ = true;

    if (int err = uv_tcp_init(&loop_, &handle_)) {
        roc_log(LogError, "tcp conn (%s): uv_tcp_init(): [%s] %s", type_str_,
                uv_err_name(err), uv_strerror(err));
        return false;
    }

    handle_.data = this;
    handle_initialized_ = true;

    connect_req_.data = this;

    stopped_ = false;

    return true;
}

void TCPConn::async_close() {
    core::Mutex::Lock lock(mutex_);

    stopped_ = true;

    if (tasks_.size() == 0) {
        close_();
    }
}

const address::SocketAddr& TCPConn::destination_address() const {
    return dst_addr_;
}

bool TCPConn::connected() const {
    core::Mutex::Lock lock(mutex_);

    return connect_status_ == Status_Connected;
}

bool TCPConn::accept(uv_stream_t* stream, IConnNotifier& conn_notifier) {
    roc_panic_if_not(stream);
    roc_panic_if(conn_notifier_);

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

    conn_notifier_ = &conn_notifier;
    set_connect_status_(Status_Connected);

    conn_notifier_->notify_connected();

    return true;
}

bool TCPConn::connect(IConnNotifier& conn_notifier) {
    conn_notifier_ = &conn_notifier;

    Task task;
    task.fn = &TCPConn::connect_;

    run_task_(task);
    if (!task.result) {
        return false;
    }

    return true;
}

bool TCPConn::write(const char* data, size_t len) {
    roc_panic_if_not(data);

    uv_buf_t buf;
    buf.base = (char*)data;
    buf.len = len;

    uv_write_t req;
    req.data = this;

    Task task;
    task.fn = &TCPConn::write_;
    task.write_buf = &buf;
    task.write_req = &req;

    run_task_(task);

    return task.result;
}

ssize_t TCPConn::read(char* buf, size_t len) {
    roc_panic_if_not(buf);

    return stream_.read(buf, len);
}

void TCPConn::close_cb_(uv_handle_t* handle) {
    roc_panic_if_not(handle);
    roc_panic_if_not(handle->data);

    TCPConn& self = *(TCPConn*)handle->data;

    if (handle == (uv_handle_t*)&self.handle_) {
        self.handle_initialized_ = false;
    } else {
        self.task_sem_initialized_ = false;
    }

    if (self.handle_initialized_ || self.task_sem_initialized_) {
        return;
    }

    roc_log(LogInfo, "tcp conn (%s): closed: src=%s dst=%s", self.type_str_,
            address::socket_addr_to_str(self.src_addr_).c_str(),
            address::socket_addr_to_str(self.dst_addr_).c_str());

    self.closed_ = true;
    self.close_handler_.handle_closed(self);
}

void TCPConn::task_sem_cb_(uv_async_t* handle) {
    roc_panic_if_not(handle);
    roc_panic_if_not(handle->data);

    TCPConn& self = *(TCPConn*)handle->data;
    self.process_tasks_();

    core::Mutex::Lock lock(self.mutex_);

    if (self.stopped_ && self.tasks_.size() == 0) {
        self.close_();
    }
}

void TCPConn::connect_cb_(uv_connect_t* req, int status) {
    roc_panic_if_not(req);
    roc_panic_if_not(req->data);

    TCPConn& self = *(TCPConn*)req->data;

    if (status < 0) {
        self.set_connect_status_(Status_Error);

        roc_log(LogError, "tcp conn (%s): failed to connect: src=%s dst=%s: [%s] %s",
                self.type_str_, address::socket_addr_to_str(self.src_addr_).c_str(),
                address::socket_addr_to_str(self.dst_addr_).c_str(), uv_err_name(status),
                uv_strerror(status));

        return;
    }

    if (int err =
            uv_read_start((uv_stream_t*)&self.handle_, self.alloc_cb_, self.read_cb_)) {
        self.set_connect_status_(Status_Error);

        roc_log(LogError, "tcp conn (%s): uv_read_start(): [%s] %s", self.type_str_,
                uv_err_name(err), uv_strerror(err));

        return;
    }

    self.set_connect_status_(Status_Connected);

    roc_log(LogInfo, "tcp conn (%s): connected: src=%s dst=%s", self.type_str_,
            address::socket_addr_to_str(self.src_addr_).c_str(),
            address::socket_addr_to_str(self.dst_addr_).c_str());

    self.conn_notifier_->notify_connected();
}

void TCPConn::write_cb_(uv_write_t* req, int status) {
    roc_panic_if_not(req);
    roc_panic_if_not(req->data);

    TCPConn& self = *(TCPConn*)req->data;

    if (status < 0) {
        // TODO: should we try to write one more time?
        roc_log(LogError, "tcp conn (%s): failed to write: src=%s dst=%s: [%s] %s",
                self.type_str_, address::socket_addr_to_str(self.src_addr_).c_str(),
                address::socket_addr_to_str(self.dst_addr_).c_str(), uv_err_name(status),
                uv_strerror(status));

        return;
    }

    self.conn_notifier_->notify_writable();
}

void TCPConn::alloc_cb_(uv_handle_t* handle, size_t size, uv_buf_t* buf) {
    roc_panic_if_not(buf);
    roc_panic_if_not(handle);
    roc_panic_if_not(handle->data);

    TCPConn& self = *(TCPConn*)handle->data;

    core::SharedPtr<StreamBuffer> bp =
        new (self.allocator_) StreamBuffer(self.allocator_);
    if (!bp) {
        roc_log(LogError, "tcp conn (%s): can't allocate buffer", self.type_str_);

        buf->base = NULL;
        buf->len = 0;

        return;
    }

    if (!bp->resize(size)) {
        roc_log(LogError, "tcp conn (%s): can't resize allocated buffer", self.type_str_);

        buf->base = NULL;
        buf->len = 0;

        return;
    }

    self.stream_.write(bp);

    buf->base = (char*)bp->data();
    buf->len = size;
}

void TCPConn::read_cb_(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf) {
    roc_panic_if_not(stream);
    roc_panic_if_not(stream->data);

    TCPConn& self = *(TCPConn*)stream->data;

    if (nread < 0) {
        roc_log(LogError, "tcp conn (%s): network error: src=%s dst=%s nread=%ld",
                self.type_str_, address::socket_addr_to_str(self.src_addr_).c_str(),
                address::socket_addr_to_str(self.dst_addr_).c_str(), (long)nread);
        return;
    }

    if (nread == 0) {
        return;
    }

    roc_panic_if_not(buf);

    self.conn_notifier_->notify_readable();
}

void TCPConn::close_() {
    if (closed_) {
        return; // handle_closed() was already called
    }

    if (!handle_initialized_) {
        closed_ = true;
        close_handler_.handle_closed(*this);

        return;
    }

    if (handle_initialized_ && !uv_is_closing((uv_handle_t*)&handle_)) {
        if (int err = uv_read_stop((uv_stream_t*)&handle_)) {
            roc_log(LogError, "tcp conn (%s): uv_read_stop(): [%s] %s", type_str_,
                    uv_err_name(err), uv_strerror(err));
        }

        roc_log(LogInfo, "tcp conn (%s): closing: src=%s dst=%s", type_str_,
                address::socket_addr_to_str(src_addr_).c_str(),
                address::socket_addr_to_str(dst_addr_).c_str());

        uv_close((uv_handle_t*)&handle_, close_cb_);
    }

    if (task_sem_initialized_ && !uv_is_closing((uv_handle_t*)&task_sem_)) {
        uv_close((uv_handle_t*)&task_sem_, close_cb_);
    }
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

    if (stopped_) {
        task.result = false;

        return;
    }

    tasks_.push_back(task);

    if (int err = uv_async_send(&task_sem_)) {
        roc_panic("tcp conn (%s): uv_async_send(): [%s] %s", type_str_, uv_err_name(err),
                  uv_strerror(err));
    }

    while (!task.done) {
        cond_.wait();
    }
}

bool TCPConn::connect_(Task&) {
    if (int err =
            uv_tcp_connect(&connect_req_, &handle_, dst_addr_.saddr(), connect_cb_)) {
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
        roc_log(LogError,
                "tcp conn (%s): uv_tcp_getsockname(): unexpected len: got=%lu "
                "expected = %lu",
                type_str_, (unsigned long)addrlen, (unsigned long)src_addr_.slen());
        return false;
    }

    return true;
}

bool TCPConn::write_(Task& task) {
    roc_panic_if_not(task.write_req);
    roc_panic_if_not(conn_notifier_);

    if (int err = uv_write(task.write_req, (uv_stream_t*)&handle_, task.write_buf, 1,
                           write_cb_)) {
        roc_log(LogError, "tcp conn (%s): uv_write(): [%s] %s", type_str_,
                uv_err_name(err), uv_strerror(err));

        return false;
    }

    return true;
}

void TCPConn::set_connect_status_(ConnectStatus status) {
    core::Mutex::Lock lock(mutex_);

    connect_status_ = status;
    cond_.broadcast();
}

void TCPConn::wait_connect_status_() {
    core::Mutex::Lock lock(mutex_);

    while (connect_status_ == Status_None) {
        cond_.wait();
    }
}

} // namespace netio
} // namespace roc
