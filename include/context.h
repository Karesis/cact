#pragma once

#include <core/mem/allocer.h>
#include <std/map.h>
#include <std/strings/intern.h>
#include <std/fs/srcmanager.h>
#include <token.h>

defMap(symbol_t, TokenKind, KeywordMap);

struct Context {
	allocer_t alc;

	srcmanager_t mgr;

	interner_t itn;

	KeywordMap kw_map;

	bool had_error;
	bool panic_mode;
};

void context_init(struct Context *ctx, allocer_t alc);
void context_deinit(struct Context *ctx);

void ctx_error(struct Context *ctx, const struct Token *tok, const char *fmt,
	       ...);
