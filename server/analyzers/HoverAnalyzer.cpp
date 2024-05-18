// Copyright (c) Qinetik 2024.

#include "HoverAnalyzer.h"
#include "cst/utils/CSTUtils.h"
#include "integration/ide/model/ImportUnit.h"
#include "integration/ide/model/LexResult.h"
#include "lexer/model/tokens/RefToken.h"

HoverAnalyzer::HoverAnalyzer(Position position) : position(position) {

}

struct FuncParam {
    std::string name;
    CSTToken* type;
};

struct FuncSignature {
    // function params, name's and type tokens
    std::vector<FuncParam> params;
    // if function signature has a given return type (user wrote it)
    CSTToken* returnType;
};

FuncSignature get_signature(CompoundCSTToken* func) {
    std::vector<FuncParam> params;
    unsigned i = 2;
    CSTToken* token;
    while(i < func->tokens.size()) {
        token = func->tokens[i].get();
        if(token->type() == LexTokenType::CompFunctionParam) {
            params.emplace_back(
                param_name(token->as_compound()),
                2 < token->as_compound()->tokens.size() ? token->as_compound()->tokens[2].get() : nullptr
            );
        } else if(is_char_op(token, ')')) {
            break;
        }
        i++;
    }
    CSTToken* ret = nullptr;
    if(is_char_op(func->tokens[i + 1].get(), ':')) {
        ret = func->tokens[i + 2].get();
    }
    return {std::move(params), ret};
}

std::string HoverAnalyzer::markdown_hover(ImportUnit* unit) {
    auto file = unit->files[unit->files.size() - 1];
    auto token = get_token_at_position(file->tokens, position);
    if(token) {
        if(token->is_ref()) {
            auto linked = token->as_ref()->linked;
            if (linked) {
                switch (linked->type()) {
                    case LexTokenType::CompFunction: {
                        value += "```typescript\n";
                        value += "function " + func_name(linked->as_compound());
                        auto signature = get_signature(linked->as_compound());
                        value += '(';
                        unsigned i = 0;
                        FuncParam* param;
                        while(i < signature.params.size()) {
                            param = &signature.params[i];
                            if(param->type) {
                                value += param->name;
                                value += " : ";
                                param->type->append_representation(value);
                            } else if(param->name == "self") {
                                value += "&self";
                            }
                            if(i != signature.params.size() - 1) {
                                value += ", ";
                            }
                            i++;
                        }
                        value += ')';
                        if (signature.returnType) {
                            value += " : ";
                            signature.returnType->append_representation(value);
                        }
                        value += "\n```";
                        break;
                    }
                    case LexTokenType::CompEnumDecl:
                        value += "```typescript\n";
                        value += "enum " + enum_name(linked->as_compound());
                        value += "\n```";
                        break;
                    case LexTokenType::CompStructDef:
                        value += "```c\n";
                        value += "struct " + struct_name(linked->as_compound());
                        value += "\n```";
                        break;
                    case LexTokenType::CompInterface:
                        value += "```typescript\n";
                        value += "interface " + interface_name(linked->as_compound());
                        value += "\n```";
                        break;
                    case LexTokenType::CompVarInit:
                        value += "```typescript\n";
                        value += "var " + var_init_identifier(linked->as_compound());
                        if(is_char_op(linked->as_compound()->tokens[2].get(), ':')) {
                            value += " : ";
                            linked->as_compound()->tokens[3]->append_representation(value);
                        } else {
                            // TODO get type by value
                        }
                        value += "\n```";
                        break;
                    default:
                        value += "don't know what it is";
                        break;
                }
            } else {
                value += "couldn't find the linked token !";
            }
        }
//        else {
//            value += "that don't look like a ref bro!";
//        }
    } else {
        value += "couldn't find the token at position";
    }
    return std::move(value);
}