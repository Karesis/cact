#ifndef ERROR
#define ERROR

#include <stdnoreturn.h>
#include <stdio.h>
#include "token.h"

// 初始化错误报告系统
// input: 指向源文件完整内容的指针
// filename: 源文件的名字
void init_error_reporting(char *input, char *filename);

noreturn void error(char *fmt, ...) __attribute__((format(printf, 1, 2)));
noreturn void error_tok(Token *tok, char *fmt, ...) __attribute__((format(printf, 2, 3)));

#define unreachable() \
  error("internal error at %s:%d", __FILE__, __LINE__)

char *format(char *fmt, ...) __attribute__((format(printf, 1, 2)));

#endif