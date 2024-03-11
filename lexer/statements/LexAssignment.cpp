// Copyright (c) Qinetik 2024.

#include "lexer/Lexer.h"

bool Lexer::lexLanguageOperatorToken() {
    return lexOperatorToken('+', Operation::Addition) ||
           lexOperatorToken('-', Operation::Subtraction) ||
           lexOperatorToken('*', Operation::Multiplication) ||
           lexOperatorToken('/', Operation::Division) ||
           lexOperatorToken('%', Operation::Modulus) ||
           lexOperatorToken('&', Operation::BitwiseAND) ||
           lexOperatorToken('|', Operation::BitwiseOR) ||
           lexOperatorToken('^', Operation::BitwiseXOR) ||
           // shift
           lexOperatorToken("<<", Operation::LeftShift) ||
           lexOperatorToken(">>", Operation::RightShift) ||
           // conditional
           lexOperatorToken(">=", Operation::GreaterThanOrEqual) ||
           lexOperatorToken("<=", Operation::LessThanOrEqual) ||
           lexOperatorToken('>', Operation::GreaterThan) ||
           lexOperatorToken('<', Operation::LessThan) ||
           lexOperatorToken("==", Operation::IsEqual) ||
           lexOperatorToken("!=", Operation::IsNotEqual);
}

bool Lexer::lexAssignmentOperatorToken() {
    return lexOperatorToken('+', Operation::Addition) ||
           lexOperatorToken('-', Operation::Subtraction) ||
           lexOperatorToken('*', Operation::Multiplication) ||
           lexOperatorToken('/', Operation::Division) ||
           lexOperatorToken('%', Operation::Modulus) ||
           lexOperatorToken('&', Operation::BitwiseAND) ||
           lexOperatorToken('|', Operation::BitwiseOR) ||
           lexOperatorToken('^', Operation::BitwiseXOR) ||
           // shift
           lexOperatorToken("<<", Operation::LeftShift) ||
           lexOperatorToken(">>", Operation::RightShift);
}

bool Lexer::lexAssignmentTokens() {

    // lex an identifier token
    if (!lexAccessChain(true)) {
        return false;
    }

    // increment or decrement
    if (lexOperatorToken("++", Operation::PostfixIncrement) || lexOperatorToken("--", Operation::PostfixDecrement)) {
        return true;
    }

    // whitespace
    lexWhitespaceToken();


    // lex the operator before the equal sign
    auto assOp = lexAssignmentOperatorToken();

    // =
    if (!lexOperatorToken('=')) {
        if(assOp) {
            error("expected an equal for assignment after the assignment operator");
        }
        return true;
    }

    // whitespace
    lexWhitespaceToken();

    // value
    if (!(lexExpressionTokens() || lexArrayInit())) {
        error("expected a value for variable assignment");
        return true;
    }

    return true;

}