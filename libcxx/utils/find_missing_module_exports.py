#!/usr/bin/env python3
"""Find libc++ class/struct templates whose top-level name doesn't appear to be
re-exported from the corresponding libcxx/modules/std/<header>.inc file.

Motivation: libcxx/modules/std/*.inc files (the C++20 named-module `import std;`
interface) are maintained by hand, separately from the ordinary headers under
libcxx/include/. Several real bugs have been found where a facility was fully
implemented and wired into its `#include`-based header, but the corresponding
`.inc` file's `using std::...;` export was never added (or, worse, contained
stale hand-written duplicate logic instead of a re-export) -- so `import std;`
either couldn't see the facility at all, or saw a different, wrong
implementation than `#include <header>` did. See spwn02/clang-cxx26 issues
#104 and the "constant_wrapper/sync_wait_t" follow-up for real examples this
script would have caught.

This is a heuristic, not a verifier: it flags *candidates* for manual review,
not confirmed bugs. As of this script's introduction, running it against a
clean tree reports exactly two candidates, both confirmed NOT bugs -- worked
examples of the false-positive categories below:
  - `type_list` (__execution/completion_signatures.h): despite lacking a
    leading underscore, it's a forward-declared, exposition-only
    metaprogramming helper, never used outside other `__`-prefixed internal
    machinery. Names with a leading underscore are always this category;
    occasionally (as here) a name without one still is -- read the
    surrounding code, don't rely on the naming convention alone.
  - `get_completion_domain_t` (__execution/domain.h): deliberately-incomplete
    internal machinery (see spwn02/clang-cxx26 issue #10) with no working
    `operator()` yet, not a real end-user-facing entry point -- correctly
    unexported until that lands for real.

Other false-positive categories seen (and fixed in the regex/filtering below,
kept here as documentation in case a future header reintroduces the shape):
  - A name already exported via `_LIBCPP_USING_IF_EXISTS` (e.g. `atomic`,
    `atomic_ref`) won't match a plain `using std::Name;` regex -- check for
    it manually if the export line uses that macro.
  - An explicit/partial specialization of an already-exported primary
    template (e.g. `formatter<path>`, `pointer_traits<__wrap_iter<_It>>`) is
    not named via its own `using`-declaration at all -- only the primary
    template needs exporting, and only if the specialization's header is
    actually reachable when building the module (no
    `#if !defined(_LIBCPP_BUILDING_STD_MODULE)`-style exclusion guard on its
    `#include` -- that exact guard, on enumerate_view.h, was itself once a
    real bug; see the commit that added this script).
  - A nested member type (e.g. `some_distribution::param_type`) is not a
    top-level entity and was never meant to be exported by that bare name --
    filtered out by requiring zero leading whitespace before the
    declaration, since this codebase writes namespace-scope declarations
    unindented.

Usage:
    python3 libcxx/utils/find_missing_module_exports.py [libcxx-root]

Exits 0 always -- this is an audit aid to eyeball, not a pass/fail gate.
"""

import os
import re
import sys

CLASS_RE = re.compile(r"^(class|struct)\s+(?:_LIBCPP_[A-Z_]+\s+)?([A-Za-z_][A-Za-z0-9_]*)\s*[:{;]")
CPO_RE = re.compile(r"inline constexpr auto\s+([a-z_][a-z0-9_]*)\s*=")


def iter_impl_dirs(include_dir):
    for name in sorted(os.listdir(include_dir)):
        if name.startswith("__") and os.path.isdir(os.path.join(include_dir, name)):
            yield name[2:], os.path.join(include_dir, name)


def extract_names(path, pattern, group):
    # Only match lines with zero leading whitespace: this codebase writes
    # namespace-scope declarations unindented, so an indented `struct foo {`
    # is a nested member type (e.g. `distribution::param_type`), not a
    # top-level entity -- those aren't exported by name on their own and
    # must not be flagged.
    names = set()
    try:
        with open(path, encoding="utf-8", errors="replace") as f:
            for line in f:
                if line[:1].isspace():
                    continue
                m = pattern.match(line.rstrip("\n"))
                if m:
                    name = m.group(group)
                    if not name.startswith("_"):
                        names.add(name)
    except OSError:
        pass
    return names


def main():
    root = sys.argv[1] if len(sys.argv) > 1 else "."
    include_dir = os.path.join(root, "libcxx", "include")
    modules_dir = os.path.join(root, "libcxx", "modules", "std")

    findings = []
    for header, impl_dir in iter_impl_dirs(include_dir):
        inc_path = os.path.join(modules_dir, header + ".inc")
        if not os.path.isfile(inc_path):
            continue
        with open(inc_path, encoding="utf-8", errors="replace") as f:
            inc_text = f.read()

        for fname in sorted(os.listdir(impl_dir)):
            if not fname.endswith(".h"):
                continue
            fpath = os.path.join(impl_dir, fname)
            class_names = extract_names(fpath, CLASS_RE, 2)
            for name in class_names:
                if not re.search(r"using std::(?:[a-z_:]*::)?" + re.escape(name) + r"\b", inc_text):
                    findings.append((header, fname, "class/struct", name))

    if not findings:
        print("No candidates found.")
        return 0

    print(f"{len(findings)} candidate(s) -- review each against the false-positive")
    print("categories in this script's module docstring before treating as a real bug:\n")
    for header, fname, kind, name in findings:
        print(f"  {name}\t({kind}, from {fname}, expected in modules/std/{header}.inc)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
