// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "compiler/ASTDiagnoser.h"
#include "ast/base/ASTAny.h"
#include <iosfwd>
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include "CTopLevelDeclVisitor.h"
#include "std/chem_string_view.h"
#include "preprocess/visitors/NonRecursiveVisitor.h"

class GlobalInterpretScope;
class ASTAllocator;

class CTopLevelDeclarationVisitor;
class CValueDeclarationVisitor;
class CDestructionVisitor;
class CBeforeStmtVisitor;
class CAfterStmtVisitor;

class FunctionType;
class MembersContainer;

class ToCAstVisitor : public NonRecursiveVisitor<ToCAstVisitor>, public ASTDiagnoser {
public:

    /**
     * compile time interpret scope
     */
    GlobalInterpretScope& comptime_scope;

    /**
     * this option is here to support struct initialization in tinyCC compiler
     * llvm uses the same approach whereby if a function returns a struct
     * we change it's return type to void and pass that struct as a pointer parameter to the function
     * allocation of the struct is done at function call
     */
    bool pass_structs_to_initialize = true;

    /**
     * this should be set to true
     */
    bool top_level_node = true;

    /**
     * is system 64bit
     */
    bool is64Bit = true;

    /**
     * whether to output debug comments or not
     */
#ifdef DEBUG
    bool debug_comments = true;
#else
    bool debug_comments = false;
#endif

    /**
     * a typedef struct containing two void pointers is prepared
     */
    std::string fat_pointer_type;

    /**
     * allocated values locally, based on Value*
     */
    std::unordered_map<Value*, std::string> local_allocated;

    /**
     * comptime functions are evaluated once and put on this unordered map
     * once used, they are disposed as well
     */
    std::unordered_map<FunctionCall*, Value*> evaluated_func_calls;

    /**
     * implicit arguments are stored on this unordered map
     */
    std::unordered_map<chem::string_view, Value*> implicit_args;

    /**
     * this visitor takes out values like lambda from within functions
     * to file level scope
     */
    std::unique_ptr<CValueDeclarationVisitor> declarer;

    /**
     * top level declarations will be declared by this visitor
     * for example functions and structs, declared so can be used if declared below their usage
     */
    CTopLevelDeclarationVisitor tld;

    /**
     * before writing a statement, it's values can be visited with this visitor
     * this allows to take out some values, or do preparation, for example to allocate struct values
     * before passing them to functions for initialization
     */
    std::unique_ptr<CBeforeStmtVisitor> before_stmt;

    /**
     * after writing a statement, the whole usage of the value has been final for example
     * x.y.z().d here z calls a function that returns a struct which has d as a member
     * after this statement completes, we've stored d in a variable, the struct returned by z
     * may have a destructor we may like to call
     */
    std::unique_ptr<CAfterStmtVisitor> after_stmt;

    /**
     * this destruction visitor, calls destructors on things when it's required
     */
    std::unique_ptr<CDestructionVisitor> destructor;

    /**
     * the function type for which code is being generated
     */
    FunctionType* current_func_type = nullptr;

    /**
     * current members container to which functions belong
     */
    MembersContainer* current_members_container = nullptr;

    /**
     * the interfaces are collected in this vector, if this is not null
     * and no code is generated for compiler interface (compiler:interface annotation)
     */
    std::vector<std::string>* const compiler_interfaces;

    /**
     * a reference to the stream it's going to write results to
     */
    std::ostream* output;

    /**
     * allocator
     */
    ASTAllocator& allocator;

    /**
     * 0 means in root, no indentation
     * 1 means a single '\t' and so on...
     */
    unsigned int indentation_level = 0;

    /**
     * local temporary variable counter, we create these variables for allocations
     */
    unsigned int local_temp_var_i = 0;

    /**
     * if true, function calls won't have a semicolon at the end
     */
    bool nested_value = false;

    /**
     * when not empty, return statement would make a goto to this block instead
     */
    std::string return_redirect_block;

    // --------- Configuration Variables ------------------

    /**
     * by default it takes out enum values for debugging purposes
     */
    bool inline_enum_member_access = true;

    /**
     * when true, output c will be like c++
     * it'll use bool instead of _Bool for example
     */
    bool cpp_like = false;

    /**
     * constructor
     * @param path the current file path being processed
     */
    ToCAstVisitor(
        GlobalInterpretScope& global,
        std::ostream* output,
        ASTAllocator& allocator,
        LocationManager& manager,
        std::vector<std::string>* compiler_interfaces = nullptr
    );

    /**
     * used to write a character to the stream
     */
    void write(char value);

    /**
     * indentation of \t or spaces will be added for current indentation level
     */
    void indent();

    /**
     * get a local variable name, that is unique
     */
    std::string get_local_temp_var_name();

    /**
     * write a new line and indent to the indentation level
     */
    inline void new_line() {
        write('\n');
    }

    /**
     * creates a new line and indents to current indentation level
     */
    inline void new_line_and_indent() {
        new_line();
        indent();
    }

    /**
     * used to insert a space in stream
     */
    inline void space() {
        write(' ');
    }

    /**
     * accept any node to this visitor and receive a string instead
     */
    std::string string_accept(Value* any);

    /**
     * used to write a string to a stream
     */
    void write(std::string& str);

    /**
     * will write this string to stream
     */
    void write_str(const std::string& str);

    /**
     * write the string view to stream
     */
    void write(std::string_view& str);

    /**
     * write the string view to stream
     */
    void write(chem::string_view& str);

    /**
     * write this string view to stream
     */
    void write(const chem::string_view& str);

    /**
     * write a debug comment
     */
    void debug_comment(const chem::string_view& value, bool new_line = true) {
        if(debug_comments) {
            write("/** ");
            write(value);
            write(" **/");
            if(new_line) new_line_and_indent();
        }
    }

    /**
     * this function mutates the value based on type, however it doesn't check for implicit constructors
     */
    void accept_mutating_value_explicit(BaseType* type, Value* value, bool assigning_value);

    /**
     * when a value is implicitly mutable, for example var x : dyn Phone = SmartPhone {}
     * here SmartPhone struct is initialized and put inside a fat pointer represented by dyn Phone
     * implicitly, this can be called, this will account for mutating values
     * @see lang/docs/notes/MutatingValue.md
     */
    void accept_mutating_value(BaseType* type, Value* value, bool assigning_value);

    /**
     * this should be called before calling translate
     */
    void prepare_translate();

    /**
     * this should be called after translating one set of nodes (belonging to a single file)
     * so the visitor can be reused to translate another set of nodes
     * you could create another visitor, but that might be too expensive
     */
    void reset();

    /**
     * declare nodes before translating them
     * suppose to work along with function translate_after_declaration
     */
    void declare_before_translation(std::vector<ASTNode*>& nodes);

    /**
     * translate nodes after declaring them
     * suppose to work along with function declare_before_translation
     */
    void translate_after_declaration(std::vector<ASTNode*>& nodes);

    /**
     * will translate given nodes
     */
    inline void declare_and_translate(std::vector<ASTNode*>& nodes) {
        declare_before_translation(nodes);
        translate_after_declaration(nodes);
    }

    /**
     * will only declare these external (from another module) nodes
     */
    void external_declare(std::vector<ASTNode*>& nodes);

    /**
     * does the return value require 'return' keyword before it
     */
    bool requires_return(Value* val);

    /**
     * write the return value, the type is the type of the function
     */
    void return_value(Value* val, BaseType* type);

    void write_identifier(VariableIdentifier* value, bool is_first);

    void visit_scope(Scope *scope, unsigned destruct_begin);

    // NODES

    void VisitAssignmentStmt(AssignStatement* node);

    void VisitBreakStmt(BreakStatement* node);

    void VisitContinueStmt(ContinueStatement* node);

    void VisitUnreachableStmt(UnreachableStmt* node);

    void VisitDeleteStmt(DestructStmt* node);

    void VisitImportStmt(ImportStatement* node);

    void VisitReturnStmt(ReturnStatement* node);

    void VisitSwitchStmt(SwitchStatement* node);

    // throw statement is not supported, we may remove it
    void VisitThrowStmt(ThrowStatement* node) {}

    void VisitTypealiasStmt(TypealiasStatement* node);

    void VisitUsingStmt(UsingStmt* node) {
        // we do not handle using stmt as it's just for symbol resolution
    }

    void VisitVarInitStmt(VarInitStatement* node);

    void VisitLoopBlock(LoopBlock* node);

    void VisitProvideStmt(ProvideStmt* node);

    void VisitComptimeBlock(ComptimeBlock* node);

    void VisitWhileLoopStmt(WhileLoop* node);

    void VisitDoWhileLoopStmt(DoWhileLoop* node);

    // TODO throw error when found, suppose to replaced
    void VisitSymResNode(SymResNode* node) {}

    void VisitForLoopStmt(ForLoop* node);

    void VisitIfStmt(IfStatement* node);

    void VisitTryStmt(TryCatch* node);

    void VisitValueNode(ValueNode* node);

    void VisitValueWrapper(ValueWrapperNode* node);

    void VisitEnumDecl(EnumDeclaration* node);

    // enum members are handled along with enum declaration
    void VisitEnumMember(EnumMember* node) {}

    void VisitFunctionDecl(FunctionDeclaration* node);

    void VisitGenericFuncDecl(GenericFuncDecl* node);

    void VisitGenericStructDecl(GenericStructDecl* node);

    void VisitExtensionFunctionDecl(ExtensionFunction* node);

    // TODO handle multi function node
    void VisitMultiFunctionNode(MultiFunctionNode* node) {}

    void VisitImplDecl(ImplDefinition* node);

    void VisitInterfaceDecl(InterfaceDefinition* node);

    void VisitInitBlock(InitBlock* node);

    void VisitStructDecl(StructDefinition* node);

    void VisitStructMember(StructMember* node);

    void VisitNamespaceDecl(Namespace* node);

    void VisitUnionDecl(UnionDef* node);

    void VisitVariantDecl(VariantDefinition* node);

    // variant members are handled in variant definition
    void VisitVariantMember(VariantMember* node) {}

    void VisitUnnamedStruct(UnnamedStruct* node);

    void VisitUnnamedUnion(UnnamedUnion* node);

    void VisitScope(Scope* node);

    void VisitUnsafeBlock(UnsafeBlock* node);

    void VisitFunctionParam(FunctionParam* node);

    // we handle extension func receiver when extension func is received
    void VisitExtensionFuncReceiver(ExtensionFuncReceiver* node) {}

    // we handle generic type parameters along with function / struct
    void VisitGenericTypeParam(GenericTypeParameter* node) {}

    // variant member paremeters are also handle along with variant decl
    void VisitVariantMemberParam(VariantMemberParam* node) {}

    // are handled in when processing lambda
    void VisitCapturedVariable(CapturedVariable* node) {}

    // are handled in switch or related constructs
    void VisitVariantCaseVariable(VariantCaseVariable* node) {}

    // Values

    void VisitCharValue(CharValue* value);

    void VisitShortValue(ShortValue* value);

    void VisitIntValue(IntValue* value);

    void VisitLongValue(LongValue* value);

    void VisitBigIntValue(BigIntValue* value);

    void VisitInt128Value(Int128Value* value);

    void VisitUCharValue(UCharValue* value);

    void VisitUShortValue(UShortValue* value);

    void VisitUIntValue(UIntValue* value);

    void VisitULongValue(ULongValue* value);

    void VisitUBigIntValue(UBigIntValue* value);

    void VisitUInt128Value(UInt128Value* value);

    void VisitNumberValue(NumberValue* value);

    void VisitFloatValue(FloatValue* value);

    void VisitDoubleValue(DoubleValue* value);

    void VisitBoolValue(BoolValue* value);

    void VisitStringValue(StringValue* value);

    void VisitExpression(Expression* value);

    void VisitArrayValue(ArrayValue* value);

    void VisitStructValue(StructValue* value);

    void VisitLambdaFunction(LambdaFunction* value);

    void VisitNewTypedValue(NewTypedValue* value);

    void VisitNewValue(NewValue* value);

    void VisitPlacementNewValue(PlacementNewValue* value);

    void VisitIncDecValue(IncDecValue* value);

    void VisitIsValue(IsValue* value);

    void VisitDereferenceValue(DereferenceValue* value);

    void VisitRetStructParamValue(RetStructParamValue* value);

    void VisitAccessChain(AccessChain* value);

    void VisitCastedValue(CastedValue* value);

    void VisitVariableIdentifier(VariableIdentifier* value);

    void VisitIndexOperator(IndexOperator* value);

    void VisitFunctionCall(FunctionCall* value);

    void VisitNegativeValue(NegativeValue* value);

    void VisitNotValue(NotValue* value);

    void VisitNullValue(NullValue* value);

    void VisitSizeOfValue(SizeOfValue* value);

    // TODO throw error as this gets replaced
    void VisitSymResValue(SymResValue* value) {}

    // TODO handle unsafe value
    void VisitUnsafeValue(UnsafeValue* value) {}

    // comptime values are replaced
    // throw error if it exists
    void VisitComptimeValue(ComptimeValue* value) {}

    void VisitAlignOfValue(AlignOfValue* value);

    void VisitVariantCase(VariantCase* value);

    void VisitAddrOfValue(AddrOfValue* value);

    // TODO handle pointer value
    void VisitPointerValue(PointerValue* value) {}

    void VisitBlockValue(BlockValue* value);

    // TODO handle wrap value
    void VisitWrapValue(WrapValue* value) {}

    // destruct value is just used for interpretation
    // TODO throw error if it exists
    void VisitDestructValue(DestructValue* value) {}

    // TYPES

    void VisitAnyType(AnyType* type);

    void VisitArrayType(ArrayType* type);

    void VisitStructType(StructType* type);

    void VisitUnionType(UnionType* type);

    void VisitBoolType(BoolType* type);

    void VisitDoubleType(DoubleType* type);

    void VisitFloatType(FloatType* type);

    void VisitLongDoubleType(LongDoubleType* type);

    void VisitComplexType(ComplexType* type);

    void VisitFloat128Type(Float128Type* type);

    void VisitFunctionType(FunctionType* type);

    void VisitGenericType(GenericType* type);

    void VisitIntNType(IntNType* type);

    void VisitPointerType(PointerType* type);

    void VisitReferenceType(ReferenceType* type);

    void VisitLinkedType(LinkedType* type);

    void VisitStringType(StringType* type);

    void VisitLiteralType(LiteralType* type);

    void VisitDynamicType(DynamicType* type);

    void VisitVoidType(VoidType* type);

    // TODO expression type
    void VisitExpressionType(ExpressionType* type) {}

    ~ToCAstVisitor();

};

inline void SubVisitor::space() const {
    visitor.space();
}

/**
 * write fn using visitor
 */
inline void SubVisitor::write(char value) const {
    visitor.write(value);
}

/**
 * write fn using visitor
 */
inline void SubVisitor::write(const chem::string_view& value) const {
    visitor.write(value);
}

/**
 * new line and indent to current indentation level
 */
inline void SubVisitor::new_line_and_indent() {
    visitor.new_line_and_indent();
}