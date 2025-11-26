#pragma once
#include <core/span.h>
#include <core/type.h>
#include <std/strings/str.h>
#include <std/strings/intern.h>

typedef enum TokenKind
{
#define TOK(ID) TokenKind_##ID,
#define KW(ID, STR) TokenKind_##ID,
#define PUNCT(ID, STR) TokenKind_##ID,
#include <token.def>
    TokenKind_COUNT,
} TokenKind;

struct Token
{
    TokenKind kind;
    span_t span;

    union value {
        symbol_t name;
        int as_int;
        float as_float;
        double as_double;
        bool as_bool;
    } value;
};

str_t tokenkind_to_str(TokenKind kind);
const char *tokenkind_to_cstr(TokenKind kind);

