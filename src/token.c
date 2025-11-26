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
