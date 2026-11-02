# Tuples (first implementation slice)

Status: implemented for construction, field access, parameters/returns,
nested tuples, element assignment, `Copy`, auto traits, drop-once semantics,
flat tuple destructuring (`var (a, b) = value`), named tuple structs
(`struct Point(int, int)`), and the unit tuple `()`.

## Syntax

```chemical
var pair = (1, true)                 // tuple literal
var typed : (int, bool) = pair       // tuple type
var first = pair._0                  // element access
var second = pair._1
pair._0 = 9                          // element assignment

var (a, b) = pair                    // flat destructuring
var (only,) = (1,)                   // one-element tuple pattern
const (x, y) = pair                  // const bindings

func make_pair(a : int, b : bool) : (int, bool) {
    return (a, b)
}

func consume(pair : (int, int)) : int {
    return pair._0 + pair._1
}

struct Point(int, int)               // named tuple struct
struct Pair<T>(T, T)                 // generic tuple struct
struct Unit()                        // zero-field tuple struct

var point = Point(3, 4)              // positional constructor
var generic = Pair<int>(5, 6)
var (x, y) = point                   // tuple-struct destructuring
var unit : () = ()                   // unit tuple value/type
```

- `(T)` is a parenthesized type, not a one-element tuple.
- `(T,)` is a one-element tuple.
- `(x)` is a parenthesized expression, not a one-element tuple.
- `(x,)` is a one-element tuple value.
- `()` is the unit tuple type/value. A function with no explicit return type
  still uses `void`.
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

A named tuple struct is a normal `StructDefinition` whose fields are `_0`,
`_1`, ... and whose generated positional constructor returns the struct. It is
therefore nominal (unlike a structural tuple), participates in the normal
constructor/destructor/auto-trait paths, and supports generic instantiation.
Tuple struct fields are currently always public; per-field visibility is not
part of this slice.

## Destructuring

`var (a, b) = value` (and `const (a, b) = value`) binds each element to a new
local. The value is evaluated once and moved/copied into an owned local
snapshot before the elements are bound, so mutating the source afterwards does
not change the bindings. Destructible elements are moved with their per-field
drop flags cleared, so each owned element is dropped exactly once. A bare
one-element tuple uses a trailing comma: `var (only,) = value`.

Tuple patterns are irrefutable: they do not take an `else` branch. They work
for structural tuples and named tuple structs. The current slice supports flat
identifier elements only; nested patterns (`var ((a, b), c)`) and wildcard
elements (`var (_, b)`) are not implemented yet. `if(var (a, b) = value)` is
also not part of this slice.

## Type facts

- Structural equality: `(int, bool)` equals another `(int, bool)`.
- A tuple is `Copy` when every element is `Copy`.
- `Send`/`Sync`/`Unpin` are derived from every element.
- A tuple with destructible elements is dropped field-wise, exactly once.
- Tuple values can be passed and returned by value; aggregate ABI rules apply.
- A named tuple struct follows the normal struct facts: nominal equality,
  field-wise `Copy`/`Send`/`Sync`/`Unpin`, generated destructor when needed,
  and the standard aggregate ABI.

## Not implemented

- Nested tuple patterns, wildcard elements, and tuple patterns in `if`/match
  position.
- Tuple-struct field visibility and methods/inherent impls written inline after
  the field list.
- `impl Trait for (T1, T2)` / methods on structural tuples.
- Slices, `&str`, const generics, GATs, and `impl Trait` are separate M1 work.
