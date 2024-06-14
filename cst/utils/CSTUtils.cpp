// Copyright (c) Qinetik 2024.

#include "CSTUtils.h"
#include "lexer/model/tokens/RefToken.h"
#include "integration/ide/model/ImportUnit.h"
#include "integration/ide/model/LexResult.h"

bool is_var_init_const(CompoundCSTToken* cst) {
    return str_token(cst->tokens[0].get()) == "const";
}

std::string param_name(CompoundCSTToken* param) {
    return str_token(param->tokens[is_char_op(param->tokens[0].get(), '&') ? 1 : 0].get());
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

CSTToken* get_linked_from_type(CSTToken* token) {
    switch(token->type()) {
        case LexTokenType::Type:
        case LexTokenType::Variable:
            if(token->as_ref()->linked) {
                return token->as_ref()->linked;
            } else {
                return token; // native types aren't linked
            }
        case LexTokenType::CompFunctionType:
            return get_linked_from_type(token->as_compound()->tokens[token->as_compound()->tokens.size() - 1].get());
        case LexTokenType::CompArrayType:
            return get_linked(token->as_compound()->tokens[0].get());
        default:
            return nullptr;
    }
}

CSTToken* get_linked_from_var_init(std::vector<std::unique_ptr<CSTToken>>& tokens) {
    if (is_char_op(tokens[2].get(), ':')) {
        return get_linked_from_type(tokens[3].get());
    } else if(is_char_op(tokens[2].get(), '=')) {
        return get_linked_from_value(tokens[3].get());
    } else {
        return nullptr;
    }
}

CSTToken* get_linked_from_typealias(std::vector<std::unique_ptr<CSTToken>>& tokens) {
    return get_linked(tokens[3].get());
}

CSTToken* get_linked_from_func(std::vector<std::unique_ptr<CSTToken>>& tokens) {
    unsigned i = 3;
    CSTToken* token;
    while(i < tokens.size()) {
        token = tokens[i].get();
        if(is_char_op(token, ')')) {
            break;
        }
        i++;
    }
    if(i + 1 < tokens.size() && is_char_op(tokens[i + 1].get(), ':')) {
        return get_linked(tokens[i + 2].get());
    }
    return nullptr;
}

CSTToken* get_linked_from_node(CSTToken* token) {
    switch(token->type()) {
        case LexTokenType::CompVarInit:
            return get_linked_from_var_init(token->as_compound()->tokens);
        case LexTokenType::CompTypealias:
            return get_linked_from_typealias(token->as_compound()->tokens);
        case LexTokenType::CompFunction:
            return get_linked_from_func(token->as_compound()->tokens);
        default:
            return nullptr;
    }
}

CSTToken* get_child_type(CSTToken* token) {
    switch(token->type()) {
        case LexTokenType::CompFunctionType:
            return get_linked_from_type(token->as_compound()->tokens[token->as_compound()->tokens.size() - 1].get());
        case LexTokenType::CompArrayType:
            return get_linked(token->as_compound()->tokens[0].get());
        default:
            return nullptr;
    }
}

CSTToken* link_child(CSTToken* parent, CSTToken* token) {
    switch(parent->type()) {
        case LexTokenType::CompEnumDecl:
            return find_identifier(parent->as_compound()->tokens, token->as_lex_token()->value, 3);
        case LexTokenType::CompStructDef:
        case LexTokenType::CompInterface:
             return find_func_or_var_init(parent->as_compound()->tokens, token->as_lex_token()->value, 3);
        case LexTokenType::CompVarInit: {
            auto linked = get_linked_from_var_init(parent->as_compound()->tokens);
            if (linked) {
                return link_child(linked, token);
            } else {
                return nullptr;
            }
        }
        case LexTokenType::CompTypealias: {
            auto linked = get_linked_from_typealias(parent->as_compound()->tokens);
            if(linked) {
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

token_with_parent get_token_at_position(CompoundCSTToken* container, std::vector<std::unique_ptr<CSTToken>>& tokens, const Position& position) {
    unsigned i = 0;
    CSTToken* token;
    while(i < tokens.size()) {
        token = tokens[i].get();
        if(token->compound() && token->as_compound()->tokens.size() == 1) {
            token = token->as_compound()->tokens[0].get();
        }
        if(token->compound()) {
            if(!position.is_behind(token->start_token()->position) && is_behind_end_of(position, token->end_token())) {
                return get_token_at_position(token->as_compound(), token->as_compound()->tokens, position);
            }
        } else if(!position.is_behind(token->as_lex_token()->position) && position.line == token->as_lex_token()->position.line && (position.character < (token->as_lex_token()->position.character + token->as_lex_token()->value.size()))) {
            return {container,i};
        }
        i++;
    }
    return {nullptr, -1};
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

LexResult* find_containing_file(ImportUnit* unit, CSTToken* token) {
    for(auto& file : unit->files) {
        auto result = get_token_at_position(file->tokens, token->start_token()->position);
        if(result && result->start_token() == token->start_token()) {
            return file.get();
        }
    }
    return nullptr;
}

token_parent_file find_token_parent(ImportUnit* unit, CSTToken* token) {
    token_with_parent result;
    CSTToken* found;
    for(auto& file : unit->files) {
        result = get_token_at_position(nullptr, file->tokens, token->start_token()->position);
        if(result.second != -1) {
            found = result.first ? result.first->tokens[result.second].get() : file->tokens[result.second].get();
            if (found->start_token() == token->start_token()) {
                return { file.get(), result };
            }
        }
    }
    return { nullptr, { nullptr, -1 } };
}

CSTToken* annotation_arg(unsigned index, CSTToken* token) {
    if(token->type() != LexTokenType::CompAnnotation) return nullptr;
    auto param_index = index + 2;
    if(index > 0) {
        param_index += index;
    }
    auto c = token->as_compound();
    if(param_index < c->tokens.size()) {
        return c->tokens[param_index].get();
    }
    return nullptr;
}

std::string annotation_str_arg(unsigned index, CSTToken* token) {
    auto got = annotation_arg(index, token);
    if(got && got->type() == LexTokenType::String) {
        return escaped_str_token(got);
    }
    return "";
}

std::optional<bool> annotation_bool_arg(unsigned index, CSTToken* token) {
    auto got = annotation_arg(index, token);
    if(got && got->type() == LexTokenType::Bool) {
        return escaped_str_token(got) == "true";
    }
    return std::nullopt;
}