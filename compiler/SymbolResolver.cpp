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
    bool is64Bit,
    ASTAllocator& fileAllocator,
    ASTAllocator* modAllocator,
    ASTAllocator* astAllocator
) : comptime_scope(global), ASTDiagnoser(global.loc_man), is64Bit(is64Bit), allocator(fileAllocator),
    mod_allocator(modAllocator), ast_allocator(astAllocator), genericInstantiator(*astAllocator, *this), table(512)
{
    global_scope_start();
    dispose_file_symbols.reserve(128);
    dispose_module_symbols.reserve(128);
}

void SymbolResolver::dup_sym_error(const chem::string_view& name, ASTNode* previous, ASTNode* new_node) {
    error(new_node) << "duplicate symbol being declared, symbol '" << name << "' already exists";
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
    if(!result.duplicates.empty()) {
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
    if(is_current_file_scope()) { // only top level scope symbols are disposed at module's end
        dispose_module_symbols.emplace_back(name);
    }
}

void SymbolResolver::declare(const chem::string_view &name, ASTNode *node) {
#ifdef DEBUG
    if(name.empty()) {
        std::cerr << rang::fg::red << "empty symbol being declared" << rang::fg::reset << std::endl;
        return;
    }
#endif
    table.declare(name, node);
    if (is_current_file_scope()) { // only top level scope symbols are disposed at module's end
        dispose_module_symbols.emplace_back(name);
    }
}

void SymbolResolver::declare_file_disposable(const chem::string_view &name, ASTNode *node) {
    table.declare(name, node);
    if (is_current_file_scope()) {
        dispose_file_symbols.emplace_back(SymbolRef{ name }, node);
    }
}

void SymbolResolver::declare_function(const chem::string_view& name, FunctionDeclaration* declaration) {
#ifdef DEBUG
    if(name.empty()) {
        std::cerr << rang::fg::red << "error: ";
        std::cerr << "empty symbol being declared" << rang::fg::reset << std::endl;
        return;
    }
#endif
    const auto new_sym = declare_function_quietly(name, declaration);
    if(new_sym && is_current_file_scope()) { // only top level scope symbols are disposed at module's end
        dispose_module_symbols.emplace_back(name);
    }
}

void SymbolResolver::declare_private_function(const chem::string_view& name, FunctionDeclaration* declaration) {
    const auto new_sym = declare_function_quietly(name, declaration);
    if(new_sym && is_current_file_scope()) {
        dispose_file_symbols.emplace_back(SymbolRef { name }, declaration);
    }
}

void SymbolResolver::declare_node(const chem::string_view& name, ASTNode* node, AccessSpecifier specifier, bool has_runtime) {
    switch(specifier) {
        case AccessSpecifier::Private:
        case AccessSpecifier::Protected:
            declare_file_disposable(name, node);
            return;
        case AccessSpecifier::Public:
            declare_exported(name, node);
            if(has_runtime) {
                auto str = node->runtime_name_str();
                declare_runtime(chem::string_view(str.data(), str.size()), node);
            }
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
                auto str = decl->runtime_name_str();
                declare_runtime(chem::string_view(str.data(), str.size()), decl);
            }
            return;
        case AccessSpecifier::Internal:
            declare_function(name, decl);
            return;
    }
}

void SymbolResolver::enable_file_symbols(const SymbolRange& range) {
    if(range.symbol_start == range.symbol_end) return;
    auto sym = dispose_file_symbols.data() + range.symbol_start;
    const auto end = dispose_file_symbols.data() + range.symbol_end;
    while(sym != end) {
        table.declare(sym->symbol, sym->node);
        sym++;
    }
}

void SymbolResolver::dispose_file_symbols_now(const std::string_view& abs_path, const SymbolRange& range) {
    if(range.symbol_start == range.symbol_end) return;
    auto sym = dispose_file_symbols.data() + range.symbol_start;
    const auto end = dispose_file_symbols.data() + range.symbol_end;
    while(sym != end) {
        if(!table.erase(sym->symbol)) {
            std::cerr << rang::fg::yellow << "[SymRes] unable to un-declare file symbol " << sym->symbol << " in file " << abs_path  << rang::fg::reset << std::endl;
        }
        sym++;
    }
}

void SymbolResolver::resolve_file(Scope& scope, const std::string& abs_path) {
    file_scope_start();
    scope.link_asynchronously(*this);
    dispose_all_file_symbols(abs_path);
}

SymbolRange SymbolResolver::tld_declare_file(Scope& scope, const std::string& abs_path) {
    const auto scope_index = file_scope_start();
    auto& linker = *this;
    const auto start = dispose_file_symbols.size();
    scope.tld_declare(linker);
    const auto end = dispose_file_symbols.size();
    auto range = SymbolRange { (unsigned int) start, (unsigned int) end };
    dispose_file_symbols_now(abs_path, range);
    return range;
}

void SymbolResolver::link_signature_file(Scope& scope, const std::string& abs_path, const SymbolRange& range) {
    enable_file_symbols(range);
    scope.link_signature(*this);
    dispose_file_symbols_now(abs_path, range);
}

void SymbolResolver::link_file(Scope& nodes_scope, const std::string& abs_path, const SymbolRange& range) {
    enable_file_symbols(range);
    nodes_scope.declare_and_link(*this);
    dispose_file_symbols_now(abs_path, range);
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

void SymbolResolver::dispose_all_file_symbols(const std::string_view& abs_path) {
    if(dispose_file_symbols.empty()) return;
#ifdef DEBUG
    if(!is_current_file_scope()) {
        throw std::runtime_error("undeclare in current file, while current file is not a file");
    }
#endif
    for(const auto& sym : dispose_file_symbols) {
        if(!table.erase(sym.symbol)) {
            std::cerr << rang::fg::yellow << "[SymRes] unable to un-declare file symbol " << sym.symbol << " in file " << abs_path  << rang::fg::reset << std::endl;
        }
    }
    dispose_file_symbols.clear();
}

void SymbolResolver::dispose_module_symbols_now(const std::string& module_name) {
    if(dispose_module_symbols.empty()) return;
    for(const auto& sym : dispose_module_symbols) {
        if(!table.erase(sym.symbol)) {
            std::cerr << rang::fg::yellow << "[SymRes] unable to un-declare module symbol " << sym.symbol << " in module " << module_name << rang::fg::reset << std::endl;
        }
    }
}

void SymbolResolver::unsatisfied_type_err(Value* value, BaseType* type) {
    const auto val_type = value->create_type(allocator);
    if(val_type) {
        error(value) << "value with type '" << val_type->representation() << "' does not satisfy type '" << type->representation() << "'";
    } else {
        error(value) << "value does not satisfy type '" << type->representation() << "'";
    }
}