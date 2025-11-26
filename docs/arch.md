       +----------------------+
       |      OS Kernel       |
       +----------------------+
                  ^
                  | malloc/free
       +----------------------+
       |   System Allocator   | <--- std/allocers/system.h
       +----------------------+
                  ^
                  | request chunks (4KB/1MB...)
       +----------------------+
       |    Bump Allocator    | <--- std/allocers/bump.h (The Arena)
       | (Manages memory pool)|
       +----------+-----------+
                  | provides `allocer_t` interface
                  |
+-----------------v------------------+
|            Context (God)           |
|------------------------------------|
| - alc (interface to Bump)          | <--- 所有组件都用这个
| - mgr (Source Code)                |
| - itn (Symbols)                    |
| - kw_map (Keywords)                |
| - had_error                        |
+------------------------------------+
        ^               ^
        | uses          | uses
+-------+------+  +-----+------+
|    Lexer     |  |   Sema     |
+--------------+  +------------+
        ^               ^
        | streams       | validates
        |               |
+-------+---------------v------+
|            Parser            |
| (Builds AST using ctx->alc)  |
+------------------------------+
