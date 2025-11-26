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
