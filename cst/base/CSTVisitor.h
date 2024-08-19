// Copyright (c) Qinetik 2024.

#pragma once

class CSTToken;

class CompoundCSTToken;

class LexToken;

class NumberToken;

class CSTVisitor {
public:

    virtual void visitCommon(CSTToken *token) {
        // do nothing
    }

    virtual void visitCompoundCommon(CompoundCSTToken *compound) {
        visitCommon((CSTToken *) compound);
    }

    virtual void visitLexTokenCommon(LexToken *token) {
        visitCommon((CSTToken *) token);
    }

    virtual void visitVarInit(CompoundCSTToken *varInit) {
        visitCompoundCommon((CompoundCSTToken *) varInit);
    }

    virtual void visitAssignment(CompoundCSTToken *assignment) {
        visitCompoundCommon((CompoundCSTToken *) assignment);
    }

    virtual void visitContinue(CompoundCSTToken *continueCst) {
        visitCompoundCommon((CompoundCSTToken *) continueCst);
    }

    virtual void visitBreak(CompoundCSTToken *breakCST) {
        visitCompoundCommon((CompoundCSTToken *) breakCST);
    }

    virtual void visitReturn(CompoundCSTToken *returnCst) {
        visitCompoundCommon((CompoundCSTToken *) returnCst);
    }

    virtual void visitSwitch(CompoundCSTToken *switchCst) {
        visitCompoundCommon((CompoundCSTToken *) switchCst);
    }

    virtual void visitImport(CompoundCSTToken *importCst) {
        visitCompoundCommon((CompoundCSTToken *) importCst);
    }

    virtual void visitTypealias(CompoundCSTToken *alias) {
        visitCompoundCommon((CompoundCSTToken *) alias);
    }

    virtual void visitThrow(CompoundCSTToken *throwStmt) {
        visitCompoundCommon((CompoundCSTToken *) throwStmt);
    }

    virtual void visitUsing(CompoundCSTToken *usingStmt) {
        visitCompoundCommon((CompoundCSTToken *) usingStmt);
    }

    virtual void visitDelete(CompoundCSTToken *delStmt) {
        visitCompoundCommon((CompoundCSTToken *) delStmt);
    }

    virtual void visitIf(CompoundCSTToken *ifCst) {
        visitCompoundCommon((CompoundCSTToken *) ifCst);
    }

    virtual void visitIncDec(CompoundCSTToken *incDec) {
        visitCompoundCommon((CompoundCSTToken *) incDec);
    }

    virtual void visitBody(CompoundCSTToken *bodyCst) {
        visitCompoundCommon((CompoundCSTToken *) bodyCst);
    }

    virtual void visitDoWhile(CompoundCSTToken *doWhileCst) {
        visitCompoundCommon((CompoundCSTToken *) doWhileCst);
    }

    virtual void visitEnumDecl(CompoundCSTToken *enumDecl) {
        visitCompoundCommon((CompoundCSTToken *) enumDecl);
    }

    virtual void visitGenericParamsList(CompoundCSTToken *enumDecl) {
        visitCompoundCommon((CompoundCSTToken *) enumDecl);
    }

    virtual void visitFunctionParam(CompoundCSTToken *param) {
        visitCompoundCommon((CompoundCSTToken *) param);
    }

    virtual void visitMacro(CompoundCSTToken* cst) {
        visitCompoundCommon((CompoundCSTToken *) cst);
    }

    virtual void visitFunction(CompoundCSTToken *function) {
        visitCompoundCommon((CompoundCSTToken *) function);
    }

    virtual void visitStructDef(CompoundCSTToken *structDef) {
        visitCompoundCommon((CompoundCSTToken *) structDef);
    }

    virtual void visitUnionDef(CompoundCSTToken *unionDef) {
        visitCompoundCommon((CompoundCSTToken *) unionDef);
    }

    virtual void visitInterface(CompoundCSTToken *interface) {
        visitCompoundCommon((CompoundCSTToken *) interface);
    }

    virtual void visitNamespace(CompoundCSTToken* ns) {
        visitCompoundCommon((CompoundCSTToken *) ns);
    }

    virtual void visitTryCatch(CompoundCSTToken *tryCatch) {
        visitCompoundCommon((CompoundCSTToken *) tryCatch);
    }

    virtual void visitWhile(CompoundCSTToken *whileCst) {
        visitCompoundCommon((CompoundCSTToken *) whileCst);
    }

    virtual void visitForLoop(CompoundCSTToken *forLoop) {
        visitCompoundCommon((CompoundCSTToken *) forLoop);
    }

    virtual void visitArrayType(CompoundCSTToken *arrayType) {
        visitCompoundCommon((CompoundCSTToken *) arrayType);
    }

    virtual void visitFunctionType(CompoundCSTToken *functionType) {
        visitCompoundCommon((CompoundCSTToken *) functionType);
    }

    virtual void visitReferencedValueType(CompoundCSTToken *ref_value) {
        visitCompoundCommon((CompoundCSTToken *) ref_value);
    }

    virtual void visitGenericType(CompoundCSTToken *genericType) {
        visitCompoundCommon((CompoundCSTToken *) genericType);
    }

    virtual void visitSpecializedType(CompoundCSTToken *specType) {
        visitCompoundCommon((CompoundCSTToken *) specType);
    }

    virtual void visitPointerType(CompoundCSTToken *pointerType) {
        visitCompoundCommon((CompoundCSTToken *) pointerType);
    }

    virtual void visitAccessChain(CompoundCSTToken *accessChain) {
        visitCompoundCommon((CompoundCSTToken *) accessChain);
    }

    virtual void visitArrayValue(CompoundCSTToken *arrayValue) {
        visitCompoundCommon((CompoundCSTToken *) arrayValue);
    }

    virtual void visitCast(CompoundCSTToken *castCst) {
        visitCompoundCommon((CompoundCSTToken *) castCst);
    }

    virtual void visitAddrOf(CompoundCSTToken *castCst) {
        visitCompoundCommon((CompoundCSTToken *) castCst);
    }

    virtual void visitExpression(CompoundCSTToken *expressionCst) {
        visitCompoundCommon((CompoundCSTToken *) expressionCst);
    }

    virtual void visitFunctionCall(CompoundCSTToken *call) {
        visitCompoundCommon((CompoundCSTToken *) call);
    }

    virtual void visitImpl(CompoundCSTToken *impl) {
        visitCompoundCommon((CompoundCSTToken *) impl);
    }

    virtual void visitVariant(CompoundCSTToken *variant) {
        visitCompoundCommon((CompoundCSTToken *) variant);
    }

    virtual void visitVariantMember(CompoundCSTToken *variant_member) {
        visitCompoundCommon((CompoundCSTToken *) variant_member);
    }

    virtual void visitGenericList(CompoundCSTToken* list) {
        visitCompoundCommon((CompoundCSTToken *) list);
    }

    virtual void visitIndexOp(CompoundCSTToken *op) {
        visitCompoundCommon((CompoundCSTToken *) op);
    }

    virtual void visitLambda(CompoundCSTToken *cst) {
        visitCompoundCommon((CompoundCSTToken *) cst);
    }

    virtual void visitNegative(CompoundCSTToken *negativeCst) {
        visitCompoundCommon((CompoundCSTToken *) negativeCst);
    }

    virtual void visitAnnotation(CompoundCSTToken *annotation) {
        visitCompoundCommon((CompoundCSTToken *) annotation);
    }

    virtual void visitNot(CompoundCSTToken *notCst) {
        visitCompoundCommon((CompoundCSTToken *) notCst);
    }

    virtual void visitDereference(CompoundCSTToken *notCst) {
        visitCompoundCommon((CompoundCSTToken *) notCst);
    }

    virtual void visitStructValue(CompoundCSTToken *structValueCst) {
        visitCompoundCommon((CompoundCSTToken *) structValueCst);
    }

    virtual void visitCharOperatorToken(LexToken *token) {
        visitLexTokenCommon(token);
    };

    virtual void visitStringOperatorToken(LexToken *token) {
        visitLexTokenCommon(token);
    };

    virtual void visitCharToken(LexToken *token) {
        visitLexTokenCommon(token);
    };

    virtual void visitCommentToken(LexToken *token) {
        visitLexTokenCommon(token);
    };

    virtual void visitTypeToken(LexToken *token) {
        visitLexTokenCommon(token);
    };

    virtual void visitKeywordToken(LexToken *token) {
        visitLexTokenCommon(token);
    };

    virtual void visitMacroToken(LexToken *token) {
        visitLexTokenCommon(token);
    };

    virtual void visitAnnotationToken(LexToken *token) {
        visitLexTokenCommon(token);
    };

    virtual void visitMultilineComment(LexToken *token) {
        visitLexTokenCommon(token);
    };

    virtual void visitOperationToken(LexToken *token) {
        visitLexTokenCommon(token);
    };

    virtual void visitStringToken(LexToken *token) {
        visitLexTokenCommon(token);
    };

    virtual void visitNumberToken(NumberToken *token) {
        visitLexTokenCommon((LexToken*) token);
    };

    virtual void visitBoolToken(LexToken* token) {
        visitLexTokenCommon(token);
    }

    virtual void visitNullToken(LexToken* token) {
        visitLexTokenCommon(token);
    }

    virtual void visitVariableToken(LexToken *token) {
        visitLexTokenCommon(token);
    }

    virtual void visitIdentifierToken(LexToken *token) {
        visitLexTokenCommon(token);
    }

    virtual void visitRawToken(LexToken* token) {
        visitLexTokenCommon(token);
    }

    virtual void visitUserToken(LexToken *token) {
        visitLexTokenCommon(token);
    };


};