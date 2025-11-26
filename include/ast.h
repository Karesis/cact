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
#include <token.h>

struct Type;
struct SemaSymbol;

/*
 * ==========================================================================
 * 1. Node Kinds
 * ==========================================================================
 */

typedef enum NodeKind {

	ND_LIT_INT,
	ND_LIT_FLOAT,
	ND_LIT_DOUBLE,
	ND_LIT_BOOL,
	ND_INIT_LIST,

	ND_VAR,
	ND_FUNC_CALL,
	ND_ARRAY_ACCESS,

	ND_NEG,
	ND_LOG_NOT,
	ND_CAST,

	ND_ADD,
	ND_SUB,
	ND_MUL,
	ND_DIV,
	ND_MOD,

	ND_EQ,
	ND_NE,
	ND_LT,
	ND_LE,
	ND_GT,
	ND_GE,

	ND_LOG_AND,
	ND_LOG_OR,

	ND_ASSIGN,

	ND_BLOCK,
	ND_IF,
	ND_WHILE,
	ND_RETURN,
	ND_EXPR_STMT,
	ND_VAR_DECL,
	ND_BREAK,
	ND_CONTINUE,
} NodeKind;

/*
 * ==========================================================================
 * 2. Base Node
 * ==========================================================================
 */

struct Node {
	NodeKind kind;
	struct Token *tok;
	struct Type *ty;
};

defVec(struct Node *, NodeVec);

/*
 * ==========================================================================
 * 3. Concrete Nodes (Struct Inheritance)
 * ==========================================================================
 */

struct NodeLitInt {
	struct Node base;
	int val;
};

struct NodeLitFloat {
	struct Node base;
	float val;
};

struct NodeLitDouble {
	struct Node base;
	double val;
};

struct NodeLitBool {
	struct Node base;
	bool val;
};

struct NodeInitList {
	struct Node base;
	NodeVec inits;
};

struct NodeVar {
	struct Node base;
	struct SemaSymbol *var;
};

struct NodeUnary {
	struct Node base;
	struct Node *lhs;
};

struct NodeBinary {
	struct Node base;
	struct Node *lhs;
	struct Node *rhs;
};

struct NodeIf {
	struct Node base;
	struct Node *cond;
	struct Node *then_branch;
	struct Node *else_branch;
};

struct NodeWhile {
	struct Node base;
	struct Node *cond;
	struct Node *body;
};

struct NodeBlock {
	struct Node base;
	NodeVec stmts;
};

struct NodeCall {
	struct Node base;
	char *func_name;
	NodeVec args;
};

struct NodeVarDecl {
	struct Node base;
	struct SemaSymbol *var;
	struct Node *init;
};

/*
 * ==========================================================================
 * 4. Helpers (Downcasting)
 * ==========================================================================
 * 利用 container_of 或直接强制转换（因为 base 是第一个成员）
 */

#define as_binary(n) ((struct NodeBinary *)(n))
#define as_unary(n) ((struct NodeUnary *)(n))
#define as_if(n) ((struct NodeIf *)(n))
#define as_while(n) ((struct NodeWhile *)(n))
#define as_block(n) ((struct NodeBlock *)(n))
#define as_var(n) ((struct NodeVar *)(n))
#define as_call(n) ((struct NodeCall *)(n))
#define as_decl(n) ((struct NodeVarDecl *)(n))

#define as_lit_int(n) ((struct NodeLitInt *)(n))
#define as_lit_float(n) ((struct NodeLitFloat *)(n))
#define as_lit_double(n) ((struct NodeLitDouble *)(n))
#define as_lit_bool(n) ((struct NodeLitBool *)(n))
#define as_init_list(n) ((struct NodeInitList *)(n))
