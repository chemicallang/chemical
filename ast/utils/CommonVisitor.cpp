// Copyright (c) Qinetik 2024.

#include "CommonVisitor.h"
#include "ast/statements/VarInit.h"
#include "ast/statements/Typealias.h"
#include "ast/statements/Continue.h"
#include "ast/statements/Break.h"
#include "ast/statements/Return.h"
#include "ast/statements/Assignment.h"
#include "ast/statements/SwitchStatement.h"
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
//#include "ast/types/UIntType.h"
//#include "ast/types/ULongType.h"
//#include "ast/types/UShortType.h"
//#include "ast/types/VoidType.h"
//#include "ast/values/UShortValue.h"
//#include "ast/values/VariableIdentifier.h"
//#include "ast/values/IntValue.h"
//#include "ast/values/DoubleValue.h"
#include "ast/values/FunctionCall.h"
#include "ast/values/VariantCall.h"
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
#include "ast/values/PlacementNewValue.h"

void CommonVisitor::visit(LambdaFunction *func) {
    func->scope.accept(this);
}

void CommonVisitor::visit(Scope *scope) {
    auto prev = is_top_level_node;
    is_top_level_node = false;
    for(const auto node : scope->nodes) {
        node->accept(this);
    }
    is_top_level_node = prev;
}

void CommonVisitor::visit(FunctionCall *call) {
    call->parent_val->accept(this);
    for(const auto val : call->values) {
        val->accept(this);
    }
}

void CommonVisitor::visit(VariantCall *call) {
    for(auto& val : call->values) {
        val->accept(this);
    }
}

void CommonVisitor::visit(ValueWrapperNode *node) {
    node->value->accept(this);
}

void CommonVisitor::visit(VarInitStatement *init) {
    if(init->type) {
        init->type->accept(this);
    }
    if(init->value) {
        init->value->accept(this);
    }
}

void CommonVisitor::visit(ReturnStatement *stmt) {
    if(stmt->value) {
        stmt->value->accept(this);
    }
}

void CommonVisitor::visit(AssignStatement *assign) {
    assign->value->accept(this);
}

void CommonVisitor::visit(FunctionDeclaration *decl) {
    for(auto& param : decl->params) {
        param->accept(this);
    }
    if(decl->body.has_value()) {
        decl->body.value().accept(this);
    }
    decl->returnType->accept(this);
}

void CommonVisitor::visit(FunctionParam *param) {
    param->type->accept(this);
    if(param->defValue) {
        param->defValue->accept(this);
    }
}

void CommonVisitor::visit(StructDefinition *def) {
    for(auto& mem : def->variables) {
        mem.second->accept(this);
    }
    for(auto& func : def->functions()) {
        func->accept(this);
    }
}

void CommonVisitor::visit(StructMember *member) {
    member->type->accept(this);
    if(member->defValue) {
        member->defValue->accept(this);
    }
}

void CommonVisitor::visit(IfStatement *stmt) {
    stmt->condition->accept(this);
    stmt->ifBody.accept(this);
    for (auto& elif : stmt->elseIfs) {
        elif.first->accept(this);
        elif.second.accept(this);
    }
    if(stmt->elseBody.has_value()) {
        stmt->elseBody.value().accept(this);
    }
}

void CommonVisitor::visit(DestructStmt *stmt) {
    stmt->identifier->accept(this);
}

void CommonVisitor::visit(WhileLoop *loop) {
    loop->condition->accept(this);
    loop->body.accept(this);
}

void CommonVisitor::visit(DoWhileLoop *loop) {
    loop->body.accept(this);
    loop->condition->accept(this);
}

void CommonVisitor::visit(Namespace *ns) {
    for(const auto node : ns->nodes){
        node->accept(this);
    }
}

void CommonVisitor::visit(ForLoop *loop) {
    loop->initializer->accept(this);
    loop->incrementerExpr->accept(this);
    loop->body.accept(this);
}

void CommonVisitor::visit(SwitchStatement *stmt) {
    stmt->expression->accept(this);
    for(auto& scope : stmt->scopes) {
        scope.accept(this);
    }
}

void CommonVisitor::visit(AccessChain *chain) {
    for(auto& val : chain->values) {
        val->accept(this);
    }
}

void CommonVisitor::visit(StructValue *val) {
    for(auto& value : val->values) {
        value.second.value->accept(this);
    }
}

void CommonVisitor::visit(ArrayValue *arr) {
    for(auto& value : arr->values) {
        value->accept(this);
    }
}

void CommonVisitor::visit(ArrayType *type) {
    type->elem_type->accept(this);
}

void CommonVisitor::visit(NewValue *value) {
    value->value->accept(this);
}

void CommonVisitor::visit(PlacementNewValue *value) {
    value->pointer->accept(this);
    value->value->accept(this);
}

void CommonVisitor::visit(CastedValue *casted) {
    casted->value->accept(this);
}

void CommonVisitor::visit(ValueNode *node) {
    node->value->accept(this);
}

void CommonVisitor::visit(Expression *expr) {
    expr->firstValue->accept(this);
    expr->secondValue->accept(this);
}

void CommonVisitor::visit(UnsafeBlock *block) {
    block->scope.accept(this);
}