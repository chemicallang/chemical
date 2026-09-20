---
name: Parser Internals
description: Comprehensive guide to the Chemical recursive descent parser — how Chemical source code is parsed into AST nodes.
---

# Parser Internals

The Chemical parser is a hand-written recursive descent parser. It takes a stream of tokens from the lexer and produces an AST (Abstract Syntax Tree).

## Architecture

### Pipeline

```
Source code → Lexer → Token stream → Parser → AST
```

### Key Files

| File | Purpose |
|------|---------|
| `parser/Parser.h` | Parser class declaration — all parsing method declarations |
| `parser/Parser.cpp` | Main parser implementation — orchestration and entry points |
| `lexer/Lexer.h` | Lexer class — tokenizer |
| `lexer/Lexer.cpp` | Lexer implementation |
| `lexer/Token.h` | Token structure and types |
| `lexer/TokenType.h` | Token type enum |
| `lexer/LexUnit.h` | Lexed token unit (whole-file token buffer) |
| `parser/structures/` | Per-construct parsers (functions, structs, control flow, etc.) |
| `parser/values/` | Lambda and struct-value parsers (`LambdaValue.cpp`, `StructValue.cpp`) |
| `parser/statements/` | Statement parsers (dispatch, imports, assignments, annotations, etc.) |
| `parser/utils/` | Expression, literal and helper parsing (`Expression.cpp`, `LexValue.cpp`, `Helpers.cpp`) |

### Per-Construct Parser Files

| File | Construct |
|------|-----------|
| `parser/structures/Function.cpp` | Function declarations + **inline asm parsing** (`parseInlineAsmStatement`) |
| `parser/structures/Struct.cpp` | Struct definitions |
| `parser/structures/Variant.cpp` | Variant definitions |
| `parser/structures/Enum.cpp` | Enum definitions |
| `parser/structures/Interface.cpp` | Interface definitions |
| `parser/structures/ImplDef.cpp` | Implementation blocks (`impl`) |
| `parser/structures/Namespace.cpp` | Namespace declarations |
| `parser/structures/Block.cpp` | Block scopes (`{ }`) |
| `parser/structures/IfBlock.cpp` | If/else statements |
| `parser/structures/WhileBlock.cpp` | While loops |
| `parser/structures/DoWhile.cpp` | Do-while loops |
| `parser/structures/ForBlock.cpp` | For loops |
| `parser/structures/Switch.cpp` | Switch statements |
| `parser/structures/TryCatch.cpp` | Try/catch blocks |
| `parser/structures/Union.cpp` | Union definitions |
| `parser/values/LambdaValue.cpp` | Lambda function values |
| `parser/values/StructValue.cpp` | Struct literal values |
| `parser/utils/LexValue.cpp` | Lexed values (integers, floats, strings, chars) |
| `parser/utils/Expression.cpp` | Binary/unary expression parsing (shunting-yard) |
| `parser/utils/Helpers.cpp` | Helper functions (access specifiers, generics lookahead) |
| `parser/statements/LexStatement.cpp` | Statement dispatch (top-level & nested) |
| `parser/statements/LexAssignment.cpp` | Assignment statements |
| `parser/statements/Import.cpp` | Import statements |
| `parser/statements/Export.cpp` | Export statements |
| `parser/statements/VarInitialization.cpp` | Variable initialization |
| `parser/statements/Typealias.cpp` | Type alias statements |
| `parser/statements/AccessChain.cpp` | Access chain parsing (a.b.c) |
| `parser/statements/AnnotationMacro.cpp` | Annotation/macro parsing |
| `parser/statements/OptionStmt.cpp` | `option` statements in `.mod` files |

## Parser State

The Parser class holds:

```cpp
class BasicParser : public ASTDiagnoser {
    unsigned int file_id;                    // file id for the location manager
    Token* token;                            // pointer to the current token
    ASTNode* parent_node = nullptr;          // current parent node

    // Token consumption helpers:
    void consumeAny();                       // advance to the next token
    bool consumeToken(TokenType type);       // advance if the current token matches
    Token* consumeOfType(TokenType type);    // like consumeToken, but returns the token
    Token* consumeIdentifier();              // consume strictly an Identifier
    Token* consumeIdentifierOrKeyword();     // consume an Identifier or a keyword
    void consumeNewLines();                   // skip NewLine tokens
};

class Parser : public BasicParser {
    CompilerBinder* const binder;            // CBI binder (nullptr when disabled)
    AnnotationController& controller;        // annotation definitions and handlers
    ASTAllocator& global_allocator;          // global (job) arena allocator
    ASTAllocator& mod_allocator;             // module arena allocator
    TypeBuilder& typeBuilder;                // cached type builder
    bool is64Bit;                            // target bitness
    int pending_greater_count = 0;           // handles '>>' inside nested generics
    std::vector<SavedAnnotation> annotations;// annotations awaiting the next node

    // Entry points / dispatch:
    void parse(std::vector<ASTNode*>& nodes);                       // parse all top-level nodes
    ASTNode* parseTopLevelStatement(ASTAllocator&, bool comptime);
    ASTNode* parseNestedLevelStatementTokens(ASTAllocator&, bool is_value = false, bool parse_value_node = false);
    Value* parseExpression(ASTAllocator&, bool parseStruct = false, bool parseLambda = true);
    TypeLoc parseTypeLoc(ASTAllocator&);
};
```

Note: the parser is not a separate token-stream class — the header is tokenized into a `LexUnit`/`std::vector<Token>` up front and the Parser walks a `Token*` pointer through it (with `isGenericEndAhead()` for limited lookahead).

## Recursive Descent Pattern

The parser walks a `Token*` through the pre-lexed token vector with a small amount of lookahead (`isGenericEndAhead()` disambiguates `<` as generic vs. less-than):

### `ident<...>` — generic arguments attach to three different places

In `parseAccessChainAfterId`'s `LessThanSym` arm, after `parseGenericArgsListNoStart` has read
the arguments, the **next token decides where they go**:

| Followed by | Node | Holds the arguments |
|---|---|---|
| `{` | `StructValue` | the `GenericType` reference |
| `(` | `FunctionCall` | `FunctionCall::generic_list` |
| anything else | `GenericInstIdentifier` wrapping the last identifier | `GenericInstIdentifier::generic_list` |

The third case is the *bare reference* `var f : (x : int) => int = ident<int>`, and it is the
only case that must survive into symbol resolution as a value rather than as a type — see the
[Generics skill](../generics/SKILL.md#generic-function-references-identint-as-a-value). It
replaces `chain->values.back()`, so the chain's leaf is **not** a `VariableIdentifier`; code
that consumes chain leaves must unwrap via `Value::as_identifier_of(...)`.

```cpp
// Simplified illustration — the real entry point is
// Parser::parseFunctionStructureTokens in parser/structures/Function.cpp
ASTNode* Parser::parseFunctionStructureTokens(ASTAllocator& allocator, ASTAllocator& body_allocator,
                                              AccessSpecifier specifier, bool allow_extensions, bool comptime) {
    // 1. Check for 'func' keyword
    if(!consumeToken(TokenType::FuncKw)) return nullptr;

    // 2. Parse function name
    auto* name = consumeIdentifierOrKeyword();

    // 3. Parse generic parameters (optional)
    std::vector<GenericTypeParameter*> generic_params;
    if(token->type == TokenType::LessThanSym) {
        parseGenericParametersList(allocator, generic_params);
    }

    // 4. Parse parameters
    if(!consumeToken(TokenType::LParen)) { /* error */ }
    std::vector<FunctionParam*> params;
    bool variadic = parseParameterList(allocator, params);
    consumeToken(TokenType::RParen);

    // 5. Parse return type (optional)
    if(consumeToken(TokenType::ColonSym)) {
        auto return_type = parseTypeLoc(allocator);
    }

    // 6. Parse function body (declarations may omit it)
    if(token->type == TokenType::LBrace) {
        auto body = parseBraceBlock("function", nullptr, body_allocator);
    }

    // 7. The function node is then created / annotated
    return nullptr; // (node construction omitted)
}
```

## Parsing Rules by Construct

### Expressions

Expressions are parsed with a shunting-yard algorithm (`shunting_yard_on_operator` / `Parser::parseExpressionWith` in `parser/utils/Expression.cpp`). Operator precedence is defined by `to_precedence()` in `ast/utils/Operation.cpp`; left/right associativity by `is_assoc_left_to_right()`. Higher precedence numbers bind tighter:

```cpp
// Precedence levels (lowest to highest):
Assignment    (=, +=, -=, *=, /=, ...)
Conditional   (?:)
LogicalOr     (||)
LogicalAnd    (&&)
BitwiseOr     (|)
BitwiseXor    (^)
BitwiseAnd    (&)
Equality      (==, !=)
Comparison    (<, >, <=, >=)
Shift         (<<, >>)
Additive      (+, -)
Multiplicative (*, /, %)
Unary         (!, -, ~, &, *, &raw, ++, --)
Postfix       (), [], ., ->
```

### Types

Types are parsed with a recursive type parser. `Parser::parseTypeLocNoPostOps` handles leading type constructors, and `Parser::parseArrayAndPointerTypesAfterTypeId` handles post-ops (`*`, `&`, `[]`) that follow a named type:

```cpp
// Simplified illustration — see Parser::parseTypeLocNoPostOps in parser/statements/LexType.cpp
TypeLoc Parser::parseTypeLocNoPostOps(ASTAllocator& allocator) {
    switch(token->type) {
        // Pointer types: *int, *mut int
        case TokenType::MultiplySym: {
            const auto ptrToken = token; token++;
            auto is_mutable = token->type == TokenType::MutKw;
            if(is_mutable) token++;
            return { new(allocator.allocate<PointerType>()) PointerType(parseType(allocator), is_mutable), loc_single(ptrToken) };
        }
        // Reference types: &int, &mut int
        case TokenType::AmpersandSym: {
            const auto refToken = token; token++;
            auto is_mutable = token->type == TokenType::MutKw;
            if(is_mutable) token++;
            return { new(allocator.allocate<ReferenceType>()) ReferenceType(parseType(allocator), is_mutable), loc_single(refToken) };
        }
        // Array types: [10]int
        case TokenType::LBracket: {
            const auto& t = *token; token++;
            auto size = parseExpression(allocator);
            consumeToken(TokenType::RBracket);
            auto child = parseTypeLoc(allocator);
            return { new(allocator.allocate<ArrayType>()) ArrayType(child, size), loc_single(&t) };
        }
        // Function types: (int) => bool
        case TokenType::LParen:
            return parseLambdaTypeLoc(allocator, false);
        // Named types: int, string, MyStruct, GenericType<int>
        default:
            return { parseLinkedOrGenericType(allocator), loc_single(token) };
    }
}
```

### Statements

Statement parsing dispatches on the first token. Top-level dispatch lives in `Parser::parseTopLevelStatement` / `parseTopLevelAccessSpecifiedDecl`, and nested dispatch in `Parser::parseNestedLevelStatementTokens` (both in `parser/statements/LexStatement.cpp`) — some constructs are only valid at top level:

| Token | Statement |
|-------|-----------|
| `var` / `const` | Variable declaration |
| `if` | If statement |
| `while` | While loop |
| `do` | Do-while loop |
| `for` | For loop |
| `loop` | Loop block (loop expression) |
| `switch` | Switch statement |
| `return` | Return statement |
| `break` | Break statement |
| `continue` | Continue statement |
| `unsafe` | Unsafe block |
| `comptime` | Comptime block |
| `{` | Scope block |
| `import` | Import statement |
| `export` | Export statement (top level) |
| `type` | Type alias |
| `throw` | Throw statement |
| `try` | Try/catch |
| `delete` / `destruct` | Destruct statement |
| `dealloc` | Dealloc statement |
| `new` | Placement new |
| `unreachable` | Unreachable statement |
| `alias` | Alias statement |
| `provide` | Provide statement |
| `@` | Annotation (buffered for the next node) |
| `#` | Macro node/macro value |
| `func` | Function (top level) |
| `struct` | Struct (top level) |
| `variant` | Variant (top level) |
| `union` | Union (top level) |
| `enum` | Enum (top level) |
| `interface` | Interface (top level) |
| `impl` | Implementation (top level) |
| `namespace` | Namespace (top level) |
| `using` | Using declaration |
| `asm` | Inline assembly statement |
| Other | Expression / assignment statement |

### Async / Await Parsing (contextual keywords)

`async` and `await` are **contextual** keywords: they must remain usable as
identifiers (e.g. the `async` namespace, an `async` field). The parser
distinguishes by position:

- `Token::isKeywordOrId` accepts `async`/`await` in name/path/type position;
  `Parser::consumeIdentifierOrKeyword` and `read_type_involving_token` consume
  them as identifiers. A value dispatch has an explicit `async::path`
  disambiguation so `async::block_on(...)` parses as a namespaced call.
- An `async` followed by `func` is an async function declaration
  (`parser/structures/Function.cpp`); `async` followed by a lambda introduces an
  async closure (`Parser::parseAsyncClosureValue`,
  `parser/values/LambdaValue.cpp`, called from `AccessChain.cpp`,
  `LexStatement.cpp`, `LexValue.cpp`). Async closures parse but are rejected in
  symres.
- `await expr` is an expression (`AwaitExpression`), so `var v = await f()` and
  `return await f()` parse; a bare `await f()` **statement** does not — see the
  `chemical_source` skill.

> Async closures parse here but are **not lowered** (symres diagnoses them).
> See `lang/docs/async-remaining-work.md` (item **AC**) for the fix plan.

### Error Recovery

The parser uses basic error recovery:

1. **Missing semicolons**: If a statement doesn't start with a known keyword, the parser tries to parse an expression statement and expects a semicolon
2. **Unexpected tokens**: When a token is unexpected, the parser skips to the next semicolon or closing brace
3. **Missing closing brackets**: Reports an error but continues parsing (assuming the closing bracket)
4. **Comptime if**: Both branches are parsed for syntax, but only the selected branch is type-checked

### `consumeNewLines()` — Newline Tolerance

The parser calls `consumeNewLines()` to skip `NewLine` tokens between elements. This enables newline-tolerant parsing in function signatures and asm statements. For example, in `parseInlineAsmStatement`, `consumeNewLines()` is called between operands, constraint strings, and parentheses to allow multi-line asm templates.

### Inline Assembly Parsing (`parseInlineAsmStatement`)

Located in `parser/structures/Function.cpp`. Triggered when the parser sees `TokenType::AsmKw`.

**Syntax**:
```chemical
// Simple form — template only
asm("movq %1, %0")

// Extended form — with outputs, inputs, clobbers
asm("lock cmpxchgq %3, %2\n\tsete %1"
    : "=a"(*expected), "=q"(success), "+m"(*ptr)    // outputs
    : "r"(desired), "0"(*expected)                   // inputs
    : "cc", "memory")                                // clobbers

// Skip outputs with :: (Go-style)
asm("" ::: "memory")

// Clobber-only
asm("" ::: "memory", "cc")
```

**Parsing steps**:
1. Consume `asm` keyword and `(`
2. Parse string literal as asm template
3. If `)` follows → return (simple form)
4. If `:` follows → parse output operands: `"constraint"(expr)` separated by `,`
5. If `:` follows → parse input operands (same format)
6. If `:` follows → parse clobber strings: `"memory"`, `"cc"`, etc.
7. Consume `)`

**Output operand constraint strings**:
- `"=a"(expr)` — write to `eax`/`rax`
- `"=q"(expr)` — write to any register
- `"+m"(expr)` — read/write memory operand
- `"=r"(expr)` — write to any register

**Input operand constraint strings**:
- `"r"(expr)` — value in any register
- `"0"(expr)` — same register as operand 0
- `"m"(expr)` — value in memory

The `InlineAsmStatement` AST node stores:
- `asm_template` — the template string
- `output_operands` — vector of `(constraint, expr)` pairs
- `input_operands` — vector of `(constraint, expr)` pairs
- `clobbers` — vector of clobber strings

### The `@` Annotation Macro System

The `@` prefix (`TokenType::Annotation`) triggers annotation parsing, which can either:

1. **Registered annotations**: annotations are looked up by name in the `AnnotationController` (`controller.get_definition(name)`). Built-in annotations such as `@extern`, `@test`, `@deprecated`, `@no_mangle`, `@inline`, `@noinline`, etc. register intrinsic handlers; CBI plugins register their own definitions too.
2. **Unknown annotations**: an unregistered `@name` is reported as an error (`unknown annotation found '...'`).

Annotations are buffered on the Parser (`std::vector<SavedAnnotation> annotations`) and applied to the next node by `Parser::annotate(node)`, which calls `controller.handle_annotation(...)`.

```cpp
bool Parser::parseAnnotation(ASTAllocator& allocator) {
    if(token->type != TokenType::Annotation) return false;
    const auto annot = token;
    token++;
    auto name_view = chem::string_view(annot->value.data() + 1, annot->value.size() - 1);
    auto definition = controller.get_definition(name_view);
    if(definition == nullptr) {
        error() << "unknown annotation found '" << annot->value << "'";
        return true;
    }
    annotations.emplace_back(*definition, std::vector<Value*>{});
    // ... parse optional (args) and store them on the SavedAnnotation ...
}
```

## Lexer Integration

The lexer is in `lexer/Lexer.cpp` and produces tokens consumed by the parser:

```cpp
class Lexer : public LexerState {
    SourceProvider provider;                    // reads source input
    std::string file_path;                      // file being lexed
    CompilerBinder* const binder;               // CBI binder (nullptr when disabled)
    BatchAllocator& file_allocator;             // allocator for token strings
    bool lex_whitespace = false;                // LSP: keep Whitespace tokens
    bool keep_comments = false;                 // LSP: keep comment tokens

    Token getNextToken();                       // get the next token
    void getTokens(std::vector<Token>& tokens); // lex the whole file
    void getUnit(LexUnit& outUnit);             // lex the whole file into a LexUnit
};
```

### Token Types

| Token Type | Example |
|------------|---------|
| `Identifier` | `foo`, `Bar`, `_baz` |
| `Number` | `42`, `0xFF`, `0b1010`, `3.14`, `1e10` |
| `String` | `"hello"`, `"\"escaped\""` |
| `Char` | `'a'`, `'\n'` |
| `FuncKw` | `func` |
| `StructKw` | `struct` |
| `VarKw` | `var` |
| `ConstKw` | `const` |
| `AsmKw` | `asm` |
| `LParen` | `(` |
| `RParen` | `)` |
| `LBrace` | `{` |
| `RBrace` | `}` |
| `LBracket` | `[` |
| `RBracket` | `]` |
| `SemiColonSym` | `;` |
| `ColonSym` | `:` |
| `LambdaSym` | `=>` |
| `AmpersandSym` | `&` |
| `MultiplySym` | `*` |
| `DotSym` | `.` |
| `CommaSym` | `,` |
| `Annotation` | `@extern` |
| `HashMacro` | `#html` |
| ... | ... |

## AST Allocator

All AST nodes are allocated through `ASTAllocator`, a `BatchAllocator` subclass:

```cpp
class ASTAllocator final : public BatchAllocator {
    template<typename T> T* allocate();              // Arena allocation — no individual free
    char* allocate_size(size_t size, size_t align);
    void clear();                                     // Free everything, reuse the arena
};
```

This means:
- No per-node `delete` ever called
- AST is deallocated in bulk when the compilation phase ends
- Fast allocation (pointer bump)
- Memory is contiguous, improving cache performance

## Common Parsing Issues

| Issue | Cause | Fix |
|-------|-------|-----|
| Ambiguous grammar | Expression vs. declaration confusion | Use `consumeToken()` with careful token checking |
| Left recursion | Direct/indirect left-recursive rules | Rewrite as iteration (e.g., `a + b + c`) |
| Operator precedence | Wrong binding strength | Use the shunting-yard algorithm (`to_precedence()`) |
| Lookahead limit | Need >1 token to disambiguate | Buffer tokens or use backtracking |
| Generic parsing | `<` vs. less-than ambiguity | Use context tracking to distinguish |
