// Copyright (c) Chemical Language Foundation 2025.

//
// Created by Waqas Tahir on 2/19/2024.
//

#include "parser/Parser.h"
#include "ast/types/LinkedType.h"
#include "ast/values/VariableIdentifier.h"
#include "ast/base/TypeBuilder.h"
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

TypeLoc Parser::parseLambdaTypeLoc(ASTAllocator& allocator, bool isCapturing) {
    auto t1 = consumeOfType(TokenType::LParen);
    if(t1) {
        const auto loc = loc_single(t1);
        auto func_type = new (allocator.allocate<FunctionType>()) FunctionType(nullptr, false, isCapturing, false);
        const auto isVariadic = parseParameterList(allocator, func_type->params);
        func_type->setIsVariadic(isVariadic);
        consumeNewLines();
        if(!consumeToken(TokenType::RParen)) {
            error("expected a ')' after the ')' in lambda function type");
        }
        if(consumeToken(TokenType::LambdaSym)) {
            auto type = parseTypeLoc(allocator);
            if(type) {
                func_type->returnType = type;
            } else {
                error("expected a return type for lambda function type");
            }
        } else {
            error("expected '=>' for lambda function type");
        }
        return { func_type, loc };
    } else {
        return { nullptr, ZERO_LOC };
    }
}

BaseType* Parser::parseLambdaType(ASTAllocator& allocator, bool isCapturing) {
    return const_cast<BaseType*>(parseLambdaTypeLoc(allocator, isCapturing).getType());
}

BaseType* Parser::parseGenericTypeAfterId(ASTAllocator& allocator, BaseType* idType) {
    if(consumeToken(TokenType::LessThanSym)) {
        std::vector<TypeLoc> types;
        do {
            auto type = parseTypeLoc(allocator);
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
        if(types.size() == 1 && ((NamedLinkedType*) idType)->debug_link_name() == "literal") {
            auto underlying = types.back();
            if(underlying->kind() == BaseTypeKind::Linked && ((NamedLinkedType*) underlying.getType())->debug_link_name() == "string") {
                underlying = {new(allocator.allocate<StringType>()) StringType(), underlying.getLocation()};
            }
            return new (allocator.allocate<LiteralType>()) LiteralType(underlying);
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
    auto idType = new (allocator.allocate<NamedLinkedType>()) NamedLinkedType(allocate_view(allocator, id->value));
    return parseGenericTypeAfterId(allocator, idType);
}

BaseType* Parser::parseArrayAndPointerTypesAfterTypeId(ASTAllocator& allocator, BaseType* typeId, SourceLocation location) {
    while(true) {
        if(consumeToken(TokenType::LBracket)) {
            // optional array size
            auto expr = parseExpression(allocator);
            if(!consumeToken(TokenType::RBracket)) {
                error("expected ']' for array type");
                return typeId;
            }
            typeId = new (allocator.allocate<ArrayType>()) ArrayType({typeId, location}, expr);
        } else {
            break;
        }
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
                error() << "expected an identifier after '" << type->value << "::' for a type";
                return nullptr;
            }
        } else {
            return new (allocator.allocate<LinkedValueType>()) LinkedValueType(chain);
        }
    }
}

/**
 * since dyn Phone[] means (dyn Phone)[] and not dyn (Phone[])
 * since dyn Phone* or dyn *Phone means (dyn Phone)* or *(dyn Phone) and not dyn (*Phone) or dyn (Phone*)
 * since dyn &Phone or dyn &Phone means (dyn Phone)& or &(dyn Phone) and not dyn (&Phone) or dyn (Phone&)
 */
BaseType* make_dynamic_type(ASTAllocator& allocator, BaseType* elem_type, SourceLocation location) {
    auto t = elem_type;
    const auto kind = elem_type->kind();
    switch(kind) {
        case BaseTypeKind::Array:{
            // since dyn Phone[] means (dyn Phone)[] and not dyn (Phone[])
            const auto arr_elem_type = ((ArrayType*) t)->elem_type;
            ((ArrayType*) t)->elem_type = {new (allocator.allocate<DynamicType>()) DynamicType(arr_elem_type), location};
            return t;
        }
        case BaseTypeKind::Pointer: {
            // since dyn Phone* or dyn *Phone means (dyn Phone)* or *(dyn Phone) and not dyn (*Phone) or dyn (Phone*)
            const auto ptr_elem_type = ((PointerType*) t)->type;
            ((PointerType*) t)->type = new(allocator.allocate<DynamicType>()) DynamicType(ptr_elem_type);
            return t;
        }
        case BaseTypeKind::Reference: {
            // since dyn &Phone or dyn &Phone means (dyn Phone)& or &(dyn Phone) and not dyn (&Phone) or dyn (Phone&)
            const auto ref_elem_type = ((ReferenceType*) t)->type;
            ((ReferenceType*) t)->type = new(allocator.allocate<DynamicType>()) DynamicType(ref_elem_type);
            return t;
        }
        case BaseTypeKind::Linked:
        case BaseTypeKind::Generic:
            return new (allocator.allocate<DynamicType>()) DynamicType(t);
        default:
            // TODO error out unknown child type with dynamic type
            return new (allocator.allocate<DynamicType>()) DynamicType(t);
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
            if(parseContainerMembersInto(type, allocator, AccessSpecifier::Public)) {
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

TypeLoc Parser::parseStructTypeLoc(ASTAllocator& allocator) {
    const auto type = parseStructType(allocator);
    return { type, type ? type->encoded_location() : ZERO_LOC };
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
            if(parseContainerMembersInto(type, allocator, AccessSpecifier::Public)) {
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

TypeLoc Parser::parseUnionTypeLoc(ASTAllocator& allocator) {
    const auto type = parseStructType(allocator);
    return { type, type ? type->encoded_location() : ZERO_LOC };
}

// (First & Second) & Third = First & (Second & Third)
// (First & Second) | Third = (First & Second) | Third
// (First | Second) & Third = First | (Second & Third)
// (First | Second) | Third = First | (Second | Third)
// (First & (Second & Third)) & Fourth = First & ((Second & Third) & Fourth)
// (First & (Second & Third)) | Fourth = (First & (Second & Third)) | Fourth
// ((First & Second) | Third) | Fourth = ((First & Second) | (Third | Fourth)
// (First | (Second & Third)) & Fourth = (First | ((Second & Third) & Fourth))
// (First | (Second | Third)) | Fourth = (First | ((Second | Third) | Fourth)
BaseType* Parser::parseExpressionType(ASTAllocator& allocator, BaseType* firstType) {
    const auto tok_type = token->type;
    const auto isLogicalAnd = tok_type == TokenType::LogicalAndSym;
    if(isLogicalAnd || tok_type == TokenType::LogicalOrSym) {
        token++;
        const auto loc_first = loc_single(token);
        const auto secondType = parseType(allocator);
        if(!secondType) {
            error("expected second type in expression type");
            return firstType;
        }
        auto rootExpr = new (allocator.allocate<ExpressionType>()) ExpressionType(firstType, secondType, isLogicalAnd);
        auto currentExpr = rootExpr;
        while(true) {
            const auto type = token->type;
            const auto isNextLogicalAnd = type == TokenType::LogicalAndSym;
            if(isNextLogicalAnd || type == TokenType::LogicalOrSym) {
                token++;
                const auto loc = loc_single(token);
                const auto nextType = parseType(allocator);
                if(!nextType) {
                    error("expected second type in expression type");
                    return rootExpr;
                }
                if(!isNextLogicalAnd && currentExpr->isLogicalAnd) {
                    const auto isCurrentRoot = currentExpr == rootExpr;
                    currentExpr = new (allocator.allocate<ExpressionType>()) ExpressionType(currentExpr, nextType, false);
                    if(isCurrentRoot) {
                        rootExpr = currentExpr;
                    }
                } else {
                    const auto newExpr = new(allocator.allocate<ExpressionType>()) ExpressionType(currentExpr->secondType, nextType, isNextLogicalAnd);
                    currentExpr->secondType = newExpr;
                    currentExpr = newExpr;
                }
            } else {
                return rootExpr;
            }
        }
    } else {
        return firstType;
    }
}

MembersContainer* find_container_parent(ASTNode* parent) {
    if(parent == nullptr) return nullptr;
    if(ASTNode::isMembersContainer(parent->kind())) {
        return parent->as_members_container_unsafe();
    } else {
        return find_container_parent(parent->parent());
    }
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
TypeLoc Parser::parseTypeLoc(ASTAllocator& allocator) {

    switch(token->type) {
        case TokenType::SelfKw: {
            const auto self_tok = token;
            token++;
            const auto parent = find_container_parent(parent_node);
            if(parent) {
               return {new (allocator.allocate<LinkedType>()) LinkedType((ASTNode*) parent), loc_single(self_tok)};
            } else {
                error("couldn't find the parent container");
                return {nullptr, ZERO_LOC};
            }
        }
        case TokenType::StructKw:
            return parseStructTypeLoc(allocator);
        case TokenType::UnionKw:
            return parseUnionTypeLoc(allocator);
        case TokenType::LogicalOrSym:
        case TokenType::PipeSym:{
            const auto is_pipe = token->type == TokenType::PipeSym;
            token++;
            if(is_pipe) {
                const auto has_number = token->type == TokenType::Number;
                if (has_number) {
                    // max number of parameters in capture list for lambda
                    // TODO this
                    token++;
                }
                if(!consumeToken(TokenType::PipeSym)) {
                    error("expected '|' for capturing lambda type");
                    return { nullptr, ZERO_LOC };
                }
            }
            auto lambdaType = parseLambdaTypeLoc(allocator, true);
            if(lambdaType) {
                return lambdaType;
            } else {
                error("expected a lambda type after '||' for capturing lambda");
                return { nullptr, ZERO_LOC };
            }
        }
        case TokenType::LParen:
            return parseLambdaTypeLoc(allocator, false);
        case TokenType::MultiplySym: {
            const auto ptrToken = token;
            token++;
            auto is_mutable = token->type == TokenType::MutKw;
            if (is_mutable) {
                token++;
            }
            auto type = parseType(allocator);
            if (type) {
                return { new(allocator.allocate<PointerType>()) PointerType(type, is_mutable), loc_single(ptrToken) };
            } else {
                error("expected a type after the *");
                return {nullptr, ZERO_LOC};
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
                return { new(allocator.allocate<ReferenceType>()) ReferenceType(type, is_mutable), loc_single(refToken) };
            } else {
                error("expected a type after the &");
                return {nullptr, ZERO_LOC};;
            }
        }
        case TokenType::DynKw: {
            const auto dynToken = token;
            token++;
            auto type = parseType(allocator);
            if (type) {
                const auto loc = loc_single(dynToken);
                return { make_dynamic_type(allocator, type, loc), loc };
            } else {
                error("expected a type after the qualifier");
                return {nullptr, ZERO_LOC};;
            }
        }
        case TokenType::MutKw: {
            const auto loc = loc_single(token);
            token++;
            auto type = parseType(allocator);
            if(type) {
                type->make_mutable();
                return { type, loc };
            } else {
                error("expected a type after the qualifier");
                return {nullptr, ZERO_LOC};;
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
        return {nullptr, ZERO_LOC};;
    }
    const auto location = loc_single(typeToken);
    BaseType* type;
    switch(idType) {
        case TokenType::AnyKw:
            type = typeBuilder.getAnyType();
            break;
        case TokenType::BoolKw:
            type = typeBuilder.getBoolType();
            break;
        case TokenType::CharKw:
            type = typeBuilder.getCharType();
            break;
        case TokenType::UCharKw:
            type = typeBuilder.getUCharType();
            break;
        case TokenType::DoubleKw:
            type = typeBuilder.getDoubleType();
            break;
        case TokenType::FloatKw:
            type = typeBuilder.getFloatType();
            break;
        case TokenType::LongdoubleKw:
            type = typeBuilder.getLongDoubleType();
            break;
        case TokenType::IntKw:
            type = typeBuilder.getIntType();
            break;
        case TokenType::UIntKw:
            type = typeBuilder.getUIntType();
            break;
        case TokenType::ShortKw:
            type = typeBuilder.getShortType();
            break;
        case TokenType::UShortKw:
            type = typeBuilder.getUShortType();
            break;
        case TokenType::LongKw:
            type = typeBuilder.getLongType();
            break;
        case TokenType::ULongKw:
            type = typeBuilder.getULongType();
            break;
        case TokenType::BigintKw:
            type = typeBuilder.getBigIntType();
            break;
        case TokenType::UBigintKw:
            type = typeBuilder.getUBigIntType();
            break;
        case TokenType::Int128Kw:
            type = typeBuilder.getInt128Type();
            break;
        case TokenType::Uint128Kw:
            type = typeBuilder.getUInt128Type();
            break;
        case TokenType::Float128Kw:
            type = typeBuilder.getFloat128Type();
            break;
        case TokenType::VoidKw:
            type = typeBuilder.getVoidType();
            break;
        default:
            if(token->type == TokenType::DoubleColonSym) {
                type = parseLinkedValueType(allocator, typeToken, location);
            } else {
                type = new (allocator.allocate<NamedLinkedType>()) NamedLinkedType(allocate_view(allocator, typeToken->value));
            }
            type = parseGenericTypeAfterId(allocator, type);
            break;
    }
    type = parseArrayAndPointerTypesAfterTypeId(allocator, type, location);
    return {type, location};
}