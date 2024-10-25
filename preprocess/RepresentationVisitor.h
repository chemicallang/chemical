// Copyright (c) Qinetik 2024.

#include "ast/base/Visitor.h"
#include "ast/base/AccessSpecifier.h"
#include <iosfwd>
#include <string>
#include <vector>
#include <memory>

class RepresentationVisitor : public Visitor {
public:

    /**
     * this should be set to true
     */
    bool top_level_node = true;

    /**
     * a reference to the stream it's going to write results to
     */
    std::ostream& output;

    /**
     * 0 means in root, no indentation
     * 1 means a single '\t' and so on...
     */
    unsigned int indentation_level = 0;

    /**
     * interpret representation means, no quotes on strings or characters basically
     */
    bool interpret_representation = false;

    /**
     * if true, function calls won't have a semicolon at the end
     */
    bool nested_value = false;

    /**
     * constructor
     */
    explicit RepresentationVisitor(std::ostream& output);

    /**
     * used to write a character to the stream
     */
    void write(char value);

    /**
     * indentation of \t or spaces will be added for current indentation level
     */
    void indent();

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
    void write(std::string& value);

    /**
     * write this string view to the stream
     */
    void write(const std::string_view& view);

    /**
     * this should be called before calling translate
     */
    void prepare_translate();

    /**
     * will translate given nodes
     */
    void translate(std::vector<ASTNode*>& nodes);

    /**
     * access specifier is written and true is returned if written
     */
    bool write(AccessSpecifier specifier);

    /**
     * writes the access specifier and if written, writes a space
     */
    void write_ws(AccessSpecifier specifier) {
        if(write(specifier)) {
            write(' ');
        }
    }

    /**
     * comma separated
     */
    template<typename T>
    void comma_separated_accept(T& things);

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

    void visit(ImportStatement* importStatement) final;

    void visit(ReturnStatement* returnStatement) final;

    void visit(DoWhileLoop* doWhileLoop) final;

    void visit(EnumDeclaration* enumDeclaration) final;

    void visit(ForLoop* forLoop) final;

    void visit(FunctionParam* functionParam) final;

    void visit(FunctionDeclaration* functionDeclaration) final;

    void visit(IfStatement* ifStatement) final;

    void visit(ImplDefinition* implDefinition) final;

    void visit(InterfaceDefinition* interfaceDefinition) final;

    void visit(Namespace *ns) final;

    void visit(Scope* scope) final;

    void visit(StructDefinition* structDefinition) final;

    void visit(GenericTypeParameter *type_param) final;

    void visit(UnsafeBlock *block) final;

    void visit(WhileLoop* whileLoop) final;

    void visit(AccessChain* chain) final;

    void visit(MacroValueStatement* statement) final;

    void visit(StructMember* member) final;

    void visit(TypealiasStatement* statement) final;

    void visit(SwitchStatement* statement) final;

    void visit(TryCatch* statement) final;

    // Value Vis override;

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

    void visit(StringValue* stringVal) final;

    void visit(BoolValue* boolVal) final;

    void visit(ArrayValue* arrayVal) final;

    void visit(StructValue* structValue) final;

    void visit(VariableIdentifier* identifier) final;

    void visit(Expression* expr) final;

    void visit(CastedValue* casted) final;

    void visit(AddrOfValue* casted) final;

    void visit(DereferenceValue* casted) final;

    void visit(FunctionCall* call) final;

    void visit(IndexOperator* op) final;

    void visit(NegativeValue* negValue) final;

    void visit(NotValue* notValue) final;

    void visit(NullValue* nullValue) final;

    void visit(LambdaFunction* func) final;

    void visit(AnyType* func) final;

    void visit(ArrayType* func) final;

    void visit(BigIntType* func) final;

    void visit(BoolType* func) final;

    void visit(CharType* func) final;

    void visit(DoubleType* func) final;

    void visit(FloatType* func) final;

    void visit(Float128Type *type) final;

    void visit(LongDoubleType *type) final;

    void visit(ComplexType *floatVal) final;

    void visit(FunctionType* func) final;

    void visit(GenericType* func) final;

    void visit(Int128Type* func) final;

    void visit(IntType* func) final;

    void visit(LongType* func) final;

    void visit(PointerType* type) final;

    void visit(ReferenceType *type) final;

    void visit(LinkedType* func) final;

    void visit(ShortType* func) final;

    void visit(StringType* func) final;

    void visit(StructType* func) final;

    void visit(UBigIntType* func) final;

    void visit(UInt128Type* func) final;

    void visit(UIntType* func) final;

    void visit(ULongType* func) final;

    void visit(UShortType* func) final;

    void visit(VoidType* func) final;

    void visit(LoopBlock *scope) final;

    void visit(ValueNode *node) final;

    void visit(VariantCall *call) final;

    void visit(IsValue *casted) final;

    void visit(DestructStmt *delStmt) final;

    void visit(VariantCase *chain) final;

    void visit(VariantDefinition *variant_def) final;

    void visit(DynamicType *type) final;

    void visit(SizeOfValue *size_of) final;

    void visit(RetStructParamValue *paramVal) final;

    void visit(UsingStmt *usingStmt) final;

    void visit(LinkedValueType *ref_type) final;

    void visit(UCharType *uchar) final;

    void visit(LiteralType *func) final;

    void visit(UnnamedStruct *def) final;

    void visit(UnnamedUnion *def) final;

    void visit(UnionDef *def) final;

    void visit(ExtensionFunction *extensionFunc) final;

    void visit(ExtensionFuncReceiver *receiver) final;

    void visit(ThrowStatement *throwStmt) final;

    void visit(UCharValue *charVal) final;

    void visit(UnionType *unionType) final;

    void visitCommonType(BaseType *value) final;

    ~RepresentationVisitor();

};

template<typename T>
void RepresentationVisitor::comma_separated_accept(T& things) {
    bool has_before = false;
    for(auto& sub_type : things) {
        if(has_before) {
            write(", ");
        } else {
            has_before = true;
        }
        sub_type->accept(this);
    }
}