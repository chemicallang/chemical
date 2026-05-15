// Copyright (c) Chemical Language Foundation 2026.

#pragma once

#include <unordered_map>
#include "ast/utils/Operation.h"

class ASTNode;
class InterfaceDefinition;
class ASTAny;
class ImplDefinition;
class CoreNodes;
class FunctionDeclaration;
class MembersContainer;

struct ImplementationIndexKey {
    ASTNode* interface; // interface can be GenericInterfaceDecl or InterfaceDefinition
    ASTAny* for_; // for_ can be a type (BaseType*) or (StructDefinition, VariantDefinition or UnionDefinition...)
    bool operator==(const ImplementationIndexKey& other) const {
        return interface == other.interface && for_ == other.for_;
    }
};

struct ImplementationIndexKeyHash {
    std::size_t operator()(const ImplementationIndexKey& k) const {
        const auto h1 = std::hash<const void*>{}((void*) k.interface);
        const auto h2 = std::hash<const void*>{}((void*) k.for_);
        // Combine hashes
        return h1 ^ (h2 << 1);
    }
};

/**
 * basically all implementations are indexed in a map (interface + for struct/type)
 * why ? you can get any impl block by interface and struct/type
 * then you can use impl block to get method definitions
 * this is required for operator overloading, because a container (struct / type) contains indexed children
 * we can get child by name 'add' for example, but that maybe a direct child hiding the actual implementation of
 * 'core::ops::Add' interface, we need exact implementation of 'add', we already index 'core::ops::Add' interface
 * we can get that interface from CoreNodes (in the same directory), then get the struct from where operator is being invoked
 * and use that to get impl and from impl the actual 'add' implementation, this avoids shadowing, chooses the exact implementation
 * user intended
 * why use one map for all instantiations, because we can store a type pointer (BaseType*) too, and a Struct pointer (ASTNode*) too
 */
class ImplementationsIndex {
public:

    std::unordered_map<ImplementationIndexKey, ImplDefinition*, ImplementationIndexKeyHash> map_;

    void add(ASTNode* interface, ASTAny* for_, ImplDefinition* impl) {
        map_.emplace(ImplementationIndexKey{ interface, for_ }, impl);
    }

    /**
     * this method automatically gets the generic parent and adds that if the interface is generic
     * otherwise uses the interface as the key node
     */
    bool add_interface(ASTNode* interface, ASTAny* for_, ImplDefinition* impl);

    void add_interface(InterfaceDefinition* interface, ASTAny* for_, ImplDefinition* impl);

    ImplDefinition* get(ASTNode* interface, ASTAny* for_) {
        const auto it = map_.find(ImplementationIndexKey{ interface, for_ });
        if(it == map_.end()) return nullptr;
        return it->second;
    }

    ImplDefinition* get_impl(ASTNode* interface, ASTAny* for_);

    ImplDefinition* get_impl(InterfaceDefinition* interface, ASTAny* for_);

    void clear() {
        map_.clear();
    }

    FunctionDeclaration* get_expr_op_impl(CoreNodes& coreNodes, MembersContainer* container, Operation op);

    FunctionDeclaration* get_ass_op_impl(CoreNodes& coreNodes, MembersContainer* container, Operation op);

    FunctionDeclaration* get_inc_dec_op_impl(CoreNodes& coreNodes, MembersContainer* container, bool increment, bool post);

    FunctionDeclaration* get_index_op_impl(CoreNodes& coreNodes, MembersContainer* container);

    FunctionDeclaration* get_neg_op_impl(CoreNodes& coreNodes, MembersContainer* container);

    FunctionDeclaration* get_not_op_impl(CoreNodes& coreNodes, MembersContainer* container);

    FunctionDeclaration* get_linear_data_impl(CoreNodes& coreNodes, MembersContainer* container);

    FunctionDeclaration* get_linear_size_impl(CoreNodes& coreNodes, MembersContainer* container);

    FunctionDeclaration* get_chunked_begin_chunks_impl(CoreNodes& coreNodes, MembersContainer* container);

    FunctionDeclaration* get_chunked_valid_chunk_impl(CoreNodes& coreNodes, MembersContainer* container);

    FunctionDeclaration* get_chunked_current_chunk_impl(CoreNodes& coreNodes, MembersContainer* container);

    FunctionDeclaration* get_chunked_next_chunk_impl(CoreNodes& coreNodes, MembersContainer* container);

    FunctionDeclaration* get_chunked_rbegin_chunks_impl(CoreNodes& coreNodes, MembersContainer* container);

    FunctionDeclaration* get_chunked_previous_chunk_impl(CoreNodes& coreNodes, MembersContainer* container);

    FunctionDeclaration* get_chunked_total_size_impl(CoreNodes& coreNodes, MembersContainer* container);

    FunctionDeclaration* get_iterable_begin_impl(CoreNodes& coreNodes, MembersContainer* container);

    FunctionDeclaration* get_iterable_valid_impl(CoreNodes& coreNodes, MembersContainer* container);

    FunctionDeclaration* get_iterable_current_impl(CoreNodes& coreNodes, MembersContainer* container);

    FunctionDeclaration* get_iterable_next_impl(CoreNodes& coreNodes, MembersContainer* container);

    FunctionDeclaration* get_reversible_iterable_rbegin_impl(CoreNodes& coreNodes, MembersContainer* container);
    FunctionDeclaration* get_reversible_iterable_previous_impl(CoreNodes& coreNodes, MembersContainer* container);
    FunctionDeclaration* get_reversible_iterable_count_impl(CoreNodes& coreNodes, MembersContainer* container);

    FunctionDeclaration* get_stream_write_signed_impl(CoreNodes& coreNodes, MembersContainer* container);
    FunctionDeclaration* get_stream_write_unsigned_impl(CoreNodes& coreNodes, MembersContainer* container);
    FunctionDeclaration* get_stream_write_str_impl(CoreNodes& coreNodes, MembersContainer* container);
    FunctionDeclaration* get_stream_write_str_no_len_impl(CoreNodes& coreNodes, MembersContainer* container);
    FunctionDeclaration* get_stream_write_float_impl(CoreNodes& coreNodes, MembersContainer* container);
    FunctionDeclaration* get_stream_write_double_impl(CoreNodes& coreNodes, MembersContainer* container);
    FunctionDeclaration* get_stream_write_char_impl(CoreNodes& coreNodes, MembersContainer* container);
    FunctionDeclaration* get_stream_write_uchar_impl(CoreNodes& coreNodes, MembersContainer* container);

};
