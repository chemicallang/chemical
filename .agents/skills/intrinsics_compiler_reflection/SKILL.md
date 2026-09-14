---
name: Compiler Intrinsics and Reflection
description: Comprehensive guide to Chemical's compiler intrinsic functions and reflection APIs — how GlobalFunctions.cpp provides interpreter-friendly implementations, compile-time reflection, and metadata access.
---

# Compiler Intrinsics and Reflection

Chemical provides a rich set of compiler intrinsic functions (in `ast/utils/GlobalFunctions.cpp`) that are implemented directly in C++ and accessible as regular Chemical functions. These intrinsics provide reflection, compile-time evaluation, interpreter support for standard library types, and lower-level compiler interaction.

Everything lives under the `intrinsics::` namespace (with `intrinsics::llvm::` and `intrinsics::mem::` sub-namespaces), plus a handful of *top-level* built-ins (`defined(...)` and the `def` target-data object). The single source of truth is `ast/utils/GlobalFunctions.cpp` (3369 lines, 60+ helper classes).

## Architecture

The intrinsic functions are C++ classes that extend `FunctionDeclaration` and override `call()`. They are registered in the compiler's `GlobalInterpretScope` during initialization and become available as `intrinsics::function_name()`.

```cpp
// Pattern for creating an intrinsic:
class InterpretMyIntrinsic : public FunctionDeclaration {
public:
    explicit InterpretMyIntrinsic(TypeBuilder& cache, ASTNode* parent_node)
        : FunctionDeclaration(
            "my_intrinsic",                    // Name visible in Chemical code
            {cache.getVoidType(), ZERO_LOC},   // Return type
            false,                              // variadic?
            parent_node,                        // Parent node
            ZERO_LOC,                           // Source location
            AccessSpecifier::Public,            // Access specifier
            true                                // Is compiler declaration?
    ) {
        set_compiler_decl(true);  // Mark as compiler-provided
    }

    Value *call(InterpretScope *call_scope, ASTAllocator& allocator,
                FunctionCall *call, Value *parent_val,
                bool evaluate_refs) override {
        // Implementation here
        // Access arguments via call->values[i]->evaluated_value(*call_scope)
        // Use call_scope->error() for error reporting
        // Return values via allocator.allocate<T>()
    }
};
```

### Key File

| File | Lines | Purpose |
|------|-------|---------|
| `ast/utils/GlobalFunctions.cpp` | 3369 | All intrinsic function implementations + the interpreter-friendly `InterpretVector` + registration (`IntrinsicsNamespace`, `create_container`, `rebind_container`) |
| `ast/utils/GlobalFunctions.cpp` (`#include`s at :1-57) | — | The intrinsic classes are local to this translation unit — there is no `GlobalFunctions.h` |
| `compiler/lab/BackendContext.h` | 100 | Pure-virtual backend interface (`name`, `emit`, `forget`, `mem_copy`, `supports`, atomics) that codegen intrinsics call |
| `compiler/lab/backend/atomics.h` | — | `BackendAtomicMemoryOrder`, `BackendAtomicSyncScope`, `BackendAtomicOp` enums used by `intrinsics::llvm::atomic_*` |
| `compiler/frontend/AnnotationController.h` / `.cpp` | — | Annotation/marker/test collection read by `get_tests` and `get_single_marked_decl_ptr` |

### Registration Model (how `intrinsics::foo` resolves)

The `intrinsics` namespace is **not** declared in a `.ch` file — it is built in C++ and injected into every symbol resolver:

1. `struct IntrinsicsNamespace : public Namespace` (`GlobalFunctions.cpp:2822`) is named `"intrinsics"` and owns one member per intrinsic plus the `LLVMNamespace llvmNamespace` (:2779), `MemNamespace memNamespace` (:2807), and `InterpretVector::InterpretVectorNode vectorNode` (:2842).
2. Its constructor (`:2891-2921`) builds each `FunctionDeclaration` child and puts them in `nodes = { &interpretSupports, &printFn, ... }`. **A class that is not listed in `nodes` is not reachable from Chemical.**
3. `struct GlobalContainer` (`:2971`) owns the `IntrinsicsNamespace intrinsics_namespace`, the top-level `InterpretDefined defined`, and `DefThing defThing` (which materializes the `def` target-data object).
4. `GlobalInterpretScope::create_container` (`:3342`) and `rebind_container` (`:3311`) run `TopLevelDeclSymDeclare::VisitNamespaceDecl` on the namespace. That visitor (`compiler/symres/DeclareTopLevel.cpp:263`) calls `linker.declare_tld_node("intrinsics", ns, ...)` and registers every child in the namespace's `extended` map with `MapSymbolDeclarer`. Lookups like `intrinsics::get_tests` then walk `Namespace::extended`.
5. Because the comptime allocator is destroyed at the end of every job, `rebind_container` first clears `intrinsics_namespace.extended` / `memNamespace.extended` (`:3316-3317`) and re-declares everything. Any per-job state an intrinsic holds (e.g. `InterpretExprStrBlockValue::set_impls`) must be re-wired here.
6. `defined` and `def` are declared directly in the resolver root scope (`:3330-3332`) — as top-level names, **not** under `intrinsics::`.

### The `def` target-data object

`declare_def_values` (`:3259-3301`) populates `def` with booleans for every target/build axis: `def.c`, `def.tcc`, `def.clang`, `def.cbi`, `def.lsp`, `def.test`, `def.debug`, `def.debug_quick`, `def.debug_complete`, `def.release`, `def.release_safe`, `def.release_small`, `def.release_fast`, `def.little_endian`, `def.big_endian`, `def.is64Bit`, `def.windows`, `def.posix`, `def.win32`, `def.win64`, `def.linux`, `def.musl`, `def.macos`, `def.freebsd`, `def.unix`, `def.gnu`, `def.android`, `def.cygwin`, `def.mingw32`, `def.mingw64`, `def.x86_64`, `def.i386`, `def.arm`, `def.aarch64`, `def.powerpc`, `def.powerpc64`, `def.riscv32`, `def.riscv64`, `def.wasm32`, `def.wasm64`. It is recomputed per job/target; `defined("name")` instead consults the job's `definitions` set (`:1046`).

## Intrinsic Function Categories

### 1. Print/Debug Functions

| Intrinsic | Class | Signature | Purpose |
|-----------|-------|-----------|---------|
| `intrinsics::print` | `InterpretPrint` (`:291`) | `print(value : any...)` | Print values to stdout (space-joined via the visitor loop) |
| `intrinsics::println` | `InterpretPrintLn` (`:333`) | `println(value : any...)` | Print + newline |
| `intrinsics::to_string` | `InterpretToString` (`:412`) | `to_string(value : any...)` | Format one or more values into a `string` |
| `intrinsics::expr_print` | `InterpretExprPrint` (`:345`) | `expr_print(expr : %expressive_string)` | Print expressive string (backtick templates) |
| `intrinsics::expr_println` | `InterpretExprPrintLn` (`:399`) | `expr_println(expr : %expressive_string)` | Print expressive string + newline |

**Key implementation details:**
- `InterpretPrint` and `InterpretPrintLn` use `RepresentationVisitor` with `interpret_representation = true` (set at `:309`) for type-aware printing; each value is written with `visitor.visit(paramValue)` and a null prints as `null`.
- `InterpretExprPrint`/`InterpretExprPrintLn` handle expressive strings by iterating `ExpressiveString::values` (`:380-393`) — `ValueKind::String` segments are written to `std::cout` directly, `${}` expression segments are evaluated and printed via `RepresentationVisitor`.
- `InterpretToString` (`:418-444`) is the string-returning variant; it allocates the result on the arena with `allocator.allocate_str`.
- The old `print`/`println` are variadic (`any...`, `InterpretPrint` passes `true` for the variadic flag at `:301`); the new `expr_print`/`expr_println` take a single `%expressive_string` (`cache.getExprStrType()`, `:361`).

### 2. Compile-Time Mode Detection

| Intrinsic | Class | Purpose |
|-----------|-------|---------|
| `intrinsics::is_interpretation()` | `InterpretIsInterpretation` (`:666`) | True if running in interpretation mode (`global->interpretation_mode`) |
| `intrinsics::is_comptime()` | `InterpretIsComptime` (`:726`) | True if inside a comptime evaluation |
| `intrinsics::is_runtime()` | `InterpretIsRuntime` (`:689`) | True if running in codegen (not comptime) |
| `intrinsics::is_tcc_based()` | `InterpretIsTcc` (`:621`) | True if compiled with `TCC_BUILD` |
| `intrinsics::is_clang()` | `InterpretIsClang` (`:644`) | True if compiled with `COMPILER_BUILD` (LLVM/Clang) |

**Key implementation details:**
- `is_interpretation()` returns `global->interpretation_mode` (`:682`).
- `is_runtime()`/`is_comptime()` behave differently depending on whether they are reached from codegen or from symres:
  - In interpretation mode: `is_runtime()==false`, `is_comptime()==true` (`:708`, `:745`).
  - In compiled mode outside codegen eval (`global->is_runtime_call == false`): `is_runtime()==true`, `is_comptime()==false` (`:712-713`, `:749-750`).
  - In compiled mode inside codegen eval: runtime iff `global->call_stack.empty()` (`:720`), comptime iff it is non-empty (`:755`).
- `is_interpretation()` and `is_comptime()` set `ContractFlag::IsInterpretation = true`; `is_runtime()` sets it to `false` (`:679`, `:702`, `:739`).
- **The contract mechanism is what makes `comptime if` free.** `TypeVerifier::VisitIfStmt` (`compiler/typeverify/TypeVerify.cpp:1339-1431`) recognizes a `comptime if` whose condition calls a contract-bearing function and sets its internal `is_interpretation_mode` for each branch. `verifyArguments` short-circuits when `is_interpretation_mode` is true (`TypeVerify.cpp:847-848`), so argument type-checking rules that only hold at runtime are skipped in the interpretation branch. Codegen later folds the branch.

### 3. Type Reflection

| Intrinsic | Class | Signature | Purpose |
|-----------|-------|-----------|---------|
| `intrinsics::satisfies` | `InterpretSatisfies` (`:1079`) | `<T, U>()` | True if the type `U` can be used where `T` is expected (`T->satisfies(U)`) |
| `intrinsics::is` | `InterpretIs` (`:1107`) | `<T, U>()` | True if types `T` and `U` are identical (`T->is_same(U->canonical())`) |
| `intrinsics::value_satisfies` | `InterpretValueSatisfies` (`:1135`) | `(val1, val2)` | True if `val1`'s declared type satisfies `val2`'s value (`val1.getType()->satisfies(val2, false)`) |
| `intrinsics::is_same_type` | `InterpretIsSameType` (`:1176`) | `(val1, val2)` | True if both values have the same type |
| `intrinsics::type_to_string` | `InterpretTypeToString` (`:1508`) | `<T>()` | Returns string representation of type T |
| `intrinsics::size` | `InterpretSize` (`:470`) | `(value) : u64` | Returns the length of a `string` or the element count of an `ArrayValue` |
| `intrinsics::supports` | `InterpretSupports` (`:565`) | `(feature) : bool` | True if the backend supports a feature (string or enum-int) |

**Key implementation details:**
- `satisfies` / `is` operate on **generic type arguments** (`call->generic_list`), not values — they resolve at compile time. Source order is `satisfies<T, U>` → `T->satisfies(U)`, which is *assignability* semantics: the derived-to-base tests in `lang/tests/common/src/satisfies.ch:222-223` show `satisfies<Base, Derived>()` is true (`Derived` extends `Base`), and `satisfies<Derived, Base>()` is false. I.e. **`satisfies<To, From>` asks "can a `From` be used where a `To` is required"** — not "does `To` implement interface `From`".
- `is` canonicalizes the second operand before comparing (`:1131`).
- `value_satisfies` and `is_same_type` operate on **runtime values**; both declare their parameters with `cache.getMaybeRuntimeAnyType()` and return `false` when the first value has no type (`:1168-1172`, `:1208-1213`). `value_satisfies(T, U)` returned false in tests means the value's type does *not* satisfy the other value.
- `type_to_string` uses `RepresentationVisitor` (`interpret_representation = true`, `:1525`) on `generic_list.front()->pure_type(allocator)`.
- `size` resolves references through `resolve_ref()` (`:462-468`) so `size(&x)` sees the underlying value; only `String` and `ArrayValue` are accepted, otherwise it errors (`:501-503`).
- `supports` does backend lookup. With a string it hashes (djb2, `std::hash<chem::string_view>`) and matches only `"float128"`, `"atomic"`, `"inlineasm"` / `"InlineAsm"` (`:595-608`); any other string returns `false`. With a number it casts to `CompilerFeatureKind` (`:611-614`). The `CompilerFeatureKind` enum (`compiler/lab/BackendContext.h:8-14`) is `Float128=0, AtomicBuiltins=1, InlineAsm=2, Volatile=3`, but `"volatile"` is **not** in the string dispatch.
- Backend answers differ: `LLVMBackendContext::supports` always returns `true` (`compiler/backend/LLVMBackendContext.h:31-33`); `ToCBackendContext::supports` (`preprocess/2c/2cASTVisitor.cpp:8118-8145`) returns `Float128` only when `!target.tcc`, and `true` for `AtomicBuiltins`, `InlineAsm`, `Volatile`.

### 4. Source Location Reflection

| Intrinsic | Class | Purpose |
|-----------|-------|---------|
| `intrinsics::get_raw_location()` | `InterpretGetRawLocation` (`:761`) | Returns the encoded `SourceLocation` of the call site (u64) |
| `intrinsics::get_raw_loc_of(arg)` | `InterpretGetRawLocOf` (`:781`) | Returns the encoded `SourceLocation` of the argument |
| `intrinsics::get_line_no()` | `InterpretGetLineNo` (`:807`) | 1-based line number of the call site (u64) |
| `intrinsics::get_char_no()` | `InterpretGetCharacterNo` (`:830`) | 1-based character offset of the call site (u64) |
| `intrinsics::get_caller_line_no()` | `InterpretGetCallerLineNo` (`:860`) | 1-based line of the outermost comptime caller (0 if none) |
| `intrinsics::get_caller_char_no()` | `InterpretGetCallerCharacterNo` (`:887`) | 1-based char of the outermost comptime caller (0 if none) |
| `intrinsics::get_call_loc(back)` | `InterpretGetCallLoc` (`:914`) | Encoded location of the Nth caller in `global->call_stack` |
| `intrinsics::decode_location<T>(loc)` | `InterpretDecodeLocation` (`:955`) | Decodes an encoded location into a `T` with `filename`, `line`, `character` fields |
| `intrinsics::get_loc_file_path(loc)` | `InterpretGetLocFilePath` (`:1271`) | File path string for an encoded location |
| `intrinsics::get_current_file_path()` | `InterpretGetCurrentFilePath` (`:1352`) | File path of the current source file |

**Key implementation details:**
- `SourceLocation` is a compact 64-bit encoded value (10-bit file id, 18-bit line start, 12-bit char start, 11-bit line-end offset, 12-bit char end; see `core/source/LocationManager.h` and the Compiler API skill).
- `get_caller_line_no` / `get_caller_char_no` use `get_runtime_call()` (`:851-858`), which returns `global->call_stack.back()` — the outermost comptime call.
- `get_call_loc(back)` indexes the call stack from the top; if `back` exceeds the stack size it falls back to `call_stack.front()` (`:949`); if the stack is empty it returns the call site's own location.
- `decode_location<T>` requires exactly one generic argument naming a `StructDecl`; it fills `filename` (string), `line` (uint) and `character` (uint) (`:1002-1016`).
- `get_raw_location` declares its return as `cache.getUIntType()` but constructs the value with `getU64Type()` — treat it as a 64-bit raw location.

### 5. Module/Scope Reflection

| Intrinsic | Class | Purpose |
|-----------|-------|---------|
| `intrinsics::get_module_scope()` | `InterpretGetModuleScope` (`:1374`) | Returns the scope name of the enclosing module |
| `intrinsics::get_module_name()` | `InterpretGetModuleName` (`:1401`) | Returns the module name |
| `intrinsics::get_module_dir()` | `InterpretGetModuleDir` (`:1428`) | Returns the directory path of the enclosing module (empty for non-directory modules) |
| `intrinsics::get_build_dir()` | `InterpretGetBuildDir` (`:1305`) | Returns the build output directory |
| `intrinsics::get_compiler_path()` | `InterpretGetCompilerPath` (`:1329`) | Returns the compiler binary path |
| `intrinsics::get_target()` | `InterpretGetTarget` (`:1249`) | Returns the target triple string (e.g. `"x86_64-linux-gnu"`) |
| `intrinsics::get_libs_dir()` | `InterpretGetLibsDir` (`:2130`) | Returns `PROJECT_SOURCE_DIR/lang/libs` (source build) or `<exe>/../libs` |
| `intrinsics::version()` | `InterpretCompilerVersion` (`:546`) | Returns the compiler version string (`VERSION_STRING`) |
| `intrinsics::get_backend_name()` | `InterpretGetBackendName` (`:1995`) | Returns `"C"` (TCC backend) or `"LLVM"` |

**Key implementation details:**
- Module information is retrieved via `call_scope->global->current_func_type->get_parent()` → `get_mod_scope()` (`:1389-1397`, `:1416-1424`). All return `""` if there is no current function/module (`:1390`, `:1417`, `:1444`).
- `get_module_dir()` accesses `scope->container->paths[0]` and returns `""` unless `container->type == LabModuleType::Directory` (`:1451-1455`).
- `get_build_dir()` reads `build_compiler->options->build_dir`; `get_compiler_path()` reads `options->exe_path` (`:1324`, `:1347`). Both fall back to `""` when there is no build compiler (e.g. LSP).
- Target triple is from `build_compiler->current_job->target_triple` (`:1265`).
- `get_backend_name()` is `global->backend_context->name()` — used heavily by `std::atomic` to choose inline asm vs. LLVM atomics (see `lang/libs/atomic/src/atomic.ch:692` etc.).

### 6. Function / Annotation / Test Reflection

| Intrinsic | Class | Purpose |
|-----------|-------|---------|
| `intrinsics::get_child_fn<T>(name)` | `InterpretGetChildFunction` (`:1460`) | Returns a pointer to a *direct* child function of type `T` by name |
| `intrinsics::get_single_marked_decl_ptr(name)` | `InterpretGetSingleMarkedDeclPointer` (`:1575`) | Returns a pointer to the single declaration marked with an annotation name (functions only) |
| `intrinsics::get_tests<T>()` | `InterpretGetTests` (`:1623`) | Returns the generated table of `@test` functions for test struct `T` |
| `defined(name : string) : bool` | `InterpretDefined` (`:1022`) | Top-level (not `intrinsics::`) — true if a build definition is set |

**Key implementation details:**
- `get_child_fn<T>` requires exactly one value argument (the name) and exactly one generic argument (`:1480-1487`). It resolves `type->get_members_container()->direct_child_function(name)` (`:1494-1496`) and returns an `AccessChain` over a `VariableIdentifier`, or `null` on failure. Only **direct** children are supported (comment at `:1459`; there is a TODO at `:2885` to remove it in favour of explicit destructor lookup). Example: `intrinsics::get_child_fn<CTStructGetChild>("fake_sum")` (`lang/tests/common/src/comptime/basic.ch:65`).
- `get_single_marked_decl_ptr` uses `build_compiler->controller.get_single_marked(name)` and returns the function as a `VariableIdentifier`, or `null` if absent / not a `FunctionDecl` (`:1598-1617`). Used by the test runner for `@test.before_each` / `@test.after_each` (`lang/libs/test/src/runner.ch:85-86`).
- `get_tests<T>` requires one generic argument naming a `StructDecl` (`:1659-1670`), fetches collection 0 from the `AnnotationController` (`controller.get_definition("test")` → `get_collection(annot_def->collection_id)`, `:1673-1675`), and builds an `ArrayValue` of structs. Each element carries: `id` (from `@test.id` or `INT_MAX/2 + i`), `name` (`@test.name` or function name), `group` (`@test.group`), `ptr`, `file`, `timeout` (default `10000` ms), `retry` (clamped `0..999999`), `returns_bool`, `pass_on_crash`, `benchmark`, `lineNum`, `charNum` (`:1685-1789`). Entry point: `lang/libs/test/src/main.ch:1`.
- `defined` is a top-level function (registered at `GlobalFunctions.cpp:3330`) and is called as `defined("CHECK_DEF")` — **not** `intrinsics::defined(...)` (`lang/tests/common/src/comptime/basic.ch:315`). It reads `build_compiler->current_job->definitions` (`:1046`).
- The annotation machinery (`get_definition`, `get_collection`, `get_args`, `is_marked`) is documented in the **Annotations** skill.

#### Reflection deep-dive: what can and cannot be queried

| Capability | API | Status |
|-----------|-----|--------|
| Look up a direct child function | `get_child_fn<T>(name)` | Implemented |
| Fetch `@test` functions | `get_tests<T>()` | Implemented |
| Fetch a single marker decl | `get_single_marked_decl_ptr(name)` | Implemented (functions only) |
| Check annotation presence (compile time, C++ side) | `AnnotationController::is_marked(node, name)` | Implemented (not directly exposed as an intrinsic) |
| Enumerate struct fields / members | — | **Not implemented** (roadmap) |
| Enumerate implemented interfaces | — | **Not implemented** (roadmap) |
| Query member offsets / alignment | — | **Not implemented** (roadmap) |
| General `get_marked_decls<T>()` returning all marked decls | — | **Does not exist** despite mentions in `AGENTS.md`; only `get_single_marked_decl_ptr` and `get_tests` are real |

### 7. Compile-Time Computation, Errors & Memory

| Intrinsic | Class | Signature | Purpose |
|-----------|-------|-----------|---------|
| `intrinsics::error(message)` | `InterpretError` (`:1053`) | `(value : string)` | Emits a compile-time error with the given message |
| `intrinsics::forget(thing)` | `InterpretForget` (`:1545`) | `(thing) : bool` | Tells the backend to skip destructor/drop for a value (manual memory management) |
| `intrinsics::mem::copy(dest, src)` | `InterpretMemCopy` (`:1217`) | `(dest, src)` | Backend memory copy (`backend_context->mem_copy`) |
| `intrinsics::return_struct(...)` | `InterpretRetStructPtr` (`:520`) | `(...) : *void` | Produces a `RetStructParamValue` — sret handling (used by LLVM codegen; see `compiler/backend/LLVM.cpp:1255`) |
| `intrinsics::raw(value)` | `InterpretRawLiteral` (`:2059`) | `(value : string)` | Wraps a string as a `RawLiteral` (raw backend text) |
| `intrinsics::emit(value)` | `InterpretEmitRaw` (`:2023`) | `(value : string) : bool` | Emits raw text through `backend_context->emit` (C backend writes it; LLVM is a no-op) |
| `intrinsics::multiple(...)` | `InterpretMultiple` (`:2094`) | `(value : any...)` | Produces a `MultipleValue` from the evaluated arguments (variadic) |

> `%runtime_value(expr)` is a **value-level construct** parsed by the lexer (`parser/utils/LexValue.cpp`, producing `ValueKind::RuntimeValue`), not an intrinsic function. It wraps an expression for runtime evaluation. `%runtime`, `%maybe_runtime` and `%expressive_string` are type wrappers, likewise not intrinsics.

**Key implementation details:**
- `error` requires a `string` argument and reports through `call_scope->error(call) << ...` (`:1074`). It is a compile-time hard error.
- `forget` looks up the argument's `linked_node()` and returns `backend_context->forget(node)` (`:1569-1571`). Used by `std::vector`/`std::deque`/`json` to suppress destruction of moved-out values.
- `mem::copy` calls `backend.mem_copy(call->values[0], call->values[1])`; on the wrong arity it errors with "std::mem::copy called with arguments of length not equal to two" (`:1240-1242`).
- `emit`/`raw`/`multiple` are mostly used by macro plugins and the standard streams (`lang/libs/std/src/stream.ch`).

### 8. Expressive-String Stream Block

`intrinsics::expr_str_block_value(stream, expr, additional...)` → `InterpretExprStrBlockValue` (`:2162`) is an internal intrinsic (returns `i32`, variadic) that turns an expressive string into a `BlockValue` of `stream.writeX(...)` method calls. It is what powers `${}` interpolation for user-defined stream types (`std::string` at `lang/libs/std/src/string.ch:379`, `CommandLineStream` at `lang/libs/std/src/stream.ch:250`). It needs the `ImplementationsIndex` and `CoreNodes`, which are wired in per job via `set_impls` (`:2186`) from `create_container`/`rebind_container` (`:3320`, `:3349`). Supported `${}` element types: `string`, `char`/`uchar`, signed/unsigned ints, `float`, `double`, and any struct with a `stream` child method (`:2249-2317`).

### 9. Exported Lambda Capture Intrinsics

These expose closure internals and are consumed by codegen instead of the interpreter. All produce an `ExtractionValue` whose `ExtractionKind` selects the operation (`create_extraction_value_type`):

| Intrinsic | Class | ExtractionKind | Purpose |
|-----------|-------|----------------|---------|
| `intrinsics::get_lambda_fn_ptr(thing)` | `InterpretGetLambdaFnPtr` (`:1798`) | `LambdaFnPtr` | Raw function pointer of a lambda |
| `intrinsics::get_lambda_cap_ptr(thing)` | `InterpretGetLambdaCapPtr` (`:1832`) | `LambdaCapturedPtr` | Pointer to the captured environment |
| `intrinsics::get_lambda_cap_destructor(thing)` | `InterpretGetLambdaCapDestructor` (`:1867`) | `LambdaCapturedDestructor` | Destructor for the captured environment |
| `intrinsics::sizeof_lambda_captured(thing)` | `InterpretSizeOfLambdaCaptured` (`:1901`) | `SizeOfLambdaCaptured` | Size (u64) of captured data |
| `intrinsics::alignof_lambda_captured(thing)` | `InterpretAlignOfLambdaCaptured` (`:1936`) | `AlignOfLambdaCaptured` | Alignment (u64) of captured data |

### 10. Interpreter-Friendly Standard Library (`InterpretVector`)

The `InterpretVector` namespace (`GlobalFunctions.cpp:73-289`) provides a full interpreter-friendly implementation of `std::vector<T>`, reachable as **`intrinsics::vector<T>`** (not as `std::vector`):

```cpp
namespace InterpretVector {
    class InterpretVectorNode : public StructDefinition {      // :135
        GenericTypeParameter typeParam;                        // T
        InterpretVectorConstructor constructorFn;              // :77
        InterpretVectorSize sizeFn;                            // :86
        InterpretVectorGet getFn;                              // :97
        InterpretVectorPush pushFn;                            // :110
        InterpretVectorRemove removeFn;                        // :123
    };
    class InterpretVectorVal : public StructValue {            // :153
        std::vector<Value*> values;                            // actual elements
    };
}
```

**Supported operations:**

| Method | Implementation |
|--------|----------------|
| `intrinsics::vector<T>()` | `InterpretVectorConstructor::call` creates an empty `InterpretVectorVal` (`:178-185`) |
| `.size()` | `values.size()` as an `int` (`:199-201`) |
| `.get(index)` | `values[index]->scope_value(...)` — a **copy**, not a reference (TODO at `:204`) |
| `.push(value)` | `values.emplace_back(call->values[0]->scope_value(*call_scope))` (`:247-250`) |
| `.remove(index)` | `values.erase(values.begin() + index)` (`:266-275`) |

**How it works:**
- `InterpretVectorNode` is a `StructDefinition` whose child functions are `InterpretVectorConstructor`, `InterpretVectorSize`, `InterpretVectorGet`, `InterpretVectorPush`, `InterpretVectorRemove` (`:166-287`).
- `InterpretVectorVal` extends `StructValue` and holds a `std::vector<Value*>` for the actual data (`:153-164`).
- When `intrinsics::vector<T>` is used in interpretation mode, the constructor creates an `InterpretVectorVal` instead of the runtime `std::vector<T>` C++ implementation (`:178-185`).

Example (`lang/tests/common/src/comptime/vector.ch`):
```chemical
comptime func compiler_vector_sum(a : int) : int {
    var vec = intrinsics::vector<int>();
    vec.push(a); vec.push(10); vec.push(20); vec.push(40); vec.remove(3);
    var sum = 0; var i = 0;
    while(i < vec.size()) { sum += vec.get(i as uint) as int; i++; }
    return sum
}
```

**Current limitations (future work):**
- `.get()` returns a **copy**, not a reference — `v.get(0) = 42` won't modify the vector
- No `operator[]` overload in the interpreter
- No `get_ptr()` and no capacity management
- `std::unordered_map<K, V>` not yet interpreter-friendly
- `std::string` operations (`append`, `append_view`, `append_string`) not implemented as intrinsics
- `std::string_view` / `std::span<T>` not implemented as intrinsics

### 11. Backend-Provided (LLVM Atomic) Intrinsics

The `intrinsics::llvm::*` namespace (`LLVMNamespace`, `GlobalFunctions.cpp:2779`) exposes atomic primitives that only work when a `backend_context` exists. On the C/TCC backend many of them are unsupported and draw an error telling you to use inline asm or `%runtime_value()` instead (see `lang/libs/atomic/src/atomic.ch` and `preprocess/2c/2cASTVisitor.cpp:8147+`).

| Intrinsic | Class | Parameters | Returns |
|-----------|-------|-----------|---------|
| `intrinsics::llvm::atomic_fence(order, scope)` | `InterpretLLVMAtomicFence` (`:2411`) | `int, int` | void |
| `intrinsics::llvm::atomic_load(ptr, order, scope)` | `InterpretLLVMAtomicLoad` (`:2459`) | `%runtime<*any>, int, int` | `any` |
| `intrinsics::llvm::atomic_store(ptr, value, order, scope)` | `InterpretLLVMAtomicStore` (`:2507`) | `%runtime<*mut any>, %maybe_runtime<any>, int, int` | void |
| `intrinsics::llvm::atomic_cmp_exch_weak(ptr, expected, value, order1, order2, scope)` | `InterpretLLVMAtomicCmpExchWeak` (`:2561`) | 6 args | bool |
| `intrinsics::llvm::atomic_cmp_exch_strong(ptr, expected, value, order1, order2, scope)` | `InterpretLLVMAtomicCmpExchStrong` (`:2619`) | 6 args | bool |
| `intrinsics::llvm::atomic_op(op, ptr, value, order, scope)` | `InterpretLLVMAtomicOperation` (`:2677`) | `int, ptr, value, int, int` | `any` |
| `intrinsics::llvm::atomic_signal_fence(order)` | `InterpretLLVMAtomicSignalFence` (`:2735`) | `int` | void (compiler-only fence) |

**Key implementation details:**
- Every method validates arity, checks that order/scope/op integers are within `BackendAtomicMemoryOrder`/`BackendAtomicSyncScope`/`BackendAtomicOp` (`compiler/lab/backend/atomics.h`), and errors if `backend_context` is `null`.
- `atomic_fence`, `atomic_store` and `atomic_signal_fence` return the backend's renderable value (empty `RawLiteral` on C, `null` on LLVM) so the call site renders correctly.
- The corresponding `BackendContext` virtuals are declared in `compiler/lab/BackendContext.h:60-98` and implemented per backend.

### 12. Non-Registered / Reserved Names

- `InterpretDestructCallSite` (`:1970`, name `"destruct_call_site"`) is **defined but never added to `IntrinsicsNamespace::nodes`** — it is currently unreachable from Chemical code.
- `intrinsics::unreachable()` and `intrinsics::wrap(...)` do **not** exist. `unreachable` is a keyword/`UnreachableStmt` (`lexer/Lexer.cpp:79`, `parser/structures/ForBlock.cpp:46`), and `intrinsics::wrap` appears only in an intentionally-invalid negative test (`lang/compiled/bug.ch`) — do not rely on either.

## Availability Matrix (comptime / runtime / interpretation)

Every intrinsic is implemented by a C++ `FunctionDeclaration::call()` that runs **at compile time** — either while the interpreter executes a `comptime` function/block during symres, or while a backend evaluates a comptime call during codegen. The table below says where each family is *usable* and what it needs.

| Intrinsic family | `--interpret` (pure interpretation) | Comptime during codegen | Emitted runtime code | Requirements / notes |
|------------------|:---:|:---:|:---:|----------------------|
| `print`, `println`, `to_string`, `expr_print`, `expr_println` | ✅ | ✅ | ❌ | Writes to `std::cout`/returns a compile-time `StringValue`; not a runtime print |
| `is_interpretation`, `is_comptime`, `is_runtime`, `is_tcc_based`, `is_clang` | ✅ | ✅ | folded | Contract flag drives `comptime if` branch pruning (`TypeVerify.cpp:1339-1431`) |
| `satisfies`, `is`, `value_satisfies`, `is_same_type`, `type_to_string` | ✅ | ✅ | ❌ | Pure compile-time type queries |
| `size`, `defined`, `supports` | ✅ | ✅ | ❌ | `supports` needs a backend to give meaningful answers |
| `get_raw_location`, `get_raw_loc_of`, `get_line_no`, `get_char_no`, `get_caller_line_no`, `get_caller_char_no`, `get_call_loc`, `decode_location`, `get_loc_file_path`, `get_current_file_path` | ✅ | ✅ | ❌ | Read `LocationManager` / call stack |
| `get_module_scope`, `get_module_name`, `get_module_dir`, `get_target`, `get_build_dir`, `get_compiler_path`, `get_libs_dir`, `version`, `get_backend_name` | ✅ | ✅ | ❌ | Need `build_compiler`/`backend_context`; return `""` otherwise |
| `get_child_fn`, `get_single_marked_decl_ptr`, `get_tests` | ✅ | ✅ | ❌ | Reflection over resolved declarations; need `build_compiler->controller` |
| `error` | ✅ | ✅ | ❌ | Hard compile-time diagnostic |
| `forget` | ➖ | ✅ | ❌ | Calls `backend_context->forget`; no backend → null deref guard needed |
| `mem::copy` | ➖ | ✅ | ❌ | Calls `backend_context->mem_copy`; needs a backend |
| `emit`, `raw`, `multiple`, `return_struct` | ✅/➖ | ✅ | ✅ | `emit` writes raw backend text (C) / no-op (LLVM); `raw`/`multiple` are codegen carriers |
| `expr_str_block_value` | ✅ | ✅ | ❌ | Generates a `BlockValue` of `stream.writeX` calls; needs impls index |
| `get_lambda_*`, `sizeof_lambda_captured`, `alignof_lambda_captured` | ➖ | ✅ | ✅ | Produce `ExtractionValue`s consumed by closure codegen |
| `llvm::atomic_*` | ❌ | ✅ | ✅ | Require `backend_context`; C backend supports only via inline asm on `atomic_op`/`signal_fence`, other ops error |
| `vector<T>` | ✅ | ✅ | ❌ | Interpreter-only container (`InterpretVectorVal`) |

Legend: ✅ works, ➖ limited/needs a backend, ❌ not usable there. "Comptime during codegen" means the intrinsic is callable from a `comptime` function/block that a backend evaluates (LLVM or 2c) — the common path for `std::atomic` and `std::string.append`.

## Adding a New Intrinsic

### Step-by-Step

1. **Create the class** in `ast/utils/GlobalFunctions.cpp` (there is no header):

```cpp
class InterpretMyFeature : public FunctionDeclaration {
public:
    // Declare parameters as class members
    FunctionParam myParam;

    explicit InterpretMyFeature(TypeBuilder& cache, ASTNode* parent_node)
        : FunctionDeclaration(
            "my_feature",                                // Name
            {cache.getIntType(), ZERO_LOC},              // Return type (int)
            false,                                        // Not variadic
            parent_node,
            ZERO_LOC,
            AccessSpecifier::Public,
            true                                          // compiler declaration
    ), myParam("input", {cache.getStringType(), ZERO_LOC}, 0, nullptr, false, this, ZERO_LOC) {
        set_compiler_decl(true);   // Mark as compiler-provided
        params = { &myParam };     // Register parameters
    }

    Value *call(InterpretScope *call_scope, ASTAllocator& allocator,
                FunctionCall *call, Value *parent_val,
                bool evaluate_refs) override {
        // Validate arguments
        if(call->values.size() != 1) {
            call_scope->error("my_feature requires 1 argument", call);
            return nullptr;
        }

        // Evaluate the argument
        auto input = call->values[0]->evaluated_value(*call_scope);

        // Check the value kind
        if(input->val_kind() != ValueKind::String) {
            call_scope->error("expected a string argument", call);
            return nullptr;
        }

        // Return an integer
        auto& str = input->get_the_string();
        return new (allocator.allocate<IntNumValue>())
            IntNumValue(str.size(), call_scope->global->typeBuilder.getIntType(), ZERO_LOC);
    }
};
```

2. **Add a member to `IntrinsicsNamespace`** (`:2822`) — or to `LLVMNamespace`/`MemNamespace` for a sub-namespace. Add the same member to the constructor initializer list (`:2891`) **and** to the `nodes = { ... }` list (`:2909`). Missing either leaves the intrinsic unreachable.

3. **Wire per-job dependencies if any.** If the intrinsic needs `ImplementationsIndex`, `CoreNodes`, or the annotation controller, store them on the class and set them in `rebind_container` (`:3311`) and `create_container` (`:3342`) — these run once per job/target.

4. **For backend-specific codegen**, add a pure virtual to `BackendContext` (`compiler/lab/BackendContext.h`) and implement it in `LLVMBackendContext` (`compiler/backend/LLVMBackendContext.h`/`LLVM.cpp`) and `ToCBackendContext` (`preprocess/2c/2cBackendContext.h`/`2cASTVisitor.cpp`). Guard `backend_context == nullptr` and return a sensible fallback.

5. **No Chemical declaration is required** — the namespace is built from C++. It becomes callable as `intrinsics::my_feature("hello")` after symres re-declares the namespace.

6. **Add tests** in `lang/tests/common/src/comptime/intrinsics.ch`, `lang/tests/common/src/comptime/basic.ch`, or `lang/tests/src/basic/comptime/intrinsics.ch`. For reflection, a `comptime func` + `test(...)` pair is standard. For codegen intrinsics, test under both `--tcc` and `--interpret`.

7. **If you add a new `CompilerFeatureKind`**, remember `intrinsics::supports`'s string dispatch is a hardcoded switch (`:595-608`) — update it (and consider adding the enum name) or the feature can only be queried by integer.

### Parameter Pattern

```cpp
// Simple parameter:
FunctionParam myParam(
    "name",                                    // Parameter name
    { cache.getType(), ZERO_LOC },             // Type + location
    paramIndex,                                 // Parameter index
    defaultValue,                               // Default value (or nullptr)
    isSelf,                                     // Is it &self?
    parentFunction,                             // Owning function
    encodedLocation
);
```

### Common Return Patterns

```cpp
// Return integer
new (allocator.allocate<IntNumValue>())
    IntNumValue(value, type, location);

// Return boolean
new (allocator.allocate<BoolValue>())
    BoolValue(value, call_scope->global->typeBuilder.getBoolType(), ZERO_LOC);

// Return string
new (allocator.allocate<StringValue>())
    StringValue(string_view, call_scope->global->typeBuilder.getStringType(), ZERO_LOC);

// Return null
new (allocator.allocate<NullValue>())
    NullValue(call_scope->global->typeBuilder.getNullPtrType(), ZERO_LOC);

// Return void (no value)
return nullptr;
```

## The Interpreter-Friendly Standard Library (overview)

The interpreter provides its own implementations of certain standard library types so they can be used during comptime/interpretation. These use the same machinery as intrinsics (C++ `StructDefinition` + `FunctionDeclaration` children + `StructValue` data).

- **`InterpretVector`** (section 10) — the only fully implemented interpreter-friendly container, exposed as `intrinsics::vector<T>`.
- `std::string` is represented by `StringValue` holding a `chem::string_view`; `append*` methods are not implemented.
- `std::unordered_map`, `std::string_view`, `std::span<T>` are not implemented for the interpreter.

### How RepresentationVisitor powers printing

`intrinsics::print`/`println`/`to_string`/`expr_print*`/`type_to_string` all use `RepresentationVisitor` (`preprocess/RepresentationVisitor.h`, flag at `:33`) with `interpret_representation = true`, which suppresses quotes/escapes around strings and chars (`preprocess/RepresentationVisitor.cpp:681-699`). For `expr_print*` the `ExpressiveString`'s alternating literal/expression segments are evaluated with `interpret_representation = true` (see the Interpreter skill for `InterpretExprPrintLn`).

## Gotchas

- **`intrinsics::llvm::*` require a backend.** During symres (or LSP) `backend_context` may be `null`, and these intrinsics error out. Guard with `intrinsics::supports("atomic")` / `intrinsics::get_backend_name()` as `std::atomic` does.
- **`supports`'s string set is tiny and hashed.** Only `"float128"`, `"atomic"`, `"inlineasm"`/`"InlineAsm"` are recognized as strings. `"volatile"` exists in `CompilerFeatureKind` but is **not** in the dispatch. Prefer the enum-int form for precision.
- **`defined` is top-level, not `intrinsics::defined`.** Call it as `defined("NAME")`. The target-data object is `def.x86_64`, `def.linux`, etc.
- **`intrinsics::vector<T>` ≠ `std::vector<T>`.** The interpreter vector is a separate type; `.get()` returns a copy and `get_ptr`/iteration are unsupported.
- **Interpreter pointer bounds are enforced, compiled mode isn't.** Intrinsics that hand raw pointers around may behave differently in `--interpret`.
- **`is_interpretation()`/`is_comptime()`/`is_runtime()` affect type checking.** They carry `ContractFlag::IsInterpretation`; `TypeVerifier::VisitIfStmt` toggles argument verification off inside the interpretation branch (`TypeVerify.cpp:847-848, 1367-1431`). This is deliberate — code valid only at comptime (e.g. passing `&mut` to `*mut`) is allowed there.
- **Atomic order/scope enums must match `compiler/lab/backend/atomics.h`.** `BackendAtomicMemoryOrder` skips `3` (`NotAtomic=0, Unordered=1, Monotonic=2, Acquire=4, Release=5, AcquireRelease=6, SequentiallyConsistent=7`).
- **Variadic intrinsics declare `any...`/`%maybe_runtime<any>` params**, so the type checker is lenient; the C++ `call()` must validate arity and kinds and call `call_scope->error(...)`.
- **`get_tests<T>` / `decode_location<T>` require a `StructDecl` generic argument**, not an arbitrary type.
- **`get_child_fn<T>` only finds direct children** of `T`'s members container, and cannot see extension/impl methods.
- **`get_tests` state comes from `AnnotationController` collection 0**, which is cleared between jobs (`clear_marked_or_collected`). Tests must be declared in the same job.
- **`intrinsics::multiple` passes `false` for the variadic constructor flag and then calls `setIsVariadic(true)`** (`:2102-2113`) — a reminder that variadic-ness can be set two ways.
- **`error(msg)` is not a runtime panic** — it is a compile-time diagnostic.
- **`%runtime_value`, `%runtime`, `%maybe_runtime`, `%expressive_string`** are value/type syntax handled by the lexer/parser, not functions in `GlobalFunctions.cpp`.

## Future Goals for Reflection

Based on the project's roadmap:

### 1. Better Compiler Reflection
- **Field enumeration**: Allow iterating struct/union/variant members at compile time
- **Type properties**: Query type alignment, size, offset of members
- **Interface checks**: Find all types that implement a given interface
- **Call chain introspection**: Access full call stack at compile time
- **`get_marked_decls<T>()`**: enumerate *all* decls carrying a marker (today only `get_single_marked_decl_ptr` + `get_tests` exist)

### 2. Interpreter-Friendly Standard Library
- **`std::vector<T>`**: Already partially implemented (`InterpretVector`), needs:
  - Return references from `.get()` instead of copies
  - Support `operator[]` in the interpreter
  - `get_ptr()`, `reserve()`, iteration
- **`std::string`**: Need interpreter-friendly intrinsics for:
  - `append()`, `append_view()`, `append_string()`
  - `size()`, `get()`, `to_view()`
  - Copy constructor and assignment
- **`std::unordered_map<K, V>`**: Need a full `InterpretUnorderedMap` similar to `InterpretVector`
- **`std::string_view`**: Need lightweight view intrinsic
- **`std::span<T>`**: Need bounded view intrinsic

### 3. Automatic Serializer/Deserializer Generation
- Use `get_child_fn<T>` and marker lookups to find struct fields at compile time
- Generate `to_json()` and `from_json()` functions automatically
- See the **JSON Serialization** skill for the current `#json` approach via `json_cbi`.

Illustrative future API (from the original roadmap; not yet implemented — `intrinsics::get_members<T>()` does not exist today):

```chemical
// Future API:
func to_json<T>(val : T) : json::JsonValue {
    // Generated at compile time using compiler reflection
    comptime {
        var result = json::Object()
        for(field in intrinsics::get_members<T>()) {
            result[field.name] = to_json(field.value)
        }
        return result
    }
}
```

### 4. Automatic Generic Impl Declaration Instantiation
- When the user uses `myType.method()`, if `method` comes from a generic impl declaration, automatically instantiate and resolve it
- Deprecate the need for nested `impl` blocks

## Related Skills

- **Interpreter Internals** (`.agents/skills/interpreter/SKILL.md`) — The AST interpreter, scope management, move semantics, `InterpretVector` internals
- **Symbol Resolution** (`.agents/skills/symres/SKILL.md`) — How symbols are resolved; relevant for `intrinsics::*` namespace declaration and `get_child_fn`
- **Compiler API** (`.agents/skills/compiler_api/SKILL.md`) — The C++ `ASTNode`/`Value`/`BaseType` hierarchy and `lang/libs/compiler/` binding wrappers used when implementing intrinsics
- **Annotations** (`.agents/skills/annotations/SKILL.md`) — `AnnotationController`, `@test.*` markers, `get_tests`/`get_single_marked_decl_ptr` data source
- **AST Framework** (`.agents/skills/ast_framework/SKILL.md`) — `FunctionDeclaration`, `StructDefinition`, `StructValue` base classes and the `ASTAllocator` arena
- **CBI Plugin API** (`.agents/skills/cbi_plugin_api/SKILL.md`) — How macro plugins (which heavily exercise `get_raw_location`, `emit`, `raw`, `get_child_fn`) are built
- **Intrinsics & Reflection** (this file) — `GlobalFunctions.cpp` registration, all intrinsic implementations
- **LLVM Backend** (`.agents/skills/llvm_backend/SKILL.md`) — `LLVMBackendContext` and the sret/atomic codegen intrinsics drive
- **C Codegen (2c)** (`.agents/skills/c_codegen/SKILL.md`) — `ToCBackendContext` and `emit`/`mem_copy`/atomic behavior on the TCC backend
