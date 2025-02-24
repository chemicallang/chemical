// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include <utility>
#include "ast/base/BaseType.h"
#include "ast/types/LinkedType.h"
#include <unordered_map>

class ASTDiagnoser;

class GenericType : public BaseType {
public:

    LinkedType* referenced;
    std::vector<BaseType*> types;
    int16_t generic_iteration = -1;
    /**
     * when this generic type subscribes to a parent generic node, we receive
     * generic iterations for each iteration of parent generic node
     *
     * the parent generic iteration which is the key of this map corresponds
     * to a set of generic args that we registered for this type and got a generic iteration.
     * the received generic iteration is the value of this map.
     */
    std::unordered_map<int16_t, int16_t> subscribed_map;

    /**
     * constructor
     */
    GenericType(LinkedType* referenced) : BaseType(BaseTypeKind::Generic, referenced->encoded_location()), referenced(referenced) {

    }

    /**
     * constructor
     */
    GenericType(LinkedType* referenced, std::vector<BaseType*> types) : BaseType(BaseTypeKind::Generic, referenced->encoded_location()), referenced(referenced), types(std::move(types)) {

    }

    /**
     * constructor
     */
    GenericType(LinkedType* referenced, int16_t generic_itr) : BaseType(BaseTypeKind::Generic, referenced->encoded_location()), referenced(referenced), generic_iteration(generic_itr) {

    }

    /**
     * when an arg of this generic type is referencing a generic type parameter
     * we subscribe to implementations of that arg, for example
     * struct TalkingDog<X> : Dog<X> <--- here Dog<X> is the generic type subscribing to
     * TalkingDog<X>, when a use of TalkingDog<X> is detected, it's registered with Dog<X> as well
     */
    bool subscribe_to_parent_generic();

    /**
     * if this type is completely specialized, 'Dog<int, int>' we report this set
     * of generic arguments to the owner type 'Dog'
     */
    void report_generic_usage(ASTAllocator& astAllocator, ASTDiagnoser& diagnoser);

    /**
     * the given generic arguments are registered
     */
    int16_t report_generic_args(ASTAllocator& astAllocator, ASTDiagnoser& diagnoser, std::vector<BaseType*>& gen_args);

    /**
     * this is invoked by the parent generic, we subscribed to, in this function
     * we get the last usage of the linked generic type parameters of the parent generic node
     * and then report those usages to the referenced type
     */
    void report_parent_usage(ASTAllocator& astAllocator, ASTDiagnoser& diagnoser, int16_t parent_itr);

    /**
     * this is called by parent generic node, to which we subscribed
     * we stored it's iteration in report_parent_usage, now we will use it to set
     * our own generic iteration from the subscribed map
     */
    void set_parent_iteration(int16_t parent_itr);

    /**
     * link func
     */
    bool link(SymbolResolver &linker) final;

    ASTNode *linked_node() final;

    bool is_same(BaseType *other) final {
        return other->kind() == kind();
    }

    bool satisfies(BaseType *type) final;

    [[nodiscard]]
    GenericType* copy(ASTAllocator& allocator) const final;

#ifdef COMPILER_BUILD

    llvm::Type *llvm_type(Codegen &gen) final;

    llvm::Type *llvm_param_type(Codegen &gen) final;

#endif

};