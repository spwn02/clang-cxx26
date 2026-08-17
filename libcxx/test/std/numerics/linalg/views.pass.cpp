//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <linalg>

#include <linalg>

#include <complex>
#include <type_traits>

#include "test_macros.h"

namespace adl_complex {
struct number {
  int real;
  int imag;

  friend constexpr bool operator==(number, number) = default;
  friend constexpr number conj(number value) { return {value.real, -value.imag}; }
};
} // namespace adl_complex

struct offset_accessor {
  using offset_policy    = std::default_accessor<const int>;
  using element_type     = int;
  using reference        = int&;
  using data_handle_type = int*;

  constexpr reference access(data_handle_type handle, size_t index) const noexcept { return handle[index]; }
  constexpr typename offset_policy::data_handle_type offset(data_handle_type handle, size_t index) const noexcept {
    return handle + index;
  }
};

constexpr bool test() {
  int vector_data[] = {1, 2, 3};
  std::mdspan vector(vector_data, 3);
  auto scaled = std::linalg::scaled(2.5, vector);
  ASSERT_SAME_TYPE(typename decltype(scaled)::element_type, const double);
  if (scaled[0] != 2.5 || scaled[2] != 7.5)
    return false;

  std::complex<double> complex_data[] = {{1.0, 2.0}, {-3.0, 4.0}};
  std::mdspan complex_vector(complex_data, 2);
  auto conjugated = std::linalg::conjugated(complex_vector);
  if (conjugated[0] != std::complex<double>(1.0, -2.0) || conjugated[1] != std::complex<double>(-3.0, -4.0))
    return false;

  adl_complex::number adl_complex_data[] = {{1, 2}, {-3, 4}};
  std::mdspan adl_complex_vector(adl_complex_data, 2);
  auto adl_conjugated = std::linalg::conjugated(adl_complex_vector);
  ASSERT_SAME_TYPE(typename decltype(adl_conjugated)::accessor_type::element_type, const adl_complex::number);
  if (adl_conjugated[0] != adl_complex::number{1, -2} || adl_conjugated[1] != adl_complex::number{-3, -4})
    return false;
  auto adl_conjugated_twice  = std::linalg::conjugated(adl_conjugated);
  using adl_complex_accessor = std::default_accessor<adl_complex::number>;
  ASSERT_SAME_TYPE(typename decltype(adl_conjugated_twice)::accessor_type, adl_complex_accessor);
  if (adl_conjugated_twice[0] != adl_complex_data[0])
    return false;

  int offset_data[]                = {1, 2};
  using scaled_offset_accessor     = std::linalg::scaled_accessor<int, offset_accessor>;
  using conjugated_offset_accessor = std::linalg::conjugated_accessor<offset_accessor>;
  static_assert(std::same_as<decltype(scaled_offset_accessor{}.offset(offset_data, 1)), const int*>);
  static_assert(std::same_as<decltype(conjugated_offset_accessor{}.offset(offset_data, 1)), const int*>);

  int matrix_data[] = {1, 2, 3, 4, 5, 6};
  std::mdspan matrix(matrix_data, 2, 3);
  auto transposed = std::linalg::transposed(matrix);
  if (transposed.extent(0) != 3 || transposed.extent(1) != 2 || transposed.stride(0) != matrix.stride(1) ||
      transposed.stride(1) != matrix.stride(0) || transposed[0, 1] != matrix[1, 0] || transposed[2, 1] != matrix[1, 2])
    return false;

  std::complex<double> complex_matrix_data[] = {{1.0, 2.0}, {9.0, 8.0}, {3.0, -4.0}, {7.0, 6.0}};
  std::mdspan complex_matrix(complex_matrix_data, 2, 2);
  auto conjugate_transposed = std::linalg::conjugate_transposed(complex_matrix);
  if (conjugate_transposed[0, 1] != std::complex<double>(3.0, 4.0) ||
      conjugate_transposed[1, 0] != std::complex<double>(9.0, -8.0))
    return false;

  return true;
}

// [linalg.scaled.scaledaccessor] / [linalg.conj.conjugatedaccessor] fix the
// member typedefs exactly; in particular `reference` is remove_const_t of
// `element_type`, not `element_type` itself.
namespace accessor_conformance {

using nested   = std::default_accessor<double>;
using scaled_a = std::linalg::scaled_accessor<double, nested>;

static_assert(std::is_same_v<scaled_a::element_type, const double>);
static_assert(std::is_same_v<scaled_a::reference, double>);
static_assert(std::is_same_v<scaled_a::data_handle_type, nested::data_handle_type>);
static_assert(std::is_same_v<scaled_a::offset_policy, std::linalg::scaled_accessor<double, nested::offset_policy>>);
static_assert(std::is_same_v<decltype(std::declval<const scaled_a&>().access(nullptr, 0)), double>);

using conj_nested = std::default_accessor<std::complex<double>>;
using conj_a      = std::linalg::conjugated_accessor<conj_nested>;

static_assert(std::is_same_v<conj_a::element_type, const std::complex<double>>);
static_assert(std::is_same_v<conj_a::reference, std::complex<double>>);
static_assert(std::is_same_v<conj_a::offset_policy, std::linalg::conjugated_accessor<conj_nested::offset_policy>>);
static_assert(std::is_same_v<decltype(std::declval<const conj_a&>().access(nullptr, 0)), std::complex<double>>);

// The nested-accessor constructor is not explicit.
static_assert(std::is_convertible_v<conj_nested, conj_a>);

} // namespace accessor_conformance

int main(int, char**) {
  static_assert(test());
  return test() ? 0 : 1;
}
