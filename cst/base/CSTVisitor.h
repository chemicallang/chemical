// Copyright (c) Qinetik 2024.

#pragma once

class CSTToken;

class CompoundCSTToken;

class LexToken;

class VarInitCST;

class AssignmentCST;

class ContinueCST;

class BreakCST;

class ReturnCST;

class SwitchCST;

class ImportCST;

class TypealiasCST;

class IfCST;

class BodyCST;

class DoWhileCST;

class ForLoopCST;

class EnumDeclCST;

class IncDecCST;

class FunctionParamCST;

class FunctionCST;

class StructDefCST;

class InterfaceCST;

class TryCatchCST;

class WhileCST;

class ArrayTypeCST;

class FunctionTypeCST;

class GenericTypeCST;

class PointerTypeCST;

class AccessChainCST;

class ArrayValueCST;

class CastCST;

class AddrOfCST;

class DereferenceCST;

class ExpressionCST;

class FunctionCallCST;

class IndexOpCST;

class NegativeCST;

class NotCST;

class StructValueCST;

class AbstractStringToken;

class CharOperatorToken;

class CharToken;

class CharOperatorToken;

class CharToken;

class CommentToken;

class KeywordToken;

class MacroToken;

class AnnotationToken;

class TypeToken;

class MultilineCommentToken;

class OperationToken;

class StringToken;

class LambdaCST;

class ImplCST;

class NumberToken;

class BoolToken;

class VariableToken;

class LexUserToken;

class NullToken;

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

    virtual void visit(VarInitCST *varInit) {
        visitCompoundCommon((CompoundCSTToken *) varInit);
    }

    virtual void visit(AssignmentCST *assignment) {
        visitCompoundCommon((CompoundCSTToken *) assignment);
    }

    virtual void visit(ContinueCST *continueCst) {
        visitCompoundCommon((CompoundCSTToken *) continueCst);
    }

    virtual void visit(BreakCST *breakCST) {
        visitCompoundCommon((CompoundCSTToken *) breakCST);
    }

    virtual void visit(ReturnCST *returnCst) {
        visitCompoundCommon((CompoundCSTToken *) returnCst);
    }

    virtual void visit(SwitchCST *switchCst) {
        visitCompoundCommon((CompoundCSTToken *) switchCst);
    }

    virtual void visit(ImportCST *importCst) {
        visitCompoundCommon((CompoundCSTToken *) importCst);
    }

    virtual void visit(TypealiasCST *alias) {
        visitCompoundCommon((CompoundCSTToken *) alias);
    }

    virtual void visit(IfCST *ifCst) {
        visitCompoundCommon((CompoundCSTToken *) ifCst);
    }

    virtual void visit(IncDecCST *incDec) {
        visitCompoundCommon((CompoundCSTToken *) incDec);
    }

    virtual void visit(BodyCST *bodyCst) {
        visitCompoundCommon((CompoundCSTToken *) bodyCst);
    }

    virtual void visit(DoWhileCST *doWhileCst) {
        visitCompoundCommon((CompoundCSTToken *) doWhileCst);
    }

    virtual void visit(EnumDeclCST *enumDecl) {
        visitCompoundCommon((CompoundCSTToken *) enumDecl);
    }

    virtual void visit(FunctionParamCST *param) {
        visitCompoundCommon((CompoundCSTToken *) param);
    }

    virtual void visit(FunctionCST *function) {
        visitCompoundCommon((CompoundCSTToken *) function);
    }

    virtual void visit(StructDefCST *structDef) {
        visitCompoundCommon((CompoundCSTToken *) structDef);
    }

    virtual void visit(InterfaceCST *interface) {
        visitCompoundCommon((CompoundCSTToken *) interface);
    }

    virtual void visit(TryCatchCST *tryCatch) {
        visitCompoundCommon((CompoundCSTToken *) tryCatch);
    }

    virtual void visit(WhileCST *whileCst) {
        visitCompoundCommon((CompoundCSTToken *) whileCst);
    }

    virtual void visit(ForLoopCST *forLoop) {
        visitCompoundCommon((CompoundCSTToken *) forLoop);
    }

    virtual void visit(ArrayTypeCST *arrayType) {
        visitCompoundCommon((CompoundCSTToken *) arrayType);
    }

    virtual void visit(FunctionTypeCST *functionType) {
        visitCompoundCommon((CompoundCSTToken *) functionType);
    }

    virtual void visit(GenericTypeCST *genericType) {
        visitCompoundCommon((CompoundCSTToken *) genericType);
    }

    virtual void visit(PointerTypeCST *pointerType) {
        visitCompoundCommon((CompoundCSTToken *) pointerType);
    }

    virtual void visit(AccessChainCST *accessChain) {
        visitCompoundCommon((CompoundCSTToken *) accessChain);
    }

    virtual void visit(ArrayValueCST *arrayValue) {
        visitCompoundCommon((CompoundCSTToken *) arrayValue);
    }

    virtual void visit(CastCST *castCst) {
        visitCompoundCommon((CompoundCSTToken *) castCst);
    }

    virtual void visit(AddrOfCST *castCst) {
        visitCompoundCommon((CompoundCSTToken *) castCst);
    }

    virtual void visit(ExpressionCST *expressionCst) {
        visitCompoundCommon((CompoundCSTToken *) expressionCst);
    }

    virtual void visit(FunctionCallCST *call) {
        visitCompoundCommon((CompoundCSTToken *) call);
    }

    virtual void visit(ImplCST *impl) {
        visitCompoundCommon((CompoundCSTToken *) impl);
    }

    virtual void visit(IndexOpCST *op) {
        visitCompoundCommon((CompoundCSTToken *) op);
    }

    virtual void visit(LambdaCST *cst) {
        visitCompoundCommon((CompoundCSTToken *) cst);
    }

    virtual void visit(NegativeCST *negativeCst) {
        visitCompoundCommon((CompoundCSTToken *) negativeCst);
    }

    virtual void visit(NotCST *notCst) {
        visitCompoundCommon((CompoundCSTToken *) notCst);
    }

    virtual void visit(DereferenceCST *notCst) {
        visitCompoundCommon((CompoundCSTToken *) notCst);
    }

    virtual void visit(StructValueCST *structValueCst) {
        visitCompoundCommon((CompoundCSTToken *) structValueCst);
    }

    virtual void visitStringCommon(AbstractStringToken *token) {
        visitLexTokenCommon((LexToken *) token);
    }

    virtual void visit(CharOperatorToken *token) {
        visitLexTokenCommon((LexToken *) token);
    };

    virtual void visit(CharToken *token) {
        visitLexTokenCommon((LexToken *) token);
    };

    virtual void visit(CommentToken *token) {
        visitLexTokenCommon((LexToken *) token);
    };

    virtual void visit(TypeToken *token) {
        visitStringCommon((AbstractStringToken *) token);
    };

    virtual void visit(KeywordToken *token) {
        visitLexTokenCommon((LexToken *) token);
    };

    virtual void visit(MacroToken *token) {
        visitLexTokenCommon((LexToken *) token);
    };

    virtual void visit(AnnotationToken *token) {
        visitLexTokenCommon((LexToken *) token);
    };

    virtual void visit(MultilineCommentToken *token) {
        visitLexTokenCommon((LexToken *) token);
    };

    virtual void visit(OperationToken *token) {
        visitLexTokenCommon((LexToken *) token);
    };

    virtual void visit(StringToken *token) {
        visitLexTokenCommon((LexToken *) token);
    };

    virtual void visit(NumberToken *token) {
        visitStringCommon((AbstractStringToken *) token);
    };

    virtual void visit(BoolToken* token) {
        visitLexTokenCommon((LexToken *) token);
    }

    virtual void visit(NullToken* token) {
        visitLexTokenCommon((LexToken *) token);
    }

    virtual void visit(VariableToken *token) {
        visitStringCommon((AbstractStringToken *) token);
    }

    virtual void visit(LexUserToken *token) {
        visitLexTokenCommon((LexToken *) token);
    };


};