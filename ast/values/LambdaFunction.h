// Copyright (c) Chemical Language Foundation 2025.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include "ast/base/Value.h"
#include "ast/structures/Scope.h"
#include "ast/types/FunctionType.h"
#include "std/except.h"

class FunctionDeclaration;

class CapturedVariable;

class LambdaFunctionCopy : public Value {
public:
    using Value::Value;
    Value* copy(ASTAllocator &allocator) override;
};

/**
 * @brief Class representing an integer value.
 */
class LambdaFunction : public LambdaFunctionCopy, public FunctionTypeBody {
public:

    std::vector<CapturedVariable*> captureList;
    Scope scope;

#ifdef COMPILER_BUILD

    llvm::Function* func_ptr = nullptr;
    llvm::Value *captured_struct = nullptr;
    llvm::Function* capturedDestructor = nullptr;

#endif

    /**
     * constructor
     */
    constexpr LambdaFunction(
            bool isVariadic,
            ASTNode* parent_node,
            SourceLocation location
    ) : LambdaFunctionCopy(ValueKind::LambdaFunc, this, location), FunctionTypeBody(nullptr, isVariadic, false, false), scope(parent_node, location) {

    }

    ASTNode* get_parent() final {
        return scope.parent();
    }

    SourceLocation get_location() final {
        return encoded_location();
    }

    LambdaFunction* as_lambda() final {
        return this;
    }

    int data_struct_index() {
        return has_self_param() ? 1 : 0;
    }

#ifdef COMPILER_BUILD

    llvm::Type *capture_struct_type(Codegen &gen);

protected:

    /**
     * capture the variables in capture list into a single struct and return it
     */
    llvm::AllocaInst *capture_struct(Codegen &gen);

public:

    llvm::Type *llvm_type(Codegen &gen) final;

    llvm::Value* llvm_value_unpacked(Codegen &gen, BaseType* expected_type);

    llvm::Value *llvm_value(Codegen &gen, BaseType* expected_type) final;

    llvm::Value* llvm_pointer(Codegen &gen) override {
        return llvm_value(gen, nullptr);
    }

    void llvm_assign_value(Codegen &gen, llvm::Value *lhsPtr, Value *lhs) override;

    void generate_captured_destructor(Codegen &gen);

#endif

    /**
     * should generate a destructor for capture struct
     */
    bool has_destructor_for_capture();

    void set_return(InterpretScope &scope, Value *value) override {
#ifdef DEBUG
        CHEM_THROW_RUNTIME("NOT YET IMPLEMENTED");
#endif
    }

};