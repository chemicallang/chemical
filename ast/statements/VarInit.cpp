// Copyright (c) Qinetik 2024.

#include "VarInit.h"
#include "compiler/SymbolResolver.h"
#include "ast/types/ArrayType.h"
#include "ast/values/VariableIdentifier.h"
#include "ast/types/GenericType.h"
#include "ast/structures/StructDefinition.h"
#include "ast/values/ArrayValue.h"
#include "ast/structures/VariantDefinition.h"

#ifdef COMPILER_BUILD

#include "compiler/Codegen.h"
#include "compiler/llvmimpl.h"

void VarInitStatement::code_gen_global_var(Codegen &gen, bool initialize) {
    llvm::Constant* initializer;
    llvm::GlobalValue::LinkageTypes linkage;
    switch(specifier) {
        case AccessSpecifier::Private:
        case AccessSpecifier::Protected:
            linkage = llvm::GlobalValue::LinkageTypes::PrivateLinkage;
            break;
        case AccessSpecifier::Internal:
            linkage = llvm::GlobalValue::LinkageTypes::InternalLinkage;
            break;
        case AccessSpecifier::Public:
            linkage = llvm::GlobalValue::LinkageTypes::ExternalLinkage;
            break;
    }
    if(value && initialize) {
        const auto string_val = value->as_string_value();
        if(string_val) {
            const auto global = gen.builder->CreateGlobalString(string_val->value, runtime_name_fast(), 0, gen.module.get());
            global->setLinkage(linkage);
            global->setConstant(is_const);
            llvm_ptr = global;
            return;
        }
        initializer = (llvm::Constant*) value->llvm_value(gen, type_ptr_fast());
    } else {
        initializer = nullptr;
    }
    const auto global = new llvm::GlobalVariable(*gen.module, llvm_type(gen), is_const, linkage, initializer, runtime_name_fast());
    global->setDSOLocal(true);
    llvm_ptr = global;
}

void VarInitStatement::code_gen(Codegen &gen) {
    if (gen.current_function == nullptr) {
        if(is_const && has_annotation(AnnotationKind::CompTime)) {
            llvm_ptr = value->llvm_value(gen, type ? type : nullptr);
            return;
        }
        code_gen_global_var(gen, true);
    } else {
        if (value) {
            if(is_const && !value->as_struct_value() && !value->as_array_value()) {
                llvm_ptr = value->llvm_value(gen, type_ptr_fast());
                gen.destruct_nodes.emplace_back(this);
                return;
            }
            bool moved = false;
            if(value->is_ref_moved()) {
                auto known_t = value->pure_type_ptr();
                auto node = known_t->get_direct_linked_node();
                if(node && node->isStoredStructType(node->kind())) {
                    llvm_ptr = gen.builder->CreateAlloca(llvm_type(gen), nullptr, identifier);
                    gen.move_by_memcpy(node, value, llvm_ptr, value->llvm_value(gen));
                    moved = true;
                }
            }

            if(!moved) {

                llvm::Value* dyn_obj_impl = nullptr;

                if(type && value->value_type() == ValueType::Struct && value->as_struct_value() == nullptr) {
                    // get it's dynamic object implementation based on expected type
                    dyn_obj_impl = gen.get_dyn_obj_impl(value, type_ptr_fast());
                }

                if(!dyn_obj_impl) {
                    auto llvmType = llvm_type(gen);
                    // is referencing another struct, that is non movable and must be mem copied into the pointer
                    llvm_ptr = gen.memcpy_ref_struct(known_type(), value, nullptr, llvmType);
                    if (llvm_ptr) {
                        return;
                    }
                }

                llvm_ptr = value->llvm_allocate(gen, identifier,type_ptr_fast());
                if(dyn_obj_impl) {
                    gen.assign_dyn_obj_impl(llvm_ptr, dyn_obj_impl);
                }

            }

        } else {
            const auto t = llvm_type(gen);
            llvm_ptr = gen.builder->CreateAlloca(t, nullptr, identifier);
            const auto var = type->get_direct_linked_variant();
            if(var) {
                auto gep = gen.builder->CreateGEP(t, llvm_ptr, { gen.builder->getInt32(0), gen.builder->getInt32(0) }, "", gen.inbounds);
                gen.builder->CreateStore(gen.builder->getInt32(var->variables.size()), gep);
            }
        }
    }
    gen.destruct_nodes.emplace_back(this);
}

void VarInitStatement::code_gen_destruct(Codegen &gen, Value* returnValue) {
    if(has_moved) return;
    if(returnValue) {
        auto id = returnValue->as_identifier();
        if(id && id->linked == this) {
            return;
        }
    }
    if(value) {
        if(value->is_ref_moved()) {
            known_type()->linked_node()->llvm_destruct(gen, llvm_ptr);
            return;
        }
        value->llvm_destruct(gen, llvm_ptr);
    } else {
        auto kind = type->kind();
        switch (kind) {
            case BaseTypeKind::Linked:
                type->linked_node()->llvm_destruct(gen, llvm_ptr);
                break;
            case BaseTypeKind::Generic: {
                const auto generic_struct = type->get_generic_struct();
                const auto prev_itr = generic_struct->active_iteration;
                generic_struct->set_active_iteration(type->get_generic_iteration());
                generic_struct->llvm_destruct(gen, llvm_ptr);
                generic_struct->set_active_iteration(prev_itr);
                break;
            }
            case BaseTypeKind::Array: {
                const auto arr_type = (ArrayType *) type;
                if (arr_type->elem_type->kind() == BaseTypeKind::Linked ||
                arr_type->elem_type->kind() == BaseTypeKind::Generic) {
                    gen.destruct(llvm_ptr, arr_type->array_size, arr_type->elem_type, [](llvm::Value*){});
                }
                break;
            }
            default:
                break;
        }
    }
}

void VarInitStatement::code_gen_external_declare(Codegen &gen) {
    code_gen_global_var(gen, false);
}

llvm::Value *VarInitStatement::llvm_load(Codegen &gen) {
    if(is_const) {
        if(is_top_level()) {
            if (has_annotation(AnnotationKind::CompTime)) {
                return llvm_pointer(gen);
            }
        } else if(value) {
            return llvm_pointer(gen);
        }
        if(value && value->value_type() == ValueType::String) {
            return llvm_pointer(gen);
        }
    } else {
        const auto k_type = known_type();
        if(k_type->value_type() == ValueType::Struct) {
            return llvm_pointer(gen);
        }
//        const auto pure = k_type->pure_type();
//        if(pure->kind() == BaseTypeKind::Struct) {
//            return llvm_pointer(gen);
//        } else if(pure->get_direct_linked_variant()) {
//            return llvm_pointer(gen);
//        }
    }
    auto v = llvm_pointer(gen);
    return gen.builder->CreateLoad(llvm_type(gen), v, identifier);
}

bool VarInitStatement::add_child_index(Codegen &gen, std::vector<llvm::Value *> &indexes, const std::string &name) {
    if (value) {
        return value->add_child_index(gen, indexes, name);
    } else if (type) {
        return type->linked_node()->add_child_index(gen, indexes, name);
    }
    return false;
}

inline void VarInitStatement::check_has_type(Codegen &gen) {
    if (!type && !value) {
        gen.error("neither variable type nor variable value were given", this);
        return;
    }
}

llvm::Value *VarInitStatement::llvm_pointer(Codegen &gen) {
    return llvm_ptr;
}

llvm::Type *VarInitStatement::llvm_elem_type(Codegen &gen) {
    return value ? value->llvm_elem_type(gen) : nullptr;
}

llvm::Type *VarInitStatement::llvm_type(Codegen &gen) {
    check_has_type(gen);
    return type ? type->llvm_type(gen) : value->llvm_type(gen);
}

llvm::Type *VarInitStatement::llvm_chain_type(Codegen &gen, std::vector<ChainValue*> &values, unsigned int index) {
    check_has_type(gen);
    return type ? type->llvm_chain_type(gen, values, index) : value->llvm_chain_type(gen, values, index);
}

llvm::FunctionType *VarInitStatement::llvm_func_type(Codegen &gen) {
    return type ? type->llvm_func_type(gen) : value->llvm_func_type(gen);
}

#endif

VarInitStatement::VarInitStatement(
        bool is_const,
        std::string identifier,
        BaseType* type,
        Value* value,
        ASTNode* parent_node,
        CSTToken* token,
        AccessSpecifier specifier
) : is_const(is_const), identifier(std::move(identifier)), type(type), value(value), parent_node(parent_node), token(token), specifier(specifier) {

}

bool VarInitStatement::is_top_level() {
    return parent_node == nullptr || parent_node->as_namespace();
}

BaseType* VarInitStatement::create_value_type(ASTAllocator& allocator) {
    if(type) {
        return type;
    } else {
        return value->create_type(allocator);
    }
}

//hybrid_ptr<BaseType> VarInitStatement::get_value_type() {
//    if(type) {
//        return hybrid_ptr<BaseType> { type, false };
//    } else {
//        return value->get_base_type();
//    }
//}

BaseType* VarInitStatement::known_type() {
    if(type) {
        return type;
    }
    auto known_type = value->known_type();
    if(known_type) {
        return known_type;
    }
    return nullptr;
}

void VarInitStatement::accept(Visitor *visitor) {
    visitor->visit(this);
}

ASTNode *VarInitStatement::child(const std::string &name) {
    if (type) {
        const auto linked = type->linked_node();
        return linked ? linked->child(name) : nullptr;
    } else if (value) {
        const auto linked = value->linked_node();
        return linked ? linked->child(name) : nullptr;
    }
    return nullptr;
}

void VarInitStatement::declare_top_level(SymbolResolver &linker) {
    if(is_top_level()) {
        linker.declare_node(identifier, this, specifier, true);
    }
}

void VarInitStatement::declare_and_link(SymbolResolver &linker) {
    if (type) {
        type->link(linker);
    }
    if (value && value->link(linker, value, type_ptr_fast())) {
        linker.current_func_type->mark_moved_value(linker.allocator, value, known_type(), linker, type != nullptr);
    }
    if(!is_top_level()) {
        linker.declare(identifier, this);
    }
    if(type && value) {
        if(!type->satisfies(linker.allocator, value, false)) {
            linker.unsatisfied_type_err(value, type);
        }
        const auto as_array = value->as_array_value();
        if(type->kind() == BaseTypeKind::Array && as_array) {
            const auto arr_type = ((ArrayType*) type);
            if(arr_type->array_size == -1) {
                arr_type->array_size = (int) as_array->array_size();
            }
        }
    }
}

void VarInitStatement::interpret(InterpretScope &scope) {
    if (value) {
        auto initializer = value->scope_value(scope);
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
    if(type) {
        return type->value_type();
    } else {
        return value->value_type();
    }
}

BaseTypeKind VarInitStatement::type_kind() const {
    if(type) {
        return type->kind();
    } else {
        return value->type_kind();
    }
}