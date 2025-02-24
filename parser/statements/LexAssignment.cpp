// Copyright (c) Chemical Language Foundation 2025.

#include "parser/Parser.h"
#include "ast/statements/Assignment.h"
#include "ast/values/IntValue.h"
#include "ast/values/NumberValue.h"
#include "ast/values/AccessChain.h"
#include "ast/values/IncDecValue.h"
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

ASTNode* Parser::parseAssignmentStmt(ASTAllocator& allocator) {

    auto& start_tok = *token;

    auto lhs = parseAccessChainOrAddrOf(allocator);
    if(!lhs) {
        return nullptr;
    }

    // increment or decrement
    auto& tok = *token;
    switch(tok.type) {
        case TokenType::DoublePlusSym: {
            token++;
            const auto val = new (allocator.allocate<IncDecValue>()) IncDecValue(lhs, true, true, loc_single(tok));
            return new (allocator.allocate<ValueWrapperNode>()) ValueWrapperNode(val, parent_node);
        }
        case TokenType::DoubleMinusSym:{
            token++;
            const auto val = new (allocator.allocate<IncDecValue>()) IncDecValue(lhs, false, true, loc_single(tok));
            return new (allocator.allocate<ValueWrapperNode>()) ValueWrapperNode(val, parent_node);
        }
        default:
            break;
    }

    // lex the operator before the equal sign
    auto assOp = parseAssignmentOperator();

    // =
    if (!consumeToken(TokenType::EqualSym)) {
        if (assOp.has_value()) {
            error("expected an equal for assignment after the assignment operator");
        }
        const auto lhs_kind = lhs->val_kind();
        if (lhs_kind == ValueKind::AccessChain) {
            const auto chain = lhs->as_access_chain_unsafe();
            chain->set_is_node(true);
            return new (allocator.allocate<ValueWrapperNode>()) ValueWrapperNode(chain, parent_node);
        } else if(lhs_kind == ValueKind::IncDecValue) {
            return new (allocator.allocate<ValueWrapperNode>()) ValueWrapperNode(lhs, parent_node);
        }
    }

    auto stmt = new (allocator.allocate<AssignStatement>()) AssignStatement(lhs, nullptr, Operation::Assignment, parent_node, loc_single(start_tok));

    if(assOp.has_value()) {
        stmt->assOp = assOp.value();
    }

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