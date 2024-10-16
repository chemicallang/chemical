// Copyright (c) Qinetik 2024.

#include "ast/base/GlobalInterpretScope.h"
#include "compiler/lab/LabBuildCompiler.h"
#include "compiler/lab/LabBuildContext.h"
#include "ast/structures/FunctionParam.h"
#include "sstream"
#include <utility>
#include "ast/types/VoidType.h"
#include "ast/types/StringType.h"
#include "ast/types/BoolType.h"
#include "ast/values/IntValue.h"
#include "ast/values/Expression.h"
#include "ast/values/BoolValue.h"
#include "ast/values/UBigIntValue.h"
#include "ast/values/RetStructParamValue.h"
#include "ast/values/StructValue.h"
#include "ast/values/StringValue.h"
#include "ast/values/ArrayValue.h"
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

namespace InterpretVector {

    class InterpretVectorNode;

    class InterpretVectorConstructor : public FunctionDeclaration {
    public:

        explicit InterpretVectorConstructor(InterpretVectorNode* node);

        Value* call(InterpretScope *call_scope, FunctionCall *call, Value *parent_val, bool evaluate_refs);

    };

    class InterpretVectorSize : public FunctionDeclaration {
    public:

        FunctionParam selfParam;
        IntType retType;

        explicit InterpretVectorSize(InterpretVectorNode* node);

        Value *call(InterpretScope *call_scope, FunctionCall *call, Value *parent_val, bool evaluate_refs) override;

    };

    class InterpretVectorGet : public FunctionDeclaration {
    public:

        FunctionParam selfParam;
        UIntType indexType;
        FunctionParam indexParam;
        LinkedType returnLinkedType;

        explicit InterpretVectorGet(InterpretVectorNode* node);

        Value *call(InterpretScope *call_scope, FunctionCall *call, Value *parent_val, bool evaluate_refs) override;

    };

    class InterpretVectorPush : public FunctionDeclaration {
    public:

        FunctionParam selfParam;
        LinkedType valueType;
        FunctionParam valueParam;
        VoidType returnVoidType;

        explicit InterpretVectorPush(InterpretVectorNode* node);

        Value *call(InterpretScope *call_scope, FunctionCall *call, Value *parent_val, bool evaluate_refs) override;

    };

    class InterpretVectorRemove : public FunctionDeclaration {
    public:

        FunctionParam selfParam;
        VoidType returnVoidType;
        UIntType indexType;
        FunctionParam indexParam;

        explicit InterpretVectorRemove(InterpretVectorNode* node);

        Value *call(InterpretScope *call_scope, FunctionCall *call, Value *parent_val, bool evaluate_refs) override;

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
            {},
            (StructDefinition*) node,
            nullptr,
            nullptr
        ) {

        }
    };

    InterpretVectorConstructor::InterpretVectorConstructor(InterpretVectorNode* node) : FunctionDeclaration(
            "constructor",
            std::vector<FunctionParam*> {},
            &node->selfType,
            false,
            node,
            nullptr,
            std::nullopt,
            AccessSpecifier::Public
    ) {
        add_annotation(AnnotationKind::Constructor);
    }

    Value *InterpretVectorConstructor::call(InterpretScope *call_scope, FunctionCall *call, Value *parent_val, bool evaluate_refs) {
        return new (call_scope->allocate<InterpretVectorVal>()) InterpretVectorVal((InterpretVectorNode*) parent_node);
    }

    InterpretVectorSize::InterpretVectorSize(InterpretVectorNode* node) : FunctionDeclaration(
        "size",
        std::vector<FunctionParam*> {},
        &retType,
        false,
        node,
        nullptr,
        std::nullopt,
        AccessSpecifier::Public
    ), retType(nullptr), selfParam("self", &node->selfReference, 0, nullptr, true, this, nullptr) {
        params.emplace_back(&selfParam);
    }

    Value *InterpretVectorSize::call(InterpretScope *call_scope, FunctionCall *call, Value *parent_val, bool evaluate_refs) {
        return new (call_scope->allocate<IntValue>()) IntValue(static_cast<InterpretVectorVal*>(parent_val)->values.size(), nullptr);
    }


    // TODO interpret vector get should return a reference to T
    InterpretVectorGet::InterpretVectorGet(InterpretVectorNode* node) : FunctionDeclaration(
            "get",
            std::vector<FunctionParam*> {},
            &returnLinkedType,
            false,
            node,
            nullptr,
            std::nullopt,
            AccessSpecifier::Public
    ), returnLinkedType("T", &node->typeParam, nullptr),
        selfParam("self", &node->selfReference, 0, nullptr, true, this, nullptr), indexType(nullptr), indexParam("index", &indexType, 1, nullptr, false, this, nullptr)
    {
        params.emplace_back(&selfParam);
        params.emplace_back(&indexParam);
    }

    Value *InterpretVectorGet::call(InterpretScope *call_scope, FunctionCall *call, Value *parent_val, bool evaluate_refs) {
        return static_cast<InterpretVectorVal*>(parent_val)->values[call->values[0]->evaluated_value(*call_scope)->get_the_int()]->scope_value(*call_scope);
    }

    InterpretVectorPush::InterpretVectorPush(InterpretVectorNode* node) : FunctionDeclaration(
            "push",
            std::vector<FunctionParam*> {},
            &returnVoidType,
            false,
            node,
            nullptr,
            std::nullopt,
            AccessSpecifier::Public
    ), selfParam("self", &node->selfReference, 0, nullptr, true, this, nullptr), returnVoidType(nullptr),
        valueType("T", &node->typeParam, nullptr), valueParam("value", &valueType, 1, nullptr, false, this, nullptr)
    {
        params.emplace_back(&selfParam);
        params.emplace_back(&valueParam);
    }

    Value *InterpretVectorPush::call(InterpretScope *call_scope, FunctionCall *call, Value *parent_val, bool evaluate_refs) {
        static_cast<InterpretVectorVal*>(parent_val)->values.emplace_back(call->values[0]->scope_value(*call_scope));
        return nullptr;
    }

    InterpretVectorRemove::InterpretVectorRemove(InterpretVectorNode* node) : FunctionDeclaration(
            "remove",
            std::vector<FunctionParam*> {},
            &returnVoidType,
            false,
            node,
            nullptr,
            std::nullopt,
            AccessSpecifier::Public
    ), selfParam("self", &node->selfReference, 0, nullptr, true, this, nullptr), returnVoidType(nullptr),
        indexType(nullptr), indexParam("index", &indexType, 1, nullptr, false, this, nullptr)
    {
        params.emplace_back(&selfParam);
        params.emplace_back(&indexParam);
    }
    Value *InterpretVectorRemove::call(InterpretScope *call_scope, FunctionCall *call, Value *parent_val, bool evaluate_refs) {
        auto& ref = static_cast<InterpretVectorVal*>(parent_val)->values;
        ref.erase(ref.begin() + call->values[0]->evaluated_value(*call_scope)->get_the_int());
        return nullptr;
    }

    InterpretVectorNode::InterpretVectorNode(
        ASTNode* parent_node
    ): StructDefinition("vector", parent_node, nullptr, AccessSpecifier::Public),
        constructorFn(this), sizeFn(this), getFn(this), pushFn(this), removeFn(this),
        typeParam("T", nullptr, nullptr, this, 0, nullptr),
        selfType("vector", this, nullptr), selfReference(&selfType, nullptr)
    {
        add_annotation(AnnotationKind::CompTime);
        generic_params.emplace_back(&typeParam);
        insert_func(&constructorFn);
        insert_func(&sizeFn);
        insert_func(&getFn);
        insert_func(&pushFn);
        insert_func(&removeFn);
    }

}

class InterpretPrint : public FunctionDeclaration {
public:

    std::ostringstream ostring;
    RepresentationVisitor visitor;
    VoidType returnType;


    explicit InterpretPrint(ASTNode* parent_node) : FunctionDeclaration(
            "print",
            std::vector<FunctionParam*> {},
            &returnType,
            true,
            parent_node,
            nullptr,
            std::nullopt,
            AccessSpecifier::Public
    ), visitor(ostring),  returnType(nullptr) {
        visitor.interpret_representation = true;
    }
    Value *call(InterpretScope *call_scope, FunctionCall *call, Value *parent_val, bool evaluate_refs) override {
        ostring.str("");
        ostring.clear();
        for (auto const &value: call->values) {
            auto paramValue = value->evaluated_value(*call_scope);
            if(paramValue == nullptr) {
                ostring.write("null", 4);
            } else {
                paramValue->accept(&visitor);
            }
        }
        std::cout << ostring.str();
        return nullptr;
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
            "size",
            std::vector<FunctionParam*> {},
            &returnType,
            false,
            parent_node,
            nullptr,
            std::nullopt,
            AccessSpecifier::Public
    ), returnType(nullptr), anyType(nullptr), valueParam("value", &anyType, 0, nullptr, false, this, nullptr) {
        add_annotation(AnnotationKind::CompTime);
        params.emplace_back(&valueParam);
    }
    Value *call(InterpretScope *call_scope, FunctionCall *call, Value *parent_val, bool evaluate_refs) override {
        if(call->values.empty()) {
            call_scope->error("compiler::size called without arguments", call);
            return nullptr;
        }
        const auto val = call->values[0];
        const auto val_type = val->value_type();
        if(val_type != ValueType::String && val_type != ValueType::Array) {
            call_scope->error("compiler::size called with invalid arguments", call);
            return nullptr;
        }
        auto value = resolve_ref(val, call_scope);
        if(!value) {
            call_scope->error("couldn't get value for compiler::size", call);
            return nullptr;
        }
        switch(val_type) {
            case ValueType::String:
                return new (call_scope->allocate<UBigIntValue>()) UBigIntValue(value->get_the_string().length(), nullptr);
            case ValueType::Array:
                return new (call_scope->allocate<UBigIntValue>()) UBigIntValue(value->as_array_value()->array_size(), nullptr);
            default:
                return new (call_scope->allocate<UBigIntValue>()) UBigIntValue(0, nullptr);
        }
    }
};

class WrapValue : public Value {
public:
    Value* underlying;
    explicit WrapValue(Value* underlying) : underlying(underlying) {

    }
    CSTToken *cst_token() override {
        return nullptr;
    }
    void accept(Visitor *visitor) override {
        throw std::runtime_error("compiler::wrap value cannot be visited");
    }
    ValueKind val_kind() override {
        return ValueKind::WrapValue;
    }
    Value *copy(ASTAllocator& allocator) override {
        return new (allocator.allocate<WrapValue>()) WrapValue(underlying->copy(allocator));
    }
    Value* evaluated_value(InterpretScope &scope) override {
        return underlying;
    }
};

class InterpretWrap : public FunctionDeclaration {
public:

    AnyType anyType;
    FunctionParam valueParam;

    explicit InterpretWrap(ASTNode* parent_node) : FunctionDeclaration(
            "wrap",
            std::vector<FunctionParam*> {},
            &anyType,
            true,
            parent_node,
            nullptr,
            std::nullopt,
            AccessSpecifier::Public
    ), anyType(nullptr), valueParam("value", &anyType, 0, nullptr, false, this, nullptr) {
        add_annotation(AnnotationKind::CompTime);
        // having a generic type parameter T requires that user gives type during function call to wrap
        // when we can successfully avoid giving type for generic parameters in functions, we should do this
//        generic_params.emplace_back(new (call_scope->allocate<GenericTypeParameter>()) GenericTypeParameter("T", nullptr, this));
//        returnType = std::make_unique<ReferencedType>("T", generic_params[0].get());
        params.emplace_back(&valueParam);
    }
    Value *call(InterpretScope *call_scope, FunctionCall *call, Value *parent_val, bool evaluate_refs) override {
        auto underlying = call->values[0]->copy(call_scope->allocator);
        underlying->evaluate_children(*call_scope);
        return new (call_scope->allocate<WrapValue>()) WrapValue(underlying);
    }
};

class InterpretUnwrap : public FunctionDeclaration {
public:

    AnyType anyType;
    FunctionParam valueParam;

    explicit InterpretUnwrap(ASTNode* parent_node) : FunctionDeclaration(
            "unwrap",
            std::vector<FunctionParam*> {},
            &anyType,
            true,
            parent_node,
            nullptr,
            std::nullopt,
            AccessSpecifier::Public
    ), anyType(nullptr), valueParam("value", &anyType, 0, nullptr, false, this, nullptr) {
        add_annotation(AnnotationKind::CompTime);
        // having a generic type parameter T requires that user gives type during function call to wrap
        // when we can successfully avoid giving type for generic parameters in functions, we should do this
//        generic_params.emplace_back(new (call_scope->allocate<GenericTypeParameter>()) GenericTypeParameter("T", nullptr, this));
//        returnType = std::make_unique<ReferencedType>("T", generic_params[0].get());
        params.emplace_back(&valueParam);
    }
    Value *call(InterpretScope *call_scope, FunctionCall *call, Value *parent_val, bool evaluate_refs) override {
        return call->values[0]->evaluated_value(*call_scope)->copy(call_scope->allocator);
    }
};

class InterpretRetStructPtr : public FunctionDeclaration {
public:

    // TODO we shouldn't return pointer to void type
    VoidType voidType;
    PointerType ptrType;

    explicit InterpretRetStructPtr(ASTNode* parent_node) : FunctionDeclaration(
            "return_struct",
            std::vector<FunctionParam*> {},
            &ptrType,
            true,
            parent_node,
            nullptr,
            std::nullopt,
            AccessSpecifier::Public
    ), voidType(nullptr), ptrType(&voidType, nullptr) {
        add_annotation(AnnotationKind::CompTime);
    }
    Value *call(InterpretScope *call_scope, FunctionCall *call, Value *parent_val, bool evaluate_refs) override {
        return new (call_scope->allocate<RetStructParamValue>()) RetStructParamValue(nullptr);
    }
};

class InterpretCompilerVersion : public FunctionDeclaration {
public:

    StringType stringType;

    explicit InterpretCompilerVersion(ASTNode* parent_node) : FunctionDeclaration(
            "version",
            std::vector<FunctionParam*> {},
            &stringType,
            false,
            parent_node,
            nullptr,
            std::nullopt,
            AccessSpecifier::Public
    ), stringType(nullptr) {
        add_annotation(AnnotationKind::CompTime);
    }
    Value *call(InterpretScope *call_scope, FunctionCall *call, Value *parent_val, bool evaluate_refs) override {
        std::string val;
        val.append(std::to_string(PROJECT_VERSION_MAJOR));
        val.append(1, '.');
        val.append(std::to_string(PROJECT_VERSION_MINOR));
        val.append(1, '.');
        val.append(std::to_string(PROJECT_VERSION_PATCH));
        return new (call_scope->allocate<StringValue>()) StringValue(std::move(val), nullptr);
    }
};

class InterpretIsTcc : public FunctionDeclaration {
public:

    BoolType boolType;

    explicit InterpretIsTcc(ASTNode* parent_node) : FunctionDeclaration(
            "is_tcc_based",
            std::vector<FunctionParam*> {},
            &boolType,
            false,
            parent_node,
            nullptr,
            std::nullopt,
            AccessSpecifier::Public
    ), boolType(nullptr) {
        add_annotation(AnnotationKind::CompTime);
    }
    Value *call(InterpretScope *call_scope, FunctionCall *call, Value *parent_val, bool evaluate_refs) override {
#ifdef TCC_BUILD
        return new (call_scope->allocate<BoolValue>()) BoolValue(true, nullptr);
#else
        return new (call_scope->allocate<BoolValue>()) BoolValue(false, nullptr);
#endif
    }
};

class InterpretIsClang : public FunctionDeclaration {
public:

    BoolType boolType;

    explicit InterpretIsClang(ASTNode* parent_node) : FunctionDeclaration(
            "is_clang_based",
            std::vector<FunctionParam*> {},
            &boolType,
            false,
            parent_node,
            nullptr,
            std::nullopt,
            AccessSpecifier::Public
    ), boolType(nullptr) {
        add_annotation(AnnotationKind::CompTime);
    }
    Value *call(InterpretScope *call_scope, FunctionCall *call, Value *parent_val, bool evaluate_refs) override {
#ifdef COMPILER_BUILD
        return new (call_scope->allocate<BoolValue>()) BoolValue(true, nullptr);
#else
        return new (call_scope->allocate<BoolValue>()) BoolValue(false, nullptr);
#endif
    }
};

class InterpretDefined : public FunctionDeclaration {
public:

    BoolType boolType;
    StringType stringType;
    FunctionParam valueParam;

    explicit InterpretDefined() : FunctionDeclaration(
            "defined",
            std::vector<FunctionParam*> {},
            &boolType,
            false,
            nullptr,
            nullptr,
            std::nullopt,
            AccessSpecifier::Public
    ), boolType(nullptr), stringType(nullptr), valueParam("value", &stringType, 0, nullptr, false, this, nullptr) {
        add_annotation(AnnotationKind::CompTime);
        params.emplace_back(&valueParam);
    }
    Value *call(InterpretScope *call_scope, FunctionCall *call, Value *parent_val, bool evaluate_refs) override {
        if(call->values.empty()) return new (call_scope->allocate<BoolValue>()) BoolValue(false, nullptr);
        auto val = call->values[0]->evaluated_value(*call_scope);
        if(val->val_kind() != ValueKind::String) return new (call_scope->allocate<BoolValue>()) BoolValue(false, nullptr);
        auto& definitions = call_scope->global->build_compiler->current_job->definitions;
        auto found = definitions.find(val->get_the_string());
        return new (call_scope->allocate<BoolValue>()) BoolValue(found != definitions.end(), nullptr);
    }
};

class InterpretSatisfies : public FunctionDeclaration {
public:

    BoolType returnType;
    AnyType anyType;
    FunctionParam valueParam;
    FunctionParam valueParam2;

    explicit InterpretSatisfies(ASTNode* parent_node) : FunctionDeclaration(
            "satisfies",
            std::vector<FunctionParam*> {},
            &returnType,
            false,
            parent_node,
            nullptr,
            std::nullopt,
            AccessSpecifier::Public
    ), returnType(nullptr), anyType(nullptr),
    valueParam("value", &anyType, 0, nullptr, false, this, nullptr),
    valueParam2("value2", &anyType, 1, nullptr, false, this, nullptr) {
        add_annotation(AnnotationKind::CompTime);
        params.emplace_back(&valueParam);
        params.emplace_back(&valueParam2);
    }
    inline Value* get_bool(InterpretScope *call_scope, bool value) {
        return new (call_scope->allocate<BoolValue>()) BoolValue(value, nullptr);
    }
    Value *call(InterpretScope *call_scope, FunctionCall *call, Value *parent_val, bool evaluate_refs) override {
        if(call->values.size() != 2) {
            call_scope->error("wrong arguments size given to compiler::satisfies function", call);
            return nullptr;
        }
        const auto val_one = call->values[0];
        const auto val_two = call->values[1];
        const auto first_type = val_one->known_type();
        if(first_type) {
            return get_bool(call_scope, first_type->satisfies(call_scope->allocator, val_two, false));
        } else {
            return get_bool(call_scope, false);
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
            "isNull",
            std::vector<FunctionParam*> {},
            &boolType,
            false,
            parent_node,
            nullptr,
            std::nullopt,
              AccessSpecifier::Public
    ), boolType(nullptr), nullVal(nullptr), anyType(nullptr), ptrType(&anyType, nullptr),
        valueParam("value", &ptrType, 0, nullptr, false, this, nullptr)
    {
        annotations.emplace_back(AnnotationKind::CompTime);
        params.emplace_back(&valueParam);
    }
    Value *call(InterpretScope *call_scope, FunctionCall *call, Value *parent_val, bool evaluate_refs) override {
        return new (call_scope->allocate<WrapValue>()) WrapValue(new (call_scope->allocate<Expression>()) Expression(call->values[0], &nullVal, Operation::IsEqual, false, nullptr));
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
            "isNotNull",
            std::vector<FunctionParam*> {},
            &boolType,
            false,
            parent_node,
            nullptr,
            std::nullopt,
              AccessSpecifier::Public
    ), boolType(nullptr), nullVal(nullptr), anyType(nullptr), ptrType(&anyType, nullptr),
        valueParam("value", &ptrType, 0, nullptr, false, this, nullptr)
    {
        annotations.emplace_back(AnnotationKind::CompTime);
        params.emplace_back(&valueParam);
    }
    Value *call(InterpretScope *call_scope, FunctionCall *call, Value *parent_val, bool evaluate_refs) override {
        return new (call_scope->allocate<WrapValue>()) WrapValue(new (call_scope->allocate<Expression>()) Expression(call->values[0], &nullVal, Operation::IsNotEqual, false, nullptr));
    }
};

class InterpretMemCopy : public FunctionDeclaration {
public:

    BoolType boolType;
    StringType stringType;
    FunctionParam destValueParam;
    FunctionParam sourceValueParam;

    explicit InterpretMemCopy(ASTNode* parent_node) : FunctionDeclaration(
            "copy",
            std::vector<FunctionParam*> {},
            &boolType,
            false,
            parent_node,
            nullptr,
            std::nullopt,
            AccessSpecifier::Public
    ), boolType(nullptr), stringType(nullptr), destValueParam("dest_value", &stringType, 0, nullptr, false, this, nullptr),
      sourceValueParam("source_value", &stringType, 1, nullptr, false, this, nullptr){
        add_annotation(AnnotationKind::CompTime);
        params.emplace_back(&destValueParam);
        params.emplace_back(&sourceValueParam);
    }
    Value *call(InterpretScope *call_scope, FunctionCall *call, Value *parent_val, bool evaluate_refs) override {
        auto& backend = *call_scope->global->backend_context;
        if(call->values.size() != 2) {
            call_scope->error("std::mem::copy called with arguments of length not equal to two", call);
            return nullptr;
        }
        backend.mem_copy(call->values[0], call->values[1]);
    }
};

//class InterpretConstruct : public FunctionDeclaration {
//public:
//    explicit InterpretConstruct(ASTNode* parent_node) : FunctionDeclaration(
//            "construct",
//            std::vector<FunctionParam*> {},
//            std::make_unique<VoidType>(),
//            false,
//            parent_node,
//            std::nullopt,
//              AccessSpecifier::Public
//    ) {
//        annotations.emplace_back(AnnotationKind::CompTime);
//        params.emplace_back(std::make_unique<FunctionParam>("ptr", std::make_unique<PointerType>(std::make_unique<VoidType>()), 0, std::nullopt, this));
//        params.emplace_back(std::make_unique<FunctionParam>("value", std::make_unique<AnyType>(), 1, std::nullopt, this));
//    }
//    Value *call(InterpretScope *call_scope, FunctionCall *call, Value *parent_val, bool evaluate_refs) override {
//
//    }
//};

class CompilerNamespace : public Namespace {
public:

    InterpretPrint printFn;
    InterpretWrap wrapFn;
    InterpretUnwrap unwrapFn;
    InterpretRetStructPtr retStructPtr;
    InterpretCompilerVersion verFn;
    InterpretIsTcc isTccFn;
    InterpretIsClang isClangFn;
    InterpretSize sizeFn;
    InterpretVector::InterpretVectorNode vectorNode;
    InterpretSatisfies satisfiesFn;

    CompilerNamespace(

    ) : Namespace("compiler", nullptr, nullptr, AccessSpecifier::Public),
        printFn(this), wrapFn(this), unwrapFn(this), retStructPtr(this), verFn(this),
        isTccFn(this), isClangFn(this), sizeFn(this), vectorNode(this),
        satisfiesFn(this)
    {

        add_annotation(AnnotationKind::CompTime);
        nodes = {
            &printFn, &wrapFn, &unwrapFn, &retStructPtr, &verFn, &isTccFn, &isClangFn, &sizeFn, &vectorNode,
            &satisfiesFn
        };

    }

};

class MemNamespace : public Namespace {
public:

    InterpretMemCopy memCopyFn;

    explicit MemNamespace(
        ASTNode* parent_node
    ) : Namespace("mem", parent_node, nullptr, AccessSpecifier::Public), memCopyFn(this) {
        add_annotation(AnnotationKind::CompTime);
        nodes = { &memCopyFn };
    }

};

class PtrNamespace : public Namespace {
public:

    InterpretIsPtrNull isNullFn;
    InterpretIsPtrNotNull isNotNullFn;

    explicit PtrNamespace(
            ASTNode* parent_node
    ) : Namespace("ptr", parent_node, nullptr, AccessSpecifier::Public),
        isNullFn(this), isNotNullFn(this)
    {
        add_annotation(AnnotationKind::CompTime);
        nodes = { &isNullFn, &isNotNullFn };
    }

};


class StdNamespace : public Namespace {
public:

    MemNamespace memNamespace;
    PtrNamespace ptrNamespace;

    StdNamespace(

    ) : Namespace("std", nullptr, nullptr, AccessSpecifier::Public),
        memNamespace(this), ptrNamespace(this)
    {
        add_annotation(AnnotationKind::CompTime);
        nodes = { &memNamespace, &ptrNamespace };
    }

};

struct GlobalContainer {
    CompilerNamespace compiler_namespace;
    StdNamespace std_namespace;
    InterpretDefined defined;
};

GlobalContainer global_fns;

void GlobalInterpretScope::prepare_top_level_namespaces(SymbolResolver& resolver) {
    global_fns.compiler_namespace.declare_top_level(resolver);
    global_fns.compiler_namespace.declare_and_link(resolver);
    global_fns.std_namespace.declare_top_level(resolver);
    global_fns.std_namespace.declare_and_link(resolver);
    global_fns.defined.declare_top_level(resolver);
    global_fns.defined.declare_and_link(resolver);
}

//void GlobalInterpretScope::rebind_compiler_namespace(SymbolResolver &resolver) {
//    auto& compiler_ns = global_nodes["compiler"];
//    compiler_ns->declare_top_level(resolver, compiler_ns);
//}