# Async/Await Design for Chemical

> **Status: Design Document — August 25, 2026**
>
> This document specifies how to add first-class async/await to Chemical.
> Chemical is a native language that compiles to LLVM IR and C.
> This design is written for implementers (other AIs or humans) who will
> carry out the actual compiler and library changes.

---

## Table of Contents

1. [Goals and Non-Goals](#1-goals-and-non-goals)
2. [Language Syntax](#2-language-syntax)
3. [AST Node Changes](#3-ast-node-changes)
4. [Lexer Changes](#4-lexer-changes)
5. [Parser Changes](#5-parser-changes)
6. [Symbol Resolution Changes](#6-symbol-resolution-changes)
7. [Type Verification Changes](#7-type-verification-changes)
8. [LLVM Backend Changes](#8-llvm-backend-changes)
9. [C Codegen (2c) Backend Changes](#9-c-codegen-2c-backend-changes)
10. [Interpreter Changes](#10-interpreter-changes)
11. [Runtime Library: `lang/libs/async/`](#11-runtime-library-langlibsasync)
12. [Built-in Async Types](#12-built-in-async-types)
13. [Platform-Specific Implementations](#13-platform-specific-implementations)
14. [Library Benefits and Migration](#14-library-benefits-and-migration)
15. [Implementation Phases](#15-implementation-phases)
16. [Edge Cases and Gotchas](#16-edge-cases-and-gotchas)
17. [Examples](#17-examples)

---

## 1. Goals and Non-Goals

### Goals

1. **Zero-cost abstraction** — async/await compiles to efficient C code or LLVM IR
   with no hidden allocations beyond what the user explicitly writes.
2. **Ergonomic API** — writing async code should feel like writing synchronous code.
3. **No runtime requirement** — Chemical is native; async should not require a
   garbage collector or a specific runtime. A lightweight executor is optional.
4. **Interop with existing libraries** — async functions must be callable from
   synchronous code and vice versa.
5. **Works on both LLVM and TCC backends** — the 2c (C translation) backend must
   also support async/await, using platform-specific primitives.

### Non-Goals

1. **No green threads** — async/await is cooperative, not preemptive.
2. **No structured concurrency (initially)** — can be added later as a library feature.
3. **No effect system integration (initially)** — the effect system proposal
   (`FX_SUSPENDS`) can be integrated later. Async functions are implicitly
   suspendable.

---

## 2. Language Syntax

### 2.1 Declaring an Async Function

An async function is declared with the `async` keyword before `func`:

```chemical
async func fetch_data(url: *char) : std::Result<std::string, std::string> {
    var client = http::Client()
    var response = await client.get(url)     // suspends here
    if(response is std::Result.Err) {
        return std::Result.Err(response.error)
    }
    var Ok(res) = response else unreachable
    return std::Result.Ok(res.body.read_all())
}
```

**Syntax rule:** `async` can appear before `func` at any position where `func`
is valid (top-level, inside struct, inside namespace, inside impl block).

```chemical
// Top-level async function
async func main() : int {
    var data = await fetch_data("https://example.com")
    return 0
}

// Struct method async function
struct HttpClient {
    var pool: *mut ConnectionPool

    async func request(&self, url: *char) : std::Result<Response, std::string> {
        var conn = await self.pool.acquire()
        return await conn.send(url)
    }
}

// Async function with generics
async func <T> fetch_as(url: *char) : std::Result<T, std::string> {
    var data = await fetch_data(url)
    // ... deserialize T from data
}
```

### 2.2 The `await` Expression

`await` is a unary prefix expression that can only appear inside an `async` function:

```chemical
var value = await some_async_call()    // suspend, resume when ready
var result = await future.get()         // suspend until future completes
```

**Semantic:** `await expr` evaluates `expr`, which must produce a `Future<T>`.
The current function is suspended until the future completes. The result of
the `await` expression is `T`.

### 2.3 The `Future<T>` Type

`Future<T>` is a built-in generic type. It represents a value that will be
available later. The compiler knows about `Future<T>` for type checking and
code generation.

```chemical
// Future<T> is returned by async functions
async func compute() : int {
    return 42
}
// compute() returns Future<int>

// Future<T> can be stored and awaited
var f = compute()
var result = await f
```

### 2.4 Creating Futures Manually

Users can create `Future<T>` values without `async`/`await`:

```chemical
// From a callback-based API
func fetch_async(url: *char) : Future<Response> {
    var promise = std::async::Promise<Response>()
    // ... start async work, set promise when done ...
    return promise.future()
}

// From a thread pool
func compute_async(data: *mut WorkData) : Future<Result> {
    return std::async::spawn(|data|() : Result => {
        return heavy_computation(data)
    })
}
```

### 2.5 Async Blocks (Closures)

Async closures/lambdas allow creating futures inline:

```chemical
var future = async |data|() : int => {
    return await heavy_work(data)
}

// Or with a block body:
var future = async |data|() : int => {
    var result = await fetch(data.url)
    return result.status_code
}
```

**Parser rule:** `async` before `|` or `() =>` triggers async closure parsing.
The resulting type is `Future<T>` where `T` is the closure's return type.

### 2.6 `select` Statement (Optional, Phase 2)

A `select` statement awaits multiple futures simultaneously (like Go's `select`
or Rust's `tokio::select!`):

```chemical
select {
    response = await fetch(url1) => {
        handle_response(response)
    }
    timeout = await sleep(5000) => {
        handle_timeout()
    }
    default => {
        // no future ready yet, continue
    }
}
```

**Note:** `select` is a Phase 2 feature. The initial implementation does not
require it.

---

## 3. AST Node Changes

### 3.1 New AST Node: `AsyncFuncDecl`

**File:** `ast/structures/AsyncFuncDecl.h` (new file)

```cpp
#pragma once
#include "ast/structures/FunctionDeclaration.h"

class AsyncFuncDecl : public FunctionDeclaration {
public:
    AsyncFuncDecl(
        chem::string_view identifier,
        TypeLoc returnType,
        bool isVariadic,
        ASTNode* parent_node,
        SourceLocation location,
        AccessSpecifier specifier = AccessSpecifier::Internal
    ) : FunctionDeclaration(
            identifier,
            returnType,
            isVariadic,
            parent_node,
            location,
            specifier
        ) {
        // Mark as async
        attrs.is_async = true;
    }
};
```

**Decision:** Rather than creating a separate node class, you can also add
a boolean flag `is_async` to `FuncDeclAttributes`. This is simpler and avoids
changing the visitor dispatch everywhere. **Recommended approach: add
`bool is_async = false;` to `FuncDeclAttributes` in
`ast/structures/FunctionDeclaration.h`.**

### 3.2 New Value Node: `AwaitExpression`

**File:** `ast/values/AwaitExpression.h` (new file)

```cpp
#pragma once
#include "ast/base/Value.h"

class AwaitExpression : public Value {
public:
    Value* inner;   // the expression producing a Future<T>

    AwaitExpression(Value* inner, SourceLocation location)
        : Value(ValueKind::AwaitExpr, inner->known_type(), location),
          inner(inner) {}

    // The resolved type is T, where inner produces Future<T>
    BaseType* resolved_type = nullptr;

    // During symres: extract T from Future<T> and set resolved_type
    // During codegen: generate the suspension/resumption code
};
```

### 3.3 `ValueKind` Addition

In `ast/base/ValueKind.h`, add:

```cpp
AwaitExpr,       // await <expression>
```

### 3.4 `ASTNodeKind` Addition (if using separate node)

In `ast/base/ASTNodeKind.h`, add only if creating a separate `AsyncFuncDecl`:

```cpp
AsyncFuncDecl,
```

### 3.5 `FunctionTypeBody` Changes

The function type must carry the async flag so that `Future<T>` return types
are correctly inferred:

```cpp
// In ast/types/FunctionType.h or ast/structures/FunctionTypeBody.h
struct FunctionTypeBody {
    // ... existing fields ...
    bool is_async = false;  // NEW: marks this function as async
};
```

---

## 4. Lexer Changes

### 4.1 New Token: `AsyncKw`

In `lexer/TokenType.h`, add:

```cpp
AsyncKw,    // the 'async' keyword
AwaitKw,    // the 'await' keyword
```

### 4.2 Lexer Keyword Table

In `lexer/Lexer.cpp`, add to the keyword lookup table:

```cpp
{"async", TokenType::AsyncKw},
{"await", TokenType::AwaitKw},
```

### 4.3 Reserved Words

`async` and `await` become reserved keywords. This means they cannot be used
as variable names, function names, or identifiers in existing code. A
compatibility migration may be needed if any user code uses these names.

**Check:** Search the codebase for `async` and `await` used as identifiers.
Currently, `async` is used in the HTTP server (`serve_async` method), but as a
method name suffix, not a standalone identifier — this is fine. The `await` name
is not currently used anywhere in the language code.

---

## 5. Parser Changes

### 5.1 Function Declaration Parsing

**File:** `parser/structures/Function.cpp`

In `parse_func_decl()`, add a check for `async` before the `func` keyword:

```cpp
FunctionDeclaration* Parser::parse_func_decl() {
    // NEW: check for async keyword
    bool is_async = consume_if(TokenType::AsyncKw);

    // Existing: check for 'func' keyword
    if (!consume_if(TokenType::FuncKw)) {
        // If we consumed 'async' but no 'func' follows, error
        if (is_async) {
            diagnoser.error(current, "expected 'func' after 'async'");
        }
        return nullptr;
    }

    // ... rest of function parsing ...

    // Set async flag on the created FunctionDeclaration
    func->attrs.is_async = is_async;
    // Also set on the FunctionTypeBody:
    func->FunctionTypeBody::data.is_async = is_async;

    return func;
}
```

**Where async is valid:** Before `func` at:
- Top level (free function)
- Inside struct/variant/enum bodies (method)
- Inside namespace
- Inside impl blocks

### 5.2 Await Expression Parsing

**File:** `parser/values/Expression.cpp`

In the expression parser, add `await` as a unary prefix operator with the
same precedence as `!` (logical not):

```cpp
Value* Parser::parse_unary() {
    // ... existing unary checks ...

    if (consume_if(TokenType::AwaitKw)) {
        auto inner = parse_unary();  // right-associative
        return allocator.create<AwaitExpression>(inner, inner->location());
    }

    // ... rest of unary parsing ...
}
```

**Precedence:** `await` binds tighter than binary operators but weaker than
function calls and member access. So `await f()` parses as `await (f())`,
and `await x.method()` parses as `await (x.method())`.

### 5.3 Async Closure Parsing

**File:** `parser/values/LexValue.cpp` or `parser/structures/Block.cpp`

In the lambda/closure parser, add a check for `async` before `|`:

```cpp
// In the lambda parsing code:
bool is_async = consume_if(TokenType::AsyncKw);

if (current.type == TokenType::Pipe || current.type == TokenType::OpenParen) {
    // Parse closure parameters
    auto closure = parse_closure_body();
    closure->is_async = is_async;
    // Return type becomes Future<T> where T is the closure's return type
    return closure;
}
```

### 5.4 Error Recovery

When `await` appears outside an async function, emit:

```
error: 'await' can only be used inside an async function
```

When `async` appears on a function that returns a non-async type, the type
checker handles this (not the parser). The parser just stores the flag.

---

## 6. Symbol Resolution Changes

### 6.1 `Future<T>` Type Registration

**File:** `compiler/symres/SymbolResolver.cpp`

During module initialization, register `Future<T>` as a built-in type. The
`Future<T>` type is defined in `lang/libs/async/` but the compiler needs to
recognize it for type checking:

```cpp
// In SymbolResolver::link_core_nodes() or a new link_async_nodes():
void SymbolResolver::link_async_nodes() {
    // Find Future<T> in the async module
    const auto asyncNode = find("async");
    if (!asyncNode) return;

    const auto futureNode = asyncNode->child("Future");
    if (!futureNode || futureNode->kind() != ASTNodeKind::GenericStructDecl) return;

    // Store reference for type checking
    asyncNodes.future_type = futureNode->as_generic_struct_unsafe();
}
```

### 6.2 Async Function Return Type Resolution

**File:** `compiler/symres/LinkSignature.h`

When resolving the return type of an async function:
1. If the user writes `async func foo() : int`, the actual return type is
   `Future<int>`.
2. The symres pass wraps the declared return type in `Future<T>`.

```cpp
// In TopLevelLinkSignature visit FunctionDeclaration:
if (func->attrs.is_async) {
    // Wrap return type in Future<T>
    auto future_type = create_future_type(func->returnType);
    func->returnType = TypeLoc(future_type, func->returnType.location());
}
```

### 6.3 `await` Expression Resolution

**File:** `compiler/symres/SymResLinkBody.h`

When resolving an `AwaitExpression`:
1. Resolve the inner expression.
2. Check that the inner expression's type is `Future<T>`.
3. Extract `T` and set it as the type of the `AwaitExpression`.

```cpp
// In SymResLinkBody::VisitAwaitExpression:
void SymResLinkBody::VisitAwaitExpression(AwaitExpression* expr) {
    // Resolve the inner expression first
    resolve_value(expr->inner);

    // Get the type of the inner expression
    auto type = expr->inner->known_type();

    // Check it's a Future<T>
    if (!is_future_type(type)) {
        diagnoser.error(expr, "await requires a Future<T>, got %s", type);
        return;
    }

    // Extract T from Future<T>
    expr->resolved_type = extract_future_inner_type(type);
    expr->set_known_type(expr->resolved_type);
}
```

### 6.4 Async Function Body: Implicit Suspension Points

During body resolution, the symres pass must:
1. Track that we are inside an async function body.
2. Allow `await` expressions (otherwise error).
3. Validate that closures captured by async functions do not outlive their
   captures (this is complex; initial implementation can defer this check
   and rely on the `unsafe` keyword for raw pointer captures).

---

## 7. Type Verification Changes

### 7.1 `await` Type Check

**File:** `compiler/typeverify/TypeVerify.cpp`

```cpp
// In VisitAwaitExpression:
void TypeVerify::VisitAwaitExpression(AwaitExpression* expr) {
    // Verify inner expression produces Future<T>
    auto inner_type = expr->inner->known_type();
    if (!is_future_type(inner_type)) {
        report_error(expr, "expected Future<T>, got %s", inner_type);
        return;
    }

    // Verify the type of the await expression matches the context
    auto expected = expr->expected_type();
    if (expected && !type_satisfies(expr->resolved_type, expected)) {
        report_error(expr, "type mismatch: await produces %s, expected %s",
                     expr->resolved_type, expected);
    }
}
```

### 7.2 Async Function Return Type Check

```cpp
// In VisitFunctionDeclaration:
if (func->attrs.is_async) {
    auto ret_type = func->returnType.get();
    if (!is_future_type(ret_type)) {
        // This should not happen — symres wraps it
        report_error(func, "async function must return Future<T>");
    }
}
```

### 7.3 `await` Inside Non-Async Context

```cpp
// In VisitAwaitExpression:
if (!current_function_is_async()) {
    report_error(expr, "'await' can only be used inside an async function");
}
```

---

## 8. LLVM Backend Changes

This is the most complex part. There are two viable strategies:

### Strategy A: Stackful Coroutines (Recommended for Initial Implementation)

Use platform threads with stack switching. Each async function runs on its
own thread. When `await` is called, the thread blocks on a condition variable.
The executor thread pool resumes it when the future completes.

**Pros:** Simple to implement, works with existing LLVM IR.
**Cons:** Higher memory usage per task (thread stack), context switch overhead.

### Strategy B: Stackless Coroutines (Recommended for Production)

Use LLVM's coroutine intrinsics (`llvm.coro.*`) to transform async functions
into state machines that can be resumed on any thread.

**Pros:** Very lightweight (no thread per task), O(1) memory per suspended frame.
**Cons:** Complex implementation, requires careful handling of destructors and
captured state.

**Recommendation:** Implement Strategy A first (simpler, faster to ship),
then migrate to Strategy B for performance.

### 8.1 Strategy A: Stackful Implementation

#### Async Function Codegen

An async function is compiled as a normal function that:
1. Takes a hidden `Future<T>*` parameter (the output slot).
2. Runs the body synchronously on whatever thread calls it.
3. When it hits `await`, submits itself to the executor and returns.
4. The executor resumes it later.

```cpp
// In LLVM.cpp — VisitFunctionDeclaration:
void LLVMBackendContext::code_gen_async_function(FunctionDeclaration* func) {
    // 1. Create the function with a hidden Future<T>* parameter
    auto future_type = get_future_type(func->returnType);
    auto param_types = func->param_types();
    param_types.insert(param_types.begin(), future_type->pointer_type());  // hidden param

    // 2. Generate function body as normal
    // 3. At each await expression:
    //    a. Call the executor to suspend the current task
    //    b. Store the continuation (resume point) in the Future
    //    c. Return from the function
    // 4. The executor resumes by calling back into the function at the
    //    stored resume point
}
```

#### Await Codegen

```llvm
; await some_future
%future = call %Future* @create_future()
call void @executor_suspend(%Future* %future, i8* %resume_label)
; ... code after await is in a separate basic block ...
; The executor calls @executor_resume(%Future* %future) which jumps to %resume_label
```

#### Simplified: Thread-per-Await

For the simplest implementation, `await` simply blocks the current thread:

```llvm
; Simplified await:
; 1. Get the Future<T> from the expression
; 2. Call Future::block_on() which blocks until complete
; 3. Extract T from the Future

%future = call %Future* @future_expr()
call void @future_block_on(%Future* %future)
%result = call %T* @future_get_result(%Future* %future)
```

This is the simplest possible implementation: `await` just blocks. It's
equivalent to synchronous code but with the async API surface. The real
concurrent execution comes from calling async functions from different threads
via the thread pool.

### 8.2 Strategy B: Stackless Coroutine Implementation

#### Coroutine Frame

Each async function gets a **coroutine frame** — a heap-allocated struct
containing all local variables and the current suspension point:

```cpp
// For async func foo(x: int) : int:
// The compiler generates:

struct FooCoroutineFrame {
    // Frame header
    i8 suspend_point;         // which await point are we at?
    void (*resume_fn)(void*); // function to call to resume
    void (*destroy_fn)(void*); // function to clean up

    // Saved local variables
    i32 x;                    // parameter
    i32 temp_result;          // any temps across suspend points
    // ... all locals that are live across an await point ...
};
```

#### LLVM Coroutine Intrinsics

LLVM provides these built-in intrinsics:

```
llvm.coro.id      — identify a coroutine
llvm.coro.suspend — suspend at an await point
llvm.coro.resume  — resume a suspended coroutine
llvm.coro.destroy — destroy a coroutine frame
llvm.coro.done    — check if coroutine is complete
llvm.coro.alloc   — check if coroutine frame needs allocation
llvm.coro.begin   — begin coroutine execution
llvm.coro.end     — end coroutine execution
llvm.coro.save    — save coroutine state
llvm.coro.free    — free coroutine frame
```

#### Codegen Pattern

```llvm
; Async function: async func add(a: int, b: int) : int

define i32 @add(i32 %a, i32 %b) {
entry:
  ; Allocate coroutine frame
  %id = call token @llvm.coro.id(i32 0, ptr null, ptr null, ptr null)
  %size = call i64 @llvm.coro.size.i64()
  %frame = call ptr @malloc(i64 %size)
  %hdl = call ptr @llvm.coro.begin(token %id, ptr %frame)

  ; Save state and suspend at await point
  %state = call i8 @llvm.coro.suspend(token none, i1 false)
  switch i8 %state, label %suspend [
    i8 0, label %resume    ; resumed
    i8 1, label %cleanup   ; destroyed
  ]

resume:
  ; Code after the await point
  ; ...

cleanup:
  call ptr @llvm.coro.free(token %id, ptr %hdl)
  br label %suspend

suspend:
  call i1 @llvm.coro.end(ptr %hdl, i1 false, token none)
  ret i32 0
}
```

#### Chemical-Specific Codegen (Strategy B)

**File:** `compiler/backend/LLVM.cpp`

```cpp
void LLVMBackendContext::code_gen_async_function(FunctionDeclaration* func) {
    // 1. Mark the LLVM function as a coroutine
    auto llvm_func = get_or_create_function(func);
    llvm_func->setDoesNotThrow();
    llvm_func->setCallingConv(llvm::CallingConv::Fast);

    // 2. Create the coroutine id and frame allocation
    auto coro_id = builder.CreateCall(
        Intrinsic::coro_id,
        {builder.getInt32(0), null_ptr, null_ptr, null_ptr}
    );

    // 3. Generate the function body normally
    generate_function_body(func);

    // 4. At each AwaitExpression:
    //    a. Save current state (coro.save)
    //    b. Call llvm.coro.suspend
    //    c. Branch to resume/cleanup/suspend based on return value
}
```

### 8.3 Recommendation

**Use Strategy A (simplified blocking) for the initial implementation.** This
gives users the async/await API immediately. The implementation is:

1. `async func foo() : T` compiles to a normal C function `void foo(Future_T* __future)`.
2. `await expr` compiles to: call `expr`, then call `Future::block_on()`, then
   extract the result.
3. The `Future<T>` is implemented in the runtime library as a mutex+condvar
   wrapper (like the existing `std::concurrent::Promise<T>`).

Later, when the stackless coroutine implementation is ready:
1. Functions with `await` get a coroutine frame.
2. `await` uses `llvm.coro.suspend`.
3. The executor schedules continuations instead of blocking.

---

## 9. C Codegen (2c) Backend Changes

### 9.1 Strategy A (Blocking) — Simple

```c
// Chemical: async func fetch(url: *char) : int
// C translation:
void fetch(int* __result, const char* url) {
    // ... body ...
    // await some_future:
    future_block_on(some_future);     // blocks until ready
    int value = future_get_int(some_future);  // extract result
    // ... continue ...
    *__result = value;                // store in Future<T>
}
```

### 9.2 Platform-Specific Coroutine Support

For stackless coroutines (Strategy B), the 2c backend would need to generate
platform-specific code:

**Linux/macOS (setjmp/longjmp or ucontext):**
```c
#include <ucontext.h>

typedef struct {
    ucontext_t ctx;
    int suspend_point;
    char stack[1024 * 1024];  // 1MB stack for the coroutine
} CoroutineFrame;
```

**Windows (Fiber API):**
```c
#include <windows.h>

typedef struct {
    LPVOID fiber;
    int suspend_point;
} CoroutineFrame;
```

**Alternative: cooperative scheduler with setjmp/longjmp** (no extra deps):
```c
#include <setjmp.h>

typedef struct {
    jmp_buf env;
    int suspend_point;
    // saved locals across suspend points
} AsyncFrame;
```

### 9.3 Minimal 2c Changes

For the initial blocking implementation, the 2c backend changes are minimal:
- Async functions are translated as normal C functions with a `Future<T>*` hidden parameter.
- `await` calls `future_block_on()` then `future_get_result()`.

```cpp
// In preprocess/2c/2cASTVisitor.cpp:

void ToCAstVisitor::VisitAwaitExpression(AwaitExpression* expr) {
    // Generate: future_block_on(<expr>); future_get_result(<expr>)
    VisitNode(expr->inner);
    writer.write("future_block_on(");
    visit_inner(expr->inner);
    writer.write(");\n");

    // The result is extracted from the future
    writer.write("future_get_result(");
    visit_inner(expr->inner);
    writer.write(")");
}
```

---

## 10. Interpreter Changes

### 10.1 Strategy A (Blocking) — No Interpreter Changes

In interpretation mode, `await` simply evaluates the inner expression,
blocks on the result (if it's a `Future<T>`), and returns the value.
The interpreter already supports blocking operations via the thread pool.

### 10.2 Comptime Compatibility

`async` functions can be called at comptime only if:
1. They don't actually suspend (no `await` in the body, or the awaited futures
   are already resolved).
2. The comptime interpreter runs everything synchronously.

If an `async` function tries to suspend at comptime, the compiler emits:

```
error: cannot suspend at compile time
```

---

## 11. Runtime Library: `lang/libs/async/`

### 11.1 Module Structure

```
lang/libs/async/
├── chemical.mod
└── src/
    ├── main.ch           # exports
    ├── future.ch         # Future<T> type
    ├── promise.ch        # Promise<T> type (internal)
    ├── executor.ch       # Task executor
    ├── spawn.ch          # spawn/spawn_blocking helpers
    └── channel.ch        # Channel<T> for cross-task communication
```

### 11.2 `chemical.mod`

```
application async
source "src"
import std
import cstd
```

### 11.3 `Future<T>`

**File:** `lang/libs/async/src/future.ch`

```chemical
public namespace std {

    public namespace async {

        // Status of a future
        public enum FutureStatus {
            Pending,
            Ready,
            Cancelled
        }

        // A Future<T> represents a value that will be available later.
        // It wraps a Promise<T> and provides methods to wait for the result.
        public struct Future<T> {
            var promise : *mut PromiseState<T>
            var status : FutureStatus

            @constructor
            func constructor(p : *mut PromiseState<T>) {
                return Future<T> {
                    promise = p,
                    status = FutureStatus.Pending
                }
            }

            // Block until the future completes and return the value.
            // This is what `await` compiles to in Strategy A.
            public func block_on(&mut self) : T {
                if(self.status == FutureStatus.Ready) {
                    return self.promise.get_value()
                }
                self.promise.wait_until_ready()
                self.status = FutureStatus.Ready
                return self.promise.take_value()
            }

            // Non-blocking check
            public func is_ready(&self) : bool {
                return self.status == FutureStatus.Ready
            }

            // Get a reference to the result (only valid after block_on)
            public func result(&self) : &T {
                return self.promise.get_value_ref()
            }

            // Map/transform the result
            public func <U> map(&mut self, f : (T) => U) : Future<U> {
                var new_promise = Promise<U>()
                var new_state = new_promise.state
                // When this future completes, apply f and complete new_promise
                self.promise.set_continuation(|self, new_state, f|() : void => {
                    var val = self.block_on()
                    var mapped = f(val)
                    new_state.set_value(mapped)
                })
                return new_promise.future()
            }

            // Chain two futures sequentially
            public func <U> then(&mut self, f : (T) => Future<U>) : Future<U> {
                var new_promise = Promise<U>()
                var new_state = new_promise.state
                self.promise.set_continuation(|self, new_state, f|() : void => {
                    var val = self.block_on()
                    var next = f(val)
                    var result = next.block_on()
                    new_state.set_value(result)
                })
                return new_promise.future()
            }

            @delete
            func delete(&mut self) {
                if(promise != null) {
                    promise.release()
                    promise = null
                }
            }
        }

    }  // namespace async

}  // namespace std
```

### 11.4 `Promise<T>`

**File:** `lang/libs/async/src/promise.ch`

```chemical
namespace std {
namespace async {

    // PromiseState is the internal state shared between Promise and Future.
    // It's heap-allocated and reference-counted.
    public struct PromiseState<T> {
        var value : T
        var ready : bool
        var mutex : std::mutex
        var condvar : std::condvar
        var ref_count : u32
        var continuation : std::function<() => void>

        @constructor
        func constructor() {
            unsafe var default_val : T
            return PromiseState<T> {
                value = default_val,
                ready = false,
                mutex = std::mutex(),
                condvar = std::condvar(),
                ref_count = 2u,
                continuation = std::function<() => void>()
            }
        }

        public func set_value(&mut self, val : T) {
            mutex.lock()
            value = val
            ready = true
            condvar.notify_all()
            var has_cont = !continuation.is_empty()
            mutex.unlock()
            if(has_cont) {
                continuation()
            }
        }

        public func wait_until_ready(&mut self) {
            mutex.lock()
            while(!ready) {
                condvar.wait(&mut mutex)
            }
            mutex.unlock()
        }

        public func get_value(&self) : T {
            return value
        }

        public func get_value_ref(&self) : &T {
            return &value
        }

        public func take_value(&mut self) : T {
            var temp : T = value
            return temp
        }

        public func set_continuation(&mut self, f : std::function<() => void>) {
            mutex.lock()
            continuation = f
            var is_ready = self.ready
            mutex.unlock()
            if(is_ready) {
                f()
            }
        }

        public func release(&mut self) {
            ref_count = ref_count - 1u
            if(ref_count == 0u) {
                // TODO: free self
            }
        }

        @delete
        func delete(&mut self) {
            // Don't free here — use release()
        }
    }

    // Promise<T> is the producer side. Setting a value completes the future.
    public struct Promise<T> {
        var state : *mut PromiseState<T>

        @constructor
        func constructor() {
            var s = malloc(sizeof(PromiseState<T>)) as *mut PromiseState<T>
            new(s) PromiseState<T>()
            return Promise<T> {
                state = s
            }
        }

        public func set_value(&mut self, val : T) {
            state.set_value(val)
        }

        public func future(&self) : Future<T> {
            return Future<T>(state)
        }

        @delete
        func delete(&mut self) {
            if(state != null) {
                state.release()
                state = null
            }
        }
    }

}  // namespace async
}  // namespace std
```

### 11.5 `spawn` and `spawn_blocking`

**File:** `lang/libs/async/src/spawn.ch`

```chemical
namespace std {
namespace async {

    // Spawn an async task on the executor thread pool
    public func spawn<T>(task : std::function<() => T>) : Future<T> {
        var promise = Promise<T>()
        var future = promise.future()
        std::global_executor.submit(|task, promise|() : void => {
            var result = task()
            promise.set_value(result)
        })
        return future
    }

    // Spawn a blocking task on a dedicated blocking thread
    public func spawn_blocking<T>(task : std::function<() => T>) : Future<T> {
        var promise = Promise<T>()
        var future = promise.future()
        std::global_blocking_executor.submit(|task, promise|() : void => {
            var result = task()
            promise.set_value(result)
        })
        return future
    }

    // Sleep for a duration (async version)
    public async func sleep(ms : u64) : void {
        // Use the executor's timer facility
        // For Strategy A, this just blocks the thread
        std::concurrent::sleep_ms(ms)
    }

}  // namespace async
}  // namespace std
```

### 11.6 Executor

**File:** `lang/libs/async/src/executor.ch`

```chemical
namespace std {
namespace async {

    // Global async executor (thread pool based)
    public var global_executor : std.concurrent.ThreadPool
    public var global_blocking_executor : std.concurrent.ThreadPool

    // Initialize the global executors
    func init_executors() {
        var hw = std::concurrent.hardware_threads()
        global_executor = std::concurrent.create_pool(hw as uint)
        global_blocking_executor = std::concurrent.create_pool(
            (hw * 2u) as uint   // more threads for blocking tasks
        )
    }

    // Shutdown the global executors
    func shutdown_executors() {
        // ThreadPool destructor handles this
        delete global_executor
        delete global_blocking_executor
    }

}  // namespace async
}  // namespace std
```

---

## 12. Built-in Async Types

### 12.1 Compiler-Aware Types

The compiler must know about these types for correct type checking:

| Type | Purpose | Compiler Knowledge |
|------|---------|-------------------|
| `Future<T>` | Represents an async result | Required for `await` type checking |
| `Promise<T>` | Producer side of a future | Library type, no compiler magic |

### 12.2 How `async func` Return Type Works

When a user writes:
```chemical
async func compute() : int {
    return 42
}
```

The compiler internally transforms the return type to `Future<int>`. The
function's actual compiled signature is:
```c
void compute(Future_int* __future);  // C translation
```

When the user writes `var result = await compute()`:
1. Call `compute(&temp_future)`.
2. Call `temp_future.block_on()`.
3. Extract `int` from the future.

---

## 13. Platform-Specific Implementations

### 13.1 Windows

```chemical
// lang/libs/async/src/platform/windows.ch

// Windows: use IOCP for async I/O
@extern public func CreateIoCompletionPort(
    FileHandle: HANDLE,
    ExistingCompletionPort: HANDLE,
    CompletionKey: usize,
    NumberOfConcurrentThreads: u32
) : HANDLE

@extern public func GetQueuedCompletionStatus(
    CompletionPort: HANDLE,
    lpNumberOfBytesTransferred: *mut u32,
    lpCompletionKey: *mut usize,
    lpOverlapped: *mut*mut void,
    dwMilliseconds: u32
) : int
```

### 13.2 Linux/macOS

```chemical
// lang/libs/async/src/platform/posix.ch

// POSIX: use epoll (Linux) or kqueue (macOS)
@extern public func epoll_create1(flags: int) : int
@extern public func epoll_ctl(epfd: int, op: int, fd: int, event: *mut void) : int
@extern public func epoll_wait(epfd: int, events: *mut void, maxevents: int, timeout: int) : int
```

---

## 14. Library Benefits and Migration

### 14.1 `http` Library

**Current state:** The HTTP server uses a thread pool with callback-based
async. The server's `handle_conn` is synchronous, blocking the worker thread.

**With async/await:**

```chemical
// Before (current):
func handle_conn(&self, s: net::Socket) {
    var req_opt = http::read_request_incremental(s, ...)
    // ... synchronous processing ...
    route.handler(req, resw)
}

// After:
async func handle_conn(&self, s: net::Socket) {
    var req = await http::read_request(s)       // async I/O
    var body = await req.read_body()             // async body read
    var resw = http::ResponseWriter(s)
    route.handler(req, resw)                     // sync handler
    await resw.flush()                           // async write
}
```

**Benefits:**
- Worker threads are not blocked during I/O waits.
- More concurrent connections with fewer threads.
- Simpler code (no nested callbacks).

### 14.2 `http` Client

**Current state:** `http::Client::request()` is synchronous and blocks
until the response is received.

**With async/await:**

```chemical
// Before:
var response = client.get(url)

// After:
var response = await client.get_async(url)
```

**Benefits:**
- Non-blocking HTTP requests.
- Can issue multiple requests concurrently.

### 14.3 `net` Library

**Current state:** `net::dial()`, `net::send_all()`, `net::recv_all()` are
all blocking.

**With async/await:**

```chemical
// Before:
var s = net::dial(host, port)
net::send_all(s, data, len)
var n = net::recv_all(s, buf, cap)

// After:
var s = await net::dial_async(host, port)
await net::send_all_async(s, data, len)
var n = await net::recv_all_async(s, buf, cap)
```

**Benefits:**
- Non-blocking network I/O.
- Can multiplex multiple connections on a single thread.

### 14.4 `tls` Library

The TLS handshake is a blocking operation that benefits from async:

```chemical
// Before:
tls::tls_connect(ssl, host, port)

// After:
await tls::tls_connect_async(ssl, host, port)
```

### 14.5 `fs` Library (Future)

File I/O can be made async with platform-specific APIs:
- Linux: `io_uring` or `aio`
- Windows: IOCP
- macOS: `kqueue`

```chemical
var content = await fs::read_file_async("data.txt")
await fs::write_file_async("output.txt", content)
```

### 14.6 `process` Library

The `process` library can benefit from async for non-blocking process I/O:

```chemical
var proc = await process::Command::new("git")
    .arg("status")
    .spawn_async()
var output = await proc.read_stdout()
```

### 14.7 Migration Path

For libraries that are currently callback-based, the migration is:

1. **Add `*_async` variants** alongside existing sync functions.
2. **Keep backward compatibility** — existing sync functions still work.
3. **Mark old callbacks as deprecated** when async versions are ready.

Example migration for `http::Client`:

```chemical
// Phase 1: Add async variants
async func get_async(&self, url: &std::string_view) : std::Result<Response, std::string> {
    // ... async implementation ...
}

// Phase 2: Deprecate sync versions (optional)
@deprecated("use get_async() instead")
func get(&self, url: &std::string_view) : std::Result<Response, std::string> {
    return self.get_async(url).block_on()
}
```

---

## 15. Implementation Phases

### Phase 1: Language Syntax + Blocking Implementation (4-6 weeks)

1. **Lexer:** Add `AsyncKw` and `AwaitKw` token types.
2. **Parser:** Parse `async func`, `await expr`, async closures.
3. **AST:** Add `is_async` flag to `FuncDeclAttributes`, `AwaitExpression` value node.
4. **Symres:** Register `Future<T>` type, resolve async return types, validate await usage.
5. **TypeVerify:** Check await produces `Future<T>`, check non-async context.
6. **LLVM Backend:** Compile `await` as `future.block_on()` (blocking).
7. **C Codegen:** Translate `await` to `future_block_on()` call.
8. **Runtime Library:** Create `lang/libs/async/` with `Future<T>`, `Promise<T>`, executor.
9. **Tests:** Write tests for async functions, await, futures, closures.

### Phase 2: Non-Blocking Executor (2-3 weeks)

1. **Executor:** Implement a proper work-stealing executor.
2. **I/O Integration:** Add async wrappers for `net`, `tls`, `http`.
3. **Timer Support:** Add `async::sleep()` using platform timers.
4. **Channels:** Add `Channel<T>` for cross-task communication.

### Phase 3: Stackless Coroutines (4-8 weeks)

1. **LLVM Coroutines:** Use `llvm.coro.*` intrinsics for stackless suspension.
2. **Frame Layout:** Compute which locals are live across suspend points.
3. **Destructor Handling:** Ensure destructors run correctly at suspend points.
4. **Memory Management:** Implement coroutine frame allocation and deallocation.
5. **C Codegen:** Use platform-specific primitives (ucontext, fibers, setjmp).

### Phase 4: Advanced Features (Ongoing)

1. **Structured Concurrency:** `async let`, task groups, cancellation.
2. **Select Statement:** `select {}` for multiplexing.
3. **Effect System Integration:** `FX_SUSPENDS` effect bit.
4. **Async Traits:** Interface methods can be async.
5. **Async Generics:** `async func <T> fetch_as() : T`.

---

## 16. Edge Cases and Gotchas

### 16.1 Move Semantics with Futures

`Future<T>` has a destructor (`@delete`). Moving a `Future<T>` is fine
(Chemical's move semantics handle this). But after moving, the original
variable becomes invalid:

```chemical
var f = compute()
var g = f           // move: f is now invalid
var result = await g   // OK
// await f           // ERROR: f was moved
```

### 16.2 Borrowing Across Await Points

**Critical:** References borrowed before an `await` may become dangling
after the await resumes, because the function may be resumed on a different
stack/thread.

```chemical
// DANGEROUS — don't do this:
var data = vector<int>()
var slice = &data          // borrow reference
await fetch()             // suspend — data might move!
slice[0]                  // use-after-move potential

// SAFE pattern:
var data = vector<int>()
await fetch()             // suspend first
var slice = &data         // borrow after await
slice[0]                  // OK
```

**Compiler enforcement (Phase 3):** The type checker should track borrows
across await points and reject code where a reference's lifetime crosses
a suspension point. For Phase 1 (blocking implementation), this is not an
issue because the stack doesn't move.

### 16.3 Comptime Context

`await` cannot be used at comptime:

```chemical
comptime func bad() : int {
    var x = await compute()    // ERROR: cannot suspend at compile time
    return x
}
```

### 16.4 Error Propagation

`await` works with `Result`:

```chemical
async func fetch(url: *char) : std::Result<Response, std::string> {
    var client = http::Client()
    var response = await client.get_async(url)
    // response is Result<Response, std::string>
    return response
}

async func main() : int {
    var result = await fetch("https://example.com")
    if(result is std::Result.Err) {
        printf("Error: %s\n", result.error.data())
        return 1
    }
    var Ok(response) = result else unreachable
    printf("Status: %d\n", response.status)
    return 0
}
```

### 16.5 Async Function Pointers

Function pointers to async functions have type `(args...) => Future<T>`:

```chemical
var fptr : () => Future<int> = compute
var result = await fptr()
```

### 16.6 Async Methods on Structs

```chemical
struct Database {
    var pool: *mut ConnectionPool

    async func query(&self, sql: *char) : std::Result<ResultSet, std::string> {
        var conn = await self.pool.acquire_async()
        var result = await conn.execute_async(sql)
        return result
    }
}
```

The `&self` parameter is captured in the future. The borrow must not outlive
the future (handled by the borrow checker in Phase 3).

### 16.7 Recursion

Async functions can call themselves recursively. Each recursive call creates
a new `Future<T>`. The stackful implementation (Strategy A) handles this
naturally. The stackless implementation (Strategy B) needs to handle
self-recursive coroutines specially (may need to allocate a new frame on each
recursive call).

---

## 17. Examples

### 17.1 Simple Async Function

```chemical
import std
import async

async func greet(name: *char) : std::string {
    await async::sleep(1000)   // wait 1 second
    var s = std::string("Hello, ")
    s.append_view(name)
    return s
}

public func main() : int {
    var greeting = await greet("World")
    println(greeting)
    return 0
}
```

### 17.2 Multiple Concurrent Tasks

```chemical
import std
import async

async func fetch_user(id: int) : std::string {
    // Simulate async I/O
    await async::sleep(100)
    return std::string("User_") + id
}

async func fetch_orders(user_id: int) : std::string {
    await async::sleep(200)
    return std::string("Orders_") + user_id
}

public func main() : int {
    // Start both tasks concurrently
    var user_future = async::spawn(| |() : std::string => {
        return fetch_user(1)
    })
    var orders_future = async::spawn(| |() : std::string => {
        return fetch_orders(1)
    })

    // Wait for both
    var user = await user_future
    var orders = await orders_future

    println("User: ${user}")
    println("Orders: ${orders}")
    return 0
}
```

### 17.3 HTTP Server with Async

```chemical
import std
import http
import async

async func handle_request(req: http::Request, res: http::ResponseWriter) {
    var body = await req.read_body()

    if(req.path.equals_with_len("/api/data", 9)) {
        var data = await fetch_from_database()
        res.set_header("Content-Type", "application/json")
        res.write_string(data)
    } else if(req.path.equals_with_len("/api/users", 10)) {
        var users = await fetch_users()
        res.write_string(users)
    } else {
        res.status = 404
        res.write_string("Not Found")
    }
}

public func main() : int {
    var server = http::server::Server()
    server.router.add_handler("GET", "/api/*", |req, res|() : void => {
        async::spawn(|req, res|() : void => {
            handle_request(req, res)
        })
    })
    server.start(8080)
    server.serve_non_iocp()
    return 0
}
```

### 17.4 Async with Error Handling

```chemical
async func safe_fetch(url: *char) : std::Result<std::string, std::string> {
    var client = http::Client()
    var response = await client.get_async(url)
    if(response is std::Result.Err) {
        return std::Result.Err(response.error)
    }
    var Ok(res) = response else unreachable
    return std::Result.Ok(res.body.read_all())
}

async func main() : int {
    var result = await safe_fetch("https://api.example.com/data")
    switch(result) {
        std::Result.Ok(data) => {
            println("Data: ${data}")
        }
        std::Result.Err(err) => {
            println("Error: ${err}")
        }
    }
    return 0
}
```

### 17.5 Async Generator (Future Enhancement)

```chemical
// Phase 4 feature
async func read_lines(path: *char) : AsyncIterator<std::string> {
    var file = await fs::open_async(path)
    while(var line = await file.read_line_async()) {
        yield line
    }
}

public func main() : int {
    var lines = read_lines("data.txt")
    for(var line in lines) {
        println(line)
    }
    return 0
}
```

---

## Appendix A: Implementation Checklist

- [ ] Add `AsyncKw` and `AwaitKw` to `lexer/TokenType.h`
- [ ] Add keywords to `lexer/Lexer.cpp` keyword table
- [ ] Add `is_async` to `FuncDeclAttributes` in `ast/structures/FunctionDeclaration.h`
- [ ] Add `AwaitExpression` to `ast/values/AwaitExpression.h`
- [ ] Add `AwaitExpr` to `ValueKind` enum in `ast/base/ValueKind.h`
- [ ] Parse `async func` in `parser/structures/Function.cpp`
- [ ] Parse `await expr` in `parser/values/Expression.cpp`
- [ ] Parse async closures in `parser/structures/Block.cpp`
- [ ] Register `Future<T>` in `compiler/symres/SymbolResolver.cpp`
- [ ] Resolve async return types in `compiler/symres/LinkSignature.h`
- [ ] Validate await expressions in `compiler/symres/SymResLinkBody.h`
- [ ] Type-check await in `compiler/typeverify/TypeVerify.cpp`
- [ ] Codegen await as blocking in `compiler/backend/LLVM.cpp`
- [ ] Codegen await as blocking in `preprocess/2c/2cASTVisitor.cpp`
- [ ] Create `lang/libs/async/chemical.mod`
- [ ] Create `lang/libs/async/src/future.ch`
- [ ] Create `lang/libs/async/src/promise.ch`
- [ ] Create `lang/libs/async/src/spawn.ch`
- [ ] Create `lang/libs/async/src/executor.ch`
- [ ] Write tests in `lang/tests/async/`

## Appendix B: Files to Modify

| File | Change Type | Description |
|------|-------------|-------------|
| `lexer/TokenType.h` | Add enum values | `AsyncKw`, `AwaitKw` |
| `lexer/Lexer.cpp` | Add keyword mapping | `"async"` → `AsyncKw`, `"await"` → `AwaitKw` |
| `ast/base/ValueKind.h` | Add enum value | `AwaitExpr` |
| `ast/base/ASTNodeKind.h` | Add enum value (optional) | `AsyncFuncDecl` (only if separate node) |
| `ast/structures/FunctionDeclaration.h` | Add field | `bool is_async` in `FuncDeclAttributes` |
| `ast/values/AwaitExpression.h` | New file | `AwaitExpression` AST node |
| `parser/structures/Function.cpp` | Modify | Parse `async` before `func` |
| `parser/values/Expression.cpp` | Modify | Parse `await expr` |
| `parser/structures/Block.cpp` | Modify | Parse `async` closures |
| `compiler/symres/SymbolResolver.cpp` | Modify | Register `Future<T>` |
| `compiler/symres/LinkSignature.h` | Modify | Wrap async return type in `Future<T>` |
| `compiler/symres/SymResLinkBody.h` | Modify | Resolve `AwaitExpression` |
| `compiler/typeverify/TypeVerify.cpp` | Modify | Type-check `await` |
| `compiler/backend/LLVM.cpp` | Modify | Codegen for async functions and await |
| `preprocess/2c/2cASTVisitor.cpp` | Modify | C translation for async/await |
| `compiler/Interpreter/Core.cpp` | Modify | Interpret `await` (blocking) |
| `lang/libs/async/` | New directory | Runtime library |

## Appendix C: Reference Material

### Existing Concurrency Primitives

- `std::concurrent::ThreadPool` — `lang/libs/std/src/concurrency/threadpool.ch`
- `std::concurrent::Future<T>` — `lang/libs/std/src/concurrency/threadpool.ch`
- `std::concurrent::Promise<T>` — `lang/libs/std/src/concurrency/threadpool.ch`
- `std::concurrent::Thread` — `lang/libs/std/src/concurrency/threadpool.ch`
- `std::mutex` — `lang/libs/std/src/mutex.ch` (wraps platform mutex)
- `std::condvar` — platform condition variable
- `std::function<T>` — `lang/libs/std/src/function.ch` (closure type)

### Key Compiler Files

- `ast/base/ASTNodeKind.h` — all AST node kinds
- `ast/base/ValueKind.h` — all value kinds
- `ast/structures/FunctionDeclaration.h` — function declaration node
- `ast/values/FunctionCall.h` — function call node
- `parser/structures/Function.cpp` — function parsing
- `parser/values/Expression.cpp` — expression parsing
- `compiler/symres/SymbolResolver.cpp` — symbol resolution orchestration
- `compiler/symres/SymResLinkBody.h` — body linking
- `compiler/typeverify/TypeVerify.cpp` — type verification
- `compiler/backend/LLVM.cpp` — LLVM codegen
- `preprocess/2c/2cASTVisitor.cpp` — C translation codegen

### Effect System Integration Points

- `FX_SUSPENDS` bit in `compiler/effects/EffectBits.h` (from effect system proposal)
- `FunctionDeclaration::attrs.is_async` maps to `FX_SUSPENDS` during effect computation
- Future: `[pure]` functions cannot call `async` functions (unless also marked `[async]`)
