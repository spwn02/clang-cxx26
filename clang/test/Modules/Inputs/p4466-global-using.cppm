export module p4466_global_using;

extern "C" {
typedef unsigned long size_t;
size_t strlen(const char *);
}

export {
  using ::size_t;
  using ::strlen;
}

// P4466R2 §8, [namespace.udecl] *.2.1-.2: a global-scope using-declarator
// is not reflection-transparent when a non-inline named namespace intervenes
// between it and the declaration it names.
export namespace NonInline {
void from_export_block();
}
export { using NonInline::from_export_block; }

// A named namespace associated with an extern "C" linkage-specification is
// still separated from the declaration by NonInline; extern "C" does not
// make that namespace transparent under *.2.2.
export namespace Named {
namespace NonInline {
void from_linkage_block();
}
extern "C" { using NonInline::from_linkage_block; }
}

// Global-scope export and linkage wrappers add no intervening named
// namespace, so these using-declarators satisfy *.2.1-.2.
export void global_from_export_block();
export { using ::global_from_export_block; }
export {
  extern "C" {
    void global_from_linkage_block();
    using ::global_from_linkage_block;
  }
}
