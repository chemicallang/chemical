// Copyright (c) Qinetik 2024.


#include "ast/base/ASTNode.h"
#include "SymbolResolver.h"
#include "ast/structures/MultiFunctionNode.h"
#include "ast/structures/FunctionDeclaration.h"

SymbolResolver::SymbolResolver(bool is64Bit) : ASTDiagnoser(), is64Bit(is64Bit) {

}

void SymbolResolver::dup_sym_error(const std::string& name, ASTNode* previous, ASTNode* new_node) {
    std::string err("duplicate symbol being declared " + name + " symbol already exists\n");
    err.append("previous : " + previous->representation() + "\n");
    err.append("new : " + new_node->representation() + "\n");
    error(err, new_node);
}

ASTNode *SymbolResolver::find_in_current_file(const std::string& name) {
    int i = current.size() - 1;
    while (i >= 0) {
        const auto& last = current[i];
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

ASTNode *SymbolResolver::find(const std::string &name) {
    int i = current.size() - 1;
    while (i >= 0) {
        const auto& last = current[i];
        auto found = last.symbols.find(name);
        if (found != last.symbols.end()) {
            return found->second;
        }
        i--;
    }
    return nullptr;
}

void SymbolResolver::declare(const std::string& name, ASTNode* node) {
    if(name == "_") {
        // symbols with name '_' aren't declared
        return;
    }
    auto& last = current.back();
    if(override_symbols) {
        last.symbols[name] = node;
        // since this is a file scope, we must undeclare a duplicate symbol in file scopes above
        if(last.kind == SymResScopeKind::File) {
            undeclare_in_other_files(name);
        }
    } else {
        auto found = last.symbols.find(name);
        if(found == last.symbols.end()) {
            last.symbols[name] = node;
            // since this is a file scope, we must check for duplicate symbols in file scopes above
            if(last.kind == SymResScopeKind::File) {
                dup_check_in_other_files(name, node);
            }
        } else {
            dup_sym_error(name, found->second, node);
        }
    }
}

bool SymbolResolver::undeclare(const std::string_view& name) {
    int i = current.size() - 1;
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

bool SymbolResolver::undeclare_in_other_files(const std::string& name) {
    int i = current.size() - 1;
    bool started_undeclaring = false;
    while (i >= 0) {
        auto& last = current[i];
        if(started_undeclaring) {
            auto found = last.symbols.find(name);
            if (found != last.symbols.end()) {
                last.symbols.erase(found);
                return true;
            }
        } else {
            if (last.kind == SymResScopeKind::File) {
                started_undeclaring = true;
            }
        }
        i--;
    }
    return false;
}

bool SymbolResolver::dup_check_in_other_files(const std::string& name, ASTNode* new_node) {
    int i = current.size() - 1;
    bool checking = false;
    while (i >= 0) {
        auto& last = current[i];
        if(checking) {
            auto found = last.symbols.find(name);
            if (found != last.symbols.end()) {
                dup_sym_error(name, found->second, new_node);
                return true;
            }
        } else {
            if (last.kind == SymResScopeKind::File) {
                checking = true;
            }
        }
        i--;
    }
    return false;
}

void SymbolResolver::declare_function(const std::string& name, FunctionDeclaration* declaration) {
    if(name == "_") return;
    auto& last = current.back();
    auto found = last.symbols.find(name);
    if(found == last.symbols.end()) {
        last.symbols[name] = declaration;
    } else {
        if(declaration->has_annotation(AnnotationKind::Override)) {
            const auto func = found->second->as_function();
            if (func->returnType->is_same(declaration->returnType.get()) && func->do_param_types_match(declaration->params, false)) {
                last.symbols[name] = declaration;
                return;
            } else {
                dup_sym_error(declaration->name, found->second, declaration);
                error("function " + declaration->name + " cannot override because it's parameter types and return type don't match", (AnnotableNode*) declaration);
                return;
            }
        }
        auto result = handle_name_overridable_function(name, found->second, declaration);
        if(!result.duplicates.empty()) {
            for(auto dup : result.duplicates) {
                dup_sym_error(name, dup, declaration);
            }
        } else if(result.new_multi_func_node) {
            helper_nodes.emplace_back(result.new_multi_func_node);
            // override the previous symbol
            last.symbols[name] = result.new_multi_func_node;
        }
    }
}