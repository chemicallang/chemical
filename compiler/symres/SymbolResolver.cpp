// Copyright (c) Chemical Language Foundation 2025.


#include "ast/base/ASTNode.h"
#include "SymbolResolver.h"
#include "ast/values/AccessChain.h"
#include "ast/structures/Namespace.h"
#include "ast/structures/MultiFunctionNode.h"
#include "ast/structures/FunctionDeclaration.h"
#include "ast/base/GlobalInterpretScope.h"
#include "rang.hpp"
#include "compiler/typeverify/TypeVerifyAPI.h"
#include "DeclareTopLevel.h"
#include "LinkSignatureAPI.h"
#include "SymResLinkBodyAPI.h"
#include "ast/statements/UnresolvedDecl.h"
#include "ast/base/TypeBuilder.h"

SymbolResolver::SymbolResolver(
    CompilerBinder& binder,
    GlobalInterpretScope& global,
    ImportPathHandler& handler,
    AnnotationController& controller,
    InstantiationsContainer& container,
    bool is64Bit,
    ASTAllocator& fileAllocator,
    ASTAllocator* modAllocator,
    ASTAllocator* astAllocator
) : binder(binder), comptime_scope(global), path_handler(handler), instContainer(container), ASTDiagnoser(global.loc_man), is64Bit(is64Bit),
    allocator(fileAllocator), mod_allocator(modAllocator), ast_allocator(astAllocator), controller(controller),
    genericInstantiator(binder, child_resolver, container, *astAllocator, *this, global.typeBuilder), table(512)
{
    global_scope_start();
    stored_file_symbols.reserve(128);
}

UnresolvedDecl* SymbolResolver::get_unresolved_decl() {
    return comptime_scope.typeBuilder.getUnresolvedDecl();
}

void SymbolResolver::dup_sym_error(const chem::string_view& name, ASTNode* previous, ASTNode* new_node) {
    error(new_node) << "duplicate symbol being declared, symbol '" << name << "' already exists";
    error(previous) << "symbol has a conflict";
}

bool SymbolResolver::declare_quietly(const chem::string_view& name, ASTNode* node) {
    const auto previous = table.declare_no_shadow(name, node);
    if(previous == nullptr) {
        return true;
    } else {
        const auto p = node->parent();
        // symbols with namespace as parents, aren't duplicates, they are hiding members
        if(p && p->kind() == ASTNodeKind::NamespaceDecl) {
            // shadow the current symbol
            table.declare(name, node);
        } else {
            // shadow the current symbol
            table.declare(name, node);
            dup_sym_error(name, previous, node);
        }
        return false;
    }
}

bool params_satisfy(FunctionType* type, std::vector<FunctionParam*>& param_types, bool check_self) {
    if(type->params.size() != param_types.size()) return false;
    unsigned i = check_self ? 0 : (type->has_self_param() ? 1 : 0);
    const auto siz = type->params.size();
    while(i < siz) {
        if(!type->params[i]->type->satisfies(param_types[i]->type)) {
            return false;
        }
        i++;
    }
    return true;
}

bool SymbolResolver::overload_function(const chem::string_view& name, ASTNode* const previous, FunctionDeclaration* declaration) {
    if(declaration->is_override()) {
        const auto func = previous->as_function();
        if(func == nullptr) {
            error((ASTNode*) declaration) << "node with name '" << name << "' cannot be overridden because its not a function";
            return false;
        }
        if (func->returnType->satisfies(declaration->returnType) && params_satisfy(func, declaration->params, false)) {
            table.declare(name, declaration);
            return true;
        } else {
            dup_sym_error(declaration->name_view(), previous, declaration);
            error((ASTNode*) declaration) << "function '" << declaration->name_view() << "' cannot override because it's parameter types or return type don't match";
            return false;
        }
    }
    auto result = handle_name_overload_function(*ast_allocator, previous, declaration);
    if(result.specifier_mismatch) {
        error("couldn't overload function because it's access specifier is different from previous function", (ASTNode*) declaration);
        return false;
    } else if(!result.duplicates.empty()) {
        for(auto dup : result.duplicates) {
            dup_sym_error(name, dup, declaration);
        }
    } else if(result.new_multi_func_node) {
        // override the previous symbol
        table.declare(name, result.new_multi_func_node);
        return true;
    }
    return false;
}

void SymbolResolver::declare_or_shadow(const chem::string_view &name, ASTNode* node) {
#ifdef DEBUG
    if(name.empty()) {
        std::cerr << rang::fg::red << "empty symbol being declared" << rang::fg::reset << std::endl;
        return;
    }
#endif
    // we'll allow it to shadow, since when the scope ends, the previous symbol will become visible
    // we have to see who's calling this method
    table.declare(name, node);
}

void SymbolResolver::declare_local_var(const chem::string_view &name, ASTNode *node, unsigned long lambda_scope_start, bool in_lambda_scope) {
#ifdef DEBUG
    if(name.empty()) {
        std::cerr << rang::fg::red << "empty symbol being declared" << rang::fg::reset << std::endl;
        return;
    }
#endif
    const auto previous = table.declare_no_shadow_sym(name, node);
    if(previous) {
        if(in_lambda_scope && previous->index < lambda_scope_start) {
            // previous symbol outside lambda scope, allow shadowing
            table.declare(name, node);
            return;
        }
        if(previous->activeNode->is_top_level()) {
            // previous symbol is a top level symbol, allow shadowing
            table.declare(name, node);
            return;
        }
        // error out, symbol now allowed to be shadowed
        error(node) << "symbol with name '" << name << "' already exists";
        error(previous->activeNode) << "symbol has a conflict";
        // shadow the symbol, why shadow ? so errors consider user's intention to shadow
        table.declare(name, node);
    }
}

void SymbolResolver::declare(const chem::string_view &name, ASTNode *node) {
#ifdef DEBUG
    if(name.empty()) {
        std::cerr << rang::fg::red << "empty symbol being declared" << rang::fg::reset << std::endl;
        return;
    }
#endif
    const auto previous = table.declare_no_shadow(name, node);
    if(previous) {
        error(node) << "symbol with name '" << name << "' already exists";
        error(previous) << "symbol has a conflict";
        // shadow the symbol
        table.declare(name, node);
    }
}

void SymbolResolver::declare_file_disposable(const chem::string_view &name, ASTNode *node) {
    stored_file_symbols.emplace_back(name, node);
}

void SymbolResolver::declare_function(const chem::string_view& name, FunctionDeclaration* declaration) {
#ifdef DEBUG
    if(name.empty()) {
        std::cerr << rang::fg::red << "error: ";
        std::cerr << "empty symbol being declared" << rang::fg::reset << std::endl;
        return;
    }
#endif
    declare_function_quietly(name, declaration);
}

void SymbolResolver::declare_private_function(const chem::string_view& name, FunctionDeclaration* declaration) {
    // TODO please note that this doesn't take into account name overloading
    stored_file_symbols.emplace_back(name, declaration);
}

void SymbolResolver::declare_node(const chem::string_view& name, ASTNode* node, AccessSpecifier specifier, bool has_runtime) {
    switch(specifier) {
        case AccessSpecifier::Private:
        case AccessSpecifier::Protected:
            declare_file_disposable(name, node);
            return;
        case AccessSpecifier::Public:
            declare_exported(name, node);
            // TODO do we need to check for conflicts in top level runtime symbols
//            if(has_runtime) {
//                auto str = node->runtime_name_str();
//                declare_runtime(chem::string_view(str.data(), str.size()), node);
//            }
            return;
        case AccessSpecifier::Internal:
            declare(name, node);
            return;
    }
}

void SymbolResolver::declare_function(const chem::string_view& name, FunctionDeclaration* decl, AccessSpecifier specifier) {
    switch(specifier) {
        case AccessSpecifier::Private:
        case AccessSpecifier::Protected:
            declare_private_function(name, decl);
            return;
        case AccessSpecifier::Public:
            {
                declare_exported_function(name, decl);
                // TODO do we need to check for conflicts in top level runtime
//                auto str = decl->runtime_name_str();
//                declare_runtime(chem::string_view(str.data(), str.size()), decl);
            }
            return;
        case AccessSpecifier::Internal:
            declare_function(name, decl);
            return;
    }
}

void SymbolResolver::enable_file_symbols(const SymbolRange& range) {
    if(range.symbol_start == range.symbol_end) return;
    auto sym = stored_file_symbols.data() + range.symbol_start;
    const auto end = stored_file_symbols.data() + range.symbol_end;
    while(sym != end) {
        table.declare(sym->symbol, sym->node);
        sym++;
    }
}

SymbolRange SymbolResolver::tld_declare_file(
        Scope& scope,
        unsigned int fileId,
        const std::string& abs_path
) {
    instContainer.current_file_id = fileId;
    const auto scope_index = file_scope_start();
    // TODO abs_path could be referencing a path that would freed
    declared_files.emplace(chem::string_view(abs_path), scope);
    auto& linker = *this;
    const auto start = stored_file_symbols.size();
    TopLevelDeclSymDeclare declarer(*this);
    declarer.VisitScope(&scope);
    const auto end = stored_file_symbols.size();
    return SymbolRange { (unsigned int) start, (unsigned int) end };
}

void SymbolResolver::before_link_signature_file(
        Scope& scope,
        unsigned int fileId,
        const SymbolRange& range
) {
    instContainer.current_file_id = fileId;
    // we create a scope_index, this scope is strictly for private entries
    // when this scope drops, every private symbol and non closed scope will automatically be dropped
    const auto scope_index = file_scope_start();
    enable_file_symbols(range);
    // symbol resolve the scope
    sym_res_before_signature(*this, &scope);
    file_scope_end(scope_index);
}

void SymbolResolver::link_signature_file(
        Scope& scope,
        unsigned int fileId,
        const SymbolRange& range
) {
    instContainer.current_file_id = fileId;
    // we create a scope_index, this scope is strictly for private entries
    // when this scope drops, every private symbol and non closed scope will automatically be dropped
    const auto scope_index = file_scope_start();
    enable_file_symbols(range);
    // symbol resolve the scope
    sym_res_signature(*this, &scope);
    file_scope_end(scope_index);
    // when linking signature is done, we should type verify only the top level var init decls
    if(!has_errors) {
        // we only do this if there are no errors (everything symbol resolved properly)
        type_verify(*this, allocator, scope.nodes);
    }
}

void SymbolResolver::link_file(
        Scope& nodes_scope,
        unsigned int fileId,
        const SymbolRange& range
) {
    instContainer.current_file_id = fileId;
    // we create a scope_index, this scope is strictly for private entries
    // when this scope drops, every private symbol and non closed scope will automatically be dropped
    const auto scope_index = file_scope_start();
    enable_file_symbols(range);
    sym_res_link_body(*this, &nodes_scope);
    file_scope_end(scope_index);
}

void SymbolResolver::declare_and_link_file(Scope& scope, unsigned int fileId, const std::string& abs_path) {
    instContainer.current_file_id = fileId;
    const auto scope_index = file_scope_start();
    // TODO abs_path could be referencing a path that would freed
    declared_files.emplace(chem::string_view(abs_path), scope);
    auto& linker = *this;
    const auto start = stored_file_symbols.size();
    TopLevelDeclSymDeclare declarer(*this);
    declarer.VisitScope(&scope);
    const auto end = stored_file_symbols.size();
    auto range = SymbolRange { (unsigned int) start, (unsigned int) end };
    enable_file_symbols(range);
    sym_res_before_signature(*this, &scope);
    sym_res_signature(*this, &scope);
    sym_res_link_body(*this, &scope);
    file_scope_end(scope_index);
}

void SymbolResolver::import_file(std::vector<ASTNode*>& nodes, const std::string_view& path, bool restrict_public) {
    file_scope_start();
    for(const auto node : nodes) {
        const auto requested_specifier = node->specifier();
        const auto specifier = restrict_public ? requested_specifier == AccessSpecifier::Public ? AccessSpecifier::Internal : requested_specifier :  requested_specifier;
        auto id = node->get_located_id();
        if(id && specifier != AccessSpecifier::Private) {
            declare_node(id->identifier, node, specifier, true);
        }
    }
    print_diagnostics(chem::string_view(path), "SymRes");
    diagnostics.clear();
}

void SymbolResolver::unsatisfied_type_err(Value* value, BaseType* type) {
    ::unsatisfied_type_err(*this, allocator, value, type);
}