// Copyright (c) Chemical Language Foundation 2025.

//
// Created by Waqas Tahir on 26/02/2024.
//

#include "parser/Parser.h"
#include "ast/structures/LoopBlock.h"
#include "ast/statements/Break.h"
#include "ast/statements/Continue.h"
#include "ast/statements/Unreachable.h"
#include "ast/structures/ForLoop.h"
#include "ast/values/NullValue.h"
#include "ast/values/LoopValue.h"

ContinueStatement* Parser::parseContinueStatement(ASTAllocator& allocator) {
    auto& tok = *token;
    if(tok.type == TokenType::ContinueKw) {
        token++;
        auto stmt = new (allocator.allocate<ContinueStatement>()) ContinueStatement(parent_node, loc_single(tok));
        return stmt;
    } else {
        return nullptr;
    }
}

BreakStatement* Parser::parseBreakStatement(ASTAllocator& allocator) {
    auto& tok = *token;
    if(tok.type == TokenType::BreakKw) {
        token++;
        auto stmt = new (allocator.allocate<BreakStatement>()) BreakStatement(parent_node, loc_single(tok));
        auto value = parseAccessChainOrValue(allocator);
        if(value) {
            stmt->value = value;
        } else {
            stmt->value = nullptr;
        }
        return stmt;
    } else {
        return nullptr;
    }
}

UnreachableStmt* Parser::parseUnreachableStatement(ASTAllocator& allocator) {
    auto& tok = *token;
    if(tok.type == TokenType::UnreachableKw) {
        token++;
        const auto stmt = new (allocator.allocate<UnreachableStmt>()) UnreachableStmt(parent_node, loc_single(tok));
        return stmt;
    } else {
        return nullptr;
    }
}

ForLoop* fix_loop(ForLoop* loop, ASTAllocator& allocator) {
    const auto unreachableStmt = new (allocator.allocate<UnreachableStmt>()) UnreachableStmt(loop, ZERO_LOC);
    if(!loop->initializer) {
        loop->initializer = unreachableStmt;
    }
    if(!loop->conditionExpr) {
        loop->conditionExpr = new (allocator.allocate<NullValue>()) NullValue(nullptr, ZERO_LOC);
    }
    if(!loop->incrementerExpr) {
        loop->incrementerExpr = unreachableStmt;
    }
    return loop;
}

ForLoop* Parser::parseForLoop(ASTAllocator& allocator) {

    auto& tok = *token;

    if (tok.type != TokenType::ForKw) {
        return nullptr;
    }

    token++;

    auto loop = new (allocator.allocate<ForLoop>()) ForLoop(nullptr, nullptr, nullptr, parent_node, loc_single(tok));

    // start parenthesis
    if(!consumeToken(TokenType::LParen)) {
        unexpected_error("expected a starting parenthesis ( in a for block");
        return fix_loop(loop, allocator);
    }

    switch(token->type) {
        case TokenType::VarKw:
        case TokenType::ConstKw: {
            auto statement = parseVarInitializationTokens(allocator, AccessSpecifier::Internal, false, false, false, false);
            if(statement) {
                loop->initializer = statement;
                break;
            } else {
                unexpected_error("expected a var initialization in for loop");
                return fix_loop(loop, allocator);
            }
        }
        default: {
            const auto assign = parseAssignmentStmt(allocator);
            if(assign) {
                loop->initializer = assign;
                break;
            } else {
                unexpected_error("expected a variable initialization or assignment statement in for loop");
                return fix_loop(loop, allocator);
            }
        }
    }

    // lex ;
    if(!consumeToken(TokenType::SemiColonSym)){
        unexpected_error("expected semicolon ; after the initialization in for loop");
        return fix_loop(loop, allocator);
    }

    // lex conditional expression
    auto expr = parseExpression(allocator);
    if(expr) {
        loop->conditionExpr = expr;
    } else {
        unexpected_error("expected a conditional expression after the var initialization in for loop");
        return fix_loop(loop, allocator);
    }

    if(!consumeToken(TokenType::SemiColonSym)){
        unexpected_error("expected semicolon ; after the condition in for loop");
        return fix_loop(loop, allocator);
    }

    // lex assignment token
    auto assignment = parseAssignmentStmt(allocator);
    if(assignment) {
        loop->incrementerExpr = assignment;
    } else {
        error("missing assignment in for loop after the condition");
        return fix_loop(loop, allocator);
    }

    // end parenthesis
    if(!consumeToken(TokenType::RParen)) {
        unexpected_error("expected a closing parenthesis ) in a for block");
        return loop;
    }

    // { statement(s) } with continue & break support
    auto block = parseBraceBlockOrSingleStmt("forloop", loop, allocator);
    if(block.has_value()) {
        loop->body = std::move(block.value());
    } else {
        unexpected_error("expected a brace block in a for block");
        return loop;
    }

    return loop;

}

void Parser::parseLoopBlock(ASTAllocator& allocator, LoopBlock* loopBlock) {
    // { statement(s) } with continue & break support
    auto block = parseBraceBlock("loop", loopBlock, allocator);
    if(block.has_value()) {
        loopBlock->body = std::move(block.value());
    } else {
        unexpected_error("expected a brace block in a for block");
        return;
    }

}

LoopBlock* Parser::parseLoopBlockTokens(ASTAllocator& allocator) {

    auto& tok = *token;
    if (tok.type != TokenType::LoopKw) {
        return nullptr;
    }

    token++;

    auto loopBlock = new (allocator.allocate<LoopBlock>()) LoopBlock(parent_node, loc_single(tok));

    parseLoopBlock(allocator, loopBlock);

    return loopBlock;

}

LoopValue* Parser::parseLoopValue(ASTAllocator& allocator) {

    auto& tok = *token;
    if (tok.type != TokenType::LoopKw) {
        return nullptr;
    }

    token++;

    const auto val = new (allocator.allocate<LoopValue>()) LoopValue(parent_node, loc_single(tok));

    parseLoopBlock(allocator, &val->stmt);

    return val;

}