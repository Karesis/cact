#pragma once 
#include <std/map.h>
#include <std/strings/intern.h>
#include <token.h>

defMap(symbol_t, TokenKind, KeywordMap);

struct Context 
{
    /// god allocer
    allocer_t alc;
    /// god internrer
    interner_t itn;
    KeywordMap kw_map;
};

void context_init(struct Context *ctx, allocer_t alc);
void context_deinit(struct Context *ctx);
