#!/usr/bin/env python3
# ===- gen_libcxx_modules.py -  ------------------------------*- python -*--===#
#
# Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
# See https://llvm.org/LICENSE.txt for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#
# ===------------------------------------------------------------------------===#
"""Generates StdCxx26SymbolMap.inc: the standard library names that the
cppreference based StdSymbolMap.inc does not know yet (it is generated from an
offline book of 2024-11), for include-cleaner and clangd's auto-include.

The source is libc++'s module interface partitions (libcxx/modules/std/<header>.inc):
every `using std::[ns::]name;` in <header>.inc is a name that <header> provides.

  gen_libcxx_modules.py --libcxx <libcxx dir> --existing StdSymbolMap.inc \
      --existing StdSpecialSymbolMap.inc --existing StdTsSymbolMap.inc > StdCxx26SymbolMap.inc
"""
import argparse, os, re, sys

USING = re.compile(r"^\s*using\s+(?:std::)((?:[A-Za-z_]\w*::)*)([A-Za-z_]\w*)\s*;")
SYM = re.compile(r"^SYMBOL\((\w+), (None|std::(?:\w+::)*), (<[^>]+>|/\*no headers\*/)")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--libcxx", required=True)
    ap.add_argument("--existing", action="append", default=[])
    args = ap.parse_args()

    known = set()
    known_headers = set()
    for path in args.existing:
        for line in open(path):
            m = SYM.match(line)
            if m:
                ns = "" if m.group(2) == "None" else m.group(2)
                known.add(ns + m.group(1))
                known_headers.add(m.group(3))

    found = {}  # (qualified name) -> (name, ns, header)
    mod_dir = os.path.join(args.libcxx, "modules", "std")
    for fn in sorted(os.listdir(mod_dir)):
        if not fn.endswith(".inc"):
            continue
        header = "<" + fn[:-4] + ">"
        for line in open(os.path.join(mod_dir, fn)):
            m = USING.match(line)
            if not m:
                continue
            parts = [p for p in m.group(1).split("::") if p]
            if any(p.startswith("_") for p in parts):
                parts = [p for p in parts if not p.startswith("_")]
            name = m.group(2)
            if name.startswith("_") or name.startswith("operator"):
                continue
            ns = "std::" + "".join(p + "::" for p in parts)
            q = ns + name
            if q in known:
                continue
            # A name directly in std:: that an existing header lacks was left
            # out of the cppreference map on purpose (std::move is in both
            # <algorithm> and <utility>); only the headers that are new, and
            # the names in namespaces, are added.
            if ns == "std::" and header in known_headers:
                continue
            found[(q, header)] = (name, ns, header)

    out = sys.stdout
    out.write("""//===-- gen_libcxx_modules.py generated file ---------------------*- C++ -*-===//
//
// Standard library names that StdSymbolMap.inc (cppreference, 2024-11) does not
// have, taken from libc++'s module interface files libcxx/modules/std/*.inc.
//
// This file was generated automatically by
// clang/tools/include-mapping/gen_libcxx_modules.py, DO NOT EDIT!
//===----------------------------------------------------------------------===//

""")
    for (q, header), (name, ns, h) in sorted(found.items()):
        out.write("SYMBOL(%s, %s, %s)\n" % (name, ns, h))


if __name__ == "__main__":
    main()
