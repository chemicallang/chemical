// Copyright (c) Qinetik 2024.

#include "CSTUtils.h"
#include "integration/cbi/model/LexImportUnit.h"
#include "integration/cbi/model/LexResult.h"

unsigned int func_name_index(CSTToken* function) {
    unsigned i = 1;

    auto spec = specifier_token(function->tokens[0]);
    if(spec.has_value()) {
        // set at name, extension or generic params list
        i += 1;
    }

    const auto& gen_token = function->tokens[i];
    const auto is_generic = gen_token->type() == LexTokenType::CompGenericParamsList;

    if(is_generic) {
        i += 1;
    }

    const auto extension_start = i;
    const auto is_extension = is_char_op(function->tokens[i], '(');

    if(is_extension) {
        i += 3;
    }

    return i;
}

bool is_var_init_const(CSTToken* cst) {
    return str_token(cst->tokens[specifier_index(cst->tokens[0], 0)]) == "const";
}

std::optional<AccessSpecifier> specifier_token(CSTToken* token) {
    if(token->type() == LexTokenType::Keyword) {
        if(token->value() == "public") {
            return AccessSpecifier::Public;
        } else if(token->value() == "private") {
            return AccessSpecifier::Private;
        } else if(token->value() == "internal") {
            return AccessSpecifier::Internal;
        } else if(token->value() == "protected") {
            return AccessSpecifier::Protected;
        }
    }
    return std::nullopt;
}

const std::string& param_name(CSTToken* param) {
    return str_token(param->tokens[is_char_op(param->tokens[0], '&') ? 1 : 0]);
}

void visit(CSTVisitor* visitor, std::vector<CSTToken*>& tokens, unsigned int start, unsigned int end) {
    while(start < end) {
        tokens[start]->accept(visitor);
        start++;
    }
}

CSTToken* find_identifier(std::vector<CSTToken*>& tokens, const std::string& identifier, unsigned start) {
    CSTToken* token;
    while(start < tokens.size()) {
        token = tokens[start];
        if(token->type() == LexTokenType::Identifier && token->as_lex_token()->value() == identifier) {
            return token;
        }
        start++;
    }
    return nullptr;
}

CSTToken* find_func_or_var_init(std::vector<CSTToken*>& tokens, const std::string& name, unsigned start) {
    CSTToken* token;
    while(start < tokens.size()) {
        token = tokens[start];
        if(token->is_var_init() || token->is_func_decl()) {
            if(str_token(token->tokens[1]) == name) {
                return token;
            }
        }
        start++;
    }
    return nullptr;
}

// checks that the given position is behind the given token's end
bool is_behind_end_of(const Position& position, CSTToken* token) {
    return position.line < token->position().line || (position.line == token->position().line && (position.character < (token->position().character + token->value().size())));
}

token_with_parent get_token_at_position(CSTToken* container, std::vector<CSTToken*>& tokens, const Position& position) {
    unsigned i = 0;
    CSTToken* token;
    while(i < tokens.size()) {
        token = tokens[i];
        if(token->compound() && token->tokens.size() == 1) {
            token = token->tokens[0];
        }
        if(token->compound()) {
            if(!position.is_behind(token->start_token()->position()) && is_behind_end_of(position, token->end_token())) {
                return get_token_at_position(token, token->tokens, position);
            }
        } else if(!position.is_behind(token->position()) && position.line == token->position().line && (position.character < (token->as_lex_token()->position().character + token->as_lex_token()->value().size()))) {
            return {container,i};
        }
        i++;
    }
    return {nullptr, -1};
}

CSTToken* get_token_at_position(std::vector<CSTToken*>& tokens, const Position& position) {
    unsigned i = 0;
    CSTToken* token;
    while(i < tokens.size()) {
        token = tokens[i];
        if(token->compound() && token->tokens.size() == 1) {
            token = token->tokens[0];
        }
        if(token->compound()) {
            if(!position.is_behind(token->start_token()->position()) && is_behind_end_of(position, token->end_token())) {
                return get_token_at_position(token->tokens, position);
            }
        } else if(!position.is_behind(token->position()) && position.line == token->position().line && (position.character < (token->position().character + token->value().size()))) {
            return token;
        }
        i++;
    }
    return nullptr;
}

LexResult* find_containing_file(LexImportUnit* unit, CSTToken* token) {
    for(auto& file : unit->files) {
        auto result = get_token_at_position(file->unit.tokens, token->start_token()->position());
        if(result && result->start_token() == token->start_token()) {
            return file.get();
        }
    }
    return nullptr;
}

token_parent_file find_token_parent(LexImportUnit* unit, CSTToken* token) {
    token_with_parent result;
    CSTToken* found;
    for(auto& file : unit->files) {
        result = get_token_at_position(nullptr, file->unit.tokens, token->start_token()->position());
        if(result.second != -1) {
            found = result.first ? result.first->tokens[result.second] : file->unit.tokens[result.second];
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
        return c->tokens[param_index];
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