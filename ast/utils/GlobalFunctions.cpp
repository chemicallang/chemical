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
            {},
            (StructDefinition*) node,
            ZERO_LOC,
            nullptr
        ) {

        }
    };

    InterpretVectorConstructor::InterpretVectorConstructor(InterpretVectorNode* node) : FunctionDeclaration(
            ZERO_LOC_ID("constructor"),
            std::vector<FunctionParam*> {},
            &node->selfType,
            false,
            node,
            ZERO_LOC,
            std::nullopt,
            AccessSpecifier::Public,
            true
    ) {
        set_constructor_fn(true);
    }

    Value *InterpretVectorConstructor::call(InterpretScope *call_scope, ASTAllocator& allocator, FunctionCall *call, Value *parent_val, bool evaluate_refs) {
        return new (call_scope->allocate<InterpretVectorVal>()) InterpretVectorVal((InterpretVectorNode*) parent_node);
    }

    InterpretVectorSize::InterpretVectorSize(InterpretVectorNode* node) : FunctionDeclaration(
            ZERO_LOC_ID("size"),
        std::vector<FunctionParam*> {},
        &retType,
        false,
        node,
        ZERO_LOC,
        std::nullopt,
        AccessSpecifier::Public,
            true
    ), retType(ZERO_LOC), selfParam("self", &node->selfReference, 0, nullptr, true, this, ZERO_LOC) {
        params.emplace_back(&selfParam);
    }

    Value *InterpretVectorSize::call(InterpretScope *call_scope, ASTAllocator& allocator, FunctionCall *call, Value *parent_val, bool evaluate_refs) {
        return new (call_scope->allocate<IntValue>()) IntValue(static_cast<InterpretVectorVal*>(parent_val)->values.size(), ZERO_LOC);
    }


    // TODO interpret vector get should return a reference to T
    InterpretVectorGet::InterpretVectorGet(InterpretVectorNode* node) : FunctionDeclaration(
            ZERO_LOC_ID("get"),
            std::vector<FunctionParam*> {},
            &returnLinkedType,
            false,
            node,
            ZERO_LOC,
            std::nullopt,
            AccessSpecifier::Public,
            true
    ), returnLinkedType("T", &node->typeParam, ZERO_LOC),
        selfParam("self", &node->selfReference, 0, nullptr, true, this, ZERO_LOC), indexType(ZERO_LOC), indexParam("index", &indexType, 1, nullptr, false, this, ZERO_LOC)
    {
        params.emplace_back(&selfParam);
        params.emplace_back(&indexParam);
    }

    Value *InterpretVectorGet::call(InterpretScope *call_scope, ASTAllocator& allocator, FunctionCall *call, Value *parent_val, bool evaluate_refs) {
        return static_cast<InterpretVectorVal*>(parent_val)->values[call->values[0]->evaluated_value(*call_scope)->get_the_int()]->scope_value(*call_scope);
    }

    InterpretVectorPush::InterpretVectorPush(InterpretVectorNode* node) : FunctionDeclaration(
            ZERO_LOC_ID("push"),
            std::vector<FunctionParam*> {},
            &returnVoidType,
            false,
            node,
            ZERO_LOC,
            std::nullopt,
            AccessSpecifier::Public,
            true
    ), selfParam("self", &node->selfReference, 0, nullptr, true, this, ZERO_LOC), returnVoidType(ZERO_LOC),
        valueType("T", &node->typeParam, ZERO_LOC), valueParam("value", &valueType, 1, nullptr, false, this, ZERO_LOC)
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
            std::vector<FunctionParam*> {},
            &returnVoidType,
            false,
            node,
            ZERO_LOC,
            std::nullopt,
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
        ref.erase(ref.begin() + call->values[0]->evaluated_value(*call_scope)->get_the_int());
        return nullptr;
    }

    InterpretVectorNode::InterpretVectorNode(
        ASTNode* parent_node
    ): StructDefinition(ZERO_LOC_ID("vector"), parent_node, ZERO_LOC, AccessSpecifier::Public),
        constructorFn(this), sizeFn(this), getFn(this), pushFn(this), removeFn(this),
        typeParam("T", nullptr, nullptr, this, 0, ZERO_LOC),
        selfType("vector", this, ZERO_LOC), selfReference(&selfType, ZERO_LOC)
    {
        set_comptime(true);
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
            ZERO_LOC_ID("print"),
            std::vector<FunctionParam*> {},
            &returnType,
            true,
            parent_node,
            ZERO_LOC,
            std::nullopt,
            AccessSpecifier::Public,
            true
    ), visitor(ostring),  returnType(ZERO_LOC) {
        visitor.interpret_representation = true;
    }
    Value *call(InterpretScope *call_scope, ASTAllocator& allocator, FunctionCall *call, Value *parent_val, bool evaluate_refs) final {
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
            ZERO_LOC_ID("size"),
            std::vector<FunctionParam*> {},
            &returnType,
            false,
            parent_node,
            ZERO_LOC,
            std::nullopt,
            AccessSpecifier::Public,
            true
    ), returnType(ZERO_LOC), anyType(ZERO_LOC), valueParam("value", &anyType, 0, nullptr, false, this, ZERO_LOC) {
        set_comptime(true);
        params.emplace_back(&valueParam);
    }
    Value *call(InterpretScope *call_scope, ASTAllocator& allocator, FunctionCall *call, Value *parent_val, bool evaluate_refs) final {
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
                return new (call_scope->allocate<UBigIntValue>()) UBigIntValue(value->get_the_string().length(), ZERO_LOC);
            case ValueType::Array:
                return new (call_scope->allocate<UBigIntValue>()) UBigIntValue(value->as_array_value()->array_size(), ZERO_LOC);
            default:
                return new (call_scope->allocate<UBigIntValue>()) UBigIntValue(0, ZERO_LOC);
        }
    }
};

class WrapValue : public Value {
public:
    Value* underlying;
    explicit WrapValue(Value* underlying) : underlying(underlying) {

    }
    SourceLocation encoded_location() final {
        return ZERO_LOC;
    }
    void accept(Visitor *visitor) final {
        throw std::runtime_error("compiler::wrap value cannot be visited");
    }
    ValueKind val_kind() final {
        return ValueKind::WrapValue;
    }
    Value *copy(ASTAllocator& allocator) final {
        return new (allocator.allocate<WrapValue>()) WrapValue(underlying->copy(allocator));
    }
    Value* evaluated_value(InterpretScope &scope) final {
        return underlying;
    }
};

class InterpretWrap : public FunctionDeclaration {
public:

    AnyType anyType;
    FunctionParam valueParam;

    explicit InterpretWrap(ASTNode* parent_node) : FunctionDeclaration(
            ZERO_LOC_ID("wrap"),
            std::vector<FunctionParam*> {},
            &anyType,
            true,
            parent_node,
            ZERO_LOC,
            std::nullopt,
            AccessSpecifier::Public,
            true
    ), anyType(ZERO_LOC), valueParam("value", &anyType, 0, nullptr, false, this, ZERO_LOC) {
        set_comptime(true);
        // having a generic type parameter T requires that user gives type during function call to wrap
        // when we can successfully avoid giving type for generic parameters in functions, we should do this
//        generic_params.emplace_back(new (call_scope->allocate<GenericTypeParameter>()) GenericTypeParameter("T", nullptr, this));
//        returnType = std::make_unique<ReferencedType>("T", generic_params[0].get());
        params.emplace_back(&valueParam);
    }
    Value *call(InterpretScope *call_scope, ASTAllocator& allocator, FunctionCall *call, Value *parent_val, bool evaluate_refs) final {
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
            ZERO_LOC_ID("unwrap"),
            std::vector<FunctionParam*> {},
            &anyType,
            true,
            parent_node,
            ZERO_LOC,
            std::nullopt,
            AccessSpecifier::Public,
            true
    ), anyType(ZERO_LOC), valueParam("value", &anyType, 0, nullptr, false, this, ZERO_LOC) {
        set_comptime(true);
        // having a generic type parameter T requires that user gives type during function call to wrap
        // when we can successfully avoid giving type for generic parameters in functions, we should do this
//        generic_params.emplace_back(new (call_scope->allocate<GenericTypeParameter>()) GenericTypeParameter("T", nullptr, this));
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
            std::vector<FunctionParam*> {},
            &ptrType,
            true,
            parent_node,
            ZERO_LOC,
            std::nullopt,
            AccessSpecifier::Public,
            true
    ), voidType(ZERO_LOC), ptrType(&voidType, ZERO_LOC) {
        set_comptime(true);
    }
    Value *call(InterpretScope *call_scope, ASTAllocator& allocator, FunctionCall *call, Value *parent_val, bool evaluate_refs) final {
        return new (call_scope->allocate<RetStructParamValue>()) RetStructParamValue(ZERO_LOC);
    }
};

class InterpretCompilerVersion : public FunctionDeclaration {
public:

    StringType stringType;

    explicit InterpretCompilerVersion(ASTNode* parent_node) : FunctionDeclaration(
            ZERO_LOC_ID("version"),
            std::vector<FunctionParam*> {},
            &stringType,
            false,
            parent_node,
            ZERO_LOC,
            std::nullopt,
            AccessSpecifier::Public,
            true
    ), stringType(ZERO_LOC) {
        set_comptime(true);
    }
    Value *call(InterpretScope *call_scope, ASTAllocator& allocator, FunctionCall *call, Value *parent_val, bool evaluate_refs) final {
        std::string val;
        val.append(std::to_string(PROJECT_VERSION_MAJOR));
        val.append(1, '.');
        val.append(std::to_string(PROJECT_VERSION_MINOR));
        val.append(1, '.');
        val.append(std::to_string(PROJECT_VERSION_PATCH));
        return new (call_scope->allocate<StringValue>()) StringValue(std::move(val), ZERO_LOC);
    }
};

class InterpretIsTcc : public FunctionDeclaration {
public:

    BoolType boolType;

    explicit InterpretIsTcc(ASTNode* parent_node) : FunctionDeclaration(
            ZERO_LOC_ID("is_tcc_based"),
            std::vector<FunctionParam*> {},
            &boolType,
            false,
            parent_node,
            ZERO_LOC,
            std::nullopt,
            AccessSpecifier::Public,
            true
    ), boolType(ZERO_LOC) {
        set_comptime(true);
    }
    Value *call(InterpretScope *call_scope, ASTAllocator& allocator, FunctionCall *call, Value *parent_val, bool evaluate_refs) final {
#ifdef TCC_BUILD
        return new (call_scope->allocate<BoolValue>()) BoolValue(true, ZERO_LOC);
#else
        return new (call_scope->allocate<BoolValue>()) BoolValue(false, ZERO_LOC);
#endif
    }
};

class InterpretIsClang : public FunctionDeclaration {
public:

    BoolType boolType;

    explicit InterpretIsClang(ASTNode* parent_node) : FunctionDeclaration(
            ZERO_LOC_ID("is_clang"),
            std::vector<FunctionParam*> {},
            &boolType,
            false,
            parent_node,
            ZERO_LOC,
            std::nullopt,
            AccessSpecifier::Public,
            true
    ), boolType(ZERO_LOC) {
        set_comptime(true);
    }
    Value *call(InterpretScope *call_scope, ASTAllocator& allocator, FunctionCall *call, Value *parent_val, bool evaluate_refs) final {
#ifdef COMPILER_BUILD
        return new (call_scope->allocate<BoolValue>()) BoolValue(true, ZERO_LOC);
#else
        return new (call_scope->allocate<BoolValue>()) BoolValue(false, ZERO_LOC);
#endif
    }
};

class InterpretGetRawLocation : public FunctionDeclaration {
public:

    UBigIntType uIntType;

    explicit InterpretGetRawLocation() : FunctionDeclaration(
            ZERO_LOC_ID("get_raw_location"),
            std::vector<FunctionParam*> {},
            &uIntType,
            false,
            nullptr,
            ZERO_LOC,
            std::nullopt,
            AccessSpecifier::Public,
            true
    ), uIntType(ZERO_LOC) {
        set_comptime(true);
    }
    Value *call(InterpretScope *call_scope, ASTAllocator& allocator, FunctionCall *call, Value *parent_val, bool evaluate_refs) final {
        return new (call_scope->allocate<UBigIntValue>()) UBigIntValue(call->location.encoded, call->location);
    }

};

class InterpretGetLineNo : public FunctionDeclaration {
public:

    UBigIntType uIntType;

    explicit InterpretGetLineNo() : FunctionDeclaration(
            ZERO_LOC_ID("get_line_no"),
            std::vector<FunctionParam*> {},
            &uIntType,
            false,
            nullptr,
            ZERO_LOC,
            std::nullopt,
            AccessSpecifier::Public,
            true
    ), uIntType(ZERO_LOC) {
        set_comptime(true);
    }
    Value *call(InterpretScope *call_scope, ASTAllocator& allocator, FunctionCall *call, Value *parent_val, bool evaluate_refs) final {
        const auto loc = call_scope->global->loc_man.getLocation(call->location);
        return new (call_scope->allocate<UBigIntValue>()) UBigIntValue(loc.lineStart + 1, call->location);
    }

};

class InterpretGetCharacterNo : public FunctionDeclaration {
public:

    UBigIntType uIntType;

    explicit InterpretGetCharacterNo() : FunctionDeclaration(
            ZERO_LOC_ID("get_char_no"),
            std::vector<FunctionParam*> {},
            &uIntType,
            false,
            nullptr,
            ZERO_LOC,
            std::nullopt,
            AccessSpecifier::Public,
            true
    ), uIntType(ZERO_LOC) {
        set_comptime(true);
    }
    Value *call(InterpretScope *call_scope, ASTAllocator& allocator, FunctionCall *call, Value *parent_val, bool evaluate_refs) final {
        const auto loc = call_scope->global->loc_man.getLocation(call->location);
        return new (call_scope->allocate<UBigIntValue>()) UBigIntValue(loc.charStart + 1, call->location);
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
            std::vector<FunctionParam*> {},
            &uIntType,
            false,
            nullptr,
            ZERO_LOC,
            std::nullopt,
            AccessSpecifier::Public,
            true
    ), uIntType(ZERO_LOC) {
        set_comptime(true);
    }
    Value *call(InterpretScope *call_scope, ASTAllocator& allocator, FunctionCall *call, Value *parent_val, bool evaluate_refs) final {
        const auto global = call_scope->global;;
        const auto runtime_call = get_runtime_call(global);
        if(runtime_call) {
            const auto loc = global->loc_man.getLocation(runtime_call->location);
            return new (call_scope->allocate<UBigIntValue>()) UBigIntValue(loc.lineStart + 1, ZERO_LOC);
        } else {
            return new (call_scope->allocate<UBigIntValue>()) UBigIntValue(0, ZERO_LOC);
        }
    }

};

class InterpretGetCallerCharacterNo : public FunctionDeclaration {
public:

    UBigIntType uIntType;

    explicit InterpretGetCallerCharacterNo() : FunctionDeclaration(
            ZERO_LOC_ID("get_caller_char_no"),
            std::vector<FunctionParam*> {},
            &uIntType,
            false,
            nullptr,
            ZERO_LOC,
            std::nullopt,
            AccessSpecifier::Public,
            true
    ), uIntType(ZERO_LOC) {
        set_comptime(true);
    }
    Value *call(InterpretScope *call_scope, ASTAllocator& allocator, FunctionCall *self_call, Value *parent_val, bool evaluate_refs) final {
        const auto global = call_scope->global;;
        const auto runtime_call = get_runtime_call(global);
        if(runtime_call) {
            const auto loc = global->loc_man.getLocation(runtime_call->location);
            return new (call_scope->allocate<UBigIntValue>()) UBigIntValue(loc.charStart + 1, ZERO_LOC);
        } else {
            return new (call_scope->allocate<UBigIntValue>()) UBigIntValue(0, ZERO_LOC);
        }
    }

};

class InterpretDefined : public FunctionDeclaration {
public:

    BoolType boolType;
    StringType stringType;
    FunctionParam valueParam;

    explicit InterpretDefined() : FunctionDeclaration(
            ZERO_LOC_ID("defined"),
            std::vector<FunctionParam*> {},
            &boolType,
            false,
            nullptr,
            ZERO_LOC,
            std::nullopt,
            AccessSpecifier::Public,
            true
    ), boolType(ZERO_LOC), stringType(ZERO_LOC), valueParam("value", &stringType, 0, nullptr, false, this, ZERO_LOC) {
        set_comptime(true);
        params.emplace_back(&valueParam);
    }
    Value *call(InterpretScope *call_scope, ASTAllocator& allocator, FunctionCall *call, Value *parent_val, bool evaluate_refs) final {
        if(call->values.empty()) return new (call_scope->allocate<BoolValue>()) BoolValue(false, ZERO_LOC);
        auto val = call->values[0]->evaluated_value(*call_scope);
        if(val->val_kind() != ValueKind::String) return new (call_scope->allocate<BoolValue>()) BoolValue(false, ZERO_LOC);
        auto& definitions = call_scope->global->build_compiler->current_job->definitions;
        auto found = definitions.find(val->get_the_string());
        return new (call_scope->allocate<BoolValue>()) BoolValue(found != definitions.end(), ZERO_LOC);
    }
};

// function is used to error out at compile time
class InterpretError : public FunctionDeclaration {
public:

    VoidType voidType;
    StringType stringType;
    FunctionParam valueParam;

    explicit InterpretError() : FunctionDeclaration(
            ZERO_LOC_ID("error"),
            std::vector<FunctionParam*> {},
            &voidType,
            false,
            nullptr,
            ZERO_LOC,
            std::nullopt,
            AccessSpecifier::Public,
            true
    ), voidType(ZERO_LOC), stringType(ZERO_LOC), valueParam("value", &stringType, 0, nullptr, false, this, ZERO_LOC) {
        set_comptime(true);
        params.emplace_back(&valueParam);
    }
    Value *call(InterpretScope *call_scope, ASTAllocator& allocator, FunctionCall *call, Value *parent_val, bool evaluate_refs) final {
        if(call->values.empty()) return new (call_scope->allocate<BoolValue>()) BoolValue(false, ZERO_LOC);
        auto val = call->values[0]->evaluated_value(*call_scope);
        if(val->val_kind() != ValueKind::String) return new (call_scope->allocate<BoolValue>()) BoolValue(false, ZERO_LOC);
        call_scope->error(val->get_the_string(), call);
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
            std::vector<FunctionParam*> {},
            &returnType,
            false,
            parent_node,
            ZERO_LOC,
            std::nullopt,
            AccessSpecifier::Public,
            true
    ), returnType(ZERO_LOC), anyType(ZERO_LOC),
    valueParam("value", &anyType, 0, nullptr, false, this, ZERO_LOC),
    valueParam2("value2", &anyType, 1, nullptr, false, this, ZERO_LOC) {
        set_comptime(true);
        params.emplace_back(&valueParam);
        params.emplace_back(&valueParam2);
    }
    inline Value* get_bool(InterpretScope *call_scope, bool value) {
        return new (call_scope->allocate<BoolValue>()) BoolValue(value, ZERO_LOC);
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
            ZERO_LOC_ID("isNull"),
            std::vector<FunctionParam*> {},
            &boolType,
            false,
            parent_node,
            ZERO_LOC,
            std::nullopt,
            AccessSpecifier::Public,
            true
    ), boolType(ZERO_LOC), nullVal(ZERO_LOC), anyType(ZERO_LOC), ptrType(&anyType, ZERO_LOC),
        valueParam("value", &ptrType, 0, nullptr, false, this, ZERO_LOC)
    {
        set_comptime(true);
        params.emplace_back(&valueParam);
    }
    Value *call(InterpretScope *call_scope, ASTAllocator& allocator, FunctionCall *call, Value *parent_val, bool evaluate_refs) final {
        return new (call_scope->allocate<WrapValue>()) WrapValue(new (call_scope->allocate<Expression>()) Expression(call->values[0], &nullVal, Operation::IsEqual, false, ZERO_LOC));
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
            std::vector<FunctionParam*> {},
            &boolType,
            false,
            parent_node,
            ZERO_LOC,
            std::nullopt,
            AccessSpecifier::Public,
            true
    ), boolType(ZERO_LOC), nullVal(ZERO_LOC), anyType(ZERO_LOC), ptrType(&anyType, ZERO_LOC),
        valueParam("value", &ptrType, 0, nullptr, false, this, ZERO_LOC)
    {
        set_comptime(true);
        params.emplace_back(&valueParam);
    }
    Value *call(InterpretScope *call_scope, ASTAllocator& allocator, FunctionCall *call, Value *parent_val, bool evaluate_refs) final {
        return new (call_scope->allocate<WrapValue>()) WrapValue(new (call_scope->allocate<Expression>()) Expression(call->values[0], &nullVal, Operation::IsNotEqual, false, ZERO_LOC));
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
            std::vector<FunctionParam*> {},
            &boolType,
            false,
            parent_node,
            ZERO_LOC,
            std::nullopt,
            AccessSpecifier::Public,
            true
    ), boolType(ZERO_LOC), stringType(ZERO_LOC), destValueParam("dest_value", &stringType, 0, nullptr, false, this, ZERO_LOC),
      sourceValueParam("source_value", &stringType, 1, nullptr, false, this, ZERO_LOC){
        set_comptime(true);
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
            std::vector<FunctionParam*> {},
            &stringType,
            false,
            parent_node,
            ZERO_LOC,
            std::nullopt,
            AccessSpecifier::Public,
            true
    ), stringType(ZERO_LOC) {
        set_comptime(true);
    }
    Value *call(InterpretScope *call_scope, ASTAllocator& allocator, FunctionCall *call, Value *parent_val, bool evaluate_refs) final {
        return new (call_scope->allocate<StringValue>()) StringValue(call_scope->global->target_triple, call->location);
    }
};

class InterpretGetCurrentFilePath : public FunctionDeclaration {
public:

    StringType stringType;

    explicit InterpretGetCurrentFilePath(ASTNode* parent_node) : FunctionDeclaration(
            ZERO_LOC_ID("get_current_file_path"),
            std::vector<FunctionParam*> {},
            &stringType,
            false,
            parent_node,
            ZERO_LOC,
            std::nullopt,
            AccessSpecifier::Public,
            true
    ), stringType(ZERO_LOC) {
        set_comptime(true);
    }
    Value *call(InterpretScope *call_scope, ASTAllocator& allocator, FunctionCall *call, Value *parent_val, bool evaluate_refs) final {
        auto& loc_man = call_scope->global->loc_man;
        auto location = loc_man.getLocation(call->location);
        auto fileId = loc_man.getPathForFileId(location.fileId);
        return new (call_scope->allocate<StringValue>()) StringValue(std::string(fileId), call->location);
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
//        set_comptime(true);
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
    InterpretGetLineNo get_line_no;
    InterpretGetCharacterNo get_char_no;
    InterpretGetCallerLineNo get_caller_line_no;
    InterpretGetCallerCharacterNo get_caller_char_no;
    InterpretGetTarget get_target_fn;
    InterpretGetCurrentFilePath get_current_file_path;
    InterpretError error_fn;

    CompilerNamespace(

    ) : Namespace("compiler", nullptr, ZERO_LOC, AccessSpecifier::Public),
        printFn(this), wrapFn(this), unwrapFn(this), retStructPtr(this), verFn(this),
        isTccFn(this), isClangFn(this), sizeFn(this), vectorNode(this),
        satisfiesFn(this), get_target_fn(this), get_current_file_path(this)
    {

        set_comptime(true);
        nodes = {
            &printFn, &wrapFn, &unwrapFn, &retStructPtr, &verFn, &isTccFn, &isClangFn, &sizeFn, &vectorNode,
            &satisfiesFn, &get_raw_location, &get_line_no, &get_char_no, &get_caller_line_no, &get_caller_char_no,
            &error_fn, &get_target_fn, &get_current_file_path
        };

    }

};

class MemNamespace : public Namespace {
public:

    InterpretMemCopy memCopyFn;

    explicit MemNamespace(
        ASTNode* parent_node
    ) : Namespace("mem", parent_node, ZERO_LOC, AccessSpecifier::Public), memCopyFn(this) {
        set_comptime(true);
        nodes = { &memCopyFn };
    }

};

class PtrNamespace : public Namespace {
public:

    InterpretIsPtrNull isNullFn;
    InterpretIsPtrNotNull isNotNullFn;

    explicit PtrNamespace(
            ASTNode* parent_node
    ) : Namespace("ptr", parent_node, ZERO_LOC, AccessSpecifier::Public),
        isNullFn(this), isNotNullFn(this)
    {
        set_comptime(true);
        nodes = { &isNullFn, &isNotNullFn };
    }

};


class StdNamespace : public Namespace {
public:

    MemNamespace memNamespace;
    PtrNamespace ptrNamespace;

    StdNamespace(

    ) : Namespace("std", nullptr, ZERO_LOC, AccessSpecifier::Public),
        memNamespace(this), ptrNamespace(this)
    {
        set_comptime(true);
        nodes = { &memNamespace, &ptrNamespace };
    }

};

class DefDecl : public StructDefinition {
public:

    DefDecl() : StructDefinition(
            ZERO_LOC_ID("Def"), nullptr, ZERO_LOC, AccessSpecifier::Public
    ) {
        set_comptime(true);
    }

};

class DefValue : public StructValue {
public:

    LinkedType id;

    DefValue(DefDecl *defDecl) : StructValue(
            &id,
            {},
            defDecl,
            ZERO_LOC,
            nullptr
    ), id("Def", defDecl, ZERO_LOC) {}

};

struct DefThing {

    DefDecl decl;
    DefValue defValue;
    VarInitStatement defStmt;

    DefThing() : defValue(&decl), defStmt(true, ZERO_LOC_ID("def"), defValue.refType, &defValue, nullptr, ZERO_LOC, AccessSpecifier::Public) {
        defStmt.set_comptime(true);
    }

    void declare_value(ASTAllocator& allocator, const std::string& name, BaseType* type, Value* value) {
        const auto member = new (allocator.allocate<StructMember>()) StructMember(name, type, nullptr, &decl, ZERO_LOC, true);
        decl.variables[name] = member;
        const auto init = new (allocator.allocate<StructMemberInitializer>()) StructMemberInitializer(name, value, &defValue, member);
        defValue.values[name] = init;
    }

    void clear_values() {
        decl.variables.clear();
        defValue.values.clear();
    }

    inline void declare_value(ASTAllocator& allocator, const std::string& name, Value* value) {
        declare_value(allocator, name, value->create_type(allocator), value);
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

#ifdef __GNUC__
    data.is_gcc = true;
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

}

#endif

BoolValue* boolValue(ASTAllocator& allocator, bool value) {
    return new (allocator.allocate<BoolValue>()) BoolValue(value, ZERO_LOC);
}

void create_target_data_in_def(GlobalInterpretScope& scope, DefThing& defThing) {
    TargetData targetData;
    scope.prepare_target_data(targetData);
    // declaring native definitions like windows and stuff
    auto& allocator = scope.allocator;
    // we change the global interpret scope for each job, so we must redeclare def value
    scope.values["def"] = &defThing.defValue;
    const auto boolType = new (allocator.allocate<BoolType>()) BoolType(ZERO_LOC);
    const auto mode = scope.build_compiler->options->def_mode;
    defThing.declare_value(allocator, "debug", boolType, boolValue(allocator, is_debug(mode)));
    defThing.declare_value(allocator, "debug_quick", boolType, boolValue(allocator, mode == OutputMode::DebugQuick));
    defThing.declare_value(allocator, "debug_complete", boolType, boolValue(allocator, mode == OutputMode::DebugComplete));
    defThing.declare_value(allocator, "release", boolType, boolValue(allocator, is_release(mode)));
    defThing.declare_value(allocator, "release_safe", boolType, boolValue(allocator, mode == OutputMode::ReleaseSafe));
    defThing.declare_value(allocator, "release_small", boolType, boolValue(allocator, mode == OutputMode::ReleaseSmall));
    defThing.declare_value(allocator, "release_fast", boolType, boolValue(allocator, mode == OutputMode::ReleaseFast));
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

    container.compiler_namespace.declare_top_level(resolver);
    container.std_namespace.declare_top_level(resolver);

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

    container.compiler_namespace.declare_top_level(resolver);
    container.std_namespace.declare_top_level(resolver);
    container.defined.declare_top_level(resolver);

    // definitions using defThing
    container.defThing.decl.declare_top_level(resolver);
    container.defThing.defStmt.declare_top_level(resolver);

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