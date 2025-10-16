#ifndef TOKEN_H
#define TOKEN_H

#include <stdbool.h>

typedef enum 
{
    TK_EOF,       // End-of-file marker

    // ident
    TK_IDENT,     // Identifier (e.g., main, my_var)

    // Literals
    TK_NUM_INT,   // Integer literal (e.g., 123, 0x1A, 077)
    TK_NUM_FLOAT, // Float literal (e.g., 3.14f)
    TK_NUM_DOUBLE,// Double literal (e.g., 1.23, 4.56E2)
    TK_LIT_BOOL,      // Boolean literal (true, false)

    // Keywords
    TK_CONST,     // "const"
    TK_INT,       // "int"
    TK_BOOL,      // "bool"
    TK_FLOAT,     // "float"
    TK_DOUBLE,    // "double"
    TK_VOID,      // "void"
    TK_IF,        // "if"
    TK_ELSE,      // "else"
    TK_WHILE,     // "while"
    TK_BREAK,     // "break"
    TK_CONTINUE,  // "continue"
    TK_RETURN,    // "return"

    // Punctuators
    TK_PLUS,      // +
    TK_MINUS,     // -
    TK_STAR,      // *
    TK_SLASH,     // /
    TK_PERCENT,   // %
    TK_EQ,        // ==
    TK_NEQ,       // !=
    TK_LT,        // <
    TK_LE,        // <=
    TK_GT,        // >
    TK_GE,        // >=
    TK_L_PAREN,   // (
    TK_R_PAREN,   // )
    TK_L_BRACE,   // {
    TK_R_BRACE,   // }
    TK_L_BRACKET, // [
    TK_R_BRACKET, // ]
    TK_ASSIGN,    // =
    TK_SEMICOLON, // ;
    TK_COMMA,     // ,
    TK_LOG_AND,   // &&
    TK_LOG_OR,    // ||
    TK_LOG_NOT,   // !

} 
TokenKind;

// 用于存储字面量的值。使用 union 可以节省内存，
// 因为一个 token 不可能同时是整数和浮点数。
typedef union {
    int i_val;       // for TK_NUM_INT
    float f_val;     // for TK_NUM_FLOAT
    double d_val;    // for TK_NUM_DOUBLE
    bool b_val;      // for TK_BOOL
} 
LiteralValue;

// Token 结构体
typedef struct Token Token;
struct Token 
{
    TokenKind kind;     // Token 的类型

    Token *next;        // 指向下一个 token，形成一个链表

    char *loc;          // Token 在源代码中的起始位置
    int len;            // Token 的长度

    LiteralValue val;   // Token 的字面量值 (如果适用)

    int row;            // Token 所在的行号 (用于错误报告)
    int col;            // 列号
};

#endif