// Copyright (c) Qinetik 2024.

#pragma once

#include "cst/base/CSTVisitor.h"
#include "BaseSymbolResolver.h"
#include "cst/base/CSTDiagnoser.h"
#include <vector>
#include <unordered_map>
#include <string>

class ImportUnit;

class CSTSymbolResolver : public BaseSymbolResolver<CSTToken>, public CSTVisitor, public CSTDiagnoser {
public:

    /**
     * declares a node with string, the string value is taken from the token
     * if there's an error it will be on this token too
     */
    void declare(LexToken* token, CSTToken *node);

    /**
     * same as declare above, but will treat CSTToken token as a LexToken
     */
    inline void declare(CSTToken* token, CSTToken* node) {
        declare((LexToken*) token, node);
    }

    /**
     * resolve an import unit
     */
    void resolve(ImportUnit* unit);

    /**
     * this just sets the linked field of ref token to the other given token
     * it's a virtual method so it can be overridden so references can be collected for a token
     * we're interested in to provide usage statistics
     */
    virtual void link(RefToken* ref, CSTToken* token);

    //-------------------------
    //------------ Visitors
    //-------------------------


    void visitCompoundCommon(CompoundCSTToken *compound) override;

    void visitBody(CompoundCSTToken *bodyCst) override;

    void visitVarInit(CompoundCSTToken *varInit) override;

    void visitTypealias(CompoundCSTToken *alias) override;

    void visitFunction(CompoundCSTToken *function) override;

    void visitEnumDecl(CompoundCSTToken *enumDecl) override;

    void visitInterface(CompoundCSTToken *interface) override;

    void visitStructDef(CompoundCSTToken *structDef) override;

    void visitImpl(CompoundCSTToken *impl) override;

    void visitAccessChain(AccessChainCST *accessChain) override;

    void visitVariableToken(LexToken *token) override;

    void visitFunctionCall(CompoundCSTToken *call) override;

    void visitIndexOp(CompoundCSTToken *op) override;

    void visitTypeToken(LexToken *token) override;

};