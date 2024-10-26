// Copyright (c) Qinetik 2024.

#include "DocumentSymbolsAnalyzer.h"
#include "cst/utils/CSTUtils.h"
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
void DocumentSymbolsAnalyzer::put(const std::string &name, lsSymbolKind kind, lsRange range, lsRange selRange) {
    symbols.emplace_back(
            name,
            kind,
            range,
            selRange
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

void DocumentSymbolsAnalyzer::visit(FunctionDeclaration *decl) {
    put(decl->name(), lsSymbolKind::Function, range(decl->location), range(decl->identifier.location));
}

void DocumentSymbolsAnalyzer::visit(StructDefinition *def) {
    put(def->name(), lsSymbolKind::Struct, range(def->location), range(def->identifier.location));
}

void DocumentSymbolsAnalyzer::visit(UnionDef *def) {
    put(def->name(), lsSymbolKind::Struct, range(def->location), range(def->identifier.location));
}

void DocumentSymbolsAnalyzer::visit(VariantDefinition *def) {
    put(def->name(), lsSymbolKind::Struct, range(def->location), range(def->identifier.location));
}

void DocumentSymbolsAnalyzer::visit(InterfaceDefinition *def) {
    put(def->name(), lsSymbolKind::Interface, range(def->location), range(def->identifier.location));
}

void DocumentSymbolsAnalyzer::visit(TypealiasStatement *def) {
    put(def->name(), lsSymbolKind::Interface, range(def->location), range(def->identifier.location));
}

void DocumentSymbolsAnalyzer::visit(EnumDeclaration *def) {
    put(def->name, lsSymbolKind::Enum, range(def->location), range(enum_name_tok(enumDecl)));
}

void DocumentSymbolsAnalyzer::visit(VarInitStatement *init) {
    put(init->identifier, lsSymbolKind::Enum, range(init->location), range(enum_name_tok(enumDecl)));
}