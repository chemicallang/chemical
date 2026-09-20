// Copyright (c) Chemical Language Foundation 2025.

//
// Created by Waqas Tahir on 02/03/2024.
//

#pragma once

#include <utility>
#include <atomic>
#include <vector>
#include "ast/base/Value.h"
#include "ast/statements/VarInit.h"

#ifdef COMPILER_BUILD

#include "compiler/llvmfwd.h"

#endif

/**
 * @brief Class representing a VariableIdentifier.
 */
class VariableIdentifier : public Value {
public:

    /**
     * string value of identifier
     */
    chem::string_view value;
    ASTNode *linked = nullptr;
    bool is_ns;
    bool is_moved = false;

    /**
     * Interpreter inline cache: number of parent-scope hops from the evaluation
     * point to the scope that owns this identifier's binding. The syntactic
     * nesting of a given identifier is fixed, so this depth is stable across
     * loop iterations and recursive calls, letting us skip re-scanning ancestor
     * scopes on every read/write. Reset (to false) on copy.
     */
    std::atomic<unsigned> cached_value_depth{0};
    std::atomic<bool> value_depth_cached{false};

    /**
     * constructor
     */
    constexpr VariableIdentifier(
        chem::string_view value,
        SourceLocation location,
        bool is_ns = false
    ) : Value(ValueKind::Identifier, location), value(value), is_ns(is_ns) {

    }

    /**
     * constructor
     */
    constexpr VariableIdentifier(
            chem::string_view value,
            BaseType* type,
            SourceLocation location,
            bool is_ns = false
    ) : Value(ValueKind::Identifier, type, location), value(value), is_ns(is_ns) {

    }

protected:

    /**
     * constructor for derived values that carry extra front-end state but behave
     * exactly like an identifier (see GenericInstIdentifier). Such a value is an
     * identifier with a different ValueKind, so isIdentifier() / as_identifier()
     * accept it and every identifier method applies to it as it is
     */
    constexpr VariableIdentifier(
            ValueKind kind,
            chem::string_view value,
            BaseType* type,
            SourceLocation location,
            bool is_ns
    ) : Value(kind, type, location), value(value), is_ns(is_ns) {

    }

public:

    uint64_t byte_size(const TargetData& targetData) final;

    Value *child(InterpretScope &scope, const chem::string_view &name) final;

    // will find value by this name in the parent
    Value *find_in(InterpretScope &scope, Value *parent) final;

    void set_value_in(InterpretScope &scope, Value *parent, Value *next_value, Operation op, SourceLocation location);

    void set_value(InterpretScope &scope, Value *rawValue, Operation op, SourceLocation location);

    void process_linked();

    void process_linked(ASTDiagnoser* linker, FunctionTypeBody* curr_func);

    ASTNode *linked_node() final;

    bool primitive() final {
        return false;
    }

    bool compile_time_computable() final;

#ifdef COMPILER_BUILD

    bool add_member_index(Codegen &gen, Value *parent, std::vector<llvm::Value *> &indexes) final;

    bool add_child_index(Codegen& gen, std::vector<llvm::Value *>& indexes, const chem::string_view& name) final;

    llvm::Type *llvm_type(Codegen &gen) final;

    llvm::Type *llvm_chain_type(Codegen &gen, std::vector<Value*> &chain, unsigned int index) final;

    llvm::Value *llvm_pointer(Codegen &gen) final;

    llvm::Value *llvm_value(Codegen &gen, BaseType* expected_type) final;

#endif

    Value* evaluated_value(InterpretScope &scope) final;

    VariableIdentifier *copy(ASTAllocator& allocator) override;

};