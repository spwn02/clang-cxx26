import re, sys
D=sys.argv[1]
t=open(f'{D}/source/memory.tex').read()
t=re.sub(r'//[^\n]*','',t)
t=re.sub(r'@\\[a-zA-Z]+\{([^}]*)\}@',r'\1',t)
decls=[]
for m in re.finditer(r'\b(\w+)\(Ep&&\s*exec,((?:[^;()]|\([^()]*\))*)\)\s*;',t):
    name,params=m.groups()
    decls.append((name,' '.join(params.split())))
seen=set(); decls=[d for d in decls if not (d in seen or seen.add(d))]
M={'first':'a','last':'a+8','r':'sp(a)','first1':'a','last1':'a+8','first2':'b','last2':'b+8','r1':'sp(a)','r2':'sp(b)','result':'c','ifirst':'a','ilast':'a+8','in_range':'sp(a)','out_range':'sp(c)','ofirst':'c','olast':'c+8','result_first':'c','result_last':'c+8','count':'3','n':'3','value':'0','old_value':'0','new_value':'0','init':'0','pred':'pred','f':'fn','gen':'gen','binary_op':'bop','unary_op':'uop','x':'0','middle':'a+4','mid':'a+4','nth':'a+4','out':'c','o_first':'c','o_last':'c+8','out_r':'sp(c)','result_r':'sp(c)','first_':'a','comp':'bpred','op':'uop','offset':'1','k':'1','val':'0'}
out=['#include <algorithm>\n#include <execution>\n#include <numeric>\n#include <memory>\n#include <span>\n#include <ranges>\n#include <functional>',
'inline int a[8]{}, b[8]{}, c[8]{};','inline auto pred=[](int){return true;}; inline auto bpred=[](int,int){return true;}; inline auto bop=[](int x,int){return x;}; inline auto uop=[](int x){return x;}; inline auto fn=[](int){}; inline auto gen=[](){return 0;};',
'inline std::span<int> sp(int* p){return std::span<int>(p,8);}','inline constexpr auto& seq = std::execution::seq;']
skip=0
for name,params in decls:
    ps=[p.strip() for p in re.split(r',(?![^<]*>)',params) if p.strip()]
    args=[]; ok=True
    for p in ps:
        if '=' in p: continue
        nm=p.split()[-1].lstrip('&')
        if p.startswith('iterator_t<R>'): args.append('std::span<int>(a).begin()+4'); continue
        if nm in M: args.append(M[nm])
        else: ok=False; break
    if not ok: skip+=1; out.append(f'// SKIP {name}({params})'); continue
    out.append(f'static_assert(requires {{ std::ranges::{name}(seq, {", ".join(args)}); }}, "{name}({params})");')
out.append('int main(){}')
open(sys.argv[2],'w').write('\n'.join(out)+'\n'); print(len(decls),'decls',skip,'skipped')
