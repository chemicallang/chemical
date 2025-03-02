// Copyright (c) Chemical Language Foundation 2025.

#include "VariantDefinition.h"
#include "VariantMember.h"
#include "ast/types/LinkedType.h"
#include "compiler/SymbolResolver.h"
#include "ast/base/ChainValue.h"
#include "ast/values/VariantCase.h"
#include "ast/values/AccessChain.h"
#include "ast/values/FunctionCall.h"
#include "ast/values/VariableIdentifier.h"
#include "ast/statements/SwitchStatement.h"
#include "ast/structures/StructDefinition.h"
#include "ast/utils/GenericUtils.h"
#include "ast/types/GenericType.h"

inline void restore(std::pair<BaseType*, int16_t> pair) {
    pair.first->set_generic_iteration(pair.second);
}

#ifdef COMPILER_BUILD

#include "compiler/Codegen.h"
#include "compiler/llvmimpl.h"

llvm::StructType* VariantDefinition::llvm_type_with_member(Codegen& gen, VariantMember* member, bool anonymous) {
    std::vector<llvm::Type*> elements;
    elements.emplace_back(gen.builder->getInt32Ty()); // <--- int enum is stored at top, so we know the type
    if(member) {
        std::vector<llvm::Type*> sub_elements { member->llvm_raw_struct_type(gen) };
        elements.emplace_back(llvm::StructType::get(*gen.ctx, sub_elements));
    }
    if(anonymous) {
        return llvm::StructType::get(*gen.ctx, elements);
    } else {
        return llvm::StructType::create(*gen.ctx, elements, member->parent()->runtime_name_str());
    }
}

llvm::Type* VariantDefinition::llvm_type(Codegen& gen) {
    auto found = llvm_struct_types.find(active_iteration);
    if(found != llvm_struct_types.end()) {
        return found->second;
    }
    const auto largest = largest_member()->as_variant_member_unsafe();
    const auto type = llvm_type_with_member(gen, largest, is_anonymous());
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

llvm::Type* VariantDefinition::llvm_chain_type(Codegen &gen, std::vector<ChainValue*> &values, unsigned int index) {
    auto member = variables.begin() + index;
    if(index + 1 < values.size()) {
        auto linked = values[index + 1]->linked_node();
        if(linked && member->second == linked) {
            std::vector<llvm::Type *> struct_type{member->second->llvm_chain_type(gen, values, index + 1)};
            return llvm::StructType::get(*gen.ctx, struct_type);
        }
    }
    return llvm_type(gen);
}

void VariantDefinition::code_gen_function_declare(Codegen &gen, FunctionDeclaration* decl) {
    decl->code_gen_declare(gen, this);
}

void VariantDefinition::code_gen_function_body(Codegen &gen, FunctionDeclaration* decl) {
    decl->code_gen_body(gen, this);
}

void VariantDefinition::code_gen_once(Codegen &gen, bool declare) {
    if(declare) {
        llvm_type(gen);
        for (auto& func: functions()) {
            func->code_gen_declare(gen, this);
        }
    } else {
        for (auto& func: functions()) {
            func->code_gen_body(gen, this);
        }
    }
}

void VariantDefinition::code_gen(Codegen &gen, bool declare) {
    auto& itr_ptr = declare ? iterations_declared : iterations_body_done;
    if(generic_params.empty()) {
        if(itr_ptr == 0) {
            code_gen_once(gen, declare);
            itr_ptr++;
        }
    } else {
        const auto total = total_generic_iterations();
        const auto prev_itr = active_iteration;
        auto i = itr_ptr;
        while(i < total) {
            set_active_iteration(i);
            if(declare) {
                early_declare_structural_generic_args(gen);
            }
            code_gen_once(gen, declare);
            i++;
        }
        set_active_iteration(prev_itr);
        itr_ptr = total;
    }
}

bool VariantDefinition::add_child_index(Codegen& gen, std::vector<llvm::Value *>& indexes, const chem::string_view& name) {
    if(indexes.empty()) {
        indexes.emplace_back(gen.builder->getInt32(0));
    }
    indexes.emplace_back(gen.builder->getInt32(1));
    return llvm_union_child_index(gen, indexes, name);
}

void VariantDefinition::code_gen_external_declare(Codegen &gen) {
    // clear the stored llvm types so they are generated again for this module
    llvm_struct_types.clear();
    for(auto& function : functions()) {
        function->code_gen_external_declare(gen);
    }
}

void VariantDefinition::llvm_destruct(Codegen &gen, llvm::Value *allocaInst, SourceLocation location) {
    const auto destr = destructor_func();
    if(destr) {
        // making a call to destructor function
        const auto data = llvm_func_data(destr);
        const auto instr = gen.builder->CreateCall(data, { allocaInst });
        gen.di.instr(instr, location);
    }
}

bool VariantMember::add_child_index(Codegen& gen, std::vector<llvm::Value *>& indexes, const chem::string_view& name) {
    const auto found = values.find(name);
    if(found == values.end()) {
        return false;
    }
    const auto index = found - values.begin();
    indexes.emplace_back(gen.builder->getInt32(index));
}

llvm::Type* VariantMember::llvm_raw_struct_type(Codegen &gen) {
    std::vector<llvm::Type*> elements;
    for(auto& value : values) {
        elements.emplace_back(value.second->llvm_type(gen));
    }
    return llvm::StructType::get(*gen.ctx, elements);
}

llvm::Type *VariantMember::llvm_type(Codegen &gen) {
    return VariantDefinition::llvm_type_with_member(gen, this);
}

llvm::Type* VariantMemberParam::llvm_type(Codegen &gen) {
    return type->llvm_type(gen);
}

bool VariantMemberParam::add_child_index(Codegen& gen, std::vector<llvm::Value *>& indexes, const chem::string_view& name) {
    return type->pure_type(gen.allocator)->linked_node()->add_child_index(gen, indexes, name);
}

llvm::Value* VariantCase::llvm_value(Codegen &gen, BaseType *type) {
    const auto linked_member = parent_val->linked_node()->as_variant_member();
    auto index = linked_member->parent()->direct_child_index(linked_member->name);
    if(index == -1) {
        gen.error(this) << "couldn't find case index of variant member '" << parent_val->representation() << "'";
        return nullptr;
    } else {
        return gen.builder->getInt32(index);
    }
}

llvm::Value* VariantCaseVariable::llvm_pointer_no_itr(Codegen& gen) {
    const auto holder_pointer = switch_statement->expression->llvm_pointer(gen);
    const auto linked_member = parent_val->linked_node()->as_variant_member();
    const auto linked_def = linked_member->parent();
    const auto largest_member = linked_def->largest_member()->as_variant_member_unsafe();
    llvm::Type* container_type;
    if(largest_member == linked_member) {
        container_type = linked_def->llvm_type(gen);
    } else {
        container_type = linked_def->llvm_type_with_member(gen, linked_member);
    }
    std::vector<llvm::Value*> idxList { gen.builder->getInt32(0), gen.builder->getInt32(1), gen.builder->getInt32(0), gen.builder->getInt32((int) member_param->index) };
    return gen.builder->CreateGEP(container_type, holder_pointer, idxList, "", gen.inbounds);
}

llvm::Value* VariantCaseVariable::llvm_pointer(Codegen &gen) {
    const auto expr = switch_statement->expression;
    const auto expr_type = expr->create_type(gen.allocator);
    const auto prev_itr = expr_type->set_generic_iteration(expr_type->get_generic_iteration());
    const auto ptr = llvm_pointer_no_itr(gen);
    expr_type->set_generic_iteration(prev_itr);
    return ptr;
}

llvm::Value* VariantCaseVariable::llvm_load(Codegen& gen, SourceLocation location) {
    const auto expr = switch_statement->expression;
    const auto expr_type = expr->create_type(gen.allocator);
    const auto prev_itr = expr_type->set_generic_iteration(expr_type->get_generic_iteration());
    const auto value = Value::load_value(gen, known_type(), llvm_type(gen), llvm_pointer_no_itr(gen), location);
    expr_type->set_generic_iteration(prev_itr);
    return value;
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

bool VariantCaseVariable::add_child_index(Codegen& gen, std::vector<llvm::Value *>& indexes, const chem::string_view& name) {
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

ASTNode* VariantDefinition::child(const chem::string_view &child_name) {
    return ExtendableMembersContainerNode::child(child_name);
}

void VariantDefinition::declare_top_level(SymbolResolver &linker, ASTNode*& node_ptr) {
    linker.declare_node(name_view(), this, specifier(), true);
}

void VariantDefinition::link_signature(SymbolResolver &linker) {
    MembersContainer::link_signature(linker);
}

void VariantDefinition::declare_and_link(SymbolResolver &linker, ASTNode*& node_ptr) {
    auto& allocator = specifier() == AccessSpecifier::Public ? *linker.ast_allocator : *linker.mod_allocator;
    bool has_destructor = false;
    bool has_clear_fn = false;
    bool has_move_fn = false;
    for(auto& func : functions()) {
        if(func->is_delete_fn()) {
            func->ensure_destructor(linker, this);
            has_destructor = true;
        }
        if(func->is_clear_fn()) {
            func->ensure_clear_fn(linker, this);
            has_clear_fn = true;
        }
        if(func->is_move_fn()) {
            func->ensure_move_fn(linker, this);
            has_move_fn = true;
        }
        if(func->is_copy_fn()) {
            func->ensure_copy_fn(linker, this);
        }
    }
    MembersContainer::declare_and_link(linker, node_ptr);
//    register_use_to_inherited_interfaces(this);
    if(!has_clear_fn && any_member_has_clear_func()) {
        create_def_clear_fn(allocator, linker);
    }
    if(!has_move_fn && any_member_has_pre_move_func()) {
        create_def_move_fn(allocator, linker);
    }
    if(!has_destructor && any_member_has_destructor()) {
        create_def_destructor(allocator, linker);
    }
}

BaseType* VariantDefinition::known_type() {
    return &ref_type;
}

bool VariantDefinition::requires_destructor() {
    for(auto& var : variables) {
        auto member = var.second->as_variant_member();
        if(member->requires_destructor()) {
            return true;
        }
    }
    return false;
}

uint64_t VariantDefinition::byte_size(bool is64Bit) {
    const auto type_size = is64Bit ? 4 : 2; // <--- an int type enum is stored inside
    const auto large = largest_member();
    if(!large) return type_size;
    return large->byte_size(is64Bit) + type_size;
}

BaseType* VariantDefinition::create_value_type(ASTAllocator& allocator) {
    return create_linked_type(name_view(), allocator);
}

//hybrid_ptr<BaseType> VariantDefinition::get_value_type() {
//    return hybrid_ptr<BaseType> { create_value_type(), true };
//}

int16_t VariantDefinition::register_call(SymbolResolver& resolver, FunctionCall* call, BaseType* expected_type) {

    const auto total = generic_params.size();
    std::vector<BaseType*> generic_args(total);

    // set all to default type (if default type is not present, it would automatically be nullptr)
    unsigned i = 0;
    while(i < total) {
        generic_args[i] = generic_params[i]->def_type;
        i++;
    }

    // set given generic args to generic parameters
    i = 0;
    for(auto& arg : call->generic_list) {
        generic_args[i] = arg;
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
                generic_args[i] = arg;
                i++;
            }
        }
    }

    // register and report to subscribers
    auto& astAllocator = *resolver.ast_allocator;
    const auto itr = register_generic_usage(astAllocator, generic_params, generic_args);
    if(itr.second) {
        for (auto sub: subscribers) {
            sub->report_parent_usage(astAllocator, resolver, itr.first);
        }
    }

    return itr.first;
}

BaseDefMember *VariantMember::copy_member(ASTAllocator& allocator) {
    const auto member = new (allocator.allocate<VariantMember>()) VariantMember(name, parent(), encoded_location());
    for(auto& value : values) {
        member->values[value.first] = value.second->copy(allocator);
    }
    return member;
}

void VariantMember::declare_top_level(SymbolResolver &linker, ASTNode*& node_ptr) {

}

void VariantMember::declare_and_link(SymbolResolver &linker, ASTNode*& node_ptr) {
    for(auto& value : values) {
        value.second->declare_and_link(linker, (ASTNode*&) value.second);
    }
}

ASTNode *VariantMember::child(const chem::string_view &name) {
    auto found = values.find(name);
    if(found == values.end()) {
        return (ASTNode*) &found->second;
    }
    return nullptr;
}

ASTNode *VariantMember::child(unsigned int index) {
    if(index >= values.size()) return nullptr;
    return (values.begin() + index)->second;
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

bool VariantMember::requires_clear_fn() {
    for(auto& value : values) {
        if(value.second->type->requires_clear_fn()) {
            return true;
        }
    }
    return false;
}

bool VariantMember::requires_copy_fn() {
    for(auto& value : values) {
        if(value.second->type->requires_copy_fn()) {
            return true;
        }
    }
    return false;
}

bool VariantMember::requires_move_fn() {
    for(auto& value : values) {
        if(value.second->type->requires_move_fn()) {
            return true;
        }
    }
    return false;
}

BaseType* VariantMember::known_type() {
    return &ref_type;
}

BaseType* VariantMember::create_value_type(ASTAllocator& allocator) {
    return new (allocator.allocate<LinkedType>()) LinkedType(name, this, encoded_location());
}

//hybrid_ptr<BaseType> VariantMember::get_value_type() {
//    return hybrid_ptr<BaseType> { &ref_type, false };
//}

BaseTypeKind VariantMember::type_kind() const {
    return BaseTypeKind::Struct;
}

VariantMemberParam* VariantMemberParam::copy(ASTAllocator& allocator) {
    return new (allocator.allocate<VariantMemberParam>()) VariantMemberParam(name, index, is_const, type->copy(allocator), def_value ? def_value->copy(allocator) : nullptr, parent(), encoded_location());
}

void VariantMemberParam::declare_and_link(SymbolResolver &linker, ASTNode*& node_ptr) {
    type->link(linker);
    if(def_value) {
        def_value->link(linker, def_value);
    }
}

ASTNode* VariantMemberParam::child(int varIndex) {
    const auto linked = type->linked_node();
    return linked->child(varIndex);
}

int VariantMemberParam::child_index(const chem::string_view &varName) {
    return type->linked_node()->child_index(varName);
}

ASTNode* VariantMemberParam::child(const chem::string_view &varName) {
    const auto pure_type = type->pure_type();
    const auto linked_node = pure_type->linked_node();
    return linked_node->child(varName);
}

bool VariantCase::link(SymbolResolver &linker, Value*& value_ptr, BaseType *expected_type) {
    // access chain in variant case allows no replacement of access chain, so nullptr in value_ptr
    Value* empty_val = nullptr;
    parent_val->link(linker, empty_val, nullptr);
    for(auto& variable : identifier_list) {
        variable.declare_and_link(linker, (ASTNode*&) variable);
    }
    return true;
}

void VariantCaseVariable::declare_and_link(SymbolResolver &linker, ASTNode*& node_ptr) {
    const auto member = parent_val->linked_node()->as_variant_member();
    auto node = member->values.find(name);
    if(node == member->values.end()) {
        linker.error(this) << "variant case member variable not found in switch statement, name '" << name << "' not found";
        return;
    }
    member_param = node->second;
    linker.declare(name, this);
}

//hybrid_ptr<BaseType> VariantCaseVariable::get_value_type() {
//    return hybrid_ptr<BaseType> { member_param->type.get(), false };
//}

BaseType* VariantCaseVariable::create_value_type(ASTAllocator& allocator) {
    const auto expr = switch_statement->expression;
    const auto expr_type = expr->create_type(allocator);
    const auto prev_itr = expr_type->set_generic_iteration(expr_type->get_generic_iteration());
    const auto copied_type = member_param->type->copy(allocator)->pure_type(allocator);
    expr_type->set_generic_iteration(prev_itr);
    return copied_type;
}

BaseType* VariantCaseVariable::known_type() {
    return member_param->type;
}

std::pair<BaseType*, int16_t> VariantCaseVariable::set_iteration() {
    const auto known_type = switch_statement->expression->known_type();
    const auto prev_itr = known_type->set_generic_iteration(known_type->get_generic_iteration());
    return { known_type, prev_itr };
}

bool VariantCaseVariable::is_generic_param() {
    const auto linked = member_param->type->linked_node();
    return linked != nullptr && linked->as_generic_type_param() != nullptr;
}

ASTNode* VariantCaseVariable::child(const chem::string_view &child_name) {
    if(is_generic_param()) {
        auto itr = set_iteration();
        const auto result = member_param->child(child_name);
        restore(itr);
        return result;
    } else {
        return member_param->child(child_name);
    }
}

int VariantCaseVariable::child_index(const chem::string_view &child_index) {
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