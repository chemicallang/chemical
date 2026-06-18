# AGENTS.md — Chemical Programming Language

## Setup (after cloning)

```bash
./scripts/setup.sh                 # Downloads libtcc, updates submodules
./scripts/setup.sh --with-llvm     # Also download prebuilt LLVM (v22.1.1)
./scripts/configure.sh             # CMake configure → cmake-build-debug/
./scripts/configure.sh --no-llvm   # BUILD_COMPILER=OFF (TCCCompiler only)
```

- `setup.sh` runs `git submodule update --init --recursive` before downloading libtcc.
- Prebuilt LLVM from `chemicallang/llvm-prebuilt`.
- LSP depends on `lib/lsp-framework` submodule.

## Build

```bash
./scripts/build.sh --tcc           # TCCCompiler (TinyCC backend)
./scripts/build.sh --llvm          # Compiler (LLVM/Clang backend)
./scripts/build.sh --lsp           # ChemicalLsp
./scripts/build.sh --all           # All targets
./scripts/build.sh --llvm -j 16    # Parallel build
```

- Build dir: `cmake-build-debug/` (CI releases use `out/build`).
- `Compiler` + `TCCCompiler` both link dynamically with TinyCC (`libtcc.so`).
- `Compiler` additionally links LLVM + Clang + LLD.
- macOS: no static linking of standard libraries.

## Build TUI

```bash
./scripts/tui.sh                   # Interactive curses menu for all scripts
./scripts/tui.sh --run myconfig    # Load saved config, run all commands headless
```

Configs saved as JSON in `scripts/tui-configs/`. Last config auto-restored.

## Test

```bash
./scripts/test.sh --tcc            # Build TCCCompiler, compile & run tests
./scripts/test.sh --llvm           # Build Compiler (LLVM), compile & run tests
./scripts/test.sh --tcc --libs     # Include library tests (html, css, js, etc.)
./scripts/test.sh --tcc --no-run   # Compile only, don't run
./scripts/test.sh --tcc --no-build # Use existing compiler binary
./scripts/test.sh --tcc --mode debug_complete  # Override compilation mode
./scripts/test.sh --tcc --cache    # Use cached objects (default: --no-cache)
./scripts/test.sh --tcc --cached-plugins  # Skip CBI plugin recompile
./scripts/test.sh --tcc --emit-c   # Emit C translation output
./scripts/test.sh --tcc --use-c    # Translate to C + compile with embedded Clang
./scripts/test.sh --tcc -g         # Debug symbols
./scripts/test.sh --tcc -o my_tests -j 8  # Custom output + parallel jobs
```

Internal defaults: `--no-cache`, `-frecompile-plugins`. Pass `--cache`/`--cached-plugins` to opt out.

CI equivalent (runs from release binary, no `--no-cache`):
```bash
./chemical lang/tests/build.lab -arg-minimal -bm -v --assertions --mode debug_complete
```

Lib tests CI:
```bash
./chemical lang/tests/build.lab -arg-test-libs -bm -v --assertions --mode debug_complete --no-cache
```

## Test structure

- `lang/tests/src/test.ch` — assertion framework (`test(name, lambda)`, `assertEquals`).
- `lang/tests/src/tests.ch` — `run_executable_tests()` lists inline tests.
- **Inline tests**: manually listed in `tests.ch` via `test(name, () => bool)`.
- **`@test` annotations**: auto-discovered by `test_runner(argc, argv)` from `test_env` lib.
- Source dirs: `basic/`, `comptime/`, `core/`, `generic/`, `libs/`, `nodes/`, `stdlib/`.
- Lib tests in `lang/tests/libs/*/src/`.

## Architecture

| Path | Purpose |
|------|---------|
| `ast/` | AST node definitions (values, types, structures, statements) |
| `compiler/` | Compilation pipeline |
| `compiler/backend/LLVM.cpp` | LLVM codegen (primary backend) |
| `compiler/backend/CLANG.cpp` | Clang/LLVM driver integration |
| `compiler/symres/` | Symbol resolution (semantic analysis) |
| `compiler/lab/` | Build system (Lab), `build.lab` execution |
| `compiler/cbi/` | Compiler Binding Interface (C plugins) |
| `compiler/generics/` | Monomorphization/generic instantiation |
| `compiler/typeverify/` | Second type-verification pass |
| `parser/` | Recursive descent parser |
| `lexer/` | Tokenizer |
| `preprocess/2c/` | C translation (2c) visitors and codegen |
| `core/main/CompilerMain.cpp` | CLI entrypoint |
| `server/` | LSP server implementation |
| `lang/libs/` | Standard library + CBI macro plugin libs |
| `lang/libs/compiler/` | Compiler API bindings for macros |
| `lang/tests/` | Test suite |

## Conventions

- Custom strings: `chem::string` (SSO, no `std::string`).
- Name mangling: scoped → `scope_name` prefix, generics → `__cgs__N`/`__cfg__N` suffix.
- Operator overloading: `a + b` → looks for `add(a, b)` method.
- `#macro` and `#universal` are CBI plugins compiled by TinyCC at build time.
- AST uses `ASTAllocator` arena — batch-allocated, no per-node `delete`.
- CLI entry is `LabBuildCompiler` → translates `chemical.mod` → JIT-compiles `build.lab` (TinyCC) → `LabJob` objects → parse → symres → typecheck → codegen → link.

## LLVM Backend Gotchas

### External declarations must not use `dso_local`

When a global variable is declared in another module (`submod_extern_globe_var`), do **not** set `dso_local` on the LLVM global value. The linker will reject `dso_local` + external declarations as mismatched. Relevant: `ast/statements/VarInit.cpp`, `compiler/backend/LLVM.cpp`.

### Struct assignment: temp + destruct + memcpy

The LLVM backend (`StructValue.cpp`) assigns structs via a three-step pattern:
1. **Bitwise copy** source into a stack-allocated temp
2. **Call destructor** on the destination
3. **memcpy** the temp onto the destination

This pattern **breaks self-referencing pointers**. If a struct has a pointer field pointing to one of its own members (e.g., `function`'s `fn_data_ptr`), the bitwise copy produces a dangling pointer — the temp shares the same pointer, but after `memcpy` over the destination, the temp is destroyed. The destination's pointer now points to freed memory.

Fixes considered: `@reflat` annotation to skip destruct+memcpy, `@move` hook for custom move semantics, or forbidding self-referencing pointers inside value types.

### Unitialized variables and PHI nodes

LLVM requires `UndefValue` for uninitialized phis in certain lowering patterns. Check `IRBuilder::CreatePHI` usage in `LLVM.cpp` when adding new PHI-based constructs.

## C Codegen Gotchas

### Struct-returning function expression pattern

Functions returning structs are translated to C using the `(*({ ... }))` compound-expression pattern:
```c
(*({ struct Type __tmp; func(&__tmp, args...); &__tmp; }))
```
This allocates a temp, passes it as a hidden sret pointer, and dereferences the result to produce the struct value.

**Warning:** When the result is discarded (expression-statement context), `gcc -Wall` emits `-Wunused-value` on the `*` dereference. This is a pre-existing pattern throughout generated C.

### Method chains and `&self`

In a method chain like `a.b().c()`, the receiver of `c()` is the result of `b()`. When `c()` has no `&self` parameter (no receiver needed), the C codegen in `preprocess/2c/2cASTVisitor.cpp` must **not** create a receiver pointer variable. Before the fix, it created an unused `struct Type* __chx__recv__N` variable, producing "unused variable" warnings.

## Compiled Packages (`lang/compiled/`)

Each package under `lang/compiled/<name>/` is a standalone Chemical application:
- Entry: `chemical.mod` — declares `application <name>`, `source "src"`, imports.
- Build: `cmake-build-debug/TCCCompiler lang/compiled/<name>/chemical.mod -o <output> --mode debug_quick --no-cache`
- Compiled apps depend on `std`, `page`, `html_cbi`, `css_cbi`, `js_cbi`, `universal_cbi`, `net`, `json`, `fs`, and optionally `mongodb`, `totp`, `accountlib`.

### Universal Component Pipeline

`#universal ComponentName(props) { ... }` components (processed by `universal_cbi`):

1. **Definition** — JSX-based components with `state` for reactivity, event handlers, `.map()` for loops.
2. **SSR (server-side rendering)** — The component's JSX is compiled to C++ that writes HTML into `HtmlPage.pageHtml`. During SSR, `state` initializers run, conditionals evaluate, and static HTML is emitted.
3. **Hydration** — The same JSX compiles to JavaScript that runs in the browser. The SSR HTML is captured and passed to a client-side function that hydrates it in-place.
4. **Mounting** — `#html { <Component prop={value} /> }` generates:
   - Server: calls the component's C++ SSR function → writes HTML to `page`
   - Client: emits `window.$__uni_dispatch('scoped_ComponentName', element, {props})` which calls the JS version to hydrate

### How `#html {}` Blocks Work

The `#html { ... }` macro (processed by `html_cbi`):
- Parses JSX/HTML into an AST
- For native HTML elements (`<div>`, `<span>`), generates `page.append_view(...)` calls
- For universal components (`<QuizTaker>`), generates:
  1. C++ SSR function call (writes HTML to page buffer)
  2. `page.capture_html_delta_to_js(index)` — copies the SSR HTML into the JS bundle
  3. `page.truncate_html(index)` — removes SSR HTML from the page buffer (it's now in JS)
  4. Emits `window.$_uc_h(htmlString, "ComponentName", props)` into the JS bundle
- Prop values (`{expr}`) are C++ expressions evaluated at runtime

### Prop Serialization Gotchas

When passing C++ values as props to universal components via `#html { <Comp prop={value} /> }`:

**Safe types:** `string`, `string_view`, `*char`, `int`, `uint`, `i64`, `bool`, `double`
**Unsafe types:** C++ structs with `vector<>` fields — the serializer produces corrupt output

The JS serializer wraps `string` values in **single quotes** in the generated JavaScript. This means:
- `'` (single quote) in the data MUST be escaped as `\u0027` (valid JSON) — the serializer does NOT escape it
- `\` (backslash) in the data will be interpreted as a JS escape sequence — if your JSON contains `\n`, it becomes a literal newline in the JS string. You must pre-escape `\` as `\\` in your string before passing
- **Workaround:** Post-process string values with `js_string_escape()` that doubles backslashes and escapes single quotes

```chemical
func js_string_escape(view : string_view) : string {
    var out = string()
    for(var i = 0u; i < view.size(); i++) {
        var c = view.get(i)
        if(c == '\\') { out.append_view("\\\\") }
        else if(c == '\'') { out.append_view("\\u0027") }
        else { out.append(c) }
    }
    return out
}
```

### Vector Access: `.get_ptr(i)` vs `.get(i)`

For `vector<T>` where `T` has a destructor (strings, structs), **never use `.get(i)`**:
- `.get(i)` returns `T` by **bitwise copy** — internals (like string data pointers) are shared
- When the temporary is destroyed, it frees the shared pointer, causing **double-free**
- Always use `.get_ptr(i)` which returns `*mut T` (a safe pointer)
- Chemical auto-dereferences pointers for member access: `(*ptr).field` → just `ptr.field`

Pattern:
```chemical
for(var i = 0u; i < vec.size(); i++) {
    var item = vec.get_ptr(i)
    // Use item.field, item.method() — auto-dereferenced
}
```

### Json Library Notes

- `JsonParser(bufferSize, 4096)` — parses JSON with configurable buffer
- `ASTJsonHandler` — builds an AST of `JsonValue` variants
- Types are in the top-level module (directly accessible, no `json::` prefix)
- `JsonValue` variants: `Null`, `Boolean(bool)`, `Integer(i64)`, `Double(f64)`, `String(string)`, `Array(vector<JsonValue>)`, `Object(unordered_map<string, JsonValue>)`
- Pattern matching: `if(val is JsonValue.String) { var String(s) = val else unreachable }`
- Access map values: `obj_map.get_ptr(string("key"))` — returns `*mut JsonValue` or null

### MongoDB Integration

- Uses `lang/compiled/mongodb/` package
- MongoDB C driver shared libraries: `libmongoc2.so`, `libbson2.so` in the build directory
- Pool pattern: `pool.pop()` to get a client, `pool.push(&mut client)` to return it
- Document building: `mongodb::Document.new()`, `.append_utf8(key, value)`, `.append_int64(key, value)`, `.append_oid(key, &oid)`
- OID: `mongodb::OID.from_string(hex_id)`
- Queries: `collection.find(filter).first()`, `collection.insert_one_with_id(&doc)`, `collection.delete_one(&filter)`

## Release
