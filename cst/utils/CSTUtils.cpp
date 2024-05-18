// Copyright (c) Qinetik 2024.

#include "CSTUtils.h"
#include "lexer/model/tokens/RefToken.h"
#include "integration/ide/model/ImportUnit.h"
#include "integration/ide/model/LexResult.h"

bool is_var_init_const(CompoundCSTToken* cst) {
    return str_token(cst->tokens[0].get()) == "const";
}

void visit(CSTVisitor* visitor, std::vector<std::unique_ptr<CSTToken>>& tokens, unsigned int start, unsigned int end) {
    while(start < end) {
        tokens[start]->accept(visitor);
        start++;
    }
}

CSTToken* find_identifier(std::vector<std::unique_ptr<CSTToken>>& tokens, const std::string& identifier, unsigned start) {
    CSTToken* token;
    while(start < tokens.size()) {
        token = tokens[start].get();
        if(token->type() == LexTokenType::Identifier && token->as_lex_token()->value == identifier) {
            return token;
        }
        start++;
    }
    return nullptr;
}

CSTToken* find_func_or_var_init(std::vector<std::unique_ptr<CSTToken>>& tokens, const std::string& name, unsigned start) {
    CSTToken* token;
    while(start < tokens.size()) {
        token = tokens[start].get();
        if(token->is_var_init() || token->is_func_decl()) {
            if(str_token(token->as_compound()->tokens[1].get()) == name) {
                return token;
            }
        }
        start++;
    }
    return nullptr;
}

CSTToken* get_linked(CSTToken* ref) {
    switch(ref->type()) {
        case LexTokenType::Variable:
        case LexTokenType::Type:
            return ((RefToken*) ref)->linked;
        default:
            return nullptr;
    }
}

CSTToken* get_linked_from_value(CSTToken* value) {
    switch(value->type()) {
        case LexTokenType::CompStructValue:
            return get_linked(value->as_compound()->tokens[0].get());
        default:
            return nullptr;
    }
}

CSTToken* get_linked_from_var_init(std::vector<std::unique_ptr<CSTToken>>& tokens) {
    if (is_char_op(tokens[2].get(), ':')) {
        return get_linked(tokens[3].get());
    } else if(is_char_op(tokens[2].get(), '=')) {
        return get_linked_from_value(tokens[3].get());
    } else {
        return nullptr;
    }
}

CSTToken* link_child(CSTToken* parent, CSTToken* token) {
    switch(parent->type()) {
        case LexTokenType::CompEnumDecl:
            if(token->type() != LexTokenType::Variable) {
                return nullptr;
            }
            ((RefToken*) token)->linked = find_identifier(parent->as_compound()->tokens, token->as_lex_token()->value, 3);
            return ((RefToken*) token)->linked;
        case LexTokenType::CompStructDef:
        case LexTokenType::CompInterface:
            if(token->type() != LexTokenType::Variable) {
                return nullptr;
            }
            return find_func_or_var_init(parent->as_compound()->tokens, token->as_lex_token()->value, 3);
        case LexTokenType::CompVarInit: {
            auto linked = get_linked_from_var_init(parent->as_compound()->tokens);
            if (linked) {
                return link_child(linked, token);
            } else {
                return nullptr;
            }
        }
        default:
            return nullptr;
    }
}

LexToken* get_token_at_position(std::vector<std::unique_ptr<CSTToken>>& tokens, const Position& position) {
    for(auto& token : tokens) {
        if(token->compound()) {
            if(!position.is_behind(token->start_token()->position) && !position.is_ahead(token->end_token()->position)) {
                return get_token_at_position(token->as_compound()->tokens, position);
            }
        } else if(!position.is_behind(token->as_lex_token()->position) && position.line == token->as_lex_token()->position.line && (position.character < (token->as_lex_token()->position.character + token->as_lex_token()->value.size()))) {
            return token->as_lex_token();
        }
    }
    return nullptr;
}

LexResult* find_container(ImportUnit* unit, CSTToken* token) {
    for(auto& file : unit->files) {
        auto result = get_token_at_position(file->tokens, token->start_token()->position);
        if(result && result->start_token() == token->start_token()) {
            return file.get();
        }
    }
    return nullptr;
}