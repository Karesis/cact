/*
 *    Copyright 2025 Karesis
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#include <context.h>
#include <type.h>
#include <core/msg.h>
#include <core/hash.h>
#include <stdarg.h>
#include <stdio.h>

/*
 * ==========================================================================
 * 1. Map Operations for symbol_t
 * ==========================================================================
 */

static u64 _sym_hash(const void *key)
{
	const symbol_t *s = (const symbol_t *)key;
	return (u64)s->id * 2654435761u;
}

static bool _sym_eq(const void *lhs, const void *rhs)
{
	return ((const symbol_t *)lhs)->id == ((const symbol_t *)rhs)->id;
}

static const map_ops_t MAP_OPS_SYMBOL = {
	.hash = _sym_hash,
	.equals = _sym_eq,
};

/*
 * ==========================================================================
 * 2. Lifecycle
 * ==========================================================================
 */

void context_init(struct Context *ctx, allocer_t alc)
{
	ctx->alc = alc;
	ctx->had_error = false;
	ctx->panic_mode = false;

	types_init(alc);

	if (!srcmanager_init(&ctx->mgr, alc)) {
		log_panic("Failed to init SourceManager");
	}

	if (!intern_init(&ctx->itn, alc)) {
		log_panic("Failed to initialize Interner");
	}

	if (!map_init(ctx->kw_map, alc, MAP_OPS_SYMBOL)) {
		log_panic("Failed to initialize KeywordMap");
	}

	symbol_t sym;
	TokenKind kind;

#define TOK(ID)
#define PUNCT(ID, STR)

#define KW(ID, STR)                                                              \
	do {                                                                     \
		/* a. 将关键字字符串驻留，获得唯一的 Symbol ID */                \
		sym = intern(&ctx->itn, str_from_cstr(STR));                     \
		/* b. 获取对应的枚举值 (假设 token.h 中定义了 TokenKind_##ID) */ \
		kind = TokenKind_##ID;                                           \
		/* c. 存入 Map */                                                \
		map_put(ctx->kw_map, sym, kind);                                 \
	} while (0);

#include "token.def"
}

void context_deinit(struct Context *ctx)
{
	map_deinit(ctx->kw_map);

	intern_deinit(&ctx->itn);

	srcmanager_deinit(&ctx->mgr);

	types_deinit(ctx->alc);
}

/*
 * ==========================================================================
 * 3. Error Reporting
 * ==========================================================================
 */

void ctx_error(struct Context *ctx, const struct Token *tok, const char *fmt,
	       ...)
{
	if (ctx->panic_mode)
		return;

	ctx->panic_mode = true;
	ctx->had_error = true;

	srcloc_t loc = { 0 };
	bool has_loc = false;

	if (tok) {
		has_loc = srcmanager_lookup(&ctx->mgr, tok->span.start, &loc);
	}

	if (has_loc) {
		fprintf(stderr, "%s:%zu:%zu: Error: ", loc.filename, loc.line,
			loc.col);
	} else {
		fprintf(stderr, "Error: ");
	}

	va_list ap;
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	fprintf(stderr, "\n");

	if (has_loc) {
		str_t line_content =
			srcmanager_get_line_content(&ctx->mgr, tok->span.start);

		if (line_content.len > 0) {
			fprintf(stderr, "    %.*s\n", (int)line_content.len,
				line_content.ptr);

			fprintf(stderr, "    %*s^\n", (int)(loc.col - 1), "");
		}
	}
}
