// Copyright (c) Chemical Language Foundation 2025.

#include "RepresentationVisitor.h"
#include <memory>
#include <ostream>
#include "ast/statements/VarInit.h"
#include "ast/statements/Typealias.h"
#include "ast/statements/Continue.h"
#include "ast/statements/Break.h"
#include "ast/statements/Return.h"
#include "ast/statements/AccessChainNode.h"
#include "ast/statements/Assignment.h"
#include "ast/statements/SwitchStatement.h"
#include "ast/statements/DeallocStmt.h"
#include "ast/statements/Export.h"
#include "ast/statements/Import.h"
#include "ast/structures/EnumMember.h"
#include "ast/structures/VariantMember.h"
#include "ast/structures/EnumDeclaration.h"
#include "ast/structures/StructMember.h"
#include "ast/structures/ImplDefinition.h"
#include "ast/structures/VariantDefinition.h"
#include "ast/structures/FunctionParam.h"
#include "ast/structures/LoopBlock.h"
#include "ast/statements/DestructStmt.h"
#include "ast/values/ValueNode.h"
#include "ast/values/VariantCase.h"
#include "ast/values/IsValue.h"
#include "ast/values/ExpressiveString.h"
#include "ast/values/DynamicValue.h"
#include "ast/structures/InterfaceDefinition.h"
#include "ast/structures/FunctionDeclaration.h"
#include "ast/structures/TryCatch.h"
#include "ast/structures/DoWhileLoop.h"
#include "ast/statements/ValueWrapperNode.h"
#include "ast/statements/IncDecNode.h"
#include "ast/statements/PatternMatchExprNode.h"
#include "ast/statements/PlacementNewNode.h"
#include "ast/types/DynamicType.h"
#include "ast/types/MaybeRuntimeType.h"
#include "ast/types/RuntimeType.h"
#include "ast/structures/If.h"
#include "ast/structures/StructDefinition.h"
#include "ast/values/SizeOfValue.h"
#include "ast/values/AlignOfValue.h"
#include "ast/values/InValue.h"
#include "ast/types/LinkedValueType.h"
#include "ast/structures/Namespace.h"
#include "ast/structures/UnsafeBlock.h"
#include "ast/structures/ForLoop.h"
#include "ast/structures/CapturedVariable.h"
#include "ast/structures/MembersContainer.h"
#include "ast/structures/Scope.h"
#include "ast/structures/WhileLoop.h"
#include "ast/types/LinkedType.h"
#include "ast/types/PointerType.h"
#include "ast/types/ReferenceType.h"
#include "ast/types/GenericType.h"
#include "ast/types/AnyType.h"
#include "ast/types/ArrayType.h"
#include "ast/types/BoolType.h"
#include "ast/types/DoubleType.h"
#include "ast/types/FloatType.h"
#include "ast/types/IntNType.h"
#include "ast/types/CapturingFunctionType.h"
#include "ast/types/LinkedType.h"
#include "ast/types/StringType.h"
#include "ast/types/StructType.h"
#include "ast/types/ComplexType.h"
#include "ast/types/LiteralType.h"
#include "ast/types/VoidType.h"
#include "ast/values/VariableIdentifier.h"
#include "ast/values/DoubleValue.h"
#include "ast/values/FunctionCall.h"
#include "ast/values/LambdaFunction.h"
#include "ast/values/CastedValue.h"
#include "ast/values/AccessChain.h"
#include "ast/values/StructValue.h"
#include "ast/values/AddrOfValue.h"
#include "ast/values/ArrayValue.h"
#include "ast/values/BoolValue.h"
#include "ast/values/DereferenceValue.h"
#include "ast/values/Expression.h"
#include "ast/values/FloatValue.h"
#include "ast/values/IndexOperator.h"
#include "ast/values/IntNumValue.h"
#include "ast/values/Negative.h"
#include "ast/values/NotValue.h"
#include "ast/values/NullValue.h"
#include "ast/values/StringValue.h"
#include "ast/values/UnsafeValue.h"
#include "ast/values/ComptimeValue.h"
#include "ast/values/IfValue.h"
#include "ast/values/SwitchValue.h"
#include "ast/values/LoopValue.h"
#include "utils/RepresentationUtils.h"
#include "preprocess/2c/2cASTVisitor.h"

RepresentationVisitor::RepresentationVisitor(std::ostream &output) : output(output) {
//    declarer = std::make_unique<CValueDeclarationVisitor>(this);
//    tld = std::make_unique<CTopLevelDeclarationVisitor>(this, declarer.get());
}

class RepresentationVisitor;

// will write a scope to visitor
void scope(RepresentationVisitor& visitor, Scope& scope) {
    visitor.write('{');
    visitor.indentation_level+=1;
    visitor.visit(&scope);
    visitor.indentation_level-=1;
    visitor.new_line_and_indent();
    visitor.write('}');
}

void write_encoded(RepresentationVisitor& visitor, const chem::string_view& value) {
    auto& out = visitor.output;
    for(char c : value) {
        write_escape_encoded(out, c);
    }
}

void RepresentationVisitor::prepare_translate() {

}

void RepresentationVisitor::translate(std::vector<ASTNode*>& nodes) {

    // declare the top level things with this visitor
//    for(auto& node : nodes) {
//        node->accept(tld.get());
//    }

    // take out values like lambda from within functions
//    for(auto& node : nodes) {
//        node->accept(declarer.get());
//    }

    // writing
    for(const auto node : nodes) {
        new_line_and_indent();
        visit(node);
    }

}

RepresentationVisitor::~RepresentationVisitor() = default;

void RepresentationVisitor::VisitCommonNode(ASTNode* node) {
#ifdef DEBUG
    CHEM_THROW_RUNTIME("RepresentationVisitor::VisitCommonNode called");
#endif
}

void RepresentationVisitor::VisitCommonValue(Value* value) {
#ifdef DEBUG
    CHEM_THROW_RUNTIME("RepresentationVisitor::VisitCommonValue called");
#endif
}

void RepresentationVisitor::VisitCommonType(BaseType* type) {
#ifdef DEBUG
    CHEM_THROW_RUNTIME("RepresentationVisitor::VisitCommonType called");
#endif
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

void RepresentationVisitor::write_str(const std::string& value) {
    output.write(value.c_str(), (std::streamsize) value.size());
}

void RepresentationVisitor::write_view(std::string_view& view) {
    output.write(view.data(), (std::streamsize) view.size());
}

void RepresentationVisitor::write(const chem::string_view& view) {
    output.write(view.data(), (std::streamsize) view.size());
}

void RepresentationVisitor::VisitVarInitStmt(VarInitStatement *init) {
    if (init->is_const()) {
        write("const ");
    } else {
        write("var ");
    }
    write(init->id_view());
    if (init->type) {
        write(" : ");
        visit(init->type);
    }
    if (init->value) {
        write(" = ");
        visit(init->value);
    }
}

void RepresentationVisitor::VisitAssignmentStmt(AssignStatement *stmt) {
    visit(stmt->lhs);
    if (stmt->assOp != Operation::Assignment) {
        write(' ');
        write(to_string(stmt->assOp));
        write("= ");
    } else {
        write(" = ");
    }
    visit(stmt->value);
}

void RepresentationVisitor::VisitBreakStmt(BreakStatement *breakStatement) {
    write("break;");
}

void RepresentationVisitor::VisitContinueStmt(ContinueStatement *continueStatement) {
    write("continue;");
}

void write_import_item(RepresentationVisitor& visitor, const ImportItem& item) {
    bool first_item = true;
    for(const auto& part : item.parts) {
        if(first_item) {
            first_item = false;
        } else {
            visitor.write('.');
        }
        visitor.write(part);
    }
    if(!item.alias.empty()) {
        visitor.write(" as ");
        visitor.write(item.alias);
    }
}

void RepresentationVisitor::VisitImportStmt(ImportStatement *stmt) {
    write("import ");

    auto& import_items = stmt->getImportItems();
    if(!import_items.empty()) {
        write("{ ");
        bool first_item = true;
        for(const auto& item : import_items) {
            if(first_item) {
                first_item = false;
            } else {
                write(", ");
            }
            write_import_item(*this, item);
        }
        write(" } from ");
    }

    if(stmt->isNativeLibImport()) {
        write(stmt->getSourcePath());
    } else {
        write('"');
        write(stmt->getSourcePath());
        write('"');
    }

    const auto& alias = stmt->getTopLevelAlias();
    if(!alias.empty()) {
        write(" as ");
        write(alias);
    }

    const auto& version = stmt->getVersion();
    const auto& subdir = stmt->getSubdir();
    const auto& branch = stmt->getBranch();
    const auto& commit = stmt->getCommit();

    if(!version.empty()) {
        write(" version ");
        write(version);
    }

    if(!subdir.empty()) {
        write(" subdir ");
        write(subdir);
    }

    if(!branch.empty()) {
        write(" branch ");
        write(branch);
    }

    if(!commit.empty()) {
        write(" commit ");
        write(commit);
    }

    // TODO: write the if conditional

}

void RepresentationVisitor::VisitReturnStmt(ReturnStatement *returnStatement) {
    if(returnStatement->value) {
        write("return ");
        visit(returnStatement->value);
        write(';');
    } else {
        write("return;");
    }
}

void RepresentationVisitor::VisitDoWhileLoopStmt(DoWhileLoop *doWhileLoop) {
    write("do ");
    scope(*this, doWhileLoop->body);
    write(" while(");
    visit(doWhileLoop->condition);
    write(");");
}

void RepresentationVisitor::VisitEnumDecl(EnumDeclaration *enumDecl) {
    write("enum ");
    write(enumDecl->name_view());
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

void RepresentationVisitor::VisitForLoopStmt(ForLoop *forLoop) {
    write("for(");
    visit(forLoop->initializer);
    visit(forLoop->conditionExpr);
    write(';');
    visit(forLoop->incrementerExpr);
    write(')');
    scope(*this, forLoop->body);
}

void RepresentationVisitor::VisitFunctionParam(FunctionParam *param) {
    if(param->name.empty()) {
        write('-');
    } else {
        write(param->name);
    }
    write(" : ");
    visit(param->type);
}

void RepresentationVisitor::VisitFunctionDecl(FunctionDeclaration *decl) {
    write_ws(decl->specifier());
    write("func ");
    write(decl->name_view());
    write('(');
    int i = 0;
    while (i < decl->params.size()) {
        const auto &param = decl->params[i];
        visit(param);
        if (i < decl->params.size() - 1) {
            write(", ");
        } else {
            if (decl->isVariadic()) {
                write("...");
            }
        }
        i++;
    }
    write(')');
    if(decl->returnType->kind() != BaseTypeKind::Void) {
        write(" : ");
        visit(decl->returnType);
    }
    write(' ');
    if (decl->body.has_value()) {
        scope(*this, decl->body.value());
    }
}

void RepresentationVisitor::VisitIfStmt(IfStatement *decl) {
    write("if(");
    nested_value = true;
    visit(decl->condition);
    nested_value = false;
    write(')');
    scope(*this, decl->ifBody);
    unsigned i = 0;
    while(i < decl->elseIfs.size()) {
        auto& elif = decl->elseIfs[i];
        write("else if(");
        visit(elif.first);
        write(')');
        scope(*this, elif.second);
        i++;
    }
    if(decl->elseBody.has_value()) {
        write(" else ");
        scope(*this, decl->elseBody.value());
    }
}

void write_variables(RepresentationVisitor& visitor, VariablesContainer* container) {
    int i = 0;
    for (const auto field: container->variables()) {
        visitor.new_line_and_indent();
        visitor.visit(field);
        i++;
    }
}

void write_members(RepresentationVisitor& visitor, MembersContainer* container) {
    write_variables(visitor, container);
    for (const auto field : container->functions()) {
        visitor.new_line_and_indent();
        visitor.visit(field);
    }
}

void RepresentationVisitor::VisitImplDecl(ImplDefinition *def) {
    write("impl ");
    visit(def->interface_type);
    space();
    if (def->struct_type) {
        write("for ");
        visit(def->struct_type);
    }
    write("{");
    indentation_level+=1;
    write_members(*this, def);
    indentation_level-=1;
    new_line_and_indent();
    write("}");
}

void RepresentationVisitor::VisitInterfaceDecl(InterfaceDefinition *def) {
    write("interface ");
    write(def->name_view());
    space();
    write("{");
    indentation_level+=1;
    write_members(*this, def);
    indentation_level-=1;
    new_line_and_indent();
    write("}");
}

void RepresentationVisitor::VisitScope(Scope *scope) {
    auto prev = top_level_node;
    top_level_node = false;
    for(const auto node : scope->nodes) {
        new_line_and_indent();
        visit(node);
    }
    top_level_node = prev;
}

bool RepresentationVisitor::write(AccessSpecifier specifier) {
    switch(specifier) {
        case AccessSpecifier::Private:
            write("private");
            return true;
        case AccessSpecifier::Public:
            write("public");
            return true;
        case AccessSpecifier::Protected:
            write("protected");
            return true;
        case AccessSpecifier::Internal:
            write("internal");
            return true;
        default:
            return false;
    }
}

template<typename T>
void comma_separated_accept(RepresentationVisitor& visitor, T& things) {
    bool has_before = false;
    for(const auto sub_type : things) {
        if(has_before) {
            visitor.write(", ");
        } else {
            has_before = true;
        }
        visitor.visit(sub_type);
    }
}

void RepresentationVisitor::VisitStructDecl(StructDefinition *def) {
    write_ws(def->specifier());
    write("struct ");
    write(def->name_view());
    if(!def->inherited.empty()) {
        write(" : ");
        unsigned i = 0;
        const auto size = def->inherited.size();
        while(i < size) {
            const auto& thing = def->inherited[i];
            write(thing.specifier);
            space();
            write(def->inherited[i].specifier);
            if(i < size - 1) write(", ");
            i++;
        }
    }
    space();
    write("{");
    indentation_level+=1;
    write_members(*this, def);
    indentation_level-=1;
    new_line_and_indent();
    write("}");
}

void RepresentationVisitor::VisitWhileLoopStmt(WhileLoop *whileLoop) {
    write("while(");
    visit(whileLoop->condition);
    write(") ");
    scope(*this, whileLoop->body);
}

void RepresentationVisitor::VisitUnsafeBlock(UnsafeBlock *block) {
    visit(&block->scope);
}

void RepresentationVisitor::VisitAccessChain(AccessChain *chain) {
    int i = 0;
    while (i < chain->values.size()) {
        visit(chain->values[i]);
        if (i != chain->values.size() - 1) {
            write('.');
        }
        i++;
    }
}

void RepresentationVisitor::VisitStructMember(StructMember *member) {
    write("var ");
    write(member->name);
    write(" : ");
    visit(member->type);
    if (member->defValue) {
        write(" = ");
        visit(member->defValue);
    }
    write(';');
}

void RepresentationVisitor::VisitTypealiasStmt(TypealiasStatement *stmt) {
    write_ws(stmt->specifier());
    write("typealias ");
    write(stmt->name_view());
    write(" = ");
    visit(stmt->actual_type);
}

void RepresentationVisitor::VisitSwitchStmt(SwitchStatement *statement) {
    write("switch(");
    visit(statement->expression);
    write(") {");
    unsigned i = 0;
    indentation_level += 1;
    while(i < statement->scopes.size()) {
        auto& scope = statement->scopes[i];
        new_line_and_indent();

        unsigned case_ind = 0;
        const auto size = statement->cases.size();
        while(case_ind < size) {
            auto& switch_case = statement->cases[case_ind];
            if(switch_case.second == i) {
                if(case_ind > 0) {
                    write(" | ");
                }
                visit(switch_case.first);
            }
            case_ind++;
        }
        write("=>");
        space();
        write('{');
        indentation_level += 1;
        visit(&scope);
        indentation_level -= 1;
        write('}');
        i++;
    }
    indentation_level -= 1;
    new_line_and_indent();
    write('}');
}

void RepresentationVisitor::VisitNamespaceDecl(Namespace *ns) {
    if(ns->nodes.empty()) return;
    write("namespace ");
    write(ns->name());
    space();
    write('{');
    indentation_level++;
    for(const auto node : ns->nodes) {
        new_line_and_indent();
        visit(node);
    }
    indentation_level--;
    new_line_and_indent();
    write('}');
}

void RepresentationVisitor::VisitExportStmt(ExportStmt* node) {
    write("export ");
    for (size_t i = 0; i < node->ids.size(); ++i) {
        write(node->ids[i]);
        if (i < node->ids.size() - 1) {
            write("::");
        }
    }
    if (!node->as_id.empty()) {
        write(" as ");
        write(node->as_id);
    }
    write(';');
}

void RepresentationVisitor::VisitTryStmt(TryCatch *statement) {
    write("[TryCatch_UNIMPLEMENTED]");
}

void RepresentationVisitor::VisitValueWrapper(ValueWrapperNode *node) {
    visit(node->value);
}

void RepresentationVisitor::VisitAccessChainNode(AccessChainNode* node) {
    visit(&node->chain);
}

void RepresentationVisitor::VisitIncDecNode(IncDecNode* node) {
    visit(&node->value);
}

void RepresentationVisitor::VisitPatternMatchExprNode(PatternMatchExprNode* node) {
    visit(&node->value);
}

void RepresentationVisitor::VisitPlacementNewNode(PlacementNewNode* node) {
    visit(&node->value);
}

void RepresentationVisitor::VisitIntNValue(IntNumValue* value) {
    const auto type = value->getType()->IntNKind();
    const auto is_char = type == IntNTypeKind::Char || type == IntNTypeKind::UChar;
    if(is_char && !interpret_representation) {
        if(!interpret_representation) write('\'');
        write_escape_encoded(output, (char) value->value);
        if(!interpret_representation) write('\'');
    } else {
        write_str(std::to_string(value->value));
    }
}

void RepresentationVisitor::VisitFloatValue(FloatValue *val) {
    write_str(std::to_string(val->value));
}

void RepresentationVisitor::VisitDoubleValue(DoubleValue *val) {
    write_str(std::to_string(val->value));
}

void RepresentationVisitor::VisitStringValue(StringValue *val) {
    if(interpret_representation) {
        write(val->value);
    } else {
        write('"');
        write_encoded(*this, val->value);
        write('"');
    }
}

void RepresentationVisitor::VisitBoolValue(BoolValue *boolVal) {
    if(boolVal->value) {
        write("true");
    } else {
        write("false");
    }
}

void RepresentationVisitor::VisitArrayValue(ArrayValue *arr) {
    write('[');
    unsigned i = 0;
    while(i < arr->values.size()) {
        visit(arr->values[i]);
        if(i != arr->values.size() - 1) {
            write(',');
        }
        i++;
    }
    write(']');
}

void RepresentationVisitor::VisitStructValue(StructValue *val) {
    visit(val->getRefType());
    write(" {");
    indentation_level += 1;
    unsigned i = 0;
    for (const auto &value: val->values) {
        new_line_and_indent();
        write(value.first);
        write(" : ");
        visit(value.second.value);
        if (i < val->values.size() - 1) write(",");
        i++;
    }
    indentation_level -= 1;
    new_line_and_indent();
    write('}');
}

void RepresentationVisitor::VisitVariableIdentifier(VariableIdentifier *identifier) {
    write(identifier->value);
}

void RepresentationVisitor::VisitExpression(Expression *expr) {
    write('(');
    nested_value = true;
    visit(expr->firstValue);
    space();
    write(to_string(expr->operation));
    space();
    visit(expr->secondValue);
    nested_value = false;
    write(')');
}

void RepresentationVisitor::VisitDynamicValue(DynamicValue* value) {
    write("dyn<");
    visit(value->getInterfaceType());
    write(">(");
    visit(value->value);
    write(')');
}

void RepresentationVisitor::VisitCastedValue(CastedValue *casted) {
    write('(');
    visit(casted->value);
    write(" as ");
    visit(casted->getType());
    write(')');
}

void RepresentationVisitor::VisitAddrOfValue(AddrOfValue *value) {
    if(value->is_mutable) {
        write("&mut ");
    } else {
        write('&');
    }
    visit(value->value);
}

void RepresentationVisitor::VisitDereferenceValue(DereferenceValue *casted) {
    write('*');
    visit(casted->getValue());
}

void RepresentationVisitor::VisitIfValue(IfValue* value) {
    VisitIfStmt(&value->stmt);
}

void RepresentationVisitor::VisitSwitchValue(SwitchValue* value) {
    VisitSwitchStmt(&value->stmt);
}

void RepresentationVisitor::VisitLoopValue(LoopValue* value) {
    VisitLoopBlock(&value->stmt);
}

void RepresentationVisitor::VisitExpressiveString(ExpressiveString* value) {
    for(auto val : value->values) {
        visit(val);
    }
}

void RepresentationVisitor::VisitFunctionCall(FunctionCall *call) {
    visit(call->parent_val);
    write('(');
    int i = 0;
    while (i < call->values.size()) {
        visit(call->values[i]);
        if (i != call->values.size() - 1) {
            write(',');
        }
        i++;
    }
    write(')');
}

void RepresentationVisitor::VisitIndexOperator(IndexOperator *op) {
    visit(op->parent_val);
    write('[');
    visit(op->idx);
    write(']');
}

void RepresentationVisitor::VisitNegativeValue(NegativeValue *negValue) {
    write('-');
    visit(negValue->getValue());
}

void RepresentationVisitor::VisitNotValue(NotValue *notValue) {
    write('!');
    visit(notValue->getValue());
}

void RepresentationVisitor::VisitNullValue(NullValue *nullValue) {
    write("null");
}

void RepresentationVisitor::VisitLambdaFunction(LambdaFunction *lamb) {
    write('[');
    unsigned i = 0;
    unsigned size = lamb->captureList.size();
    while(i < size) {
        visit(lamb->captureList[i]);
        if(i < size - 1){
            write(',');
        }
        i++;
    }
    write("](");
    i = 0;
    size = lamb->params.size();
    while(i < size) {
        visit(lamb->params[i]);
        if(i < size - 1){
            write(',');
        }
        i++;
    }
    write(") => ");
    scope(*this, lamb->scope);
}

void RepresentationVisitor::VisitAnyType(AnyType *func) {
    write("any");
}

void RepresentationVisitor::VisitArrayType(ArrayType *type) {
    if(type->has_array_size()) {
        write('[');
        write_str(std::to_string(type->get_array_size()));
        write(']');
    } else {
        write("[]");
    }
    visit(type->elem_type);
}

void RepresentationVisitor::VisitBoolType(BoolType *func) {
    write("bool");
}

void RepresentationVisitor::VisitIntNType(IntNType *type) {
    switch(type->IntNKind()) {
        case IntNTypeKind::I8:
            write("i8");
            return;
        case IntNTypeKind::I16:
            write("i16");
            return;
        case IntNTypeKind::I32:
            write("i32");
            return;
        case IntNTypeKind::I64:
            write("i64");
            return;
        case IntNTypeKind::Int128:
            write("int128");
            return;
        case IntNTypeKind::U8:
            write("u8");
            return;
        case IntNTypeKind::U16:
            write("u16");
            return;
        case IntNTypeKind::U32:
            write("u32");
            return;
        case IntNTypeKind::U64:
            write("u64");
            return;
        case IntNTypeKind::UInt128:
            write("uint128");
            return;
        case IntNTypeKind::Char:
            write("char");
            return;
        case IntNTypeKind::Short:
            write("short");
            return;
        case IntNTypeKind::Int:
            write("int");
            return;
        case IntNTypeKind::Long:
            write("long");
            return;
        case IntNTypeKind::LongLong:
            write("longlong");
            return;
        case IntNTypeKind::UChar:
            write("uchar");
            return;
        case IntNTypeKind::UShort:
            write("ushort");
            return;
        case IntNTypeKind::UInt:
            write("uint");
            return;
        case IntNTypeKind::ULong:
            write("ulong");
            return;
        case IntNTypeKind::ULongLong:
            write("ulonglong");
            return;
    }
}

void RepresentationVisitor::VisitDoubleType(DoubleType *func) {
    write("double");
}

void RepresentationVisitor::VisitFloatType(FloatType *func) {
    write("float");
}

void RepresentationVisitor::VisitFloat128Type(Float128Type *type) {
    write("float128");
}

void RepresentationVisitor::VisitLongDoubleType(LongDoubleType *type) {
    write("longdouble");
}

void RepresentationVisitor::VisitComplexType(ComplexType *type) {
    write("complex ");
    visit(type->elem_type);
}

void RepresentationVisitor::VisitFunctionType(FunctionType *type) {
    write('(');
    unsigned i = 0;
    auto size = type->params.size();
    while(i < size) {
        visit(type->params[i]);
        if(i < size - 1) {
            write(", ");
        }
        i++;
    }
    write(") => ");
    visit(type->returnType);
}

void RepresentationVisitor::VisitGenericType(GenericType *type) {
    visit(type->referenced);
    write('<');
    comma_separated_accept(*this, type->types);
    write('>');
}

void RepresentationVisitor::VisitGenericTypeParam(GenericTypeParameter *type_param) {
    write(type_param->identifier);
}

void RepresentationVisitor::VisitPointerType(PointerType *type) {
    write('*');
    if(type->is_mutable) {
        write("mut ");
    }
    visit(type->type);
}

void RepresentationVisitor::VisitReferenceType(ReferenceType *type) {
    write('&');
    if(type->is_mutable) {
        write("mut ");
    }
    visit(type->type);
}

void write_as_it_is(RepresentationVisitor& visitor, LinkedType* type) {
    if(type->is_named()) {
        const auto named = (NamedLinkedType*) type;
        visitor.write(named->debug_link_name());
    } else if(type->is_value()) {
        visitor.visit(((LinkedValueType*) type)->value);
    } else {
        visitor.write("UNKNOWN_TYPE");
    }
}

void RepresentationVisitor::VisitLinkedType(LinkedType *type) {
    if(type->linked) {
        const auto id = type->linked->get_located_id();
        if (id) {
            write(id->identifier);
        } else if (type->linked->kind() == ASTNodeKind::GenericTypeParam) {
            write(type->linked->as_generic_type_param_unsafe()->identifier);
        } else {
            write_as_it_is(*this, type);
        }
    } else {
        write_as_it_is(*this, type);
    }
}

void RepresentationVisitor::VisitStringType(StringType *func) {
    write("string");
}

void RepresentationVisitor::VisitStructType(StructType *def) {
    write("struct ");
    write(def->name);
    if(!def->inherited.empty()) {
        write(" : ");
        unsigned i = 0;
        const auto size = def->inherited.size();
        while(i < size) {
            const auto& thing = def->inherited[i];
            write(thing.specifier);
            space();
            write(def->inherited[i].specifier);
            if(i < size - 1) write(", ");
            i++;
        }
    }
    space();
    write("{");
    indentation_level+=1;
    write_variables(*this, def);
    indentation_level-=1;
    new_line_and_indent();
    write("}");
}

void RepresentationVisitor::VisitVoidType(VoidType *func) {
    write("void");
}

void RepresentationVisitor::VisitCapturingFunctionType(CapturingFunctionType* type) {
    write("%capture<");
    visit(type->func_type);
    write(',');
    visit(type->instance_type);
    write('>');
}
void RepresentationVisitor::VisitExpressiveStringType(ExpressiveStringType* type) {
    write("%expressive_string");
}

void RepresentationVisitor::VisitNullPtrType(NullPtrType* type) {
    // TODO use the nullptr type
    write("*mut void");
}

void RepresentationVisitor::VisitLoopBlock(LoopBlock *block) {
    write("loop");
    scope(*this, block->body);
}

void RepresentationVisitor::VisitValueNode(ValueNode *node) {
    visit(node->value);
}

void RepresentationVisitor::VisitIsValue(IsValue *isVal) {
    visit(isVal->value);
    write(" is ");
    visit(isVal->type);
}

void RepresentationVisitor::VisitInValue(InValue* value) {
    visit(value->value);
    write(" in ");
    bool has_value_before = false;
    for(const auto val : value->values) {
        if(has_value_before) {
            write(", ");
        } else {
            has_value_before = true;
        }
        visit(val);
    }
}

void RepresentationVisitor::VisitDeleteStmt(DestructStmt *delStmt) {
    write(delStmt->getFreeAfter() ? "delete" : "destruct");
    if(delStmt->is_array) {
        write('[');
        if(delStmt->array_value) {
            visit(delStmt->array_value);
        }
        write(']');
    }
    write(' ');
    visit(delStmt->identifier);
}

void RepresentationVisitor::VisitDeallocStmt(DeallocStmt* node) {
    write("dealloc ");
    visit(node->ptr);
}

void RepresentationVisitor::VisitVariantCase(VariantCase *chain) {
    write(chain->member->name);
    write('(');
    bool is_first = true;
    for(const auto var : chain->identifier_list) {
        if(!is_first) {
            write(", ");
        }
        visit(var);
        is_first = false;
    }
    write(')');
}

void RepresentationVisitor::VisitVariantDecl(VariantDefinition *variant_def) {
    write_ws(variant_def->specifier());
    write("variant ");
    write(variant_def->name_view());
    write('{');
    indentation_level+=1;
    for(const auto var : variant_def->variables()) {
        new_line_and_indent();
        visit(var);
    }
    indentation_level-=1;
    new_line_and_indent();
    write('}');
}

void RepresentationVisitor::VisitDynamicType(DynamicType *type) {
    write("dyn ");
    visit(type->referenced);
}

void RepresentationVisitor::VisitSizeOfValue(SizeOfValue *size_of) {
    write("sizeof(");
    visit(size_of->for_type);
    write(')');
}

void RepresentationVisitor::VisitAlignOfValue(AlignOfValue *align_of) {
    write("alignof(");
    visit(align_of->for_type);
    write(')');
}

void RepresentationVisitor::VisitUnsafeValue(UnsafeValue* value) {
    write("unsafe {");
    visit(value->getValue());
    write('}');
}

void RepresentationVisitor::VisitComptimeValue(ComptimeValue* value) {
    write("comptime {");
    visit(value->getValue());
    write('}');
}

void RepresentationVisitor::VisitRetStructParamValue(RetStructParamValue *paramVal) {
    write("[UnimplementedRetStructParamValue]");
}

void RepresentationVisitor::VisitUsingStmt(UsingStmt *usingStmt) {
    write("using [TODO Unimplemented]");
}

void RepresentationVisitor::VisitLiteralType(LiteralType *type) {
    write("%literal<");
    visit(type->underlying);
    write('>');
}

void RepresentationVisitor::VisitMaybeRuntimeType(MaybeRuntimeType* type) {
    write("%maybe_runtime<");
    visit(type->underlying);
    write('>');
}

void RepresentationVisitor::VisitRuntimeType(RuntimeType* type) {
    write("%runtime<");
    visit(type->underlying);
    write('>');
}

void RepresentationVisitor::VisitUnnamedStruct(UnnamedStruct *def) {
    write("[UnimplementedUnnamedStruct]");
}

void RepresentationVisitor::VisitUnnamedUnion(UnnamedUnion *def) {
    write("[UnimplementedUnnamedUnion]");
}

void RepresentationVisitor::VisitUnionDecl(UnionDef *def) {
    write("[UnimplementedUnionDef]");
}

void RepresentationVisitor::VisitThrowStmt(ThrowStatement *throwStmt) {
    write("throw ");
    write("TODO");
}

void RepresentationVisitor::VisitUnionType(UnionType *unionType) {
    write("[UnimplementedUnionType]");
}