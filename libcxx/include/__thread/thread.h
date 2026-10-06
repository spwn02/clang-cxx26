// -*- C++ -*-
//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___THREAD_THREAD_H
#define _LIBCPP___THREAD_THREAD_H

#include <__assert>
#include <__concepts/same_as.h>
#include <__condition_variable/condition_variable.h>
#include <__config>
#include <__exception/terminate.h>
#include <__fwd/string.h>
#include <__functional/hash.h>
#include <__functional/unary_function.h>
#include <__locale>
#include <__memory/addressof.h>
#include <__memory/unique_ptr.h>
#include <__mutex/mutex.h>
#include <__system_error/throw_system_error.h>
#include <__thread/id.h>
#include <__thread/support.h>
#include <__type_traits/decay.h>
#include <__type_traits/enable_if.h>
#include <__type_traits/invoke.h>
#include <__type_traits/is_constructible.h>
#include <__type_traits/is_same.h>
#include <__type_traits/remove_cvref.h>
#include <__utility/forward.h>
#include <__utility/integer_sequence.h>
#include <string_view>
#include <tuple>

#if _LIBCPP_HAS_LOCALIZATION
#  include <sstream>
#endif

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_PUSH_MACROS
#include <__undef_macros>

_LIBCPP_BEGIN_NAMESPACE_STD

#if _LIBCPP_HAS_THREADS

template <class _Tp>
class __thread_specific_ptr;
class _LIBCPP_EXPORTED_FROM_ABI __thread_struct;
class _LIBCPP_HIDDEN __thread_struct_imp;
class __assoc_sub_state;

_LIBCPP_EXPORTED_FROM_ABI __thread_specific_ptr<__thread_struct>& __thread_local_data();

class _LIBCPP_EXPORTED_FROM_ABI __thread_struct {
  __thread_struct_imp* __p_;

  __thread_struct(const __thread_struct&);
  __thread_struct& operator=(const __thread_struct&);

public:
  __thread_struct();
  ~__thread_struct();

  void notify_all_at_thread_exit(condition_variable*, mutex*);
  void __make_ready_at_thread_exit(__assoc_sub_state*);
};

template <class _Tp>
class __thread_specific_ptr {
  __libcpp_tls_key __key_;

  // Only __thread_local_data() may construct a __thread_specific_ptr
  // and only with _Tp == __thread_struct.
  static_assert(is_same<_Tp, __thread_struct>::value, "");
  __thread_specific_ptr();
  friend _LIBCPP_EXPORTED_FROM_ABI __thread_specific_ptr<__thread_struct>& __thread_local_data();

  _LIBCPP_HIDDEN static void _LIBCPP_TLS_DESTRUCTOR_CC __at_thread_exit(void*);

public:
  typedef _Tp* pointer;

  __thread_specific_ptr(const __thread_specific_ptr&)            = delete;
  __thread_specific_ptr& operator=(const __thread_specific_ptr&) = delete;
  ~__thread_specific_ptr();

  _LIBCPP_HIDE_FROM_ABI pointer get() const { return static_cast<_Tp*>(__libcpp_tls_get(__key_)); }
  _LIBCPP_HIDE_FROM_ABI pointer operator*() const { return *get(); }
  _LIBCPP_HIDE_FROM_ABI pointer operator->() const { return get(); }
  void set_pointer(pointer __p);
};

template <class _Tp>
void _LIBCPP_TLS_DESTRUCTOR_CC __thread_specific_ptr<_Tp>::__at_thread_exit(void* __p) {
  delete static_cast<pointer>(__p);
}

template <class _Tp>
__thread_specific_ptr<_Tp>::__thread_specific_ptr() {
  int __ec = __libcpp_tls_create(&__key_, &__thread_specific_ptr::__at_thread_exit);
  if (__ec)
    std::__throw_system_error(__ec, "__thread_specific_ptr construction failed");
}

template <class _Tp>
__thread_specific_ptr<_Tp>::~__thread_specific_ptr() {
  // __thread_specific_ptr is only created with a static storage duration
  // so this destructor is only invoked during program termination. Invoking
  // pthread_key_delete(__key_) may prevent other threads from deleting their
  // thread local data. For this reason we leak the key.
}

template <class _Tp>
void __thread_specific_ptr<_Tp>::set_pointer(pointer __p) {
  _LIBCPP_ASSERT_INTERNAL(get() == nullptr, "Attempting to overwrite thread local data");
  std::__libcpp_tls_set(__key_, __p);
}

template <>
struct hash<__thread_id> : public __unary_function<__thread_id, size_t> {
  _LIBCPP_HIDE_FROM_ABI size_t operator()(__thread_id __v) const _NOEXCEPT {
    return hash<__libcpp_thread_id>()(__v.__id_);
  }
};

#  if _LIBCPP_HAS_LOCALIZATION
template <class _CharT, class _Traits>
_LIBCPP_HIDE_FROM_ABI basic_ostream<_CharT, _Traits>&
operator<<(basic_ostream<_CharT, _Traits>& __os, __thread_id __id) {
  // [thread.thread.id]/9
  //   Effects: Inserts the text representation for charT of id into out.
  //
  // [thread.thread.id]/2
  //   The text representation for the character type charT of an
  //   object of type thread::id is an unspecified sequence of charT
  //   such that, for two objects of type thread::id x and y, if
  //   x == y is true, the thread::id objects have the same text
  //   representation, and if x != y is true, the thread::id objects
  //   have distinct text representations.
  //
  // Since various flags in the output stream can affect how the
  // thread id is represented (e.g. numpunct or showbase), we
  // use a temporary stream instead and just output the thread
  // id representation as a string.

  basic_ostringstream<_CharT, _Traits> __sstr;
  __sstr.imbue(locale::classic());
  __sstr << __id.__id_;
  return __os << __sstr.str();
}
#  endif // _LIBCPP_HAS_LOCALIZATION

#  ifndef _LIBCPP_CXX03_LANG

template <class _TSp, class _Fp, class... _Args, size_t... _Indices>
inline _LIBCPP_HIDE_FROM_ABI void __thread_execute(tuple<_TSp, _Fp, _Args...>& __t, __index_sequence<_Indices...>) {
  std::__invoke(std::move(std::get<_Indices + 1>(__t))...);
}

template <class _Fp>
_LIBCPP_HIDE_FROM_ABI void* __thread_proxy(void* __vp) {
  // _Fp = tuple< unique_ptr<__thread_struct>, Functor, Args...>
  unique_ptr<_Fp> __p(static_cast<_Fp*>(__vp));
  __thread_local_data().set_pointer(std::get<0>(*__p.get()).release());
  std::__thread_execute(*__p.get(), __make_index_sequence<tuple_size<_Fp>::value - 1>());
  return nullptr;
}

#  else // _LIBCPP_CXX03_LANG

template <class _Fp>
struct __thread_invoke_pair {
  // This type is used to pass memory for thread local storage and a functor
  // to a newly created thread because std::pair doesn't work with
  // std::unique_ptr in C++03.
  _LIBCPP_HIDE_FROM_ABI __thread_invoke_pair(_Fp& __f) : __tsp_(new __thread_struct), __fn_(__f) {}
  unique_ptr<__thread_struct> __tsp_;
  _Fp __fn_;
};

template <class _Fp>
_LIBCPP_HIDE_FROM_ABI void* __thread_proxy_cxx03(void* __vp) {
  unique_ptr<_Fp> __p(static_cast<_Fp*>(__vp));
  __thread_local_data().set_pointer(__p->__tsp_.release());
  (__p->__fn_)();
  return nullptr;
}

#  endif // _LIBCPP_CXX03_LANG

#  if _LIBCPP_STD_VER >= 26 && !defined(_LIBCPP_CXX03_LANG)

// [thread.attributes]: the thread attribute types are thread::name_hint<char>, thread::stack_size_hint (and none
// else); specialized after the class.
template <class _Tp>
inline constexpr bool __is_thread_attribute_v = false;

template <class _Head, class... _Tail>
inline constexpr bool __thread_attributes_distinct_v = (!is_same_v<_Head, _Tail> && ...) && __thread_attributes_distinct_v<_Tail...>;
template <>
inline constexpr bool __thread_attributes_distinct_v<void> = true;

// A copy of the name of a name_hint (truncated: platforms accept short names only) that is passed to the new thread.
struct __thread_name_storage {
  char __chars_[64];
  size_t __size_;
};

template <size_t... _Indices, class _TSp, class _Name, class _Fp, class... _Args>
_LIBCPP_HIDE_FROM_ABI void __thread_execute_hints(tuple<_TSp, _Name, _Fp, _Args...>& __t, __index_sequence<_Indices...>) {
  std::__invoke(std::move(std::get<_Indices + 2>(__t))...);
}

template <class _Fp>
_LIBCPP_HIDE_FROM_ABI void* __thread_proxy_hints(void* __vp) {
  // _Fp = tuple< unique_ptr<__thread_struct>, __thread_name_storage, Functor, Args...>
  unique_ptr<_Fp> __p(static_cast<_Fp*>(__vp));
  __thread_local_data().set_pointer(std::get<0>(*__p.get()).release());
  const __thread_name_storage& __name = std::get<1>(*__p.get());
  if (__name.__size_ != 0)
    std::__libcpp_thread_set_current_name(__name.__chars_, __name.__size_);
  std::__thread_execute_hints(*__p.get(), __make_index_sequence<tuple_size<_Fp>::value - 2>());
  return nullptr;
}

#  endif // _LIBCPP_STD_VER >= 26 && !defined(_LIBCPP_CXX03_LANG)

class _LIBCPP_EXPORTED_FROM_ABI thread {
  __libcpp_thread_t __t_;

  thread(const thread&);
  thread& operator=(const thread&);

public:
#  if _LIBCPP_STD_VER >= 26 && !defined(_LIBCPP_CXX03_LANG)
  // [thread.attributes], thread attributes
  template <same_as<char> _Tp>
  class name_hint {
  public:
    _LIBCPP_HIDE_FROM_ABI constexpr explicit name_hint(basic_string_view<_Tp> __n) noexcept : __name_(__n) {}
    name_hint(name_hint&&)      = delete;
    name_hint(const name_hint&) = delete;

  private:
    basic_string_view<_Tp> __name_;
    friend class thread;
  };
  _LIBCPP_DIAGNOSTIC_PUSH
  _LIBCPP_CLANG_DIAGNOSTIC_IGNORED("-Wunused-template")
  template <class _Tp>
  name_hint(const _Tp*) -> name_hint<_Tp>;
  template <class _Tp>
  name_hint(basic_string<_Tp>) -> name_hint<_Tp>;
  _LIBCPP_DIAGNOSTIC_POP

  class stack_size_hint {
  public:
    _LIBCPP_HIDE_FROM_ABI constexpr explicit stack_size_hint(size_t __s) noexcept : __size_(__s) {}

  private:
    size_t __size_;
    friend class thread;
  };
#  endif

private:
#  if _LIBCPP_STD_VER >= 26 && !defined(_LIBCPP_CXX03_LANG)
  template <class... _Ts>
  _LIBCPP_HIDE_FROM_ABI static consteval size_t __first_non_attribute_index() {
    constexpr bool __is_function_arg[] = {!__is_thread_attribute_v<_Ts>..., true};
    size_t __i                     = 0;
    while (!__is_function_arg[__i])
      ++__i;
    return __i;
  }

  _LIBCPP_HIDE_FROM_ABI static void __apply_attribute(size_t& __stack_size, __thread_name_storage&, const stack_size_hint& __a) {
    __stack_size = __a.__size_;
  }
  _LIBCPP_HIDE_FROM_ABI static void __apply_attribute(size_t&, __thread_name_storage& __name, const name_hint<char>& __a) {
    const size_t __n = __a.__name_.size() < sizeof(__name.__chars_) - 1 ? __a.__name_.size() : sizeof(__name.__chars_) - 1;
    for (size_t __j = 0; __j != __n; ++__j)
      __name.__chars_[__j] = __a.__name_[__j];
    __name.__chars_[__n] = '\0';
    __name.__size_       = __n;
  }

  template <size_t... _Ai, size_t... _Fi, class... _Ts>
  _LIBCPP_HIDE_FROM_ABI void __start(__index_sequence<_Ai...>, __index_sequence<_Fi...>, tuple<_Ts...>&& __all) {
    constexpr size_t __i = sizeof...(_Ai);
    using _Types         = tuple<_Ts...>; // the elements are the (possibly reference) argument types
    using _Fp            = tuple_element_t<__i, _Types>;
    static_assert(is_constructible_v<__decay_t<_Fp>, _Fp>, "Mandates: the function is constructible from its argument");
    static_assert((is_constructible_v<__decay_t<tuple_element_t<__i + 1 + _Fi, _Types>>, tuple_element_t<__i + 1 + _Fi, _Types>> &&
                   ...),
                  "Mandates: the arguments are constructible from themselves");
    static_assert(__is_invocable_v<__decay_t<_Fp>, __decay_t<tuple_element_t<__i + 1 + _Fi, _Types>>...>,
                  "Mandates: the decayed function is invocable with the decayed arguments");
    static_assert(__thread_attributes_distinct_v<__remove_cvref_t<tuple_element_t<_Ai, _Types>>..., void>,
                  "Mandates: no thread attribute type is present more than once");

    size_t __stack_size = 0;
    __thread_name_storage __name;
    __name.__size_ = 0;
    (std::thread::__apply_attribute(__stack_size, __name, std::get<_Ai>(std::move(__all))), ...);

    typedef unique_ptr<__thread_struct> _TSPtr;
    _TSPtr __tsp(new __thread_struct);
    typedef tuple<_TSPtr,
                  __thread_name_storage,
                  __decay_t<_Fp>,
                  __decay_t<tuple_element_t<__i + 1 + _Fi, _Types>>...>
        _Gp;
    unique_ptr<_Gp> __p(new _Gp(std::move(__tsp),
                                __name,
                                std::forward<_Fp>(std::get<__i>(std::move(__all))),
                                std::forward<tuple_element_t<__i + 1 + _Fi, _Types>>(
                                    std::get<__i + 1 + _Fi>(std::move(__all)))...));
    int __ec = std::__libcpp_thread_create_with_stack_size(
        &__t_, std::addressof(__thread_proxy_hints<_Gp>), __p.get(), __stack_size);
    if (__ec == 0)
      __p.release();
    else
      __throw_system_error(__ec, "thread constructor failed");
  }
#  endif // _LIBCPP_STD_VER >= 26 && !defined(_LIBCPP_CXX03_LANG)

public:
  typedef __thread_id id;
  typedef __libcpp_thread_t native_handle_type;

  _LIBCPP_HIDE_FROM_ABI thread() _NOEXCEPT : __t_(_LIBCPP_NULL_THREAD) {}

#  ifndef _LIBCPP_CXX03_LANG
#    if _LIBCPP_STD_VER < 26
  template <class _Fp, class... _Args, __enable_if_t<!is_same<__remove_cvref_t<_Fp>, thread>::value, int> = 0>
  _LIBCPP_HIDE_FROM_ABI explicit thread(_Fp&& __f, _Args&&... __args) {
    static_assert(is_constructible<__decay_t<_Fp>, _Fp>::value, "");
    static_assert(_And<is_constructible<__decay_t<_Args>, _Args>...>::value, "");
    static_assert(__is_invocable_v<__decay_t<_Fp>, __decay_t<_Args>...>, "");

    typedef unique_ptr<__thread_struct> _TSPtr;
    _TSPtr __tsp(new __thread_struct);
    typedef tuple<_TSPtr, __decay_t<_Fp>, __decay_t<_Args>...> _Gp;
    unique_ptr<_Gp> __p(new _Gp(std::move(__tsp), std::forward<_Fp>(__f), std::forward<_Args>(__args)...));
    int __ec = std::__libcpp_thread_create(&__t_, std::addressof(__thread_proxy<_Gp>), __p.get());
    if (__ec == 0)
      __p.release();
    else
      __throw_system_error(__ec, "thread constructor failed");
  }
#    else // _LIBCPP_STD_VER < 26
  // [thread.thread.constr]: thread(attrs..., f, fargs...): the attributes (thread::name_hint<char>,
  // thread::stack_size_hint) come first and may affect the new thread.
  template <class... _Args>
    requires(sizeof...(_Args) != 0 && !same_as<remove_cvref_t<_Args...[0]>, thread>)
  _LIBCPP_HIDE_FROM_ABI explicit thread(_Args&&... __args) {
    constexpr size_t __i = __first_non_attribute_index<__decay_t<_Args>...>();
    static_assert(__i < sizeof...(_Args), "Mandates: a function to invoke follows the thread attributes");
    if constexpr (__i < sizeof...(_Args)) // otherwise the static_assert above is the only diagnostic
      __start(__make_index_sequence<__i>(),
              __make_index_sequence<sizeof...(_Args) - __i - 1>(),
              tuple<_Args&&...>(std::forward<_Args>(__args)...));
  }
#    endif // _LIBCPP_STD_VER < 26
#  else // _LIBCPP_CXX03_LANG
  template <class _Fp>
  _LIBCPP_HIDE_FROM_ABI explicit thread(_Fp __f) {
    typedef __thread_invoke_pair<_Fp> _InvokePair;
    typedef unique_ptr<_InvokePair> _PairPtr;
    _PairPtr __pp(new _InvokePair(__f));
    int __ec = std::__libcpp_thread_create(&__t_, &__thread_proxy_cxx03<_InvokePair>, __pp.get());
    if (__ec == 0)
      __pp.release();
    else
      __throw_system_error(__ec, "thread constructor failed");
  }
#  endif
  ~thread();

  _LIBCPP_HIDE_FROM_ABI thread(thread&& __t) _NOEXCEPT : __t_(__t.__t_) { __t.__t_ = _LIBCPP_NULL_THREAD; }

  _LIBCPP_HIDE_FROM_ABI thread& operator=(thread&& __t) _NOEXCEPT {
    if (!__libcpp_thread_isnull(&__t_))
      terminate();
    __t_     = __t.__t_;
    __t.__t_ = _LIBCPP_NULL_THREAD;
    return *this;
  }

  _LIBCPP_HIDE_FROM_ABI void swap(thread& __t) _NOEXCEPT { std::swap(__t_, __t.__t_); }

  [[__nodiscard__]] _LIBCPP_HIDE_FROM_ABI bool joinable() const _NOEXCEPT { return !__libcpp_thread_isnull(&__t_); }
  void join();
  void detach();
  [[__nodiscard__]] _LIBCPP_HIDE_FROM_ABI id get_id() const _NOEXCEPT { return __libcpp_thread_get_id(&__t_); }
  [[__nodiscard__]] _LIBCPP_HIDE_FROM_ABI native_handle_type native_handle() _NOEXCEPT { return __t_; }

  [[__nodiscard__]] static unsigned hardware_concurrency() _NOEXCEPT;
};

inline _LIBCPP_HIDE_FROM_ABI void swap(thread& __x, thread& __y) _NOEXCEPT { __x.swap(__y); }

#  if _LIBCPP_STD_VER >= 26 && !defined(_LIBCPP_CXX03_LANG)
template <>
inline constexpr bool __is_thread_attribute_v<thread::stack_size_hint> = true;
template <>
inline constexpr bool __is_thread_attribute_v<thread::name_hint<char>> = true;
#  endif

#endif // _LIBCPP_HAS_THREADS

_LIBCPP_END_NAMESPACE_STD

_LIBCPP_POP_MACROS

#endif // _LIBCPP___THREAD_THREAD_H
