// Copyright (c) Chemical Language Foundation 2025.

#include "FunctionType.h"
#include "ast/base/BaseType.h"
#include "ast/base/Value.h"
#include "ast/values/AccessChain.h"
#include "ast/values/VariableIdentifier.h"
#include "ast/types/VoidType.h"
#include "ast/types/CapturingFunctionType.h"
#include "ast/types/ArrayType.h"
#include "ast/types/PointerType.h"
#include "ast/types/ReferenceType.h"
#include "ast/types/GenericType.h"
#include "ast/structures/FunctionDeclaration.h"
#include "ast/structures/FunctionParam.h"
#include "ast/structures/StructDefinition.h"
#include "ast/structures/GenericStructDecl.h"
#include "ast/structures/GenericUnionDecl.h"
#include "ast/structures/GenericVariantDecl.h"
#include "compiler/ASTDiagnoser.h"

namespace {

chem::string_view bare_lifetime(chem::string_view name) {
    if (!name.empty() && name.front() == '\'') {
        return chem::string_view(name.data() + 1, name.size() - 1);
    }
    return name;
}

bool is_binder(chem::string_view name,
               const std::vector<chem::string_view>& binders) {
    const auto bare = bare_lifetime(name);
    for (const auto binder : binders) {
        if (bare_lifetime(binder) == bare) return true;
    }
    return false;
}

std::vector<chem::string_view> effective_binders(FunctionType* type) {
    if (!type->binder_lifetimes.empty()) return type->binder_lifetimes;
    auto* decl = type->as_function();
    if (decl != nullptr && decl->generic_parent != nullptr) {
        return decl->generic_parent->lifetime_params;
    }
    return {};
}

/**
 * Can a reference with `source` lifetime satisfy a reference with `target`
 * lifetime? A bound or elided source lifetime is generic and can be
 * instantiated at the target binder. A `'static` source cannot: it is more
 * restrictive than `for<'a>`.
 */
bool hrtb_lifetime_compatible(
        chem::string_view source,
        chem::string_view target,
        const std::vector<chem::string_view>& source_binders,
        const std::vector<chem::string_view>& target_binders) {
    const auto s = bare_lifetime(source);
    const auto t = bare_lifetime(target);
    if (t.empty() || t == "elided" || t == "self") return true;
    if (t == "static") {
        // A generic/elided source can accept the static requirement.
        return s.empty() || s == "elided" || s == "self" ||
               s == "static" || is_binder(s, source_binders);
    }
    if (is_binder(t, target_binders)) {
        if (s == "static") return false;
        return s.empty() || s == "elided" || s == "self" ||
               is_binder(s, source_binders);
    }
    return s == t;
}

bool hrtb_types_compatible(
        BaseType* source,
        BaseType* target,
        const std::vector<chem::string_view>& source_binders,
        const std::vector<chem::string_view>& target_binders) {
    if (source == nullptr || target == nullptr) return source == target;
    if (source->kind() != target->kind()) return false;
    switch (source->kind()) {
        case BaseTypeKind::Reference: {
            auto* s = source->as_reference_type_unsafe();
            auto* t = target->as_reference_type_unsafe();
            if (s->is_mutable != t->is_mutable) return false;
            if (!hrtb_lifetime_compatible(s->lifetime, t->lifetime,
                                          source_binders, target_binders)) {
                return false;
            }
            return hrtb_types_compatible(s->type, t->type,
                                         source_binders, target_binders);
        }
        case BaseTypeKind::Pointer: {
            auto* s = source->as_pointer_type_unsafe();
            auto* t = target->as_pointer_type_unsafe();
            if (s->is_mutable != t->is_mutable) return false;
            return hrtb_types_compatible(s->type, t->type,
                                         source_binders, target_binders);
        }
        case BaseTypeKind::Array: {
            auto* s = source->as_array_type_unsafe();
            auto* t = target->as_array_type_unsafe();
            return hrtb_types_compatible(s->elem_type, t->elem_type,
                                         source_binders, target_binders);
        }
        case BaseTypeKind::Generic: {
            auto* s = source->as_generic_type_unsafe();
            auto* t = target->as_generic_type_unsafe();
            if (s->lifetime_args.size() != t->lifetime_args.size()) return false;
            for (size_t i = 0; i < s->lifetime_args.size(); ++i) {
                if (!hrtb_lifetime_compatible(s->lifetime_args[i],
                                              t->lifetime_args[i],
                                              source_binders, target_binders)) {
                    return false;
                }
            }
            if (s->types.size() != t->types.size()) return false;
            for (size_t i = 0; i < s->types.size(); ++i) {
                if (!hrtb_types_compatible(s->types[i], t->types[i],
                                           source_binders, target_binders)) {
                    return false;
                }
            }
            return true;
        }
        case BaseTypeKind::Function: {
            auto* s = source->as_function_type_unsafe();
            auto* t = target->as_function_type_unsafe();
            // Nested higher-ranked function types need their own binder
            // environment; keep the conservative boundary rather than
            // accepting a pattern whose regions are not modeled.
            if (!s->binder_lifetimes.empty() || !t->binder_lifetimes.empty()) {
                return false;
            }
            if (s->params.size() != t->params.size()) return false;
            for (size_t i = 0; i < s->params.size(); ++i) {
                if (s->params[i] == nullptr || t->params[i] == nullptr) {
                    if (s->params[i] != t->params[i]) return false;
                    continue;
                }
                if (!hrtb_types_compatible(s->params[i]->type, t->params[i]->type,
                                           source_binders, target_binders)) {
                    return false;
                }
            }
            return hrtb_types_compatible(s->returnType, t->returnType,
                                         source_binders, target_binders);
        }
        default:
            return source->is_same(target);
    }
}

} // namespace

bool CapturingFunctionType::satisfies(BaseType *type) {
    switch(type->kind()) {
        case BaseTypeKind::Function:
            // `satisfies` is evaluated on the target with the source as the
            // argument; preserve that direction for higher-ranked targets.
            return type->as_function_type_unsafe()->satisfies(func_type);
        case BaseTypeKind::CapturingFunction:
            return type->as_capturing_func_type_unsafe()->func_type->satisfies(func_type);
        default:
            return false;
    }
}

bool FunctionType::satisfies(BaseType *type) {
    switch(type->kind()) {
        case BaseTypeKind::NullPtr:
            return true;
        case BaseTypeKind::Function: {
            const auto func = type->as_function_type_unsafe();
            if (!equal_type(func)) return false;
            // `satisfies` is called on the *target* type with the source as the
            // argument. An ordinary target accepts a higher-ranked source (the
            // source is more general), but a higher-ranked target needs a
            // source that can be instantiated at its binder.
            if (binder_lifetimes.empty()) return true;
            const auto source_binders = effective_binders(func);
            if (params.size() != func->params.size()) return false;
            for (size_t i = 0; i < params.size(); ++i) {
                if (params[i] == nullptr || func->params[i] == nullptr) {
                    if (params[i] != func->params[i]) return false;
                    continue;
                }
                if (!hrtb_types_compatible(func->params[i]->type,
                                           params[i]->type,
                                           source_binders,
                                           binder_lifetimes)) {
                    return false;
                }
            }
            return hrtb_types_compatible(func->returnType, returnType,
                                         source_binders,
                                         binder_lifetimes);
        }
        default:
            return false;
    }
}

bool FunctionType::isInVarArgs(unsigned index) const {
    return isVariadic() && index >= (params.size() - 1);
}

uint64_t FunctionType::byte_size(const TargetData& target) {
    if(isCapturing()) {
        return target.is64Bit ? 16 : 8;
    } else {
        return target.is64Bit ? 8 : 4;
    }
}
unsigned FunctionType::total_implicit_params() {
    unsigned i = 0;
    for(auto& param : params) {
        if(param->is_implicit()) {
            i++;
        } else {
            break;
        }
    }
    return i;
}

unsigned int FunctionType::expectedArgsSize() {
    return params.size() - total_implicit_params() - (isExtensionFn() ? 1 : 0) - (isVariadic() ? 1 : 0);
}

FunctionParam* FunctionType::func_param_for_arg_at(unsigned index) {
    if(params.empty()) return nullptr;
    const auto offset = explicit_func_arg_offset(); // first argument for implicit self
    if(isVariadic() && index >= (params.size() - 1 - offset)) {
        return params.back();
    }
    const auto expected = index + offset;
    if(expected < params.size()) {
        return params[expected];
    } else {
        return nullptr;
    }
}

bool FunctionType::satisfy_args(std::vector<Value*>& forArgs) {
    auto has_self = has_self_param();
    unsigned offset = has_self ? 1 : 0;
    auto required_args_len = params.size() - offset;
    if(forArgs.size() != required_args_len) {
        return false;
    }
    unsigned i = offset; // first argument for implicit self
    while(i < params.size()) {
        if(!params[i]->type->satisfies(forArgs[i - offset], false)) {
            return false;
        }
        i++;
    }
    return true;
}

bool FunctionType::do_param_types_match(std::vector<FunctionParam*>& param_types, bool check_self) {
    if(params.size() != param_types.size()) return false;
    unsigned i = check_self ? 0 : (has_self_param() ? 1 : 0);
    const auto siz = params.size();
    while(i < siz) {
        if(!param_types[i]->type->is_same(params[i]->type)) {
            return false;
        }
        i++;
    }
    return true;
}

FunctionParam* FunctionType::get_self_param() {
    if(!params.empty()) {
        auto& param = params[0];
        if(isExtensionFn()) {
            // in extension functions, the first parameter is the receiver
            return param;
        }
        if(param->is_implicit() && (param->name == "self" || param->name == "this")) {
            return param;
        }
    }
    return nullptr;
}

bool FunctionType::has_explicit_params() {
    for(auto& param : params) {
        if(!param->is_implicit()) return true;
    }
    return false;
}

bool FunctionType::equal(FunctionType *other) const {
    if (isVariadic() != other->isVariadic()) {
        return false;
    }
    if (requiresUnsafe() != other->requiresUnsafe()) {
        return false;
    }
    // return type can be nullptr
    // because sometimes user doesn't give return type for lambda
    // we can't set it to void (because we have to determine it by traversing ast)
    if (returnType == nullptr) {
        if (other->returnType == nullptr || other->returnType->canonical()->kind() != BaseTypeKind::Void) {
            return false;
        }
    } else {
        if (other->returnType == nullptr) {
            if (returnType->canonical()->kind() != BaseTypeKind::Void) {
                return false;
            }
        } else {
            if (!returnType->is_same(other->returnType)) {
                return false;
            }
        }
    }
    if(params.size() != other->params.size()) {
        return false;
    }
    unsigned i = 0;
    while (i < params.size()) {
        if (!params[i]->type->is_same(other->params[i]->type)) {
            return false;
        }
        i++;
    }
    return true;
}

FunctionType *FunctionType::copy(ASTAllocator& allocator) {
    const auto func_type = new (allocator.allocate<FunctionType>()) FunctionType(returnType.copy(allocator), isExtensionFn(), isVariadic(), isCapturing(), data.signature_resolved);
    func_type->data = data;
    func_type->binder_lifetimes = binder_lifetimes;
    for (auto &param: params) {
        func_type->params.emplace_back(param->copy(allocator));
    }
    return func_type;
}