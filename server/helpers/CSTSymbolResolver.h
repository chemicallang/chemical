// Copyright (c) Qinetik 2024.

#pragma once

#include "cst/base/CSTVisitor.h"
#include "preprocess/BaseSymbolResolver.h"
#include "cst/base/CSTDiagnoser.h"
#include <vector>
#include <unordered_map>
#include <string>

class LexImportUnit;

class CSTSymbolResolver : public BaseSymbolResolver<CSTToken>, public CSTVisitor, public CSTDiagnoser {
public:

    /**
     * declares a node with string, the string value is taken from the token
     * if there's an error it will be on this token too
     */
    void declare(CSTToken* token, CSTToken *node);

    /**
     * resolve an import unit
     */
    void resolve(LexImportUnit* unit);

    /**
     * this just sets the linked field of ref token to the other given token
     * it's a virtual method so it can be overridden so references can be collected for a token
     * we're interested in to provide usage statistics
     */
    virtual void link(CSTToken* ref, CSTToken* token);

    //-------------------------
    //------------ Visitors
    //-------------------------


    void visitCompoundCommon(CSTToken* compound) override;

    void visitBody(CSTToken* bodyCst) override;

    void visitVarInit(CSTToken* varInit) override;

    void visitTypealias(CSTToken* alias) override;

    void visitFunction(CSTToken* function) override;

    void visitEnumDecl(CSTToken* enumDecl) override;

    void visitInterface(CSTToken* interface) override;

    void visitStructDef(CSTToken* structDef) override;

    void visitImpl(CSTToken* impl) override;

    void visitAccessChain(CSTToken* accessChain) override;

    void visitVariableToken(CSTToken *token) override;

    void visitFunctionCall(CSTToken* call) override;

    void visitIndexOp(CSTToken* op) override;

    void visitTypeToken(CSTToken* token) override;

};