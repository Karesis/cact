cactc: src/*.c
	clang -o build/cactc src/*.c -Wall -Wextra -g -I src/include

.PHONY: test

test: cactc tests/test/test_syntax.sh
	./tests/test/test_syntax.sh