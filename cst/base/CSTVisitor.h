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

class StringOperatorToken;

class IdentifierToken;

class VariableToken;

class LexUserToken;

class RawToken;

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

    virtual void visitFunctionParam(CompoundCSTToken *param) {
        visitCompoundCommon((CompoundCSTToken *) param);
    }

    virtual void visitFunction(CompoundCSTToken *function) {
        visitCompoundCommon((CompoundCSTToken *) function);
    }

    virtual void visitStructDef(CompoundCSTToken *structDef) {
        visitCompoundCommon((CompoundCSTToken *) structDef);
    }

    virtual void visitInterface(CompoundCSTToken *interface) {
        visitCompoundCommon((CompoundCSTToken *) interface);
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

    virtual void visitGenericType(CompoundCSTToken *genericType) {
        visitCompoundCommon((CompoundCSTToken *) genericType);
    }

    virtual void visitPointerType(CompoundCSTToken *pointerType) {
        visitCompoundCommon((CompoundCSTToken *) pointerType);
    }

    virtual void visitAccessChain(AccessChainCST *accessChain) {
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

    virtual void visitIndexOp(CompoundCSTToken *op) {
        visitCompoundCommon((CompoundCSTToken *) op);
    }

    virtual void visitLambda(CompoundCSTToken *cst) {
        visitCompoundCommon((CompoundCSTToken *) cst);
    }

    virtual void visitNegative(CompoundCSTToken *negativeCst) {
        visitCompoundCommon((CompoundCSTToken *) negativeCst);
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

    virtual void visit(CharOperatorToken *token) {
        visitLexTokenCommon((LexToken *) token);
    };

    virtual void visit(StringOperatorToken *token) {
        visitLexTokenCommon((LexToken *) token);
    };

    virtual void visit(CharToken *token) {
        visitLexTokenCommon((LexToken *) token);
    };

    virtual void visit(CommentToken *token) {
        visitLexTokenCommon((LexToken *) token);
    };

    virtual void visit(TypeToken *token) {
        visitLexTokenCommon((LexToken *) token);
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
        visitLexTokenCommon((LexToken *) token);
    };

    virtual void visit(BoolToken* token) {
        visitLexTokenCommon((LexToken *) token);
    }

    virtual void visit(NullToken* token) {
        visitLexTokenCommon((LexToken *) token);
    }

    virtual void visit(VariableToken *token) {
        visitLexTokenCommon((LexToken *) token);
    }

    virtual void visit(IdentifierToken *token) {
        visitLexTokenCommon((LexToken *) token);
    }

    virtual void visit(RawToken* token) {
        visitLexTokenCommon((LexToken*) token);
    }

    virtual void visit(LexUserToken *token) {
        visitLexTokenCommon((LexToken *) token);
    };


};