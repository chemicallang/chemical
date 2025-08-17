// Copyright (c) Chemical Language Foundation 2025.

#include "preprocess/visitors/NonRecursiveVisitor.h"
#include "ast/base/AccessSpecifier.h"
#include "std/chem_string_view.h"
#include <iosfwd>
#include <string>
#include <vector>
#include <memory>

class RepresentationVisitor : public NonRecursiveVisitor<RepresentationVisitor> {
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
     * write the string value to stream
     */
    void write_str(const std::string& value);

    /**
     * used to write a string to a stream
     */
    inline void write(std::string& value) {
        write_str(value);
    }

    /**
     * write this string view to the stream
     */
    void write_view(std::string_view& view);

    /**
     * write this string view to the stream
     */
    void write(const chem::string_view& view);

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

    //------------------------------
    //----------Visitors------------
    //------------------------------

    void VisitCommonNode(ASTNode* node);

    void VisitCommonValue(Value* value);

    void VisitCommonType(BaseType* type);

    void VisitAssignmentStmt(AssignStatement* node);

    void VisitBreakStmt(BreakStatement* node);

    void VisitContinueStmt(ContinueStatement* node);

    // TODO implement this
    void VisitUnreachableStmt(UnreachableStmt* node) {}

    void VisitDeleteStmt(DestructStmt* node);

    void VisitDeallocStmt(DeallocStmt* node);

    void VisitImportStmt(ImportStatement* node);

    void VisitReturnStmt(ReturnStatement* node);

    void VisitSwitchStmt(SwitchStatement* node);

    void VisitThrowStmt(ThrowStatement* node);

    void VisitTypealiasStmt(TypealiasStatement* node);

    void VisitUsingStmt(UsingStmt* node);

    void VisitVarInitStmt(VarInitStatement* node);

    void VisitLoopBlock(LoopBlock* node);

    // TODO implement this
    void VisitProvideStmt(ProvideStmt* node) {}

    // TODO implement this
    void VisitComptimeBlock(ComptimeBlock* node) {}

    void VisitWhileLoopStmt(WhileLoop* node);

    void VisitDoWhileLoopStmt(DoWhileLoop* node);

    void VisitForLoopStmt(ForLoop* node);

    void VisitIfStmt(IfStatement* node);

    void VisitTryStmt(TryCatch* node);

    void VisitValueNode(ValueNode* node);

    void VisitValueWrapper(ValueWrapperNode* node);

    void VisitAccessChainNode(AccessChainNode* node);

    void VisitIncDecNode(IncDecNode* node);

    void VisitPatternMatchExprNode(PatternMatchExprNode* node);

    void VisitPlacementNewNode(PlacementNewNode* node);

    void VisitEnumDecl(EnumDeclaration* node);

    // we handle enums when handling enum declarations
    void VisitEnumMember(EnumMember* node) {}

    void VisitFunctionDecl(FunctionDeclaration* node);

    // TODO handle this
    void VisitMultiFunctionNode(MultiFunctionNode* node) {}

    void VisitImplDecl(ImplDefinition* node);

    void VisitInterfaceDecl(InterfaceDefinition* node);

    // TODO handle init block
    void VisitInitBlock(InitBlock* node) {}

    void VisitStructDecl(StructDefinition* node);

    void VisitStructMember(StructMember* node);

    void VisitNamespaceDecl(Namespace* node);

    void VisitUnionDecl(UnionDef* node);

    void VisitVariantDecl(VariantDefinition* node);

    // TODO handle variant member
    void VisitVariantMember(VariantMember* node) {}

    void VisitUnnamedStruct(UnnamedStruct* node);

    void VisitUnnamedUnion(UnnamedUnion* node);

    void VisitScope(Scope* node);

    void VisitUnsafeBlock(UnsafeBlock* node);

    void VisitFunctionParam(FunctionParam* node);

    void VisitGenericTypeParam(GenericTypeParameter* node);

    // TODO handle variant member param
    void VisitVariantMemberParam(VariantMemberParam* node) {}

    // TODO handle captured variable
    void VisitCapturedVariable(CapturedVariable* node) {}

    // TODO handle variant case variable
    void VisitVariantCaseVariable(VariantCaseVariable* node) {}

    // VALUES

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

    void VisitIfValue(IfStatement* value) {
        VisitIfStmt(value);
    }

    void VisitSwitchValue(SwitchStatement* value) {
        VisitSwitchStmt(value);
    }

    void VisitLoopValue(LoopBlock* value) {
        VisitLoopBlock(value);
    }

    // TODO handle new typed value
    void VisitNewTypedValue(NewTypedValue* value) {}

    // TODO handle new value
    void VisitNewValue(NewValue* value) {}

    // TODO handle placement new value
    void VisitPlacementNewValue(PlacementNewValue* value) {}

    // TODO handle inc dec value
    void VisitIncDecValue(IncDecValue* value) {}

    void VisitIsValue(IsValue* value);

    void VisitInValue(InValue* value);

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

    // TODO handle unsafe value
    void VisitUnsafeValue(UnsafeValue* value) {}

    // TODO error out when present not supposed to be present
    void VisitComptimeValue(ComptimeValue* value) {}

    void VisitAlignOfValue(AlignOfValue* value);

    void VisitVariantCase(VariantCase* value);

    void VisitAddrOfValue(AddrOfValue* value);

    // TODO handle this
    void VisitPointerValue(PointerValue* value) {}

    // TODO handle this
    void VisitBlockValue(BlockValue* value) {}

    // TODO handle this
    void VisitWrapValue(WrapValue* value) {}

    // TODO handle this
    void VisitDestructValue(DestructValue* value) {}

    // Types

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

    void VisitCapturingFunctionType(CapturingFunctionType* type);

    // TODO handle this
    void VisitExpressionType(ExpressionType* type) {}

    void VisitNullPtrType(NullPtrType* type);

    ~RepresentationVisitor();

};