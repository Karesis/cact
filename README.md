# cactc - A Compiler for the CACT Language

This project is a compiler for the CACT language, a small, statically-typed language similar to a subset of C. The compiler is written entirely in C and implements the front-end stages of compilation, including lexical analysis, parsing, and semantic analysis (type checking).

The final output of this stage is an Abstract Syntax Tree (AST) annotated with type information, which serves as a foundation for future code generation phases.

## Features

-   **Lexical Analysis**: Scans source code and converts it into a stream of tokens. Supports integers (decimal, hex, octal), floating-point numbers (`float`, `double`), keywords, identifiers, and both line (`//`) and block (`/* ... */`) comments.
-   **Syntactic Analysis**: Implements a recursive descent parser that builds an Abstract Syntax Tree (AST) representing the program's structure. It correctly handles operator precedence and supports all standard language constructs.
-   **Semantic Analysis**: Traverses the AST to perform type checking. It validates variable declarations, assignments, function calls, and expression types, ensuring the program is semantically correct.
-   **Scope Management**: Maintains a stack of scopes to correctly handle global variables, local variables, and function parameters.
-   **Error Reporting**: Provides clear error messages with filename, line number, column number, and the problematic line of code to facilitate debugging.

## Directory Structure

```
.
├── build/                  # Stores build artifacts (object files and the final executable)
│   └── cactc               # The compiled compiler executable
├── docs/                   # Language specification and documentation
│   ├── cact.ebnf
│   └── CACT语言规范2023版.pdf
├── Makefile                # The build script to compile the project
├── src/                    # All source code files
│   ├── include/            # Header files for all modules
│   ├── ast.c               # AST node constructors and helpers
│   ├── error.c             # Error reporting utilities
│   ├── hash.c              # A simple hash map implementation for symbol tables
│   ├── lexer.c             # The lexical analyzer (Tokenizer)
│   ├── main.c              # Main entry point and compilation driver
│   ├── parser.c            # The recursive descent parser
│   ├── str.c               # A simple string utility library
│   └── type.c              # Semantic analyzer and type system management
└── tests/                  # Test suite
    └── lex_and_syntax/     # Tests for lexical and syntax analysis
        ├── samples/        # A collection of .cact files for testing
        └── test_syntax.sh  # The main test script
```

## Code Modules Overview

The compiler is designed with a modular architecture, where each file has a distinct responsibility:

-   `main.c`: The program's entry point. It handles command-line arguments and orchestrates the calls to the lexer, parser, and other components in the correct sequence.
-   `lexer.c`: The "Tokenizer". It reads the source code as a stream of characters and outputs a linked list of tokens.
-   `parser.c`: The "Parser". It consumes tokens from the lexer and builds an Abstract Syntax Tree (AST). It is also responsible for managing scopes and the symbol table (using the hash map).
-   `ast.c`: Defines the AST node structures and provides helper functions to create new nodes.
-   `type.c`: The "Semantic Analyzer". Its main function, `add_type`, traverses the completed AST to assign a type to each node, effectively performing type checking.
-   `error.c`: Provides functions for formatted, user-friendly error reporting.
-   `str.c`: A small utility module for handling strings, primarily for reading file content.
-   `hash.c`: A generic hash map implementation used by the parser to manage symbol tables for variables and functions.

## How to Build and Use

### Prerequisites

-   A C compiler (e.g., `clang` or `gcc`)
-   `make`

### Building the Compiler

To build the compiler, simply run `make` from the project's root directory:

```bash
make
```

This will compile all source files and place the final executable at `build/cactc`.

To clean up all generated build files, run:

```bash
make clean
```

### Usage

You can run the compiler on any `.cact` source file. The compiler will perform analysis and print a debug representation of the program's structure (functions, variables, and AST).

```bash
./build/cactc <path/to/sourcefile.cact>
```

## How to Run Tests

The project includes a test suite to verify the correctness of the lexer and parser. The test script automatically runs the compiler against a set of sample files.

To run the tests, execute:

```bash
make test
```

The script will report the status of each test case and provide a final summary.

### A Note on Test Results

You may notice one test, `15_true_syntax_false_semantic.cact`, is marked as `[FAILED]`. **This is the expected and correct behavior.**

This particular test case is designed to be **syntactically valid** but **semantically invalid**. The test script, which was designed for a pure parser, expects the compiler to exit with a status code of `0` (success) for any syntactically correct file.

However, since this compiler has a fully functional **semantic analysis** phase (`type.c`), it correctly identifies the semantic error in the file and exits with a non-zero status code to report the failure. Therefore, the "failure" in the test suite actually confirms that the semantic analysis is working correctly.