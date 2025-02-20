// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/base/Visitor.h"
#include "compiler/ASTDiagnoser.h"
#include "ast/base/ASTAny.h"
#include <iosfwd>
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include "CTopLevelDeclVisitor.h"
#include "std/chem_string_view.h"

class GlobalInterpretScope;
class ASTAllocator;

class CTopLevelDeclarationVisitor;
class CValueDeclarationVisitor;
class CDestructionVisitor;
class CBeforeStmtVisitor;
class CAfterStmtVisitor;

class FunctionType;
class MembersContainer;

class ToCAstVisitor : public Visitor, public ASTDiagnoser {
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
     * should C's sizeof operator be used instead of our sizeof macro
     */
    bool use_sizeof = false;

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
    std::string string_accept(ASTAny* any);

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

    //------------------------------
    //----------Visitors------------
    //------------------------------

    void visitCommon(ASTNode* node) final;

    void visitCommonValue(Value* value) final;

    void visit(VarInitStatement* init) final;

    void visit(AssignStatement* assign) final;

    void visit(BreakStatement* breakStatement) final;

    void visit(Comment* comment) final;

    void visit(ContinueStatement* continueStatement) final;

    void visit(UnreachableStmt *stmt) final;

    void visit(UnsafeBlock *block) final;

    void visit(ImportStatement* importStatement) final;

    /**
     * does the return value require 'return' keyword before it
     */
    bool requires_return(Value* val);

    /**
     * write the return value, the type is the type of the function
     */
    void return_value(Value* val, BaseType* type);

    void visit(ReturnStatement* returnStatement) final;

    void visit(DoWhileLoop* doWhileLoop) final;

    void visit(EnumDeclaration* enumDeclaration) final;

    void visit(ForLoop* forLoop) final;

    void visit(FunctionParam* functionParam) final;

    void visit(FunctionDeclaration* functionDeclaration) final;

    void visit(ExtensionFunction *extensionFunc) final;

    void visit(IfStatement* ifStatement) final;

    void visit(ImplDefinition* implDefinition) final;

    void visit(UnionDef *def) final;

    void visit(InterfaceDefinition* interfaceDefinition) final;

    void visit_scope(Scope *scope, unsigned destruct_begin);

    void visit(Scope* scope) final;

    void visit(StructDefinition* structDefinition) final;

    void visit(VariantDefinition* variant_def) final;

    void visit(VariantCase *chain) final;

    void visit(VariantCall *call) final;

    void visit(DestructStmt *delStmt) final;

    void visit(IncDecValue *value) override;

    void visit(IsValue *casted) final;

    void visit(NewTypedValue *value) override;

    void visit(NewValue *value) override;

    void visit(PlacementNewValue *value) override;

    void visit(UnnamedUnion *def) final;

    void visit(UnnamedStruct *def) final;

    void visit(Namespace *ns) final;

    void visit(WhileLoop* whileLoop) final;

    void visit(LoopBlock *scope) final;

    void visit(AccessChain* chain) final;

    void visit(InitBlock *initBlock) final;

    void visit(MacroValueStatement* statement) final;

    void visit(StructMember* member) final;

    void visit(ProvideStmt *provideStmt) final;

    void visit(ComptimeBlock *block) final;

    void visit(TypealiasStatement* statement) final;

    void visit(SwitchStatement* statement) final;

    void visit(TryCatch* statement) final;

    void visit(UsingStmt *usingStmt) final {
        // does nothing
    }

    // Value Vis final;

    void visit(IntValue *intVal) final;

    void visit(BigIntValue* val) final;

    void visit(LongValue* val) final;

    void visit(ShortValue* val) final;

    void visit(UBigIntValue* val) final;

    void visit(UIntValue* val) final;

    void visit(ULongValue* val) final;

    void visit(UShortValue* val) final;

    void visit(Int128Value* val) final;

    void visit(UInt128Value* val) final;

    void visit(NumberValue* boolVal) final;

    void visit(FloatValue* floatVal) final;

    void visit(DoubleValue* doubleVal) final;

    void visit(CharValue* charVal) final;

    void visit(UCharValue *charVal) final;

    void visit(StringValue* stringVal) final;

    void visit(BoolValue* boolVal) final;

    void visit(ArrayValue* arrayVal) final;

    void visit(StructValue* structValue) final;

    void visit(VariableIdentifier* identifier) final;

    void visit(SizeOfValue *size_of) final;

    void visit(AlignOfValue *align_of) final;

    void visit(Expression* expr) final;

    void visit(CastedValue* casted) final;

    void visit(AddrOfValue* casted) final;

    void visit(RetStructParamValue *paramVal) final;

    void visit(DereferenceValue* casted) final;

    void visit(FunctionCall* call) final;

    void visit(IndexOperator* op) final;

    void visit(BlockValue *blockVal) override;

    void visit(NegativeValue* negValue) final;

    void visit(NotValue* notValue) final;

    void visit(ValueNode *node) final;

    void visit(ValueWrapperNode *node) override;

    void visit(NullValue* nullValue) final;

    void visit(LambdaFunction* func) final;

    void visit(LiteralType *func) final;

    void visit(AnyType* func) final;

    void visit(ArrayType* func) final;

    void visit(BigIntType* func) final;

    void visit(BoolType* func) final;

    void visit(CharType* func) final;

    void visit(DoubleType* func) final;

    void visit(FloatType* func) final;

    void visit(Float128Type* func) final;

    void visit(LongDoubleType *type) final;

    void visit(ComplexType *type) final;

    void visit(FunctionType* func) final;

    void visit(GenericType* func) final;

    void visit(Int128Type* func) final;

    void visit(IntType* func) final;

    void visit(UCharType *uchar) final;

    void visit(LongType* func) final;

    void visit(PointerType* func) final;

    void visit(ReferenceType* func) final;

    void visit(LinkedType* func) final;

    void visit(LinkedValueType *ref_type) final;

    void visit(DynamicType *type) final;

    void visit(ShortType* func) final;

    void visit(StringType* func) final;

    void visit(StructType* func) final;

    void visit(UnionType *unionType) final;

    void visit(UBigIntType* func) final;

    void visit(UInt128Type* func) final;

    void visit(UIntType* func) final;

    void visit(ULongType* func) final;

    void visit(UShortType* func) final;

    void visit(VoidType* func) final;

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