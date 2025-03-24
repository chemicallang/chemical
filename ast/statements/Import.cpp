// Copyright (c) Chemical Language Foundation 2025.

#include "Import.h"
#include <filesystem>
#include "compiler/SymbolResolver.h"
#include "preprocess/ImportPathHandler.h"
#include "ast/base/GlobalInterpretScope.h"
#include "ast/structures/FileScope.h"
#include "ast/structures/ModuleScope.h"
#include "ast/structures/Namespace.h"
#include "ast/base/LocatedIdentifier.h"
#include "ast/structures/If.h"

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

template<typename T>
class SymbolDeclarer {
public:
    inline void declare(const chem::string_view& sym, ASTNode* node) {
#ifdef DEBUG
        throw std::runtime_error("shouldn't have called this method");
#endif
    }
    inline void casted_declare(const chem::string_view& sym, ASTNode* node) {
        static_cast<T*>(this)->declare(sym, node);
    }
};

class MapSymbolDeclarer : public SymbolDeclarer<MapSymbolDeclarer> {
public:
    std::unordered_map<chem::string_view, ASTNode*>& map;
    inline MapSymbolDeclarer(std::unordered_map<chem::string_view, ASTNode*>& map) : map(map) {

    }
    inline void declare(const chem::string_view& sym, ASTNode* node) {
        map[sym] = node;
    }
};

class SymbolResolverDeclarer : public SymbolDeclarer<SymbolResolverDeclarer> {
public:
    SymbolResolver& resolver;
    inline SymbolResolverDeclarer(SymbolResolver& resolver) : resolver(resolver) {

    }
    inline void declare(const chem::string_view& sym, ASTNode* node) {
        resolver.declare_file_disposable(sym, node);
    }
};

template<typename T>
void declare_node(SymbolDeclarer<T>& declarer, ASTNode* node, AccessSpecifier at_least_spec) {
    switch(node->kind()) {
        case ASTNodeKind::NamespaceDecl: {
            const auto ns = node->as_namespace_unsafe();
            if (ns->specifier() >= at_least_spec) {
                declarer.casted_declare(ns->name(), ns->root ? ns->root : ns);
            }
            break;
        }
        case ASTNodeKind::IfStmt: {
            const auto stmt = node->as_if_stmt_unsafe();
            if(stmt->computed_scope.has_value()) {
                const auto scope = stmt->computed_scope.value();
                if(scope) {
                    for (const auto child: scope->nodes) {
                        declare_node(declarer, child, at_least_spec);
                    }
                }
            }
            break;
        }
        default:{
            const auto id = node->get_located_id();
            if(id) {
                if(node->specifier() >= at_least_spec) {
                    declarer.casted_declare(id->identifier, node);
                }
            }
            break;
        }
    }
}

void ImportStatement::declare_top_level(SymbolResolver &linker, ASTNode*& node_ptr) {
    if(as_identifier.empty()) {
        const auto is_external_module = filePath[0] == '@';
        auto itr = get_iterator(linker, this);
        if(itr.start) {
            const auto at_least_spec = is_external_module ? AccessSpecifier::Public : AccessSpecifier::Internal;
            SymbolResolverDeclarer d(linker);
            while (itr.start != itr.end) {
                const auto node = *itr.start;
                declare_node(d, node, at_least_spec);
                itr.start++;
            }
        }
    } else {
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