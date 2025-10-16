#ifndef ERROR
#define ERROR

#include <stdnoreturn.h>
#include <stdio.h>

noreturn void error(char *fmt, ...) __attribute__((format(printf, 1, 2)));

#define unreachable() \
  error("internal error at %s:%d", __FILE__, __LINE__)

char *format(char *fmt, ...) __attribute__((format(printf, 1, 2)));

#endif