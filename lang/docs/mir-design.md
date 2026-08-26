# Chemical MIR Design

Status: design proposal

This document defines the design and adoption plan for Chemical's middle-level
instruction representation (MIR). It is intentionally implementation-oriented.
An implementer must treat the invariants in this document as part of the MIR
contract, not as suggestions. The first implementation should be small and
boring, but it must establish the correct ownership, sequencing, and validation
boundaries from the beginning.

## 1. Decision Summary

Chemical should introduce a typed, instruction-based, multi-level MIR with:

* A module containing declarations, globals, types, constants, and functions.
* Functions containing basic blocks.
* Blocks containing an ordered list of instructions and exactly one terminator.
* SSA values for computed, immutable results.
* Explicit places for addressable storage and mutable state.
* Explicit `init`, `move`, `copy`, `drop`, and `destroy` operations.
* Explicit control flow, including both structured regions and canonical CFG.
* Optional source locations and stable debug identities on instructions and
  variables when diagnostics/debugging are enabled.
* No backend-specific syntax or AST `Value*` ownership in the core MIR.

The MIR is not an AST rewrite and is not LLVM IR with different names. It is a
typed semantic contract between the resolved/type-checked AST and all future
backends. It is higher-level than LLVM where that preserves source semantics or
improves C output, and lower-level than the AST where precise sequencing,
storage, and lifetime behavior are needed.

The core MIR must not contain a “write this C text” instruction. During migration
only, a separate C-translation adapter may contain a `LegacyCFragment` attached
to a statement boundary. Such a fragment is explicitly C-only, opaque to MIR
optimization, interpretation, LLVM, JVM, and Wasm, and cannot share MIR cleanup
state with surrounding instructions. The adapter must either lower the whole
containing statement/function through the legacy path or prove that the fragment
has no ownership/control-flow interaction with the MIR region. This is a
temporary migration bridge, not a permanent MIR escape hatch.

The canonical internal form is a CFG. Structured operations such as `if`,
`while`, and `switch` may be retained while building MIR, but they must have a
well-defined lowering to blocks and branches. C emission may choose structured
C when it is safe and readable; it must never depend on C structure to define
MIR semantics.

The initial adoption target is the 2c backend. A separate `MIRLowerer` will
lower one resolved function at a time, and a C emitter will translate that MIR
to the existing output buffer. `2cASTVisitor.cpp` is the integration point, not
the MIR implementation: it may invoke `MIRLowerer`, but MIR construction must
not depend on `BufferedWriter`, `nested_value`, C names, or legacy destructor
queues. Top-level declaration emission can remain unchanged until function-body
MIR is proven on the complete test suite.

## 2. Why The Current Design Needs A Boundary

The current C translator combines at least five responsibilities:

1. AST traversal and semantic decisions.
2. C type and ABI spelling.
3. Expression formatting.
4. Temporary allocation and struct-return conventions.
5. Destructor/drop scheduling and control-flow cleanup.

The main implementation is `preprocess/2c/2cASTVisitor.cpp`. Its mutable state
includes `BufferedWriter`, `nested_value`, temporary counters, `local_allocated`,
`destructible_refs`, aliases, current scope/function state, and a separate
`CDestructionVisitor`. Helpers such as `func_call_single_arg`,
`write_value_for_ref_with_val_type`, `call_implicit_constructor`, and
`assign_statement` must decide how an AST value behaves based on where it is
being printed. This is the source of the compound-expression problem:

```c
/* Current style, simplified. */
take_str(&*({ std_string __tmp; std_string_make(&__tmp); &__tmp; }));
```

The expression has to be simultaneously usable as a value, an address, a
struct-return result, a reference argument, and a temporary requiring cleanup.
TinyCC then sees copies and GNU statement expressions that a stronger optimizer
might remove but TinyCC cannot always remove.

MIR separates *what happens* from *how it is printed*:

```text
%s = alloca std::string, lifetime=temporary
call_init %s, std::string::make, "hello"
call check_str, address_of %s
drop %s
```

The C backend can emit exactly the required statements. For a pure arithmetic
expression, it can instead print a compact expression tree:

```c
take(i * 8, i * 2, i * 4);
```

No backend should have to infer ownership or evaluation order from C text.

## 3. Requirements And Non-Requirements

### 3.1 Semantic requirements

The MIR design must be capable of representing:

* Primitive integers, floating-point values, booleans, characters, and void.
* Pointers, references, pointer arithmetic, loads, stores, casts, and address-of.
* Arrays, structs, unions, variants, function pointers, and capturing lambdas.
* Named and external functions, overload resolution results, ABI details, and
  name mangling decisions already resolved by the compiler.
* Constructors, implicit constructors, moves, copies, destructors, drop flags,
  arrays of destructible values, and return cleanup.
* Calls with exact left-to-right argument evaluation.
* Calls returning scalars, aggregates through an explicit result place, and void.
* `if`, loops, `switch`, `break`, `continue`, return, unreachable, and goto.
* Static interface dispatch, weak functions, external declarations, globals,
  thread-local storage, atomics, and compiler/runtime intrinsics.
* Comptime-eliminated code and runtime blocks that survive into runtime MIR.
* Diagnostics and source locations sufficient to debug generated code.
* Interpretation without requiring C, LLVM, or a target machine.

These are semantic requirements, not a promise that every operation is supported
by every backend on the first day. The initial supported subset and capability
matrix are defined below. A represented operation with no backend implementation
must fail with a structured diagnostic; it must never silently degrade to an
incorrect operation.

### 3.2 Initial implementation subset and capability matrix

The first implementation should support primitives, scalar arithmetic and
comparison, locals, places, loads/stores, address-of, scalar calls, explicit
struct result places, returns, and canonical CFG control flow in the C emitter,
interpreter, and LLVM emitter. Add aggregate lifetime operations immediately
after this subset, before enabling MIR by default for library code.

The following matrix is normative for staged adoption:

| Feature | MIR | C | Interpreter | LLVM | Initial status |
|---|---:|---:|---:|---:|---|
| Primitive values and arithmetic | yes | yes | yes | yes | first subset |
| Locals, places, load/store | yes | yes | yes | yes | first subset |
| Scalar calls and function pointers | yes | yes | yes | yes | first subset |
| Struct result places and ABI metadata | yes | yes | partial | yes | before aggregate default |
| Struct/array/variant lifetime | yes | staged | staged | staged | capability-gated |
| CFG branches and loops | yes | staged | staged | staged | capability-gated |
| Lambdas and captures | yes | staged | staged | staged | capability-gated |
| Atomics and TLS | yes | staged | target-dependent | yes | capability-gated |
| Exceptions/throw/provide | yes | unsupported/staged | staged | unsupported/staged | diagnostic required |
| CBI/plugin jobs | yes, if lowered | legacy initially | no | no | remain legacy initially |

The matrix must be kept current in the implementation. “MIR: yes” means the
operation has a semantic definition, verifier rule, and dump format. It does not
mean that a backend may claim support without tests.

### 3.3 Deliberately deferred

These are compatible with the design but must not block the first implementation:

* A serialized public MIR format.
* Whole-program optimization and link-time MIR optimization.
* Full alias analysis, escape analysis, SSA promotion, and register allocation.
* Automatic parallel code generation inside one function.
* JVM bytecode, WebAssembly, and native JIT emitters.
* A general MLIR dependency. Chemical should borrow concepts, not add a large
  framework dependency before the representation is validated.

## 4. Semantic Model

### 4.1 Three kinds of things

Every MIR operand is one of these categories:

* **Value**: an immutable result identified by a numeric `ValueId`. Examples are
  integer constants, the result of `add`, a loaded integer, or a function pointer.
* **Place**: addressable storage identified by a `PlaceId`. Examples are a local,
  a global, a struct field, an array element, or memory reached through a pointer.
* **Symbol**: a module-level entity such as a function, global, type, intrinsic,
  or external symbol.

Do not represent a place as a value containing a hidden pointer and do not
  represent an owned aggregate as an arbitrary temporary value. This distinction
  is essential for `&raw`, references, assignment, destruction, and C emission.

`Load(place)` produces a value. `Store(place, value)` mutates a place.
`AddressOf(place)` produces a pointer/reference value. A function result that
  must be addressable is written directly into a result place; it is not first
  made into a fake C expression and then addressed.

### 4.2 SSA without forcing everything into SSA

Pure scalar computations should use SSA values. Mutable and destructible
objects should use explicit places. MIR therefore has SSA values plus memory
operations, similar in spirit to LLVM's SSA and MLIR's explicit memory effects.

This is intentional:

* LLVM-style SSA is excellent for arithmetic, control-flow joins, and
  optimization.
* A pure SSA aggregate model makes move/destructor identity difficult.
* A pure memory model generates unnecessary loads and stores.

The first MIR need not implement mem2reg. It should represent scalars as values
where naturally available and preserve addressable storage for aggregates and
references. Later passes may promote a place only when the verifier can prove
that no address escapes and lifetime semantics are unchanged.

### 4.3 Ownership and initialization are explicit

An owned place has a state tracked by MIR construction and checked by the MIR
verifier:

```text
Uninitialized -> Initialized
Initialized   -> Moved
Initialized   -> Destroyed
Moved         -> Initialized       // explicit reinitialization only
```

`drop` is not a comment and is not inferred by a C emitter. It is an instruction
or a cleanup action in MIR. A destructible place must have exactly one valid
destruction on every path where it is initialized and not moved, unless a drop
flag explicitly makes that state conditional.

The MIR builder may initially emit conservative cleanup. Optimization can remove
redundant cleanup only after proving the same state transition. The C and
interpreter backends must execute the MIR lifetime operations literally.

### 4.4 Place projections, aliasing, and borrows

Places form a projection tree, but projections can overlap:

```text
p                 whole object
p.field           field projection
p[index]          array/index projection
*p                pointee projection
```

`field_addr`, `index_addr`, and `gep` must retain their base place/pointer and
projection information. They must not be reduced to an untyped integer address
in core MIR. This lets the verifier and later alias analysis distinguish a field
from its parent and detect overlapping operations where possible.

The following rules are mandatory:

* A borrowed reference or raw pointer does not transfer ownership.
* A borrowed reference must not outlive its base place. If the language permits
  escape, lowering must explicitly extend storage to an owning allocation or
  report an escape diagnostic.
* A raw pointer may escape and may be used for native operations, but it does not
  keep a local alive and does not suppress its destructor.
* Moving a whole aggregate consumes all owned subobjects. Moving a field consumes
  only that field if the language/type contract permits partial moves; otherwise
  the language semantic phase rejects the source operation or lowers it according
  to the type contract. The structural verifier only checks that the resulting
  initialized/moved state is internally consistent. The parent cannot be
  destroyed as if all fields were still initialized.
* A field or array-element place is not independently destroyed when its parent
  is destroyed. The parent type's destruction operation owns recursive subobject
  destruction.
* Assignment of overlapping places is invalid under the language semantics unless
  the operation explicitly has memmove-like semantics. Plain `memcpy` is never
  selected for unknown overlap. This is decided by semantic analysis or the
  borrow checker, not by the structural MIR verifier.
* A variant owns and destroys only its active payload. Changing the active case
  first destroys the old active payload, then initializes the new payload and
  discriminator.
* A union has no implicit active-member knowledge unless MIR carries an explicit
  discriminator/ownership contract. Otherwise only raw storage operations are
  permitted.
* A pointer into a temporary or local does not extend that object's lifetime.
  Lifetime extension is an explicit MIR operation and is target/capability checked.

The MIR verifier is not the language's borrow checker and is not responsible for
proving that arbitrary raw-pointer use is memory-safe. Safety decisions belong to
the existing resolution/type-verification pipeline and, if adopted, a separate
borrow-checking pass at the AST or MIR level. The MIR verifier only rejects
structurally malformed MIR and invalid operation shapes. It must not reject a
well-formed bitwise move merely because a type might contain a self-referencing
pointer; that is a language-semantic/runtime concern, not MIR structural invalidity.

### 4.5 Evaluation order is a semantic invariant

MIR instruction order is observable whenever an instruction may:

* call a function, constructor, destructor, or intrinsic;
* read or write memory;
* access volatile or atomic state;
* allocate or free memory;
* throw, trap, or otherwise transfer control;
* invoke user-defined operators or interface dispatch.

The baseline rule is strict source order. For a call, arguments are lowered in
source order, and each argument's instructions are emitted before the next
argument's instructions. No pass may reorder across an effect unless it has a
proven effect/alias reason and the operation is marked movable.

This makes the following correct:

```text
%a = call some_runtime_func()       // first
%s = alloca string, temporary
call_init %s, string::make           // second
call check, %a, address_of %s
drop %s
```

The C emitter may inline pure computations into a call argument only if doing so
does not move an effectful instruction across another effectful instruction and
does not change the lifetime of an owned temporary.

### 4.6 Effects are metadata, not a substitute for order

Each instruction has an `EffectSummary` with at least:

* `Pure`: no observable state or control effect.
* `Read`: reads memory or an external state.
* `Write`: writes memory or an external state.
* `Call`: may call unknown code.
* `Allocate`, `Free`, `Construct`, `Destroy`.
* `Atomic`, `Volatile`, `MayThrow`, `MayTrap`, `ControlFlow`.

Effects enable later optimization and C expression folding. They do not permit
reordering by themselves. Unknown calls are barriers. The safe default for a new
operation is `Call | Read | Write | MayThrow` until its contract is deliberately
specified.

## 5. MIR Containers And C++ Representation

The proposed C++ representation is intentionally compact and arena-friendly.
Names are illustrative; preserve the existing repository naming conventions when
implementing.

```cpp
using TypeId = uint32_t;
using ValueId = uint32_t;
using PlaceId = uint32_t;
using BlockId = uint32_t;
using InstId = uint32_t;
using SymbolId = uint32_t;

struct MIRModule {
    MIRTypeTable types;
    MIRConstantPool constants;
    MIRSymbolTable symbols;
    std::vector<MIRGlobal> globals;
    std::vector<MIRFunction> functions;
};

struct MIRFunction {
    SymbolId symbol;
    TypeId function_type;
    std::vector<MIRBlock> blocks;
    MIRValueTable values;
    MIRPlaceTable places;
    MIRCleanupTable cleanups;
    MIRDebugTable debug;
};

struct MIRBlock {
    BlockId id;
    MIRArray<MIRInstruction> instructions;
    MIRTerminator terminator;
};
```

`MIRArray` is an illustrative name for an arena-backed append-only array. The
example must not be implemented as a separately allocating `std::vector` inside
every block. Module-level containers may use ordinary vectors because they are
few and long-lived; hot function/block instruction and operand storage must use
the function arena and contiguous buffers described below.

Implementation rules:

* Allocate function-local instructions, operands, and metadata in a function
  arena or monotonic resource. Do not heap-allocate one polymorphic object per
  instruction.
* Prefer a tagged `MIRInstruction` payload over a virtual class hierarchy in hot
  construction and emission paths. A `MIRInstructionKind` switch is predictable
  and matches existing `ASTNodeKind` practices.
* Store operands in compact vectors with offsets/counts when an instruction has
  variable operands. Avoid `std::vector` inside every instruction.
* Use stable integer IDs in the function. Do not use pointers as semantic
  identity; arenas may be reset after each file.
* Core MIR must not retain `ASTNode*`, `Value*`, `FunctionDeclaration*`,
  `MembersContainer*`, or AST-backed string views as semantic state. The current
  file allocator is cleared and reused between passes, so such pointers can
  become dangling or point at a different AST object. During lowering, optional
  provenance may temporarily refer to an AST node, but the persisted MIR record
  must copy its `SourceLocation`, source file identity, and a stable declaration
  or debug ID. Backend artifacts must own any text they retain.
* Do not use `chem::string` for every local ID. Symbols should reference an
  interned name or existing mangled symbol entry.
* A single function MIR owns its local ID namespace, so functions can be built
  and emitted concurrently without a global temporary-name mutex.

### 5.1 Instruction shape

Every instruction has:

```text
kind, result type (optional), result id (optional), operands, optional source
location, effect summary, flags/attributes, and optional debug identity
```

Instructions that produce a place use `PlaceId`; instructions that produce an
SSA result use `ValueId`. Terminators never produce ordinary results.

### 5.2 Constants and types

Types must be canonicalized in the module type table. A `TypeId` must capture
semantic facts needed by all backends, including:

* integer width and signedness;
* float format;
* pointer address space and pointee type when known;
* reference mutability and referent type;
* array length and element type;
* struct/union/variant identity and field offsets/order;
* function parameters, return type, calling convention, varargs, and sret ABI;
* capturing function environment type;
* ownership/destructor/constructor traits;
* target-layout-dependent size/alignment only in a target layout object, not
  baked into portable semantic type identity.

Pointer-like types additionally carry a provenance/capability classification:

```text
reference       managed/borrowed reference to a typed object
raw pointer     native address with no ownership transfer
function ptr    callable code address/closure handle
opaque pointer  backend/runtime handle that cannot be dereferenced in core MIR
```

Pointer arithmetic, integer-pointer casts, dereference, and `sizeof`/`offsetof`
must be classified as portable, native-only, target-layout-dependent, or
unsupported. The interpreter may attach bounds/provenance metadata to raw
pointers, while C and LLVM lower them to their native representations. This
classification is required in core MIR now so JVM and WebAssembly backends do
not discover incompatibilities only after lowering.

Constants should be immutable interned records. A string literal is a constant
plus its addressable global representation, not an arbitrary C string fragment.

## 6. Instruction Set

The initial instruction set should be small but complete. Add an instruction only
when its semantics can be specified for the C emitter, interpreter, and LLVM
emitter. Backend-only instructions belong in a later target-specific layer.

### 6.1 Function and storage

```text
alloca       result place, type, storage class, alignment
global_addr  result pointer, global symbol
function_addr result function pointer, function symbol
param        result value/place for a parameter
load         result value, place/pointer, type, volatile/atomic flags
store        destination place/pointer, value, volatile/atomic flags
address_of   result pointer/reference, place
gep          result pointer/place, base, indices, inbounds flag
field_addr   result place/pointer, aggregate, field index
index_addr   result place/pointer, aggregate, index
```

`alloca` is semantic storage, not necessarily a C `alloca` call. The C backend
normally emits a declared local. LLVM may emit an entry-block `alloca` or
promote it. The interpreter creates a frame slot. A temporary's storage class
records whether it must be destroyed and when.

### 6.2 Lifetime and aggregate operations

```text
init         destination place, constructor symbol, ordered arguments
copy_init    destination place, source value/place, copy operation
move_init    destination place, source place/value, move operation
assign       destination place, source value, assignment semantics
copy         result value/place, source, copy semantics
move         result value/place, source, move semantics
drop         place, destructor symbol, optional drop flag
destroy      place, destructor symbol, unconditional explicit destruction
set_drop     drop flag, boolean
memcpy       destination pointer, source pointer, size, alignment, volatility
memset       destination pointer, byte, size, alignment, volatility
```

`copy_init` and `move_init` are distinct even when both lower to a C assignment
for a trivially copyable type. A backend may lower them differently for a
destructible struct. `assign` must preserve the language's old-LHS destruction
ordering: evaluate the RHS first when it can refer to the LHS, destroy the old
value, then install the new value.

### 6.2.1 Type operation contract

The type table must provide an operation contract for every aggregate that can
be initialized, copied, moved, assigned, or destroyed:

```text
copy_construct  destination, source
move_construct  destination, source
copy_assign     destination, source
move_assign     destination, source
relocate        destination, source       // storage move, if supported
destroy         object
```

Each operation is explicitly one of `trivial-bitwise`, `field-wise`,
`custom-symbol`, or `unsupported-by-this-target`. A missing contract is a MIR
construction/code-generation error only when the compiler cannot generate the
default operation or an explicitly requested operation. For ordinary aggregates,
the compiler must generate a `trivial-bitwise` contract by default. A destructor
does not imply that bitwise copy, bitwise move, or relocation is semantically
safe, but the MIR must still be able to represent the operation when the language
permits it. In particular, a move of an aggregate normally lowers to a bitwise
`memcpy`, including aggregates containing pointers.

This deliberately permits the language's low-level behavior. If a moved object
contains a self-referencing pointer, a future optional post-move hook may repair
that pointer. Such a hook is an explicit user/type semantic feature and may be
unsafe; MIR must represent and call it when present, but must not invent one or
reject the `memcpy` move when it is absent. Borrow checking, if enabled later,
may diagnose a program according to the language rules, but that is separate
from whether the MIR operation is structurally valid.

The contract also records whether an operation preserves internal pointers,
whether it permits partial-field moves, whether source and destination may
overlap, and which subobjects are initialized on success. The C and LLVM
emitters use this contract; they do not rediscover it from the AST type.

Constructors and destructors are symbols with normal call ABI metadata. Do not
encode them as magic text. An implicit constructor selected by type checking is
lowered to `init`, not rediscovered by a backend.

### 6.3 Arithmetic, comparison, conversion

```text
unary       neg, bit_not, logical_not
binary      add, sub, mul, div, rem, shifts, bitwise, logical
compare     eq, ne, lt, le, gt, ge with signed/unsigned/float predicate
cast        explicit conversion with source and destination types
select      condition, true value, false value for pure scalar values
sizeof      result integer, type
alignof     result integer, type
offsetof    result integer, type, field
```

Do not use a generic string operator in MIR. The operation enum and types must
be explicit so the interpreter and LLVM emitter agree on signedness, overflow,
floating behavior, and conversions. User-defined operators are ordinary calls
after resolution.

Short-circuit `&&` and `||` must not be represented as ordinary `binary`
instructions if either operand can have effects. Lower them to branches and a
join value, or use a high-level short-circuit operation with an exact CFG
lowering.

### 6.4 Calls and ABI

```text
call         result value/optional result place, callee symbol/value,
             ordered arguments, call ABI, effects
call_indirect result, function pointer, ordered arguments, ABI
invoke       call with normal and exceptional successors, if exceptions exist
```

The call operation contains the already-resolved callee and parameter ABI. It
must not ask the C backend to inspect `FunctionCall::parent_val`, infer a self
argument, or rediscover hidden parameters. MIR lowering performs:

* extension receiver and `self` placement;
* implicit parameters;
* reference address materialization;
* interface/vtable/static implementation dispatch;
* struct-return/sret result place selection;
* varargs ABI details where supported;
* constructor and destructor call classification.

The ABI record also stores each parameter's passing mode (`value`, `borrow`,
`mutable-borrow`, `move`, `out/result`, or `varargs`), mutability, address-space,
and ownership transfer. A backend must not infer these from whether a type is
struct-like or from whether a C spelling happens to contain `*`. Hidden `self`,
environment, sret, and implicit parameters are ordinary ordered ABI parameters
in MIR.

Arguments are an ordered range of `MIRArgument` records. Each record can refer
to a value, place address, value loaded from a place, or a result place. This
avoids inventing a fake value for `&raw std::string()`.

### 6.5 Control flow

Canonical terminators:

```text
br          target block
cond_br     condition, true block, false block
switch      scrutinee, default block, case table
return      optional value/place
unreachable
throw       exception value, handler/unwind target (when supported)
indirect_br target value, allowed block table (optional low-level feature)
```

Basic blocks must be terminated. A terminator is the only instruction allowed
to transfer control. `break` and `continue` lower to block IDs, never directly
to C labels. The C emitter may choose `break`, `continue`, or `goto` after
analyzing the CFG.

MIR uses block arguments rather than a separate phi instruction as its canonical
join mechanism. A block declares zero or more typed arguments, and every incoming
edge supplies exactly one value for each argument. This works for loop-carried
values, if/switch results, cleanup state, and return staging without inventing a
special instruction class. LLVM emission lowers block arguments to PHI nodes;
C emission declares join locals and assigns them on each incoming edge before the
branch; the interpreter binds them when entering the block.

An edge carrying cleanup must also carry any value or place needed by the cleanup
block. Cleanup blocks may not read a source local after the edge has transferred
or destroyed it. The verifier checks predecessor counts, argument types, and
that each edge's cleanup state is compatible with the target block.

### 6.6 High-level operations

The builder may initially expose:

```text
if_region       condition, then region, optional else region, result type
while_region    condition region, body region
for_region      init region, condition region, body region, step region
switch_region   scrutinee, case regions
```

These are construction conveniences, not a second semantic model. Before any
backend that does not support regions runs, a canonicalization step must lower
them to blocks, branches, and explicit cleanup edges. Keeping a structured form
for C emission is allowed only when it is equivalent to the canonical CFG.

## 7. Cleanup, Destructors, And Temporaries

This is the most important part of Chemical MIR.

### 7.1 Cleanup scopes

MIR has lexical cleanup scopes associated with function entry, explicit blocks,
loops, branches, and temporary-expression regions. A cleanup scope contains
owned places and optional drop flags. On normal exit, return, break, continue,
throw, or unreachable cleanup path, cleanup actions are selected in reverse
initialization order.

The builder must create cleanup edges while lowering, not reconstruct them from
the final C text. A return is conceptually:

```text
evaluate return value into a stable result
run cleanups for active scopes in reverse order
return result
```

If the return value aliases a local being cleaned up, the result must be moved
or copied to storage whose lifetime outlives the cleanup. This must be represented
explicitly and tested.

### 7.2 Temporary lifetime regions

An expression temporary has a precise region. For example:

```chemical
check(some_runtime_func(), create_str())
```

lowers conceptually to:

```text
%a = call some_runtime_func()
%s = alloca string, temporary(region=call-argument-2)
init %s, create_str
call check, %a, address_of %s
drop %s
```

The constructor occurs after the first argument, and the destructor occurs after
the call. A C backend must not hoist `%s` before `%a` merely because it needs to
declare `%s` earlier. Declaration placement and initialization are separate.
If C requires a declaration before a statement, emit:

```c
std_string __s;
int __a;
__a = some_runtime_func();
std_string_make(&__s);
check(__a, &__s);
std_string_del(&__s);
```

For a type that can be declared and initialized without changing order, the
backend may use a declaration initializer. It must not do so for effectful
constructors before preceding effects.

### 7.3 Drop flags and moves

When a value can be moved from one owner, MIR must represent the moved-from
state. A `move` consumes the source place unless the type's move contract says
otherwise. A conditional `drop` reads the drop flag and destroys only an active
value. The interpreter and every native backend must use the same rule.

Do not implement this by matching AST pointer identity in a backend. The current
interpreter's pointer-matching move behavior is useful historical evidence, but
MIR must give the behavior stable place IDs and explicit transitions.

### 7.4 Destruction order

Destruction order is normative and must be identical in the interpreter, C
emitter, and LLVM emitter:

| Object | Required order |
|---|---|
| Local/parameter scope | reverse order of successful initialization |
| Struct value | invoke the user destructor, then destroy owned members in the language-defined member order; MIR v1 must preserve the current 2c order (inherited members followed by direct members in declaration traversal order) |
| Variant value | invoke the variant destructor contract, then destroy only the active payload; MIR v1 must preserve the current variant-member traversal order |
| Fixed array | destroy elements from the last element to the first |
| Union | destroy only the active member when an explicit active-member contract exists; otherwise no implicit member destruction |
| Explicit `destroy` | perform the type's complete destroy operation and mark the place destroyed |
| Moved-from value | no destruction of the moved resource; any remaining initialized subobjects follow the partial-move contract |

The current backends separately schedule a user destructor and recursively
destroy members. The current 2c implementation calls the user destructor first,
then emits member cleanup; fixed arrays are traversed from the last element to
the first. MIR must encode this as one type operation so it cannot be accidentally
emitted twice. `drop` means conditional execution of the complete type destroy
operation. `destroy` means unconditional execution and is only valid for an
initialized place. An explicit destroy must clear or consume the cleanup state so
lexical cleanup cannot destroy the same object again. A different member order is
a language-semantics change and must not be introduced as an incidental MIR
implementation detail.

For a partially initialized aggregate, only successfully initialized subobjects
are destroyed, in reverse initialization order. A constructor that fails or
transfers ownership must communicate that state through MIR, not through a
backend-specific convention.

### 7.5 Self-referential aggregates

The existing LLVM temp + destructor + memcpy pattern can break a struct whose
field points into itself. This is a real runtime semantic hazard, but it does
not make the MIR operation structurally invalid. Aggregate move must be
representable as bitwise `memcpy`, because that is the desired low-level move
semantics for types that do not provide another move rule. The type operation
table records the selected behavior:

* `trivial-bitwise`: copy/move/relocate with the appropriate `memcpy` semantics;
* `field-wise`: generated field operations when the type explicitly requires it;
* `custom-symbol`: a user/compiler-provided operation such as a future post-move
  repair hook;
* `unsupported-by-this-target`: the target cannot lower this operation, which is
  a backend capability error rather than a safety verdict about the source.

When a type has a self-referencing pointer and no post-move repair hook, the
bitwise move remains valid MIR and produces the language's low-level behavior.
The language's future borrow/safety checking may reject or constrain such source
code, and an optional unsafe post-move hook may repair the pointer, but MIR
construction and structural verification must not reject the `memcpy` move.

The verifier should validate the operation's operands, types, sizes, and
destination/source shape. It must not reject a structurally valid `memcpy` move
because the type may contain self-referencing pointers. The selected move
operation and any optional post-move hook are part of the language semantics and
must be emitted consistently by the interpreter, C backend, and LLVM backend.

## 8. C Backend Design

### 8.1 Two-stage C emission

The C backend should have a `CModuleEmitter` and `CFunctionEmitter`.

`CModuleEmitter` handles:

* includes and runtime support;
* type declarations and definitions;
* global declarations/definitions;
* function prototypes;
* symbols, linkage, visibility, weak declarations, TLS, and aliases.

`CFunctionEmitter` handles:

* local declarations and C names;
* MIR blocks and instruction emission;
* structured-C reconstruction where safe;
* cleanup paths and labels;
* expression compaction for pure instructions.

The emitter must write to a function-local buffer or output object. It must not
share mutable current-function state between threads.

All output operations in the MIR emitter must be fallible. `BufferedWriter`
currently aborts on allocation failure; MIR code must not inherit that contract.
The emitter must return an emission status for success, unsupported feature,
I/O failure, or allocation failure, and must never publish a partial artifact.
Allocation failure must unwind the worker and fail compilation cleanly rather
than calling `abort` or dereferencing a null buffer. This is required by the
no-crash policy and matters when several workers exhaust memory concurrently.

Emission is transactional. A backend writes an artifact into a private buffer,
runs its backend verifier or syntax validation where available, and publishes
the buffer only on success. A diagnostic or unsupported operation discards the
artifact and selects the configured fallback; it must not leave half a function
in module output. The same rule applies to parallel function artifacts.

### 8.2 C names are assigned after MIR construction

MIR IDs are not C identifiers. Assign names in the C emitter using a local name
map. Names must be collision-free, deterministic, and prefixed (for example
`__chx_mir_v17`). User/source symbols continue to use `NameMangler` output.

Do not store C spelling in MIR operands. LLVM names, JVM slots, and interpreter
locations are different backend concerns.

### 8.3 Expression compaction

MIR instructions remain instruction-based even when C prints an expression. The
C emitter may form an expression tree from a contiguous run of pure instructions
when all of these hold:

* every operand is available and dominates the use;
* the instruction is pure and non-trapping under the language semantics, or the
  emitter preserves the language's required trap/evaluation behavior;
* no owned place is created, moved, initialized, or destroyed;
* no volatile, atomic, call, allocation, free, or unknown intrinsic is crossed;
* the tree is used once or duplication is proven harmless;
* source evaluation order remains left-to-right;
* the expression is emitted through the TinyCC-compatible C spelling selected by
  the C backend.

The baseline policy should be conservative: compact integer/float arithmetic,
comparisons, casts, constants, loads known safe to inline, and pure selects;
materialize calls, aggregates, references, and all lifetime operations. Add
more compaction only with differential tests against the statement emitter.

Function argument lists require an additional C-specific rule. C does not
guarantee the evaluation order of function arguments. Therefore the C emitter
must materialize every argument whose evaluation can read/write memory, call,
allocate, destroy, trap, touch volatile/atomic state, or alias another argument.
It may print multiple arguments as expressions only after proving that their
evaluations are independent and have no observable ordering requirement. The
safe default is:

```c
/* MIR order is preserved even though C argument order is unspecified. */
int __arg0 = some_runtime_func();
std_string __arg1;
std_string_make(&__arg1);
check(__arg0, &__arg1);
std_string_del(&__arg1);
```

“Pure” is not sufficient by itself: a pure-looking load may alias a store, and
an expression that can trap may not be speculated. This rule applies even when
the final C expression would look shorter.

### 8.3.1 Optimization profiles

MIR optimization must be separated from backend emission and selected by the
compiler output mode:

* Debug modes preserve source-shaped instruction boundaries where practical,
  retain locations, and use conservative materialization. They run the MIR
  verifier and may run cheap canonicalization that cannot alter observable
  behavior.
* Release modes may run expression compaction, dead pure-instruction removal,
  place promotion, cleanup simplification, block merging, and other proven
  transformations. They must preserve the same effect order and lifetime state.
* A transformation must have a verifier or proof precondition and a regression
  test. “The generated C compiler probably optimizes this” is not a MIR pass.

The first C compaction pass should be a local emitter analysis, not a global
optimizer. This keeps debug output straightforward and makes its correctness
easy to compare with a statement-only emitter. More expensive optimization can
be added later without changing the semantic MIR contract.

For the example:

```text
%i = load i
%a1 = mul %i, 8
%a2 = mul %i, 2
%a3 = mul %i, 4
call take, %a1, %a2, %a3
```

the C emitter prints `take(i * 8, i * 2, i * 4);` because the arithmetic is
pure and each value is used once. It does not need an optimization pass to
delete three C variables.

### 8.4 C limitations and fallbacks

The emitter must always produce C accepted by TinyCC. There is no separate
GNU-extension mode on which MIR semantics may depend. MIR semantics cannot
depend on statement expressions, compiler-specific `typeof`, or C++ features.
Compiler-independent runtime macros and helper functions may hide small
compiler/platform differences, but they must themselves have TinyCC-compatible
implementations and must not change MIR semantics.

Required fallbacks include:

* declare locals first, initialize later when declaration order would move an
  effect;
* use explicit result places for struct returns;
* use labels and gotos for arbitrary CFG edges;
* use helper functions for operations TinyCC cannot express reliably;
* emit explicit temporaries for addressable rvalues;
* emit runtime support calls for checked/opaque operations.

The C emitter must never produce `&` applied to an expression that is not an
addressable C lvalue. MIR's `AddressOf` always has a place/pointer operand, so
this error should be structurally impossible after verification.

### 8.5 Struct return ABI

MIR represents a struct-return call as a call with a result place and ABI metadata.
The C emitter emits the existing hidden-result-pointer convention directly:

```c
Point __result;
make_point(&__result, x, y);
use_point(&__result);
```

If the result is immediately consumed by a reference argument, the same place is
passed. If it is moved into a local, use `move_init`/`memcpy` according to the
type's operation table. Never emit a value-block that copies an aggregate merely
to make expression composition possible.

## 9. Interpreter Backend

The interpreter executes the same MIR instruction semantics, not a separate
translation of the AST. This gives a high-value correctness oracle for C and
LLVM backends.

### 9.1 Runtime model

* `MIRFrame` maps `PlaceId` to storage slots and `ValueId` to immutable values.
* `MIRMemory` represents pointer-addressable storage with bounds metadata in
  debug/interpreter mode.
* `MIRControlState` holds current block, instruction index, and predecessor.
* `MIRCleanupState` tracks active initialized/moved places.
* Calls invoke MIR functions, registered extern/intrinsic adapters, or a clear
  unsupported-operation diagnostic.

The interpreter must not execute generated C. It must not silently return a fake
value for an unsupported instruction. During development, unsupported features
are a structured interpreter diagnostic; production behavior may be configured,
but never crash or corrupt state.

### 9.2 Interpreter equivalence

For every supported MIR instruction, document:

* operand validation;
* result and memory effects;
* initialization/move/drop transitions;
* trap/exception behavior;
* pointer bounds behavior;
* source location reporting.

Interpretation tests should run the same MIR verifier before execution. The
existing interpretation suite is especially valuable for destructors, lambdas,
pointer bounds, variants, and method chains.

## 10. LLVM Backend

The LLVM emitter lowers MIR, not AST nodes. It may use LLVM's existing `Codegen`
and `LLVMBackendContext` helpers initially, but AST-specific decisions must move
into MIR lowering over time.

Mapping rules:

* MIR SSA scalar values map to LLVM SSA values.
* MIR places map to LLVM pointers/allocas/globals.
* `load`/`store` map to LLVM load/store with volatile/atomic ordering.
* `field_addr`/`index_addr`/`gep` map to typed GEP with checked layout indices.
* `call` maps using MIR ABI metadata, including sret and calling convention.
* `cond_br`, `br`, and `switch` map directly to LLVM terminators.
* `drop`, `init`, `move_init`, and `copy_init` map to explicit calls or copies.
* debug locations map from MIR source metadata to LLVM debug metadata.

LLVM-specific optimization should occur after semantic MIR lowering. The MIR
emitter must not assume LLVM will repair incorrect lifetime ordering or aliasing.
The existing external-global `dso_local` rule remains mandatory: MIR linkage
metadata must distinguish a local definition from an external declaration.

All LLVM verifier failures must be reported with the MIR function/block/
instruction ID and source location. Do not suppress LLVM verification to make a
backend pass.

## 10.1 MIR insertion point and job applicability

The normal compilation pipeline must be:

```text
parse
  -> top-level declarations/signatures
  -> generic signature instantiation
  -> body symbol resolution and operator/implicit-conversion resolution
  -> type verification
  -> AST-to-MIR lowering
  -> MIR verification
  -> MIR optimization/canonicalization (mode-dependent)
  -> selected backend emitter
```

MIR lowering consumes the fully resolved and type-verified AST. It must not
trigger symbol lookup, overload resolution, generic instantiation, or compiler
plugin transformation as a hidden side effect. This keeps parallel body
resolution and generic instantiation ahead of backend work and gives all
backends the same semantic input.

MIR is applicable to executable, library, and ordinary C/LLVM translation jobs.
CBI/plugin jobs, build.lab compilation, and translation-only jobs may remain on
the legacy AST path initially, but that decision must be explicit in the job
configuration and diagnostics. A plugin must not accidentally receive a partial
MIR module and then call AST-only APIs. Interpretation jobs must use MIR for all
MIR-supported runtime functions; comptime evaluation remains a separate
compile-time phase until its AST interpreter is replaced or can consume MIR.

The module boundary is important: a function MIR artifact may be built per file
or per function, but symbol/type/layout tables are module artifacts. A backend
must receive a complete declaration environment before emitting cross-function
references. Generic instantiations are ordinary concrete functions/types by the
time they reach MIR lowering.

## 10.2 Global and module initialization

Globals cannot remain an unmodeled exception forever. MIR must represent:

* constant initialization that can be placed in a data section;
* runtime initialization with an ordered module initializer function;
* dependencies between runtime initializers;
* thread-local initialization and destruction;
* global destructor registration and shutdown ordering;
* external declarations versus definitions and their linkage/visibility;
* interpretation-time initialization using the same dependency order.

The initial implementation may leave global declaration spelling in the legacy
module emitter, but it must create a `MIRGlobal` record containing type, linkage,
visibility, TLS, constant/runtime initializer, source location, and destructor
metadata. Runtime initializers must be lowered into an explicit ordered init
sequence before MIR becomes the default for modules with non-constant globals.
Do not infer global order from C translation-unit order or thread scheduling.

## 11. Future JVM, WebAssembly, And JIT Backends

### 11.1 JVM

The JVM is a reference/operand-stack machine and does not provide general raw
pointer arithmetic. MIR must therefore distinguish portable reference operations
from native pointer operations now.

Portable subset:

* object/array references;
* null checks and reference comparisons;
* field and array access through typed references;
* method/function calls;
* scalar arithmetic and branches;
* explicit object construction;
* interface dispatch;
* lambda objects or method handles.

Native-only subset:

* integer-to-pointer casts;
* arbitrary pointer arithmetic;
* dereferencing unknown raw addresses;
* layout-dependent `sizeof`/`offsetof` when not representable;
* C ABI varargs and inline assembly.

Each MIR instruction should carry a portability classification or be queryable
from its type/effect. The JVM backend can reject native-only MIR with a source
diagnostic, while ordinary object/reference syntax remains usable. A JVM
destructor-like operation must be mapped to explicit close/resource semantics or
an ownership runtime; it cannot assume deterministic GC finalization.

### 11.2 WebAssembly

Wasm is naturally compatible with blocks, branches, typed numerics, linear-memory
loads/stores, and function calls. Arbitrary native pointers can be represented
as integer offsets only under a defined memory model. Reference types and GC
features should be added through a target capability layer, not by changing the
portable MIR meaning of pointers.

### 11.3 JIT

The JIT should consume verified MIR functions. Start with an interpreter or C
fallback, then add a per-function LLVM ORC/JIT emitter. Function and module IDs
must remain stable enough for lazy compilation and invalidation. Avoid designing
the core MIR around a JIT-specific instruction format.

## 12. Verification And Reliability

Reliability is a feature of the representation, not only a test-suite outcome.

### 12.1 Structural verifier checks

When enabled in debug builds, `--verify-mir`, or compiler assertion mode, the
MIR verifier must check:

* every block has one valid terminator;
* all referenced IDs exist and belong to the function/module;
* operand types match instruction requirements;
* each value definition dominates every use, or the value is supplied as a
  verified block argument;
* branch targets are valid and switch cases are unambiguous;
* block arguments have one incoming value per predecessor;
* call arguments match resolved ABI metadata;
* address-of operands are places or pointers, never arbitrary values;
* loads/stores use compatible types and valid address spaces;
* atomic order/scope combinations are valid;
* initialization, move, reinitialization, and destruction states are valid on
  every path;
* every destructible initialized place has a cleanup or an explicit transfer;
* no instruction occurs after a terminator;
* unsupported target capabilities are diagnosed before emission.

The verifier is not run on every production compilation by default. The MIR
builder's typed APIs and internal assertions must make it difficult to construct
invalid IDs, malformed instruction payloads, missing terminators, or impossible
block edges in the first place. Production backends may perform only the cheap
defensive checks needed to avoid crashes; they must not pay for full dominance,
alias, or lifetime verification unless the user enables validation. This is a
compile-time performance requirement, not permission to emit malformed MIR.

Structural validity and language safety are separate. The verifier checks that a
GEP field index exists, an operand has the expected type, a block argument is
provided, and a `memcpy` has valid typed/size operands. It does not prove that a
raw pointer is safe, that a `memcpy` move preserves a self-reference, or that a
borrow is legal. Those belong to language semantic checks and a future borrow
checker.

### 12.2 No-crash policy

Malformed MIR must produce a diagnostic object containing module, function,
block, instruction, source location, and reason. Backends must return failure,
not dereference null pointers or emit partial silently-valid code. Assertions may
remain in debug builds for programmer errors, but user-controlled invalid input
must follow the compiler's diagnostic path.

### 12.3 Differential and golden tests

For each MIR feature, maintain:

* a MIR construction test;
* verifier rejection tests for invalid forms;
* interpreter expected-result tests;
* C output golden tests where spelling matters;
* compiled C runtime tests under TinyCC and a stronger C compiler when available;
* LLVM verification/runtime tests;
* source-to-source differential tests comparing legacy 2c and MIR 2c behavior;
* evaluation-order tests with observable calls and destructors.

Do not rely on only output text. A C file can look reasonable and still destroy
the wrong object or evaluate a constructor too early.

## 13. Incremental Adoption Plan

The adoption plan is deliberately staged. Every stage must preserve the existing
test commands and run the relevant complete suite before moving on.

### Stage 0: Freeze behavior and add probes

Record current C output and runtime behavior for representative cases:

* scalar arithmetic and nested calls;
* struct-return calls;
* `&raw` and reference arguments;
* constructor/destructor temporaries;
* move and assignment from self-derived views;
* arrays and variants;
* lambdas and nested lambdas;
* loops, switch, break, continue, return cleanup;
* TLS, atomics, weak interfaces, externs, and globals.

Add a legacy-vs-MIR comparison mode that can be enabled per function. Do not
change default output yet.

### Stage 1: Introduce core MIR and verifier

Add a new isolated subsystem, for example `compiler/mir/`, containing types,
IDs, instructions, blocks, module/function arenas, verifier, dump printer, and
source-location metadata. It must have no dependency on `BufferedWriter`, LLVM,
or the C AST visitor.

Implement `MIRFunctionBuilder` with helpers for constants, places, blocks,
terminators, and diagnostics. Add unit tests that build MIR directly.

### Stage 2: Add a separate AST-to-MIR lowerer

Add `MIRLowerer`, owned by the MIR subsystem or a thin 2c integration layer. It
must consume resolved AST nodes and produce MIR without writing C. Start with a
mode for functions containing only primitive locals, arithmetic, return, and no
calls or destructors. `ToCAstVisitor` invokes the lowerer and then the C emitter;
it does not reproduce the lowering decisions itself.

The important seam is inside function translation, not in the whole compiler:
top-level declarations and type emission remain unchanged.

### Stage 3: Add the MIR interpreter alongside the first emitter

Before enabling MIR C for broader code, execute the Stage 2 MIR with the MIR
interpreter. Interpretation must be a first-class backend for every MIR feature
that is enabled for C. The existing AST interpreter remains available for
comptime and as a migration fallback, but it is not an equivalence oracle for
MIR-generated C. Add tests that run one lowered function through interpreter and
C and compare results and observable event order.

### Stage 4: Add ordered calls and places

Lower identifiers, loads/stores, address-of, references, scalar calls, implicit
parameters, extension receivers, and function pointers. Add explicit result
places for struct-return calls even if aggregate values are not yet supported.

Run evaluation-order tests before enabling any C expression compaction.

### Stage 5: Add aggregate initialization and cleanup

Lower structs, arrays, variants, constructors, implicit constructors, moves,
copies, destructors, drop flags, temporary regions, assignment, and return
cleanup. Replace `CDestructionVisitor` scheduling for MIR functions with MIR
cleanup scopes. Keep the legacy destructor visitor for legacy functions.

This stage must include tests that specifically catch the ordering bug:
`check(some_runtime_func(), create_str())`, and self-referencing assignment tests.

### Stage 6: Add control flow

Lower if/else, loops, switch, break/continue, short-circuit operators, and
returns to CFG. First emit labels/gotos in C because that is the direct and
unambiguous mapping. Add structured-C reconstruction only after CFG output is
stable.

### Stage 7: Add lambdas, interfaces, runtime blocks, and intrinsics

Lower capturing environments, nested lambdas, static/weak interface dispatch,
embedded values, runtime blocks, compiler/runtime intrinsics, atomics, TLS, and
special ABI cases. Every feature gets a MIR operation contract and capability
test before being enabled.

### Stage 8: Make MIR C the default incrementally

Use per-function, per-statement, and per-module feature gates. A statement-level
legacy C fragment may be used only through the temporary migration bridge defined
in the introduction; it is never an input to a portable MIR backend. When a
function contains unsupported ownership/control-flow interaction, lower the
whole function through the legacy path rather than silently mixing cleanup
models. When all functions in a module successfully lower and verify, emit that
module through MIR. Every fallback must be visible in verbose/debug output and
must not silently mix lifetime state between legacy and MIR code.

### Stage 9: Add LLVM lowering

LLVM lowering should initially target the same subset as MIR C and the MIR
interpreter, using existing LLVM helpers. Once parity is established, migrate
more LLVM AST lowering decisions to MIR and eventually remove duplicate AST
backend logic.

### Stage 10: Parallel translation

Only parallelize after function-local MIR construction and emission are proven
independent. Then process independent files/functions concurrently and merge
module output deterministically.

## 14. Parallelism Design

### 14.0 Correct unit of parallel MIR emission

The proposed strategy is correct only with an important qualification:

> A thread-pool task may own one source file's MIR lowering and function
> artifacts, but it must not independently own or emit a complete module.

The semantic unit remains a complete module. The parallel unit is a resolved
concrete function or a group of functions from one file. A file task is the best
initial implementation because it matches `LabModule::direct_files`, avoids
excessive scheduling overhead, and gives a natural arena lifetime. It may later
be split into function tasks when a large file becomes a load-balancing problem.

The correct topology is:

```text
module semantic barrier
  -> immutable MIRModuleContext
  -> serial module prologue
  -> thread-pool file/function tasks
       each owns MIRTaskContext + MIR arena + private backend buffer
  -> deterministic serial artifact merge
  -> module finalization and TinyCC/LLVM invocation
```

`MIRModuleContext` contains immutable references or IDs for the completed symbol
table, canonical types, target layout, ABI records, mangled names, runtime
declarations, generic-instantiation list, and module ordering. It must not be
copied into each task and must not be mutated by a worker. Passing a const
reference with a module lifetime is preferable to a reference-counted object in
the inner loop; reference-counting every operand or task adds unnecessary atomic
traffic.

Before workers start, seal module interning and numbering: assign all `TypeId`,
`SymbolId`, ABI-record IDs, runtime-support IDs, and mangled symbol spellings that
workers may need. A worker may perform read-only indexed lookup, but it must not
insert into a shared type table, symbol table, name-mangling cache, or constant
pool. If a late lowering case needs a new module entry, return it to a serial
completion queue or pre-reserve a deterministic slot; never protect the hot path
with a global mutex.

Each task creates a fresh lightweight context on the worker stack:

```text
MIRTaskContext {
    MIRFunctionBuilder builder
    MIRLowerer lowerer
    MIR bump arena
    CFunctionEmitter emitter
    local C-name/label counters
    local diagnostics
}
```

“Fresh per task” means fresh logical state, not necessarily a fresh heap
allocation. For maximum throughput, a thread-pool worker may own one reusable
`MIRWorkerContext` containing its bump arena, operand buffers, C buffer, name
counter, and diagnostic scratch storage. Before the next file task starts, reset
lengths and counters and release all references to the previous artifact. The
worker context must never retain a place, value, AST pointer, string view, or
cleanup entry from a prior task. A stack-created context is also correct for a
small initial implementation; benchmark reusable worker storage before adding
more complex pooling.

There must be no shared MIR writer, shared task arena, shared temporary counter,
mutable `ToCAstVisitor`, mutable `CDestructionVisitor`, or mutable backend
context. The task returns an owned `MIRFunctionArtifact`; the merge stage moves
that artifact into its source-order slot rather than copying its MIR or C buffer.

MIR retention is mode-dependent. In `debug_quick` C compilation, a task may
lower, emit, and immediately release its function arena after the private C
buffer is complete. The artifact then owns only the C bytes and identity needed
for merging. Interpreter runs, MIR dumps, validation, and backends that need a
later pass use `KeepMIR` and retain the arena until all consumers finish. Do not
keep every function's MIR alive in a module-wide vector merely because another
mode might request a dump.

For a normal single-C-output module, the file task emits only function bodies
and function-local support into its private buffer. Includes, type declarations,
globals, prototypes, generic declarations, and deferred interface stubs remain
module-level work. For per-file/incremental C output, every emitted C file must
receive the required common prologue and declarations through a shared immutable
declaration snapshot or generated header; workers must not independently guess
which declarations another file needs.

A literal “new complete MIR emitter per file” is therefore not the strategy:

* Correct: one new worker-local lowering/emitter context per file task, sharing
  immutable module data, producing an isolated artifact.
* Incorrect: one emitter per file that copies module types/symbols, writes to the
  final module buffer, discovers generic instantiations, or emits declarations
  according to task completion order.

Start with one task per sufficiently large file, batch small files, and reserve
per-function scheduling for files whose functions are large or uneven. This
avoids paying thread-pool, arena, and artifact overhead for tiny files while
still allowing multiple files to lower concurrently.

### 14.0.1 MIR/C emission versus TinyCC compilation

Parallel MIR emission and parallel TinyCC compilation are separate decisions.
MIR lowering and C emission can safely run concurrently because each task owns
its state. TinyCC state objects and final link/output state must also be isolated
if compilation is parallelized; never share one mutable TinyCC compiler state
between workers.

For a small module, one merged C translation unit and one TinyCC invocation may
be faster than creating many object files. For a large module, workers may emit
per-file C artifacts and compile them independently with one compiler state per
artifact, followed by serial linking. The choice belongs to the job scheduler
and must be benchmarked in `debug_quick`; MIR itself must not assume that the
number of C artifacts equals the number of source files. Per-file artifacts must
still receive the complete declaration snapshot/header described above.

### 14.0.2 Sealing, discovery, and worker contracts

The phrase “fresh emitter per file” is safe only after a serial preparation
step. MIR lowering is not allowed to discover new module symbols as a side
effect of a worker task. Before submitting tasks, the coordinator must:

1. finish parsing, symbol resolution, generic instantiation, and type
   verification;
2. enumerate every concrete function body that may be emitted, including
   nested/capturing lambdas and already-required generic functions;
3. assign deterministic declaration, function, type, ABI, runtime-support, and
   output-order IDs; and
4. build and seal an immutable `MIRModuleContext` containing the target layout,
   canonical type descriptions, symbol/linkage records, lambda environment
   descriptions, and declaration snapshot.

If lowering a function would require a new generic instantiation, lambda
declaration, type, runtime helper, or symbol, that is a pipeline error in the
first implementation. Later implementations may send an explicit request to
the coordinator, pause the worker at a barrier, perform the insertion
serially, and resume with a new sealed snapshot. A worker must never mutate a
shared table or use mutex-protected insertion in the instruction hot path.

Nested lambdas are functions in MIR, not anonymous C text inside their parent.
Their environment type, capture order, function symbol, and body must be
assigned during discovery so workers cannot race while inventing names or
declarations. A lambda body may be emitted by the same file task initially,
but it must occupy its own stable function artifact slot.

The immutable context must own or reference data whose lifetime extends past
all workers. It may not contain views into an allocator reset between AST
passes. AST pointers are permitted only transiently while lowering the current
source node; they must not escape into `MIRModuleContext`, an artifact, or a
retained diagnostic.

The coordinator owns scheduling and publication. A worker owns only its
`MIRWorkerContext` and its in-progress artifact. It must not print diagnostics,
append to the final C writer, update shared timing state, or call `reset()` or
`file_level_reset()` on the shared `ToCAstVisitor`. Worker results are moved
into preassigned slots and published only after lowering, mode-required
validation, and backend emission complete.

### 14.1 Ownership boundaries

The unit of parallel work should be a `MIRFunctionArtifact` containing:

* function symbol and signature reference;
* function MIR arena and verifier result;
* emitted backend buffer or backend-specific artifact;
* diagnostics;
* source file/function ordering key.

Each worker owns its builder, allocator, name map, backend emitter state, and
temporary counters. Shared state is immutable after semantic passes: canonical
types, symbols, mangling tables, layout descriptions, and runtime declarations.

### 14.2 Deterministic merge

Function artifacts are sorted by the existing source/module ordering before being
appended to the final module buffer. Diagnostics are merged in the same order.
Output must not depend on thread scheduling.

### 14.3 C translator migration

The current `ASTProcessor` already visits module files and resets a visitor after
translation. Replace the single shared `ToCAstVisitor` output path in stages:

1. Keep declaration traversal serial and unchanged.
2. Collect function-body AST nodes for a module.
3. Create one lowering/emitter context per worker.
4. Emit each function into an independent `BufferedWriter`.
5. Merge function buffers at the position reserved by declaration ordering.
6. Keep global initialization and runtime support emission serial until explicitly
   made thread-safe.

Do not call one `ToCAstVisitor` concurrently. Its fields such as current scope,
temporary counters, aliases, local allocations, and destructor jobs are not
thread-safe. The MIR design removes these shared mutable concerns from function
emission.

### 14.4 Performance rules

* Use arenas/monotonic allocation for MIR construction.
* Reserve instruction/operand capacity using AST estimates where available.
* Avoid hash maps in every instruction; use dense ID-indexed arrays.
* Use `string_view`/interned symbols and move diagnostics between workers.
* Do not serialize MIR merely to pass it between C++ objects in the same process.
* Keep the verifier configurable: it is enabled in debug/validation modes and
  disabled by default in production quick paths.
* Benchmark legacy direct C, MIR construction, MIR C emission, and total compile
  time separately. A faster emitter does not compensate for an unnecessarily
  expensive MIR builder.

The performance goal is low constant overhead, not a literal nanosecond promise
for arbitrary functions. The representation must avoid per-instruction heap
allocation and unnecessary text/string work so simple functions remain close to
the direct writer path.

### 14.5 `debug_quick` fast path

`debug_quick` is a distinct compilation contract, not merely `debug` with fewer
flags. It is intended to produce an executable as quickly as possible. Its MIR
policy is:

| Area | `debug_quick` policy |
|---|---|
| MIR construction | one direct AST-to-MIR pass; no temporary AST wrapper nodes |
| Allocation | one thread-owned function arena; no allocation mutex in the hot path |
| Instruction storage | packed records and contiguous operand storage |
| IDs | dense 32-bit function-local IDs |
| Debug data | absent unless `-g`, `--dump-mir`, or validation explicitly requests it |
| Verification | disabled by default; only typed builder assertions and essential backend checks |
| MIR optimization | none |
| CFG processing | construct blocks directly; no global canonicalization/dominance pass |
| C emission | direct sequential emission with TinyCC-compatible spellings; no separate global optimization pass |
| Output | private function buffer, then one deterministic merge |
| Diagnostics | collect only errors that are needed to continue/fail; do not format source snippets eagerly |

The quick path must not run full dominance, alias, lifetime, dead-code, cleanup,
expression-tree, or block-merging passes. This does not permit malformed MIR:
the builder API must make invalid IDs and malformed payloads difficult to create,
and the emitter must return failure rather than crash if defensive checks detect
bad input.

The C emitter may print a pure MIR result inline when that requires no
reordering. This is a representation choice, not a MIR optimization pass. To
make this cheap and deterministic, each function value record should maintain a
dense use count while operands are appended. A pure scalar instruction may be
printed inline when it has one use, its operands are already available in the
same straight-line region, and its opcode is on the TinyCC-safe whitelist. No
expression DAG, hash-map analysis, alias analysis, or dominance pass is allowed
for this decision. The use-count update is O(1) per operand, so the quick path
still performs linear lowering/emission while avoiding unnecessary arithmetic
locals.

This micro-compaction must never apply to calls, loads with possible aliasing,
stores, allocations, lifetime operations, volatile/atomic operations, trapping
operations, or expressions whose operands cross a block edge. It must not be
used to hide an evaluation-order decision. Otherwise the result is
materialized as a local. The quick path must prefer predictable linear work
over a smaller C file.

A non-volatile load from a local scalar place that has not escaped and cannot be
mutated between the load and its use may be treated as an expression leaf. This
is the permitted case needed for `take(i * 8, i * 2, i * 4)` to remain compact;
it is not permission to inline arbitrary memory reads.

### 14.6 Concrete hot representation

Do not implement the fast path as one heap object per instruction, a virtual
instruction hierarchy, or a `std::variant` containing heap-owning vectors. Use a
compact tagged record and one operand arena:

```cpp
struct MIRInstruction {
    uint32_t opcode_and_flags;
    uint32_t result_or_place;
    uint32_t operand_offset;
    uint16_t operand_count;
    uint16_t source_index;       // 0xffff in debug_quick
};

struct MIROperand {
    uint32_t id;
    uint32_t type_or_kind;
};
```

The dense value table should also contain a small saturating use count. Increment
it when an operand is recorded, not by walking a hash map later. If a value is
referenced from multiple blocks or more than once, conservatively materialize
it in C. This is deliberately weaker than a use-def graph and must not be
presented as a general optimization analysis.

The exact fields may change, but the following properties are mandatory:

* the common instruction header is fixed-size and trivially movable;
* variable operands are appended to one contiguous function-owned array;
* flags such as purity, volatility, atomicity, and terminator status are bit
  fields or opcode properties, not heap objects;
* calls, switches, ABI records, and debug records use side tables referenced by
  compact indices;
* source locations are compact indices into optional tables, not copied strings
  or full debug structures in every instruction;
* function-local `ValueId`, `PlaceId`, `BlockId`, and `InstId` are 32-bit unless a
  module exceeds the representable limit, in which case lowering fails clearly;
* tables are append-only during lowering, so construction is pointer-bump plus
  sequential writes.

Use a dedicated MIR bump arena rather than `ASTAllocator`. `ASTAllocator` is
designed for AST object destruction and its allocation path may lock an allocator
mutex; MIR instructions need neither destructor registration nor shared ownership.
The recommended layout is one arena per function worker with a small inline first
chunk, such as 32--64 KiB, followed by geometrically grown chunks. Reset the
whole arena after the function artifact is emitted. Do not call a destructor for
each MIR record and do not retain MIR IDs after reset.

The arena must be thread-owned. `std::pmr` is acceptable only if profiling shows
that its resource dispatch is insignificant; a small project-local bump allocator
with inline allocation is preferred for the hot path. No mutex, atomic reference
count, or global temporary counter is allowed during ordinary instruction
construction.

### 14.7 Lowering without MIR overhead multiplying AST overhead

The lowerer must be a direct switch over resolved AST node/value kinds. It should
return a small result such as:

```cpp
struct MIRExprResult {
    uint32_t id;
    uint32_t type;
    uint8_t kind; // Value, Place, Address, Void
};
```

Required fast-path rules:

* Do not re-run canonical type lookup, overload resolution, generic
  instantiation, or implicit-conversion selection; those are already completed
  by symres/type verification.
* Do not create a C name, `std::string`, source snippet, or temporary AST node to
  lower an expression.
* Use a transient AST-node-to-MIR lookup only where required for recursive
  references. Prefer a dense side vector keyed by a lowering ordinal; use a
  pointer hash map only at the boundary and never per instruction.
* Track initialization/move/drop state in dense bitsets or byte arrays keyed by
  `PlaceId`, not hash maps keyed by `Value*`.
* Append instructions in source evaluation order. Do not first build a general
  expression DAG and then topologically sort it.
* Materialize a place only for addressability, ABI, lifetime, aliasing, or C
  sequencing. Primitive expression results should remain compact SSA IDs.
* Reserve vectors once using a cheap AST node/instruction estimate. A failed
  estimate must degrade to geometric growth, never to per-item allocation.

This replaces the current `nested_value`, `current_assignable`,
`local_allocated`, and `destructible_refs` protocol only for MIR functions. Those
maps and flags must not be recreated inside the MIR implementation under new
names.

### 14.8 C output cost model

MIR introduces an intermediate representation, so quick-mode performance must
measure both memory work and text work. Track at least:

```text
AST nodes lowered
MIR instructions and operands created
bytes allocated in the MIR arena
number of C bytes emitted
number of function artifacts
MIR construction time
C emission time
module merge time
```

The fast path should use a small inline buffer for each function emitter, for
example 16--64 KiB, and grow through chunks or geometric capacity changes. A
4 MiB allocation for every small function is wasteful. In particular, do not
construct the current `BufferedWriter` with its 4 MiB default for every worker
artifact. A small inline buffer or segmented/chunked output is required, with
growth only for functions that need it. Repeated `realloc` of a large module
buffer can also copy all prior output. A final module writer may be contiguous,
but function writers should return ownership of their already-built buffers or
segments. The merge must not copy an artifact into an intermediate string and
then copy that string again into the final output; one final append is the
maximum acceptable copy in the initial implementation.

Do not optimize only for instruction count. A pass that removes three MIR
instructions but performs a hash-map lookup, allocates an expression node, or
formats a string can make `debug_quick` slower. Every proposed fast-path change
must be benchmarked against direct legacy 2c translation on small functions,
large library modules, and many-function modules.

### 14.9 Scheduling and parallelism thresholds

Function-level parallelism is useful only when the function is large enough to
amortize task scheduling and artifact merging. Use a cheap pre-scan estimate,
such as AST node count or source byte count, and keep tiny functions in the
current worker. The threshold must be measured, not guessed, and must be
configurable for benchmarking.

The initial scheduler should submit one task per sufficiently large source file,
with each task lowering all eligible functions in that file using one task arena
and one task-local emitter. This minimizes task/context overhead. Once file-level
parallelism is working, large files may be partitioned into function groups, but
the same immutable module context and artifact rules continue to apply.

The default parallel design is:

1. Emit includes, type declarations, globals, and prototypes in deterministic
   module order.
2. Build a stable list of resolved concrete function bodies, including generic
   instantiations.
3. Lower and emit each sufficiently large function into a private artifact.
4. Process small functions in batches to avoid one thread-pool task per function.
5. Store artifacts in source-order slots and merge them serially.
6. Merge diagnostics only after workers finish; do not print from worker hot paths.

No worker may access mutable `ToCAstVisitor` state, shared temporary counters,
shared writers, or a shared allocator. Shared module type/symbol tables must be
immutable during this stage. Deterministic ordering is required even when file
order is shuffled in debug builds.

The first scheduler should use the existing compilation thread pool, but it
must not create a new pool for each module or recursively submit one task per
AST node. A module coordinator submits file tasks, waits at one explicit
barrier, and then submits any function groups selected by the measured size
threshold. A worker-local context may be reused by a thread-pool worker, but
reuse is an optimization of storage, not shared ownership: reset all arena
lengths, side-table lengths, counters, diagnostics, and references before the
next task. On cancellation or failure, discard the whole context and artifact
instead of attempting partial reuse.

For the current `ASTProcessor`, this means retaining the serial declaration,
type-alias, generic, and prototype phases. Replace the serial
`implement_module()` body loop only after constructing the sealed context and a
stable list of function bodies. The existing `ToCAstVisitor` remains a serial
module-prologue visitor during migration. It must not be made concurrently
callable by adding locks: its `current_scope`, `nested_value`, temporary
counters, aliases, `local_allocated`, `destructible_refs`, and destructor jobs
are logically per-function state mixed into one object.

### 14.10 Performance gates

MIR must not become the default merely because it passes functional tests. Add
performance gates to adoption:

* MIR `debug_quick` construction and C emission are measured against legacy 2c.
* The benchmark reports allocation count/bytes and not only wall-clock time.
* A small scalar function must not pay for debug metadata, full verification,
  hash maps, or a heap allocation per instruction.
* A function with no aggregates must not create cleanup side tables or drop flags.
* A function with no branches must not run CFG analysis.
* A C artifact must not be copied more than once before final module output.
* Any regression beyond an agreed threshold blocks enabling MIR for that path.

Keep the thresholds in benchmark output/configuration rather than hard-coding a
machine-specific nanosecond number. CPU, allocator, filesystem, TinyCC version,
and module size affect absolute time; the invariant is that MIR adds minimal
constant work and no unnecessary phase.

### 14.11 Existing pipeline bottlenecks

MIR performance cannot be evaluated in isolation from the existing compiler. The
current pipeline already spends substantial time in parsing, symbol resolution,
generic instantiation, type verification, C translation, and TinyCC/LLVM
compilation. If MIR introduces a second allocation-heavy phase, it can be slower
even when its emitter is fast.

In particular, do not use the shared AST allocator as the MIR allocator. The
existing AST allocator owns AST cleanup registration and may synchronize its
allocation path; MIR worker state must be thread-local. Likewise, do not make a
shared `ToCAstVisitor` the owner of function MIR or C output. Its current mutable
scope, temporary, alias, local-allocation, and destructor state prevents safe
parallel use.

The first performance implementation should measure these phases separately:

```text
parse
symbol resolution
generic instantiation
type verification
AST-to-MIR lowering
MIR verification, when enabled
MIR-to-C emission
artifact merge
TinyCC/LLVM compilation and linking
```

If parallel type verification or another existing phase shares an allocator,
that contention must be fixed or accounted for before claiming that MIR is the
bottleneck. Give each parallel worker scratch storage and merge only stable
results and moved diagnostics. Do not add locks to MIR to compensate for a
shared upstream allocator.

`debug_quick` should also make its policy explicit instead of deriving behavior
from a generic `is_debug()` predicate:

```text
MIR: direct construction, no optimization, no full verification, no debug tables
C: TinyCC-compatible direct emission
TCC: no debug information or extra validation
LLVM: existing O0 behavior where LLVM is selected
```

The compiler executable used for these benchmarks must itself be built in a
performance-oriented configuration. Measuring a debug-built compiler and then
attributing its overhead to MIR gives misleading results.

### 14.12 Incremental parallel implementation sequence

Parallelism must be introduced as a sequence of independently testable changes,
not as a rewrite of `ASTProcessor`. The required order is:

1. Add a worker-local MIR lowerer and C function emitter, but invoke it from
   the existing serial `implement_module()` loop. Keep the existing module
   prologue, prototypes, globals, generic declaration emission, and final
   writer unchanged. Compare MIR C and legacy C on the complete TCC suite.
2. Change the loop to produce one private artifact at a time, still serially.
   This proves artifact ownership, stable names, cleanup, fallible output, and
   legacy fallback without introducing scheduling nondeterminism.
3. Pre-enumerate and assign stable function slots, then submit independent
   file tasks to the existing thread pool. Each task reads the sealed module
   context and returns artifacts plus local diagnostics. Merge slots in source
   order and compare the merged C output and runtime behavior with the serial
   MIR path.
4. Batch small files and split only large files into function groups. Use a
   measured threshold and retain one worker arena/emitter per task. Never use
   one task per AST node or one global lock around emission.
5. Only after parallel MIR-to-C is stable, consider parallel C/TinyCC object
   compilation. Each compiler invocation must own its compiler state; the
   final linker and output publication remain coordinated separately.

Every step must be independently selectable by a feature flag and must retain
the whole-function legacy fallback. A worker failure, unsupported MIR feature,
or failed artifact validation must discard that artifact and choose the legacy
whole-function path when the configured migration mode permits it. Do not mix
legacy destructor scheduling with MIR cleanup inside one function.

The minimum verification matrix for each step is:

* serial legacy C versus serial MIR C;
* serial MIR C versus parallel MIR C;
* TinyCC execution versus a stronger C compiler when available;
* deterministic output with debug file shuffling and repeated runs;
* observable evaluation order, destructor order, lambda discovery, and generic
  instantiation coverage; and
* cancellation, unsupported-operation, allocation-failure, and diagnostic
  propagation paths.

The parallel path is correct only when these comparisons are made after the
module merge. Comparing individual worker buffers is insufficient because
declaration order, global initialization, weak interface stubs, and generated
lambda declarations are module-level semantics.

## 15. Debugging And Tooling

Implement a stable textual MIR dump early. It should show:

* module/function symbol and signature;
* blocks and predecessor/successor edges;
* instruction IDs, result types, operands, effects, and source locations;
* place ownership/lifetime state in debug mode;
* cleanup edges and drop flags;
* ABI details for calls.

Example:

```text
func main() -> i32 {
entry:
  %i0 = const i32 0                         ; source 1:9
  p0 = alloca i32, local
  store p0, %i0
  %i1 = load p0
  %i2 = mul %i1, 8                          ; pure
  %i3 = mul %i1, 2                          ; pure
  call take(%i2, %i3)                       ; call|read|write
  return const i32 0
}
```

Add `--dump-mir`, `--verify-mir`, and a per-function debug switch. When a backend
fails, print MIR IDs and source locations, not only generated C line numbers.
Keep a way to dump legacy C and MIR C side by side.

## 16. Design Rules For Future Implementers

These rules should be repeated in code-review checklists and contributor docs:

1. Lower semantic decisions once, in AST-to-MIR lowering. Backends do not
   rediscover overloads, implicit constructors, receivers, or destructor rules.
2. Never encode semantics as C text fragments in MIR.
3. Never use a value where a place/address is required.
4. Never reorder effectful instructions to make C declarations prettier.
5. Never infer cleanup from variable names or C scopes.
6. Never add a backend-specific operation to core MIR without interpreter and C
   semantics, or a documented capability rejection.
7. Never use raw pointers as long-lived MIR identity.
8. Verify after each lowering pass and before each backend in debug/validation
   builds; production builds must still use typed construction and fail safely
   on backend-detected malformed input.
9. Prefer conservative materialization over an unsafe expression optimization.
10. Add a regression test before changing lifetime or evaluation-order lowering.
11. Keep legacy and MIR paths independently valid during migration.
12. If a construct cannot be represented correctly, stop with a diagnostic rather
    than emit plausible but incorrect code.

## 17. Research Basis

This design takes specific lessons from existing representations:

* LLVM IR demonstrates the value of typed instructions, SSA values, explicit
  basic blocks, a verifier, module-level symbols, linkage, calling conventions,
  and a representation usable both in memory and as a textual/debug format.
  Chemical should adopt those properties without inheriting LLVM's assumption
  that all source-level ownership is already lowered away.
* MLIR demonstrates that multiple abstraction levels and explicit lowering
  boundaries enable incremental adoption. Chemical should use this lesson in a
  smaller form: structured regions may exist above canonical CFG, but every
  level must have a verifiable lowering.
* WebAssembly demonstrates a compact typed instruction model with explicit blocks,
  branches, validation, and a clear distinction between linear memory and
  references. Its target restrictions motivate MIR capability classification.
* The JVM demonstrates that an abstract instruction machine can leave physical
  representation to the implementation and that references are not equivalent
  to arbitrary native pointers. This supports separating portable references
  from native pointer operations in MIR.

Primary references:

* LLVM Language Reference: https://llvm.org/docs/LangRef.html
* LLVM IRBuilder/programmer documentation: https://llvm.org/docs/ProgrammersManual.html
* MLIR rationale: https://mlir.llvm.org/docs/Rationale/
* MLIR language reference: https://mlir.llvm.org/docs/LangRef/
* WebAssembly specification: https://webassembly.github.io/spec/core/
* JVM specification, structure and instruction model:
  https://docs.oracle.com/javase/specs/jvms/se21/html/jvms-2.html

## 18. Recommended First Pull Request

The first implementation pull request should not attempt to lower the whole
language. It should contain:

* `compiler/mir/` core IDs, types, instruction tags, blocks, function/module
  containers, arena allocation, and textual dump;
* a structural/type/lifetime verifier with diagnostics;
* a direct MIR builder unit-test suite;
* a C emitter for constants, primitive locals, arithmetic, scalar loads/stores,
  calls, and returns;
* a small `ToCAstVisitor` feature gate for trivial functions;
* golden tests proving `take(i * 8, i * 2, i * 4)` emits no arithmetic temporaries;
* evaluation-order tests proving effectful calls and destructible temporaries
  remain ordered;
* no changes to the default backend unless the feature gate is explicitly set.

That first pull request establishes the architecture without risking the large
existing library surface. Subsequent pull requests can expand the supported MIR
subset while every step remains testable, verifiable, and reversible.
