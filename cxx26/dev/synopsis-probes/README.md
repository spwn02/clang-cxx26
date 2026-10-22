# Probes generated from the draft synopses

Each script reads the LaTeX source of a cplusplus/draft checkout (tag n5050 = C++26) and writes a C++ file of
`static_assert`s (existence of the overload with the argument kinds the declaration names, and the declared return
type). They found the policy-overload gaps of #116 (fill_n / for_each_n / generate_n / replace_copy returns, the
missing no-predicate and forward-iterator forms, the missing defaults and constraints).

    python3 gen_policy_overloads.py <draft checkout> probe.cpp          # classic ExecutionPolicy overloads
    python3 gen_ranges_policy_overloads.py <draft checkout> probe.cpp   # ranges:: overloads with Ep&&
    python3 gen_memory_ranges_policy_overloads.py <draft checkout> probe.cpp
    clang++ -std=c++26 -fsyntax-only -nostdinc++ -isystem <libc++ include> -I libcxx/test/support \
        -ferror-limit=0 probe.cpp

The committed tests libcxx/test/std/algorithms/algorithms.general/pstl_policy_overloads.compile.pass.cpp and
pstl_ranges_policy_overloads.compile.pass.cpp are the output of these scripts. A parameter the script cannot map is
reported as `// SKIP`; extend the mapping tables at the top of the script.
