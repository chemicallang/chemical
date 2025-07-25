// Copyright (c) Chemical Language Foundation 2025.

#include "VariantDefinition.h"
#include "VariantMember.h"
#include "ast/types/LinkedType.h"
#include "compiler/mangler/NameMangler.h"
#include "ast/base/ChainValue.h"
#include "ast/values/VariantCase.h"
#include "ast/values/AccessChain.h"
#include "ast/values/FunctionCall.h"
#include "ast/values/VariableIdentifier.h"
#include "ast/statements/SwitchStatement.h"
#include "ast/structures/StructDefinition.h"
#include "ast/utils/GenericUtils.h"
#include "ast/types/GenericType.h"

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
        return llvm::StructType::create(*gen.ctx, elements, gen.mangler.mangle(member->parent()));
    }
}

llvm::Value* VariantDefinition::load_type_int(Codegen &gen, llvm::Value* pointer, SourceLocation location) {
    const auto def_type = llvm_type(gen);
    std::vector<llvm::Value*> idxList { gen.builder->getInt32(0), gen.builder->getInt32(0) };
    const auto gep = gen.builder->CreateGEP(def_type, pointer, idxList, "",gen.inbounds);
    const auto loadInst = gen.builder->CreateLoad(gen.builder->getInt32Ty(), gep, "");
    gen.di.instr(loadInst, location);
    return loadInst;
}

llvm::Value* VariantDefinition::get_param_pointer(Codegen& gen, llvm::Value* pointer, VariantMemberParam* param) {
    const auto mem = param->parent();
    const auto largest = largest_member()->as_variant_member_unsafe();
    llvm::Type* container_type;
    if(largest == mem) {
        container_type = llvm_type(gen);
    } else {
        container_type = llvm_type_with_member(gen, mem);
    }
    std::vector<llvm::Value*> idxList { gen.builder->getInt32(0), gen.builder->getInt32(1), gen.builder->getInt32(0), gen.builder->getInt32((int) param->index) };
    return gen.builder->CreateGEP(container_type, pointer, idxList, "", gen.inbounds);
}

llvm::Type* VariantDefinition::llvm_type(Codegen& gen) {
    if(llvm_struct_type) {
        return llvm_struct_type;
    }
    const auto largest = largest_member()->as_variant_member_unsafe();
    const auto type = llvm_type_with_member(gen, largest, is_anonymous());
    llvm_struct_type = type;
    return type;
}

llvm::Type *VariantDefinition::llvm_type(Codegen &gen, int16_t iteration) {
    auto type = llvm_type(gen);
    return type;
}

llvm::Type* VariantDefinition::llvm_param_type(Codegen &gen) {
    return gen.builder->getPtrTy();
}

llvm::Type* VariantDefinition::llvm_chain_type(Codegen &gen, std::vector<ChainValue*> &values, unsigned int index) {
    auto member = variables()[index];
    if(index + 1 < values.size()) {
        auto linked = values[index + 1]->linked_node();
        if(linked && member == linked) {
            std::vector<llvm::Type *> struct_type{member->llvm_chain_type(gen, values, index + 1)};
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
        for (auto& func: instantiated_functions()) {
            func->code_gen_declare(gen, this);
        }
    } else {
        for (auto& func: instantiated_functions()) {
            func->code_gen_body(gen, this);
        }
    }
}

void VariantDefinition::code_gen(Codegen &gen, bool declare) {
    auto& itr_ptr = declare ? iterations_declared : iterations_body_done;
    if(itr_ptr == 0) {
        code_gen_once(gen, declare);
        itr_ptr++;
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
    llvm_struct_type = nullptr;
    for(auto& function : instantiated_functions()) {
        function->code_gen_external_declare(gen);
    }
}

void VariantDefinition::llvm_destruct(Codegen &gen, llvm::Value *allocaInst, SourceLocation location) {
    const auto destr = destructor_func();
    if(destr) {
        // making a call to destructor function
        const auto data = destr->llvm_func(gen);
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
    const auto linked_member = member;
    auto index = linked_member->parent()->direct_child_index(linked_member->name);
    if(index == -1) {
        gen.error(this) << "couldn't find case index of variant member '" << linked_member->name << "'";
        return nullptr;
    } else {
        return gen.builder->getInt32(index);
    }
}

llvm::Value* VariantCaseVariable::llvm_pointer(Codegen &gen) {
    const auto switch_statement = parent();
    const auto holder_pointer = switch_statement->expression->llvm_pointer(gen);
    const auto linked_member = member_param->parent();
    const auto linked_def = linked_member->parent();
    return linked_def->get_param_pointer(gen, holder_pointer, member_param);
}

llvm::Value* VariantCaseVariable::llvm_load(Codegen& gen, SourceLocation location) {
    return Value::load_value(gen, known_type(), llvm_type(gen), llvm_pointer(gen), location);
}

llvm::Type* VariantCaseVariable::llvm_type(Codegen &gen) {
    if(is_generic_param()) {
        const auto result = member_param->type->llvm_type(gen);
        return result;
    } else {
        return member_param->type->llvm_type(gen);
    }
}

bool VariantCaseVariable::add_child_index(Codegen& gen, std::vector<llvm::Value *>& indexes, const chem::string_view& name) {
    if(is_generic_param()) {
        const auto result = member_param->add_child_index(gen, indexes, name);
        return result;
    } else {
        return member_param->add_child_index(gen, indexes, name);
    }
}

#endif

void VariantDefinition::generate_functions(ASTAllocator& allocator, ASTDiagnoser& diagnoser) {
    bool has_destructor = false;
    for(auto& func : non_gen_range()) {
        if(func->is_delete_fn()) {
            func->ensure_destructor(allocator, diagnoser, this);
            has_destructor = true;
        }
        if(func->is_copy_fn()) {
            func->ensure_copy_fn(allocator, diagnoser, this);
        }
    }
    if(!has_destructor && any_member_has_destructor()) {
        has_destructor = true;
        create_def_destructor(allocator, diagnoser);
    }
    if(!has_destructor) {
        attrs.is_copy = true;
    }
}

BaseType* VariantDefinition::known_type() {
    return &ref_type;
}

bool VariantDefinition::requires_destructor() {
    for(const auto var : variables()) {
        auto member = var->as_variant_member();
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

bool VariantMember::requires_copy_fn() {
    for(auto& value : values) {
        if(value.second->type->requires_copy_fn()) {
            return true;
        }
    }
    return false;
}

BaseType* VariantMember::known_type() {
    return parent()->known_type();
}

BaseType* VariantCaseVariable::known_type() {
    return member_param->type;
}


bool VariantCaseVariable::is_generic_param() {
    const auto linked = member_param->type->linked_node();
    return linked != nullptr && linked->as_generic_type_param() != nullptr;
}