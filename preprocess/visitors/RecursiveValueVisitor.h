// Copyright (c) Qinetik 2025.

#pragma once

#include "NonRecursiveVisitor.h"
#include "ast/statements/VarInit.h"
#include "ast/statements/Typealias.h"
#include "ast/statements/Continue.h"
#include "ast/statements/Break.h"
#include "ast/statements/Return.h"
#include "ast/statements/Assignment.h"
#include "ast/statements/SwitchStatement.h"
#include "ast/statements/DeallocStmt.h"
//#include "ast/statements/MacroValueStatement.h"
//#include "ast/statements/Import.h"
#include "ast/statements/ValueWrapperNode.h"
#include "ast/statements/AccessChainNode.h"
#include "ast/statements/IncDecNode.h"
#include "ast/statements/PatternMatchExprNode.h"
#include "ast/statements/PlacementNewNode.h"
//#include "ast/structures/EnumDeclaration.h"
#include "ast/structures/StructMember.h"
#include "ast/structures/ImplDefinition.h"
#include "ast/structures/FunctionParam.h"
#include "ast/structures/InterfaceDefinition.h"
#include "ast/structures/FunctionDeclaration.h"
#include "ast/structures/TryCatch.h"
#include "ast/structures/DoWhileLoop.h"
#include "ast/structures/Namespace.h"
#include "ast/statements/DestructStmt.h"
#include "ast/structures/If.h"
#include "ast/structures/StructDefinition.h"
#include "ast/structures/ForLoop.h"
#include "ast/structures/UnsafeBlock.h"
//#include "ast/structures/CapturedVariable.h"
//#include "ast/structures/MembersContainer.h"
#include "ast/structures/Scope.h"
#include "ast/structures/WhileLoop.h"
//#include "ast/types/ReferencedType.h"
//#include "ast/types/PointerType.h"
//#include "ast/types/FunctionType.h"
//#include "ast/types/GenericType.h"
//#include "ast/types/AnyType.h"
#include "ast/types/ArrayType.h"
//#include "ast/types/BigIntType.h"
//#include "ast/types/BoolType.h"
//#include "ast/types/CharType.h"
//#include "ast/types/DoubleType.h"
//#include "ast/types/FloatType.h"
//#include "ast/types/Int128Type.h"
//#include "ast/types/IntNType.h"
//#include "ast/types/IntType.h"
//#include "ast/types/LongType.h"
//#include "ast/types/ShortType.h"
//#include "ast/types/StringType.h"
//#include "ast/types/StructType.h"
//#include "ast/types/UBigIntType.h"
//#include "ast/types/UInt128Type.h"
#include "ast/values/TypeInsideValue.h"
//#include "ast/types/ULongType.h"
//#include "ast/types/UShortType.h"
//#include "ast/types/VoidType.h"
//#include "ast/values/UShortValue.h"
//#include "ast/values/IntValue.h"
//#include "ast/values/DoubleValue.h"
#include "ast/values/FunctionCall.h"
#include "ast/values/LambdaFunction.h"
#include "ast/values/CastedValue.h"
#include "ast/values/AccessChain.h"
#include "ast/values/StructValue.h"
#include "ast/values/AddrOfValue.h"
#include "ast/values/ArrayValue.h"
//#include "ast/values/BigIntValue.h"
//#include "ast/values/BoolValue.h"
//#include "ast/values/CharValue.h"
#include "ast/values/DereferenceValue.h"
#include "ast/values/Expression.h"
//#include "ast/values/FloatValue.h"
#include "ast/values/ValueNode.h"
//#include "ast/values/Int128Value.h"
//#include "ast/values/IntNumValue.h"
//#include "ast/values/LongValue.h"
//#include "ast/values/Negative.h"
//#include "ast/values/NotValue.h"
//#include "ast/values/NullValue.h"
//#include "ast/values/NumberValue.h"
//#include "ast/values/ShortValue.h"
//#include "ast/values/StringValue.h"
//#include "ast/values/UBigIntValue.h"
//#include "ast/values/UInt128Value.h"
//#include "ast/values/UIntValue.h"
//#include "ast/values/ULongValue.h"
#include "ast/values/NewValue.h"
#include "ast/values/IsValue.h"
#include "ast/values/InValue.h"
#include "ast/values/PlacementNewValue.h"

template<typename T>
class RecursiveValueVisitor : public NonRecursiveVisitor<T> {
public:

    using NonRecursiveVisitor<T>::visit;

    bool is_top_level_node = true;

    void VisitScope(Scope *scope) {
        auto prev = is_top_level_node;
        is_top_level_node = false;
        for(auto node : scope->nodes) {
            visit(node);
        }
        is_top_level_node = prev;
    }

    void VisitFunctionCall(FunctionCall *call) {
        visit(call->parent_val);
        for(auto val : call->values) {
            visit(val);
        }
    }

    void VisitExpression(Expression *expr) {
        visit(expr->firstValue);
        visit(expr->secondValue);
    }

    inline void VisitIsValue(IsValue* value) {
        visit(value->value);
        visit(value->type);
    }

    void VisitInValue(InValue* value) {
        visit(value->value);
        for(auto& child : value->values) {
            visit(child);
        }
    }

    inline void VisitCastedValue(CastedValue *casted) {
        visit(casted->value);
    }

    inline void VisitValueNode(ValueNode *node) {
        visit(node->value);
    }

    inline void VisitUnsafeBlock(UnsafeBlock *block) {
        visit(&block->scope);
    }

    inline void VisitValueWrapper(ValueWrapperNode *node) {
        visit(node->value);
    }

    inline void VisitAccessChainNode(AccessChainNode *node) {
        visit(&node->chain);
    }

    inline void VisitIncDecNode(IncDecNode* node) {
        visit(&node->value);
    }

    inline void VisitPatternMatchExprNode(PatternMatchExprNode* node) {
        visit(&node->value);
    }

    inline void VisitPlacementNewNode(PlacementNewNode* node) {
        visit(&node->value);
    }

    void VisitVarInitStmt(VarInitStatement *init) {
        if(init->type) {
            visit(init->type);
        }
        if(init->value) {
            visit(init->value);
        }
    }

    void VisitReturnStmt(ReturnStatement *stmt) {
        if(stmt->value) {
            visit(stmt->value);
        }
    }

    void VisitAssignmentStmt(AssignStatement *assign) {
        visit(assign->value);
    }

    void VisitFunctionDecl(FunctionDeclaration *decl) {
        for(auto& param : decl->params) {
            visit(param);
        }
        if(decl->body.has_value()) {
            visit(&decl->body.value());
        }
        visit(decl->returnType);
    }

    void VisitFunctionParam(FunctionParam *param) {
        visit(param->type);
        if(param->defValue) {
            visit(param->defValue);
        }
    }

    void VisitStructDecl(StructDefinition *def) {
        for(auto& mem : def->variables()) {
            visit(mem);
        }
        for(auto& func : def->instantiated_functions()) {
            visit(func);
        }
    }

    void VisitNamespaceDecl(Namespace *ns) {
        for(const auto node : ns->nodes){
            visit(node);
        }
    }

    void VisitStructMember(StructMember *member) {
        visit(member->type);
        if(member->defValue) {
            visit(member->defValue);
        }
    }

    void VisitLambdaFunction(LambdaFunction *func) {
        visit(&func->scope);
    }

    void VisitIfStmt(IfStatement *stmt) {
        visit(stmt->condition);
        visit(&stmt->ifBody);
        for (auto& elif : stmt->elseIfs) {
            visit(elif.first);
            visit(&elif.second);
        }
        if(stmt->elseBody.has_value()) {
            visit(&stmt->elseBody.value());
        }
    }

    inline void VisitDeleteStmt(DestructStmt *stmt) {
        visit(stmt->identifier);
    }

    inline void VisitDeallocStmt(DeallocStmt* stmt) {
        visit(stmt->ptr);
    }

    void VisitWhileLoopStmt(WhileLoop *loop) {
        visit(loop->condition);
        visit(&loop->body);
    }

    void VisitDoWhileLoopStmt(DoWhileLoop *loop) {
        visit(&loop->body);
        visit(loop->condition);
    }

    void VisitForLoopStmt(ForLoop *loop) {
        visit(loop->initializer);
        visit(loop->incrementerExpr);
        visit(&loop->body);
    }

    void VisitSwitchStmt(SwitchStatement *stmt) {
        visit(stmt->expression);
        for(auto& scope : stmt->scopes) {
            visit(&scope);
        }
    }

    void VisitAccessChain(AccessChain *chain) {
        for(auto& val : chain->values) {
            visit(val);
        }
    }

    void VisitStructValue(StructValue *val) {
        for(auto& value : val->values) {
            visit(value.second.value);
        }
    }

    void VisitArrayValue(ArrayValue *arr) {
        for(auto& value : arr->values) {
            visit(value);
        }
    }

    void VisitArrayType(ArrayType *type) {
        visit(type->elem_type);
    }

    void VisitNewValue(NewValue *value) {
        visit(value->value);
    }

    void VisitPlacementNewValue(PlacementNewValue *value) {
        visit(value->pointer);
        visit(value->value);
    }

    void VisitTypeInsideValue(TypeInsideValue* value) {
        visit(value->type);
    }

};