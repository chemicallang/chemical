// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/base/GlobalInterpretScope.h"
#include "ast/structures/FileScope.h"
#include "ast/structures/ModuleScope.h"
#include "ast/structures/Namespace.h"
#include "ast/structures/FunctionDeclaration.h"
#include "ast/structures/GenericFuncDecl.h"
#include "ast/base/LocatedIdentifier.h"
#include "ast/structures/If.h"
#include "SymbolResolver.h"

template<typename T>
class NodeSymbolDeclarer {
public:
    inline void declare(const chem::string_view& sym, ASTNode* node) {
#ifdef DEBUG
        throw std::runtime_error("this method is a stub");
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

template<typename T>
void declare_node(NodeSymbolDeclarer<T>& declarer, ASTNode* node, AccessSpecifier at_least_spec) {
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