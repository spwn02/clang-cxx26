import re, sys
D=sys.argv[1]
t=open(f'{D}/source/algorithms.tex').read()
t=re.sub(r'//[^\n]*','',t)
t=re.sub(r'@\\[a-zA-Z]+\{([^}]*)\}@',r'\1',t)
decls=[]
for m in re.finditer(r'template<((?:[^<>;]|<[^<>;]*>)*)>\s*constexpr\s+([^;{()]*?)\s+(\w+)\(([^;]*?)\)\s*;',t,re.S):
    tp,ret,name,params=m.groups()
    if 'ExecutionPolicy' in params or 'Ep&&' in params or 'Proj' in params or re.search(r'\b(S|S1|S2|I|I1|I2|R|R1|R2)\b', params): continue
    decls.append((name,' '.join(params.split())))
seen=set(); decls=[d for d in decls if not (d in seen or seen.add(d))]
M={'first':'a','last':'a+8','first1':'a','last1':'a+8','first2':'b','last2':'b+8','result':'c','d_first':'c','n':'3','count':'3','value':'0','old_value':'0','new_value':'0','init':'0','x':'0','pred':'pred','comp':'bpred','binary_op':'bop','binary_op1':'bop','binary_op2':'bop','unary_op':'uop','op':'bop','f':'fn','gen':'gen','middle':'a+4','nth':'a+4','a':'0','b':'0','lo':'0','hi':'9','val':'0','searcher':'std::default_searcher(b, b+2)','k':'1','out':'c'}
out=['#include <algorithm>\n#include <numeric>\n#include <functional>\n#include <iterator>',
'constexpr auto pred=[](int){return true;}; constexpr auto bpred=[](int,int){return false;}; constexpr auto bop=[](int x,int){return x;}; constexpr auto uop=[](int x){return x;}; constexpr auto fn=[](int){}; constexpr auto gen=[](){return 0;};',
'constexpr int rd(int*p){ return p[0]; }']
n=0; skip=0
for name,params in decls:
    ps=[p.strip() for p in re.split(r',(?![^<]*>)',params) if p.strip()]
    args=[]; ok=True
    for p in ps:
        if '=' in p: continue
        nm=p.split()[-1].lstrip('&')
        ty=' '.join(p.split()[:-1]).replace('const ','').replace('&','').strip()
        TY={'BinaryPredicate':'bpred','Compare':'bpred','Comp':'bpred','Cmp':'bpred','Predicate':'pred','UnaryPredicate':'pred','BinaryOperation':'bop','BinaryOperation1':'bop','BinaryOperation2':'bop','UnaryOperation':'uop','Function':'fn','Generator':'gen'}
        if 'Iterator' in ty and nm in ('a','b'): args.append('a' if nm=='a' else 'b')
        elif name.endswith('_backward') and nm=='result': args.append('c+8')
        elif ty in TY: args.append(TY[ty])
        elif nm in M: args.append(M[nm])
        else: ok=False; break
    if not ok: skip+=1; out.append(f'// SKIP {name}({params})'); continue
    n+=1
    out.append(f'constexpr bool cx{n}() {{ int a[16]{{1,2,3,4,5,6,7,8}}, b[16]{{1,2,3,4,5,6,7,8}}, c[16]{{}}; (void)a; (void)b; (void)c; (void)std::{name}({", ".join(args)}); return true; }} static_assert(cx{n}(), "constexpr: {name}({params})");')
out.append('int main(){}')
open(sys.argv[2],'w').write('\n'.join(out)+'\n'); print(len(decls),'decls',skip,'skipped')
