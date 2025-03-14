// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/base/ASTNode.h"

class GenericTypeParameter : public ASTNode {
private:
    int16_t active_iteration = -1;
public:

    chem::string_view identifier;
    BaseType* at_least_type;
    BaseType* def_type;
    std::vector<BaseType*> usage;
    // TODO use int32 here
     // <-- index of active type in usage vector
    unsigned param_index = 0; // <-- index in the generic type parameters

    /**
     * constructor
     */
    constexpr GenericTypeParameter(
        chem::string_view identifier,
        BaseType* at_least_type,
        BaseType* def_type,
        ASTNode* parent_node,
        unsigned param_index,
        SourceLocation location
    ) : ASTNode(ASTNodeKind::GenericTypeParam, parent_node, location), identifier(identifier),
        at_least_type(at_least_type), def_type(def_type), param_index(param_index)
    {

    }

    void declare_only(SymbolResolver& linker);

    void declare_and_link(SymbolResolver &linker, ASTNode*& node_ptr) final;

    void register_usage(ASTAllocator& allocator, BaseType* type);

    BaseType* create_value_type(ASTAllocator& allocator) final {
        if(active_iteration == -1) {
            return nullptr;
        }
        return usage[active_iteration]->copy(allocator);
    }

    inline int16_t current_iteration() {
        return active_iteration;
    }

    void set_active_iteration(int iteration) {
        active_iteration = (int16_t) iteration;
    }

    void deactivate_iteration() {
        active_iteration = -1;
    }

    BaseType *known_type() final {
        if(active_iteration == -1) {
            return nullptr;
        } else if(usage.empty() && !at_least_type) {
            return nullptr;
        } else {
            return usage[active_iteration];
        }
    }

    /**
     * this method is called by the generic instantiator
     * @return
     */
    inline BaseType* concrete_type() {
#ifdef DEBUG
        if(active_iteration == -1 || usage.empty()) {
            throw std::runtime_error("no active iteration");
        }
#endif
        return usage[active_iteration];
    }

    BaseType* type_for_itr(int16_t iteration) {
        return usage[iteration];
    }

    ASTNode* usage_linked() {
        return active_iteration > -1 ? usage[active_iteration]->linked_node() : nullptr;
    }

    ASTNode *child(const chem::string_view &name) final {
        const auto linked = usage_linked();
        return linked ? linked->child(name) : (at_least_type ? at_least_type->linked_node()->child(name) : nullptr);
    }

#ifdef COMPILER_BUILD

    llvm::Type *llvm_param_type(Codegen &gen) final {
        return usage[active_iteration]->llvm_param_type(gen);
    }

    llvm::Type *llvm_type(Codegen &gen) final {
        if(active_iteration == -1) {
            return nullptr;
        }
        return usage[active_iteration]->llvm_type(gen);
    }

    llvm::Type *llvm_chain_type(Codegen &gen, std::vector<ChainValue*> &values, unsigned int index) final {
        return usage[active_iteration]->llvm_chain_type(gen, values, index);
    }

    bool add_child_index(Codegen& gen, std::vector<llvm::Value *>& indexes, const chem::string_view& name) final {
        return usage[active_iteration]->linked_node()->add_child_index(gen, indexes, name);
    }

#endif


};