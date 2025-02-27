// Copyright (c) Chemical Language Foundation 2025.

#include "VarInit.h"
#include "compiler/SymbolResolver.h"
#include "ast/types/ArrayType.h"
#include "ast/values/StringValue.h"
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
    switch(specifier()) {
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
            const auto global = gen.builder->CreateGlobalString(llvm::StringRef(string_val->value.data(), string_val->value.size()), runtime_name_fast(), 0, gen.module.get());
            global->setLinkage(linkage);
            global->setConstant(is_const());
            llvm_ptr = global;
            return;
        }
        initializer = (llvm::Constant*) value->llvm_value(gen, type_ptr_fast());
    } else {
        initializer = nullptr;
    }
    const auto global = new llvm::GlobalVariable(*gen.module, llvm_type(gen), is_const(), linkage, initializer, runtime_name_fast());
    global->setDSOLocal(true);
    llvm_ptr = global;
}

void VarInitStatement::code_gen(Codegen &gen) {
    if (gen.current_function == nullptr) {
        if(is_const() && is_comptime()) {
            llvm_ptr = value->llvm_value(gen, type ? type : nullptr);
            gen.di.declare(this, llvm_ptr);
            return;
        }
        code_gen_global_var(gen, true);
        gen.di.declare(this, llvm_ptr);
        return;
    } else {
        if (value) {

            if(is_const() && !value->as_struct_value() && !value->as_array_value()) {
                llvm_ptr = value->llvm_value(gen, type_ptr_fast());
                gen.destruct_nodes.emplace_back(this);
                gen.di.declare(this, llvm_ptr);
                return;
            }

            bool moved = false;
            if(value->is_ref_moved()) {
                auto known_t = value->pure_type_ptr();
                auto node = known_t->get_direct_linked_node();
                if(node && node->isStoredStructType(node->kind())) {
                    const auto& name_v = name_view();
                    const auto allocaInst = gen.builder->CreateAlloca(llvm_type(gen), nullptr, llvm::StringRef(name_v.data(), name_v.size()));
                    gen.di.instr(allocaInst, encoded_location());
                    llvm_ptr = allocaInst;
                    gen.move_by_memcpy(node, value, llvm_ptr, value->llvm_value(gen));
                    moved = true;
                }
            }

            if(!moved) {

                llvm::Value* dyn_obj_impl = nullptr;

                if(type && type->isStructLikeType() && value->as_struct_value() == nullptr) {
                    // get it's dynamic object implementation based on expected type
                    dyn_obj_impl = gen.get_dyn_obj_impl(value, type_ptr_fast());
                }

                if(!dyn_obj_impl) {
                    auto llvmType = llvm_type(gen);
                    // is referencing another struct, that is non movable and must be mem copied into the pointer
                    llvm_ptr = gen.memcpy_ref_struct(create_value_type(gen.allocator), value, nullptr, llvmType);
                    if (llvm_ptr) {
                        gen.di.declare(this, llvm_ptr);
                        return;
                    }
                }

                llvm_ptr = value->llvm_allocate(gen, name_str(),type_ptr_fast());
                if(dyn_obj_impl) {
                    gen.assign_dyn_obj_impl(llvm_ptr, dyn_obj_impl, encoded_location());
                }
                gen.di.declare(this, llvm_ptr);

            }

        } else {
            const auto t = llvm_type(gen);
            const auto& v = name_view();
            const auto allocaInst = gen.builder->CreateAlloca(t, nullptr, llvm::StringRef(v.data(), v.size()));
            gen.di.instr(allocaInst, encoded_location());
            llvm_ptr = allocaInst;
            const auto var = type->get_direct_linked_variant();
            if(var) {
                auto gep = gen.builder->CreateGEP(t, llvm_ptr, { gen.builder->getInt32(0), gen.builder->getInt32(0) }, "", gen.inbounds);
                const auto storeInst = gen.builder->CreateStore(gen.builder->getInt32(var->variables.size()), gep);
                gen.di.instr(storeInst, encoded_location());
            }
        }
        gen.destruct_nodes.emplace_back(this);
        gen.di.declare(this, llvm_ptr);
    }
}

void VarInitStatement::code_gen_destruct(Codegen &gen, Value* returnValue) {
    if(get_has_moved()) return;
    if(returnValue) {
        auto linkedNode = returnValue->linked_node();
        if(linkedNode == this) {
            return;
        }
    }
    if(value) {
        if(value->is_ref_moved()) {
            known_type()->linked_node()->llvm_destruct(gen, llvm_ptr, encoded_location());
            return;
        }
        value->llvm_destruct(gen, llvm_ptr);
    } else {
        auto kind = type->kind();
        switch (kind) {
            case BaseTypeKind::Linked:
                type->linked_node()->llvm_destruct(gen, llvm_ptr, encoded_location());
                break;
            case BaseTypeKind::Generic: {
                const auto generic_struct = type->get_generic_struct();
                const auto prev_itr = generic_struct->active_iteration;
                generic_struct->set_active_iteration(type->get_generic_iteration());
                generic_struct->llvm_destruct(gen, llvm_ptr, encoded_location());
                generic_struct->set_active_iteration(prev_itr);
                break;
            }
            case BaseTypeKind::Array: {
                const auto arr_type = (ArrayType *) type;
                if (arr_type->elem_type->kind() == BaseTypeKind::Linked ||
                arr_type->elem_type->kind() == BaseTypeKind::Generic) {
                    gen.destruct(llvm_ptr, arr_type->get_array_size(), arr_type->elem_type, encoded_location());
                }
                break;
            }
            default:
                break;
        }
    }
}

void VarInitStatement::code_gen_external_declare(Codegen &gen) {
    if(is_comptime()) {
        return;
    }
    code_gen_global_var(gen, false);
}

llvm::Value *VarInitStatement::llvm_load(Codegen& gen, SourceLocation location) {
    if(is_const()) {
        if(is_top_level()) {
            if (is_comptime()) {
                return llvm_pointer(gen);
            }
        } else if(value) {
            return llvm_pointer(gen);
        }
        if(value && value->val_kind() == ValueKind::String) {
            return llvm_pointer(gen);
        }
    } else {
        const auto k_type = create_value_type(gen.allocator);
        if(k_type->isStructLikeType()) {
            return llvm_pointer(gen);
        }
//        const auto pure = k_type->pure_type(gen.allocator);
//        if(pure->kind() == BaseTypeKind::Struct) {
//            return llvm_pointer(gen);
//        } else if(pure->get_direct_linked_variant()) {
//            return llvm_pointer(gen);
//        }
    }
    auto v = llvm_pointer(gen);
    const auto& name = name_view();
    const auto loadInst = gen.builder->CreateLoad(llvm_type(gen), v, llvm::StringRef(name.data(), name.size()));
    gen.di.instr(loadInst, location);
    return loadInst;
}

bool VarInitStatement::add_child_index(Codegen& gen, std::vector<llvm::Value *>& indexes, const chem::string_view& name) {
    if (type) {
        return type->linked_node()->add_child_index(gen, indexes, name);
    } else if (value) {
        return value->add_child_index(gen, indexes, name);
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

llvm::Type *VarInitStatement::llvm_type(Codegen &gen) {
    check_has_type(gen);
    return type ? type->llvm_type(gen) : value->llvm_type(gen);
}

llvm::Type *VarInitStatement::llvm_chain_type(Codegen &gen, std::vector<ChainValue*> &values, unsigned int index) {
    check_has_type(gen);
    return type ? type->llvm_chain_type(gen, values, index) : value->llvm_chain_type(gen, values, index);
}

#endif

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

ASTNode *VarInitStatement::child(const chem::string_view &name) {
    if (type) {
        const auto linked = type->linked_node();
        return linked ? linked->child(name) : nullptr;
    } else if (value) {
        const auto linked = value->linked_node();
        return linked ? linked->child(name) : nullptr;
    }
    return nullptr;
}

void VarInitStatement::declare_top_level(SymbolResolver &linker, ASTNode*& node_ptr) {
    if(is_top_level()) {
        linker.declare_node(id_view(), this, specifier(), true);
    }
}

void VarInitStatement::declare_and_link(SymbolResolver &linker, ASTNode*& node_ptr) {
    if (type) {
        type->link(linker);
    }
    if (value && value->link(linker, value, type_ptr_fast())) {
        linker.current_func_type->mark_moved_value(linker.allocator, value, known_type(), linker, type != nullptr);
    }
    if(!is_top_level()) {
        linker.declare(id_view(), this);
    }
    if(type && value) {
        if(!type->satisfies(linker.allocator, value, false)) {
            linker.unsatisfied_type_err(value, type);
        }
        const auto as_array = value->as_array_value();
        if(type->kind() == BaseTypeKind::Array && as_array) {
            const auto arr_type = ((ArrayType*) type);
            if(arr_type->has_no_array_size()) {
                arr_type->set_array_size(as_array->array_size());
            }
        }
    }
}

void VarInitStatement::interpret(InterpretScope &scope) {
    if (value) {
        auto initializer = value->scope_value(scope);
        scope.declare(name_view(), initializer);
    }
    decl_scope = &scope;
}

/**
 * called by assignment to assign a new value in the scope that this variable was declared
 */
void VarInitStatement::declare(Value *new_value) {
    decl_scope->declare(name_view(), new_value);
}

BaseTypeKind VarInitStatement::type_kind() const {
    if(type) {
        return type->kind();
    } else {
        return value->type_kind();
    }
}