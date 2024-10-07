// Copyright (c) Qinetik 2024.

#include "CSTToken.h"

CSTToken *CSTToken::start_token() {
    if(compound()) {
        if (tokens[0]->compound()) {
            return tokens[0]->start_token();
        } else {
            return tokens[0];
        }
    } else {
        return this;
    }
}

CSTToken *CSTToken::end_token() {
    if(compound()) {
        auto last = tokens.size() - 1;
        if (tokens[last]->compound()) {
            return tokens[last]->end_token();
        } else {
            return tokens[last];
        }
    } else {
        return this;
    }
}

void CSTToken::append_representation(std::string &rep) const {
    if(compound()) {
        for (const auto &tok: tokens) {
            tok->append_representation(rep);
        }
    } else {
        rep.append(value());
    }
}

std::string CSTToken::type_string() const {
    if (compound()) {
        std::string ret(toTypeString(type()));
        ret.append(1, '[');
        unsigned i = 0;
        unsigned size = tokens.size();
        while (i < size) {
            ret.append(tokens[i]->type_string());
            if (i < size - 1) ret.append(1, ',');
            i++;
        }
        ret.append(1, ']');
        return ret;
    } else {
        return toTypeString(type());
    }
}

const Position& CSTToken::start() const {
    if (compound()) {
        return tokens[0]->position();
    } else {
        return position();
    }
}

std::string toTypeString(LexTokenType token) {
    switch (token) {
        case LexTokenType::Annotation:
            return "Annotation";
        case LexTokenType::Bool:
            return "Bool";
        case LexTokenType::CharOperator:
            return "CharOperator";
        case LexTokenType::Char:
            return "Char";
        case LexTokenType::Comment:
            return "Comment";
        case LexTokenType::Identifier:
            return "Identifier";
        case LexTokenType::Keyword:
            return "Keyword";
        case LexTokenType::MultilineComment:
            return "MultilineComment";
        case LexTokenType::Null:
            return "Null";
        case LexTokenType::Number:
            return "Number";
        case LexTokenType::Operation:
            return "Operation";
        case LexTokenType::RawToken:
            return "Raw";
        case LexTokenType::StringOperator:
            return "StringOperator";
        case LexTokenType::String:
            return "String";
        case LexTokenType::Type:
            return "Type";
        case LexTokenType::UserToken:
            return "User";
        case LexTokenType::Variable:
            return "Variable";

        // compounds
        case LexTokenType::CompAssignment:
            return "CompAssignment";
        case LexTokenType::CompBreak:
            return "CompBreak";
        case LexTokenType::CompContinue:
            return "CompContinue";
        case LexTokenType::CompIf:
            return "CompIf";
        case LexTokenType::CompImport:
            return "CompImport";
        case LexTokenType::CompIncDec:
            return "CompIncDec";
        case LexTokenType::CompReturn:
            return "CompReturn";
        case LexTokenType::CompSwitch:
            return "CompSwitch";
        case LexTokenType::CompTypealias:
            return "CompTypealias";
        case LexTokenType::CompVarInit:
            return "CompVarInit";
        case LexTokenType::CompBody:
            return "CompBody";
        case LexTokenType::CompDoWhile:
            return "CompDoWhile";
        case LexTokenType::CompEnumDecl:
            return "CompEnumDecl";
        case LexTokenType::CompForLoop:
            return "CompForLoop";
        case LexTokenType::CompFunctionParam:
            return "CompFunctionParam";
        case LexTokenType::CompFunction:
            return "CompFunction";
        case LexTokenType::CompStructDef:
            return "CompStructDef";
        case LexTokenType::CompInterface:
            return "CompInterface";
        case LexTokenType::CompImpl:
            return "CompImpl";
        case LexTokenType::CompTryCatch:
            return "CompTryCatch";
        case LexTokenType::CompWhile:
            return "CompWhile";
        case LexTokenType::CompArrayType:
            return "CompArrayType";
        case LexTokenType::CompFunctionType:
            return "CompFunctionType";
        case LexTokenType::CompGenericType:
            return "CompGenericType";
        case LexTokenType::CompPointerType:
            return "CompPointerType";
        case LexTokenType::CompAccessChain:
            return "CompAccessChain";
        case LexTokenType::CompArrayValue:
            return "CompArrayValue";
        case LexTokenType::CompCastValue:
            return "CompCastValue";
        case LexTokenType::CompAddrOf:
            return "CompAddrOf";
        case LexTokenType::CompDeference:
            return "CompDeference";
        case LexTokenType::CompExpression:
            return "CompExpression";
        case LexTokenType::CompFunctionCall:
            return "CompFunctionCall";
        case LexTokenType::CompIndexOp:
            return "CompIndexOp";
        case LexTokenType::CompLambda:
            return "CompLambda";
        case LexTokenType::CompNegative:
            return "CompNegative";
        case LexTokenType::CompNot:
            return "CompNot";
        case LexTokenType::CompStructValue:
            return "CompStructValue";
        case LexTokenType::CompAccessChainNode:
            return "CompAccessChainNode";
        case LexTokenType::CompValueNode:
            return "CompValueNode";
        case LexTokenType::CompAnnotation:
            return "CompAnnotation";
        case LexTokenType::CompInitBlock:
            return "CompInitBlock";
        case LexTokenType::CompUnsafeBlock:
            return "CompUnsafeBlock";
        case LexTokenType::CompThrow:
            return "CompThrow";
        case LexTokenType::CompUsing:
            return "CompUsing";
        case LexTokenType::CompDestruct:
            return "CompDestruct";
        case LexTokenType::CompStructMember:
            return "CompStructMember";
        case LexTokenType::CompLoopBlock:
            return "CompLoopBlock";
        case LexTokenType::CompGenericParamsList:
            return "CompGenericParamsList";
        case LexTokenType::CompVariant:
            return "CompVariant";
        case LexTokenType::CompVariantMember:
            return "CompVariantMember";
        case LexTokenType::CompUnionDef:
            return "CompUnionDef";
        case LexTokenType::CompNamespace:
            return "CompNamespace";
        case LexTokenType::CompMacro:
            return "CompMacro";
        case LexTokenType::CompSpecializedType:
            return "CompSpecializedType";
        case LexTokenType::CompLinkedValueType:
            return "CompLinkedValueType";
        case LexTokenType::CompReferenceType:
            return "CompReferenceType";
        case LexTokenType::CompIfValue:
            return "CompIfValue";
        case LexTokenType::CompLoopValue:
            return "CompLoopValue";
        case LexTokenType::CompSwitchValue:
            return "CompSwitchValue";
        case LexTokenType::CompIsValue:
            return "CompIsValue";
        case LexTokenType::CompGenericList:
            return "CompGenericList";
        case LexTokenType::CompMalformedNode:
            return "CompMalformedNode";
        case LexTokenType::CompMalformedType:
            return "CompMalformedType";
        case LexTokenType::CompMalformedValue:
            return "CompMalformedValue";
    }
}