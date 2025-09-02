
#include "TypeVerify.h"
#include "TypeVerifyAPI.h"
#include "ast/values/FunctionCall.h"
#include "ast/values/ArrayValue.h"
#include "ast/values/StructValue.h"

void unsatisfied_type_err(ASTDiagnoser& diagnoser, ASTAllocator& allocator, Value* value, BaseType* type) {
    const auto val_type = value->getType();
    if(val_type) {
        diagnoser.error(value) << "value with type '" << val_type->representation() << "' does not satisfy type '" << type->representation() << "'";
    } else {
        diagnoser.error(value) << "value does not satisfy type '" << type->representation() << "'";
    }
}

void TypeVerifier::VisitArrayValue(ArrayValue* val) {
    RecursiveVisitor<TypeVerifier>::VisitArrayValue(val);
    auto& elemType = val->known_elem_type();
    for(const auto value : val->values) {
        if (!elemType->satisfies(allocator, value, false)) {
            unsatisfied_type_err(diagnoser, allocator, value, elemType);
        }
    }
}

void TypeVerifier::VisitFunctionCall(FunctionCall* call) {
    RecursiveVisitor<TypeVerifier>::VisitFunctionCall(call);
    auto func_type = call->function_type(allocator);
    if(!func_type || !func_type->data.signature_resolved) return;
    unsigned i = 0;
    while(i < call->values.size()) {
        const auto param = func_type->func_param_for_arg_at(i);
        if (param) {
            const auto value = call->values[i];
            auto implicit_constructor = param->type->implicit_constructor_for(allocator, value);
            if (implicit_constructor) {
                // TODO handle implicit constructor
            } else if(!param->type->satisfies(allocator, value, false)) {
                unsatisfied_type_err(diagnoser, allocator, value, param->type);
            }
        }
        i++;
    }
}

void TypeVerifier::VisitStructValue(StructValue* structValue) {
    RecursiveVisitor<TypeVerifier>::VisitStructValue(structValue);
    const auto container = structValue->variables();
    for (auto &val: structValue->values) {
        auto& val_ptr = val.second.value;
        const auto value = val_ptr;
        auto child_node = container->child_member_or_inherited_struct(val.first);
        if(!child_node) {
            diagnoser.error(structValue) << "unresolved child '" << val.first << "' in struct declaration";
            continue;
        }
        // auto child_type = child_node->known_type();
        const auto member = container->direct_variable(val.first);
        if(member) {
            const auto mem_type = member->known_type();
            auto implicit = mem_type->implicit_constructor_for(allocator, val_ptr);
            if(implicit) {
                // TODO handle implicit constructor
            } else if(!mem_type->satisfies(allocator, value, false)) {
                unsatisfied_type_err(diagnoser, allocator, value, mem_type);
            }
        }
    }
}

void type_verify(ASTDiagnoser& diagnoser, ASTAllocator& allocator, std::span<ASTNode*> nodes) {
    TypeVerifier verifier(allocator, diagnoser);
    for(const auto node : nodes) {
        switch(node->kind()) {
            case ASTNodeKind::VarInitStmt: {
                const auto stmt = node->as_var_init_unsafe();
                if(!stmt->attrs.signature_resolved) {
                    continue;
                }
                auto& type = stmt->type;
                auto& value = stmt->value;
                if(type && value && !stmt->type->satisfies(allocator, value, false)) {
                    unsatisfied_type_err(diagnoser, allocator, value, type);
                }
                if(value) {
                    verifier.visit(stmt->value);
                }
                break;
            }
            default:
                continue;
        }
    }
}