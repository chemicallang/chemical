// Copyright (c) Chemical Language Foundation 2026.

#include "compiler/async/AsyncCTypes.h"

#include "ast/base/BaseType.h"
#include "ast/base/ExtendableMembersContainerNode.h"
#include "ast/structures/StructMember.h"
#include "ast/structures/StructDefinition.h"
#include "ast/structures/FunctionDeclaration.h"
#include "ast/types/GenericType.h"
#include "ast/types/FunctionType.h"
#include "ast/types/PointerType.h"
#include "ast/types/ReferenceType.h"

static bool find_struct_member(MembersContainer* container, const chem::string_view& name,
                               StructMember*& out) {
    for (const auto var : container->variables()) {
        if (var->name == name) {
            out = var->as_struct_member_unsafe();
            return true;
        }
    }
    return false;
}

AsyncCTypes resolve_async_c_types_from_handle(BaseType* rt) {
    AsyncCTypes t;
    if (rt == nullptr || rt->kind() != BaseTypeKind::Generic) {
        return t;
    }
    auto gen = rt->as_generic_type_unsafe();
    if (gen->types.empty()) {
        return t;
    }
    t.handle = rt;
    t.inner = const_cast<BaseType*>(gen->types[0].getType());
    const auto fh = rt->get_direct_linked_container();
    if (fh == nullptr) {
        return t;
    }
    StructMember* vtbl_field = nullptr;
    if (!find_struct_member(fh, "vtbl", vtbl_field)) {
        return t;
    }
    auto vtbl_field_type = const_cast<BaseType*>(vtbl_field->type.getType());
    BaseType* table = nullptr;
    if (vtbl_field_type->kind() == BaseTypeKind::Pointer) {
        table = vtbl_field_type->as_pointer_type_unsafe()->type;
    } else if (vtbl_field_type->kind() == BaseTypeKind::Reference) {
        table = vtbl_field_type->as_reference_type_unsafe()->type;
    }
    if (table == nullptr) {
        return t;
    }
    t.table = table;
    const auto table_container = table->get_direct_linked_container();
    if (table_container == nullptr) {
        return t;
    }
    StructMember* poll_field = nullptr;
    if (!find_struct_member(table_container, "poll", poll_field)) {
        return t;
    }
    auto poll_field_type = const_cast<BaseType*>(poll_field->type.getType());
    if (poll_field_type->kind() != BaseTypeKind::Function) {
        return t;
    }
    const auto poll_fn = poll_field_type->as_function_type_unsafe();
    t.poll = poll_fn->returnType;
    if (poll_fn->params.size() >= 2) {
        t.context_ptr = poll_fn->params[1]->type;
    }
    // `inner` is the handle's own generic argument T; for a genuine handle this
    // matches the awaited result type, but we additionally require the struct to
    // actually be `core::async::FutureHandle` so unrelated generics are ignored.
    if (gen->referenced == nullptr || gen->referenced->linked == nullptr
        || gen->referenced->linked->kind() != ASTNodeKind::StructDecl
        || gen->referenced->linked->as_struct_def_unsafe()->name_view() != "FutureHandle") {
        return t;
    }
    t.ok = t.inner != nullptr && t.poll != nullptr && t.context_ptr != nullptr;
    return t;
}

AsyncCTypes resolve_async_c_types(FunctionDeclaration* decl) {
    return resolve_async_c_types_from_handle(
        decl ? const_cast<BaseType*>(decl->returnType.getType()) : nullptr);
}
