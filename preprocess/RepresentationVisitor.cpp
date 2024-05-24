// Copyright (c) Qinetik 2024.

#include "RepresentationVisitor.h"
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
#include "ast/utils/CommonVisitor.h"
#include "utils/RepresentationUtils.h"
#include "ast/statements/Comment.h"

RepresentationVisitor::RepresentationVisitor(std::ostream &output) : output(output) {
//    declarer = std::make_unique<CValueDeclarationVisitor>(this);
//    tld = std::make_unique<CTopLevelDeclarationVisitor>(this, declarer.get());
}

class RepresentationVisitor;

// will write a scope to visitor
void scope(RepresentationVisitor* visitor, Scope& scope) {
    visitor->write('{');
    visitor->indentation_level+=1;
    scope.accept(visitor);
    visitor->indentation_level-=1;
    visitor->new_line_and_indent();
    visitor->write('}');
}

void write_encoded(RepresentationVisitor* visitor, const std::string& value) {
    for(char c : value) {
        visitor->write(escape_encode(c));
    }
}

class SubVisitor {
public:

    /**
     * c visitor
     */
    RepresentationVisitor* visitor;

    /**
     * constructor
     */
    SubVisitor(RepresentationVisitor* visitor) : visitor(visitor) {

    };

    /**
     * space fn using visitor
     */
    inline void space() const {
        visitor->space();
    }

    /**
     * write fn using visitor
     */
    inline void write(char value) const {
        visitor->write(value);
    }

    /**
     * write fn using visitor
     */
    inline void write(const std::string& value) const {
        visitor->write(value);
    }

};

class CTopLevelDeclarationVisitor : public Visitor, public SubVisitor {
public:

    CValueDeclarationVisitor* value_visitor;

    CTopLevelDeclarationVisitor(
            RepresentationVisitor* visitor,
            CValueDeclarationVisitor* value_visitor
    );

};

CTopLevelDeclarationVisitor::CTopLevelDeclarationVisitor(
        RepresentationVisitor *visitor,
        CValueDeclarationVisitor *value_visitor
) : SubVisitor(visitor), value_visitor(value_visitor) {

}

class CValueDeclarationVisitor : public CommonVisitor, public SubVisitor {
public:

    using SubVisitor::SubVisitor;

};

void RepresentationVisitor::prepare_translate() {

}

void RepresentationVisitor::translate(std::vector<std::unique_ptr<ASTNode>>& nodes) {

    // declare the top level things with this visitor
//    for(auto& node : nodes) {
//        node->accept(tld.get());
//    }

    // take out values like lambda from within functions
//    for(auto& node : nodes) {
//        node->accept(declarer.get());
//    }

    // writing
    for(auto& node : nodes) {
        new_line_and_indent();
        node->accept(this);
    }

}

RepresentationVisitor::~RepresentationVisitor() = default;

void RepresentationVisitor::visitCommon(ASTNode *node) {
    throw std::runtime_error("visitor common node called in 2c ASTVisitor");
}

void RepresentationVisitor::visitCommonValue(Value *value) {
    throw std::runtime_error("visitor common value called in 2c ASTVisitor");
}

void RepresentationVisitor::write(char value) {
    output.put(value);
}

void RepresentationVisitor::indent() {
    unsigned start = 0;
    while(start < indentation_level) {
        write('\t');
        start++;
    }
}

void RepresentationVisitor::write(const std::string& value) {
    output.write(value.c_str(), value.size());
}

void RepresentationVisitor::visit(VarInitStatement *init) {
    if (init->is_const) {
        write("const ");
    } else {
        write("var ");
    }
    write(init->identifier);
    if (init->type.has_value()) {
        write(" : ");
        init->type.value()->accept(this);
    }
    if (init->value.has_value()) {
        write(" = ");
        init->value.value()->accept(this);
    }
}

void RepresentationVisitor::visit(AssignStatement *stmt) {
    stmt->lhs->accept(this);
    if (stmt->assOp != Operation::Assignment) {
        write(" " + to_string(stmt->assOp) + "= ");
    } else {
        write(" = ");
    }
    stmt->value->accept(this);
}

void RepresentationVisitor::visit(BreakStatement *breakStatement) {
    write("break;");
}

void RepresentationVisitor::visit(Comment *comment) {
    write("//");
    write(comment->comment);
}

void RepresentationVisitor::visit(ContinueStatement *continueStatement) {
    write("continue;");
}

void RepresentationVisitor::visit(ImportStatement *importStatement) {
    // leave imports alone
}

void RepresentationVisitor::visit(ReturnStatement *returnStatement) {
    if(returnStatement->value.has_value()) {
        write("return ");
        returnStatement->value.value()->accept(this);
        write(';');
    } else {
        write("return;");
    }
}

void RepresentationVisitor::visit(DoWhileLoop *doWhileLoop) {
    write("do ");
    scope(this, doWhileLoop->body);
    write(" while(");
    doWhileLoop->condition->accept(this);
    write(");");
}

void RepresentationVisitor::visit(EnumDeclaration *enumDecl) {
    write("enum ");
    write(enumDecl->name);
    space();
    write("{");
    indentation_level+=1;
    unsigned int i = 0;
    for (const auto &member: enumDecl->members) {
        new_line_and_indent();
        write(member.first);
        if (i != enumDecl->members.size() - 1) {
            write(",");
        }
        i++;
    }
    indentation_level-=1;
    new_line_and_indent();
    write('}');
}

void RepresentationVisitor::visit(ForLoop *forLoop) {
    write("for(");
    forLoop->initializer->accept(this);
    forLoop->conditionExpr->accept(this);
    write(';');
    forLoop->incrementerExpr->accept(this);
    write(')');
    scope(this, forLoop->body);
}

void RepresentationVisitor::visit(FunctionParam *param) {
    write(param->name);
    write(" : ");
    param->type->accept(this);
}

void RepresentationVisitor::visit(FunctionDeclaration *decl) {
    write("func ");
    write(decl->name);
    write('(');
    int i = 0;
    while (i < decl->params.size()) {
        const auto &param = decl->params[i];
        param->accept(this);
        if (i < decl->params.size() - 1) {
            write(", ");
        } else {
            if (decl->isVariadic) {
                write("...");
            }
        }
        i++;
    }
    write(')');
    write(" : ");
    decl->returnType->accept(this);
    write(' ');
    if (decl->body.has_value()) {
        scope(this, decl->body.value());
    }
}

void RepresentationVisitor::visit(IfStatement *decl) {
    write("if(");
    nested_value = true;
    decl->condition->accept(this);
    nested_value = false;
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

void write_members(RepresentationVisitor* visitor, MembersContainer* container) {
    int i = 0;
    for (const auto &field: container->variables) {
        visitor->new_line_and_indent();
        field.second->accept(visitor);
        i++;
    }
    i = 0;
    for (const auto &field: container->functions) {
        visitor->new_line_and_indent();
        field.second->accept(visitor);
        i++;
    }
}

void RepresentationVisitor::visit(ImplDefinition *def) {
    write("impl ");
    write(def->interface_name);
    space();
    if (def->struct_name.has_value()) {
        write("for ");
        write(def->struct_name.value());
    }
    write("{");
    indentation_level+=1;
    write_members(this, def);
    indentation_level-=1;
    new_line_and_indent();
    write("}");
}

void RepresentationVisitor::visit(InterfaceDefinition *def) {
    write("interface ");
    write(def->name);
    space();
    write("{");
    indentation_level+=1;
    write_members(this, def);
    indentation_level-=1;
    new_line_and_indent();
    write("}");
}

void RepresentationVisitor::visit(Scope *scope) {
    auto prev = top_level_node;
    top_level_node = false;
    for(auto& node : scope->nodes) {
        new_line_and_indent();
        node->accept(this);
    }
    top_level_node = prev;
}

void RepresentationVisitor::visit(StructDefinition *def) {
    write("struct ");
    write(def->name);
    space();
    if (def->overrides.has_value()) {
        write(": ");
        def->overrides.value()->accept(this);
    }
    write("{");
    indentation_level+=1;
    write_members(this, def);
    indentation_level-=1;
    new_line_and_indent();
    write("}");
}

void RepresentationVisitor::visit(WhileLoop *whileLoop) {
    write("while(");
    whileLoop->condition->accept(this);
    write(") ");
    scope(this, whileLoop->body);
}

void RepresentationVisitor::visit(AccessChain *chain) {
    int i = 0;
    while (i < chain->values.size()) {
        chain->values[i]->accept(this);
        if (i != chain->values.size() - 1) {
            write('.');
        }
        i++;
    }
}

void RepresentationVisitor::visit(MacroValueStatement *statement) {
    write("[MacroValueStatement_UNIMPLEMENTED]");
}

void RepresentationVisitor::visit(StructMember *member) {
    write("var ");
    write(member->name);
    write(" : ");
    member->type->accept(this);
    if (member->defValue.has_value()) {
        write(" = ");
        member->defValue.value()->accept(this);
    }
    write(';');
}

void RepresentationVisitor::visit(TypealiasStatement *stmt) {
    write("typealias ");
    write(stmt->from);
    write(" = ");
    stmt->to->accept(this);
}

void RepresentationVisitor::visit(SwitchStatement *statement) {
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
    new_line_and_indent();
    write('}');
}

void RepresentationVisitor::visit(TryCatch *statement) {
    write("[TryCatch_UNIMPLEMENTED]");
}

void RepresentationVisitor::visit(IntValue *val) {
    write(std::to_string(val->value));
}

void RepresentationVisitor::visit(BigIntValue *val) {
    write(std::to_string(val->value));
}

void RepresentationVisitor::visit(LongValue *val) {
    write(std::to_string(val->value));
}

void RepresentationVisitor::visit(ShortValue *val) {
    write(std::to_string(val->value));
}

void RepresentationVisitor::visit(UBigIntValue *val) {
    write(std::to_string(val->value));
}

void RepresentationVisitor::visit(UIntValue *val) {
    write(std::to_string(val->value));
}

void RepresentationVisitor::visit(ULongValue *val) {
    write(std::to_string(val->value));
}

void RepresentationVisitor::visit(UShortValue *val) {
    write(std::to_string(val->value));
}

void RepresentationVisitor::visit(Int128Value *val) {
    write(std::to_string(val->get_num_value()));
}

void RepresentationVisitor::visit(UInt128Value *val) {
    write(std::to_string(val->get_num_value()));
}

void RepresentationVisitor::visit(NumberValue *numValue) {
    write(std::to_string(numValue->get_num_value()));
}

void RepresentationVisitor::visit(FloatValue *val) {
    write(std::to_string(val->value));
}

void RepresentationVisitor::visit(DoubleValue *val) {
    write(std::to_string(val->value));
}

void RepresentationVisitor::visit(CharValue *val) {
    write('\'');
    write(escape_encode(val->value));
    write('\'');
}

void RepresentationVisitor::visit(StringValue *val) {
    write('"');
    write_encoded(this, val->value);
    write('"');
}

void RepresentationVisitor::visit(BoolValue *boolVal) {
    if(boolVal->value) {
        write("true");
    } else {
        write("false");
    }
}

void RepresentationVisitor::visit(ArrayValue *arr) {
    write('{');
    unsigned i = 0;
    while(i < arr->values.size()) {
        arr->values[i]->accept(this);
        if(i != arr->values.size() - 1) {
            write(',');
        }
        i++;
    }
    write('}');
}

void RepresentationVisitor::visit(StructValue *val) {
    write(val->structName);
    write(" {");
    indentation_level += 1;
    unsigned i = 0;
    for (const auto &value: val->values) {
        new_line_and_indent();
        write(value.first);
        write(" : ");
        value.second->accept(this);
        if (i < val->values.size() - 1) write(",");
        i++;
    }
    indentation_level -= 1;
    new_line_and_indent();
    write('}');
}

void RepresentationVisitor::visit(VariableIdentifier *identifier) {
    write(identifier->value);
}

void RepresentationVisitor::visit(Expression *expr) {
    write('(');
    nested_value = true;
    expr->firstValue->accept(this);
    space();
    write(to_string(expr->operation));
    space();
    expr->secondValue->accept(this);
    nested_value = false;
    write(')');
}

void RepresentationVisitor::visit(CastedValue *casted) {
    write('(');
    casted->value->accept(this);
    write(" as ");
    casted->type->accept(this);
    write(')');
}

void RepresentationVisitor::visit(AddrOfValue *casted) {
    write('&');
    casted->value->accept(this);
}

void RepresentationVisitor::visit(DereferenceValue *casted) {
    write('*');
    casted->value->accept(this);
}

void RepresentationVisitor::visit(FunctionCall *call) {
    write('(');
    int i = 0;
    while (i < call->values.size()) {
        call->values[i]->accept(this);
        if (i != call->values.size() - 1) {
            write(',');
        }
        i++;
    }
    write(')');
}

void RepresentationVisitor::visit(IndexOperator *op) {
    unsigned i = 0;
    while(i < op->values.size()) {
        write('[');
        auto& val = op->values[i];
        val->accept(this);
        write(']');
        i++;
    }
}

void RepresentationVisitor::visit(NegativeValue *negValue) {
    write('-');
    negValue->value->accept(this);
}

void RepresentationVisitor::visit(NotValue *notValue) {
    write('!');
    notValue->value->accept(this);
}

void RepresentationVisitor::visit(NullValue *nullValue) {
    write("null");
}

void RepresentationVisitor::visit(TernaryValue *ternary) {

}

void RepresentationVisitor::visit(LambdaFunction *lamb) {
    write('[');
    unsigned i = 0;
    unsigned size = lamb->captureList.size();
    while(i < size) {
        lamb->captureList[i]->accept(this);
        if(i < size - 1){
            write(',');
        }
        i++;
    }
    write("](");
    i = 0;
    size = lamb->params.size();
    while(i < size) {
        lamb->params[i]->accept(this);
        if(i < size - 1){
            write(',');
        }
        i++;
    }
    write(") => ");
    scope(this, lamb->scope);
}

void RepresentationVisitor::visit(AnyType *func) {
    write("any");
}

void RepresentationVisitor::visit(ArrayType *type) {
    write("[]");
    type->elem_type->accept(this);
}

void RepresentationVisitor::visit(BigIntType *func) {
    write("bigint");
}

void RepresentationVisitor::visit(BoolType *func) {
    write("bool");
}

void RepresentationVisitor::visit(CharType *func) {
    write("char");
}

void RepresentationVisitor::visit(DoubleType *func) {
    write("double");
}

void RepresentationVisitor::visit(FloatType *func) {
    write("float");
}

void RepresentationVisitor::visit(FunctionType *type) {
    write('(');
    unsigned i = 0;
    auto size = type->params.size();
    while(i < size) {
        type->params[i]->accept(this);
        if(i < size - 1) {
            write(", ");
        }
        i++;
    }
    write(") => ");
    type->returnType->accept(this);
}

void RepresentationVisitor::visit(GenericType *func) {
    write("[GenericType_UNIMPLEMENTED]");
}

void RepresentationVisitor::visit(Int128Type *func) {
    write("int128");
}

void RepresentationVisitor::visit(IntType *func) {
    write("int");
}

void RepresentationVisitor::visit(LongType *func) {
    write("long");
}

void RepresentationVisitor::visit(PointerType *func) {
    func->type->accept(this);
    write('*');
}

void RepresentationVisitor::visit(ReferencedType *type) {
    write(type->type);
}

void RepresentationVisitor::visit(ShortType *func) {
    write("short");
}

void RepresentationVisitor::visit(StringType *func) {
    write("string");
}

void RepresentationVisitor::visit(StructType *val) {
    write("[StructType_UNIMPLEMENTED]");
}

void RepresentationVisitor::visit(UBigIntType *func) {
    write("ubigint");
}

void RepresentationVisitor::visit(UInt128Type *func) {
    write("uint128");
}

void RepresentationVisitor::visit(UIntType *func) {
    write("uint");
}

void RepresentationVisitor::visit(ULongType *func) {
    write("ulong");
}

void RepresentationVisitor::visit(UShortType *func) {
    write("ushort");
}

void RepresentationVisitor::visit(VoidType *func) {
    write("void");
}