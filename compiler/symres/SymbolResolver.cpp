// Copyright (c) Chemical Language Foundation 2025.


#include "ast/base/ASTNode.h"
#include "SymbolResolver.h"
#include "ast/values/AccessChain.h"
#include "ast/structures/Namespace.h"
#include "ast/structures/MultiFunctionNode.h"
#include "ast/structures/FunctionDeclaration.h"
#include "ast/base/GlobalInterpretScope.h"
#include "rang.hpp"

SymbolResolver::SymbolResolver(
    GlobalInterpretScope& global,
    ImportPathHandler& handler,
    bool is64Bit,
    ASTAllocator& fileAllocator,
    ASTAllocator* modAllocator,
    ASTAllocator* astAllocator
) : comptime_scope(global), path_handler(handler), ASTDiagnoser(global.loc_man), is64Bit(is64Bit), allocator(fileAllocator),
    mod_allocator(modAllocator), ast_allocator(astAllocator), genericInstantiator(*astAllocator, *this), table(512)
{
    global_scope_start();
    stored_file_symbols.reserve(128);
}

void SymbolResolver::dup_sym_error(const chem::string_view& name, ASTNode* previous, ASTNode* new_node) {
    warn(new_node) << "duplicate symbol being declared, symbol '" << name << "' already exists";
}

void SymbolResolver::dup_runtime_sym_error(const chem::string_view& name, ASTNode* previous, ASTNode* new_node) {
    error(new_node) << "duplicate runtime symbol being declared " << name << " symbol already exists";
}

bool SymbolResolver::declare_quietly(const chem::string_view& name, ASTNode* node) {
    if(name == "_") {
        // symbols with name '_' aren't declared
        return false;
    }
    const auto previous = table.declare_no_shadow(name, node);
    if(previous == nullptr) {
        return true;
    } else {
        // shadow the current symbol
        table.declare(name, node);
        dup_sym_error(name, previous, node);
        return false;
    }
}

void SymbolResolver::declare_runtime(const chem::string_view& name, ASTNode* node) {
    auto str = name.str();
    auto found = runtime_symbols.find(str);
    if(found == runtime_symbols.end()) {
        runtime_symbols[str] = node;
    } else {
        dup_runtime_sym_error(name, found->second, node);
    };
}

bool SymbolResolver::overload_function(const chem::string_view& name, ASTNode* const previous, FunctionDeclaration* declaration) {
    if(declaration->is_override()) {
        const auto func = previous->as_function();
        if (func->returnType->is_same(declaration->returnType) && func->do_param_types_match(declaration->params, false)) {
            table.declare(name, declaration);
            return true;
        } else {
            dup_sym_error(declaration->name_view(), previous, declaration);
            error((AnnotableNode*) declaration) << "function " << declaration->name_view() << " cannot override because it's parameter types and return type don't match";
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

void SymbolResolver::link_body_seq_backing_moves(
        Scope& scope,
        std::vector<VariableIdentifier*>& moved_ids,
        std::vector<AccessChain*>& moved_chains
) {
    const auto curr_func = current_func_type;

    // where the moved ids / chains of if body begin
    const auto if_moved_ids_begin = curr_func->moved_identifiers.size();
    const auto if_moved_chains_begin = curr_func->moved_chains.size();

    // link the body
    scope.link_sequentially(*this);

    // save all the moved identifiers / chains inside the if body to temporary location
    curr_func->save_moved_ids_after(moved_ids, if_moved_ids_begin);
    curr_func->save_moved_chains_after(moved_chains, if_moved_chains_begin);
}

void SymbolResolver::declare_overriding(const chem::string_view &name, ASTNode* node) {
#ifdef DEBUG
    if(name.empty()) {
        std::cerr << rang::fg::red << "empty symbol being declared" << rang::fg::reset << std::endl;
        return;
    }
#endif
    if(name == "_") {
        // symbols with name '_' aren't declared
        return;
    }
    // we'll allow it to shadow, since when the scope ends, the previous symbol will become visible
    // we have to see who's calling this method
    table.declare(name, node);
}

void SymbolResolver::declare(const chem::string_view &name, ASTNode *node) {
#ifdef DEBUG
    if(name.empty()) {
        std::cerr << rang::fg::red << "empty symbol being declared" << rang::fg::reset << std::endl;
        return;
    }
#endif
    table.declare(name, node);
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

SymbolRange SymbolResolver::tld_declare_file(Scope& scope, const std::string& abs_path) {
    const auto scope_index = file_scope_start();
    // TODO abs_path could be referencing a path that would freed
    declared_files.emplace(chem::string_view(abs_path), scope);
    auto& linker = *this;
    const auto start = stored_file_symbols.size();
    scope.tld_declare(linker);
    const auto end = stored_file_symbols.size();
    return SymbolRange { (unsigned int) start, (unsigned int) end };
}

void SymbolResolver::link_signature_file(Scope& scope, const std::string& abs_path, const SymbolRange& range) {
    // we create a scope_index, this scope is strictly for private entries
    // when this scope drops, every private symbol and non closed scope will automatically be dropped
    const auto scope_index = file_scope_start();
    enable_file_symbols(range);
    scope.link_signature(*this);
    file_scope_end(scope_index);
}

void SymbolResolver::link_file(Scope& nodes_scope, const std::string& abs_path, const SymbolRange& range) {
    // we create a scope_index, this scope is strictly for private entries
    // when this scope drops, every private symbol and non closed scope will automatically be dropped
    const auto scope_index = file_scope_start();
    enable_file_symbols(range);
    nodes_scope.declare_and_link(*this);
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
    const auto val_type = value->create_type(allocator);
    if(val_type) {
        error(value) << "value with type '" << val_type->representation() << "' does not satisfy type '" << type->representation() << "'";
    } else {
        error(value) << "value does not satisfy type '" << type->representation() << "'";
    }
}