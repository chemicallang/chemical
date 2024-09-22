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
            if (function->has_annotation(AnnotationKind::Override)) {

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
            if (function->has_annotation(AnnotationKind::Override)) {
                if (!llvm_override(gen, function)) {
                    gen.error("Failed to override the function", (AnnotableNode*) function);
                }
                continue;
            }
            function->code_gen_body(gen, this);
        }
    }
}

// tries to override the function present in interface
// returns true if current function should be skipped because it has been overridden
// or errored out
bool StructDefinition::llvm_override(Codegen& gen, FunctionDeclaration* function) {
    const auto info = get_overriding_info(function);
    if(info.first) {
        const auto interface = info.first->as_interface_def();
        auto& user = interface->users[this];
        auto llvm_data = user.find(info.second);
        if(llvm_data == user.end()) {
            return false;
        }
        function->set_llvm_data(llvm_data->second, llvm_data->second->getFunctionType());
        function->code_gen_override(gen, llvm_data->second);
        return true;
    } else {
        return false;
    }
}

void StructDefinition::code_gen_function_declare(Codegen& gen, FunctionDeclaration* decl) {
    if(decl->has_annotation(AnnotationKind::Override)) {
        return;
    }
    decl->code_gen_declare(gen, this);
}

void StructDefinition::code_gen_function_body(Codegen& gen, FunctionDeclaration* decl) {
    if(decl->has_annotation(AnnotationKind::Override)) {
        if(!llvm_override(gen, decl)) {
            gen.error("Failed to override the function", (AnnotableNode*) decl);
        }
        return;
    }
    decl->code_gen_body(gen, this);
}

void StructDefinition::code_gen(Codegen &gen, bool declare) {
    if(has_annotation(AnnotationKind::CompTime)) {
        return;
    }
    auto& itr_ptr = declare ? iterations_declared : iterations_body_done;
    if(generic_params.empty()) {
        if(itr_ptr == 0) {
            struct_func_gen(gen, functions(), declare);
            if (!declare) {
                for (auto& inherits: inherited) {
                    const auto interface = inherits->type->linked_interface_def();
                    if (interface) {
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
    if(generic_params.empty()) {
        for (auto& function: functions()) {
            function->code_gen_external_declare(gen);
        }
    } else {
        if(functions().empty()) return;
        // get the total number of iterations that already exist for each function
        const auto total_existing_itr = generic_llvm_data[functions().front()].size();
        // clear the existing iteration's llvm_data since that's out of module
        for(auto& func : functions()) {
            generic_llvm_data[func].clear();
        }
        int16_t i = 0;
        const auto prev_active_iteration = active_iteration;
        while(i < total_existing_itr) {
            set_active_iteration(i);
            // declare all the functions at iteration
            for (auto& function: functions()) {
                function->code_gen_external_declare(gen);
            }
            // acquire functions llvm data at iteration
            acquire_function_iterations(i);
            i++;
        }
        set_active_iteration_safely(prev_active_iteration);
        // calling code_gen, this will generate any missing generic iterations
        code_gen_declare(gen);
        code_gen(gen);
    }
}

llvm::Type* StructMember::llvm_type(Codegen &gen) {
    return type->llvm_type(gen);
}

llvm::Value* BaseDefMember::llvm_pointer(Codegen &gen) {
    if(isAnyStructMember()) {
        const auto curr_func = gen.current_func_type->as_function();
        if(curr_func && curr_func->has_annotation(AnnotationKind::Constructor)) {
            // TODO hard coded the index for the constructor self param
            auto self_ptr = gen.current_function->getArg(0);
            auto parent_struct = parent();
            std::vector<llvm::Value*> idxList { gen.builder->getInt32(0) };
            parent_struct->add_child_index(gen, idxList, name);
            return gen.builder->CreateGEP(parent()->llvm_type(gen), self_ptr, idxList, "", gen.inbounds);
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

llvm::FunctionType *StructMember::llvm_func_type(Codegen &gen) {
    return type->llvm_func_type(gen);
}

bool StructMember::add_child_index(Codegen &gen, std::vector<llvm::Value *> &indexes, const std::string &childName) {
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
        llvm::FunctionType* func_type;
        llvm::Value* func_callee;
        if(is_generic()) {
            const auto data = llvm_generic_func_data(func, active_iteration, func->active_iteration);
            func_type = data.second;
            func_callee = data.first;
        } else {
            func_type = func->llvm_func_type(gen);
            func_callee = func->llvm_pointer(gen);
        }
        gen.builder->CreateCall(func_type, func_callee, args);
    }
}

llvm::StructType* StructDefinition::llvm_stored_type() {
    return llvm_struct_types[active_iteration];
}

void StructDefinition::llvm_store_type(llvm::StructType* type) {
    llvm_struct_types[active_iteration] = type;
}

llvm::Type *StructDefinition::llvm_type(Codegen &gen) {
    return StructType::llvm_type(gen);
}

llvm::Type *StructDefinition::llvm_param_type(Codegen &gen) {
    return StructType::llvm_param_type(gen);
}

llvm::Type *StructDefinition::llvm_chain_type(Codegen &gen, std::vector<ChainValue*> &values, unsigned int index) {
    return StructType::llvm_chain_type(gen, values, index);
}

#endif

BaseDefMember::BaseDefMember(std::string name) : name(std::move(name)) {

}

StructMember::StructMember(
        std::string name,
        BaseType* type,
        Value* defValue,
        ASTNode* parent_node,
        CSTToken* token,
        bool is_const,
        AccessSpecifier specifier
) : BaseDefMember(std::move(name)), type(type), defValue(defValue), parent_node(parent_node), token(token), is_const(is_const), specifier(specifier) {

}

void StructMember::accept(Visitor *visitor) {
    visitor->visit(this);
}

BaseType* StructMember::create_value_type(ASTAllocator& allocator) {
    return type->copy(allocator);
}

BaseDefMember *StructMember::copy_member(ASTAllocator& allocator) {
    Value* def_value = defValue ? defValue->copy(allocator) : nullptr;
    return new (allocator.allocate<StructMember>()) StructMember(name, type->copy(allocator), def_value, parent_node, token);
}

void StructMember::declare_top_level(SymbolResolver &linker) {
    linker.declare(name, this);
}

void StructMember::declare_and_link(SymbolResolver &linker) {
    linker.declare(name, this);
    type->link(linker);
    if (defValue) {
        defValue->link(linker, defValue);
    }
}

void UnnamedStruct::redeclare_top_level(SymbolResolver &linker) {
    linker.declare(name, this);
}

void UnnamedStruct::declare_and_link(SymbolResolver &linker) {
    linker.scope_start();
    VariablesContainer::declare_and_link(linker);
    linker.scope_end();
    linker.declare(name, this);
}

BaseDefMember *UnnamedStruct::copy_member(ASTAllocator& allocator) {
    auto unnamed = new (allocator.allocate<UnnamedStruct>()) UnnamedStruct(name, parent_node, token);
    for(auto& variable : variables) {
        unnamed->variables[variable.first] = variable.second->copy_member(allocator);
    }
    return unnamed;
}

VariablesContainer *UnnamedStruct::copy_container(ASTAllocator& allocator) {
    return (VariablesContainer*) copy_member(allocator);
}

ASTNode *StructMember::child(const std::string &childName) {
    auto linked = type->linked_node();
    if (!linked) return nullptr;
    return linked->child(childName);
}

ValueType StructMember::value_type() const {
    return type->value_type();
}

BaseTypeKind StructMember::type_kind() const {
    return type->kind();
}

UnnamedStruct::UnnamedStruct(
        std::string name,
        ASTNode* parent_node,
        CSTToken* token,
        AccessSpecifier specifier
) : BaseDefMember(std::move(name)), parent_node(parent_node), token(token), specifier(specifier) {

}

StructDefinition::StructDefinition(
        std::string name,
        ASTNode* parent_node,
        CSTToken* token,
        AccessSpecifier specifier
) : ExtendableMembersContainerNode(std::move(name)), parent_node(parent_node),
    token(token), specifier(specifier), linked_type("", nullptr) {

}

BaseType *StructDefinition::copy(ASTAllocator& allocator) const {
    return new (allocator.allocate<LinkedType>()) LinkedType(name, (ASTNode *) this, token);
}

BaseType* UnnamedStruct::create_value_type(ASTAllocator &allocator) {
    return new (allocator.allocate<LinkedType>()) LinkedType(name, (ASTNode *) this, token);
}

BaseType *UnnamedStruct::copy(ASTAllocator& allocator) const {
    // this is UnionType's copy method
    return new (allocator.allocate<LinkedType>()) LinkedType(name, (ASTNode *) this, token);
}

void StructDefinition::accept(Visitor *visitor) {
    visitor->visit(this);
}

void StructDefinition::declare_top_level(SymbolResolver &linker) {
    linked_type.type = name;
    linked_type.linked = this;
    linker.declare_node(name, this, specifier, true);
    is_direct_init = has_annotation(AnnotationKind::DirectInit);
}

void StructDefinition::declare_and_link(SymbolResolver &linker) {
    auto& allocator = linker.allocator;
    bool has_destructor = false;
    bool has_clear_fn = false;
    bool has_copy_fn = false;
    bool has_move_fn = false;
    for(auto& func : functions()) {
        if(func->has_annotation(AnnotationKind::Constructor)) {
            func->ensure_constructor(allocator, this);
        }
        if(func->has_annotation(AnnotationKind::Delete)) {
            func->ensure_destructor(allocator, this);
            has_destructor = true;
        }
        if(func->has_annotation(AnnotationKind::Clear)) {
            func->ensure_clear_fn(allocator, this);
            has_clear_fn = true;
        }
        if(func->has_annotation(AnnotationKind::Move)) {
            func->ensure_move_fn(allocator, this);
            has_move_fn = true;
        }
        if(func->has_annotation(AnnotationKind::Copy)) {
            func->ensure_copy_fn(allocator, this);
            has_copy_fn = true;
        }
    }
    MembersContainer::declare_and_link(linker);
    register_use_to_inherited_interfaces(this);
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

ASTNode *StructDefinition::child(const std::string &name) {
    auto node = ExtendableMembersContainerNode::child(name);
    if (node) {
        return node;
    } else if (!inherited.empty()) {
        for(auto& inherits : inherited) {
            const auto thing = inherits->type->linked_node()->child(name);
            if(thing) return thing;
        }
    };
    return nullptr;
}

VariablesContainer *StructDefinition::copy_container(ASTAllocator& allocator) {
    auto def = new (allocator.allocate<StructDefinition>()) StructDefinition(name, parent_node, token);
    for(auto& inherits : inherited) {
        def->inherited.emplace_back(inherits->copy(allocator));
    }
    for(auto& variable : variables) {
        def->variables[variable.first] = variable.second->copy_member(allocator);
    }
    return def;
}

BaseType* StructDefinition::create_value_type(ASTAllocator& allocator) {
    return &linked_type;
}

BaseType* StructDefinition::known_type() {
    return &linked_type;
}

ValueType StructDefinition::value_type() const {
    return ValueType::Struct;
}