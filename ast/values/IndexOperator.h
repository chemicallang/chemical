// Copyright (c) Chemical Language Foundation 2025.

//
// Created by Waqas Tahir on 29/02/2024.
//

#pragma once

#include <memory>
#include <utility>
#include "ast/base/ChainValue.h"

class IndexOperator : public ChainValue {
public:

    ChainValue* parent_val;
    std::vector<Value*> values;

    /**
     * constructor
     */
    constexpr IndexOperator(
            ChainValue* parent,
            SourceLocation location
    ) : ChainValue(ValueKind::IndexOperator, location), parent_val(parent) {

    }

    Value* evaluated_value(InterpretScope &scope) override;

    bool link(SymbolResolver& linker, BaseType* expected_type) final;

    ASTNode *linked_node() final;

    bool primitive() final {
        return false;
    }

    Value *find_in(InterpretScope &scope, Value *parent) final;

    IndexOperator* copy(ASTAllocator& allocator) final;

    BaseType* create_type(ASTAllocator &allocator) final;

    BaseType* known_type() final;

#ifdef COMPILER_BUILD

    llvm::Value *elem_pointer(Codegen &gen, llvm::Type *, llvm::Value *ptr);

    llvm::Value *llvm_pointer(Codegen &gen) final;

    llvm::Value *llvm_value(Codegen &gen, BaseType* expected_type) final;

    llvm::Type *llvm_type(Codegen &gen) final;

    llvm::Type *llvm_chain_type(Codegen &gen, std::vector<ChainValue*> &chain, unsigned int index) final;

    bool add_member_index(Codegen &gen, Value *parent, std::vector<llvm::Value *> &indexes) final;

    bool add_child_index(Codegen& gen, std::vector<llvm::Value *>& indexes, const chem::string_view& name) final;

#endif

};