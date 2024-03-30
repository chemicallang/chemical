// Copyright (c) Qinetik 2024.

#pragma once

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

class Visitor {
public:

    virtual void visit(VarInitStatement* init) {

    }

    virtual void visit(AssignStatement* assign) {

    }

    virtual void visit(BreakStatement* breakStatement) {

    }

    virtual void visit(Comment* comment) {

    }

    virtual void visit(ContinueStatement* continueStatement) {

    }

    virtual void visit(ImportStatement* importStatement) {

    }

    virtual void visit(ReturnStatement* returnStatement) {

    }

    virtual void visit(DoWhileLoop* doWhileLoop) {

    }

    virtual void visit(EnumDeclaration* enumDeclaration) {

    }

    virtual void visit(ForLoop* forLoop) {

    }

    virtual void visit(FunctionParam* functionParam) {

    }

    virtual void visit(FunctionDeclaration* functionDeclaration) {

    }

    virtual void visit(IfStatement* ifStatement) {

    }

    virtual void visit(ImplDefinition* implDefinition) {

    }

    virtual void visit(InterfaceDefinition* interfaceDefinition) {

    }

    virtual void visit(Scope* scope) {

    }

    virtual void visit(StructDefinition* structDefinition) {

    }

    virtual void visit(WhileLoop* whileLoop) {

    }

    virtual void visit(AccessChain* chain) {

    }

    virtual void visit(MacroValueStatement* statement) {

    }

    virtual void visit(StructMember* member) {

    }

    virtual void visit(TypealiasStatement* statement) {

    }

};