#!/usr/bin/env python3
"""Qualify the unqualified helper calls that libcpp-robust-against-adl flags (written for #299).

usage: adl-qualify.py <diag-file>   (lines: header:line:col: libcpp-robust-against-adl: ADL lookup, paths relative to libcxx/include)
Edits libcxx/include in place. For each diagnostic it reads the callee name at the column, finds the namespace that
declares it (a namespace tracker over `namespace X {` / `} // namespace` / _LIBCPP_BEGIN/END_NAMESPACE_STD, same file first,
then the whole include tree) and prefixes the shortest correct qualifier. Names the standard resolves through ADL, macro
invocations, types/functional casts and anything without exactly one declaring namespace are left and printed.
ALWAYS follow with the header sweep AND the tests: a qualified call to a function declared later only fails when instantiated
(hive's erase called erase_if before its definition)."""
import os, re, sys, collections
INC = '/home/spawn/dev/toolchains/clang-p2996/libcxx/include'
os.chdir(INC)
EXEMPT = {'conj', 'real', 'imag', 'abs', 'sqrt', '__abs_if_needed', '__conj_if_needed', '__real_if_needed', '__imag_if_needed',
          'submdspan_mapping', 'from_stream', 'reserve_hint', '__simd_select_impl', 'operator', 'swap', 'iter_swap', 'iter_move',
          'permute', 'string'}
nsopen = re.compile(r'^\s*(?:inline\s+)?namespace\s+([\w:]+)\s*\{')
nsclose = re.compile(r'^\s*\}\s*//\s*namespace\b')

def ns_at(lines):
    st, out = [], []
    for ln in lines:
        out.append(tuple(x for p in st for x in p))
        if 'namespace' in ln or 'NAMESPACE_STD' in ln:
            if '_LIBCPP_BEGIN_NAMESPACE_STD' in ln and 'define' not in ln: st.append(('std',))
            elif '_LIBCPP_END_NAMESPACE_STD' in ln and 'define' not in ln:
                if st: st.pop()
            else:
                m = nsopen.match(ln)
                if m: st.append(tuple(m.group(1).split('::')))
                elif nsclose.match(ln) and st: st.pop()
    return out

cache = {}
def get(f):
    if f not in cache:
        L = open(f).read().split('\n'); cache[f] = (L, ns_at(L))
    return cache[f]

ALL = [os.path.join(r, x)[2:] for r, d, fs in os.walk('.') for x in fs]

def decl_ns(name, prefer):
    pat = re.compile(r'(?<![\w.:>])' + re.escape(name) + r'\s*[(<]')
    res = collections.Counter()
    for f in [prefer] + [x for x in ALL if x != prefer]:
        L, N = get(f)
        for i, s in enumerate(L):
            m = pat.search(s)
            if not m: continue
            before = s[:m.start()]
            if re.search(r'[(=.]|return|->|\bif\b|#', before): continue
            if not re.search(r'[\w>\*&]\s+$', before) and before.strip(): continue
            if not before.strip() and (i == 0 or not re.search(r'(_LIBCPP_\w+|constexpr|void|auto|bool|\w+[>*&])\s*$', L[i - 1])): continue
            if N[i]: res[N[i]] += 1
        if res: break
    return res

by = collections.defaultdict(list)
for ln in open(sys.argv[1]):
    if 'robust-against-adl' in ln:
        f, l, c, _ = ln.split(':', 3); by[f].append((int(l), int(c)))
manual, edits = [], 0
for f, ds in by.items():
    L, N = get(f)
    for l, c in sorted(ds, reverse=True):
        s = L[l - 1]; m = re.match(r'[A-Za-z_]\w*', s[c - 1:])
        if not m: manual.append((f, l, 'noident')); continue
        name = m.group(0)
        if name in EXEMPT or name.startswith('_LIBCPP_') or (name[0] == '_' and name[1].isupper()):
            manual.append((f, l, 'exempt/macro/type', name)); continue
        r = decl_ns(name, f)
        if len(r) != 1: manual.append((f, l, 'decl-ns ' + str(dict(r)), name)); continue
        decl = list(r)[0]; call = N[l - 1]; k = 0
        while k < len(decl) and k < len(call) and decl[k] == call[k]: k += 1
        q = '::'.join(decl[k:]) or decl[-1]
        L[l - 1] = s[:c - 1] + q + '::' + s[c - 1:]; edits += 1
    open(f, 'w').write('\n'.join(L)); cache.pop(f, None)
print('edits', edits, 'manual', len(manual))
for x in manual: print(x)
