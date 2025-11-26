# cactc

**cactc** is a compiler for the **CACT** language (a strict subset of C), implemented in **C23**.

This project implements the compiler frontend, including lexical analysis, syntactic analysis, and semantic analysis (type checking). It is built upon a custom C foundation library, [**fluf**](https://github.com/Karesis/fluf), which provides essential data structures and memory management facilities.

## Building

**Prerequisites:**

  * A C23-compliant compiler (Clang is recommended).
  * GNU Make.
  * Python 3 (for integration test scripts).

**Build:**

```bash
# Compile the project and the fluf library
make

# Clean build artifacts
make clean
```

The binary will be generated at `build/bin/cactc`.

## Usage

To compile a CACT source file:

```bash
./build/bin/cactc path/to/source.cact
```

If successful, it prints the AST summary to stdout. If there are errors, it prints diagnostic messages with source code highlighting to stderr.

## Testing

This project uses a two-tier testing strategy to ensure correctness.

### 1\. Integration Tests (Official Samples)

The `tests/samples/` directory contains the official test cases. The python script `scripts/test_samples.py` runs the compiler against these files and verifies the exit code (0 for valid files, non-zero for invalid ones).

```bash
# Run the integration test suite
make test_samples
```

### 2\. Unit Tests (Internal Logic)

Unit tests for the Lexer and Parser are written in C. These tests verify internal APIs and check for memory leaks (using AddressSanitizer if enabled).

```bash
# Compile and run unit tests
make test
```

## Implementation Details

### Memory Management: Arena Allocation

Instead of managing the lifecycle of individual AST nodes with `malloc`/`free`, this project uses a **Bump Allocator (Arena)** provided by `fluf`.

  * **Allocation**: Extremely fast O(1) allocation for AST nodes, types, and symbols.
  * **Deallocation**: All memory is released instantly when the `Context` is destroyed at the end of compilation. This approach eliminates use-after-free bugs and memory leaks by design.

### Architecture

  * **Context**: A central structure that manages resources (memory arena, source files, interned strings) and error reporting.
  * **Lexer**: Handles tokenization. It integrates with the `Context` to report errors (e.g., invalid characters) with precise line/column numbers.
  * **Parser**: A recursive descent parser.
      * Implements **Panic Mode Recovery** to skip invalid tokens and continue parsing after an error, allowing multiple errors to be reported in a single run.
      * Handles complex grammar rules like operator precedence and identifying declarations vs. statements.
  * **Sema (Semantic Analysis)**: Performed on-the-fly during parsing.
      * **Scope Management**: Handles nested scopes and variable shadowing.
      * **Type Checking**: Enforces CACT's strict type rules (no implicit casting, strict initialization checks).

## Project Structure

```text
.
├── src/
│   ├── main.c          # Entry point: driver logic
│   ├── context.c       # Global resource management
│   ├── lexer.c         # Tokenization logic
│   ├── parser.c        # Parsing & Error recovery logic
│   ├── sema.c          # Semantic analysis & Symbol table
│   └── type.c          # Type system implementation
├── include/            # Public headers
├── vendor/fluf/        # Custom C foundation lib (Vec, Map, Allocers)
└── tests/samples/      # Official CACT test cases
```

## Future Work

  * **Codegen**: Integration with the **Calico** backend (included in `vendor/calico`) to generate IR and RISC-V assembly.
  * **Constant Folding**: Evaluate constant expressions at compile time.

## License

This project is licensed under the **Apache-2.0 License**. See the [LICENSE](./LICENSE) file for details.
