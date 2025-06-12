// Copyright (c) Chemical Language Foundation 2025.

#include "server/model/ASTResult.h"
#include "ast/values/FunctionCall.h"
#include "ast/types/FunctionType.h"
#include "ast/structures/FunctionParam.h"
#include "ast/structures/StructDefinition.h"
#include "ast/statements/VarInit.h"
#include "server/utils/AnalyzerUtils.h"
#include "SignatureHelpAnalyzer.h"
#include "core/source/LocationManager.h"
#include "server/analyzers/Documentation.h"
#include <iostream>

#ifdef DEBUG
#define DEBUG_SIG_HELP
#endif

SignatureHelpAnalyzer::SignatureHelpAnalyzer(LocationManager& loc_man, Position position) : loc_man(loc_man), position(position) {

}

std::string func_type_label(FunctionType* func_type) {
    const auto decl = func_type->as_function();
    if(decl) {
        return decl->name_view().str();
    }
    return "";
}

std::optional<std::string> func_type_documentation(FunctionType* func_type) {
    const auto decl = func_type->as_function();
    if(decl) {
        std::string out;
        small_detail_of(out, decl);
        return std::move(out);
    }
    return std::nullopt;
}

std::string parameter_doc(FunctionType* func_type, FunctionParam* func_param) {
//    std::string value;
//    value += "`";
//    value += func_param->type->representation();
//    value += "`";
//    return value;
    return "";
}

void SignatureHelpAnalyzer::analyze(LabModule* module, ModuleData* modData, LexResult* lexResult, ASTUnit* unit) {
    if(!lexResult) {
#ifdef DEBUG_SIG_HELP
        std::cerr << "no lex result given in signature help" << std::endl;
#endif
        return;
    }

    auto token = get_token_at_position(lexResult->tokens, position);
    if(!token) {
#ifdef DEBUG_SIG_HELP
        std::cerr << "couldn't get token at position in signature help" << std::endl;
#endif
        return;
    }

    auto absolute_start = lexResult->tokens.data();

    // find opening parenthesis to left, for which we do not encounter the right parenthesis
    // my_call((something_here((not_where()))), something ((())))

    // the token present before the '(' like printf() <-- here its 'printf'
    Token* found_call_id_token = nullptr;

    auto current = token;
    auto ignore_start = 0;
    while(current != absolute_start) {
        switch(current->type) {
            case TokenType::LParen:
                if(ignore_start > 0) {
                    ignore_start--;
                } else {
                    found_call_id_token = current - 1;
                    goto loop_end;
                }
                break;
            case TokenType::RParen:
                ignore_start++;
                break;
            default:
                current--;
                continue;
        }
        current--;
    }
    loop_end:

    if(found_call_id_token == nullptr) {
#ifdef DEBUG_SIG_HELP
        std::cerr << "couldn't find call identifier token in signature help" << std::endl;
#endif
        return;
    }

    if(found_call_id_token->linked == nullptr) {
#ifdef DEBUG_SIG_HELP
        std::cerr << "call identifier token not linked with anything in signature help" << std::endl;
#endif
        return;
    }

    const auto linked_node = found_call_id_token->linked->get_ref_linked_node();
    if(!linked_node) {
#ifdef DEBUG_SIG_HELP
        std::cerr << "call identifier token references no node in signature help" << std::endl;
#endif
        return;
    }

    const auto known_type = linked_node->known_type();
    if(!known_type) {
#ifdef DEBUG_SIG_HELP
        std::cerr << "linked node gave no type in signature help" << std::endl;
#endif
        return;
    }

    const auto func_type = known_type->canonical()->as_function_type();
    if(!func_type) {
#ifdef DEBUG_SIG_HELP
        std::cerr << "linked type is not a function type in signature help" << std::endl;
#endif
        return;
    }

    // tracking active parameter
    std::optional<unsigned int> active_parameter = std::nullopt;

    // creating parameter information
    std::vector<lsp::ParameterInformation> parameterInfo;
    for(const auto param : func_type->params) {
        const auto location = param->encoded_location();
        auto loc_pos = loc_man.getLocationPos(location);
        if(position.line == loc_pos.start.line && position.character >= loc_pos.start.character) {
            active_parameter = param->index;
        }
        parameterInfo.emplace_back(lsp::ParameterInformation(
            param->name.str(),
            parameter_doc(func_type, param)
        ));
    }

    help.signatures.emplace_back(lsp::SignatureInformation(
        func_type_label(func_type), func_type_documentation(func_type), std::move(parameterInfo), active_parameter
    ));

}