## Project Structure

Here's a detailed overview of the Chemical codebase. The project is written in **C++ 20**.

### Entrypoint
- [CompilerMain.cpp](core/main/CompilerMain.cpp): The main entry point for the compiler executable. It parses command-line arguments and uses the `LabBuildCompiler` API.

### Build System (Lab)

Everything related to compilation and running of `chemical.mod` and `build.lab` is located in the [compiler/lab](compiler/lab) directory.

#### How it works:
1.  **Entrypoint**: [CompilerMain.cpp](core/main/CompilerMain.cpp) invokes the [LabBuildCompiler](compiler/lab/LabBuildCompiler.cpp).
2.  **In-Memory Translation**: If a project uses `chemical.mod`, the [ModToLabConverter](compiler/lab/mod_conv/ModToLabConverter.cpp) translates it into a `build.lab` file in memory.
3.  **Lab Program**: A `build.lab` file is a Chemical program. All necessary `build.lab` files are combined into a single C program, compiled by **TinyCC** (in memory), and executed.
4.  **Job Orchestration**: This "Lab program" interacts with the compiler via the `BuildContext` interface. It emits `LabJob` objects (e.g., `Executable`, `Library`, `CBI`).
5.  **Job Execution**: The compiler then executes these jobs one by one.
6.  **Job Reordering**: Plugins can dynamically reorder these jobs using `ctx.put_job_before`. This is critical for CBI plugins, as they must be executed and booked into the compiler *before* the module that depends on them is parsed.

#### LibTCC Integration
Chemical integrates **TinyCC (libtcc)** for rapid JIT execution and build-time tasks.
- [LibTcc.cpp](integration/libtcc/LibTcc.cpp): Handles the creation and management of `TCCState`.
- **Mode Switching**: The compiler switches TCC between `TCC_OUTPUT_MEMORY` (for JIT/CBI) and `TCC_OUTPUT_EXE/OBJ/DLL` based on the target.
- **Error Handling**: Custom error printers (`handle_tcc_error`) and backtrace handlers capture runtime crashes in JIT mode.
- **Resource Bundling**: TCC's include and library paths are resolved relative to the compiler executable to ensure a portable build environment.

#### Caching & Incremental Builds
Chemical implements a simple but effective timestamp-based caching system in [Timestamp.cpp](compiler/lab/timestamp/Timestamp.cpp).
- **Mechanism**: For each module, the compiler saves the modified time and file size of all source files into a binary `.timestamp` file.
- **Validation**: On the next run, `compare_mod_timestamp` checks current file stats against the saved data. If any file has changed, the module is re-compiled.

#### Target Data & Conditional Compilation
Target-specific information is managed via `TargetData` and exposed to the build system.
- [TargetData.h](compiler/lab/TargetData.h): A bitmask-like struct containing flags for OS (windows, linux, macos), architecture (x86_64, aarch64), and compiler (tcc, clang).
- **Conditional Source**: In `chemical.mod`, you can use `@if` blocks like `source "win" if windows && !tcc`. These are evaluated by [TargetConditionAPI.h](compiler/lab/TargetConditionAPI.h) using the current `TargetData`.

#### Key Files:
- [LabBuildCompiler.cpp](compiler/lab/LabBuildCompiler.cpp): The heart of the build system.
- [ModToLabConverter.cpp](compiler/lab/mod_conv/ModToLabConverter.cpp): Logic for `chemical.mod` -> `build.lab`.
- [LabJob.h](compiler/lab/LabJob.h): Defines the various job types.
- [Timestamp.cpp](compiler/lab/timestamp/Timestamp.cpp): Caching logic.

### AST (Abstract Syntax Tree)
The [ast](ast) directory contains the models for the syntax tree.
- [ast/base](ast/base): Contains base classes.
  - [ASTNode.h](ast/base/ASTNode.h): The base class for all nodes (e.g., `WhileLoop`, `VarInit`).
  - [Value.h](ast/base/Value.h): Represents all values (referenced, primitive, arrays, structs, etc.).
  - [BaseType.h](ast/base/BaseType.h): Represents all type classes (e.g., `Int`, `ReferencedType`).
- [ast/statements](ast/statements) & [ast/structures](ast/structures): Hold various statement and declaration nodes.
- **AccessChain**: [AccessChain.h](ast/values/AccessChain.h) is a critical node representing hierarchical lookups (e.g., `x.y.z`, `a[i].b()`). It manages a vector of `ChainValue` objects and handles GetElementPtr (GEP) logic for structures and arrays. Currently, it supports identifiers, function calls, and index operations, with plans to expand into more complex value types.

### Memory Management (Allocators)
Chemical uses a high-performance **arena-based allocation** strategy to minimize overhead and simplify memory management.

- [BatchAllocator](ast/base/BatchAllocator.h): The base class for allocating memory in large batches (arenas). Instead of individual `new`/`delete` calls, memory is requested in chunks.
- [ASTAllocator](ast/base/ASTAllocator.h): Inherits from `BatchAllocator` and adds support for **automatic destruction**. It stores a list of pointers and cleanup functions that are executed when the allocator is cleared.
- **Lifetimes**:
    - `file_allocator`: Lives for the duration of a single file's processing.
    - `mod_allocator`: Lives for the duration of a module's compilation.
    - `job_allocator`: Lives for the duration of a `LabJob`.
- **Strategy**: When a phase (like parsing a module) is complete, the entire allocator is cleared (`clear()`), effectively "sweeping" all allocated nodes and values in one go.

### Type System
The type system is managed by the `TypeBuilder`, which acts as a factory and memoizer for all `BaseType` objects.

- [TypeBuilder.h](ast/base/TypeBuilder.h): Caches common types (e.g., `i32`, `bool`, `void`) and provides methods to build complex types like pointers and arrays.
- **Chemical Types**: `I8`, `I16`, `I32`, `I64`, `Int128`, and their unsigned counterparts.
- **C-Compatible Types**: `Char`, `Short`, `Int`, `Long`, `LongLong` etc., used mainly during interop.
- **Unresolved Types**: During the initial parsing phase, types are often linked to an `UnresolvedDecl` when they aren't found.

### Lexer & Parser
- [stream](stream): Defines methods for reading from source files, used by the Lexer.
- [lexer](lexer): Contains the lexer implementation.
  - [Lexer.h](lexer/Lexer.h) / [Lexer.cpp](lexer/Lexer.cpp): The core lexing logic.
- [parser](parser): Contains the recursive descent parser.
  - [Parser.h](parser/Parser.h) / [Parser.cpp](parser/Parser.cpp): Core parser functions.
  - The implementation is divided into subdirectories:
    - [parser/statements](parser/statements): Parsing logic for statements. **Note:** Some files here are named with a `Lex` prefix (e.g., `LexStatement.cpp`, `LexType.cpp`) due to historical reasons from an earlier CST implementation.
    - [parser/structures](parser/structures): Parsing logic for top-level structures like functions, structs, and interfaces.
    - [parser/values](parser/values): Parsing logic for specific value types like lambdas and struct literals.
    - [parser/utils](parser/utils): Contains general parsing utilities. **Note:** `Expression.cpp` and `LexValue.cpp` (another "Lex" prefix) handle the core expression and value parsing.

### Diagnostics and Locations
Error reporting and source location tracking are handled by a dedicated diagnostic subsystem.

- [LocationManager](core/source/LocationManager.h): The authority on all source locations. It maintains a mapping of file paths and handles the encoding/decoding of `SourceLocation`.
- [SourceLocation.h](core/source/SourceLocation.h): A `uint64_t` that uniquely identifies a position in the source code. It can represent locations in physical files, macro expansions, or virtual buffers.
- [Diagnoser](core/diag/Diagnoser.h): A class used to emit errors and warnings. It uses the `LocationManager` to translate `SourceLocation` into human-readable file:line:column strings.
- [Diagnostic.h](core/diag/Diagnostic.h): Represents a single error or warning event, including its severity, message, and location.


### Compiler Binding Interface (CBI)
CBI (**Compiler Binding Interface**) is a powerful extensibility system that allows developers to write compiler plugins in **C**. These plugins are compiled on-the-fly using **TinyCC** and hooked into the compiler.

#### Interfacing with the Compiler:
When a `build.lab` requests a `BuildContext`, it triggers the compiler to provide various C-compatible interfaces. This is handled by [CompilerBinder::import_compiler_interface](compiler/cbi/model/CompilerBinder.h), which populates the TCC state with function pointers from the compiler's own source code.

These mappings are defined in [CBI.cpp](compiler/cbi/bindings/CBI.cpp) using `interface_maps` (see `prepare_cbi_maps`). Available interfaces include:
- `Parser`, `Lexer`, `SourceProvider`
- `ASTBuilder`, `SymbolResolver`, `ASTDiagnoser`
- `BuildContext`, `BatchAllocator`

#### Plugin Lifecycle:
1.  **CBI Job**: A plugin creates a `LabJobCBI`.
2.  **Compilation**: The plugin's C code is compiled by TinyCC and relocated in memory.
3.  **Indexing**: Functions within the plugin (like macro parsers) are indexed into the `CompilerBinder` via `registerHook`.
4.  **Persistent State**: Unlike normal scripts, a CBI plugin's TCC state is kept in memory (stored in `CBIData`) as long as the compiler session is active, allowing its hooks to be called whenever needed.

- [compiler/cbi](compiler/cbi): The core CBI implementation.
- **Macros**: CBI is primarily used to implement `#macro` functionality. When the parser encounters a `#macro`, it looks for a registered hook in the `CompilerBinder` to handle lexing, parsing, or symbol resolution for that macro.
- **Examples**: You can find several CBI-based libraries in [lang/libs](lang/libs), such as `js_cbi`, `html_cbi`, `css_cbi`, `react_cbi`, `solid_cbi`, and `preact_cbi`.

#### Embedded Nodes & AST Injection
Plugins often need to inject custom logic into the Chemical pipeline. This is achieved via **Embedded Nodes**.
- [EmbeddedNode.h](ast/statements/EmbeddedNode.h): A special AST node that holds a `data_ptr` to plugin-specific data.
- **Resolution Hooks**: Plugins provide `known_type_fn` and `child_res_fn` to explain how the compiler should handle the custom node during semantic analysis.
- **AST Generation**: `EmbeddedNode` is typically replaced by standard Chemical AST nodes (stored in the `replacement` field) during the plugin pass, allowing plugins to generate backend-agnostic code.


### Annotation Handling
The [AnnotationController](compiler/frontend/AnnotationController.h) manages how annotations (e.g., `@if`, `@test`) are handled. The parser calls this controller when it encounters an annotation.

### Symbol Resolver (Semantic Analysis)
The symbol resolver (semantic analyzer) is responsible for resolving identifiers, types, and performing initial type checks. It runs in several passes:
1. [DeclareTopLevel.cpp](compiler/symres/DeclareTopLevel.cpp): Declares top-level symbols (functions, structs, etc.).
2. [LinkSignature.cpp](compiler/symres/LinkSignature.cpp): Resolves symbols in signatures and figures out variable types.
3. [SymResLinkBody.cpp](compiler/symres/SymResLinkBody.cpp): Resolves symbols within function bodies recursively.
4. [SymbolResolver.cpp](compiler/symres/SymbolResolver.cpp): Wires the entire resolution process.

### Generic Instantiation (Monomorphization)
Located in [compiler/generics](compiler/generics), this system handles the instantiation of generic types and functions.

### Comptime and Interpretation
Chemical supports compile-time execution of code via the `@comptime` keyword and intrinsic functions.

- [GlobalInterpretScope](compiler/lab/LabBuildContext.h): Manages the state for compile-time evaluation. It holds the backend context and oversees the mapping of intrinsic functions.
- [Interpreter](compiler/Interpreter/Core.cpp): An experimental tree-walking interpreter used to evaluate Chemical expressions and statements at compile-time.

#### Global Intrinsics & Comptime API
Intrinsics allow Chemical code to interact with the compiler's internal state during interpretation.
- [GlobalFunctions.cpp](ast/utils/GlobalFunctions.cpp): Defines the `intrinsics` namespace available in Chemical.
- **Important Functions**:
    - `intrinsics::print`: Console logging from the compiler.
    - `intrinsics::target_info`: Accesses the `TargetData` struct (initialized via `prepare_target_data`) to check bitness, OS, and endianness.
    - `intrinsics::wrap`/`unwrap`: Transitions between normal values and comptime-only values.
    - `intrinsics::get_raw_location`: Direct access to the `SourceLocation` of a node.
- **Backend Context**: During comptime, a `BackendContext` (either LLVM or C) is often attached to the interpretation scope to allow the code to "emit" instructions or gather metadata about the ongoing compilation.

### Backends
- **C Translation**: [preprocess/2c/2cASTVisitor.h](preprocess/2c/2cASTVisitor.h) provides a visitor that translates Chemical AST to C code.

### Name Mangling
To avoid naming collisions in the generated C or LLVM code, Chemical uses a robust mangling system in [NameMangler.cpp](compiler/mangler/NameMangler.cpp).

- **Module Prefixing**: Symbols are prefixed with their scope and module name (e.g., `scope_module_symbol`).
- **Generic Instantiation**: 
    - Structs: `Symbol__cgs__N` (where N is the instantiation index).
    - Functions: `Symbol__cfg__N`.
- **Multi-functions**: Overloaded functions use `__cmf__N` suffixes.
- **Interop**: Symbols marked with `no_mangle` (via annotations like `@no_mangle` or inside `extern` blocks) retain their original names for C compatibility.

### LLVM Backend & Codegen
The LLVM backend is the primary target for Chemical.

- [Codegen.h](compiler/Codegen.h): The central coordinator for LLVM code generation.
- **Type Mapping**: Most Chemical types map directly to LLVM types (e.g., `IntNType` to `getIntNTy`, `PointerType` to `getPtrTy`).
- **Operator Overloading**: Chemical implements operator overloading by attempting to resolve specific method names before emitting a primitive instruction. For example:
    - `a + b` -> looks for `add(a, b)`
    - `a[i]` -> looks for `index(a, i)`
    - `!a` -> looks for `not(a)`
- **VTables**: Interface calls are handled via virtual tables, whose names are also mangled (e.g., `Interface_Struct_vtable`).

### Module Resolution & Aliases
The build system uses a flexible path resolution strategy managed by the [LabBuildContext](compiler/lab/LabBuildContext.cpp).

- **Path Aliases**: You can define aliases (e.g., `@std`, `@lab`) that map to specific directories. These are resolved recursively during job initialization.
- **Module Storage**: All loaded [LabModule](compiler/lab/LabModule.h) objects are stored in a central registry, ensuring they are only parsed and compiled once.
- **Dependency Tracking**: The `LabBuildContext` tracks directed acyclic graphs (DAGs) of module dependencies to determine compilation order.

### Lambdas and Closures (Fat Pointers)
Chemical uses **Fat Pointers** as a general mechanism for abstraction (interface calls via `dyn` and capturing lambdas). 

In [LambdaFunction.cpp](ast/values/LambdaFunction.cpp) and [LLVM.cpp](compiler/backend/LLVM.cpp):
1.  **Non-Capturing Lambdas**: Compiled as standard C-style function pointers.
2.  **Capturing Lambdas & Dynamic Types (`dyn`)**: Compiled as a struct containing two pointers:
    - `ptr1`: The function pointer (for lambdas) or vtable pointer (for `dyn`).
    - `ptr2`: A pointer to the captured data struct or the concrete object instance.
- **Capture Strategy**: Variables can be captured by **value** or **reference**.
- **Lifecycle**: If any captured variable has a destructor, the compiler automatically generates a `lambda_cap_destr` function to clean up the capture struct when the lambda goes out of scope.

### C Backend Internals
The C backend ([ToCAstVisitor.h](preprocess/2c/2cASTVisitor.h)) uses a multi-pass visitor architecture for robust transpilation:

- **CValueDeclarationVisitor**: Scans function bodies to "lift" nested lambdas and static values to the top-level file scope.
- **CBeforeStmtVisitor**: Handles preparations needed *before* a statement is emitted, such as allocating temporary variables for complex expressions.
- **CDestructionVisitor**: Emits calls to destructors for local variables and temporary objects at the end of their respective scopes.
- **Struct Optimization**: Like the LLVM backend, large structs are passed as pointers to optimize performance and memory usage.

### Type Verification Pass
While the Symbol Resolver handles initial type checking, a second **Type Verification** pass ([TypeVerify.cpp](compiler/typeverify/TypeVerify.cpp)) runs later to perform validation

### Standard Library & Lab
Core language features and build utilities are implemented as Chemical modules in [lang/libs](lang/libs):

- **std**: The standard library (I/O, collections, strings).
- **lab**: Build system utilities and the `BuildContext` interface.
- **cstd**: C-compatible standard library abstractions for interop.

### Project Utilities & Libraries
Chemical uses several specialized libraries and custom types to maintain performance and portability.

- **Strings**: Chemical uses custom string types defined in [chem_string.h](std/chem_string.h) and [chem_string_view.h](std/chem_string_view.h) instead of `std::string`. These implement **Small String Optimization (SSO)** and custom move semantics tailored for compiler performance.
- **Console Output**: The [rang](https://github.com/agauniyal/rang) library is used across the codebase for cross-platform colored terminal output.
- **Concurrency**: The [ctpl](https://github.com/vit-vit/CTPL) library provides the thread pool implementation (used in `ASTProcessor.cpp`) for parallel compilation of modules.

### Debug Information
LLVM-target builds include DWARF debug information generated by a specialized builder.
- [DebugInfoBuilder.cpp](compiler/backend/DebugInfoBuilder.cpp): Maps Chemical source locations to LLVM metadata.
- **Type Mapping**: Translates Chemical types (IntN, Structs) into `DIType` objects.
- **Lexical Scopes**: Manages a stack of `DIScope` objects to correctly represent functions and nested blocks in debuggers like GDB or LLDB.

### LSP (Language Server Protocol)
The [server](server) directory contains the LSP implementation that provides IDE features like completion, goto definition, and semantic highlighting.
- [WorkspaceManager.h](server/WorkspaceManager.h): Manages the IDE session and orchestrates various analyzers.
- [Analyzers](server/analyzers/): Specific classes for features like `CompletionItemAnalyzer`, `SemanticTokensAnalyzer`, etc.

### Tests
Tests are located in [lang/tests](lang/tests). After making changes, verify that the tests still pass.

**LLVM based compiler:**
```bash
chemical "lang/tests/build.lab" -arg-minimal -v -vl -bm -bm-modules --debug-ir --mode debug_complete --assertions --no-cache
```

**TinyCC based compiler:**
```bash
chemical "lang/tests/build.lab" -arg-minimal -bm -bm-modules -v --mode debug --no-cache
```
*Note: Use `chemical.exe` on Windows.*

### Contributing to Macros

Macros are a great way to extend Chemical. To contribute a new macro or improve an existing one:

1.  **Identify the Area**: Determine if your macro needs custom lexing, parsing, or symbol resolution.
2.  **Look at Examples**: The best way to learn is by looking at existing CBI libraries in [lang/libs](lang/libs).
    - `js_cbi`: Demonstrates complex parsing of JavaScript within Chemical.
    - `html_cbi`: Shows how to integrate HTML templates.
3.  **Implement the Hook**: Write your hook in C (utilizing the provided compiler interfaces) and register it using the appropriate `CBIFunctionType`.
4.  **Testing**:
    - Macro-specific tests are located in [lang/tests/libs](lang/tests/libs).
    - Add a new test case in the relevant subdirectory (e.g., `lang/tests/libs/js` for JS macro tests).
    - Run the tests using the commands provided in the [Tests](#tests) section.

