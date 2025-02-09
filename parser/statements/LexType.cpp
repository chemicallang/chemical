// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 2/19/2024.
//

#include "parser/Parser.h"
#include "ast/types/LinkedType.h"
#include "compiler/PrimitiveTypeMap.h"
#include "ast/values/VariableIdentifier.h"
#include "ast/values/AccessChain.h"
#include "ast/types/LinkedValueType.h"
#include "ast/types/ExpressionType.h"
#include "ast/types/GenericType.h"
#include "ast/types/PointerType.h"
#include "ast/types/ReferenceType.h"
#include "ast/types/DynamicType.h"
#include "ast/types/ArrayType.h"
#include "ast/types/FunctionType.h"
#include "ast/types/LiteralType.h"
#include "ast/types/StringType.h"
#include "ast/types/AnyType.h"
#include "ast/types/BoolType.h"
#include "ast/types/StructType.h"
#include "ast/types/UnionType.h"
#include "ast/types/CharType.h"
#include "ast/types/UCharType.h"
#include "ast/types/DoubleType.h"
#include "ast/types/FloatType.h"
#include "ast/types/Float128Type.h"
#include "ast/types/IntType.h"
#include "ast/types/UIntType.h"
#include "ast/types/ShortType.h"
#include "ast/types/UShortType.h"
#include "ast/types/LongType.h"
#include "ast/types/ULongType.h"
#include "ast/types/BigIntType.h"
#include "ast/types/UBigIntType.h"
#include "ast/types/Int128Type.h"
#include "ast/types/UInt128Type.h"
#include "ast/types/LongDoubleType.h"
#include "ast/types/StringType.h"
#include "ast/types/VoidType.h"

BaseType* Parser::parseLambdaType(ASTAllocator& allocator, bool isCapturing) {
    auto t1 = consumeOfType(TokenType::LParen);
    if(t1) {

        auto func_type = new (allocator.allocate<FunctionType>()) FunctionType({}, nullptr, false, isCapturing, parent_node, loc_single(t1));
        auto prev_func_type = current_func_type;
        current_func_type = func_type;
        const auto isVariadic = parseParameterList(allocator, func_type->params);
        func_type->setIsVariadic(isVariadic);
        if(!consumeToken(TokenType::RParen)) {
            error("expected a ')' after the ')' in lambda function type");
        }
        if(consumeToken(TokenType::LambdaSym)) {
            auto type = parseType(allocator);
            if(type) {
                func_type->returnType = type;
            } else {
                error("expected a return type for lambda function type");
            }
        } else {
            error("expected '=>' for lambda function type");
        }
        current_func_type = prev_func_type;
        return func_type;
    } else {
        return nullptr;
    }
}

BaseType* Parser::parseGenericTypeAfterId(ASTAllocator& allocator, BaseType* idType) {
    if(consumeToken(TokenType::LessThanSym)) {
        std::vector<BaseType*> types;
        do {
            auto type = parseType(allocator);
            if(type) {
                types.emplace_back(type);
            } else {
                break;
            }
        } while(consumeToken(TokenType::CommaSym));
        if(!consumeToken(TokenType::GreaterThanSym)) {
            error("expected '>' for generic type");
        }

        // TODO this is not ideal
        if(types.size() == 1 && idType->linked_name() == "literal") {
            auto underlying = types.back();
            if(underlying->kind() == BaseTypeKind::Linked && ((LinkedType*) underlying)->linked_name() == "string") {
                underlying = new (allocator.allocate<StringType>()) StringType(underlying->encoded_location());
            }
            return new (allocator.allocate<LiteralType>()) LiteralType(underlying, idType->encoded_location());
        }

        return new (allocator.allocate<GenericType>()) GenericType((LinkedType*) idType, std::move(types));
    } else {
        return idType;
    }
}

BaseType* Parser::parseLinkedOrGenericType(ASTAllocator& allocator) {
    auto id = consumeIdentifierOrKeyword();
    if(!id) {
        error("missing struct / interface name in inheritance list of the struct");
        return nullptr;
    }
    auto idType = new (allocator.allocate<LinkedType>()) LinkedType(allocate_view(allocator, id->value), loc_single(id));
    return parseGenericTypeAfterId(allocator, idType);
}

BaseType* Parser::parseArrayAndPointerTypesAfterTypeId(ASTAllocator& allocator, BaseType* typeId) {
    auto t1 = consumeOfType(TokenType::LBracket);
    if(t1) {
        // optional array size
        auto expr = parseExpression(allocator);
        auto t2 = consumeOfType(TokenType::RBracket);
        if(!t2) {
            error("expected ']' for array type");
            return typeId;
        }
        typeId = new (allocator.allocate<ArrayType>()) ArrayType(typeId, expr, loc(t1, t2));
    }
    while(true) {
        auto t = consumeOfType(TokenType::MultiplySym);
        if(t) {
            warning("deprecated syntax, pointer should be before type");
            typeId = new (allocator.allocate<PointerType>()) PointerType(typeId, loc_single(t));
        } else {
            break;
        }
    }
    auto t = consumeOfType(TokenType::AmpersandSym);
    if(t) {
        warning("deprecated syntax, reference should be before type");
        typeId = new (allocator.allocate<ReferenceType>()) ReferenceType(typeId, loc_single(t));
    }
    return typeId;
}

LinkedValueType* Parser::parseLinkedValueType(ASTAllocator& allocator, Token* type, SourceLocation location) {
    auto first_id = new (allocator.allocate<VariableIdentifier>()) VariableIdentifier(allocate_view(allocator, type->value), location, true);
    auto chain = new (allocator.allocate<AccessChain>()) AccessChain({ first_id }, false, location);
    while(true) {
        if(token->type == TokenType::DoubleColonSym) {
            token++;
            auto new_type = consumeIdentifierOrKeyword();
            if(new_type) {
                auto id = new (allocator.allocate<VariableIdentifier>()) VariableIdentifier(allocate_view(allocator, new_type->value), loc_single(new_type), true);
                chain->values.emplace_back(id);
            } else {
                error("expected an identifier after '" + type->value.str() + "::' for a type");
                return nullptr;
            }
        } else {
            return new (allocator.allocate<LinkedValueType>()) LinkedValueType(chain, chain->location);
        }
    }
}

/**
 * since dyn Phone[] means (dyn Phone)[] and not dyn (Phone[])
 * since dyn Phone* or dyn *Phone means (dyn Phone)* or *(dyn Phone) and not dyn (*Phone) or dyn (Phone*)
 * since dyn &Phone or dyn &Phone means (dyn Phone)& or &(dyn Phone) and not dyn (&Phone) or dyn (Phone&)
 */
BaseType* make_dynamic_type(ASTAllocator& allocator, BaseType* elem_type, SourceLocation loc) {
    auto t = elem_type;
    const auto kind = elem_type->kind();
    switch(kind) {
        case BaseTypeKind::Array:{
            // since dyn Phone[] means (dyn Phone)[] and not dyn (Phone[])
            const auto arr_elem_type = ((ArrayType*) t)->elem_type;
            ((ArrayType*) t)->elem_type = new (allocator.allocate<DynamicType>()) DynamicType(arr_elem_type, loc);
            return t;
        }
        case BaseTypeKind::Pointer: {
            // since dyn Phone* or dyn *Phone means (dyn Phone)* or *(dyn Phone) and not dyn (*Phone) or dyn (Phone*)
            const auto ptr_elem_type = ((PointerType*) t)->type;
            ((PointerType*) t)->type = new(allocator.allocate<DynamicType>()) DynamicType(ptr_elem_type, loc);
            return t;
        }
        case BaseTypeKind::Reference: {
            // since dyn &Phone or dyn &Phone means (dyn Phone)& or &(dyn Phone) and not dyn (&Phone) or dyn (Phone&)
            const auto ref_elem_type = ((ReferenceType*) t)->type;
            ((ReferenceType*) t)->type = new(allocator.allocate<DynamicType>()) DynamicType(ref_elem_type, loc);
            return t;
        }
        case BaseTypeKind::Linked:
        case BaseTypeKind::Generic:
            return new (allocator.allocate<DynamicType>()) DynamicType(t, loc);
        default:
            // TODO error out unknown child type with dynamic type
            return new (allocator.allocate<DynamicType>()) DynamicType(t, loc);
    }
}

StructType* Parser::parseStructType(ASTAllocator& allocator) {
    auto& t = *token;
    if(t.type == TokenType::StructKw) {

        token++;


        // maybe null
        const auto id = consumeIdentifierOrKeyword();

        const auto type = new (allocator.allocate<StructType>()) StructType(id ? allocate_view(allocator, id->value) : chem::string_view(""), parent_node, loc_single(t));

        if(token->type != TokenType::LBrace) {
            error("expected a '{' after the struct keyword for struct type");
            return type;
        }

        token++;

        do {
            consumeNewLines();
            if(parseVariableMemberInto(type, allocator, AccessSpecifier::Public)) {
                consumeToken(TokenType::SemiColonSym);
            } else {
                break;
            }
        } while(token->type != TokenType::RBrace);

        if(token->type != TokenType::RBrace) {
            error("expected a '}' after the struct type declaration");
            return type;
        }

        token++;

        return type;
    } else {
        return nullptr;
    }
}

UnionType* Parser::parseUnionType(ASTAllocator& allocator) {
    auto& t = *token;
    if(t.type == TokenType::UnionKw) {

        token++;

        // maybe null
        const auto id = consumeIdentifierOrKeyword();

        const auto type = new (allocator.allocate<UnionType>()) UnionType(id ? allocate_view(allocator, id->value) : chem::string_view(""), parent_node, loc_single(t));

        if(token->type != TokenType::LBrace) {
            error("expected a '{' after the struct keyword for union type");
            return type;
        }

        token++;

        do {
            consumeNewLines();
            if(parseVariableMemberInto(type, allocator, AccessSpecifier::Public)) {
                consumeToken(TokenType::SemiColonSym);
            } else {
                break;
            }
        } while(token->type != TokenType::RBrace);

        if(token->type != TokenType::RBrace) {
            error("expected a '}' after the union type declaration");
            return type;
        }

        token++;

        return type;
    } else {
        return nullptr;
    }
}

BaseType* Parser::parseBracketedType(ASTAllocator& allocator, BaseType* firstType, SourceLocation loc) {
    const auto type = token->type;
    if(type == TokenType::RBracket) {
        token++;
        return firstType;
    }
    const auto isLogicalAnd = type == TokenType::LogicalAndSym;
    if(isLogicalAnd || type == TokenType::LogicalOrSym) {
        token++;
    } else {
        error("expected a right parenthesis or expression type in parenthesized type");
        return firstType;
    }
    const auto secondType = parseType(allocator);
    if(!secondType) {
        error("expected a second type for expression type");
        return firstType;
    }
    const auto expr = new (allocator.allocate<ExpressionType>()) ExpressionType(firstType, secondType, isLogicalAnd, loc);
    if(token->type == TokenType::RBracket) {
        token++;
    } else {
        error("expected a right parenthesis or expression type in parenthesized type");
    }
    return expr;
}

/**
 * here's how mutable, dynamic and pointer types work
 * *mut Phone <-- direct linked type made mutable, ptr before type finds the linked type, makes itself mutable \n
 * *mut dyn Phone <-- mut finds dynamic type, get's the linked type, makes it mutable, ptr before type, finds the dynamic type, makes it self mutable based on it's child linked type \n
 * *dyn mut Phone <-- mut finds linked type, makes it mutable, ptr before type finds dynamic type, makes it self mutable based on it's child linked type type \n
 * **mut Phone <-- pointer finds pointer, makes itself mutable based on it \n
 * mut Phone* <-- mut finds pointer type, makes its child linked type mutable \n
 * mut dyn Phone* <-- mut finds dyn type, makes its child linked type mutable \n
 * dyn mut Phone* <--- mut finds pointer type, makes it's child linked type mutable \n
 * mut Phone** <-- mut finds pointer type, makes it's child pointer type mutable and grand child linked type mutable \n
 */
BaseType* Parser::parseType(ASTAllocator& allocator) {

    switch(token->type) {
        case TokenType::StructKw:
            return parseStructType(allocator);
        case TokenType::UnionKw:
            return parseUnionType(allocator);
        case TokenType::LBracket:{
            token++;
            const auto has_number = token->type == TokenType::Number;
            if(has_number) {
                // max number of parameters in capture list for lambda
                token++;
            }
            if(consumeToken(TokenType::RBracket)) {
                auto lambdaType = parseLambdaType(allocator, true);
                if(lambdaType) {
                    return lambdaType;
                } else {
                    error("expected a lambda type after '[]'");
                    return nullptr;
                }
            } else if(has_number) {
                error("expected ']' after '[' for lambda type");
                return nullptr;
            } else {
                const auto loc = loc_single(token);
                const auto firstType = parseType(allocator);
                if(firstType) {
                    parseBracketedType(allocator, firstType, loc);
                } else {
                    error("expected ']' after '[' for lambda type");
                    return nullptr;
                }
            }
        }
        case TokenType::LParen:
            return parseLambdaType(allocator, false);
        case TokenType::MultiplySym: {
            const auto ptrToken = token;
            token++;
            auto is_mutable = token->type == TokenType::MutKw;
            if (is_mutable) {
                token++;
            }
            auto type = parseType(allocator);
            if (type) {
                return new(allocator.allocate<PointerType>()) PointerType(type, loc_single(ptrToken), is_mutable);
            } else {
                error("expected a type after the *");
                return nullptr;
            }
        }
        case TokenType::AmpersandSym: {
            const auto refToken = token;
            token++;
            auto is_mutable = token->type == TokenType::MutKw;
            if(is_mutable) {
                token++;
            }
            auto type = parseType(allocator);
            if(type) {
                return new (allocator.allocate<ReferenceType>()) ReferenceType(type, loc_single(refToken), is_mutable);
            } else {
                error("expected a type after the &");
                return nullptr;
            }
        }
        case TokenType::DynKw: {
            const auto dynToken = token;
            token++;
            auto type = parseType(allocator);
            if (type) {
                return make_dynamic_type(allocator, type, loc_single(dynToken));
            } else {
                error("expected a type after the qualifier");
                return nullptr;
            }
        }
        case TokenType::MutKw: {
            token++;
            auto type = parseType(allocator);
            if(type) {
                type->make_mutable(type->kind());
                return type;
            } else {
                error("expected a type after the qualifier");
                return nullptr;
            }
        }
        default:
            break;
    }

    auto typeToken = token;
    const auto idType = typeToken->type;
    if(Token::isKeywordOrId(idType)) {
        token++;
    } else {
        return nullptr;
    }
    const auto location = loc_single(typeToken);
    BaseType* type;
    switch(idType) {
        case TokenType::AnyKw:
            type = new (allocator.allocate<AnyType>()) AnyType(location);
            break;
        case TokenType::BoolKw:
            type = new (allocator.allocate<BoolType>()) BoolType(location);
            break;
        case TokenType::CharKw:
            type = new (allocator.allocate<CharType>()) CharType(location);
            break;
        case TokenType::UCharKw:
            type = new (allocator.allocate<UCharType>()) UCharType(location);
            break;
        case TokenType::DoubleKw:
            type = new (allocator.allocate<DoubleType>()) DoubleType(location);
            break;
        case TokenType::FloatKw:
            type = new (allocator.allocate<FloatType>()) FloatType(location);
            break;
        case TokenType::LongdoubleKw:
            type = new (allocator.allocate<LongDoubleType>()) LongDoubleType(location);
            break;
        case TokenType::IntKw:
            type = new (allocator.allocate<IntType>()) IntType(location);
            break;
        case TokenType::UIntKw:
            type = new (allocator.allocate<UIntType>()) UIntType(location);
            break;
        case TokenType::ShortKw:
            type = new (allocator.allocate<ShortType>()) ShortType(location);
            break;
        case TokenType::UShortKw:
            type = new (allocator.allocate<UShortType>()) UShortType(location);
            break;
        case TokenType::LongKw:
            type = new (allocator.allocate<LongType>()) LongType(is64Bit, location);
            break;
        case TokenType::ULongKw:
            type = new (allocator.allocate<ULongType>()) ULongType(is64Bit, location);
            break;
        case TokenType::BigintKw:
            type = new (allocator.allocate<BigIntType>()) BigIntType(location);
            break;
        case TokenType::UBigintKw:
            type = new (allocator.allocate<UBigIntType>()) UBigIntType(location);
            break;
        case TokenType::Int128Kw:
            type = new (allocator.allocate<Int128Type>()) Int128Type(location);
            break;
        case TokenType::Uint128Kw:
            type = new (allocator.allocate<UInt128Type>()) UInt128Type(location);
            break;
        case TokenType::Float128Kw:
            type = new (allocator.allocate<Float128Type>()) Float128Type(location);
            break;
        case TokenType::VoidKw:
            type = new (allocator.allocate<VoidType>()) VoidType(location);
            break;
        default:
            if(token->type == TokenType::DoubleColonSym) {
                type = parseLinkedValueType(allocator, typeToken, location);
            } else {
                type = new (allocator.allocate<LinkedType>()) LinkedType(allocate_view(allocator, typeToken->value), location);
            }
            type = parseGenericTypeAfterId(allocator, type);
            break;
    }
    type = parseArrayAndPointerTypesAfterTypeId(allocator, type);
    return type;
}