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
        auto overriding = get_overriding(function.get());
        if(overriding) {
            function->code_gen_override_declare(gen, overriding);
            continue;
        }
        function->code_gen_declare(gen, this);
    }
    for (auto &function: funcs) {
        if(llvm_override(gen, function.get())) {
            continue;
        }
        function->code_gen_body(gen, this);
    }
}

// tries to override the function present in interface
// returns true if current function should be skipped because it has been overridden
// or errored out
bool StructDefinition::llvm_override(Codegen& gen, FunctionDeclaration* function) {
    for(auto& inherits : inherited) {
        const auto interface = inherits->linked->as_interface_def();
        if(interface) {
            auto inter_itr = gen.unimplemented_interfaces.find(inherits->type);
            if (inter_itr == gen.unimplemented_interfaces.end()) {
                gen.error("Couldn't find overridden interface with name '" + inherits->type +
                          "' for implementation");
                return false;
            }
            auto& ref = inter_itr->second;
            auto overridden = interface->child(function->name);
            if (overridden) {
                auto found = ref.find(function->name);
                if (found == ref.end()) {
                    gen.error("Couldn't find function with name " + function->name + " in interface " + inherits->type + " for implementation");
                    return true;
                }
                if (found->second == nullptr) {
                    gen.error("Function with name " + function->name + " in interface " + inherits->type + " has already been implemented");
                    return true;
                }
                auto fn = overridden->as_function();
                if (fn) {
                    function->code_gen_override(gen, fn);
                    found->second = nullptr;
                    return true;
                } else {
                    gen.error("Function being overridden with name " + function->name + " in interface is not a function");
                }
            }
        }
    }
    return false;
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
    } else {
        // WHY IS THIS ALGORITHM SO COMPLICATED ?
        // because we must check which generic iteration has already been generated
        // and skip generating for functions for that generic iteration
        const auto total = total_generic_iterations();
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

void StructMember::declare_and_link(SymbolResolver &linker) {
    linker.declare(name, this);
    type->link(linker, type);
    if(type->kind() == BaseTypeKind::Generic) {
        ((GenericType*) type.get())->report_generic_usage();
    }
    if (defValue.has_value()) {
        defValue.value()->link(linker, defValue.value());
    }
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
    return type->value_type() == ValueType::Struct && type->linked_node()->as_struct_def()->requires_destructor();
}

// returns the struct/interface & function that is being overridden by given function in the parameter
std::pair<ASTNode*, FunctionDeclaration*> StructDefinition::get_overriding_info(FunctionDeclaration* function) {
    if(inherited.empty()) return { nullptr, nullptr };
    for(auto& inherits : inherited) {
        const auto interface = inherits->linked->as_interface_def();
        if(interface) {
            const auto child_func = interface->child_function(function->name);
            if(child_func) {
                return { interface, child_func };
            } else {
                continue;
            }
        }
        const auto struct_def = inherits->linked->as_struct_def();
        if(struct_def) {
            const auto child_func = struct_def->child_function(function->name);
            if(child_func) {
                return {struct_def, child_func};
            } else {
                const auto info = struct_def->get_overriding_info(function);
                if(info.first) {
                    return info;
                }
            }
        }
    }
    return { nullptr, nullptr };
}

// returns the function that is being overridden by given function in the parameter
FunctionDeclaration* StructDefinition::get_overriding(FunctionDeclaration* function) {
    return get_overriding_info(function).second;
};

std::pair<InterfaceDefinition*, FunctionDeclaration*> StructDefinition::get_interface_overriding_info(FunctionDeclaration* function) {
    const auto info = get_overriding_info(function);
    const auto interface = info.first ? info.first->as_interface_def() : nullptr;
    if(interface) {
        return { interface, info.second };
    } else {
        return { nullptr, nullptr };
    }
}

InterfaceDefinition* StructDefinition::get_overriding_interface(FunctionDeclaration* function) {
    const auto info = get_overriding_info(function);
    return info.first ? info.first->as_interface_def() : nullptr;
}

FunctionDeclaration* StructDefinition::create_destructor() {
    auto decl = new FunctionDeclaration("delete", {}, std::make_unique<VoidType>(), false, this, std::nullopt);
    decl->params.emplace_back(new FunctionParam("self", std::make_unique<PointerType>(std::make_unique<ReferencedType>(name, this)), 0, std::nullopt, decl));
    decl->body.emplace(LoopScope{nullptr});
    decl->annotations.emplace_back(AnnotationKind::Destructor);
    insert_func(std::unique_ptr<FunctionDeclaration>(decl));
    return decl;
}

void StructDefinition::accept(Visitor *visitor) {
    visitor->visit(this);
}

void StructDefinition::declare_top_level(SymbolResolver &linker) {
    for(auto& inherits : inherited) {
        inherits->link(linker, (std::unique_ptr<BaseType>&) inherits);
    }
    linker.declare(name, this);
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
            const auto thing = inherits->linked->child(name);
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

hybrid_ptr<BaseType> UnnamedStruct::get_value_type() {
    return hybrid_ptr<BaseType> { this, false };
}

ValueType StructDefinition::value_type() const {
    return ValueType::Struct;
}