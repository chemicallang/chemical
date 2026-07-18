---
name: Compiler Intrinsics and Reflection
description: Comprehensive guide to Chemical's compiler intrinsic functions and reflection APIs — how GlobalFunctions.cpp provides interpreter-friendly implementations, compile-time reflection, and metadata access.
---

# Compiler Intrinsics and Reflection

Chemical provides a rich set of compiler intrinsic functions (in `ast/utils/GlobalFunctions.cpp`) that are implemented directly in C++ and accessible as regular Chemical functions. These intrinsics provide reflection, compile-time evaluation, interpreter support for standard library types, and lower-level compiler interaction.

## Architecture

The intrinsic functions are C++ classes that extend `FunctionDeclaration` and override `call()`. They are registered in the compiler's `GlobalInterpretScope` during initialization and become available as `intrinsics::function_name()`.

```cpp
// Pattern for creating an intrinsic:
class InterpretMyIntrinsic : public FunctionDeclaration {
public:
    explicit InterpretMyIntrinsic(TypeBuilder& cache, ASTNode* parent_node)
        : FunctionDeclaration(
            "my_intrinsic",                    // Name visible in Chemical code
            {cache.getVoidType(), ZERO_LOC},   // Return type
            false,                              // variadic?
            parent_node,                        // Parent node
            ZERO_LOC,                           // Source location
            AccessSpecifier::Public,            // Access specifier
            true                                // Is compiler declaration?
    ) {
        set_compiler_decl(true);  // Mark as compiler-provided
    }

    Value *call(InterpretScope *call_scope, ASTAllocator& allocator,
                FunctionCall *call, Value *parent_val,
                bool evaluate_refs) override {
        // Implementation here
        // Access arguments via call->values[i]->evaluated_value(*call_scope)
        // Use call_scope->error() for error reporting
        // Return values via allocator.allocate<T>()
    }
};
```

### Key File

| File | Lines | Purpose |
|------|-------|---------|
| `ast/utils/GlobalFunctions.cpp` | ~2000+ | All intrinsic function implementations |
| `ast/utils/GlobalFunctions.h` | (implied) | Declarations |

## Intrinsic Function Categories

### 1. Print/Debug Functions

| Intrinsic | Class | Signature | Purpose |
|-----------|-------|-----------|---------|
| `intrinsics::print` | `InterpretPrint` | `print(value : any...)` | Print values to stdout (space-separated) |
| `intrinsics::println` | `InterpretPrintLn` | `println(value : any...)` | Print + newline |
| `intrinsics::expr_print` | `InterpretExprPrint` | `expr_print(expr : %expressive_string)` | Print expressive string (backtick templates) |
| `intrinsics::expr_println` | `InterpretExprPrintLn` | `expr_println(expr : %expressive_string)` | Print expressive string + newline |

**Key implementation details:**
- `InterpretPrint` and `InterpretPrintLn` use `RepresentationVisitor` with `interpret_representation = true` for type-aware printing
- `InterpretExprPrint` and `InterpretExprPrintLn` handle expressive strings by iterating `ExpressiveString::values` — literals go to `std::cout` directly, `${}` expressions are evaluated and printed via `RepresentationVisitor`
- The old `print`/`println` are variadic (`any...`); the new `expr_print`/`expr_println` take a single `%expressive_string`

### 2. Compile-Time Mode Detection

| Intrinsic | Class | Purpose |
|-----------|-------|---------|
| `intrinsics::is_interpretation()` | `InterpretIsInterpretation` | Returns true if running in interpretation mode |
| `intrinsics::is_runtime()` | `InterpretIsRuntime` | Returns true if running in codegen (not comptime) |
| `intrinsics::is_comptime()` | `InterpretIsComptime` | Returns true if inside a comptime evaluation |
| `intrinsics::is_tcc_based()` | `InterpretIsTcc` | Returns true if compiled with TCC_BUILD |
| `intrinsics::is_clang()` | `InterpretIsClang` | Returns true if compiled with COMPILER_BUILD (Clang/LLVM) |

**Key implementation details:**
- `is_interpretation()` checks `call_scope->global->interpretation_mode`
- `is_runtime()`/`is_comptime()` use the `call_stack` — non-empty means we're inside a comptime evaluation
- These have `ContractFlag::IsInterpretation` set so the compiler can optimize `comptime if` branches

### 3. Type Reflection

| Intrinsic | Class | Signature | Purpose |
|-----------|-------|-----------|---------|
| `intrinsics::satisfies` | `InterpretSatisfies` | `<T, U>()` | True if type T satisfies interface U |
| `intrinsics::is` | `InterpretIs` | `<T, U>()` | True if types T and U are identical |
| `intrinsics::value_satisfies` | `InterpretValueSatisfies` | `(val, type_val)` | True if value's type satisfies the given type |
| `intrinsics::is_same_type` | `InterpretIsSameType` | `(val1, val2)` | True if both values have the same type |
| `intrinsics::type_to_string` | `InterpretTypeToString` | `<T>()` | Returns string representation of type T |
| `intrinsics::size` | `InterpretSize` | `(value)` | Returns size of string or array |
| `intrinsics::defined` | `InterpretDefined` | `(name : string)` | True if build definition is set |
| `intrinsics::supports` | `InterpretSupports` | `(feature)` | True if backend supports the feature (e.g., float128) |

**Key implementation details:**
- `satisfies` and `is` operate on **generic type arguments**, not values — they resolve at compile time
- `value_satisfies` and `is_same_type` operate on **runtime values** — they evaluate the values first
- `type_to_string` uses `RepresentationVisitor` to produce a human-readable type name
- `size` resolves references through `resolve_ref()` to get the underlying value

### 4. Source Location Reflection

| Intrinsic | Class | Purpose |
|-----------|-------|---------|
| `intrinsics::get_raw_location()` | `InterpretGetRawLocation` | Returns the encoded SourceLocation of the call site |
| `intrinsics::get_raw_loc_of(arg)` | `InterpretGetRawLocOf` | Returns the encoded SourceLocation of the argument |
| `intrinsics::get_line_no()` | `InterpretGetLineNo` | Returns the line number of the call site |
| `intrinsics::get_char_no()` | `InterpretGetCharacterNo` | Returns the character offset of the call site |
| `intrinsics::get_caller_line_no()` | `InterpretGetCallerLineNo` | Returns the line number of the outermost comptime caller |
| `intrinsics::get_caller_char_no()` | `InterpretGetCallerCharacterNo` | Returns the character offset of the outermost comptime caller |
| `intrinsics::get_call_loc(back)` | `InterpretGetCallLoc` | Returns the encoded location of the Nth caller in the call stack |
| `intrinsics::decode_location<T>(loc)` | `InterpretDecodeLocation` | Decodes an encoded location into a struct with filename, line, character fields |
| `intrinsics::get_loc_file_path(loc)` | `InterpretGetLocFilePath` | Returns the file path for an encoded location |
| `intrinsics::get_current_file_path()` | `InterpretGetCurrentFilePath` | Returns the file path of the current source file |

**Key implementation details:**
- `SourceLocation` is a 64-bit encoded value (file_id:32, line:16, column:16)
- `decode_location<T>` requires a generic argument specifying the struct to decode into (the struct must have `filename`, `line`, and `character` fields)
- `get_caller_line_no` uses `get_runtime_call()` to find the outermost comptime call site

### 5. Module/Scope Reflection

| Intrinsic | Class | Purpose |
|-----------|-------|---------|
| `intrinsics::get_module_scope()` | `InterpretGetModuleScope` | Returns the scope name of the enclosing module |
| `intrinsics::get_module_name()` | `InterpretGetModuleName` | Returns the module name |
| `intrinsics::get_module_dir()` | `InterpretGetModuleDir` | Returns the directory path of the enclosing module |
| `intrinsics::get_build_dir()` | `InterpretGetBuildDir` | Returns the build output directory |
| `intrinsics::get_target()` | `InterpretGetTarget` | Returns the target triple string (e.g., "x86_64-linux-gnu") |
| `intrinsics::version()` | `InterpretCompilerVersion` | Returns the compiler version string |

**Key implementation details:**
- Module information is retrieved via `call_scope->global->current_func_type->get_parent()` → `get_mod_scope()`
- Target triple is from `build_compiler->current_job->target_triple`
- `get_module_dir()` accesses `scope->container->paths[0]`

### 6. Function Reflection

| Intrinsic | Class | Purpose |
|-----------|-------|---------|
| `intrinsics::get_child_fn<T>(name)` | `InterpretGetChildFunction` | Returns a pointer to a child function of type T by name |
| `intrinsics::get_single_marked_decl_ptr(name)` | `InterpretGetSingleMarkedDeclPointer` | Returns a pointer to a declaration marked with the given annotation name |
| `intrinsics::get_marked_decls<T>(name)` | `InterpretGetMarkedDeclarations` | Returns an array of declarations marked with the given annotation name |

**Key implementation details:**
- `get_child_fn<T>` uses `type->get_members_container()` → `direct_child_function(name)` to find functions
- `get_marked_decls` uses `AnnotationController::get_marked()` to find all declarations with a specific annotation

### 7. Compile-Time Computation

| Intrinsic | Class | Purpose |
|-----------|-------|---------|
| `intrinsics::error(message)` | `InterpretError` | Forces a compile-time error with the given message |
| `intrinsics::forget(thing)` | `InterpretForget` | Tells the codegen to forget a variable (used for manual memory management) |
| `intrinsics::copy(dest, src)` | `InterpretMemCopy` | Memory copy intrinsic (calls backend_context->mem_copy()) |
| `intrinsics::wrap(expr)` | (via `runtime_value_of`) | Wraps an expression for runtime evaluation; in interpretation mode, evaluates immediately |

### 8. Interpreter-Friendly Standard Library

The `InterpretVector` namespace (in `GlobalFunctions.cpp`) provides a full interpreter-friendly implementation of `std::vector<T>`:

```cpp
namespace InterpretVector {
    class InterpretVectorNode : public StructDefinition {
        // Behaves like std::vector<T> but stores Values in a C++ std::vector<Value*>
    };

    class InterpretVectorVal : public StructValue {
        std::vector<Value*> values;  // The actual vector data
    };
}
```

**Supported operations:**
- `vector<T>()` — constructor (takes generic type parameter)
- `vector<T>.size()` — returns the number of elements
- `vector<T>.get(index)` — returns element at index
- `vector<T>.push(value)` — appends an element
- `vector<T>.remove(index)` — removes element at index

**How it works:**
- `InterpretVectorNode` is a `StructDefinition` with `InterpretVectorConstructor`, `InterpretVectorSize`, `InterpretVectorGet`, `InterpretVectorPush`, `InterpretVectorRemove` as child functions
- `InterpretVectorVal` extends `StructValue` and holds a `std::vector<Value*>` for the actual data
- When `vector<T>` is used in interpretation mode, the interpreter creates `InterpretVectorVal` instances instead of the runtime `std::vector<T>` C++ implementation

**Current Limitations (future work):**
- `get()` returns a **copy**, not a reference — `TODO interpret vector get should return a reference to T`
- No `operator[]` overload in interpreter
- `std::unordered_map<K, V>` not yet interpreter-friendly
- `std::string` operations (append, append_view) not implemented as intrinsics
- `std::string_view` not implemented as intrinsic
- `std::span<T>` not implemented as intrinsic

## Adding a New Intrinsic

### Step-by-Step

1. **Create the class** in `ast/utils/GlobalFunctions.cpp`:

```cpp
class InterpretMyFeature : public FunctionDeclaration {
public:
    // Declare parameters as class members
    FunctionParam myParam;

    explicit InterpretMyFeature(TypeBuilder& cache, ASTNode* parent_node)
        : FunctionDeclaration(
            "my_feature",                                // Name
            {cache.getIntType(), ZERO_LOC},              // Return type (int)
            false,                                        // Not variadic
            parent_node,
            ZERO_LOC,
            AccessSpecifier::Public,
            true                                          // is_constructor_set? No
    ), myParam("input", {cache.getStringType(), ZERO_LOC}, 0, nullptr, false, this, ZERO_LOC) {
        set_compiler_decl(true);   // Mark as compiler-provided
        params = { &myParam };     // Register parameters
    }

    Value *call(InterpretScope *call_scope, ASTAllocator& allocator,
                FunctionCall *call, Value *parent_val,
                bool evaluate_refs) override {
        // Validate arguments
        if(call->values.size() != 1) {
            call_scope->error("my_feature requires 1 argument", call);
            return nullptr;
        }

        // Evaluate the argument
        auto input = call->values[0]->evaluated_value(*call_scope);

        // Check the value kind
        if(input->val_kind() != ValueKind::String) {
            call_scope->error("expected a string argument", call);
            return nullptr;
        }

        // Return an integer
        auto& str = input->get_the_string();
        return new (allocator.allocate<IntNumValue>())
            IntNumValue(str.size(), call_scope->global->typeBuilder.getIntType(), ZERO_LOC);
    }
};
```

2. **Register the intrinsic** in `GlobalFunctions.cpp` (search for where other intrinsics are instantiated, likely in the `initGlobalFunctions` function or similar at the bottom of the file)

3. **Make it callable** from Chemical code as `intrinsics::my_feature("hello")`

### Parameter Pattern

```cpp
// Simple parameter:
FunctionParam myParam(
    "name",                                    // Parameter name
    { cache.getType(), ZERO_LOC },             // Type + location
    paramIndex,                                 // Parameter index
    defaultValue,                               // Default value (or nullptr)
    isSelf,                                     // Is it &self?
    parentFunction,                             // Owning function
    encodedLocation
);
```

### Common Return Patterns

```cpp
// Return integer
new (allocator.allocate<IntNumValue>())
    IntNumValue(value, type, location);

// Return boolean
new (allocator.allocate<BoolValue>())
    BoolValue(value, call_scope->global->typeBuilder.getBoolType(), ZERO_LOC);

// Return string
new (allocator.allocate<StringValue>())
    StringValue(string_view, call_scope->global->typeBuilder.getStringType(), ZERO_LOC);

// Return null
new (allocator.allocate<NullValue>())
    NullValue(call_scope->global->typeBuilder.getNullPtrType(), ZERO_LOC);

// Return void (no value)
return nullptr;
```

## Future Goals for Reflection

Based on the project's roadmap:

### 1. Better Compiler Reflection
- **Field enumeration**: Allow iterating struct/union/variant members at compile time
- **Type properties**: Query type alignment, size, offset of members
- **Interface checks**: Find all types that implement a given interface
- **Call chain introspection**: Access full call stack at compile time

### 2. Interpreter-Friendly Standard Library
- **`std::vector<T>`**: Already partially implemented (`InterpretVector`), needs:
  - Return references from `.get()` instead of copies
  - Support `operator[]` in interpreter
- **`std::string`**: Need interpreter-friendly intrinsics for:
  - `append()`, `append_view()`, `append_string()`
  - `size()`, `get()`, `to_view()`
  - Copy constructor and assignment
- **`std::unordered_map<K, V>`**: Need a full `InterpretUnorderedMap` similar to `InterpretVector`
- **`std::string_view`**: Need lightweight view intrinsic
- **`std::span<T>`**: Need bounded view intrinsic

### 3. Automatic Serializer/Deserializer Generation
- Use `get_child_fn<T>` and `get_marked_decls` to find struct fields at compile time
- Generate `to_json()` and `from_json()` functions automatically for any type
- Pattern:
  ```chemical
  // Future API:
  func to_json<T>(val : T) : json::JsonValue {
      // Generated at compile time using compiler reflection
      comptime {
          var result = json::Object()
          for(field in intrinsics::get_members<T>()) {
              result[field.name] = to_json(field.value)
          }
          return result
      }
  }
  ```

### 4. Automatic Generic Impl Declaration Instantiation
- When user uses `myType.method()`, if `method` comes from a generic impl declaration, automatically instantiate and resolve it
- Deprecate the need for nested `impl` blocks

## Related Skills

- **Interpreter Internals** (`.agents/skills/interpreter/SKILL.md`) — The AST interpreter, scope management, move semantics
- **Symbol Resolution** (`.agents/skills/symres/SKILL.md`) — How symbols are resolved, relevant for understanding `get_child_fn` and similar
- **Compiler API** (`.agents/skills/compiler_api/SKILL.md`) — Compiler API bindings in `lang/libs/compiler/`
