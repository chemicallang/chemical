// Copyright (c) Chemical Language Foundation 2025.

#include "GenericImplDecl.h"
#include "ast/utils/GenericUtils.h"
#include "compiler/generics/GenInstantiatorAPI.h"
#include "ast/base/InterfaceBits.h"
#include "compiler/symres/CoreNodes.h"
#include "compiler/generics/InstantiationsContainer.h"
#include "compiler/symres/ImplementationsIndex.h"
#include "ast/structures/GenericFuncDecl.h"
#include "ast/structures/GenericStructDecl.h"
#include "ast/structures/GenericUnionDecl.h"
#include "ast/structures/GenericVariantDecl.h"
#include "ast/structures/GenericInterfaceDecl.h"
#include "ast/structures/InterfaceDefinition.h"
#include "ast/structures/MembersContainer.h"
#include "ast/structures/StructDefinition.h"
#include "ast/structures/UnionDef.h"
#include "ast/structures/VariantDefinition.h"
#include "ast/structures/FunctionDeclaration.h"
#include "ast/types/GenericType.h"
#include "ast/types/LinkedType.h"
#include "ast/types/PointerType.h"
#include "ast/types/ReferenceType.h"
#include "ast/types/ArrayType.h"
#include "ast/base/ASTAny.h"
#include "compiler/ASTDiagnoser.h"
#include "std/except.h"
#include <shared_mutex>
#include <unordered_set>
#include <unordered_map>
#include <vector>

void GenericImplDecl::finalize_signature(ASTAllocator& allocator, ImplDefinition* inst) {

    // The concrete implementation must own its interface and target types. A
    // shallow copy shares these TypeLocs with the master implementation, and
    // visiting them would rewrite the master's `referenced->linked` for every
    // instantiation. Copying first makes the substitution local to `inst`.
    if(inst->interface_type) {
        inst->interface_type = inst->interface_type.copy(allocator);
    }
    if(inst->struct_type) {
        inst->struct_type = inst->struct_type.copy(allocator);
    }

    // copying the inherited types
    for(auto& inh : inst->inherited) {
        inh.type = inh.type.copy(allocator);
    }

    // copying the variables
    inst->copy_variables_in_place(allocator, inst);

    // finalizing the signature of functions (and copying other nodes)
    for(auto& node_ptr : inst->mut_evaluated_nodes()) {
        const auto node = node_ptr;
        switch (node->kind()) {
            case ASTNodeKind::FunctionDecl:
                GenericFuncDecl::finalize_signature(allocator, node->as_function_unsafe());
                break;
            case ASTNodeKind::GenericFuncDecl:
                GenericFuncDecl::finalize_signature(allocator, node->as_gen_func_decl_unsafe()->master_impl);
                break;
            default:
                // copying other nodes
                node_ptr = node->copy(allocator);
                node_ptr->set_parent(inst);
                break;
        }
    }

}

void GenericImplDecl::finalize_body(ASTAllocator& allocator, ImplDefinition* def) {

    for(const auto func : def->master_functions()) {
        GenericFuncDecl::finalize_body(allocator, func);
    }

}

static void adopt_concrete_generic_impl(
    GenericInstantiatorAPI& instantiator,
    ImplDefinition* impl,
    MembersContainer* target,
    SourceLocation demand_loc
) {
    if(impl == nullptr || target == nullptr) return;
    auto* iface = impl->interface_type ? impl->interface_type->get_direct_linked_interface() : nullptr;
    if(iface == nullptr) {
        instantiator.getDiagnoser().error(
            "generic implementation target is not an interface", demand_loc
        );
        return;
    }

    auto& index = instantiator.getImplsIndex();
    // Serialise adoption. The target's indexes and the interface's users/vtable
    // bookkeeping are read by other resolution threads; the recursive mutex keeps
    // nested instantiation (vector of vectors) safe.
    std::lock_guard<std::recursive_mutex> adopt_lock(index.getGenericImplAdoptMutex());
    std::unique_lock<std::shared_mutex> lookup_lock(index.getGenericImplLookupMutex());

    // impl methods first so they win over interface defaults (adopt uses try_emplace)
    target->adopt(impl);

    // interface extension functions are not part of `evaluated_nodes`; index them
    // on the target just like `build_indexes_of_impl` does for a concrete impl.
    for(auto* extension : iface->extension_functions) {
        const auto name = extension->get_node_identifier();
        if(!name.empty()) {
            target->indexes.try_emplace(name, extension);
        }
    }

    // default/interface methods fill only the names the impl does not provide
    target->adopt(iface);

    iface->register_impl(impl);
    index.add_interface(iface, target, impl);
}

ImplDefinition* GenericImplDecl::register_generic_args(
    GenericInstantiatorAPI& instantiator,
    std::vector<TypeLoc>& generic_args,
    InstantiationRequirement requirement,
    MembersContainer* adopter,
    SourceLocation demand_loc
) {

    auto& container = instantiator.getContainer();
    auto& allocator = instantiator.getAllocator();
    auto& diagnoser = instantiator.getDiagnoser();
    auto& reg_mutex = instantiator.getRegistrationMutex();

    // locking the mutex to check (and maybe register) for generic instantiation
    reg_mutex.lock();

    const auto itr = register_generic_usage(allocator, this, container, generic_args, ((std::vector<void*>&) instantiations));
    if(!itr.second) {
        const auto idx = itr.first;
        // reading the existing instantiation while the registration mutex is still held,
        // because another thread may be appending to `instantiations` right now, which
        // reallocates the vector. An unlocked read would be a use after free (handing out
        // a garbage instantiation pointer into the rest of the compilation)
        const auto existing = instantiations[idx];
        reg_mutex.unlock();
        // during body finalization, wait for the dependency's signature to be finalized.
        // Adoption happens before notifySnapshotFinalized, so a waiter that reaches this
        // point can safely resolve the adopted methods afterwards.
        if(requirement == InstantiationRequirement::SignatureFinalization) {
            instantiator.waitSignatureFinalized(this, idx);
        }
        return existing;
    }

    const auto impl = master_impl->shallow_copy(allocator);

    if(itr.first != instantiations.size()) {
#ifdef DEBUG
        CHEM_THROW_RUNTIME("not the index we expected");
#endif
    }

    impl->generic_parent = this;
    impl->generic_instantiation = itr.first;
    instantiations.emplace_back(impl);
    container.put_current_module_instantiation(impl);

    // set status to Building before unlocking
    const auto inst_idx = itr.first;
    {
        std::lock_guard<std::mutex> status_lock(container.getInstantiationStatusMutex());
        instantiation_statuses.push_back({ InstantiationStatus::Registered, std::this_thread::get_id() });
    }

    // Read `body_linked` under the registration lock. `SymResLinkBody` snapshots
    // the instantiations and publishes `body_linked` under the same lock, so an
    // instantiation appended here is either in that snapshot or sees the flag.
    const bool finalize_body_now = body_linked;

    // unlock reg_mutex immediately — signature finalization only needs registration
    reg_mutex.unlock();

    // finalize signature without holding the lock. Use the single-instantiation
    // overload: this can run inside another instantiation's body finalization, so
    // it must not clear the shared symbol table (the span overload does).
    finalize_signature(allocator, impl);
    instantiator.FinalizeSignature(this, impl, inst_idx);

    // Publish the concrete impl into the target before notifying waiters: the
    // demand path that woke up must be able to resolve the requested member.
    adopt_concrete_generic_impl(instantiator, impl, adopter, demand_loc);

    // mark signature as finalized and notify waiters
    instantiator.notifySignatureFinalized(this, inst_idx);

    // body finalization proceeds without the lock
    if(finalize_body_now) {
        finalize_body(allocator, impl);
        instantiator.FinalizeBody(this, impl, inst_idx);
    }

    return impl;

}

ImplDefinition* GenericImplDecl::instantiate_type(
    GenericInstantiatorAPI& instantiator,
    std::vector<TypeLoc>& types,
    SourceLocation location,
    InstantiationRequirement requirement
) {
    auto& diagnoser = instantiator.getDiagnoser();

    std::vector<TypeLoc> generic_args;

    // initialize the generic args
    const auto success = initialize_generic_args(diagnoser, generic_args, generic_params, types);
    if(!success) {
        return nullptr;
    }

    // check all types have been inferred
    const auto success2 = check_inferred_generic_args(diagnoser, generic_args, generic_params, location);
    if(!success2) {
        return nullptr;
    }

    return register_generic_args(instantiator, generic_args, requirement);
}

// ===========================================================================
// Generic impl registration, target-pattern matching and lazy demand
// ===========================================================================

static BaseGenericDecl* generic_target_of(ImplDefinition* master) {
    if(master == nullptr || master->struct_type == nullptr) return nullptr;
    auto* type = (BaseType*) master->struct_type;
    if(type == nullptr || type->kind() != BaseTypeKind::Generic) return nullptr;
    auto* generic = type->as_generic_type_unsafe();
    auto* linked = generic->referenced ? generic->referenced->linked : nullptr;
    if(linked == nullptr) return nullptr;
    switch(linked->kind()) {
        case ASTNodeKind::GenericStructDecl:
            return linked->as_gen_struct_def_unsafe();
        case ASTNodeKind::GenericUnionDecl:
            return linked->as_gen_union_decl_unsafe();
        case ASTNodeKind::GenericVariantDecl:
            return linked->as_gen_variant_decl_unsafe();
        default:
            return nullptr;
    }
}

static GenericTypeParameter* pattern_parameter(BaseType* type) {
    if(type == nullptr || type->kind() != BaseTypeKind::Linked) return nullptr;
    auto* linked = type->as_linked_type_unsafe()->linked;
    if(linked && linked->kind() == ASTNodeKind::GenericTypeParam) {
        return linked->as_generic_type_param_unsafe();
    }
    return nullptr;
}

static bool collect_pattern_parameters(
    BaseType* pattern,
    GenericImplDecl* impl,
    std::unordered_set<GenericTypeParameter*>& used,
    ASTDiagnoser& diagnoser,
    bool& failed
) {
    if(pattern == nullptr) return true;
    if(auto* param = pattern_parameter(pattern)) {
        bool belongs = false;
        for(const auto candidate : impl->generic_params) {
            if(candidate == param) {
                belongs = true;
                break;
            }
        }
        if(!belongs) {
            diagnoser.error(impl) << "type parameter '" << param->identifier
                                  << "' in the implemented type does not belong to this implementation";
            failed = true;
            return false;
        }
        if(!used.insert(param).second) {
            diagnoser.error(impl) << "generic type parameter '" << param->identifier
                                  << "' is used more than once in the implemented type";
            failed = true;
            return false;
        }
        return true;
    }
    if(pattern->kind() == BaseTypeKind::Generic) {
        auto* generic = pattern->as_generic_type_unsafe();
        auto* linked = generic->referenced ? generic->referenced->linked : nullptr;
        if(linked == nullptr) {
            diagnoser.error(impl) << "unresolved type in the implemented type pattern";
            failed = true;
            return false;
        }
        for(auto& arg : generic->types) {
            collect_pattern_parameters((BaseType*) arg, impl, used, diagnoser, failed);
        }
    }
    return !failed;
}

static ASTNode* interface_key_of(BaseType* type) {
    if(type == nullptr) return nullptr;
    auto* node = type->get_direct_linked_canonical_node();
    if(node == nullptr) return nullptr;
    switch(node->kind()) {
        case ASTNodeKind::InterfaceDecl: {
            auto* iface = node->as_interface_def_unsafe();
            return iface->generic_parent ? (ASTNode*) iface->generic_parent : node;
        }
        case ASTNodeKind::GenericInterfaceDecl:
            return node;
        default:
            return nullptr;
    }
}

static bool same_pattern(BaseType* a, BaseType* b) {
    if(a == b) return true;
    if(a == nullptr || b == nullptr) return false;
    if(a->kind() != b->kind()) return false;
    switch(a->kind()) {
        case BaseTypeKind::Linked: {
            auto* al = a->as_linked_type_unsafe()->linked;
            auto* bl = b->as_linked_type_unsafe()->linked;
            if(al == bl) return true;
            // Type parameters are placeholders: two impls `Box<T>` and `Box<U>`
            // have the same target pattern even though the parameter nodes differ.
            if(al && bl && al->kind() == ASTNodeKind::GenericTypeParam
               && bl->kind() == ASTNodeKind::GenericTypeParam) {
                return true;
            }
            return false;
        }
        case BaseTypeKind::Generic: {
            auto* ag = a->as_generic_type_unsafe();
            auto* bg = b->as_generic_type_unsafe();
            if(!ag->referenced || !bg->referenced) return false;
            if(ag->referenced->linked != bg->referenced->linked) return false;
            if(ag->types.size() != bg->types.size()) return false;
            for(size_t i = 0; i < ag->types.size(); i++) {
                if(!same_pattern((BaseType*) ag->types[i], (BaseType*) bg->types[i])) return false;
            }
            return true;
        }
        case BaseTypeKind::Pointer:
            return same_pattern(
                a->as_pointer_type_unsafe()->type,
                b->as_pointer_type_unsafe()->type
            );
        case BaseTypeKind::Reference:
            return same_pattern(
                a->as_reference_type_unsafe()->type,
                b->as_reference_type_unsafe()->type
            );
        default:
            return a->is_same(b);
    }
}

bool register_generic_impl(
    ImplementationsIndex& index,
    GenericImplDecl* impl,
    ASTDiagnoser& diagnoser
) {
    if(impl == nullptr || impl->master_impl == nullptr) return false;
    auto* master = impl->master_impl;

    if(impl->generic_params.empty() && impl->lifetime_params.empty()) {
        diagnoser.error(master) << "a generic implementation must declare at least one type or lifetime parameter";
        return false;
    }

    auto* target = generic_target_of(master);
    if(target == nullptr) {
        diagnoser.error(master) << "generic parameters on an implementation require a generic "
                                   "target type (e.g. 'impl<T> Trait for Box<T>')";
        return false;
    }

    auto* target_type = master->struct_type->as_generic_type_unsafe();
    auto* target_params = &target->generic_params;
    if(target_type->types.size() != target_params->size()) {
        auto message = std::string("implemented type expects ")
            + std::to_string(target_params->size())
            + " type arguments, got "
            + std::to_string(target_type->types.size());
        diagnoser.error(master) << message.c_str();
        return false;
    }

    std::unordered_set<GenericTypeParameter*> used;
    bool failed = false;
    for(auto& arg : target_type->types) {
        collect_pattern_parameters((BaseType*) arg, impl, used, diagnoser, failed);
    }
    if(failed) return false;

    for(const auto param : impl->generic_params) {
        if(used.find(param) == used.end()) {
            diagnoser.error(master) << "generic type parameter '" << param->identifier
                                    << "' is not used by the implemented type";
            return false;
        }
    }

    // Reject an identical implementation of the same interface for the same
    // pattern. Overlapping-but-different patterns are diagnosed lazily at use.
    auto* this_interface = interface_key_of(master->interface_type);
    if(this_interface != nullptr) {
        for(auto* existing : index.generic_impls_for((ASTNode*) target)) {
            if(existing == impl) continue;
            auto* existing_interface = interface_key_of(existing->master_impl->interface_type);
            if(existing_interface != this_interface) continue;
            auto* existing_target = existing->master_impl->struct_type
                ? existing->master_impl->struct_type->as_generic_type_unsafe() : nullptr;
            if(existing_target == nullptr) continue;
            if(existing_target->types.size() != target_type->types.size()) continue;
            bool identical = true;
            for(size_t i = 0; i < target_type->types.size(); i++) {
                if(!same_pattern((BaseType*) target_type->types[i], (BaseType*) existing_target->types[i])) {
                    identical = false;
                    break;
                }
            }
            if(identical) {
                diagnoser.error(master) << "duplicate implementation of the same interface for this type pattern";
                diagnoser.info(existing->master_impl) << "the first implementation is here";
                return false;
            }
        }
    }

    index.add_generic_impl((ASTNode*) target, impl);
    return true;
}

// ---------------------------------------------------------------------------
// Concrete target arguments
// ---------------------------------------------------------------------------

// `get_members_container()` does not unwrap references/pointers (their
// `get_direct_linked_node()` is only defined for the node variants), so the
// demand path strips them explicitly. `v.greet()` where `v : &Box<int>` is the
// common shape.
static BaseType* strip_reference_and_pointer(BaseType* type) {
    while(type != nullptr) {
        switch(type->kind()) {
            case BaseTypeKind::Reference:
                type = type->as_reference_type_unsafe()->type;
                break;
            case BaseTypeKind::Pointer:
                type = type->as_pointer_type_unsafe()->type;
                break;
            default:
                return type;
        }
    }
    return nullptr;
}

static MembersContainer* members_container_of(ASTAny* any) {
    if(any == nullptr) return nullptr;
    switch(any->any_kind()) {
        case ASTAnyKind::Node: {
            auto* node = (ASTNode*) any;
            switch(node->kind()) {
                case ASTNodeKind::StructDecl:
                case ASTNodeKind::UnionDecl:
                case ASTNodeKind::VariantDecl:
                    return node->as_members_container_unsafe();
                default:
                    return nullptr;
            }
        }
        case ASTAnyKind::Type: {
            auto* type = strip_reference_and_pointer((BaseType*) any);
            return type ? type->get_members_container() : nullptr;
        }
        case ASTAnyKind::Value: {
            auto* value = (Value*) any;
            auto* type = strip_reference_and_pointer(value->getType());
            return type ? type->get_members_container() : nullptr;
        }
    }
    return nullptr;
}

static bool generic_args_of_container(
    InstantiationsContainer& container,
    MembersContainer* target,
    std::vector<BaseType*>& args
) {
    if(target == nullptr || target->generic_parent == nullptr || target->generic_instantiation < 0) {
        return false;
    }
    // getInstantiationTypesFor is unsynchronised; registration mutates the same
    // map. Callers hold the instantiator's recursive registration mutex.
    auto all = container.getInstantiationTypesFor(target->generic_parent);
    const auto idx = (size_t) target->generic_instantiation;
    if(idx >= all.size()) return false;
    auto span = all[idx];
    args.assign(span.begin(), span.end());
    return true;
}

static bool contains_generic_parameter(BaseType* type) {
    if(type == nullptr) return false;
    switch(type->kind()) {
        case BaseTypeKind::Linked: {
            auto* linked = type->as_linked_type_unsafe()->linked;
            return linked != nullptr && linked->kind() == ASTNodeKind::GenericTypeParam;
        }
        case BaseTypeKind::Generic: {
            auto* generic = type->as_generic_type_unsafe();
            for(auto& arg : generic->types) {
                if(contains_generic_parameter((BaseType*) arg)) return true;
            }
            auto* linked = generic->referenced ? generic->referenced->linked : nullptr;
            return linked != nullptr && linked->kind() == ASTNodeKind::GenericTypeParam;
        }
        case BaseTypeKind::Pointer:
            return contains_generic_parameter(type->as_pointer_type_unsafe()->type);
        case BaseTypeKind::Reference:
            return contains_generic_parameter(type->as_reference_type_unsafe()->type);
        case BaseTypeKind::Array:
            return contains_generic_parameter(type->as_array_type_unsafe()->elem_type);
        default:
            return false;
    }
}

static bool actual_generic_parts(
    BaseType* actual,
    InstantiationsContainer& container,
    ASTNode*& declaration,
    std::vector<BaseType*>& args
) {
    if(actual == nullptr) return false;
    if(actual->kind() == BaseTypeKind::Generic) {
        auto* generic = actual->as_generic_type_unsafe();
        auto* linked = generic->referenced ? generic->referenced->linked : nullptr;
        if(linked == nullptr) return false;
        switch(linked->kind()) {
            case ASTNodeKind::GenericStructDecl:
            case ASTNodeKind::GenericUnionDecl:
            case ASTNodeKind::GenericVariantDecl:
                declaration = linked;
                for(auto& arg : generic->types) args.push_back((BaseType*) arg);
                return true;
            default:
                break;
        }
    }
    auto* mc = actual->get_members_container();
    if(mc != nullptr && mc->generic_parent != nullptr) {
        declaration = mc->generic_parent;
        return generic_args_of_container(container, mc, args);
    }
    return false;
}

enum class PatternMatch : uint8_t { NoMatch, Match, Defer };

static PatternMatch match_pattern(
    BaseType* pattern,
    BaseType* actual,
    std::unordered_map<GenericTypeParameter*, BaseType*>& bindings,
    InstantiationsContainer& container
) {
    if(pattern == nullptr || actual == nullptr) return PatternMatch::NoMatch;

    // inside a generic master body the concrete argument can still contain the
    // enclosing parameter; it will be substituted after the outer instantiation.
    if(contains_generic_parameter(actual)) return PatternMatch::Defer;

    if(auto* param = pattern_parameter(pattern)) {
        auto found = bindings.find(param);
        if(found != bindings.end()) {
            return found->second->is_same(actual) ? PatternMatch::Match : PatternMatch::NoMatch;
        }
        bindings.emplace(param, actual);
        return PatternMatch::Match;
    }

    if(pattern->kind() == BaseTypeKind::Generic) {
        auto* pg = pattern->as_generic_type_unsafe();
        auto* pdecl = pg->referenced ? pg->referenced->linked : nullptr;
        if(pdecl == nullptr) return PatternMatch::NoMatch;
        ASTNode* adecl = nullptr;
        std::vector<BaseType*> aargs;
        if(!actual_generic_parts(actual, container, adecl, aargs)) {
            return PatternMatch::NoMatch;
        }
        if(adecl != pdecl || pg->types.size() != aargs.size()) {
            return PatternMatch::NoMatch;
        }
        bool deferred = false;
        for(size_t i = 0; i < pg->types.size(); i++) {
            const auto result = match_pattern((BaseType*) pg->types[i], aargs[i], bindings, container);
            if(result == PatternMatch::NoMatch) return PatternMatch::NoMatch;
            if(result == PatternMatch::Defer) deferred = true;
        }
        return deferred ? PatternMatch::Defer : PatternMatch::Match;
    }

    if(pattern->kind() == BaseTypeKind::Linked) {
        auto* pl = pattern->as_linked_type_unsafe()->linked;
        if(pl == nullptr) return PatternMatch::NoMatch;
        if(pl->kind() == ASTNodeKind::GenericTypeParam) {
            return PatternMatch::NoMatch; // handled above
        }
        auto* an = actual->get_direct_linked_node();
        return an == pl ? PatternMatch::Match : PatternMatch::NoMatch;
    }

    return pattern->is_same(actual) ? PatternMatch::Match : PatternMatch::NoMatch;
}

static bool interface_declares(ASTNode* key, const chem::string_view& member) {
    if(key == nullptr || member.empty()) return true;
    switch(key->kind()) {
        case ASTNodeKind::InterfaceDecl: {
            auto* iface = key->as_interface_def_unsafe();
            if(iface->direct_child(member) != nullptr) return true;
            for(auto* ext : iface->extension_functions) {
                if(ext->get_node_identifier() == member) return true;
            }
            return false;
        }
        case ASTNodeKind::GenericInterfaceDecl: {
            auto* master = key->as_gen_interface_decl_unsafe()->master_impl;
            if(master->direct_child(member) != nullptr) return true;
            for(auto* ext : master->extension_functions) {
                if(ext->get_node_identifier() == member) return true;
            }
            return false;
        }
        default:
            return false;
    }
}

static thread_local unsigned gimpl_demand_depth = 0;

struct GimplDemandDepthGuard {
    GimplDemandDepthGuard() { ++gimpl_demand_depth; }
    ~GimplDemandDepthGuard() { --gimpl_demand_depth; }
};

static bool check_generic_impl_bounds(
    GenericInstantiatorAPI& instantiator,
    GenericImplDecl* candidate,
    const std::vector<TypeLoc>& impl_args,
    SourceLocation demand_loc
) {
    if(gimpl_demand_depth > 16) {
        instantiator.getDiagnoser().error(demand_loc)
            << "generic implementation expansion exceeds the recursion limit";
        return false;
    }
    auto& index = instantiator.getImplsIndex();
    auto& diagnoser = instantiator.getDiagnoser();
    bool ok = true;

    auto check_trait = [&](BaseType* concrete_type, BaseType* trait) -> bool {
        auto* concrete = strip_reference_and_pointer(concrete_type);
        if(concrete == nullptr) return true;
        if(contains_generic_parameter(concrete)) return true; // deferred, checked after substitution
        auto* trait_node = trait->get_direct_linked_canonical_node();
        if(trait_node == nullptr) return true;
        ASTNode* interface_key = nullptr;
        switch(trait_node->kind()) {
            case ASTNodeKind::InterfaceDecl: {
                auto* iface = trait_node->as_interface_def_unsafe();
                interface_key = iface->generic_parent ? (ASTNode*) iface->generic_parent : trait_node;
                break;
            }
            case ASTNodeKind::GenericInterfaceDecl:
                interface_key = trait_node;
                break;
            default:
                return true;
        }

        auto* container = concrete->get_members_container();
        auto* for_ = container ? (ASTAny*) container : (ASTAny*) concrete->canonical();
        if(index.get_impl(interface_key, for_) != nullptr) return true;
        if(trait_node->kind() == ASTNodeKind::InterfaceDecl) {
            auto* iface = trait_node->as_interface_def_unsafe();
            if(iface->interface_bits.has(InterfaceBits::DEFAULT_BIT)) {
                if(container == nullptr || container->default_constructor_func() != nullptr) {
                    return true;
                }
            }
            if(iface->interface_bits.has(InterfaceBits::CLONE_BIT) && container != nullptr) {
                auto* copy_interface = instantiator.getCoreNodes().copy_interface;
                if(copy_interface != nullptr) {
                    if(index.get_impl(copy_interface, for_) != nullptr) return true;
                    if(container->generic_parent != nullptr) {
                        instantiate_generic_impls_for(
                            instantiator, (ASTAny*) container, copy_interface,
                            chem::string_view(), demand_loc
                        );
                        if(index.get_impl(copy_interface, for_) != nullptr) return true;
                    }
                }
            }
        }

        // A concrete generic argument may need its own generic impl first
        // (vector<vector<int>>: check vector<int> before vector<vector<int>>).
        if(container != nullptr && container->generic_parent != nullptr) {
            instantiate_generic_impls_for(
                instantiator, (ASTAny*) container, interface_key,
                chem::string_view(), demand_loc
            );
            if(index.get_impl(interface_key, for_) != nullptr) return true;
        }

        diagnoser.error(demand_loc)
            << "type '" << concrete->representation() << "' does not satisfy bound '"
            << trait->representation() << "' required by this generic implementation";
        diagnoser.info(candidate->master_impl)
            << "the bound is declared here";
        return false;
    };

    for(size_t i = 0; i < candidate->generic_params.size() && i < impl_args.size(); i++) {
        const auto param = candidate->generic_params[i];
        for(auto& trait : param->traits) {
            if(!check_trait((BaseType*) impl_args[i], (BaseType*) trait)) ok = false;
        }
    }

    return ok;
}

static bool candidate_provides(
    GenericImplDecl* candidate,
    ASTNode* interface_filter,
    const chem::string_view& member
) {
    auto* iface_key = interface_key_of(candidate->master_impl->interface_type);
    if(interface_filter != nullptr && iface_key != interface_filter) return false;
    if(member.empty()) return true;
    if(candidate->master_impl->direct_child(member) != nullptr) return true;
    return interface_declares(iface_key, member);
}

bool instantiate_generic_impls_for(
    GenericInstantiatorAPI& instantiator,
    ASTAny* for_,
    ASTNode* interface_filter,
    chem::string_view member_filter,
    SourceLocation demand_loc
) {
    GimplDemandDepthGuard depth_guard;
    auto& index = instantiator.getImplsIndex();
    if(!index.has_any_generic_impls()) return false;

    auto* target_container = members_container_of(for_);
    if(target_container == nullptr || target_container->generic_parent == nullptr) {
        return false;
    }

    BaseGenericDecl* target = target_container->generic_parent;
    std::vector<BaseType*> target_args;
    {
        std::lock_guard<std::recursive_mutex> lock(instantiator.getRegistrationMutex());
        if(!generic_args_of_container(instantiator.getContainer(), target_container, target_args)) {
            return false;
        }
    }

    auto candidates = index.generic_impls_for((ASTNode*) target);
    if(candidates.empty()) return false;

    ASTNode* normalized_filter = nullptr;
    if(interface_filter != nullptr) {
        switch(interface_filter->kind()) {
            case ASTNodeKind::InterfaceDecl: {
                auto* iface = interface_filter->as_interface_def_unsafe();
                normalized_filter = iface->generic_parent ? (ASTNode*) iface->generic_parent : interface_filter;
                break;
            }
            case ASTNodeKind::GenericInterfaceDecl:
                normalized_filter = interface_filter;
                break;
            default:
                normalized_filter = interface_filter;
                break;
        }
    }

    bool matched = false;
    auto& container = instantiator.getContainer();
    for(auto* candidate : candidates) {
        if(candidate == nullptr || candidate->master_impl == nullptr) continue;
        if(!candidate_provides(candidate, normalized_filter, member_filter)) continue;

        // The demand path only has the target declaration + argument list, so
        // match the pattern's arguments directly instead of a whole BaseType.
        std::unordered_map<GenericTypeParameter*, BaseType*> bindings;
        auto* pattern = (BaseType*) candidate->master_impl->struct_type;
        if(pattern == nullptr || pattern->kind() != BaseTypeKind::Generic) continue;
        auto* generic_pattern = pattern->as_generic_type_unsafe();
        if(generic_pattern->types.size() != target_args.size()) continue;

        bool deferred = false;
        bool matched_candidate = true;
        for(size_t i = 0; i < generic_pattern->types.size(); i++) {
            const auto r = match_pattern(
                (BaseType*) generic_pattern->types[i], target_args[i], bindings, container
            );
            if(r == PatternMatch::NoMatch) {
                matched_candidate = false;
                break;
            }
            if(r == PatternMatch::Defer) {
                deferred = true;
                break;
            }
        }
        if(!matched_candidate || deferred) {
            continue;
        }

        std::vector<TypeLoc> impl_args;
        impl_args.reserve(candidate->generic_params.size());
        bool complete = true;
        for(const auto param : candidate->generic_params) {
            auto found = bindings.find(param);
            if(found == bindings.end() || contains_generic_parameter(found->second)) {
                complete = false;
                break;
            }
            impl_args.emplace_back(found->second, demand_loc);
        }
        if(!complete) continue;

        matched = true;
        if(!check_generic_impl_bounds(instantiator, candidate, impl_args, demand_loc)) {
            continue;
        }
        candidate->register_generic_args(
            instantiator, impl_args,
            InstantiationRequirement::SignatureFinalization,
            target_container, demand_loc
        );
    }
    return matched;
}
