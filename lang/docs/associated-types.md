# Associated types (first slice)

Status: interface associated-type **declarations**, impl **bindings**, and
completeness checking are implemented. Return-position `Self::Item` is
projected for static dispatch through an interface implementation. Parameter
projections, `T::Item` generic projections, associated-type bounds/equality
constraints, and generic associated types (GATs) are not implemented yet.

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
- `Self::Item` in a parameter position is rejected: a placeholder parameter
  would be emitted as `void` in the interface prototype. Use the concrete type
  in the interface signature for now.
- `T::Item` projections through a generic bound (`T: HasItem`) are not
  implemented. Generic/interface dispatch and `dyn HasItem` are limited;
  interfaces declaring associated types are marked non-object-safe.
- Associated-type bounds (`type Item: Copy`) and equality constraints
  (`T: Iterator<Item = int>`) are not parsed.
- Generic associated types (`type Cursor<'a>`) are not supported beyond the
  declaration being rejected/ignored by the projection layer.
- Inherited interfaces must be implemented separately; their associated types
  are checked by their own implementation.

Projection through generic bounds and parameter-position associated types are
the next associated-type milestones.
