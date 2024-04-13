// Copyright (c) Qinetik 2024.

#include "lexer/Lexer.h"
#include "cst/statements/AssignmentCST.h"
#include "cst/values/AccessChainCST.h"
#include "cst/statements/IncDecCST.h"

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

    if(!lexAccessChain()) {
        return false;
    }

    auto start = tokens.size() - 1;

    // increment or decrement
    if (lexOperatorToken("++", Operation::PostfixIncrement) || lexOperatorToken("--", Operation::PostfixDecrement)) {
        compound_from<IncDecCST>(start);
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
        ((AccessChainCST*) tokens[start].get())->is_node = true;
        return true;
    }

    // whitespace
    lexWhitespaceToken();

    // value
    if (!(lexExpressionTokens(true) || lexArrayInit())) {
        error("expected a value for variable assignment");
        return true;
    }

    compound_from<AssignmentCST>(start);

    return true;

}