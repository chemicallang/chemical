// Copyright (c) Chemical Language Foundation 2025.

#include "parser/Parser.h"
#include "ast/statements/Assignment.h"
#include "ast/values/IntNumValue.h"
#include "ast/values/NullValue.h"
#include "ast/values/AccessChain.h"
#include "ast/statements/IncDecNode.h"
#include "ast/statements/AccessChainNode.h"

std::optional<Operation> Parser::parseOperation() {
    switch(token->type) {
        case TokenType::LogicalAndSym:
            token++;
            return Operation::LogicalAND;
        case TokenType::LogicalOrSym:
            token++;
            return Operation::LogicalOR;
            // arithmetic
        case TokenType::PlusSym:
            token++;
            return Operation::Addition;
        case TokenType::MinusSym:
            token++;
            return Operation::Subtraction;
        case TokenType::MultiplySym:
            token++;
            return Operation::Multiplication;
        case TokenType::DivideSym:
            token++;
            return Operation::Division;
        case TokenType::ModSym:
            token++;
            return Operation::Modulus;
        case TokenType::AmpersandSym:
            token++;
            return Operation::BitwiseAND;
        case TokenType::PipeSym:
            token++;
            return Operation::BitwiseOR;
        case TokenType::CaretUpSym:
            token++;
            return Operation::BitwiseXOR;
            // shift
        case TokenType::LeftShiftSym:
            token++;
            return Operation::LeftShift;
        case TokenType::RightShiftSym:
            token++;
            return Operation::RightShift;
            // conditional
        case TokenType::GreaterThanOrEqualSym:
            token++;
            return Operation::GreaterThanOrEqual;
        case TokenType::LessThanOrEqualSym:
            token++;
            return Operation::LessThanOrEqual;
        case TokenType::GreaterThanSym:
            token++;
            return Operation::GreaterThan;
        case TokenType::LessThanSym:
            token++;
            return Operation::LessThan;
        case TokenType::DoubleEqualSym:
            token++;
            return Operation::IsEqual;
        case TokenType::NotEqualSym:
            token++;
            return Operation::IsNotEqual;
        default:
            return std::nullopt;
    }
}

std::optional<Operation> Parser::parseAssignmentOperator() {
    switch(token->type) {
        case TokenType::PlusSym:
            token++;
            return Operation::Addition;
        case TokenType::MinusSym:
            token++;
            return Operation::Subtraction;
        case TokenType::MultiplySym:
            token++;
            return Operation::Multiplication;
        case TokenType::DivideSym:
            token++;
            return Operation::Division;
        case TokenType::ModSym:
            token++;
            return Operation::Modulus;
        case TokenType::AmpersandSym:
            token++;
            return Operation::BitwiseAND;
        case TokenType::PipeSym:
            token++;
            return Operation::BitwiseOR;
        case TokenType::CaretUpSym:
            token++;
            return Operation::BitwiseXOR;
        case TokenType::LeftShiftSym:
            token++;
            return Operation::LeftShift;
        case TokenType::RightShiftSym:
            token++;
            return Operation::RightShift;
        default:
            return std::nullopt;
    }
}

inline Value* from_values(ASTAllocator& allocator, std::vector<Value*>& chain_values) {
    if(chain_values.size() == 1) {
        const auto b = chain_values.back();
        chain_values.pop_back();
        return b;
    } else {
        const auto loc = chain_values.front()->encoded_location();
        return new(allocator.allocate<AccessChain>()) AccessChain(std::move(chain_values), loc);
    }
}

ASTNode* Parser::parseAssignmentStmt(ASTAllocator& allocator) {

    auto& start_tok = *token;

    // handle pre increment and decrement
    switch(token->type) {
        case TokenType::DoublePlusSym:
            return parsePreIncDecNode(allocator, true);
        case TokenType::DoubleMinusSym:
            return parsePreIncDecNode(allocator, false);
        default:
            break;
    }

    // allocated on stack, no allocation until push
    std::vector<Value*> chain_values;

    // parse
    auto lhsNonAccessChain = parseLhsValue(allocator);
    if(lhsNonAccessChain == nullptr) {

        // parse access chain values into the vec
        parseAccessChain(allocator, chain_values);
        if(chain_values.empty()) {
            return nullptr;
        }

        // post increment or decrement (only when its an access chain)
        auto& tok = *token;
        switch(tok.type) {
            case TokenType::DoublePlusSym: {
                token++;
                return new (allocator.allocate<IncDecNode>()) IncDecNode(from_values(allocator, chain_values), true, true, loc_single(tok), parent_node);
            }
            case TokenType::DoubleMinusSym:{
                token++;
                return new (allocator.allocate<IncDecNode>()) IncDecNode(from_values(allocator, chain_values), false, true, loc_single(tok), parent_node);
            }
            default:
                break;
        }

    }

    // lex the operator before the equal sign
    auto assOp = parseAssignmentOperator();

    // =
    if (!consumeToken(TokenType::EqualSym)) {
        if (assOp.has_value()) {
            unexpected_error("expected an equal for assignment after the assignment operator");
        }
        if(!chain_values.empty()) {
            const auto node = new (allocator.allocate<AccessChainNode>()) AccessChainNode(chain_values.front()->encoded_location(), parent_node);
            node->chain.values = std::move(chain_values);
            return node;
        } else {
            unexpected_error("expected an equal for assignment without an access chain");
        }
    }

    const auto lhs = lhsNonAccessChain ? lhsNonAccessChain : from_values(allocator, chain_values);

    auto stmt = new (allocator.allocate<AssignStatement>()) AssignStatement(lhs, nullptr, Operation::Assignment, parent_node, loc_single(start_tok));

    if(assOp.has_value()) {
        stmt->assOp = assOp.value();
    }

    // value
    auto expr = parseExpressionOrArrayOrStruct(allocator);
    if(expr) {
        stmt->value = expr;
    } else {
        unexpected_error("expected a value for variable assignment");
        stmt->value = new (allocator.allocate<NullValue>()) NullValue(nullptr, ZERO_LOC);
        return stmt;
    }

    return stmt;

}