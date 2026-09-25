# ===----------------------------------------------------------------------===##
#
# Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
# See https://llvm.org/LICENSE.txt for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#
# ===----------------------------------------------------------------------===##

# Every public header must be includable in every language mode the library
# supports, not just the mode the lit configuration defaults to. Headers that
# implement a newer standard's facilities must be empty (or fall back to the old
# behaviour) in the older modes. Without this test, C++23/26-only code leaking
# into C++20 mode (e.g. P2278R4's constant_iterator in <ranges>) went unnoticed
# because every run used the newest -std.

# RUN: %{python} %s %{libcxx-dir}/utils
# END.

import sys

sys.path.append(sys.argv[1])
from libcxx.header_information import (
    lit_header_restrictions,
    lit_header_undeprecations,
    public_headers,
)

standards = ["c++17", "c++20", "c++23", "c++26"]

for header in public_headers:
    for std in standards:
        print(
            f"""\
//--- {header}.{std.replace('+', 'x')}.sh.cpp
{lit_header_restrictions.get(header, '')}
{lit_header_undeprecations.get(header, '')}

// The lit configuration's own -std comes first in %{{compile_flags}}; ours wins.
// RUN: %{{cxx}} %{{flags}} %{{compile_flags}} -std={std} -fsyntax-only %s

#include <{header}>
"""
        )
