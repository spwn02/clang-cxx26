export module p4466_global_using;

extern "C" {
typedef unsigned long size_t;
size_t strlen(const char *);
}

export {
  using ::size_t;
  using ::strlen;
}
