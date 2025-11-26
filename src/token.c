#include <token.h>

str_t tokenkind_to_str(TokenKind kind)
{
	return str_from_cstr(tokenkind_to_cstr(kind));
}

const char *tokenkind_to_cstr(TokenKind kind)
{
	switch (kind) {
#define KW(ID, TEXT)         \
	case TokenKind_##ID: \
		return TEXT;

#define PUNCT(ID, TEXT)      \
	case TokenKind_##ID: \
		return TEXT;

#define TOK(ID)              \
	case TokenKind_##ID: \
		return "TOKEN_" #ID;

#include <token.def>

	default:
		return "<UNKNOWN>";
	}
}
