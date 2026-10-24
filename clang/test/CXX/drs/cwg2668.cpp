// RUN: %clang_cc1 -std=c++20 -pedantic-errors %s -verify=expected -fexceptions -fcxx-exceptions
// RUN: %clang_cc1 -std=c++23 -pedantic-errors %s -verify=expected -fexceptions -fcxx-exceptions
// RUN: %clang_cc1 -std=c++2c -pedantic-errors %s -verify=expected -fexceptions -fcxx-exceptions
// expected-note@*:* 0+ {{}}

namespace std {
template<class R, class... Args> struct coroutine_traits { using promise_type = typename R::promise_type; };
template<class P = void> struct coroutine_handle;
template<> struct coroutine_handle<void> {
  static coroutine_handle from_address(void *) noexcept;
  void resume() const;
  void destroy() const;
};
template<class P> struct coroutine_handle : coroutine_handle<> {
  static coroutine_handle from_address(void *) noexcept;
};
struct suspend_never {
  bool await_ready() const noexcept { return true; }
  void await_suspend(coroutine_handle<>) const noexcept {}
  void await_resume() const noexcept {}
};
} // namespace std

struct Task {
  struct promise_type {
    Task get_return_object();
    std::suspend_never initial_suspend() noexcept;
    std::suspend_never final_suspend() noexcept;
    void return_void();
    void unhandled_exception();
  };
};

namespace cwg2668 { // cwg2668: yes
void f() {
  auto l = []() -> Task { co_await std::suspend_never{}; };
  (void)l;
}
} // namespace cwg2668
