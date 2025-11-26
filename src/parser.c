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

#include <parser.h>
#include <ast.h>
#include <core/msg.h>
#include <core/macros.h>

/*
 * ==========================================================================
 * 1. Infrastructure & Helpers
 * ==========================================================================
 */

static void advance(struct Parser *p)
{
	p->prev = p->curr;
	for (;;) {
		p->curr = lexer_next(p->lex);
		if (p->curr.kind != TokenKind_ERROR)
			break;
	}
}

static void parser_error_at(struct Parser *p, struct Token *tok,
			    const char *msg)
{
	if (p->panic_mode)
		return;
	p->panic_mode = true;
	ctx_error(p->ctx, tok, "%s", msg);
}

static void parser_error(struct Parser *p, const char *msg)
{
	parser_error_at(p, &p->curr, msg);
}

static void consume(struct Parser *p, TokenKind kind, const char *msg)
{
	if (p->curr.kind == kind) {
		advance(p);
		return;
	}
	parser_error(p, msg);
}

static bool match(struct Parser *p, TokenKind kind)
{
	if (p->curr.kind != kind)
		return false;
	advance(p);
	return true;
}

static bool check_kind(struct Parser *p, TokenKind kind)
{
	return p->curr.kind == kind;
}

static void synchronize(struct Parser *p)
{
	p->panic_mode = false;

	while (p->curr.kind != TokenKind_EOF) {
		if (p->prev.kind == TokenKind_SEMICOLON)
			return;

		switch (p->curr.kind) {
		case TokenKind_IF:
		case TokenKind_WHILE:
		case TokenKind_RETURN:
		case TokenKind_INT:
		case TokenKind_FLOAT:
		case TokenKind_DOUBLE:
		case TokenKind_BOOL:
		case TokenKind_VOID:
		case TokenKind_CONST:
			return;
		default:;
		}

		advance(p);
	}
}

static struct Node *new_node(struct Parser *p, NodeKind kind, size_t size)
{
	struct Node *n = allocer_alloc(p->alc, layout(size, 8));
	n->kind = kind;
	n->tok = alloc_type(p->alc, struct Token);
	*n->tok = p->prev;
	n->ty = NULL;
	return n;
}

#define NEW_NODE(p, StructType, Kind) \
	((StructType *)new_node(p, Kind, sizeof(StructType)))

static struct Type *token_to_type(TokenKind k)
{
	switch (k) {
	case TokenKind_INT:
		return ty_int;
	case TokenKind_FLOAT:
		return ty_float;
	case TokenKind_DOUBLE:
		return ty_double;
	case TokenKind_BOOL:
		return ty_bool;
	case TokenKind_VOID:
		return ty_void;
	default:
		return NULL;
	}
}

/*
 * ==========================================================================
 * 2. Expression Parsing
 * ==========================================================================
 */

static struct Node *parse_expr(struct Parser *p);
static struct Node *parse_assign(struct Parser *p);

static struct Node *parse_primary(struct Parser *p)
{
	if (match(p, TokenKind_LIT_INT)) {
		struct NodeLitInt *n =
			NEW_NODE(p, struct NodeLitInt, ND_LIT_INT);
		n->val = p->prev.value.as_int;
		n->base.ty = ty_int;
		return (struct Node *)n;
	}
	if (match(p, TokenKind_LIT_FLOAT)) {
		struct NodeLitFloat *n =
			NEW_NODE(p, struct NodeLitFloat, ND_LIT_FLOAT);
		n->val = p->prev.value.as_float;
		n->base.ty = ty_float;
		return (struct Node *)n;
	}
	if (match(p, TokenKind_LIT_DOUBLE)) {
		struct NodeLitDouble *n =
			NEW_NODE(p, struct NodeLitDouble, ND_LIT_DOUBLE);
		n->val = p->prev.value.as_double;
		n->base.ty = ty_double;
		return (struct Node *)n;
	}
	if (match(p, TokenKind_TRUE)) {
		struct NodeLitBool *n =
			NEW_NODE(p, struct NodeLitBool, ND_LIT_BOOL);
		n->val = true;
		n->base.ty = ty_bool;
		return (struct Node *)n;
	}
	if (match(p, TokenKind_FALSE)) {
		struct NodeLitBool *n =
			NEW_NODE(p, struct NodeLitBool, ND_LIT_BOOL);
		n->val = false;
		n->base.ty = ty_bool;
		return (struct Node *)n;
	}

	if (match(p, TokenKind_L_PAREN)) {
		struct Node *expr = parse_expr(p);
		consume(p, TokenKind_R_PAREN, "Expect ')' after expression");
		return expr;
	}

	if (match(p, TokenKind_IDENT)) {
		symbol_t name = p->prev.value.name;

		if (check_kind(p, TokenKind_L_PAREN)) {
			consume(p, TokenKind_L_PAREN, "");

			struct SemaSymbol *func_sym =
				sema_lookup(&p->sema, name);
			if (!func_sym) {
				parser_error(p, "Undefined function call");
			}

			struct NodeCall *n =
				NEW_NODE(p, struct NodeCall, ND_FUNC_CALL);

			n->func_name = intern_resolve_cstr(&p->ctx->itn, name);

			massert(vec_init(n->args, p->alc, 4),
				"OOM in call args");

			if (!check_kind(p, TokenKind_R_PAREN)) {
				do {
					struct Node *arg = parse_assign(p);
					vec_push(n->args, arg);
				} while (match(p, TokenKind_COMMA));
			}
			consume(p, TokenKind_R_PAREN,
				"Expect ')' after arguments");

			if (func_sym && func_sym->ty->kind == TypeKind_FUNC) {
				n->base.ty = func_sym->ty->data.func.ret;
			} else {
				n->base.ty = ty_int;
			}
			return (struct Node *)n;
		}

		struct SemaSymbol *sym = sema_lookup(&p->sema, name);
		if (!sym) {
			parser_error(p, "Undefined variable");
		}

		struct NodeVar *n = NEW_NODE(p, struct NodeVar, ND_VAR);
		n->var = sym;
		n->base.ty = sym ? sym->ty : ty_int;

		struct Node *curr = (struct Node *)n;
		while (match(p, TokenKind_L_BRACKET)) {
			struct Node *index = parse_expr(p);
			consume(p, TokenKind_R_BRACKET, "Expect ']'");

			struct NodeBinary *access =
				NEW_NODE(p, struct NodeBinary, ND_ARRAY_ACCESS);
			access->lhs = curr;
			access->rhs = index;

			if (curr->ty && curr->ty->kind == TypeKind_ARRAY) {
				access->base.ty = curr->ty->data.array.base;
			} else {
				parser_error(
					p, "Subscripted value is not an array");
				access->base.ty = ty_int;
			}
			curr = (struct Node *)access;
		}
		return curr;
	}

	parser_error(p, "Expect expression");
	return NULL;
}

static struct Node *parse_unary(struct Parser *p)
{
	if (match(p, TokenKind_PLUS))
		return parse_unary(p);

	if (match(p, TokenKind_MINUS)) {
		struct NodeUnary *n = NEW_NODE(p, struct NodeUnary, ND_NEG);
		n->lhs = parse_unary(p);

		if (n->lhs && n->lhs->ty)
			n->base.ty = n->lhs->ty;
		return (struct Node *)n;
	}

	if (match(p, TokenKind_LOG_NOT)) {
		struct NodeUnary *n = NEW_NODE(p, struct NodeUnary, ND_LOG_NOT);
		n->lhs = parse_unary(p);
		n->base.ty = ty_bool;
		return (struct Node *)n;
	}

	return parse_primary(p);
}

static int get_prec(TokenKind k)
{
	switch (k) {
	case TokenKind_STAR:
	case TokenKind_SLASH:
	case TokenKind_PERCENT:
		return 10;
	case TokenKind_PLUS:
	case TokenKind_MINUS:
		return 9;
	case TokenKind_LT:
	case TokenKind_LE:
	case TokenKind_GT:
	case TokenKind_GE:
		return 8;
	case TokenKind_EQ:
	case TokenKind_NEQ:
		return 7;
	case TokenKind_LOG_AND:
		return 5;
	case TokenKind_LOG_OR:
		return 4;
	default:
		return -1;
	}
}

static NodeKind get_binary_kind(TokenKind k)
{
	switch (k) {
	case TokenKind_PLUS:
		return ND_ADD;
	case TokenKind_MINUS:
		return ND_SUB;
	case TokenKind_STAR:
		return ND_MUL;
	case TokenKind_SLASH:
		return ND_DIV;
	case TokenKind_PERCENT:
		return ND_MOD;
	case TokenKind_EQ:
		return ND_EQ;
	case TokenKind_NEQ:
		return ND_NE;
	case TokenKind_LT:
		return ND_LT;
	case TokenKind_LE:
		return ND_LE;
	case TokenKind_GT:
		return ND_GT;
	case TokenKind_GE:
		return ND_GE;
	case TokenKind_LOG_AND:
		return ND_LOG_AND;
	case TokenKind_LOG_OR:
		return ND_LOG_OR;
	default:
		return ND_ADD;
	}
}

static struct Node *parse_binary(struct Parser *p, int prec)
{
	struct Node *lhs = parse_unary(p);

	for (;;) {
		int current_prec = get_prec(p->curr.kind);
		if (current_prec < prec)
			break;

		TokenKind op_token = p->curr.kind;
		advance(p);

		struct Node *rhs = parse_binary(p, current_prec + 1);

		struct NodeBinary *n = NEW_NODE(p, struct NodeBinary,
						get_binary_kind(op_token));
		n->lhs = lhs;
		n->rhs = rhs;

		sema_analyze_binary(&p->sema, n);

		lhs = (struct Node *)n;
	}
	return lhs;
}

static struct Node *parse_assign(struct Parser *p)
{
	struct Node *lhs = parse_binary(p, 0);

	if (match(p, TokenKind_ASSIGN)) {
		struct Node *rhs = parse_assign(p);

		struct NodeBinary *n =
			NEW_NODE(p, struct NodeBinary, ND_ASSIGN);
		n->lhs = lhs;
		n->rhs = rhs;

		sema_analyze_assign(&p->sema, n);
		return (struct Node *)n;
	}
	return lhs;
}

static struct Node *parse_expr(struct Parser *p)
{
	return parse_binary(p, 0);
}

/*
 * ==========================================================================
 * 3. Statement Parsing
 * ==========================================================================
 */

static struct Node *parse_stmt(struct Parser *p);
static struct Node *parse_decl(struct Parser *p);

static struct Node *parse_block(struct Parser *p)
{
	consume(p, TokenKind_L_BRACE, "Expect '{' to begin block");

	struct NodeBlock *n = NEW_NODE(p, struct NodeBlock, ND_BLOCK);
	massert(vec_init(n->stmts, p->alc, 8), "OOM block");

	sema_scope_enter(&p->sema);

	while (!check_kind(p, TokenKind_R_BRACE) &&
	       !check_kind(p, TokenKind_EOF)) {
		if (p->panic_mode) {
			synchronize(p);

			if (check_kind(p, TokenKind_R_BRACE) ||
			    check_kind(p, TokenKind_EOF)) {
				break;
			}
		}

		struct Type *ty = token_to_type(p->curr.kind);
		bool is_decl = (ty != NULL) || check_kind(p, TokenKind_CONST);

		bool is_stmt_start = check_kind(p, TokenKind_IF) ||
				     check_kind(p, TokenKind_WHILE) ||
				     check_kind(p, TokenKind_RETURN) ||
				     check_kind(p, TokenKind_BREAK) ||
				     check_kind(p, TokenKind_CONTINUE) ||
				     check_kind(p, TokenKind_L_BRACE) ||

				     check_kind(p, TokenKind_IDENT) ||
				     check_kind(p, TokenKind_L_PAREN) ||
				     check_kind(p, TokenKind_LIT_INT) ||
				     check_kind(p, TokenKind_LIT_FLOAT) ||
				     check_kind(p, TokenKind_LIT_DOUBLE) ||
				     check_kind(p, TokenKind_TRUE) ||
				     check_kind(p, TokenKind_FALSE) ||
				     check_kind(p, TokenKind_PLUS) ||
				     check_kind(p, TokenKind_MINUS) ||
				     check_kind(p, TokenKind_LOG_NOT);

		if (!is_decl && !is_stmt_start) {
			parser_error(p, "Unexpected token in block");
			advance(p);
			continue;
		}

		if (is_decl) {
			struct Node *decl = parse_decl(p);
			if (decl)
				vec_push(n->stmts, decl);
		} else {
			struct Node *stmt = parse_stmt(p);
			if (stmt)
				vec_push(n->stmts, stmt);
		}
	}

	sema_scope_leave(&p->sema);
	consume(p, TokenKind_R_BRACE, "Expect '}' to end block");
	return (struct Node *)n;
}

static struct Node *parse_stmt(struct Parser *p)
{
	if (match(p, TokenKind_IF)) {
		struct NodeIf *n = NEW_NODE(p, struct NodeIf, ND_IF);
		consume(p, TokenKind_L_PAREN, "Expect '('");
		n->cond = parse_expr(p);
		consume(p, TokenKind_R_PAREN, "Expect ')'");
		n->then_branch = parse_stmt(p);
		if (match(p, TokenKind_ELSE)) {
			n->else_branch = parse_stmt(p);
		}
		return (struct Node *)n;
	}

	if (match(p, TokenKind_WHILE)) {
		struct NodeWhile *n = NEW_NODE(p, struct NodeWhile, ND_WHILE);
		consume(p, TokenKind_L_PAREN, "Expect '('");
		n->cond = parse_expr(p);
		consume(p, TokenKind_R_PAREN, "Expect ')'");
		n->body = parse_stmt(p);
		return (struct Node *)n;
	}

	if (match(p, TokenKind_RETURN)) {
		struct NodeUnary *n = NEW_NODE(p, struct NodeUnary, ND_RETURN);
		if (!check_kind(p, TokenKind_SEMICOLON)) {
			n->lhs = parse_expr(p);
		}
		consume(p, TokenKind_SEMICOLON, "Expect ';'");
		sema_analyze_return(&p->sema, n);
		return (struct Node *)n;
	}

	if (match(p, TokenKind_BREAK)) {
		struct Node *n = new_node(p, ND_BREAK, sizeof(struct Node));
		consume(p, TokenKind_SEMICOLON, "Expect ';'");
		return n;
	}

	if (match(p, TokenKind_CONTINUE)) {
		struct Node *n = new_node(p, ND_CONTINUE, sizeof(struct Node));
		consume(p, TokenKind_SEMICOLON, "Expect ';'");
		return n;
	}

	if (check_kind(p, TokenKind_L_BRACE)) {
		return parse_block(p);
	}

	struct Node *expr = parse_expr(p);

	if (expr == NULL) {
		return NULL;
	}

	if (match(p, TokenKind_ASSIGN)) {
		struct Node *rhs = parse_expr(p);

		if (rhs == NULL) {
			parser_error(p, "Expect expression after '='");
		} else {
			struct NodeBinary *assign =
				NEW_NODE(p, struct NodeBinary, ND_ASSIGN);
			assign->lhs = expr;
			assign->rhs = rhs;
			sema_analyze_assign(&p->sema, assign);
			expr = (struct Node *)assign;
		}
	}

	consume(p, TokenKind_SEMICOLON, "Expect ';'");

	struct NodeUnary *n = NEW_NODE(p, struct NodeUnary, ND_EXPR_STMT);
	n->lhs = expr;
	return (struct Node *)n;
}

/*
 * ==========================================================================
 * 4. Declarations
 * ==========================================================================
 */

static struct Type *parse_array_dims(struct Parser *p, struct Type *base)
{
	while (match(p, TokenKind_L_BRACKET)) {
		if (match(p, TokenKind_LIT_INT)) {
			int len = p->prev.value.as_int;
			consume(p, TokenKind_R_BRACKET, "Expect ']'");
			base = type_array_of(p->alc, base, len);
		} else {
			parser_error(p, "Array size must be constant int");

			if (check_kind(p, TokenKind_R_BRACKET))
				advance(p);
		}
	}
	return base;
}

static struct Node *parse_initializer_list(struct Parser *p)
{
	struct Token start_tok = p->curr;
	consume(p, TokenKind_L_BRACE, "Expect '{'");

	struct NodeInitList *n = NEW_NODE(p, struct NodeInitList, ND_INIT_LIST);
	massert(vec_init(n->inits, p->alc, 4), "OOM init list");

	if (!check_kind(p, TokenKind_R_BRACE)) {
		do {
			struct Node *val;
			if (check_kind(p, TokenKind_L_BRACE)) {
				val = parse_initializer_list(p);
			} else {
				val = parse_assign(p);
			}
			vec_push(n->inits, val);
		} while (match(p, TokenKind_COMMA));
	}

	consume(p, TokenKind_R_BRACE, "Expect '}'");
	return (struct Node *)n;
}

static struct Node *parse_var_decl_list(struct Parser *p, struct Type *base_ty,
					symbol_t first_name, bool is_const,
					bool is_global)
{
	struct NodeBlock *block = NEW_NODE(p, struct NodeBlock, ND_BLOCK);
	massert(vec_init(block->stmts, p->alc, 2), "OOM decl");

	symbol_t name = first_name;
	bool first = true;

	do {
		if (!first) {
			if (!check_kind(p, TokenKind_IDENT)) {
				parser_error(p, "Expect variable name");
				break;
			}
			advance(p);
			name = p->prev.value.name;
		}
		first = false;

		struct Type *ty = parse_array_dims(p, base_ty);

		struct SemaSymbol *sym =
			sema_define_var(&p->sema, name, ty, is_const);

		struct NodeVarDecl *n =
			NEW_NODE(p, struct NodeVarDecl, ND_VAR_DECL);
		n->var = sym;
		n->base.ty = ty;

		if (match(p, TokenKind_ASSIGN)) {
			if (check_kind(p, TokenKind_L_BRACE)) {
				n->init = parse_initializer_list(p);

			} else {
				n->init = parse_expr(p);

				if (n->init && !type_eq(ty, n->init->ty)) {
					ctx_error(p->ctx, n->base.tok,
						  "Init type mismatch");
				}
			}
		} else if (is_const) {
			parser_error(p, "Const variable must be initialized");
		}

		vec_push(block->stmts, (struct Node *)n);

	} while (match(p, TokenKind_COMMA));

	consume(p, TokenKind_SEMICOLON, "Expect ';'");
	return (struct Node *)block;
}

static struct Node *parse_decl(struct Parser *p)
{
	bool is_const = match(p, TokenKind_CONST);
	struct Type *base_ty = token_to_type(p->curr.kind);
	if (!base_ty) {
		parser_error(p, "Expect type name");
		return NULL;
	}
	advance(p);

	if (!check_kind(p, TokenKind_IDENT)) {
		parser_error(p, "Expect variable name");
		return NULL;
	}
	advance(p);
	symbol_t name = p->prev.value.name;

	return parse_var_decl_list(p, base_ty, name, is_const, false);
}

static struct Node *parse_func(struct Parser *p, struct Type *ret_ty,
			       symbol_t name)
{
	consume(p, TokenKind_L_PAREN, "");

	struct Type *func_ty = type_func_new(p->alc, ret_ty);

	struct SemaSymbol *func_sym =
		sema_define_var(&p->sema, name, func_ty, false);
	unused(func_sym);

	p->sema.curr_func_ret = ret_ty;
	sema_scope_enter(&p->sema);

	if (!check_kind(p, TokenKind_R_PAREN)) {
		do {
			struct Type *arg_ty = token_to_type(p->curr.kind);
			if (!arg_ty)
				parser_error(p, "Expect param type");
			advance(p);

			consume(p, TokenKind_IDENT, "Expect param name");
			symbol_t arg_name = p->prev.value.name;

			if (match(p, TokenKind_L_BRACKET)) {
				if (match(p, TokenKind_R_BRACKET)) {
					arg_ty = type_array_of(p->alc, arg_ty,
							       0);
				} else {
					if (match(p, TokenKind_LIT_INT)) {
						int len = p->prev.value.as_int;
						consume(p, TokenKind_R_BRACKET,
							"Expect ']'");
						arg_ty = type_array_of(
							p->alc, arg_ty, len);
					} else {
						parser_error(
							p, "Expect array size");
					}
				}

				arg_ty = parse_array_dims(p, arg_ty);
			}

			sema_define_var(&p->sema, arg_name, arg_ty, false);
			vec_push(func_ty->data.func.params, arg_ty);

		} while (match(p, TokenKind_COMMA));
	}
	consume(p, TokenKind_R_PAREN, "Expect ')'");

	struct NodeBlock *body = NEW_NODE(p, struct NodeBlock, ND_BLOCK);
	massert(vec_init(body->stmts, p->alc, 8), "OOM func body");

	consume(p, TokenKind_L_BRACE, "Expect '{'");

	while (!check_kind(p, TokenKind_R_BRACE) &&
	       !check_kind(p, TokenKind_EOF)) {
		if (p->panic_mode) {
			synchronize(p);

			if (check_kind(p, TokenKind_R_BRACE) ||
			    check_kind(p, TokenKind_EOF)) {
				break;
			}
		}

		struct Type *ty = token_to_type(p->curr.kind);
		bool is_decl = (ty != NULL) || check_kind(p, TokenKind_CONST);

		bool is_stmt_start = check_kind(p, TokenKind_IF) ||
				     check_kind(p, TokenKind_WHILE) ||
				     check_kind(p, TokenKind_RETURN) ||
				     check_kind(p, TokenKind_BREAK) ||
				     check_kind(p, TokenKind_CONTINUE) ||
				     check_kind(p, TokenKind_L_BRACE) ||

				     check_kind(p, TokenKind_IDENT) ||
				     check_kind(p, TokenKind_L_PAREN) ||
				     check_kind(p, TokenKind_LIT_INT) ||
				     check_kind(p, TokenKind_LIT_FLOAT) ||
				     check_kind(p, TokenKind_LIT_DOUBLE) ||
				     check_kind(p, TokenKind_TRUE) ||
				     check_kind(p, TokenKind_FALSE) ||
				     check_kind(p, TokenKind_PLUS) ||
				     check_kind(p, TokenKind_MINUS) ||
				     check_kind(p, TokenKind_LOG_NOT);

		if (!is_decl && !is_stmt_start) {
			parser_error(p, "Unexpected token in function body");
			advance(p);
			continue;
		}

		if (is_decl) {
			struct Node *decl = parse_decl(p);
			if (decl)
				vec_push(body->stmts, decl);
		} else {
			struct Node *stmt = parse_stmt(p);
			if (stmt)
				vec_push(body->stmts, stmt);
		}
	}

	consume(p, TokenKind_R_BRACE, "Expect '}'");

	sema_scope_leave(&p->sema);
	p->sema.curr_func_ret = NULL;

	return (struct Node *)body;
}

static struct Node *parse_top_level(struct Parser *p)
{
	bool is_const = match(p, TokenKind_CONST);
	struct Type *ty = token_to_type(p->curr.kind);

	if (!ty && !is_const)
		return NULL;
	if (!ty) {
		parser_error(p, "Expect type");
		advance(p);
		return NULL;
	}
	advance(p);

	if (!check_kind(p, TokenKind_IDENT)) {
		parser_error(p, "Expect name");
		return NULL;
	}
	consume(p, TokenKind_IDENT, "Expect name");
	symbol_t name = p->prev.value.name;

	if (check_kind(p, TokenKind_L_PAREN)) {
		return parse_func(p, ty, name);
	} else {
		return parse_var_decl_list(p, ty, name, is_const, true);
	}
}

/*
 * ==========================================================================
 * 5. Public API
 * ==========================================================================
 */

void parser_init(struct Parser *p, struct Context *ctx, struct Lexer *lex)
{
	p->ctx = ctx;
	p->lex = lex;
	p->alc = ctx->alc;
	p->panic_mode = false;
	sema_init(&p->sema, ctx);
	advance(p);
}

static void install_builtin(struct Parser *p, const char *name,
			    struct Type *ret, struct Type *arg1_ty)
{
	struct Type *func_ty = type_func_new(p->alc, ret);
	if (arg1_ty) {
		massert(vec_push(func_ty->data.func.params, arg1_ty),
			"OOM builtin");
	}

	symbol_t sym = intern_cstr(&p->ctx->itn, name);
	sema_define_var(&p->sema, sym, func_ty, false);
}

static void install_builtins(struct Parser *p)
{
	install_builtin(p, "print_int", ty_void, ty_int);
	install_builtin(p, "print_float", ty_void, ty_float);
	install_builtin(p, "print_double", ty_void, ty_double);
	install_builtin(p, "print_bool", ty_void, ty_bool);

	install_builtin(p, "get_int", ty_int, NULL);
	install_builtin(p, "get_float", ty_float, NULL);
	install_builtin(p, "get_double", ty_double, NULL);
}

NodeVec parser_parse(struct Parser *p)
{
	NodeVec globals;
	massert(vec_init(globals, p->alc, 16), "OOM globals");

	sema_scope_enter(&p->sema);

	install_builtins(p);

	while (!match(p, TokenKind_EOF)) {
		struct Node *n = parse_top_level(p);

		if (n) {
			vec_push(globals, n);
		} else {
			if (!p->ctx->had_error && !p->panic_mode) {
				parser_error(p,
					     "Unexpected token at top level");
			}

			if (p->panic_mode) {
				synchronize(p);
			}

			struct Type *ty = token_to_type(p->curr.kind);
			bool is_decl_start = (ty != NULL) ||
					     (p->curr.kind == TokenKind_CONST);

			if (!is_decl_start && p->curr.kind != TokenKind_EOF) {
				advance(p);
			}
		}
	}

	sema_scope_leave(&p->sema);
	return globals;
}
