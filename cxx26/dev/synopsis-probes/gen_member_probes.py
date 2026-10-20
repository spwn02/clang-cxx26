#!/usr/bin/env python3
# Member-function probes from the draft class synopses: for a list of class templates instantiated with int-like
# arguments, every member (constructor or function) of the synopsis whose parameter types can be named gets a
# probe: is_constructible / the call is well-formed and (when the return type can be named) returns exactly it.
# usage: genmembers.py <draft> <out.cpp>
import re, sys, os
D, OUT = sys.argv[1:3]
SRC=['containers','strings','utilities','memory','ranges','iterators','numerics']
text={f:open(f'{D}/source/{f}.tex').read() for f in SRC if os.path.exists(f'{D}/source/{f}.tex')}
def clean(s):
    s=re.sub(r'//[^\n]*','',s)
    s=re.sub(r'@\\[a-zA-Z]+\{((?:[^{}]|\{[^{}]*\})*)\}@',r'\1',s)
    s=re.sub(r'@\\[a-zA-Z]+@','',s)
    return s
bodies={}
for f,t in text.items():
    t=clean(t)
    for m in re.finditer(r'(?:template<(?:[^<>]|<[^<>]*>)*>\s*)?(?:class|struct)\s+(\w+)\s*(?:final\s*)?(?::[^{;]*)?\{',t):
        name=m.group(1); i=m.end(); depth=1
        while depth and i<len(t):
            c=t[i]
            if c=='{': depth+=1
            elif c=='}': depth-=1
            i+=1
        bodies.setdefault(name,[]).append(t[m.end():i-1])
STD={'pair','initializer_list','allocator_arg_t','from_range_t','nullptr_t','tuple','less','equal_to','hash','allocator','reference_wrapper','sorted_unique_t','sorted_equivalent_t','hive_limits','in_place_t','nullopt_t','strong_ordering','string_view','basic_string_view','char_traits','move_only_function','function','byte','ptrdiff_t','size_t','vector','allocator_traits','dynamic_extent','span','optional','unexpected','expected','monostate'}
KW={'const','volatile','unsigned','int','bool','void','char','long','short','double','float','noexcept','auto','decltype','typename','operator','struct','class','static','constexpr','explicit','virtual','override','final','default','delete','true','false','nullptr','requires','sizeof','is_nothrow_move_constructible_v','is_same_v','is_constructible_v'}
CLASSES={
 'vector':('std::vector<int>',{'T':'int','Allocator':'std::allocator<int>'}),
 'deque':('std::deque<int>',{'T':'int','Allocator':'std::allocator<int>'}),
 'list':('std::list<int>',{'T':'int','Allocator':'std::allocator<int>'}),
 'forward_list':('std::forward_list<int>',{'T':'int','Allocator':'std::allocator<int>'}),
 'array':('std::array<int,3>',{'T':'int','N':'3'}),
 'inplace_vector':('std::inplace_vector<int,4>',{'T':'int','N':'4'}),
 'hive':('std::hive<int>',{'T':'int','Allocator':'std::allocator<int>'}),
 'map':('std::map<int,int,std::less<>>',{'Key':'int','T':'int','Compare':'std::less<>','Allocator':'std::allocator<std::pair<const int,int>>'}),
 'multimap':('std::multimap<int,int,std::less<>>',{'Key':'int','T':'int','Compare':'std::less<>','Allocator':'std::allocator<std::pair<const int,int>>'}),
 'set':('std::set<int,std::less<>>',{'Key':'int','Compare':'std::less<>','Allocator':'std::allocator<int>'}),
 'multiset':('std::multiset<int,std::less<>>',{'Key':'int','Compare':'std::less<>','Allocator':'std::allocator<int>'}),
 'unordered_map':('std::unordered_map<int,int,THash,TEq>',{'Key':'int','T':'int','Hash':'THash','Pred':'TEq','Allocator':'std::allocator<std::pair<const int,int>>'}),
 'unordered_set':('std::unordered_set<int,THash,TEq>',{'Key':'int','Hash':'THash','Pred':'TEq','Allocator':'std::allocator<int>'}),
 'unordered_multimap':('std::unordered_multimap<int,int,THash,TEq>',{'Key':'int','T':'int','Hash':'THash','Pred':'TEq','Allocator':'std::allocator<std::pair<const int,int>>'}),
 'unordered_multiset':('std::unordered_multiset<int,THash,TEq>',{'Key':'int','Hash':'THash','Pred':'TEq','Allocator':'std::allocator<int>'}),
 'flat_map':('std::flat_map<int,int,std::less<>>',{'Key':'int','T':'int','Compare':'std::less<>','KeyContainer':'std::vector<int>','MappedContainer':'std::vector<int>'}),
 'flat_set':('std::flat_set<int,std::less<>>',{'Key':'int','Compare':'std::less<>','KeyContainer':'std::vector<int>'}),
 'basic_string':('std::basic_string<char>',{'charT':'char','traits':'std::char_traits<char>','Allocator':'std::allocator<char>'}),
 'basic_string_view':('std::basic_string_view<char>',{'charT':'char','traits':'std::char_traits<char>'}),
 'span':('std::span<int>',{'ElementType':'int','Extent':'std::dynamic_extent'}),
 'optional':('std::optional<int>',{'T':'int'}),
 'stack':('std::stack<int>',{'T':'int','Container':'std::deque<int>'}),
 'queue':('std::queue<int>',{'T':'int','Container':'std::deque<int>'}),
 'priority_queue':('std::priority_queue<int,std::vector<int>,std::less<>>',{'T':'int','Container':'std::vector<int>','Compare':'std::less<>'}),
 'bitset':('std::bitset<8>',{'N':'8'}),
 'valarray':('std::valarray<int>',{'T':'int'}),
}
TYPEDEFS={'size_type','value_type','reference','const_reference','iterator','const_iterator','pointer','const_pointer','difference_type','reverse_iterator','const_reverse_iterator','key_type','mapped_type','allocator_type','key_compare','value_compare','node_type','insert_return_type','hasher','key_equal','local_iterator','const_local_iterator','container_type','traits_type','element_type','container_compatible'}
out=['#include <algorithm>\n#include <array>\n#include <bitset>\n#include <deque>\n#include <flat_map>\n#include <flat_set>\n#include <forward_list>\n#include <hive>\n#include <inplace_vector>\n#include <list>\n#include <map>\n#include <memory>\n#include <optional>\n#include <queue>\n#include <set>\n#include <span>\n#include <stack>\n#include <string>\n#include <string_view>\n#include <unordered_map>\n#include <unordered_set>\n#include <valarray>\n#include <vector>\n#include <concepts>\n#include <utility>\n#include <type_traits>\nstruct THash { using is_transparent = void; unsigned long operator()(int) const; };\nstruct TEq { using is_transparent = void; bool operator()(int, int) const; };']
n=0; skipped=0
def conv(txt,cls,cfg,tparams):
    # returns converted type text or None
    if '-' in txt.replace('->',''): return None
    bad=[False]
    def rep(m):
        w=m.group(0)
        if w in KW: return w
        if w in cfg: return cfg[w]
        if w in tparams: return tparams[w]
        if w in TYPEDEFS: return f'typename {cls}::{w}'
        if w=='size_t': return 'std::size_t'
        if w in STD: return 'std::'+w
        bad[0]=True; return w
    r=re.sub(r'\b[A-Za-z_][A-Za-z_0-9]*\b',rep,txt)
    return None if bad[0] else r
def split_params(params):
    ps=[]; d=0; cur=''
    for ch in params:
        if ch in '<([': d+=1
        if ch in '>)]': d-=1
        if ch==',' and d==0: ps.append(cur); cur=''
        else: cur+=ch
    if cur.strip(): ps.append(cur)
    return [p.strip() for p in ps]
for cname,(cls,cfg) in CLASSES.items():
    seen=set()
    for body in bodies.get(cname,[]):
        b=re.sub(r'\{[^{}]*\}',';',body)
        for st in b.split(';'):
            st=' '.join(st.split())
            if '(' not in st or st.startswith(('using','typedef','friend','static_assert')): continue
            tp={}
            mt=re.match(r'template<((?:[^<>]|<[^<>]*>)*)>\s*(.*)',st)
            if mt:
                for p in re.split(r',\s*(?=class|typename|size_t|auto)',mt.group(1)):
                    mm=re.match(r'(?:class|typename)\s*(\.\.\.)?\s*(\w+)',p.strip())
                    if mm:
                        nm=mm.group(2)
                        if 'Iterator' in nm: tp[nm]='const int*'
                        elif nm in ('R','Range'): tp[nm]='std::vector<int>'
                        elif nm=='Alloc': tp[nm]='std::allocator<int>'
                        elif nm in ('Args',): tp[nm]='int'
                        elif nm in ('K',): tp[nm]='int'
                        elif nm in ('T','U') and cname not in ('vector',): pass
                st=mt.group(2)
            if 'requires' in st or 'operator' in st: continue
            m=re.search(r'(?:^|\s)(~?\w+)\s*\(',st)
            if not m: continue
            name=m.group(1)
            pre=st[:m.start(1)]
            j=st.index('(',m.end(1)-1); d=0
            for k in range(j,len(st)):
                if st[k]=='(': d+=1
                elif st[k]==')':
                    d-=1
                    if d==0: break
            params=st[j+1:k]; rest=st[k+1:]
            if name.startswith('~'): continue
            const_q = re.match(r'\s*const\b',rest) is not None
            rq = re.search(r'\bconst\s*&&|^\s*&&',rest) is not None
            is_static='static' in pre
            ps=split_params(params)
            if params.strip()=='void': ps=[]
            ptypes=[]; ok=True
            for p in ps:
                p0=p.split('=')[0].strip()
                if p0.endswith('...'): p0=p0[:-3].strip()
                # drop the parameter name (last identifier) when the text has more than one token and the last is not a type keyword
                mm=re.match(r'(.*?[\w>&*\]])\s+(\w+)$',p0)
                ty=mm.group(1) if mm else p0
                if 'Args&&' in ty: ty='Args&&'
                c=conv(ty,cls,cfg,tp)
                if c is None or re.search(r'\bvoid\b',c): ok=False; break
                ptypes.append((c,'=' in p))
            ret=re.sub(r'\b(constexpr|explicit|static|virtual|inline)\b','',pre).strip()
            rt=None
            if name!=cname and ret:
                rt=conv(ret,cls,cfg,tp)
            if not ok: skipped+=1; continue
            for use_all in (True,False):
                args=[t for t,dflt in ptypes if use_all or not dflt]
                if not use_all and all(not d_ for _,d_ in ptypes): continue
                key=(name,tuple(args),const_q)
                if key in seen: continue
                seen.add(key)
                n+=1
                al=', '.join(f'std::declval<{a}>()' for a in args)
                if name==cname:
                    if not tp and not all(re.match(r'^[^<]*$',a) or True for a in args): pass
                    out.append(f'static_assert(std::is_constructible_v<{cls}{(", " if args else "")}{", ".join(args)}>, "ctor {cname}({", ".join(args)})");')
                else:
                    obj = f'std::declval<{"const " if const_q else ""}{cls}{"" if rq else "&"}>()'
                    callee = f'{cls}::{name}' if is_static else f'{obj}.{name}'
                    out.append(f'static_assert(requires {{ {callee}({al}); }}, "exists {cname}::{name}({", ".join(args)})");')
                    if rt and rt!='void' and 'see below' not in rt and not tp:
                        out.append(f'static_assert(std::same_as<decltype({callee}({al})), {rt}>, "returns {cname}::{name}({", ".join(args)}) -> {rt}");')
                    elif rt=='void' and not tp:
                        out.append(f'static_assert(std::same_as<decltype({callee}({al})), void>, "returns void {cname}::{name}({", ".join(args)})");')
out.append('int main(){}')
open(OUT,'w').write('\n'.join(out)+'\n')
print(n,'probes',skipped,'members skipped',file=sys.stderr)
