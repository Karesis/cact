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

#include <core/type.h>
#include <context.h>
#include <std/fs/srcmanager.h>
#include <token.h>
/*
 * ==========================================================================
 * 1. Type Definition
 * ==========================================================================
 */

/**
 * @brief The Lexer Structure.
 * * Responsible for converting raw source text into a stream of Tokens.
 * It holds references to the SourceManager (for location info) and 
 * Context (for interning strings and keyword lookup).
 */
struct Lexer {
	struct Context *ctx;

	usize file_id;
	const char *content_start;
	usize content_len;
	usize cursor;
};

/*
 * ==========================================================================
 * 2. Public API
 * ==========================================================================
 */

/**
 * @brief Initialize a new Lexer for a specific file.
 * * @param lex [out] Pointer to the lexer structure to initialize.
 * @param mgr Pointer to the global SourceManager (must remain valid).
 * @param ctx Pointer to the compiler Context (must remain valid).
 * @param file_id The ID of the file to lex (returned by srcmanager_add).
 */
void lexer_init(struct Lexer *lex, struct Context *ctx, usize file_id);

/**
 * @brief Get the next Token from the stream.
 * * This function skips whitespace and comments, then parses the next valid token.
 * If the end of the file is reached, it returns a token with kind TOKEN_EOF.
 * * @param l Pointer to the initialized Lexer.
 * @return The next parsed Token.
 */
struct Token lexer_next(struct Lexer *l);

/**
 * @brief Peek the next Token without consuming it.
 * * @note This is optional for now. Implementing a true peek in a stateful lexer
 * usually requires saving/restoring state or using a lookahead buffer.
 * If you need this for the Parser, we can implement it later or let the Parser
 * handle the buffering (Parser usually holds `current_token` and `next_token`).
 */

/*
 * ==========================================================================
 * 3. Utility / Debugging
 * ==========================================================================
 */

/**
 * @brief Tokenize the entire file and return a vector of tokens.
 * * Useful for debugging or for passes that need random access to tokens.
 * * @note You need to include <std/vec.h> to use this, or forward declare a TokenVec.
 * Let's keep it commented out until you actually need a TokenVec.
 */
