// Copyright (c) Chemical Language Foundation 2025.

#include "compiler/symres/SymbolResolver.h"
#include "preprocess/ImportPathHandler.h"
#include "ast/statements/UsingStmt.h"
#include "ast/statements/AliasStmt.h"
#include "ast/statements/Import.h"
#include "ast/statements/Typealias.h"
#include "ast/statements/VarInit.h"
#include "ast/structures/EnumDeclaration.h"
#include "ast/structures/GenericFuncDecl.h"
#include "ast/structures/GenericInterfaceDecl.h"
#include "ast/structures/GenericStructDecl.h"
#include "ast/structures/GenericTypeDecl.h"
#include "ast/structures/GenericUnionDecl.h"
#include "ast/structures/GenericVariantDecl.h"
#include "ast/structures/GenericImplDecl.h"
#include "ast/structures/If.h"
#include "ast/structures/ImplDefinition.h"
#include "ast/structures/InterfaceDefinition.h"
#include "ast/structures/Namespace.h"
#include "ast/structures/StructMember.h"
#include "ast/structures/UnionDef.h"
#include "ast/structures/VariantDefinition.h"
#include "NodeSymbolDeclarer.h"
#include "utils/PathUtils.h"
#include "DeclareTopLevel.h"

void TopLevelDeclSymDeclare::VisitUsingStmt(UsingStmt* node) {
    if (node->is_propagate()) {
        if (!node->chain->link(linker, nullptr, nullptr, true, false)) {
            node->attrs.failed_chain_link = true;

            return;
        }
        node->declare_symbols(linker);
    }
}

void TopLevelDeclSymDeclare::VisitAliasStmt(AliasStmt* stmt) {
    const auto value = stmt->value;

    // currently only identifier values are supported
    if (value->kind() == ValueKind::AccessChain) {
        const auto chain = value->as_access_chain_unsafe();
        if (chain->values.size() != 1 || chain->values.front()->kind() != ValueKind::Identifier) {
            linker.error(stmt) << "incompatible value given to alias";
            return;
        }
    }
    if (value->link(linker, stmt->value, nullptr)) {
        const auto node = value->linked_node();
        if (!node) {
            linker.error(stmt) << "cannot alias incompatible value";
            return;
        }
        if (stmt->specifier >= node->specifier()) {
            linker.error(stmt) << "cannot alias a node to a higher specifier";
            return;
        }
        // declares the node without runtime
        linker.declare_node(stmt->alias_name, node, stmt->specifier, false);
    }
}

struct FileNodesIterator {
    ASTNode** start;
    ASTNode** end;
};

FileNodesIterator get_iterator(SymbolResolver& linker, ImportStatement* stmt) {
    const auto p = stmt->parent();
    if(p && p->kind() == ASTNodeKind::FileScope) {
        const auto current_file = p->as_file_scope_unsafe();
//        const auto curr_mod = current_file->parent();
        const auto result = linker.path_handler.resolve_import_path(current_file->file_path.view(), stmt->filePath.view());
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

void TopLevelDeclSymDeclare::VisitImportStmt(ImportStatement* stmt) {
    if (!stmt->as_identifier.empty()) {
        linker.declare(stmt->as_identifier, stmt);
        stmt->symbols = new std::unordered_map<chem::string_view, ASTNode*>();
        const auto is_external_module = stmt->filePath[0] == '@';
        auto itr = get_iterator(linker, stmt);
        if (itr.start) {
            const auto at_least_spec = is_external_module ? AccessSpecifier::Public : AccessSpecifier::Internal;
            MapSymbolDeclarer d(*stmt->symbols);
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

void TopLevelDeclSymDeclare::VisitTypealiasStmt(TypealiasStatement* node) {
    linker.declare_node(node->name_view(), node, node->specifier(), false);
}

void TopLevelDeclSymDeclare::VisitVarInitStmt(VarInitStatement* node) {
    linker.declare_node(node->id_view(), node, node->specifier(), node);
}

void TopLevelDeclSymDeclare::VisitEnumDecl(EnumDeclaration* node) {
    linker.declare_node(node->name_view(), node, node->specifier(), false);
}

void TopLevelDeclSymDeclare::VisitFunctionDecl(FunctionDeclaration* node) {
    // extension functions do not declare themselves
    if (!node->isExtensionFn()) {
        linker.declare_function(node->name_view(), node, node->specifier());
    }
}

void TopLevelDeclSymDeclare::VisitScope(Scope* scope) {
    for (auto& node : scope->nodes) {
        visit(node);
    }
}

void TopLevelDeclSymDeclare::VisitGenericFuncDecl(GenericFuncDecl* node) {
    if (!node->master_impl->isExtensionFn()) {
        linker.declare(node->master_impl->name_view(), node);
    }
}

void TopLevelDeclSymDeclare::VisitGenericInterfaceDecl(GenericInterfaceDecl* node) {
    node->master_impl->take_members_from_parsed_nodes(linker);
    linker.declare(node->master_impl->name_view(), node);
}

void TopLevelDeclSymDeclare::VisitGenericStructDecl(GenericStructDecl* node) {
    node->master_impl->take_members_from_parsed_nodes(linker);
    linker.declare(node->master_impl->name_view(), node);
}

void TopLevelDeclSymDeclare::VisitGenericImplDecl(GenericImplDecl* node) {
    visit(node->master_impl);
}

void TopLevelDeclSymDeclare::VisitGenericTypeDecl(GenericTypeDecl* node) {
    linker.declare(node->master_impl->name_view(), node);
}

void TopLevelDeclSymDeclare::VisitGenericUnionDecl(GenericUnionDecl* node) {
    node->master_impl->take_members_from_parsed_nodes(linker);
    linker.declare(node->master_impl->name_view(), node);
}

void TopLevelDeclSymDeclare::VisitGenericVariantDecl(GenericVariantDecl* node) {
    node->master_impl->take_members_from_parsed_nodes(linker);
    linker.declare(node->master_impl->name_view(), node);
}

void TopLevelDeclSymDeclare::VisitIfStmt(IfStatement* node) {
    if (node->is_top_level()) {
        auto scope = node->link_evaluated_scope(linker);
        if (scope) {
            visit(scope);
        }
    }
}

void TopLevelDeclSymDeclare::VisitImplDecl(ImplDefinition* node) {
    node->take_members_from_parsed_nodes(linker);
    node->interface_type.link(linker);
    if (node->struct_type) {
        node->struct_type.link(linker);
    }
    const auto linked = node->interface_type->linked_node();
    if (linked) {
        const auto interface_def = linked->as_interface_def();
        if (interface_def) {
            if (interface_def->is_static() && interface_def->has_implementation()) {
                linker.error("static interface must have only a single implementation", node->encoded_location());
            }
            interface_def->register_impl(node);
        } else {
            linker.error("expected type to be an interface", node->encoded_location());
        }
    }
}

void TopLevelDeclSymDeclare::VisitInterfaceDecl(InterfaceDefinition* node) {
    node->take_members_from_parsed_nodes(linker);
    linker.declare_node(node->name_view(), node, node->specifier(), false);
}

void TopLevelDeclSymDeclare::VisitNamespaceDecl(Namespace* ns) {
    auto& root = ns->root;
    auto previous = linker.find(ns->name());
    if (previous) {
        root = previous->as_namespace();
        if (root) {
            // namespace attributes are propagated to all namespaces with same name ?
            // TODO propagate namespace attributes
            // attrs = root->attrs;
            if (ns->specifier() < root->specifier()) {
                linker.error(ns) << "access specifier of this namespace must be at least '"
                                   << to_string(root->specifier()) << "' to match previous";
                return;
            }
            linker.scope_start();
            root->declare_extended_in_linker(linker);
            for (auto& node: ns->nodes) {
                visit(node);
            }
            // TODO we must check for duplicate symbols being declared in root_extended
            ns->put_in_extended(root->extended);
            linker.scope_end();
        } else {
            linker.dup_sym_error(ns->name(), previous, ns);
        }
    } else {
        linker.declare_node(ns->name(), ns, ns->specifier(), false);
        linker.scope_start();
        // declare top level all nodes inside the namespace
        for (auto& node: ns->nodes) {
            visit(node);
        }
        // we do not check for duplicate symbols here, because nodes are being declared first time
        MapSymbolDeclarer declarer(ns->extended);
        for (const auto node: ns->nodes) {
            ::declare_node(declarer, node, AccessSpecifier::Private);
        }
        linker.scope_end();
    }
}

void TopLevelDeclSymDeclare::VisitStructMember(StructMember* node) {
    linker.declare(node->name, node);
}

void TopLevelDeclSymDeclare::VisitStructDecl(StructDefinition* node) {
    // no variables or functions exist in containers because user maybe using
    // compile time ifs, so we take out members after linking compile time ifs
    // and put them into their containers (without linking here)
    node->take_members_from_parsed_nodes(linker);
    linker.declare_node(node->name_view(), node, node->specifier(), true);
}

void TopLevelDeclSymDeclare::VisitUnionDecl(UnionDef* node) {
    node->take_members_from_parsed_nodes(linker);
    linker.declare_node(node->name_view(), node, node->specifier(), true);
}

void TopLevelDeclSymDeclare::VisitVariantDecl(VariantDefinition* node) {
    node->take_members_from_parsed_nodes(linker);
    linker.declare_node(node->name_view(), node, node->specifier(), true);
}