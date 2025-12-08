## Project Structure

Here's a rough overview of the structure of the project

### Tests

These tests are written in `chemical`, We aren't testing C++ at the moment, we have a testing framework for
our language, however most of the tests compile & run inside a single executable (one fails others)
That's because these tests were written before the testing framework.

Here's all the [tests](lang/tests)

After performing a change in the compiler implementation, one must verify tests are succeeding, for which
there are commands (if you are using CLion or Jetbrains stuff, run configurations would be available)

Otherwise, you can just run by providing `chemical compiler` path to the `build.lab` file

LLVM based compiler:

```
chemical "lang/tests/build.lab" -arg-minimal -v -vl -bm -bm-modules --debug-ir --mode debug_complete --assertions --no-cache
```

TinyCC based compiler:

```
chemical "lang/tests/build.lab" -arg-minimal -bm -bm-modules -v --mode debug --no-cache
```

use `chemical.exe` instead of just `chemical` on windows

### Entrypoint
- The function that runs when the compiler executable is invoked is located in [CompilerMain.cpp](core/main/CompilerMain.cpp)
- Other than parsing the command line arguments, it just uses LabBuildCompiler API to execute user jobs

### Lab (build system)
Everything related to compilation and running of `chemical.mod` and `build.lab` is present in [lab](compiler/lab) directory

### AST Classes

- [ast](ast) (folder contains models for the syntax tree)
  - [ast/base](ast/base) is the folder containing base classes
  - [ASTNode](ast/base/ASTNode.h) class that is inherited by all the nodes (`WhileLoop`, `VarInit`, `ReturnStatement`)
  - [Value](ast/base/Value.h) class that represents all the values (referenced, primitive & others like Array and Struct)
  - [BaseType](ast/base/BaseType.h) class that represents all the type classes in the project like `Int` and `ReferencedType`
  - [This directory](ast/statements) Holds some of the statements
  - [This directory](ast/structures) Holds most of the declarations


### Lexer & Parser
- [stream](stream) A single class defines the methods that are used to read from a `std::ifstream`, It's used by the `lexer` to read source code.
  - The lexer always takes a `InputSource`, which is like a string_view (a data pointer and length)
- [lexer](lexer) The directory contains the lexer implementation
  -  [Lexer.h](lexer/Lexer.h) to get an idea of the API
  -  [Lexer.cpp](lexer/Lexer.cpp) for the implementation
- [parser](parser) The directory contains the parser implementation
  - [Parser.h](parser/Parser.h) to get an idea of the API
  - [Parser.cpp](parser/Parser.cpp) basic functions are implemented in this
  - Parser implementation is divided in different files, files are also wrongly named Lex
  - Early version used to convert to a CST, which has been removed now, that's why.

### Annotation Handling

Parser asks the annotation controller to handle an annotation

- [AnnotationController.h](compiler/frontend/AnnotationController.h)
- [AnnotationController.cpp](compiler/frontend/AnnotationController.cpp)

### Symbol Resolver (semantic analysis)

Symbol Resolver can be called semantic analyser, we try to understand the user's code

- Symbol resolver has four jobs
  - Resolves symbols (referenced identifiers or referenced types)
  - Figure out type of everything
    - everything in chemical that is a [Value](ast/base/Value.h) has a type pointer
    - [VariableIdentifier](ast/values/VariableIdentifier.h) inherits from [Value](ast/base/Value.h)
    - we figure out type of each identifier, then set this type pointer
  - minimal verification is done at this step, still type verification runs after symbol resolution
  - use the GenericInstantiator API to instantiate generic implementations

- Symbol Resolver is present in [this directory](compiler/symres)
- [DeclareTopLevel.cpp](compiler/symres/DeclareTopLevel.cpp)
  - first to run
  - declares the symbols (of functions, structs or variants...)
  - does not recursive (no nesting), only the top level nodes
- [LinkSignature.cpp](compiler/symres/LinkSignature.cpp)
  - second to run
  - resolves symbols in signatures (of functions, structs or variants...)
  - figures out type for each variable identifier
  - recursive but doesn't visit bodies of functions
- [SymResLinkBody.cpp](compiler/symres/SymResLinkBody.cpp)
  - third to run
  - resolves symbols in bodies of the functions
  - figures out type for each variable identifier
  - recursive, visits everything (even in bodies of functions)
- [SymbolResolver.cpp](compiler/symres/SymbolResolver.cpp)
  - here's the wiring for everything related to symbol resolution

### Generic Instantiation (Monomorphization)

Everything related to generic mono morphization is present inside [generics](compiler/generics) directory

### Type Verification
Currently, Most type verification is performed during symbol resolution, however will soon separate it
Everything related to type verification is present in this [directory](compiler/typeverify)

### Interpretation

chemical needs to interpret chemical code because at compile time user invokes functions

interpretation and comptime functions are experimental feature, but present in this [directory](compiler/Interpreter)

### Global Functions

chemical contains intrinsic global functions that allow you to interact with the compiler at comptime
the definitions for these functions is present in [GlobalFunctions.cpp](ast/utils/GlobalFunctions.cpp)

### Compile using LLVM
- [compiler](compiler/backend/LLVM.cpp)
- [class Codegen](compiler/Codegen.h)
- I organized llvm stuff into multiple files, however I'm changing that it'll take time.


### Translation To C
- [preprocess](preprocess)
  - This needs to be shifted to compiler directory
  - [2cASTVisitor](preprocess/2c/2cASTVisitor.h) The class that converts Chemical `AST` to `C`

### Translating from C

we use Clang to parse the C, therefore this is only available in LLVM based compiler.

- [CTranslator.cpp](compiler/ctranslator/CTranslator.cpp) that initializes the CTranslator
- [CLANG.cpp](compiler/backend/CLANG.cpp) is the file that includes some translation code

### chemical.mod to build.lab translation
- [ModToLabConverter.h](compiler/lab/mod_conv/ModToLabConverter.h)
- [ModToLabConverter.cpp](compiler/lab/mod_conv/ModToLabConverter.cpp)

### Name Mangling
- [NameMangler.h](compiler/mangler/NameMangler.h)
- [NameMangler.cpp](compiler/mangler/NameMangler.cpp)

### LSP
- The entry point of the LSP server is [Main.cpp](/Main.cpp) file available at the root of the project
- [server](server) This contains code related to `LSP` server which provides `IDEs` semantic tokens / completion items and stuff.
  - [WorkspaceManager](server/WorkspaceManager.h) The overseer of the IDE session in LSP, when you open IDE and edit files, this file tracks changes and launches operations and manages everything.
  - The following analyzers are used by LSP
    - [CompletionItemAnalyzer](server/analyzers/CompletionItemAnalyzer.h) The class that provides completion items to `IDEs` when you press `ctrl + space`
    - [FoldingRangeAnalyzer](server/analyzers/FoldingRangeAnalyzer.h) The class that provides folding ranges to `IDEs`
    - [DocumentSymbolsAnalyzer](server/analyzers/DocumentSymbolsAnalyzer.h) The class that provides symbols to `IDEs` (viewed inside outline panel in vscode, bottom left)
    - [GotoDefAnalyzer](server/analyzers/GotoDefAnalyzer.h) The class provides goto definition support to `IDEs` for symbols.
    - [HoverAnalyzer](server/analyzers/HoverAnalyzer.h) when you hover over a symbol in `IDE`, This class provides information about popup to show.
    - [SemanticTokensAnalyzer](server/analyzers/SemanticTokensAnalyzer.h) The class that provides syntax highlighting to `IDEs`.