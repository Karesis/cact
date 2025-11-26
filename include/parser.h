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

#include "lexer.h"
#include "sema.h"
#include "ast.h"
#include <core/mem/allocer.h>

/*
 * ==========================================================================
 * Parser Structure
 * ==========================================================================
 */

struct Parser {
	struct Context *ctx;
	struct Lexer *lex;

	struct Sema sema;

	struct Token curr;
	struct Token prev;

	allocer_t alc;

	bool panic_mode;
};

/*
 * ==========================================================================
 * Public API
 * ==========================================================================
 */

/**
 * @brief Initialize the parser.
 * * @param p   The parser instance.
 * @param ctx The global context.
 * @param lex The initialized lexer.
 */
void parser_init(struct Parser *p, struct Context *ctx, struct Lexer *lex);

/**
 * @brief Parse the entire compilation unit.
 * * According to CACT spec, a CompUnit consists of Decl or FuncDef.
 * * @return A vector of (struct Node*) representing top-level declarations/functions.
 * Returns an empty vector (or valid vector with 0 len) on error.
 */
NodeVec parser_parse(struct Parser *p);
