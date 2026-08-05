# Plan: `%runtime_block_value<T>` Magic Value + `std::string::make(expr: %expressive_string)`

## Goal

Enable `std::string(\`hello ${name}, you are ${age} years old\`)` by:

1.  Adding a new magic value expression `%runtime_block_value<T> { ... }` that tags a block body to be evaluated at runtime, with its type constrained by `<T>` (optional).
2.  Improving `evaluated_comptime()` in `GlobalFunctions.cpp` to use a recursive visitor pattern, correctly replacing comptime identifiers with their evaluated values inside nested nodes.
3.  Wiring these together so `std::string` gets a `comptime func make(expr : %expressive_string)` that uses `%runtime_block_value<std::string> { ... }` to construct the string.

---

## Step 1 — Add `RuntimeBlockValue` value class

**Files to create/modify:**

### 1.1 `ast/base/ValueKind.h` — Add enum entry

Add `RuntimeBlockValue` to the `ValueKind` enum (after `RuntimeValue`, around line 54):

```cpp
RuntimeValue,
RuntimeBlockValue,   // <-- new
```

### 1.2 `ast/values/RuntimeBlockValue.h` — Create value class (NEW FILE)

```cpp
#pragma once
#include "ast/base/Value.h"
#include "ast/structures/Scope.h"

class RuntimeBlockValue : public Value {
public:
    Scope scope;                      // the block body
    bool has_explicit_type = false;   // true if <T> was provided

    RuntimeBlockValue(ASTNode* parent, Scope scope, BaseType* explicitType, SourceLocation loc)
        : Value(ValueKind::RuntimeBlockValue, explicitType ? explicitType : nullptr, loc),
          scope(std::move(scope)),
          has_explicit_type(explicitType != nullptr) {}

    Value* get_last_value();          // returns the last node in scope if it's a Value

    Value* copy(ASTAllocator& allocator) override;

#ifdef COMPILER_BUILD
    // LLVM codegen delegates will be added (see Step 9)
#endif
};
```

**Key design decisions:**

- The type is either the `<T>` from the source (if provided) or `nullptr` (to be inferred from the last expression during symres).
- `has_explicit_type` controls whether SymRes should error if the type doesn't match.
- The `Scope` holds the block's statements (same type as `BlockValue::scope`).

### 1.3 `ast/values/RuntimeBlockValue.cpp` — Create implementation (NEW FILE)

```cpp
#include "RuntimeBlockValue.h"

Value* RuntimeBlockValue::get_last_value() {
    if (scope.nodes.empty()) return nullptr;
    auto last = scope.nodes.back();
    // if the last node has a known value, return it
    if (auto val = dynamic_cast<Value*>(last)) return val;
    // Could also check for ExpressionStatement wrapping a Value
    return nullptr;
}

Value* RuntimeBlockValue::copy(ASTAllocator& allocator) {
    auto copy = new (allocator.allocate<RuntimeBlockValue>())
        RuntimeBlockValue(scope.parent(), Scope{scope.parent(), scope.encoded_location()},
                          getType(), encoded_location());
    scope.copy_into(copy->scope, allocator, scope.parent());
    copy->has_explicit_type = has_explicit_type;
    return copy;
}
```

### 1.4 Register the value in `ast/base/Value.h`

Add helper methods matching the existing pattern:

```cpp
// in Value.h
bool isRuntimeBlockValue() const {
    return kind() == ValueKind::RuntimeBlockValue;
}
RuntimeBlockValue* as_runtime_block_value_unsafe() {
    return (RuntimeBlockValue*) this;
}
```

Also add the forward declaration:

```cpp
class RuntimeBlockValue;
```

---

## Step 2 — Parser: Parse `%runtime_block_value<TypeOpt> { ... }`

**File: `parser/utils/LexValue.cpp`** — `parseMagicValue()` (line 789)

### 2.1 Add new case in the hasher switch

```cpp
case hasher("runtime_block_value"): {
    // Optional: parse <Type> (guarded by lookahead for '<')
    BaseType* explicitType = nullptr;
    if (token->type == TokenType::LessThanSym) {
        token++;  // consume '<'
        explicitType = parseTypeLoc(allocator);
        if (!consumeGenericClose()) {
            error("expected '>' after type in %runtime_block_value<T>");
        }
    }

    // Parse the block body { ... }
    if (token->type != TokenType::LBrace) {
        error("expected '{' after %runtime_block_value");
    }
    auto scope = parseNestedBraceBlock("runtime_block_value", allocator);
    if (!scope) {
        error("expected a block body for %runtime_block_value");
    }

    return new (allocator.allocate<RuntimeBlockValue>())
        RuntimeBlockValue(current_function ? current_function : current_scope,
                          std::move(*scope), explicitType, loc);
}
```

**Important:** `parseNestedBraceBlock` exists in `parser/structures/Block.cpp` and returns `Scope*`. We need to check its exact signature and adapt.

**Alternative** if `parseNestedBraceBlock` returns `Scope` by value:

```cpp
auto scope = parseNestedBraceBlock("runtime_block_value", allocator);
return new (allocator.allocate<RuntimeBlockValue>())
    RuntimeBlockValue(parent_node, std::move(scope), explicitType, loc);
```

### 2.2 Ensure `consumeGenericClose()` is available

`consumeGenericClose()` (in `LexType.cpp:71`) already handles `>>` as two `>` tokens. It's a `Parser` method, so accessible from `LexValue.cpp`.

### 2.3 Verify `%` dispatch catches it

`parseAccessChainOrValueNoAfter` (LexValue.cpp:854) already dispatches `TokenType::ModSym` to `parseMagicValue`. No change needed.

---

## Step 3 — Parser: `parseNestedBraceBlock` investigation

**File: `parser/structures/Block.cpp`**

Read the signature of `parseNestedBraceBlock`. If it returns `Scope` by value (not pointer), you can `std::move` it into `RuntimeBlockValue`.

If it returns `Scope*` (heap allocated), change the approach accordingly.

**Fallback:** If `parseNestedBraceBlock` creates a `BlockScope` node, we may need a new helper that creates a plain `Scope` (without the scope-start/scope-end that `BlockScope` does). Or we can use `Scope` directly via the parser's `parseScopeBlock`.

---

## Step 4 — NonRecursiveVisitor: Add `VisitRuntimeBlockValue`

**File: `preprocess/visitors/NonRecursiveVisitor.h`**

Add after `VisitRuntimeValue` (around line 389):

```cpp
inline void VisitRuntimeBlockValue(RuntimeBlockValue* value) {
    static_cast<Derived*>(this)->VisitCommonValue((Value*) value);
}
```

Add switch-case in `VisitValueNoNullCheck` (around line 855):

```cpp
case ValueKind::RuntimeBlockValue:
    static_cast<Derived*>(this)->VisitRuntimeBlockValue((RuntimeBlockValue*) value);
    return;
```

Add the by-pointer overload in the `VisitByPtrTypeNoNullCheck` section (around line 1228):

```cpp
static_cast<Derived*>(this)->VisitRuntimeBlockValue(value);
```

---

## Step 5 — RecursiveVisitor: Add `VisitRuntimeBlockValue`

**File: `preprocess/visitors/RecursiveVisitor.h`**

Add after `VisitRuntimeValue` (around line 568):

```cpp
void VisitRuntimeValue(RuntimeValue* value) {
    visit_it(value->underlying);
}

// NEW:
void VisitRuntimeBlockValue(RuntimeBlockValue* value) {
    visit_it(value->scope);
    if (value->getType()) {
        visit_it(value->getType());
    }
}
```

This ensures the recursive visitor traverses into the block scope and the optional type.

---

## Step 6 — SymRes: `SymResLinkBody::VisitRuntimeBlockValue`

**File: `compiler/symres/SymResLinkBody.cpp`**

SymResLinkBody does **not** use `RecursiveVisitor`. It uses `NonRecursiveVisitor` directly and provides explicit `Visit*` methods.

### 6.1 Visit the scope

```cpp
void SymResLinkBody::VisitRuntimeBlockValue(RuntimeBlockValue* value) {
    // Visit the explicit type if given
    if (value->getType()) {
        visit(value->getType());
    }

    // Link the block body (each statement)
    table.scope_start();
    for (auto node : value->scope.nodes) {
        visit(node);
    }

    // Determine the block's result type from the last value
    auto lastVal = value->get_last_value();
    if (lastVal) {
        // If explicit type was given, verify compatibility
        if (value->has_explicit_type) {
            // TODO: check that lastVal's resolved type satisfies value->getType()
            // If not, emit a diagnostic error
        } else {
            // Infer type from last value
            value->setType(lastVal->getType());
        }
    } else if (value->has_explicit_type) {
        // Explicit type given but no last value — that's okay,
        // the block might be used for side effects only (void)
    } else {
        diagnoser.error("could not determine type of %runtime_block_value, "
                        "provide an explicit type or ensure the last expression has a type",
                        value);
    }

    table.scope_end();
}
```

### 6.2 Add declaration in header

**File: `compiler/symres/SymResLinkBody.h`** — line ~388:

```cpp
void VisitRuntimeValue(RuntimeValue* value);
void VisitRuntimeBlockValue(RuntimeBlockValue* value);  // NEW
```

---

## Step 7 — TypeVerifier: `TypeVerifier::VisitRuntimeBlockValue`

**File: `compiler/typeverify/TypeVerify.cpp`** — after `VisitRuntimeValue` (line 349):

```cpp
void TypeVerifier::VisitRuntimeBlockValue(RuntimeBlockValue* value) {
    // Visit all nodes in the block
    for (auto node : value->scope.nodes) {
        visit(node);
    }

    // Ensure the last value's type matches the block's declared type
    auto lastVal = value->get_last_value();
    if (lastVal && value->getType()) {
        auto lastType = lastVal->getType();
        if (lastType && !lastType->satisfies(value->getType())) {
            diagnoser.error(
                "type mismatch in %runtime_block_value: "
                "last expression has type '{}' but block expects '{}'",
                lastType, value->getType()
            )->highlight(lastVal->encoded_location());
        }
    }

    // Visit the type itself
    if (value->getType()) {
        visit(value->getType());
    }
}
```

**Header: `compiler/typeverify/TypeVerify.h`** — add declaration (after line 134):

```cpp
void VisitRuntimeValue(RuntimeValue* value);
void VisitRuntimeBlockValue(RuntimeBlockValue* value);  // NEW
```

---

## Step 8 — GenericInstantiator: `VisitRuntimeBlockValue`

**File: `compiler/generics/GenericInstantiator.cpp`** — after `VisitRuntimeValue` (line 559):

```cpp
void GenericInstantiator::VisitRuntimeBlockValue(RuntimeBlockValue* value) {
    RecursiveVisitor<GenericInstantiator>::VisitRuntimeBlockValue(value);
}
```

**Header: `compiler/generics/GenericInstantiator.h`** — add declaration (after line 275):

```cpp
void VisitRuntimeValue(RuntimeValue* value);
void VisitRuntimeBlockValue(RuntimeBlockValue* value);  // NEW
```

---

## Step 9 — TopLevelLinkSignature: `VisitRuntimeBlockValue`

**File: `compiler/symres/LinkSignature.cpp`** — after `VisitBlockValue` (line 459):

```cpp
void TopLevelLinkSignature::VisitRuntimeBlockValue(RuntimeBlockValue* value) {
    RecursiveVisitor<TopLevelLinkSignature>::VisitRuntimeBlockValue(value);
    if (!comptime_context) {
        diagnoser.error(RUNTIME_EVAL_ERR, value);
    }
}
```

**Header: `compiler/symres/LinkSignature.h`** — add declaration (after line 195):

```cpp
void VisitBlockValue(BlockValue* value);
void VisitRuntimeBlockValue(RuntimeBlockValue* value);  // NEW
```

---

## Step 10 — Globals/ComptimePhase: TopLevelDeclSymDeclare handling

**File: `compiler/symres/DeclareTopLevel.cpp/.h`**

Check if `DeclareTopLevel` has any `VisitRuntimeValue` or `VisitBlockValue`. If it only visits statements (not values), then `RuntimeBlockValue` inside a comptime function return statement will be handled by the general statement visitor. If needed, add:

```cpp
void DeclareTopLevel::VisitRuntimeBlockValue(RuntimeBlockValue* value) {
    for (auto node : value->scope.nodes) {
        visit(node);
    }
}
```

---

## Step 11 — LLVM Codegen: `RuntimeBlockValue` delegates

**File: `compiler/backend/LLVM.cpp`**

Add codegen methods following the `BlockValue` pattern:

```cpp
void gen_RuntimeBlockValue_scope(Codegen& gen, RuntimeBlockValue* blockVal) {
    // Track to emit only once
    blockVal->scope.code_gen(gen);
}

llvm::AllocaInst* RuntimeBlockValue::llvm_allocate(...) {
    gen_RuntimeBlockValue_scope(gen, this);
    auto lastVal = get_last_value();
    return lastVal ? lastVal->llvm_allocate(gen, identifier, expected_type) : nullptr;
}

// Repeat for: store_in_struct, store_in_array, llvm_pointer, llvm_value, llvm_conditional_branch
// All follow the same pattern: gen scope first, then delegate to last value
```

**Important:** The last value's type must match the `RuntimeBlockValue`'s declared/inferred type.

---

## Step 12 — C Codegen (2c): `VisitRuntimeBlockValue`

**File: `preprocess/2c/2cASTVisitor.cpp`** —  after `VisitBlockValue` (line 7073):

```cpp
void ToCAstVisitor::VisitRuntimeBlockValue(RuntimeBlockValue* blockVal) {
    write("({");  // GNU C compound expression
    new_line_and_indent();
    scope_no_parens(*this, blockVal->scope);
    new_line_and_indent();
    auto lastVal = blockVal->get_last_value();
    if (lastVal) {
        visit(lastVal);
        write("; ");
    }
    write("})");
}
```

**Header: `preprocess/2c/2cASTVisitor.h`** — add declaration (around line 795):

```cpp
void VisitBlockValue(BlockValue* value);
void VisitRuntimeBlockValue(RuntimeBlockValue* value);  // NEW
```

---

## Step 13 — `evaluated_comptime` Rewrite (Use Recursive Visitor)

**File: `ast/utils/GlobalFunctions.cpp`** — current `evaluated_comptime()` (line 526)

### Problem

Current implementation hand-rolls a switch on `ValueKind` and only handles 6 kinds. It misses many nested value types (`AccessChain` with function calls, `Expression`, `CastedValue`, etc.). It also uses manual recursion instead of the established visitor pattern.

### Solution

Replace `evaluated_comptime` with a proper `RecursiveVisitor` subclass that:

```cpp
class ComptimeEvaluator : public RecursiveVisitor<ComptimeEvaluator> {
    InterpretScope& scope;
    ASTAllocator& allocator;
public:
    // Override for values that need comptime evaluation:
    void VisitFunctionCall(FunctionCall* call) {
        auto decl = call->safe_linked_func();
        if (decl && decl->is_comptime()) {
            // Fully evaluate at comptime
            auto result = call->evaluated_value(scope);
            set_result(result);  // store result for parent
        } else {
            // Copy and recurse into parent_val + args
            auto copied = call->copy(allocator);
            copied->parent_val = evaluate(call->parent_val);
            for (auto& arg : copied->values) {
                arg = evaluate(arg);
            }
            set_result(copied);
        }
    }

    void VisitVariableIdentifier(VariableIdentifier* id) {
        // If this identifier is linked to a comptime-available value,
        // replace it with the evaluated value
        auto eval = id->evaluated_value(scope);
        if (eval && eval != id) {
            set_result(eval);
        } else {
            set_result(id);  // keep as-is
        }
    }

    // Other overrides follow the same pattern as the current switch
    // but now the visitor handles recursion automatically via RecursiveVisitor

    Value* evaluate(Value* val) {
        visit(val);
        return get_result();
    }
};
```

**Key benefit:** `RecursiveVisitor` already knows how to visit all node types recursively. Override only the ones that need comptime-specific treatment (function calls, identifiers, etc.). Everything else (struct values, array values, etc.) gets visited correctly by the base class.

### Integration

Replace the old `evaluated_comptime` calls. The function `runtime_value_of()` (line 590) should use the new evaluator:

```cpp
Value* runtime_value_of(InterpretScope& scope, Value* underlying) {
    if (is_interpretation_mode(&scope)) {
        return underlying->evaluated_value(scope);
    }
    ComptimeEvaluator evaluator(scope);
    return evaluator.evaluate(underlying);
}
```

### Handling `RuntimeBlockValue` in `evaluated_comptime`

The new `evaluated_comptime` must handle `RuntimeBlockValue` specially:

```cpp
void VisitRuntimeBlockValue(RuntimeBlockValue* value) {
    // Visit the block scope (recursively visit all statements)
    RecursiveVisitor<ComptimeEvaluator>::VisitRuntimeBlockValue(value);
    // Replace comptime sub-expressions in the scope
    // The scope's statements have been visited recursively
    set_result(value);  // keep the RuntimeBlockValue wrapper
}
```

The block body inside `RuntimeBlockValue` is already being visited by the recursive visitor. Statements that contain comptime function calls will be resolved. Non-comptime calls remain as-is.

### Replace string::append_expr's current approach

Currently `append_expr` uses `%runtime_value(intrinsics::expr_str_block_value(...))`. With the new `%runtime_block_value`, the `string::make` constructor can generate its own block instead.

---

## Step 14 — `std::string::make(expr : %expressive_string)`

**File: `lang/libs/std/src/string.ch`**

Add a new comptime constructor:

```chemical
comptime func make(expr : %expressive_string) : string {
    return %runtime_block_value<string> {
        var str = std::string()
        str.append_expr(expr)
        str;
    }
}
```

**How this works at comptime:**

1. `make` is a `comptime` function — it runs during compilation.
2. `%runtime_block_value<string> { ... }` creates a `RuntimeBlockValue` node with type `string`.
3. Inside the block:
   - `var str = std::string()` creates a local variable
   - `str.append_expr(expr)` uses the existing `append_expr` intrinsic to process the expressive string
   - `str;` is the last expression — it produces the string value
4. The compiler's existing codegen emits the block body as runtime code and uses the last expression's value as the block's result.

**Also add a convenience:**

```chemical
@constructor
comptime func constructor(expr : %expressive_string) {
    return make(expr)
}
```

This enables `std::string(\`hello ${name}\`)` syntax directly.

---

## Step 15 — Connect `evaluated_comptime` to `RuntimeBlockValue` in `runtime_value_of`

**File: `ast/utils/GlobalFunctions.cpp`**

When `runtime_value_of` encounters a `RuntimeBlockValue` (e.g., returned from `string::make`), it should:

1.  In **interpretation mode**: evaluate the block body directly (similar to how `BlockValue` is evaluated by the interpreter).
2.  In **compilation mode**: call `evaluated_comptime` (now the recursive visitor variant) on the scope's statements, replacing comptime identifiers with their evaluated values.

```cpp
Value* runtime_value_of(InterpretScope& scope, Value* underlying) {
    if (is_interpretation_mode(&scope)) {
        return underlying->evaluated_value(scope);
    }
    // For RuntimeBlockValue, evaluate the scope's comptime sub-expressions
    if (underlying->kind() == ValueKind::RuntimeBlockValue) {
        auto rbv = underlying->as_runtime_block_value_unsafe();
        ComptimeEvaluator evaluator(scope);
        evaluator.evaluate_scope(rbv->scope);
        return rbv;
    }
    ComptimeEvaluator evaluator(scope);
    return evaluator.evaluate(underlying);
}
```

---

## Step 16 — RepresentationVisitor: Display `%runtime_block_value`

**File: `preprocess/RepresentationVisitor.cpp`**

Add support for printing the magic syntax back (for diagnostics and LSP):

After `VisitRuntimeValue` handler (line 826):

```cpp
void RepresentationVisitor::VisitRuntimeBlockValue(RuntimeBlockValue* value) {
    write("%runtime_block_value");
    if (value->getType()) {
        write("<");
        visit(value->getType());
        write(">");
    }
    write(" {");
    indent_needed();
    // Visit scope nodes
    for (auto node : value->scope.nodes) {
        new_line_and_indent();
        visit(node);
        write(";");
    }
    dedent_needed();
    new_line_and_indent();
    write("}");
}
```

---

## Step 17 — CMakeLists: register new source files

**File: `ast/CMakeLists.txt`** (or the relevant CMake file that lists `.cpp` source files)

Add `values/RuntimeBlockValue.cpp` to the source list.

---

## Summary of Files to Create/Modify

| # | Action | File | Description |
|---|--------|------|-------------|
| 1 | Create | `ast/values/RuntimeBlockValue.h` | Value class header |
| 2 | Create | `ast/values/RuntimeBlockValue.cpp` | Value class impl |
| 3 | Modify | `ast/base/ValueKind.h` | Add `RuntimeBlockValue` enum |
| 4 | Modify | `ast/base/Value.h` | Add helper methods + fwd decl |
| 5 | Modify | `parser/utils/LexValue.cpp` | Parse `%runtime_block_value<...> { ... }` |
| 6 | Modify | `preprocess/visitors/NonRecursiveVisitor.h` | Add `VisitRuntimeBlockValue` |
| 7 | Modify | `preprocess/visitors/RecursiveVisitor.h` | Add `VisitRuntimeBlockValue` |
| 8 | Modify | `compiler/symres/SymResLinkBody.cpp` | `VisitRuntimeBlockValue` — link body |
| 9 | Modify | `compiler/symres/SymResLinkBody.h` | Declare it |
| 10 | Modify | `compiler/symres/LinkSignature.cpp` | `VisitRuntimeBlockValue` |
| 11 | Modify | `compiler/symres/LinkSignature.h` | Declare it |
| 12 | Modify | `compiler/typeverify/TypeVerify.cpp` | `VisitRuntimeBlockValue` — type check |
| 13 | Modify | `compiler/typeverify/TypeVerify.h` | Declare it |
| 14 | Modify | `compiler/generics/GenericInstantiator.cpp` | `VisitRuntimeBlockValue` |
| 15 | Modify | `compiler/generics/GenericInstantiator.h` | Declare it |
| 16 | Modify | `compiler/backend/LLVM.cpp` | LLVM codegen for RuntimeBlockValue |
| 17 | Modify | `preprocess/2c/2cASTVisitor.cpp` | C codegen for RuntimeBlockValue |
| 18 | Modify | `preprocess/2c/2cASTVisitor.h` | Declare |
| 19 | Modify | `preprocess/RepresentationVisitor.cpp` | Print `%runtime_block_value` |
| 20 | Modify | `ast/utils/GlobalFunctions.cpp` | Rewrite `evaluated_comptime` as RecursiveVisitor; integrate RuntimeBlockValue |
| 21 | Modify | `lang/libs/std/src/string.ch` | Add `make(expr: %expressive_string)` and `constructor(expr: %expressive_string)` |
| 22 | Modify | CMakeLists (ast) | Add `RuntimeBlockValue.cpp` |

## Test Plan

After implementation:

1. **Write a test file** `lang/tests/src/stdlib/mutex/string_expr.ch` (or similar) that tests:
   ```chemical
   var name = "World"
   var greeting = string(`Hello ${name}!`)
   test("string from expressive string", () => {
       return greeting == "Hello World!"
   })
   ```

2. **Edge cases:**
   - Empty expressive string: `` string(``) ``
   - Multiple expressions: `` string(`${a}, ${b}, ${c}`) ``
   - Nested comptime expressions: `` string(`${intrinsics::size(some_var)}`) ``
   - With explicit numeric conversions: `` string(`${i + 1}`) ``

3. **Error cases:**
   - Using `%runtime_block_value` without a type and without an inferable last expression
   - Type mismatch between last expression and explicit `<T>`

4. **Run tests:**
   ```bash
   ./scripts/test.sh --tcc
   ```
   Verify only the 6 pre-existing failures remain.
