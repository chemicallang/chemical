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
            std::string identifier,
            std::optional<std::unique_ptr<BaseType>> type,
            std::optional<std::unique_ptr<Value>> value
    ) : identifier(std::move(identifier)), type(std::move(type)), value(std::move(value)) {}

    void accept(Visitor& visitor) override {
        visitor.visit(this);
    }

#ifdef COMPILER_BUILD
    inline void declare(Codegen &gen) {
        gen.current[identifier] = this;
    }

    inline void check_has_type(Codegen& gen) {
        if (!type.has_value() && !value.has_value()) {
            gen.error("neither variable type no variable value were given");
            return;
        }
    }

    void undeclare(Codegen &gen) override {
        gen.current.erase(identifier);
        gen.allocated.erase(identifier);
    }

    llvm::Value * llvm_pointer(Codegen &gen) override {
        return value.has_value() ? value.value()->llvm_pointer(gen) : nullptr;
    }

    llvm::Type * llvm_elem_type(Codegen &gen) override {
        return value.has_value() ? value.value()->llvm_elem_type(gen) : nullptr;
    }

    llvm::Type *llvm_type(Codegen &gen) override {
        check_has_type(gen);
        return value.has_value() ? value.value()->llvm_type(gen) : type.value()->llvm_type(gen);
    }

    void code_gen(Codegen &gen) override {
        declare(gen);
        if(value.has_value()) {
            value.value()->llvm_allocate(gen, identifier);
        } else {
            gen.allocated[identifier] = gen.builder->CreateAlloca(llvm_type(gen), nullptr, identifier);
        }
    }
#endif

    void interpret(InterpretScope &scope) override {
        if (value.has_value()) {
            if (value.value()->primitive()) {
                scope.global->values[identifier] = value.value()->copy();
            } else {
                scope.global->values[identifier] = value.value()->evaluated_value(scope);
            }
        } else {
            scope.global->nodes[identifier] = this;
        }
    }

    void interpret_scope_ends(InterpretScope &scope) override {
        if(value.has_value()) {
            auto found = scope.global->values.find(identifier);
            if(found != scope.global->values.end()) {
                if(found->second->primitive()) {
                    delete found->second;
                }
                scope.global->values.erase(found);
            } else {
                scope.error("cannot clear non existent variable on the value map " + identifier);
            }
        } else {
            scope.global->nodes.erase(identifier);
        }
    }

    std::string representation() const override {
        std::string rep;
        rep.append("var ");
        rep.append(identifier);
        if (type.has_value()) {
            rep.append(" : ");
            rep.append(type.value()->representation());
        }
        if (value.has_value()) {
            rep.append(" = ");
            rep.append(value.value()->representation());
        }
        return rep;
    }

    std::string identifier; ///< The identifier being initialized.
    std::optional<std::unique_ptr<BaseType>> type;
    std::optional<std::unique_ptr<Value>> value; ///< The value being assigned to the identifier.

};