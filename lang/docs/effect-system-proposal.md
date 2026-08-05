# Effect System — Syntax Comparison & Design Proposal

> **Status: Design Analysis — July 29, 2026**
>
> Comprehensive comparison of syntax options for effect tracking in Chemical.
> Evaluated against: syntax ease (IDE, readability), extensibility, parallel
> parser performance, and memory efficiency.

---

## Table of Contents

1. [Evaluation Criteria](#evaluation-criteria)
2. [Key Architectural Constraints](#key-architectural-constraints)
3. [Complete Syntax Catalog](#complete-syntax-catalog)
4. [Scoring Matrix](#scoring-matrix)
5. [Deep Analysis of Top Candidates](#deep-analysis-of-top-candidates)
6. [Recommended: Square Bracket Specifiers](#recommended-square-bracket-specifiers)
7. [Implementation Strategy](#implementation-strategy)
8. [Storage & Checking](#storage--checking)
9. [Module-Level Policy](#module-level-policy)
10. [FAQ & Open Questions](#faq--open-questions)

---

## Evaluation Criteria

| # | Criterion | What It Measures |
|---|-----------|------------------|
| R1 | Syntax Ease | Readability, IDE support (completion, hover, diagnostics under the effect keyword), visually distinct from annotations, learnability |
| R2 | Extensibility | Add new effects without parser changes, support custom/domain effects, compose and parameterize |
| R3 | Performance | Parser throughput with parallel per-file parsing, checking cost (O(1) vs traversal), no cross-file blocking at parse time |
| R4 | Memory Efficiency | Bytes per function, heap allocations, fits on existing AST fields |

---

## Key Architectural Constraints

### Parser runs per-file in parallel

Each file is parsed independently. The parser must unambiguously identify
effect syntax without knowledge of other files' symbols.

**Valid tokens at top level before `func`** (current grammar):

| Token | Currently valid before `func`? |
|-------|-------------------------------|
| `public` / `private` / `internal` | Yes — access specifiers |
| `@name` | Yes — annotations |
| `comptime` | Yes — compile-time function |
| `extern` | Yes — external linkage |
| `inline` / `noinline` | Yes |
| `static` | Yes |
| `(` | Yes (function pointer type) |
| `<` | **No** — generics come AFTER `func`: `func <T> name()` |
| `[` | **No** — not valid before `func` in any grammar rule |
| identifier (e.g. `pure`) | Contextual — depends if reserved |

**Key insight:** `<` and `[` before `func` are **unambiguously not part of
the current grammar**. The parser encountering `<pure> func` or `[pure] func`
immediately knows these are effect specifiers. Zero lookahead required.

### Effects must be visually distinct from annotations

Annotations (`@extern`, `@inline`, `@deprecated`) are **metadata** or
**behavior modifiers**. They can be added, removed, or ignored without
changing the type safety of the program.

Effects are **type-checking rules** enforced by the compiler. They restrict
what a function can do. Calling an effectful function from a restricted
context is a compile error.

These are fundamentally different concepts and should **look different**
in source code. Using `@pure` for effects conflates annotations (advisory)
with effects (enforced).

---

## Complete Syntax Catalog

### Category A: Delimited specifiers before `func`

Visual: an opening delimiter → effects → closing delimiter → `func`.

#### A1: Square brackets `[effect] func`

```chemical
[pure]
func add(x: int, y: int) : int

[unsafe, alloc]
func process() { ... }

[no(unsafe, alloc)]
func read_only() { ... }
```

**Parser:** `[` before `func` is not valid in current grammar → unambiguous.
Consume `[`, parse identifiers/comma/`]`, then expect `func`.

**R1: 9** — Clean, self-delimiting, immediately visible. C#/Java attribute
syntax is familiar. No confusion with annotations (`@`). IDE highlights
`[`..`]` as a distinct syntactic unit — effect names get their own color.

**R2: 8** — New effect = new identifier inside `[]`. No parser grammar
changes. Composable via comma. Parameterizable: `[no(unsafe, alloc)]`.

**R3: 10** — Single token `[` triggers effect-parse mode. No lookahead,
no ambiguity, no cross-file knowledge. Consumed before `func` is seen.

**R4: 10** — 2 bytes per function.

---

#### A2: Angle brackets `<effect> func` (Verse-like)

```chemical
<pure>
func add(x: int, y: int) : int

<unsafe, alloc>
func process() { ... }

<no(unsafe, alloc)>
func read_only() { ... }
```

**Parser:** `<` before `func` is not valid in current grammar. In current
Chemical, `<` after `func` means generics: `func <T> name()`. Before `func`,
`<` is unambiguous — consumes `<`, parses effects, expects `>`, then `func`.

**R1: 7** — Verse-inspired. `<>` immediately suggests "type-level constraint"
which is semantically close (effects are type-checking rules). However,
`<>` is heavily associated with generics in users' minds — seeing `<pure>`
could be momentarily confusing ("is `pure` a type?"). For Verse adopters
this is a pro.

**R2: 8** — Same as A1 — no parser grammar changes for new effects.

**R3: 10** — Same as A1 — `<` triggers effect-parse mode at top level.
Unambiguous before `func`, zero lookahead.

**R4: 10** — Same.

---

### Category B: Keyword modifiers

#### B1: Direct keywords before `func`

```chemical
pure func add(x: int, y: int) : int
unsafe alloc func process() { ... }
```

**Parser:** Must treat `pure`, `unsafe`, `alloc`, etc. as contextual keywords
(only meaningful before `func`). Requires one-token lookahead: after reading
`pure`, peek for `func`. If `func` follows → effect. If not → identifier
(could be type, variable, etc.).

**R1: 9** — Most readable. "pure function", "unsafe function" reads naturally.
Familiar from `async function` (TypeScript), `unsafe fn` (Rust). No brackets,
no delimiters. IDE coloring on the keyword.

**R2: 4** — Each new effect = new reserved word or contextual keyword.
User-defined effects impossible without keyword reservation. Parameterization
awkward: `no(unsafe, alloc) func` — now we're mixing keywords with parenthesized
args, losing the simplicity.

**R3: 8** — One-token lookahead is cheap but not zero. If `pure` is used as
a variable name elsewhere in the same file, the parser doesn't know until it
sees the next token. This is fine in practice (recursive descent does this
naturally) but adds a tiny overhead per declaration.

**R4: 10** — Same.

---

#### B2: Explicit `effect` keyword before `func`

```chemical
effect pure func add(x: int, y: int) : int
effect unsafe, alloc func process() { ... }
```

**Parser:** `effect` is a new contextual keyword. Consume `effect`, then
parse effect identifiers until `func`.

**R1: 6** — Explicit and clear, but verbose. 7 extra characters for `effect`
every time. No mainstream language uses this pattern.

**R2: 7** — `effect` keyword is a single extension point. Inside the effect
clause, identifiers are effect names — no grammar changes needed.

**R3: 8** — One-token for `effect`, then parse identifiers. Same lookahead
concern as B1 (`effect` could be a variable name elsewhere).

**R4: 10** — Same.

---

### Category C: After-`func` placement

#### C1: After params, before body

```chemical
func add(x: int, y: int) : int pure { ... }
func process() unsafe alloc { ... }
```

**Parser:** After `)` closing params, parse effects before `{`. Uses keyword
lookahead.

**R1: 6** — Buried after the parameter list. Less visible. Harder to find
when scanning a file. Especially bad when params span multiple lines.

**R2: 3** — Same keyword/reservation issues as B1.

**R3: 7** — Parser is already past params, knows it's in a declaration.
Context is clearer so less ambiguity.

**R4: 10** — Same.

---

#### C2: Effects on new line after signature

```chemical
func add(x: int, y: int) : int
[pure]
```

**Parser:** Requires dedented block or continuation marker.

**R1: 3** — Easy to miss, breaks pattern of reading left-to-right.

**R2: 8** — Same as A1 but after signature.

**R3: 8** — Needs newline-sensitivity in parser.

**R4: 10** — Same.

---

### Category D: Annotation-based (included for comparison)

```chemical
@pure
func add(x: int, y: int) : int
```

**R1: 6** — Clean syntax, but indistinguishable from metadata annotations
(`@extern`, `@inline`). Effects ARE NOT annotations — they are type-checking
enforcement. Using the same sigil conflates two different concepts. IDE would
color `@pure` the same as `@deprecated`, misleading readers about severity.

**R2: 9** — No parser changes. New effects = new annotation handler.

**R3: 10** — Parser stores as opaque metadata. Zero parse cost.

**R4: 10** — Same.

**Verdict:** Ruled out by "effects must be visually distinct from annotations"
constraint. Included as baseline for comparison.

---

### Category E: Return type modifiers

```chemical
func add(x: int, y: int) : pure int
```

**R1: 4** — Semantically incorrect: `pure` qualifies the function, not the
return type. `no_unsafe no_alloc int` — where does effect list end and type
start? Unparseable without delimiters.

**R2: 3** — Same keyword issues.

**R3: 4** — Ambiguity between effect identifiers and type identifiers.

**R4: 10** — Same.

---

### Category F: Comment-based / metadata-only

```chemical
func add(x: int, y: int) : int  // pure
```

**R1: 1** — Not enforceable by compiler. IDE can't validate.

**R2: 8** — Freeform text.

**R3: 10** — Not parsed.

**R4: 1** — Not stored in AST.

---

## Scoring Matrix

| # | Option | R1 Syntax | R2 Extensible | R3 Performant | R4 Memory | **Total** |
|---|--------|:---------:|:-------------:|:-------------:|:---------:|:---------:|
| A1 | `[pure] func` | **9** | 8 | **10** | **10** | **37** |
| A2 | `<pure> func` | 7 | 8 | **10** | **10** | 35 |
| B1 | `pure func` keyword | **9** | 4 | 8 | **10** | 31 |
| D | `@pure` annotation | 6 | **9** | **10** | **10** | 35 |
| B2 | `effect pure func` | 6 | 7 | 8 | **10** | 31 |
| C1 | After params | 6 | 3 | 7 | **10** | 26 |
| E | Return type | 4 | 3 | 4 | **10** | 21 |
| F | Comment | 1 | 8 | **10** | 1 | 20 |

### Key observations

**A1 (`[pure] func`) vs A2 (`<pure> func`):** A2 has `<>` generics association
and Verse familiarity. A1 wins on syntax ease — `[]` is more neutral, not
overloaded with "type parameter" expectation. Both are performant and
extensible.

**D (`@pure`) vs A1:** D scores higher on extensibility but loses on R1
because annotations are the wrong visual category. The user explicitly wants
effects to look different from `@` annotations.

**B1 loses on extensibility:** Adding a new effect requires reserving a new
keyword or creating a contextual keyword — both are heavy for the language.
User-defined effects are impossible.

---

## Deep Analysis of Top Candidates

### A1: `[pure] func` — Square brackets (Score 37)

```
Strengths:
  • [ at top level before func is unambiguous — not valid in current grammar
  • Self-delimiting: parser sees [, parses identifiers up to ], then expects func
  • Visually distinct from @ annotations — immediately recognizable as effect
  • Can group: [unsafe, alloc], or use compact form: [no(unsafe, alloc)]
  • Familiar from C# attributes, but different position (before not inside)
  • IDE: [] pair is a natural highlight unit; effect names get their own color
  • New effects: just a new identifier inside [] — no grammar changes
  
  Parser flow (3 tokens consumed, zero lookahead):
    [  → begin effect specifier
    pure / unsafe, alloc  → effect names
    ]  → end effect specifier
    func  → function keyword (expected next)

Weaknesses:
  • [] is used for array types: var arr : [10]int. But that's in type position
    (after :), not before func. No actual ambiguity, but a reader might 
    momentarily wonder
  • Two extra characters for the brackets — negligible
  • Not as "fluent" as bare keywords (pure func)

Memory: 2 bytes per function
```

### A2: `<pure> func` — Angle brackets (Score 35)

```
Strengths:
  • Same parser advantages as A1: < before func is unambiguous
  • Verse connection — familiar to Fortnite/UEFN developers
  • <> naturally suggests "type-level constraint" — semantically fitting
  • Multiple effects: <unsafe, alloc> reads well
  • IDE can highlight <> pair

Weaknesses:
  • <> already means generics in Chemical: func <T> name(). Even though 
    position is different (after func vs before func), visual overload 
    is real — readers may think <pure> is a generic parameter
  • <pure func — if user forgets closing >, parser recovery is harder than
    missing ]
  • Verse's <> syntax for effects is tied to their specifier system which 
    includes <decides>, <transacts>, <diverges>, <suspends> — importing 
    just <> without the Verse semantics may disappoint expectations
  • Slightly lower R1 than [] due to generics overload

Memory: 2 bytes per function
```

### B1: `pure func` — Keyword modifiers (Score 31)

```
Strengths:
  • Best readability — "pure function" reads like English
  • Familiar from async fn, unsafe fn
  • No brackets or delimiters
  • IDE: keyword coloring on pure, unsafe, alloc

Weaknesses:
  • Each new effect = new contextual keyword or reserved word
  • User-defined effects impossible
  • Parameterization requires parenthesized args which breaks the clean
    keyword pattern: no(unsafe) func — now it's not just "keyword func"
  • If someone names a variable pure, the parser needs context to 
    disambiguate
  • Cannot retrofit — existing code may use pure, alloc, mut as identifiers

Memory: 2 bytes per function
```

### D: `@pure` annotation (Score 35 — tied A2)

```
Strengths:
  • Zero parser changes — annotations stored as opaque metadata
  • Most extensible
  • Already works in current compiler

Weaknesses:
  • User explicitly rejects this: effects and annotations are different
    concepts and must look different
  • @ is overloaded — @deprecated, @inline, @pure all look the same
    despite meaning completely different things (advisory vs enforced)
  • IDEs color @ annotations uniformly — no visual distinction for effects
  • A beginner sees @pure and thinks "oh it's like @deprecated, just a note"

Memory: 2 bytes per function
```

---

## Recommended: Square Bracket Specifiers

### Syntax

```chemical
// No effects (pure)
[pure]
func add(x: int, y: int) : int { return x + y }

// Single effect restriction
[no_unsafe]
func process(data: *mut u8) { *data = 0 }       // ERROR

// Multiple restrictions
[no(unsafe, alloc)]
func read_only() { ... }

// Ambient — no restriction (default)
func any() { ... }
```

### Suppression blocks (orthogonal, use keywords)

```chemical
// unsafe {} already exists
// Generalize to all effects:

func example() {
    alloc {                         // <-- keyword block
        var v = vector<int>()
    }
    unsafe alloc {                  // <-- combined
        var p = malloc(100)
        *p = 42
    }
}
```

Suppression blocks use **keywords in statement position** — which are
unambiguous because keywords before `{` in statement position (like
`if`, `while`, `for`) are already part of the grammar.

### Why square brackets over the alternatives

| Concern | How `[]` addresses it |
|---------|-----------------------|
| Parser ambiguity | `[` before `func` is not valid in current grammar — zero ambiguity |
| Visual distinction | `[]` vs `@` — completely different sigils |
| Extensibility | New effect = new identifier inside `[]`, no grammar changes |
| Grouping | `[unsafe, alloc]` — comma-separated, one bracket pair |
| Parameterization | `[no(unsafe, alloc)]` — parenthesized args if needed |
| IDE support | `[` `]` pairs are trivially highlighted; effect names get their own syntax color |
| Learnability | C# attributes, Python `[decorator]` pattern |
| Backward compat | No existing code uses `[` before `func` — zero breakage |

---

## Implementation Strategy

### Phase 0: Effect Bits

**`compiler/effects/EffectBits.h`** (new — 15 lines)

```cpp
#pragma once
#include <cstdint>

enum EffectBit : uint8_t {
    FX_UNSAFE   = 1 << 0,
    FX_ALLOC    = 1 << 1,
    FX_MUT      = 1 << 2,
    FX_SUSPENDS = 1 << 3,  // reserved for async
    FX_ALL      = FX_UNSAFE | FX_ALLOC | FX_MUT | FX_SUSPENDS,
};
```

### Phase 1: Storage

**`ast/structures/FunctionDeclaration.h`** — in `FuncDeclAttributes`:

```cpp
uint8_t fx_computed = FX_ALL;   // actual effects (computed by compiler)
uint8_t fx_exempt   = 0;         // exempt effects (from [pure], [no_unsafe], etc.)
```

+2 bytes per function.

### Phase 2: Parser

**In `parser/structures/Function.cpp`** — before parsing access specifier
or `func` keyword:

```cpp
// Parse effect specifier: [effect1, effect2, ...]
// At top level before func, [ is unambiguous.
EffectSpecifier parseEffectSpecifier() {
    // consuming [
    // parse identifiers (pure, no_unsafe, alloc, etc.)
    // support [no(unsafe, alloc)] compact form
    // consuming ]
    // return parsed specifier
}
```

The parser stores the parsed effect mask on `FunctionDeclaration::fx_exempt`:

```cpp
func->fx_exempt = parsed_effects;
```

**Why this is fast:** `[` triggers effect parsing. Up to `]` — all identifiers
inside are effect names (no type resolution needed). Effects are either
built-in names that map directly to bits, or custom names that are stored
for later resolution. Total work: one token per effect name + O(1) bit
assignment.

### Phase 3: Compute `fx_computed`

**`compiler/symres/SymResLinkBody.cpp`** — accumulate during body visiting:

```cpp
// VisitUnsafeBlock, VisitDereferenceValue → fx_computed |= FX_UNSAFE
// VisitNewStmt, VisitDeallocStmt, VisitDeleteStmt → fx_computed |= FX_ALLOC
// VisitFunctionCall → fx_computed |= callee->fx_computed
// Visit global var write → fx_computed |= FX_MUT
```

**Two subphase approach for parallel body linking:**

Subphase A (parallel per-file, no cross-file deps):
- Compute `fx_local` from direct operations only
- Ignore function calls for effect propagation
- Store callee references for subphase B

Subphase B (parallel per-function, reads finalized masks):
```cpp
fx_computed = fx_local;
for each callee C: fx_computed |= C->fx_computed;
```

Byte reads are atomic on all architectures. No mutex.

### Phase 4: Checking

**`compiler/typeverify/TypeVerify.cpp`** — two O(1) checks:

```cpp
// Check 1: function body matches declared restrictions
if (fx_computed & fx_exempt) {
    // Error: function has effect it declared absent
}

// Check 2: caller restrictions respect callee effects (in VisitFunctionCall)
if (caller.fx_exempt & callee->fx_computed) {
    // Error: caller restricts effect that callee has
}
```

Both checks: single `AND` + compare. No tree walking, no allocation.

### Phase 5: Suppression Block Parser

```
unsafe   { ... }   → suppress unsafe effect for block body
alloc    { ... }   → suppress alloc effect
mut      { ... }   → suppress mut effect
```

Parsed as statements: `keyword { block }`. In statement position, these
are unambiguous (not valid identifiers before `{` in current grammar).

### Phase 6: Module Policy

**`chemical.mod`:**

```toml
var effect = "pure"          # all functions implicitly [pure]
var effect = "no_unsafe"     # all functions implicitly [no_unsafe]
```

```cpp
func->fx_exempt |= module_exempt_bits;  // additive with per-function
```

---

## Storage & Checking

### Memory: 2 bytes per function

```
┌─────────────────────────────────────────────┐
│ FuncDeclAttributes                            │
│  ┌──────────────┐  ┌────────────────────┐    │
│  │ fx_computed  │  │   fx_exempt        │    │
│  │   (uint8_t)  │  │   (uint8_t)        │    │
│  │   = FX_ALL   │  │   = 0              │    │
│  │  computed    │  │  from [pure] etc.  │    │
│  └──────────────┘  └────────────────────┘    │
└─────────────────────────────────────────────┘
```

### Checking: single machine instruction

```asm
; Check 1: function body matches its restrictions
mov al, [func.fx_computed]
and al, [func.fx_exempt]
jnz  error

; Check 2: caller restrictions respect callee effects
mov al, [caller.fx_exempt]
and al, [callee.fx_computed]
jnz  error
```

4 instructions per check, both in register. Zero cache misses.

### Parallel compilation flow

```
Pass 1 (parse, parallel per-file):
  File 1 ──► [pure] func add → fx_exempt = FX_ALL
  File 2 ──► [no_unsafe] func process → fx_exempt = FX_UNSAFE

Pass 2a (symres link bodies — local effects, parallel per-file, no cross-file deps):
  File 1: fx_local[add] = 0   (pure body has no effects)
  File 2: fx_local[process] = FX_UNSAFE (has *data = 0)

Pass 2b (propagate through call graph, parallel per-function):
  For each function: fx_computed = fx_local | union(callee.fx_computed)

Pass 3 (typeverify — check, parallel per-function):
  fx_computed & fx_exempt?  (O(1))
  caller.fx_exempt & callee.fx_computed?  (O(1))
```

---

## Module-Level Policy

### In chemical.mod

```toml
application my_app
source "src"

# Module-wide effect policy — applied to every function
var effect = "pure"          # every function is [pure]
var effect = "no_unsafe"     # every function is [no_unsafe]
var effect = "no_alloc"      # every function is [no_alloc]
var effect = "default"       # no restriction (backward compatible)
```

### CLI

```bash
--effect=pure           # override all modules
--effect=no_unsafe
--effect=default
```

```cpp
// Module setup:
uint8_t module_exempt = 0;
if (policy == "pure")       module_exempt = FX_ALL;
if (policy == "no_unsafe")  module_exempt = FX_UNSAFE;
// ...

// Applied per function (OR — additive with per-function specifiers):
func->fx_exempt |= module_exempt;
```

---

## FAQ

### Q: Does `[` before `func` conflict with array literal syntax `[1, 2, 3]`?

No. Array literals appear in expression/initializer position:
```chemical
var arr = [1, 2, 3]     // array literal — after =
var arr : [10]int        // array type — after :
```

Before `func`, `[` is not part of any existing grammar rule. The parser
unambiguously enters effect mode.

### Q: How do effect specifiers interact with annotations?

Annotations go on their own lines (as they do now), effects go in `[]`:

```chemical
@extern
[pure]
func sqrt(x: double) : double

@deprecated("use new_api instead")
[no_unsafe]
func old_process(data: *mut u8) { ... }
```

Two distinct sigils (`@` vs `[]`), two distinct concepts.

### Q: Can I put effects on struct/variant declarations?

Phase 1: functions only. Structs and variants can have effect specifiers
on their methods:

```chemical
struct Buffer {
    [pure]
    func len(&self) : usize { return self.size }

    [no_alloc]
    unsafe func write(&mut self, val: u8) { ... }

    @delete
    func delete(&mut self) { ... }
}
```

Future: struct-level effects can apply to all methods:
```chemical
[pure]
struct Point { var x: int; var y: int }
```

### Q: How does `[pure]` interact with `unsafe {}` blocks?

`[pure]` sets `fx_exempt = FX_ALL`. `unsafe {}` in the body sets
`fx_computed |= FX_UNSAFE`. Check: `FX_ALL & FX_UNSAFE ≠ 0 → error`.

`[pure]` functions cannot contain `unsafe {}` blocks at all — the block
itself is an unsafe operation. If you need `unsafe {}`, use `[no_unsafe]`
instead (which restricts nothing about unsafe, only means "I don't have
unsafe ops" — but if you do, it errors).

Wait — this makes `[no_unsafe]` useless if you need `unsafe {}`. Let me
clarify the semantics:

- `[no_unsafe]`: function body must NOT contain any unsafe operation
  (deref, ptr arith, unsafe blocks, dealloc). This is an absolute ban.
- Ambient (no specifier): function may contain unsafe operations freely.

If you want to use `unsafe {}` but call it from a safe context, you can't
use `[no_unsafe]`. You either:
1. Make the function ambient (no specifier)
2. Extract the unsafe parts into a separate ambient function

### Q: What about `[pure]` calling `[no_unsafe]` function?

```cpp
caller.fx_exempt = FX_ALL     // [pure]
callee.fx_computed = FX_ALLOC  // [no_unsafe] function allocates

// Check:
FX_ALL & FX_ALLOC = FX_ALLOC ≠ 0 → error
```

A `[pure]` function can only call functions whose computed effects are
empty (truly pure). This means either:
- Functions also marked `[pure]`
- Unmarked functions whose body happens to have no effects (computed as 0)

### Q: Why not `[pure]` as default (opt-in for effects)?

Because existing code would break. Every function that uses `malloc`,
`vector`, or `printf` would suddenly fail to compile. The ambient default
is backward compatible.

Verse also uses this model — `<computes>` is the opt-out, not the default.
Default in Verse is `<transacts>`.

### Q: Can `[]` effects be used on lambdas?

Yes:

```chemical
[pure]
var fn = (x: int, y: int) : int => x + y

[no_alloc]
var cb = |ctx|(val: int) : int => {
    ctx.counter += val
    return val
}
```

### Q: How many effect bits do we have?

3 used, 5 reserved in `uint8_t`. Room for `suspends`, `io`, `network`,
`filesystem`, `custom1`. If more needed, upgrade to `uint16_t` — 2 extra
bytes per function, same O(1) checking.

---

## Summary

| Aspect | Decision |
|--------|----------|
| **Syntax** | `[pure] func`, `[unsafe, alloc] func`, `[no(unsafe, alloc)] func` |
| **Suppression** | `unsafe {}`, `alloc {}`, `mut {}` keyword blocks (statement position, unambiguous) |
| **Why not `@` annotations** | Effects are type-checking enforcement, not metadata. Must look different. |
| **Why not keyword modifiers** | Not extensible — new effects need new reserved words. User-defined effects impossible. |
| **Why not angle brackets** | `<>` is overloaded with generics. Visual confusion despite positional unambiguity. |
| **Why `[]` works** | `[` before `func` is not valid in current grammar. Zero lookahead, zero ambiguity, zero parser changes. |
| **Storage** | 2 bytes per function (`uint8_t fx_computed` + `uint8_t fx_exempt`) |
| **Checking** | O(1) bitwise AND — 4 instructions per check |
| **Parallel** | Subphase A (local effects, no deps) + Subphase B (byte reads, atomic) |
| **Default** | Ambient (no specifier) — all effects allowed, backward compatible |
| **Model** | Opt-out — `[pure]` removes all effects (like Verse's `<computes>`) |
| **Parser impact** | Minimal — `[` case added to top-level declaration parsing |
