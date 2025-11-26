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

#pragma once
#include <core/span.h>
#include <core/type.h>
#include <std/strings/str.h>
#include <std/strings/intern.h>

typedef enum TokenKind {
#define TOK(ID) TokenKind_##ID,
#define KW(ID, STR) TokenKind_##ID,
#define PUNCT(ID, STR) TokenKind_##ID,
#include <token.def>
	TokenKind_COUNT,
} TokenKind;

struct Token {
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
