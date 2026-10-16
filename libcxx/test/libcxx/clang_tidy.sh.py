# ===----------------------------------------------------------------------===##
#
# Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
# See https://llvm.org/LICENSE.txt for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#
# ===----------------------------------------------------------------------===##

# REQUIRES: has-clang-tidy

# The diagnostics that are known (code that is identical to the base revision of this repository) are listed in
# clang_tidy.expected; a new diagnostic fails the test and so does a listed one that is gone.
# RUN: %{python} %{libcxx-dir}/utils/check_clang_tidy_expected.py %{libcxx-dir}/test/libcxx/clang_tidy.expected -- \
# RUN:      %{python} %{libcxx-dir}/../clang-tools-extra/clang-tidy/tool/run-clang-tidy.py   \
# RUN:      -clang-tidy-binary %{clang-tidy}                                            \
# RUN:      -warnings-as-errors "*"                                                     \
# RUN:      -source-filter=".*libcxx/src.*"                                             \
# RUN:      -quiet -p %{bin-dir}/..
