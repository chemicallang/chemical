# Associated types (first slice)

Status: interface associated-type **declarations** and impl **bindings** are
implemented, with completeness checking. Projections (`Self::Item`, `T::Item`),
associated-type bounds/equality constraints, and generic associated types (GATs)
are not implemented yet.

## Syntax

```chemical
interface HasItem {
    type Item
    func value(&self) : int
}

struct Counter {
    var n : int
}

impl HasItem for Counter {
    type Item = int
    func value(&self) : int { return self.n }
}
```

- `type Item` is only valid directly inside an interface (or generic
  interface). It declares a contract.
- `type Item = T` inside an implementation supplies the concrete type.
- Every interface associated-type declaration must have a matching binding in
  each implementation. Missing bindings are a compile error:
  `type does not implement associated type 'Item'`.

## Limits

- The declaration is a contract, not a usable type expression. `Item` or
  `Self::Item` in a method signature is not projected to the implementation's
  binding yet.
- Associated-type bounds (`type Item: Copy`) and equality constraints
  (`T: Iterator<Item = int>`) are not parsed.
- Generic associated types (`type Cursor<'a>`) are not supported beyond the
  declaration being rejected/ignored by the projection layer.
- Inherited interfaces must be implemented separately; their associated types
  are checked by their own implementation.

The current slice is deliberately small: it establishes the parser, AST flag,
and implementation-completeness contract without changing the type system's
projection rules. Projections are the next associated-type milestone.
