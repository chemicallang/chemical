---
name: Interpreter Internals
description: Comprehensive guide to the Chemical AST interpreter — how it works, move semantics, temp destruction, debugging, and extending interpretation test coverage.
---

# Interpreter Internals

The AST interpreter (`compiler/Interpreter/`) evaluates Chemical code directly without generating C or LLVM IR. It's used for:
- **Comptime evaluation**: `comptime` functions and `comptime { }` blocks run in the interpreter
- **Interpretation tests**: `--arg-interpret` runs the test suite through the interpreter

## Architecture Overview

### Core Classes

| Class | File | Purpose |
|-------|------|---------|
| `InterpretScope` | `ast/base/InterpretScope.h` | Per-function/per-block scope with value map, parent chain, global pointer |
| `GlobalInterpretScope` | `ast/base/GlobalInterpretScope.h` | Global state — allocator, type builder, call stack, backend context |
| `Value` | `ast/base/Value.h` | Base class for all interpreted values (IntN, Bool, Float, String, StructValue, PointerValue, etc.) |
| `PointerValue` | `ast/values/PointerValue.h` | Simulates C pointers with `data`, `behind`, `ahead` for bounds checking |

### Entry Point

The interpreter is invoked from `do_interpretation_job()` in `compiler/lab/LabBuildCompiler.cpp`:
1. Parses, symres, and typechecks `interpret/` + `common/` modules
2. Initializes module-level `VarInitStmt` variables on the global scope
3. Finds the `main()` function and calls it directly via the AST interpreter
4. Never generates object code or links an executable

### Scope Hierarchy

```
GlobalInterpretScope (global)
  └─ InterpretScope (fn_scope, parent=global) — created by FunctionDeclaration::call()
       └─ InterpretScope (body_scope, parent=fn_scope) — for function body
            └─ InterpretScope (child, parent=body_scope) — for if/while/for blocks
```

- `parent == global` means the scope is a function-level scope (return values are stored here)
- `should_destruct_values` is `true` by default, `false` for the global scope

## Move Semantics

### The Problem

When a destructible struct variable is used as an initializer or argument:
```chemical
var x = MyStruct()
var y = x  // x is moved, should NOT be destructed when its scope ends
```

Without move semantics, `x` would be destructed **twice** — once when `y` is created (via `scope_value()` which copies), and again when `x`'s scope is destroyed. This causes double-free crashes.

### The Solution: `move_clear_source()`

Defined in `ast/base/InterpretScope.cpp`. Uses **pointer-matching** instead of AST node type checks:

```cpp
void InterpretScope::move_clear_source(Value* initializer, const chem::string_view& new_name) {
    if(!initializer || initializer->val_kind() != ValueKind::StructValue) return;
    auto structVal = initializer->as_struct_value_unsafe();
    auto ext = structVal->linked_extendable();
    if(!ext || ext->kind() != ASTNodeKind::StructDecl) return;
    auto sd = (StructDefinition*)ext;
    if(!sd->has_destructor()) return;  // Only destructible types need clearing
    InterpretScope* scanScope = this;
    while(scanScope) {
        for(auto& [name, val] : scanScope->values) {
            if(name != new_name && val == initializer) {
                val = nullptr;  // Clear the source — it's been moved
                return;
            }
        }
        scanScope = scanScope->parent;
    }
}
```

#### Why pointer-matching?

The compiler may replace `VariableIdentifier` AST nodes with the resolved `StructValue` during resolution. Old code that checked `val_kind() == ValueKind::Identifier` would miss these cases. Pointer-matching (`val == valuePtr`) works regardless of AST node types.

#### Usage Locations

| Location | File | New Name Arg | What It Does |
|----------|------|-------------|--------------|
| `VarInitStatement::interpret()` | `compiler/Interpreter/Core.cpp` | Variable name | `var d = s` → clears `s` (move into `d`) |
| `AssignStatement::interpret()` | `compiler/Interpreter/Core.cpp` | LHS name | `a = d` → clears `d` (move into `a`) |
| `StructValue::initialized_value()` | `ast/values/StructValue.cpp` | empty string | Member init: `{ field: src }` → clears `src` |
| `FunctionCall::interpret_value()` | `ast/values/FunctionCall.cpp` | empty string | Variant ctor: `Ctor(src)` → clears `src` |
| `FunctionDeclaration::call()` | `ast/structures/FunctionDecl.cpp` | empty string | Function arg: `func(src)` → clears `src` |

When `new_name` is **empty**, the function will clear **any** matching variable in the scope chain. This is used for function arguments, variant constructor args, and struct member initialization, where there's no named variable being declared.

When `new_name` is **non-empty** (e.g., the variable being declared), the function skips that name to avoid self-clearing (`var x = x` shouldn't nullify `x`).

## Temp Struct Destruction

### The Problem

Bare expressions that produce destructible structs need their temporaries cleaned up:
```chemical
create_destructible(...)           // Returns a destructible struct — must destruct it
d.copy().copy().copy()             // Method chain — ALL intermediate temps must be destructed
var data = create_destructible(...).data  // Field access — destruct the intermediate struct
```

### The Helper: `destruct_temp_struct()`

Defined in `compiler/Interpreter/Core.cpp`:

```cpp
static void destruct_temp_struct(InterpretScope& scope, Value* val) {
    if(!val || val->val_kind() != ValueKind::StructValue) return;
    auto structVal = val->as_struct_value_unsafe();
    auto ext = structVal->linked_extendable();
    if(!ext) return;
    auto container = static_cast<ExtendableMembersContainerNode*>(ext);
    if(!container->has_destructor()) return;     // Skip non-destructible types
    auto destructor_fn = container->destructor_func();
    if(!destructor_fn || !destructor_fn->body.has_value()) return;
    InterpretScope temp_scope(scope.global, scope.allocator, scope.global);
    temp_scope.declare("self", val);               // Pass as 'self'
    temp_scope.interpret(&destructor_fn->body.value());
    auto self_it = temp_scope.values.find("self");
    if(self_it != temp_scope.values.end()) {
        temp_scope.values.erase(self_it);          // Prevent double-destruction
    }
}
```

**Critical**: The destructor runs in a temp scope. After running the destructor body, `self` is removed from the temp scope so it's not destructed a second time when the temp scope is destroyed.

### Three Call Sites for Temp Destruction

#### 1. `ValueWrapperNode::interpret()` (bare expression statements)

Handles `create_destructible(...)` as a statement:
- If value is `FunctionCall` → evaluate, destruct result
- If value is `AccessChain` → evaluate each step, collect ALL FunctionCall results, destruct all

#### 2. `AccessChainNode::interpret()` (bare expression chains)

Handles `d.copy().copy().copy()` as a statement:
- Evaluates each value in the chain sequentially
- Collects FunctionCall results in a `std::vector<Value*> temps`
- After the loop, destructs ALL collected temps

#### 3. `AccessChain::evaluated_value()` (chain in expression context)

Handles `create_destructible(...).data` inside expressions:
- After `evaluate_from()` gives the field value, destructs `values[0]` if it was a FunctionCall returning a destructible struct
- The result (field value) is a separate value from the temp struct, so destruction is safe

### Interaction with `Scope::destroy_values()`

The normal scope destructor (`InterpretScope::~InterpretScope()`) calls `destroy_values()`, which iterates all values in the scope and calls destructors for structs/variants that have them. However, this only works for **named variables** in the scope. Bare expression temps aren't named variables, so they must be handled explicitly via `destruct_temp_struct()`.

## AssignStatement: Old-Value Destruction

Assignment uses a precise 3-step approach to handle self-referencing assignments:

```cpp
void interpret(InterpretScope& scope, AssignStatement* assign) {
    // Step 1: Save the old LHS value pointer
    Value* oldLhsVal = nullptr;
    if(assign->assOp == Operation::Assignment) {
        auto lhsId = assign->lhs->as_identifier_unsafe();
        auto lhsIt = scope.find_value_iterator(lhsId->value);
        if(lhsIt.first != lhsIt.second.values.end()) {
            oldLhsVal = lhsIt.first->second;
        }
    }
    // Step 2: Perform the assignment (evaluates RHS, may take pointer to LHS)
    assign->lhs->set_value(scope, assign->value, assign->assOp, assign->encoded_location());
    // Step 3: Destruct the old LHS value (safe — RHS already resolved)
    if(oldLhsVal) {
        destruct_temp_struct(scope, oldLhsVal);
    }
    // Step 4: Clear the RHS source (move semantics)
    scope.move_clear_source(rhsVal, lhsName);
}
```

**Why destruct AFTER set_value?** For `x = f(x.get_ptr(), N)`, the RHS function `f` takes a pointer to `x`'s data. If we destruct `x` before evaluating `f`, the pointer becomes dangling. By destructing after `set_value()`, the RHS evaluation sees valid data.

**Why save old value BEFORE set_value?** `set_value()` overwrites the scope entry, losing the old pointer. Without saving, we'd leak the old struct.

## Pointer Model

`PointerValue` tracks bounds for safe interpretation:

```cpp
class PointerValue {
    void* data;     // Raw pointer to data
    size_t behind;  // Bytes available BEFORE data
    size_t ahead;   // Bytes available AFTER data
};
```

- **Increment**: `data += sizeof(T)`, `behind += sizeof(T)`, `ahead -= sizeof(T)` — fails if `ahead < amount`
- **Decrement**: `data -= sizeof(T)`, `behind -= sizeof(T)`, `ahead += sizeof(T)` — fails if `behind < amount`
- **Dereference**: checks `typeSize <= ahead` — returns null on failure (non-fatal)

**Critical**: Pointer bounds are enforced in interpretation mode but NOT in compiled mode (native C doesn't track bounds). Tests that pass pointers past element boundaries fail in interpretation mode but work when compiled.

## Function Call Flow

### `FunctionDeclaration::call()`

1. Saves and replaces `global->current_func_type`
2. Pushes call onto `global->call_stack`
3. Creates `InterpretScope fn_scope(global, func_allocator, global)` (parent=global)
4. Propagates implicit args from lexical scope chain into `fn_scope`
5. Declares `self`, arguments, and default-value parameters
6. Calls the internal `call()` method which interprets the body
7. Reads `fn_scope->returnValue` and restores `current_func_type`/call_stack
8. Returns the return value

### `set_return()` — Used by `ReturnStatement`

Sets `fn_scope->returnValue` and propagates `stopInterpretation` flags up the scope chain to prevent sibling node execution after return.

## Value Destruction

### `InterpretScope::destroy_values()`

Called by the destructor (`~InterpretScope()`) when `should_destruct_values` is true:

1. Iterates all values in the scope
2. For each `StructValue`:
   - Calls destructor body in a temporary scope
   - Recursively destructs all member values
3. For each `ArrayValue`:
   - Destructs each element that is a struct with a destructor
4. Skips `returnValue` (it's been moved to the caller)
5. Skips `nullptr` entries (cleared by move semantics)

### Interaction with Move Semantics

When a struct is moved (`var y = x`), `move_clear_source()` sets `x`'s entry to `nullptr`. When the scope destructor runs, it **skips** `nullptr` entries, so the moved-from variable is not destructed.

### Interaction with Temp Destruction

Temp struct destruction (`destruct_temp_struct()`) runs outside of `destroy_values()` — it creates its own temp scope, runs the destructor, and the temp scope's destructor cleans up. The key is removing `self` from the temp scope to prevent double-destruction.

## Variant Handling in the Interpreter

### Variant Construction

When a variant member constructor is called (e.g., `Option.Some(value)`), `FunctionCall::interpret_value()`:
1. Creates a `StructValue` for the variant
2. Stores the member index in `scope.global->variant_member_index_map` (maps `StructValue* → member_index`)
3. Evaluates each parameter and stores it in the struct's `values` map
4. Applies move semantics to each argument via `move_clear_source()`

### Variant Pattern Matching in Switch

`eval_switch_stmt_block()` checks if the switch expression is a variant:
1. If the condition is a `StructValue`, looks up its member index in `variant_member_index_map`
2. Finds the matching case by comparing member indices
3. Returns the matching scope body
4. `declare_variant_case_vars()` declares the case variables from the matched variant's values

### What's NOT Supported for Variants

- Full variant type matching at comptime (the else-expression path for break/continue/return/defValue IS supported)
- `@test` annotation dispatch

## Testing Infrastructure

### Running Interpretation Tests

```bash
./scripts/test.sh --tcc --interpret              # Build + run
./scripts/test.sh --tcc --interpret --no-build   # Skip rebuild

# Manual:
./chemical lang/tests/build.lab --arg-interpret --mode debug_complete --no-cache
```

> ⚠️ **`--no-build` warning**: This flag **skips rebuilding the C++ compiler binary**.
> Changes to `.cpp`/`.h` files in the compiler source **will NOT be picked up** —
> the previously built binary is used. Only use `--no-build` when iterating on `.ch`
> test files **without any compiler C++ changes**. To pick up C++ changes, omit
> `--no-build` (run once without it first).

### Test Organization

| Module | Location | What It Tests |
|--------|----------|---------------|
| `common_tests` | `lang/tests/common/` | Core features (arithmetic, loops, structs, variants, inc/dec, destructors) |
| `native_common_tests` | `lang/tests/native_common/` | Pointer operations, casts, comptime pointer arithmetic |
| `interpret_tests` | `lang/tests/interpret/` | Wrapper that imports both and runs `main()` |

### Test Structure

```chemical
func test_my_feature() {
    test("my feature works", () => {
        return my_new_feature(3, 4) == 7;
    });
}
```

Tests use `comptime if(intrinsics::is_interpretation())` to branch between `expr_println` (interpretation) and `printf` (compiled mode).

### Adding a New Test

1. Write test function in `lang/tests/common/src/`, `lang/tests/src/comptime/`, or `lang/tests/native_common/src/`
2. Register in the appropriate runner function
3. Run `./scripts/test.sh --tcc --interpret --no-build`

## Debugging Guide

### Known Failure Categories (16 tests remain as of last fix session)

| Tests | Root Cause | Approach |
|-------|-----------|----------|
| 757-760 | Array elements set via pointer aren't tracked for destruction | Track pointer-modified array elements, or use a mark-and-sweep approach |
| 798-799, 822 | Method chains — `get_parent_from()` returns nullptr for self | Fix method dispatch through chained temporaries |
| 811-812 | Self-referencing assignment destructor corrupts new value | Delay old-value destruction or use copy-on-write semantics |
| 722-723, 800 | Generic monomorphization doesn't trigger move-semantic paths | Ensure generic function calls go through the same code paths |
| 629, 831, 838 | Misc edge cases | Individual investigation needed |
| 784 | Variant destruct path not fully covered | Ensure variant member destructors are called for all paths |

### Quick Debugging Tips

1. **Check test output** for `[InterpretError]` messages — these give exact failure locations
2. **Add debug output** to interpreter source:
   ```cpp
   std::cerr << "[DEBUG] scope values:" << std::endl;
   scope.print_values();
   ```
3. **Trace destruction**: Add `std::cerr << "[DESTROY] value " << val->representation() << " in scope" << std::endl;` in `destroy_values()`
4. **Trace move semantics**: Add `std::cerr << "[MOVE] clearing " << name << " = " << initializer->representation() << std::endl;` in `move_clear_source()`
5. **Check pointer bounds**: Look for `\"cannot dereference pointer while type size is larger than bytes available\"` — pointer went past allocated bounds
6. **Check function resolution**: Look for `\"calling a function that is not found or has no body\"` — a function reference couldn't be resolved
7. **Check type operations**: Look for `\"unknown operation between values\"` — incompatible types being operated on
8. **Worst-case failure**: `\"RUNTIME ERROR: invalid memory access\"` — the interpreter has been hardened to return `getNullValue()` instead of crashing on deref failures

### Common Pitfalls

- **Pointer arithmetic past end of array/string**: Works in compiled mode but fails in interpreter. Check `ahead` values.
- **`&raw struct_val`**: Not supported in interpreter. Use `&mut struct_val` instead.
- **Function pointers as parameters**: If a function reference can't be linked, you get a non-fatal `\"function not found\"` error.
- **Destructor bodies**: Must be valid Chemical code the interpreter can evaluate. Empty destructors `{}` work fine.
- **Vector `.get(i)` vs `.get_ptr(i)`**: Never use `.get(i)` for types with destructors — it bitwise-copies, causing double-free. Always use `.get_ptr(i)`.

## Interpreter-Friendly Standard Library Data Structures

The interpreter provides its own implementations of certain standard library types so they can be used during comptime/interpretation mode. These are defined as intrinsics in `ast/utils/GlobalFunctions.cpp`.

### InterpretVector (Interpreter-Friendly `std::vector<T>`)

Located in the `InterpretVector` namespace in `GlobalFunctions.cpp`. This provides a full interpreter-friendly `std::vector<T>` replacement.

#### How it works

```cpp
namespace InterpretVector {
    // The "type" — a StructDefinition that looks like std::vector<T>
    class InterpretVectorNode : public StructDefinition {
        GenericTypeParameter typeParam;       // The T in vector<T>
        InterpretVectorConstructor constructorFn;
        InterpretVectorSize sizeFn;
        InterpretVectorGet getFn;
        InterpretVectorPush pushFn;
        InterpretVectorRemove removeFn;
    };

    // The "value" — holds actual data in a C++ vector<Value*>
    class InterpretVectorVal : public StructValue {
        std::vector<Value*> values;  // The actual elements
    };
}
```

**Construction flow:**
1. User writes `var v = std::vector<int>()`
2. Parser resolves to a `FunctionCall` with generic arg `int`
3. During symres, the call is linked to `InterpretVectorConstructor::call()`
4. A new `InterpretVectorVal` is created with an empty `values` vector
5. The type parameter `T` is stored in `InterpretVectorNode::typeParam`

**Method dispatch flow:**
1. `v.push(42)` → resolved as a method call on `InterpretVectorVal`
2. The method is found in `InterpretVectorNode`'s child functions
3. `InterpretVectorPush::call()` is invoked with `parent_val = the InterpretVectorVal instance`
4. The argument value is copied (`scope_value`) and appended to the internal `values` vector

**Supported operations:**
| Method | Implementation |
|--------|----------------|
| `vector<T>()` | Creates empty InterpretVectorVal |
| `.size()` | Returns `values.size()` as IntNumValue |
| `.get(index)` | Returns `values[index]->scope_value(call_scope)` (a COPY, not a reference) |
| `.push(value)` | `values.push_back(value->scope_value(call_scope))` |
| `.remove(index)` | `values.erase(values.begin() + index)` |

**Current limitations (future work needed):**
1. **`.get()` returns a copy, not a reference** — marked with `TODO interpret vector get should return a reference to T`
   - This means `v.get(0) = 42` won't modify the vector
   - A proper implementation would return a `PointerValue` into the vector's storage
2. **No `operator[]` support** — `v[0]` doesn't work in interpreter mode
3. **No iteration support** — `for(x in v)` doesn't work in interpreter mode without an `Iterable` impl
4. **No capacity management** — `reserve()`, `capacity()` not implemented
5. **No `get_ptr()` support** — returns a pointer to element (needed for modification)

### Interpreter-Friendly `std::string` (NOT yet implemented)

The runtime `std::string` (defined in `std/chem_string.h`) is NOT directly available in the interpreter. The interpreter uses `StringValue` for string values, which stores a `chem::string_view`.

**Current string operations in interpreter:**
- String literals (`"hello"`) → `StringValue`
- String concatenation via expressive strings (backtick templates): `` `hello ${name}` ``
- `intrinsics::size(str)` → returns string length

**Missing (future work needed):**
- `string.append(c)` — append character
- `string.append_view(v)` — append string_view
- `string.append_string(other)` — append another string
- `string.copy()` — explicit copy
- `string.to_view()` — convert to string_view
- `string.get(i)` — get character at index

To support these, create intrinsic function declarations similar to `InterpretVectorPush` that manipulate the `StringValue`'s internal `chem::string_view` data.

### Interpreter-Friendly `std::unordered_map<K, V>` (NOT yet implemented)

This would follow the same pattern as `InterpretVector`:

```cpp
class InterpretUnorderedMapNode : public StructDefinition {
    GenericTypeParameter keyType;   // K
    GenericTypeParameter valueType; // V
    // Constructor, insert, get, contains, erase, size
};

class InterpretUnorderedMapVal : public StructValue {
    std::unordered_map<Value*, Value*> data;  // Map storage
};
```

**Required methods:**
- `unordered_map<K, V>()` — constructor with two generic type params
- `.insert(key, value)` — insert key-value pair
- `.get_ptr(key)` → return pointer to value or null
- `.contains(key)` → bool
- `.erase(key)` → remove entry
- `.size()` → number of entries
- `.clear()` → remove all entries

### Interpreter-Friendly `std::string_view` (NOT yet implemented)

A lightweight view intrinsic that wraps a `chem::string_view`:
- `.size()` → length
- `.get(i)` → character at index
- Implicit construction from `*char` and `string`

### Interpreter-Friendly `std::span<T>` (NOT yet implemented)

A bounded view intrinsic:
- Constructor from array/pointer + length
- `.size()` → length
- `.get(i)` → element at index
- `.data()` → pointer to start

### Adding Interpreter-Friendly Support for a New Type

1. **Create the struct definition class** (e.g., `InterpretUnorderedMapNode` extending `StructDefinition`)
   - Add generic type parameters for the type's template args
   - Add child function declarations for each method

2. **Create the value class** (e.g., `InterpretUnorderedMapVal` extending `StructValue`)
   - Add C++ data member for the actual storage (e.g., `std::unordered_map<...>`)
   - Override any needed methods

3. **Create method intrinsic classes** (extending `FunctionDeclaration` for each method)
   - Add `self` parameter if it's a method
   - Implement `call()` to manipulate the value's data

4. **Register in the type system** — add to the type builder or global functions so the compiler recognizes the type name

5. **Add tests** — both interpretation (`--arg-interpret`) and compiled tests

## Future Goals for Interpreter

### 1. Full Standard Library Support
Make the following `std` types work in interpreter mode:
- `std::string` — append, append_view, get, size, copy
- `std::string_view` — lightweight view operations
- `std::unordered_map<K, V>` — insert, get_ptr, contains, erase
- `std::span<T>` — bounded view
- `std::vector<T>` — complete with references from get(), operator[], iteration

### 2. Compiler Reflection Enhancements
- Field enumeration at compile time
- Automatic JSON serializer/deserializer generation
- `intrinsics::get_members<T>()` — returns list of struct members
- `intrinsics::get_interfaces<T>()` — returns list of implemented interfaces

### 3. Automatic Generic Impl Instantiation
- When user calls `myType.method()`, if `method` comes from a generic impl declaration, automatically instantiate it
- Deprecate nested `impl` blocks

### 4. Remaining Interpretation Test Fixes
The 16 remaining test failures (as of last fix session):
- Tests 757-760: Array element destruction
- Tests 798-799, 822: Method chain dispatch through temporaries
- Tests 811-812: Self-referencing assignment destructor
- Tests 722-723, 800: Generic dispatch move semantics
- Tests 629, 831, 838: Misc edge cases
- Test 784: Variant destruct

## Code Map

### Key Files for Interpreter Development

| File | Purpose |
|------|---------|
| `compiler/Interpreter/Core.cpp` | Statement interpretation (assign, var init, loops, if, switch, value wrapper, access chain) |
| `ast/base/InterpretScope.h` | Scope class, value management, `move_clear_source()` |
| `ast/base/InterpretScope.cpp` | Value operations, destroy_values, move_clear_source implementation |
| `ast/base/GlobalInterpretScope.h` | Global interpreter state |
| `ast/utils/GlobalFunctions.cpp` | All intrinsics AND interpreter-friendly std types (InterpretVector, etc.) |
| `ast/values/StructValue.cpp` | Struct initialization, `initialized_value()`, move semantics |
| `ast/values/FunctionCall.cpp` | Function call evaluation, variant constructor, move semantics |
| `ast/structures/FunctionDecl.cpp` | Function call dispatch, argument handling, return values |
| `ast/values/AccessChain.cpp` | Chain evaluation, temp destruction |
| `ast/values/PointerValue.h` | Pointer bounds tracking |
| `compiler/lab/LabBuildCompiler.cpp` | `do_interpretation_job()` — top-level entry |

### Key Function Signatures

```cpp
// In InterpretScope.h
void move_clear_source(Value* initializer, const chem::string_view& new_name);

// In Core.cpp
static void destruct_temp_struct(InterpretScope& scope, Value* val);

// In FunctionDecl.cpp
Value* call(InterpretScope* call_scope, std::vector<Value*>& call_args,
            Value* parent, InterpretScope* fn_scope, bool evaluate_refs,
            Value* debug_value);

// In FunctionCall.cpp
Value* interpret_value(FunctionCall* call, InterpretScope& scope, Value* parent);

// In AccessChain.cpp
Value* evaluate_from(std::vector<Value*>& values, InterpretScope& scope,
                     Value* evaluated, unsigned i);

// In StructValue.cpp
StructValue* initialized_value(InterpretScope& scope);
```

## Testing Flow for New Features

1. **Add the Chemical-level test** in the appropriate test directory
2. **Run interpretation tests**: `./scripts/test.sh --tcc --interpret`
3. **Fix failures**: Add debug output, trace the issue, implement fix in C++
4. **Verify both modes**: `./scripts/test.sh --tcc --interpret --no-build && ./scripts/test.sh --tcc --no-build`
5. **Run full test suite** to check for regressions: `./scripts/test.sh --tcc`

## Related Skills

- **Intrinsics & Compiler Reflection** (`.agents/skills/intrinsics_compiler_reflection/SKILL.md`) — All intrinsic functions, reflection APIs, how to add new intrinsics
- **Build System** (`.agents/skills/build_system/SKILL.md`) — How interpretation jobs work in the build pipeline
