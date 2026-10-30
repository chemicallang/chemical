// Copyright (c) Chemical Language Foundation 2025.
//
// Shared function-prototype ABI and record — see MIRPrototype.h.

#include "MIRPrototype.h"

#include "MIRTypeBuilder.h"
#include "MIRABI.h"

#include "ast/base/ASTNode.h"
#include "ast/base/BaseType.h"
#include "ast/structures/FunctionDeclaration.h"
#include "ast/structures/FunctionParam.h"
#include "ast/structures/ImplDefinition.h"
#include "ast/structures/InterfaceDefinition.h"
#include "compiler/mangler/NameMangler.h"
#include "preprocess/2c/BufferedWriter.h"

namespace mir {

namespace {

/**
 * A method of an `impl StaticInterface for T` is emitted with the interface's
 * erased `self` ABI, not `T*`: the interface's own method declaration carries
 * that type and the impl's definition must match. Returns the interface's
 * matching method, or null.
 */
FunctionDeclaration* static_iface_base_method(FunctionDeclaration* decl) {
    if (!decl) return nullptr;
    ASTNode* parent = decl->parent();
    if (!parent || parent->kind() != ASTNodeKind::ImplDecl) return nullptr;
    auto* impl = parent->as_impl_def_unsafe();
    if (!impl->interface_type) return nullptr;
    auto* iface = impl->interface_type->get_direct_linked_interface();
    if (!iface || !iface->is_static()) return nullptr;
    return iface->direct_child_function(decl->name_view());
}

TypeId map_param(MIRTypeBuilder& types, MIRModule& module, FunctionParam* p) {
    TypeId pt = p && p->type ? types.map(p->type) : types.opaque_type();
    if (pt != MIR_INVALID_ID && module.types.get(pt).kind == MIRTypeKind::Array) {
        pt = types.pointer_type(module.types.get(pt).element, true);
    } else if (abi::is_aggregate(module, pt)) {
        pt = types.pointer_type(pt, true);
    }
    return pt;
}

// C passes arrays as pointers but struct/union/variant values directly. The
// internal ABI converts every aggregate to a pointer; the C ABI keeps the
// by-value type so the backend/LLVM target classifier applies the real
// platform convention.
TypeId map_c_param(MIRTypeBuilder& types, MIRModule& module, FunctionParam* p) {
    TypeId pt = p && p->type ? types.map(p->type) : types.opaque_type();
    if (pt != MIR_INVALID_ID && module.types.get(pt).kind == MIRTypeKind::Array) {
        pt = types.pointer_type(module.types.get(pt).element, true);
    }
    return pt;
}

} // namespace

TypeId mir_function_signature(MIRTypeBuilder& types, MIRModule& module,
                              FunctionDeclaration* decl) {
    const TypeId ret = decl->returnType ? types.map(decl->returnType) : types.void_type();
    const bool sret = abi::needs_sret(module, ret);
    std::vector<TypeId> ptypes;
    if (sret) ptypes.push_back(types.pointer_type(ret, true));

    FunctionParam* self_param = decl->has_self_param() ? decl->get_self_param() : nullptr;
    bool self_in_params = false;
    for (FunctionParam* p : decl->params) {
        if (p == self_param) {
            self_in_params = true;
            break;
        }
    }
    if (self_in_params) self_param = nullptr;

    if (self_param) {
        FunctionDeclaration* base = static_iface_base_method(decl);
        FunctionParam* base_self = base ? base->get_self_param() : nullptr;
        ptypes.push_back(base_self && base_self->type ? types.map(base_self->type)
                                                      : map_param(types, module, self_param));
    }
    for (FunctionParam* p : decl->params) ptypes.push_back(map_param(types, module, p));
    if (decl->isVariadic() && !decl->params.empty()) ptypes.pop_back();

    return types.function_signature(sret ? types.void_type() : ret, ptypes, decl->isVariadic());
}

static bool c_abi_by_value_type(BaseType* type) {
    if (type == nullptr) return false;
    BaseType* canonical = type->canonical();
    if (canonical == nullptr) return false;
    if (canonical->kind() == BaseTypeKind::Struct || canonical->kind() == BaseTypeKind::Union) {
        return true;
    }
    ASTNode* linked = canonical->get_direct_linked_canonical_node();
    return linked != nullptr &&
           (linked->kind() == ASTNodeKind::StructDecl || linked->kind() == ASTNodeKind::UnionDecl);
}

bool mir_needs_c_abi(FunctionDeclaration* decl) {
    if (decl == nullptr || !decl->is_extern()) return false;
    if (c_abi_by_value_type(const_cast<BaseType*>(decl->returnType.getType()))) return true;
    for (FunctionParam* p : decl->params) {
        if (p != nullptr && c_abi_by_value_type(p->type)) return true;
    }
    return false;
}

TypeId mir_c_function_signature(MIRTypeBuilder& types, MIRModule& module,
                                FunctionDeclaration* decl) {
    const TypeId ret = decl->returnType ? types.map(decl->returnType) : types.void_type();
    std::vector<TypeId> ptypes;
    FunctionParam* self_param = decl->has_self_param() ? decl->get_self_param() : nullptr;
    bool self_in_params = false;
    for (FunctionParam* p : decl->params) {
        if (p == self_param) {
            self_in_params = true;
            break;
        }
    }
    if (self_in_params) self_param = nullptr;
    if (self_param) {
        FunctionDeclaration* base = static_iface_base_method(decl);
        FunctionParam* base_self = base ? base->get_self_param() : nullptr;
        ptypes.push_back(base_self && base_self->type ? types.map(base_self->type)
                                                      : map_c_param(types, module, self_param));
    }
    for (FunctionParam* p : decl->params) ptypes.push_back(map_c_param(types, module, p));
    if (decl->isVariadic() && !decl->params.empty()) ptypes.pop_back();
    return types.function_signature(ret, ptypes, decl->isVariadic());
}

void build_function_prototype(NameMangler& mangler, MIRTypeBuilder& types, MIRModule& module,
                              FunctionDeclaration* decl, MIRPrototype& out) {
    out = MIRPrototype{};
    out.type = mir_function_signature(types, module, decl);
    if (mir_needs_c_abi(decl)) out.c_abi_type = mir_c_function_signature(types, module, decl);
    {
        BufferedWriter w(256);
        mangler.mangle(w, decl);
        out.name.assign(w.data(), w.getPosition());
    }
    if (decl->is_std_call()) out.name = "__chem_stdcall " + out.name;
    if (decl->is_extern()) out.flags |= MPF_EXTERN;
    if (decl->body.has_value() && !decl->is_linkage_public()) out.flags |= MPF_STATIC;
    if (decl->is_dll_import()) out.flags |= MPF_DLLIMPORT;
}

} // namespace mir
