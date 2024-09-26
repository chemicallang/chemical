// Copyright (c) Qinetik 2024.

#include "DocumentSymbolsAnalyzer.h"
#include "cst/utils/CSTUtils.h"

// TODO put nested symbols
// TODO put small detail about function
// TODO investigate why double clicking selects the wrong range, is it correct behavior
void DocumentSymbolsAnalyzer::put(const std::string &name, lsSymbolKind kind, lsRange range, lsRange selRange) {
    symbols.emplace_back(
            name,
            kind,
            range,
            selRange
    );
}

lsRange range(CSTToken *token) {
    auto &start = token->start_token()->position();
    auto end = token->end_token();
    return lsRange{
            lsPosition{
                    static_cast<int>(start.line),
                    static_cast<int>(start.character)
            },
            lsPosition{
                    static_cast<int>(end->position().line),
                    static_cast<int>(end->position().character + end->value().size())
            }
    };
}

void DocumentSymbolsAnalyzer::visitCompoundCommon(CSTToken* cst) {

}

void DocumentSymbolsAnalyzer::visitFunction(CSTToken* function) {
    put(func_name(function), lsSymbolKind::Function, range(function), range(func_name_tok(function)));
}

void DocumentSymbolsAnalyzer::visitStructDef(CSTToken* structDef) {
    put(struct_name(structDef), lsSymbolKind::Struct, range(structDef), range(struct_name_tok(structDef)));
}

void DocumentSymbolsAnalyzer::visitInterface(CSTToken* interface) {
    put(interface_name(interface), lsSymbolKind::Interface, range(interface), range(interface_name_tok(interface)));
}

void DocumentSymbolsAnalyzer::visitTypealias(CSTToken* alias) {
    put(typealias_name(alias), lsSymbolKind::TypeAlias, range(alias), range(typealias_name_tok(alias)));
}

void DocumentSymbolsAnalyzer::visitEnumDecl(CSTToken* enumDecl) {
    put(enum_name(enumDecl), lsSymbolKind::Enum, range(enumDecl), range(enum_name_tok(enumDecl)));
}

void DocumentSymbolsAnalyzer::visitVarInit(CSTToken* varInit) {
    put(var_init_identifier(varInit), lsSymbolKind::Variable, range(varInit), range(var_init_name_tok(varInit)));
}

void DocumentSymbolsAnalyzer::analyze(std::vector<CSTToken*> &tokens) {
    ::visit(this, tokens);
}