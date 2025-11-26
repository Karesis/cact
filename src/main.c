#include <core/msg.h>
#include <core/macros.h>
#include <std/allocers/system.h>
#include <std/allocers/bump.h>
#include <std/fs.h>
#include <std/strings/string.h>
#include <context.h>
#include <lexer.h>
#include <parser.h>
#include <ast.h>

/*
 * ==========================================================================
 * Constants & Help
 * ==========================================================================
 */

#define CACTC_VERSION "0.1.0"

static const char *USAGE_INFO =
	"cactc - The CACT Compiler\n"
	"\n"
	"Usage:\n"
	"    cactc [options] <file>\n"
	"\n"
	"Options:\n"
	"    -o <file>      Output file (default: a.out)\n"
	"    -h, --help     Show this help message\n"
	"\n";

/*
 * ==========================================================================
 * Compiler Pipeline
 * ==========================================================================
 */

static bool run_compile(struct Context *ctx, const char *filepath)
{
	string_t content;
	if (!string_init(&content, ctx->alc, 0)) {
		log_error("OOM reading file");
		return false;
	}

	if (!file_read_to_string(filepath, &content)) {
		log_error("Could not read file '%s'", filepath);
		return false;
	}

	usize file_id = srcmanager_add(&ctx->mgr, str_from_cstr(filepath),
				       string_as_str(&content));

	struct Lexer lex;
	lexer_init(&lex, ctx, file_id);

	struct Parser p;
	parser_init(&p, ctx, &lex);

	printf("[INFO] Compiling '%s'...\n", filepath);
	NodeVec globals = parser_parse(&p);

	if (ctx->had_error) {
		return false;
	}

	printf("[INFO] Parsed %zu top-level nodes.\n", vec_len(globals));
	vec_foreach(node_ptr, globals)
	{
		struct Node *n = *node_ptr;
		printf("  - Node Kind: %d\n", n->kind);
	}

	return true;
}

/*
 * ==========================================================================
 * Entry Point
 * ==========================================================================
 */

int main(int argc, char **argv)
{
	allocer_t sys = allocer_system();

	if (argc < 2) {
		printf("%s", USAGE_INFO);
		return 1;
	}

	const char *input_file = NULL;
	for (int i = 1; i < argc; ++i) {
		if (strcmp(argv[i], "-h") == 0 ||
		    strcmp(argv[i], "--help") == 0) {
			printf("%s", USAGE_INFO);
			return 0;
		}
		if (argv[i][0] != '-') {
			input_file = argv[i];
		}
	}

	if (!input_file) {
		fprintf(stderr, "Error: No input file specified.\n");
		return 1;
	}

	bump_t arena;
	bump_init(&arena, sys, 8);
	allocer_t arena_alc = bump_allocer(&arena);

	struct Context ctx;
	context_init(&ctx, arena_alc);

	bool success = run_compile(&ctx, input_file);

	context_deinit(&ctx);
	bump_deinit(&arena);

	return success ? 0 : 1;
}
