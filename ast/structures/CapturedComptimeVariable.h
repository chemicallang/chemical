// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/base/ASTNode.h"
#include "ast/base/InterpretScope.h"
#include "ast/types/ReferenceType.h"
#include "ast/values/FunctionCall.h"
#include "std/chem_string_view.h"

class Value;

/**
 * what kind of comptime environment reference a captured ref represents
 */
enum class CapturedComptimeRefKind {
    /**
     * a reference to a variable (parameter/local) from the enclosing comptime
     * function's scope. resolved by looking the identifier name up in the
     * interpret scope where the runtime value's return is interpreted
     */
    Variable,

    /**
     * a comptime function call nested inside the runtime value's expression.
     * resolved by evaluating the call while the enclosing comptime scope (and
     * its call stack) is still alive. this matters for location-aware
     * intrinsics like intrinsics::get_call_loc and for call arguments that
     * reference the enclosing function's locals, which cannot be resolved
     * after the scope has been destroyed (during translation)
     */
    Call
};

/**
 * a reference to a comptime value that was automatically captured from the
 * enclosing comptime function's scope into a %runtime_value /
 * %runtime_block_value.
 *
 * every %runtime_value / %runtime_block_value owns a vector of these refs (its
 * CapturedComptimeValues) which acts as a template: the evaluated value starts
 * as nullptr. when evaluated_value is called on the runtime value (while the
 * interpret scope of the returning comptime function is still alive), a
 * shallow copy of the runtime value is created, each ref is evaluated and its
 * value stored here, and the bridge node (CapturedComptimeVariable) / call
 * node of the ref is re-pointed to the evaluated copy, so the backends can
 * resolve the captured value through the copy while translating it (after the
 * interpret scope has been destroyed).
 */
struct CapturedComptimeValueRef {

    /**
     * what kind of capture this ref represents
     */
    CapturedComptimeRefKind kind = CapturedComptimeRefKind::Variable;

    /**
     * the CapturedComptimeVariable bridge node in the runtime value's AST
     * (Variable kind only). when the runtime value is evaluated, this node's
     * parent refs pointer is re-pointed to the evaluated copy
     */
    class CapturedComptimeVariable* node = nullptr;

    /**
     * the nested comptime call (Call kind only). when the runtime value is
     * evaluated, the call is evaluated with the still-alive scope and the call
     * node is re-pointed to the evaluated copy
     */
    class FunctionCall* call = nullptr;

    /**
     * the identifier name of the captured variable (Variable kind only), used
     * to look the value up in the interpret scope where the runtime value's
     * return is interpreted
     */
    chem::string_view name;

    /**
     * the evaluated value of the captured variable / call. nullptr until the
     * runtime value is evaluated (evaluated_value)
     */
    Value* evaluated = nullptr;

};

/**
 * the captured comptime value references of a %runtime_value /
 * %runtime_block_value. each runtime value is a template: the refs' evaluated
 * values start as nullptr and are filled when evaluated_value creates a copy
 * of the runtime value
 */
struct CapturedComptimeValues {

    std::vector<CapturedComptimeValueRef> refs;

    /**
     * registers a newly captured variable (its bridge node and identifier
     * name) and returns its index
     */
    inline unsigned add_ref(class CapturedComptimeVariable* node, const chem::string_view& name) {
        const auto index = refs.size();
        refs.push_back({CapturedComptimeRefKind::Variable, node, nullptr, name, nullptr});
        return (unsigned) index;
    }

    /**
     * registers a newly captured comptime call and returns its index
     */
    inline unsigned add_call_ref(class FunctionCall* call) {
        const auto index = refs.size();
        refs.push_back({CapturedComptimeRefKind::Call, nullptr, call, chem::string_view(), nullptr});
        return (unsigned) index;
    }

    /**
     * evaluates the captured comptime values with the given interpret scope
     * (which must still be alive, i.e. the scope where the runtime value's
     * return is being interpreted) and stores the values in the refs. the
     * bridge nodes / call nodes are re-pointed to this container (the
     * evaluated copy) so the backends can resolve the captured values through
     * it during translation.
     *
     * this is called on the evaluated copy of a runtime value; the original
     * keeps acting as a template with unevaluated refs
     */
    void evaluate(InterpretScope& scope);

};

class CapturedComptimeVariable : public ASTNode {
public:

    /**
     * the resolved symbol node of the captured variable, used for type info
     */
    ASTNode *linked;

    /**
     * the captured refs of the owning %runtime_value / %runtime_block_value.
     * set at symbol resolution to the template's refs; when the runtime value
     * is evaluated (evaluated_value), this is re-pointed to the evaluated
     * copy, which holds the evaluated values
     */
    CapturedComptimeValues* parent_refs = nullptr;

    /**
     * the index of this captured variable inside parent_refs->refs
     */
    unsigned index = 0;

    /**
     * constructor
     */
    constexpr CapturedComptimeVariable(
        ASTNode* linked,
        ASTNode* parent_node,
        SourceLocation location
    ) : ASTNode(ASTNodeKind::CapturedComptimeVariable, parent_node, location), linked(linked) {

    }

    CapturedComptimeVariable* copy(ASTAllocator &allocator) override {
        const auto var = new (allocator.allocate<CapturedComptimeVariable>()) CapturedComptimeVariable(
            linked,
            parent(),
            encoded_location()
        );
        var->parent_refs = parent_refs;
        var->index = index;
        return var;
    }

#ifdef COMPILER_BUILD

    bool add_child_index(Codegen& gen, std::vector<llvm::Value *>& indexes, const chem::string_view& name) final {
        return linked->add_child_index(gen, indexes, name);
    }

    llvm::Value *llvm_load(Codegen& gen, SourceLocation location) final;

    llvm::Value *llvm_pointer(Codegen &gen);

    inline llvm::Value* loadable_llvm_pointer(Codegen& gen) {
        // stored in a struct, always requires a load
        return llvm_pointer(gen);
    }

    llvm::Type *llvm_type(Codegen &gen) final;

#endif

    BaseType* known_type() {
        return linked->known_type();
    }

};

inline void CapturedComptimeValues::evaluate(InterpretScope& scope) {
    for(unsigned i = 0; i < refs.size(); i++) {
        auto& ref = refs[i];
        if(ref.kind == CapturedComptimeRefKind::Variable) {
            if(!ref.evaluated) {
                ref.evaluated = scope.find_value(ref.name);
            }
            ref.node->parent_refs = this;
            ref.node->index = i;
        } else {
            if(!ref.evaluated && ref.call) {
                // evaluate the nested comptime call now, while the enclosing
                // comptime scope and its call stack are still alive
                ref.evaluated = ref.call->evaluated_value(scope);
            }
            ref.call->captured_ref_owner = this;
            ref.call->captured_ref_index = (int) i;
        }
    }
}
