---
name: Compiler API
description:
    Documentation of API of the compiler, and compiler bindings library present in lang/libs/compiler
---

- `ASTNode` is the base class for a statement or declaration
- `Value` is the base class for values
- `BaseType` is the base class for types

where compiler requires a `Value` you can't pass a `ASTNode` and vice versa.
types as well can't be passed for values or nodes.

Values can be stored inside nodes. Types as well. Nodes can also be stored in values and types but that's rare.

Look for their definitions in `ast/base` directory, Definition of different ast structures is present in 
`ast/structures` and `ast/statements`

One of the very interesting library is compiler in the `lang/libs/compiler` directory

The library is very simple, but provides structs that allow you to access compiler APIs

These include (some important bindings)

- BatchAllocator
- ASTBuilder
- SourceProvider
- Lexer
- Parser
- SymbolResolver

to name a few.

## Related Skills

- **CBI Plugin API** (`.agents/skills/cbi_plugin_api/SKILL.md`) — How to use the compiler API bindings in a plugin, ASTBuilder patterns, registration
- **Macro Codegen** (`.agents/skills/macro_code_gen/SKILL.md`) — How the macro plugins (html_cbi, universal_cbi, etc.) use the compiler API for code generation
- **Parser Internals** (`.agents/skills/parser_internals/SKILL.md`) — How the parser creates AST nodes that the compiler API works with