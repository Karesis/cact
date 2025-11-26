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
#include <std/vec.h>

typedef enum TypeKind {
	TypeKind_VOID,
	TypeKind_BOOL,
	TypeKind_INT,
	TypeKind_FLOAT,
	TypeKind_DOUBLE,
	TypeKind_ARRAY,
	TypeKind_FUNC
} TypeKind;

struct Type {
	TypeKind kind;
	int size;
	int align;

	union data {
		struct array {
			struct Type *base;
			int len;
		} array;

		struct func {
			struct Type *ret;

			vec(struct Type *) params;
		} func;
	} data;
};

extern struct Type *ty_void;
extern struct Type *ty_bool;
extern struct Type *ty_int;
extern struct Type *ty_float;
extern struct Type *ty_double;

void types_init(allocer_t alc);
void types_deinit(allocer_t alc);

struct Type *type_array_of(allocer_t alc, struct Type *base, int len);
struct Type *type_func_new(allocer_t alc, struct Type *ret);

bool type_eq(struct Type *a, struct Type *b);

bool type_is_arithmetic(struct Type *ty);
