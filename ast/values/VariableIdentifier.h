// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 02/03/2024.
//

#pragma once

#include <utility>
#include "ast/base/ChainValue.h"
#include "ast/statements/VarInit.h"

#ifdef COMPILER_BUILD

#include "compiler/llvmfwd.h"

#endif

/**
 * @brief Class representing a VariableIdentifier.
 */
class VariableIdentifier : public ChainValue {
public:

    /**
     * string value of identifier
     */
    chem::string_view value;
    ASTNode *linked = nullptr;
    ChainValue* parent_val = nullptr;
    SourceLocation location;
    bool is_ns;
    bool is_moved = false;

    /**
     * constructor
     */
    VariableIdentifier(
        chem::string_view value,
        SourceLocation location,
        bool is_ns = false
    ) : ChainValue(ValueKind::Identifier), value(value), location(location), is_ns(is_ns) {

    }

    SourceLocation encoded_location() final {
        return location;
    }

    uint64_t byte_size(bool is64Bit) final;

    void accept(Visitor *visitor) final {
        visitor->visit(this);
    }

    BaseType* known_type() final;

    Value *child(InterpretScope &scope, const chem::string_view &name) final;

    // will find value by this name in the parent
    Value *find_in(InterpretScope &scope, Value *parent) final;

    void set_value_in(InterpretScope &scope, Value *parent, Value *next_value, Operation op, SourceLocation location);

    void set_value(InterpretScope &scope, Value *rawValue, Operation op, SourceLocation location);

    bool link(SymbolResolver &linker, bool check_access);

    bool link(SymbolResolver &linker, std::vector<ChainValue *> &values, unsigned int index, BaseType *expected_type) final {
        const auto values_size = values.size();
        const auto parent_index = index - 1;
        const auto parent = parent_index < values_size ? values[parent_index] : nullptr;
        if(parent) {
            return find_link_in_parent(parent, linker, expected_type);
        } else {
            return link(linker, false);
        }
    }

    bool link(SymbolResolver &linker, Value*& value_ptr, BaseType *expected_type = nullptr) final {
        return link(linker, true);
    }

    bool link_assign(SymbolResolver &linker, Value *&value_ptr, BaseType *expected_type = nullptr) final {
        return link(linker, false);
    }

    void relink_parent(ChainValue *parent) final;

    ASTNode *linked_node() final;

    bool find_link_in_parent(ChainValue *parent, ASTDiagnoser *diagnoser);

    bool find_link_in_parent(ChainValue *parent, SymbolResolver &resolver, BaseType *expected_type);

    bool primitive() final {
        return false;
    }

    bool compile_time_computable() final;

#ifdef COMPILER_BUILD

    bool add_member_index(Codegen &gen, Value *parent, std::vector<llvm::Value *> &indexes) final;

    bool add_child_index(Codegen& gen, std::vector<llvm::Value *>& indexes, const chem::string_view& name) final;

    llvm::Type *llvm_type(Codegen &gen) final;

    llvm::Type *llvm_chain_type(Codegen &gen, std::vector<ChainValue*> &chain, unsigned int index) final;

    llvm::Value *llvm_pointer(Codegen &gen) final;

    llvm::Value *llvm_value(Codegen &gen, BaseType* expected_type) final;

    llvm::Value *llvm_ret_value(Codegen &gen, ReturnStatement *returnStmt) final;

#endif

    Value* evaluated_value(InterpretScope &scope) final;

    VariableIdentifier *copy(ASTAllocator& allocator) final;

    BaseType* create_type(ASTAllocator &allocator) final;

    [[nodiscard]]
    BaseTypeKind type_kind() const final;

};