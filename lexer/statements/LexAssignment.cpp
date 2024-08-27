// Copyright (c) Qinetik 2024.

#include "lexer/Lexer.h"

bool Lexer::lexLanguageOperatorToken() {
    return lexOperatorToken("&&", Operation::LogicalAND) || // logical
           lexOperatorToken("||", Operation::LogicalOR) ||
           // arithmetic
           lexOperationToken('+', Operation::Addition) ||
           lexOperationToken('-', Operation::Subtraction) ||
           lexOperationToken('*', Operation::Multiplication) ||
           lexOperationToken('/', Operation::Division) ||
           lexOperationToken('%', Operation::Modulus) ||
           lexOperationToken('&', Operation::BitwiseAND) ||
           lexOperationToken('|', Operation::BitwiseOR) ||
           lexOperationToken('^', Operation::BitwiseXOR) ||
           // shift
           lexOperatorToken("<<", Operation::LeftShift) ||
            lexOperatorToken(">>", Operation::RightShift) ||
           // conditional
           lexOperatorToken(">=", Operation::GreaterThanOrEqual) ||
            lexOperatorToken("<=", Operation::LessThanOrEqual) ||
            lexOperationToken('>', Operation::GreaterThan) ||
            lexOperationToken('<', Operation::LessThan) ||
            lexOperatorToken("==", Operation::IsEqual) ||
           lexOperatorToken("!=", Operation::IsNotEqual);
}

bool Lexer::lexAssignmentOperatorToken() {
    return lexOperationToken('+', Operation::Addition) ||
           lexOperationToken('-', Operation::Subtraction) ||
           lexOperationToken('*', Operation::Multiplication) ||
           lexOperationToken('/', Operation::Division) ||
           lexOperationToken('%', Operation::Modulus) ||
           lexOperationToken('&', Operation::BitwiseAND) ||
           lexOperationToken('|', Operation::BitwiseOR) ||
           lexOperationToken('^', Operation::BitwiseXOR) ||
           // shift
           lexOperatorToken("<<", Operation::LeftShift) ||
           lexOperatorToken(">>", Operation::RightShift);
}

bool Lexer::lexAssignmentTokens() {

    if (!lexAccessChainOrAddrOf()) {
        return false;
    }

    auto start = tokens_size() - 1;

    // increment or decrement
    if (lexOperatorToken("++", Operation::PostfixIncrement) || lexOperatorToken("--", Operation::PostfixDecrement)) {
        compound_from(start, LexTokenType::CompIncDec);
        return true;
    }

    // whitespace
    lexWhitespaceToken();


    // lex the operator before the equal sign
    auto assOp = lexAssignmentOperatorToken();

    // =
    if (!lexOperatorToken('=')) {
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