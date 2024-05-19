// Copyright (c) Qinetik 2024.

#include "2cASTVisitor.h"
#include <memory>
#include <ostream>
#include "ast/statements/VarInit.h"
#include "ast/statements/Typealias.h"
#include "ast/statements/Continue.h"
#include "ast/statements/Break.h"
#include "ast/statements/Return.h"
#include "ast/statements/Assignment.h"
#include "ast/statements/SwitchStatement.h"
#include "ast/statements/MacroValueStatement.h"
#include "ast/statements/Import.h"
#include "ast/structures/EnumMember.h"
#include "ast/structures/EnumDeclaration.h"
#include "ast/structures/StructMember.h"
#include "ast/structures/ImplDefinition.h"
#include "ast/structures/FunctionParam.h"
#include "ast/structures/InterfaceDefinition.h"
#include "ast/structures/FunctionDeclaration.h"
#include "ast/structures/TryCatch.h"
#include "ast/structures/DoWhileLoop.h"
#include "ast/structures/If.h"
#include "ast/structures/StructDefinition.h"
#include "ast/structures/ForLoop.h"
#include "ast/structures/LoopScope.h"
#include "ast/structures/CapturedVariable.h"
#include "ast/structures/MembersContainer.h"
#include "ast/structures/Scope.h"
#include "ast/structures/WhileLoop.h"
#include "ast/types/ReferencedType.h"
#include "ast/types/PointerType.h"
#include "ast/types/FunctionType.h"
#include "ast/types/GenericType.h"
#include "ast/types/AnyType.h"
#include "ast/types/ArrayType.h"
#include "ast/types/BigIntType.h"
#include "ast/types/BoolType.h"
#include "ast/types/CharType.h"
#include "ast/types/DoubleType.h"
#include "ast/types/FloatType.h"
#include "ast/types/Int128Type.h"
#include "ast/types/IntNType.h"
#include "ast/types/IntType.h"
#include "ast/types/LongType.h"
#include "ast/types/ShortType.h"
#include "ast/types/StringType.h"
#include "ast/types/StructType.h"
#include "ast/types/UBigIntType.h"
#include "ast/types/UInt128Type.h"
#include "ast/types/UIntType.h"
#include "ast/types/ULongType.h"
#include "ast/types/UShortType.h"
#include "ast/types/VoidType.h"
#include "ast/values/UShortValue.h"
#include "ast/values/VariableIdentifier.h"
#include "ast/values/IntValue.h"
#include "ast/values/DoubleValue.h"
#include "ast/values/FunctionCall.h"
#include "ast/values/LambdaFunction.h"
#include "ast/values/CastedValue.h"
#include "ast/values/AccessChain.h"
#include "ast/values/StructValue.h"
#include "ast/values/AddrOfValue.h"
#include "ast/values/ArrayValue.h"
#include "ast/values/BigIntValue.h"
#include "ast/values/BoolValue.h"
#include "ast/values/CharValue.h"
#include "ast/values/DereferenceValue.h"
#include "ast/values/Expression.h"
#include "ast/values/FloatValue.h"
#include "ast/values/IndexOperator.h"
#include "ast/values/Int128Value.h"
#include "ast/values/IntNumValue.h"
#include "ast/values/LongValue.h"
#include "ast/values/Negative.h"
#include "ast/values/NotValue.h"
#include "ast/values/NullValue.h"
#include "ast/values/NumberValue.h"
#include "ast/values/ShortValue.h"
#include "ast/values/StringValue.h"
#include "ast/values/TernaryValue.h"
#include "ast/values/UBigIntValue.h"
#include "ast/values/UInt128Value.h"
#include "ast/values/UIntValue.h"
#include "ast/values/ULongValue.h"

ToCAstVisitor::ToCAstVisitor(std::ostream &output) : output(output) {

}

class ToCAstVisitor;

class CDeclareVisitor : public Visitor {
public:

    /**
     * c visitor
     */
    ToCAstVisitor* visitor;

    /**
     * constructor
     */
    CDeclareVisitor(ToCAstVisitor* visitor) : visitor(visitor) {

    };

};

void ToCAstVisitor::translate(std::vector<std::unique_ptr<ASTNode>>& nodes) {

    declarer = std::make_unique<CDeclareVisitor>(this);

    // declaring things using declaration visitor
    for(auto& node : nodes) {
        node->accept(declarer.get());
    }

    // writing
    for(auto& node : nodes) {
        new_line_and_indent();
        node->accept(this);
    }

}

ToCAstVisitor::~ToCAstVisitor() = default;

void ToCAstVisitor::visitCommon(ASTNode *node) {
    throw std::runtime_error("visitor common node called in 2c ASTVisitor");
}

void ToCAstVisitor::visitCommonValue(Value *value) {
    throw std::runtime_error("visitor common value called in 2c ASTVisitor");
}

void ToCAstVisitor::write(char value) {
    output.put(value);
}

// will write a scope to visitor
void scope(ToCAstVisitor* visitor, Scope& scope) {
    visitor->write('{');
    visitor->indentation_level+=1;
    scope.accept(visitor);
    visitor->indentation_level-=1;
    visitor->new_line_and_indent();
    visitor->write('}');
}

void ToCAstVisitor::indent() {
    unsigned start = 0;
    while(start < indentation_level) {
        write('\t');
        start++;
    }
}

void ToCAstVisitor::write(const std::string& value) {
    output.write(value.c_str(), value.size());
}

void ToCAstVisitor::visit(VarInitStatement *init) {
    if (init->type.has_value()) {
        init->type.value()->accept(this);
    } else {
        init->value.value()->create_type()->accept(this);
    }
    space();
    write(init->identifier);
    if(init->value.has_value()) {
        write(" = ");
        init->value.value()->accept(this);
    }
    write(';');
}

void ToCAstVisitor::visit(AssignStatement *assign) {
    assign->lhs->accept(this);
    write(" = ");
    assign->value->accept(this);
    write(';');
}

void ToCAstVisitor::visit(BreakStatement *breakStatement) {
    write("break;");
}

void ToCAstVisitor::visit(Comment *comment) {
    // leave comments alone
}

void ToCAstVisitor::visit(ContinueStatement *continueStatement) {
    write("continue;");
}

void ToCAstVisitor::visit(ImportStatement *importStatement) {
    // leave imports alone
}

void ToCAstVisitor::visit(ReturnStatement *returnStatement) {
    if(returnStatement->value.has_value()) {
        write("return ");
        returnStatement->value.value()->accept(this);
        write(';');
    } else {
        write("return;");
    }
}

void ToCAstVisitor::visit(DoWhileLoop *doWhileLoop) {
    write("do {");
    new_line();
    indentation_level+=1;
    doWhileLoop->body.accept(this);
    indentation_level-=1;
    new_line_and_indent();
    write("} while(");
    doWhileLoop->condition->accept(this);
    write(");");
}

void ToCAstVisitor::visit(EnumDeclaration *enumDecl) {
    write("enum ");
    write(enumDecl->name);
    write(" {");
    indentation_level+=1;
    for(auto& mem : enumDecl->members) {
        new_line_and_indent();
        write(mem.second->name);
        write("=");
        write(std::to_string(mem.second->index));
        write(',');
    }
    indentation_level-=1;
    write('}');
}

void ToCAstVisitor::visit(ForLoop *forLoop) {
    write("for(");
    forLoop->initializer->accept(this);
    write(';');
    forLoop->conditionExpr->accept(this);
    write(';');
    forLoop->incrementerExpr->accept(this);
    write(')');
    scope(this, forLoop->body);
}

void ToCAstVisitor::visit(FunctionParam *functionParam) {
    write("[FunctionParam_UNIMPLEMENTED]");
}

void ToCAstVisitor::visit(FunctionDeclaration *decl) {
    decl->returnType->accept(this);
    space();
    write(decl->name);
    write('(');
    unsigned i = 0;
    FunctionParam* param;
    while(i < decl->params.size()) {
        param = decl->params[i].get();
        param->type->accept(this);
        space();
        write(param->name);
        if(i != decl->params.size() - 1) {
            write(", ");
        }
        i++;
    }
    write(')');
    if(decl->body.has_value()) {
        scope(this, decl->body.value());
    } else {
        write(';');
    }
}

void ToCAstVisitor::visit(IfStatement *decl) {
    write("if(");
    decl->condition->accept(this);
    write(')');
    scope(this, decl->ifBody);
    unsigned i = 0;
    while(i < decl->elseIfs.size()) {
        auto& elif = decl->elseIfs[i];
        write("else if(");
        elif.first->accept(this);
        write(')');
        scope(this, elif.second);
        i++;
    }
    if(decl->elseBody.has_value()) {
        write(" else ");
        scope(this, decl->elseBody.value());
    }
}

void ToCAstVisitor::visit(ImplDefinition *implDefinition) {
    write("[ImplDefinition_UNIMPLEMENTED]");
}

void ToCAstVisitor::visit(InterfaceDefinition *interfaceDefinition) {
    write("[InterfaceDefinition_UNIMPLEMENTED]");
}

void ToCAstVisitor::visit(Scope *scope) {
    for(auto& node : scope->nodes) {
        new_line_and_indent();
        node->accept(this);
    }
}

void ToCAstVisitor::visit(StructDefinition *structDefinition) {
    write("[StructDefinition_UNIMPLEMENTED]");
}

void ToCAstVisitor::visit(WhileLoop *whileLoop) {
    write("while(");
    whileLoop->condition->accept(this);
    write(") ");
    scope(this, whileLoop->body);

}

void ToCAstVisitor::visit(AccessChain *chain) {
    if(chain->values.size() == 1) {
        chain->values[0]->accept(this);
        return;
    }
    unsigned i = 0;
    while(i < chain->values.size()) {
        chain->values[i]->accept(this);
        if(i != chain->values.size() - 1 && chain->values[i + 1]->as_func_call() == nullptr) {
            write('.');
        }
        i++;
    }
}

void ToCAstVisitor::visit(MacroValueStatement *statement) {
    write("[MacroValueStatement_UNIMPLEMENTED]");
}

void ToCAstVisitor::visit(StructMember *member) {
    write("[StructMember_UNIMPLEMENTED]");
}

void ToCAstVisitor::visit(TypealiasStatement *statement) {
    write("[TypealiasStatement_UNIMPLEMENTED]");
}

void ToCAstVisitor::visit(SwitchStatement *statement) {
    write("switch(");
    statement->expression->accept(this);
    write(") {");
    unsigned i = 0;
    indentation_level += 1;
    while(i < statement->scopes.size()) {
        auto& scope = statement->scopes[i];
        new_line_and_indent();

        write("case ");
        scope.first->accept(this);
        write(':');

        indentation_level += 1;
        scope.second.accept(this);
        indentation_level -= 1;
        i++;
    }
    indentation_level -= 1;
    write('}');
}

void ToCAstVisitor::visit(TryCatch *statement) {
    write("[TryCatch_UNIMPLEMENTED]");
}

void ToCAstVisitor::visit(BigIntValue *val) {
    write(std::to_string(val->value));
}

void ToCAstVisitor::visit(LongValue *val) {
    write(std::to_string(val->value));
}

void ToCAstVisitor::visit(ShortValue *val) {
    write(std::to_string(val->value));
}

void ToCAstVisitor::visit(UBigIntValue *val) {
    write(std::to_string(val->value));
}

void ToCAstVisitor::visit(UIntValue *val) {
    write(std::to_string(val->value));
}

void ToCAstVisitor::visit(ULongValue *val) {
    write(std::to_string(val->value));
}

void ToCAstVisitor::visit(UShortValue *val) {
    write(std::to_string(val->value));
}

void ToCAstVisitor::visit(Int128Value *val) {
    write(std::to_string(val->get_num_value()));
}

void ToCAstVisitor::visit(UInt128Value *val) {
    write(std::to_string(val->get_num_value()));
}

void ToCAstVisitor::visit(NumberValue *numValue) {
    write("[NumberValue_UNIMPLEMENTED]");
}

void ToCAstVisitor::visit(FloatValue *val) {
    write(std::to_string(val->value));
}

void ToCAstVisitor::visit(DoubleValue *val) {
    write(std::to_string(val->value));
}

void ToCAstVisitor::visit(CharValue *val) {
    write('\'');
    write(val->value);
    write('\'');
}

void ToCAstVisitor::visit(StringValue *val) {
    write('"');
    write(val->value);
    write('"');
}

void ToCAstVisitor::visit(BoolValue *boolVal) {
    if(boolVal->value) {
        write("true");
    } else {
        write("false");
    }
}

void ToCAstVisitor::visit(ArrayValue *arrayVal) {
    write("[ArrayValue_UNIMPLEMENTED]");
}

void ToCAstVisitor::visit(StructValue *structValue) {
    write("[StructValue_UNIMPLEMENTED]");
}

void ToCAstVisitor::visit(VariableIdentifier *identifier) {
    write(identifier->value);
}

void ToCAstVisitor::visit(Expression *expr) {
    write('(');
    expr->firstValue->accept(this);
    space();
    to_string(expr->operation);
    space();
    expr->secondValue->accept(this);
    write(')');
}

void ToCAstVisitor::visit(CastedValue *casted) {
    write('(');
    write('(');
    casted->type->accept(this);
    write(')');
    write(' ');
    casted->value->accept(this);
    write(')');
}

void ToCAstVisitor::visit(AddrOfValue *casted) {
    write('&');
    casted->value->accept(this);
}

void ToCAstVisitor::visit(DereferenceValue *casted) {
    write('*');
    casted->value->accept(this);
}

void ToCAstVisitor::visit(FunctionCall *call) {
    write('(');
    unsigned i = 0;
    while(i < call->values.size()) {
        auto& val = call->values[i];
        val->accept(this);
        if(i != call->values.size() - 1) {
            write(", ");
        }
        i++;
    }
    write(')');
}

void ToCAstVisitor::visit(IndexOperator *op) {
    unsigned i = 0;
    while(i < op->values.size()) {
        write('[');
        auto& val = op->values[i];
        val->accept(this);
        write(']');
        i++;
    }
}

void ToCAstVisitor::visit(NegativeValue *negValue) {
    write('-');
    negValue->value->accept(this);
}

void ToCAstVisitor::visit(NotValue *notValue) {
    write('!');
    notValue->value->accept(this);
}

void ToCAstVisitor::visit(NullValue *nullValue) {
    write("NULL");
}

void ToCAstVisitor::visit(TernaryValue *ternary) {

}

void ToCAstVisitor::visit(LambdaFunction *func) {
    write("[LambdaFunction_UNIMPLEMENTED]");
}

void ToCAstVisitor::visit(AnyType *func) {

}

void ToCAstVisitor::visit(ArrayType *func) {
    write("[ArrayType_UNIMPLEMENTED]");
}

void ToCAstVisitor::visit(BigIntType *func) {
    write("long long");
}

void ToCAstVisitor::visit(BoolType *func) {
    write("_Bool");
}

void ToCAstVisitor::visit(CharType *func) {
    write("char");
}

void ToCAstVisitor::visit(DoubleType *func) {
    write("double");
}

void ToCAstVisitor::visit(FloatType *func) {
    write("float");
}

void ToCAstVisitor::visit(FunctionType *func) {
    // TODO function pointers
    write("void*");
}

void ToCAstVisitor::visit(GenericType *func) {
    write("[GenericType_UNIMPLEMENTED]");
}

void ToCAstVisitor::visit(Int128Type *func) {
    write("[Int128Type_UNIMPLEMENTED]");
}

void ToCAstVisitor::visit(IntType *func) {
    write("int");
}

void ToCAstVisitor::visit(LongType *func) {
    write("long");
}

void ToCAstVisitor::visit(PointerType *func) {
    func->type->accept(this);
    write('*');
}

void ToCAstVisitor::visit(ReferencedType *func) {
    write(func->type);
}

void ToCAstVisitor::visit(ShortType *func) {
    write("short");
}

void ToCAstVisitor::visit(StringType *func) {
    write("char*");
}

void ToCAstVisitor::visit(StructType *val) {
    write("[StructType_UNIMPLEMENTED]");
}

void ToCAstVisitor::visit(UBigIntType *func) {
    write("unsigned long long");
}

void ToCAstVisitor::visit(UInt128Type *func) {
    write("[UInt128Type_UNIMPLEMENTED]");
}

void ToCAstVisitor::visit(UIntType *func) {
    write("unsigned int");
}

void ToCAstVisitor::visit(ULongType *func) {
    write("unsigned long");
}

void ToCAstVisitor::visit(UShortType *func) {
    write("unsigned short");
}

void ToCAstVisitor::visit(VoidType *func) {
    write("void");
}