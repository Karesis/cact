#include <lexer.h>
#include <token.h>
#include <context.h>

#include <core/msg.h> // for massert, log_error
#include <core/macros.h> // for unlikely, unused
#include <std/strings/chars.h> // for char_is_alpha, etc.
#include <std/strings/str.h> // for str_from_parts
#include <stdlib.h> // for strtod, strtoll

void lexer_init(struct Lexer *lex, const srcmanager_t *mgr, struct Context *ctx,
		usize file_id)
{
	lex->mgr = mgr;
	lex->ctx = ctx;
	lex->file_id = file_id;

	/// for convenience
	const srcfile_t *file = srcmanager_get_file(mgr, file_id);

	lex->content_start = file->content;
	lex->content_len = file->len;
	lex->cursor = 0;
}

/*
 * ==========================================================================
 * 1. Internal Helpers
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
 * * @param l The lexer
 * @param start_cursor The local cursor where the token started
 */
static inline span_t make_span(struct Lexer *l, usize start_cursor)
{
	// 1. Get file base offset (cached or retrieved)
	// Optimization: You could cache base_offset in Lexer struct during init
	const srcfile_t *file = srcmanager_get_file(l->mgr, l->file_id);
	usize base = file->base_offset;

	// 2. Calculate global positions
	return span(base + start_cursor, base + l->cursor);
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
				// Line comment: skip until \n
				while (peek(l) != '\n' && peek(l) != '\0') {
					advance(l);
				}
			} else if (peek_next(l) == '*') {
				// Block comment: skip until */
				advance(l); // eat /
				advance(l); // eat *
				while (true) {
					if (peek(l) == '\0') {
						// EOF inside comment
						// Maybe need to return err token?
						// Just stop, let next call to lexer_next handle EOF
						return;
					}
					if (peek(l) == '*' &&
					    peek_next(l) == '/') {
						advance(l); // eat *
						advance(l); // eat /
						break;
					}
					advance(l);
				}
			} else {
				return; // Just a slash, handled in next_token
			}
			break;
		default:
			return;
		}
	}
}

/*
 * ==========================================================================
 * 2. Scanners
 * ==========================================================================
 */

static struct Token scan_identifier(struct Lexer *l)
{
	usize start = l->cursor;

	// Consume alphanumeric + _
	while (char_is_alphanum(peek(l)) || peek(l) == '_') {
		advance(l);
	}

	usize len = l->cursor - start;
	span_t sp = make_span(l, start);

	// 1. Create slice
	str_t s = str_from_parts(l->content_start + start, len);

	// 2. Intern string
	symbol_t sym = intern(&l->ctx->itn, s); // Fix: use ctx->itn

	// 3. Check Keyword Map
	TokenKind *kind_ptr = map_get(l->ctx->kw_map, sym);

	if (kind_ptr) {
		// It is a keyword (e.g., "int", "if")
		return (struct Token){
			.kind = *kind_ptr, .span = sp,
			// keywords don't need value
		};
	}

	// It is an identifier
	return (struct Token){ .kind = TokenKind_IDENT,
			       .span = sp,
			       .value.name = sym };
}

static struct Token scan_number(struct Lexer *l)
{
    usize start = l->cursor;
    bool has_dot = false;
    bool has_exponent = false;
    bool is_float_32 = false; // 移动到这里声明

    // 1. Check Hex (0x...)
    // 如果是十六进制，我们只消耗字符，不设置 has_dot/exponent
    if (peek(l) == '0' && (peek_next(l) == 'x' || peek_next(l) == 'X')) {
        advance(l); // 0
        advance(l); // x
        while (char_is_hex(peek(l))) {
            advance(l);
        }
        // 注意：这里不再 goto，而是自然往下执行
    } 
    else {
        // 2. Decimal / Float Logic (放在 else 里)
        
        // 整数部分
        while (char_is_digit(peek(l)))
            advance(l);

        // 小数部分
        if (peek(l) == '.') {
            has_dot = true;
            advance(l); // consume .
            while (char_is_digit(peek(l)))
                advance(l);
        }

        // 指数部分
        if (peek(l) == 'e' || peek(l) == 'E') {
            has_exponent = true;
            advance(l);
            if (peek(l) == '+' || peek(l) == '-')
                advance(l);
            
            if (!char_is_digit(peek(l))) {
                // 指数符号后必须跟数字
                return (struct Token){ .kind = TokenKind_ERROR,
                                       .span = make_span(l, start) };
            }
            while (char_is_digit(peek(l)))
                advance(l);
        }

        // 后缀 f/F
        if (peek(l) == 'f' || peek(l) == 'F') {
            // 校验：整数加 f 后缀算错误 (如 3f)
            if (!has_dot && !has_exponent) {
                return (struct Token){ .kind = TokenKind_ERROR,
                                       .span = make_span(l, start) };
            }
            is_float_32 = true;
            advance(l);
        }
    }

    // ==================================================
    // 3. 统一生成 Token (Common Path)
    // ==================================================
    
    // 现在无论走哪条路，都会执行这里的初始化
    span_t sp = make_span(l, start);
    usize len = l->cursor - start;

    // Copy to buffer
    char buf[128];
    if (len >= sizeof(buf))
        return (struct Token){ .kind = TokenKind_ERROR, .span = sp };
    
    memcpy(buf, l->content_start + start, len);
    buf[len] = '\0';

    // 4. 分发类型
    if (has_dot || has_exponent) {
        if (is_float_32) {
            return (struct Token){ .kind = TokenKind_LIT_FLOAT,
                                   .span = sp,
                                   .value.as_float = strtof(buf, NULL) };
        } else {
            return (struct Token){ .kind = TokenKind_LIT_DOUBLE,
                                   .span = sp,
                                   .value.as_double = strtod(buf, NULL) };
        }
    }

    // 整数 (Decimal or Hex)
    // strtoll with base 0 handles "0x" prefix automatically!
    return (struct Token){ .kind = TokenKind_LIT_INT,
                           .span = sp,
                           .value.as_int = (int)strtoll(buf, NULL, 0) };
}

/*
 * ==========================================================================
 * 3. Public API
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

	// --- Identifiers & Keywords ---
	if (char_is_alpha(c) || c == '_') {
		// Backtrack one char to let scan_identifier handle the whole string
		l->cursor--;
		return scan_identifier(l);
	}

	// --- Numbers ---
	if (char_is_digit(c)) {
		l->cursor--;
		return scan_number(l);
	}

// --- Punctuation ---
// Macro to simplify single char tokens
#define RET_TOK(k)                                                 \
	return (struct Token)                                      \
	{                                                          \
		.kind = TokenKind_##k, .span = make_span(l, start) \
	}

	switch (c) {

	case '.':
        if (char_is_digit(peek(l))) {
            l->cursor--; // Backup, let scan_number handle this.
            return scan_number(l);
        }
        goto error;

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

	// Double char tokens
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
		goto error; // CACT doesn't support bitwise & yet?

	case '|':
		if (match(l, '|'))
			RET_TOK(LOG_OR);
		goto error;

	case '/':
		// Comments are handled in skip_whitespace, so if we are here, it's a division
		RET_TOK(SLASH);

	default:
		goto error;
	}

error:
	return (struct Token){ .kind = TokenKind_ERROR,
			       .span = make_span(l, start) };

#undef RET_TOK
}
