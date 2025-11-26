#include <token.h>

str_t tokenkind_to_str(TokenKind kind)
{
	return str_from_cstr(tokenkind_to_cstr(kind));
}

const char *tokenkind_to_cstr(TokenKind kind)
{
	switch (kind) {

		/// keyword
#define KW(ID, TEXT)        \
	case TokenKind_##ID: \
		return TEXT;

		/// punctuation
#define PUNCT(ID, TEXT)  \
	case TokenKind_##ID: \
		return TEXT;

		/// other tokens
#define TOK(ID)          \
	case TokenKind_##ID: \
		return "TOKEN_" #ID;

#include <token.def>

	default:
		return "<UNKNOWN>";
	}
}


