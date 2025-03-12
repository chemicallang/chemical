// Copyright (c) Chemical Language Foundation 2025.

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
    bool is_ns;
    bool is_moved = false;

    /**
     * constructor
     */
    constexpr VariableIdentifier(
        chem::string_view value,
        SourceLocation location,
        bool is_ns = false
    ) : ChainValue(ValueKind::Identifier, location), value(value), is_ns(is_ns) {

    }


    uint64_t byte_size(bool is64Bit) final;

    BaseType* known_type() final;

    Value *child(InterpretScope &scope, const chem::string_view &name) final;

    // will find value by this name in the parent
    Value *find_in(InterpretScope &scope, Value *parent) final;

    void set_value_in(InterpretScope &scope, Value *parent, Value *next_value, Operation op, SourceLocation location);

    void set_value(InterpretScope &scope, Value *rawValue, Operation op, SourceLocation location);

    void process_linked(ASTDiagnoser* linker);

    bool link(SymbolResolver &linker, bool check_access, Value** ptr_ref);

    bool link(SymbolResolver &linker, Value*& value_ptr, BaseType *expected_type = nullptr) final {
        return link(linker, true, &value_ptr);
    }

    inline bool link_assign(SymbolResolver &linker, Value *&value_ptr, BaseType *expected_type = nullptr) {
        return link(linker, false, &value_ptr);
    }

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

#endif

    Value* evaluated_value(InterpretScope &scope) final;

    VariableIdentifier *copy(ASTAllocator& allocator) final;

    BaseType* create_type(ASTAllocator &allocator) final;

    [[nodiscard]]
    BaseTypeKind type_kind() const final;

};