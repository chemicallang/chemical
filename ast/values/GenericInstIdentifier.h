// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/base/Value.h"
#include "ast/base/TypeLoc.h"
#include "ast/values/VariableIdentifier.h"
#include <vector>

/**
 * a bare generic function reference used as a value, with explicit generic
 * arguments. It is written `ident<int>` or `ns::ident<int>` and appears as the
 * last value of an access chain (or as a lone value when the chain is singlified).
 *
 * The arguments are stored on this node instead of on VariableIdentifier, so that
 * only the rare generic function reference pays for the vector — every other
 * identifier stays small.
 *
 * Symbol resolution links the wrapped identifier to the generic declaration, then
 * instantiates the function with these arguments and relinks the identifier to the
 * resulting concrete FunctionDeclaration. Every other phase treats this node as a
 * transparent wrapper around its identifier (mirroring UnsafeValue), so it behaves
 * exactly like the identifier would have.
 */
class GenericInstIdentifier : public Value {
private:

    /**
     * the identifier being instantiated, never null
     */
    VariableIdentifier* identifier;

public:

    std::vector<TypeLoc> generic_list;

    GenericInstIdentifier(
        VariableIdentifier* identifier,
        std::vector<TypeLoc> generic_list
    ) : Value(
            ValueKind::GenericInstIdentifier,
            identifier->getType(),
            identifier->encoded_location()
        ),
        identifier(identifier),
        generic_list(std::move(generic_list)) {

    }

    inline VariableIdentifier* getIdentifier() const {
        return identifier;
    }

    void setIdentifier(VariableIdentifier* newIdentifier) {
        identifier = newIdentifier;
        setType(newIdentifier->getType());
    }

    GenericInstIdentifier* copy(ASTAllocator &allocator) override;

    ASTNode* linked_node() final {
        return identifier->linked_node();
    }

    uint64_t byte_size(const TargetData& targetData) final {
        return identifier->byte_size(targetData);
    }

    bool primitive() final {
        return false;
    }

    bool compile_time_computable() final {
        return identifier->compile_time_computable();
    }

    Value* child(InterpretScope& scope, const chem::string_view& name) final {
        return identifier->child(scope, name);
    }

    Value* find_in(InterpretScope& scope, Value* parent) final {
        return identifier->find_in(scope, parent);
    }

    void set_value(InterpretScope& scope, Value* value, Operation op, SourceLocation location) final {
        identifier->set_value(scope, value, op, location);
    }

    void set_value_in(InterpretScope& scope, Value* parent, Value* value, Operation op, SourceLocation location) final {
        identifier->set_value_in(scope, parent, value, op, location);
    }

    Value* evaluated_value(InterpretScope& scope) final {
        return identifier->evaluated_value(scope);
    }

#ifdef COMPILER_BUILD

    // This wrapper has no runtime meaning of its own: after symbol resolution the
    // identifier is linked to the concrete function, so every code generation entry
    // point simply forwards to it.

    llvm::Value* llvm_value(Codegen& gen, BaseType* type = nullptr) final {
        return identifier->llvm_value(gen, type);
    }

    llvm::Value* llvm_pointer(Codegen& gen) final {
        return identifier->llvm_pointer(gen);
    }

    llvm::Type* llvm_type(Codegen& gen) final {
        return identifier->llvm_type(gen);
    }

    llvm::Type* llvm_chain_type(Codegen& gen, std::vector<Value*>& values, unsigned int index) final {
        return identifier->llvm_chain_type(gen, values, index);
    }

    llvm::Value* llvm_arg_value(Codegen& gen, BaseType* expected_type) final {
        return identifier->llvm_arg_value(gen, expected_type);
    }

    llvm::Value* llvm_ret_value(Codegen& gen, Value* returnValue) final {
        return identifier->llvm_ret_value(gen, returnValue);
    }

    void llvm_assign_value(Codegen& gen, llvm::Value* storagePtr, Value* lhs, llvm::Value* lhsPtr) final {
        identifier->llvm_assign_value(gen, storagePtr, lhs, lhsPtr);
    }

    void llvm_conditional_branch(Codegen& gen, llvm::BasicBlock* then_block, llvm::BasicBlock* otherwise_block) final {
        identifier->llvm_conditional_branch(gen, then_block, otherwise_block);
    }

    llvm::AllocaInst* llvm_allocate(Codegen& gen, const std::string& name, BaseType* expected_type) final {
        return identifier->llvm_allocate(gen, name, expected_type);
    }

    bool add_member_index(Codegen& gen, Value* parent, std::vector<llvm::Value*>& indexes) final {
        return identifier->add_member_index(gen, parent, indexes);
    }

    bool add_child_index(Codegen& gen, std::vector<llvm::Value*>& indexes, const chem::string_view& name) final {
        return identifier->add_child_index(gen, indexes, name);
    }

    llvm::AllocaInst* access_chain_allocate(
        Codegen& gen,
        std::vector<Value*>& values,
        unsigned int until,
        BaseType* expected_type
    ) final {
        return identifier->access_chain_allocate(gen, values, until, expected_type);
    }

    void access_chain_assign_value(
        Codegen& gen,
        AccessChain* chain,
        unsigned int until,
        std::vector<std::pair<Value*, llvm::Value*>>& destructibles,
        llvm::Value* storagePtr,
        Value* lhs,
        llvm::Value* lhsPtr,
        BaseType* expected_type
    ) final {
        identifier->access_chain_assign_value(gen, chain, until, destructibles, storagePtr, lhs, lhsPtr, expected_type);
    }

#endif

};
