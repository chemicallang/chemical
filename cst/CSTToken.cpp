// Copyright (c) Qinetik 2024.

#include "base/CSTToken.h"
#include "cst/base/CSTVisitor.h"
#include <iostream>

void CSTToken::accept(CSTVisitor *visitor) {
    switch (tok_type) {
        case LexTokenType::CharOperator:
            visitor->visitCharOperatorToken((CSTToken*) this);
            return;
        case LexTokenType::Operation:
            visitor->visitOperationToken((CSTToken*) this);
            return;
        case LexTokenType::Char:
            visitor->visitCharToken((CSTToken*) this);
            return;
        case LexTokenType::Comment:
            visitor->visitCommentToken((CSTToken*) this);
            return;
        case LexTokenType::MultilineComment:
            visitor->visitMultilineComment((CSTToken*) this);
            return;
        case LexTokenType::String:
            visitor->visitStringToken((CSTToken*) this);
            return;
        case LexTokenType::Bool:
            visitor->visitBoolToken((CSTToken*) this);
            return;
        case LexTokenType::Annotation:
            visitor->visitAnnotationToken((CSTToken*) this);
            return;
        case LexTokenType::UserToken:
            visitor->visitUserToken((CSTToken*) this);
            return;
        case LexTokenType::Keyword:
            visitor->visitKeywordToken((CSTToken*) this);
            return;
        case LexTokenType::Number:
            visitor->visitNumberToken((NumberToken*) this);
            return;
        case LexTokenType::Type:
            visitor->visitTypeToken((CSTToken*) this);
            return;
        case LexTokenType::Null:
            visitor->visitNullToken((CSTToken*) this);
            return;
        case LexTokenType::StringOperator:
            visitor->visitStringOperatorToken((CSTToken*) this);
            return;
        case LexTokenType::Variable:
            visitor->visitVariableToken((CSTToken*) this);
            return;
        case LexTokenType::Identifier:
            visitor->visitIdentifierToken((CSTToken*) this);
            return;
        case LexTokenType::RawToken:
            visitor->visitRawToken((CSTToken*) this);
            return;
        case LexTokenType::CompAssignment:
            visitor->visitAssignment((CSTToken*) this);
            return;
        case LexTokenType::CompLoopBlock:
        case LexTokenType::CompLoopValue:
            visitor->visitLoopBlock(this);
            return;
        case LexTokenType::CompInitBlock:
            visitor->visitInitBlock((CSTToken*) this);
            return;
        case LexTokenType::CompMalformedNode:
        case LexTokenType::CompMalformedType:
        case LexTokenType::CompMalformedValue:
            visitor->visitMalformedInput((CSTToken*) this);
            return;
        case LexTokenType::CompAccessChainNode:
        case LexTokenType::CompAccessChain:
            visitor->visitAccessChain((CSTToken*) this);
            return;
        case LexTokenType::CompAnnotation:
            visitor->visitAnnotation((CSTToken*) this);
            return;
        case LexTokenType::CompBreak:
            visitor->visitBreak((CSTToken*) this);
            return;
        case LexTokenType::CompContinue:
            visitor->visitContinue((CSTToken*) this);
            return;
        case LexTokenType::CompUnreachable:
            visitor->visitUnreachable((CSTToken*) this);
            return;
        case LexTokenType::CompThrow:
            visitor->visitThrow((CSTToken*) this);
            return;
        case LexTokenType::CompUsing:
            visitor->visitUsing((CSTToken*) this);
            return;
        case LexTokenType::CompProvide:
            visitor->visitProvide((CSTToken*) this);
            return;
        case LexTokenType::CompComptime:
            visitor->visitComptimeBlock((CSTToken*) this);
            return;
        case LexTokenType::CompDestruct:
            visitor->visitDestruct((CSTToken*) this);
            return;
        case LexTokenType::CompValueNode:
            visitor->visitValueNode((CSTToken*) this);
            return;
        case LexTokenType::CompIf:
        case LexTokenType::CompIfValue:
            visitor->visitIf((CSTToken*) this);
            return;
        case LexTokenType::CompImport:
            visitor->visitImport((CSTToken*) this);
            return;
        case LexTokenType::CompIncDec:
            visitor->visitIncDec((CSTToken*) this);
            return;
        case LexTokenType::CompReturn:
            visitor->visitReturn((CSTToken*) this);
            return;
        case LexTokenType::CompSwitch:
        case LexTokenType::CompSwitchValue:
            visitor->visitSwitch((CSTToken*) this);
            return;
        case LexTokenType::CompTypealias:
            visitor->visitTypealias((CSTToken*) this);
            return;
        case LexTokenType::CompVarInit:
        case LexTokenType::CompStructMember:
            visitor->visitVarInit((CSTToken*) this);
            return;
        case LexTokenType::CompBody:
            visitor->visitBody((CSTToken*) this);
            return;
        case LexTokenType::CompUnsafeBlock:
            visitor->visitUnsafeBlock((CSTToken*) this);
            return;
        case LexTokenType::CompDoWhile:
            visitor->visitDoWhile((CSTToken*) this);
            return;
        case LexTokenType::CompEnumDecl:
            visitor->visitEnumDecl((CSTToken*) this);
            return;
        case LexTokenType::CompForLoop:
            visitor->visitForLoop((CSTToken*) this);
            return;
        case LexTokenType::CompFunctionParam:
            visitor->visitFunctionParam((CSTToken*) this);
            return;
        case LexTokenType::CompFunction:
            visitor->visitFunction((CSTToken*) this);
            return;
        case LexTokenType::CompGenericParamsList:
            visitor->visitGenericParamsList((CSTToken*) this);
            return;
        case LexTokenType::CompStructDef:
            visitor->visitStructDef((CSTToken*) this);
            return;
        case LexTokenType::CompUnionDef:
            visitor->visitUnionDef((CSTToken*) this);
            return;
        case LexTokenType::CompInterface:
            visitor->visitInterface((CSTToken*) this);
            return;
        case LexTokenType::CompImpl:
            visitor->visitImpl((CSTToken*) this);
            return;
        case LexTokenType::CompNamespace:
            visitor->visitNamespace((CSTToken*) this);
            return;
        case LexTokenType::CompTryCatch:
            visitor->visitTryCatch((CSTToken*) this);
            return;
        case LexTokenType::CompMacro:
            visitor->visitMacro((CSTToken*) this);
            return;
        case LexTokenType::CompWhile:
            visitor->visitWhile((CSTToken*) this);
            return;
        case LexTokenType::CompArrayType:
            visitor->visitArrayType((CSTToken*) this);
            return;
        case LexTokenType::CompFunctionType:
            visitor->visitFunctionType((CSTToken*) this);
            return;
        case LexTokenType::CompQualifiedType:
            visitor->visitQualifiedType((CSTToken*) this);
            return;
        case LexTokenType::CompGenericType:
            visitor->visitGenericType((CSTToken*) this);
            return;
        case LexTokenType::CompLinkedValueType:
            visitor->visitLinkedValueType((CSTToken*) this);
            return;
        case LexTokenType::CompPointerType:
            visitor->visitPointerType((CSTToken*) this);
            return;
        case LexTokenType::CompReferenceType:
            visitor->visitReferenceType((CSTToken*) this);
            return;
        case LexTokenType::CompArrayValue:
            visitor->visitArrayValue((CSTToken*) this);
            return;
        case LexTokenType::CompCastValue:
            visitor->visitCast((CSTToken*) this);
            return;
        case LexTokenType::CompIsValue:
            visitor->visitIsValue((CSTToken*) this);
            return;
        case LexTokenType::CompAddrOf:
            visitor->visitAddrOf((CSTToken*) this);
            return;
        case LexTokenType::CompDeference:
            visitor->visitDereference((CSTToken*) this);
            return;
        case LexTokenType::CompExpression:
            visitor->visitExpression((CSTToken*) this);
            return;
        case LexTokenType::CompFunctionCall:
            visitor->visitFunctionCall((CSTToken*) this);
            return;
        case LexTokenType::CompIndexOp:
            visitor->visitIndexOp((CSTToken*) this);
            return;
        case LexTokenType::CompLambda:
            visitor->visitLambda((CSTToken*) this);
            return;
        case LexTokenType::CompNegative:
            visitor->visitNegative((CSTToken*) this);
            return;
        case LexTokenType::CompNot:
            visitor->visitNot((CSTToken*) this);
            return;
        case LexTokenType::CompStructValue:
            visitor->visitStructValue((CSTToken*) this);
            return;
        case LexTokenType::CompVariant:
            visitor->visitVariant((CSTToken*) this);
            return;
        case LexTokenType::CompVariantMember:
            visitor->visitVariantMember((CSTToken*) this);
            return;
        case LexTokenType::CompGenericList:
            visitor->visitGenericList((CSTToken*) this);
            return;
#ifdef DEBUG
        default:
            throw std::runtime_error("UNKNOWN TOKEN TYPE " + std::to_string((uint8_t) tok_type));
#endif
    }
}