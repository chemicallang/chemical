// Copyright (c) Qinetik 2024.

#pragma once

class CSTToken;

class NumberToken;

class CSTVisitor {
public:

    virtual void visitCommon(CSTToken *token) {
        // do nothing
    }

    virtual void visitCompoundCommon(CSTToken *compound) {
        visitCommon(compound);
    }

    virtual void visitLexTokenCommon(CSTToken* token) {
        visitCommon(token);
    }

    virtual void visitVarInit(CSTToken *varInit) {
        visitCompoundCommon(varInit);
    }

    virtual void visitAssignment(CSTToken *assignment) {
        visitCompoundCommon(assignment);
    }

    virtual void visitLoopBlock(CSTToken *block) {
        visitCompoundCommon(block);
    }

    virtual void visitContinue(CSTToken *continueCst) {
        visitCompoundCommon(continueCst);
    }

    virtual void visitBreak(CSTToken *breakCST) {
        visitCompoundCommon(breakCST);
    }

    virtual void visitReturn(CSTToken *returnCst) {
        visitCompoundCommon(returnCst);
    }

    virtual void visitSwitch(CSTToken *switchCst) {
        visitCompoundCommon(switchCst);
    }

    virtual void visitImport(CSTToken *importCst) {
        visitCompoundCommon(importCst);
    }

    virtual void visitTypealias(CSTToken *alias) {
        visitCompoundCommon(alias);
    }

    virtual void visitThrow(CSTToken *throwStmt) {
        visitCompoundCommon(throwStmt);
    }

    virtual void visitUsing(CSTToken *usingStmt) {
        visitCompoundCommon(usingStmt);
    }

    virtual void visitDestruct(CSTToken *delStmt) {
        visitCompoundCommon(delStmt);
    }

    virtual void visitValueNode(CSTToken *ifCst) {
        visitCompoundCommon(ifCst);
    }

    virtual void visitIf(CSTToken *ifCst) {
        visitCompoundCommon(ifCst);
    }

    virtual void visitIncDec(CSTToken *incDec) {
        visitCompoundCommon(incDec);
    }

    virtual void visitBody(CSTToken *bodyCst) {
        visitCompoundCommon(bodyCst);
    }

    virtual void visitUnsafeBlock(CSTToken *unsafeBlock) {
        visitCompoundCommon(unsafeBlock);
    }

    virtual void visitDoWhile(CSTToken *doWhileCst) {
        visitCompoundCommon(doWhileCst);
    }

    virtual void visitInitBlock(CSTToken *initBlock) {
        visitCompoundCommon(initBlock);
    }

    virtual void visitEnumDecl(CSTToken *enumDecl) {
        visitCompoundCommon(enumDecl);
    }

    virtual void visitGenericParamsList(CSTToken *enumDecl) {
        visitCompoundCommon(enumDecl);
    }

    virtual void visitFunctionParam(CSTToken *param) {
        visitCompoundCommon(param);
    }

    virtual void visitMacro(CSTToken* cst) {
        visitCompoundCommon(cst);
    }

    virtual void visitFunction(CSTToken *function) {
        visitCompoundCommon(function);
    }

    virtual void visitStructDef(CSTToken *structDef) {
        visitCompoundCommon(structDef);
    }

    virtual void visitUnionDef(CSTToken *unionDef) {
        visitCompoundCommon(unionDef);
    }

    virtual void visitInterface(CSTToken *interface) {
        visitCompoundCommon(interface);
    }

    virtual void visitNamespace(CSTToken* ns) {
        visitCompoundCommon(ns);
    }

    virtual void visitTryCatch(CSTToken *tryCatch) {
        visitCompoundCommon(tryCatch);
    }

    virtual void visitWhile(CSTToken *whileCst) {
        visitCompoundCommon(whileCst);
    }

    virtual void visitForLoop(CSTToken *forLoop) {
        visitCompoundCommon(forLoop);
    }

    virtual void visitArrayType(CSTToken *arrayType) {
        visitCompoundCommon(arrayType);
    }

    virtual void visitFunctionType(CSTToken *functionType) {
        visitCompoundCommon(functionType);
    }

    virtual void visitLinkedValueType(CSTToken *ref_value) {
        visitCompoundCommon(ref_value);
    }

    virtual void visitGenericType(CSTToken *genericType) {
        visitCompoundCommon(genericType);
    }

    virtual void visitSpecializedType(CSTToken *specType) {
        visitCompoundCommon(specType);
    }

    virtual void visitPointerType(CSTToken *pointerType) {
        visitCompoundCommon(pointerType);
    }

    virtual void visitReferenceType(CSTToken *refType) {
        visitCompoundCommon(refType);
    }

    virtual void visitAccessChain(CSTToken *accessChain) {
        visitCompoundCommon(accessChain);
    }

    virtual void visitArrayValue(CSTToken *arrayValue) {
        visitCompoundCommon(arrayValue);
    }

    virtual void visitCast(CSTToken *castCst) {
        visitCompoundCommon(castCst);
    }

    virtual void visitIsValue(CSTToken *castCst) {
        visitCompoundCommon(castCst);
    }

    virtual void visitAddrOf(CSTToken *castCst) {
        visitCompoundCommon(castCst);
    }

    virtual void visitExpression(CSTToken *expressionCst) {
        visitCompoundCommon(expressionCst);
    }

    virtual void visitFunctionCall(CSTToken *call) {
        visitCompoundCommon(call);
    }

    virtual void visitImpl(CSTToken *impl) {
        visitCompoundCommon(impl);
    }

    virtual void visitVariant(CSTToken *variant) {
        visitCompoundCommon(variant);
    }

    virtual void visitVariantMember(CSTToken *variant_member) {
        visitCompoundCommon(variant_member);
    }

    virtual void visitGenericList(CSTToken* list) {
        visitCompoundCommon(list);
    }

    virtual void visitIndexOp(CSTToken *op) {
        visitCompoundCommon(op);
    }

    virtual void visitLambda(CSTToken *cst) {
        visitCompoundCommon(cst);
    }

    virtual void visitNegative(CSTToken *negativeCst) {
        visitCompoundCommon(negativeCst);
    }

    virtual void visitAnnotation(CSTToken *annotation) {
        visitCompoundCommon(annotation);
    }

    virtual void visitNot(CSTToken *notCst) {
        visitCompoundCommon(notCst);
    }

    virtual void visitDereference(CSTToken *notCst) {
        visitCompoundCommon(notCst);
    }

    virtual void visitStructValue(CSTToken *structValueCst) {
        visitCompoundCommon(structValueCst);
    }

    virtual void visitCharOperatorToken(CSTToken* token) {
        visitLexTokenCommon(token);
    };

    virtual void visitStringOperatorToken(CSTToken* token) {
        visitLexTokenCommon(token);
    };

    virtual void visitCharToken(CSTToken* token) {
        visitLexTokenCommon(token);
    };

    virtual void visitCommentToken(CSTToken* token) {
        visitLexTokenCommon(token);
    };

    virtual void visitTypeToken(CSTToken* token) {
        visitLexTokenCommon(token);
    };

    virtual void visitKeywordToken(CSTToken* token) {
        visitLexTokenCommon(token);
    };

    virtual void visitMacroToken(CSTToken* token) {
        visitLexTokenCommon(token);
    };

    virtual void visitAnnotationToken(CSTToken* token) {
        visitLexTokenCommon(token);
    };

    virtual void visitMultilineComment(CSTToken* token) {
        visitLexTokenCommon(token);
    };

    virtual void visitOperationToken(CSTToken* token) {
        visitLexTokenCommon(token);
    };

    virtual void visitStringToken(CSTToken* token) {
        visitLexTokenCommon(token);
    };

    virtual void visitNumberToken(NumberToken *token) {
        visitLexTokenCommon((CSTToken*) token);
    };

    virtual void visitBoolToken(CSTToken* token) {
        visitLexTokenCommon(token);
    }

    virtual void visitNullToken(CSTToken* token) {
        visitLexTokenCommon(token);
    }

    virtual void visitVariableToken(CSTToken* token) {
        visitLexTokenCommon(token);
    }

    virtual void visitIdentifierToken(CSTToken* token) {
        visitLexTokenCommon(token);
    }

    virtual void visitRawToken(CSTToken* token) {
        visitLexTokenCommon(token);
    }

    virtual void visitUserToken(CSTToken* token) {
        visitLexTokenCommon(token);
    };


};