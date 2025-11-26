#pragma once
#include <type.h>
#include <ast.h>
#include <std/map.h>
#include <std/strings/intern.h>

struct SemaSymbol;

defMap(symbol_t, struct SemaSymbol *, SymbolMap);

struct SemaSymbol {
	symbol_t name;
	struct Type *ty;
	bool is_const;
	bool is_global;
	int stack_offset;
};

struct Scope {
	struct Scope *parent;

	SymbolMap symbols;
};

struct Sema {
	struct Context *ctx;
	struct Scope *curr_scope;
	struct Type *curr_func_ret;
};

void sema_init(struct Sema *s, struct Context *ctx);

void sema_scope_enter(struct Sema *s);
void sema_scope_leave(struct Sema *s);

struct SemaSymbol *sema_define_var(struct Sema *s, symbol_t name,
				   struct Type *ty, bool is_const);

struct SemaSymbol *sema_lookup(struct Sema *s, symbol_t name);

void sema_analyze_binary(struct Sema *s, struct NodeBinary *node);

void sema_analyze_assign(struct Sema *s, struct NodeBinary *node);

void sema_analyze_return(struct Sema *s, struct NodeUnary *node);
