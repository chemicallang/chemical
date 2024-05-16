// Copyright (c) Qinetik 2024.

#pragma once

#include "cst/base/CSTVisitor.h"
#include "BaseSymbolResolver.h"
#include "cst/base/CSTDiagnoser.h"
#include <vector>
#include <unordered_map>
#include <string>

class CSTSymbolResolver : public BaseSymbolResolver<CSTToken>, public CSTVisitor, public CSTDiagnoser {
public:

    /**
     * when traversing nodes, a node can declare itself on the map
     * this is vector of scopes, the last scope is current scope
     */
    std::vector<std::unordered_map<std::string, CSTToken*>> current = {{}};

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

    //-------------------------
    //------------ Visitors
    //-------------------------


    void visitCompoundCommon(CompoundCSTToken *compound) override;

    void visitBody(CompoundCSTToken *bodyCst) override;

    void visitVarInit(CompoundCSTToken *varInit) override;

    void visitEnumDecl(CompoundCSTToken *enumDecl) override;

    void visitInterface(CompoundCSTToken *interface) override;

    void visitStructDef(CompoundCSTToken *structDef) override;

    void visitAccessChain(AccessChainCST *accessChain) override;

    void visitVariableToken(LexToken *token) override;

};