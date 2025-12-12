#ifndef COMMON_H_
#define COMMON_H_

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#define _XSTR(s) _STR(s)
#define _STR(s) #s

#define ASSERT_MSG(cond, fmt, ...)                                             \
  {                                                                            \
    if (!(cond)) {                                                             \
      fprintf(stderr, "%s:%d: %s: Assertion `%s` failed: " fmt "\n", __FILE__, \
              __LINE__, __PRETTY_FUNCTION__,                                   \
              _XSTR(cond) __VA_OPT__(, ) __VA_ARGS__);                         \
      abort();                                                                 \
    }                                                                          \
  }

#define UNREACHABLE_MSG(fmt, ...)                                      \
  {                                                                    \
    fprintf(stderr, "%s:%d: %s: Hit unreachable statement: " fmt "\n", \
            __FILE__, __LINE__,                                        \
            __PRETTY_FUNCTION__ __VA_OPT__(, ) __VA_ARGS__);           \
    abort();                                                           \
  }

#if defined(DOTRACE)
#define TRACE(fmt, ...)                                             \
  fprintf(stderr, "TRACE %s:%d: %s: " fmt "\n", __FILE__, __LINE__, \
          __PRETTY_FUNCTION__ __VA_OPT__(, ) __VA_ARGS__)
#else
#define TRACE(fmt, ...) while (0)  // Here to absorb a trailing `;`
#endif

static bool is_power_of_2(size_t x) { return (x & (x - 1)) == 0 && x != 0; }

static size_t align_up(size_t x, size_t alignment) {
  assert(is_power_of_2(alignment) && "Invalid alignment");
  return (x + (alignment - 1)) & ~(alignment - 1);
}

typedef bool (*ConstUnaryOp)(const void* it, void* arg);
static inline void* find_if(const void* start, const void* end,
                            size_t sizeof_elem, ConstUnaryOp op, void* arg) {
  const char* start_ = start;
  const char* end_ = end;
  for (; start_ != end_; start_ += sizeof_elem) {
    if (op(start_, arg))
      return (void*)start_;
  }
  return (void*)end_;
}

#endif  // COMMON_H_
