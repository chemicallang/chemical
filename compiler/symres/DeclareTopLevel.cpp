// Copyright (c) Chemical Language Foundation 2025.

#include "compiler/symres/SymbolResolver.h"
#include "preprocess/ImportPathHandler.h"
#include "ast/statements/UsingStmt.h"
#include "ast/statements/AliasStmt.h"
#include "ast/statements/Import.h"
#include "ast/statements/Typealias.h"
#include "ast/statements/VarInit.h"
#include "ast/statements/ChildrenMapNode.h"
#include "ast/statements/EmbeddedNode.h"
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
#include "compiler/cbi/model/CBIFunctionType.h"
#include "compiler/cbi/model/CompilerBinder.h"
#include "compiler/lab/LabModule.h"

static void append_parts(Diag& diag, const std::vector<chem::string_view>& parts) {
    bool is_first = true;
    for(auto& part : parts) {
        if(is_first) {
            is_first = false;
        } else {
            diag << '.';
        }
        diag << part;
    }
}

inline static void append_alias(Diag& diag, const chem::string_view& alias) {
    if(!alias.empty()) {
        diag << " as ";
        diag << alias;
    }
}

static void append_alias_and_part(Diag& d, const std::vector<chem::string_view>& parts, const chem::string_view& alias) {
    if(parts.size() > 1) {
        d << " part from '";
        append_parts(d, parts);
        d << "'";
    }
    append_alias(d, alias);
}

// this checks for the import items too
void declareChildren(SymbolResolver& linker, ChildrenMapNode* node, ImportStatement* stmt) {
    auto& import_items = stmt->getImportItems();
    if(import_items.empty() && !stmt->is_force_empty_import_items()) {
        auto& alias = stmt->getTopLevelAlias();
        if(alias.empty()) {
            // user asked to declare all the symbols
            for(auto& sym : node->symbols) {
                linker.declare_or_shadow(sym.first, sym.second);
            }
        } else {
            linker.declare(alias, node);
        }
        return;
    } else {
        auto& symbols = node->symbols;
        for(auto& item : import_items) {
            auto found = symbols.find(item.parts[0]);
            if(found == symbols.end()) {
                auto& d = linker.error(stmt) << "couldn't find symbol '" << item.parts[0] << "'";
                append_alias_and_part(d, item.parts, item.alias);
                continue;
            }
            if(item.parts.size() == 1) {
                // import { file as f } from std <-- single part (file), alias 'f' or 'file' (the first part)
                auto& name = item.alias.empty() ? item.parts[0] : item.alias;
                linker.declare(name, node);
                continue;
            }
            // import { file.change as c } from std <--- multiple parts, alias 'c' or 'change' (the last part)
            ASTNode* current = found->second;
            auto start = item.parts.data() + 1;
            const auto end = item.parts.data() + item.parts.size();
            while (start != end) {
                const auto child = current->child(*start);
                if(child == nullptr) {
                    auto& d = linker.error(stmt) << "couldn't find child symbol '" << *start << "'";
                    append_alias_and_part(d, item.parts, item.alias);
                    current = nullptr;
                    break;
                }
                current = child;
                start++;
            }
            if(current) {
                linker.declare(item.alias.empty() ? *(end - 1) : item.alias, current);
            }
        }
    }
}

void TopLevelDeclSymDeclare::VisitImportStmt(ImportStatement* stmt) {
    switch(stmt->getResultKind()) {
        case ImportResultKind::None:
            linker.error(stmt) << "couldn't import file/module '" << stmt->getSourcePath() << "' because of missing result";
            return;
        case ImportResultKind::File: {
            const auto res = stmt->getFileResult();
            if(res->children != nullptr) {
                declareChildren(linker, res->children, stmt);
                return;
            }

            // create a children map node for the given file
            const auto children = linker.ast_allocator->allocate<ChildrenMapNode>();
            new (children) ChildrenMapNode(stmt->parent(), stmt->encoded_location());

            // declare symbols into the children map node
            const auto is_external_module = stmt->isExternalModuleLabImport();
            const auto at_least_spec = is_external_module ? AccessSpecifier::Public : AccessSpecifier::Internal;
            MapSymbolDeclarer d(children->symbols);
            for(const auto node : stmt->getFileResult()->unit.scope.body.nodes) {
                declare_node(d, node, at_least_spec);
            }

            // store it for caching
            res->children = children;

            // declare the children map node so user can access symbols through it
            declareChildren(linker, children, stmt);
            return;
        }
        case ImportResultKind::Module: {
            const auto module = stmt->getModuleResult();
            if(module->children != nullptr) {
                declareChildren(linker, module->children, stmt);
                return;
            }

            // create a children map node for the given file
            const auto children = linker.ast_allocator->allocate<ChildrenMapNode>();
            new (children) ChildrenMapNode(stmt->parent(), stmt->encoded_location());

            // declare symbols into the children map node
            MapSymbolDeclarer d(children->symbols);
            for(auto& file_ptr : module->direct_files) {
                auto& file = *file_ptr.result;
                for(const auto node : file.unit.scope.body.nodes) {
                    declare_node(d, node, AccessSpecifier::Public);
                }
            }

            // store it for caching
            module->children = children;

            // declare the children map node so user can access symbols through it
            declareChildren(linker, children, stmt);
            return;
        }
    }
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
    // TODO: condition should not reference a comptime variable/constant that is present in same module
    //  because we declare in order, and we link the conditions in order too, any symbols that appear below
    //  won't be considered
    auto& comptime_scope = linker.comptime_scope;
    ASTDiagnoser& diagnoser = linker;
    // we don't know if conditions of this if statement have linked or failed
    node->link_conditions(linker);
    auto condition_evaluated = node->get_condition_const(comptime_scope);;
    if(!condition_evaluated.has_value()) {
        diagnoser.error("couldn't evaluate condition", node->condition);
        return;
    }
    auto eval = node->get_evaluated_scope(comptime_scope, &diagnoser, condition_evaluated.value());
    node->computed_scope = eval;
    if(eval) {
        visit(eval);
    }
}

void TopLevelDeclSymDeclare::VisitImplDecl(ImplDefinition* node) {
    node->take_members_from_parsed_nodes(linker);
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

void TopLevelDeclSymDeclare::VisitEmbeddedNode(EmbeddedNode* node) {
    auto found = linker.binder.findHook(node->name, CBIFunctionType::SymResDeclareTopLevelNode);
    if(found) {
        ((EmbeddedNodeSymResDeclareTopLevel) found)(&linker, node);
    } else {
        // maybe the node doesn't want to declare a top level symbol
    }
}