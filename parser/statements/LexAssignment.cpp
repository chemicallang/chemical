// Copyright (c) Qinetik 2024.

#include "parser/Parser.h"
#include "ast/statements/Assignment.h"
#include "ast/values/IntValue.h"
#include "ast/values/NumberValue.h"
#include "ast/values/AccessChain.h"

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

ASTNode* Parser::parseAssignmentStmt(ASTAllocator& allocator) {

    auto lhs = parseAccessChainOrAddrOf(allocator);
    if(!lhs) {
        return nullptr;
    }

    // increment or decrement
    switch(token->type) {
        case TokenType::DoublePlusSym: {
            token++;
            auto rhs = new (allocator.allocate<NumberValue>()) NumberValue(1, 0);
            auto stmt = new (allocator.allocate<AssignStatement>()) AssignStatement(lhs, rhs, Operation::Addition, parent_node, 0);
            return stmt;
        }
        case TokenType::DoubleMinusSym:{
            token++;
            auto rhs = new (allocator.allocate<NumberValue>()) NumberValue(1, 0);
            auto stmt = new (allocator.allocate<AssignStatement>()) AssignStatement(lhs, rhs, Operation::Subtraction, parent_node, 0);
            return stmt;
        }
        default:
            break;
    }

    // whitespace
    lexWhitespaceToken();

    // lex the operator before the equal sign
    auto assOp = parseAssignmentOperator();

    // =
    if (!consumeToken(TokenType::EqualSym)) {
        if (assOp.has_value()) {
            error("expected an equal for assignment after the assignment operator");
        }
        if (lhs->val_kind() == ValueKind::AccessChain) {
            auto chain = lhs->as_access_chain();
            chain->is_node = true;
            return chain;
        }
    }

    auto stmt = new (allocator.allocate<AssignStatement>()) AssignStatement(lhs, nullptr, Operation::Assignment, parent_node, 0);

    if(assOp.has_value()) {
        stmt->assOp = assOp.value();
    }

    // whitespace
    lexWhitespaceToken();

    // value
    auto expr = parseExpression(allocator, true);
    if(expr) {
        stmt->value = expr;
    } else {
        auto init = parseArrayInit(allocator);
        if(init) {
            stmt->value = expr;
        } else {
            error("expected a value for variable assignment");
            return stmt;
        }
    }

    return stmt;

}