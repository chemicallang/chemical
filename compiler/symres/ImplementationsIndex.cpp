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

static FunctionDeclaration* implementation_of(ImplementationsIndex& index, FunctionDeclaration* op_base, ASTAny* for_type) {
    if (op_base == nullptr) {
        return nullptr;
    }
    const auto implBlock = index.get(op_base->parent(), for_type);
    if (implBlock == nullptr) {
        return nullptr;
    }
    return implBlock->implementation_of(op_base);
}

FunctionDeclaration* ImplementationsIndex::get_expr_op_impl(CoreNodes& coreNodes, MembersContainer* container, Operation op) {
    return implementation_of(*this, coreNodes.expr_operator_impl_base(op), (ASTAny*) container);
}

FunctionDeclaration* ImplementationsIndex::get_ass_op_impl(CoreNodes& coreNodes, MembersContainer* container, Operation op) {
    return implementation_of(*this, coreNodes.assignment_operator_impl_base(op), (ASTAny*) container);
}

FunctionDeclaration* ImplementationsIndex::get_inc_dec_op_impl(CoreNodes& coreNodes, MembersContainer* container, bool increment, bool post) {
    return implementation_of(*this, coreNodes.inc_dec_operator_impl_base(increment, post), (ASTAny*) container);
}

FunctionDeclaration* ImplementationsIndex::get_index_op_impl(CoreNodes& coreNodes, MembersContainer* container) {
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

FunctionDeclaration* ImplementationsIndex::get_neg_op_impl(CoreNodes& coreNodes, MembersContainer* container) {
    return implementation_of(*this, coreNodes.ops.neg, (ASTAny*) container);
}

FunctionDeclaration* ImplementationsIndex::get_not_op_impl(CoreNodes& coreNodes, MembersContainer* container) {
    return implementation_of(*this, coreNodes.ops._not, (ASTAny*) container);
}

FunctionDeclaration* ImplementationsIndex::get_linear_data_impl(CoreNodes& coreNodes, MembersContainer* container) {
    return implementation_of(*this, coreNodes.iterable.linear_data, (ASTAny*) container);
}

FunctionDeclaration* ImplementationsIndex::get_linear_size_impl(CoreNodes& coreNodes, MembersContainer* container) {
    return implementation_of(*this, coreNodes.iterable.linear_size, (ASTAny*) container);
}

FunctionDeclaration* ImplementationsIndex::get_chunked_begin_chunks_impl(CoreNodes& coreNodes, MembersContainer* container) {
    return implementation_of(*this, coreNodes.iterable.chunked_begin_chunks, (ASTAny*) container);
}

FunctionDeclaration* ImplementationsIndex::get_chunked_valid_chunk_impl(CoreNodes& coreNodes, MembersContainer* container) {
    return implementation_of(*this, coreNodes.iterable.chunked_valid_chunk, (ASTAny*) container);
}

FunctionDeclaration* ImplementationsIndex::get_chunked_current_chunk_impl(CoreNodes& coreNodes, MembersContainer* container) {
    return implementation_of(*this, coreNodes.iterable.chunked_current_chunk, (ASTAny*) container);
}

FunctionDeclaration* ImplementationsIndex::get_chunked_next_chunk_impl(CoreNodes& coreNodes, MembersContainer* container) {
    return implementation_of(*this, coreNodes.iterable.chunked_next_chunk, (ASTAny*) container);
}

FunctionDeclaration* ImplementationsIndex::get_chunked_rbegin_chunks_impl(CoreNodes& coreNodes, MembersContainer* container) {
    return implementation_of(*this, coreNodes.iterable.chunked_rbegin_chunks, (ASTAny*) container);
}

FunctionDeclaration* ImplementationsIndex::get_chunked_previous_chunk_impl(CoreNodes& coreNodes, MembersContainer* container) {
    return implementation_of(*this, coreNodes.iterable.chunked_previous_chunk, (ASTAny*) container);
}

FunctionDeclaration* ImplementationsIndex::get_chunked_total_size_impl(CoreNodes& coreNodes, MembersContainer* container) {
    return implementation_of(*this, coreNodes.iterable.chunked_total_size, (ASTAny*) container);
}

FunctionDeclaration* ImplementationsIndex::get_iterable_begin_impl(CoreNodes& coreNodes, MembersContainer* container) {
    return implementation_of(*this, coreNodes.iterable.iterable_begin, (ASTAny*) container);
}

FunctionDeclaration* ImplementationsIndex::get_iterable_valid_impl(CoreNodes& coreNodes, MembersContainer* container) {
    return implementation_of(*this, coreNodes.iterable.iterable_valid, (ASTAny*) container);
}

FunctionDeclaration* ImplementationsIndex::get_iterable_current_impl(CoreNodes& coreNodes, MembersContainer* container) {
    return implementation_of(*this, coreNodes.iterable.iterable_current, (ASTAny*) container);
}

FunctionDeclaration* ImplementationsIndex::get_iterable_next_impl(CoreNodes& coreNodes, MembersContainer* container) {
    return implementation_of(*this, coreNodes.iterable.iterable_next, (ASTAny*) container);
}

FunctionDeclaration* ImplementationsIndex::get_reversible_iterable_rbegin_impl(CoreNodes& coreNodes, MembersContainer* container) {
    return implementation_of(*this, coreNodes.iterable.reversible_iterable_rbegin, (ASTAny*) container);
}

FunctionDeclaration* ImplementationsIndex::get_reversible_iterable_previous_impl(CoreNodes& coreNodes, MembersContainer* container) {
    return implementation_of(*this, coreNodes.iterable.reversible_iterable_previous, (ASTAny*) container);
}

FunctionDeclaration* ImplementationsIndex::get_reversible_iterable_count_impl(CoreNodes& coreNodes, MembersContainer* container) {
    return implementation_of(*this, coreNodes.iterable.reversible_iterable_count, (ASTAny*) container);
}

FunctionDeclaration* ImplementationsIndex::get_stream_write_i8_impl(CoreNodes& coreNodes, MembersContainer* container) {
    return implementation_of(*this, coreNodes.stream.stream_write_i8, (ASTAny*) container);
}

FunctionDeclaration* ImplementationsIndex::get_stream_write_i16_impl(CoreNodes& coreNodes, MembersContainer* container) {
    return implementation_of(*this, coreNodes.stream.stream_write_i16, (ASTAny*) container);
}

FunctionDeclaration* ImplementationsIndex::get_stream_write_i32_impl(CoreNodes& coreNodes, MembersContainer* container) {
    return implementation_of(*this, coreNodes.stream.stream_write_i32, (ASTAny*) container);
}

FunctionDeclaration* ImplementationsIndex::get_stream_write_i64_impl(CoreNodes& coreNodes, MembersContainer* container) {
    return implementation_of(*this, coreNodes.stream.stream_write_i64, (ASTAny*) container);
}

FunctionDeclaration* ImplementationsIndex::get_stream_write_u8_impl(CoreNodes& coreNodes, MembersContainer* container) {
    return implementation_of(*this, coreNodes.stream.stream_write_u8, (ASTAny*) container);
}

FunctionDeclaration* ImplementationsIndex::get_stream_write_u16_impl(CoreNodes& coreNodes, MembersContainer* container) {
    return implementation_of(*this, coreNodes.stream.stream_write_u16, (ASTAny*) container);
}

FunctionDeclaration* ImplementationsIndex::get_stream_write_u32_impl(CoreNodes& coreNodes, MembersContainer* container) {
    return implementation_of(*this, coreNodes.stream.stream_write_u32, (ASTAny*) container);
}

FunctionDeclaration* ImplementationsIndex::get_stream_write_u64_impl(CoreNodes& coreNodes, MembersContainer* container) {
    return implementation_of(*this, coreNodes.stream.stream_write_u64, (ASTAny*) container);
}

FunctionDeclaration* ImplementationsIndex::get_stream_write_char_impl(CoreNodes& coreNodes, MembersContainer* container) {
    return implementation_of(*this, coreNodes.stream.stream_write_char, (ASTAny*) container);
}

FunctionDeclaration* ImplementationsIndex::get_stream_write_uchar_impl(CoreNodes& coreNodes, MembersContainer* container) {
    return implementation_of(*this, coreNodes.stream.stream_write_uchar, (ASTAny*) container);
}

FunctionDeclaration* ImplementationsIndex::get_stream_write_short_impl(CoreNodes& coreNodes, MembersContainer* container) {
    return implementation_of(*this, coreNodes.stream.stream_write_short, (ASTAny*) container);
}

FunctionDeclaration* ImplementationsIndex::get_stream_write_ushort_impl(CoreNodes& coreNodes, MembersContainer* container) {
    return implementation_of(*this, coreNodes.stream.stream_write_ushort, (ASTAny*) container);
}

FunctionDeclaration* ImplementationsIndex::get_stream_write_int_impl(CoreNodes& coreNodes, MembersContainer* container) {
    return implementation_of(*this, coreNodes.stream.stream_write_int, (ASTAny*) container);
}

FunctionDeclaration* ImplementationsIndex::get_stream_write_uint_impl(CoreNodes& coreNodes, MembersContainer* container) {
    return implementation_of(*this, coreNodes.stream.stream_write_uint, (ASTAny*) container);
}

FunctionDeclaration* ImplementationsIndex::get_stream_write_long_impl(CoreNodes& coreNodes, MembersContainer* container) {
    return implementation_of(*this, coreNodes.stream.stream_write_long, (ASTAny*) container);
}

FunctionDeclaration* ImplementationsIndex::get_stream_write_ulong_impl(CoreNodes& coreNodes, MembersContainer* container) {
    return implementation_of(*this, coreNodes.stream.stream_write_ulong, (ASTAny*) container);
}

FunctionDeclaration* ImplementationsIndex::get_stream_write_longlong_impl(CoreNodes& coreNodes, MembersContainer* container) {
    return implementation_of(*this, coreNodes.stream.stream_write_longlong, (ASTAny*) container);
}

FunctionDeclaration* ImplementationsIndex::get_stream_write_ulonglong_impl(CoreNodes& coreNodes, MembersContainer* container) {
    return implementation_of(*this, coreNodes.stream.stream_write_ulonglong, (ASTAny*) container);
}

FunctionDeclaration* ImplementationsIndex::get_stream_write_float_impl(CoreNodes& coreNodes, MembersContainer* container) {
    return implementation_of(*this, coreNodes.stream.stream_write_float, (ASTAny*) container);
}

FunctionDeclaration* ImplementationsIndex::get_stream_write_double_impl(CoreNodes& coreNodes, MembersContainer* container) {
    return implementation_of(*this, coreNodes.stream.stream_write_double, (ASTAny*) container);
}

FunctionDeclaration* ImplementationsIndex::get_stream_write_str_impl(CoreNodes& coreNodes, MembersContainer* container) {
    return implementation_of(*this, coreNodes.stream.stream_write_str, (ASTAny*) container);
}

FunctionDeclaration* ImplementationsIndex::get_stream_write_str_no_len_impl(CoreNodes& coreNodes, MembersContainer* container) {
    return implementation_of(*this, coreNodes.stream.stream_write_str_no_len, (ASTAny*) container);
}
