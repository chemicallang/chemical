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
#include "ast/structures/InterfaceDefinition.h"
#include "ast/utils/GenericUtils.h"
#include "ast/types/GenericType.h"

#ifdef COMPILER_BUILD

#include "compiler/Codegen.h"
#include "compiler/llvmimpl.h"

// get no of directly inherited composed struct types inside variant definition
unsigned int direct_inh_composed_structs(VariantDefinition* def) {
    unsigned i = 0;
    for(const auto &inherits : def->inherited) {
        if(inherits.type->get_direct_linked_struct()) {
            i++;
        }
    }
    return i;
}

llvm::StructType* VariantDefinition::llvm_type_with_member(Codegen& gen, VariantMember* member, bool anonymous) {
    std::vector<llvm::Type*> elements;
    const auto def = member->parent();

    elements.reserve(def->variables().size() + def->inherited.size());
    for(const auto &inherits : def->inherited) {
        if(inherits.type->get_direct_linked_struct()) {
            elements.emplace_back(inherits.type->llvm_type(gen));
        }
    }

    elements.emplace_back(gen.builder->getInt32Ty()); // <--- int enum is stored at top (after inherited structs), so we know the type
    std::vector<llvm::Type*> sub_elements { member->llvm_raw_struct_type(gen) };
    elements.emplace_back(llvm::StructType::get(*gen.ctx, sub_elements));
    if(anonymous) {
        return llvm::StructType::get(*gen.ctx, elements);
    } else {
        ScratchString<128> temp_name;
        gen.mangler.mangle(temp_name, def);
        return llvm::StructType::create(*gen.ctx, elements, (std::string_view) temp_name);
    }
}

llvm::Value* VariantDefinition::ptr_to_type_int(Codegen& gen, llvm::Type* def_type, llvm::Value* pointer) {
    std::initializer_list<llvm::Value*> idxList { gen.builder->getInt32(0), gen.builder->getInt32(direct_inh_composed_structs(this)) };
    return gen.builder->CreateGEP(def_type, pointer, idxList, "",gen.inbounds);
}

llvm::Value* VariantDefinition::load_type_int(Codegen &gen, llvm::Type* def_type, llvm::Value* pointer, SourceLocation location) {
    const auto gep = ptr_to_type_int(gen, def_type, pointer);
    const auto loadInst = gen.builder->CreateLoad(gen.builder->getInt32Ty(), gep, "");
    gen.di.instr(loadInst, location);
    return loadInst;
}

llvm::Value* VariantDefinition::get_member_pointer(Codegen& gen, llvm::Type* def_type, llvm::Value* pointer) {
    std::initializer_list<llvm::Value*> idxList { gen.builder->getInt32(0), gen.builder->getInt32(1 + direct_inh_composed_structs(this)) };
    return gen.builder->CreateGEP(def_type, pointer, idxList, "", gen.inbounds);
}

llvm::Value* VariantDefinition::get_param_pointer(Codegen& gen, llvm::Type* def_type, llvm::Value* pointer, VariantMemberParam* param) {
    std::initializer_list<llvm::Value*> idxList { gen.builder->getInt32(0), gen.builder->getInt32(1 + direct_inh_composed_structs(this)), gen.builder->getInt32(0), gen.builder->getInt32((int) param->index) };
    return gen.builder->CreateGEP(def_type, pointer, idxList, "", gen.inbounds);
}

llvm::Value* VariantDefinition::get_param_pointer(Codegen& gen, llvm::Value* pointer, VariantMemberParam* param) {
    const auto mem = param->parent();
    const auto container_type = llvm_type_with_member(gen, mem);
    return get_param_pointer(gen, container_type, pointer, param);
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
    if(decl->is_override()) {
        llvm_override_declare(gen, decl);
        return;
    }
    decl->code_gen_declare(gen, this);
}

void llvm_override(Codegen& gen, FunctionDeclaration* function) {
    const auto func = function->known_func(gen);
    if(func == nullptr) {
        gen.error("couldn't get function pointer", function);
        return;
    }
    function->code_gen_override(gen, func);
}

void VariantDefinition::code_gen_function_body(Codegen &gen, FunctionDeclaration* decl) {
    if(decl->is_override()) {
        llvm_override(gen, decl);
        return;
    }
    decl->code_gen_body(gen, this);
}

void VariantDefinition::code_gen_once(Codegen &gen, bool declare) {
    if(declare) {
        llvm_type(gen);
        for (auto& func: instantiated_functions()) {
            if (func->is_override()) {
                llvm_override_declare(gen, func);
                continue;
            }
            func->code_gen_declare(gen, this);
        }
    } else {
        for (auto& func: instantiated_functions()) {
            if (func->is_override()) {
                llvm_override(gen, func);
                continue;
            }
            func->code_gen_body(gen, this);
        }
    }
}

void VariantDefinition::code_gen(Codegen &gen, bool declare) {
    auto& has_done = declare ? has_declared : has_implemented;
    if(!has_done) {
        code_gen_once(gen, declare);
        if (!declare) {
            for (auto& inherits: inherited) {
                const auto interface = inherits.type->get_direct_linked_interface();
                if (interface && !interface->is_static()) {
                    interface->llvm_global_vtable(gen, this);
                }
            }
        }
        has_done = true;
    }
}

bool VariantDefinition::add_child_index(Codegen& gen, std::vector<llvm::Value *>& indexes, const chem::string_view& name) {
    if(indexes.empty()) {
        indexes.emplace_back(gen.builder->getInt32(0));
    }

    auto index = variable_index(name, false);
    if (index == -1) {
        const auto curr_size = (int) indexes.size();
        int inherit_ind = 0;
        // checking the inherited structs for given child
        for(auto& inherits : inherited) {
            auto linked_def = inherits.type->get_direct_linked_struct();
            if(linked_def) {
                if(linked_def->add_child_index(gen, indexes, name)) {
                    const auto itr = indexes.begin() + curr_size;
                    indexes.insert(itr, gen.builder->getInt32(inherit_ind));
                    return true;
                }
            }
            inherit_ind++;
        }
        return false;
    }

    return llvm_union_child_index(gen, indexes, name);
}

void VariantDefinition::code_gen_external_declare(Codegen &gen) {
    // clear the stored llvm types so they are generated again for this module
    llvm_struct_type = nullptr;
    extendable_external_declare(gen);
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
    const auto p = parent();
    indexes.emplace_back(gen.builder->getInt32(direct_inh_composed_structs(p) + index));
    return true;
}

llvm::Type* VariantMember::llvm_raw_struct_type(Codegen &gen) {
    std::vector<llvm::Type*> elements;
    for(auto& value : values) {
        elements.emplace_back(value.second->llvm_type(gen));
    }
    return llvm::StructType::get(*gen.ctx, elements);
}

llvm::Type *VariantMember::llvm_type(Codegen &gen) {
    return parent()->llvm_type(gen);
}

llvm::Type* VariantMemberParam::llvm_type(Codegen &gen) {
    return type->llvm_type(gen);
}

bool VariantMemberParam::add_child_index(Codegen& gen, std::vector<llvm::Value *>& indexes, const chem::string_view& name) {
    return type->canonical()->linked_node()->add_child_index(gen, indexes, name);
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

void VariantDefinition::generate_functions(ASTAllocator& allocator, ASTDiagnoser& diagnoser, ASTNode* returnNode) {
    bool has_constructor = false;
    bool has_def_constructor = false;
    bool has_destructor = false;
    for(auto& func : non_gen_range()) {
        if(func->is_constructor_fn()) {
            has_constructor = true;
            if(!func->has_explicit_params()) {
                has_def_constructor = true;
            }
            func->ensure_constructor(allocator, diagnoser, returnNode);
        }
        if(func->is_delete_fn()) {
            func->ensure_destructor(allocator, diagnoser, returnNode);
            has_destructor = true;
        }
        if(func->is_copy_fn()) {
            func->ensure_copy_fn(allocator, diagnoser, returnNode);
        }
    }
    if(!has_destructor && any_member_has_destructor()) {
        has_destructor = true;
        create_def_destructor(allocator, diagnoser, returnNode, false);
    }
    if(!has_destructor) {
        attrs.is_copy = true;
    }
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

uint64_t VariantDefinition::byte_size(TargetData& target) {
    const auto type_size = target.is64Bit ? 4 : 2; // <--- an int type enum is stored inside
    const auto large = largest_member();
    if(!large) return type_size;
    return large->byte_size(target) + type_size;
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

bool VariantCaseVariable::is_generic_param() {
    const auto linked = member_param->type->linked_node();
    return linked != nullptr && linked->as_generic_type_param() != nullptr;
}