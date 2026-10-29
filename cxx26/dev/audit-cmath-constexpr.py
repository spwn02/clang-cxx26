#!/usr/bin/env python3
"""Checks that every floating-point-type function of <cmath> is constexpr for every floating-point type, in the
language mode where [cmath.syn] makes it so: the exact functions (P0533R9) from C++23, the transcendental ones
(P1383R2) and nextup/nextdown from C++26. Prints every (mode, function, type) that cannot be evaluated in a
constant expression and exits 1 if there is one. The rounding-mode dependent functions (nearbyint, rint, lrint,
llrint) are not constexpr by design.

usage: audit-cmath-constexpr.py --clang CLANG --headers DIR --generated DIR
  --headers:   the libc++ headers to test (libcxx/include of the tree, or an installed include/c++/v1)
  --generated: a directory that holds the generated __config_site and __assertion_handler (for example
               build-libcxx/include/c++/v1); only those two files are used, so that the headers it holds do not
               shadow the ones under test
"""
import argparse, os, shutil, subprocess, sys, tempfile

TYPES = {  # type -> (literal, feature macro or None)
    "float": ("0.5f", None), "double": ("0.5", None), "long double": ("0.5L", None),
    "std::float16_t": ("std::float16_t(0.5)", "__STDCPP_FLOAT16_T__"),
    "std::float32_t": ("std::float32_t(0.5)", "__STDCPP_FLOAT32_T__"),
    "std::float64_t": ("std::float64_t(0.5)", "__STDCPP_FLOAT64_T__"),
    "std::bfloat16_t": ("std::bfloat16_t(0.5)", "__STDCPP_BFLOAT16_T__"),
    "std::float128_t": ("std::float128_t(0.5)", ("__STDCPP_FLOAT128_T__", "__GLIBC__")),
}
UN23 = "ceil floor round trunc logb fabs".split()
UN26 = "acos asin asinh atan atanh cbrt cos cosh erf erfc exp exp2 expm1 lgamma log log10 log1p log2 sin sinh tan tanh tgamma sqrt".split()
BI23 = "copysign fdim fmax fmin fmod nextafter remainder".split()
BI26 = "atan2 hypot pow".split()

def rows():
    r = []
    for f in UN23: r.append((f, f"std::{f}(X)", 23))
    for f in UN26: r.append((f, f"std::{f}(X)", 26))
    r.append(("acosh", "std::acosh(X + X + X)", 26))
    for f in BI23: r.append((f, f"std::{f}(X, X)", 23))
    for f in BI26: r.append((f, f"std::{f}(X, X)", 26))
    r += [("fma", "std::fma(X, X, X)", 23), ("ilogb", "std::ilogb(X)", 23), ("lround", "std::lround(X)", 23),
          ("llround", "std::llround(X)", 23), ("ldexp", "std::ldexp(X, 2)", 23), ("scalbn", "std::scalbn(X, 2)", 23),
          ("scalbln", "std::scalbln(X, 2L)", 23), ("nexttoward", "std::nexttoward(X, 2.0L)", 23),
          ("frexp", "[]{ int e; return std::frexp(X, &e); }()", 23),
          ("modf", "[]{ decltype(X) i; return std::modf(X, &i); }()", 23),
          ("remquo", "[]{ int q; return std::remquo(X, X, &q); }()", 23), ("abs", "std::abs(X)", 23),
          ("nextup", "std::nextup(X)", 26), ("nextdown", "std::nextdown(X)", 26)]
    return r

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--clang", required=True)
    ap.add_argument("--headers", required=True)
    ap.add_argument("--generated", required=True)
    args = ap.parse_args()
    bad = {}
    with tempfile.TemporaryDirectory() as tmp:
        gen = os.path.join(tmp, "generated")
        os.mkdir(gen)
        for f in ("__config_site", "__assertion_handler"):
            shutil.copy(os.path.join(args.generated, f), gen)
        inc = ["-isystem", args.headers, "-isystem", gen]
        src = os.path.join(tmp, "t.cpp")
        for std in (23, 26):
            for t, (lit, feat) in TYPES.items():
                lines = ["#include <cmath>", "#include <stdfloat>"]
                if feat:
                    feats = feat if isinstance(feat, tuple) else (feat,)
                    lines.append("#if " + " && ".join(f"defined({f})" for f in feats))
                names = []
                for i, (n, e, since) in enumerate(rows()):
                    if since > std: continue
                    lines.append(f"constexpr auto v{i} = {e.replace('X', '(' + lit + ')')}; // {n}")
                    names.append(n)
                if feat: lines.append("#endif")
                open(src, "w").write("\n".join(lines) + "\n")
                r = subprocess.run([args.clang, f"-std=c++{std}", "-fsyntax-only", "-ferror-limit=0", "-nostdinc++"] + inc + [src],
                                   capture_output=True, text=True)
                text = open(src).read().split("\n")
                if r.returncode != 0 and "constexpr variable" not in r.stderr:
                    print("the probe for", t, f"c++{std}", "failed for another reason:", r.stderr[:400], file=sys.stderr); sys.exit(2)
                for line in r.stderr.splitlines():
                    if "error:" in line and "constexpr variable" in line:
                        ln = int(line.split(":")[1])
                        bad.setdefault((f"c++{std}", text[ln - 1].split("//")[-1].strip()), []).append(t)
                    elif "error:" in line and ("fatal" in line or "file not found" in line):
                        print("cannot compile the probe:", line, file=sys.stderr); sys.exit(2)
    for (s, n), ts in sorted(bad.items()): print(s, n, ":", ", ".join(ts))
    print(f"audit-cmath-constexpr: {len(bad)} function/mode pairs are not constexpr")
    sys.exit(1 if bad else 0)

main()
