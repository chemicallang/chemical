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
            ((RefToken*) token)->linked =  find_func_or_var_init(parent->as_compound()->tokens, token->as_lex_token()->value, 3);
            return ((RefToken*) token)->linked;
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

// checks that the given position is behind the given token's end
bool is_behind_end_of(const Position& position, LexToken* token) {
    return position.line < token->position.line || (position.line == token->position.line && (position.character < (token->position.character + token->value.size())));
}

LexToken* get_token_at_position(std::vector<std::unique_ptr<CSTToken>>& tokens, const Position& position) {
    unsigned i = 0;
    CSTToken* token;
    while(i < tokens.size()) {
        token = tokens[i].get();
        if(token->compound() && token->as_compound()->tokens.size() == 1) {
            token = token->as_compound()->tokens[0].get();
        }
        if(token->compound()) {
            if(!position.is_behind(token->start_token()->position) && is_behind_end_of(position, token->end_token())) {
                return get_token_at_position(token->as_compound()->tokens, position);
            }
        } else if(!position.is_behind(token->as_lex_token()->position) && position.line == token->as_lex_token()->position.line && (position.character < (token->as_lex_token()->position.character + token->as_lex_token()->value.size()))) {
            return token->as_lex_token();
        }
        i++;
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