---
name: Compiler Bindings
description: How Compiler Bindings work in Chemical, How compiler APIs are exposed for (build system, compilation and lsp)
---

# Bindings

By binding means compiler API exposed into Chemical source code, We don't mean intrinsic functions, chemical has those too, 
Those get called and interpreted, but bindings mean real functions in the compiler executable called from actual chemical source code,
We do not use libffi for this, We instead use Tiny CC. All chemical targets (Compiler, TCCCompiler, ChemicalLSP) contain libtcc
as a dependency.

## Basic overview

- Compiler translates user code (chemical) to C
- Compiler compiles that C code in memory using Tiny CC
- Compiler symbols are added using `tcc_add_symbol`
- Functions can be called inside the compiled program

## Where does it happen ?

- During the build process
  - We translate user's `build.lab` or `chemical.mod` into C, then we JIT compile it using Tiny CC
  - A build API is exposed to `build.lab`, which user uses to explain his dependencies and so on...
- During the compilation process
  - Inside the `build.lab`, User can advertise plugins to the compiler, which can be used during the compilation process
  - For example, macro processors are used to handle certain macros.
  - A compiler API is exposed to the plugin, which includes hooks for lexing, parsing and semantic analysis...
- During the LSP process
  - We expose some functions to compiler plugins to also provide semantic highlighting and folding ranges.
  - The current API is limited but there are plans to expand it.

## Source Code

These files contain the heart of how bindings interact, inside the Compiler

- [CBI.cpp](/compiler/cbi/bindings/CBI.cpp)
    - This file contains all the functions with the exact names they are being exposed
- [LibTcc.cpp](/integration/libtcc/LibTcc.cpp)
    - Tiny CC code which compiles the C code
- [LabBuildCompiler.cpp](/compiler/lab/LabBuildCompiler.cpp)
    - Entire build system implementation, compilation process
    - Search within it `LabBuildCompiler::link_cbi_job` (the function that links a compiler plugin)
- [BuildContextCBI.cpp](/compiler/cbi/bindings/BuildContextCBI.cpp)
    - Build context implementation, this provides the build API
- [lab.ch](/lang/libs/lab/src/lab.ch)
    - BuildContext binding written in chemical source code that is exposed to user
- [CompilerBinder.h](/compiler/cbi/model/CompilerBinder.h)
    - Used as the handler for binding of functions into compiler plugins

## Exposed APIs

- [lab.ch](/lang/libs/lab/src/lab.ch)
    - BuildContext binding written in chemical source code that is exposed to user
- [compiler lib](/lang/libs/compiler)
    - Compiler API like Lexer, Parser, SymbolResolver are here
    - The most important file is [ASTBuilder](/lang/libs/compiler/src/ASTBuilder.ch) which allows to create chemical AST nodes, values and types
- LSP bindings
    - [ide lib](/lang/libs/ide/) FoldingRangeAnalyzer, SemanticTokenScopes, SemanticTokensAnalyzer and more...
    - [minlsp lib](/lang/libs/minlsp)
- [transformer lib](/lang/libs/transformer)
  - Transformer API helps transform chemical AST to other stuff
  - For example, we use transformer API in [refgen](/lang/libs/refgen) to create API documentation in HTML for Chemical

## The Compiler Lifecycle

This lifecycle takes into account all the processes mentioned above. This is very simplified and does not cover all the details.

- Compiler translates user's `build.lab` to C and then JIT compiles it and expose a build API to it
- User's `build.lab` uses the build API to explain dependencies, module graph and jobs
- User's `build.lab` uses the compiler API to advertise plugins as well
- Compiler compiles the plugins before building anything, so it can call them when parsing actual user code
- Compiler initiates the build, executes user's jobs, compiles the modules, calls the plugins when macros are encountered
- Compiler plugins parse their code, usually convert to our AST and ask our compiler to compile it
- Compiler generates assets (executables, dynamic libs, whatever user asked)

## Related Skills

- **Compiler Plugin API** (`.agents/skills/cbi_plugin_api/SKILL.md`) — How to develop compiler plugins: ASTBuilder API, macro registration, plugin structure, debugging
- **Build System (LabBuildCompiler)** (`.agents/skills/build_system/SKILL.md`) — How plugins are compiled and loaded during the build process
- **Performance** (`.agents/skills/performance/SKILL.md`) — Optimization patterns, parallel compilation of plugins

## Future

In the Future, We may use LLVM ORC JIT instead of Tiny CC, but the binding system will remain the same.