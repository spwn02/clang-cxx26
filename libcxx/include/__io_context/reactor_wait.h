//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___IO_CONTEXT_REACTOR_WAIT_H
#define _LIBCPP___IO_CONTEXT_REACTOR_WAIT_H

#include <__config>
#include <__coroutine/coroutine_handle.h>
#include <__io/execution_context.h>
#include <__io/io_env.h>
#include <__io/support/config.h>

#if _LIBCPP_HAS_IO_REACTOR
#  include <__io/support/poller_types.h>
#  include <__io_context/reactor.h>
#  include <__io_stream/error.h>
#endif

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

#if _LIBCPP_HAS_IO_REACTOR

_LIBCPP_PUSH_MACROS
#  include <__undef_macros>

_LIBCPP_BEGIN_NAMESPACE_STD

namespace io {

// Internal building block for the descriptor-based facilities that come later in the series (TCP, UDP, files): an
// IoAwaitable that completes when the file descriptor __fd is ready for the events in __events (a combination of
// __poll_read and __poll_write). The result is io_result<unsigned>: the error_code and the set of events that fired
// (__poll_read/__poll_write, plus __poll_error when the descriptor reports an error or a hang-up; an error condition
// completes every wait on the descriptor). Level-triggered and one-shot: a stream operation is "wait until ready, then
// try the system call, wait again if it would block". Cancellation through the stop token of the awaiting coroutine
// completes it with error::canceled. The context must be an io_context (it runs the reactor); regular files cannot be
// waited for (the error is errc::operation_not_permitted).
class __reactor_wait_awaiter : public __fd_op {
public:
  _LIBCPP_HIDE_FROM_ABI __reactor_wait_awaiter(execution_context& __ctx, int __fd, unsigned __events)
      : __fd_op(&__ctx.use_service<__reactor_service>(), __fd, __events) {}
  _LIBCPP_HIDE_FROM_ABI __reactor_wait_awaiter(__reactor_wait_awaiter&& __o) noexcept : __fd_op(std::move(__o)) {}

  _LIBCPP_HIDE_FROM_ABI bool await_ready() const noexcept { return false; }

  _LIBCPP_HIDE_FROM_ABI bool await_suspend(coroutine_handle<> __h, const io_env* __env) {
    __bind(__h, __env);
    if (__env->stop_token.stop_requested()) {
      __ec_ = make_error_code(error::canceled);
      return false;
    }
    __arm_stop(__env);
    return __svc_->__start_fd_wait(*this);
  }

  _LIBCPP_HIDE_FROM_ABI io_result<unsigned> await_resume() noexcept {
    __finish();
    return io_result<unsigned>(__ec_, __ec_ ? 0u : __got_);
  }
};

[[nodiscard]] _LIBCPP_HIDE_FROM_ABI inline __reactor_wait_awaiter
__reactor_wait(execution_context& __ctx, int __fd, unsigned __events) {
  return __reactor_wait_awaiter(__ctx, __fd, __events);
}

} // namespace io

_LIBCPP_END_NAMESPACE_STD

_LIBCPP_POP_MACROS

#endif // _LIBCPP_HAS_IO_REACTOR

#endif // _LIBCPP___IO_CONTEXT_REACTOR_WAIT_H
