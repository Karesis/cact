#include <context.h>
#include <token.h>
#include <core/type.h>
#include <core/msg.h>
#include <std/strings/intern.h>
#include <std/map.h>

/// fast for just u32
static u64 _sym_hash(const void *key) {
    const symbol_t *s = (const symbol_t *)key;
    return (u64)s->id * 2654435761u; 
}

static bool _sym_eq(const void *lhs, const void *rhs) {
    return ((const symbol_t *)lhs)->id == ((const symbol_t *)rhs)->id;
}

const map_ops_t MAP_OPS_SYMBOL = {
    .hash = _sym_hash,
    .equals = _sym_eq,
};

void context_init(struct Context *ctx, allocer_t alc) {
    ctx->alc = alc;
    massert(intern_init(&ctx->itn, alc), "Context init failed");
    
    /// init map
    massert(map_init(ctx->kw_map, alc, MAP_OPS_SYMBOL), "Context::kw_map init failed");

    symbol_t sym;
    TokenKind kind;

    #define KW(ID, STR) \
        do { \
            /* 1. get symbol */ \
            sym = intern(&ctx->itn, str_from_cstr(STR)); \
            /* 2. get the Kind */ \
            kind = TokenKind_##ID; \
            /* 3. put into map */ \
            map_put(ctx->kw_map, sym, kind); \
        } while(0);

    #include <token.def>
}

void context_deinit(struct Context *ctx) {
    map_deinit(ctx->kw_map);
    intern_deinit(&ctx->itn);
}
