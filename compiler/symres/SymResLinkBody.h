// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "preprocess/visitors/NonRecursiveVisitor.h"

class SymResLinkBody : public NonRecursiveVisitor<SymResLinkBody> {
public:

    SymbolResolver& linker;

    /**
     * expected type used by values to coerce
     */
    BaseType* expected_type = nullptr;

    /**
     * this is set before visiting type
     */
    SourceLocation type_location = 0;

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
     * the start index of the current lambda scope, only set when in_lambda_scope is true
     * this can be used to verify symbols are inside a lambda scope
     */
    unsigned long lambda_scope_start = 0;

    /**
     * turned on when symbol resolving a lambda body
     */
    bool in_lambda_scope = false;

    /**
     * constructor
     */
    SymResLinkBody(SymbolResolver& linker) : linker(linker) {

    }

    BaseType* getErroredType();

    void LinkMembersContainerNoScope(MembersContainer* container);

    void LinkMembersContainer(MembersContainer* container) {
        linker.scope_start();
        LinkMembersContainerNoScope(container);
        linker.scope_end();
    }

    template<typename T>
    inline void visit(T* ptr) {
        VisitByPtrTypeNoNullCheck(ptr);
    }
    inline void visit(ASTNode* node) {
        VisitNodeNoNullCheck(node);
    }
    inline void visit(BaseDefMember* node) {
        VisitNodeNoNullCheck((ASTNode*) node);
    }
    inline void visit(Value* value) {
        VisitValueNoNullCheck(value);
    }
    inline void visit(Value* value, BaseType* exp_type) {
        const auto prev = expected_type;
        expected_type = exp_type;
        VisitValueNoNullCheck(value);
        expected_type = prev;
    }
    inline void visit(BaseType*& type_ref) {
        VisitTypeNoNullCheck(type_ref);
    }
    inline void visit(BaseType* type, SourceLocation location) {
        type_location = location;
        VisitTypeNoNullCheck(type);
    }
    inline void visit(TypeLoc& type) {
        type_location = type.encoded_location();
        VisitTypeNoNullCheck(const_cast<BaseType*>(type.getType()));
    }
    inline void visit(LinkedType*& type_ref) {
        visit((BaseType*&) type_ref);
    }
    inline void visit(Scope& scope) {
        VisitScope(&scope);
    }

    // Special Visitor Methods

    void VisitAccessChain(AccessChain* chain, bool check_validity, bool assignment);

    void VisitVariableIdentifier(VariableIdentifier* identifier, bool check_access);

    // Visitor Methods

    void VisitAssignmentStmt(AssignStatement *assign);

    void VisitUsingStmt(UsingStmt* node);

    void VisitExportStmt(ExportStmt* node);

    void VisitBreakStmt(BreakStatement* node);

    void VisitDeleteStmt(DestructStmt* node);

    void VisitDeallocStmt(DeallocStmt* node);

    void VisitProvideStmt(ProvideStmt* node);

    void VisitReturnStmt(ReturnStatement* node);

    void VisitSwitchStmt(SwitchStatement *stmt);

    void VisitTypealiasStmt(TypealiasStatement* node);

    void VisitVarInitStmt(VarInitStatement* node);

    void VisitComptimeBlock(ComptimeBlock* node);

    void VisitDoWhileLoopStmt(DoWhileLoop* node);

    void VisitEnumMember(EnumMember* node);

    void VisitEnumDecl(EnumDeclaration* node);

    void VisitForLoopStmt(ForLoop* node);

    void VisitForInLoopStmt(ForInLoop* node);

    void VisitFunctionParam(FunctionParam* node);

    void VisitGenericTypeParam(GenericTypeParameter* node);

    void VisitFunctionDecl(FunctionDeclaration* node);

    void VisitInterfaceDecl(InterfaceDefinition* node);

    void VisitStructDecl(StructDefinition* node);

    void VisitVariantDecl(VariantDefinition* node);

    void VisitCapturedVariable(CapturedVariable* node);

    void VisitGenericFuncDecl(GenericFuncDecl* node);

    void VisitGenericImplDecl(GenericImplDecl* node);

    void VisitGenericInterfaceDecl(GenericInterfaceDecl* node);

    void VisitGenericStructDecl(GenericStructDecl* node);

    void VisitGenericUnionDecl(GenericUnionDecl* node);

    void VisitGenericVariantDecl(GenericVariantDecl* node);

    void VisitIfStmt(IfStatement* node);

    void VisitImplDecl(ImplDefinition* node);

    void VisitNamespaceDecl(Namespace* node);

    void VisitScope(Scope* node);

    void VisitBlockScope(BlockScope* node);

    void VisitLoopBlock(LoopBlock* node);

    void VisitUnionDecl(UnionDef* node);

    void VisitUnsafeBlock(UnsafeBlock* node);

    void VisitVariantCaseVariable(VariantCaseVariable* node);

    void VisitWhileLoopStmt(WhileLoop* node);

    void VisitValueNode(ValueNode* node);

    void VisitMultiFunctionNode(MultiFunctionNode* node);

    void VisitValueWrapper(ValueWrapperNode* node);

    void VisitAccessChainNode(AccessChainNode* node);

    void VisitIncDecNode(IncDecNode* node);

    void VisitPatternMatchExprNode(PatternMatchExprNode* node);

    void VisitPlacementNewNode(PlacementNewNode* node);

    void VisitEmbeddedNode(EmbeddedNode* node);

    // ------------------------------------
    // ----------- Values -----------------
    // ------------------------------------

    void VisitAccessChain(AccessChain *chain);

    void VisitFunctionCall(FunctionCall* value);

    void VisitEmbeddedValue(EmbeddedValue* value);

    void VisitComptimeValue(ComptimeValue* value);

    void VisitIncDecValue(IncDecValue* value);

    void VisitVariantCase(VariantCase* value);

    void VisitArrayType(ArrayType* type);

    void VisitDynamicType(DynamicType* type);

    void VisitFunctionType(FunctionType* type);

    void VisitGenericType(GenericType* type);

    void VisitLinkedType(LinkedType* type);

    void VisitPointerType(PointerType* type);

    void VisitReferenceType(ReferenceType* type);

    void VisitCapturingFunctionType(CapturingFunctionType* type);

    void VisitStructType(StructType* type);

    void VisitUnionType(UnionType* type);

    void VisitIfType(IfType* type);

    void VisitAddrOfValue(AddrOfValue* value);

    void VisitArrayValue(ArrayValue* value);

    void VisitCastedValue(CastedValue* value);

    void VisitDereferenceValue(DereferenceValue* value);

    void VisitExpression(Expression* value);

    void VisitIndexOperator(IndexOperator* value);

    void VisitIsValue(IsValue* value);

    void VisitInValue(InValue* value);

    void VisitLambdaFunction(LambdaFunction* value);

    void VisitNegativeValue(NegativeValue* value);

    void VisitUnsafeValue(UnsafeValue* value);

    void VisitTypeInsideValue(TypeInsideValue* value);

    void VisitNewValue(NewValue* value);

    void VisitNewTypedValue(NewTypedValue* value);

    void VisitPlacementNewValue(PlacementNewValue* value);

    void VisitNotValue(NotValue* value);

    void VisitBitwiseNot(BitwiseNot* value);

    void VisitPatternMatchExpr(PatternMatchExpr* value);

    void VisitSizeOfValue(SizeOfValue* value);

    void VisitAlignOfValue(AlignOfValue* value);

    void VisitIfValue(IfValue* value);

    void VisitSwitchValue(SwitchValue* value);

    void VisitLoopValue(LoopValue* value);

    void VisitStringValue(StringValue* value);

    void VisitStructValue(StructValue* value);

    inline void VisitVariableIdentifier(VariableIdentifier* value) {
        // by default access is checked
        VisitVariableIdentifier(value, true);
    }

    void VisitExpressiveString(ExpressiveString* value);

    void VisitDynamicValue(DynamicValue* value);

    void VisitZeroedValue(ZeroedValue* value);

    // Movement API

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
    Value* find_moved_chain_value(VariableIdentifier* id);

    /**
     * the ultimate function that should be used to check for moved chain values
     */
    Value* find_moved_chain_value(AccessChain* chain_ptr);

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