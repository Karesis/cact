#include <std/test.h>
#include <std/allocers/system.h>
#include <lexer.h>
#include <context.h>
#include <token.h>

/*
 * ==========================================================================
 * Test Fixture (Setup & Teardown)
 * ==========================================================================
 */

struct TestEnv {
    allocer_t alc;
    srcmanager_t *mgr;
    struct Context ctx;
    struct Lexer lex;
};

static void setup(struct TestEnv *env, const char *source_code) {
    env->alc = allocer_system();
    env->mgr = srcmanager_new(env->alc);
    context_init(&env->ctx, env->alc);
    
    usize file_id = srcmanager_add(env->mgr, str("test.cact"), str_from_cstr(source_code));
    lexer_init(&env->lex, env->mgr, &env->ctx, file_id);
}

static void teardown(struct TestEnv *env) {
    context_deinit(&env->ctx);
    srcmanager_drop(env->mgr);
}

// [FIX] 1. Cast to (int) to silence -Wsign-compare
#define EXPECT_TOK(env, expected_kind) \
    do { \
        struct Token t = lexer_next(&(env)->lex); \
        if (t.kind != (expected_kind)) { \
            fprintf(stderr, "    Expected %s, got %s\n", \
                tokenkind_to_cstr(expected_kind), \
                tokenkind_to_cstr(t.kind)); \
            expect_eq((int)(expected_kind), (int)t.kind); \
        } \
    } while(0)

// [FIX] 2. Change function to MACRO so 'return false' exits the TEST function
#define check_ident(env, expected_text) \
    do { \
        struct Token t = lexer_next(&(env)->lex); \
        expect_eq((int)TokenKind_IDENT, (int)t.kind); \
        str_t s = intern_resolve(&(env)->ctx.itn, t.value.name); \
        if (!str_eq_cstr(s, expected_text)) { \
            fprintf(stderr, "    Ident mismatch: expected '%s', got '%.*s'\n", \
                    expected_text, fmt_str(s)); \
            return false; \
        } \
    } while(0)

/*
 * ==========================================================================
 * Test Cases
 * ==========================================================================
 */

TEST(keywords_and_bools) {
    struct TestEnv env;
    setup(&env, "int void if else return true false while");

    EXPECT_TOK(&env, TokenKind_INT);
    EXPECT_TOK(&env, TokenKind_VOID);
    EXPECT_TOK(&env, TokenKind_IF);
    EXPECT_TOK(&env, TokenKind_ELSE);
    EXPECT_TOK(&env, TokenKind_RETURN);
    EXPECT_TOK(&env, TokenKind_TRUE);
    EXPECT_TOK(&env, TokenKind_FALSE);
    EXPECT_TOK(&env, TokenKind_WHILE);
    EXPECT_TOK(&env, TokenKind_EOF);

    teardown(&env);
    return true;
}

TEST(identifiers) {
    struct TestEnv env;
    setup(&env, "main foo _bar var123");

    check_ident(&env, "main");
    check_ident(&env, "foo");
    check_ident(&env, "_bar");
    check_ident(&env, "var123");
    EXPECT_TOK(&env, TokenKind_EOF);

    teardown(&env);
    return true;
}

TEST(integers) {
    struct TestEnv env;
    setup(&env, "123 0xFF 0 010"); 

    struct Token t;

    // 123
    t = lexer_next(&env.lex);
    expect_eq((int)TokenKind_LIT_INT, (int)t.kind); // Cast here
    expect_eq(123, t.value.as_int);

    // 0xFF (255)
    t = lexer_next(&env.lex);
    expect_eq((int)TokenKind_LIT_INT, (int)t.kind);
    expect_eq(255, t.value.as_int);

    // 0
    t = lexer_next(&env.lex);
    expect_eq((int)TokenKind_LIT_INT, (int)t.kind);
    expect_eq(0, t.value.as_int);

    // 010 (Octal 8)
    t = lexer_next(&env.lex);
    expect_eq((int)TokenKind_LIT_INT, (int)t.kind);
    expect_eq(8, t.value.as_int);

    teardown(&env);
    return true;
}

TEST(floats_cact_spec) {
    struct TestEnv env;
    setup(&env, "3.14 3.14f 1e10 .5 5.");

    struct Token t;

    // 3.14 -> Double
    t = lexer_next(&env.lex);
    expect_eq((int)TokenKind_LIT_DOUBLE, (int)t.kind);
    expect(t.value.as_double > 3.13 && t.value.as_double < 3.15);

    // 3.14f -> Float
    t = lexer_next(&env.lex);
    expect_eq((int)TokenKind_LIT_FLOAT, (int)t.kind);
    expect(t.value.as_float > 3.13f && t.value.as_float < 3.15f);

    // 1e10 -> Double
    t = lexer_next(&env.lex);
    expect_eq((int)TokenKind_LIT_DOUBLE, (int)t.kind);
    expect(t.value.as_double == 1e10);

    // .5 -> Double
    t = lexer_next(&env.lex);
    expect_eq((int)TokenKind_LIT_DOUBLE, (int)t.kind);
    expect(t.value.as_double == 0.5);

    // 5. -> Double
    t = lexer_next(&env.lex);
    expect_eq((int)TokenKind_LIT_DOUBLE, (int)t.kind);
    expect(t.value.as_double == 5.0);

    teardown(&env);
    return true;
}

TEST(invalid_numbers) {
    struct TestEnv env;
    setup(&env, "3f");

    struct Token t = lexer_next(&env.lex);
    expect_eq((int)TokenKind_ERROR, (int)t.kind);

    teardown(&env);
    return true;
}

TEST(punctuations) {
    struct TestEnv env;
    setup(&env, "+ - * / % < <= > >= == != && || ! =");

    EXPECT_TOK(&env, TokenKind_PLUS);
    EXPECT_TOK(&env, TokenKind_MINUS);
    EXPECT_TOK(&env, TokenKind_STAR);
    EXPECT_TOK(&env, TokenKind_SLASH);
    EXPECT_TOK(&env, TokenKind_PERCENT);
    EXPECT_TOK(&env, TokenKind_LT);
    EXPECT_TOK(&env, TokenKind_LE);
    EXPECT_TOK(&env, TokenKind_GT);
    EXPECT_TOK(&env, TokenKind_GE);
    EXPECT_TOK(&env, TokenKind_EQ);
    EXPECT_TOK(&env, TokenKind_NEQ);
    EXPECT_TOK(&env, TokenKind_LOG_AND);
    EXPECT_TOK(&env, TokenKind_LOG_OR);
    EXPECT_TOK(&env, TokenKind_LOG_NOT);
    EXPECT_TOK(&env, TokenKind_ASSIGN);

    teardown(&env);
    return true;
}

TEST(comments) {
    struct TestEnv env;
    setup(&env, 
        "int // this is a comment\n"
        "/* this is a \n block comment */ main"
    );

    EXPECT_TOK(&env, TokenKind_INT);
    check_ident(&env, "main");
    EXPECT_TOK(&env, TokenKind_EOF);

    teardown(&env);
    return true;
}

TEST(complex_statement) {
    struct TestEnv env;
    setup(&env, "if (a <= 10) { return a + 1; }");

    EXPECT_TOK(&env, TokenKind_IF);
    EXPECT_TOK(&env, TokenKind_L_PAREN);
    check_ident(&env, "a");
    EXPECT_TOK(&env, TokenKind_LE);
    
    struct Token t = lexer_next(&env.lex);
    expect_eq((int)TokenKind_LIT_INT, (int)t.kind);
    expect_eq(10, t.value.as_int);

    EXPECT_TOK(&env, TokenKind_R_PAREN);
    EXPECT_TOK(&env, TokenKind_L_BRACE);
    EXPECT_TOK(&env, TokenKind_RETURN);
    check_ident(&env, "a");
    EXPECT_TOK(&env, TokenKind_PLUS);
    
    t = lexer_next(&env.lex);
    expect_eq((int)TokenKind_LIT_INT, (int)t.kind);
    expect_eq(1, t.value.as_int);

    EXPECT_TOK(&env, TokenKind_SEMICOLON);
    EXPECT_TOK(&env, TokenKind_R_BRACE);
    EXPECT_TOK(&env, TokenKind_EOF);

    teardown(&env);
    return true;
}

int main(void) {
    RUN(keywords_and_bools);
    RUN(identifiers);
    RUN(integers);
    RUN(floats_cact_spec);
    RUN(invalid_numbers);
    RUN(punctuations);
    RUN(comments);
    RUN(complex_statement);
    
    SUMMARY();
}
