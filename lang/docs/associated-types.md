# Associated types (first slice)

Status: interface associated-type **declarations**, impl **bindings**, and
completeness checking are implemented. Return-position `Self::Item` and
abstract-method parameter-position `Self::Item` are projected for static
dispatch through an interface implementation. `T::Item` generic projections,
associated-type bounds/equality constraints, default-method parameter
projections, and generic associated types (GATs) are not implemented yet.

## Syntax

```chemical
interface HasItem {
    type Item
    func value(&self) : Self::Item
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

- Return-position `Self::Item` is accepted in an interface method. A direct
  call through a concrete implementation uses that implementation's concrete
  method, so static dispatch works. The interface method signature itself still
  carries a placeholder type.
- `Self::Item` in an **abstract** interface method parameter is accepted. The
  abstract declaration has no body to lower; a concrete implementation supplies
  the real parameter type, and static dispatch reaches that implementation.
- `Self::Item` in a **default** interface method parameter is still rejected:
  the default body is lowered against the interface prototype, where the
  associated declaration is only a placeholder.
- An interface declaring an associated type, or inheriting an interface that
  declares one, is not object safe and cannot be used through `dyn` (for
  example `dyn HasItem` and `dyn Child` are rejected).
- `T::Item` projections through a generic bound (`T: HasItem`) are not
  implemented. Generic interface calls that pass the associated value through
  (for example `t.set(t.get())`) work after monomorphization, but the projection
  cannot be spelled directly.
- Associated-type bounds (`type Item: Copy`) and equality constraints
  (`T: Iterator<Item = int>`) are not parsed.
- Generic associated types (`type Cursor<'a>`) are not supported beyond the
  declaration being rejected/ignored by the projection layer.
- Inherited interfaces must be implemented separately; their associated types
  are checked by their own implementation.

`T::Item` projection through generic bounds, default-method parameter
projection, associated-type bounds/equality constraints, and GATs are the next
associated-type milestones.
