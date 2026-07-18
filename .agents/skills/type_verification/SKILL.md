---
name: Type Verification
description: Comprehensive guide to the Chemical type verification pass — how types are verified after symbol resolution, before codegen.
---

# Type Verification

The type verification pass runs after symbol resolution and before code generation. It validates that all type constraints are satisfied, catches type mismatches that symbol resolution missed, and ensures the AST is type-safe for codegen.

## Architecture

### Pipeline Position

```
Parse → Symbol Resolution (SymResLinkBody) → Type Verify → Codegen
```

### Key Files

| File | Purpose |
|------|---------|
| `compiler/typeverify/TypeVerify.h` | Type verifier class — visits all AST nodes for type checking |
| `compiler/typeverify/TypeVerify.cpp` | Implementation — type checking logic for each AST node |
| `compiler/typeverify/TypeVerifyAPI.h` | API entry point — `type_verify()` function |

## Entry Point

```cpp
// In TypeVerifyAPI.h:
void type_verify(
    ImplementationsIndex& index,
    ASTDiagnoser& diagnoser,
    ASTAllocator& allocator,
    std::span<ASTNode*> nodes
);
```

This is called once per module, after symbol resolution completes:

```cpp
// In the compilation pipeline:
after_symres(module) {
    type_verify(implsIndex, diagnoser, allocator, module->topLevelNodes);
}
```

## TypeVerifier Class

```cpp
class TypeVerifier {
    ImplementationsIndex& index;       // For operator overload resolution
    ASTDiagnoser& diagnoser;           // Error reporting
    ASTAllocator& allocator;           // Arena for temporary allocations
    
    // Visitor methods — one per AST node type
    void VisitFunctionDecl(FunctionDeclaration* node);
    void VisitVarInitStmt(VarInitStatement* node);
    void VisitAssignStmt(AssignStatement* node);
    void VisitExpression(Expression* node);
    void VisitFunctionCall(FunctionCall* node);
    void VisitReturnStmt(ReturnStatement* node);
    void VisitStructValue(StructValue* node);
    void VisitIfStmt(IfStatement* node);
    void VisitWhileStmt(WhileLoop* node);
    // ... etc
};
```

## What Type Verification Checks

### 1. Function Return Types

Verifies that the returned value type matches the function's declared return type:

```cpp
void TypeVerifier::VisitFunctionDecl(FunctionDeclaration* node) {
    BaseType* declaredReturn = node->returnType;
    for(auto& stmt : node->body.nodes) {
        if(auto* retStmt = stmt->as_return_stmt_unsafe()) {
            BaseType* actualReturn = retStmt->value->get_type();
            if(!types_match(declaredReturn, actualReturn)) {
                diagnoser.error(retStmt->location(),
                    "expected return type '" + declaredReturn->representation() +
                    "', got '" + actualReturn->representation() + "'");
            }
        }
    }
}
```

### 2. Variable Type Compatibility

Verifies that a variable's initializer type matches the declared type:

```cpp
void TypeVerifier::VisitVarInitStmt(VarInitStatement* node) {
    BaseType* declaredType = node->get_type();
    BaseType* initializerType = node->initialValue->get_type();
    if(!types_match(declaredType, initializerType)) {
        // Report type mismatch
        unsatisfied_type_err(diagnoser, node->initialValue, declaredType);
    }
}
```

### 3. Assignment Type Checking

Verifies that the RHS type is compatible with the LHS type:

```cpp
void TypeVerifier::VisitAssignStmt(AssignStatement* node) {
    BaseType* lhsType = node->lhs->get_type();
    BaseType* rhsType = node->value->get_type();
    if(!types_match(lhsType, rhsType)) {
        unsatisfied_type_err(diagnoser, node->value, lhsType);
    }
}
```

### 4. Expression Type Checking

For binary/unary expressions, verifies that the operator is valid for the operand types:

```cpp
void TypeVerifier::VisitExpression(Expression* node) {
    BaseType* lhsType = node->lhs->get_type();
    BaseType* rhsType = node->rhs->get_type();
    
    // Check operator overload availability
    if(!is_valid_operator(node->op, lhsType, rhsType)) {
        // Check if an impl for the operator exists
        auto* impl = index.get_expr_op_impl(coreNodes, lhsType->get_container(), node->op);
        if(!impl) {
            diagnoser.error(node->location(),
                "operator '" + operation_str(node->op) + "' not supported for types '" +
                lhsType->representation() + "' and '" + rhsType->representation() + "'");
        }
    }
}
```

### 5. Function Call Argument Types

Verifies that argument types match parameter types:

```cpp
void TypeVerifier::VisitFunctionCall(FunctionCall* node) {
    auto* func = node->resolved_function();
    for(size_t i = 0; i < func->params.size(); i++) {
        BaseType* paramType = func->params[i]->get_type();
        BaseType* argType = node->args[i]->get_type();
        if(!types_match(paramType, argType)) {
            diagnoser.error(node->args[i]->location(),
                "argument " + std::to_string(i) + " type mismatch: expected '" +
                paramType->representation() + "', got '" + argType->representation() + "'");
        }
    }
}
```

### 6. Struct Field Types

Verifies that struct literal field values match the struct member types:

```cpp
void TypeVerifier::VisitStructValue(StructValue* node) {
    auto* structDecl = node->linked_struct();
    for(auto& [name, value] : node->values) {
        BaseType* memberType = structDecl->get_member_type(name);
        BaseType* valueType = value->get_type();
        if(!types_match(memberType, valueType)) {
            unsatisfied_type_err(diagnoser, value, memberType);
        }
    }
}
```

### 7. Implicit Conversion Checking

Some implicit conversions are allowed, and the type verifier validates them:

| From | To | Allowed? | Rule |
|------|----|----------|------|
| `int` | `long` | Yes | Widening |
| `float` | `double` | Yes | Widening |
| `*int` | `*void` | Yes | Any pointer to void pointer |
| `*T` | `*mut T` | No | Const to mutable not allowed |
| `int` | `float` | No | Requires explicit `as` cast |
| `T` | `&T` | No | Requires explicit `&` |

## The `unsatisfied_type_err` Helper

```cpp
void unsatisfied_type_err(ASTDiagnoser& diagnoser, Value* value, BaseType* type) {
    diagnoser.error(value->encoded_location(),
        "type mismatch: expected '" + type->representation() + 
        "', but value has type '" + value->get_type()->representation() + "'");
}
```

## Integration with Operator Overloads

The type verifier uses `ImplementationsIndex` to check if operator overloads exist:

```cpp
bool TypeVerifier::has_operator_overload(Operation op, BaseType* lhs, BaseType* rhs) {
    // Get the container (struct/interface) for the LHS type
    auto* container = lhs->get_container();
    if(!container) return false;
    
    // Look up the operation in the implementations index
    auto* implFunc = index.get_expr_op_impl(coreNodes, container, op);
    if(!implFunc) return false;
    
    // Check that the RHS type matches the impl parameter
    return types_match(implFunc->params[0]->get_type(), rhs);
}
```

## What's NOT Checked in Type Verify

Some checks are handled in other passes:

| Check | Pass | Reason |
|-------|------|--------|
| Move semantics | SymResLinkBody | Must be checked during linking, not after |
| Access control (public/private) | SymResLinkBody | Checked during symbol lookup |
| Generic type bounds | GenericInstantiation | Checked during monomorphization |
| Unsafe block violations | SymResLinkBody | Tracked via context flags |
| Recursion limits | Codegen or Runtime | Not a type-level check |
| Lifetime/borrow checking | N/A | Not yet implemented in Chemical |

## Diagnostics

Type verification errors follow a standard format:

```
file.ch:line:col: error: type mismatch: expected 'int', but value has type 'float'
file.ch:line:col: error: cannot assign 'string' to 'int'
file.ch:line:col: error: operator '+' not supported for types 'bool' and 'int'
```

## Performance Considerations

1. **Single pass**: Type verification is a single pass over the AST — no backtracking
2. **No AST mutation**: The type verifier does not modify the AST (read-only)
3. **Early exit on errors**: If symbol resolution had errors, type verification may be skipped
4. **Per-module**: Type verification runs per module, enabling parallel execution
