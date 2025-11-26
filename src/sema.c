#include <sema.h>
#include <context.h>
#include <std/map.h>
#include <core/msg.h>

static u64 _sym_hash(const void *key)
{
	const symbol_t *s = (const symbol_t *)key;
	return (u64)s->id * 2654435761u;
}
static bool _sym_eq(const void *lhs, const void *rhs)
{
	return ((const symbol_t *)lhs)->id == ((const symbol_t *)rhs)->id;
}
static const map_ops_t SEMA_MAP_OPS = { .hash = _sym_hash, .equals = _sym_eq };

void sema_init(struct Sema *s, struct Context *ctx)
{
	s->ctx = ctx;
	s->curr_scope = NULL;
	s->curr_func_ret = NULL;
}

void sema_scope_enter(struct Sema *s)
{
	struct Scope *sc = alloc_type(s->ctx->alc, struct Scope);
	map_init(sc->symbols, s->ctx->alc, SEMA_MAP_OPS);
	sc->parent = s->curr_scope;
	s->curr_scope = sc;
}

void sema_scope_leave(struct Sema *s)
{
	if (s->curr_scope) {
		map_deinit(s->curr_scope->symbols);

		struct Scope *parent = s->curr_scope->parent;
		free_type(s->ctx->alc, s->curr_scope);

		s->curr_scope = parent;
	}
}

struct SemaSymbol *sema_define_var(struct Sema *s, symbol_t name,
				   struct Type *ty, bool is_const)
{
	if (!s->curr_scope)
		return NULL;

	if (map_get(s->curr_scope->symbols, name)) {
		ctx_error(s->ctx, NULL,
			  "Redefinition of symbol in the same scope");
		return NULL;
	}

	struct SemaSymbol *sym = alloc_type(s->ctx->alc, struct SemaSymbol);
	sym->name = name;
	sym->ty = ty;
	sym->is_const = is_const;
	sym->is_global = (s->curr_scope->parent == NULL);
	sym->stack_offset = 0;

	map_put(s->curr_scope->symbols, name, sym);
	return sym;
}

struct SemaSymbol *sema_lookup(struct Sema *s, symbol_t name)
{
	for (struct Scope *sc = s->curr_scope; sc; sc = sc->parent) {
		struct SemaSymbol **found = map_get(sc->symbols, name);
		if (found)
			return *found;
	}
	return NULL;
}

void sema_analyze_binary(struct Sema *s, struct NodeBinary *node)
{
	struct Type *lhs = node->lhs->ty;
	struct Type *rhs = node->rhs->ty;

	if (!type_eq(lhs, rhs)) {
		ctx_error(s->ctx, node->base.tok,
			  "Type mismatch in binary expression");
		node->base.ty = ty_void;
		return;
	}

	switch (node->base.kind) {
	case ND_ADD:
	case ND_SUB:
	case ND_MUL:
	case ND_DIV:
		if (!type_is_arithmetic(lhs)) {
			ctx_error(
				s->ctx, node->base.tok,
				"Arithmetic operator requires numeric operands");
		}
		node->base.ty = lhs;
		break;
	case ND_MOD:

		if (lhs != ty_int) {
			ctx_error(s->ctx, node->base.tok,
				  "Modulo operator requires integer operands");
		}
		node->base.ty = ty_int;
		break;
	case ND_EQ:
	case ND_NE:
	case ND_LT:
	case ND_LE:
	case ND_GT:
	case ND_GE:
		node->base.ty = ty_bool;
		break;
	case ND_LOG_AND:
	case ND_LOG_OR:
		if (lhs != ty_bool) {
			ctx_error(s->ctx, node->base.tok,
				  "Logical operator requires boolean operands");
		}
		node->base.ty = ty_bool;
		break;
	default:
		break;
	}
}

void sema_analyze_assign(struct Sema *s, struct NodeBinary *node)
{
	if (node->lhs->kind == ND_VAR) {
		struct NodeVar *v = (struct NodeVar *)node->lhs;
		if (v->var && v->var->is_const) {
			ctx_error(s->ctx, node->base.tok,
				  "Cannot assign to const variable");
		}
	}

	if (!type_eq(node->lhs->ty, node->rhs->ty)) {
		ctx_error(s->ctx, node->base.tok,
			  "Type mismatch in assignment");
	}

	node->base.ty = node->lhs->ty;
}

void sema_analyze_return(struct Sema *s, struct NodeUnary *node)
{
	struct Type *actual = node->lhs ? node->lhs->ty : ty_void;

	if (s->curr_func_ret == ty_void) {
		if (actual != ty_void) {
			ctx_error(s->ctx, node->base.tok,
				  "Void function should not return a value");
		}
	} else {
		if (actual == ty_void) {
			ctx_error(s->ctx, node->base.tok,
				  "Non-void function must return a value");
		} else if (!type_eq(s->curr_func_ret, actual)) {
			ctx_error(s->ctx, node->base.tok,
				  "Return type mismatch");
		}
	}
	node->base.ty = ty_void;
}
