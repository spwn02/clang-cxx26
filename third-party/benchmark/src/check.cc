#include "check.h"

namespace benchmark {
namespace internal {

// std::abort has C language linkage, so it is not of type AbortHandlerT.
static void DefaultAbort() { std::abort(); }

static AbortHandlerT* handler = &DefaultAbort;

BENCHMARK_EXPORT AbortHandlerT*& GetAbortHandler() { return handler; }

}  // namespace internal
}  // namespace benchmark
