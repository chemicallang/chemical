// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 26/02/2024.
//

#include "parser/Parser.h"
#include "ast/structures/LoopBlock.h"
#include "ast/statements/Break.h"
#include "ast/statements/Continue.h"
#include "ast/statements/Unreachable.h"
#include "ast/structures/ForLoop.h"

ContinueStatement* Parser::parseContinueStatement(ASTAllocator& allocator) {
    auto& tok = *token;
    if(tok.type == TokenType::ContinueKw) {
        token++;
        auto stmt = new (allocator.allocate<ContinueStatement>()) ContinueStatement(current_loop_node, parent_node, loc_single(tok));
        return stmt;
    } else {
        return nullptr;
    }
}

BreakStatement* Parser::parseBreakStatement(ASTAllocator& allocator) {
    auto& tok = *token;
    if(tok.type == TokenType::BreakKw) {
        token++;
        auto stmt = new (allocator.allocate<BreakStatement>()) BreakStatement(current_loop_node, parent_node, loc_single(tok));
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

ForLoop* Parser::parseForLoop(ASTAllocator& allocator) {

    auto& tok = *token;

    if (tok.type != TokenType::ForKw) {
        return nullptr;
    }

    token++;

    auto loop = new (allocator.allocate<ForLoop>()) ForLoop(nullptr, nullptr, nullptr, { nullptr, 0 }, parent_node, loc_single(tok));

    // start parenthesis
    if(!consumeToken(TokenType::LParen)) {
        error("expected a starting parenthesis ( in a for block");
        return loop;
    }

    switch(token->type) {
        case TokenType::VarKw:
        case TokenType::ConstKw: {
            auto statement = parseVarInitializationTokens(allocator, AccessSpecifier::Internal, false, false);
            if(statement) {
                loop->initializer = statement;
                break;
            } else {
                error("expected a var initialization in for loop");
                return loop;
            }
        }
        default: {
            const auto assign = parseAssignmentStmt(allocator);
            if(assign) {
                loop->initializer = assign;
                break;
            } else {
                error("expected a variable initialization or assignment statement in for loop");
                return loop;
            }
        }
    }

    // lex ;
    if(!consumeToken(TokenType::SemiColonSym)){
        error("expected semicolon ; after the initialization in for loop");
        return loop;
    }

    // lex conditional expression
    auto expr = parseExpression(allocator);
    if(expr) {
        loop->conditionExpr = expr;
    } else {
        error("expected a conditional expression after the var initialization in for loop");
        return loop;
    }

    if(!consumeToken(TokenType::SemiColonSym)){
        error("expected semicolon ; after the condition in for loop");
        return loop;
    }

    // lex assignment token
    auto assignment = parseAssignmentStmt(allocator);
    if(assignment) {
        loop->incrementerExpr = assignment;
    } else {
        error("missing assignment in for loop after the condition");
        return loop;
    }

    // end parenthesis
    if(!consumeToken(TokenType::RParen)) {
        error("expected a closing parenthesis ) in a for block");
        return loop;
    }

    // { statement(s) } with continue & break support
    auto prev_loop_node = current_loop_node;
    current_loop_node = loop;
    auto block = parseBraceBlock("forloop", loop, allocator);
    if(block.has_value()) {
        loop->body.nodes = std::move(block.value().nodes);
        loop->body.location = block.value().location;
    } else {
        error("expected a brace block in a for block");
        current_loop_node = prev_loop_node;
        return loop;
    }
    current_loop_node = prev_loop_node;

    return loop;

}

LoopBlock* Parser::parseLoopBlockTokens(ASTAllocator& allocator, bool is_value) {

    auto& tok = *token;
    if (tok.type != TokenType::LoopKw) {
        return nullptr;
    }

    token++;

    auto loopBlock = new (allocator.allocate<LoopBlock>()) LoopBlock({ parent_node, 0 }, parent_node, loc_single(tok));

    // { statement(s) } with continue & break support
    auto prev_loop_node = current_loop_node;
    current_loop_node = loopBlock;
    auto block = parseBraceBlock("loop", loopBlock, allocator);
    if(block.has_value()) {
        auto& blk = block.value();
        loopBlock->body = { std::move(blk.nodes), blk.parent_node, blk.location };
    } else {
        error("expected a brace block in a for block");
        current_loop_node = prev_loop_node;
        return loopBlock;
    }
    current_loop_node = prev_loop_node;

    return loopBlock;

}