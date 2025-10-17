cc = clang
cflags = -Wall -Wextra -g
include = -I src/include

srcdir = src
builddir = build
target = $(builddir)/cactc

sources = $(wildcard $(srcdir)/*.c)
objects = $(patsubst $(srcdir)/%.c,$(builddir)/%.o,$(sources))

all: $(target)

# linking
$(target): $(objects)
	@mkdir -p $(builddir)
	$(cc) -o $@ $^ $(cflags) $(include)
	@echo "Compiler '$(target)' built successfully."

# compiling
$(builddir)/%.o: $(srcdir)/%.c
	@mkdir -p $(builddir)
	$(cc) -c -o $@ $< $(cflags) $(include)

test: all
	@echo "Running tests..."
	@./tests/lex_and_syntax/test_syntax.sh

clean:
	@rm -rf $(builddir)
	@echo "Cleaned build artifacts."

.PHONY: all clean test