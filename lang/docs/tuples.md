# Tuples (first implementation slice)

Status: implemented for construction, field access, parameters/returns,
nested tuples, element assignment, `Copy`, auto traits, and drop-once
semantics. Tuple structs and destructuring patterns are not implemented yet.

## Syntax

```chemical
var pair = (1, true)                 // tuple literal
var typed : (int, bool) = pair       // tuple type
var first = pair._0                  // element access
var second = pair._1
pair._0 = 9                          // element assignment

func make_pair(a : int, b : bool) : (int, bool) {
    return (a, b)
}

func consume(pair : (int, int)) : int {
    return pair._0 + pair._1
}
```

- `(T)` is a parenthesized type, not a one-element tuple.
- `(T,)` is a one-element tuple.
- `(x)` is a parenthesized expression, not a one-element tuple.
- `(x,)` is a one-element tuple value.
- `()` is currently treated as the void type; a unit tuple value is not
  implemented.
- Nested tuples work: `((1, 2), (3, 4))._0._1`.
- `(a, b) => ...` remains a lambda; a tuple literal is distinguished by the
  absence of a following `=>`.

## Representation

A tuple is a structural `StructType` with generated members `_0`, `_1`, ... .
This lets the existing aggregate machinery handle layout, field stores, moves,
and drops. MIR interns a tuple aggregate by its element type ids and gives it a
structural C tag (`__chx_tuple_<hash>`), so a tuple type used in a prototype and
in a body produces the same C tag.

The generated C fields are `_0`, `_1`, ...; the source spells them `.0`, `.1`,
and the parser maps the numeric access to the generated member name.

## Type facts

- Structural equality: `(int, bool)` equals another `(int, bool)`.
- A tuple is `Copy` when every element is `Copy`.
- `Send`/`Sync`/`Unpin` are derived from every element.
- A tuple with destructible elements is dropped field-wise, exactly once.
- Tuple values can be passed and returned by value; aggregate ABI rules apply.

## Not implemented

- Tuple destructuring (`var (a, b) = value`) and match patterns.
- Tuple structs (`struct Point(int, int)`).
- A real unit tuple `()` value.
- `impl Trait for (T1, T2)` / named tuple methods.
- Slices, `&str`, const generics, GATs, and `impl Trait` are separate M1 work.
