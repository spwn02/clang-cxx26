//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23
// UNSUPPORTED: no-threads

// [task.state]p4.3 chooses the first valid scheduler construction form.

#include <cassert>
#include <concepts>
#include <coroutine>
#include <exception>
#include <execution>
#include <memory>
#include <thread>
#include <utility>

namespace ex = std::execution;

template <int _Case>
struct RecordingScheduler {
  using scheduler_concept    = ex::scheduler_tag;
  inline static int selected = 0;

  RecordingScheduler() noexcept { selected = _Case == 1 ? 9 : 5; }
  RecordingScheduler(const RecordingScheduler&) noexcept = default;

  RecordingScheduler(ex::inline_scheduler, const std::allocator<int>&) noexcept
    requires(_Case == 2)
  {
    selected = 2;
  }
  RecordingScheduler(ex::inline_scheduler) noexcept
    requires(_Case == 2 || _Case == 3)
  {
    selected = 3;
  }
  RecordingScheduler(const std::allocator<int>&) noexcept
    requires(_Case == 2 || _Case == 4)
  {
    selected = 4;
  }

  auto schedule() const noexcept { return ex::inline_scheduler{}.schedule(); }
  ex::forward_progress_guarantee query(ex::get_forward_progress_guarantee_t) const noexcept {
    return ex::forward_progress_guarantee::weakly_parallel;
  }
  friend bool operator==(RecordingScheduler, RecordingScheduler) = default;
};

template <int _Case>
struct SchedulerEnv {
  using start_scheduler_type = RecordingScheduler<_Case>;

  auto query(ex::get_start_scheduler_t) const noexcept {
    if constexpr (_Case == 1)
      return RecordingScheduler<1>{};
    else
      return ex::inline_scheduler{};
  }
  std::allocator<int> query(std::get_scheduler_allocator_t) const noexcept { return {}; }
  std::allocator<int> query(std::get_allocator_t) const noexcept { return {}; }
};

template <int _Case>
struct Receiver {
  using receiver_concept = ex::receiver_tag;
  void set_value() && noexcept {}
  void set_error(std::exception_ptr) && noexcept { assert(false); }
  void set_stopped() && noexcept { assert(false); }
  SchedulerEnv<_Case> get_env() const noexcept { return {}; }
};

template <int _Case>
ex::task<void, SchedulerEnv<_Case>> completed_task() {
  co_return;
}

template <int _Case>
void check_scheduler_selection(int _Expected) {
  auto __task                         = completed_task<_Case>();
  RecordingScheduler<_Case>::selected = 0;
  auto __op                           = ex::connect(std::move(__task), Receiver<_Case>{});
  ex::start(__op);
  assert(RecordingScheduler<_Case>::selected == _Expected);
}

int main(int, char**) {
  static_assert(!std::default_initializable<ex::task_scheduler>);

  check_scheduler_selection<1>(9); // The query returns the selected scheduler value.
  check_scheduler_selection<2>(2); // start scheduler and scheduler allocator
  check_scheduler_selection<3>(3); // start scheduler
  check_scheduler_selection<4>(4); // scheduler allocator
  check_scheduler_selection<5>(5); // default construction

  static_assert(std::constructible_from<ex::task_scheduler, ex::inline_scheduler, std::allocator<int>>);
  ex::task_scheduler __scheduler(ex::inline_scheduler{}, std::allocator<int>{});
  (void)__scheduler;

  auto __allocator = std::allocator<int>{};
  auto __result    = std::this_thread::sync_wait(ex::starts_on(
      ex::task_scheduler(ex::inline_scheduler{}, __allocator), [](std::allocator_arg_t, auto) -> ex::task<void> {
        auto __scheduler = co_await ex::read_env(ex::get_start_scheduler);
        co_await ex::starts_on(__scheduler, ex::just());
      }(std::allocator_arg, __allocator)));
  assert(__result);
  return 0;
}
