// Copyright (c) Chemical Language Foundation 2025.

#include "parser/Parser.h"
#include "ast/statements/Assignment.h"
#include "ast/values/IntValue.h"
#include "ast/values/NullValue.h"
#include "ast/values/NumberValue.h"
#include "ast/values/AccessChain.h"
#include "ast/statements/IncDecNode.h"
#include "ast/statements/AccessChainNode.h"
#include "ast/statements/ValueWrapperNode.h"

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

inline AccessChain* from_values(ASTAllocator& allocator, std::vector<ChainValue*>& chain_values) {
    const auto loc = chain_values.front()->encoded_location();
    return new (allocator.allocate<AccessChain>()) AccessChain(std::move(chain_values), false, loc);
}

ASTNode* Parser::parseAssignmentStmt(ASTAllocator& allocator) {

    auto& start_tok = *token;

    // allocated on stack, no allocation until push
    std::vector<ChainValue*> chain_values;

    // parse
    auto lhsNonAccessChain = parseLhsValue(allocator);
    if(lhsNonAccessChain == nullptr) {

        // parse access chain values into the vec
        parseAccessChain(allocator, chain_values);
        if(chain_values.empty()) {
            return nullptr;
        }

        // increment or decrement (only when its an access chain)
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
        if(lhsNonAccessChain) {
            // TODO: remove this in favor of IncDecNode
            return new(allocator.allocate<ValueWrapperNode>()) ValueWrapperNode(lhsNonAccessChain, parent_node);
        } else if(!chain_values.empty()) {
            const auto node = new (allocator.allocate<AccessChainNode>()) AccessChainNode(chain_values.front()->encoded_location(), parent_node);
            node->chain.values = std::move(chain_values);
            return node;
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