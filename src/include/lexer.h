#ifndef LEXER_H
#define LEXER_H

#include "token.h"
#include "str.h"

// 词法分析器的主要入口函数。
// 输入: 包含源代码的 str 对象和文件名（用于错误报告）。
// 输出: 指向 Token 链表头部的指针。
Token* tokenize(str source_code, const char* filename);

// 释放由 tokenize 创建的整个 Token 链表
void free_tokens(Token* token);

const char* token_kind_to_str(TokenKind kind);

#endif // LEXER_H