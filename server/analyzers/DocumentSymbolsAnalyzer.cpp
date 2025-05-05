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
void DocumentSymbolsAnalyzer::put(const chem::string_view &name, lsSymbolKind kind, lsRange range, lsRange selRange) {
    symbols.emplace_back(
            name.str(),
            kind,
            range,
            selRange
    );
}

void DocumentSymbolsAnalyzer::put(const chem::string_view& name, lsSymbolKind kind, lsRange range) {
    symbols.emplace_back(
            name.str(),
            kind,
            range,
            lsRange { range.start, { static_cast<int>(range.start.line), static_cast<int>(range.start.character + name.size()) } }
    );
}

lsRange DocumentSymbolsAnalyzer::range(SourceLocation location) {
    const auto pos = loc_man.getLocationPos(location);
    return lsRange{
            lsPosition{
                    static_cast<int>(pos.start.line),
                    static_cast<int>(pos.start.character)
            },
            lsPosition{
                    static_cast<int>(pos.end.line),
                    static_cast<int>(pos.end.character)
            }
    };
}

void DocumentSymbolsAnalyzer::VisitFunctionDecl(FunctionDeclaration *decl) {
    put(decl->name_view(), lsSymbolKind::Function, range(decl->encoded_location()));
}

void DocumentSymbolsAnalyzer::VisitStructDecl(StructDefinition *def) {
    put(def->name_view(), lsSymbolKind::Struct, range(def->encoded_location()));
}

void DocumentSymbolsAnalyzer::VisitUnionDecl(UnionDef *def) {
    put(def->name_view(), lsSymbolKind::Struct, range(def->encoded_location()));
}

void DocumentSymbolsAnalyzer::VisitVariantDecl(VariantDefinition *def) {
    put(def->name_view(), lsSymbolKind::Struct, range(def->encoded_location()));
}

void DocumentSymbolsAnalyzer::VisitInterfaceDecl(InterfaceDefinition *def) {
    put(def->name_view(), lsSymbolKind::Interface, range(def->encoded_location()));
}

void DocumentSymbolsAnalyzer::VisitTypealiasStmt(TypealiasStatement *def) {
    put(def->name_view(), lsSymbolKind::Interface, range(def->encoded_location()));
}

void DocumentSymbolsAnalyzer::VisitEnumDecl(EnumDeclaration *def) {
    put(def->name_view(), lsSymbolKind::Enum, range(def->encoded_location()));
}

void DocumentSymbolsAnalyzer::VisitVarInitStmt(VarInitStatement *init) {
    put(init->name_view(), lsSymbolKind::Enum, range(init->encoded_location()));
}