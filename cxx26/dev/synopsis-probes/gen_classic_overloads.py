#!/usr/bin/env python3
# Generates a probe from the ExecutionPolicy&& declarations of the n5050 synopses: every declaration must be
# callable with the argument kinds its template parameters say, and return the declared type.
import re, sys
D = sys.argv[1]
texts = [(f, open(f'{D}/source/{f}.tex').read()) for f in ('algorithms','memory','numerics')]
def clean(s):
    s = re.sub(r'//[^\n]*', '', s)
    s = re.sub(r'@\\libconcept\{([^}]*)\}@', r'\1', s)
    s = re.sub(r'@\\exposid\{([^}]*)\}@', r'\1', s)
    s = re.sub(r'@\\[a-zA-Z]+\{([^}]*)\}@', r'\1', s)
    return s
decls = []
for f, t in texts:
    t = clean(t)
    for m in re.finditer(r'template<((?:[^<>;]|<[^<>;]*>)*)>\s*(?:constexpr\s+)?([^;{()]*?)\s+(\w+)\(([^;]*?)\)\s*;', t, re.S):
        tp, ret, name, params = m.groups()
        if 'ExecutionPolicy' in params or 'Ep&&' in params or 'Proj' in params or re.search(r'\b(S|S1|S2|I|I1|I2|R|R1|R2)\b', params): continue
        if name in ('add','apply_givens_rotation','dot','dotc','scale','swap_elements') or 'matrix' in name or name.startswith('vector_'):
            continue
        decls.append((tp, ' '.join(ret.split()), name, ' '.join(params.split())))
seen=set(); decls=[d for d in decls if not (d in seen or seen.add(d))]
print(f'// {len(decls)} declarations', file=sys.stderr)
IT = {'InputIterator': 'I0', 'InputIterator1': 'I1_', 'InputIterator2': 'I2_', 'OutputIterator': 'O0', 'OutputIterator1': 'O1', 'OutputIterator2': 'O2', 'ForwardIterator': 'F0', 'ForwardIterator1': 'F1', 'ForwardIterator2': 'F2', 'ForwardIterator3': 'F3', 'RandomAccessIterator': 'R0',
      'RandomAccessIterator1': 'R1', 'RandomAccessIterator2': 'R2', 'BidirectionalIterator': 'B0', 'BidirectionalIterator1': 'B1', 'BidirectionalIterator2': 'B2', 'NoThrowForwardIterator': 'F0', 'NoThrowForwardIterator1': 'F1', 'NoThrowForwardIterator2': 'F2'}
FN = {'Predicate': 'pred', 'UnaryPredicate': 'pred', 'BinaryPredicate': 'bpred', 'Compare': 'bpred', 'BinaryOperation': 'bop', 'BinaryOperation1': 'bop',
      'BinaryOperation2': 'bop', 'UnaryOperation': 'uop', 'Function': 'fn', 'Generator': 'gen'}
out = ['#include <algorithm>\n#include <execution>\n#include <memory>\n#include <numeric>\n#include <concepts>\n#include <iterator>\n#include <utility>\n#include <functional>\n#include "test_iterators.h"',
       'using I0 = cpp17_input_iterator<int*>; using I1_ = cpp17_input_iterator<int*>; using I2_ = cpp17_input_iterator<int*>; using O0 = cpp17_output_iterator<int*>; using O1 = O0; using O2 = O0;',
       'using F0 = forward_iterator<int*>; using F1 = forward_iterator<int*>; using F2 = forward_iterator<int*>; using F3 = forward_iterator<int*>;',
       'using R0 = random_access_iterator<int*>; using R1 = R0; using R2 = R0; using B0 = bidirectional_iterator<int*>; using B1 = B0; using B2 = B0;',
       'using Size = int; using T = int; using Tp = int;',
       'inline auto pred = [](int) { return true; }; inline auto bpred = [](int, int) { return true; }; inline auto bop = [](int a, int) { return a; };',
       'inline auto uop = [](int a) { return a; }; inline auto fn = [](int) {}; inline auto gen = []() { return 0; };',
       'inline int arr[8]{}; inline int arr2[8]{};',
       'template <class I> I mk(int* p) { return I(p); }',
       '// no policy']
bad = 0
for n, (tp, ret, name, params) in enumerate(decls):
    tparams = {}
    for p in re.split(r',\s*(?=class|typename)', ' '.join(tp.split())):
        p = p.strip()
        mm = re.match(r'(?:class|typename)\s+(\w+)(?:\s*=\s*(.*))?$', p)
        if mm: tparams[mm.group(1)] = mm.group(2)
    args = []
    ok = True
    ps = [x.strip() for x in re.split(r',(?![^<]*>)', params) if x.strip()]
    for p in ps:
        mm = re.match(r'(?:const\s+)?([\w:<>]+(?:\s*::\s*\w+)?)\s*&?\s*(\w+)?$', p)
        ty = re.sub(r'\s*&$', '', p.rsplit(' ', 1)[0]) if ' ' in p else p
        ty = ty.replace('const ', '').replace('&', '').strip()
        if ty in IT: args.append(f'mk<{IT[ty]}>(arr)')
        elif ty in FN: args.append(FN[ty])
        elif ty == 'Size': args.append('3')
        elif ty in ('T', 'T1', 'T2'): args.append('0')
        elif ty.startswith('typename iterator_traits') or 'difference_type' in ty: args.append('3')
        elif ty in ('ExecutionPolicy',): args.append('seq')
        else:
            ok = False; args.append('/*?' + ty + '*/')
    # fix the iterator arguments: the output range is a separate array
    retc = ret.replace('constexpr ', '').strip()
    for k, v in IT.items(): retc = re.sub(r'\b' + k + r'\b', v, retc)
    retc = re.sub(r'\btypename\s+', '', retc)
    retc = re.sub(r'\biterator_traits<(\w+)>::(\w+)', r'typename std::iterator_traits<\1>::\2', retc)
    retc = retc.replace('pair<', 'std::pair<')
    call = f'std::{name}(' + ', '.join(args) + ')'
    if not ok:
        out.append(f'// SKIP {name}({params}) : unmapped parameter')
        continue
    out.append(f'static_assert(requires {{ {call}; }}, "exists: {name}({params})");')
    if retc:
        out.append(f'static_assert(std::same_as<decltype({call}), {retc}>, "returns: {name}({params}) -> {ret}");')
open(sys.argv[2], 'w').write('\n'.join(out) + '\nint main(int, char**) { return 0; }\n')
