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

#include <lexer.h>
#include <token.h>
#include <context.h>

#include <core/msg.h>
#include <core/macros.h>
#include <std/strings/chars.h>
#include <std/strings/str.h>
#include <stdlib.h>
#include <stdio.h>

/*
 * ==========================================================================
 * 1. Initialization
 * ==========================================================================
 */

void lexer_init(struct Lexer *lex, struct Context *ctx, usize file_id)
{
	lex->ctx = ctx;
	lex->file_id = file_id;

	const srcfile_t *file = srcmanager_get_file(&ctx->mgr, file_id);
	massert(file != NULL, "File ID %zu not found in Context SourceManager",
		file_id);

	lex->content_start = file->content;
	lex->content_len = file->len;
	lex->cursor = 0;
}

/*
 * ==========================================================================
 * 2. Internal Helpers
 * ==========================================================================
 */

static inline char peek(struct Lexer *l)
{
	if (l->cursor >= l->content_len)
		return '\0';
	return l->content_start[l->cursor];
}

static inline char peek_next(struct Lexer *l)
{
	if (l->cursor + 1 >= l->content_len)
		return '\0';
	return l->content_start[l->cursor + 1];
}

static inline char advance(struct Lexer *l)
{
	if (l->cursor >= l->content_len)
		return '\0';
	return l->content_start[l->cursor++];
}

static inline bool match(struct Lexer *l, char expected)
{
	if (peek(l) == expected) {
		l->cursor++;
		return true;
	}
	return false;
}

/**
 * @brief Calculate the global span for the current token.
 * @param l The lexer
 * @param start_cursor The local cursor where the token started
 */
static inline span_t make_span(struct Lexer *l, usize start_cursor)
{
	const srcfile_t *file = srcmanager_get_file(&l->ctx->mgr, l->file_id);
	usize base = file->base_offset;
	return span(base + start_cursor, base + l->cursor);
}

/**
 * @brief Helper to generate an error token and report it via Context.
 */
static struct Token lexer_error_token(struct Lexer *l, const char *msg,
				      usize start)
{
	struct Token err_tok;
	err_tok.kind = TokenKind_ERROR;
	err_tok.span = make_span(l, start);

	ctx_error(l->ctx, &err_tok, "%s", msg);

	return err_tok;
}

static void skip_whitespace(struct Lexer *l)
{
	while (true) {
		char c = peek(l);
		switch (c) {
		case ' ':
		case '\r':
		case '\t':
		case '\n':
		case '\v':
		case '\f':
			advance(l);
			break;
		case '/':
			if (peek_next(l) == '/') {
				while (peek(l) != '\n' && peek(l) != '\0') {
					advance(l);
				}
			} else if (peek_next(l) == '*') {
				advance(l);
				advance(l);
				while (true) {
					if (peek(l) == '\0') {
						return;
					}
					if (peek(l) == '*' &&
					    peek_next(l) == '/') {
						advance(l);
						advance(l);
						break;
					}
					advance(l);
				}
			} else {
				return;
			}
			break;
		default:
			return;
		}
	}
}

/*
 * ==========================================================================
 * 3. Scanners
 * ==========================================================================
 */

static struct Token scan_identifier(struct Lexer *l)
{
	usize start = l->cursor;

	while (char_is_alphanum(peek(l)) || peek(l) == '_') {
		advance(l);
	}

	span_t sp = make_span(l, start);
	usize len = l->cursor - start;

	str_t s = str_from_parts(l->content_start + start, len);

	symbol_t sym = intern(&l->ctx->itn, s);

	TokenKind *kind_ptr = map_get(l->ctx->kw_map, sym);

	if (kind_ptr) {
		return (struct Token){ .kind = *kind_ptr, .span = sp };
	}

	return (struct Token){ .kind = TokenKind_IDENT,
			       .span = sp,
			       .value.name = sym };
}

static struct Token scan_number(struct Lexer *l)
{
	usize start = l->cursor;
	bool has_dot = false;
	bool has_exponent = false;
	bool is_float_32 = false;

	if (peek(l) == '0' && (peek_next(l) == 'x' || peek_next(l) == 'X')) {
		advance(l);
		advance(l);

		if (!char_is_hex(peek(l))) {
			return lexer_error_token(
				l, "Hex literal must have at least one digit",
				start);
		}

		while (char_is_hex(peek(l))) {
			advance(l);
		}
	} else {
		while (char_is_digit(peek(l)))
			advance(l);

		if (peek(l) == '.') {
			has_dot = true;
			advance(l);
			while (char_is_digit(peek(l)))
				advance(l);
		}

		if (peek(l) == 'e' || peek(l) == 'E') {
			has_exponent = true;
			advance(l);
			if (peek(l) == '+' || peek(l) == '-')
				advance(l);

			if (!char_is_digit(peek(l))) {
				return lexer_error_token(
					l, "Exponent has no digits", start);
			}
			while (char_is_digit(peek(l)))
				advance(l);
		}

		if (peek(l) == 'f' || peek(l) == 'F') {
			if (!has_dot && !has_exponent) {
				return lexer_error_token(
					l,
					"Invalid suffix 'f' on integer constant",
					start);
			}
			is_float_32 = true;
			advance(l);
		}
	}

	span_t sp = make_span(l, start);
	usize len = l->cursor - start;

	char buf[128];
	if (len >= sizeof(buf)) {
		return lexer_error_token(l, "Number literal too long", start);
	}
	memcpy(buf, l->content_start + start, len);
	buf[len] = '\0';

	if (has_dot || has_exponent) {
		if (is_float_32) {
			return (struct Token){ .kind = TokenKind_LIT_FLOAT,
					       .span = sp,
					       .value.as_float =
						       strtof(buf, NULL) };
		} else {
			return (struct Token){ .kind = TokenKind_LIT_DOUBLE,
					       .span = sp,
					       .value.as_double =
						       strtod(buf, NULL) };
		}
	}

	return (struct Token){ .kind = TokenKind_LIT_INT,
			       .span = sp,
			       .value.as_int = (int)strtoll(buf, NULL, 0) };
}

/*
 * ==========================================================================
 * 4. Public API Implementation
 * ==========================================================================
 */

struct Token lexer_next(struct Lexer *l)
{
	skip_whitespace(l);

	usize start = l->cursor;

	if (l->cursor >= l->content_len) {
		return (struct Token){ .kind = TokenKind_EOF,
				       .span = make_span(l, start) };
	}

	char c = advance(l);

	if (char_is_alpha(c) || c == '_') {
		l->cursor--;
		return scan_identifier(l);
	}

	if (char_is_digit(c)) {
		l->cursor--;
		return scan_number(l);
	}

#define RET_TOK(k)                                                 \
	return (struct Token)                                      \
	{                                                          \
		.kind = TokenKind_##k, .span = make_span(l, start) \
	}

	switch (c) {
	case '.':
		if (char_is_digit(peek(l))) {
			l->cursor--;
			return scan_number(l);
		}
		return lexer_error_token(l, "Unexpected character '.'", start);

	case '+':
		RET_TOK(PLUS);
	case '-':
		RET_TOK(MINUS);
	case '*':
		RET_TOK(STAR);
	case '%':
		RET_TOK(PERCENT);
	case '(':
		RET_TOK(L_PAREN);
	case ')':
		RET_TOK(R_PAREN);
	case '{':
		RET_TOK(L_BRACE);
	case '}':
		RET_TOK(R_BRACE);
	case '[':
		RET_TOK(L_BRACKET);
	case ']':
		RET_TOK(R_BRACKET);
	case ';':
		RET_TOK(SEMICOLON);
	case ',':
		RET_TOK(COMMA);

	case '=':
		if (match(l, '='))
			RET_TOK(EQ);
		RET_TOK(ASSIGN);

	case '!':
		if (match(l, '='))
			RET_TOK(NEQ);
		RET_TOK(LOG_NOT);

	case '<':
		if (match(l, '='))
			RET_TOK(LE);
		RET_TOK(LT);

	case '>':
		if (match(l, '='))
			RET_TOK(GE);
		RET_TOK(GT);

	case '&':
		if (match(l, '&'))
			RET_TOK(LOG_AND);

		return lexer_error_token(
			l,
			"Unexpected character '&' (Bitwise AND not supported)",
			start);

	case '|':
		if (match(l, '|'))
			RET_TOK(LOG_OR);
		return lexer_error_token(
			l,
			"Unexpected character '|' (Bitwise OR not supported)",
			start);

	case '/':

		RET_TOK(SLASH);

	default: {
		char msg[64];
		snprintf(msg, sizeof(msg), "Unexpected character '%c'", c);
		return lexer_error_token(l, msg, start);
	}
	}

#undef RET_TOK
}
