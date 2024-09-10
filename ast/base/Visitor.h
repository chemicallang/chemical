// Copyright (c) Qinetik 2024.

#pragma once

// Nodes

class ASTNode;

class BaseType;

class VarInitStatement;

class AssignStatement;

class BreakStatement;

class Comment;

class ContinueStatement;

class VariantMember;

class ImportStatement;

class ReturnStatement;

class DoWhileLoop;

class EnumDeclaration;

class ForLoop;

class UnionDef;

class FunctionParam;

class FunctionDeclaration;

class IfStatement;

class ImplDefinition;

class InterfaceDefinition;

class Scope;

class LoopBlock;

class StructDefinition;

class WhileLoop;

class AccessChain;

class MacroValueStatement;

class StructMember;

class VariantCase;

class ValueNode;

class UnnamedUnion;

class UnnamedStruct;

class TypealiasStatement;

class SwitchStatement;

class VariantDefinition;

class TryCatch;

// Values Begin

class Value;

class IntValue;

class BigIntValue;

class LongValue;

class ShortValue;

class UBigIntValue;

class UIntValue;

class UCharValue;

class ULongValue;

class UShortValue;

class FloatValue;

class Int128Value;

class UInt128Value;

class DoubleValue;

class CharValue;

class StringValue;

class BoolValue;

class ArrayValue;

class StructValue;

class VariableIdentifier;

class NamespaceIdentifier;

class Expression;

class AccessChain;

class CastedValue;

class IsValue;

class AddrOfValue;

class RetStructParamValue;

class DereferenceValue;

class FunctionCall;

class VariantCall;

class IndexOperator;

class NegativeValue;

class NotValue;

class NullValue;

class NumberValue;

class TernaryValue;

class SizeOfValue;

class LambdaFunction;

class AnyType;

class ArrayType;

class BigIntType;

class BoolType;

class CharType;

class DoubleType;

class FloatType;

class FunctionType;

class GenericType;

class UnionType;

class Int128Type;

class IntType;

class LongType;

class PointerType;

class LinkedType;

class GenericTypeParameter;

class LinkedValueType;

class StructMemberInitializer;

class ShortType;

class UCharType;

class StringType;

class StructType;

class UBigIntType;

class UInt128Type;

class LiteralType;

class UIntType;

class ULongType;

class UShortType;

class VoidType;

class DynamicType;

class ExtensionFunction;

class ExtensionFuncReceiver;

class ThrowStatement;

class DestructStmt;

class UsingStmt;

class Namespace;

// Visitor Class

class Visitor {
public:

    virtual void visitCommon(ASTNode* node) {
        // do nothing
    }

    virtual void visitCommonValue(Value* value) {
        // do nothing
    }

    virtual void visitCommonType(BaseType* value) {
        // do nothing
    }

    virtual void visit(VarInitStatement* init) {
        visitCommon((ASTNode*) init);
    }

    virtual void visit(AssignStatement* assign) {
        visitCommon((ASTNode*) assign);
    }

    virtual void visit(BreakStatement* breakStatement) {
        visitCommon((ASTNode*) breakStatement);
    }

    virtual void visit(Comment* comment) {
        visitCommon((ASTNode*) comment);
    }

    virtual void visit(ContinueStatement* continueStatement) {
        visitCommon((ASTNode*) continueStatement);
    }

    virtual void visit(ImportStatement* importStatement) {
        visitCommon((ASTNode*) importStatement);
    }

    virtual void visit(ThrowStatement* throwStmt) {
        visitCommon((ASTNode*) throwStmt);
    }

    virtual void visit(UsingStmt* usingStmt) {
        visitCommon((ASTNode*) usingStmt);
    }

    virtual void visit(GenericTypeParameter* type_param) {
        visitCommonType((BaseType*) type_param);
    }

    virtual void visit(DestructStmt* delStmt) {
        visitCommon((ASTNode*) delStmt);
    }

    virtual void visit(ReturnStatement* returnStatement) {
        visitCommon((ASTNode*) returnStatement);
    }

    virtual void visit(DoWhileLoop* doWhileLoop) {
        visitCommon((ASTNode*) doWhileLoop);
    }

    virtual void visit(EnumDeclaration* enumDeclaration) {
        visitCommon((ASTNode*) enumDeclaration);
    }

    virtual void visit(ForLoop* forLoop) {
        visitCommon((ASTNode*) forLoop);
    }

    virtual void visit(ExtensionFuncReceiver* receiver) {
        visitCommon((ASTNode*) receiver);
    }

    virtual void visit(FunctionParam* functionParam) {
        visitCommon((ASTNode*) functionParam);
    }

    virtual void visit(FunctionDeclaration* functionDeclaration) {
        visitCommon((ASTNode*) functionDeclaration);
    }

    virtual void visit(ExtensionFunction* extensionFunc) {
        visitCommon((ASTNode*) extensionFunc);
    }

    virtual void visit(IfStatement* ifStatement) {
        visitCommon((ASTNode*) ifStatement);
    }

    virtual void visit(ImplDefinition* implDefinition) {
        visitCommon((ASTNode*) implDefinition);
    }

    virtual void visit(Namespace* ns) {
        visitCommon((ASTNode*) ns);
    }

    virtual void visit(InterfaceDefinition* interfaceDefinition) {
        visitCommon((ASTNode*) interfaceDefinition);
    }

    virtual void visit(VariantDefinition* variant_def) {
        visitCommon((ASTNode*) variant_def);
    }

    virtual void visit(Scope* scope) {
        visitCommon((ASTNode*) scope);
    }

    virtual void visit(LoopBlock* scope) {
        visitCommon((ASTNode*) scope);
    }

    virtual void visit(StructDefinition* structDefinition) {
        visitCommon((ASTNode*) structDefinition);
    }

    virtual void visit(UnionDef* def) {
        visitCommon((ASTNode*) def);
    }

    virtual void visit(UnnamedStruct* def) {
        visitCommon((ASTNode*) def);
    }

    virtual void visit(UnnamedUnion* def) {
        visitCommon((ASTNode*) def);
    }

    virtual void visit(WhileLoop* whileLoop) {
        visitCommon((ASTNode*) whileLoop);
    }

    virtual void visit(AccessChain* chain) {
        visitCommon((ASTNode*) chain);
    }

    virtual void visit(VariantCase* chain) {
        visitCommon((ASTNode*) chain);
    }

    virtual void visit(MacroValueStatement* statement) {
        visitCommon((ASTNode*) statement);
    }

    virtual void visit(StructMember* member) {
        visitCommon((ASTNode*) member);
    }

    virtual void visit(StructMemberInitializer* init) {
        visitCommon((ASTNode*) init);
    }

    virtual void visit(TypealiasStatement* statement) {
        visitCommon((ASTNode*) statement);
    }

    virtual void visit(SwitchStatement* statement) {
        visitCommon((ASTNode*) statement);
    }

    virtual void visit(TryCatch* statement) {
        visitCommon((ASTNode*) statement);
    }

    // Value Visit Methods

    virtual void visit(IntValue* intVal) {
        visitCommonValue((Value*) intVal);
    }

    virtual void visit(BigIntValue* val) {
        visitCommonValue((Value*) val);
    }

    virtual void visit(LongValue* val) {
        visitCommonValue((Value*) val);
    }

    virtual void visit(ShortValue* val) {
        visitCommonValue((Value*) val);
    }

    virtual void visit(UBigIntValue* val) {
        visitCommonValue((Value*) val);
    }

    virtual void visit(UIntValue* val) {
        visitCommonValue((Value*) val);
    }

    virtual void visit(ULongValue* val) {
        visitCommonValue((Value*) val);
    }

    virtual void visit(UShortValue* val) {
        visitCommonValue((Value*) val);
    }

    virtual void visit(Int128Value* val) {
        visitCommonValue((Value*) val);
    }

    virtual void visit(UInt128Value* val) {
        visitCommonValue((Value*) val);
    }

    virtual void visit(NumberValue* boolVal) {
        visitCommonValue((Value*) boolVal);
    }

    virtual void visit(FloatValue* floatVal) {
        visitCommonValue((Value*) floatVal);
    }

    virtual void visit(DoubleValue* doubleVal) {
        visitCommonValue((Value*) doubleVal);
    }

    virtual void visit(CharValue* charVal) {
        visitCommonValue((Value*) charVal);
    }

    virtual void visit(UCharValue* charVal) {
        visitCommonValue((Value*) charVal);
    }

    virtual void visit(StringValue* stringVal) {
        visitCommonValue((Value*) stringVal);
    }

    virtual void visit(BoolValue* boolVal) {
        visitCommonValue((Value*) boolVal);
    }

    virtual void visit(ArrayValue* arrayVal) {
        visitCommonValue((Value*) arrayVal);
    }

    virtual void visit(StructValue* structValue) {
        visitCommonValue((Value*) structValue);
    }

    virtual void visit(VariableIdentifier* identifier) {
        visitCommonValue((Value*) identifier);
    }

    virtual void visit(Expression* expr) {
        visitCommonValue((Value*) expr);
    }

    virtual void visit(CastedValue* casted) {
        visitCommonValue((Value*) casted);
    }

    virtual void visit(IsValue* casted) {
        visitCommonValue((Value*) casted);
    }

    virtual void visit(AddrOfValue* casted) {
        visitCommonValue((Value*) casted);
    }

    virtual void visit(DereferenceValue* casted) {
        visitCommonValue((Value*) casted);
    }

    virtual void visit(FunctionCall* call) {
        visitCommonValue((Value*) call);
    }

    virtual void visit(VariantCall* call) {
        visitCommonValue((Value*) call);
    }

    virtual void visit(ValueNode* node) {
        visitCommonValue((Value*) node);
    }

    virtual void visit(IndexOperator* op) {
        visitCommonValue((Value*) op);
    }

    virtual void visit(RetStructParamValue* paramVal) {
        visitCommonValue((Value*) paramVal);
    }

    virtual void visit(NegativeValue* negValue) {
        visitCommonValue((Value*) negValue);
    }

    virtual void visit(NotValue* notValue) {
        visitCommonValue((Value*) notValue);
    }

    virtual void visit(NullValue* nullValue) {
        visitCommonValue((Value*) nullValue);
    }

    virtual void visit(TernaryValue* ternary) {
        visitCommonValue((Value*) ternary);
    }

    virtual void visit(SizeOfValue* size_of) {
        visitCommonValue((Value*) size_of);
    }

    virtual void visit(LambdaFunction* func) {
        visitCommonValue((Value*) func);
    }

    virtual void visit(AnyType* func) {
        visitCommonType((BaseType*) func);
    }

    virtual void visit(ArrayType* func) {
        visitCommonType((BaseType*) func);
    }

    virtual void visit(BigIntType* func) {
        visitCommonType((BaseType*) func);
    }

    virtual void visit(BoolType* func) {
        visitCommonType((BaseType*) func);
    }

    virtual void visit(CharType* func) {
        visitCommonType((BaseType*) func);
    }

    virtual void visit(UCharType* uchar) {
        visitCommonType((BaseType*) uchar);
    }

    virtual void visit(DoubleType* func) {
        visitCommonType((BaseType*) func);
    }

    virtual void visit(FloatType* func) {
        visitCommonType((BaseType*) func);
    }

    virtual void visit(FunctionType* func) {
        visitCommonType((BaseType*) func);
    }

    virtual void visit(GenericType* func) {
        visitCommonType((BaseType*) func);
    }

    virtual void visit(Int128Type* func) {
        visitCommonType((BaseType*) func);
    }

    virtual void visit(IntType* func) {
        visitCommonType((BaseType*) func);
    }

    virtual void visit(LongType* func) {
        visitCommonType((BaseType*) func);
    }

    virtual void visit(PointerType* func) {
        visitCommonType((BaseType*) func);
    }

    virtual void visit(LinkedType* func) {
        visitCommonType((BaseType*) func);
    }

    virtual void visit(LinkedValueType* ref_type) {
        visitCommonType((BaseType*) ref_type);
    }

    virtual void visit(ShortType* func) {
        visitCommonType((BaseType*) func);
    }

    virtual void visit(StringType* func) {
        visitCommonType((BaseType*) func);
    }

    virtual void visit(StructType* func) {
        visitCommonType((BaseType*) func);
    }

    virtual void visit(UnionType* unionType) {
        visitCommonType((BaseType*) unionType);
    }

    virtual void visit(LiteralType* func) {
        visitCommonType((BaseType*) func);
    }

    virtual void visit(DynamicType* type) {
        visitCommonType((BaseType*) type);
    }

    virtual void visit(UBigIntType* func) {
        visitCommonType((BaseType*) func);
    }

    virtual void visit(UInt128Type* func) {
        visitCommonType((BaseType*) func);
    }

    virtual void visit(UIntType* func) {
        visitCommonType((BaseType*) func);
    }

    virtual void visit(ULongType* func) {
        visitCommonType((BaseType*) func);
    }

    virtual void visit(UShortType* func) {
        visitCommonType((BaseType*) func);
    }

    virtual void visit(VoidType* func) {
        visitCommonType((BaseType*) func);
    }

};