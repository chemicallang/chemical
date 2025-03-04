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
) : comptime_scope(global), ASTDiagnoser(global.loc_man), is64Bit(is64Bit), allocator(fileAllocator), mod_allocator(modAllocator), ast_allocator(astAllocator) {
    current.emplace_back(SymResScopeKind::Global);
    dispose_file_symbols.reserve(100);
    dispose_module_symbols.reserve(100);
}

void SymbolResolver::dup_sym_error(const chem::string_view& name, ASTNode* previous, ASTNode* new_node) {
    error(new_node) << "duplicate symbol being declared, symbol '" << name << "' already exists";
}

void SymbolResolver::dup_runtime_sym_error(const chem::string_view& name, ASTNode* previous, ASTNode* new_node) {
    error(new_node) << "duplicate runtime symbol being declared " << name << " symbol already exists";
}

ASTNode *SymbolResolver::find_in_current_file(const chem::string_view& name) {
    int i = ((int) current.size()) - 1;
    while (i >= 0) {
        auto& last = current[i];
        auto found = last.symbols.find(name);
        if (found != last.symbols.end()) {
            return found->second;
        }
        if(last.kind == SymResScopeKind::File) {
            // we didn't find the symbol in the last file scope
            // so no need to search more
            return nullptr;
        }
        i--;
    }
    return nullptr;
}

ASTNode *SymbolResolver::find(const chem::string_view &name) {
    // TODO find method always finds every symbol (even private symbols of other files)
    // we declare private symbols as well with the top level symbols (of different files)
    // this allows symbols in files above to link with private symbols of files below
    int i = (int) current.size() - 1;
    while (i >= 0) {
        auto& last = current[i];
        auto found = last.symbols.find(name);
        if (found != last.symbols.end()) {
            return found->second;
        }
        i--;
    }
    return nullptr;
}

bool SymbolResolver::declare_quietly(const chem::string_view& name, ASTNode* node) {
    if(name == "_") {
        // symbols with name '_' aren't declared
        return false;
    }
    auto& last = current.back();
    auto found = last.symbols.find(name);
    if(found == last.symbols.end()) {
        last.symbols[name] = node;
        // since this is a file scope, we must check for duplicate symbols in file scopes above
        if(last.kind == SymResScopeKind::File) {
            dup_check_in_scopes_above(name, node, (int) current.size() - 2);
        }
        return true;
    } else {
        dup_sym_error(name, found->second, node);
        return false;
    }
}

void SymbolResolver::declare_exported_runtime(const chem::string_view& name, const chem::string_view& runtime_name, ASTNode* node) {
    declare_quietly(name, node);
    declare_runtime(runtime_name, node);
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

bool SymbolResolver::undeclare(const chem::string_view& name) {
    int i = ((int) current.size()) - 1;
    while (i >= 0) {
        auto& last = current[i];
        auto found = last.symbols.find(name);
        if (found != last.symbols.end()) {
            last.symbols.erase(found);
            return true;
        }
        i--;
    }
    return false;
}

bool SymbolResolver::undeclare_in_current_file(const chem::string_view& name) {
    auto& last_scope = current.back();
    if(last_scope.kind == SymResScopeKind::File) {
        return last_scope.symbols.erase(name) > 0;
    } else {
#ifdef DEBUG
        throw std::runtime_error("undeclare in current file, while current file is not a file");
#else
        return false;
#endif
    }
//    int i = current.size() - 1;
//    while (i >= 0) {
//        auto& last = current[i];
//        if(last.symbols.erase(name) > 0) {
//            return true;
//        }
//        if(last.kind == SymResScopeKind::File) {
//            return false;
//        }
//        i--;
//    }
//    return false;
}

bool SymbolResolver::undeclare_in_current_module(const chem::string_view& name) {
    int i = (int) current.size() - 1;
    while (i >= 0) {
        auto& scope = current[i];
        auto found = scope.symbols.find(name);
        if (found != scope.symbols.end()) {
            scope.symbols.erase(found);
            return true;
        }
        if(scope.kind == SymResScopeKind::Module) {
            return false;
        }
        i--;
    }
    return false;
}

bool SymbolResolver::undeclare_in_scopes_above(const chem::string_view& name, int i) {
    while (i >= 0) {
        auto& scope = current[i];
        auto found = scope.symbols.find(name);
        if (found != scope.symbols.end()) {
            scope.symbols.erase(found);
            return true;
        }
        i--;
    }
    return false;
}

bool SymbolResolver::dup_check_in_scopes_above(const chem::string_view& name, ASTNode* new_node, int i) {
    while (i >= 0) {
        auto& scope = current[i];
        auto found = scope.symbols.find(name);
        if (found != scope.symbols.end()) {
            dup_sym_error(name, found->second, new_node);
            return true;
        }
        i--;
    }
    return false;
}

bool SymbolResolver::overload_function(const chem::string_view& name, ASTNode*& previous, FunctionDeclaration* declaration) {
    if(declaration->is_override()) {
        const auto func = previous->as_function();
        if (func->returnType->is_same(declaration->returnType) && func->do_param_types_match(declaration->params, false)) {
            previous = declaration;
            return true;
        } else {
            dup_sym_error(declaration->name_view(), previous, declaration);
            error((AnnotableNode*) declaration) << "function " << declaration->name_view() << " cannot override because it's parameter types and return type don't match";
            return false;
        }
    }
    auto result = handle_name_overload_function(name, previous, declaration);
    if(!result.duplicates.empty()) {
        for(auto dup : result.duplicates) {
            dup_sym_error(name, dup, declaration);
        }
    } else if(result.new_multi_func_node) {
        helper_nodes.emplace_back(result.new_multi_func_node);
        // override the previous symbol
        previous = result.new_multi_func_node;
        return true;
    }
    return false;
}

ASTNode** SymbolResolver::declare_function_no_overload(const chem::string_view& name, FunctionDeclaration* declaration) {
    auto& last = current.back();
    auto found = last.symbols.find(name);
    if(found == last.symbols.end()) {
        last.symbols[name] = declaration;
        return nullptr;
    } else {
        return &found->second;
    }
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
    auto& scope = current.back();
    scope.symbols[name] = node;
    // since this is a file scope, we must undeclare a duplicate symbol in file scopes above
    if(scope.kind == SymResScopeKind::File) {
        undeclare_in_scopes_above(name, (int) current.size() - 2);
    }
    if(scope.kind == SymResScopeKind::File) { // only top level scope symbols are disposed at module's end
        dispose_module_symbols.emplace_back(current.size() - 1, name);
    }
}

void SymbolResolver::declare(const chem::string_view &name, ASTNode *node) {
#ifdef DEBUG
    if(name.empty()) {
        std::cerr << rang::fg::red << "empty symbol being declared" << rang::fg::reset << std::endl;
        return;
    }
#endif
    if(declare_quietly(name, node)) {
        auto& scope = current.back();
        if (scope.kind == SymResScopeKind::File) { // only top level scope symbols are disposed at module's end
            dispose_module_symbols.emplace_back(current.size() - 1, name);
        }
    }
}

void SymbolResolver::declare_file_disposable(const chem::string_view &name, ASTNode *node) {
    if(declare_quietly(name, node)) {
        auto& scope = current.back();
        if (scope.kind == SymResScopeKind::File) {
            dispose_file_symbols.emplace_back(SymbolRef{current.size() - 1, name}, node);
        }
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
    auto& scope = current.back();
    if(new_sym && scope.kind == SymResScopeKind::File) { // only top level scope symbols are disposed at module's end
        dispose_module_symbols.emplace_back(current.size() - 1, name);
    }
}

void SymbolResolver::declare_private_function(const chem::string_view& name, FunctionDeclaration* declaration) {
    const auto new_sym = declare_function_quietly(name, declaration);
    auto& scope = current.back();
    if(new_sym && scope.kind == SymResScopeKind::File) {
        dispose_file_symbols.emplace_back(SymbolRef { current.size() - 1, name }, declaration);
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
    auto& scope = current[range.scope_index];
    auto sym = dispose_file_symbols.data() + range.symbol_start;
    const auto end = dispose_file_symbols.data() + range.symbol_end;
    while(sym != end) {
        scope.symbols[sym->symbol] = sym->node;
        sym++;
    }
}

void SymbolResolver::dispose_file_symbols_now(const std::string_view& abs_path, const SymbolRange& range) {
    if(range.symbol_start == range.symbol_end) return;
    auto& scope = current[range.scope_index];
    auto sym = dispose_file_symbols.data() + range.symbol_start;
    const auto end = dispose_file_symbols.data() + range.symbol_end;
    while(sym != end) {
        if(scope.symbols.erase(sym->symbol) <= 0) {
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
    auto range = SymbolRange { (unsigned int) scope_index, (unsigned int) start, (unsigned int) end };
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
    // dispose symbols of previous file
    auto& last_scope = current.back();
#ifdef DEBUG
    if(last_scope.kind != SymResScopeKind::File) {
        throw std::runtime_error("undeclare in current file, while current file is not a file");
    }
#endif
    for(const auto& sym : dispose_file_symbols) {
        auto& scope = current[sym.scope_index];
        auto& sym_name = sym.symbol;
        if(scope.symbols.erase(sym_name) <= 0) {
            std::cerr << rang::fg::yellow << "[SymRes] unable to un-declare file symbol " << sym_name << " in file " << abs_path  << rang::fg::reset << std::endl;
        }
    }
    dispose_file_symbols.clear();
}

void SymbolResolver::dispose_module_symbols_now(const std::string& module_name) {
    if(dispose_module_symbols.empty()) return;
    for(const auto& sym : dispose_module_symbols) {
        auto& scope = current[sym.scope_index];
        auto& sym_name = sym.symbol;
        if(scope.symbols.erase(sym_name) <= 0) {
            std::cerr << rang::fg::yellow << "[SymRes] unable to un-declare module symbol " << sym_name << " in module " << module_name << rang::fg::reset << std::endl;
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