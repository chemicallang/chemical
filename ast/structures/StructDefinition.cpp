// Copyright (c) Qinetik 2024.

#include "StructMember.h"
#include "StructDefinition.h"
#include "FunctionDeclaration.h"
#include "ast/types/LinkedType.h"
#include "ast/types/StructType.h"
#include "compiler/SymbolResolver.h"
#include "InterfaceDefinition.h"
#include "ast/structures/FunctionDeclaration.h"
#include "ast/structures/FunctionParam.h"
#include "ast/types/VoidType.h"
#include "ast/types/PointerType.h"
#include "ast/types/GenericType.h"
#include "UnnamedStruct.h"

#ifdef COMPILER_BUILD

#include "compiler/Codegen.h"
#include "compiler/llvmimpl.h"
#include "ast/values/IntValue.h"
#include "ast/types/LinkedType.h"

void StructDefinition::struct_func_gen(
    Codegen& gen,
    const std::vector<FunctionDeclaration*>& funcs,
    bool declare
) {
    if(declare) {
        for (auto& function: funcs) {
            if (function->is_override()) {

                // Do not declare the function because it overrides another function
                // when a function is being overridden which is already present in an interface
                // interface generates all declarations with entry blocks for it's users

                // BUT interface hasn't been tested to do this across modules

                continue;
            }
            function->code_gen_declare(gen, this);
        }
    } else {
        for (auto& function: funcs) {
            if (function->is_override()) {
                if (!llvm_override(gen, function)) {
                    gen.error("Failed to override the function", (AnnotableNode*) function);
                }
                continue;
            }
            function->activate_gen_call_iterations(function->active_iteration);
            function->code_gen_body(gen, this);
        }
    }
}

// tries to override the function present in interface
// returns true if current function should be skipped because it has been overridden
// or errored out
bool StructDefinition::llvm_override(Codegen& gen, FunctionDeclaration* function) {
    const auto info = get_func_overriding_info(function);
    if(info.base_container) {
        const auto interface = info.base_container->as_interface_def();
        // we always assume base container as interface, it could be something else (abstract struct maybe)
        if(interface->is_static()) {
            const auto inh_type = info.type->type;
            const auto prev_itr = interface->set_active_itr_ret_prev(inh_type->get_generic_iteration());
            const auto func = info.base_func->llvm_func();
            const auto interfaceModule = func->getParent();
            if(interfaceModule != gen.module.get()) {
                // interface is present in another module
                // we create a new function with strong linkage in this module
                const auto new_func = gen.create_function(func->getName().str(), func->getFunctionType(), AccessSpecifier::Public);
                function->set_llvm_data(new_func);
                function->code_gen_override(gen, new_func);
            } else {
                // internal interface, present in current module
                // we will implement the interface in place, since its present in current module
                function->set_llvm_data(func);
                if(func->size() == 1) {
                    // remove the stub block present in functions internal to module
                    auto& stubEntry = func->getEntryBlock();
                    stubEntry.removeFromParent();
                }
                const auto final_specifier = interface->specifier() == AccessSpecifier::Public || specifier() == AccessSpecifier::Public ? llvm::GlobalValue::ExternalLinkage : llvm::GlobalValue::PrivateLinkage;
                // change the function's linkage to internal
                func->setLinkage(final_specifier);
                gen.createFunctionBlock(func);
                function->code_gen_override(gen, func);
            }
            interface->set_active_iteration(prev_itr);
        } else {
            auto& user = interface->users[this];
            auto llvm_data = user.find(info.base_func);
            if (llvm_data == user.end()) {
                return false;
            }
            function->set_llvm_data(llvm_data->second);
            function->code_gen_override(gen, llvm_data->second);
        }
        return true;
    } else {
        return false;
    }
}

void StructDefinition::code_gen_function_declare(Codegen& gen, FunctionDeclaration* decl) {
    if(decl->is_override()) {
        return;
    }
    decl->code_gen_declare(gen, this);
}

void StructDefinition::code_gen_function_body(Codegen& gen, FunctionDeclaration* decl) {
    if(decl->is_override()) {
        if(!llvm_override(gen, decl)) {
            gen.error("Failed to override the function", (AnnotableNode*) decl);
        }
        return;
    }
    decl->code_gen_body(gen, this);
}

void StructDefinition::code_gen(Codegen &gen, bool declare) {
    if(is_comptime()) {
        return;
    }
    auto& itr_ptr = declare ? iterations_declared : iterations_body_done;
    if(generic_params.empty()) {
        if(itr_ptr == 0) {
            struct_func_gen(gen, functions(), declare);
            if (!declare) {
                for (auto& inherits: inherited) {
                    const auto interface = inherits.type->linked_interface_def();
                    if (interface && !interface->is_static()) {
                        interface->llvm_global_vtable(gen, this);
                    }
                }
            }
            itr_ptr++;
        }
    } else {
        const auto total = total_generic_iterations();
        if(total == 0) return; // generic type was never used
        auto prev_active_iteration = active_iteration;
        auto struct_itr = itr_ptr;
        while(struct_itr < total) {
            // generating code and copying iterations
            set_active_iteration(struct_itr);
            if(declare) {
                early_declare_structural_generic_args(gen);
            }
            struct_func_gen(gen, functions(), declare);
            if(declare) {
                acquire_function_iterations(struct_itr);
            }
            struct_itr++;
        }
        itr_ptr = struct_itr;
        set_active_iteration(prev_active_iteration);
    }
}

void StructDefinition::code_gen_generic(Codegen &gen) {
    code_gen_declare(gen);
    code_gen(gen);
}

void StructDefinition::code_gen_external_declare(Codegen &gen) {
    // clear the stored llvm types, so they must be declared again by StructType
    llvm_struct_types.clear();
    extendable_external_declare(gen);
}

llvm::Type* StructMember::llvm_type(Codegen &gen) {
    return type->llvm_type(gen);
}

llvm::Value* child_of_self_ptr(Codegen& gen, BaseDefMember& member, llvm::Value* self_ptr) {
    auto parent_struct = member.parent();
    std::vector<llvm::Value*> idxList { gen.builder->getInt32(0) };
    parent_struct->add_child_index(gen, idxList, member.name);
    return gen.builder->CreateGEP(member.parent()->llvm_type(gen), self_ptr, idxList, "", gen.inbounds);
}

llvm::Value* BaseDefMember::llvm_pointer(Codegen &gen) {
    if(isAnyStructMember(kind())) {
        const auto curr_func = gen.current_func_type->as_function();
        if(curr_func && curr_func->is_constructor_fn()) {
            // TODO hard coded the index for the constructor self param
            auto self_ptr = gen.current_function->getArg(0);
            return child_of_self_ptr(gen, *this, self_ptr);
        } else {
            auto self_param = curr_func->get_self_param();
            if(self_param) {
                auto self_ptr = gen.current_function->getArg(self_param->calculate_c_or_llvm_index());
                return child_of_self_ptr(gen, *this, self_ptr);
            }
        }
    }
#ifdef DEBUG
    throw std::runtime_error("called pointer on struct member, using an unknown self pointer");
#endif
    return nullptr;
}

llvm::Value* BaseDefMember::llvm_load(Codegen &gen) {
    auto pointer = llvm_pointer(gen);
    return Value::load_value(gen, known_type(), llvm_type(gen), pointer);
}

llvm::Type* StructMember::llvm_chain_type(Codegen &gen, std::vector<ChainValue*> &values, unsigned int index) {
    return type->llvm_chain_type(gen, values, index);
}

bool StructMember::add_child_index(Codegen &gen, std::vector<llvm::Value *> &indexes, const chem::string_view &childName) {
    auto linked = type->linked_node();
    if (!linked) return false;
    linked->add_child_index(gen, indexes, childName);
    return true;
}

void StructDefinition::llvm_destruct(Codegen &gen, llvm::Value *allocaInst) {
    auto func = destructor_func();
    if(func) {
        std::vector<llvm::Value*> args;
        if(func->has_self_param()) {
            args.emplace_back(allocaInst);
        }
        llvm::Function* func_data;
        if(is_generic()) {
            func_data = llvm_generic_func_data(func, active_iteration, func->active_iteration);
        } else {
            func_data = func->llvm_func();
        }
        gen.builder->CreateCall(func_data, args);
    }
}

llvm::StructType* StructDefinition::llvm_stored_type() {
    return llvm_struct_types[active_iteration];
}

void StructDefinition::llvm_store_type(llvm::StructType* type) {
    llvm_struct_types[active_iteration] = type;
}

llvm::Type* StructDefinition::with_elements_type(
        Codegen &gen,
        const std::vector<llvm::Type *>& elements,
        const std::string& runtime_name
) {
    if(runtime_name.empty()) {
        return llvm::StructType::get(*gen.ctx, elements);
    }
    auto stored = llvm_stored_type();
    if(!stored) {
        auto new_stored = llvm::StructType::create(*gen.ctx, elements, runtime_name);
        llvm_store_type(new_stored);
        return new_stored;
    }
    return stored;
}

llvm::Type *StructDefinition::llvm_type(Codegen &gen) {
    return with_elements_type(gen, elements_type(gen), get_runtime_name());
}

llvm::Type *StructDefinition::llvm_param_type(Codegen &gen) {
    return gen.builder->getPtrTy();
}

llvm::Type *StructDefinition::llvm_chain_type(Codegen &gen, std::vector<ChainValue*> &values, unsigned int index) {
    return with_elements_type(gen, elements_type(gen, values, index), "");
}

llvm::Type* UnnamedStruct::llvm_type(Codegen &gen) {
    return llvm::StructType::get(*gen.ctx, elements_type(gen));
}

llvm::Type* UnnamedStruct::llvm_chain_type(Codegen &gen, std::vector<ChainValue*> &values, unsigned int index) {
    return llvm::StructType::get(*gen.ctx, elements_type(gen, values, index));
}

#endif

void StructMember::accept(Visitor *visitor) {
    visitor->visit(this);
}

BaseType* StructMember::create_value_type(ASTAllocator& allocator) {
    return type->copy(allocator);
}

BaseDefMember *StructMember::copy_member(ASTAllocator& allocator) {
    Value* def_value = defValue ? defValue->copy(allocator) : nullptr;
    return new (allocator.allocate<StructMember>()) StructMember(name, type->copy(allocator), def_value, parent_node, encoded_location());
}

void StructMember::declare_top_level(SymbolResolver &linker, ASTNode*& node_ptr) {
    linker.declare(name, this);
}

void StructMember::declare_and_link(SymbolResolver &linker, ASTNode*& node_ptr) {
    linker.declare(name, this);
    type->link(linker);
    if (defValue) {
        defValue->link(linker, defValue);
    }
}

void UnnamedStruct::redeclare_top_level(SymbolResolver &linker) {
    linker.declare(name, this);
}

void UnnamedStruct::declare_and_link(SymbolResolver &linker, ASTNode*& node_ptr) {
    linker.scope_start();
    VariablesContainer::declare_and_link(linker, node_ptr);
    linker.scope_end();
    linker.declare(name, this);
}

BaseDefMember *UnnamedStruct::copy_member(ASTAllocator& allocator) {
    auto unnamed = new (allocator.allocate<UnnamedStruct>()) UnnamedStruct(name, parent_node, encoded_location());
    for(auto& variable : variables) {
        unnamed->variables[variable.first] = variable.second->copy_member(allocator);
    }
    return unnamed;
}

VariablesContainer *UnnamedStruct::copy_container(ASTAllocator& allocator) {
    return (VariablesContainer*) copy_member(allocator);
}

ASTNode *StructMember::child(const chem::string_view &childName) {
    auto linked = type->linked_node();
    if (!linked) return nullptr;
    return linked->child(childName);
}

BaseTypeKind StructMember::type_kind() const {
    return type->kind();
}

//BaseType *StructDefinition::copy(ASTAllocator& allocator) const {
//    return new (allocator.allocate<LinkedType>()) LinkedType(name_view(), (ASTNode *) this, location);
//}

BaseType* UnnamedStruct::create_value_type(ASTAllocator &allocator) {
    return new (allocator.allocate<LinkedType>()) LinkedType(name, (ASTNode *) this, encoded_location());
}

//BaseType *UnnamedStruct::copy(ASTAllocator& allocator) const {
//    // this is UnionType's copy method
//    return new (allocator.allocate<LinkedType>()) LinkedType(name, (ASTNode *) this, location);
//}

void StructDefinition::accept(Visitor *visitor) {
    visitor->visit(this);
}

void StructDefinition::declare_top_level(SymbolResolver &linker, ASTNode*& node_ptr) {
    linker.declare_node(name_view(), this, specifier(), true);
}

void StructDefinition::redeclare_top_level(SymbolResolver &linker) {
    linker.declare(name_view(), this);
}

void StructDefinition::link_signature(SymbolResolver &linker) {
    MembersContainer::link_signature(linker);
}

void StructDefinition::declare_and_link(SymbolResolver &linker, ASTNode*& node_ptr) {
    auto& allocator = specifier() == AccessSpecifier::Public ? *linker.ast_allocator : *linker.mod_allocator;
    bool has_def_constructor = false;
    bool has_destructor = false;
    bool has_clear_fn = false;
    bool has_copy_fn = false;
    bool has_move_fn = false;
    for(auto& func : functions()) {
        if(func->is_constructor_fn()) {
            if(!func->has_explicit_params()) {
                has_def_constructor = true;
            }
            func->ensure_constructor(linker, this);
        }
        if(func->is_delete_fn()) {
            func->ensure_destructor(linker, this);
            has_destructor = true;
        }
        if(func->is_post_move_fn()) {
            func->ensure_clear_fn(linker, this);
            has_clear_fn = true;
        }
        if(func->is_move_fn()) {
            func->ensure_move_fn(linker, this);
            has_move_fn = true;
        }
        if(func->is_copy_fn()) {
            func->ensure_copy_fn(linker, this);
            has_copy_fn = true;
        }
    }
    MembersContainer::declare_and_link(linker, node_ptr);
    register_use_to_inherited_interfaces(this);
    if(!has_def_constructor && any_member_has_def_constructor()) {
        create_def_constructor_checking(allocator, linker, name_view());
    }
    if(!has_copy_fn && any_member_has_copy_func()) {
        create_def_copy_fn(allocator, linker);
    }
    if(!has_clear_fn && any_member_has_clear_func()) {
        create_def_clear_fn(allocator, linker);
    }
    if(!has_move_fn && any_member_has_pre_move_func()) {
        create_def_move_fn(allocator, linker);
    }
    if(!has_destructor && any_member_has_destructor()) {
        create_def_destructor(allocator, linker);
    }
//    if(init_values_req_size() != 0) {
//        for (auto& func: functions()) {
//            if (func->has_annotation(AnnotationKind::Constructor) && !func->has_annotation(AnnotationKind::CompTime)) {
//                func->ensure_has_init_block(linker);
//            }
//        }
//    }
}

ASTNode *StructDefinition::child(const chem::string_view &name) {
    auto node = ExtendableMembersContainerNode::child(name);
    if (node) {
        return node;
    } else if (!inherited.empty()) {
        for(auto& inherits : inherited) {
            if(inherits.specifier == AccessSpecifier::Public) {
                const auto thing = inherits.type->linked_node()->child(name);
                if (thing) return thing;
            }
        }
    };
    return nullptr;
}

VariablesContainer *StructDefinition::copy_container(ASTAllocator& allocator) {
    auto def = new (allocator.allocate<StructDefinition>()) StructDefinition(identifier, parent_node, encoded_location());
    for(auto& inherits : inherited) {
        def->inherited.emplace_back(inherits.copy(allocator));
    }
    for(auto& variable : variables) {
        def->variables[variable.first] = variable.second->copy_member(allocator);
    }
    return def;
}

BaseType* StructDefinition::create_value_type(ASTAllocator& allocator) {
    return create_linked_type(name_view(), allocator);
}

BaseType* StructDefinition::known_type() {
    return &linked_type;
}