// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 11/03/2024.
//

#include "CompletionItemAnalyzer.h"
#include <unordered_set>
#include "cst/base/CSTToken.h"
#include "cst/utils/CSTUtils.h"
#include "integration/cbi/model/LexImportUnit.h"
#include "integration/cbi/model/LexResult.h"
#include "Documentation.h"

#define DEBUG_COMPLETION true

void CompletionItemAnalyzer::put(const std::string &label, lsCompletionItemKind kind) {
    list.items.emplace_back(label, kind);
}

void CompletionItemAnalyzer::put_with_md_doc(const std::string& label, lsCompletionItemKind kind, const std::string& detail, const std::string& doc) {
    list.items.emplace_back(label, kind, detail, std::pair(std::nullopt, MarkupContent{ "markdown", doc }));
}

bool CompletionItemAnalyzer::is_eq_caret(CSTToken* token) const {
    return is_eq_caret(token->position());
}

bool CompletionItemAnalyzer::is_ahead(CSTToken *token) const {
    return is_ahead(token->position());
}

bool CompletionItemAnalyzer::is_caret_inside(CSTToken *token) {
    return is_behind(token->start_token()->position()) && !is_behind(token->end_token()->position());
}

CSTToken* CompletionItemAnalyzer::child_container(CSTToken* compound) {
    for (auto &token: compound->tokens) {
        if (token->compound() && is_caret_inside(token)) {
            return token;
        }
    }
    return nullptr;
}

CSTToken* CompletionItemAnalyzer::direct_parent(std::vector<CSTToken*> &tokens) {
    CSTToken* child;
    CSTToken* nested;
    for (auto &token: tokens) {
        if (token->compound() && is_caret_inside(token)) {
            child = (CSTToken* ) token;
            while (true) {
                nested = child_container(child);
                if (nested == nullptr) {
                    return child;
                } else {
                    child = nested;
                }
            }
        }
    }
    return nullptr;
}

CSTToken* last_direct_parent(CSTToken* token) {
    if(token->compound()) {
        auto last = token->tokens[token->tokens.size() - 1];
        if(last->compound()) {
            return last_direct_parent(last);
        } else {
            return token;
        }
    } else {
        return token;
    }
}

CSTToken* CompletionItemAnalyzer::token_before_caret(std::vector<CSTToken*> &tokens) {
    int i = 0;
    while (i < tokens.size()) {
        if (is_caret_eq_or_behind(tokens[i]->start_token())) {
            return last_direct_parent(tokens[i - 1]);
        }
        i++;
    }
    return nullptr;
}

void
CompletionItemAnalyzer::visit(std::vector<CSTToken*> &tokens, unsigned int start, unsigned int end) {
    CSTToken *token;
    while (start < end) {
        token = tokens[start];
        if (is_ahead(token->start_token()->position())) {
            break;
        } else {
            token->accept(this);
        }
        start++;
    }
}

void CompletionItemAnalyzer::visitBody(CSTToken* bodyCst) {
    if (is_caret_inside(bodyCst)) {
        visit(bodyCst->tokens);
    }
}

void CompletionItemAnalyzer::visitVarInit(CSTToken* varInit) {
    put(str_token(varInit->tokens[1]), lsCompletionItemKind::Variable);
    if (3 < varInit->tokens.size() && is_caret_inside(varInit->tokens[3])) {
        varInit->tokens[3]->accept(this);
    }
}

void CompletionItemAnalyzer::visitAssignment(CSTToken* cst) {
    if (is_caret_inside(cst->tokens[2])) {
        cst->tokens[2]->accept(this);
    }
}

void CompletionItemAnalyzer::visitFunction(CSTToken* function) {
    put(str_token(function->tokens[1]), lsCompletionItemKind::Function);
    function->tokens[function->tokens.size() - 1]->accept(this);
}

void CompletionItemAnalyzer::visitEnumDecl(CSTToken* cst) {
    if (is_caret_ahead(cst->tokens[cst->tokens.size() - 1]->end_token())) {
        put(str_token(cst->tokens[1]), lsCompletionItemKind::Enum);
    }
}

void CompletionItemAnalyzer::visitStructDef(CSTToken* cst) {
    auto has_override = is_char_op(cst->tokens[3], ':');
    auto l_brace = has_override ? 4 : 2;
    if (is_caret_ahead(cst->tokens[l_brace]->end_token())) {
        put(str_token(cst->tokens[1]), lsCompletionItemKind::Struct);
        if (is_caret_behind(cst->tokens[cst->tokens.size() - 1]->end_token())) {
            visit(cst->tokens, l_brace + 1);
        }
    }
}

void CompletionItemAnalyzer::visitInterface(CSTToken* cst) {
    if (is_caret_ahead(cst->tokens[2]->start_token())) {
        put(str_token(cst->tokens[1]), lsCompletionItemKind::Interface);
        if (is_caret_behind(cst->tokens[cst->tokens.size() - 1]->end_token())) {
            visit(cst->tokens, 3);
        }
    }
}

void CompletionItemAnalyzer::visitImpl(CSTToken* cst) {
    if (is_caret_ahead(cst->tokens[2]->start_token()) &&
        is_caret_behind(cst->tokens[cst->tokens.size() - 1]->end_token())) {
        visit(cst->tokens, 3);
    }
}

void CompletionItemAnalyzer::visitIf(CSTToken* ifCst) {
    visit(ifCst->tokens);
}

void CompletionItemAnalyzer::visitWhile(CSTToken* whileCst) {
    visit(whileCst->tokens);
}

void CompletionItemAnalyzer::visitDoWhile(CSTToken* doWhileCst) {
    visit(doWhileCst->tokens);
}

void CompletionItemAnalyzer::visitForLoop(CSTToken* forLoop) {
    if (is_caret_inside(forLoop->tokens[8])) {
        forLoop->tokens[2]->accept(this);
        forLoop->tokens[8]->accept(this);
    }
}

void CompletionItemAnalyzer::visitLambda(CSTToken* cst) {
    cst->tokens[cst->tokens.size() - 1]->accept(this);
}

void CompletionItemAnalyzer::visitStructValue(CSTToken* cst) {
    if (is_caret_ahead(cst->tokens[2]->start_token()) &&
        is_caret_behind(cst->tokens[cst->tokens.size() - 1]->end_token())) {
        visit(cst->tokens, 3, cst->tokens.size());
    }
}

void CompletionItemAnalyzer::visitArrayValue(CSTToken* arrayValue) {
    if (is_caret_inside(arrayValue)) {
        visit(arrayValue->tokens);
    }
}

void CompletionItemAnalyzer::visitSwitch(CSTToken* switchCst) {

}

void CompletionItemAnalyzer::visitMultilineComment(CSTToken *token) {

}

CompletionList CompletionItemAnalyzer::analyze(std::vector<CSTToken*> &tokens) {
    auto chain = chain_before_caret(tokens);
    if(chain) {
        if(handle_chain_before_caret(chain)) {
            return list;
        } else {
            std::cout << "[Unknown] member access into access chain : " + chain->type_string() << std::endl;
        }
    }
    visit(tokens);
//#if defined DEBUG_COMPLETION && DEBUG_COMPLETION
//    for(const auto & item : items) {
//        std::cout << item.label << std::endl;
//    }
//#endif
    return std::move(list);
}

CSTToken* CompletionItemAnalyzer::chain_before_caret(std::vector<CSTToken*> &tokens) {
    auto parent = direct_parent(tokens);
    if (parent == nullptr) {
#if defined DEBUG_COMPLETION && DEBUG_COMPLETION
        std::cout << "Couldn't find direct parent" << std::endl;
#endif
        return nullptr;
    } else {
        auto token = token_before_caret(parent->tokens);
        if (token) {
#if defined DEBUG_COMPLETION && DEBUG_COMPLETION
            std::cout << "token before index : " + token->representation() << " type " << token->type_string() << " parent type " << parent->type_string() << std::endl;
#endif
            if(token->type() == LexTokenType::CompAccessChain) {
                return (CSTToken*) token;
            }
            return nullptr;
        } else {
#if defined DEBUG_COMPLETION && DEBUG_COMPLETION
            std::cout << "no token before the caret position" << std::endl;
#endif
            return nullptr;
        }
    }
}

void CompletionItemAnalyzer::put_identifiers(std::vector<CSTToken*>& tokens, unsigned int start) {
    CSTToken* token;
    while(start < tokens.size()) {
        token = tokens[start];
        if(token->type() == LexTokenType::Identifier) {
            put(token->value(), lsCompletionItemKind::EnumMember);
        }
        start++;
    }
}

void put_with_doc(CompletionItemAnalyzer* analyzer, const std::string& label, lsCompletionItemKind kind, CSTToken* token, CSTToken* parent) {
    std::string doc;
    markdown_documentation_old(doc, analyzer->current_file, nullptr, parent, token);
    std::string detail;
    small_detail_of(detail, token);
    analyzer->put_with_md_doc(label, kind, detail, doc);
}

void put_function_with_doc(CompletionItemAnalyzer* analyzer, CSTToken* token, CSTToken* parent) {
    put_with_doc(analyzer, str_token(((CSTToken*) token)->tokens[1]), lsCompletionItemKind::Function, token, parent);
}

void put_var_init_with_doc(CompletionItemAnalyzer* analyzer, CSTToken* token, CSTToken* parent) {
    put_with_doc(analyzer, str_token(((CSTToken*) token)->tokens[1]), lsCompletionItemKind::Variable, token, parent);
}

void collect_struct_functions(
        CompletionItemAnalyzer* analyzer,
        CSTToken* parent,
        unsigned i
) {
    CSTToken* token;
    while(i < parent->tokens.size()) {
        token = parent->tokens[i];
        if(token->is_func_decl()) {
            // TODO collect function if it doesn't have a self | this member
            put_function_with_doc(analyzer, token, parent);
        }
        i++;
    }
}

void collect_struct_members(
        CompletionItemAnalyzer* analyzer,
        CSTToken* parent,
        unsigned i
) {
    CSTToken* token;
    while(i < parent->tokens.size()) {
        token = parent->tokens[i];
        if(token->is_func_decl()) {
            put_function_with_doc(analyzer, token, parent);
        } else if(token->is_var_init()) {
            put_var_init_with_doc(analyzer, token, parent);
        }
        i++;
    }
}

bool put_children(CompletionItemAnalyzer* analyzer, CSTToken* parent, bool put_values = false) {
    switch(parent->type()) {
        case LexTokenType::CompEnumDecl:
            analyzer->put_identifiers(parent->tokens, 2);
            return true;
        case LexTokenType::CompStructDef:
        case LexTokenType::CompInterface:
            (put_values ? (collect_struct_members) : (collect_struct_functions))(
                    analyzer,
                    parent,
                    (is_char_op(parent->tokens[2], ':')) ? 5 : 3
            );
            return true;
        case LexTokenType::CompVarInit: {
            auto linked = get_linked_from_var_init(parent->tokens);
            if(linked) {
                return put_children(analyzer, linked, true);
            } else {
                return false;
            }
        }
        case LexTokenType::CompTypealias: {
            auto linked = get_linked_from_typealias(parent->tokens);
            if(linked) {
                return put_children(analyzer, linked, put_values);
            } else {
                return false;
            }
        }
        default:
            return false;
    }
}

bool put_children_of_ref(CompletionItemAnalyzer* analyzer, CSTToken* chain) {
    auto parent = chain->tokens[chain->tokens.size() - 2];
    switch(parent->type()) {
        case LexTokenType::Variable:
        case LexTokenType::Type:{
            if(!parent->linked) return false;
            return put_children(analyzer, parent->linked);
        }
        case LexTokenType::CompIndexOp:{
            auto grandpa = chain->tokens[chain->tokens.size() - 3];
            auto linked = get_linked_from_node(grandpa->linked);
            if(!linked) return false;
            return put_children(analyzer, linked, true);
        }
        case LexTokenType::CompFunctionCall:{
            auto grandpa = chain->tokens[chain->tokens.size() - 3];
            auto linked = get_linked_from_node(grandpa->linked);
            if(!linked) return false;
            return put_children(analyzer, linked, true);
        }
        default:
            return false;
    }
}

bool CompletionItemAnalyzer::handle_chain_before_caret(CSTToken* chain) {
    if(!chain->tokens.empty() && is_char_op(chain->tokens[chain->tokens.size() - 1], '.')) {
        return put_children_of_ref(this, chain);
    }
    return false;
}

CompletionList CompletionItemAnalyzer::analyze(LexImportUnit* unit) {

    if(unit->files.size() == 1) return analyze(unit->files[0]->unit.tokens);
    if(unit->files.empty()) return list;

    // check is caret position before a chain
    auto chain = chain_before_caret(unit->files[unit->files.size() - 1]->unit.tokens);
    if(chain) {
        if(handle_chain_before_caret(chain)) {
            return list;
        } else {
            std::cout << "[Unknown] member access into access chain : " + chain->type_string() << std::endl;
        }
    }

    auto prev_caret_position = caret_position;
    unsigned i = 0;
    auto size = unit->files.size();
    while(i < size) {
        auto& file = unit->files[i];
        if(i == size - 1) { // last file
            caret_position = prev_caret_position;
        } else {
            if(!file->unit.tokens.empty()) { // not last file
                // set caret position at the end of file, so all tokens are analyzed
                auto& pos = file->unit.tokens[file->unit.tokens.size() - 1]->end_token()->position();
                caret_position = {pos.line + 2, 0};
            } else {
                i++;
                continue;
            }
        }
        current_file = file.get();
        visit(file->unit.tokens);
        i++;
    }
    return list;
}