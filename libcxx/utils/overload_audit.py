#!/usr/bin/env python3
# ===----------------------------------------------------------------------===##
#
# Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
# See https://llvm.org/LICENSE.txt for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#
# ===----------------------------------------------------------------------===##

"""
Overload audit (release gate, issue #283, "library semantics: extra overloads").

vocabulary_audit.py proves that no non-reserved *name* is exposed that the draft does not
mention. This script checks the *overload sets*: for every non-reserved function, member
function, constructor and function template that the installed headers declare in namespace
std, the numbers of arguments it can be called with must be a number of arguments of some
declaration of the same name in the draft (synopses and item declarations).

Reported: <name> <number of arguments> (the declaration's location). Anything reported must
be removed, made reserved, or listed in the allow-list with its justification, one entry per
line:   <name> <arguments>  # reason

Usage:
  overload_audit.py --draft <draft checkout> --clang <clang++> --include <installed include dir>
      [--std c++26] [--allow FILE] [--headers FILE]
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

KEYWORDS = set(
    'if for while switch return sizeof alignof noexcept decltype requires static_assert typeof defined '
    'constexpr consteval constinit explicit operator template typename class struct union enum using '
    'namespace new delete throw catch try do else case default co_await co_yield co_return asm '
    'static_cast dynamic_cast reinterpret_cast const_cast typeid alignas'.split())


def split_params(text):
    """Splits the text between the parentheses of a declaration at top-level commas."""
    depth = 0
    parts, cur = [], []
    for ch in text:
        if ch in '(<[{':
            depth += 1
        elif ch in ')>]}':
            depth -= 1
        if ch == ',' and depth == 0:
            parts.append(''.join(cur))
            cur = []
        else:
            cur.append(ch)
    if ''.join(cur).strip() or parts:
        parts.append(''.join(cur))
    return [p.strip() for p in parts if p.strip()]


def _decl_arities(text):
    """Yields (enclosing class or '', name, set of argument counts) for the declarations of a synopsis."""
    text = re.sub(r'@\\[a-z]+\{([^{}]*)\}@', r'\1', text)
    text = re.sub(r'//[^\n]*', '', text)
    stack = []  # ('class', name) | ('ns', '') | ('other', '')
    last = 0
    ident = re.compile(r'(~?[A-Za-z_][A-Za-z_0-9]*)\s*\(')
    i = 0
    n = len(text)
    while i < n:
        c = text[i]
        if c == '{':
            head = text[last:i]
            ms = list(re.finditer(r'\b(class|struct|union)\s+([A-Za-z_][A-Za-z_0-9]*)', head))
            m = ms[-1] if ms else None
            if m and '(' not in head[m.end():] and not re.search(r'\b(enum)\b', head):
                stack.append(('class', m.group(2)))
            elif re.search(r'\bnamespace\b', head):
                nm = re.search(r'\bnamespace\s+([A-Za-z_][A-Za-z_0-9:]*)', head)
                stack.append(('ns', nm.group(1) if nm else ''))
            else:
                stack.append(('other', ''))
            last = i + 1
        elif c == '}':
            if stack:
                stack.pop()
            last = i + 1
        elif c == ';':
            last = i + 1
        else:
            m = ident.match(text, i) if (c.isalpha() or c in '_~') and (i == 0 or not (text[i - 1].isalnum() or text[i - 1] == '_')) else None
            if m:
                name = m.group(1).lstrip('~')
                j, depth = m.end(), 1
                while j < n and depth:
                    if text[j] == '(':
                        depth += 1
                    elif text[j] == ')':
                        depth -= 1
                    j += 1
                if m.group(1).startswith('~'):
                    pass  # destructors are special members, not overloads
                elif name not in KEYWORDS and (not stack or stack[-1][0] in ('class', 'ns')):
                    params = split_params(text[m.end():j - 1])
                    cls = stack[-1][1] if stack and stack[-1][0] == 'class' else ''
                    if not cls and stack and stack[-1][0] == 'ns' and stack[-1][1] != 'std':
                        cls = '<ns:' + stack[-1][1] + '>'
                    if any('...' in p for p in params):
                        yield cls, name, {-1}
                    else:
                        total = len([p for p in params if p != 'void'])
                        required = len([p for p in params if '=' not in p and p != 'void'])
                        yield cls, name, set(range(required, total + 1))
                i = m.end() - 1
        i += 1


def draft_arities(draft):
    """(class or '', name) -> set of argument counts that some declaration of the draft accepts."""
    out = collections.defaultdict(set)
    for f in glob.glob(os.path.join(draft, 'source', '*.tex')):
        txt = open(f, errors='replace').read()
        for m in re.finditer(r'\\begin\{codeblock\}(.*?)\\end\{codeblock\}', txt, re.S):
            for cls, name, ar in _decl_arities(m.group(1)):
                out[(cls, name)].update(ar)
                out[('', name)].update(ar)  # a free function, or any class: the coarse fallback
                if not cls:
                    out[('<free>', name)].update(ar)
        # item declarations: the class comes from the index entries that precede them.
        for m in re.finditer(r'((?:\\indexlibrary[a-z]*\{[^}]*\}(?:\{[^}]*\})?%?\s*)+)\\begin\{itemdecl\}(.*?)\\end\{itemdecl\}', txt, re.S):
            classes = set(re.findall(r'\\indexlibrarymember\{[^}]*\}\{([^}]*)\}', m.group(1)))
            classes |= set(re.findall(r'\\indexlibrary(?:ctor|dtor)\{([^}]*)\}', m.group(1)))
            classes = {c.replace('\\_', '_') for c in classes}
            for cls, name, ar in _decl_arities(m.group(2)):
                for c in classes or {''}:
                    out[(c, name)].update(ar)
                out[('', name)].update(ar)
    return out


def impl_arities(clang, include, std, headers):
    """(name, location) -> set of argument counts, parsed from the AST dump of the headers."""
    with tempfile.NamedTemporaryFile('w', suffix='.cpp', delete=False) as tu:
        for h in headers:
            tu.write('#include <%s>\n' % h)
        tu_name = tu.name
    cmd = [clang, '-std=' + std, '-nostdinc++', '-isystem', include, '-fsyntax-only', '-Xclang', '-ast-dump', tu_name]
    proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.DEVNULL, text=True, errors='replace')
    out = collections.defaultdict(lambda: {'args': set(), 'loc': {}})
    scope = []  # (depth, kind, name)
    cur = None  # (depth, name, loc, params list)

    def finish(c):
        if c is None:
            return
        _, name, loc, params, cls, rsv = c
        if any(p == 'pack' for p in params):
            return
        # A declaration whose parameter types spell a reserved name cannot be called by a program, and a copy
        # or move constructor is a special member function, not an overload.
        sig = loc.split(' :: ', 1)[1] if ' :: ' in loc else ''
        inner = sig[sig.find('(') + 1:sig.rfind(')')] if '(' in sig else ''
        special = False
        m = re.fullmatch(r'(?:const )?([A-Za-z_][A-Za-z_0-9:]*)(?:<.*>)? ?&&?', inner.strip())
        if m and name == cls and m.group(1).split('::')[-1] == cls:
            special = True  # copy or move constructor
        if special:
            return
        total = len(params)
        if rsv:
            # A parameter whose type spells a reserved name cannot be passed by a program, but the defaulted
            # trailing ones can be left out: only the arities before the first of them are callable.
            first = rsv[0]
            if any(p == 'req' for p in params[first:]):
                return
            total = first
        elif re.search(r'\b__', inner):
            return
        required = len([p for p in params[:total] if p == 'req'])
        e = out[(cls, name)]
        e['args'].update(range(required, total + 1))
        for a in range(required, total + 1):
            e['loc'].setdefault(a, loc)

    marker = re.compile(r'^([| ]*)[|`]-')
    decl = re.compile(r'^([A-Za-z]+) 0x[0-9a-f]+ (?:prev 0x[0-9a-f]+ )?<[^>]*>(?: [^ ]*)? ?(.*)$')
    for line in proc.stdout:
        m = marker.match(line)
        if not m:
            continue
        depth = len(m.group(1)) // 2
        body = line[m.end():]
        while scope and scope[-1][0] >= depth:
            scope.pop()
        if cur is not None and depth <= cur[0]:
            finish(cur)
            cur = None
        d = decl.match(body)
        if not d:
            continue
        kind, rest = d.group(1), d.group(2)
        if kind == 'NamespaceDecl':
            name = rest.split()[-1] if rest else ''
            if rest.endswith('inline') or ' inline' in rest:
                name = rest.split()[-2]
            scope.append([depth, 'ns', name.replace('inline', '').strip(), 'public'])
        elif kind in ('CXXRecordDecl', 'ClassTemplateSpecializationDecl', 'ClassTemplatePartialSpecializationDecl') and rest:
            all_toks = rest.split()
            toks = [t for t in all_toks if t not in ('referenced', 'implicit', 'used', 'class', 'struct', 'union',
                                                     'definition', 'invalid', 'implicit_instantiation', 'explicit_specialization',
                                                     'explicit_instantiation_definition', 'explicit_instantiation_declaration')]
            access = 'private' if 'class' in all_toks else 'public'
            scope.append([depth, 'class', toks[-1] if toks else '', access])
        elif kind == 'AccessSpecDecl':
            for sc in reversed(scope):
                if sc[1] == 'class':
                    sc[3] = rest.split()[-1]
                    break
        elif kind in ('FunctionDecl', 'CXXMethodDecl', 'CXXConstructorDecl', 'CXXConversionDecl'):
            names = [s[2] for s in scope if s[2] != '__1']
            if not names or names[0] != 'std' or any(n.startswith('_') for n in names if n):
                continue
            if kind == 'CXXConversionDecl' or 'implicit' in rest.split():
                continue
            if any(sc[1] == 'class' and sc[3] != 'public' for sc in scope):
                continue
            t = rest.split(' ')
            qual = [x for x in t if x and x not in ('referenced', 'used', 'invalid', 'constexpr', 'inline', 'static',
                                                    'extern', 'consteval', 'implicit', 'noexcept', 'virtual',
                                                    'delete', 'default', 'pure', 'implicit-inline', 'cinit',
                                                    'friend', 'explicit')]
            # col:N or line:N:M, then the name, then the quoted type.
            idx = next((i for i, x in enumerate(qual) if x.startswith("'")), None)
            if idx is None or idx == 0:
                continue
            name = qual[idx - 1]
            if name.startswith('_') or name.startswith('operator') or not re.fullmatch(r'[A-Za-z][A-Za-z_0-9]*', name):
                continue
            sig = next((x for x in re.findall(r"'([^']*)'", body)), '')
            cls = next((sc[2] for sc in reversed(scope) if sc[1] == 'class'), '')
            if not cls and names != ['std']:
                cls = '<ns:' + names[-1] + '>'
            cur = (depth, name, ' '.join(names + [name]) + ' :: ' + sig, [], cls, [])
        elif kind == 'ParmVarDecl' and cur is not None and depth == cur[0] + 1:
            req = 'cinit' not in body.split("'")[-1] and 'cinit' not in body
            cur[3].append('req' if req else 'opt')
            if re.search(r'\b__', ' '.join(re.findall(r"'([^']*)'", body)[:1])):
                cur[5].append(len(cur[3]) - 1)
            if '...' in body or 'ParmVarDecl' in body and ' pack' in body:
                cur[3][-1] = 'pack'
    finish(cur)
    proc.wait()
    os.unlink(tu_name)
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--draft', required=True)
    ap.add_argument('--clang', required=True)
    ap.add_argument('--include', required=True)
    ap.add_argument('--std', default='c++26')
    ap.add_argument('--allow')
    ap.add_argument('--headers')
    ap.add_argument('--missing', action='store_true', help='also report draft overloads the library lacks')
    args = ap.parse_args()

    if args.headers:
        headers = open(args.headers).read().split()
    else:
        skip = re.compile(r'^(_.*|.*\.h|.*\.txt|.*\.in|.*\.modulemap|ext|experimental|module.*|ccomplex|ciso646|cstdalign|'
                          r'cstdbool|ctgmath|stdatomic\.h)$')
        headers = sorted(f for f in os.listdir(args.include)
                         if not skip.match(f) and os.path.isfile(os.path.join(args.include, f)))
    allow = set()
    if args.allow:
        for l in open(args.allow):
            l = l.split('#')[0].strip()
            if l:
                allow.add(tuple(l.split()))

    ref = draft_arities(args.draft)
    impl = impl_arities(args.clang, args.include, args.std, headers)
    bad = 0
    for key in sorted(impl):
        cls, name = key
        want = ref.get((cls, name)) or ref.get(('', name))
        if want is None or -1 in want:
            continue  # unknown names are vocabulary_audit's business; variadic draft entries accept anything
        for a in sorted(impl[key]['args']):
            if a not in want and (name, str(a)) not in allow and (cls + '::' + name, str(a)) not in allow:
                print('%s::%s %d  # %s' % (cls, name, a, impl[key]['loc'].get(a, '')))
                bad += 1
    if args.missing:
        # The other direction: a member function the library declares with fewer overloads than the draft's class
        # synopsis (only classes and names both sides declare are compared).
        for (cls, name), want in sorted(ref.items()):
            if not cls or -1 in want:
                continue
            have = impl.get(('' if cls == '<free>' else cls, name))
            if have is None:
                continue
            for a in sorted(want - have['args']):
                if (cls + '::' + name, 'missing-%d' % a) in allow:
                    continue
                if (name == cls and a == 1) or (name == 'swap' and a == 2):
                    continue  # copy and move constructors, and the non-member swap listed in the synopsis
                print('MISSING %s::%s %d' % (cls, name, a))
                bad += 1
    print('overload audit: %d unexplained entries' % bad, file=sys.stderr)
    return 1 if bad else 0


if __name__ == '__main__':
    sys.exit(main())
