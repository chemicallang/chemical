// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include <vector>
#include <memory>
#include <string>
#include "ast/base/TypeLoc.h"
#include "ast/base/ValueKind.h"
#include "ast/structures/FunctionParam.h"

#ifdef COMPILER_BUILD
#include "compiler/llvmfwd.h"
#include "ast/base/AnnotationKind.h"

class Codegen;
#endif

class BaseType;

class FunctionParam;

class ExtensionFunction;

class FunctionDeclaration;

class LocatedIdentifier;

class Value;

class ASTNode;

class ASTDiagnoser;

struct FunctionTypeData {
    /**
     * extension functions are same as functions, however are children of containers
     */
    bool isExtension = false;
    /**
     * is the function variadic
     */
    bool isVariadic = false;
    /**
     * this is marked true, when symbol resolution of the signature completes successfully
     */
    bool signature_resolved = false;
    /**
     * is the function capturing
     */
    bool isCapturing = false;
};

static_assert(sizeof(FunctionTypeData) <= 8);

class FunctionDeclaration;

class FunctionType : public BaseType {
public:

    std::vector<FunctionParam*> params;
    TypeLoc returnType = nullptr;
    // if the function is variadic, the last type in params is the type given to the variadic parameter
    /**
     * function type data
     */
    FunctionTypeData data;

    /**
     * constructor
     */
    constexpr FunctionType(
            TypeLoc returnType,
            bool isVariadic,
            bool isCapturing,
            bool signature_resolved
    ) : data(false, isVariadic, signature_resolved, isCapturing), returnType(returnType),
        BaseType(BaseTypeKind::Function) {

    }

    /**
     * constructor
     */
    constexpr FunctionType(
            TypeLoc returnType,
            bool isExtension,
            bool isVariadic,
            bool isCapturing,
            bool signature_resolved
    ) : data(isExtension, isVariadic, signature_resolved, isCapturing), returnType(returnType),
        BaseType(BaseTypeKind::Function) {

    }


    [[nodiscard]]
    inline bool isExtensionFn() const {
        return data.isExtension;
    }

    inline void setIsExtension(bool value) {
        data.isExtension = value;
    }

    [[nodiscard]]
    inline bool isCapturing() const {
        return data.isCapturing;
    }

    [[nodiscard]]
    inline bool isVariadic() const {
        return data.isVariadic;
    }

    inline void setIsCapturing(bool capturing) {
        data.isCapturing = capturing;
    }

    inline void setIsVariadic(bool variadic) {
        data.isVariadic = variadic;
    }

    /**
     * copy this function type into another
     */
    void shallow_copy_into(FunctionType& other, ASTAllocator& allocator, ASTNode* new_parent) {
        other.params = params;
        other.returnType = returnType;
        other.data = data;
    }

    /**
     * copy this function type into another
     */
    void copy_into(FunctionType& other, ASTAllocator& allocator, ASTNode* new_parent) {
        other.params.reserve(params.size());
        for(auto& param : params) {
            const auto new_param = param->copy(allocator);
            new_param->set_parent(new_parent);
            other.params.emplace_back(new_param);
        }
        other.returnType = returnType.copy(allocator);
        other.data = data;
    }

    /**
     * check if these args satisfy, this is useful, if calling a constructor
     * user provides the arguments, we check arguments against params, to see if it's compatible
     * if not, another function is selected that is compatible with arguments provided
     */
    bool satisfy_args(std::vector<Value*>& forArgs);

    /**
     * get the total implicit parameters
     */
    unsigned total_implicit_params();

    /**
     * suppose this function takes a self argument passed implicitly
     * the index of arguments would start at 1
     */
    inline unsigned explicit_func_arg_offset() {
        const auto ext_offset = isExtensionFn() ? 1 : 0;
        return total_implicit_params() + ext_offset;
    }

    /**
     * get function param for argument index
     */
    FunctionParam* func_param_for_arg_at(unsigned index);

    /**
     * get implicit parameter for given name
     */
    FunctionParam* implicit_param_for(const chem::string_view& name);

    /**
     * do parameter types match with the given function parameter types
     */
    bool do_param_types_match(std::vector<FunctionParam*>& param_types, bool check_self = true);

    bool isInVarArgs(unsigned index) const;

    unsigned int expectedArgsSize();

    uint64_t byte_size(bool is64Bit) final;

    ASTNode* linked_node() override {
        return returnType->linked_node();
    }

    virtual ExtensionFunction* as_extension_func() {
        return nullptr;
    }

    virtual FunctionDeclaration *as_function() {
        return nullptr;
    }

    virtual LambdaFunction* as_lambda() {
        return nullptr;
    }

    virtual LocatedIdentifier* get_func_name_id() {
        return nullptr;
    }

    inline bool equal_type(FunctionType *other) const {
        return equal(other);
    }

    bool is_same(BaseType *other) final {
        return other->kind() == kind() && equal_type(static_cast<FunctionType *>(other));
    }

    bool satisfies(BaseType *type) override;

    [[nodiscard]]
    FunctionType* copy(ASTAllocator& allocator) override;

#ifdef COMPILER_BUILD

    virtual std::vector<llvm::Type *> param_types(Codegen &gen);

    void queue_destruct_params(Codegen &gen);

    llvm::Type *llvm_type(Codegen &gen);

    llvm::FunctionType *llvm_func_type(Codegen &gen);

#endif

    /**
     * get the self parameter of the function if it exists
     */
    virtual FunctionParam* get_self_param();

    /**
     * whether this function requires self parameter
     */
    bool has_self_param() {
        return get_self_param() != nullptr;
    }

    /**
     * does function has explicit parameters
     */
    bool has_explicit_params();

    /**
     * start index of c or llvm functions for this type
     */
    virtual unsigned c_or_llvm_arg_start_index();

    /**
     * check if this function type is equal to other
     */
    bool equal(FunctionType *other) const;

    /**
     * virtual destructor
     */
    ~FunctionType() override = default;

};

/**
 * this should only be inherited by functions or lambdas
 * those that have bodies
 */
class FunctionTypeBody : public FunctionType {
public:

    using FunctionType::FunctionType;

    /**
     * moved identifiers are stored in this vector, this is similar to moved_chains, single variable
     * identifiers are not stored in access chains, so to simplify storage and so to not having to deal with
     * multiple types in the vector, this vector has been created for singular identifiers, why is this simple
     * since when finding a moved chain, we find the smallest chain possible, the singular identifiers are
     * smallest, so if we find a single identifier which are easier to search we can return fast
     */
    std::vector<VariableIdentifier*> moved_identifiers;
    /**
     * moved chains are stored in current function type, which are filled when a function is linked
     * moved chains belong to this function (inside the function's body), these chains tell which objects have
     * been moved inside them
     */
    std::vector<AccessChain*> moved_chains;

    /**
     * get the parent node
     */
    virtual ASTNode* get_parent() = 0;

    /**
     * get the encoded location
     */
    virtual SourceLocation get_location() = 0;

    /**
       * copy function type body into the other
       */
    inline void shallow_copy_into(FunctionTypeBody& other, ASTAllocator& allocator, ASTNode* new_parent) {
        FunctionType::shallow_copy_into(other, allocator, new_parent);
    }

    /**
     * copy function type body into the other
     */
    inline void copy_into(FunctionTypeBody& other, ASTAllocator& allocator, ASTNode* new_parent) {
        FunctionType::copy_into(other, allocator, new_parent);
    }

    // Extracts elements from `index` to the end and stores them in `backup`, removing them from `original`
    template<typename T>
    void extractEnd(std::vector<T>& original, std::vector<T>& backup, size_t index) {
        if (index >= original.size()) return; // No elements to extract
        backup.assign(original.begin() + index, original.end()); // Store in backup
        original.erase(original.begin() + index, original.end()); // Remove from original
    }

    // Restores the backed-up elements into the original vector at the end
    template<typename T>
    inline void restoreEnd(std::vector<T>& original, const std::vector<T>& backup) {
        original.insert(original.end(), backup.begin(), backup.end()); // Append backup to original
    }

    inline void save_moved_ids_after(std::vector<VariableIdentifier*>& backup, std::size_t index) {
        extractEnd(moved_identifiers, backup, index);
    }

    inline void save_moved_chains_after(std::vector<AccessChain*>& backup, std::size_t index) {
        extractEnd(moved_chains, backup, index);
    }

    void erase_moved_ids_after(std::size_t index) {
        moved_identifiers.erase(moved_identifiers.begin() + index, moved_identifiers.end());
    }

    void erase_moved_chains_after(std::size_t index) {
        moved_chains.erase(moved_chains.begin() + index, moved_chains.end());
    }

    inline void restore_moved_ids(const std::vector<VariableIdentifier*>& backup) {
        restoreEnd(moved_identifiers, backup);
    }

    inline void restore_moved_chains(const std::vector<AccessChain*>& backup) {
        restoreEnd(moved_chains, backup);
    }

    /**
     * called by return statement to set the return value and stop
     * interpretation of function body
     */
    virtual void set_return(InterpretScope& scope, Value* value) = 0;

    /**
     * un_move a chain, if found to be moved
     * return true if found and removed, otherwise false
     */
    bool un_move_chain(AccessChain* chain);

    /**
     * un_move a moved id, which matches functionally (linked with same node)
     * return true if found and removed, otherwise false
     * this only removes, exact identifiers, doesn't check access chains
     */
    bool un_move_exact_id(VariableIdentifier* id);

    /**
     * un_move a moved chain, which matches functionally
     * the first value of the chain will be matching with the given identifier functionally
     * linked with same node
     */
    bool un_move_chain_with_first_id(VariableIdentifier* id);

    /**
     * the function that you should call, if you want to unmove an identifier
     * this will check for functional equality, meaning the identifier's linkage is checked
     * access chains with first identifier functionally equal to given identifier is also removed
     */
    bool un_move_id(VariableIdentifier* id);

    /**
     * will find a identifier who has linked node same as the given identifier
     */
    VariableIdentifier* find_moved_id(VariableIdentifier* id);

    /**
     * an access is found that partially matches the given access chain, his checks partially matching moved chains
     *
     * for example when consider_nested_members and consider_last_member are true:
     * for given 'm' if only 'm.x' has been moved, we return it (nested members considered)
     * for given 'm.x' if only 'm' has been moved, we return it (parent member considered)
     * for given 'm.x' if only 'm.y' has been moved, we return null (unrelated not considered)
     * for given 'm.x' if only 'm.x' has been moved, we return it (last member considered)
     *
     * for example when consider_nested_members and consider_last_member are false:
     * for given 'm.x' if only 'm.x.y' has been moved, we return null (nested members not considered)
     * for given 'm.x' if only 'm' has been moved, we return it (parent member being considered)
     * for given 'm.x' if only 'm.y' has been moved, we return null (unrelated nor considered)
     * for given 'm.x' if only 'm.x' has been moved, we return null (last member not considered)
     */
    AccessChain* find_partially_matching_moved_chain(AccessChain& chain, bool consider_nested_members, bool consider_last_member);

    /**
     * for an identifier, finds the smallest moved access chain, for example
     * for 'x', it could be 'x.a' or 'x.a.b.c' but for 'x' you'd never get 'x'
     */
    AccessChain* find_smallest_moved_access_chain(VariableIdentifier* id);

    /**
     * for an identifier, finds the smallest moved access chain, for example
     * for 'x', it could be 'x.a' or 'x.a.b.c' but for 'x' you'd never get 'x'
     */
    AccessChain* find_moved_access_chain(VariableIdentifier* id);

    /**
     * find's a chain value (identifier or access chain) that matches the identifier functionally
     * in the case of access chain, the first element is expected to be an identifier, that is linked
     * with same node as the given identifier
     */
    ChainValue* find_moved_chain_value(VariableIdentifier* id);

    /**
     * the ultimate function that should be used to check for moved chain values
     */
    ChainValue* find_moved_chain_value(AccessChain* chain_ptr);

    /**
     * marks given chain moved without checking
     */
    void mark_moved_no_check(AccessChain* chain);

    /**
     * marks given identifier moved without checking
     */
    void mark_moved_no_check(VariableIdentifier* id);

    /**
     * check if the given access chain is accessible or assignable (depending on bool assigning)
     * for example when assigning or accessing x.y.z
     * access:
     * an error when 'x' or 'x.y'  has been moved
     * an error when 'x.y.z' has been moved (considers the last member)
     * an error if a nested member after 'z' has been moved (considers nested members)
     *
     * assignment:
     * error if 'x' or 'x.y' has been moved
     * no error when 'x.y.z' has been moved (doesn't consider the last member)
     * no error if a nested member after 'z' has been moved (doesn't consider nested members)
     *
     * @return false if error was caused
     */
    bool check_chain(AccessChain* chain, bool assigning, ASTDiagnoser& diagnoser);

    /**
     * check if the given identifier is accessible or assignable (depending on bool assigning)
     * for example when assigning or accessing x
     * access:
     * an error when 'x' or 'x.y'  has been moved
     * an error when 'x.y.z' has been moved (considers the last member)
     * an error if a nested member after 'z' has been moved (considers nested members)
     *
     * assignment:
     * a single identifier moved or not moved, is always assignable !
     * at the moment, references support is limited
     */
    bool check_id(VariableIdentifier* id, ASTDiagnoser& diagnoser);

    /**
     * specific function marking identifier moved
     */
    bool mark_moved_id(VariableIdentifier* value, ASTDiagnoser& diagnoser);

    /**
     * checks if the value is movable and moves it (marks it move and all that)
     * @return true if moved otherwise false
     */
    bool mark_moved_value(Value* value, ASTDiagnoser& diagnoser);

    /**
     * check if the given value is movable
     */
    bool is_value_movable(Value* value_ptr, BaseType* type);

    /**
     * the following value will be moved, by checking the expected type
     * this then takes into account, moves into implicit constructors
     */
    bool mark_moved_value(
            ASTAllocator& allocator,
            Value* value_ptr,
            BaseType* expected_type,
            ASTDiagnoser& diagnoser,
            bool check_implicit_constructors = true
    );

    /**
     * it will try to un move the given access chain or identifier value pointer
     * by checking if it's moved
     * This is called by the assignment statement when lhs is a moved value and assignment is
     * being done, when called the access chain or identifier is un moved. making it usable
     */
    bool mark_un_moved_lhs_value(Value* value_ptr, BaseType* value_type);

};

#ifdef COMPILER_BUILD

/**
 * creates llvm function return type, based on our return type of the function
 */
llvm::Type* llvm_func_return(Codegen &gen, BaseType* type);

/**
 * add a single param type to param types
 */
void llvm_func_param_type(
        Codegen &gen,
        std::vector<llvm::Type*>& paramTypes,
        BaseType* type
);

/**
 * creates llvm function parameter types, based on our parameter types of the function
 */
void llvm_func_param_types_into(
        Codegen &gen,
        std::vector<llvm::Type*>& paramTypes,
        std::vector<FunctionParam*>& params,
        BaseType* returnType,
        bool isCapturing,
        bool isVariadic,
        FunctionDeclaration* decl
);

/**
 * creates llvm function parameter types, based on our parameter types of the function
 * a helper to call function defined above it
 */
inline std::vector<llvm::Type*> llvm_func_param_types(
        Codegen &gen,
        std::vector<FunctionParam*>& params,
        BaseType* returnType,
        bool isCapturing,
        bool isVariadic,
        FunctionDeclaration* decl
) {
    std::vector<llvm::Type*> paramTypes;
    llvm_func_param_types_into(gen, paramTypes, params, returnType, isCapturing, isVariadic, decl);
    return paramTypes;
}

#endif