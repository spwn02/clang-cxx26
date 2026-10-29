#!/usr/bin/env python3
"""Checks that every function that the draft declares constexpr in the synopses of <cmath>, <cstdlib> and
<cinttypes> can be evaluated in a constant expression, for every floating-point type that the declaration covers
(floating-point-type), in the language mode that requires it. The set of C++23 is the one of the tag n4950 of the
draft, the set of C++26 the one of n5050, so that no list of functions is written by hand here. Prints every
(mode, function, type) that cannot be evaluated and exits 1 if there is one. The rounding-mode dependent functions
are not constexpr in the draft, so they are not looked at.

usage: audit-cmath-constexpr.py --clang CLANG --headers DIR --generated DIR --draft DIR [--tag23 n4950] [--tag26 n5050]
  --headers:   the libc++ headers to test (libcxx/include of the tree, or an installed include/c++/v1)
  --generated: a directory that holds the generated __config_site and __assertion_handler (for example
               build-libcxx/include/c++/v1); only those two files are used, so that the headers it holds do not
               shadow the ones under test
  --draft:     a checkout of github.com/cplusplus/draft that has the two tags
"""
import argparse, os, re, shutil, subprocess, sys, tempfile

TYPES = {  # type -> (literal, feature macros that must be defined)
    "float": ("0.5f", ()), "double": ("0.5", ()), "long double": ("0.5L", ()),
    "std::float16_t": ("std::float16_t(0.5)", ("__STDCPP_FLOAT16_T__",)),
    "std::float32_t": ("std::float32_t(0.5)", ("__STDCPP_FLOAT32_T__",)),
    "std::float64_t": ("std::float64_t(0.5)", ("__STDCPP_FLOAT64_T__",)),
    "std::bfloat16_t": ("std::bfloat16_t(0.5)", ("__STDCPP_BFLOAT16_T__",)),
    "std::float128_t": ("std::float128_t(0.5)", ("__STDCPP_FLOAT128_T__", "__GLIBC__")),
}
# arguments for which 0.5 is outside of the domain of the function
DOMAIN = {"acosh": 2.0, "acoshf": 2.0, "acoshl": 2.0}
SECTIONS = {"cmath.syn": "cmath", "cstdlib.syn": "cstdlib", "cinttypes.syn": "cinttypes"}


def synopsis(draft, tag, label):
    files = subprocess.run(["git", "-C", draft, "ls-tree", "--name-only", tag, "source/"], capture_output=True, text=True).stdout.split()
    for f in files:
        if not f.endswith(".tex"):
            continue
        r = subprocess.run(["git", "-C", draft, "show", f"{tag}:{f}"], capture_output=True, text=True)
        i = r.stdout.find(f"\\rSec2[{label}]")
        if i >= 0:
            return r.stdout[i:r.stdout.find("\\rSec", i + 10)]
    return ""


def declarations(text):
    """(returns, name, [param types]) of the constexpr declarations in a synopsis."""
    text = re.sub(r"@\\placeholder(?:nc)?\{floating-point-type\}@", "FPT", text)
    text = re.sub(r"@[^@]*@", "", text)
    text = re.sub(r"//[^\n]*", "", text)
    result = []
    for stmt in text.split(";"):
        stmt = " ".join(stmt.split())
        m = re.search(r"\bconstexpr ([\w:<>\s*]+?) (\w+)\((.*)\)(?: noexcept)?$", stmt)
        if not m:
            continue
        ret, name, params = m.group(1), m.group(2), m.group(3)
        types = []
        for p in [x.strip() for x in params.split(",") if x.strip()]:
            p = re.sub(r"\bconst\b", "", p)
            words = p.replace("*", " * ").split()
            # drop the parameter name (the last word that is not a type word or a pointer)
            tw = {"int", "long", "unsigned", "float", "double", "short", "char", "FPT", "*", "intmax_t", "size_t"}
            if len(words) > 1 and words[-1] not in tw:
                words = words[:-1]
            elif len(words) == 1 and words[0] not in tw:
                words = words[:-1]
            types.append(" ".join(words).replace(" *", "*"))
        result.append((ret.strip(), name, tuple(types)))
    return result


def plain_declarations(text):
    """(returns, name, [param types]) of the function declarations of a synopsis that are not constexpr."""
    text = re.sub(r"@\\placeholder(?:nc)?\{floating-point-type\}@", "FPT", text)
    text = re.sub(r"@[^@]*@", "", text)
    text = re.sub(r"//[^\n]*", "", text)
    text = "\n".join(l for l in text.split("\n") if not l.lstrip().startswith("#"))
    result = []
    for stmt in text.split(";"):
        stmt = " ".join(stmt.split())
        if not stmt or "constexpr" in stmt or stmt.startswith(("namespace", "using", "template", "\\")):
            continue
        m = re.search(r"([\w:<>\s*]+?) (\w+)\((.*)\)(?: noexcept)?$", stmt)
        if not m:
            continue
        ret, name, params = m.group(1).strip(), m.group(2), m.group(3)
        ret = ret.split()[-1] if ret.split()[-1] in ("float", "double", "FPT", "int", "bool", "long") else ret
        types = []
        for p in [x.strip() for x in params.split(",") if x.strip()]:
            p = re.sub(r"\bconst\b", "", p)
            words = p.replace("*", " * ").split()
            tw = {"int", "long", "unsigned", "float", "double", "short", "char", "FPT", "*", "intmax_t", "size_t"}
            if len(words) > 1 and words[-1] not in tw:
                words = words[:-1]
            elif len(words) == 1 and words[0] not in tw:
                words = words[:-1]
            types.append(" ".join(words).replace(" *", "*"))
        result.append((ret, name, tuple(types)))
    return result


def argument(t, name, ptype):
    """An expression for a parameter of type t, T is the type that stands for floating-point-type."""
    if t == "FPT":
        return f"static_cast<{ptype}>({DOMAIN.get(name, 0.5)})"
    if t == "float": return f"{DOMAIN.get(name, 0.5)}f"
    if t == "double": return f"{DOMAIN.get(name, 0.5)}"
    if t == "long double": return f"{DOMAIN.get(name, 0.5)}L"
    if t == "int*": return "&_i"
    if t in ("FPT*",): return "&_f"
    if t == "char*" or t == "char *": return '""'
    if t == "float*": return "&_pf"
    if t == "double*": return "&_pd"
    if t == "long double*": return "&_pl"
    if t in ("int",): return "2"
    if t in ("unsigned", "unsigned int"): return "2u"
    if t in ("long", "long int"): return "2L"
    if t in ("long long", "long long int"): return "2LL"
    if t in ("intmax_t",): return "2"
    raise ValueError(f"no argument for the parameter type '{t}' of {name}")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--clang", required=True)
    ap.add_argument("--headers", required=True)
    ap.add_argument("--generated", required=True)
    ap.add_argument("--draft", required=True)
    ap.add_argument("--tag23", default="n4950")
    ap.add_argument("--tag26", default="n5050")
    args = ap.parse_args()

    sets = {}
    for std, tag in ((23, args.tag23), (26, args.tag26)):
        d = []
        for label in SECTIONS:
            text = synopsis(args.draft, tag, label)
            if not text:
                print(f"no synopsis {label} in {tag}", file=sys.stderr); sys.exit(2)
            d += declarations(text)
        sets[std] = {(n, p): r for r, n, p in d}
    required = {23: set(sets[23]), 26: set(sets[26])}
    total = len(required[26])
    if total < 100:
        print(f"only {total} constexpr declarations found in the synopses: the parser is wrong", file=sys.stderr); sys.exit(2)

    bad = {}
    tested = 0
    with tempfile.TemporaryDirectory() as tmp:
        gen = os.path.join(tmp, "generated")
        os.mkdir(gen)
        for f in ("__config_site", "__assertion_handler", "__log_hardening_failure"):
            if os.path.exists(os.path.join(args.generated, f)):
                shutil.copy(os.path.join(args.generated, f), gen)
        inc = ["-isystem", args.headers, "-isystem", gen]
        src = os.path.join(tmp, "t.cpp")
        for std in (23, 26):
            keys = sorted(required[std])
            for typename, (lit, feats) in TYPES.items():
                lines = ["#include <cmath>", "#include <cstdlib>", "#include <cinttypes>", "#include <stdfloat>"]
                if feats:
                    lines.append("#if " + " && ".join(f"defined({f})" for f in feats))
                names = []
                for i, (name, params) in enumerate(keys):
                    generic = sets[std][(name, params)] == "FPT" or any("FPT" in p for p in params)
                    if not generic and typename != "float":
                        continue  # a declaration for fixed types is tested once
                    try:
                        a = ", ".join(argument(p, name, typename) for p in params)
                    except ValueError as e:
                        print("cannot build a call:", e, file=sys.stderr); sys.exit(2)
                    lines.append(f"constexpr auto v{i} = []{{ int _i = 0; {typename} _f = 0; float _pf = 0; double _pd = 0; long double _pl = 0; (void)_i; (void)_f; (void)_pf; (void)_pd; (void)_pl; "
                                 f"return std::{name}({a}); }}(); // {name}{'(' + ', '.join(params) + ')'}")
                    names.append(name)
                    tested += 1
                if feats:
                    lines.append("#endif")
                open(src, "w").write("\n".join(lines) + "\n")
                r = subprocess.run([args.clang, f"-std=c++{std}", "-fsyntax-only", "-ferror-limit=0", "-nostdinc++"] + inc + [src],
                                   capture_output=True, text=True)
                text = open(src).read().split("\n")
                other = [l for l in r.stderr.splitlines() if "error:" in l and "constexpr variable" not in l
                         and "cannot be used in a constant" not in l and "must be initialized" not in l]
                for line in r.stderr.splitlines():
                    if "error:" in line and ("must be initialized by a constant expression" in line):
                        ln = int(line.split(":")[1])
                        bad.setdefault((f"c++{std}", text[ln - 1].split("//")[-1].strip()), []).append(typename)
                if other and not bad:
                    pass
                for l in other:
                    if "fatal error" in l or "file not found" in l or "no matching function" in l or "undeclared" in l \
                            or "no member named" in l:
                        # a function that does not exist for this type is a gap as well, but say so
                        ln = int(l.split(":")[1]) if l.split(":")[1].isdigit() else 0
                        label = text[ln - 1].split("//")[-1].strip() if 0 < ln <= len(text) else l[:80]
                        bad.setdefault((f"c++{std}", label + "  [does not exist: " + l.split("error:")[1][:60].strip() + "]"), []).append(typename)
    # The other direction: [constexpr.functions] forbids a library function to be constexpr unless the draft says so, so
    # every function that the synopsis of <cmath> of n5050 declares without constexpr must fail in a constant expression.
    plain = {}
    for ret, name, params in plain_declarations(synopsis(args.draft, args.tag26, "cmath.syn")):
        plain[(name, params)] = ret
    reverse_bad = {}
    reverse_calls = 0
    with tempfile.TemporaryDirectory() as tmp2:
        gen2 = os.path.join(tmp2, "generated")
        os.mkdir(gen2)
        for f in ("__config_site", "__assertion_handler", "__log_hardening_failure"):
            if os.path.exists(os.path.join(args.generated, f)):
                shutil.copy(os.path.join(args.generated, f), gen2)
        inc2 = ["-isystem", args.headers, "-isystem", gen2]
        src2 = os.path.join(tmp2, "t.cpp")
        for typename, (lit, feats) in TYPES.items():
            lines = ["#include <cmath>", "#include <stdfloat>"]
            if feats:
                lines.append("#if " + " && ".join(f"defined({f})" for f in feats))
            labels = {}
            for i, ((name, params), ret) in enumerate(sorted(plain.items())):
                generic = ret == "FPT" or any("FPT" in p for p in params)
                if not generic and typename != "float":
                    continue
                try:
                    a2 = ", ".join(argument(p, name, typename) for p in params)
                except ValueError as e:
                    print("cannot build a call:", e, file=sys.stderr); sys.exit(2)
                label = f"{name}({', '.join(params)})"
                lines.append(f"constexpr auto v{i} = []{{ int _i = 0; {typename} _f = 0; float _pf = 0; double _pd = 0; "
                             f"long double _pl = 0; (void)_i; (void)_f; (void)_pf; (void)_pd; (void)_pl; "
                             f"return std::{name}({a2}); }}(); // {label}")
                labels[len(lines)] = label
                reverse_calls += 1
            if feats:
                lines.append("#endif")
            open(src2, "w").write("\n".join(lines) + "\n")
            r = subprocess.run([args.clang, "-std=c++26", "-fsyntax-only", "-ferror-limit=0", "-nostdinc++"] + inc2 + [src2],
                               capture_output=True, text=True)
            failing = set()
            for line in r.stderr.splitlines():
                if "error:" not in line:
                    continue
                ln = int(line.split(":")[1]) if line.split(":")[1].isdigit() else 0
                if "must be initialized by a constant expression" in line:
                    failing.add(ln)
                elif ln in labels:
                    print("the reverse probe", labels[ln], "for", typename, "does not compile:", line[:200], file=sys.stderr)
                    sys.exit(2)
            for ln, label in labels.items():
                if ln not in failing:
                    reverse_bad.setdefault(label, []).append(typename)
    for label, ts in sorted(reverse_bad.items()):
        print("c++26", label, ": is constexpr, the draft does not make it so, for", ", ".join(sorted(set(ts))))
    print(f"audit-cmath-constexpr (reverse): {reverse_calls} calls from {len(plain)} declarations that are not constexpr, "
          f"{len(reverse_bad)} of them can be evaluated in a constant expression")
    bad.update({("c++26", "[reverse] " + k): v for k, v in reverse_bad.items()})

    for (s, n), ts in sorted(bad.items()):
        print(s, n, ":", ", ".join(sorted(set(ts))))
    print(f"audit-cmath-constexpr: {tested} calls from {total} constexpr declarations of the draft, "
          f"{len(bad)} function/mode pairs cannot be evaluated in a constant expression")
    sys.exit(1 if bad else 0)


main()
