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
| `parser/structures/` | Per-construct parsers (one file per AST construct) |
| `parser/values/` | Value parsers (expressions, literals, etc.) |
| `parser/statements/` | Statement parsers |
| `parser/utils/` | Parsing utilities |

### Per-Construct Parser Files

| File | Construct |
|------|-----------|
| `parser/structures/Function.cpp` | Function declarations |
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
| `parser/values/StructValue.cpp` | Struct literal values |
| `parser/values/LexValue.cpp` | Lexed values (integers, floats, strings) |
| `parser/values/Expression.cpp` | Binary/unary expressions |
| `parser/utils/Helpers.cpp` | Helper functions |
| `parser/statements/Import.cpp` | Import statements |
| `parser/statements/Export.cpp` | Export statements |
| `parser/statements/VarInitialization.cpp` | Variable initialization |
| `parser/statements/Typealias.cpp` | Type alias statements |
| `parser/statements/AccessChain.cpp` | Access chain parsing (a.b.c) |
| `parser/statements/AnnotationMacro.cpp` | Annotation/macro parsing |

## Parser State

The Parser class holds:

```cpp
class Parser {
    Lexer& lexer;                           // Token source
    ASTAllocator& allocator;                 // Arena allocator for AST nodes
    ASTDiagnoser& diagnoser;                 // Error reporting
    Token current;                           // Current token
    Token peeked;                            // Lookahead token (optional)
    bool has_peeked;                         // Whether peeked token is valid
    
    // Methods:
    Token& consume();                        // Advance to next token
    Token& peek();                           // Look at next token without consuming
    bool consume_if(TokenType type);         // Consume if matches type
    bool expect(TokenType type);             // Expect specific type or error
    
    // Entry points:
    Scope* parse_scope();                    // Parse a { } scope
    ASTNode* parse_declaration();            // Parse a top-level declaration
    Value* parse_expression();               // Parse an expression
    Value* parse_value();                    // Parse a value
    BaseType* parse_type();                  // Parse a type
};
```

## Recursive Descent Pattern

The parser uses standard recursive descent with one-token lookahead:

```cpp
// Example: parsing a function declaration
FunctionDeclaration* Parser::parse_func_decl() {
    // 1. Check for 'func' keyword
    if(!consume_if(TokenType::FuncKw)) return nullptr;
    
    // 2. Parse function name
    auto name = expect_identifier();
    
    // 3. Parse generic parameters (optional)
    std::vector<GenericTypeParameter*> generic_params;
    if(consume_if(TokenType::LessThan)) {
        generic_params = parse_generic_params();
        expect(TokenType::GreaterThan);
    }
    
    // 4. Parse parameters
    expect(TokenType::OpenParen);
    auto params = parse_params();
    expect(TokenType::CloseParen);
    
    // 5. Parse return type (optional)
    BaseType* return_type = nullptr;
    if(consume_if(TokenType::Colon)) {
        return_type = parse_type();
    }
    
    // 6. Parse function body
    Scope* body = nullptr;
    if(current.type == TokenType::OpenBrace) {
        body = parse_scope();
    } else if(consume_if(TokenType::EqualsGreater)) {
        // Lambda-style: () => expr
        body = parse_lambda_body();
    }
    
    // 7. Create AST node
    return create_func_decl(name, params, return_type, body, generic_params);
}
```

## Parsing Rules by Construct

### Expressions

Expressions are parsed with precedence climbing:

```cpp
// Precedence levels (lowest to highest):
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
Unary         (!, -, ~, &, *, ++, --)
Postfix       (), [], ., ?,
```

### Types

Types are parsed with a recursive type parser:

```cpp
BaseType* Parser::parse_type() {
    // Handle pointer types: *int, **int, *mut int
    if(consume_if(TokenType::Star)) {
        auto mutable_ = consume_if(TokenType::MutKw);
        auto inner = parse_type();
        return allocator.create<PointerType>(inner, mutable_);
    }
    
    // Handle reference types: &int, &mut int
    if(consume_if(TokenType::Ampersand)) {
        auto mutable_ = consume_if(TokenType::MutKw);
        auto inner = parse_type();
        return allocator.create<ReferenceType>(inner, mutable_);
    }
    
    // Handle array types: [10]int
    if(consume_if(TokenType::OpenBracket)) {
        auto size = parse_expression();
        expect(TokenType::CloseBracket);
        auto inner = parse_type();
        return allocator.create<ArrayType>(inner, size);
    }
    
    // Handle function types: (int) => bool
    if(consume_if(TokenType::OpenParen)) {
        return parse_function_type();
    }
    
    // Handle named types: int, string, MyStruct, GenericType<int>
    return parse_named_type();
}
```

### Statements

Statement parsing dispatches based on the first token:

| Token | Statement |
|-------|-----------|
| `var` / `const` | Variable declaration |
| `if` | If statement |
| `while` | While loop |
| `do` | Do-while loop |
| `for` | For loop |
| `switch` | Switch statement |
| `return` | Return statement |
| `break` | Break statement |
| `continue` | Continue statement |
| `unsafe` | Unsafe block |
| `comptime` | Comptime block |
| `{` | Scope block |
| `import` | Import statement |
| `export` | Export statement |
| `typealias` | Type alias |
| `throw` | Throw statement |
| `delete` | Delete statement |
| `dealloc` | Dealloc statement |
| `@` | Annotation/macro |
| `func` | Function |
| `struct` | Struct |
| `variant` | Variant |
| `union` | Union |
| `enum` | Enum |
| `interface` | Interface |
| `impl` | Implementation |
| `namespace` | Namespace |
| `using` | Using declaration |
| Other | Expression statement |

### Error Recovery

The parser uses basic error recovery:

1. **Missing semicolons**: If a statement doesn't start with a known keyword, the parser tries to parse an expression statement and expects a semicolon
2. **Unexpected tokens**: When a token is unexpected, the parser skips to the next semicolon or closing brace
3. **Missing closing brackets**: Reports an error but continues parsing (assuming the closing bracket)
4. **Comptime if**: Both branches are parsed for syntax, but only the selected branch is type-checked

### The `@` Annotation Macro System

The `@` prefix triggers annotation parsing, which can either:

1. **Built-in annotations**: `@extern`, `@test`, `@deprecated`, `@no_mangle`, `@inline`, `@noinline`, etc.
2. **Plugin annotations**: Any `@name` that isn't a built-in is forwarded to CBI plugin handlers

```cpp
void Parser::parse_annotation() {
    consume();  // consume @
    auto name = expect_identifier();
    
    // Check built-in annotations
    if(name == "extern") { /* handle @extern */ }
    else if(name == "test") { /* handle @test */ }
    // ... built-in annotations ...
    else {
        // Forward to CBI plugin
        binder.dispatch_annotation(name, ...);
    }
}
```

## Lexer Integration

The lexer is in `lexer/Lexer.cpp` and produces tokens consumed by the parser:

```cpp
class Lexer {
    InputSource& source;       // Source file
    Token current;             // Current token
    SourceLocation location;   // Current location
    
    Token next();              // Get next token
    Token peek();              // Lookahead
};
```

### Token Types

| Token Type | Example |
|------------|---------|
| `Identifier` | `foo`, `Bar`, `_baz` |
| `IntegerLiteral` | `42`, `0xFF`, `0b1010` |
| `FloatLiteral` | `3.14`, `1e10` |
| `StringLiteral` | `"hello"`, `"\"escaped\""` |
| `CharLiteral` | `'a'`, `'\n'` |
| `FuncKw` | `func` |
| `StructKw` | `struct` |
| `VarKw` | `var` |
| `ConstKw` | `const` |
| `OpenParen` | `(` |
| `CloseParen` | `)` |
| `OpenBrace` | `{` |
| `CloseBrace` | `}` |
| `OpenBracket` | `[` |
| `CloseBracket` | `]` |
| `Semicolon` | `;` |
| `Colon` | `:` |
| `EqualsGreater` | `=>` |
| `Ampersand` | `&` |
| `Star` | `*` |
| `Arrow` | `->` |
| `Dot` | `.` |
| `Comma` | `,` |
| ... | ... |

## AST Allocator

All AST nodes are allocated through `ASTAllocator`, an arena allocator:

```cpp
class ASTAllocator {
    void* allocate(size_t size);     // Arena allocation — no individual free
    void reset();                     // Reset entire arena
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
| Ambiguous grammar | Expression vs. declaration confusion | Use `consume_if()` with careful token checking |
| Left recursion | Direct/indirect left-recursive rules | Rewrite as iteration (e.g., `a + b + c`) |
| Operator precedence | Wrong binding strength | Use precedence climbing or Pratt parsing |
| Lookahead limit | Need >1 token to disambiguate | Buffer tokens or use backtracking |
| Generic parsing | `<` vs. less-than ambiguity | Use context tracking to distinguish |
