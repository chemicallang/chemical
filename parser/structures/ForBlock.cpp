// Copyright (c) Chemical Language Foundation 2025.

//
// Created by Waqas Tahir on 26/02/2024.
//

#include "ast/base/TypeBuilder.h"
#include "parser/Parser.h"
#include "ast/structures/LoopBlock.h"
#include "ast/statements/Break.h"
#include "ast/statements/Continue.h"
#include "ast/statements/Unreachable.h"
#include "ast/structures/ForInLoop.h"
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

static ForLoop* fix_loop(Parser* parser, ForLoop* loop, ASTAllocator& allocator) {
    const auto unreachableStmt = new (allocator.allocate<UnreachableStmt>()) UnreachableStmt(loop, ZERO_LOC);
    if(!loop->initializer) {
        loop->initializer = unreachableStmt;
    }
    if(!loop->conditionExpr) {
        loop->conditionExpr = parser->getErroredValue(allocator);
    }
    if(!loop->incrementerExpr) {
        loop->incrementerExpr = unreachableStmt;
    }
    return loop;
}

static ForLoop* empty_loop(Parser* parser, ASTAllocator& allocator) {
    auto loop = new (allocator.allocate<ForLoop>()) ForLoop(nullptr, nullptr, nullptr, parser->parent_node, parser->loc_single(parser->token));
    return fix_loop(parser, loop, allocator);
}

static ForInLoop* parseForInLoop(Parser& parser, ASTAllocator& allocator, const chem::string_view& id_non_alloc, const chem::string_view& index_id_non_alloc, Token& tok, bool is_reference, bool is_reference_mutable) {

    const auto loc = parser.loc_single(tok);

    const auto id = parser.allocate_view(allocator, id_non_alloc);
    const auto index_id = index_id_non_alloc.empty() ? "" : parser.allocate_view(allocator, index_id_non_alloc);

    const auto loop = new (allocator.allocate<ForInLoop>()) ForInLoop(id, nullptr, nullptr, parser.parent_node, loc);

    if (index_id.empty()) {
        loop->index_init = nullptr;
    } else {
        loop->index_init = new (allocator.allocate<VarInitStatement>()) VarInitStatement(
            false, false, index_id, TypeLoc(parser.typeBuilder.getU32Type(), loc), nullptr, loop, loc
        );
    }

    // set reference
    if (is_reference) {
        loop->elem_type = new (allocator.allocate<ReferenceType>()) ReferenceType(nullptr, is_reference_mutable);
        loop->set_is_reference(true);
    }

    auto expr = parser.parseExpression(allocator, false, false);
    if (expr == nullptr) {
        loop->expr = parser.getErroredValue(allocator);
        return loop;
    } else {
        loop->expr = expr;
    }

    if (parser.token->type == TokenType::ReversedKw) {
        parser.token++;
        // by default 'reversed' makes both loop and counter go in reverse
        loop->set_reversed(true);
        loop->set_reversed_counter(true);
    }

    if(!parser.consumeToken(TokenType::RParen)) {
        parser.unexpected_error("expected a closing parenthesis ) in a for block");
        return loop;
    }

    // { statement(s) } with continue & break support
    auto block = parser.parseBraceBlockOrSingleStmt("forinloop", loop, allocator);
    if(block.has_value()) {
        loop->body = std::move(block.value());
    } else {
        parser.unexpected_error("expected a brace block in a for block");
        return loop;
    }

    return loop;
}

static ForLoop* parseForLoopAfterInit(Parser& parser, ASTAllocator& allocator, ForLoop* loop) {
    // lex ;
    if(!parser.consumeToken(TokenType::SemiColonSym)){
        parser.unexpected_error("expected semicolon ; after the initialization in for loop");
        return fix_loop(&parser, loop, allocator);
    }

    // lex conditional expression
    auto expr = parser.parseExpression(allocator);
    if(expr) {
        loop->conditionExpr = expr;
    } else {
        parser.unexpected_error("expected a conditional expression after the var initialization in for loop");
        return fix_loop(&parser, loop, allocator);
    }

    if(!parser.consumeToken(TokenType::SemiColonSym)){
        parser.unexpected_error("expected semicolon ; after the condition in for loop");
        return fix_loop(&parser, loop, allocator);
    }

    // lex assignment token
    auto assignment = parser.parseAssignmentStmt(allocator);
    if(assignment) {
        loop->incrementerExpr = assignment;
    } else {
        parser.error("missing assignment in for loop after the condition");
        return fix_loop(&parser, loop, allocator);
    }

    // end parenthesis
    if(!parser.consumeToken(TokenType::RParen)) {
        parser.unexpected_error("expected a closing parenthesis ) in a for block");
        return loop;
    }

    // { statement(s) } with continue & break support
    auto block = parser.parseBraceBlockOrSingleStmt("forloop", loop, allocator);
    if(block.has_value()) {
        loop->body = std::move(block.value());
    } else {
        parser.unexpected_error("expected a brace block in a for block");
        return loop;
    }

    return loop;
}

ASTNode* Parser::parseForLoop(ASTAllocator& allocator) {

    auto& tok = *token;

    if (tok.type != TokenType::ForKw) {
        return nullptr;
    }

    token++;

    // start parenthesis
    if(!consumeToken(TokenType::LParen)) {
        unexpected_error("expected a starting parenthesis ( in a for block");
        return empty_loop(this, allocator);
    }

    switch(token->type) {
        case TokenType::VarKw:
        case TokenType::ConstKw: {
            auto& tok_after_var = *(token + 1);
            if (tok_after_var.type == TokenType::AmpersandSym) {
                // var& c in circles
                // var& c, i in circles
                auto& first_id_tok = *(token + 2);
                if (first_id_tok.type == TokenType::Identifier) {
                    const auto tok_after_id_type = (token + 3)->type;
                    if (tok_after_id_type == TokenType::InKw) {
                        token += 4;
                        return parseForInLoop(*this, allocator, first_id_tok.value, "", tok, true, token->type == TokenType::VarKw);
                    } else if (tok_after_id_type == TokenType::CommaSym) {
                        auto& second_id_tok = *(token + 4);
                        if (second_id_tok.type == TokenType::Identifier && (token + 5)->type == TokenType::InKw) {
                            token += 6;
                            return parseForInLoop(*this, allocator, first_id_tok.value, second_id_tok.value, tok, true, token->type == TokenType::VarKw);
                        }
                    }
                }

            } else if (tok_after_var.type == TokenType::Identifier) {
                // var c in circles
                // var c, i in circles
                const auto tok_after_id_type = (token + 2)->type;
                if (tok_after_id_type == TokenType::InKw) {
                    token += 3;
                    return parseForInLoop(*this, allocator, tok_after_var.value, "", tok, false, false);
                } else if (tok_after_id_type == TokenType::CommaSym) {
                    auto& second_id_tok = *(token + 3);
                    if (second_id_tok.type == TokenType::Identifier && (token + 4)->type == TokenType::InKw) {
                        token += 5;
                        return parseForInLoop(*this, allocator, tok_after_var.value, second_id_tok.value, tok, false, false);
                    }
                }
            }
            auto loop = new (allocator.allocate<ForLoop>()) ForLoop(nullptr, nullptr, nullptr, parent_node, loc_single(tok));
            auto statement = parseVarInitializationTokens(allocator, AccessSpecifier::Internal, false, false, false, false);
            if(statement) {
                loop->initializer = statement;
                return parseForLoopAfterInit(*this, allocator, loop);
            } else {
                unexpected_error("expected a var initialization in for loop");
                return fix_loop(this, loop, allocator);
            }
        }
        default: {
            auto& first_id = *token;
            if (first_id.type == TokenType::Identifier) {
                if ((token + 1)->type == TokenType::CommaSym) {
                    auto& second_id = *(token + 2);
                    if (second_id.type == TokenType::Identifier && (token + 3)->type == TokenType::InKw) {
                        token += 4;
                        return parseForInLoop(*this, allocator, first_id.value, second_id.value, tok, false, false);
                    }
                } else if ((token + 1)->type == TokenType::InKw) {
                    token += 2;
                    return parseForInLoop(*this, allocator, first_id.value, "", tok, false, false);
                }
            }
            auto loop = new (allocator.allocate<ForLoop>()) ForLoop(nullptr, nullptr, nullptr, parent_node, loc_single(tok));
            const auto assign = parseAssignmentStmt(allocator);
            if(assign) {
                loop->initializer = assign;
                return parseForLoopAfterInit(*this, allocator, loop);
            } else {
                unexpected_error("expected a variable initialization or assignment statement in for loop");
                return fix_loop(this, loop, allocator);
            }
        }
    }

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