// Copyright (c) Chemical Language Foundation 2025.

#include "DocumentSymbolsAnalyzer.h"
#include "cst/LocationManager.h"
#include "ast/structures/FunctionDeclaration.h"
#include "ast/structures/StructDefinition.h"
#include "ast/structures/InterfaceDefinition.h"
#include "ast/structures/UnionDef.h"
#include "ast/structures/VariantDefinition.h"
#include "ast/statements/Typealias.h"
#include "ast/statements/VarInit.h"
#include "ast/structures/EnumDeclaration.h"

// TODO put nested symbols
// TODO put small detail about function
// TODO investigate why double clicking selects the wrong range, is it correct behavior
void DocumentSymbolsAnalyzer::put(const chem::string_view &name, lsp::SymbolKind kind, lsp::Range range, lsp::Range selRange) {
    symbols.emplace_back(
            name.str(),
            kind,
            range,
            selRange
    );
}

void DocumentSymbolsAnalyzer::put(const chem::string_view& name, lsp::SymbolKind kind, lsp::Range range) {
    symbols.emplace_back(
            name.str(),
            kind,
            range,
            lsp::Range { range.start, { range.start.line, static_cast<lsp::uint>(range.start.character + name.size()) } }
    );
}

void DocumentSymbolsAnalyzer::put(const chem::string_view& name, lsp::SymbolKind kind, SourceLocation location) {
    const auto pos = loc_man.getLocationPos(location);
    auto range = lsp::Range{
            lsp::Position {
                    pos.start.line,
                    pos.start.character
            },
            lsp::Position {
                    pos.start.line,
                    static_cast<lsp::uint>(pos.start.character + name.size())
            }
    };
    symbols.emplace_back(
            name.str(),
            kind,
            range,
            range
    );
}

lsp::Range DocumentSymbolsAnalyzer::range(SourceLocation location) {
    const auto pos = loc_man.getLocationPos(location);
    return lsp::Range{
            lsp::Position {
                    pos.start.line,
                    pos.start.character
            },
            lsp::Position {
                    pos.end.line,
                    pos.end.character
            }
    };
}

void DocumentSymbolsAnalyzer::VisitFunctionDecl(FunctionDeclaration *decl) {
    if(!decl->name_view().empty()) {
        put(decl->name_view(), lsp::SymbolKind::Function, decl->encoded_location());
    }
}

void DocumentSymbolsAnalyzer::VisitStructDecl(StructDefinition *def) {
    put(def->name_view(), lsp::SymbolKind::Struct, def->encoded_location());
}

void DocumentSymbolsAnalyzer::VisitUnionDecl(UnionDef *def) {
    put(def->name_view(), lsp::SymbolKind::Struct, def->encoded_location());
}

void DocumentSymbolsAnalyzer::VisitVariantDecl(VariantDefinition *def) {
    put(def->name_view(), lsp::SymbolKind::Struct, def->encoded_location());
}

void DocumentSymbolsAnalyzer::VisitInterfaceDecl(InterfaceDefinition *def) {
    put(def->name_view(), lsp::SymbolKind::Interface, def->encoded_location());
}

void DocumentSymbolsAnalyzer::VisitTypealiasStmt(TypealiasStatement *def) {
    put(def->name_view(), lsp::SymbolKind::Interface, def->encoded_location());
}

void DocumentSymbolsAnalyzer::VisitEnumDecl(EnumDeclaration *def) {
    put(def->name_view(), lsp::SymbolKind::Enum, def->encoded_location());
}

void DocumentSymbolsAnalyzer::VisitVarInitStmt(VarInitStatement *init) {
    put(init->name_view(), lsp::SymbolKind::Enum, init->encoded_location());
}