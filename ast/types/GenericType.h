// Copyright (c) Qinetik 2024.

#pragma once

#include <utility>
#include "ast/base/BaseType.h"
#include <unordered_map>

class GenericType : public BaseType {
public:

    std::unique_ptr<ReferencedType> referenced;
    std::vector<std::unique_ptr<BaseType>> types;
    int16_t generic_iteration = -1;
    /**
     * when this generic type subscribes to a parent generic node, we receive
     * generic iterations for each iteration of parent generic node
     *
     * the parent generic iteration which is the key of this map corresponds
     * to a set of generic args that we registered for this type and got a generic iteration.
     * the received generic iteration is the value of this map.
     *
     *
     */
    std::unordered_map<int16_t, int16_t> subscribed_map;

    /**
     * constructor
     */
    explicit GenericType(std::unique_ptr<ReferencedType> referenced);

    /**
     * constructor
     */
    GenericType(std::unique_ptr<ReferencedType> referenced, std::vector<std::unique_ptr<BaseType>> types);

    /**
     * constructor
     */
    GenericType(std::unique_ptr<ReferencedType> referenced, int16_t generic_itr);

    /**
     * constructor
     */
    explicit GenericType(std::string base);

    /**
     * accept the visitor
     */
    void accept(Visitor *visitor) override {
        visitor->visit(this);
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
    void report_generic_usage(SymbolResolver &linker);

    /**
     * the given generic arguments are registered
     */
    int16_t report_generic_args(SymbolResolver &linker, std::vector<std::unique_ptr<BaseType>>& gen_args);

    /**
     * this is invoked by the parent generic, we subscribed to, in this function
     * we get the last usage of the linked generic type parameters of the parent generic node
     * and then report those usages to the referenced type
     */
    void report_parent_usage(SymbolResolver &linker, int16_t parent_itr);

    /**
     * this is called by parent generic node, to which we subscribed
     * we stored it's iteration in report_parent_usage, now we will use it to set
     * our own generic iteration from the subscribed map
     */
    void set_parent_iteration(int16_t parent_itr);

    /**
     * link func
     */
    void link(SymbolResolver &linker, std::unique_ptr<BaseType>& current) override;

    int16_t get_generic_iteration() override {
        return generic_iteration;
    }

    ASTNode *linked_node() override;

    [[nodiscard]]
    BaseTypeKind kind() const override {
        return BaseTypeKind::Generic;
    }

    [[nodiscard]]
    ValueType value_type() const override;

    bool is_same(BaseType *other) override {
        return other->kind() == kind();
    }

    bool satisfies(ValueType value_type) override;

    bool satisfies(Value *value) override;

    [[nodiscard]]
    GenericType* copy() const override;

#ifdef COMPILER_BUILD

    llvm::Type *llvm_type(Codegen &gen) override;

    llvm::Type *llvm_param_type(Codegen &gen) override;

#endif

};