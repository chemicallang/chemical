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
#include "ast/structures/VariantMember.h"
#include "ast/structures/VariantMemberParam.h"
//#include "ast/statements/MacroValueStatement.h"
//#include "ast/statements/Import.h"
#include "ast/statements/ValueWrapperNode.h"
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
#include "ast/structures/UnnamedUnion.h"
#include "ast/structures/UnnamedStruct.h"
#include "ast/structures/CapturedVariable.h"
//#include "ast/structures/MembersContainer.h"
#include "ast/structures/Scope.h"
#include "ast/structures/WhileLoop.h"
#include "ast/types/ReferenceType.h"
#include "ast/types/PointerType.h"
#include "ast/types/FunctionType.h"
#include "ast/types/GenericType.h"
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
#include "ast/types/StructType.h"
#include "ast/types/UnionType.h"
//#include "ast/types/UBigIntType.h"
//#include "ast/types/UInt128Type.h"
//#include "ast/types/UIntType.h"
//#include "ast/types/ULongType.h"
//#include "ast/types/UShortType.h"
//#include "ast/types/VoidType.h"
//#include "ast/values/UShortValue.h"
//#include "ast/values/VariableIdentifier.h"
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
#include "ast/values/IsValue.h"
//#include "ast/values/FloatValue.h"
#include "ast/values/ValueNode.h"
#include "ast/values/SizeOfValue.h"
#include "ast/values/AlignOfValue.h"
#include "ast/values/IndexOperator.h"
#include "ast/values/TypeInsideValue.h"
//#include "ast/values/IntNumValue.h"
#include "ast/values/AddrOfValue.h"
#include "ast/values/DereferenceValue.h"
#include "ast/values/NotValue.h"
#include "ast/values/Negative.h"
#include "ast/values/IncDecValue.h"
//#include "ast/values/NullValue.h"
//#include "ast/values/NumberValue.h"
//#include "ast/values/ShortValue.h"
//#include "ast/values/StringValue.h"
//#include "ast/values/UBigIntValue.h"
//#include "ast/values/UInt128Value.h"
//#include "ast/values/UIntValue.h"
//#include "ast/values/ULongValue.h"
#include "ast/values/NewValue.h"
#include "ast/values/PlacementNewValue.h"

template<typename Derived>
class RecursiveVisitor : public NonRecursiveVisitor<Derived> {
public:

    inline void visit_it(Scope& scope) {
        static_cast<Derived*>(this)->VisitScope(&scope);
    }

    inline void visit_it(FunctionDeclaration* decl) {
        static_cast<Derived*>(this)->VisitFunctionDecl(decl);
    }

    template<typename T>
    inline void visit_it(T*& thing) {
        static_cast<Derived*>(this)->visit(thing);
    }

    void VisitScope(Scope *scope) {
        for(auto& node : scope->nodes) {
            visit_it(node);
        }
    }

    void VisitFunctionCall(FunctionCall *call) {
        visit_it(call->parent_val);
        for(auto& arg : call->generic_list) {
            visit_it(arg);
        }
        for(auto& val : call->values) {
            visit_it(val);
        }
    }

    void VisitIndexOperator(IndexOperator* value) {
        visit_it(value->parent_val);
        for(auto& val : value->values) {
            visit_it(val);
        }
    }

    void VisitExpression(Expression *expr) {
        visit_it(expr->firstValue);
        visit_it(expr->secondValue);
    }

    void VisitNegativeValue(NegativeValue* value) {
        visit_it(value->value);
    }

    void VisitNotValue(NotValue* value) {
        visit_it(value->value);
    }

    void VisitIncDecValue(IncDecValue* value) {
        visit_it(value->value);
    }

    void VisitCastedValue(CastedValue *casted) {
        visit_it(casted->value);
        visit_it(casted->type);
    }

    void VisitValueNode(ValueNode *node) {
        visit_it(node->value);
    }

    void VisitUnsafeBlock(UnsafeBlock *block) {
        visit_it(block->scope);
    }

    void VisitValueWrapper(ValueWrapperNode *node) {
        visit_it(node->value);
    }

    void VisitVarInitStmt(VarInitStatement *init) {
        if(init->type) {
            visit_it(init->type);
        }
        if(init->value) {
            visit_it(init->value);
        }
    }

    void VisitReturnStmt(ReturnStatement *stmt) {
        if(stmt->value) {
            visit_it(stmt->value);
        }
    }

    void VisitAssignmentStmt(AssignStatement *assign) {
        visit_it(assign->lhs);
        visit_it(assign->value);
    }

    void VisitFunctionDecl(FunctionDeclaration *decl) {
        for(auto& param : decl->params) {
            visit_it(param);
        }
        visit_it(decl->returnType);
        if(decl->body.has_value()) {
            visit_it(decl->body.value());
        }
    }

    void VisitFunctionParam(FunctionParam *param) {
        visit_it(param->type);
        if(param->defValue) {
            visit_it(param->defValue);
        }
    }

    void VisitVariables(tsl::ordered_map<chem::string_view, BaseDefMember*>& variables) {
        auto itr = variables.begin();
        while(itr != variables.end()) {
            visit_it(itr.value());
            itr++;
        }
    }

    inline void VisitUnnamedStruct(UnnamedStruct* def) {
        VisitVariables(def->variables);
    }

    inline void VisitUnnamedUnion(UnnamedUnion* def) {
        VisitVariables(def->variables);
    }

    inline void VisitStructDecl(StructDefinition *def) {
        VisitVariables(def->variables);
        for(auto& func : def->functions()) {
            visit_it(func);
        }
    }

    void VisitNamespaceDecl(Namespace *ns) {
        for(auto& node : ns->nodes){
            visit_it(node);
        }
    }

    void VisitStructMember(StructMember *member) {
        visit_it(member->type);
        if(member->defValue) {
            visit_it(member->defValue);
        }
    }

    inline void VisitVariantMemberParam(VariantMemberParam* node) {
        visit_it(node->type);
        if(node->def_value) {
            visit_it(node->def_value);
        }
    }

    void VisitVariantMember(VariantMember* node) {
        auto begin = node->values.begin();
        auto end = node->values.end();
        while(begin != end) {
            static_cast<Derived*>(this)->VisitVariantMemberParam(begin.value());
            begin++;
        }
    }

    void VisitLambdaFunction(LambdaFunction *func) {
        for(auto& var : func->captureList) {
            visit_it(var);
        }
        visit_it(func->scope);
    }

    void VisitIfStmt(IfStatement *stmt) {
        visit_it(stmt->condition);
        visit_it(stmt->ifBody);
        for (auto& elif : stmt->elseIfs) {
            visit_it(elif.first);
            visit_it(elif.second);
        }
        if(stmt->elseBody.has_value()) {
            visit_it(stmt->elseBody.value());
        }
    }

    void VisitDeleteStmt(DestructStmt *stmt) {
        visit_it(stmt->identifier);
    }

    void VisitWhileLoopStmt(WhileLoop *loop) {
        visit_it(loop->condition);
        visit_it(loop->body);
    }

    void VisitDoWhileLoopStmt(DoWhileLoop *loop) {
        visit_it(loop->body);
        visit_it(loop->condition);
    }

    void VisitForLoopStmt(ForLoop *loop) {
        visit_it(loop->initializer);
        visit_it(loop->conditionExpr);
        visit_it(loop->incrementerExpr);
        visit_it(loop->body);
    }

    void VisitSwitchStmt(SwitchStatement *stmt) {
        visit_it(stmt->expression);
        for(auto& thing : stmt->cases) {
            visit_it(thing.first);
        }
        for(auto& scope : stmt->scopes) {
            visit_it(scope);
        }
    }

    void VisitAccessChain(AccessChain *chain) {
        for(auto& val : chain->values) {
            visit_it(val);
        }
    }

    void VisitStructValue(StructValue *val) {
        for(auto& value : val->values) {
            visit_it(value.second.value);
        }
    }

    void VisitArrayValue(ArrayValue *arr) {
        for(auto& value : arr->values) {
            visit_it(value);
        }
    }

    inline void VisitArrayType(ArrayType *type) {
        visit_it(type->elem_type);
        if(type->array_size_value) {
            visit_it(type->array_size_value);
        }
    }

    inline void VisitIsValue(IsValue* value) {
        visit_it(value->value);
        visit_it(value->type);
    }

    inline void VisitNewValue(NewValue *value) {
        visit_it(value->value);
    }

    inline void VisitAddrOfValue(AddrOfValue* value) {
        visit_it(value->value);
    }

    inline void VisitDereferenceValue(DereferenceValue* value) {
        visit_it(value->value);
    }

    void VisitPlacementNewValue(PlacementNewValue *value) {
        visit_it(value->pointer);
        visit_it(value->value);
    }

    inline void VisitSizeOfValue(SizeOfValue* value) {
        visit_it(value->for_type);
    }

    inline void VisitAlignOfValue(AlignOfValue* value) {
        visit_it(value->for_type);
    }

    void VisitTypeInsideValue(TypeInsideValue* value) {
        visit_it(value->type);
    }

    inline void VisitReferenceType(ReferenceType* type) {
        visit_it(type->type);
    }

    inline void VisitPointerType(PointerType* type) {
        visit_it(type->type);
    }

    void VisitGenericType(GenericType* type) {
        visit_it(type->referenced);
        for(auto& ty : type->types) {
            visit_it(ty);
        }
    }

    void VisitFunctionType(FunctionType* type) {
        for(auto& ty : type->params) {
            visit_it(ty);
        }
        visit_it(type->returnType);
    }

    void VisitStructType(StructType* type) {
        VisitVariables(type->variables);
    }

    void VisitUnionType(UnionType* type) {
        VisitVariables(type->variables);
    }

};