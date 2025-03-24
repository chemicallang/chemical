// Copyright (c) Chemical Language Foundation 2025.

#include "Import.h"
#include <filesystem>
#include "compiler/SymbolResolver.h"
#include "preprocess/ImportPathHandler.h"
#include "ast/base/GlobalInterpretScope.h"
#include "ast/structures/FileScope.h"

struct FileSymbolIterator {
    int index;
    const SymbolEntry* start;
    const SymbolEntry* end;
};

FileSymbolIterator get_iterator(SymbolResolver& linker, ImportStatement* stmt) {
    const auto p = stmt->parent();
    if(p && p->kind() == ASTNodeKind::FileScope) {
        const auto current_file = p->as_file_scope_unsafe();
        const auto result = linker.path_handler.resolve_import_path(current_file->file_path.str(), stmt->filePath.str());
        if (result.error.empty()) {
            auto found = linker.scope_indexes.find(chem::string_view(result.replaced));
            if (found != linker.scope_indexes.end()) {
                const auto scope = linker.get_scope_at_index(found->second);
                if (scope != nullptr) {
                    const auto next = linker.get_scope_at_index(found->second + 1);
                    const auto& file_symbols = linker.get_symbols();
                    const auto symStart = file_symbols.data() + scope->start;
                    const auto symEnd = file_symbols.data() + (next ? next->start : file_symbols.size());
                    return { scope->start, symStart, symEnd };
                } else {
                    linker.error("couldn't find scope for the index", stmt);
                }
            } else {
                linker.error("couldn't find the scope index for the given file", stmt);
            }
        } else {
            linker.error("couldn't resolve import path", stmt);
        }
    } else {
        linker.error("unknown parent of the import statement", stmt);
    }
    return { -1, nullptr, nullptr };
}

void ImportStatement::declare_top_level(SymbolResolver &linker, ASTNode*& node_ptr) {
    if(as_identifier.empty()) {
        if(filePath[0] != '@') {
            // currently any file that is not beginning with '@' means it's internal to module
            // we always have symbols available for files present inside the module
            return;
        }
        auto itr = get_iterator(linker, this);
        if(itr.start) {
            while(itr.start != itr.end) {
                linker.declare_entry(itr.start, itr.index);
                itr.start++;
                itr.index++;
            }
        }
    } else {
        linker.declare(as_identifier, this);
//        if(filePath[0] != '@') {
//            // currently any file that is not beginning with '@' means it's internal to module
//            // we always have symbols available for files present inside the module
//            return;
//        }
        symbols = new std::unordered_map<chem::string_view, ASTNode*>();
        auto itr = get_iterator(linker, this);
        if(itr.start) {
            while(itr.start != itr.end) {
                symbols->emplace(itr.start->key, itr.start->node);
                itr.start++;
            }
        }
    }
}

ASTNode* ImportStatement::child(const chem::string_view &name) {
    if(symbols) {
        return symbols->find(name)->second;
    } else {
#ifdef DEBUG
      throw std::runtime_error("symbols pointer doesn't exist in import statement");
#endif
    }
}