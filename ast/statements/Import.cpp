// Copyright (c) Chemical Language Foundation 2025.

#include "Import.h"
#include <filesystem>
#include "compiler/SymbolResolver.h"
#include "preprocess/ImportPathHandler.h"
#include "ast/structures/ModuleScope.h"
#include "ast/base/LocatedIdentifier.h"
#include "compiler/symres/NodeSymbolDeclarer.h"
#include "utils/PathUtils.h"

struct FileNodesIterator {
    ASTNode** start;
    ASTNode** end;
};

FileNodesIterator get_iterator(SymbolResolver& linker, ImportStatement* stmt) {
    const auto p = stmt->parent();
    if(p && p->kind() == ASTNodeKind::FileScope) {
        const auto current_file = p->as_file_scope_unsafe();
//        const auto curr_mod = current_file->parent();
        const auto result = linker.path_handler.resolve_import_path(current_file->file_path.str(), stmt->filePath.str());
//        const auto id = linker.path_handler.get_mod_identifier_from_import_path(result.replaced);
        if (result.error.empty()) {
            auto found = linker.declared_files.find(chem::string_view(result.replaced));
            if(found != linker.declared_files.end()) {
                auto& nodes = found->second.nodes;
                return { nodes.data(), nodes.data() + nodes.size() };
            } else {
                if(result.replaced.ends_with("build.lab")) {
                    auto mod_path = resolve_sibling(result.replaced, "chemical.mod");
                    auto sec_found = linker.declared_files.find(chem::string_view(mod_path));
                    if(sec_found != linker.declared_files.end()) {
                        auto& nodes = sec_found->second.nodes;
                        return { nodes.data(), nodes.data() + nodes.size() };
                    }
                }
                linker.error(stmt) << "couldn't find the file '" << result.replaced << "' to import ";
            }
        } else {
            linker.error("couldn't resolve import path", stmt);
        }
    } else {
        linker.error("unknown parent of the import statement", stmt);
    }
    return { nullptr, nullptr };
}

void ImportStatement::declare_top_level(SymbolResolver &linker, ASTNode*& node_ptr) {
    if(!as_identifier.empty()) {
        linker.declare(as_identifier, this);
        symbols = new std::unordered_map<chem::string_view, ASTNode*>();
        const auto is_external_module = filePath[0] == '@';
        auto itr = get_iterator(linker, this);
        if(itr.start) {
            const auto at_least_spec = is_external_module ? AccessSpecifier::Public : AccessSpecifier::Internal;
            MapSymbolDeclarer d(*symbols);
            while (itr.start != itr.end) {
                const auto node = *itr.start;
                declare_node(d, node, at_least_spec);
                itr.start++;
            }
        }
    }
// else {
//          const auto is_external_module = filePath[0] == '@';
//          auto itr = get_iterator(linker, this);
//          if(itr.start) {
//              const auto at_least_spec = is_external_module ? AccessSpecifier::Public : AccessSpecifier::Internal;
//              SymbolResolverFileLvlDeclarer d(linker);
//              while (itr.start != itr.end) {
//                  const auto node = *itr.start;
//                  declare_node(d, node, at_least_spec);
//                  itr.start++;
//              }
//          }
//    }
}

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