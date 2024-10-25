// Copyright (c) Qinetik 2024.

#pragma once

#include "SubVisitor.h"
#include "ast/utils/CommonVisitor.h"
#include <string>
#include <unordered_map>

class CValueDeclarationVisitor : public CommonVisitor, public SubVisitor {
public:

    using SubVisitor::SubVisitor;

    std::unordered_map<void*, std::string> aliases;

    unsigned lambda_num = 0;

    unsigned func_type_num = 0;

    unsigned alias_num = 0;

    unsigned enum_num = 0;

    void visit(LambdaFunction *func) final;

    void visit(FunctionDeclaration *functionDeclaration) final;

    void visit(ExtensionFunction *extensionFunc) final;

    void visit(EnumDeclaration *enumDeclaration) final;

    void visit(TypealiasStatement *statement) final;

    void visit(FunctionType *func) final;

    void visit(StructMember *member) final;

    void visit(IfStatement *ifStatement) final;

    void visit(ReturnStatement *stmt) final;

    void visit(FunctionCall *call) final;

    void visit(VariantCall *call) final;

    void visit(ArrayValue *arrayVal) final;

    void visit(StructValue *structValue) final;

    void reset() final {
        aliases.clear();
    }

};