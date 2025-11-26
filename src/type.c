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

#include <type.h>
#include <core/msg.h>

struct Type *ty_void;
struct Type *ty_bool;
struct Type *ty_int;
struct Type *ty_float;
struct Type *ty_double;

static struct Type *new_primitive(allocer_t alc, TypeKind kind, int size,
				  int align)
{
	struct Type *ty = alloc_type(alc, struct Type);
	ty->kind = kind;
	ty->size = size;
	ty->align = align;
	return ty;
}

void types_init(allocer_t alc)
{
	ty_void = new_primitive(alc, TypeKind_VOID, 0, 0);
	ty_bool = new_primitive(alc, TypeKind_BOOL, 1, 1);
	ty_int = new_primitive(alc, TypeKind_INT, 4, 4);
	ty_float = new_primitive(alc, TypeKind_FLOAT, 4, 4);
	ty_double = new_primitive(alc, TypeKind_DOUBLE, 8, 8);
}

static void free_primitive(allocer_t alc, struct Type *ty)
{
	if (ty) {
		free_type(alc, ty);
	}
}

void types_deinit(allocer_t alc)
{
	free_primitive(alc, ty_void);
	free_primitive(alc, ty_bool);
	free_primitive(alc, ty_int);
	free_primitive(alc, ty_float);
	free_primitive(alc, ty_double);

	ty_void = NULL;
	ty_bool = NULL;
	ty_int = NULL;
	ty_float = NULL;
	ty_double = NULL;
}

struct Type *type_array_of(allocer_t alc, struct Type *base, int len)
{
	struct Type *ty = alloc_type(alc, struct Type);
	ty->kind = TypeKind_ARRAY;
	ty->data.array.base = base;
	ty->data.array.len = len;

	ty->size = base->size * len;
	ty->align = base->align;
	return ty;
}

struct Type *type_func_new(allocer_t alc, struct Type *ret)
{
	struct Type *ty = alloc_type(alc, struct Type);
	ty->kind = TypeKind_FUNC;
	ty->data.func.ret = ret;
	ty->size = 8;
	ty->align = 8;

	massert(vec_init(ty->data.func.params, alc, 4),
		"Func params init failed");
	return ty;
}

bool type_eq(struct Type *a, struct Type *b)
{
	if (a == b)
		return true;
	if (!a || !b)
		return false;
	if (a->kind != b->kind)
		return false;

	switch (a->kind) {
	case TypeKind_ARRAY:
		return a->data.array.len == b->data.array.len &&
		       type_eq(a->data.array.base, b->data.array.base);
	case TypeKind_FUNC:
		if (!type_eq(a->data.func.ret, b->data.func.ret))
			return false;
		if (a->data.func.params.len != b->data.func.params.len)
			return false;

		for (usize i = 0; i < a->data.func.params.len; ++i) {
			if (!type_eq(vec_at(a->data.func.params, i),
				     vec_at(b->data.func.params, i)))
				return false;
		}
		return true;
	default:
		return false;
	}
}

bool type_is_arithmetic(struct Type *ty)
{
	return ty == ty_int || ty == ty_float || ty == ty_double;
}
