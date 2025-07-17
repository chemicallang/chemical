## Project Structure

Here's a rough overview of the structure of the project

### Tests

These tests are written in `chemical`, We aren't testing C++ at the moment, There's no testing framework
for our language as well, so as long as that's the case, we are going to compile all tests to a single executable and run that executable
to get the output

Here's all the [tests](../tests)

After performing a change in the compiler implementation, one must verify tests are succeeding, for which
there are commands (if you are using CLion or Jetbrains stuff, run configurations would be available)

Otherwise, you can just run by providing `chemical compiler` path to the `build.lab` file

### AST Classes

- [ast](../../ast) (folder contains models for the syntax tree)
  - [ast/base](../../ast/base) is the folder containing base classes
  - [ASTNode](../../ast/base/ASTNode.h) class that is inherited by all the nodes (`WhileLoop`, `VarInit`, `ReturnStatement`)
  - [Value](../../ast/base/Value.h) class that represents all the values (referenced, primitive & others like Array and Struct)
  - [BaseType](../../ast/base/BaseType.h) class that represents all the type classes in the project like `Int` and `ReferencedType`
  - [This directory](../../ast/statements) Holds some of the statements
  - [This directory](../../ast/structures) Holds most of the declarations


### Lexer & Parser
- [stream](../../stream) A single class defines the methods that are used to read from a `std::ifstream`, It's used by the `lexer` to read source code.
- [lexer](../../lexer) The directory contains the lexer implementation
  -  [Lexer.h](../../lexer/Lexer.h) to get an idea of the API
- [parser](../../parser) The directory contains the parser implementation
  - [Parser.cpp](../../parser/Parser.h) to get an idea of the API
  - Parser implementation is divided in different files, files are also wrongly named Lex
  - Early version used to convert to a CST, which has been removed now, that's why.


### Symbol Resolver
- Symbol resolver has two jobs, one of which is to resolve symbols, and the other to figure out `type` of everything
- Symbol Resolver is present in [this directory](../../compiler/symres)


### Compile using LLVM
- [compiler](../../compiler/backend/LLVM.cpp)
- [class Codegen](../../compiler/Codegen.h)
- I organized llvm stuff into multiple files, however I'm changing that it'll take time.


### Translation To C
- [preprocess](../../preprocess)
  - This needs to be shifted to compiler directory
  - [2cASTVisitor](../../preprocess/2c/2cASTVisitor.h) The class that converts Chemical `AST` to `C`


### LSP
- The entry point of the LSP server is [Main.cpp](/Main.cpp) file available at the root of the project
- [server](../../server) This contains code related to `LSP` server which provides `IDEs` semantic tokens / completion items and stuff.
  - [WorkspaceManager](../../server/WorkspaceManager.h) The overseer of the IDE session in LSP, when you open IDE and edit files, this file tracks changes and launches operations and manages everything.
  - The following analyzers are used by LSP
    - [CompletionItemAnalyzer](../../server/analyzers/CompletionItemAnalyzer.h) The class that provides completion items to `IDEs` when you press `ctrl + space`
    - [FoldingRangeAnalyzer](../../server/analyzers/FoldingRangeAnalyzer.h) The class that provides folding ranges to `IDEs`
    - [DocumentSymbolsAnalyzer](../../server/analyzers/DocumentSymbolsAnalyzer.h) The class that provides symbols to `IDEs` (viewed inside outline panel in vscode, bottom left)
    - [GotoDefAnalyzer](../../server/analyzers/GotoDefAnalyzer.h) The class provides goto definition support to `IDEs` for symbols.
    - [HoverAnalyzer](../../server/analyzers/HoverAnalyzer.h) when you hover over a symbol in `IDE`, This class provides information about popup to show.
    - [SemanticTokensAnalyzer](../../server/analyzers/SemanticTokensAnalyzer.h) The class that provides syntax highlighting to `IDEs`.