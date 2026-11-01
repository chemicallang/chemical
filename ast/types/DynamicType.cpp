// Copyright (c) Chemical Language Foundation 2025.

#include "DynamicType.h"
#include "ast/structures/InterfaceDefinition.h"
#include "compiler/lab/TargetData.h"
#include "ast/structures/InheritedType.h"

#include <unordered_set>

namespace {

InterfaceDefinition* linked_interface(BaseType* type) {
    if (type == nullptr) return nullptr;
    BaseType* canonical = type->canonical();
    if (canonical == nullptr) return nullptr;
    ASTNode* node = canonical->get_direct_linked_canonical_node();
    if (node == nullptr) node = canonical->get_direct_linked_node();
    if (node == nullptr || node->kind() != ASTNodeKind::InterfaceDecl) {
        return nullptr;
    }
    return node->as_interface_def_unsafe();
}

/**
 * `dyn Derived` can be used as `dyn Base` only when Base's slots are a prefix
 * of Derived's vtable. The vtable builder flattens inherited interfaces first,
 * so this is true exactly when Base is in the first-inheritance chain.
 */
bool interface_prefix_extends(InterfaceDefinition* from,
                              InterfaceDefinition* to,
                              std::unordered_set<InterfaceDefinition*>& seen,
                              unsigned depth = 0) {
    if (from == nullptr || to == nullptr || depth > 16) return false;
    if (from == to) return true;
    if (!seen.insert(from).second) return false;
    if (from->inherited.empty()) return false;
    InterfaceDefinition* first = linked_interface(const_cast<BaseType*>(from->inherited.front().type.getType()));
    return interface_prefix_extends(first, to, seen, depth + 1);
}

} // namespace

bool DynamicType::satisfies(BaseType *type) {
    const auto type_kind = type->kind();
    if(type_kind != BaseTypeKind::Dynamic) {
        return false;
    }
    auto* other = (DynamicType*) type;
    if (referenced->satisfies(other->referenced)) return true;
    std::unordered_set<InterfaceDefinition*> seen;
    return interface_prefix_extends(linked_interface(referenced),
                                    linked_interface(other->referenced), seen);
}

uint64_t DynamicType::byte_size(const TargetData& target) {
    // `dyn I` is always a two-word fat pointer: { data, vtable }.
    // The size follows the target pointer width, not a hard-coded 64-bit ABI.
    return target.is64Bit ? 16 : 8;
}