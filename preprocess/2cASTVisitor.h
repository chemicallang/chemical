// Copyright (c) Qinetik 2024.

#include "ast/base/Visitor.h"
#include "compiler/ASTDiagnoser.h"
#include <iosfwd>
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

class CTopLevelDeclarationVisitor;
class CValueDeclarationVisitor;
class CDestructionVisitor;
class CBeforeStmtVisitor;
class CAfterStmtVisitor;

class ToCAstVisitor : public Visitor, public ASTDiagnoser {
public:

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
     * a typedef struct containing two void pointers is prepared
     */
    std::string fat_pointer_type;

    /**
     * allocated values locally, based on Value*
     */
    std::unordered_map<Value*, std::string> local_allocated;

    /**
     * top level declarations will be declared by this visitor
     * for example functions and structs, declared so can be used if declared below their usage
     */
    std::unique_ptr<CTopLevelDeclarationVisitor> tld;

    /**
     * this visitor takes out values like lambda from within functions
     * to file level scope
     */
    std::unique_ptr<CValueDeclarationVisitor> declarer;

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
     * a reference to the stream it's going to write results to
     */
    std::ostream* output;

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
     * when false, function types in struct members (lambdas) are typedef at top level
     */
    bool inline_struct_members_fn_types = true;

    /**
     * when false, function types in function parameters aren typedef at top level
     */
    bool inline_fn_types_in_params = true;

    /**
     * when false, function types in function returns aren typedef at top level
     */
    bool inline_fn_types_in_returns = true;

    /**
     * when not empty, return statement would make a goto to this block instead
     */
    std::string return_redirect_block;

    /**
     * when true, output c will be like c++
     * it'll use bool instead of _Bool for example
     */
    bool cpp_like = false;

    /**
     * constructor
     * @param path the current file path being processed
     */
    ToCAstVisitor(std::ostream* output, const std::string& path);

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
     * used to write a string to a stream
     */
    void write(const std::string& value);

    /**
     * this should be called before calling translate
     */
    void prepare_translate();

    /**
     * will translate given nodes
     */
    template <typename NodesVec>
    void translate(NodesVec& nodes);

    //------------------------------
    //----------Visitors------------
    //------------------------------

    void visitCommon(ASTNode* node) override;

    void visitCommonValue(Value* value) override;

    void visit(VarInitStatement* init) override;

    void visit(AssignStatement* assign) override;

    void visit(BreakStatement* breakStatement) override;

    void visit(Comment* comment) override;

    void visit(ContinueStatement* continueStatement) override;

    void visit(ImportStatement* importStatement) override;

    void visit(ReturnStatement* returnStatement) override;

    void visit(DoWhileLoop* doWhileLoop) override;

    void visit(EnumDeclaration* enumDeclaration) override;

    void visit(ForLoop* forLoop) override;

    void visit(FunctionParam* functionParam) override;

    void visit(FunctionDeclaration* functionDeclaration) override;

    void visit(ExtensionFunction *extensionFunc) override;

    void visit(IfStatement* ifStatement) override;

    void visit(ImplDefinition* implDefinition) override;

    void visit(UnionDef *def) override;

    void visit(InterfaceDefinition* interfaceDefinition) override;

    void visit(Scope* scope) override;

    void visit(StructDefinition* structDefinition) override;

    void visit(Namespace *ns) override;

    void visit(WhileLoop* whileLoop) override;

    void visit(AccessChain* chain) override;

    void visit(MacroValueStatement* statement) override;

    void visit(StructMember* member) override;

    void visit(TypealiasStatement* statement) override;

    void visit(SwitchStatement* statement) override;

    void visit(TryCatch* statement) override;

    // Value Vis override;

    void visit(IntValue *intVal) override;

    void visit(BigIntValue* val) override;

    void visit(LongValue* val) override;

    void visit(ShortValue* val) override;

    void visit(UBigIntValue* val) override;

    void visit(UIntValue* val) override;

    void visit(ULongValue* val) override;

    void visit(UShortValue* val) override;

    void visit(Int128Value* val) override;

    void visit(UInt128Value* val) override;

    void visit(NumberValue* boolVal) override;

    void visit(FloatValue* floatVal) override;

    void visit(DoubleValue* doubleVal) override;

    void visit(CharValue* charVal) override;

    void visit(StringValue* stringVal) override;

    void visit(BoolValue* boolVal) override;

    void visit(ArrayValue* arrayVal) override;

    void visit(StructValue* structValue) override;

    void visit(VariableIdentifier* identifier) override;

    void visit(Expression* expr) override;

    void visit(CastedValue* casted) override;

    void visit(AddrOfValue* casted) override;

    void visit(DereferenceValue* casted) override;

    void visit(FunctionCall* call) override;

    void visit(IndexOperator* op) override;

    void visit(NegativeValue* negValue) override;

    void visit(NotValue* notValue) override;

    void visit(NullValue* nullValue) override;

    void visit(TernaryValue* ternary) override;

    void visit(LambdaFunction* func) override;

    void visit(AnyType* func) override;

    void visit(ArrayType* func) override;

    void visit(BigIntType* func) override;

    void visit(BoolType* func) override;

    void visit(CharType* func) override;

    void visit(DoubleType* func) override;

    void visit(FloatType* func) override;

    void visit(FunctionType* func) override;

    void visit(GenericType* func) override;

    void visit(Int128Type* func) override;

    void visit(IntType* func) override;

    void visit(LongType* func) override;

    void visit(PointerType* func) override;

    void visit(ReferencedType* func) override;

    void visit(ShortType* func) override;

    void visit(StringType* func) override;

    void visit(StructType* func) override;

    void visit(UBigIntType* func) override;

    void visit(UInt128Type* func) override;

    void visit(UIntType* func) override;

    void visit(ULongType* func) override;

    void visit(UShortType* func) override;

    void visit(VoidType* func) override;

    ~ToCAstVisitor();

};

template <typename NodesVec>
void ToCAstVisitor::translate(NodesVec& nodes) {

    // declare the top level things with this visitor
    for(auto& node : nodes) {
        node->accept((Visitor*) tld.get());
    }

    // take out values like lambda from within functions
    for(auto& node : nodes) {
        node->accept((Visitor*) declarer.get());
    }

    // writing
    for(auto& node : nodes) {
        node->accept(this);
    }

}