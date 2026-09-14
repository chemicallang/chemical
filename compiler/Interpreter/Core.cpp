// Copyright (c) Chemical Language Foundation 2025.

#include "ast/base/InterpretScope.h"
#include "ast/base/TypeBuilder.h"
#include "ast/base/ASTNode.h"
#include "ast/statements/Assignment.h"
#include "ast/statements/Break.h"
#include "ast/statements/Continue.h"
#include "ast/statements/VarInit.h"
#include "ast/statements/AccessChainNode.h"
#include "ast/types/FunctionType.h"
#include "ast/structures/ForLoop.h"
#include "ast/structures/ForInLoop.h"
#include "ast/structures/BlockScope.h"
#include "ast/values/IntNumValue.h"
#include "ast/values/StringValue.h"
#include "ast/values/ArrayValue.h"
#include "ast/types/LinkedType.h"
#include "ast/values/PointerValue.h"
#include "ast/values/FunctionCall.h"
#include "ast/values/ReferenceOfValue.h"
#include "ast/values/AddrOfValue.h"
#include "ast/types/PointerType.h"
#include "ast/values/BoolValue.h"
#include "ast/values/FloatValue.h"
#include "ast/values/DoubleValue.h"
#include "ast/structures/StructDefinition.h"
#include "ast/structures/UnnamedStruct.h"
#include "ast/structures/UnnamedUnion.h"
#include "ast/structures/ImplDefinition.h"
#include "ast/structures/FileScope.h"
#include "ast/structures/Scope.h"
#include "ast/structures/Namespace.h"
#include "compiler/lab/LabBuildCompiler.h"
#include "ast/statements/Typealias.h"
#include "ast/statements/Return.h"
#include "ast/values/SwitchValue.h"
#include "ast/values/IfValue.h"
#include "ast/values/PatternMatchExpr.h"
#include "ast/values/ValueNode.h"
#include "ast/statements/ValueWrapperNode.h"
#include "ast/statements/IncDecNode.h"
#include "ast/statements/PatternMatchExprNode.h"
#include "ast/statements/PlacementNewNode.h"
#include "ast/structures/DoWhileLoop.h"
#include "ast/structures/WhileLoop.h"
#include "ast/structures/If.h"
#include "ast/base/LoopASTNode.h"
#include "ast/values/VariantCase.h"
#include "ast/values/VariantCaseVariable.h"
#include "ast/values/StructValue.h"
#include "ast/values/AccessChain.h"
#include "ast/structures/VariantMember.h"
#include "ast/structures/VariantDefinition.h"
#include "ast/statements/ProvideStmt.h"
#include "ast/structures/LoopBlock.h"
#include "ast/values/LoopValue.h"
#include "ast/structures/UnsafeBlock.h"
#include "ast/statements/DestructStmt.h"
#include "ast/types/PointerType.h"


// Walk up the scope chain to the nearest loop frame, stopping interpretation of
// every scope we pass through (the current scope included). The loop frame
// itself is left running so it can observe the signal. If no loop frame is
// active (e.g. `break` outside a loop), the current scope is stopped.
static void raise_loop_signal(InterpretScope& scope, int signal) {
    InterpretScope* s = &scope;
    while(s && !s->is_loop_scope) {
        s->stopInterpretation = true;
        s = s->parent;
    }
    if(s) {
        s->loop_signal = signal;
    } else {
        scope.stopInterpretation = true;
    }
}

static void raise_loop_break(InterpretScope& scope) {
    raise_loop_signal(scope, 1);
}

static void raise_loop_continue(InterpretScope& scope) {
    raise_loop_signal(scope, 2);
}

Value* evaluate(InterpretScope& scope, Scope* body);

inline void interpret(InterpretScope& scope, BreakStatement* stmt) {
    if(stmt->value) {
        // Store the break value on the nearest enclosing loop-value scope so it
        // survives destruction of intermediate child scopes (if bodies, switch
        // cases) and is not clobbered by nested loops. Falls back to the global
        // scope only if no loop-value scope is active.
        auto v = stmt->value->evaluated_value(scope);
        InterpretScope* target = &scope;
        while(target && !target->is_loop_value_scope) {
            target = target->parent;
        }
        if(target) {
            target->loop_break_value = v;
        } else {
            scope.global->loop_break_value = v;
        }
    }
    raise_loop_break(scope);
}

inline void interpret(InterpretScope& scope, ContinueStatement* stmt) {
    raise_loop_continue(scope);
}

inline void interpret(InterpretScope& scope, AssignStatement* assign) {
    // Handle compound assignments on struct types by dispatching the operator overload
    // (e.g., add_assign for +=) before the general set_value() fallback.
    if(assign->assOp != Operation::Assignment) {
        auto lhsType = assign->lhs->getType();
        if(lhsType) {
            auto canon = lhsType->canonical();
            ExtendableMembersContainerNode* container = nullptr;
            if(canon->kind() == BaseTypeKind::Linked) {
                auto linked = canon->as_linked_type_unsafe()->linked;
                if(linked && (linked->kind() == ASTNodeKind::StructDecl ||
                              linked->kind() == ASTNodeKind::VariantDecl ||
                              linked->kind() == ASTNodeKind::UnionDecl)) {
                    container = (ExtendableMembersContainerNode*)linked;
                }
            }
            if(container) {
                auto glob = scope.global;
                if(glob && glob->build_compiler) {
                    const char* opName = nullptr;
                    switch(assign->assOp) {
                        case Operation::Addition: opName = "add_assign"; break;
                        case Operation::Subtraction: opName = "sub_assign"; break;
                        case Operation::Multiplication: opName = "mul_assign"; break;
                        case Operation::Division: opName = "div_assign"; break;
                        case Operation::Modulus: opName = "rem_assign"; break;
                        case Operation::LeftShift: opName = "shl_assign"; break;
                        case Operation::RightShift: opName = "shr_assign"; break;
                        case Operation::BitwiseAND: opName = "bitand_assign"; break;
                        case Operation::BitwiseXOR: opName = "bitxor_assign"; break;
                        case Operation::BitwiseOR: opName = "bitor_assign"; break;
                        default: break;
                    }
                    if(opName) {
                        FunctionDeclaration* overloaded = nullptr;
                        chem::string_view nameSv(opName);
                        ASTNode* searchNode = container->parent();
                        while(searchNode) {
                            std::vector<ASTNode*>* nodes = nullptr;
                            if(searchNode->kind() == ASTNodeKind::NamespaceDecl) {
                                nodes = &searchNode->as_namespace_unsafe()->nodes;
                            } else if(searchNode->kind() == ASTNodeKind::FileScope) {
                                nodes = &static_cast<FileScope*>(searchNode)->body.nodes;
                            } else if(searchNode->kind() == ASTNodeKind::Scope) {
                                nodes = &static_cast<Scope*>(searchNode)->nodes;
                            }
                            if(nodes) {
                                for(auto child : *nodes) {
                                    if(!child || child->kind() != ASTNodeKind::ImplDecl) continue;
                                    auto impl = child->as_impl_def_unsafe();
                                    if(!impl || !impl->struct_type) continue;
                                    auto implStruct = impl->struct_type->get_direct_linked_canonical_node();
                                    bool matches = (implStruct == container);
                                    if(!matches && implStruct && container &&
                                        implStruct->get_node_identifier() == container->get_node_identifier()) {
                                        matches = true;
                                    }
                                    if(!matches) continue;
                                    auto fn = impl->direct_child_function(nameSv);
                                    if(fn && fn->body.has_value()) {
                                        overloaded = fn;
                                        break;
                                    }
                                }
                                if(overloaded) break;
                            }
                            searchNode = searchNode->parent();
                        }
                        if(overloaded) {
                            auto rhsVal = assign->value->evaluated_value(scope);
                            if(!rhsVal) return;
                            Value* selfVal = nullptr;
                            if(assign->lhs->val_kind() == ValueKind::Identifier) {
                                auto lhsId = assign->lhs->as_identifier_unsafe();
                                auto lhsIt = scope.find_value_iterator(lhsId->value);
                                if(lhsIt.first != lhsIt.second.values.end()) {
                                    selfVal = lhsIt.first->second;
                                }
                            } else {
                                selfVal = assign->lhs->evaluated_value(scope);
                            }
                            if(!selfVal) return;
                            const auto prev_func = glob->current_func_type;
                            glob->current_func_type = overloaded;
                            glob->call_stack.emplace_back(nullptr);
                            InterpretScope fn_scope(glob, glob->allocator, glob);
                            std::vector<Value*> opArgs = { rhsVal };
                            auto result = overloaded->call(&scope, opArgs, selfVal, &fn_scope, true, assign->value);
                            glob->call_stack.pop_back();
                            glob->current_func_type = prev_func;
                            return;
                        }
                    }
                }
            }
        }
    }
    // Handle assignment with move semantics:
    // 1. Save the old LHS value (must NOT destruct before set_value, because
    //    the RHS may take a pointer to the LHS — e.g. x = f(x.get_ptr(), N)).
    // 2. Perform the assignment (set_value evaluates RHS, overwrites LHS).
    // 3. Destruct the old LHS value (safe now — set_value already resolved RHS).
    // 4. Clear the RHS source (move, not copy) so it's not destructed twice.
    Value* oldLhsVal = nullptr;
    if(assign->assOp == Operation::Assignment) {
        if(assign->is_first_init) {
            // First initialization of a previously uninitialized variable: there is
            // no previous value to destroy (it would be garbage / uninitialized memory).
        } else if(assign->lhs->val_kind() == ValueKind::Identifier) {
            auto lhsId = assign->lhs->as_identifier_unsafe();
            auto lhsIt = scope.find_value_iterator(lhsId->value);
            if(lhsIt.first != lhsIt.second.values.end()) {
                oldLhsVal = lhsIt.first->second;
            }
        }
    }
    // Evaluate the RHS exactly once. We pass the evaluated value to set_value
    // (instead of the AST node) so that side-effecting expressions — function
    // calls, access chains, index operators — are not executed twice, and we
    // reuse the same value as the move source below. The compiler may replace
    // identifier AST nodes with resolved values, so resolving by kind is not
    // reliable; evaluating here is the single authoritative evaluation.
    Value* rhsEvaluated = nullptr;
    Value* rhsForSet = assign->value;
    if(assign->assOp == Operation::Assignment) {
        rhsEvaluated = assign->value->evaluated_value(scope);
        if(rhsEvaluated) {
            rhsForSet = rhsEvaluated;
        }
    }
    assign->lhs->set_value(scope, rhsForSet, assign->assOp, assign->encoded_location());
    if(oldLhsVal) {
        scope.destroy_value(oldLhsVal);
    }
    // Clear the RHS source (move semantics) using the single evaluated value.
    if(assign->assOp == Operation::Assignment) {
        chem::string_view lhsName;
        if(assign->lhs->val_kind() == ValueKind::Identifier) {
            lhsName = assign->lhs->as_identifier_unsafe()->value;
        }
        if(rhsEvaluated) {
            scope.move_clear_source(rhsEvaluated, lhsName);
        }
    }
}

void interpret(InterpretScope& scope, ForLoop* loop) {
    InterpretScope loop_scope(&scope, scope.allocator, scope.global);
    loop_scope.is_loop_scope = true;
    loop_scope.interpret(loop->initializer);
    while (loop->conditionExpr->evaluated_bool(loop_scope)) {
        {
            InterpretScope body_scope(&loop_scope, scope.allocator, scope.global);
            body_scope.interpret(&loop->body);
        } // body_scope destroyed here, calling destructors on iteration-local variables
        if (loop_scope.stopInterpretation) {
            // A `return` unwound through this loop; leave the flag set.
            break;
        }
        if (loop_scope.loop_signal == 1) { // break
            loop_scope.loop_signal = 0;
            break;
        }
        loop_scope.loop_signal = 0; // continue or normal
        loop_scope.interpret(loop->incrementerExpr);
    }
}

void interpret(InterpretScope& scope, ForInLoop* loop) {
    const auto evalExpr = loop->expr->evaluated_value(scope);
    if(!evalExpr) {
        scope.error("couldn't evaluate for-in expression", loop->expr);
        return;
    }

    InterpretScope loop_scope(&scope, scope.allocator, scope.global);
    loop_scope.is_loop_scope = true;

    // Stops the loop on `break` or `return`; `continue` resets the signal and
    // lets the next iteration proceed.
    auto should_stop = [&]() -> bool {
        if(loop_scope.stopInterpretation) return true;
        if(loop_scope.loop_signal == 1) {
            loop_scope.loop_signal = 0;
            return true;
        }
        loop_scope.loop_signal = 0;
        return false;
    };

    const auto kind = evalExpr->val_kind();

    if(kind == ValueKind::String) {
        const auto strVal = evalExpr->as_string_unsafe();
        const auto& str = strVal->value;

        for(uint64_t i = 0; i < str.size(); i++) {
            InterpretScope child(&loop_scope, scope.allocator, scope.global);

            // declare the loop variable (char)
            auto charVal = new (scope.allocate<IntNumValue>()) IntNumValue(
                (uint64_t)(unsigned char)str[i],
                scope.global->typeBuilder.getCharType(),
                ZERO_LOC
            );
            child.declare(loop->id, charVal);

            // declare the index variable if present
            if(loop->index_init) {
                auto indexVal = new (scope.allocate<IntNumValue>()) IntNumValue(
                    i,
                    (IntNType*) (BaseType*) loop->index_init->type,
                    ZERO_LOC
                );
                child.declare(loop->index_init->name_view(), indexVal);
            }

            child.interpret(&loop->body);

            if(should_stop()) break;
        }
    } else if(kind == ValueKind::ArrayValue) {
        const auto arrVal = evalExpr->as_array_value_unsafe();

        for(uint64_t i = 0; i < arrVal->values.size(); i++) {
            InterpretScope child(&loop_scope, scope.allocator, scope.global);

            const auto elemVal = arrVal->values[i]->evaluated_value(scope);
            if(elemVal) {
                child.declare(loop->id, elemVal);
            }

            // declare the index variable if present
            if(loop->index_init) {
                auto indexVal = new (scope.allocate<IntNumValue>()) IntNumValue(
                    i,
                    (IntNType*) (BaseType*) loop->index_init->type,
                    ZERO_LOC
                );
                child.declare(loop->index_init->name_view(), indexVal);
            }

            child.interpret(&loop->body);

            if(should_stop()) break;
        }
    } else if(loop->iteration_kind == ForInLoopIterationKind::Linear) {
        if(evalExpr->val_kind() != ValueKind::StructValue) {
            scope.error("for-in loop with Linear iteration expects a struct value", loop->expr);
            return;
        }
        auto structVal = evalExpr->as_struct_value_unsafe();

        bool reversed = loop->is_reversed();

        auto dataChild = structVal->child(scope, "_data");
        if(!dataChild) {
            scope.error("for-in loop with Linear iteration: struct must have _data field", loop->expr);
            return;
        }

        auto sizeChild = structVal->child(scope, "_size");
        if(!sizeChild) {
            scope.error("for-in loop with Linear iteration: struct must have _size field", loop->expr);
            return;
        }
        auto sizeNum = sizeChild->get_number();
        if(!sizeNum.has_value()) {
            scope.error("for-in loop with Linear iteration: _size must be a numeric value", loop->expr);
            return;
        }
        uint64_t size = sizeNum.value();
        if(size == 0) return;

        // If _data is a PointerValue backed by an ArrayValue (from array-to-pointer
        // decay), unwrap it to the underlying array so the existing ArrayValue
        // iteration logic handles struct elements correctly.
        if(dataChild->val_kind() == ValueKind::PointerValue) {
            auto pv = (PointerValue*)dataChild;
            if(pv->backingArray) {
                dataChild = pv->backingArray;
            }
        }
        if(dataChild->val_kind() == ValueKind::ArrayValue) {
            auto arrayVal = (ArrayValue*)dataChild;
            BaseType* elemType = arrayVal->known_elem_type();
            uint64_t elemSize = elemType->byte_size(scope.global->target_data);

            bool stopped = false;
            auto iterate = [&](uint64_t idx) {
                if(idx >= arrayVal->values.size()) return;
                InterpretScope child_scope(&loop_scope, scope.allocator, scope.global);
                if(loop->is_reference()) {
                    if(arrayVal->contiguousData) {
                        auto elemPtr = new (scope.allocate<PointerValue>()) PointerValue(
                            (void*)((uint8_t*)arrayVal->contiguousData + idx * elemSize),
                            elemType, 0, elemSize, loop->encoded_location()
                        );
                        child_scope.declare(loop->id, elemPtr);
                    } else {
                        child_scope.declare(loop->id, arrayVal->values[idx]);
                    }
                } else {
                    child_scope.declare(loop->id, arrayVal->values[idx]);
                }
                if(loop->index_init) {
                    auto indexVal = new (scope.allocate<IntNumValue>()) IntNumValue(
                        idx,
                        (IntNType*)(BaseType*)loop->index_init->type,
                        ZERO_LOC
                    );
                    child_scope.declare(loop->index_init->name_view(), indexVal);
                }
                child_scope.interpret(&loop->body);
                if(should_stop()) stopped = true;
            };

            if(reversed) {
                for(uint64_t idx = size; idx-- > 0; ) {
                    iterate(idx);
                    if(stopped) break;
                }
            } else {
                for(uint64_t idx = 0; idx < size; idx++) {
                    iterate(idx);
                    if(stopped) break;
                }
            }
        } else if(dataChild->val_kind() == ValueKind::PointerValue) {
            auto dataPtr = (PointerValue*)dataChild;

            auto ptrType = dataPtr->getType();
            BaseType* elemType = ptrType;
            if(ptrType && ptrType->kind() == BaseTypeKind::Pointer) {
                elemType = ptrType->as_pointer_type_unsafe()->type;
            }
            uint64_t elemSize = elemType->byte_size(scope.global->target_data);

            if(reversed) {
                for(uint64_t idx = size; idx-- > 0; ) {
                    InterpretScope child_scope(&loop_scope, scope.allocator, scope.global);
                    auto elemPtr = new (scope.allocate<PointerValue>()) PointerValue(
                        (void*)((uint8_t*)dataPtr->data + idx * elemSize),
                        elemType,
                        0,
                        elemSize,
                        loop->encoded_location()
                    );
                    if(loop->is_reference()) {
                        child_scope.declare(loop->id, elemPtr);
                    } else {
                        auto elemVal = elemPtr->deref(scope, loop->encoded_location(), loop->expr);
                    if(elemVal) {
                        child_scope.declare(loop->id, elemVal);
                    }
                }
                if(loop->index_init) {
                    auto indexVal = new (scope.allocate<IntNumValue>()) IntNumValue(
                        idx,
                        (IntNType*)(BaseType*)loop->index_init->type,
                        ZERO_LOC
                    );
                    child_scope.declare(loop->index_init->name_view(), indexVal);
                }
                child_scope.interpret(&loop->body);
                if(should_stop()) break;
            }
        } else {
            for(uint64_t idx = 0; idx < size; idx++) {
                InterpretScope child_scope(&loop_scope, scope.allocator, scope.global);
                auto elemPtr = new (scope.allocate<PointerValue>()) PointerValue(
                    (void*)((uint8_t*)dataPtr->data + idx * elemSize),
                    elemType,
                    0,
                    elemSize,
                    loop->encoded_location()
                );
                if(loop->is_reference()) {
                    child_scope.declare(loop->id, elemPtr);
                } else {
                    auto elemVal = elemPtr->deref(scope, loop->encoded_location(), loop->expr);
                    if(elemVal) {
                        child_scope.declare(loop->id, elemVal);
                    }
                }
                if(loop->index_init) {
                    auto indexVal = new (scope.allocate<IntNumValue>()) IntNumValue(
                        idx,
                        (IntNType*)(BaseType*)loop->index_init->type,
                        ZERO_LOC
                    );
                    child_scope.declare(loop->index_init->name_view(), indexVal);
                }
                child_scope.interpret(&loop->body);
                if(should_stop()) break;
            }
        }
        }
    } else {
        scope.error(loop->expr) << "for-in loop not supported for expression type (kind: " << std::to_string(static_cast<int>(kind)) << ") in comptime";
    }
}

void interpret(InterpretScope& scope, ReturnStatement* stmt) {
    // set_return() records the return value on the function-level scope and
    // propagates stopInterpretation up the scope chain (including active loop
    // frames), which unwinds every nested scope.
    scope.global->current_func_type->set_return(scope, stmt->value);
}

Scope* eval_switch_stmt_block(InterpretScope& scope, SwitchStatement* stmt, Value* cond) {
    if(!cond) {
        scope.error("couldn't evaluate the expression", stmt->expression);
        return nullptr;
    }

    // Check if this is a variant switch — match by member index instead of value equality.
    const auto variantDef = stmt->getVarDefFromExpr();
    
    // Determine the active member index from the condition (the variant struct value).
    int condMemberIndex = -1;
    Value* derefCond = cond;
    if(cond->val_kind() == ValueKind::StructValue) {
        auto structVal = cond->as_struct_value_unsafe();
        condMemberIndex = (int)structVal->get_variant_member_index();
    } else if(cond->val_kind() == ValueKind::PointerValue) {
        auto ptrVal = static_cast<PointerValue*>(cond);
        auto deref = ptrVal->deref(scope, cond->encoded_location(), cond);
        if(deref && deref->val_kind() == ValueKind::StructValue) {
            derefCond = deref;
            condMemberIndex = (int)deref->as_struct_value_unsafe()->get_variant_member_index();
        }
    } else if(cond->val_kind() == ValueKind::Identifier) {
        auto eval = cond->evaluated_value(scope);
        if(eval && eval->val_kind() == ValueKind::StructValue) {
            derefCond = eval;
            condMemberIndex = (int)eval->as_struct_value_unsafe()->get_variant_member_index();
        }
    }
    
    if(condMemberIndex >= 0) {
        unsigned i = 0;
        const auto size = stmt->scopes.size();
        while(i < size) {
            for(auto& casePair : stmt->cases) {
                if(casePair.second == i && casePair.first) {
                    // VariantCase carries the VariantMember pointer
                    if(casePair.first->val_kind() == ValueKind::VariantCase) {
                        auto varCase = casePair.first->as_variant_case_unsafe();
                        auto member = varCase->member;
                        if(member) {
                            auto memberParent = member->parent();
                            int caseMemberIndex = (int)memberParent->direct_child_index(member->name_view());
                            if(caseMemberIndex == condMemberIndex) {
                                return &stmt->scopes[i];
                            }
                        }
                    }
                }
            }
            i++;
        }
        if(stmt->has_default_case()) {
            return &stmt->scopes[stmt->defScopeInd];
        }
        return nullptr;
    }
    
    // Fall through to normal matching if we couldn't determine the member index

    unsigned i = 0;
    const auto size = stmt->scopes.size();
    while(i < size) {
        for(auto& casePair : stmt->cases) {
            if(casePair.second == i && casePair.first) {
                // VariantCase matching: match by member index instead of value equality.
                // Handles StructValue, Identifier, and PointerValue condition types.
                if(casePair.first->val_kind() == ValueKind::VariantCase) {
                    Value* effectiveCond = cond;
                    if(effectiveCond->val_kind() == ValueKind::Identifier) {
                        effectiveCond = effectiveCond->evaluated_value(scope);
                    } else if(effectiveCond->val_kind() == ValueKind::PointerValue) {
                        // Dereference pointer to get the underlying struct
                        auto ptrVal = static_cast<PointerValue*>(effectiveCond);
                        auto deref = ptrVal->deref(scope, cond->encoded_location(), cond);
                        if(deref) effectiveCond = deref;
                    }
                    if(effectiveCond && effectiveCond->val_kind() == ValueKind::StructValue) {
                        auto varCase = casePair.first->as_variant_case_unsafe();
                        if(varCase && varCase->member) {
                            auto condStruct = effectiveCond->as_struct_value_unsafe();
                            int condIdx = (int)condStruct->get_variant_member_index();
                            auto memberParent = varCase->member->parent();
                            int caseIdx = (int)memberParent->direct_child_index(varCase->member->name_view());
                            if(condIdx == caseIdx) {
                                return &stmt->scopes[i];
                            }
                            // Fallback: check if case member's parameter names are in the struct.
                            // Only applies when the case has parameters (otherwise can't distinguish).
                            if(condIdx < 0 && !varCase->identifier_list.empty()) {
                                bool allParamsMatch = true;
                                for(auto& caseVar : varCase->identifier_list) {
                                    if(caseVar->member_param) {
                                        if(condStruct->values.find(caseVar->member_param->name) == condStruct->values.end()) {
                                            allParamsMatch = false;
                                            break;
                                        }
                                    }
                                }
                                if(allParamsMatch) {
                                    return &stmt->scopes[i];
                                }
                            }
                        }
                    }
                }
                auto eval_first = casePair.first->evaluated_value(scope);
                const auto isEqualEval = scope.evaluate(Operation::IsEqual, eval_first, cond, ZERO_LOC, casePair.first);
                if(!isEqualEval || isEqualEval->val_kind() == ValueKind::NullValue) {
                    // evaluate returned null (unknown operation) — skip this case
                    continue;
                }
                if(isEqualEval->val_kind() == ValueKind::Bool && isEqualEval->get_the_bool()) {
                    return &stmt->scopes[i];
                }
            }
        }
        i++;
    }
    if(stmt->has_default_case()) {
        return &stmt->scopes[stmt->defScopeInd];
    }
    return nullptr;
}

/**
 * Helper: declare variant case variables from the matched variant case
 * into the given interpret scope. Returns a vector of declared variable names
 * so the caller can erase them after the body is interpreted, preventing
 * double-destruction when the variant struct's destructor cleans up the shared value.
 */
static std::vector<chem::string_view> declare_variant_case_vars(
    InterpretScope& scope,
    SwitchStatement* stmt,
    Scope* body,
    Value* cond
) {
    std::vector<chem::string_view> declared;
    if(!cond) return declared;
    Value* effectiveCond = cond;
    // Unwrap Identifier and PointerValue to get the underlying struct
    if(effectiveCond->val_kind() == ValueKind::Identifier) {
        effectiveCond = effectiveCond->evaluated_value(scope);
    }
    if(effectiveCond && effectiveCond->val_kind() == ValueKind::PointerValue) {
        auto ptrVal = static_cast<PointerValue*>(effectiveCond);
        effectiveCond = ptrVal->deref(scope, cond->encoded_location(), cond);
    }
    if(!effectiveCond || effectiveCond->val_kind() != ValueKind::StructValue) return declared;
    auto condStruct = effectiveCond->as_struct_value_unsafe();
    const auto variantDef = stmt->getVarDefFromExpr();
    if(!variantDef) return declared;
    for(auto& casePair : stmt->cases) {
        if(casePair.first && casePair.first->val_kind() == ValueKind::VariantCase) {
            if(&stmt->scopes[casePair.second] == body) {
                auto varCase = casePair.first->as_variant_case_unsafe();
                for(auto& caseVar : varCase->identifier_list) {
                    // Use the actual variant parameter name, not the user-chosen case label name
                    auto paramName = caseVar->member_param ? caseVar->member_param->name : caseVar->name;
                    auto found = condStruct->values.find(paramName);
                    if(found != condStruct->values.end() && found->second.value) {
                        scope.declare(caseVar->name, found->second.value);
                        declared.push_back(caseVar->name);
                    }
                }
                break;
            }
        }
    }
    return declared;
}

void interpret(InterpretScope& scope, SwitchStatement* stmt) {
    // Evaluate the switch subject exactly once. It is shared by case selection
    // and variant-case variable declaration.
    const auto cond = stmt->expression->evaluated_value(scope);
    const auto body = eval_switch_stmt_block(scope, stmt, cond);
    if(body) {
        // Use a child scope for variant switch cases to keep variable declarations clean
        InterpretScope child(&scope, scope.allocator, scope.global);
        std::vector<chem::string_view> caseVarNames;
        caseVarNames = declare_variant_case_vars(child, stmt, body, cond);
        child.interpret(body);
        // Erase pattern-matched case variables from child scope to prevent
        // double-destruction. These variables share pointers with the variant
        // struct's internal values; when the child scope is destroyed, the
        // shared structs would be destructed again when the variant goes out
        // of scope. By erasing from the child scope, we let the variant's
        // destructor (called when the outer scope ends) handle cleanup.
        for(auto& varName : caseVarNames) {
            child.values.erase(varName);
        }
    }
}

Value* evaluated_value(InterpretScope &scope, SwitchStatement* stmt) {
    // Evaluate the switch subject exactly once.
    const auto cond = stmt->expression->evaluated_value(scope);
    const auto body = eval_switch_stmt_block(scope, stmt, cond);
    if(body) {
        // For variant switches, need to declare case variables before evaluating
        declare_variant_case_vars(scope, stmt, body, cond);
        return evaluate(scope, body);
    }
    return scope.getNullValue();
}

Scope* pm_eval_body_for_if(InterpretScope& scope, IfStatement* stmt) {
    if(stmt->condition->val_kind() == ValueKind::PatternMatchExpr) {
        auto patt = static_cast<PatternMatchExpr*>(stmt->condition);
        auto exprVal = patt->expression->evaluated_value(scope);
        bool matches = patt->evaluated_bool(scope);
        if(matches) {
            return &stmt->ifBody;
        }
        for(auto const &elseIf : stmt->elseIfs) {
            if(elseIf.first->val_kind() == ValueKind::PatternMatchExpr) {
                auto elsePatt = static_cast<PatternMatchExpr*>(elseIf.first);
                exprVal = elsePatt->expression->evaluated_value(scope);
                matches = elsePatt->evaluated_bool(scope);
                if(matches) {
                    return const_cast<Scope *>(&elseIf.second);
                }
            } else if(elseIf.first->evaluated_bool(scope)) {
                return const_cast<Scope *>(&elseIf.second);
            }
        }
        if(stmt->elseBody.has_value()) {
            return &stmt->elseBody.value();
        }
        return nullptr;
    }
    if(stmt->condition->evaluated_bool(scope)) {
        return &stmt->ifBody;
    } else {
        for(auto const &elseIf : stmt->elseIfs) {
            if(elseIf.first->val_kind() == ValueKind::PatternMatchExpr) {
                auto elsePatt = static_cast<PatternMatchExpr*>(elseIf.first);
                auto exprVal = elsePatt->expression->evaluated_value(scope);
                bool matches = elsePatt->evaluated_bool(scope);
                if(matches) {
                    return const_cast<Scope *>(&elseIf.second);
                }
            } else if(elseIf.first->evaluated_bool(scope)) {
                return const_cast<Scope *>(&elseIf.second);
            }
        }
        if(stmt->elseBody.has_value()) {
            return &stmt->elseBody.value();
        }
    }
    return nullptr;
}

static void pm_declare_vars_from_patt(InterpretScope& scope, InterpretScope& child, PatternMatchExpr* patt) {
    if(patt->param_names.empty() || !patt->member) return;
    auto exprVal = patt->expression->evaluated_value(scope);
    // the matched expression may be a reference stored inside a struct member
    // (e.g. h.value where value : &Variant) — dereference it to get the variant
    if(exprVal && exprVal->val_kind() == ValueKind::PointerValue) {
        auto deref = ((PointerValue*) exprVal)->deref(scope, patt->encoded_location(), exprVal);
        if(deref) {
            exprVal = deref;
        }
    }
    if(exprVal && exprVal->val_kind() == ValueKind::StructValue) {
        auto vs = exprVal->as_struct_value_unsafe();
        for(auto& pm : patt->param_names) {
            if(pm->member_param) {
                auto found = vs->values.find(pm->member_param->name);
                if(found != vs->values.end() && found->second.value) {
                    child.declare(pm->identifier, found->second.value);
                    vs->values.erase(found);
                }
            }
        }
    }
}

void interpret(InterpretScope& scope, IfStatement* stmt) {
    const auto block = pm_eval_body_for_if(scope, stmt);
    if(block) {
        InterpretScope child(&scope, scope.allocator, scope.global);
        if(stmt->condition->val_kind() == ValueKind::PatternMatchExpr) {
            pm_declare_vars_from_patt(scope, child, static_cast<PatternMatchExpr*>(stmt->condition));
        }
        child.interpret(block);
    }
}

Value* evaluated_value(InterpretScope &scope, IfStatement* stmt) {
    const auto body = pm_eval_body_for_if(scope, stmt);
    if(body) {
        InterpretScope child(&scope, scope.allocator, scope.global);
        if(stmt->condition->val_kind() == ValueKind::PatternMatchExpr) {
            pm_declare_vars_from_patt(scope, child, static_cast<PatternMatchExpr*>(stmt->condition));
        }
        return evaluate(child, body);
    }
    return scope.getNullValue();
}

inline void interpret(InterpretScope& scope, ValueWrapperNode* node) {
    // Destruct temp structs from bare expressions
    if(node->value->val_kind() == ValueKind::FunctionCall) {
        auto call = node->value->as_func_call_unsafe();
        auto result = node->value->evaluated_value(scope);
        scope.destroy_value(result);
        // Also destruct temps created by arguments wrapped in ReferenceOfValue/AddrOfValue.
        // E.g. take_gen_destruct_ref(&create_short_gen_dest(...)) — the inner FunctionCall
        // creates a temp struct that must be destructed after the outer call completes.
                for(auto arg : call->values) {
                    if(arg->val_kind() == ValueKind::ReferenceOfValue) {
                        auto refOf = arg->as_reference_of_value_unsafe();
                        if(refOf->innerEvaluatedResult) {
                            scope.destroy_value(refOf->innerEvaluatedResult);
                        }
                    } else if(arg->val_kind() == ValueKind::AddrOfValue) {
                        auto addrOf = arg->as_addr_of_value_unsafe();
                        if(addrOf->innerEvaluatedResult) {
                            scope.destroy_value(addrOf->innerEvaluatedResult);
                        }
                    }
                }
    } else if(node->value->val_kind() == ValueKind::AccessChain) {
        auto chain = node->value->as_access_chain_unsafe();
        if(chain->values.empty()) return;
        // Same temp tracking as AccessChainNode::interpret
        std::vector<Value*> temps;
        Value* current = nullptr;
        for(unsigned i = 0; i < chain->values.size(); i++) {
            auto& val = chain->values[i];
            if(val->val_kind() == ValueKind::Identifier) {
                auto id = val->as_identifier_unsafe();
                current = current ? current->child(scope, id->value) : id->evaluated_value(scope);
            } else {
                current = val->evaluated_value(scope);
                if(val->val_kind() == ValueKind::FunctionCall && current) {
                    temps.push_back(current);
                }
            }
        }
        for(auto t : temps) {
            scope.destroy_value(t);
        }
    } else {
        node->value->evaluated_value(scope);
    }
}


inline void interpret(InterpretScope& scope, AccessChainNode* node) {
    auto& chain = node->chain;
    if(chain.values.empty()) return;
    // Evaluate the chain. For method chains like d.copy().copy().copy(),
    // ALL function call temps must be destructed at the end.
    // Strategy: first evaluate the chain to get the final result, then
    // go back and destruct every FunctionCall's result.
    // We evaluate each FunctionCall in sequence to create its temp struct,
    // collect the temps, then destruct them all after.
    
    if(chain.values.size() == 1) {
        auto val = chain.values[0];
        auto result = val->evaluated_value(scope);
        scope.destroy_value(result);
        // Also destruct temps created by arguments wrapped in ReferenceOfValue/AddrOfValue.
        // E.g. take_gen_destruct_ref(&create_short_gen_dest(...)) — the inner FunctionCall
        // creates a temp struct that must be destructed after the outer call completes.
        if(val->val_kind() == ValueKind::FunctionCall) {
            auto call = val->as_func_call_unsafe();
            for(auto arg : call->values) {
                if(arg->val_kind() == ValueKind::ReferenceOfValue) {
                    auto refOf = arg->as_reference_of_value_unsafe();
                    if(refOf->innerEvaluatedResult) {
                        scope.destroy_value(refOf->innerEvaluatedResult);
                    }
                } else if(arg->val_kind() == ValueKind::AddrOfValue) {
                    auto addrOf = arg->as_addr_of_value_unsafe();
                    if(addrOf->innerEvaluatedResult) {
                        scope.destroy_value(addrOf->innerEvaluatedResult);
                    }
                }
            }
        }
        return;
    }
    
    // Evaluate each value in the chain, collecting FunctionCall temps
    std::vector<Value*> temps;
    Value* current = nullptr;
    for(unsigned i = 0; i < chain.values.size(); i++) {
        auto& val = chain.values[i];
        if(val->val_kind() == ValueKind::Identifier) {
            auto id = val->as_identifier_unsafe();
            current = current ? current->child(scope, id->value) : id->evaluated_value(scope);
        } else {
            current = val->evaluated_value(scope);
            // Track function call results (temps that need destruction)
            if(val->val_kind() == ValueKind::FunctionCall && current) {
                temps.push_back(current);
            }
        }
    }
    
    // Destruct all collected temps — for a bare statement, everything is discarded
    for(auto t : temps) {
        scope.destroy_value(t);
    }
}

inline void interpret(InterpretScope& scope, IncDecNode* node) {
    node->value.evaluated_value(scope);
}

inline void interpret(InterpretScope& scope, PatternMatchExprNode* node) {
    // Evaluate the pattern match: check if the variant matches the expected member
    bool matches = false;
    Value* evalExpr = nullptr;
    if(!node->value.param_names.empty() && node->value.member) {
        evalExpr = node->value.expression->evaluated_value(scope);
        // the matched expression may be a reference stored inside a struct member
        // (e.g. h.value where value : &Variant) — dereference it to get the variant
        if(evalExpr && evalExpr->val_kind() == ValueKind::PointerValue) {
            evalExpr = ((PointerValue*) evalExpr)->deref(scope, node->encoded_location(), evalExpr);
        }
        if(evalExpr && evalExpr->val_kind() == ValueKind::StructValue) {
            auto evalBool = node->value.evaluated_value(scope);
            matches = evalBool && evalBool->get_the_bool();
        }
    }
    if(matches) {
        // Pattern matched — extract variables from the variant struct
        if(evalExpr && evalExpr->val_kind() == ValueKind::StructValue) {
            auto variantStruct = evalExpr->as_struct_value_unsafe();
            for(auto& pm : node->value.param_names) {
                if(pm->member_param) {
                    auto found = variantStruct->values.find(pm->member_param->name);
                    if(found != variantStruct->values.end() && found->second.value) {
                        scope.declare(pm->identifier, found->second.value);
                        variantStruct->values.erase(found);
                    }
                }
            }
        }
    } else {
        // Pattern didn't match — handle the else expression
        const auto& elseExpr = node->value.elseExpression;
        switch(elseExpr.kind) {
            case PatternElseExprKind::Break:
                raise_loop_break(scope);
                break;
            case PatternElseExprKind::Continue:
                raise_loop_continue(scope);
                break;
            case PatternElseExprKind::Return: {
                // set_return handles a null value too: it still propagates the
                // stop signal up the scope chain.
                scope.global->current_func_type->set_return(scope, elseExpr.value);
                break;
            }
            case PatternElseExprKind::DefValue: {
                if(elseExpr.value && !node->value.param_names.empty()) {
                    const auto evalValue = elseExpr.value->evaluated_value(scope);
                    if(evalValue) {
                        scope.declare(node->value.param_names[0]->identifier, evalValue);
                    }
                }
                break;
            }
            case PatternElseExprKind::Unreachable: {
                // Unreachable was handled above — pattern should have matched
                break;
            }
            default:
                // No else expression — just evaluate normally
                node->value.evaluated_value(scope);
                break;
        }
    }
}

inline void interpret(InterpretScope& scope, PlacementNewNode* node) {
    node->value.evaluated_value(scope);
}

void interpret(InterpretScope& scope, LoopBlock* loop) {
    InterpretScope loop_scope(&scope, scope.allocator, scope.global);
    loop_scope.is_loop_scope = true;
    while (true) {
        {
            InterpretScope body_scope(&loop_scope, scope.allocator, scope.global);
            body_scope.interpret(&loop->body);
        } // body_scope destroyed here, calling destructors on iteration-local variables
        if (loop_scope.stopInterpretation) break; // return unwinds
        if (loop_scope.loop_signal == 1) { // break
            loop_scope.loop_signal = 0;
            break;
        }
        loop_scope.loop_signal = 0; // continue
    }
}

Value* LoopValue::evaluated_value(InterpretScope& scope) {
    InterpretScope child(&scope, scope.allocator, scope.global);
    // Mark this frame so `break value` statements inside the body store their
    // value here (walking up the scope chain) instead of in a global slot, and
    // so `break`/`continue` signal this loop.
    child.is_loop_value_scope = true;
    child.is_loop_scope = true;
    while (true) {
        {
            InterpretScope body_scope(&child, scope.allocator, scope.global);
            body_scope.interpret(&stmt.body);
        }
        if (child.stopInterpretation) break; // return unwinds
        if (child.loop_signal == 1) { // break
            child.loop_signal = 0;
            break;
        }
        child.loop_signal = 0; // continue
    }
    auto result = child.loop_break_value;
    child.loop_break_value = nullptr;
    return result ? result : scope.getNullValue();
}

void interpret(InterpretScope& scope, ProvideStmt* stmt) {
    // Evaluate the provide value
    auto val = stmt->value->evaluated_value(scope);
    if(val) {
        // Store the implicit arg in the current scope's map.
        // The provide body is interpreted in-place (same scope, no new scope),
        // so the implicit arg is available for any function called within.
        scope.implicit_args_ref()[stmt->identifier] = val;
    }
    scope.interpret(&stmt->body);
    // Clean up
    scope.implicit_args_ref().erase(stmt->identifier);
}

void interpret(InterpretScope& scope, VarInitStatement* stmt) {
    if (stmt->value) {
        // Check if the initializer is an AccessChain starting with a FunctionCall.
        // In that case, the FunctionCall produces an intermediate struct temp that
        // must be destructed after the field access completes.
        // E.g.: var data = create_destructible(&raw mut count, 858).data
        if(stmt->value->val_kind() == ValueKind::AccessChain) {
            auto chain = stmt->value->as_access_chain_unsafe();
            if(!chain->values.empty() && chain->values[0]->val_kind() == ValueKind::FunctionCall && chain->values.size() > 1) {
                auto chainTemp = chain->values[0]->evaluated_value(scope);
                auto chainResult = evaluate_from(chain->values, scope, chainTemp, 1);
                scope.declare(stmt->name_view(), chainResult);
                scope.destroy_value(chainTemp);
                return;
            }
        }
        // Handle string literal to char array conversion:
        // var str : [10]char = "hello" — convert string to char array with padding
        if(stmt->value->val_kind() == ValueKind::String && stmt->type && stmt->type->kind() == BaseTypeKind::Array) {
            auto arrType = stmt->type->as_array_type_unsafe();
            auto elemType = arrType->elem_type;
            if(elemType && elemType->canonical()->kind() == BaseTypeKind::IntN &&
               elemType->canonical()->isCharType()) {
                auto strVal = stmt->value->as_string_unsafe();
                auto arrSize = arrType->get_array_size();
                if(arrSize > 0) {
                    auto arrVal = new (scope.allocate<ArrayValue>()) ArrayValue(stmt->encoded_location(), arrType);
                    arrVal->explicit_size = (unsigned int)arrSize;
                    // Copy string chars into array values up to the array size
                    for(uint64_t i = 0; i < (uint64_t)arrSize; i++) {
                        char c = (i < strVal->value.size()) ? strVal->value[i] : '\0';
                        auto charVal = new (scope.allocate<IntNumValue>()) IntNumValue(
                            (uint64_t)(unsigned char)c,
                            scope.global->typeBuilder.getCharType(),
                            stmt->encoded_location()
                        );
                        arrVal->values.push_back(charVal);
                    }
                    scope.declare(stmt->name_view(), arrVal);
                    return;
                }
            }
        }
        Value* initializer;
        if(stmt->value->val_kind() == ValueKind::ArrayValue) {
            // ArrayValue initializers: use evaluated_value instead of scope_value.
            // scope_value calls copy() which deep-copies ALL elements, creating new
            // StructValue pointers that break move_clear_source's ability to match
            // the original scope variable's pointer against the copied array element.
            // Since array literal AST nodes are used only once per declaration,
            // returning the original node is safe and preserves move semantics.
            initializer = stmt->value->evaluated_value(scope);
        } else {
            initializer = stmt->value->scope_value(scope);
        }
        // Coerce integer literal types to match the declared type (e.g. var i : long = 61
        // should create an IntNumValue with LongType, not IntType).
        if(stmt->type != nullptr && initializer && initializer->val_kind() == ValueKind::IntN &&
           stmt->type->kind() == BaseTypeKind::IntN) {
            auto initType = static_cast<IntNType*>(initializer->getType());
            auto declType = static_cast<IntNType*>((BaseType*)stmt->type);
            if(initType && declType && initType->IntNKind() != declType->IntNKind()) {
                auto intVal = static_cast<IntNumValue*>(initializer);
                initializer = new (scope.allocate<IntNumValue>()) IntNumValue(
                    intVal->value, declType, stmt->encoded_location()
                );
            }
        }

        scope.declare(stmt->name_view(), initializer);
        // Handle move semantics: if the initializer references an existing destructible
        // struct variable, clear the source (move, not copy). Uses pointer-matching
        // instead of AST node type checks, because the compiler may have replaced
        // VariableIdentifier nodes with the resolved StructValue during resolution.
        scope.move_clear_source(initializer, stmt->name_view());
    } else if (stmt->type) {
        // Uninitialized variable: create a default zero value based on type
        auto type = stmt->type;
        auto kind = type->kind();
        if (kind == BaseTypeKind::Array) {
            auto arrType = type->as_array_type_unsafe();
            auto size = arrType->get_array_size();
            if (size > 0) {
                auto arrVal = new (scope.allocate<ArrayValue>()) ArrayValue(stmt->encoded_location(), arrType);
                arrVal->explicit_size = (unsigned int)size;
                // evaluated_value will allocate zero-initialized contiguousData
                arrVal->evaluated_value(scope);
                scope.declare(stmt->name_view(), arrVal);
            }
        } else if (kind == BaseTypeKind::Linked) {
            auto linkedType = type->as_linked_type_unsafe();
            // Resolve typealiases to the underlying struct definition or primitive type
            ASTNode* linked = linkedType->linked;
            BaseType* resolvedActualType = nullptr;
            if (linked && linked->kind() == ASTNodeKind::TypealiasStmt) {
                auto alias = (TypealiasStatement*) linked;
                if (alias->actual_type) {
                    resolvedActualType = alias->actual_type;
                    auto actualKind = alias->actual_type->kind();
                    if (actualKind == BaseTypeKind::Linked) {
                        linked = alias->actual_type->as_linked_type_unsafe()->linked;
                    }
                }
            }
            // Handle primitive types resolved through typealias (e.g. namespace_typealias = int)
            if(resolvedActualType) {
                auto resolvedKind = resolvedActualType->kind();
                if(resolvedKind == BaseTypeKind::IntN) {
                    auto intType = resolvedActualType->as_intn_type_unsafe();
                    auto val = new (scope.allocate<IntNumValue>()) IntNumValue(0, intType, stmt->encoded_location());
                    scope.declare(stmt->name_view(), val);
                    return;
                } else if(resolvedKind == BaseTypeKind::Bool) {
                    auto val = new (scope.allocate<BoolValue>()) BoolValue(false, scope.global->typeBuilder.getBoolType(), stmt->encoded_location());
                    scope.declare(stmt->name_view(), val);
                    return;
                } else if(resolvedKind == BaseTypeKind::Pointer) {
                    auto ptrType = resolvedActualType->as_pointer_type_unsafe();
                    auto val = new (scope.allocate<PointerValue>()) PointerValue(nullptr, ptrType->type, 0, 0, stmt->encoded_location());
                    scope.declare(stmt->name_view(), val);
                    return;
                } else if(resolvedKind == BaseTypeKind::Array) {
                    auto arrType = resolvedActualType->as_array_type_unsafe();
                    auto size = arrType->get_array_size();
                    if(size > 0) {
                        auto arrVal = new (scope.allocate<ArrayValue>()) ArrayValue(stmt->encoded_location(), arrType);
                        arrVal->explicit_size = (unsigned int)size;
                        arrVal->evaluated_value(scope);
                        scope.declare(stmt->name_view(), arrVal);
                        return;
                    }
                }
            }
            if (linked && (linked->kind() == ASTNodeKind::StructDecl || linked->kind() == ASTNodeKind::UnionDecl)) {
                auto container = (ExtendableMembersContainerNode*) linked;
                auto structVal = new (scope.allocate<StructValue>()) StructValue(
                    stmt->type, container, container, stmt->encoded_location()
                );
                // Populate all fields with default/null values
                for (const auto field : container->variables()) {
                    auto defValue = field->default_value();
                    if (defValue) {
                        structVal->values.emplace(
                            field->name,
                            StructMemberInitializer { field->name, defValue->scope_value(scope) }
                        );
                    } else {
                        // check if field type is an unnamed struct/union — recursively init
                        auto fieldType = field->known_type();
                        if(fieldType && fieldType->kind() == BaseTypeKind::Linked) {
                            auto fieldLinked = fieldType->as_linked_type_unsafe()->linked;
                            if(fieldLinked && (fieldLinked->kind() == ASTNodeKind::UnnamedStruct || fieldLinked->kind() == ASTNodeKind::UnnamedUnion)) {
                                VariablesContainerBase* subContainer;
                                if(fieldLinked->kind() == ASTNodeKind::UnnamedStruct) {
                                    subContainer = fieldLinked->as_unnamed_struct_unsafe();
                                } else {
                                    subContainer = fieldLinked->as_unnamed_union_unsafe();
                                }
                                auto subVal = new (scope.allocate<StructValue>()) StructValue(
                                    fieldType, nullptr, subContainer, stmt->encoded_location()
                                );
                                for(const auto subField : subContainer->variables()) {
                                    auto subDef = subField->default_value();
                                    // Recursively initialize unnamed struct/union members
                                    auto subFieldType = subField->known_type();
                                    if(subFieldType && subFieldType->kind() == BaseTypeKind::Linked) {
                                        auto subFieldLinked = subFieldType->as_linked_type_unsafe()->linked;
                                        if(subFieldLinked && (subFieldLinked->kind() == ASTNodeKind::UnnamedStruct || subFieldLinked->kind() == ASTNodeKind::UnnamedUnion)) {
                                            VariablesContainerBase* subSubContainer;
                                            if(subFieldLinked->kind() == ASTNodeKind::UnnamedStruct) {
                                                subSubContainer = subFieldLinked->as_unnamed_struct_unsafe();
                                            } else {
                                                subSubContainer = subFieldLinked->as_unnamed_union_unsafe();
                                            }
                                            auto innerSubVal = new (scope.allocate<StructValue>()) StructValue(
                                                subFieldType, nullptr, subSubContainer, stmt->encoded_location()
                                            );
                                            for(const auto innerField : subSubContainer->variables()) {
                                                auto innerDef = innerField->default_value();
                                                innerSubVal->values.emplace(
                                                    innerField->name,
                                                    StructMemberInitializer { innerField->name, innerDef ? innerDef->scope_value(scope) : scope.getNullValue() }
                                                );
                                            }
                                            subVal->values.emplace(
                                                subField->name,
                                                StructMemberInitializer { subField->name, innerSubVal }
                                            );
                                        } else {
                                            subVal->values.emplace(
                                                subField->name,
                                                StructMemberInitializer { subField->name, subDef ? subDef->scope_value(scope) : scope.getNullValue() }
                                            );
                                        }
                                    } else {
                                        subVal->values.emplace(
                                            subField->name,
                                            StructMemberInitializer { subField->name, subDef ? subDef->scope_value(scope) : scope.getNullValue() }
                                        );
                                    }
                                }
                                structVal->values.emplace(
                                    field->name,
                                    StructMemberInitializer { field->name, subVal }
                                );
                            } else {
                                structVal->values.emplace(
                                    field->name,
                                    StructMemberInitializer { field->name, scope.getNullValue() }
                                );
                            }
                        } else {
                            // Check field type and create proper zero value
                            auto fieldType = field->known_type();
                            if(fieldType) {
                                auto fieldKind = fieldType->kind();
                                if(fieldKind == BaseTypeKind::IntN) {
                                    auto intType = fieldType->as_intn_type_unsafe();
                                    auto zeroVal = new (scope.allocate<IntNumValue>()) IntNumValue(0, intType, stmt->encoded_location());
                                    structVal->values.emplace(field->name, StructMemberInitializer { field->name, zeroVal });
                                } else if(fieldKind == BaseTypeKind::Bool) {
                                    auto zeroVal = new (scope.allocate<BoolValue>()) BoolValue(false, scope.global->typeBuilder.getBoolType(), stmt->encoded_location());
                                    structVal->values.emplace(field->name, StructMemberInitializer { field->name, zeroVal });
                                } else if(fieldKind == BaseTypeKind::Float) {
                                    auto zeroVal = new (scope.allocate<FloatValue>()) FloatValue(0.0f, scope.global->typeBuilder.getFloatType(), stmt->encoded_location());
                                    structVal->values.emplace(field->name, StructMemberInitializer { field->name, zeroVal });
                                } else if(fieldKind == BaseTypeKind::Double) {
                                    auto zeroVal = new (scope.allocate<DoubleValue>()) DoubleValue(0.0, scope.global->typeBuilder.getDoubleType(), stmt->encoded_location());
                                    structVal->values.emplace(field->name, StructMemberInitializer { field->name, zeroVal });
                                } else if(fieldKind == BaseTypeKind::Pointer) {
                                    auto ptrType = fieldType->as_pointer_type_unsafe();
                                    auto zeroVal = new (scope.allocate<PointerValue>()) PointerValue(nullptr, ptrType->type, 0, 0, stmt->encoded_location());
                                    structVal->values.emplace(field->name, StructMemberInitializer { field->name, zeroVal });
                                } else {
                                    structVal->values.emplace(field->name, StructMemberInitializer { field->name, scope.getNullValue() });
                                }
                            } else {
                                structVal->values.emplace(field->name, StructMemberInitializer { field->name, scope.getNullValue() });
                            }
                        }
                    }
                }
                scope.declare(stmt->name_view(), structVal);
            }
        } else if (kind == BaseTypeKind::IntN) {
            auto intType = type->as_intn_type_unsafe();
            auto val = new (scope.allocate<IntNumValue>()) IntNumValue(0, intType, stmt->encoded_location());
            scope.declare(stmt->name_view(), val);
        } else if (kind == BaseTypeKind::Pointer) {
            // Uninitialized pointer: declare as null pointer
            auto val = new (scope.allocate<PointerValue>()) PointerValue(
                nullptr, type->as_pointer_type_unsafe()->type, 0, 0, stmt->encoded_location()
            );
            scope.declare(stmt->name_view(), val);
        } else if (kind == BaseTypeKind::Bool) {
            auto& typeBuilder = scope.global->typeBuilder;
            auto val = new (scope.allocate<BoolValue>()) BoolValue(false, typeBuilder.getBoolType(), stmt->encoded_location());
            scope.declare(stmt->name_view(), val);
        }
    }
}

void interpret(InterpretScope& scope, DoWhileLoop* loop) {
    InterpretScope loop_scope(&scope, scope.allocator, scope.global);
    loop_scope.is_loop_scope = true;
    do {
        {
            InterpretScope body_scope(&loop_scope, scope.allocator, scope.global);
            body_scope.interpret(&loop->body);
        } // body_scope destroyed here, calling destructors on iteration-local variables
        if (loop_scope.stopInterpretation) break; // return unwinds
        if (loop_scope.loop_signal == 1) { // break
            loop_scope.loop_signal = 0;
            break;
        }
        loop_scope.loop_signal = 0; // continue: fall through to condition
    } while (loop->condition->evaluated_bool(loop_scope));
}

void interpret(InterpretScope& scope, WhileLoop* loop) {
    InterpretScope loop_scope(&scope, scope.allocator, scope.global);
    loop_scope.is_loop_scope = true;
    while (loop->condition->evaluated_bool(loop_scope)) {
        {
            InterpretScope body_scope(&loop_scope, scope.allocator, scope.global);
            body_scope.interpret(&loop->body);
        } // body_scope destroyed here, calling destructors on iteration-local variables
        if (loop_scope.stopInterpretation) break; // return unwinds
        if (loop_scope.loop_signal == 1) { // break
            loop_scope.loop_signal = 0;
            break;
        }
        loop_scope.loop_signal = 0; // continue
    }
}

void interpret(InterpretScope& scope, std::vector<ASTNode*>& nodes) {
    for (const auto &node: nodes) {
        scope.interpret(node);
        if (scope.stopInterpretation) {
            return;
        }
    }
}

inline void interpret(InterpretScope& scope, Scope* body) {
    interpret(scope, body->nodes);
}

inline void interpret(InterpretScope& scope, BlockScope* body) {
    InterpretScope child_scope(&scope, scope.allocator, scope.global);
    interpret(child_scope, body->nodes);
}

Value* evaluate(InterpretScope& scope, Scope* body);

Value* evaluate(InterpretScope& scope, BlockScope* body) {
    auto& nodes = body->nodes;
    if(nodes.empty()) return scope.getNullValue();
    if(nodes.size() > 1) {
        for(unsigned i = 0; i < nodes.size() - 1; i++) {
            scope.interpret(nodes[i]);
            if(scope.stopInterpretation) return scope.getNullValue();
        }
    }
    const auto last = nodes.back();
    switch(last->kind()) {
        case ASTNodeKind::ValueNode:
            return last->as_value_node_unsafe()->value->evaluated_value(scope);
        case ASTNodeKind::IfStmt:
            return evaluated_value(scope, last->as_if_stmt_unsafe());
        case ASTNodeKind::SwitchStmt:
            return evaluated_value(scope, last->as_switch_stmt_unsafe());
        case ASTNodeKind::Block:
            return evaluate(scope, last->as_block_scope_unsafe());
        default:
            scope.interpret(last);
            return scope.getNullValue();
    }
}

Value* evaluate(InterpretScope& scope, Scope* body) {
    auto& nodes = body->nodes;
    if(nodes.size() > 1) {
        auto start = body->nodes.data();
        // skip the last one
        const auto end = start + (nodes.size() - 1);
        while(start != end) {
            scope.interpret(*start);
            if(scope.stopInterpretation) return scope.getNullValue();
            start++;
        }
    }
    // lets do the last one
    const auto last = nodes.back();
    switch(last->kind()) {
        case ASTNodeKind::ValueNode:
            return last->as_value_node_unsafe()->value->evaluated_value(scope);
        case ASTNodeKind::IfStmt:
            return evaluated_value(scope, last->as_if_stmt_unsafe());
        case ASTNodeKind::SwitchStmt:
            return evaluated_value(scope, last->as_switch_stmt_unsafe());
        case ASTNodeKind::Block:
            return evaluate(scope, last->as_block_scope_unsafe());
        default:
            scope.interpret(last);
            return scope.getNullValue();
    }
}

void InterpretScope::interpret(ASTNode* node) {
    switch(node->kind()) {
        case ASTNodeKind::AssignmentStmt:
            ::interpret(*this, node->as_assignment_unsafe());
            break;
        case ASTNodeKind::BreakStmt:
            ::interpret(*this, node->as_break_stmt_unsafe());
            break;
        case ASTNodeKind::ContinueStmt:
            ::interpret(*this, node->as_continue_stmt_unsafe());
            break;
        case ASTNodeKind::ForLoopStmt:
            ::interpret(*this, node->as_for_loop_unsafe());
            break;
        case ASTNodeKind::ForInLoopStmt:
            ::interpret(*this, node->as_for_in_loop_unsafe());
            break;
        case ASTNodeKind::ReturnStmt:
            ::interpret(*this, node->as_return_unsafe());
            break;
        case ASTNodeKind::InlineAsmStmt:
            // inline asm is a no-op during interpretation
            break;
        case ASTNodeKind::SwitchStmt:
            ::interpret(*this, node->as_switch_stmt_unsafe());
            break;
        case ASTNodeKind::IfStmt:
            ::interpret(*this, node->as_if_stmt_unsafe());
            break;
        case ASTNodeKind::ValueWrapper:
            ::interpret(*this, node->as_value_wrapper_unsafe());
            break;
        case ASTNodeKind::AccessChainNode:
            ::interpret(*this, node->as_access_chain_node_unsafe());
            break;
        case ASTNodeKind::IncDecNode:
            ::interpret(*this, node->as_inc_dec_node_unsafe());
            break;
        case ASTNodeKind::PatternMatchExprNode:
            ::interpret(*this, node->as_pattern_match_expr_node_unsafe());
            break;
        case ASTNodeKind::PlacementNewNode:
            ::interpret(*this, node->as_placement_new_node_unsafe());
            break;
        case ASTNodeKind::VarInitStmt:
            ::interpret(*this, node->as_var_init_unsafe());
            break;
        case ASTNodeKind::DoWhileLoopStmt:
            ::interpret(*this, node->as_do_while_loop_unsafe());
            break;
        case ASTNodeKind::WhileLoopStmt:
            ::interpret(*this, node->as_while_loop_unsafe());
            break;
        case ASTNodeKind::LoopBlock:
            ::interpret(*this, node->as_loop_block_unsafe());
            break;
        case ASTNodeKind::ProvideStmt:
            ::interpret(*this, node->as_provide_stmt_unsafe());
            break;
        case ASTNodeKind::Scope:
            ::interpret(*this, node->as_scope_unsafe());
            break;
    case ASTNodeKind::Block:
        ::interpret(*this, node->as_block_scope_unsafe());
        break;
    case ASTNodeKind::UnsafeBlock:
        ::interpret(*this, &node->as_unsafe_block_unsafe()->scope);
        break;
    case ASTNodeKind::DeleteStmt: {
        auto stmt = node->as_destruct_stmt_unsafe();
        auto ptrVal = stmt->identifier->evaluated_value(*this);
        if(ptrVal && ptrVal->val_kind() == ValueKind::PointerValue) {
            auto pv = (PointerValue*)ptrVal;
            if(pv->data != nullptr) {
                if(stmt->is_array) {
                    // Array destruct: destruct[N] ptr — destruct N elements
                    auto sizeVal = stmt->array_value ? stmt->array_value->evaluated_value(*this) : nullptr;
                    uint64_t arraySize = 0;
                    if(sizeVal) {
                        auto num = sizeVal->get_number();
                        if(num.has_value()) {
                            arraySize = num.value();
                        }
                    }
                    if(arraySize > 0) {
                        // Get element type and size
                        auto ptrType = pv->getType();
                        BaseType* elemType = nullptr;
                        if(ptrType && ptrType->kind() == BaseTypeKind::Pointer) {
                            elemType = ((PointerType*)ptrType)->type;
                        }
                        uint64_t elemSize = elemType ? elemType->byte_size(global->target_data) : 1;
                        // Iterate in reverse order (size-1 down to 0), matching C codegen
                        for(uint64_t i = arraySize; i-- > 0; ) {
                            auto elemPtr = new (allocate<PointerValue>()) PointerValue(
                                (void*)((uint8_t*)pv->data + i * elemSize),
                                elemType,
                                i * elemSize,
                                (arraySize - i) * elemSize,
                                stmt->encoded_location()
                            );
                            auto elemVal = elemPtr->deref(*this, stmt->encoded_location(), stmt->identifier);
                            if(elemVal) {
                                destroy_value(elemVal);
                            }
                        }
                    }
                    // If arraySize == 0, skip destruct entirely
                } else {
                    // Single pointer destruct
                    auto structVal = pv->deref(*this, stmt->encoded_location(), stmt->identifier);
                    if(structVal) {
                        destroy_value(structVal);
                    }
                    if(stmt->getFreeAfter()) {
                        pv->data = nullptr;
                        pv->ahead = 0;
                        pv->behind = 0;
                    }
                }
            }
        }
        break;
    }
    default:
        break;
    }
}

// ---------- values

Value* IfValue::evaluated_value(InterpretScope &scope) {
    return ::evaluated_value(scope, &stmt);
}

Value* SwitchValue::evaluated_value(InterpretScope &scope) {
    return ::evaluated_value(scope, &stmt);
}