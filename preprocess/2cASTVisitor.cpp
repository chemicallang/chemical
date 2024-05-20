// Copyright (c) Qinetik 2024.

#include "2cASTVisitor.h"
#include <memory>
#include <ostream>
#include <random>
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

ToCAstVisitor::ToCAstVisitor(std::ostream &output) : output(output) {
    declarer = std::make_unique<CDeclareVisitor>(this);
}

int random(int min, int max) //range : [min, max]
{
    static bool first = true;
    if (first)
    {
        srand( time(NULL) ); //seeding for the first time only!
        first = false;
    }
    return min + rand() % (( max + 1 ) - min);
}

class ToCAstVisitor;

// will write a scope to visitor
void scope(ToCAstVisitor* visitor, Scope& scope) {
    visitor->write('{');
    visitor->indentation_level+=1;
    scope.accept(visitor);
    visitor->indentation_level-=1;
    visitor->new_line_and_indent();
    visitor->write('}');
}

void write_encoded(ToCAstVisitor* visitor, char value){
    switch(value) {
        case '\a':
            visitor->write("\\a");
            break;
        case '\f':
            visitor->write("\\f");
            break;
        case '\r':
            visitor->write("\\r");
            break;
        case '\n':
            visitor->write("\\n");
            break;
        case '\0':
            visitor->write("\\0");
            break;
        case '\t':
            visitor->write("\\t");
            break;
        case '\v':
            visitor->write("\\v");
            break;
        case '\b':
            visitor->write("\\b");
            break;
        case '\"':
            visitor->write("\\\"");
            break;
        case '\?':
            visitor->write("\\?");
            break;
        case '\x1b':
            visitor->write("\\x1b");
            break;
        default:
            visitor->write(value);
            break;
    }
}

void write_encoded(ToCAstVisitor* visitor, const std::string& value) {
    for(char c : value) {
        write_encoded(visitor, c);
    }
}

void write_type_post_id(ToCAstVisitor* visitor, BaseType* type) {
    if(type->kind() == BaseTypeKind::Array) {
        visitor->write('[');
        auto arrType = ((ArrayType*) type);
        if(arrType->array_size != -1) {
            visitor->write(std::to_string(arrType->array_size));
        }
        visitor->write(']');
        if(arrType->elem_type->kind() == BaseTypeKind::Array) {
            write_type_post_id(visitor, arrType->elem_type.get());
        }
    }
}

void assign_statement(ToCAstVisitor* visitor, AssignStatement* assign) {
    assign->lhs->accept(visitor);
    visitor->write(" = ");
    assign->value->accept(visitor);
}

void var_init(ToCAstVisitor* visitor, VarInitStatement* init) {
    BaseType* type;
    if (init->type.has_value()) {
        type = init->type.value().get();
        init->type.value()->accept(visitor);
    } else {
        auto created = init->value.value()->create_type();
        type = created.get();
        created->accept(visitor);
    }
    visitor->space();
    visitor->write(init->identifier);
    write_type_post_id(visitor, type);
    if(init->value.has_value()) {
        visitor->write(" = ");
        init->value.value()->accept(visitor);
    }
    visitor->write(';');
}

class SubVisitor {
public:

    /**
     * c visitor
     */
    ToCAstVisitor* visitor;

    /**
     * constructor
     */
    SubVisitor(ToCAstVisitor* visitor) : visitor(visitor) {

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

class CDeclareVisitor : public CommonVisitor, public SubVisitor {
public:

    using SubVisitor::SubVisitor;

    std::unordered_map<void*, std::string> aliases;

    unsigned lambda_num = 0;

    unsigned alias_num = 0;

    unsigned enum_num = 0;

    void visit(VarInitStatement *init) override;

    void visit(LambdaFunction *func) override;

    void visit(FunctionDeclaration *functionDeclaration) override;

    void visit(EnumDeclaration *enumDeclaration) override;

    void visit(StructDefinition *structDefinition) override;

    void visit(TypealiasStatement *statement) override;

};

void CDeclareVisitor::visit(VarInitStatement *init) {
    CommonVisitor::visit(init);
    if(!is_top_level_node) return;
    visitor->new_line_and_indent();
    var_init(visitor, init);
}

void CDeclareVisitor::visit(LambdaFunction *lamb) {
    CommonVisitor::visit(lamb);
    visitor->new_line_and_indent();
    lamb->func_type->returnType->accept(visitor);
    space();
    std::string lamb_name = "__chemda_";
    lamb_name += std::to_string(random(100,999)) + "_";
    lamb_name += std::to_string(lambda_num++);
    write(lamb_name);
    aliases[lamb] = lamb_name;
    write('(');
    unsigned i = 0;
    FunctionParam* param;
    auto size = lamb->func_type->isVariadic ? lamb->func_type->params.size() - 1 : lamb->func_type->params.size();
    while(i < size) {
        param = lamb->func_type->params[i].get();
        param->type->accept(visitor);
        space();
        write(param->name);
        if(i != lamb->func_type->params.size() - 1) {
            write(", ");
        }
        i++;
    }
    if(lamb->func_type->isVariadic) {
        write("...");
    }
    write(')');
    scope(visitor, lamb->scope);
}

void CDeclareVisitor::visit(FunctionDeclaration *decl) {
    CommonVisitor::visit(decl);
    visitor->new_line_and_indent();
    if(decl->returnType->kind() == BaseTypeKind::Void && decl->name == "main") {
        write("int");
    } else {
        decl->returnType->accept(visitor);
    }
    space();
    write(decl->name);
    write('(');
    unsigned i = 0;
    FunctionParam* param;
    auto size = decl->isVariadic ? decl->params.size() - 1 : decl->params.size();
    while(i < size) {
        param = decl->params[i].get();
        param->type->accept(visitor);
        space();
        write(param->name);
        if(i != decl->params.size() - 1) {
            write(", ");
        }
        i++;
    }
    if(decl->isVariadic) {
        write("...");
    }
    write(");");
}

void CDeclareVisitor::visit(EnumDeclaration *enumDecl) {
    unsigned i = 0;
    for(auto& mem : enumDecl->members) {
        visitor->new_line_and_indent();
        std::string value = ("__CHENUM_");
        value += (std::to_string(enum_num++));
        value += ('_');
        value += (enumDecl->name);
        value += (std::to_string(random(100, 999)));
        value += ("_");
        value += (mem.first);
        write("#define ");
        write(value);
        write(' ');
        write(std::to_string(mem.second->index));
        aliases[mem.second.get()] = value;
        i++;
    }
}

void CDeclareVisitor::visit(StructDefinition *def) {
    CommonVisitor::visit(def);
    visitor->new_line_and_indent();
    write("struct ");
    write(def->name);
    write(" {");
    visitor->indentation_level+=1;
    for(auto& var : def->variables) {
        visitor->new_line_and_indent();
        var.second->accept(visitor);
    }
    visitor->indentation_level-=1;
    visitor->new_line_and_indent();
    write("};");
}

void CDeclareVisitor::visit(TypealiasStatement *stmt) {
    visitor->new_line_and_indent();
    write("typedef ");
    stmt->to->accept(visitor);
    write(' ');
    if(is_top_level_node) {
        write(stmt->from);
    } else {
        std::string alias = "__chalias_";
        alias += std::to_string(random(100,999)) + "_";
        alias += std::to_string(alias_num++);
        write(alias);
        aliases[stmt] = alias;
    }
    write(';');
}

void ToCAstVisitor::translate(std::vector<std::unique_ptr<ASTNode>>& nodes) {

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
    if(top_level_node) return;
    var_init(this, init);
}

void ToCAstVisitor::visit(AssignStatement *assign) {
    assign_statement(this, assign);
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
    write("do ");
    scope(this, doWhileLoop->body);
    write(" while(");
    doWhileLoop->condition->accept(this);
    write(");");
}

void ToCAstVisitor::visit(EnumDeclaration *enumDecl) {

}

void ToCAstVisitor::visit(ForLoop *forLoop) {
    write("for(");
    forLoop->initializer->accept(this);
    forLoop->conditionExpr->accept(this);
    write(';');
    if(forLoop->incrementerExpr->as_assignment() != nullptr) {
        assign_statement(this, forLoop->incrementerExpr->as_assignment());
    } else {
        forLoop->incrementerExpr->accept(this);
    }
    write(')');
    scope(this, forLoop->body);
}

void ToCAstVisitor::visit(FunctionParam *functionParam) {
    write("[FunctionParam_UNIMPLEMENTED]");
}

void ToCAstVisitor::visit(FunctionDeclaration *decl) {
    if(!decl->body.has_value()) {
        return;
    }
    if(decl->returnType->kind() == BaseTypeKind::Void && decl->name == "main") {
        write("int");
    } else {
        decl->returnType->accept(this);
    }
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
    scope(this, decl->body.value());
}

void ToCAstVisitor::visit(IfStatement *decl) {
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

void ToCAstVisitor::visit(ImplDefinition *def) {
    write("impl ");
    write(def->interface_name);
    write(" {");
    indentation_level+=1;
    for(auto& var : def->variables) {
        new_line_and_indent();
        var.second->accept(this);
    }
    indentation_level-=1;
    new_line_and_indent();
    write("};");
    for(auto& var : def->functions) {
        new_line_and_indent();
        var.second->accept(this);
    }
}

void ToCAstVisitor::visit(InterfaceDefinition *def) {
    write("struct ");
    write(def->name);
    write(" {");
    indentation_level+=1;
    for(auto& var : def->variables) {
        new_line_and_indent();
        var.second->accept(this);
    }
    indentation_level-=1;
    new_line_and_indent();
    write("};");
    for(auto& var : def->functions) {
        new_line_and_indent();
        var.second->accept(this);
    }
}

void ToCAstVisitor::visit(Scope *scope) {
    auto prev = top_level_node;
    top_level_node = false;
    for(auto& node : scope->nodes) {
        new_line_and_indent();
        node->accept(this);
    }
    top_level_node = prev;
}

void ToCAstVisitor::visit(StructDefinition *structDefinition) {
    for(auto& var : structDefinition->functions) {
        new_line_and_indent();
        var.second->accept(this);
    }
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
        if(i != chain->values.size() - 1) {
            auto& next = chain->values[i + 1];
            if(next->as_func_call() == nullptr && next->as_index_op() == nullptr) {
                if(chain->values[i]->linked_node()->as_enum_decl() != nullptr) {
                    auto found = declarer->aliases.find(next->linked_node()->as_enum_member());
                    if(found != declarer->aliases.end()) {
                        write(found->second);
                        i++;
                    } else {
                        write("[EnumAC_NOT_FOUND:" + chain->values[i]->representation() + "." + next->representation() + "]");
                    }
                } else {
                    if(chain->values[i]->type_kind() == BaseTypeKind::Pointer) {
                        chain->values[i]->accept(this);
                        write("->");
                    } else {
                        chain->values[i]->accept(this);
                        write('.');
                    }
                }
            } else {
                chain->values[i]->accept(this);
            }
        } else {
            chain->values[i]->accept(this);
        }
        i++;
    }
}

void ToCAstVisitor::visit(MacroValueStatement *statement) {
    write("[MacroValueStatement_UNIMPLEMENTED]");
}

void ToCAstVisitor::visit(StructMember *member) {
    member->type->accept(this);
    space();
    write(member->name);
    write_type_post_id(this, member->type.get());
    write(';');
}

void ToCAstVisitor::visit(TypealiasStatement *stmt) {
    // declared above
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
    new_line_and_indent();
    write('}');
}

void ToCAstVisitor::visit(TryCatch *statement) {
    write("[TryCatch_UNIMPLEMENTED]");
}

void ToCAstVisitor::visit(IntValue *val) {
    write(std::to_string(val->value));
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
    write(std::to_string(numValue->get_num_value()));
}

void ToCAstVisitor::visit(FloatValue *val) {
    write(std::to_string(val->value));
}

void ToCAstVisitor::visit(DoubleValue *val) {
    write(std::to_string(val->value));
}

void ToCAstVisitor::visit(CharValue *val) {
    write('\'');
    write_encoded(this, val->value);
    write('\'');
}

void ToCAstVisitor::visit(StringValue *val) {
    write('"');
    write_encoded(this, val->value);
    write('"');
}

void ToCAstVisitor::visit(BoolValue *boolVal) {
    if(boolVal->value) {
        write('1');
    } else {
        write('0');
    }
}

void ToCAstVisitor::visit(ArrayValue *arr) {
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

void ToCAstVisitor::visit(StructValue *val) {
    write('{');
    for(auto& value : val->values) {
        write('.');
        write(value.first);
        write(" = ");
        value.second->accept(this);
        write(", ");
    }
    write('}');
}

void ToCAstVisitor::visit(VariableIdentifier *identifier) {
    write(identifier->value);
}

void ToCAstVisitor::visit(Expression *expr) {
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
    if(!nested_value) {
        write(';');
    }
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
    auto found = declarer->aliases.find(func);
    if(found != declarer->aliases.end()) {
        write(found->second);
    } else {
        write("[LambdaFunction_NOT_FOUND]");
    }
}

void ToCAstVisitor::visit(AnyType *func) {

}

void ToCAstVisitor::visit(ArrayType *type) {
    type->elem_type->accept(this);
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

void ToCAstVisitor::visit(ReferencedType *type) {
    if(type->linked->as_struct_def()) {
        write("struct ");
    }
    if(type->linked->as_typealias() != nullptr) {
        auto alias = declarer->aliases.find(type->linked->as_typealias());
        if(alias != declarer->aliases.end()) {
            write(alias->second);
            return;
        }
    }
    write(type->type);
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