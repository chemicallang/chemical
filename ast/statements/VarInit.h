// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include "ast/base/ASTNode.h"
#include "ast/base/Value.h"
#include "lexer/model/tokens/NumberToken.h"
#include <optional>
#include "ast/base/BaseType.h"
#include "ast/base/GlobalInterpretScope.h"

class VarInitStatement : public ASTNode {
public:

    /**
     * @brief Construct a new InitStatement object.
     *
     * @param identifier The identifier being initialized.
     * @param value The value being assigned to the identifier.
     */
    VarInitStatement(
            bool is_const,
            std::string identifier,
            std::optional<std::unique_ptr<BaseType>> type,
            std::optional<std::unique_ptr<Value>> value
    );

    void accept(Visitor *visitor) override;

    Value* holding_value() override {
        return value.has_value() ? value.value().get() : nullptr;
    }

    BaseType* holding_value_type() override {
        return type.has_value() ? type.value().get() : nullptr;
    }

#ifdef COMPILER_BUILD

    inline void check_has_type(Codegen &gen);

    llvm::Value *llvm_pointer(Codegen &gen) override;

    llvm::Type *llvm_elem_type(Codegen &gen) override;

    llvm::Type *llvm_type(Codegen &gen) override;

    llvm::Value *llvm_load(Codegen &gen) override;

    llvm::FunctionType *llvm_func_type(Codegen &gen) override;

    bool add_child_index(Codegen &gen, std::vector<llvm::Value *> &indexes, const std::string &name) override;

    void code_gen(Codegen &gen) override;

    void code_gen_destruct(Codegen &gen) override;

#endif

    ASTNode *child(const std::string &name) override;

    VarInitStatement *as_var_init() override;

    void declare_and_link(SymbolResolver &linker) override;

    void interpret(InterpretScope &scope) override;

    std::unique_ptr<BaseType> create_value_type() override;

    /**
     * called by assignment to assign a new value in the scope that this variable was declared
     */
    void declare(Value *new_value);

    /**
     * called when the value associated with this var init has been moved
     */
    void moved();

    void interpret_scope_ends(InterpretScope &scope) override;

    std::string representation() const override;

    ValueType value_type() const override;

    BaseTypeKind type_kind() const override;

    bool is_const;
    bool is_reference = false;
    bool has_moved = false;
    InterpretScope *decl_scope = nullptr;
    std::string identifier; ///< The identifier being initialized.
    std::optional<std::unique_ptr<BaseType>> type;
    std::optional<std::unique_ptr<Value>> value; ///< The value being assigned to the identifier.

#ifdef COMPILER_BUILD
    llvm::Value *llvm_ptr;
#endif

};