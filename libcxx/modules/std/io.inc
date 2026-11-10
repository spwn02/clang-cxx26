// -*- C++ -*-
//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

export namespace std {
#if _LIBCPP_HAS_EXPERIMENTAL_NETWORKING && _LIBCPP_HAS_THREADS
  // P4003R3, experimental: C++29 with -fexperimental-library
  namespace io {
    using std::io::continuation;

    using std::io::Executor;
    using std::io::executor_ref;

    using std::io::execution_context;
    using std::io::ExecutionContext;

    using std::io::io_env;

    using std::io::IoAwaitable;
    using std::io::IoRunnable;

    using std::io::get_cached_frame_allocator;
    using std::io::safe_resume;
    using std::io::set_cached_frame_allocator;

    using std::io::get_environment;
    using std::io::get_stop_token;
    using std::io::run;
    using std::io::run_async;
    using std::io::system_context;
    using std::io::task;
    using std::io::thread_pool;
  } // namespace io
#endif
} // namespace std
