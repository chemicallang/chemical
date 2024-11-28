// Copyright (c) Qinetik 2024.

#include "parser/Parser.h"

bool Parser::lexLanguageOperatorToken() {
    return lexOperationToken(TokenType::LogicalAndSym, Operation::LogicalAND) || // logical
           lexOperationToken(TokenType::LogicalOrSym, Operation::LogicalOR) ||
           // arithmetic
           lexOperationToken(TokenType::PlusSym, Operation::Addition) ||
           lexOperationToken(TokenType::MinusSym, Operation::Subtraction) ||
           lexOperationToken(TokenType::MultiplySym, Operation::Multiplication) ||
           lexOperationToken(TokenType::DivideSym, Operation::Division) ||
           lexOperationToken(TokenType::ModSym, Operation::Modulus) ||
           lexOperationToken(TokenType::AmpersandSym, Operation::BitwiseAND) ||
           lexOperationToken(TokenType::PipeSym, Operation::BitwiseOR) ||
           lexOperationToken(TokenType::CaretUpSym, Operation::BitwiseXOR) ||
           // shift
           lexOperationToken(TokenType::LeftShiftSym, Operation::LeftShift) ||
            lexOperationToken(TokenType::RightShiftSym, Operation::RightShift) ||
           // conditional
           lexOperationToken(TokenType::GreaterThanOrEqualSym, Operation::GreaterThanOrEqual) ||
            lexOperationToken(TokenType::LessThanOrEqualSym, Operation::LessThanOrEqual) ||
            lexOperationToken(TokenType::GreaterThanSym, Operation::GreaterThan) ||
            lexOperationToken(TokenType::LessThanSym, Operation::LessThan) ||
            lexOperationToken(TokenType::DoubleEqualSym, Operation::IsEqual) ||
            lexOperationToken(TokenType::NotEqualSym, Operation::IsNotEqual);
}

bool Parser::lexAssignmentOperatorToken() {
    return lexOperationToken(TokenType::PlusSym, Operation::Addition) ||
           lexOperationToken(TokenType::MinusSym, Operation::Subtraction) ||
           lexOperationToken(TokenType::MultiplySym, Operation::Multiplication) ||
           lexOperationToken(TokenType::DivideSym, Operation::Division) ||
           lexOperationToken(TokenType::ModSym, Operation::Modulus) ||
           lexOperationToken(TokenType::AmpersandSym, Operation::BitwiseAND) ||
           lexOperationToken(TokenType::PipeSym, Operation::BitwiseOR) ||
           lexOperationToken(TokenType::CaretUpSym, Operation::BitwiseXOR) ||
           // shift
           lexOperationToken(TokenType::LeftShiftSym, Operation::LeftShift) ||
            lexOperationToken(TokenType::RightShiftSym, Operation::RightShift);
}

bool Parser::lexAssignmentTokens() {

    if (!lexAccessChainOrAddrOf()) {
        return false;
    }

    auto start = tokens_size() - 1;

    // increment or decrement
    if (lexOperationToken(TokenType::DoublePlusSym, Operation::PostfixIncrement) || lexOperationToken(TokenType::DoubleMinusSym, Operation::PostfixDecrement)) {
        compound_from(start, LexTokenType::CompIncDec);
        return true;
    }

    // whitespace
    lexWhitespaceToken();


    // lex the operator before the equal sign
    auto assOp = lexAssignmentOperatorToken();

    // =
    if (!lexOperatorToken(TokenType::EqualSym)) {
        if (assOp) {
            error("expected an equal for assignment after the assignment operator");
        }
        auto& chain = *unit.tokens[start];
        if(chain.tok_type == LexTokenType::CompAccessChain) {
            chain.tok_type = LexTokenType::CompAccessChainNode;
            return true;
        } else {
            return false;
        }
    }

    // whitespace
    lexWhitespaceToken();

    // value
    if (!(lexExpressionTokens(true) || lexArrayInit())) {
        error("expected a value for variable assignment");
        return true;
    }

    compound_from(start, LexTokenType::CompAssignment);

    return true;

}