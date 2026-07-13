// Copyright (c) Chemical Language Foundation 2026.

#include "ImplementationsIndex.h"
#include "CoreNodes.h"
#include "ast/structures/FunctionDeclaration.h"
#include "ast/structures/ImplDefinition.h"

void ImplementationsIndex::add_interface(InterfaceDefinition* interface, ASTAny* for_, ImplDefinition* impl) {
    add(interface->generic_parent != nullptr ? (ASTNode*) interface->generic_parent : interface, for_, impl);
}

bool ImplementationsIndex::add_interface(ASTNode* interface, ASTAny* for_, ImplDefinition* impl) {
    switch (interface->kind()) {
        case ASTNodeKind::InterfaceDecl: {
            const auto gen_parent = interface->as_interface_def_unsafe()->generic_parent;
            const auto index_key = gen_parent != nullptr ? (ASTNode*) gen_parent : ((ASTNode*) interface);
            add(index_key, for_, impl);
            return true;
        }
        case ASTNodeKind::GenericInterfaceDecl:
            add((ASTNode*) interface, for_, impl);
            return true;
        default:
            return false;
    }
}

ImplDefinition* ImplementationsIndex::get_impl(InterfaceDefinition* interface, ASTAny* for_) {
    return get(interface->generic_parent != nullptr ? (ASTNode*) interface->generic_parent : interface, for_);
}

ImplDefinition* ImplementationsIndex::get_impl(ASTNode* interface, ASTAny* for_) {
    switch (interface->kind()) {
        case ASTNodeKind::InterfaceDecl: {
            const auto gen_parent = interface->as_interface_def_unsafe()->generic_parent;
            const auto index_key = gen_parent != nullptr ? (ASTNode*) gen_parent : ((ASTNode*) interface);
            return get(index_key, for_);
        }
        case ASTNodeKind::GenericInterfaceDecl:
            return get((ASTNode*) interface, for_);
        default:
            return nullptr;
    }
}

static FunctionDeclaration* implementation_of(const ImplementationsIndex& index, FunctionDeclaration* op_base, ASTAny* for_type) {
    if (op_base == nullptr) {
        return nullptr;
    }
    const auto implBlock = index.get(op_base->parent(), for_type);
    if (implBlock == nullptr) {
        return nullptr;
    }
    return implBlock->implementation_of(op_base);
}

FunctionDeclaration* ImplementationsIndex::get_expr_op_impl(const CoreNodes& coreNodes, MembersContainer* container, Operation op) const {
    return implementation_of(*this, coreNodes.expr_operator_impl_base(op), (ASTAny*) container);
}

FunctionDeclaration* ImplementationsIndex::get_ass_op_impl(const CoreNodes& coreNodes, MembersContainer* container, Operation op) const {
    return implementation_of(*this, coreNodes.assignment_operator_impl_base(op), (ASTAny*) container);
}

FunctionDeclaration* ImplementationsIndex::get_inc_dec_op_impl(const CoreNodes& coreNodes, MembersContainer* container, bool increment, bool post) const {
    return implementation_of(*this, coreNodes.inc_dec_operator_impl_base(increment, post), (ASTAny*) container);
}

FunctionDeclaration* ImplementationsIndex::get_index_op_impl(const CoreNodes& coreNodes, MembersContainer* container) const {
    const auto indexFunc = coreNodes.ops.index;
    if (indexFunc != nullptr) {
        const auto implPresent = get(indexFunc->parent(), container);
        if (implPresent) {
            return implPresent->implementation_of(indexFunc);
        }
    }
    const auto mutIndexFunc = coreNodes.ops.index_mut;
    if (mutIndexFunc) {
        const auto implPresent = get(mutIndexFunc->parent(), container);
        if (implPresent) {
            return implPresent->implementation_of(mutIndexFunc);
        }
    }
    return nullptr;
}

FunctionDeclaration* ImplementationsIndex::get_neg_op_impl(const CoreNodes& coreNodes, MembersContainer* container) const {
    return implementation_of(*this, coreNodes.ops.neg, (ASTAny*) container);
}

FunctionDeclaration* ImplementationsIndex::get_not_op_impl(const CoreNodes& coreNodes, MembersContainer* container) const {
    return implementation_of(*this, coreNodes.ops._not, (ASTAny*) container);
}

FunctionDeclaration* ImplementationsIndex::get_bitnot_op_impl(const CoreNodes& coreNodes, MembersContainer* container) const {
    return implementation_of(*this, coreNodes.ops.bitnot, (ASTAny*) container);
}

FunctionDeclaration* ImplementationsIndex::get_linear_data_impl(const CoreNodes& coreNodes, MembersContainer* container) const {
    return implementation_of(*this, coreNodes.iterable.linear_data, (ASTAny*) container);
}

FunctionDeclaration* ImplementationsIndex::get_linear_size_impl(const CoreNodes& coreNodes, MembersContainer* container) const {
    return implementation_of(*this, coreNodes.iterable.linear_size, (ASTAny*) container);
}

FunctionDeclaration* ImplementationsIndex::get_chunked_begin_chunks_impl(const CoreNodes& coreNodes, MembersContainer* container) const {
    return implementation_of(*this, coreNodes.iterable.chunked_begin_chunks, (ASTAny*) container);
}

FunctionDeclaration* ImplementationsIndex::get_chunked_valid_chunk_impl(const CoreNodes& coreNodes, MembersContainer* container) const {
    return implementation_of(*this, coreNodes.iterable.chunked_valid_chunk, (ASTAny*) container);
}

FunctionDeclaration* ImplementationsIndex::get_chunked_current_chunk_impl(const CoreNodes& coreNodes, MembersContainer* container) const {
    return implementation_of(*this, coreNodes.iterable.chunked_current_chunk, (ASTAny*) container);
}

FunctionDeclaration* ImplementationsIndex::get_chunked_next_chunk_impl(const CoreNodes& coreNodes, MembersContainer* container) const {
    return implementation_of(*this, coreNodes.iterable.chunked_next_chunk, (ASTAny*) container);
}

FunctionDeclaration* ImplementationsIndex::get_chunked_rbegin_chunks_impl(const CoreNodes& coreNodes, MembersContainer* container) const {
    return implementation_of(*this, coreNodes.iterable.chunked_rbegin_chunks, (ASTAny*) container);
}

FunctionDeclaration* ImplementationsIndex::get_chunked_previous_chunk_impl(const CoreNodes& coreNodes, MembersContainer* container) const {
    return implementation_of(*this, coreNodes.iterable.chunked_previous_chunk, (ASTAny*) container);
}

FunctionDeclaration* ImplementationsIndex::get_chunked_total_size_impl(const CoreNodes& coreNodes, MembersContainer* container) const {
    return implementation_of(*this, coreNodes.iterable.chunked_total_size, (ASTAny*) container);
}

FunctionDeclaration* ImplementationsIndex::get_iterable_begin_impl(const CoreNodes& coreNodes, MembersContainer* container) const {
    return implementation_of(*this, coreNodes.iterable.iterable_begin, (ASTAny*) container);
}

FunctionDeclaration* ImplementationsIndex::get_iterable_valid_impl(const CoreNodes& coreNodes, MembersContainer* container) const {
    return implementation_of(*this, coreNodes.iterable.iterable_valid, (ASTAny*) container);
}

FunctionDeclaration* ImplementationsIndex::get_iterable_current_impl(const CoreNodes& coreNodes, MembersContainer* container) const {
    return implementation_of(*this, coreNodes.iterable.iterable_current, (ASTAny*) container);
}

FunctionDeclaration* ImplementationsIndex::get_iterable_next_impl(const CoreNodes& coreNodes, MembersContainer* container) const {
    return implementation_of(*this, coreNodes.iterable.iterable_next, (ASTAny*) container);
}

FunctionDeclaration* ImplementationsIndex::get_reversible_iterable_rbegin_impl(const CoreNodes& coreNodes, MembersContainer* container) const {
    return implementation_of(*this, coreNodes.iterable.reversible_iterable_rbegin, (ASTAny*) container);
}

FunctionDeclaration* ImplementationsIndex::get_reversible_iterable_previous_impl(const CoreNodes& coreNodes, MembersContainer* container) const {
    return implementation_of(*this, coreNodes.iterable.reversible_iterable_previous, (ASTAny*) container);
}

FunctionDeclaration* ImplementationsIndex::get_reversible_iterable_count_impl(const CoreNodes& coreNodes, MembersContainer* container) const {
    return implementation_of(*this, coreNodes.iterable.reversible_iterable_count, (ASTAny*) container);
}

FunctionDeclaration* ImplementationsIndex::get_stream_write_signed_impl(const CoreNodes& coreNodes, MembersContainer* container) const {
    return implementation_of(*this, coreNodes.stream.stream_write_signed, (ASTAny*) container);
}

FunctionDeclaration* ImplementationsIndex::get_stream_write_unsigned_impl(const CoreNodes& coreNodes, MembersContainer* container) const {
    return implementation_of(*this, coreNodes.stream.stream_write_unsigned, (ASTAny*) container);
}

FunctionDeclaration* ImplementationsIndex::get_stream_write_str_impl(const CoreNodes& coreNodes, MembersContainer* container) const {
    return implementation_of(*this, coreNodes.stream.stream_write_str, (ASTAny*) container);
}

FunctionDeclaration* ImplementationsIndex::get_stream_write_str_no_len_impl(const CoreNodes& coreNodes, MembersContainer* container) const {
    return implementation_of(*this, coreNodes.stream.stream_write_str_no_len, (ASTAny*) container);
}

FunctionDeclaration* ImplementationsIndex::get_stream_write_float_impl(const CoreNodes& coreNodes, MembersContainer* container) const {
    return implementation_of(*this, coreNodes.stream.stream_write_float, (ASTAny*) container);
}

FunctionDeclaration* ImplementationsIndex::get_stream_write_double_impl(const CoreNodes& coreNodes, MembersContainer* container) const {
    return implementation_of(*this, coreNodes.stream.stream_write_double, (ASTAny*) container);
}

FunctionDeclaration* ImplementationsIndex::get_stream_write_char_impl(const CoreNodes& coreNodes, MembersContainer* container) const {
    return implementation_of(*this, coreNodes.stream.stream_write_char, (ASTAny*) container);
}

FunctionDeclaration* ImplementationsIndex::get_stream_write_uchar_impl(const CoreNodes& coreNodes, MembersContainer* container) const {
    return implementation_of(*this, coreNodes.stream.stream_write_uchar, (ASTAny*) container);
}
