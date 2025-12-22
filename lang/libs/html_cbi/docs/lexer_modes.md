# Lexer Mode Shift Exploration

This document explores how to accurately shift the lexer to "Chemical mode" for expressions that are not wrapped in braces `{}` (e.g., `if` conditions in `#html`).

## Current Implementation

Currently, the lexer switches to Chemical mode when `lb_count` (left brace count) increases and exits when it returns to the base level.

```mermaid
graph TD
    Normal["Normal Mode (HTML)"] -->|'{' detected| Chem["Chemical Mode"]
    Chem -->|'}' and lb_count == 1| Normal
    Chem -->|Nested '{'| Chem
```

## Problem: Parenthesized Expressions

For `if(condition)`, the `condition` is Chemical code but is delimited by `()` instead of `{}`.

### Option 1: Boolean Flag `paren_expression`

Enable a boolean when `if(` is detected.

**Pros:**
- Simple to implement for basic cases.

**Cons:**
- Doesn't handle nested parentheses well (e.g., `if((a + b) > 0)`).
- Needs to accurately know when the *outer* `)` is reached.

### Option 2: `paren_count` in `HtmlLexer`

Similar to `lb_count`, track `paren_count`.

**Pros:**
- Handles nested parentheses correctly.
- Robust.

**Cons:**
- Slightly more tracking in `HtmlLexer`.

## Proposed Solution: Unified Expression Tracking

We can enhance `HtmlLexer` to track both braces and parentheses.

### 1. Updated `HtmlLexer` Struct

```chemical
public struct HtmlLexer {
    // ... existing fields ...
    var lb_count : uchar
    var paren_count : uchar
    var in_paren_expr : bool
}
```

### 2. Lexer Logic (`getNextToken` in `main.ch`)

The main lexer loop should monitor `paren_count` when `in_paren_expr` is true.

```chemical
if(html.chemical_mode) {
    var nested = lexer.getEmbeddedToken();
    if(nested.type == ChemicalTokenType.LParen) {
        html.paren_count++;
    } else if(nested.type == ChemicalTokenType.RParen) {
        html.paren_count--;
        if(html.in_paren_expr && html.paren_count == 0) {
            html.other_mode = false;
            html.chemical_mode = false;
            html.in_paren_expr = false;
        }
    }
    // ... existing brace logic ...
    return nested;
}
```

### 3. Keyword Detection (`getNextToken2` in `nextToken.ch`)

When `if` is followed by `(`, we trigger Chemical mode.

```chemical
if(value.equals("if")) {
    if(provider.peek() == '(') {
        provider.readCharacter(); // consume '('
        html.other_mode = true;
        html.chemical_mode = true;
        html.in_paren_expr = true;
        html.paren_count = 1; // we already consumed one
        return Token { type: TokenType.If, ... }
    }
}
```

> [!NOTE]
> Consuming `(` in the lexer and returning `TokenType.If` might be tricky if the parser expects to see the `(`. We should return `TokenType.If` and then the NEXT token will be handled by the chemical lexer starting with the first token *after* `(`. Wait, the Chemical lexer should probably handle the `(` as well to keep `paren_count` consistent.

## Keyword Hashing in Lexer

To make keyword detection efficient and extensible, we will use `comptime_fnv1_hash` in a `switch` statement within `nextToken.ch`.

```chemical
const hash = fnv1_hash(value.data(), value.size());
switch(hash) {
    case comptime_fnv1_hash("if") => { ... }
    case comptime_fnv1_hash("else") => { ... }
    case comptime_fnv1_hash("for") => { ... }
    default => { ... }
}
```

This allows us to easily add new keywords like `for` or `while` in the future.
