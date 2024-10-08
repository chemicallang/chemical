// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/base/Visitor.h"

class CommonVisitor : public Visitor {
public:

    bool is_top_level_node = true;

    void visit(Scope *scope) override;

    void visit(FunctionCall *call) override;

    void visit(VariantCall *call) override;

    void visit(Expression *expr) override;

    void visit(CastedValue *casted) override;

    void visit(ValueNode *node) override;

    void visit(UnsafeBlock *block) override;

    void visit(VarInitStatement *init) override;

    void visit(ReturnStatement *stmt) override;

    void visit(AssignStatement *assign) override;

    void visit(FunctionDeclaration *decl) override;

    void visit(FunctionParam *functionParam) override;

    void visit(StructDefinition *structDefinition) override;

    void visit(Namespace *ns) override;

    void visit(StructMember *member) override;

    void visit(LambdaFunction *func) override;

    void visit(IfStatement *stmt) override;

    void visit(DestructStmt *delStmt) override;

    void visit(WhileLoop *whileLoop) override;

    void visit(DoWhileLoop *doWhileLoop) override;

    void visit(ForLoop *forLoop) override;

    void visit(SwitchStatement *statement) override;

    void visit(AccessChain *chain) override;

    void visit(StructValue *structValue) override;

    void visit(StructMemberInitializer *init) override;

    void visit(ArrayValue *arrayVal) override;

    void visit(ArrayType *func) override;

};