#!/usr/bin/env python3
# Audit of the specifiers of the member functions of the class synopses of N5050 against libc++:
# every member the draft declares constexpr / unconditionally noexcept must be so in the library
# (every library overload of that name that accepts that number of arguments).
# usage: genflags.py <draft> <std include dir> <out report>
import re, sys, json, subprocess, os, tempfile
D, INC, OUT = sys.argv[1:4]
FILES = ['containers','strings','utilities','iterators','ranges','algorithms','numerics','time','text','memory','mdspan'] if False else None
src = {}
for f in ('containers','strings','utilities','ranges','iterators','time','text','numerics','algorithms','memory','threads','iostreams','locales','regex','diagnostics'):
    p=f'{D}/source/{f}.tex'
    if os.path.exists(p): src[f]=open(p).read()
def clean(s):
    s=re.sub(r'//[^\n]*','',s)
    s=re.sub(r'@\\[a-zA-Z]+\{((?:[^{}]|\{[^{}]*\})*)\}@',r'\1',s)
    s=re.sub(r'@\\[a-zA-Z]+@','',s)
    s=re.sub(r'@\\[a-zA-Z]+\{([^}]*)\}\{([^}]*)\}@',r'\1',s)
    return s
classes={}
for f,t in src.items():
    t=clean(t)
    for m in re.finditer(r'(?:template<(?:[^<>]|<[^<>]*>)*>\s*)?(?:class|struct)\s+(\w+)\s*(?:final\s*)?(?::[^{;]*)?\{',t):
        name=m.group(1); i=m.end(); depth=1
        while depth and i<len(t):
            c=t[i]
            if c=='{': depth+=1
            elif c=='}': depth-=1
            i+=1
        body=t[m.end():i-1]
        classes.setdefault(name,[]).append(body)
members={}
for name,bodies in classes.items():
    for body in bodies:
        # flatten nested braces (inline bodies) to ';'
        b=re.sub(r'\{[^{}]*\}',';',body)
        for st in b.split(';'):
            st=' '.join(st.split())
            if '(' not in st or st.startswith(('using','typedef','friend','template<')) and False: continue
            m=re.search(r'(?:^|\s)((?:~)?\w+|operator\s*[^\s(]+(?:\s*\(\s*\))?)\s*\(',st)
            if not m: continue
            nm=m.group(1)
            pre=st[:m.start(1)]
            cx='constexpr' in pre
            ex='explicit' in pre
            # params
            depth=0; j=st.index('(',m.end(1)-0) ; k=j; 
            depth=0
            for k in range(j,len(st)):
                if st[k]=='(': depth+=1
                elif st[k]==')':
                    depth-=1
                    if depth==0: break
            params=st[j+1:k]; rest=st[k+1:]
            ps=[]; d=0; cur=''
            for ch in params:
                if ch in '<([': d+=1
                if ch in '>)]': d-=1
                if ch==',' and d==0: ps.append(cur); cur=''
                else: cur+=ch
            if cur.strip(): ps.append(cur)
            if params.strip()=='void': ps=[]
            hi=len(ps); lo=sum(1 for p in ps if '=' not in p)
            nx = re.search(r'\bnoexcept\b(?!\s*\()',rest) is not None
            members.setdefault(name,[]).append((nm,lo,hi,cx,nx,ex,st))
print(len(classes),'classes',sum(len(v) for v in members.values()),'member decls',file=sys.stderr)
json.dump({k:[list(x) for x in v] for k,v in members.items()},open(OUT+'.draft.json','w'))

# ---- library side
import concurrent.futures
HEADERS=['algorithm','any','array','atomic','barrier','bit','bitset','chrono','complex','condition_variable','deque','exception','execution','expected','filesystem','flat_map','flat_set','format','forward_list','fstream','functional','future','generator','hive','inplace_vector','iomanip','ios','iostream','iterator','latch','linalg','list','locale','map','mdspan','memory','memory_resource','meta','mutex','new','numeric','optional','print','queue','random','ranges','ratio','regex','scoped_allocator','semaphore','set','shared_mutex','simd','span','sstream','stack','stacktrace','stdexcept','stop_token','streambuf','string','string_view','syncstream','system_error','text_encoding','thread','tuple','type_traits','typeindex','unordered_map','unordered_set','utility','valarray','variant','vector','version','istream','ostream','charconv','cmath','compare','coroutine','functional','memory']
def lib_members(cls):
    inc=''.join(f'#include <{h}>\n' for h in HEADERS)
    with tempfile.NamedTemporaryFile('w',suffix='.cpp',delete=False,dir='/tmp/claude-1000/sweep/tmp') as f:
        f.write(inc); path=f.name
    r=subprocess.run(['/home/spawn/dev/toolchains/clang-p2996/build-nyx/bin/clang++','-std=c++26','-fsyntax-only','-nostdinc++','-isystem',INC,'-Xclang','-ast-dump=json','-Xclang','-ast-dump-filter='+cls,path],capture_output=True,text=True)
    os.unlink(path)
    dec=json.JSONDecoder(); s=r.stdout; i=0; out=[]
    while i<len(s):
        while i<len(s) and s[i].isspace(): i+=1
        if i>=len(s): break
        try: o,j=dec.raw_decode(s,i)
        except Exception: break
        i=j
        if o.get('name')!=cls: continue
        if o.get('kind') not in ('ClassTemplateDecl','CXXRecordDecl'): continue
        recs=[o] if o['kind']=='CXXRecordDecl' else [x for x in o.get('inner',[]) if x.get('kind')=='CXXRecordDecl']
        for rec in recs:
            for m in rec.get('inner',[]):
                kk=m.get('kind')
                if kk not in ('CXXMethodDecl','CXXConstructorDecl','CXXDestructorDecl','CXXConversionDecl','FunctionTemplateDecl'): continue
                mm=m
                if kk=='FunctionTemplateDecl':
                    c=[x for x in m['inner'] if x.get('kind','').startswith('CXX')]
                    if not c: continue
                    mm=c[0]
                nm=re.sub(r'<.*','',mm.get('name',''))
                ps=[p for p in mm.get('inner',[]) if p.get('kind')=='ParmVarDecl']
                hi=len(ps); lo=sum(1 for p in ps if 'init' not in p)
                q=mm.get('type',{}).get('qualType','')
                out.append((nm,lo,hi,bool(mm.get('constexpr')),'noexcept' in q))
    return cls,out
os.makedirs('/tmp/claude-1000/sweep/tmp',exist_ok=True)
names=[n for n in members if not n.startswith('_')]
res={}
with concurrent.futures.ThreadPoolExecutor(max_workers=int(os.environ.get('J','10'))) as ex:
    for cls,out in ex.map(lib_members,names):
        res[cls]=out
rep=[]
for cls in names:
    lib=res.get(cls,[])
    if not lib: rep.append(f'NOLIB {cls}'); continue
    for (nm,lo,hi,cx,nx,ex_,st) in members[cls]:
        nm2=nm.replace('~','~')
        cand=[l for l in lib if (l[0]==nm2 or (nm2.startswith('~') and l[0]=='~'+cls)) and l[1]<=hi and l[2]>=lo]
        if not cand: rep.append(f'MISSING {cls}::{nm} arity {lo}-{hi}: {st[:110]}'); continue
        if cx and not all(c[3] for c in cand): rep.append(f'NOT-CONSTEXPR {cls}::{nm} arity {lo}-{hi}: {st[:110]}')
        if nx and not all(c[4] for c in cand): rep.append(f'NOT-NOEXCEPT {cls}::{nm} arity {lo}-{hi}: {st[:110]}')
open(OUT,'w').write('\n'.join(rep)+'\n')
print(len(rep),'findings')
