// Copyright (c) Qinetik 2024.

#include "VarInit.h"
#include "compiler/SymbolResolver.h"
#include "ast/types/ArrayType.h"
#include "ast/values/VariableIdentifier.h"

#ifdef COMPILER_BUILD

#include "compiler/Codegen.h"
#include "compiler/llvmimpl.h"

void VarInitStatement::code_gen(Codegen &gen) {
    if (gen.current_function == nullptr) {
        if(is_const && has_annotation(AnnotationKind::CompTime)) {
            llvm_ptr = value.value()->llvm_value(gen);
            return;
        }
        if (value.has_value()) {
            llvm_ptr = value.value()->llvm_global_variable(gen, is_const, identifier);
        } else {
            llvm_ptr = new llvm::GlobalVariable(*gen.module, llvm_type(gen), is_const, llvm::GlobalValue::LinkageTypes::PrivateLinkage, nullptr, identifier);
        }
    } else {
        if (value.has_value()) {
            if(is_const) {
                llvm_ptr = value.value()->llvm_value(gen);
                return;
            }
            llvm_ptr = value.value()->llvm_allocate(gen, identifier);
        } else {
            llvm_ptr = gen.builder->CreateAlloca(llvm_type(gen), nullptr, identifier);
        }
    }
    gen.destruct_nodes.emplace_back(this);
}

void VarInitStatement::code_gen_destruct(Codegen &gen, Value* returnValue) {
    if(returnValue) {
        auto id = returnValue->as_identifier();
        if(id && id->linked == this) {
            return;
        }
    }
    if(value.has_value()) {
        value.value()->llvm_destruct(gen, llvm_ptr);
    } else {
        auto kind = type.value()->kind();
        switch(kind) {
            case BaseTypeKind::Referenced:
                type.value()->linked_node()->llvm_destruct(gen, llvm_ptr);
                break;
            case BaseTypeKind::Array: {
                const auto arr_type = (ArrayType *) type.value().get();
                if(arr_type->elem_type->kind() == BaseTypeKind::Referenced) {
                    gen.destruct(llvm_ptr, arr_type->array_size, arr_type->elem_type.get());
                }
                break;
            }
            default:
                break;
        }
    }
}

llvm::Value *VarInitStatement::llvm_load(Codegen &gen) {
    if(is_const) {
        if(is_top_level()) {
            if (has_annotation(AnnotationKind::CompTime)) {
                return llvm_pointer(gen);
            }
        } else if(value.has_value()) {
            return llvm_pointer(gen);
        }
        if(value.has_value() && value.value()->value_type() == ValueType::String) {
            return llvm_pointer(gen);
        }
    }
    auto v = llvm_pointer(gen);
    return gen.builder->CreateLoad(llvm_type(gen), v, identifier);
}

bool VarInitStatement::add_child_index(Codegen &gen, std::vector<llvm::Value *> &indexes, const std::string &name) {
    if (value.has_value()) {
        return value.value()->add_child_index(gen, indexes, name);
    } else if (type.has_value()) {
        return type.value()->linked_node()->add_child_index(gen, indexes, name);
    }
    return false;
}

inline void VarInitStatement::check_has_type(Codegen &gen) {
    if (!type.has_value() && !value.has_value()) {
        gen.error("neither variable type no variable value were given");
        return;
    }
}

llvm::Value *VarInitStatement::llvm_pointer(Codegen &gen) {
    return llvm_ptr;
}

llvm::Type *VarInitStatement::llvm_elem_type(Codegen &gen) {
    return value.has_value() ? value.value()->llvm_elem_type(gen) : nullptr;
}

llvm::Type *VarInitStatement::llvm_type(Codegen &gen) {
    check_has_type(gen);
    return type.has_value() ? type.value()->llvm_type(gen) : value.value()->llvm_type(gen);
}

llvm::Type *VarInitStatement::llvm_chain_type(Codegen &gen, std::vector<std::unique_ptr<Value>> &values, unsigned int index) {
    check_has_type(gen);
    return type.has_value() ? type.value()->llvm_chain_type(gen, values, index) : value.value()->llvm_chain_type(gen, values, index);
}

llvm::FunctionType *VarInitStatement::llvm_func_type(Codegen &gen) {
    return type.has_value() ? type.value()->llvm_func_type(gen) : value.value()->llvm_func_type(gen);
}

#endif

VarInitStatement::VarInitStatement(
        bool is_const,
        std::string identifier,
        std::optional<std::unique_ptr<BaseType>> type,
        std::optional<std::unique_ptr<Value>> value,
        ASTNode* parent_node
) : is_const(is_const), identifier(std::move(identifier)), type(std::move(type)), value(std::move(value)), parent_node(parent_node) {}

bool VarInitStatement::is_top_level() {
    return parent_node == nullptr || parent_node->as_namespace();
}

std::unique_ptr<BaseType> VarInitStatement::create_value_type() {
    if(type.has_value()) {
        return std::unique_ptr<BaseType>(type.value()->copy());
    } else {
        return value.value()->create_type();
    }
}

hybrid_ptr<BaseType> VarInitStatement::get_value_type() {
    if(type.has_value()) {
        return hybrid_ptr<BaseType> { type->get(), false };
    } else {
        auto base_type = value.value()->get_base_type();
        if(base_type.get_will_free()) {
            type.emplace(std::unique_ptr<BaseType>(base_type.get()));
            base_type.do_not_free();
        }
        return base_type;
    }
}

void VarInitStatement::accept(Visitor *visitor) {
    visitor->visit(this);
}

VarInitStatement *VarInitStatement::as_var_init() {
    return this;
}

ASTNode *VarInitStatement::child(const std::string &name) {
    if (type.has_value()) {
        return type.value()->linked_node()->child(name);
    } else if (value.has_value()) {
        return value.value()->linked_node()->child(name);
    }
    return nullptr;
}

void VarInitStatement::declare_and_link(SymbolResolver &linker) {
    linker.declare(identifier, this);
    if (type.has_value()) {
        type.value()->link(linker, type.value());
    }
    if (value.has_value()) {
        value.value()->link(linker, this);
    }
}

void VarInitStatement::interpret(InterpretScope &scope) {
    if (value.has_value()) {
        auto initializer = value.value()->initializer_value(scope);
        scope.declare(identifier, initializer);
    }
    decl_scope = &scope;
}

/**
 * called by assignment to assign a new value in the scope that this variable was declared
 */
void VarInitStatement::declare(Value *new_value) {
    decl_scope->declare(identifier, new_value);
}

ValueType VarInitStatement::value_type() const {
    if(type.has_value()) {
        return type.value()->value_type();
    } else {
        return value.value()->value_type();
    }
}

BaseTypeKind VarInitStatement::type_kind() const {
    if(type.has_value()) {
        return type.value()->kind();
    } else {
        return value.value()->type_kind();
    }
}