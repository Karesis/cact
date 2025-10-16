#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "error.h"
#include "token.h"

// --- 模块级静态变量 ---
// 用于存储源文件内容和文件名，以便 error_tok 函数可以访问
static char *current_input;
static char *current_filename;

// 报告一个错误并退出。
void error(char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  va_end(ap);
  exit(1);
}

// Takes a printf-style format string and returns a formatted string.
char *format(char *fmt, ...) {
  char *buf;
  size_t buflen;
  FILE *out = open_memstream(&buf, &buflen);

  va_list ap;
  va_start(ap, fmt);
  vfprintf(out, fmt, ap);
  va_end(ap);
  fclose(out);
  return buf;
}

void error_tok(Token *tok, char *fmt, ...) {
    // 打印文件名和行列号
    fprintf(stderr, "%s:%d:%d: ", current_filename, tok->row, tok->col);

    // 打印格式化的错误信息
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    va_end(ap);

    // --- 打印出错的代码行和指示符 ---

    // 查找当前行的起始位置
    char *line_start = tok->loc;
    while (current_input < line_start && line_start[-1] != '\n') {
        line_start--;
    }

    // 查找当前行的结束位置
    char *line_end = tok->loc;
    while (*line_end != '\n' && *line_end != '\0') {
        line_end++;
    }

    // 打印代码行
    fprintf(stderr, "%.*s\n", (int)(line_end - line_start), line_start);

    // 打印指示符 (^)
    int indent = tok->loc - line_start;
    fprintf(stderr, "%*s", indent, ""); // 打印前导空格
    fprintf(stderr, "^\n");

    exit(1);
}