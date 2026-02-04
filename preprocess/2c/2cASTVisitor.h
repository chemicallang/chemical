// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "compiler/ASTDiagnoser.h"
#include "ast/base/ASTAny.h"
#include <string>
#include <vector>
#include <unordered_map>
#include "CTopLevelDeclVisitor.h"
#include "CBeforeStmtVisitor.h"
#include "CAfterStmtVisitor.h"
#include "CValueDeclVisitor.h"
#include "CDestructionVisitor.h"
#include "std/chem_string_view.h"
#include "preprocess/visitors/NonRecursiveVisitor.h"
#include "compiler/mangler/NameMangler.h"
#include "core/source/LocationManager.h"
#include "BufferedWriter.h"

class GlobalInterpretScope;
class ASTAllocator;
class CompilerBinder;
class CDestructionVisitor;

class FunctionType;
class MembersContainer;

class ToCAstVisitor : public NonRecursiveVisitor<ToCAstVisitor>, public ASTDiagnoser {
public:

    /**
     * the location manager is used to emit locations
     */
    LocationManager& loc_man;

    /**
     * compile time interpret scope
     */
    GlobalInterpretScope& comptime_scope;

    /**
     * name mangler that would mangle the names
     */
    NameMangler& mangler;

    /**
     * required to replace embedded node and embedded values
     */
    CompilerBinder& binder;

    /**
     * the writer is used to actually write to an in memory buffer
     * the in memory buffer is really large
     */
    BufferedWriter writer;

    /**
     * this option is here to support struct initialization in tinyCC compiler
     * llvm uses the same approach whereby if a function returns a struct
     * we change it's return type to void and pass that struct as a pointer parameter to the function
     * allocation of the struct is done at function call
     */
    bool pass_structs_to_initialize = true;

    /**
     * is system 64bit
     */
    bool is64Bit = true;

    /**
     * when true, output c will be like c++
     * it'll use bool instead of _Bool for example
     */
    bool cpp_like = false;

    /**
     * when on, line directives will be written
     */
    bool line_directives = false;

    /**
     * this minifies the C code, removing any indentation
     * and new lines
     */
    bool minify = false;

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
     * array types are by default emitted as pointer types
     */
    bool array_types_as_subscript = false;

    /**
     * if true, function calls won't have a semicolon at the end
     */
    bool nested_value = false;

    /**
     * a typedef struct containing two void pointers is prepared
     */
    std::string fat_pointer_type;

    /**
     * store the name of the current assignable
     */
    std::string current_assignable;

    /**
     * allocated values locally, based on Value*
     */
    std::unordered_map<Value*, std::string> local_allocated;

    /**
     * destructible_refs are references to structs / function calls that created structs inside function calls
     * and we're basically going to destruct them after the function call
     */
    std::unordered_map<Value*, std::string> destructible_refs;

    /**
     * comptime functions are evaluated once and put on this unordered map
     * once used, they are disposed as well
     */
    std::unordered_map<FunctionCall*, Value*> evaluated_func_calls;

    /**
     * map tells which static interfaces have implementations
     * at the end, we iterate over every static interface here and generate a stub
     */
    std::unordered_map<InterfaceDefinition*, bool> unimplemented_static_interfaces;

    /**
     * implicit arguments are stored on this unordered map
     */
    std::unordered_map<chem::string_view, Value*> implicit_args;

    /**
     * this visitor takes out values like lambda from within functions
     * to file level scope
     */
    CValueDeclarationVisitor declarer;

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
    CBeforeStmtVisitor before_stmt;

    /**
     * after writing a statement, the whole usage of the value has been final for example
     * x.y.z().d here z calls a function that returns a struct which has d as a member
     * after this statement completes, we've stored d in a variable, the struct returned by z
     * may have a destructor we may like to call
     */
    CAfterStmtVisitor after_stmt;

    /**
     * this destruction visitor, calls destructors on things when it's required
     */
    CDestructionVisitor destructor;

    /**
     * the function type for which code is being generated
     */
    FunctionTypeBody* current_func_type = nullptr;

    /**
     * allocator
     */
    ASTAllocator& allocator;

    /**
     * when not empty, return statement would make a goto to this block instead
     */
    std::string return_redirect_block;

    /**
     * sometimes temporary var names are allocated
     */
    std::vector<std::string> allocated_temp_var_names;

    /**
     * constructor
     * @param path the current file path being processed
     */
    ToCAstVisitor(
        CompilerBinder& binder,
        GlobalInterpretScope& global,
        NameMangler& mangler,
        ASTAllocator& allocator,
        LocationManager& manager,
        bool debug_info,
        bool minify
    );

    /**
     * used to write a character to the stream
     */
    inline void write(char value) noexcept {
        writer.append_char(value);
    }

    /**
     * indentation of \t or spaces will be added for current indentation level
     */
    void indent();

    /**
     * mangle and write to output the runtime name of given node
     */
    inline void mangle(ASTNode* node) {
        mangler.mangle(writer, node);
    }

    /**
     * mangle and write to output the runtime name of given node
     */
    inline void mangle(FunctionDeclaration* decl) {
        mangler.mangle(writer, decl);
    }

    /**
     * get a local variable name, that is unique
     */
    std::string get_local_temp_var_name();

    /**
     * get a local variable name that is a view
     */
    chem::string_view get_local_temp_var_name_view() {
        allocated_temp_var_names.emplace_back(get_local_temp_var_name());
        return chem::string_view(allocated_temp_var_names.back());
    }

    /**
     * emits a new line and writes line directives if needed
     */
    inline void new_line_no_check(SourceLocation location) {
        write('\n');
        if(line_directives) {
            write_line_directive(location);
            write('\n');
        }
    }

    /**
     * emits a new line, line directive if required
     */
    inline void new_line(SourceLocation location) {
        if(minify) return;
        new_line_no_check(location);
    }

    /**
     * helper method
     */
    inline void new_line_and_indent(SourceLocation location) {
        if(minify) return;
        new_line_no_check(location);
        indent();
    }

    /**
     * helper method
     */
    inline void new_line(ASTNode* node) {
        new_line(node->encoded_location());
    }
    /**
     * helper method
     */
    inline void new_line(Value* value) {
        new_line(value->encoded_location());
    }
    /**
     * helper method
     */
    inline void new_line(const TypeLoc& type) {
        new_line(type.encoded_location());
    }
    /**
     * helper method
     */
    inline void new_line_and_indent(ASTNode* node) {
        new_line_and_indent(node->encoded_location());
    }
    /**
     * helper method
     */
    inline void new_line_and_indent(Value* value) {
        new_line_and_indent(value->encoded_location());
    }
    /**
     * helper method
     */
    inline void new_line_and_indent(const TypeLoc& type) {
        new_line_and_indent(type.encoded_location());
    }

    /**
     * write a new line and indent to the indentation level
     */
    inline void new_line() {
        if(minify) return;
        write('\n');
    }

    /**
     * creates a new line and indents to current indentation level
     */
    inline void new_line_and_indent() {
        if(minify) return;
        write('\n');
        indent();
    }

    /**
     * used to insert a space in stream
     */
    inline void space() noexcept {
        write(' ');
    }

    /**
     * write a number
     */
    inline void write(unsigned int num) noexcept {
        writer << num;
    }

    /**
     * used to write a string to a stream
     */
    inline void write(std::string& str) noexcept {
        writer.append(str.data(), str.size());
    }

    /**
     * will write this string to stream
     */
    inline void write_str(const std::string& str) noexcept {
        writer.append(str.data(), str.size());
    }

    /**
     * write a chemical string
     */
    inline void write(chem::string& str) noexcept {
        writer.append(str.data(), str.size());
    }

    /**
     * write the string view to stream
     */
    inline void write(std::string_view& str) noexcept {
        writer.append(str.data(), str.size());
    }

    /**
     * write the string view to stream
     */
    inline void write(chem::string_view& str) {
        writer.append(str.data(), str.size());
    }

    /**
     * write this string view to stream
     */
    inline void write(const chem::string_view& str) noexcept {
        writer.append(str.data(), str.size());
    }

    /**
     * write the view encoded
     */
    void write_str_value(const chem::string_view& view);

    /**
     * this writes a line directive
     */
    void write_line_directive(unsigned int lineNumber, const chem::string_view& file_path) {
        write("#line ");
        write(lineNumber);
        write(' ');
        write_str_value(file_path);
    }

    /**
     * writes the line directive for given source location
     */
    void write_line_directive(SourceLocation location) {
        auto loc_pos = loc_man.getLocationPos(location);
        auto filePath = loc_man.getPathForFileId(loc_pos.fileId);
        write_line_directive(loc_pos.start.line + 1, chem::string_view(filePath));
    }

#ifdef DEBUG

    /**
     * write a debug comment
     */
    void debug_comment(const chem::string_view& value, bool new_line = true) {
        if(minify) return;
        if(new_line) new_line_and_indent();
        write("/** ");
        write(value);
        write(" **/");
    }

#else

    /**
     * write a debug comment
     */
    inline void debug_comment(const chem::string_view& value, bool new_line = true) {

    }

#endif

    /**
     * store this static interface, for generation of stub implementation of it
     * unless it gets implemented by the user
     */
    void store_static_interface_for_stub_impl(InterfaceDefinition* def) {
        auto found = unimplemented_static_interfaces.find(def);
        if(found == unimplemented_static_interfaces.end()) {
            unimplemented_static_interfaces[def] = true;
        }
    }

    /**
     * remove this static interface from storage, so stub implementation for it
     * should not be generated
     */
    void remove_static_interface_for_stub_impl(InterfaceDefinition* def) {
        auto found = unimplemented_static_interfaces.find(def);
        if(found != unimplemented_static_interfaces.end()) {
            unimplemented_static_interfaces.erase(found);
        }
    }

    /**
     * destructs the scopes above, limiting to job index (begin_until)
     */
    void destruct_scopes_above(Value* returnValue, int begin_until);

    /**
     * destruct the current scope and scopes above
     */
    inline void destruct_scopes_above(Value* returnValue) {
        destruct_scopes_above(returnValue, 0);
    }

    /**
     * destructs scopes above, however stops at recent loop (continue stmt does this)
     */
    void destruct_till_loop_scope_above();

    /**
     * this function mutates the value based on type, however it doesn't check for implicit constructors
     */
    void accept_mutating_value_explicit(BaseType* type, Value* value);

    /**
     * when a value is implicitly mutable, for example var x : dyn Phone = SmartPhone {}
     * here SmartPhone struct is initialized and put inside a fat pointer represented by dyn Phone
     * implicitly, this can be called, this will account for mutating values
     * @see lang/docs/notes/MutatingValue.md
     */
    void accept_mutating_value(BaseType* type, Value* value);

    /**
     * this should be called before calling translate
     */
    void prepare_translate();

    /**
     * end the translation, this would implement all the static interfaces
     */
    void end_translate();

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
     * forward declare a node
     */
    void fwd_declare(ASTNode* node);

    /**
     * forward declare new instantiations of generics
     */
    void ext_fwd_declare(ASTNode* node);

    /**
     * forward declare a node
     */
    void declare_type_alias(ASTNode* node);

    /**
     * forward declare a type
     */
    void fwd_declare(BaseType* type);

    /**
     * forward declare these nodes
     */
    void fwd_declare(std::vector<ASTNode*>& nodes) {
        for(const auto node : nodes) {
            fwd_declare(node);
        }
    }

    /**
     * forward declare new instantiations of generics
     */
    void ext_fwd_declare(std::vector<ASTNode*>& nodes) {
        for(const auto node : nodes) {
            ext_fwd_declare(node);
        }
    }

    /**
     * declare type aliases before declaring functions
     */
    void declare_type_aliases(std::vector<ASTNode*>& nodes) {
        for(const auto node : nodes) {
            declare_type_alias(node);
        }
    }

    /**
     * will only declare these external (from another module) nodes
     */
    void external_declare(std::vector<ASTNode*>& nodes);

    /**
     * will add any implementations from external module
     * why ? because usage of generics from imported module causes new instantiations
     * which require generating code after both modules have declared themselves
     */
    void external_implement(std::vector<ASTNode*>& nodes);

    /**
     * does the return value require 'return' keyword before it
     */
    bool requires_return(Value* val);

    /**
     * its like writing a return statement
     */
    void writeReturnStmtFor(Value* value);

    /**
     * write the if statement as a value
     */
    void writeIfStmtValue(IfStatement& value);

    /**
     * write the switch statement as a value
     */
    void writeSwitchStmtValue(SwitchStatement& value, BaseType* type);

    /**
     * the write the loop block as a value
     */
    void writeLoopStmtValue(LoopBlock& value, BaseType* type);

    /**
     * write the return value, the type is the type of the function
     */
    void return_value(Value* val, BaseType* type);

    void write_identifier(VariableIdentifier* value, bool is_first);

    void visit_value_scope(Scope* scope, unsigned destruct_begin);

    void visit_scope(Scope *scope, unsigned destruct_begin);

    // NODES

    void VisitAssignmentStmt(AssignStatement* node);

    void VisitBreakStmt(BreakStatement* node);

    void VisitContinueStmt(ContinueStatement* node);

    void VisitUnreachableStmt(UnreachableStmt* node);

    void VisitDeleteStmt(DestructStmt* node);

    void VisitDeallocStmt(DeallocStmt* node);

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

    void VisitForLoopStmt(ForLoop* node);

    void VisitIfStmt(IfStatement* node);

    void VisitTryStmt(TryCatch* node);

    void VisitValueNode(ValueNode* node);

    void VisitValueWrapper(ValueWrapperNode* node);

    void VisitAccessChainNode(AccessChainNode* node);

    void VisitIncDecNode(IncDecNode* node);

    void VisitPatternMatchExprNode(PatternMatchExprNode* node);

    void VisitPlacementNewNode(PlacementNewNode *node);

    void VisitEnumDecl(EnumDeclaration* node);

    // enum members are handled along with enum declaration
    void VisitEnumMember(EnumMember* node) {}

    void VisitFunctionDecl(FunctionDeclaration* node);

    void VisitGenericFuncDecl(GenericFuncDecl* node);

    // TODO handle multi function node
    void VisitMultiFunctionNode(MultiFunctionNode* node) {}

    void VisitImplDecl(ImplDefinition* node);

    void VisitInterfaceDecl(InterfaceDefinition* node);

    void VisitInitBlock(InitBlock* node);

    void VisitGenericTypeDecl(GenericTypeDecl* node);

    void VisitStructDecl(StructDefinition* node);

    void VisitGenericStructDecl(GenericStructDecl* node);

    void VisitGenericUnionDecl(GenericUnionDecl* node);

    void VisitGenericInterfaceDecl(GenericInterfaceDecl* node);

    void VisitGenericVariantDecl(GenericVariantDecl* node);

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

    // we handle generic type parameters along with function / struct
    void VisitGenericTypeParam(GenericTypeParameter* node) {}

    // variant member paremeters are also handle along with variant decl
    void VisitVariantMemberParam(VariantMemberParam* node) {}

    // are handled in when processing lambda
    void VisitCapturedVariable(CapturedVariable* node) {}

    // are handled in switch or related constructs
    void VisitVariantCaseVariable(VariantCaseVariable* node) {}

    void VisitEmbeddedNode(EmbeddedNode* node);

    // Values

    void VisitIntNValue(IntNumValue* value);

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

    void VisitInValue(InValue* value);

    void VisitIfValue(IfValue* value);

    void VisitSwitchValue(SwitchValue* value);

    void VisitLoopValue(LoopValue* value);

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

    // comptime values are replaced
    // throw error if it exists
    void VisitComptimeValue(ComptimeValue* value);

    void VisitAlignOfValue(AlignOfValue* value);

    void VisitVariantCase(VariantCase* value);

    void VisitAddrOfValue(AddrOfValue* value);

    void VisitPatternMatchExpr(PatternMatchExpr* value);

    void VisitMultipleValue(MultipleValue* value);

    void VisitRawLiteral(RawLiteral* value);

    // TODO handle pointer value
    void VisitPointerValue(PointerValue* value) {}

    void VisitBlockValue(BlockValue* value);

    // TODO handle wrap value
    void VisitWrapValue(WrapValue* value) {}

    // destruct value is just used for interpretation
    // TODO throw error if it exists
    void VisitDestructValue(DestructValue* value) {}

    void VisitExtractionValue(ExtractionValue* value);

    void VisitEmbeddedValue(EmbeddedValue* value);

    void VisitDynamicValue(DynamicValue* value);

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

    void VisitCapturingFunctionType(CapturingFunctionType* type);

    void VisitGenericType(GenericType* type);

    void VisitIntNType(IntNType* type);

    void VisitPointerType(PointerType* type);

    void VisitReferenceType(ReferenceType* type);

    void VisitLinkedType(LinkedType* type);

    void VisitStringType(StringType* type);

    void VisitDynamicType(DynamicType* type);

    void VisitVoidType(VoidType* type);

    void VisitLiteralType(LiteralType* type);

    void VisitMaybeRuntimeType(MaybeRuntimeType* type);

    void VisitRuntimeType(RuntimeType* type);

    // TODO expression type
    void VisitExpressionType(ExpressionType* type) {}

    void VisitNullPtrType(NullPtrType* type);

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