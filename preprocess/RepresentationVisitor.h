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

    void visit(IfStatement* ifStatement) override;

    void visit(ImplDefinition* implDefinition) override;

    void visit(InterfaceDefinition* interfaceDefinition) override;

    void visit(Namespace *ns) override;

    void visit(Scope* scope) override;

    void visit(StructDefinition* structDefinition) override;

    void visit(GenericTypeParameter *type_param) override;

    void visit(UnsafeBlock *block) override;

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

    void visit(LambdaFunction* func) override;

    void visit(AnyType* func) override;

    void visit(ArrayType* func) override;

    void visit(BigIntType* func) override;

    void visit(BoolType* func) override;

    void visit(CharType* func) override;

    void visit(DoubleType* func) override;

    void visit(FloatType* func) override;

    void visit(Float128Type *type) override;

    void visit(LongDoubleType *type) override;

    void visit(FunctionType* func) override;

    void visit(GenericType* func) override;

    void visit(Int128Type* func) override;

    void visit(IntType* func) override;

    void visit(LongType* func) override;

    void visit(PointerType* type) override;

    void visit(ReferenceType *type) override;

    void visit(LinkedType* func) override;

    void visit(ShortType* func) override;

    void visit(StringType* func) override;

    void visit(StructType* func) override;

    void visit(UBigIntType* func) override;

    void visit(UInt128Type* func) override;

    void visit(UIntType* func) override;

    void visit(ULongType* func) override;

    void visit(UShortType* func) override;

    void visit(VoidType* func) override;

    void visit(LoopBlock *scope) override;

    void visit(ValueNode *node) override;

    void visit(VariantCall *call) override;

    void visit(IsValue *casted) override;

    void visit(DestructStmt *delStmt) override;

    void visit(VariantCase *chain) override;

    void visit(VariantDefinition *variant_def) override;

    void visit(DynamicType *type) override;

    void visit(SizeOfValue *size_of) override;

    void visit(RetStructParamValue *paramVal) override;

    void visit(UsingStmt *usingStmt) override;

    void visit(LinkedValueType *ref_type) override;

    void visit(UCharType *uchar) override;

    void visit(LiteralType *func) override;

    void visit(UnnamedStruct *def) override;

    void visit(UnnamedUnion *def) override;

    void visit(UnionDef *def) override;

    void visit(ExtensionFunction *extensionFunc) override;

    void visit(ExtensionFuncReceiver *receiver) override;

    void visit(ThrowStatement *throwStmt) override;

    void visit(UCharValue *charVal) override;

    void visit(UnionType *unionType) override;

    void visitCommonType(BaseType *value) override;

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