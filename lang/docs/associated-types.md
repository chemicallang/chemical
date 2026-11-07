# Associated types — REMOVED

Associated types (declarations, impl bindings, `Self::Item`/`T::Item`
projections, associated bounds, and equality constraints) were removed in
safety-sharpen Phase 1. They had no shipped-library users and carried a large
generic-instantiation surface.

Use an ordinary generic parameter instead:

```chemical
interface HasValue<T> {
    func value(&self) : T
}
```

The compiler reports `REMOVED-001` for the removed syntax. See
`lang/docs/safety-sharpen/REMOVED.md`.
