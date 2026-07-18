---
name: C Codegen (2c)
description: Comprehensive guide to the C translation backend (2c) — how Chemical AST is translated to C code, key patterns, gotchas, and optimization strategies.
---

# C Codegen (2c Backend)

The "2c" backend translates the Chemical AST into C code. This is used by `TCCCompiler` and is also the foundation for the TinyCC JIT compilation path. The 2c backend produces readable, portable C that can be compiled with any C compiler.

## Architecture

### Pipeline

```
Type-checked AST → ToCAstVisitor (structure/declaration level) → BufferedWriter → C source file
```

### Key Files

| File | Purpose |
|------|---------|
| `preprocess/2c/2cASTVisitor.h` | Main C codegen visitor declaration — `ToCAstVisitor` class |
| `preprocess/2c/2cASTVisitor.cpp` | Main implementation — translates all AST nodes to C |
| `preprocess/2c/SubVisitor.h` | Base classes and utilities for C translation visitors |
| `preprocess/2c/CTopLevelDeclVisitor.h` | Top-level declaration visitor (functions, structs, globals) |
| `preprocess/2c/CDestructionVisitor.h` | Visitor for destructor code generation |
| `preprocess/2c/2cBackendContext.h` | Backend context — manages output, naming, and state |
| `preprocess/2c/BufferedWriter.h` | Efficient buffered output writer |

## Key Translation Patterns

### 1. Struct Translation

Chemical structs become C structs:

```chemical
struct Point {
    var x : int
    var y : int
}
```

```c
typedef struct Point {
    int32_t x;
    int32_t y;
} Point;
```

**Struct methods** become C functions with the `self` pointer as the first parameter:

```chemical
func p.sum(&self) : int {
    return self.x + self.y;
}
```

```c
int32_t Point_sum(Point* self) {
    return self->x + self->y;
}
```

### 2. Function Translation

Chemical functions become C functions with name mangling applied:

| Chemical Function | C Name |
|-------------------|--------|
| `func foo()` | `foo` (no mangling for plain names) |
| `func Namespace::foo()` | `Namespace_foo` (scope prefix) |
| `generic foo<T>()` | `foo__cgs__N` or `foo__cfg__N` (generic suffix) |
| `@extern func printf()` | `printf` (no mangling at all) |
| `@no_mangle func bar()` | `bar` (no mangling) |

### 3. Struct-Returning Functions

Functions returning structs use the sret (struct return) pattern via compound expressions:

```c
// Chemical:
func create_point(x : int, y : int) : Point { ... }

// C — hidden sret pointer:
void create_point(Point* __result, int32_t x, int32_t y);

// Using the result in an expression:
(*({ struct Point __tmp; create_point(&__tmp, 1, 2); &__tmp; }))
```

**Warning**: When the result is discarded (expression-statement context), `gcc -Wall` emits `-Wunused-value` on the `*` dereference. This is a pre-existing pattern throughout generated C.

### 4. Method Chains and `&self`

In a method chain like `a.b().c()`:

1. `b()` is called on `a` — generates receiver variable
2. `c()` is called on the result of `b()` — if `c()` has no `&self` parameter, **do not** create a receiver variable

**Before the fix**: `ToCAstVisitor` created an unused `struct Type* __chx__recv__N` variable, producing "unused variable" warnings.

**After the fix**: Check if the called function has a `self` parameter before creating the receiver variable.

### 5. Pointer and Reference Translation

| Chemical | C |
|----------|---|
| `*int` | `int32_t*` |
| `*mut int` | `int32_t*` (no const) |
| `*char` | `const char*` |
| `&int` (ref) | `int32_t*` (same as pointer) |
| `[10]int` | `int32_t[10]` |

### 6. Variant Translation

Variants become a struct with a discriminator and union:

```chemical
variant Option<int> {
    Some(value : int)
    None()
}
```

```c
typedef struct Option_i32 {
    uint8_t __disc;  // discriminator
    union {
        struct { int32_t value; } Some;
        struct { } None;
    } __data;
} Option_i32;
```

### 7. Name Generation

The `BufferedWriter` and `CTopLevelDeclVisitor` handle:

- **Unique names**: Generated names use `__chx__` prefix to avoid collisions
- **Temp variables**: `__chx__temp__N` for intermediate values
- **Result pointers**: `__chx__result` for sret pointers
- **Receiver variables**: `__chx__recv__N` for method chain receivers
- **Loop labels**: `__chx__for__N`, `__chx__while__N` for break/continue targets

### 8. Control Flow Translation

| Construct | C Pattern |
|-----------|-----------|
| `if/else` | `if(...) { ... } else { ... }` |
| `while` | `while(...) { ... }` |
| `do-while` | `do { ... } while(...)` |
| `for` | `for(...; ...; ...) { ... }` |
| `switch` | `switch(...) { case ...: ... }` |
| `break` | `goto __chx__break__N;` (for nested loops) |
| `continue` | `goto __chx__continue__N;` |
| Defer | N/A (no defer in Chemical) |

**Break/continue implementation**: Because C's `break`/`continue` only works for the innermost loop, Chemical's break/continue are implemented as `goto` to unique labels. Each loop gets a `__chx__break__N` and `__chx__continue__N` label.

## Codegen State Management

### ToCBackendContext

```cpp
class ToCBackendContext {
    BufferedWriter writer;              // Output buffer
    std::vector<ASTNode*> currentScope; // Scope stack
    bool hasReturnType;                  // Current function has non-void return
    // ... encoding state, name counters, etc.
};
```

### Scoping

The backend maintains scope information for:
1. **Variable names** — ensuring unique C identifiers
2. **Label names** — for goto-based break/continue
3. **Struct names** — for nested/anonymous structs
4. **Function names** — name mangling

## Gotchas

### 1. Compound Expression Wrapping

Functions returning structs use the `(*({ ... }))` pattern. When the result is discarded:

```c
// Generated when discarding a struct return:
(*({ struct Type __tmp; func(&__tmp, args...); &__tmp; }));
// gcc -Wall: warning: unused value
```

**Workaround**: Wrap in `(void)` cast or use a different pattern for discarded results.

### 2. Bool-to-Int Promotion

Chemical `bool` is `i1` in LLVM but `int` in C. The codegen must:

```c
// Chemical bool:
bool flag = true;

// C translation:
int32_t flag = 1;  // bool promoted to int32_t
```

### 3. Zero-Length Arrays

Chemical allows zero-length arrays. In C, this requires flexible array member or zero-length array extension:

```c
// Chemical: var arr : [0]int
// C (GCC extension): int32_t arr[0];
```

### 4. Variadic Function Declarations

Chemical does NOT support variadic function declarations for user-defined functions. Only `@extern` C functions use variadic args:

```c
// Chemical:
@extern func printf(format : *char, ...) : int

// C:
extern int printf(const char* format, ...);
```

### 5. Anonymous Scopes

Chemical supports bare `{ }` as a scope expression. These must generate a compound statement:

```c
// Chemical:
var x = {
    var tmp = 42;
    tmp
};

// C:
int32_t x = ({ int32_t tmp = 42; tmp; });
```

### 6. Hexadecimal Float Literals

Chemical supports hex float literals. C99+ supports `0x` prefix for floats:

```c
// Chemical: 0x1.921fb6p+1f
// C: 0x1.921fb6p+1f
```

## Performance Optimization

### Output Efficiency

1. **BufferedWriter**: Uses a large buffer (8KB+) to minimize write calls
2. **Minimal includes**: Only includes headers that are actually needed
3. **No dead code**: The 2c backend skips dead branches detected during comptime evaluation
4. **Type deduplication**: Common types are emitted once, with typedefs

### Parallelization

Currently, the 2c backend is mostly serial within a job. Future strategies:

1. **Per-function codegen**: Each function's C codegen is independent
2. **Per-module codegen**: Modules can be translated to C concurrently
3. **String concatenation optimization**: Use `BufferedWriter` with pre-allocated buffers

## Debugging C Output

### Emitting C

```bash
# With TCCCompiler:
cmake-build-debug/TCCCompiler "lang/compiled/temp.ch" -o "lang/compiled/temp.c" -v -bm-modules
```

The `.c` extension tells the compiler to emit C source instead of an executable.

### Common C Compiler Warnings from Generated Code

| Warning | Cause | Fix |
|---------|-------|-----|
| `unused variable` | Unused receiver for method chain without `&self` | Check receiver requirement in `ToCAstVisitor` |
| `unused value` | Discarded struct-returning expression | Use different pattern for discarded results |
| `incompatible pointer types` | Wrong type in a cast | Check type conversion in codegen |
| `implicit declaration` | Missing forward declaration | Declare before use or use extern |

## Extending the 2c Backend

When adding a new AST node to the 2c backend:

1. **Add a visitor method** in `2cASTVisitor.cpp`
2. **Declare the visitor** in `2cASTVisitor.h`
3. **Add any new helper** in `CDestructionVisitor.h` or `CTopLevelDeclVisitor.h` if needed
4. **Register in `SubVisitor.h`** — add the node type to the dispatch table

### Template for a New Visitor

```cpp
// In 2cASTVisitor.cpp:
void ToCAstVisitor::VisitNewNode(NewNode* node) {
    // 1. Handle any dependencies
    VisitNode(node->dependency);
    
    // 2. Generate C code
    writer.write("/* chemical new node */\n");
    
    // 3. Visit children
    for(auto child : node->children) {
        VisitNode(child);
    }
}
```
