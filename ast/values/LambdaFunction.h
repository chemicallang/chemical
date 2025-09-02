// Copyright (c) Chemical Language Foundation 2025.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include "ast/base/Value.h"
#include "ast/structures/Scope.h"
#include "ast/types/FunctionType.h"

class FunctionDeclaration;

class CapturedVariable;

/**
 * @brief Class representing an integer value.
 */
class LambdaFunction : public Value, public FunctionTypeBody {
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
    ) : Value(ValueKind::LambdaFunc, this, location), FunctionTypeBody(nullptr, isVariadic, false, false), scope(parent_node, location) {

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

    void llvm_assign_value(Codegen &gen, llvm::Value *lhsPtr, Value *lhs) override;

    void generate_captured_destructor(Codegen &gen);

#endif

    BaseType* known_type() final {
        return this;
    }

    /**
     * should generate a destructor for capture struct
     */
    bool has_destructor_for_capture();

    /**
     * copy copies the entire lambda
     */
    Value* copy(ASTAllocator &allocator) override {
        // TODO:
//        const auto lambda = new (allocator.allocate<LambdaFunction>()) LambdaFunction(
//            isVariadic(), scope.parent(), getType(), encoded_location()
//        );
//        scope.copy_into(lambda->scope, allocator, scope.parent());
//        FunctionTypeBody::copy_into(*lambda, allocator, scope.parent());
//        return lambda;
        return this;
    }

    void set_return(InterpretScope &scope, Value *value) override {
#ifdef DEBUG
      throw std::runtime_error("NOT YET IMPLEMENTED");
#endif
    }

};