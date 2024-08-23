// Copyright (c) Qinetik 2024.

#include "StructMember.h"
#include "StructDefinition.h"
#include "FunctionDeclaration.h"
#include "ast/types/ReferencedType.h"
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
#include "ast/types/ReferencedType.h"

void StructDefinition::struct_func_gen(Codegen& gen, const std::vector<std::unique_ptr<FunctionDeclaration>>& funcs) {
    for(auto& function : funcs) {
        if(function->has_annotation(AnnotationKind::Override)) {
//            auto overriding = get_overriding(function.get());
//            if (overriding) {
//                function->code_gen_override_declare(gen, overriding);
//            } else {
//                gen.error("Failed to override (declare) the function", function.get());
//            }
            continue;
        }
        function->code_gen_declare(gen, this);
    }
    for (auto &function: funcs) {
        if(function->has_annotation(AnnotationKind::Override)) {
            if(!llvm_override(gen, function.get())) {
                gen.error("Failed to override the function", function.get());
            }
            continue;
        }
        function->code_gen_body(gen, this);
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

class AutoReleasedFunctionsContainer {
public:
    std::vector<std::unique_ptr<FunctionDeclaration>> functions;
    ~AutoReleasedFunctionsContainer() {
        for(auto& func : functions) {
            func.release();
        }
    }
};

void StructDefinition::code_gen(Codegen &gen) {
    if(generic_params.empty()) {
        struct_func_gen(gen, functions());
        for(auto& inherits : inherited) {
            const auto interface = inherits->type->linked_interface_def();
            if(interface) {
                interface->llvm_global_vtable(gen, this);
            }
        }
    } else {
        // WHY IS THIS ALGORITHM SO COMPLICATED ?
        // because we must check which generic iteration has already been generated
        // and skip generating for functions for that generic iteration
        const auto total = total_generic_iterations();
        if(total == 0) return; // generic type was never used
        auto prev_active_iteration = active_iteration;
        int16_t struct_itr = 0;
        while(struct_itr < total) {
            AutoReleasedFunctionsContainer container;
            for (auto &function: functions()) {
                auto &func_data = generic_llvm_data[function.get()]; // <--- automatic insertion
                if (struct_itr == func_data.size()) {
                    container.functions.emplace_back(function.get());
                    func_data.emplace_back();
                } else if (struct_itr < func_data.size()) {
                    auto &func_iters = func_data[struct_itr];
                    if (func_iters.empty() || func_iters.size() < function->total_generic_iterations()) {
                        container.functions.emplace_back(function.get());
                    }
                } else {
#ifdef DEBUG
                    throw std::runtime_error("expected struct iteration to be smaller than initialized iterations");
#endif
                }
            }
            // generating code and copying iterations
            if(!container.functions.empty()) {
                set_active_iteration(struct_itr);
                struct_func_gen(gen, container.functions);
                acquire_function_iterations(struct_itr);
            }
            struct_itr++;
        }
        set_active_iteration(prev_active_iteration);
    }
}

void StructDefinition::code_gen_generic(Codegen &gen) {
    code_gen(gen);
}

void StructDefinition::acquire_function_iterations(int16_t iteration) {
    for(auto& function : functions()) {
        auto& func_data = generic_llvm_data[function.get()];
        func_data[iteration] = function->llvm_data;
    }
}

llvm::Type *StructMember::llvm_type(Codegen &gen) {
    return type->llvm_type(gen);
}

llvm::Type *StructMember::llvm_chain_type(Codegen &gen, std::vector<std::unique_ptr<ChainValue>> &values, unsigned int index) {
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

llvm::Type *StructDefinition::llvm_type(Codegen &gen, int16_t iteration) {
    auto prev = active_iteration;
    set_active_iteration(iteration);
    auto type = llvm_type(gen);
    set_active_iteration(prev);
    return type;
}

llvm::Type *StructDefinition::llvm_param_type(Codegen &gen) {
    return StructType::llvm_param_type(gen);
}

llvm::Type *StructDefinition::llvm_chain_type(Codegen &gen, std::vector<std::unique_ptr<ChainValue>> &values, unsigned int index) {
    return StructType::llvm_chain_type(gen, values, index);
}

#endif

BaseDefMember::BaseDefMember(std::string name) : name(std::move(name)) {

}

StructMember::StructMember(
        std::string name,
        std::unique_ptr<BaseType> type,
        std::optional<std::unique_ptr<Value>> defValue,
        ASTNode* parent_node
) : BaseDefMember(std::move(name)), type(std::move(type)), defValue(std::move(defValue)), parent_node(parent_node) {

}

void StructMember::accept(Visitor *visitor) {
    visitor->visit(this);
}

std::unique_ptr<BaseType> StructMember::create_value_type() {
    return std::unique_ptr<BaseType>(type->copy());
}

hybrid_ptr<BaseType> StructMember::get_value_type() {
    return hybrid_ptr<BaseType> { type.get(), false };
}

BaseDefMember *StructMember::copy_member() {
    std::optional<std::unique_ptr<Value>> def_value = std::nullopt;
    if(defValue.has_value()) {
        def_value.emplace(defValue.value()->copy());
    }
    return new StructMember(name, std::unique_ptr<BaseType>(type->copy()), std::move(def_value), parent_node);
}

void StructMember::declare_top_level(SymbolResolver &linker) {
    linker.declare(name, this);
}

void StructMember::declare_and_link(SymbolResolver &linker) {
    linker.declare(name, this);
    type->link(linker, type);
    if (defValue.has_value()) {
        defValue.value()->link(linker, defValue.value());
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

BaseDefMember *UnnamedStruct::copy_member() {
    auto unnamed = new UnnamedStruct(name, parent_node);
    for(auto& variable : variables) {
        unnamed->variables[variable.first] = std::unique_ptr<BaseDefMember>(variable.second->copy_member());
    }
    return unnamed;
}

VariablesContainer *UnnamedStruct::copy_container() {
    return (VariablesContainer*) copy_member();
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
        ASTNode* parent_node
) : BaseDefMember(std::move(name)), parent_node(parent_node) {

}

StructDefinition::StructDefinition(
        std::string name,
        ASTNode* parent_node
) : ExtendableMembersContainerNode(std::move(name)), parent_node(parent_node) {}

BaseType *StructDefinition::copy() const {
    return new ReferencedType(name, (ASTNode *) this);
}

BaseType *UnnamedStruct::copy() const {
    return new ReferencedType(name, (ASTNode *) this);
}

bool StructMember::requires_destructor() {
    return type->requires_destructor();
}

void StructDefinition::accept(Visitor *visitor) {
    visitor->visit(this);
}

void StructDefinition::declare_top_level(SymbolResolver &linker) {
    linker.declare(name, this);
    is_direct_init = has_annotation(AnnotationKind::DirectInit);
}

void StructDefinition::declare_and_link(SymbolResolver &linker) {
    bool has_destructor = false;
    for(auto& func : functions()) {
        if(func->has_annotation(AnnotationKind::Constructor)) {
            func->ensure_constructor(this);
        }
        if(func->has_annotation(AnnotationKind::Destructor)) {
            has_destructor = true;
        }
    }
    MembersContainer::declare_and_link(linker);
    register_use_to_inherited_interfaces(this);
    if(!has_destructor && requires_destructor()) {
        if(contains_func("delete")) {
            linker.error("default destructor is created by name 'delete' , a function by name 'delete' already exists in struct '" + name + "', please create a destructor by hand if you'd like to reserve 'delete' for your own usage");
            return;
        }
        create_destructor();
    }
}

StructDefinition *StructDefinition::as_struct_def() {
    return this;
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

VariablesContainer *StructDefinition::copy_container() {
    auto def = new StructDefinition(name, parent_node);
    for(auto& inherits : inherited) {
        def->inherited.emplace_back(inherits->copy());
    }
    for(auto& variable : variables) {
        def->variables[variable.first] = std::unique_ptr<BaseDefMember>(variable.second->copy_member());
    }
    return def;
}

std::unique_ptr<BaseType> StructDefinition::create_value_type() {
    return std::make_unique<ReferencedType>(name, this);
}

hybrid_ptr<BaseType> StructDefinition::get_value_type() {
    return hybrid_ptr<BaseType> { this, false };
}

BaseType* StructDefinition::known_type() {
    return this;
}

hybrid_ptr<BaseType> UnnamedStruct::get_value_type() {
    return hybrid_ptr<BaseType> { this, false };
}

ValueType StructDefinition::value_type() const {
    return ValueType::Struct;
}