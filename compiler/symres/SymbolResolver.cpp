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
#include "GenericInstantiationPass.h"
#include "SymResLinkBodyAPI.h"
#include "ast/statements/UnresolvedDecl.h"
#include "ast/statements/ChildrenMapNode.h"
#include "ast/base/TypeBuilder.h"
#include "ast/structures/InterfaceDefinition.h"

SymbolResolver::SymbolResolver(
    CompilerBinder& binder,
    GlobalInterpretScope& global,
    ImportPathHandler& handler,
    AnnotationController& controller,
    InstantiationsContainer& container,
    CoreNodes& coreNodes,
    ImplementationsIndex& implsIndex,
    bool is64Bit,
    ASTAllocator& fileAllocator,
    ASTAllocator* modAllocator,
    ASTAllocator* astAllocator
) : binder(binder), comptime_scope(global), path_handler(handler), instContainer(container), ASTDiagnoser(global.loc_man), is64Bit(is64Bit),
    allocator(fileAllocator), mod_allocator(modAllocator), ast_allocator(astAllocator), controller(controller), coreNodes(coreNodes), implsIndex(implsIndex),
    genericInstantiator(controller, binder, child_resolver, container, coreNodes, implsIndex, *astAllocator, *this, global.typeBuilder, global.target_data), table(512)
{
    global_scope_start();
    stored_file_symbols.reserve(128);
}

UnresolvedDecl* SymbolResolver::get_unresolved_decl() {
    return comptime_scope.typeBuilder.getUnresolvedDecl();
}

static FunctionDeclaration* func_of_interface(ASTNode* container, const chem::string_view& interface, const chem::string_view& method) {
    const auto found = container->child(interface);
    if (found == nullptr) return nullptr;
    // this interface can be a generic interface
    // we can search for child anyway
    const auto child = found->child(method);
    if (child == nullptr || child->kind() != ASTNodeKind::FunctionDecl) return nullptr;
    return child->as_function_unsafe();
}

void SymbolResolver::link_core_nodes() {

    // lookup the Copy marker interface
    {
        const auto copyNode = find("Copy");
        if(copyNode && copyNode->kind() == ASTNodeKind::InterfaceDecl) {
            coreNodes.copy_interface = copyNode->as_interface_def_unsafe();
            coreNodes.copy_interface->interface_bits.set(InterfaceBits::COPY_BIT);
        }
    }

    const auto coreNode = find("core");
    if (coreNode == nullptr || coreNode->kind() != ASTNodeKind::NamespaceDecl) return;
    if (coreNode->as_namespace_unsafe()->specifier() != AccessSpecifier::Public) return;

    const auto opsNode = coreNode->child("ops");
    if (opsNode == nullptr || opsNode->kind() != ASTNodeKind::NamespaceDecl) return;
    if (opsNode->as_namespace_unsafe()->specifier() != AccessSpecifier::Public) return;

    coreNodes.ops.add = func_of_interface(opsNode, "Add", "add");
    coreNodes.ops.sub = func_of_interface(opsNode, "Sub", "sub");
    coreNodes.ops.mul = func_of_interface(opsNode, "Mul", "mul");
    coreNodes.ops.div = func_of_interface(opsNode, "Div", "div");
    coreNodes.ops.rem = func_of_interface(opsNode, "Rem", "rem");
    coreNodes.ops.neg = func_of_interface(opsNode, "Neg", "neg");
    coreNodes.ops._not = func_of_interface(opsNode, "Not", "not");
    coreNodes.ops.bitnot = func_of_interface(opsNode, "BitNot", "bitnot");

    coreNodes.ops.add_assign = func_of_interface(opsNode, "AddAssign", "add_assign");
    coreNodes.ops.sub_assign = func_of_interface(opsNode, "SubAssign", "sub_assign");
    coreNodes.ops.mul_assign = func_of_interface(opsNode, "MulAssign", "mul_assign");
    coreNodes.ops.div_assign = func_of_interface(opsNode, "DivAssign", "div_assign");
    coreNodes.ops.rem_assign = func_of_interface(opsNode, "RemAssign", "rem_assign");

    coreNodes.ops.bit_and_assign = func_of_interface(opsNode, "BitAndAssign", "bitand_assign");
    coreNodes.ops.bit_or_assign = func_of_interface(opsNode, "BitOrAssign", "bitor_assign");
    coreNodes.ops.bit_xor_assign = func_of_interface(opsNode, "BitXorAssign", "bitxor_assign");

    coreNodes.ops.shl_assign = func_of_interface(opsNode, "ShlAssign", "shl_assign");
    coreNodes.ops.shr_assign = func_of_interface(opsNode, "ShrAssign", "shr_assign");

    coreNodes.ops.bit_and = func_of_interface(opsNode, "BitAnd", "bitand");
    coreNodes.ops.bit_or = func_of_interface(opsNode, "BitOr", "bitor");
    coreNodes.ops.bit_xor = func_of_interface(opsNode, "BitXor", "bitxor");

    coreNodes.ops.shl = func_of_interface(opsNode, "Shl", "shl");
    coreNodes.ops.shr = func_of_interface(opsNode, "Shr", "shr");

    coreNodes.ops.eq = func_of_interface(opsNode, "PartialEq", "eq");
    coreNodes.ops.ne = func_of_interface(opsNode, "PartialEq", "ne");

    coreNodes.ops.gt = func_of_interface(opsNode, "Ord", "gt");
    coreNodes.ops.lt = func_of_interface(opsNode, "Ord", "lt");
    coreNodes.ops.gte = func_of_interface(opsNode, "Ord", "gte");
    coreNodes.ops.lte = func_of_interface(opsNode, "Ord", "lte");

    coreNodes.ops.inc_pre = func_of_interface(opsNode, "Increment", "inc_pre");
    coreNodes.ops.inc_post = func_of_interface(opsNode, "Increment", "inc_post");
    coreNodes.ops.dec_pre = func_of_interface(opsNode, "Decrement", "dec_pre");
    coreNodes.ops.dec_post = func_of_interface(opsNode, "Decrement", "dec_post");

    coreNodes.ops.index = func_of_interface(opsNode, "Index", "index");
    coreNodes.ops.index_mut = func_of_interface(opsNode, "IndexMut", "index");

    const auto iterableNode = coreNode->child("iterable");
    if (iterableNode == nullptr || iterableNode->kind() != ASTNodeKind::NamespaceDecl) return;
    if (iterableNode->as_namespace_unsafe()->specifier() != AccessSpecifier::Public) return;

    coreNodes.iterable.linear_data = func_of_interface(iterableNode, "Linear", "data");
    coreNodes.iterable.linear_size = func_of_interface(iterableNode, "Linear", "size");

    coreNodes.iterable.chunked_begin_chunks = func_of_interface(iterableNode, "Chunked", "begin_chunks");
    coreNodes.iterable.chunked_valid_chunk = func_of_interface(iterableNode, "Chunked", "valid_chunk");
    coreNodes.iterable.chunked_current_chunk = func_of_interface(iterableNode, "Chunked", "current_chunk");
    coreNodes.iterable.chunked_next_chunk = func_of_interface(iterableNode, "Chunked", "next_chunk");
    coreNodes.iterable.chunked_rbegin_chunks = func_of_interface(iterableNode, "Chunked", "rbegin_chunks");
    coreNodes.iterable.chunked_previous_chunk = func_of_interface(iterableNode, "Chunked", "previous_chunk");
    coreNodes.iterable.chunked_total_size = func_of_interface(iterableNode, "Chunked", "total_size");

    coreNodes.iterable.iterable_begin = func_of_interface(iterableNode, "Iterable", "begin");
    coreNodes.iterable.iterable_valid = func_of_interface(iterableNode, "Iterable", "valid");
    coreNodes.iterable.iterable_current = func_of_interface(iterableNode, "Iterable", "current");
    coreNodes.iterable.iterable_next = func_of_interface(iterableNode, "Iterable", "next");

    coreNodes.iterable.reversible_iterable_rbegin = func_of_interface(iterableNode, "ReversibleIterable", "rbegin");
    coreNodes.iterable.reversible_iterable_previous = func_of_interface(iterableNode, "ReversibleIterable", "previous");
    coreNodes.iterable.reversible_iterable_count = func_of_interface(iterableNode, "ReversibleIterable", "count");

    const auto streamNode = coreNode->child("stream");
    if (streamNode == nullptr || streamNode->kind() != ASTNodeKind::NamespaceDecl) return;
    if (streamNode->as_namespace_unsafe()->specifier() != AccessSpecifier::Public) return;

    coreNodes.stream.stream_write_signed = func_of_interface(streamNode, "Stream", "writeSigned");
    coreNodes.stream.stream_write_unsigned = func_of_interface(streamNode, "Stream", "writeUnsigned");
    coreNodes.stream.stream_write_str = func_of_interface(streamNode, "Stream", "writeStr");
    coreNodes.stream.stream_write_str_no_len = func_of_interface(streamNode, "Stream", "writeStrNoLen");
    coreNodes.stream.stream_write_float = func_of_interface(streamNode, "Stream", "writeFloat");
    coreNodes.stream.stream_write_double = func_of_interface(streamNode, "Stream", "writeDouble");
    coreNodes.stream.stream_write_char = func_of_interface(streamNode, "Stream", "writeChar");
    coreNodes.stream.stream_write_uchar = func_of_interface(streamNode, "Stream", "writeUChar");

}

void SymbolResolver::dup_sym_error(const chem::string_view& name, ASTNode* previous, ASTNode* new_node) {
    error(new_node) << "duplicate symbol being declared, symbol '" << name << "' already exists";
    warn(previous) << "symbol has a conflict";
}

bool SymbolResolver::declare_default(const chem::string_view& name, ASTNode* node) {
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

static void append_parts(Diag& diag, const std::span<chem::string_view>& parts) {
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

static void append_alias_and_part(Diag& d, const std::span<chem::string_view>& parts, const chem::string_view& alias) {
    if(parts.size() > 1) {
        d << " part from '";
        append_parts(d, parts);
        d << "'";
    }
    append_alias(d, alias);
}

void SymbolResolver::declareImportedSymbol(
        ChildrenMapNode* node,
        const std::span<chem::string_view>& parts,
        const chem::string_view& alias,
        SourceLocation loc
) {
    auto& symbols = node->symbols;
    auto found = symbols.find(parts[0]);
    if(found == symbols.end()) {
        auto& d = error(loc) << "couldn't find symbol '" << parts[0] << "'";
        append_alias_and_part(d, parts, alias);
        return;
    }
    if(parts.size() == 1) {
        // import { file as f } from std <-- single part (file), alias 'f' or 'file' (the first part)
        auto& name = alias.empty() ? parts[0] : alias;
        declare(name, found->second);
        return;
    }
    // import { file.change as c } from std <--- multiple parts, alias 'c' or 'change' (the last part)
    ASTNode* current = found->second;
    auto start = parts.data() + 1;
    const auto end = parts.data() + parts.size();
    while (start != end) {
        const auto child = current->child(*start);
        if(child == nullptr) {
            auto& d = error(loc) << "couldn't find child symbol '" << *start << "'";
            append_alias_and_part(d, parts, alias);
            current = nullptr;
            break;
        }
        current = child;
        start++;
    }
    if(current) {
        declare(alias.empty() ? *(end - 1) : alias, current);
    }
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
        if(previous->activeNode->is_member_or_top_level()) {
            // previous symbol is a top level symbol or a member (function or struct/variant member), allow shadowing
            table.declare(name, node);
            return;
        }
        // error out, symbol now allowed to be shadowed
        error(node) << "symbol with name '" << name << "' already exists";
        warn(previous->activeNode) << "symbol has a conflict";
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
        warn(previous) << "symbol has a conflict";
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
            declare_quietly(name, node);
            return;
    }
}

void SymbolResolver::declare_function(const chem::string_view& name, FunctionDeclaration* decl, AccessSpecifier specifier) {
    switch(specifier) {
        case AccessSpecifier::Private:
            declare_private_function(name, decl);
            return;
        case AccessSpecifier::Public:
            declare_exported_function(name, decl);
            return;
        case AccessSpecifier::Internal:
        case AccessSpecifier::Protected:
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
    const auto start = stored_file_symbols.size();
    TopLevelDeclSymDeclare declarer(*this);
    declarer.VisitScope(&scope);
    const auto end = stored_file_symbols.size();
    return SymbolRange { (unsigned int) start, (unsigned int) end };
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
}

void SymbolResolver::generic_instantiation_file(
        Scope& scope,
        unsigned int fileId,
        const SymbolRange& range
) {
    instContainer.current_file_id = fileId;
    const auto scope_index = file_scope_start();
    enable_file_symbols(range);
    sym_res_generic_instantiation(*this, &scope);
    file_scope_end(scope_index);
}

void SymbolResolver::after_link_signature_file(
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
    sym_res_after_signature(*this, &scope);
    file_scope_end(scope_index);
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
    const auto start = stored_file_symbols.size();
    TopLevelDeclSymDeclare declarer(*this);
    declarer.VisitScope(&scope);
    const auto end = stored_file_symbols.size();
    auto range = SymbolRange { (unsigned int) start, (unsigned int) end };
    enable_file_symbols(range);
    sym_res_signature(*this, &scope);
    sym_res_generic_instantiation(*this, &scope);
    sym_res_after_signature(*this, &scope);
    sym_res_link_body(*this, &scope);
    file_scope_end(scope_index);
}

void SymbolResolver::import_file(std::vector<ASTNode*>& nodes, const std::string_view& path, bool restrict_public) {
    file_scope_start();
    for(const auto node : nodes) {
        const auto requested_specifier = node->specifier();
        const auto specifier = restrict_public ? requested_specifier == AccessSpecifier::Public ? AccessSpecifier::Internal : requested_specifier :  requested_specifier;
        auto id = node->get_node_identifier();
        if(!id.empty() && specifier != AccessSpecifier::Private) {
            declare_node(id, node, specifier, true);
        }
    }
    print_diagnostics(chem::string_view(path), "SymRes");
    diagnostics.clear();
}

void SymbolResolver::unsatisfied_type_err(Value* value, BaseType* type) {
    ::unsatisfied_type_err(*this, allocator, value, type);
}
