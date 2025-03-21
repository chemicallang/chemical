// Copyright (c) Chemical Language Foundation 2025.

#include "SwitchStatement.h"
#include "ast/base/Value.h"
#include "ast/values/AccessChain.h"
#include "ast/values/FunctionCall.h"
#include "ast/values/VariantCaseVariable.h"
#include "ast/structures/VariantMember.h"
#include "ast/values/VariableIdentifier.h"
#include "compiler/SymbolResolver.h"
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

llvm::ConstantInt* write_variant_call_id_index(Codegen& gen, VariantDefinition* variant, VariableIdentifier* value) {
    const auto member = value->linked->as_variant_member();
    if(member) {
        return gen.builder->getInt32(variant->variable_index(member->name, false));
    } else {
        return gen.builder->getInt32(-1);
    }
}

llvm::ConstantInt* write_variant_call_call_index(Codegen& gen, VariantDefinition* variant, FunctionCall* value) {
    const auto member = value->parent_val->linked_node()->as_variant_member();
    if(member) {
        return gen.builder->getInt32(variant->variable_index(member->name, false));
    } else {
        return gen.builder->getInt32(-1);
    }
}

llvm::ConstantInt* write_variant_call_index(Codegen& gen, VariantDefinition* variant, Value* value) {
    switch(value->val_kind()) {
        case ValueKind::Identifier:
            return write_variant_call_id_index(gen, variant, value->as_identifier_unsafe());
        case ValueKind::FunctionCall:
            return write_variant_call_call_index(gen, variant, value->as_func_call_unsafe());
        case ValueKind::AccessChain: {
            const auto chain = value->as_access_chain_unsafe();
            if(chain) {
                return write_variant_call_index(gen, variant, chain->values.back());
            } else {
                return gen.builder->getInt32(-1);
            }
        }
        default:
            return gen.builder->getInt32(-1);
    }
}

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
                const auto def_type = variant_def->llvm_type(gen);
                std::vector<llvm::Value*> idxList { gen.builder->getInt32(0), gen.builder->getInt32(0) };
                const auto gep = gen.builder->CreateGEP(def_type, expr_value, idxList, "",gen.inbounds);
                const auto loadInst = gen.builder->CreateLoad(gen.builder->getInt32Ty(), gep, "");
                gen.di.instr(loadInst, expression);
                expr_value = loadInst;
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
                if(variant_def) {
                    switchInst->addCase(write_variant_call_index(gen, variant_def, switch_case.first), caseBlock);
                } else {
                    const auto caseValue =  switch_case.first->llvm_value(gen);
                    if(llvm::isa<llvm::ConstantInt>(caseValue)) {
                        const auto castedCase = implicit_cast_constant((llvm::ConstantInt*) caseValue, expr_type, expr_value->getType());
                        switchInst->addCase(castedCase, caseBlock);
                    } else {
                        gen.error("switch case value is not a constant", switch_case.first);
                    }
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

void link_variant_mem(SymbolResolver& resolver, VariantDefinition* var_def, VariableIdentifier* id) {
    id->linked = var_def->child(id->value);
    if(!id->linked) {
        resolver.error(id) << "couldn't find the variant member with name '" << id->value << "'";
    }
}

void link_variant_call(SymbolResolver& resolver, VariantDefinition* var_def, FunctionCall* call, SwitchStatement* stmt) {
    auto& astAlloc = *resolver.ast_allocator;
    const auto first_id = call->parent_val->as_identifier();
    if(first_id) {
        link_variant_mem(resolver, var_def, first_id);
        for(const auto value : call->values) {
            const auto id = value->as_identifier();
            if(id) {
                auto variable = new (astAlloc.allocate<VariantCaseVariable>()) VariantCaseVariable(id->value, first_id, stmt, 0);
                variable->declare_and_link(resolver, (ASTNode*&) (variable));
            } else {
                resolver.error("expected value to be a identifier", value);
            }
        }
    } else {
        resolver.error("expected first value in the function call to be identifier", call->parent_val);
    }
}

bool SwitchStatement::declare_and_link(SymbolResolver &linker, Value** value_ptr) {
    VariantDefinition* variant_def = nullptr;
    bool result = true;
    if(expression->link(linker, expression)) {
        const auto expr_type = expression->known_type();
        if(expr_type) {
            const auto linked = expr_type->linked_node();
            if(linked) {
                const auto kind = linked->kind();
                if(kind == ASTNodeKind::VariantMember) {
                    const auto member = linked->as_variant_member_unsafe();
                    variant_def = member->parent();
                } else if(kind == ASTNodeKind::VariantDecl) {
                    variant_def = linked->as_variant_def_unsafe();
                }
                if (value_ptr && variant_def && (scopes.size() < variant_def->variables().size() && !has_default_case())) {
                    linker.error("expected all cases of variant in switch statement when no default case is specified", (ASTNode*) this);
                    return false;
                }
            }
        }
    } else {
        result = false;
    }

    std::vector<VariableIdentifier*> moved_ids;
    std::vector<AccessChain*> moved_chains;

    unsigned i = 0;
    const auto scopes_size = scopes.size();
    while(i < scopes_size) {
        auto& scope = scopes[i];
        linker.scope_start();
        for(auto& switch_case : cases) {
            if(switch_case.second == i && switch_case.first) {
                if(variant_def) {
                    // it's a variant definition, declare the identifier list
                    // link with the case
                    const auto case_kind = switch_case.first->val_kind();
                    switch(case_kind) {
                        case ValueKind::Identifier:
                            link_variant_mem(linker, variant_def, switch_case.first->as_identifier_unsafe());
                            continue;
                        case ValueKind::FunctionCall:
                            link_variant_call(linker, variant_def, switch_case.first->as_func_call_unsafe(), this);
                            continue;
                        case ValueKind::AccessChain:{
                            const auto chain = switch_case.first->as_access_chain_unsafe();
                            if(chain && chain->values.size() == 1) {
                                const auto kind = chain->values.back()->val_kind();
                                if(kind == ValueKind::FunctionCall) {
                                    link_variant_call(linker, variant_def, chain->values.back()->as_func_call_unsafe(), this);
                                } else if(kind == ValueKind::Identifier) {
                                    link_variant_mem(linker, variant_def, chain->values.back()->as_identifier_unsafe());
                                } else {
                                    linker.error("unknown value in switch when resolving variant cases", switch_case.first);
                                }
                            } else {
                                linker.error("unknown value in switch when resolving variant cases", switch_case.first);
                            }
                            continue;
                        }
                        default:
                            linker.error("unknown value in switch when resolving variant cases", switch_case.first);
                    }
                } else {
                    // link the switch case value
                    switch_case.first->link(linker, switch_case.first);
                }
            }
        }
        linker.link_body_seq_backing_moves(scope, moved_ids, moved_chains);
        linker.scope_end();
        i++;
    }

    // restoring all the moved identifiers and chains, in all the scopes
    const auto curr_func = linker.current_func_type;
    curr_func->restore_moved_ids(moved_ids);
    curr_func->restore_moved_chains(moved_chains);

    if(result && value_ptr) {
        auto val_node = get_value_node();
        if(!val_node) {
            linker.error("expected a single value node for the value", (ASTNode*) this);
            return false;
        }
    }

    return result;
}