# Tuples (first implementation slice)

Status: implemented for construction, field access, parameters/returns,
nested tuples, element assignment, `Copy`, auto traits, drop-once semantics,
flat and nested tuple destructuring (`var ((a, b), c) = value`), wildcard
elements, tuple patterns in `if` conditions, named tuple structs
(`struct Point(int, int)`), inline and external inherent methods on tuple
structs (including generic tuple structs), fixed-array patterns (`var [a, b] = arr`), and the unit tuple `()`.

## Syntax

```chemical
var pair = (1, true)                 // tuple literal
var typed : (int, bool) = pair       // tuple type
var first = pair._0                  // element access
var second = pair._1
pair._0 = 9                          // element assignment

var (a, b) = pair                    // flat destructuring
var ((left, right), tail) = value    // nested destructuring
var (_, (b, c)) = value              // wildcard + nested group
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

struct PointMethods(int, int) {      // inline methods on a tuple struct
    func sum(&self) : int { return self._0 + self._1 }
}

struct PointExternal(int, int)
impl PointExternal {                 // external inherent implementation
    func product(&self) : int { return self._0 * self._1 }
}

var point = Point(3, 4)              // positional constructor
var generic = Pair<int>(5, 6)
var (x, y) = point                   // tuple-struct destructuring
var unit : () = ()                   // unit tuple value/type

var values : [4]int = [1, 2, 3, 4]
var [a, b, c, d] = values            // exact fixed-array pattern
var [first, .., last] = values       // one `..` rest marker
var [_, second, _, fourth] = values  // `_` discards
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
for structural tuples and named tuple structs. Elements may be identifiers,
wildcards (`_`), or nested tuple patterns (`var ((a, b), c)`). Every tuple
level must have matching arity; a nested position whose value is not a tuple
is rejected. `if(var (a, b) = value)` is supported for structural tuples and named tuple
structs, including nested groups.

## Fixed-array patterns

`var [a, b] = arr` destructures a fixed-size array of exactly the pattern
length. One `..` may appear anywhere to ignore a middle or tail run:
`[first, .., last]`, `[.., tail]`, `[head, ..]`, and `[..]`. `_` discards one
element without binding a local. The source is evaluated once; Copy elements
are copied into the bindings before later mutation of the source.

Copy elements are copied into the bindings. A fixed-size array held in a
mutable local (including an async-frame local) may also be destructured by
moving each bound element: each binding clears the source element’s drop flag,
and any wildcard/rest elements remain owned by the source. Non-Copy
destructuring of a `const`, parameter, temporary, array field, or borrowed/slice
view remains rejected with `SAFE-MOVE-004`. Nested non-Copy patterns are
supported for owned local arrays up to two fixed-array levels; deeper nesting
remains rejected.

Fixed arrays support flat exact/rest statement patterns only. Slice-view
patterns, condition patterns, nested patterns, and `Box<[T]>` patterns were
removed in safety-sharpen Phase 1 (`REMOVED-003`/`REMOVED-004`/`REMOVED-005`).

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

- Tuple patterns in match/switch position, nested tuple/array patterns, and
  tuple/slice patterns in `if` conditions (removed in Phase 1).
- Tuple-struct field visibility (fields are always public).
- `impl Trait for (T1, T2)` / methods on structural tuples.
- Slice-view patterns, `Box<[T]>` patterns, nested non-Copy array
  destructuring, and condition/`else` statement patterns. Flat fixed-array
  exact/rest destructuring of owned locals is supported.
- Slices, `&str`, const generics, GATs, and `impl Trait` are separate M1 work.
