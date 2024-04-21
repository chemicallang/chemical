// Copyright (c) Qinetik 2024.

#pragma once

// Nodes

class ASTNode;

class VarInitStatement;

class AssignStatement;

class BreakStatement;

class Comment;

class ContinueStatement;

class ImportStatement;

class ReturnStatement;

class DoWhileLoop;

class EnumDeclaration;

class ForLoop;

class FunctionParam;

class FunctionDeclaration;

class IfStatement;

class ImplDefinition;

class InterfaceDefinition;

class Scope;

class StructDefinition;

class WhileLoop;

class AccessChain;

class MacroValueStatement;

class StructMember;

class TypealiasStatement;

class SwitchStatement;

class TryCatch;

// Values Begin

class Value;

class IntValue;

class FloatValue;

class DoubleValue;

class CharValue;

class StringValue;

class BoolValue;

class ArrayValue;

class StructValue;

class VariableIdentifier;

class Expression;

class AccessChain;

class CastedValue;

class AddrOfValue;

class FunctionCall;

class IndexOperator;

class NegativeValue;

class NotValue;

class TernaryValue;

class LambdaFunction;

using func_params = std::vector<std::unique_ptr<FunctionParam>>;

// Visitor Class

class Visitor {
public:

    virtual void visitCommon(ASTNode* node) {
        // do nothing
    }

    virtual void visitCommonValue(Value* value) {
        // do nothing
    }

    virtual void visit(VarInitStatement* init) {
        visitCommon((ASTNode*) init);
    }

    virtual void visit(AssignStatement* assign) {
        visitCommon((ASTNode*) assign);
    }

    virtual void visit(BreakStatement* breakStatement) {
        visitCommon((ASTNode*) breakStatement);
    }

    virtual void visit(Comment* comment) {
        visitCommon((ASTNode*) comment);
    }

    virtual void visit(ContinueStatement* continueStatement) {
        visitCommon((ASTNode*) continueStatement);
    }

    virtual void visit(ImportStatement* importStatement) {
        visitCommon((ASTNode*) importStatement);
    }

    virtual void visit(ReturnStatement* returnStatement) {
        visitCommon((ASTNode*) returnStatement);
    }

    virtual void visit(DoWhileLoop* doWhileLoop) {
        visitCommon((ASTNode*) doWhileLoop);
    }

    virtual void visit(EnumDeclaration* enumDeclaration) {
        visitCommon((ASTNode*) enumDeclaration);
    }

    virtual void visit(ForLoop* forLoop) {
        visitCommon((ASTNode*) forLoop);
    }

    virtual void visit(FunctionParam* functionParam) {
        visitCommon((ASTNode*) functionParam);
    }

    virtual void visit(FunctionDeclaration* functionDeclaration) {
        visitCommon((ASTNode*) functionDeclaration);
    }

    virtual void visit(IfStatement* ifStatement) {
        visitCommon((ASTNode*) ifStatement);
    }

    virtual void visit(ImplDefinition* implDefinition) {
        visitCommon((ASTNode*) implDefinition);
    }

    virtual void visit(InterfaceDefinition* interfaceDefinition) {
        visitCommon((ASTNode*) interfaceDefinition);
    }

    virtual void visit(Scope* scope) {
        visitCommon((ASTNode*) scope);
    }

    virtual void visit(StructDefinition* structDefinition) {
        visitCommon((ASTNode*) structDefinition);
    }

    virtual void visit(WhileLoop* whileLoop) {
        visitCommon((ASTNode*) whileLoop);
    }

    virtual void visit(AccessChain* chain) {
        visitCommon((ASTNode*) chain);
    }

    virtual void visit(MacroValueStatement* statement) {
        visitCommon((ASTNode*) statement);
    }

    virtual void visit(StructMember* member) {
        visitCommon((ASTNode*) member);
    }

    virtual void visit(TypealiasStatement* statement) {
        visitCommon((ASTNode*) statement);
    }

    virtual void visit(SwitchStatement* statement) {
        visitCommon((ASTNode*) statement);
    }

    virtual void visit(TryCatch* statement) {
        visitCommon((ASTNode*) statement);
    }

    // Value Visit Methods

    virtual void visit(IntValue* intVal) {
        visitCommonValue((Value*) intVal);
    }

    virtual void visit(FloatValue* floatVal) {
        visitCommonValue((Value*) floatVal);
    }

    virtual void visit(DoubleValue* doubleVal) {
        visitCommonValue((Value*) doubleVal);
    }

    virtual void visit(CharValue* charVal) {
        visitCommonValue((Value*) charVal);
    }

    virtual void visit(StringValue* stringVal) {
        visitCommonValue((Value*) stringVal);
    }

    virtual void visit(BoolValue* boolVal) {
        visitCommonValue((Value*) boolVal);
    }

    virtual void visit(ArrayValue* arrayVal) {
        visitCommonValue((Value*) arrayVal);
    }

    virtual void visit(StructValue* structValue) {
        visitCommonValue((Value*) structValue);
    }

    virtual void visit(VariableIdentifier* identifier) {
        visitCommonValue((Value*) identifier);
    }

    virtual void visit(Expression* expr) {
        visitCommonValue((Value*) expr);
    }

    virtual void visit(CastedValue* casted) {
        visitCommonValue((Value*) casted);
    }

    virtual void visit(AddrOfValue* casted) {
        visitCommonValue((Value*) casted);
    }

    virtual void visit(FunctionCall* call) {
        visitCommonValue((Value*) call);
    }

    virtual void visit(IndexOperator* op) {
        visitCommonValue((Value*) op);
    }

    virtual void visit(NegativeValue* negValue) {
        visitCommonValue((Value*) negValue);
    }

    virtual void visit(NotValue* notValue) {
        visitCommonValue((Value*) notValue);
    }

    virtual void visit(TernaryValue* ternary) {
        visitCommonValue((Value*) ternary);
    }

    virtual void visit(LambdaFunction* func) {
        visitCommonValue((Value*) func);
    }

};