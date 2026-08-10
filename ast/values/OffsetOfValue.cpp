// Copyright (c) Chemical Language Foundation 2025.

#include "OffsetOfValue.h"
#include "ast/base/BaseType.h"
#include "ast/base/GlobalInterpretScope.h"
#include "ast/base/TypeBuilder.h"
#include "ast/structures/MembersContainer.h"
#include "ast/values/IntNumValue.h"

/**
 * Computes the byte offset of `member_name` within the struct/union type `type`
 * using the same layout rules as VariablesContainerBase::total_byte_size():
 * inherited structs are embedded first (each aligned to its own alignment),
 * then the direct members follow, each aligned to its own alignment.
 *
 * Returns (uint64_t) -1 when the type cannot be resolved, is not a
 * struct/union, or the member was not found.
 */
static uint64_t struct_member_offset(BaseType* type, const chem::string_view& member_name, const TargetData& data) {
    const auto node = type->get_direct_linked_node();
    if(!node) {
        return (uint64_t) -1;
    }
    const auto container = node->get_members_container();
    if(!container) {
        return (uint64_t) -1;
    }

    uint64_t offset = 0;

    // inherited structs come first in the layout (embedded as nested structs)
    for(const auto& inherits : container->inherited) {
        const auto inh_type = inherits.type;
        const auto inh_align = (size_t) inh_type->type_alignment(data);
        offset += (inh_align - (offset % inh_align)) % inh_align;
        offset += inh_type->byte_size(data);
    }

    // direct members in declaration order
    for(const auto member : container->variables()) {
        const auto member_align = (size_t) member->known_type()->type_alignment(data);
        offset += (member_align - (offset % member_align)) % member_align;
        if(member->name_view() == member_name) {
            return offset;
        }
        offset += member->byte_size(data);
    }

    return (uint64_t) -1;
}

// NOTE: the LLVM codegen for OffsetOfValue (llvm_type / llvm_value) lives in
// compiler/backend/LLVM.cpp, matching the SizeOfValue/AlignOfValue convention.

Value* OffsetOfValue::evaluated_value(InterpretScope &scope) {
    const auto offset = struct_member_offset(for_type, member_name, scope.global->target_data);
    return new (scope.allocate<IntNumValue>()) IntNumValue(offset, scope.global->typeBuilder.getU64Type(), encoded_location());
}
