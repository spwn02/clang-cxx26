#!/usr/bin/env python3
# ===----------------------------------------------------------------------===##
#
# Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
# See https://llvm.org/LICENSE.txt for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#
# ===----------------------------------------------------------------------===##

"""
Vocabulary audit: finds public names that the installed library exposes but the
C++ draft does not mention (release gate, see issue #283).

It checks three things against a checkout of https://github.com/cplusplus/draft
at the tag the release targets:

  1. headers shipped by the library that are not in the draft's header tables,
  2. non-reserved names of the library (every declaration the compiler sees,
     including class members and template parameters, via -ast-list) whose last
     component never appears in a code context of the draft,
  3. non-reserved macros defined after including every header, and every
     __cpp_lib_* macro that is not in the draft.

Anything reported must be removed, renamed to a reserved spelling, gated behind
an explicit opt-in, or listed (with the justification) in the allow-list file,
one entry per line:  <kind> <name>  # reason, with kind in header/name/macro.

Usage:
  vocabulary_audit.py --draft <draft checkout> --clang <clang++> \
      --include <installed libc++ include dir> [--std c++26] [--allow FILE]
Exit status is 1 if there is an unexplained entry.
"""

import argparse
import collections
import glob
import os
import re
import subprocess
import sys
import tempfile

IDENT = re.compile(r'[A-Za-z_][A-Za-z0-9_]*')


def draft_reference(draft):
    ref = set()
    headers = set()
    lib_macros = set()
    inline = (r'\\(?:tcode|exposid|libglobal|libmember|keyword|term|grammarterm|placeholder|indexlibrary[a-z]*|'
              r'indexdefn|indexgrammar|libheader|libheaderdef|libheaderref|libheaderrefx|libspec|defnlibxname|'
              r'defnxname|mname)\{([^{}]*(?:\{[^{}]*\}[^{}]*)*)\}')
    for f in glob.glob(os.path.join(draft, 'source', '*.tex')):
        txt = open(f, errors='replace').read()
        for m in re.finditer(r'\\begin\{(codeblock|codeblockdigitsep|itemdecl|outputblock)\}(.*?)\\end\{\1\}', txt, re.S):
            ref.update(IDENT.findall(m.group(2)))
        for m in re.finditer(inline, txt):
            ref.update(IDENT.findall(m.group(1)))
        for m in re.finditer(r'\\indexlibrary[a-z]*\{([^}]*)\}(?:\{([^}]*)\})?', txt):
            for g in m.groups():
                if g:
                    ref.update(IDENT.findall(g))
        for m in re.finditer(r'\\tcode\|([^|]*)\|', txt):
            ref.update(IDENT.findall(m.group(1)))
        for m in re.finditer(r'\\(?:libheader|libheaderdef|libheaderref|libheaderrefx)\{([^}]+)\}', txt):
            headers.add(m.group(1))
        lib_macros.update('__' + m for m in re.findall(r'cpp_lib_[a-z0-9_]+', txt))
    return ref, headers, lib_macros


def public_headers(include_dir):
    names = []
    for e in sorted(os.listdir(include_dir)):
        p = os.path.join(include_dir, e)
        if os.path.isfile(p) and not e.startswith(('_', '.')) and '.' not in e and e not in ('version',):
            names.append(e)
    return names


def run(cmd, **kw):
    r = subprocess.run(cmd, capture_output=True, text=True, **kw)
    return r


def reserved(c):
    return c.startswith('_') or '__' in c


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--draft', required=True)
    ap.add_argument('--clang', required=True)
    ap.add_argument('--include', required=True)
    ap.add_argument('--std', default='c++26')
    ap.add_argument('--allow')
    ap.add_argument('--flags', default='-freflection')
    args = ap.parse_args()

    allow = set()
    if args.allow:
        for line in open(args.allow):
            line = line.split('#')[0].strip()
            if line:
                kind, name = line.split(None, 1)
                allow.add((kind, name.strip()))

    ref, draft_headers, draft_lib_macros = draft_reference(args.draft)
    ref.add('std')
    problems = []

    # 1. headers
    headers = public_headers(args.include)
    for h in headers:
        if h not in draft_headers and ('header', h) not in allow:
            problems.append(('header', h))

    # 2. names
    with tempfile.TemporaryDirectory() as td:
        tu = os.path.join(td, 'all.cpp')
        with open(tu, 'w') as f:
            for h in headers:
                f.write('#if __has_include(<%s>)\n#include <%s>\n#endif\n' % (h, h))
        base = [args.clang, '-std=' + args.std, '-nostdinc++', '-I', args.include] + args.flags.split()
        r = run(base + ['-fsyntax-only', '-Xclang', '-ast-list', tu])
        if r.returncode != 0 and not r.stdout:
            sys.stderr.write(r.stderr[:2000])
            return 2
        by_last = collections.defaultdict(set)
        for line in r.stdout.splitlines():
            q = line.strip()
            if not q.startswith('std'):
                continue
            comps = q.split('::')
            if any(reserved(c) or '(' in c or '<' in c or c.startswith(('operator', '~')) for c in comps):
                continue
            if comps[-1] in ref:
                continue
            by_last[comps[-1]].add(q)
        for last, qs in sorted(by_last.items()):
            q = sorted(qs)[0]
            if ('name', q) not in allow and ('name', last) not in allow:
                problems.append(('name', q + (' (+%d more)' % (len(qs) - 1) if len(qs) > 1 else '')))

        # 3. macros defined by the library's own headers (system headers define many more)
        r = run(base + ['-E', '-dD', tu])
        inc = os.path.realpath(args.include)
        in_lib = False
        for line in r.stdout.splitlines():
            m = re.match(r'#\s+\d+\s+"([^"]*)"', line)
            if m:
                in_lib = os.path.realpath(m.group(1)).startswith(inc + os.sep) or m.group(1).startswith(args.include)
                continue
            m = re.match(r'#define\s+([A-Za-z_][A-Za-z0-9_]*)', line)
            if not m or not in_lib:
                continue
            name = m.group(1)
            if name.startswith('__cpp_lib_'):
                if name not in draft_lib_macros and ('macro', name) not in allow:
                    problems.append(('macro', name))
            elif not name.startswith('_') and name not in ref and ('macro', name) not in allow:
                problems.append(('macro', name))

    for kind, name in problems:
        print('%s %s' % (kind, name))
    sys.stderr.write('%d unexplained entries\n' % len(problems))
    return 1 if problems else 0


if __name__ == '__main__':
    sys.exit(main())
