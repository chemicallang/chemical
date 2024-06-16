## Rules

1 - Check that `lang/tests/tests.ch` compiles and run's with all tests successful
2 - Check that `lang/tests/tests.ch` can be translated to C and tests are still successful

## Project Structure

Here are the folders in the project and their explanation

- [ast](../../ast) (folder contains models for the syntax tree)
  - [ast/base](../../ast/base) is the folder containing base classes
  - [ASTNode](../../ast/base/ASTNode.h) class that is inherited by all the nodes (`WhileLoop`, `VarInit`, `ReturnStatement`)
  - [Value](../../ast/base/Value.h) class that represents all the values (referenced, primitive & others like Array and Struct)
  - [BaseType](../../ast/base/BaseType.h) class that represents all the type classes in the project like `Int` and `ReferencedType`
  - [Visitor](../../ast/base/Visitor.h) class that allows visiting all nodes in the AST
    - By default, visitor doesn't visit values, suppose you want to process lambda functions in the `AST`, since lambda's are values, they won't be visited, unless you visit the `VarInitStatement` which contains `std::optional<std::unique_ptr<Value>>` value
    - To visit the values / types, there's a [CommonVisitor](../../ast/utils/CommonVisitor.h) which implements the Visitor class


- [stream](../../stream) A single class defines the methods that are used to read from a `std::ifstream`, It's used by the `lexer` to read source code.


- [lexer](../../lexer) The folder contains the lexer implementation in different files, These
files contain functions for lexing different nodes / values / types.
  - You'd learn the lexer pretty fast by just visiting the source files present in the folders inside lexer folder.
  - The lexer creates a hierarchy of tokens, YES, It creates a CST straight from the source code. which is then converted to a AST using [CSTConvert](../../cst/CSTConvert.cpp)
  - Checkout [Lexer](../../lexer/Lexer.h) for all functions that lexer has. 


- [cst](../../cst) The folder cst contains models for CST, As I've explained earlier A CST is created by the lexer instead of an AST.
  - The cst implementation is very easy and that's why you should read lexer source.


    - May change by removing compound tokens classes with a single struct, to allocate at-least 1000 tokens on stack.

- [preprocess](../../preprocess) These are preprocessors on `CST` or `AST`
  - [2cASTVisitor](../../preprocess/2cASTVisitor.h) The class that converts Chemical `AST` to `C`
  - [CSTSymbolResolver](../../preprocess/CSTSymbolResolver.h) The class traverses the `CST` to link referenced variables and types
  - [ImportGraphMaker](../../preprocess/ImportGraphMaker.h) The class visits the `CST` to quickly create an import graph (which files depend on which) so we compile the independent files first
  - [ImportPathHandler](../../preprocess/ImportPathHandler.h) If user imports C headers, this class resolves the path to those system headers
  - [ToCTranslator](../../preprocess/ToCTranslator.h) It uses the `2cASTVisitor` to convert the `AST` to `C`


- [compiler](../../compiler) the compiler implementation, however please note that I've put llvm stuff into a lot files to be very organized to avoid errors, of course it takes some time to build the project.
    - In the future the compiler will be `AST` visitor


- [server](../../server) This contains code related to `LSP` server which provides `IDEs` semantic tokens / completion items and stuff.
  - [WorkspaceManager](../../server/WorkspaceManager.h) The overseer of the IDE session in LSP, when you open IDE and edit files, this file tracks changes and launches operations and manages everything.
  - The following `CST` analyzers are used by LSP
    - [CompletionItemAnalyzer](../../server/analyzers/CompletionItemAnalyzer.h) The class that provides completion items to `IDEs` when you press `ctrl + space`
    - [FoldingRangeAnalyzer](../../server/analyzers/FoldingRangeAnalyzer.h) The class that provides folding ranges to `IDEs`
    - [DocumentSymbolsAnalyzer](../../server/analyzers/DocumentSymbolsAnalyzer.h) The class that provides symbols to `IDEs` (viewed inside outline panel in vscode, bottom left)
    - [GotoDefAnalyzer](../../server/analyzers/GotoDefAnalyzer.h) The class provides goto definition support to `IDEs` for symbols.
    - [HoverAnalyzer](../../server/analyzers/HoverAnalyzer.h) when you hover over a symbol in `IDE`, This class provides information about popup to show.
    - [SemanticTokensAnalyzer](../../server/analyzers/SemanticTokensAnalyzer.h) The class that provides syntax highlighting to `IDEs`.


- [lang](../../lang) This folder contains source code written in our own programming language, tests, assets and docs related to our programming language.