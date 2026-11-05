namespace std::meta { struct info_t; using info = decltype(^^int); }
struct Rs { int m; };
constexpr std::meta::info rt = ^^Rs;
constexpr std::meta::info rd = ^^Rs::m;
constexpr std::meta::info rn = ^^std::meta;
constexpr std::meta::info rb = ^^int;
