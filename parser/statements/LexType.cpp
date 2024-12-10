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
#include "ast/types/GenericType.h"
#include "ast/types/PointerType.h"
#include "ast/types/ReferenceType.h"
#include "ast/types/DynamicType.h"
#include "ast/types/ArrayType.h"
#include "ast/types/FunctionType.h"
#include "ast/types/LiteralType.h"
#include "ast/types/StringType.h"

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
        lexWhitespaceToken();
        if(consumeToken(TokenType::LambdaSym)) {
            lexWhitespaceToken();
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
            lexWhitespaceToken();
            auto type = parseType(allocator);
            if(type) {
                types.emplace_back(type);
            } else {
                break;
            }
            lexWhitespaceToken();
        } while(consumeToken(TokenType::CommaSym));
        lexWhitespaceToken();
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

        // TODO Generic Type doesn't support linked value type
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
    auto idType = new (allocator.allocate<LinkedType>()) LinkedType(std::string(id->value), loc_single(id));
    lexWhitespaceToken();
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

BaseType* Parser::parseTypeId(ASTAllocator& allocator, Token* type) {
    Token* first_type = type;
    AccessChain* chain = nullptr;
    while(true) {
        if(token->type == TokenType::DoubleColonSym) {
            token++;
            auto id = new (allocator.allocate<VariableIdentifier>()) VariableIdentifier(std::string(type->value), loc_single(type), true);
            auto new_type = consumeIdentifierOrKeyword();
            if(!new_type) {
                error("expected an identifier after '" + std::string(type->value) + "::' for a type");
                return nullptr;
            } else {
                if(chain) {
                    chain->values.emplace_back(id);
                } else {
                    chain = new (allocator.allocate<AccessChain>()) AccessChain({ id }, parent_node, false, 0);
                }
                type = new_type;
            }
        } else {
            if(chain) {
                auto id = new (allocator.allocate<VariableIdentifier>()) VariableIdentifier(std::string(type->value), loc_single(type));
                chain->values.emplace_back(id);
                chain->location = loc(first_type, type);
                return new (allocator.allocate<LinkedValueType>()) LinkedValueType(chain, chain->location);
            } else {
                auto primitive = TypeMakers::PrimitiveMap.find(type->value);
                if (primitive == TypeMakers::PrimitiveMap.end()) {
                    return new (allocator.allocate<LinkedType>()) LinkedType(std::string(type->value), loc_single(type));
                } else {
                    return primitive->second(allocator, is64Bit, loc_single(type));
                }
            }
        }
    }
}

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

BaseType* Parser::parseType(ASTAllocator& allocator) {

    auto t1 = consumeOfType(TokenType::LBracket);
    if(t1) {
        if(!consumeToken(TokenType::RBracket)) {
            error("expected ']' after '[' for lambda type");
            return nullptr;
        }
        lexWhitespaceToken();
        auto lambdaType = parseLambdaType(allocator, true);
        if(lambdaType) {
            return lambdaType;
        } else {
            error("expected a lambda type after '[]'");
            return nullptr;
        }
    }

    auto lambdaType = parseLambdaType(allocator, false);
    if(lambdaType) {
        return lambdaType;
    }

    auto ptrToken = consumeOfType(TokenType::MultiplySym);
    if(ptrToken) {
        auto type = parseType(allocator);
        if(type) {
            return new (allocator.allocate<PointerType>()) PointerType(type, loc_single(ptrToken));
        } else {
            error("expected a type after the *");
            return nullptr;
        }
    } else {
        auto refToken = consumeOfType(TokenType::AmpersandSym);
        if(refToken) {
            auto type = parseType(allocator);
            if(type) {
                return new (allocator.allocate<ReferenceType>()) ReferenceType(type, loc_single(refToken));
            } else {
                error("expected a type after the &");
                return nullptr;
            }
        }
    }

    auto dynToken = consumeWSOfType(TokenType::DynKw);
    if(dynToken) {
        auto type = parseType(allocator);
        if(type) {
            return make_dynamic_type(allocator, type, loc_single(dynToken));
        } else {
            error("expected a type after the qualifier");
            return nullptr;
        }
    } else {
        auto mutToken = consumeWSOfType(TokenType::MutKw);
        if(mutToken) {
            auto type = parseType(allocator);
            if(type) {
                type->make_mutable(type->kind());
                return type;
            } else {
                error("expected a type after the qualifier");
                return nullptr;
            }
        }
    }
    auto typeToken = consumeIdentifierOrKeyword();
    if(!typeToken) return nullptr;
    unsigned start = tokens_size();
    auto type = parseTypeId(allocator, typeToken);
    if(!type) {
        return nullptr;
    }
    type = parseGenericTypeAfterId(allocator, type);
    type = parseArrayAndPointerTypesAfterTypeId(allocator, type);
    return type;
}