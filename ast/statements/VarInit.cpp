// Copyright (c) Chemical Language Foundation 2025.

#include "VarInit.h"
#include "compiler/SymbolResolver.h"
#include "ast/types/ArrayType.h"
#include "ast/values/StringValue.h"
#include "ast/values/VariableIdentifier.h"
#include "compiler/mangler/NameMangler.h"
#include "ast/types/GenericType.h"
#include "ast/structures/StructDefinition.h"
#include "ast/structures/VariantMember.h"
#include "ast/values/ArrayValue.h"
#include "ast/structures/VariantDefinition.h"

#ifdef COMPILER_BUILD

#include "compiler/Codegen.h"
#include "compiler/llvmimpl.h"

llvm::Value* VarInitStatement::initializer_value(Codegen &gen) {
    const auto ty = type_ptr_fast();
    const auto llvmValue = value->llvm_value(gen, ty);
    return ty ? gen.implicit_cast(llvmValue, ty, ty->llvm_type(gen)) : llvmValue;
}

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
            const auto global = gen.builder->CreateGlobalString(llvm::StringRef(string_val->value.data(), string_val->value.size()), gen.mangler.mangle(this), 0, gen.module.get());
            global->setLinkage(linkage);
            global->setConstant(is_const());
            llvm_ptr = global;
            return;
        }
        initializer = (llvm::Constant*) initializer_value(gen);
    } else {
        initializer = nullptr;
    }
    const auto global = new llvm::GlobalVariable(*gen.module, llvm_type(gen), is_const(), linkage, initializer, gen.mangler.mangle(this));
    global->setDSOLocal(true);
    llvm_ptr = global;
}

void VarInitStatement::code_gen(Codegen &gen) {
    if (gen.current_function == nullptr) {
        if(is_const() && is_comptime()) {
            llvm_ptr = initializer_value(gen);
            gen.di.declare(this, llvm_ptr);
            return;
        }
        code_gen_global_var(gen, true);
        gen.di.declare(this, llvm_ptr);
        return;
    } else {
        if (value) {

            if(is_const() && !value->as_struct_value() && !value->as_array_value()) {
                llvm_ptr = initializer_value(gen);
                put_destructible(gen);
                gen.di.declare(this, llvm_ptr);
                return;
            }

//            bool moved = false;
//            if(value->is_ref_moved()) {
//                auto known_t = value->pure_type_ptr();
//                auto node = known_t->get_direct_linked_node();
//                if(node && node->isStoredStructType(node->kind())) {
//                    const auto& name_v = name_view();
//                    const auto allocaInst = gen.builder->CreateAlloca(llvm_type(gen), nullptr, llvm::StringRef(name_v.data(), name_v.size()));
//                    gen.di.instr(allocaInst, encoded_location());
//                    llvm_ptr = allocaInst;
//                    gen.move_by_memcpy(node, value, llvm_ptr, value->llvm_value(gen));
//                    moved = true;
//                }
//            }

                // copy or move the struct, if required
                const auto exp_type = known_type_or_err();
                if(value->requires_memcpy_ref_struct(exp_type)) {
                    const auto& name_v = name_view();
                    const auto allocaInst = gen.builder->CreateAlloca(llvm_type(gen), nullptr, llvm::StringRef(name_v.data(), name_v.size()));
                    gen.di.instr(allocaInst, encoded_location());
                    llvm_ptr = allocaInst;
                    if(!gen.copy_or_move_struct(exp_type, value, allocaInst)) {
                        gen.warn("couldn't copy or move the struct to location", this);
                    }
                    put_destructible(gen);
                    gen.di.declare(this, llvm_ptr);
                    return;
                }

//            if(!moved) {

                llvm::Value* dyn_obj_impl = nullptr;

                if(type && type->isStructLikeType() && value->as_struct_value() == nullptr) {
                    // get it's dynamic object implementation based on expected type
                    dyn_obj_impl = gen.get_dyn_obj_impl(value, type_ptr_fast());
                }

//                if(!dyn_obj_impl) {
//                    auto llvmType = llvm_type(gen);
//                    // is referencing another struct, that is non movable and must be mem copied into the pointer
//                    llvm_ptr = gen.memcpy_ref_struct(created_val_type, value, nullptr, llvmType);
//                    if (llvm_ptr) {
//                        gen.di.declare(this, llvm_ptr);
//                        return;
//                    }
//                }

                llvm_ptr = value->llvm_allocate(gen, name_str(),type_ptr_fast());
                if(dyn_obj_impl) {
                    gen.assign_dyn_obj_impl(llvm_ptr, dyn_obj_impl, encoded_location());
                }
                gen.di.declare(this, llvm_ptr);

//            }

        } else {
            const auto t = llvm_type(gen);
            const auto& v = name_view();
            const auto allocaInst = gen.builder->CreateAlloca(t, nullptr, llvm::StringRef(v.data(), v.size()));
            gen.di.instr(allocaInst, encoded_location());
            llvm_ptr = allocaInst;
            const auto var = type->get_direct_linked_variant();
            if(var) {
                auto gep = gen.builder->CreateGEP(t, llvm_ptr, { gen.builder->getInt32(0), gen.builder->getInt32(0) }, "", gen.inbounds);
                const auto storeInst = gen.builder->CreateStore(gen.builder->getInt32(var->variables().size()), gep);
                gen.di.instr(storeInst, encoded_location());
            }
        }
        put_destructible(gen);
        gen.di.declare(this, llvm_ptr);
    }
}

void queue_destruct(Codegen& gen, VarInitStatement* node, ASTNode* typeLinked) {
    const auto container = typeLinked->get_members_container();
    if(container) {
        const auto destructor = container->destructor_func();
        if(destructor) {
            llvm::Value* should_destruct = nullptr;
            // if a single move was performed using this variable
            // we allocate a flag
            if(node->get_has_move()) {

                // create a boolean flag
                const auto instr = gen.builder->CreateAlloca(gen.builder->getInt1Ty());
                gen.di.instr(instr, node);

                // store true in it, that this value should be destructed
                const auto storeIns = gen.builder->CreateStore(gen.builder->getInt1(true), instr);
                gen.di.instr(storeIns, node);

                should_destruct = instr;
            }
            gen.destruct_nodes.emplace_back(node, should_destruct);
        }
    } else if(typeLinked->kind() == ASTNodeKind::VariantMember) {
        queue_destruct(gen, node, typeLinked->as_variant_member_unsafe()->parent());
    }
}

void queue_destruct(Codegen& gen, VarInitStatement* node, BaseType* type) {
    switch(type->kind()) {
        case BaseTypeKind::Linked:
            queue_destruct(gen, node, type->as_linked_type_unsafe()->linked);
            return;
        case BaseTypeKind::Generic:
            queue_destruct(gen, node, type->as_generic_type_unsafe()->referenced->linked);
            return;
        case BaseTypeKind::Array:
            queue_destruct(gen, node, type->as_array_type_unsafe()->elem_type);
            return;
        default:
            return;
    }
}

void VarInitStatement::put_destructible(Codegen& gen) {
    queue_destruct(gen, this, known_type_or_err());
}

void VarInitStatement::code_gen_external_declare(Codegen &gen) {
    if(is_comptime() && is_const()) {
        llvm_ptr = initializer_value(gen);
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
        const auto k_type = known_type_or_err();
        if(k_type->isStructLikeType()) {
            return llvm_pointer(gen);
        }
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

BaseType* VarInitStatement::known_type_SymRes(ASTAllocator& allocator) {
    if(type) {
        return type;
    } else {
        return value->create_type(allocator);
    }
}

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
        if(value->kind() == ValueKind::CastedValue) {
            const auto t = value->known_type();
            const auto l = t->linked_node();
            return l ? l->child(name) : nullptr;
        } else {
            const auto linked = value->linked_node();
            return linked ? linked->child(name) : nullptr;
        }
    }
    return nullptr;
}

void VarInitStatement::declare_top_level(SymbolResolver &linker, ASTNode*& node_ptr) {
    linker.declare_node(id_view(), this, specifier(), true);
}

void VarInitStatement::link_signature(SymbolResolver &linker) {
    const auto type_resolved = !type || type.link(linker);
    const auto value_resolved = !value || value->link(linker, value, type_ptr_fast());
    if(!type_resolved || !value_resolved) {
        attrs.signature_resolved = false;
    } else {
        if(type && value) {
            const auto as_array = value->as_array_value();
            if(type->kind() == BaseTypeKind::Array && as_array) {
                const auto arr_type = ((ArrayType*) type.getType());
                if(arr_type->has_no_array_size()) {
                    arr_type->set_array_size(as_array->array_size());
                } else if(!as_array->has_explicit_size()) {
                    as_array->set_array_size(arr_type->get_array_size());
                }
            }
            if(!type->satisfies(linker.allocator, value, false)) {
                linker.unsatisfied_type_err(value, type);
            }
        }
    }
}

void VarInitStatement::declare_and_link(SymbolResolver &linker, ASTNode*& node_ptr) {
    if(is_top_level()) {
        if(attrs.signature_resolved && !type && value) {
            type = {value->create_type(*linker.ast_allocator), type.getLocation()};
        }
    } else {
        const auto type_resolved = !type || type.link(linker);
        const auto value_resolved = !value || value->link(linker, value, type_ptr_fast());
        if (!type_resolved || !value_resolved) {
            attrs.signature_resolved = false;
        }
        linker.declare(id_view(), this);
        if (attrs.signature_resolved) {
            if(value) {
                linker.current_func_type->mark_moved_value(linker.allocator, value, known_type(), linker, type != nullptr);
            }
            if(type && value) {
                const auto as_array = value->as_array_value();
                if(type->kind() == BaseTypeKind::Array && as_array) {
                    const auto arr_type = ((ArrayType*) type.getType());
                    if(arr_type->has_no_array_size()) {
                        arr_type->set_array_size(as_array->array_size());
                    } else if(!as_array->has_explicit_size()) {
                        as_array->set_array_size(arr_type->get_array_size());
                    }
                }
                if(!type->satisfies(linker.allocator, value, false)) {
                    linker.unsatisfied_type_err(value, type);
                }
            }
            if(!type && value && !linker.generic_context) {
                type = {value->create_type(*linker.ast_allocator), type.getLocation()};
            }
        }
    }
}

/**
 * called by assignment to assign a new value in the scope that this variable was declared
 */
void VarInitStatement::declare(Value *new_value) {
    decl_scope->declare(name_view(), new_value);
}