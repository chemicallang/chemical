// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/base/GlobalInterpretScope.h"
#include "ast/structures/FileScope.h"
#include "ast/statements/Export.h"
#include "ast/structures/ModuleScope.h"
#include "ast/structures/Namespace.h"
#include "ast/statements/EmbeddedNode.h"
#include "ast/structures/FunctionDeclaration.h"
#include "ast/structures/GenericFuncDecl.h"
#include "ast/structures/If.h"
#include "SymbolResolver.h"

template<typename T>
class NodeSymbolDeclarer {
public:
    inline void declare(const chem::string_view& sym, ASTNode* node) {
#ifdef DEBUG
        CHEM_THROW_RUNTIME("this method is a stub");
#endif
    }
    inline void casted_declare(const chem::string_view& sym, ASTNode* node) {
        static_cast<T*>(this)->declare(sym, node);
    }
};

class MapSymbolDeclarer : public NodeSymbolDeclarer<MapSymbolDeclarer> {
public:
    std::unordered_map<chem::string_view, ASTNode*>& map;
    inline MapSymbolDeclarer(std::unordered_map<chem::string_view, ASTNode*>& map) : map(map) {

    }
    inline void declare(const chem::string_view& sym, ASTNode* node) {
        map[sym] = node;
    }
};

class MapSymbolDeclarerNoOverride : public NodeSymbolDeclarer<MapSymbolDeclarerNoOverride> {
public:
    std::unordered_map<chem::string_view, ASTNode*>& map;
    ASTDiagnoser& diagnoser;
    inline MapSymbolDeclarerNoOverride(
            ASTDiagnoser& diagnoser,
            std::unordered_map<chem::string_view, ASTNode*>& map
        ) : map(map), diagnoser(diagnoser) {

    }
    inline void declare(const chem::string_view& sym, ASTNode* node) {
        auto [it, inserted] = map.try_emplace(sym, node);
        if (!inserted) {
            diagnoser.error(node) << "symbol with name '" << sym << "' already exists";
            diagnoser.warn(it->second) << "symbol has a conflict";
        }
    }
};

class SymbolResolverFileLvlDeclarer : public NodeSymbolDeclarer<SymbolResolverFileLvlDeclarer> {
public:
    SymbolResolver& resolver;
    inline SymbolResolverFileLvlDeclarer(SymbolResolver& resolver) : resolver(resolver) {

    }
    inline void declare(const chem::string_view& sym, ASTNode* node) {
        resolver.declare_file_disposable(sym, node);
    }
};

class SymbolResolverDeclarer : public NodeSymbolDeclarer<SymbolResolverDeclarer> {
public:
    SymbolResolver& resolver;
    inline SymbolResolverDeclarer(SymbolResolver& resolver) : resolver(resolver) {

    }
    inline void declare(const chem::string_view& sym, ASTNode* node) {
        resolver.declare(sym, node);
    }
};

class SymbolResolverShadowDeclarer : public NodeSymbolDeclarer<SymbolResolverShadowDeclarer> {
public:
    SymbolResolver& resolver;
    inline SymbolResolverShadowDeclarer(SymbolResolver& resolver) : resolver(resolver) {

    }
    inline void declare(const chem::string_view& sym, ASTNode* node) {
        resolver.declare_or_shadow(sym, node);
    }
};

template<typename T>
void declare_node(NodeSymbolDeclarer<T>& declarer, ASTNode* node, AccessSpecifier at_least_spec) {
    switch(node->kind()) {
        case ASTNodeKind::ExportStmt: {
            const auto stmt = node->as_export_stmt_unsafe();
            const auto& name_view = stmt->as_id.empty() ? stmt->ids.back() : stmt->as_id;
#ifdef DEBUG
            if(stmt->linked_node == nullptr) {
                CHEM_THROW_RUNTIME("export statement is not linked with any node");
            }
#endif
            declarer.casted_declare(name_view, stmt->linked_node);
            break;
        }
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
        case ASTNodeKind::FunctionDecl: {
            const auto decl = node->as_function_unsafe();
            if(!decl->isExtensionFn() && decl->specifier() >= at_least_spec) {
                declarer.casted_declare(decl->name_view(), decl);
            }
            break;
        }
        case ASTNodeKind::GenericFuncDecl: {
            const auto decl = node->as_gen_func_decl_unsafe();
            if(!decl->master_impl->isExtensionFn() && decl->master_impl->specifier() >= at_least_spec) {
                declarer.casted_declare(decl->name_view(), decl);
            }
            break;
        }
        case ASTNodeKind::EmbeddedNode: {
            const auto em = node->as_embedded_node_unsafe();
            if (em->attrs.top_level) {
                const auto declare_proxy = [](void* obj, chem::string_view* name, ASTNode* node) {
                    ((NodeSymbolDeclarer<T>*) obj)->casted_declare(*name, node);
                };
                ((TopLevelEmbeddedNode*) em)->proxy_fn((void*) &declarer, em, declare_proxy, static_cast<int>(at_least_spec));
            }
            break;
        }
        default:{
            const auto id = node->get_node_identifier();
            if(!id.empty() && node->specifier() >= at_least_spec) {
                declarer.casted_declare(id, node);
            }
            break;
        }
    }
}

template<typename T>
void declare_node_no_ns(NodeSymbolDeclarer<T>& declarer, ASTNode* node, AccessSpecifier at_least_spec) {
    switch(node->kind()) {
        case ASTNodeKind::ExportStmt: {
            const auto stmt = node->as_export_stmt_unsafe();
            const auto& name_view = stmt->as_id.empty() ? stmt->ids.back() : stmt->as_id;
#ifdef DEBUG
            if(stmt->linked_node == nullptr) {
                CHEM_THROW_RUNTIME("export statement is not linked with any node");
            }
#endif
            declarer.casted_declare(name_view, stmt->linked_node);
            break;
        }
        case ASTNodeKind::NamespaceDecl: {
            const auto ns = node->as_namespace_unsafe();
            if (ns->specifier() >= at_least_spec) {
                if(ns->root == nullptr) {
                    declarer.casted_declare(ns->name(), ns);
                }
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
        case ASTNodeKind::FunctionDecl: {
            const auto decl = node->as_function_unsafe();
            if(!decl->isExtensionFn() && decl->specifier() >= at_least_spec) {
                declarer.casted_declare(decl->name_view(), decl);
            }
            break;
        }
        case ASTNodeKind::GenericFuncDecl: {
            const auto decl = node->as_gen_func_decl_unsafe();
            if(!decl->master_impl->isExtensionFn() && decl->master_impl->specifier() >= at_least_spec) {
                declarer.casted_declare(decl->name_view(), decl);
            }
            break;
        }
        default:{
            const auto id = node->get_node_identifier();
            if(!id.empty() && node->specifier() >= at_least_spec) {
                declarer.casted_declare(id, node);
            }
            break;
        }
    }
}