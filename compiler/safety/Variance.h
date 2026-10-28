// Copyright (c) Chemical Language Foundation 2026.
//
// Variance analysis (S4.2b).
//
// Variance is a property of a declaration's type/lifetime parameters. It is
// computed structurally from the declaration's fields:
//
//   &'a T          covariant in 'a and T
//   &'a mut T      covariant in 'a, invariant in T
//   *T             covariant in T
//   *mut T         invariant in T
//   [T; N]         covariant in T
//   fn(T) -> U     contravariant in T, covariant in U
//   Cell<T>        invariant in T
//   UnsafeCell<T>  invariant in T
//   PhantomData<T> covariant in T
//   union          invariant in every parameter (conservative)
//
// A nested generic type contributes its declared parameter variance, so
// `&'a mut Cell<&'b T>` is invariant in 'b and T while `&'a &'b T` is
// covariant in both. The recursive walk stops at pointers/references (which
// cannot cycle by value) and at a bounded depth.
//
// The result is consumed at the MIR lowering boundary: region signatures use
// it to map an aggregate parameter's field references to the right declared
// lifetime argument instead of the old "first lifetime argument" shortcut.

#pragma once

#include "ast/base/BaseType.h"
#include "ast/base/BaseTypeKind.h"
#include "ast/types/ArrayType.h"
#include "ast/types/FunctionType.h"
#include "ast/types/GenericType.h"
#include "ast/types/LinkedType.h"
#include "ast/types/PointerType.h"
#include "ast/types/ReferenceType.h"
#include "ast/structures/GenericMembersDecl.h"
#include "ast/structures/GenericTypeParameter.h"
#include "ast/structures/MembersContainer.h"
#include "ast/structures/StructDefinition.h"
#include "ast/structures/StructMember.h"
#include "ast/structures/UnionDef.h"
#include "ast/structures/VariantDefinition.h"
#include "ast/structures/VariantMember.h"
#include "ast/structures/VariantMemberParam.h"
#include "ast/statements/Typealias.h"

#include <algorithm>
#include <cstdint>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

enum class Variance : uint8_t {
    /** The parameter does not appear (or appears only in a phantom position). */
    Bivariant = 0,
    Covariant = 1,
    Contravariant = 2,
    Invariant = 3,
};

/** Variance of `inner` under an `outer` position (Bivariant propagates: the
 *  parameter does not occur there at all). */
inline Variance compose_variance(Variance outer, Variance inner) {
    if (outer == Variance::Bivariant || inner == Variance::Bivariant) {
        return Variance::Bivariant;
    }
    if (outer == Variance::Invariant || inner == Variance::Invariant) {
        return Variance::Invariant;
    }
    if (outer == Variance::Covariant) return inner;
    // Contravariant flips covariant/contravariant and keeps contravariant.
    return inner == Variance::Covariant ? Variance::Contravariant
                                        : Variance::Covariant;
}

/** Accumulate one occurrence into a parameter's running variance. */
inline Variance accumulate_variance(Variance current, Variance occurrence) {
    if (occurrence == Variance::Bivariant) return current;
    if (current == Variance::Bivariant) return occurrence;
    if (current == occurrence) return current;
    // A parameter seen both covariantly and contravariantly (or through an
    // invariant position) is invariant.
    return Variance::Invariant;
}

/** A declaration's parameter variance, parallel to its parameter lists. */
struct ContainerVariance {
    std::vector<Variance> type_params;
    std::vector<Variance> lifetime_params;
    std::vector<chem::string_view> type_param_names;
    std::vector<chem::string_view> lifetime_param_names;
};

inline chem::string_view strip_lifetime_quote(chem::string_view name) {
    if (!name.empty() && name.front() == '\'') {
        return chem::string_view(name.data() + 1, name.size() - 1);
    }
    return name;
}

/** `TypeLoc::getType()` is const-qualified; the AST walk is read-only. */
inline BaseType* mutable_type(const BaseType* type) {
    return const_cast<BaseType*>(type);
}

inline int lifetime_param_index(const std::vector<chem::string_view>& params,
                                chem::string_view name) {
    name = strip_lifetime_quote(name);
    if (name.empty()) return -1;
    for (size_t i = 0; i < params.size(); ++i) {
        if (strip_lifetime_quote(params[i]) == name) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

class VarianceAnalysis {
public:
    /**
     * Which of `lifetime_params` occur in `type`, and with what variance.
     * The returned vector is parallel to `lifetime_params`.
     */
    std::vector<Variance> lifetime_uses(
        BaseType* type,
        const std::vector<chem::string_view>& lifetime_params) {
        std::vector<Variance> out(lifetime_params.size(), Variance::Bivariant);
        if (type == nullptr || lifetime_params.empty()) return out;
        collect(type, lifetime_params, out, Variance::Covariant, 0);
        return out;
    }

    /**
     * Compute a container's declared parameter variance from its fields and
     * inherited bases. Cycles (only reachable through a pointer/reference,
     * which are handled before recursion) are treated as bivariant.
     */
    ContainerVariance container_variance(MembersContainer* container) {
        if (container == nullptr) return {};
        const auto hit = cache_.find(container);
        if (hit != cache_.end()) return hit->second;
        if (!visiting_.insert(container).second) {
            // A by-value cycle is invalid; conservatively make the parameters
            // invariant so the caller cannot widen anything.
            ContainerVariance cycle;
            cycle.type_param_names = type_param_names(container);
            cycle.lifetime_param_names = lifetime_param_names(container);
            cycle.type_params.assign(cycle.type_param_names.size(), Variance::Invariant);
            cycle.lifetime_params.assign(cycle.lifetime_param_names.size(), Variance::Invariant);
            return cycle;
        }

        ContainerVariance result;
        result.type_param_names = type_param_names(container);
        result.lifetime_param_names = lifetime_param_names(container);
        result.type_params.assign(result.type_param_names.size(), Variance::Bivariant);
        result.lifetime_params.assign(result.lifetime_param_names.size(), Variance::Bivariant);

        combine_fields(container, result);
        visiting_.erase(container);
        cache_[container] = result;
        return result;
    }

private:
    std::unordered_map<MembersContainer*, ContainerVariance> cache_;
    std::unordered_set<MembersContainer*> visiting_;

    static std::vector<chem::string_view> lifetime_param_names(MembersContainer* c) {
        if (c == nullptr) return {};
        if (auto* g = c->generic_parent) {
            std::vector<chem::string_view> out;
            out.reserve(g->lifetime_params.size());
            for (chem::string_view n : g->lifetime_params) out.push_back(n);
            return out;
        }
        if (c->kind() == ASTNodeKind::StructDecl) {
            auto* s = c->as_struct_def_unsafe();
            std::vector<chem::string_view> out;
            out.reserve(s->lifetime_params.size());
            for (chem::string_view n : s->lifetime_params) out.push_back(n);
            return out;
        }
        return {};
    }

    static std::vector<chem::string_view> type_param_names(MembersContainer* c) {
        std::vector<chem::string_view> out;
        if (c == nullptr || c->generic_parent == nullptr) return out;
        out.reserve(c->generic_parent->generic_params.size());
        for (GenericTypeParameter* p : c->generic_parent->generic_params) {
            out.push_back(p ? p->identifier : chem::string_view());
        }
        return out;
    }

    static MembersContainer* linked_container(BaseType* type) {
        if (type == nullptr) return nullptr;
        if (auto* s = type->get_direct_linked_struct()) return s;
        if (auto* v = type->get_direct_linked_variant()) return v;
        ASTNode* n = type->get_direct_linked_node();
        if (n == nullptr) return nullptr;
        switch (n->kind()) {
            case ASTNodeKind::StructDecl: return n->as_struct_def_unsafe();
            case ASTNodeKind::UnionDecl: return n->as_union_def_unsafe();
            case ASTNodeKind::VariantDecl: return n->as_variant_def_unsafe();
            default: return nullptr;
        }
    }

    static int named_param_index(const std::vector<chem::string_view>& names,
                                 chem::string_view name) {
        if (name.empty()) return -1;
        for (size_t i = 0; i < names.size(); ++i) {
            if (names[i] == name) return static_cast<int>(i);
        }
        return -1;
    }

    /**
     * Record the variance of every parameter occurrence in `type` under the
     * current `context` position. `type_params`/`lifetime_params` are the
     * *enclosing* declaration's parameters.
     */
    void collect_into(
        BaseType* type,
        const std::vector<chem::string_view>& type_params,
        const std::vector<chem::string_view>& lifetime_params,
        std::vector<Variance>& type_out,
        std::vector<Variance>& lifetime_out,
        Variance context,
        uint32_t depth) {
        if (type == nullptr || depth > 16) return;
        switch (type->kind()) {
            case BaseTypeKind::Reference: {
                auto* rt = type->as_reference_type_unsafe();
                const int li = lifetime_param_index(lifetime_params, rt->lifetime);
                if (li >= 0) {
                    lifetime_out[static_cast<size_t>(li)] =
                        accumulate_variance(lifetime_out[static_cast<size_t>(li)], context);
                }
                const Variance pointee = compose_variance(
                    context, rt->is_mutable ? Variance::Invariant : Variance::Covariant);
                collect_into(rt->type, type_params, lifetime_params, type_out,
                             lifetime_out, pointee, depth + 1);
                break;
            }
            case BaseTypeKind::Pointer: {
                auto* pt = type->as_pointer_type_unsafe();
                const Variance pointee = compose_variance(
                    context, pt->is_mutable ? Variance::Invariant : Variance::Covariant);
                collect_into(pt->type, type_params, lifetime_params, type_out,
                             lifetime_out, pointee, depth + 1);
                break;
            }
            case BaseTypeKind::Array: {
                auto* at = type->as_array_type_unsafe();
                collect_into(at->known_child_type(), type_params, lifetime_params,
                             type_out, lifetime_out, context, depth + 1);
                break;
            }
            case BaseTypeKind::Function: {
                auto* ft = type->as_function_type_unsafe();
                for (FunctionParam* p : ft->params) {
                    if (p == nullptr) continue;
                    collect_into(mutable_type(p->type.getType()), type_params, lifetime_params,
                                 type_out, lifetime_out,
                                 compose_variance(context, Variance::Contravariant),
                                 depth + 1);
                }
                collect_into(mutable_type(ft->returnType.getType()), type_params, lifetime_params,
                             type_out, lifetime_out, context, depth + 1);
                break;
            }
            case BaseTypeKind::Generic: {
                auto* gt = type->as_generic_type_unsafe();

                // Lifetime arguments: each is a covariant position unless the
                // referenced declaration declares that parameter invariant.
                ContainerVariance nested;
                MembersContainer* nc = linked_container(gt->referenced);
                if (nc != nullptr) nested = container_variance(nc);

                for (size_t i = 0; i < gt->lifetime_args.size(); ++i) {
                    chem::string_view arg = gt->lifetime_args[i];
                    const int li = lifetime_param_index(lifetime_params, arg);
                    if (li < 0) continue;
                    Variance v = Variance::Covariant;
                    if (i < nested.lifetime_params.size()) {
                        v = nested.lifetime_params[i];
                    }
                    lifetime_out[static_cast<size_t>(li)] =
                        accumulate_variance(lifetime_out[static_cast<size_t>(li)],
                                            compose_variance(context, v));
                }

                // Type arguments: use the referenced declaration's parameter
                // variance when it is known; otherwise treat the occurrence as
                // covariant (the usual generic position).
                for (size_t i = 0; i < gt->types.size(); ++i) {
                    BaseType* arg = mutable_type(gt->types[i].getType());
                    Variance v = Variance::Covariant;
                    if (i < nested.type_params.size()) v = nested.type_params[i];
                    collect_into(arg, type_params, lifetime_params, type_out,
                                 lifetime_out, compose_variance(context, v),
                                 depth + 1);
                }

                // A type argument may itself be a direct use of an enclosing
                // type parameter (e.g. `PhantomData<T>`), handled by the call
                // above through `Linked`/`Generic` recursion.
                break;
            }
            case BaseTypeKind::Linked: {
                ASTNode* n = type->get_direct_linked_node();
                if (n == nullptr) break;
                if (n->kind() == ASTNodeKind::GenericTypeParam) {
                    auto* gp = n->as_generic_type_param_unsafe();
                    const int ti = named_param_index(type_params, gp->identifier);
                    if (ti >= 0) {
                        type_out[static_cast<size_t>(ti)] =
                            accumulate_variance(type_out[static_cast<size_t>(ti)], context);
                    }
                    break;
                }
                if (n->kind() == ASTNodeKind::TypealiasStmt) {
                    BaseType* underlying = n->as_typealias_unsafe()->known_type();
                    if (underlying != nullptr && underlying != type) {
                        collect_into(underlying, type_params, lifetime_params, type_out,
                                     lifetime_out, context, depth + 1);
                    }
                }
                break;
            }
            default:
                break;
        }
    }

    /** Region-signature entry point: only lifetime occurrences are needed. */
    void collect(BaseType* type,
                 const std::vector<chem::string_view>& lifetime_params,
                 std::vector<Variance>& out,
                 Variance context,
                 uint32_t depth) {
        std::vector<Variance> ignored;
        collect_into(type, {}, lifetime_params, ignored, out, context, depth);
    }

    void combine_fields(MembersContainer* container, ContainerVariance& out) {
        const Variance top = Variance::Covariant;
        const bool is_union = container->kind() == ASTNodeKind::UnionDecl;
        const Variance field_ctx = is_union ? Variance::Invariant : top;
        for (BaseDefMember* member : container->variables()) {
            if (member == nullptr) continue;
            switch (member->kind()) {
                case ASTNodeKind::StructMember: {
                    auto* sm = member->as_struct_member();
                    BaseType* t = sm ? mutable_type(sm->type.getType()) : nullptr;
                    combine_type(t, out, field_ctx);
                    break;
                }
                case ASTNodeKind::VariantMember: {
                    auto* vm = member->as_variant_member_unsafe();
                    if (vm == nullptr) break;
                    for (auto& entry : vm->values) {
                        VariantMemberParam* p = entry.second;
                        if (p == nullptr) continue;
                        combine_type(mutable_type(p->type.getType()), out, field_ctx);
                    }
                    break;
                }
                default:
                    break;
            }
        }
        // Inherited bases contribute their fields to this container's layout.
        for (auto& inh : container->inherited) {
            StructDefinition* base = inh.type ? inh.type->get_direct_linked_struct() : nullptr;
            if (base == nullptr) continue;
            ContainerVariance base_v = container_variance(base);
            const size_t n = std::min(base_v.type_params.size(), out.type_params.size());
            for (size_t i = 0; i < n; ++i) {
                out.type_params[i] = accumulate_variance(out.type_params[i], base_v.type_params[i]);
            }
            const size_t ln = std::min(base_v.lifetime_params.size(), out.lifetime_params.size());
            for (size_t i = 0; i < ln; ++i) {
                out.lifetime_params[i] =
                    accumulate_variance(out.lifetime_params[i], base_v.lifetime_params[i]);
            }
        }
    }

    void combine_type(BaseType* type, ContainerVariance& out, Variance context) {
        std::vector<Variance> t(out.type_params.size(), Variance::Bivariant);
        std::vector<Variance> l(out.lifetime_params.size(), Variance::Bivariant);
        collect_into(type, out.type_param_names, out.lifetime_param_names,
                     t, l, context, 0);
        for (size_t i = 0; i < out.type_params.size(); ++i) {
            if (t[i] != Variance::Bivariant) {
                out.type_params[i] = accumulate_variance(out.type_params[i], t[i]);
            }
        }
        for (size_t i = 0; i < out.lifetime_params.size(); ++i) {
            if (l[i] != Variance::Bivariant) {
                out.lifetime_params[i] = accumulate_variance(out.lifetime_params[i], l[i]);
            }
        }
    }
};
