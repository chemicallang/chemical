// Copyright (c) Qinetik 2024.

#include "base/CSTToken.h"
#include "cst/base/CSTVisitor.h"
#include <iostream>

void CSTToken::accept(CSTVisitor *visitor) {
    switch (tok_type) {
        case LexTokenType::CharOperator:
            visitor->visitCharOperatorToken((LexToken*) this);
            return;
        case LexTokenType::Operation:
            visitor->visitOperationToken((LexToken*) this);
            return;
        case LexTokenType::Char:
            visitor->visitCharToken((LexToken*) this);
            return;
        case LexTokenType::Comment:
            visitor->visitCommentToken((LexToken*) this);
            return;
        case LexTokenType::MultilineComment:
            visitor->visitMultilineComment((LexToken*) this);
            return;
        case LexTokenType::String:
            visitor->visitStringToken((LexToken*) this);
            return;
        case LexTokenType::Bool:
            visitor->visitBoolToken((LexToken*) this);
            return;
        case LexTokenType::Annotation:
            visitor->visitAnnotationToken((LexToken*) this);
            return;
        case LexTokenType::UserToken:
            visitor->visitUserToken((LexToken*) this);
            return;
        case LexTokenType::Keyword:
            visitor->visitKeywordToken((LexToken*) this);
            return;
        case LexTokenType::Number:
            visitor->visitNumberToken((NumberToken*) this);
            return;
        case LexTokenType::Type:
            visitor->visitTypeToken((LexToken*) this);
            return;
        case LexTokenType::Null:
            visitor->visitNullToken((LexToken*) this);
            return;
        case LexTokenType::StringOperator:
            visitor->visitStringOperatorToken((LexToken*) this);
            return;
        case LexTokenType::Variable:
            visitor->visitVariableToken((LexToken*) this);
            return;
        case LexTokenType::Identifier:
            visitor->visitIdentifierToken((LexToken*) this);
            return;
        case LexTokenType::RawToken:
            visitor->visitRawToken((LexToken*) this);
            return;
        case LexTokenType::CompAssignment:
            visitor->visitAssignment((CompoundCSTToken*) this);
            return;
        case LexTokenType::CompAccessChainNode:
            visitor->visitAccessChain((CompoundCSTToken*) this);
            return;
        case LexTokenType::CompAnnotation:
            visitor->visitAnnotation((CompoundCSTToken*) this);
            return;
        case LexTokenType::CompBreak:
            visitor->visitBreak((CompoundCSTToken*) this);
            return;
        case LexTokenType::CompContinue:
            visitor->visitContinue((CompoundCSTToken*) this);
            return;
        case LexTokenType::CompThrow:
            visitor->visitThrow((CompoundCSTToken*) this);
            return;
        case LexTokenType::CompUsing:
            visitor->visitUsing((CompoundCSTToken*) this);
            return;
        case LexTokenType::CompDelete:
            visitor->visitDelete((CompoundCSTToken*) this);
            return;
        case LexTokenType::CompIf:
            visitor->visitIf((CompoundCSTToken*) this);
            return;
        case LexTokenType::CompImport:
            visitor->visitImport((CompoundCSTToken*) this);
            return;
        case LexTokenType::CompIncDec:
            visitor->visitIncDec((CompoundCSTToken*) this);
            return;
        case LexTokenType::CompReturn:
            visitor->visitReturn((CompoundCSTToken*) this);
            return;
        case LexTokenType::CompSwitch:
            visitor->visitSwitch((CompoundCSTToken*) this);
            return;
        case LexTokenType::CompTypealias:
            visitor->visitTypealias((CompoundCSTToken*) this);
            return;
        case LexTokenType::CompVarInit:
            visitor->visitVarInit((CompoundCSTToken*) this);
            return;
        case LexTokenType::CompBody:
            visitor->visitBody((CompoundCSTToken*) this);
            return;
        case LexTokenType::CompDoWhile:
            visitor->visitDoWhile((CompoundCSTToken*) this);
            return;
        case LexTokenType::CompEnumDecl:
            visitor->visitEnumDecl((CompoundCSTToken*) this);
            return;
        case LexTokenType::CompForLoop:
            visitor->visitForLoop((CompoundCSTToken*) this);
            return;
        case LexTokenType::CompFunctionParam:
            visitor->visitFunctionParam((CompoundCSTToken*) this);
            return;
        case LexTokenType::CompFunction:
            visitor->visitFunction((CompoundCSTToken*) this);
            return;
        case LexTokenType::CompGenericParamsList:
            visitor->visitGenericParamsList((CompoundCSTToken*) this);
            return;
        case LexTokenType::CompStructDef:
            visitor->visitStructDef((CompoundCSTToken*) this);
            return;
        case LexTokenType::CompUnionDef:
            visitor->visitUnionDef((CompoundCSTToken*) this);
            return;
        case LexTokenType::CompInterface:
            visitor->visitInterface((CompoundCSTToken*) this);
            return;
        case LexTokenType::CompImpl:
            visitor->visitImpl((CompoundCSTToken*) this);
            return;
        case LexTokenType::CompNamespace:
            visitor->visitNamespace((CompoundCSTToken*) this);
            return;
        case LexTokenType::CompTryCatch:
            visitor->visitTryCatch((CompoundCSTToken*) this);
            return;
        case LexTokenType::CompMacro:
            visitor->visitMacro((CompoundCSTToken*) this);
            return;
        case LexTokenType::CompWhile:
            visitor->visitWhile((CompoundCSTToken*) this);
            return;
        case LexTokenType::CompArrayType:
            visitor->visitArrayType((CompoundCSTToken*) this);
            return;
        case LexTokenType::CompFunctionType:
            visitor->visitFunctionType((CompoundCSTToken*) this);
            return;
        case LexTokenType::CompGenericType:
            visitor->visitGenericType((CompoundCSTToken*) this);
            return;
        case LexTokenType::CompReferencedValueType:
            visitor->visitReferencedValueType((CompoundCSTToken*) this);
            return;
        case LexTokenType::CompPointerType:
            visitor->visitPointerType((CompoundCSTToken*) this);
            return;
        case LexTokenType::CompAccessChain:
            visitor->visitAccessChain((CompoundCSTToken*) this);
            return;
        case LexTokenType::CompArrayValue:
            visitor->visitArrayValue((CompoundCSTToken*) this);
            return;
        case LexTokenType::CompCastValue:
            visitor->visitCast((CompoundCSTToken*) this);
            return;
        case LexTokenType::CompAddrOf:
            visitor->visitAddrOf((CompoundCSTToken*) this);
            return;
        case LexTokenType::CompDeference:
            visitor->visitDereference((CompoundCSTToken*) this);
            return;
        case LexTokenType::CompExpression:
            visitor->visitExpression((CompoundCSTToken*) this);
            return;
        case LexTokenType::CompFunctionCall:
            visitor->visitFunctionCall((CompoundCSTToken*) this);
            return;
        case LexTokenType::CompIndexOp:
            visitor->visitIndexOp((CompoundCSTToken*) this);
            return;
        case LexTokenType::CompLambda:
            visitor->visitLambda((CompoundCSTToken*) this);
            return;
        case LexTokenType::CompNegative:
            visitor->visitNegative((CompoundCSTToken*) this);
            return;
        case LexTokenType::CompNot:
            visitor->visitNot((CompoundCSTToken*) this);
            return;
        case LexTokenType::CompStructValue:
            visitor->visitStructValue((CompoundCSTToken*) this);
            return;
        case LexTokenType::CompGenericList:
            visitor->visitGenericList((CompoundCSTToken*) this);
            return;
#ifdef DEBUG
//        default:
//            std::cerr << "UNKNOWN TOKEN TYPE " + std::to_string((uint8_t) tok_type) << std::endl;
#endif
    }
}