cact-lexer: src/*.c
	clang -o cact-lexer src/*.c -Wall -Wextra -g -I src/include