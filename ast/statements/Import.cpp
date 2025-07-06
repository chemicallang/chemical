// Copyright (c) Chemical Language Foundation 2025.

#include "Import.h"
#include <filesystem>
#include "compiler/SymbolResolver.h"
#include "preprocess/ImportPathHandler.h"
#include "ast/structures/ModuleScope.h"
#include "ast/base/LocatedIdentifier.h"
#include "compiler/symres/NodeSymbolDeclarer.h"
#include "utils/PathUtils.h"

ASTNode* ImportStatement::child(const chem::string_view &name) {
    if(symbols) {
        auto found = symbols->find(name);
        return found != symbols->end() ? found->second : nullptr;
    } else {
#ifdef DEBUG
        throw std::runtime_error("symbols pointer doesn't exist in import statement");
#endif
        return nullptr;
    }
}