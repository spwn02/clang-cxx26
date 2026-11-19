#!/usr/bin/env bash
# Does the compiler survive a module graph whose dependency was rebuilt differently?
# (#486 hypothesis: the module files used at crash time were older than the sources.)
# usage: stale-graph.sh <clang++> <libcxx-source-dir> <libcxx-build-include-dir>
#   e.g. stale-graph.sh build-nyx/bin/clang++ libcxx build-libcxx/include/c++/v1
# Builds `std`, a project module P (partition P:Concepts + primary P) against it, then a
# second `std` built from different sources, and compiles an importer of P against the
# NEW std with the OLD P files. C++20 named-module imports store no signature of the
# module they import, so the mismatch is not detected up front. Before #486 was hardened
# the AST reader asserted (clang::ASTRecordReader::readNestedNameSpecifierLoc); now it
# reports 'module file is out of date with the module files it imports' and cc1 stops.
# Exit 0: diagnosed; 1: crashed.
set -u
clangxx=$(realpath "$1"); lcx=$(realpath "$2"); lcxinc=$(realpath "$3")
work=$(mktemp -d); trap 'rm -rf "$work"' EXIT; cd "$work"
cc() { "$clangxx" -std=c++26 -stdlib=libc++ -nostdinc++ -isystem "$lcx/include" -idirafter "$lcxinc" -Wno-reserved-module-identifier "$@"; }
mkdir mods && cp -r "$lcx/modules/std" mods/std && sed -n '1,$p' "$lcxinc/../../../modules/c++/v1/std.cppm" > mods/std.cppm 2>/dev/null \
  || { echo "need the generated std.cppm next to the libc++ build tree" >&2; exit 2; }
cat > conc.cppm <<'EOT'
export module P:Concepts;
import std;
export template <class T> concept IsEnum = std::is_enum_v<T>;
export template <class T> concept NotEnum = not IsEnum<T>;
EOT
cat > p.cppm <<'EOT'
export module P;
export import :Concepts;
import std;
export template <class T> struct Fmt { };
template <class T> requires NotEnum<T> struct Fmt<T> { static constexpr int v = 1; };
template <class T> requires IsEnum<T> struct Fmt<T> { static constexpr int v = 2; };
EOT
cat > use.cpp <<'EOT'
import P;
import std;
enum class E {};
static_assert(Fmt<int>::v == 1);
static_assert(Fmt<E>::v == 2);
int main() { return std::is_nothrow_destructible_v<Fmt<int>> ? 0 : 1; }
EOT
cc --precompile mods/std.cppm -o std.pcm || exit 2
cc -fmodule-file=std=std.pcm --precompile conc.cppm -o conc.pcm || exit 2
cc -fmodule-file=std=std.pcm -fmodule-file=P:Concepts=conc.pcm --precompile p.cppm -o p.pcm || exit 2
cc -fmodule-file=std=std.pcm -fmodule-file=P:Concepts=conc.pcm -fmodule-file=P=p.pcm -fsyntax-only use.cpp || { echo "baseline failed" >&2; exit 2; }
echo 'inline int stale_marker;' >> mods/std/algorithm.inc    # a different std
cc --precompile mods/std.cppm -o std2.pcm || exit 2
out=$(cc -fmodule-file=std=std2.pcm -fmodule-file=P:Concepts=conc.pcm -fmodule-file=P=p.pcm -fsyntax-only use.cpp 2>&1); rc=$?
if grep -qE 'Stack dump|PLEASE submit|Assertion' <<<"$out"; then echo "CRASHED (rc=$rc): $(grep -m1 -E 'Assertion|PLEASE' <<<"$out" | cut -c1-160)"; exit 1; fi
echo "diagnosed (rc=$rc): $(grep -m1 error <<<"$out" | cut -c1-160)"; exit 0
