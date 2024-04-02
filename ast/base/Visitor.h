// Copyright (c) Qinetik 2024.

#pragma once

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

class Visitor {
public:

    virtual void visitCommon(ASTNode* node) {
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

};