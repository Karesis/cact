cact-lexer: src/*.c
	clang -o build/cactc src/*.c -Wall -Wextra -g -I src/include