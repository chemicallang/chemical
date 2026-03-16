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

Look for their definitions in `ast/base` directory