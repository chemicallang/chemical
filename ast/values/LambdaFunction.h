// Copyright (c) Qinetik 2024.

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

#endif

    /**
     * @brief Construct a new IntValue object.
     *
     * @param value The integer value.
     */
    LambdaFunction(
            std::vector<CapturedVariable*> captureList,
            std::vector<FunctionParam*> params,
            bool isVariadic,
            Scope scope,
            ASTNode* parent_node,
            SourceLocation location
    ) : Value(ValueKind::LambdaFunc, location), captureList(std::move(captureList)), FunctionTypeBody(std::move(params), nullptr, isVariadic, !captureList.empty(), parent_node, location), scope(std::move(scope)) {

    }

    /**
     * constructor
     */
    LambdaFunction(
            bool isVariadic,
            Scope scope,
            ASTNode* parent_node,
            SourceLocation location
    ) : Value(ValueKind::LambdaFunc, location), FunctionTypeBody(nullptr, isVariadic, !captureList.empty(), parent_node, location), scope(std::move(scope)) {

    }

    LambdaFunction* as_lambda() final {
        return this;
    }

    ASTNode *parent() final {
        return parent_node;
    }

    void accept(Visitor *visitor) final {
        visitor->visit(this);
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

    llvm::Value *llvm_value(Codegen &gen, BaseType* expected_type) final;

#endif

    BaseType* create_type(ASTAllocator &allocator) final;

    BaseType* known_type() final {
        return this;
    }

    bool link(SymbolResolver &linker, Value*& value_ptr, BaseType *expected_type = nullptr) final;

    bool link(SymbolResolver &linker, FunctionType* func_type);

    void set_return(InterpretScope &scope, Value *value) override {
#ifdef DEBUG
      throw std::runtime_error("NOT YET IMPLEMENTED");
#endif
    }

};