// Copyright (c) Chemical Language Foundation 2025.

#include "ast/base/GlobalInterpretScope.h"
#include "compiler/lab/LabBuildCompiler.h"
#include "compiler/lab/LabBuildContext.h"
#include "ast/structures/FunctionParam.h"
#include "sstream"
#include <iostream>
#include <utility>
#include "ast/types/VoidType.h"
#include "ast/types/StringType.h"
#include "ast/types/BoolType.h"
#include "ast/values/IntValue.h"
#include "ast/values/Expression.h"
#include "ast/values/BoolValue.h"
#include "ast/values/UBigIntValue.h"
#include "ast/values/CastedValue.h"
#include "ast/values/WrapValue.h"
#include "ast/values/RetStructParamValue.h"
#include "ast/values/StructValue.h"
#include "ast/values/StringValue.h"
#include "ast/values/ArrayValue.h"
#include "ast/values/AccessChain.h"
#include "ast/values/VariableIdentifier.h"
#include "ast/values/FunctionCall.h"
#include "compiler/SymbolResolver.h"
#include "ast/structures/Namespace.h"
#include "ast/structures/StructDefinition.h"
#include "compiler/lab/BackendContext.h"
#include "ast/structures/FunctionParam.h"
#include "ast/types/PointerType.h"
#include "ast/types/LinkedType.h"
#include "ast/types/GenericType.h"
#include "ast/types/UBigIntType.h"
#include "ast/types/AnyType.h"
#include "preprocess/RepresentationVisitor.h"
#include "utils/Version.h"
#include "ast/types/UIntType.h"
#include "ast/values/NullValue.h"
#include "ast/types/ReferenceType.h"
#include "cst/LocationManager.h"
#ifdef COMPILER_BUILD
#include "llvm/TargetParser/Triple.h"
#endif

#include <cstdint>
#if defined(__BYTE_ORDER__) && defined(__ORDER_BIG_ENDIAN__) && defined(__ORDER_LITTLE_ENDIAN__)
#define IS_LITTLE_ENDIAN (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
#elif defined(_WIN32) || defined(_WIN64)
#define IS_LITTLE_ENDIAN 1
#else
#include <endian.h>
    #define IS_LITTLE_ENDIAN (__BYTE_ORDER == __LITTLE_ENDIAN)
#endif

namespace InterpretVector {

    class InterpretVectorNode;

    class InterpretVectorConstructor : public FunctionDeclaration {
    public:

        explicit InterpretVectorConstructor(InterpretVectorNode* node);

        Value* call(InterpretScope *call_scope, ASTAllocator& allocator, FunctionCall *call, Value *parent_val, bool evaluate_refs);

    };

    class InterpretVectorSize : public FunctionDeclaration {
    public:

        FunctionParam selfParam;
        IntType retType;

        explicit InterpretVectorSize(InterpretVectorNode* node);

        Value *call(InterpretScope *call_scope, ASTAllocator& allocator, FunctionCall *call, Value *parent_val, bool evaluate_refs) final;

    };

    class InterpretVectorGet : public FunctionDeclaration {
    public:

        FunctionParam selfParam;
        UIntType indexType;
        FunctionParam indexParam;
        LinkedType returnLinkedType;

        explicit InterpretVectorGet(InterpretVectorNode* node);

        Value *call(InterpretScope *call_scope, ASTAllocator& allocator, FunctionCall *call, Value *parent_val, bool evaluate_refs) final;

    };

    class InterpretVectorPush : public FunctionDeclaration {
    public:

        FunctionParam selfParam;
        LinkedType valueType;
        FunctionParam valueParam;
        VoidType returnVoidType;

        explicit InterpretVectorPush(InterpretVectorNode* node);

        Value *call(InterpretScope *call_scope, ASTAllocator& allocator, FunctionCall *call, Value *parent_val, bool evaluate_refs) final;

    };

    class InterpretVectorRemove : public FunctionDeclaration {
    public:

        FunctionParam selfParam;
        VoidType returnVoidType;
        UIntType indexType;
        FunctionParam indexParam;

        explicit InterpretVectorRemove(InterpretVectorNode* node);

        Value *call(InterpretScope *call_scope, ASTAllocator& allocator, FunctionCall *call, Value *parent_val, bool evaluate_refs) final;

    };

    class InterpretVectorNode : public StructDefinition {
    public:

        GenericTypeParameter typeParam;

        LinkedType selfType;
        ReferenceType selfReference;

        InterpretVectorConstructor constructorFn;
        InterpretVectorSize sizeFn;
        InterpretVectorGet getFn;
        InterpretVectorPush pushFn;
        InterpretVectorRemove removeFn;

        explicit InterpretVectorNode(ASTNode* parent_node);

    };

    class InterpretVectorVal : public StructValue {
    public:
        std::vector<Value*> values;
        explicit InterpretVectorVal(InterpretVectorNode* node) : StructValue(
            nullptr,
            (StructDefinition*) node,
            (StructDefinition*) node,
            ZERO_LOC
        ) {

        }
    };

    InterpretVectorConstructor::InterpretVectorConstructor(InterpretVectorNode* node) : FunctionDeclaration(
            ZERO_LOC_ID("constructor"),
            &node->selfType,
            false,
            node,
            ZERO_LOC,
            AccessSpecifier::Public,
            true
    ) {
        set_constructor_fn(true);
    }

    Value *InterpretVectorConstructor::call(InterpretScope *call_scope, ASTAllocator& allocator, FunctionCall *call, Value *parent_val, bool evaluate_refs) {
        if(call->generic_list.empty()) {
            call_scope->error("expected a generic argument for compiler::vector", call);
            return nullptr;
        }
        const auto node = (InterpretVectorNode*) parent();
        return new (allocator.allocate<InterpretVectorVal>()) InterpretVectorVal(node);
    }

    InterpretVectorSize::InterpretVectorSize(InterpretVectorNode* node) : FunctionDeclaration(
            ZERO_LOC_ID("size"),
            &retType,
            false,
            node,
            ZERO_LOC,
            AccessSpecifier::Public,
            true
    ), retType(ZERO_LOC), selfParam("self", &node->selfReference, 0, nullptr, true, this, ZERO_LOC) {
        params.emplace_back(&selfParam);
    }

    Value *InterpretVectorSize::call(InterpretScope *call_scope, ASTAllocator& allocator, FunctionCall *call, Value *parent_val, bool evaluate_refs) {
        return new (allocator.allocate<IntValue>()) IntValue(static_cast<InterpretVectorVal*>(parent_val)->values.size(), ZERO_LOC);
    }


    // TODO interpret vector get should return a reference to T
    InterpretVectorGet::InterpretVectorGet(InterpretVectorNode* node) : FunctionDeclaration(
            ZERO_LOC_ID("get"),
            &returnLinkedType,
            false,
            node,
            ZERO_LOC,
            AccessSpecifier::Public,
            true
    ), returnLinkedType(&node->typeParam, ZERO_LOC),
        selfParam("self", &node->selfReference, 0, nullptr, true, this, ZERO_LOC), indexType(ZERO_LOC), indexParam("index", &indexType, 1, nullptr, false, this, ZERO_LOC)
    {
        params.emplace_back(&selfParam);
        params.emplace_back(&indexParam);
    }

    Value *InterpretVectorGet::call(InterpretScope *call_scope, ASTAllocator& allocator, FunctionCall *call, Value *parent_val, bool evaluate_refs) {
        const auto index_val = call->values[0]->evaluated_value(*call_scope);
        const auto number = index_val->get_number();
        if(number.has_value()) {
            return static_cast<InterpretVectorVal*>(parent_val)->values[number.value()]->scope_value(*call_scope);
        } else {
            call_scope->error("vector::get only supports integer value as an index", call);
            return new (call_scope->allocate<NullValue>()) NullValue(call->encoded_location());
        }

    }

    InterpretVectorPush::InterpretVectorPush(InterpretVectorNode* node) : FunctionDeclaration(
            ZERO_LOC_ID("push"),
            &returnVoidType,
            false,
            node,
            ZERO_LOC,
            AccessSpecifier::Public,
            true
    ), selfParam("self", &node->selfReference, 0, nullptr, true, this, ZERO_LOC), returnVoidType(ZERO_LOC),
        valueType(&node->typeParam, ZERO_LOC), valueParam("value", &valueType, 1, nullptr, false, this, ZERO_LOC)
    {
        params.emplace_back(&selfParam);
        params.emplace_back(&valueParam);
    }

    Value *InterpretVectorPush::call(InterpretScope *call_scope, ASTAllocator& allocator, FunctionCall *call, Value *parent_val, bool evaluate_refs) {
        static_cast<InterpretVectorVal*>(parent_val)->values.emplace_back(call->values[0]->scope_value(*call_scope));
        return nullptr;
    }

    InterpretVectorRemove::InterpretVectorRemove(InterpretVectorNode* node) : FunctionDeclaration(
            ZERO_LOC_ID("remove"),
            &returnVoidType,
            false,
            node,
            ZERO_LOC,
            AccessSpecifier::Public,
            true
    ), selfParam("self", &node->selfReference, 0, nullptr, true, this, ZERO_LOC), returnVoidType(ZERO_LOC),
        indexType(ZERO_LOC), indexParam("index", &indexType, 1, nullptr, false, this, ZERO_LOC)
    {
        params.emplace_back(&selfParam);
        params.emplace_back(&indexParam);
    }
    Value *InterpretVectorRemove::call(InterpretScope *call_scope, ASTAllocator& allocator, FunctionCall *call, Value *parent_val, bool evaluate_refs) {
        auto& ref = static_cast<InterpretVectorVal*>(parent_val)->values;
        const auto ind = call->values[0]->evaluated_value(*call_scope)->get_number();
        if(ind.has_value()) {
            ref.erase(ref.begin() + ind.value());
        } else {
            call_scope->error("expected a number value for index", call);
        }
        return nullptr;
    }

    InterpretVectorNode::InterpretVectorNode(
        ASTNode* parent_node
    ): StructDefinition(ZERO_LOC_ID("vector"), parent_node, ZERO_LOC, AccessSpecifier::Public),
        constructorFn(this), sizeFn(this), getFn(this), pushFn(this), removeFn(this),
        typeParam("T", nullptr, nullptr, this, 0, ZERO_LOC),
        selfType(this, ZERO_LOC), selfReference(&selfType, ZERO_LOC)
    {
        set_compiler_decl(true);
        insert_functions({ &constructorFn, &sizeFn, &getFn, &pushFn, &removeFn });
    }

}

class InterpretPrint : public FunctionDeclaration {
public:

    std::ostringstream ostring;
    RepresentationVisitor visitor;
    VoidType returnType;


    explicit InterpretPrint(ASTNode* parent_node, LocatedIdentifier identifier) : FunctionDeclaration(
            identifier,
            &returnType,
            true,
            parent_node,
            ZERO_LOC,
            AccessSpecifier::Public,
            true
    ), visitor(ostring),  returnType(ZERO_LOC) {
        visitor.interpret_representation = true;
    }

    inline explicit InterpretPrint(ASTNode* parent_node) : InterpretPrint(parent_node, ZERO_LOC_ID("print")) {

    }

    Value *call(InterpretScope *call_scope, ASTAllocator& allocator, FunctionCall *call, Value *parent_val, bool evaluate_refs) override {
        for (auto const &value: call->values) {
            auto paramValue = value->evaluated_value(*call_scope);
            if(paramValue == nullptr) {
                ostring.write("null", 4);
            } else {
                visitor.visit(paramValue);
            }
        }
        std::cout << ostring.str();
        ostring.str("");
        ostring.clear();
        return nullptr;
    }
};

class InterpretPrintLn : public InterpretPrint {
public:
    inline explicit InterpretPrintLn(ASTNode* parent_node) : InterpretPrint(parent_node, ZERO_LOC_ID("println")) {

    }
    Value* call(InterpretScope *call_scope, ASTAllocator &allocator, FunctionCall *call, Value *parent_val, bool evaluate_refs) override {
        InterpretPrint::call(call_scope, allocator, call, parent_val, evaluate_refs);
        std::cout << std::endl;
        return nullptr;
    }
};

class InterpretToString : public FunctionDeclaration {
public:

    std::ostringstream ostring;
    RepresentationVisitor visitor;
    StringType returnType;

    explicit InterpretToString(ASTNode* parent_node) : FunctionDeclaration(
            ZERO_LOC_ID("to_string"),
            &returnType,
            true,
            parent_node,
            ZERO_LOC,
            AccessSpecifier::Public,
            true
    ), visitor(ostring),  returnType(ZERO_LOC) {
        visitor.interpret_representation = true;
    }

    Value *call(InterpretScope *call_scope, ASTAllocator& allocator, FunctionCall *call, Value *parent_val, bool evaluate_refs) override {
        if(!call->values.empty()) {
            const auto paramValue = call->values.front()->evaluated_value(*call_scope);
            if(paramValue == nullptr) {
                ostring.write("null", 4);
            } else {
                visitor.visit(paramValue);
            }
        }
        const auto& str = ostring.str();
        const auto returnValue = new (allocator.allocate<StringValue>()) StringValue(chem::string_view(allocator.allocate_str(str.data(), str.size()), str.size()), call->encoded_location());
        ostring.str("");
        ostring.clear();
        return returnValue;
    }

};

Value* resolve_ref(Value* val, InterpretScope *call_scope) {
    Value* value = nullptr;
    if(val->reference()) {
        auto linked = val->linked_node();
        if(linked && !linked->as_func_param()) {
            auto holding = linked->holding_value();
            if(holding) {
                value = holding;
            }
        } else {
            value = val->evaluated_value(*call_scope);
        }
    } else {
        value = val->evaluated_value(*call_scope);
    }
    if(value && value->reference()) {
        return resolve_ref(value, call_scope);
    }
    return value;
}

class InterpretSize : public FunctionDeclaration {
public:

    UBigIntType returnType;
    AnyType anyType;
    FunctionParam valueParam;

    explicit InterpretSize(ASTNode* parent_node) : FunctionDeclaration(
            ZERO_LOC_ID("size"),
            &returnType,
            false,
            parent_node,
            ZERO_LOC,
            AccessSpecifier::Public,
            true
    ), returnType(ZERO_LOC), anyType(ZERO_LOC), valueParam("value", &anyType, 0, nullptr, false, this, ZERO_LOC) {
        set_compiler_decl(true);
        params.emplace_back(&valueParam);
    }
    Value *call(InterpretScope *call_scope, ASTAllocator& allocator, FunctionCall *call, Value *parent_val, bool evaluate_refs) final {
        if(call->values.empty()) {
            call_scope->error("compiler::size called without arguments", call);
            return nullptr;
        }
        const auto val = call->values[0];
//        const auto val_type = val->create_type(allocator);
//        const auto val_type_pure = val_type->pure_type(allocator);
//        const auto val_type_kind = val_type_pure->kind();
//        if(val_type_kind != BaseTypeKind::String && val_type_kind != BaseTypeKind::Array) {
//            call_scope->error("compiler::size called with invalid arguments", call);
//            return nullptr;
//        }
        auto value = resolve_ref(val, call_scope);
        if(!value) {
            call_scope->error("couldn't get value for compiler::size", call);
            return nullptr;
        }
        const auto val_kind = value->val_kind();
        if(val_kind != ValueKind::String && val_kind != ValueKind::ArrayValue) {
            call_scope->error("compiler::size called with invalid arguments", call);
            return nullptr;
        }
        switch(val_kind) {
            case ValueKind::String:
                return new (allocator.allocate<UBigIntValue>()) UBigIntValue(value->get_the_string().size(), ZERO_LOC);
            case ValueKind::ArrayValue:
                return new (allocator.allocate<UBigIntValue>()) UBigIntValue(value->as_array_value()->array_size(), ZERO_LOC);
            default:
                return new (allocator.allocate<UBigIntValue>()) UBigIntValue(0, ZERO_LOC);
        }
    }
};

/**
 * evaluates comptime identifiers and function calls in a wrap value
 * suppose compiler::wrap(constructor(value, compiler::size(value)))
 * in this value constructor is a function call that is not comptime
 * so constructor won't be called, but compiler::size is a comptime
 * function call, compiler::size will be called with value as argument
 * so the ending call would become constructor(value, 12) or something
 * since here value is a identifier, which could be pointing to value present
 * inside the function declaration, we replace that with it's evaluated value as well
 */
Value* evaluated_comptime(Value* value, InterpretScope& scope) {
    switch(value->val_kind()) {
        case ValueKind::AccessChain: {
            const auto chain = value->as_access_chain_unsafe();
            unsigned size = chain->values.size();
            const auto first = chain->values[0];
            if(size == 1) {
                return evaluated_comptime(first, scope);
            } else {
                // partially evaluating the access chain
                const auto first_eval = evaluated_comptime(first, scope);
                return evaluate_from(chain->values, scope, first_eval, 1);
            }
        }
        case ValueKind::FunctionCall: {
            const auto call = value->as_func_call_unsafe();
            const auto decl = call->safe_linked_func();
            if(decl && decl->is_comptime()) {
                const auto val = call->evaluated_value(scope);
                if(val->kind() == ValueKind::WrapValue) {
                    // TODO handle nested wrap values
                    // unsure how to handle nested wrap values at the moment (must think about it more)
                    return val->as_wrap_value_unsafe()->underlying;
                } else {
                    return val;
                }
            } else {
                const auto copied = call->copy(scope.allocator);
                copied->parent_val = (ChainValue*) evaluated_comptime(call->parent_val, scope);
                auto i = 0;
                const auto args_size = copied->values.size();
                while(i < args_size) {
                    auto& arg = copied->values[i];
                    arg = evaluated_comptime(arg, scope);
                    i++;
                }
                return copied;
            }
        }
        // explicitly should be evaluated when present in a wrap
        case ValueKind::SizeOfValue:
        case ValueKind::AlignOfValue:
            return value;
        case ValueKind::CastedValue: {
            const auto casted = value->as_casted_value_unsafe();
            const auto copied = (CastedValue*) casted->copy(scope.allocator);
            copied->value = evaluated_comptime(copied->value, scope);
            return copied;
        }
        case ValueKind::Expression: {
            const auto expr = value->as_expression_unsafe();
            const auto copied = (Expression*) expr->copy(scope.allocator);
            copied->firstValue = evaluated_comptime(copied->firstValue, scope);
            copied->secondValue = evaluated_comptime(copied->secondValue, scope);
            return copied;
        }
        default:
            const auto eval = value->evaluated_value(scope);
            return eval ? eval : value;
    }
}

class InterpretWrap : public FunctionDeclaration {
public:

    AnyType anyType;
    FunctionParam valueParam;

    explicit InterpretWrap(ASTNode* parent_node) : FunctionDeclaration(
            ZERO_LOC_ID("wrap"),
            &anyType,
            true,
            parent_node,
            ZERO_LOC,
            AccessSpecifier::Public,
            true
    ), anyType(ZERO_LOC),
        valueParam("value", &anyType, 0, nullptr, false, this, ZERO_LOC)
    {
        set_compiler_decl(true);
        // having a generic type parameter T requires that user gives type during function call to wrap
        // when we can successfully avoid giving type for generic parameters in functions, we should do this
//        generic_params.emplace_back(new (allocator.allocate<GenericTypeParameter>()) GenericTypeParameter("T", nullptr, this));
//        returnType = std::make_unique<ReferencedType>("T", generic_params[0].get());
        params.emplace_back(&valueParam);
    }
    Value *call(InterpretScope *call_scope, ASTAllocator& allocator, FunctionCall *call, Value *parent_val, bool evaluate_refs) final {
        auto underlying = call->values[0];
        const auto evaluated = evaluated_comptime(underlying, *call_scope);
        return new (allocator.allocate<WrapValue>()) WrapValue(evaluated);
    }
};

class InterpretUnwrap : public FunctionDeclaration {
public:

    AnyType anyType;
    FunctionParam valueParam;

    explicit InterpretUnwrap(ASTNode* parent_node) : FunctionDeclaration(
            ZERO_LOC_ID("unwrap"),
            &anyType,
            true,
            parent_node,
            ZERO_LOC,
            AccessSpecifier::Public,
            true
    ), anyType(ZERO_LOC), valueParam("value", &anyType, 0, nullptr, false, this, ZERO_LOC) {
        set_compiler_decl(true);
        // having a generic type parameter T requires that user gives type during function call to wrap
        // when we can successfully avoid giving type for generic parameters in functions, we should do this
//        generic_params.emplace_back(new (allocator.allocate<GenericTypeParameter>()) GenericTypeParameter("T", nullptr, this));
//        returnType = std::make_unique<ReferencedType>("T", generic_params[0].get());
        params.emplace_back(&valueParam);
    }
    Value *call(InterpretScope *call_scope, ASTAllocator& allocator, FunctionCall *call, Value *parent_val, bool evaluate_refs) final {
        return call->values[0]->evaluated_value(*call_scope)->copy(call_scope->allocator);
    }
};

class InterpretRetStructPtr : public FunctionDeclaration {
public:

    // TODO we shouldn't return pointer to void type
    VoidType voidType;
    PointerType ptrType;

    explicit InterpretRetStructPtr(ASTNode* parent_node) : FunctionDeclaration(
            ZERO_LOC_ID("return_struct"),
            &ptrType,
            true,
            parent_node,
            ZERO_LOC,
            AccessSpecifier::Public,
            true
    ), voidType(ZERO_LOC), ptrType(&voidType, ZERO_LOC) {
        set_compiler_decl(true);
    }
    Value *call(InterpretScope *call_scope, ASTAllocator& allocator, FunctionCall *call, Value *parent_val, bool evaluate_refs) final {
        return new (allocator.allocate<RetStructParamValue>()) RetStructParamValue(ZERO_LOC);
    }
};

class InterpretCompilerVersion : public FunctionDeclaration {
public:

    StringType stringType;

    explicit InterpretCompilerVersion(ASTNode* parent_node) : FunctionDeclaration(
            ZERO_LOC_ID("version"),
            &stringType,
            false,
            parent_node,
            ZERO_LOC,
            AccessSpecifier::Public,
            true
    ), stringType(ZERO_LOC) {
        set_compiler_decl(true);
    }
    Value *call(InterpretScope *call_scope, ASTAllocator& allocator, FunctionCall *call, Value *parent_val, bool evaluate_refs) final {
        return new (allocator.allocate<StringValue>()) StringValue(VERSION_STRING, ZERO_LOC);
    }
};

class InterpretIsTcc : public FunctionDeclaration {
public:

    BoolType boolType;

    explicit InterpretIsTcc(ASTNode* parent_node) : FunctionDeclaration(
            ZERO_LOC_ID("is_tcc_based"),
            &boolType,
            false,
            parent_node,
            ZERO_LOC,
            AccessSpecifier::Public,
            true
    ), boolType(ZERO_LOC) {
        set_compiler_decl(true);
    }
    Value *call(InterpretScope *call_scope, ASTAllocator& allocator, FunctionCall *call, Value *parent_val, bool evaluate_refs) final {
#ifdef TCC_BUILD
        return new (allocator.allocate<BoolValue>()) BoolValue(true, ZERO_LOC);
#else
        return new (allocator.allocate<BoolValue>()) BoolValue(false, ZERO_LOC);
#endif
    }
};

class InterpretIsClang : public FunctionDeclaration {
public:

    BoolType boolType;

    explicit InterpretIsClang(ASTNode* parent_node) : FunctionDeclaration(
            ZERO_LOC_ID("is_clang"),
            &boolType,
            false,
            parent_node,
            ZERO_LOC,
            AccessSpecifier::Public,
            true
    ), boolType(ZERO_LOC) {
        set_compiler_decl(true);
    }
    Value *call(InterpretScope *call_scope, ASTAllocator& allocator, FunctionCall *call, Value *parent_val, bool evaluate_refs) final {
#ifdef COMPILER_BUILD
        return new (allocator.allocate<BoolValue>()) BoolValue(true, ZERO_LOC);
#else
        return new (allocator.allocate<BoolValue>()) BoolValue(false, ZERO_LOC);
#endif
    }
};

class InterpretGetRawLocation : public FunctionDeclaration {
public:

    UBigIntType uIntType;

    explicit InterpretGetRawLocation() : FunctionDeclaration(
            ZERO_LOC_ID("get_raw_location"),
            &uIntType,
            false,
            nullptr,
            ZERO_LOC,
            AccessSpecifier::Public,
            true
    ), uIntType(ZERO_LOC) {
        set_compiler_decl(true);
    }
    Value *call(InterpretScope *call_scope, ASTAllocator& allocator, FunctionCall *call, Value *parent_val, bool evaluate_refs) final {
        return new (allocator.allocate<UBigIntValue>()) UBigIntValue(call->encoded_location().encoded, call->encoded_location());
    }

};

class InterpretGetRawLocOf : public FunctionDeclaration {
public:

    UBigIntType uIntType;

    explicit InterpretGetRawLocOf() : FunctionDeclaration(
            ZERO_LOC_ID("get_raw_loc_of"),
            &uIntType,
            false,
            nullptr,
            ZERO_LOC,
            AccessSpecifier::Public,
            true
    ), uIntType(ZERO_LOC) {
        set_compiler_decl(true);
    }
    Value *call(InterpretScope *call_scope, ASTAllocator& allocator, FunctionCall *call, Value *parent_val, bool evaluate_refs) final {
        if(call->values.empty()) {
            call_scope->error("get_raw_loc_of expects a single argument", call);
            return nullptr;
        }
        const auto first_arg = call->values[0];
        const auto first_arg_eval = first_arg ? first_arg->evaluated_value(*call_scope) : first_arg;
        return new (allocator.allocate<UBigIntValue>()) UBigIntValue(first_arg_eval->encoded_location().encoded, call->encoded_location());
    }

};

class InterpretGetLineNo : public FunctionDeclaration {
public:

    UBigIntType uIntType;

    explicit InterpretGetLineNo() : FunctionDeclaration(
            ZERO_LOC_ID("get_line_no"),
            &uIntType,
            false,
            nullptr,
            ZERO_LOC,
            AccessSpecifier::Public,
            true
    ), uIntType(ZERO_LOC) {
        set_compiler_decl(true);
    }
    Value *call(InterpretScope *call_scope, ASTAllocator& allocator, FunctionCall *call, Value *parent_val, bool evaluate_refs) final {
        const auto loc = call_scope->global->loc_man.getLocation(call->encoded_location());
        return new (allocator.allocate<UBigIntValue>()) UBigIntValue(loc.lineStart + 1, call->encoded_location());
    }

};

class InterpretGetCharacterNo : public FunctionDeclaration {
public:

    UBigIntType uIntType;

    explicit InterpretGetCharacterNo() : FunctionDeclaration(
            ZERO_LOC_ID("get_char_no"),
            &uIntType,
            false,
            nullptr,
            ZERO_LOC,
            AccessSpecifier::Public,
            true
    ), uIntType(ZERO_LOC) {
        set_compiler_decl(true);
    }
    Value *call(InterpretScope *call_scope, ASTAllocator& allocator, FunctionCall *call, Value *parent_val, bool evaluate_refs) final {
        const auto loc = call_scope->global->loc_man.getLocation(call->encoded_location());
        return new (allocator.allocate<UBigIntValue>()) UBigIntValue(loc.charStart + 1, call->encoded_location());
    }

};

FunctionCall* get_runtime_call(GlobalInterpretScope* global) {
    if(global->call_stack.empty()) {
        return nullptr;
    } else {
        // first comptime function call
        return global->call_stack.front();
    }
}

class InterpretGetCallerLineNo : public FunctionDeclaration {
public:

    UBigIntType uIntType;

    explicit InterpretGetCallerLineNo() : FunctionDeclaration(
            ZERO_LOC_ID("get_caller_line_no"),
            &uIntType,
            false,
            nullptr,
            ZERO_LOC,
            AccessSpecifier::Public,
            true
    ), uIntType(ZERO_LOC) {
        set_compiler_decl(true);
    }
    Value *call(InterpretScope *call_scope, ASTAllocator& allocator, FunctionCall *call, Value *parent_val, bool evaluate_refs) final {
        const auto global = call_scope->global;;
        const auto runtime_call = get_runtime_call(global);
        if(runtime_call) {
            const auto loc = global->loc_man.getLocation(runtime_call->encoded_location());
            return new (allocator.allocate<UBigIntValue>()) UBigIntValue(loc.lineStart + 1, ZERO_LOC);
        } else {
            return new (allocator.allocate<UBigIntValue>()) UBigIntValue(0, ZERO_LOC);
        }
    }

};

class InterpretGetCallerCharacterNo : public FunctionDeclaration {
public:

    UBigIntType uIntType;

    explicit InterpretGetCallerCharacterNo() : FunctionDeclaration(
            ZERO_LOC_ID("get_caller_char_no"),
            &uIntType,
            false,
            nullptr,
            ZERO_LOC,
            AccessSpecifier::Public,
            true
    ), uIntType(ZERO_LOC) {
        set_compiler_decl(true);
    }
    Value *call(InterpretScope *call_scope, ASTAllocator& allocator, FunctionCall *self_call, Value *parent_val, bool evaluate_refs) final {
        const auto global = call_scope->global;;
        const auto runtime_call = get_runtime_call(global);
        if(runtime_call) {
            const auto loc = global->loc_man.getLocation(runtime_call->encoded_location());
            return new (allocator.allocate<UBigIntValue>()) UBigIntValue(loc.charStart + 1, ZERO_LOC);
        } else {
            return new (allocator.allocate<UBigIntValue>()) UBigIntValue(0, ZERO_LOC);
        }
    }

};

class InterpretGetCallLoc : public FunctionDeclaration {
public:

    UBigIntType uIntType;

    explicit InterpretGetCallLoc() : FunctionDeclaration(
            ZERO_LOC_ID("get_call_loc"),
            &uIntType,
            false,
            nullptr,
            ZERO_LOC,
            AccessSpecifier::Public,
            true
    ), uIntType(ZERO_LOC) {
        set_compiler_decl(true);
    }
    Value *call(InterpretScope *call_scope, ASTAllocator& allocator, FunctionCall *call, Value *parent_val, bool evaluate_refs) final {
        if(call->values.empty()) {
            call_scope->error("get_call_loc expects a single argument", call);
            return nullptr;
        }
        const auto argVal = call->values.back();
        const auto arg = argVal->evaluated_value(*call_scope);
        if(!arg || arg->kind() < ValueKind::IntNStart || arg->kind() > ValueKind::IntNEnd) {
            call_scope->error("get_call_loc expects a integer argument", arg);
            return nullptr;
        }
        const auto num = arg->get_number();
        const auto global = call_scope->global;
        const auto call_number = num.value();
        const auto stack_size = global->call_stack.size();
        if(stack_size == 0) {
            return new (allocator.allocate<UBigIntValue>()) UBigIntValue(call->encoded_location().encoded, ZERO_LOC);
        }
        const auto final_call = call_number > stack_size ? global->call_stack.front() : global->call_stack[stack_size - 1 - call_number];
        return new (allocator.allocate<UBigIntValue>()) UBigIntValue(final_call->encoded_location().encoded, ZERO_LOC);
    }

};

class InterpretDefined : public FunctionDeclaration {
public:

    BoolType boolType;
    StringType stringType;
    FunctionParam valueParam;

    explicit InterpretDefined() : FunctionDeclaration(
            ZERO_LOC_ID("defined"),
            &boolType,
            false,
            nullptr,
            ZERO_LOC,
            AccessSpecifier::Public,
            true
    ), boolType(ZERO_LOC), stringType(ZERO_LOC), valueParam("value", &stringType, 0, nullptr, false, this, ZERO_LOC) {
        set_compiler_decl(true);
        params.emplace_back(&valueParam);
    }
    Value *call(InterpretScope *call_scope, ASTAllocator& allocator, FunctionCall *call, Value *parent_val, bool evaluate_refs) final {
        if(call->values.empty()) return new (allocator.allocate<BoolValue>()) BoolValue(false, ZERO_LOC);
        auto val = call->values[0]->evaluated_value(*call_scope);
        if(val->val_kind() != ValueKind::String) return new (allocator.allocate<BoolValue>()) BoolValue(false, ZERO_LOC);
        auto& definitions = call_scope->global->build_compiler->current_job->definitions;
        auto found = definitions.find(val->get_the_string().str());
        return new (allocator.allocate<BoolValue>()) BoolValue(found != definitions.end(), ZERO_LOC);
    }
};

// function is used to error out at compile time
class InterpretError : public FunctionDeclaration {
public:

    VoidType voidType;
    StringType stringType;
    FunctionParam valueParam;

    explicit InterpretError(ASTNode* parent_node) : FunctionDeclaration(
            ZERO_LOC_ID("error"),
            &voidType,
            false,
            parent_node,
            ZERO_LOC,
            AccessSpecifier::Public,
            true
    ), voidType(ZERO_LOC), stringType(ZERO_LOC), valueParam("value", &stringType, 0, nullptr, false, this, ZERO_LOC) {
        set_compiler_decl(true);
        params.emplace_back(&valueParam);
    }
    Value *call(InterpretScope *call_scope, ASTAllocator& allocator, FunctionCall *call, Value *parent_val, bool evaluate_refs) final {
        if(call->values.empty()) return new (allocator.allocate<BoolValue>()) BoolValue(false, ZERO_LOC);
        auto val = call->values[0]->evaluated_value(*call_scope);
        if(val->val_kind() != ValueKind::String) return new (allocator.allocate<BoolValue>()) BoolValue(false, ZERO_LOC);
        call_scope->error(val->get_the_string().view(), call);
    }
};

class InterpretSatisfies : public FunctionDeclaration {
public:

    BoolType returnType;
    AnyType anyType;
    FunctionParam valueParam;
    FunctionParam valueParam2;

    explicit InterpretSatisfies(ASTNode* parent_node) : FunctionDeclaration(
            ZERO_LOC_ID("satisfies"),
            &returnType,
            false,
            parent_node,
            ZERO_LOC,
            AccessSpecifier::Public,
            true
    ), returnType(ZERO_LOC), anyType(ZERO_LOC),
    valueParam("value", &anyType, 0, nullptr, false, this, ZERO_LOC),
    valueParam2("value2", &anyType, 1, nullptr, false, this, ZERO_LOC) {
        set_compiler_decl(true);
        params.emplace_back(&valueParam);
        params.emplace_back(&valueParam2);
    }
    inline Value* get_bool(ASTAllocator& allocator, bool value) {
        return new (allocator.allocate<BoolValue>()) BoolValue(value, ZERO_LOC);
    }
    Value *call(InterpretScope *call_scope, ASTAllocator& allocator, FunctionCall *call, Value *parent_val, bool evaluate_refs) final {
        if(call->values.size() != 2) {
            call_scope->error("wrong arguments size given to compiler::satisfies function", call);
            return nullptr;
        }
        const auto val_one = call->values[0];
        const auto val_two = call->values[1];
        const auto first_type = val_one->known_type();
        if(first_type) {
            return get_bool(allocator, first_type->satisfies(call_scope->allocator, val_two, false));
        } else {
            return get_bool(allocator, false);
        }
    }
};


class InterpretIsPtrNull : public FunctionDeclaration {
public:

    BoolType boolType;
    AnyType anyType;
    PointerType ptrType;
    FunctionParam valueParam;

    NullValue nullVal;

    explicit InterpretIsPtrNull(ASTNode* parent_node) : FunctionDeclaration(
            ZERO_LOC_ID("isNull"),
            &boolType,
            false,
            parent_node,
            ZERO_LOC,
            AccessSpecifier::Public,
            true
    ), boolType(ZERO_LOC), nullVal(ZERO_LOC), anyType(ZERO_LOC), ptrType(&anyType, ZERO_LOC),
        valueParam("value", &ptrType, 0, nullptr, false, this, ZERO_LOC)
    {
        set_compiler_decl(true);
        params.emplace_back(&valueParam);
    }
    Value *call(InterpretScope *call_scope, ASTAllocator& allocator, FunctionCall *call, Value *parent_val, bool evaluate_refs) final {
        return new (allocator.allocate<WrapValue>()) WrapValue(new (allocator.allocate<Expression>()) Expression(call->values[0], &nullVal, Operation::IsEqual, ZERO_LOC));
    }
};

class InterpretIsPtrNotNull : public FunctionDeclaration {
public:

    BoolType boolType;
    AnyType anyType;
    PointerType ptrType;
    FunctionParam valueParam;

    NullValue nullVal;

    explicit InterpretIsPtrNotNull(ASTNode* parent_node) : FunctionDeclaration(
            ZERO_LOC_ID("isNotNull"),
            &boolType,
            false,
            parent_node,
            ZERO_LOC,
            AccessSpecifier::Public,
            true
    ), boolType(ZERO_LOC), nullVal(ZERO_LOC), anyType(ZERO_LOC), ptrType(&anyType, ZERO_LOC),
        valueParam("value", &ptrType, 0, nullptr, false, this, ZERO_LOC)
    {
        set_compiler_decl(true);
        params.emplace_back(&valueParam);
    }
    Value *call(InterpretScope *call_scope, ASTAllocator& allocator, FunctionCall *call, Value *parent_val, bool evaluate_refs) final {
        return new (allocator.allocate<WrapValue>()) WrapValue(new (allocator.allocate<Expression>()) Expression(call->values[0], &nullVal, Operation::IsNotEqual, ZERO_LOC));
    }
};

class InterpretMemCopy : public FunctionDeclaration {
public:

    BoolType boolType;
    StringType stringType;
    FunctionParam destValueParam;
    FunctionParam sourceValueParam;

    explicit InterpretMemCopy(ASTNode* parent_node) : FunctionDeclaration(
            ZERO_LOC_ID("copy"),
            &boolType,
            false,
            parent_node,
            ZERO_LOC,
            AccessSpecifier::Public,
            true
    ), boolType(ZERO_LOC), stringType(ZERO_LOC), destValueParam("dest_value", &stringType, 0, nullptr, false, this, ZERO_LOC),
      sourceValueParam("source_value", &stringType, 1, nullptr, false, this, ZERO_LOC){
        set_compiler_decl(true);
        params.emplace_back(&destValueParam);
        params.emplace_back(&sourceValueParam);
    }
    Value *call(InterpretScope *call_scope, ASTAllocator& allocator, FunctionCall *call, Value *parent_val, bool evaluate_refs) final {
        auto& backend = *call_scope->global->backend_context;
        if(call->values.size() != 2) {
            call_scope->error("std::mem::copy called with arguments of length not equal to two", call);
            return nullptr;
        }
        backend.mem_copy(call->values[0], call->values[1]);
    }
};

class InterpretGetTarget : public FunctionDeclaration {
public:

    StringType stringType;

    explicit InterpretGetTarget(ASTNode* parent_node) : FunctionDeclaration(
            ZERO_LOC_ID("get_target"),
            &stringType,
            false,
            parent_node,
            ZERO_LOC,
            AccessSpecifier::Public,
            true
    ), stringType(ZERO_LOC) {
        set_compiler_decl(true);
    }
    Value *call(InterpretScope *call_scope, ASTAllocator& allocator, FunctionCall *call, Value *parent_val, bool evaluate_refs) final {
        auto& triple = call_scope->global->target_triple;
        const auto triple_ptr = allocator.allocate_str(triple.data(), triple.size());
        return new (allocator.allocate<StringValue>()) StringValue(chem::string_view(triple_ptr, triple.size()), call->encoded_location());
    }
};

class InterpretGetLocFilePath : public FunctionDeclaration {
public:

    StringType stringType;

    explicit InterpretGetLocFilePath(ASTNode* parent_node) : FunctionDeclaration(
            ZERO_LOC_ID("get_loc_file_path"),
            &stringType,
            false,
            parent_node,
            ZERO_LOC,
            AccessSpecifier::Public,
            true
    ), stringType(ZERO_LOC) {
        set_compiler_decl(true);
    }
    Value *call(InterpretScope *call_scope, ASTAllocator& allocator, FunctionCall *call, Value *parent_val, bool evaluate_refs) final {
        if(call->values.size() != 1) {
            call_scope->error("get_loc_file_path expects a single argument", call);
            return nullptr;
        }
        const auto eval_value = call->values.back()->evaluated_value(*call_scope);
        if(eval_value == nullptr || eval_value->kind() != ValueKind::UBigInt) {
            call_scope->error("couldn't evaluate value in get_loc_file_path", call);
            return nullptr;
        }
        auto& loc_man = call_scope->global->loc_man;
        auto location = loc_man.getLocation(eval_value->as_ubigint_unsafe()->value);
        auto fileId = loc_man.getPathForFileId(location.fileId);
        return new (allocator.allocate<StringValue>()) StringValue(chem::string_view(fileId.data(), fileId.size()), call->encoded_location());
    }
};

class InterpretGetCurrentFilePath : public FunctionDeclaration {
public:

    StringType stringType;

    explicit InterpretGetCurrentFilePath(ASTNode* parent_node) : FunctionDeclaration(
            ZERO_LOC_ID("get_current_file_path"),
            &stringType,
            false,
            parent_node,
            ZERO_LOC,
            AccessSpecifier::Public,
            true
    ), stringType(ZERO_LOC) {
        set_compiler_decl(true);
    }
    Value *call(InterpretScope *call_scope, ASTAllocator& allocator, FunctionCall *call, Value *parent_val, bool evaluate_refs) final {
        auto& loc_man = call_scope->global->loc_man;
        auto location = loc_man.getLocation(call->encoded_location());
        auto fileId = loc_man.getPathForFileId(location.fileId);
        return new (allocator.allocate<StringValue>()) StringValue(chem::string_view(fileId.data(), fileId.size()), call->encoded_location());
    }
};

// only direct child functions are supported at the moment
class InterpretGetChildFunction : public FunctionDeclaration {
public:

    AnyType anyType;
    FunctionParam param;
    StringType strType;
    FunctionParam methodParam;

    explicit InterpretGetChildFunction(ASTNode* parent_node) : FunctionDeclaration(
            ZERO_LOC_ID("get_child_fn"),
            &anyType,
            false,
            parent_node,
            ZERO_LOC,
            AccessSpecifier::Public,
            true
    ), param("value", &anyType, 0, nullptr, false, this, 0),
        methodParam("method", &strType, 1, nullptr, false, this, 0), anyType(0), strType(0) {
        set_compiler_decl(true);
        params = { &param, &methodParam };
    };

    Value *call(InterpretScope *call_scope, ASTAllocator& allocator, FunctionCall *call, Value *parent_val, bool evaluate_refs) final {
        if(call->values.size() != 2) {
            call_scope->error("error, get_child_fn expects two arguments", call);
            return new (allocator.allocate<NullValue>()) NullValue(&anyType, 0);
        }
        const auto nameVal = call->values[1]->evaluated_value(*call_scope);
        if(nameVal->val_kind() != ValueKind::String) {
            call_scope->error("expected second argument to get_child_fn to be a string", call);
            return new(allocator.allocate<NullValue>()) NullValue(&anyType, 0);
        }
        const auto type = call->values.front()->create_type(allocator);
        const auto linked = type->linked_node();
        if(linked && ASTNode::isMembersContainer(linked->kind())) {
            const auto container = linked->as_members_container_unsafe();
            const auto value = container->direct_child_function(nameVal->get_the_string());
            if(value) {
                const auto id = new(allocator.allocate<VariableIdentifier>()) VariableIdentifier(value->name_view(), 0, true);
                id->linked = value;
                return new(allocator.allocate<AccessChain>()) AccessChain({id}, false, 0);
            }
        }
        return new (allocator.allocate<NullValue>()) NullValue(&anyType, 0);
    }

};

class InterpretTypeToString : public FunctionDeclaration {
public:

    std::ostringstream ostring;
    RepresentationVisitor visitor;
    StringType returnType;

    explicit InterpretTypeToString(ASTNode* parent_node) : FunctionDeclaration(
            ZERO_LOC_ID("type_to_string"),
            &returnType,
            true,
            parent_node,
            ZERO_LOC,
            AccessSpecifier::Public,
            true
    ), visitor(ostring),  returnType(ZERO_LOC) {
        visitor.interpret_representation = true;
    }

    Value *call(InterpretScope *call_scope, ASTAllocator& allocator, FunctionCall *call, Value *parent_val, bool evaluate_refs) override {
        if(call->generic_list.empty()) {
            call_scope->error("type_to_string expects a single type parameter", call);
            return nullptr;
        }
        const auto childType = call->generic_list.front()->pure_type(allocator);
        visitor.visit(childType);
        const auto& str = ostring.str();
        const auto returnValue = new (allocator.allocate<StringValue>()) StringValue(chem::string_view(allocator.allocate_str(str.data(), str.size()), str.size()), call->encoded_location());
        ostring.str("");
        ostring.clear();
        return returnValue;
    }
};

//class InterpretGetDeleteFnPtr : public FunctionDeclaration {
//public:
//
//    StringType stringType;
//
//    explicit InterpretGetCurrentFilePath(ASTNode* parent_node) : FunctionDeclaration(
//            ZERO_LOC_ID("get_current_file_path"),
//            &stringType,
//            false,
//            parent_node,
//            ZERO_LOC,
//            AccessSpecifier::Public,
//            true
//    ), stringType(ZERO_LOC) {
//        set_compiler_decl(true);
//    }
//    Value *call(InterpretScope *call_scope, ASTAllocator& allocator, FunctionCall *call, Value *parent_val, bool evaluate_refs) final {
//        auto& loc_man = call_scope->global->loc_man;
//        auto location = loc_man.getLocation(call->location);
//        auto fileId = loc_man.getPathForFileId(location.fileId);
//        return new (allocator.allocate<StringValue>()) StringValue(std::string(fileId), call->location);
//    }
//};


//class InterpretConstruct : public FunctionDeclaration {
//public:
//    explicit InterpretConstruct(ASTNode* parent_node) : FunctionDeclaration(
//            "construct",
//            std::make_unique<VoidType>(),
//            false,
//            parent_node,
//            AccessSpecifier::Public
//    ) {
//        set_compiler_decl(true);
//        params.emplace_back(std::make_unique<FunctionParam>("ptr", std::make_unique<PointerType>(std::make_unique<VoidType>()), 0, std::nullopt, this));
//        params.emplace_back(std::make_unique<FunctionParam>("value", std::make_unique<AnyType>(), 1, std::nullopt, this));
//    }
//    Value *call(InterpretScope *call_scope, FunctionCall *call, Value *parent_val, bool evaluate_refs) final {
//
//    }
//};

class CompilerNamespace : public Namespace {
public:

    InterpretPrint printFn;
    InterpretPrintLn printlnFn;
    InterpretToString to_stringFn;
    InterpretTypeToString type_to_stringFn;
    InterpretWrap wrapFn;
    InterpretUnwrap unwrapFn;
    InterpretRetStructPtr retStructPtr;
    InterpretCompilerVersion verFn;
    InterpretIsTcc isTccFn;
    InterpretIsClang isClangFn;
    InterpretSize sizeFn;
    InterpretVector::InterpretVectorNode vectorNode;
    InterpretSatisfies satisfiesFn;

    InterpretGetRawLocation get_raw_location;
    InterpretGetRawLocOf get_raw_loc_of;
    InterpretGetCallLoc get_call_loc;
    InterpretGetLineNo get_line_no;
    InterpretGetCharacterNo get_char_no;
    InterpretGetCallerLineNo get_caller_line_no;
    InterpretGetCallerCharacterNo get_caller_char_no;
    InterpretGetTarget get_target_fn;
    InterpretGetCurrentFilePath get_current_file_path;
    InterpretGetLocFilePath get_loc_file_path;
    InterpretGetChildFunction get_child_fn;
    InterpretError error_fn;

    CompilerNamespace(

    ) : Namespace(ZERO_LOC_ID("compiler"), nullptr, ZERO_LOC, AccessSpecifier::Public),
        printFn(this), printlnFn(this), to_stringFn(this), type_to_stringFn(this), wrapFn(this), unwrapFn(this), retStructPtr(this), verFn(this),
        isTccFn(this), isClangFn(this), sizeFn(this), vectorNode(this), satisfiesFn(this),
        get_target_fn(this), get_current_file_path(this), get_loc_file_path(this), get_child_fn(this), error_fn(this)
    {
        set_compiler_decl(true);
        nodes = {
            &printFn, &printlnFn, &to_stringFn, &type_to_stringFn, &wrapFn, &unwrapFn, &retStructPtr, &verFn, &isTccFn, &isClangFn, &sizeFn, &vectorNode,
            &satisfiesFn, &get_raw_location, &get_raw_loc_of, &get_call_loc, &get_line_no, &get_char_no, &get_caller_line_no, &get_caller_char_no,
            &get_target_fn, &get_current_file_path, &get_loc_file_path, &get_child_fn, &error_fn
        };
    }

};

class MemNamespace : public Namespace {
public:

    InterpretMemCopy memCopyFn;

    explicit MemNamespace(
        ASTNode* parent_node
    ) : Namespace(ZERO_LOC_ID("mem"), parent_node, ZERO_LOC, AccessSpecifier::Public), memCopyFn(this) {
        set_compiler_decl(true);
        nodes = { &memCopyFn };
    }

};

class PtrNamespace : public Namespace {
public:

    InterpretIsPtrNull isNullFn;
    InterpretIsPtrNotNull isNotNullFn;

    explicit PtrNamespace(
            ASTNode* parent_node
    ) : Namespace(ZERO_LOC_ID("ptr"), parent_node, ZERO_LOC, AccessSpecifier::Public),
        isNullFn(this), isNotNullFn(this)
    {
        set_compiler_decl(true);
        nodes = { &isNullFn, &isNotNullFn };
    }

};


class StdNamespace : public Namespace {
public:

    MemNamespace memNamespace;
    PtrNamespace ptrNamespace;

    StdNamespace(

    ) : Namespace(ZERO_LOC_ID("std"), nullptr, ZERO_LOC, AccessSpecifier::Public),
        memNamespace(this), ptrNamespace(this)
    {
        set_compiler_decl(true);
        nodes = { &memNamespace, &ptrNamespace };
    }

};

class DefDecl : public StructDefinition {
public:

    DefDecl() : StructDefinition(
            ZERO_LOC_ID("Def"), nullptr, ZERO_LOC, AccessSpecifier::Public
    ) {
        set_compiler_decl(true);
    }

};

class DefValue : public StructValue {
public:

    LinkedType id;

    DefValue(DefDecl *defDecl) : StructValue(
            &id,
            defDecl,
            defDecl,
            ZERO_LOC
    ), id(defDecl, ZERO_LOC) {}

};

struct DefThing {

    DefDecl decl;
    DefValue defValue;
    VarInitStatement defStmt;

    DefThing() : defValue(&decl), defStmt(true, false, ZERO_LOC_ID("def"), defValue.refType, &defValue, nullptr, ZERO_LOC, AccessSpecifier::Public) {
        defStmt.set_compiler_decl(true);
    }

    void declare_value(ASTAllocator& allocator, const chem::string_view& name, BaseType* type, Value* value) {
        const auto member = new (allocator.allocate<StructMember>()) StructMember(name, type, nullptr, &decl, ZERO_LOC, true);
        decl.insert_variable_no_check(member);
        defValue.values.emplace(name, StructMemberInitializer{ name, value });
    }

    void clear_values() {
        decl.clear_variables_and_indexes();
        defValue.values.clear();
    }

};

struct GlobalContainer {
    CompilerNamespace compiler_namespace;
    StdNamespace std_namespace;
    InterpretDefined defined;
    DefThing defThing;
};

#ifdef COMPILER_BUILD

void init_target_data(llvm::Triple& triple, TargetData& data) {

    const auto arhType = triple.getArch();

    // Check for Windows
    if (triple.isOSWindows()) {
        data.is_windows = true;
        if (arhType == llvm::Triple::x86) {
            data.is_win32 = true;
        } else if (arhType == llvm::Triple::x86_64) {
            data.is_win64 = true;
        }
    }

    // Check for Linux
    if (triple.isOSLinux()) {
        data.is_linux = true;
        data.is_unix = true;
    }

    // Check for macOS
    if (triple.isMacOSX()) {
        data.is_macos = true;
        data.is_unix = true;
    }

    // Check for FreeBSD
    if (triple.isOSFreeBSD()) {
        data.is_freebsd = true;
        data.is_unix = true;
    }

    // Check for Android
    if (triple.isAndroid()) {
        data.is_android = true;
        data.is_unix = true;
    }

    // Check for Cygwin
    if (triple.isOSCygMing()) {
        data.is_cygwin = true;
        data.is_unix = true;
    }

    if(triple.isArch64Bit()) {
        data.is_64Bit = true;
    }

    // Check for architecture
    switch(arhType) {
        case llvm::Triple::x86_64:
            data.is_x86_64 = true;
            break;
        case llvm::Triple::x86:
            data.is_i386 = true;
            break;
        case llvm::Triple::arm:
            data.is_arm = true;
            break;
        case llvm::Triple::aarch64:
            data.is_aarch64 = true;
            break;
        default:
            break;
    }

    data.is_little_endian = triple.isLittleEndian();

}

void GlobalInterpretScope::prepare_target_data(TargetData& data) {
    auto triple = llvm::Triple(target_triple);
    init_target_data(triple, data);
}

#else

// this puts the target data of the current executable
void prepare_executable_target_data(TargetData& data) {
#ifdef _WIN32
    data.is_windows = true;
    data.is_win32 = true;
#endif

#ifdef _WIN64
    data.is_windows = true;
    data.is_win64 = true;
#endif

    data.is_64Bit = sizeof(void*) == 8;

#ifdef __linux__
    data.is_linux = true;
    data.is_unix = true;
#endif

#ifdef __APPLE__
    data.is_macos = true;
    data.is_unix = true;
#endif

#ifdef __FreeBSD__
    data.is_freebsd = true;
    data.is_unix = true;
#endif

#ifdef __ANDROID__
    data.is_android = true;
    data.is_unix = true;
#endif

#ifdef __CYGWIN__
    data.is_cygwin = true;
    data.is_unix = true;
#endif

#ifdef __MINGW32__
    data.is_mingw32 = true;
    data.is_win32 = true;
    data.is_windows = true;
#endif

#ifdef __MINGW64__
    data.is_mingw64 = true;
    data.is_win64 = true;
    data.is_windows = true;
#endif

// Detect architecture using predefined macros
#ifdef __x86_64__
    data.is_x86_64 = true;
#endif

#ifdef __i386__
    data.is_i386 = true;
#endif

#ifdef __arm__
    data.is_arm = true;
#endif

#ifdef __aarch64__
    data.is_aarch64 = true;
#endif

    data.is_little_endian = IS_LITTLE_ENDIAN;

}

void GlobalInterpretScope::prepare_target_data(TargetData& data) {

    // Split the target string by '-'
    std::vector<std::string> parts;
    std::istringstream stream(target_triple);
    std::string part;
    while (std::getline(stream, part, '-')) {
        parts.push_back(part);
    }

    if (parts.size() < 3) {
        prepare_executable_target_data(data);
        return;  // Invalid target string format
    }

    std::string arch = parts[0];
    std::string sys = parts[2];
    std::string abi = (parts.size() > 3) ? parts[3] : ""; // Optional ABI part

    // Determine architecture
    if (arch == "x86_64") {
        data.is_x86_64 = true;
    } else if (arch == "i386") {
        data.is_i386 = true;
    } else if (arch == "arm") {
        data.is_arm = true;
    } else if (arch == "aarch64") {
        data.is_aarch64 = true;
    }

    // Determine operating system
    if (sys == "linux") {
        data.is_linux = true;
        data.is_unix = true;
    } else if (sys == "windows") {
        data.is_win32 = true;
        data.is_windows = true;
    } else if (sys == "darwin") {
        data.is_macos = true;
        data.is_unix = true;
    } else if (sys == "freebsd") {
        data.is_freebsd = true;
        data.is_unix = true;
    } else if (sys == "android") {
        data.is_android = true;
        data.is_unix = true;
    } else if (sys == "cygwin") {
        data.is_cygwin = true;
        data.is_unix = true;
    } else if (sys == "mingw32") {
        data.is_mingw32 = true;
        data.is_win32 = true;
        data.is_windows = true;
    } else if (sys == "mingw64") {
        data.is_win64 = true;
        data.is_windows = true;
    }

    if (arch == "x86_64" || arch == "i386" || arch == "aarch64") {
        data.is_little_endian = true;
    } else if (arch == "arm") {
        if (target_triple.find("arm-be") != std::string::npos || abi == "be") {
            data.is_little_endian = false;
        } else if (target_triple.find("arm-le") != std::string::npos || abi == "le") {
            data.is_little_endian = true;
        } else {
            data.is_little_endian = IS_LITTLE_ENDIAN;
        }
    } else if (arch.find("be") != std::string::npos) {
        data.is_little_endian = false;
    } else {
        data.is_little_endian = IS_LITTLE_ENDIAN;
    }

}

#endif

BoolValue* boolValue(ASTAllocator& allocator, bool value) {
    return new (allocator.allocate<BoolValue>()) BoolValue(value, ZERO_LOC);
}

void create_target_data_in_def(GlobalInterpretScope& scope, DefThing& defThing) {
    auto& targetData = scope.target_data;
    scope.prepare_target_data(targetData);
    // declaring native definitions like windows and stuff
    auto& allocator = scope.allocator;
    // we change the global interpret scope for each job, so we must redeclare def value
    scope.values["def"] = &defThing.defValue;
    const auto boolType = new (allocator.allocate<BoolType>()) BoolType(ZERO_LOC);
    const auto mode = scope.build_compiler->options->outMode;
    defThing.declare_value(allocator, "debug", boolType, boolValue(allocator, is_debug(mode)));
    defThing.declare_value(allocator, "debug_quick", boolType, boolValue(allocator, mode == OutputMode::DebugQuick));
    defThing.declare_value(allocator, "debug_complete", boolType, boolValue(allocator, mode == OutputMode::DebugComplete));
    defThing.declare_value(allocator, "release", boolType, boolValue(allocator, is_release(mode)));
    defThing.declare_value(allocator, "release_safe", boolType, boolValue(allocator, mode == OutputMode::ReleaseSafe));
    defThing.declare_value(allocator, "release_small", boolType, boolValue(allocator, mode == OutputMode::ReleaseSmall));
    defThing.declare_value(allocator, "release_fast", boolType, boolValue(allocator, mode == OutputMode::ReleaseFast));
    defThing.declare_value(allocator, "is_little_endian", boolType, boolValue(allocator, targetData.is_little_endian));
    defThing.declare_value(allocator, "is_big_endian", boolType, boolValue(allocator, !targetData.is_little_endian));
    defThing.declare_value(allocator, "is64Bit", boolType, boolValue(allocator, targetData.is_64Bit));
    defThing.declare_value(allocator, "windows", boolType, boolValue(allocator, targetData.is_windows));
    defThing.declare_value(allocator, "win32", boolType, boolValue(allocator, targetData.is_win32));
    defThing.declare_value(allocator, "win64", boolType, boolValue(allocator, targetData.is_win64));
    defThing.declare_value(allocator, "linux", boolType, boolValue(allocator, targetData.is_linux));
    defThing.declare_value(allocator, "macos", boolType, boolValue(allocator, targetData.is_macos));
    defThing.declare_value(allocator, "freebsd", boolType, boolValue(allocator, targetData.is_freebsd));
    defThing.declare_value(allocator, "unix", boolType, boolValue(allocator, targetData.is_unix));
    defThing.declare_value(allocator, "android", boolType, boolValue(allocator, targetData.is_android));
    defThing.declare_value(allocator, "cygwin", boolType, boolValue(allocator, targetData.is_cygwin));
    defThing.declare_value(allocator, "mingw32", boolType, boolValue(allocator, targetData.is_mingw32));
    defThing.declare_value(allocator, "x86_64", boolType, boolValue(allocator, targetData.is_x86_64));
    defThing.declare_value(allocator, "i386", boolType, boolValue(allocator, targetData.is_i386));
    defThing.declare_value(allocator, "arm", boolType, boolValue(allocator, targetData.is_arm));
    defThing.declare_value(allocator, "aarch64", boolType, boolValue(allocator, targetData.is_aarch64));
}

void GlobalInterpretScope::rebind_container(SymbolResolver& resolver, GlobalContainer* container_ptr) {
    auto& container = *container_ptr;

    container.compiler_namespace.extended.clear();
    container.std_namespace.extended.clear();

    container.compiler_namespace.declare_top_level(resolver, (ASTNode*&) container.compiler_namespace);
    container.std_namespace.declare_top_level(resolver, (ASTNode*&) container.std_namespace);

    // TODO these symbols will be removed when module ends
    // TODO use exported declare or rename function to convey meaning better
    resolver.declare(container.defined.name_view(), &container.defined);
    resolver.declare(container.defThing.decl.name_view(), &container.defThing.decl);
    resolver.declare(container.defThing.defStmt.id_view(), &container.defThing.defStmt);

    container.defThing.clear_values();
    // we recreate the target data, because the allocator disposes at the end of each job
    // and this method is called after disposal of the allocator when the job ends, and a new job starts
    create_target_data_in_def(*this, container.defThing);

}

GlobalContainer* GlobalInterpretScope::create_container(SymbolResolver& resolver) {

    const auto container_ptr = new GlobalContainer;
    auto& container = *container_ptr;

    container.compiler_namespace.declare_top_level(resolver, (ASTNode*&) container.compiler_namespace);
    container.std_namespace.declare_top_level(resolver, (ASTNode*&) container.std_namespace);
    container.defined.declare_top_level(resolver, (ASTNode*&) container.defined);

    // definitions using defThing
    container.defThing.decl.declare_top_level(resolver, (ASTNode*&) container.defThing.decl);
    container.defThing.defStmt.declare_top_level(resolver, (ASTNode*&) container.defThing.defStmt);

    create_target_data_in_def(*this, container.defThing);

    return container_ptr;
}

void GlobalInterpretScope::dispose_container(GlobalContainer* container) {
    delete container;
}

//void GlobalInterpretScope::rebind_compiler_namespace(SymbolResolver &resolver) {
//    auto& compiler_ns = global_nodes["compiler"];
//    compiler_ns->declare_top_level(resolver, compiler_ns);
//}