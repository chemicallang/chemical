// Copyright (c) Qinetik 2024.

#include "VariantDefinition.h"
#include "VariantMember.h"
#include "ast/types/ReferencedType.h"
#include "compiler/SymbolResolver.h"
#include "ast/base/ChainValue.h"

#ifdef COMPILER_BUILD

#include "compiler/Codegen.h"
#include "compiler/llvmimpl.h"

llvm::Type* VariantDefinition::llvm_type(Codegen &gen) {
    auto found = llvm_struct_types.find(active_iteration);
    if(found != llvm_struct_types.end()) {
        return found->second;
    }
    std::vector<llvm::Type*> elements;
    elements.emplace_back(gen.builder->getInt32Ty()); // <--- int enum is stored at top, so we know the type
    const auto large = largest_member();
    if(large) {
        std::vector<llvm::Type*> sub_elements { large->llvm_type(gen) };
        elements.emplace_back(llvm::StructType::get(*gen.ctx, sub_elements));
    }
    const auto type = llvm::StructType::create(*gen.ctx, elements, name);
    llvm_struct_types[active_iteration] = type;
    return type;
}

llvm::Type *VariantDefinition::llvm_type(Codegen &gen, int16_t iteration) {
    auto prev = active_iteration;
    set_active_iteration(iteration);
    auto type = llvm_type(gen);
    set_active_iteration(prev);
    return type;
}

llvm::Type* VariantDefinition::llvm_param_type(Codegen &gen) {
    return gen.builder->getPtrTy();
}

llvm::Type* VariantDefinition::llvm_chain_type(Codegen &gen, std::vector<std::unique_ptr<ChainValue>> &values, unsigned int index) {
    auto member = variables.begin() + index;
    if(index + 1 < values.size()) {
        auto linked = values[index + 1]->linked_node();
        if(linked && member->second.get() == linked) {
            std::vector<llvm::Type *> struct_type{member->second->llvm_chain_type(gen, values, index + 1)};
            return llvm::StructType::get(*gen.ctx, struct_type);
        }
    }
    return llvm_type(gen);
}

void VariantDefinition::code_gen(Codegen &gen) {
    llvm_type(gen);
}

bool VariantDefinition::add_child_index(Codegen &gen, std::vector<llvm::Value *> &indexes, const std::string &name) {
    if(indexes.empty()) {
        indexes.emplace_back(gen.builder->getInt32(0));
    }
    indexes.emplace_back(gen.builder->getInt32(1));
    return llvm_union_child_index(gen, indexes, name);
}

void VariantDefinition::code_gen_generic(Codegen &gen) {

}

void VariantDefinition::llvm_destruct(Codegen &gen, llvm::Value *allocaInst) {

}

bool VariantMember::add_child_index(Codegen &gen, std::vector<llvm::Value *> &indexes, const std::string &name) {
    const auto found = values.find(name);
    if(found == values.end()) {
        return false;
    }
    const auto index = found - values.begin();
    indexes.emplace_back(gen.builder->getInt32(index));
}

llvm::Type *VariantMember::llvm_type(Codegen &gen) {
    std::vector<llvm::Type*> elements;
    for(auto& value : values) {
        elements.emplace_back(value.second->llvm_type(gen));
    }
    return llvm::StructType::get(*gen.ctx, elements);
}

llvm::Type* VariantMemberParam::llvm_type(Codegen &gen) {
    return type->llvm_type(gen);
}

llvm::FunctionType* VariantMemberParam::llvm_func_type(Codegen &gen) {
    return type->llvm_func_type(gen);
}

#endif

VariantDefinition::VariantDefinition(
    std::string name,
    ASTNode* parent_node
) : ExtendableMembersContainerNode(std::move(name)), parent_node(parent_node), ref_type(name, this) {
}

ASTNode* VariantDefinition::child(const std::string &child_name) {
    auto found = variables.find(child_name);
    if(found != variables.end()) {
        return found->second.get();
    }
    return nullptr;
}

void VariantDefinition::declare_top_level(SymbolResolver &linker) {
    linker.declare(name, this);
}

void VariantDefinition::declare_and_link(SymbolResolver &linker) {
    MembersContainer::declare_and_link(linker);
//    register_use_to_inherited_interfaces(this);
    if(requires_destructor()) {
        if(contains_func("delete")) {
            linker.error("default destructor is created by name 'delete' , a function by name 'delete' already exists in struct '" + name + "', please create a destructor by hand if you'd like to reserve 'delete' for your own usage");
            return;
        }
        create_destructor();
    }
}

BaseType* VariantDefinition::known_type() {
    return &ref_type;
}

[[nodiscard]]
ValueType VariantDefinition::value_type() const {
    return ValueType::Struct;
}

uint64_t VariantDefinition::byte_size(bool is64Bit) {
    const auto type_size = is64Bit ? 32 : 16; // <--- an int type enum is stored inside
    const auto large = largest_member();
    if(!large) return type_size;
    return large->byte_size(is64Bit) + type_size;
}

std::unique_ptr<BaseType> VariantDefinition::create_value_type() {
    return std::make_unique<ReferencedType>(name, this);
}

hybrid_ptr<BaseType> VariantDefinition::get_value_type() {
    return hybrid_ptr<BaseType> { create_value_type().release(), true };
}


VariantMember::VariantMember(
        const std::string& name,
        ASTNode* parent_node
) : BaseDefMember(name), parent_node(parent_node), ref_type(name, this) {

}

BaseDefMember *VariantMember::copy_member() {
    const auto member = new VariantMember(name, parent_node);
    for(auto& value : values) {
        member->values[value.first] = std::unique_ptr<VariantMemberParam>(value.second->copy());
    }
    return member;
}

void VariantMember::accept(Visitor *visitor) {

}

void VariantMember::declare_top_level(SymbolResolver &linker) {

}

void VariantMember::declare_and_link(SymbolResolver &linker) {
    for(auto& value : values) {
        value.second->declare_and_link(linker);
    }
}

ASTNode *VariantMember::child(const std::string &name) {
    auto found = values.find(name);
    if(found == values.end()) {
        return (ASTNode*) &found->second;
    }
    return nullptr;
}

ASTNode *VariantMember::child(unsigned int index) {
    if(index >= values.size()) return nullptr;
    return (values.begin() + index)->second.get();
}

BaseType* VariantMember::child_type(unsigned int index) {
    const auto c = child(index);
    return c ? c->known_type() : nullptr;
}

bool VariantMember::requires_destructor() {
    for(auto& value : values) {
        if(value.second->type->requires_destructor()) {
            return true;
        }
    }
    return false;
}

BaseType* VariantMember::known_type() {
    return &ref_type;
}

std::unique_ptr<BaseType> VariantMember::create_value_type() {
    return std::make_unique<ReferencedType>(name, this);
}

hybrid_ptr<BaseType> VariantMember::get_value_type() {
    return hybrid_ptr<BaseType> { &ref_type, false };
}

ValueType VariantMember::value_type() const {
    return ValueType::Struct;
}

BaseTypeKind VariantMember::type_kind() const {
    return BaseTypeKind::Struct;
}

VariantMemberParam::VariantMemberParam(
    std::string name,
    std::unique_ptr<BaseType> type,
    std::unique_ptr<Value> def_value,
    VariantMember* parent_node
) : name(std::move(name)), type(std::move(type)), def_value(std::move(def_value)), parent_node(parent_node) {

}

VariantMemberParam* VariantMemberParam::copy() {
    return new VariantMemberParam(name, std::unique_ptr<BaseType>(type->copy()), std::unique_ptr<Value>(def_value ? def_value->copy() : nullptr), parent_node);
}

void VariantMemberParam::declare_and_link(SymbolResolver &linker) {
    type->link(linker, type);
    if(def_value) {
        def_value->link(linker, def_value);
    }
}