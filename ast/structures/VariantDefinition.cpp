// Copyright (c) Qinetik 2024.

#include "VariantDefinition.h"
#include "VariantMember.h"
#include "ast/types/ReferencedType.h"
#include "compiler/SymbolResolver.h"
#include "ast/base/ChainValue.h"
#include "ast/values/VariantCase.h"
#include "ast/values/AccessChain.h"
#include "ast/values/FunctionCall.h"
#include "ast/values/VariableIdentifier.h"
#include "ast/statements/SwitchStatement.h"
#include "ast/values/VariantCall.h"
#include "ast/utils/GenericUtils.h"
#include "ast/types/GenericType.h"

inline void restore(std::pair<BaseType*, int16_t> pair) {
    pair.first->set_generic_iteration(pair.second);
}

#ifdef COMPILER_BUILD

#include "compiler/Codegen.h"
#include "compiler/llvmimpl.h"

llvm::StructType* VariantDefinition::llvm_type_with_member(Codegen& gen, BaseDefMember* member, bool anonymous) {
    std::vector<llvm::Type*> elements;
    elements.emplace_back(gen.builder->getInt32Ty()); // <--- int enum is stored at top, so we know the type
    if(member) {
        std::vector<llvm::Type*> sub_elements { member->llvm_type(gen) };
        elements.emplace_back(llvm::StructType::get(*gen.ctx, sub_elements));
    }
    if(anonymous) {
        return llvm::StructType::get(*gen.ctx, elements);
    } else {
        return llvm::StructType::create(*gen.ctx, elements, name);
    }
}

llvm::Type* VariantDefinition::llvm_type(Codegen& gen) {
    auto found = llvm_struct_types.find(active_iteration);
    if(found != llvm_struct_types.end()) {
        return found->second;
    }
    const auto type = llvm_type_with_member(gen, largest_member(), false);
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
    if(generic_params.empty()) {
        llvm_type(gen);
    } else {
        const auto total = total_generic_iterations();
        const auto prev_itr = active_iteration;
        int16_t i = 0;
        while(i < total) {
            set_active_iteration(i);
            llvm_type(gen);
            i++;
        }
        set_active_iteration(prev_itr);
    }
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

bool VariantMemberParam::add_child_index(Codegen &gen, std::vector<llvm::Value *> &indexes, const std::string &name) {
    return type->pure_type()->linked_node()->add_child_index(gen, indexes, name);
}

llvm::Value* VariantCase::llvm_value(Codegen &gen, BaseType *type) {
    const auto linked_member = chain->linked_node()->as_variant_member();
    auto index = linked_member->parent_node->direct_child_index(linked_member->name);
    if(index == -1) {
        gen.error("couldn't find case index of variant member '" + chain->chain_representation() + "'");
        return nullptr;
    } else {
        return gen.builder->getInt32(index);
    }
}

llvm::Value* VariantCaseVariable::llvm_pointer(Codegen &gen) {
    const auto holder_pointer = variant_case->switch_statement->expression->llvm_pointer(gen);
    const auto linked_def = variant_case->switch_statement->expression->known_type()->linked_node()->as_variant_def();
    const auto linked_member = variant_case->chain->linked_node()->as_variant_member();
    const auto largest_member = linked_def->largest_member();
    llvm::Type* container_type;
    if(largest_member == linked_member) {
        container_type = linked_def->llvm_type(gen);
    } else {
        container_type = linked_def->llvm_type_with_member(gen, linked_member);
    }
    std::vector<llvm::Value*> idxList { gen.builder->getInt32(0), gen.builder->getInt32(1), gen.builder->getInt32(0), gen.builder->getInt32((int) member_param->index) };
    return gen.builder->CreateGEP(container_type, holder_pointer, idxList, "", gen.inbounds);
}

llvm::Value* VariantCaseVariable::llvm_load(Codegen &gen) {
    return gen.builder->CreateLoad(llvm_type(gen), llvm_pointer(gen));
}

llvm::Type* VariantCaseVariable::llvm_type(Codegen &gen) {
    if(is_generic_param()) {
        auto itr = set_iteration();
        const auto result = member_param->type->llvm_type(gen);
        restore(itr);
        return result;
    } else {
        return member_param->type->llvm_type(gen);
    }
}

bool VariantCaseVariable::add_child_index(Codegen &gen, std::vector<llvm::Value *> &indexes, const std::string &name) {
    if(is_generic_param()) {
        auto itr = set_iteration();
        const auto result = member_param->add_child_index(gen, indexes, name);
        restore(itr);
        return result;
    } else {
        return member_param->add_child_index(gen, indexes, name);
    }
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

int16_t VariantDefinition::register_call(SymbolResolver& resolver, VariantCall* call, BaseType* expected_type) {

    const auto total = generic_params.size();
    std::vector<BaseType*> generic_args(total);

    // set all to default type (if default type is not present, it would automatically be nullptr)
    unsigned i = 0;
    while(i < total) {
        generic_args[i] = generic_params[i]->def_type.get();
        i++;
    }

    // set given generic args to generic parameters
    i = 0;
    for(auto& arg : call->generic_list) {
        generic_args[i] = arg.get();
        i++;
    }

    // infer args, if user gave less args than expected
    if(call->generic_list.size() != total) {
        call->infer_generic_args(resolver, generic_args);
    }

    // inferring type by expected type
    if(expected_type && expected_type->kind() == BaseTypeKind::Generic) {
        const auto type = ((GenericType*) expected_type);
        if(type->linked_node() == this) {
            i = 0;
            for(auto& arg : type->types) {
                generic_args[i] = arg.get();
                i++;
            }
        }
    }

    // register and report to subscribers
    const auto itr = register_generic_usage(resolver, this, generic_params, generic_args);
    if(itr.second) {
        for (auto sub: subscribers) {
            sub->report_parent_usage(resolver, itr.first);
        }
    }

    return itr.first;
}

VariantMember::VariantMember(
        const std::string& name,
        VariantDefinition* parent_node
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
    unsigned index,
    std::unique_ptr<BaseType> type,
    std::unique_ptr<Value> def_value,
    VariantMember* parent_node
) : name(std::move(name)), index(index), type(std::move(type)), def_value(std::move(def_value)), parent_node(parent_node) {

}

VariantMemberParam* VariantMemberParam::copy() {
    return new VariantMemberParam(name, index,std::unique_ptr<BaseType>(type->copy()), std::unique_ptr<Value>(def_value ? def_value->copy() : nullptr), parent_node);
}

void VariantMemberParam::declare_and_link(SymbolResolver &linker) {
    type->link(linker, type);
    if(def_value) {
        def_value->link(linker, def_value);
    }
}

ASTNode* VariantMemberParam::child(int varIndex) {
    const auto linked = type->linked_node();
    return linked->child(varIndex);
}

int VariantMemberParam::child_index(const std::string &varName) {
    return type->linked_node()->child_index(varName);
}

ASTNode* VariantMemberParam::child(const std::string &varName) {
    const auto pure_type = type->pure_type();
    const auto linked_node = pure_type->linked_node();
    return linked_node->child(varName);
}

VariantCase::VariantCase(std::unique_ptr<AccessChain> _chain, ASTDiagnoser& diagnoser, SwitchStatement* statement) : chain(std::move(_chain)), switch_statement(statement) {
    const auto func_call = chain->values.back()->as_func_call();
    if(func_call) {
        for(auto& value : func_call->values) {
            const auto id = value->as_identifier();
            if(!id) {
                diagnoser.error("switch variant case with a function call doesn't contain identifiers '" + chain->chain_representation() + "', in question " + value->representation());
                return;
            }
            identifier_list.emplace_back(id->value, this);
        }
        // remove the last function call, as we took it's identifiers
        chain->values.pop_back();
    }
}

void VariantCase::link(SymbolResolver &linker, std::unique_ptr<Value> &value_ptr) {
    // access chain in variant case allows no replacement of access chain, so nullptr in value_ptr
    chain->link(linker, (BaseType*) nullptr, nullptr);
    for(auto& variable : identifier_list) {
        variable.declare_and_link(linker);
    }
}

void VariantCaseVariable::declare_and_link(SymbolResolver &linker) {
    const auto member = variant_case->chain->linked_node()->as_variant_member();
    auto node = member->values.find(name);
    if(node == member->values.end()) {
        linker.error("variant case member variable not found in switch statement, name '" + name + "' not found");
        return;
    }
    member_param = node->second.get();
    linker.declare(name, this);
}

VariantCaseVariable::VariantCaseVariable(std::string name, VariantCase* variant_case) : name(std::move(name)), variant_case(variant_case) {

}

void VariantCaseVariable::accept(Visitor *visitor) {
    throw std::runtime_error("VariantCaseVariable cannot be visited, As is it always contained within a VariantCase which is visited");
}

ASTNode* VariantCaseVariable::parent() {
    return (ASTNode*) variant_case->switch_statement;
}

hybrid_ptr<BaseType> VariantCaseVariable::get_value_type() {
    return hybrid_ptr<BaseType> { member_param->type.get(), false };
}

std::unique_ptr<BaseType> VariantCaseVariable::create_value_type() {
    return std::unique_ptr<BaseType>(member_param->type->copy());
}

BaseType* VariantCaseVariable::known_type() {
    return member_param->type.get();
}

std::pair<BaseType*, int16_t> VariantCaseVariable::set_iteration() {
    const auto known_type = variant_case->switch_statement->expression->known_type();
    const auto prev_itr = known_type->set_generic_iteration(known_type->get_generic_iteration());
    return { known_type, prev_itr };
}

bool VariantCaseVariable::is_generic_param() {
    const auto linked = member_param->type->linked_node();
    return linked ? linked->as_generic_type_param() != nullptr : false;
}

ASTNode* VariantCaseVariable::child(const std::string &child_name) {
    if(is_generic_param()) {
        auto itr = set_iteration();
        const auto result = member_param->child(child_name);
        restore(itr);
        return result;
    } else {
        return member_param->child(child_name);
    }
}

int VariantCaseVariable::child_index(const std::string &child_index) {
    if(is_generic_param()) {
        auto itr = set_iteration();
        const auto result = member_param->child_index(child_index);
        restore(itr);
        return result;
    } else {
        return member_param->child_index(child_index);
    }
}

ASTNode* VariantCaseVariable::child(int index) {
    if(is_generic_param()) {
        auto itr = set_iteration();
        const auto result = member_param->child(index);
        restore(itr);
        return result;
    } else {
        return member_param->child(index);
    }
}