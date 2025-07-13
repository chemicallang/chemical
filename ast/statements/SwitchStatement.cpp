// Copyright (c) Chemical Language Foundation 2025.

#include "SwitchStatement.h"
#include "ast/base/Value.h"
#include "ast/values/AccessChain.h"
#include "ast/values/FunctionCall.h"
#include "ast/values/VariantCaseVariable.h"
#include "ast/structures/VariantMember.h"
#include "ast/structures/GenericVariantDecl.h"
#include "ast/values/VariableIdentifier.h"
#include "ast/values/VariantCase.h"
#include "ast/types/ReferenceType.h"
#include "ast/types/IntNType.h"
#include "ast/structures/VariantDefinition.h"

#ifdef COMPILER_BUILD

#include "compiler/llvmimpl.h"
#include "compiler/Codegen.h"

llvm::Type* SwitchStatement::llvm_type(Codegen &gen) {
    const auto node = get_value_node();
    return node->llvm_type(gen);
}

llvm::AllocaInst* SwitchStatement::llvm_allocate(Codegen &gen, const std::string &identifier, BaseType *expected_type) {
    const auto allocated = gen.builder->CreateAlloca(expected_type ? expected_type->llvm_type(gen) : llvm_type(gen));
    gen.di.instr(allocated, Value::encoded_location());
    auto prev_assignable = gen.current_assignable;
    gen.current_assignable = { nullptr, allocated };
    code_gen(gen);
    gen.current_assignable = prev_assignable;
    return allocated;
}

llvm::Value* SwitchStatement::llvm_value(Codegen &gen, BaseType *type) {
    code_gen(gen);
    return nullptr;
}

void SwitchStatement::llvm_assign_value(Codegen &gen, llvm::Value *lhsPtr, Value *lhs) {
    auto prev_assignable = gen.current_assignable;
    gen.current_assignable = { lhs, lhsPtr };
    code_gen(gen);
    gen.current_assignable = prev_assignable;
}

//llvm::ConstantInt* write_variant_call_id_index(Codegen& gen, VariantDefinition* variant, VariableIdentifier* value) {
//    const auto member = value->linked->as_variant_member();
//    if(member) {
//        return gen.builder->getInt32(variant->variable_index(member->name, false));
//    } else {
//        return gen.builder->getInt32(-1);
//    }
//}
//
//llvm::ConstantInt* write_variant_call_call_index(Codegen& gen, VariantDefinition* variant, FunctionCall* value) {
//    const auto member = value->parent_val->linked_node()->as_variant_member();
//    if(member) {
//        return gen.builder->getInt32(variant->variable_index(member->name, false));
//    } else {
//        return gen.builder->getInt32(-1);
//    }
//}
//
//llvm::ConstantInt* write_variant_call_index(Codegen& gen, VariantDefinition* variant, Value* value) {
//    switch(value->val_kind()) {
//        case ValueKind::Identifier:
//            return write_variant_call_id_index(gen, variant, value->as_identifier_unsafe());
//        case ValueKind::FunctionCall:
//            return write_variant_call_call_index(gen, variant, value->as_func_call_unsafe());
//        case ValueKind::AccessChain: {
//            const auto chain = value->as_access_chain_unsafe();
//            if(chain) {
//                return write_variant_call_index(gen, variant, chain->values.back());
//            } else {
//                return gen.builder->getInt32(-1);
//            }
//        }
//        default:
//            return gen.builder->getInt32(-1);
//    }
//}

/// Implicitly casts an integer constant to the target integer type,
/// performing sign or zero extension (or truncation) as needed.
/// Returns a llvm::ConstantInt* if the conversion is valid, or nullptr if not.
llvm::ConstantInt* implicit_cast_constant(llvm::ConstantInt* value, BaseType* to_type, llvm::Type* to_type_llvm) {

    // The target type must be an integer type.
    if (!to_type_llvm->isIntegerTy())
        return value;

    const auto fromIntTy = value->getType();
    const auto toIntTy = llvm::cast<llvm::IntegerType>(to_type_llvm);

    unsigned srcWidth = value->getType()->getIntegerBitWidth();
    unsigned dstWidth = toIntTy->getBitWidth();

    // If the widths are already equal, no conversion is needed.
    if (srcWidth == dstWidth)
        return value;

    // Widen the integer.
    if (srcWidth < dstWidth) {
        const auto is_unsigned = to_type->kind() == BaseTypeKind::IntN && to_type->as_intn_type_unsafe()->is_unsigned();
        if(is_unsigned && !value->isNegative()) {
            const auto val = llvm::ConstantInt::get(toIntTy, value->getValue().zext(dstWidth));
            return llvm::dyn_cast<llvm::ConstantInt>(val);
        } else {
            const auto val = llvm::ConstantInt::get(toIntTy, value->getValue().sext(dstWidth));
            return llvm::dyn_cast<llvm::ConstantInt>(val);
        }
    }

    // Narrow the integer.
    const auto val = llvm::ConstantInt::get(toIntTy, value->getValue().trunc(dstWidth));
    return llvm::dyn_cast<llvm::ConstantInt>(val);

}

void SwitchStatement::code_gen(Codegen &gen, bool last_block) {

    auto total_scopes = scopes.size();

    // the end block
    llvm::BasicBlock* end = llvm::BasicBlock::Create(*gen.ctx, "end", gen.current_function);

    VariantDefinition* variant_def = nullptr;

    // this boolean can be set to true, to set to last case as default
    // this should be only set when it's guaranteed that default scope is not needed
    // because all cases are covered
    bool auto_default_case = false;

    llvm::Value* expr_value = expression->llvm_value(gen);
    const auto expr_type = expression->create_type(gen.allocator);
    if(expr_type) {

        // automatic dereference
        const auto pure_type = expr_type->pure_type(gen.allocator);
        if(pure_type->kind() == BaseTypeKind::Reference) {
            const auto ref = pure_type->as_reference_type_unsafe()->type->pure_type(gen.allocator);
            const auto ref_kind = ref->kind();
            if(BaseType::isIntNType(ref_kind) || ref_kind == BaseTypeKind::Bool) {
                const auto loadInst = gen.builder->CreateLoad(ref->llvm_type(gen), expr_value);
                gen.di.instr(loadInst, expression);
                expr_value = loadInst;
            }
        }

        // variant members
        const auto linked = expr_type->linked_node();
        if(linked) {
            const auto linked_kind = linked->kind();
            if(linked_kind == ASTNodeKind::VariantDecl) {
                variant_def = linked->as_variant_def_unsafe();
            } else if(linked_kind == ASTNodeKind::VariantMember) {
                variant_def = linked->as_variant_member_unsafe()->parent();
            }
            if (variant_def) {
                if (scopes.size() == variant_def->variables().size() && !has_default_case()) {
                    // TODO only do this when switch is a value
                    auto_default_case = true;
                }
                expr_value = variant_def->load_type_int(gen, expr_value, expression->encoded_location());
            }
        }

    }

    auto switchInst = gen.builder->CreateSwitch(expr_value, end, total_scopes);
    gen.di.instr(switchInst, Value::encoded_location());

    bool all_scopes_return = true;

    llvm::BasicBlock* caseBlock = nullptr;

    unsigned scope_ind = 0;
    const auto scopes_size = scopes.size();
    while(scope_ind < scopes_size) {
        auto& scope = scopes[scope_ind];

        caseBlock = llvm::BasicBlock::Create(*gen.ctx, "case", gen.current_function);
        gen.SetInsertPoint(caseBlock);
        scope.code_gen(gen);
        if(!gen.has_current_block_ended) {
            all_scopes_return = false;
        }
        gen.CreateBr(end, scope.encoded_location());

        for(auto& switch_case : cases) {
            if(switch_case.second == scope_ind) {
                const auto caseValue =  switch_case.first->llvm_value(gen);
                if(llvm::isa<llvm::ConstantInt>(caseValue)) {
                    const auto castedCase = implicit_cast_constant((llvm::ConstantInt*) caseValue, expr_type, expr_value->getType());
                    switchInst->addCase(castedCase, caseBlock);
                } else {
                    gen.error("switch case value is not a constant", switch_case.first);
                }
            }
        }

        if(defScopeInd == scope_ind) {
            switchInst->setDefaultDest(caseBlock);
        }

        scope_ind++;
    }

    if(end) {
        if (all_scopes_return && last_block) {
            end->eraseFromParent();
            gen.destroy_current_scope = false;
            if(!has_default_case()) {
                if(auto_default_case && caseBlock) {
                    switchInst->setDefaultDest(caseBlock);
                } else {
                    gen.error(
                            "A default case must be present when generating switch instruction or it must not be the last statement in the function", (ASTNode*) this);
                }
            }
        } else {
            gen.SetInsertPoint(end);
        }
    }

}

void SwitchStatement::code_gen(Codegen &gen, Scope* scope, unsigned int index) {
    code_gen(gen, index == scope->nodes.size() - 1);
}

#endif

Value* SwitchStatement::get_value_node() {
    return Value::get_first_value_from_value_node(this);
}

BaseType* SwitchStatement::create_type(ASTAllocator& allocator) {
    if(!is_value || scopes.empty()) return nullptr;
    auto last_val = get_value_node();
    return last_val ? last_val->create_type(allocator) : nullptr;
}

BaseType *SwitchStatement::known_type() {
    if(!is_value || scopes.empty()) return nullptr;
    auto last_val = get_value_node();
    return last_val ? last_val->known_type() : nullptr;
}

ASTNode *SwitchStatement::linked_node() {
    const auto known = known_type();
    return known ? known->linked_node() : nullptr;
}

VariantDefinition* SwitchStatement::getVarDefFromExpr() {
    const auto expr_type = expression->known_type();
    if(expr_type) {
        const auto linked = expr_type->linked_node();
        if(linked) {
            const auto kind = linked->kind();
            if(kind == ASTNodeKind::VariantMember) {
                const auto member = linked->as_variant_member_unsafe();
                return member->parent();
            } else if(kind == ASTNodeKind::VariantDecl) {
                return linked->as_variant_def_unsafe();
            } else if(kind == ASTNodeKind::GenericVariantDecl) {
                return linked->as_gen_variant_decl_unsafe()->master_impl;
            }
        }
    }
    return nullptr;
}


SwitchStatement* SwitchStatement::copy(ASTAllocator &allocator) {
    const auto stmt = new (allocator.allocate<SwitchStatement>()) SwitchStatement(
            expression->copy(allocator),
            parent(),
            is_value,
            ASTNode::encoded_location()
    );
    stmt->cases.reserve(cases.size());
    for(auto& aCase : cases) {
        const auto copied_case = aCase.first->copy(allocator);
        if(copied_case->kind() == ValueKind::VariantCase) {
            // since this is a variant case, we must ensure its linked properly
            const auto var_case = copied_case->as_variant_case_unsafe();
            // use the copied switch
            var_case->switch_statement = stmt;
            // copy the variables
            unsigned i = 0;
            const auto total_vars = var_case->identifier_list.size();
            while(i < total_vars) {
                auto& var = var_case->identifier_list[i];
                var = var->copy(allocator);
                var->set_parent(stmt);
                i++;
            }
        }
        stmt->cases.emplace_back(copied_case, aCase.second);
    }
    stmt->scopes.reserve(scopes.size());
    for(auto& scope : scopes) {
        stmt->scopes.emplace_back(scope.parent(), scope.encoded_location());
        scope.copy_into(stmt->scopes.back(), allocator, stmt);
    }
    return stmt;
}