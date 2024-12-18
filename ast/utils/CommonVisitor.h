// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/base/Visitor.h"

class CommonVisitor : public Visitor {
public:

    bool is_top_level_node = true;

    void visit(Scope *scope);

    void visit(FunctionCall *call);

    void visit(VariantCall *call);

    void visit(Expression *expr) final;

    void visit(CastedValue *casted) final;

    void visit(ValueNode *node) final;

    void visit(UnsafeBlock *block) final;

    void visit(VarInitStatement *init);

    void visit(ReturnStatement *stmt);

    void visit(AssignStatement *assign) final;

    void visit(FunctionDeclaration *decl);

    void visit(FunctionParam *functionParam) final;

    void visit(StructDefinition *structDefinition) final;

    void visit(Namespace *ns) final;

    void visit(StructMember *member);

    void visit(LambdaFunction *func);

    void visit(IfStatement *stmt);

    void visit(DestructStmt *delStmt) final;

    void visit(WhileLoop *whileLoop) final;

    void visit(DoWhileLoop *doWhileLoop) final;

    void visit(ForLoop *forLoop) final;

    void visit(SwitchStatement *statement) final;

    void visit(AccessChain *chain);

    void visit(StructValue *structValue);

    void visit(StructMemberInitializer *init) final;

    void visit(ArrayValue *arrayVal);

    void visit(ArrayType *func) final;

    void visit(NewValue *value) override;

    void visit(PlacementNewValue *value) override;

};