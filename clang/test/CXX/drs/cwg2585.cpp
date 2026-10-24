// RUN: %clang_cc1 -std=c++20 -pedantic-errors %s -verify=expected -fexceptions -fcxx-exceptions
// RUN: %clang_cc1 -std=c++23 -pedantic-errors %s -verify=expected -fexceptions -fcxx-exceptions
// RUN: %clang_cc1 -std=c++2c -pedantic-errors %s -verify=expected -fexceptions -fcxx-exceptions

namespace std {
template<class R, class... Args> struct coroutine_traits { using promise_type = typename R::promise_type; };
template<class P = void> struct coroutine_handle;
template<> struct coroutine_handle<void> { static coroutine_handle from_address(void*); void* address() const; };
template<class P> struct coroutine_handle : coroutine_handle<> { static coroutine_handle from_address(void*); static coroutine_handle from_promise(P&); };
struct suspend_never { bool await_ready() const noexcept { return true; } void await_suspend(coroutine_handle<>) const noexcept {} void await_resume() const noexcept {} };
}

namespace cwg2585 { // cwg2585: yes
struct task {
  struct promise_type {
    task get_return_object();
    std::suspend_never initial_suspend();
    std::suspend_never final_suspend() noexcept;
    void return_void();
    void unhandled_exception();
    static void *operator new(unsigned long);
  };
};
task f(int) { co_return; }

struct task2 {
  struct promise_type {
    task2 get_return_object();
    std::suspend_never initial_suspend();
    std::suspend_never final_suspend() noexcept;
    void return_void();
    void unhandled_exception();
    static void *operator new(unsigned long, char *);
  };
};
task2 g(int) { co_return; }
// expected-error@-1 {{'operator new' provided by 'std::coroutine_traits<task2, int>::promise_type' (aka 'typename cwg2585::task2::promise_type') is not usable with the function signature of 'g'}}
} // namespace cwg2585
