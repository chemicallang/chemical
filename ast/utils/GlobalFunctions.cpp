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
#include "ast/values/IntNumValue.h"
#include "ast/values/Expression.h"
#include "ast/values/BoolValue.h"
#include "ast/values/CastedValue.h"
#include "ast/values/WrapValue.h"
#include "ast/values/RetStructParamValue.h"
#include "ast/values/StructValue.h"
#include "ast/values/StringValue.h"
#include "ast/values/ArrayValue.h"
#include "ast/values/ExtractionValue.h"
#include "ast/values/ExpressiveString.h"
#include "ast/values/BlockValue.h"
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
#include "ast/types/AnyType.h"
#include "preprocess/RepresentationVisitor.h"
#include "utils/Version.h"
#include "ast/values/NullValue.h"
#include "ast/values/RawLiteral.h"
#include "ast/values/MultipleValue.h"
#include "ast/types/ReferenceType.h"
#include "ast/types/IntNType.h"
#include "core/source/LocationManager.h"
#include "ast/base/TypeBuilder.h"
#include "compiler/symres/DeclareTopLevel.h"
#include "compiler/processor/ModuleFileData.h"
#include "ast/statements/AccessChainNode.h"
#include "ast/statements/Typealias.h"
#include "ast/structures/GenericTypeDecl.h"
#include "utils/PathUtils.h"

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

        explicit InterpretVectorSize(TypeBuilder& cache, InterpretVectorNode* node);

        Value *call(InterpretScope *call_scope, ASTAllocator& allocator, FunctionCall *call, Value *parent_val, bool evaluate_refs) final;

    };

    class InterpretVectorGet : public FunctionDeclaration {
    public:

        FunctionParam selfParam;
        FunctionParam indexParam;
        LinkedType returnLinkedType;

        explicit InterpretVectorGet(TypeBuilder& cache, InterpretVectorNode* node);

        Value *call(InterpretScope *call_scope, ASTAllocator& allocator, FunctionCall *call, Value *parent_val, bool evaluate_refs) final;

    };

    class InterpretVectorPush : public FunctionDeclaration {
    public:

        FunctionParam selfParam;
        LinkedType valueType;
        FunctionParam valueParam;

        explicit InterpretVectorPush(TypeBuilder& cache, InterpretVectorNode* node);

        Value *call(InterpretScope *call_scope, ASTAllocator& allocator, FunctionCall *call, Value *parent_val, bool evaluate_refs) final;

    };

    class InterpretVectorRemove : public FunctionDeclaration {
    public:

        FunctionParam selfParam;
        FunctionParam indexParam;

        explicit InterpretVectorRemove(TypeBuilder& cache, InterpretVectorNode* node);

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

        explicit InterpretVectorNode(TypeBuilder& cache, ASTNode* parent_node);

    };

    class InterpretVectorVal : public StructValue {
    public:
        std::vector<Value*> values;
        explicit InterpretVectorVal(InterpretVectorNode* node) : StructValue(
            node->known_type(),
            (StructDefinition*) node,
            (StructDefinition*) node,
            ZERO_LOC
        ) {

        }
    };

    InterpretVectorConstructor::InterpretVectorConstructor(InterpretVectorNode* node) : FunctionDeclaration(
            ZERO_LOC_ID("constructor"),
            {&node->selfType, ZERO_LOC},
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
            call_scope->error("expected a generic argument for intrinsics::vector", call);
            return nullptr;
        }
        const auto node = (InterpretVectorNode*) parent();
        return new (allocator.allocate<InterpretVectorVal>()) InterpretVectorVal(node);
    }

    InterpretVectorSize::InterpretVectorSize(TypeBuilder& cache, InterpretVectorNode* node) : FunctionDeclaration(
            ZERO_LOC_ID("size"),
            {cache.getIntType(), ZERO_LOC},
            false,
            node,
            ZERO_LOC,
            AccessSpecifier::Public,
            true
    ), selfParam("self", TypeLoc(&node->selfReference, ZERO_LOC), 0, nullptr, true, this, ZERO_LOC) {
        params.emplace_back(&selfParam);
    }

    Value *InterpretVectorSize::call(InterpretScope *call_scope, ASTAllocator& allocator, FunctionCall *call, Value *parent_val, bool evaluate_refs) {
        return new (allocator.allocate<IntNumValue>()) IntNumValue(static_cast<InterpretVectorVal*>(parent_val)->values.size(), call_scope->global->typeBuilder.getIntType(), ZERO_LOC);
    }


    // TODO interpret vector get should return a reference to T
    InterpretVectorGet::InterpretVectorGet(TypeBuilder& cache, InterpretVectorNode* node) : FunctionDeclaration(
            ZERO_LOC_ID("get"),
            {&returnLinkedType, ZERO_LOC},
            false,
            node,
            ZERO_LOC,
            AccessSpecifier::Public,
            true
    ), returnLinkedType(&node->typeParam), selfParam("self", {&node->selfReference, ZERO_LOC}, 0, nullptr, true, this, ZERO_LOC),
        indexParam("index", { cache.getUIntType(), ZERO_LOC }, 1, nullptr, false, this, ZERO_LOC)
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
            return new (call_scope->allocate<NullValue>()) NullValue(call_scope->global->typeBuilder.getNullPtrType(), call->encoded_location());
        }

    }

    InterpretVectorPush::InterpretVectorPush(TypeBuilder& cache, InterpretVectorNode* node) : FunctionDeclaration(
            ZERO_LOC_ID("push"),
            {cache.getVoidType(), ZERO_LOC},
            false,
            node,
            ZERO_LOC,
            AccessSpecifier::Public,
            true
    ), selfParam("self", { &node->selfReference, ZERO_LOC }, 0, nullptr, true, this, ZERO_LOC),
                                                                                              valueType(&node->typeParam), valueParam("value", { &valueType, ZERO_LOC }, 1, nullptr, false, this, ZERO_LOC)
    {
        params.emplace_back(&selfParam);
        params.emplace_back(&valueParam);
    }

    Value *InterpretVectorPush::call(InterpretScope *call_scope, ASTAllocator& allocator, FunctionCall *call, Value *parent_val, bool evaluate_refs) {
        static_cast<InterpretVectorVal*>(parent_val)->values.emplace_back(call->values[0]->scope_value(*call_scope));
        return nullptr;
    }

    InterpretVectorRemove::InterpretVectorRemove(TypeBuilder& cache, InterpretVectorNode* node) : FunctionDeclaration(
            ZERO_LOC_ID("remove"),
            {cache.getVoidType(), ZERO_LOC},
            false,
            node,
            ZERO_LOC,
            AccessSpecifier::Public,
            true
    ), selfParam("self", { &node->selfReference, ZERO_LOC }, 0, nullptr, true, this, ZERO_LOC),
                                                                                                  indexParam("index", { cache.getUIntType(), ZERO_LOC }, 1, nullptr, false, this, ZERO_LOC)
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
            TypeBuilder& cache,
            ASTNode* parent_node
    ): StructDefinition(ZERO_LOC_ID("vector"), parent_node, ZERO_LOC, AccessSpecifier::Public),
        constructorFn(this), sizeFn(cache, this), getFn(cache, this), pushFn(cache, this), removeFn(cache, this),
        typeParam("T", nullptr, nullptr, this, 0, ZERO_LOC),
        selfType(this), selfReference(&selfType, ZERO_LOC)
    {
        set_compiler_decl(true);
        insert_functions({ &constructorFn, &sizeFn, &getFn, &pushFn, &removeFn });
    }

}

class InterpretPrint : public FunctionDeclaration {
public:

    FunctionParam param;
    std::ostringstream ostring;
    RepresentationVisitor visitor;

    explicit InterpretPrint(TypeBuilder& cache, ASTNode* parent_node, LocatedIdentifier identifier) : FunctionDeclaration(
            identifier,
            {cache.getVoidType(), ZERO_LOC},
            true,
            parent_node,
            ZERO_LOC,
            AccessSpecifier::Public,
            true
    ), visitor(ostring), param("value", { cache.getAnyType(), ZERO_LOC }, 0, nullptr, false, this, 0)
    {
        set_compiler_decl(true);
        visitor.interpret_representation = true;
        params = { &param };
    };

    inline explicit InterpretPrint(TypeBuilder& cache, ASTNode* parent_node) : InterpretPrint(cache, parent_node, ZERO_LOC_ID("print")) {

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
    inline explicit InterpretPrintLn(TypeBuilder& cache, ASTNode* parent_node) : InterpretPrint(cache, parent_node, ZERO_LOC_ID("println")) {

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

    explicit InterpretToString(TypeBuilder& cache, ASTNode* parent_node) : FunctionDeclaration(
            ZERO_LOC_ID("to_string"),
            {cache.getStringType(), ZERO_LOC},
            true,
            parent_node,
            ZERO_LOC,
            AccessSpecifier::Public,
            true
    ), visitor(ostring) {
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
        const auto returnValue = new (allocator.allocate<StringValue>()) StringValue(chem::string_view(allocator.allocate_str(str.data(), str.size()), str.size()), call_scope->global->typeBuilder.getStringType(), call->encoded_location());
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

    FunctionParam valueParam;

    explicit InterpretSize(TypeBuilder& cache, ASTNode* parent_node) : FunctionDeclaration(
            ZERO_LOC_ID("size"),
            {cache.getU64Type(), ZERO_LOC},
            false,
            parent_node,
            ZERO_LOC,
            AccessSpecifier::Public,
            true
    ), valueParam("value", { cache.getAnyType(), ZERO_LOC }, 0, nullptr, false, this, ZERO_LOC) {
        set_compiler_decl(true);
        params.emplace_back(&valueParam);
    }
    Value *call(InterpretScope *call_scope, ASTAllocator& allocator, FunctionCall *call, Value *parent_val, bool evaluate_refs) final {
        if(call->values.empty()) {
            call_scope->error("intrinsics::size called without arguments", call);
            return nullptr;
        }
        const auto val = call->values[0];
//        const auto val_type = val->getType();
//        const auto val_type_pure = val_type->pure_type(allocator);
//        const auto val_type_kind = val_type_pure->kind();
//        if(val_type_kind != BaseTypeKind::String && val_type_kind != BaseTypeKind::Array) {
//            call_scope->error("intrinsics::size called with invalid arguments", call);
//            return nullptr;
//        }
        auto value = resolve_ref(val, call_scope);
        if(!value) {
            call_scope->error("couldn't get value for intrinsics::size", call);
            return nullptr;
        }
        const auto val_kind = value->val_kind();
        if(val_kind != ValueKind::String && val_kind != ValueKind::ArrayValue) {
            call_scope->error("intrinsics::size called with invalid arguments", call);
            return nullptr;
        }
        switch(val_kind) {
            case ValueKind::String:
                return new (allocator.allocate<IntNumValue>()) IntNumValue(value->get_the_string().size(), call_scope->global->typeBuilder.getU64Type(), ZERO_LOC);
            case ValueKind::ArrayValue:
                return new (allocator.allocate<IntNumValue>()) IntNumValue(value->as_array_value()->array_size(), call_scope->global->typeBuilder.getU64Type(), ZERO_LOC);
            default:
                return new (allocator.allocate<IntNumValue>()) IntNumValue(0, call_scope->global->typeBuilder.getU64Type(), ZERO_LOC);
        }
    }
};

/**
 * evaluates comptime identifiers and function calls in a wrap value
 * suppose intrinsics::wrap(constructor(value, intrinsics::size(value)))
 * in this value constructor is a function call that is not comptime
 * so constructor won't be called, but intrinsics::size is a comptime
 * function call, intrinsics::size will be called with value as argument
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

    FunctionParam valueParam;

    explicit InterpretWrap(TypeBuilder& cache, ASTNode* parent_node) : FunctionDeclaration(
            ZERO_LOC_ID("wrap"),
            {cache.getAnyType(), ZERO_LOC},
            true,
            parent_node,
            ZERO_LOC,
            AccessSpecifier::Public,
            true
    ), valueParam("value", { cache.getAnyType(), ZERO_LOC }, 0, nullptr, false, this, ZERO_LOC)
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

    FunctionParam valueParam;

    explicit InterpretUnwrap(TypeBuilder& cache, ASTNode* parent_node) : FunctionDeclaration(
            ZERO_LOC_ID("unwrap"),
            {cache.getAnyType(), ZERO_LOC},
            true,
            parent_node,
            ZERO_LOC,
            AccessSpecifier::Public,
            true
    ), valueParam("value", { cache.getAnyType(), ZERO_LOC }, 0, nullptr, false, this, ZERO_LOC) {
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

    FunctionParam param;
    // TODO we shouldn't return pointer to void type
    PointerType ptrType;

    explicit InterpretRetStructPtr(TypeBuilder& cache, ASTNode* parent_node) : FunctionDeclaration(
            ZERO_LOC_ID("return_struct"),
            {&ptrType, ZERO_LOC},
            true,
            parent_node,
            ZERO_LOC,
            AccessSpecifier::Public,
            true
    ), ptrType(cache.getVoidType(), ZERO_LOC),
        param("value", { cache.getAnyType(), ZERO_LOC }, 0, nullptr, false, this, 0)
    {
        set_compiler_decl(true);
        params = { &param };
    };
    Value *call(InterpretScope *call_scope, ASTAllocator& allocator, FunctionCall *call, Value *parent_val, bool evaluate_refs) final {
        return new (allocator.allocate<RetStructParamValue>()) RetStructParamValue(ZERO_LOC);
    }
};

class InterpretCompilerVersion : public FunctionDeclaration {
public:

    explicit InterpretCompilerVersion(TypeBuilder& cache, ASTNode* parent_node) : FunctionDeclaration(
            ZERO_LOC_ID("version"),
            {cache.getStringType(), ZERO_LOC},
            false,
            parent_node,
            ZERO_LOC,
            AccessSpecifier::Public,
            true
    ) {
        set_compiler_decl(true);
    }
    Value *call(InterpretScope *call_scope, ASTAllocator& allocator, FunctionCall *call, Value *parent_val, bool evaluate_refs) final {
        return new (allocator.allocate<StringValue>()) StringValue(VERSION_STRING, call_scope->global->typeBuilder.getStringType(), ZERO_LOC);
    }
};

class InterpretSupports : public FunctionDeclaration {
public:

    FunctionParam valueParam;

    explicit InterpretSupports(TypeBuilder& cache, ASTNode* parent_node) : FunctionDeclaration(
            ZERO_LOC_ID("supports"),
            {cache.getBoolType(), ZERO_LOC},
            false,
            parent_node,
            ZERO_LOC,
            AccessSpecifier::Public,
            true
    ), valueParam("value", { cache.getAnyType(), ZERO_LOC }, 0, nullptr, false, this, ZERO_LOC) {
        set_compiler_decl(true);
        params.emplace_back(&valueParam);
    }
    Value *call(InterpretScope *call_scope, ASTAllocator& allocator, FunctionCall *call, Value *parent_val, bool evaluate_refs) final {
        if(call->values.empty()) {
            call_scope->error("expects a single argument", call);
            return new (allocator.allocate<BoolValue>()) BoolValue(false, call_scope->global->typeBuilder.getBoolType(), ZERO_LOC);
        }
        const auto eval = call->values.back()->evaluated_value(*call_scope);
        bool supports = false;
        if(call_scope->global->backend_context) {
            if (eval->kind() == ValueKind::String) {
                // TODO compiler feature cannot handle other features yet!!
                if (eval->get_the_string() == "float128") {
                    supports = call_scope->global->backend_context->supports(CompilerFeatureKind::Float128);
                }
            } else {
                const auto number = eval->get_number();
                if (number.has_value()) {
                    supports = call_scope->global->backend_context->supports((CompilerFeatureKind) (int) number.value());
                }
            }
        }
        return new (allocator.allocate<BoolValue>()) BoolValue(supports, call_scope->global->typeBuilder.getBoolType(), ZERO_LOC);
    }
};

class InterpretIsTcc : public FunctionDeclaration {
public:

    explicit InterpretIsTcc(TypeBuilder& cache, ASTNode* parent_node) : FunctionDeclaration(
            ZERO_LOC_ID("is_tcc_based"),
            {cache.getBoolType(), ZERO_LOC},
            false,
            parent_node,
            ZERO_LOC,
            AccessSpecifier::Public,
            true
    ) {
        set_compiler_decl(true);
    }
    Value *call(InterpretScope *call_scope, ASTAllocator& allocator, FunctionCall *call, Value *parent_val, bool evaluate_refs) final {
#ifdef TCC_BUILD
        return new (allocator.allocate<BoolValue>()) BoolValue(true, call_scope->global->typeBuilder.getBoolType(), ZERO_LOC);
#else
        return new (allocator.allocate<BoolValue>()) BoolValue(false, call_scope->global->typeBuilder.getBoolType(), ZERO_LOC);
#endif
    }
};

class InterpretIsClang : public FunctionDeclaration {
public:

    explicit InterpretIsClang(TypeBuilder& cache, ASTNode* parent_node) : FunctionDeclaration(
            ZERO_LOC_ID("is_clang"),
            {cache.getBoolType(), ZERO_LOC},
            false,
            parent_node,
            ZERO_LOC,
            AccessSpecifier::Public,
            true
    ) {
        set_compiler_decl(true);
    }
    Value *call(InterpretScope *call_scope, ASTAllocator& allocator, FunctionCall *call, Value *parent_val, bool evaluate_refs) final {
#ifdef COMPILER_BUILD
        return new (allocator.allocate<BoolValue>()) BoolValue(true, call_scope->global->typeBuilder.getBoolType(), ZERO_LOC);
#else
        return new (allocator.allocate<BoolValue>()) BoolValue(false, call_scope->global->typeBuilder.getBoolType(), ZERO_LOC);
#endif
    }
};

class InterpretGetRawLocation : public FunctionDeclaration {
public:

    explicit InterpretGetRawLocation(TypeBuilder& cache, ASTNode* parent) : FunctionDeclaration(
            ZERO_LOC_ID("get_raw_location"),
            {cache.getUIntType(), ZERO_LOC},
            false,
            parent,
            ZERO_LOC,
            AccessSpecifier::Public,
            true
    ) {
        set_compiler_decl(true);
    }
    Value *call(InterpretScope *call_scope, ASTAllocator& allocator, FunctionCall *call, Value *parent_val, bool evaluate_refs) final {
        return new (allocator.allocate<IntNumValue>()) IntNumValue(call->encoded_location().encoded, call_scope->global->typeBuilder.getU64Type(), call->encoded_location());
    }

};

class InterpretGetRawLocOf : public FunctionDeclaration {
public:

    explicit InterpretGetRawLocOf(TypeBuilder& cache, ASTNode* parent) : FunctionDeclaration(
            ZERO_LOC_ID("get_raw_loc_of"),
            {cache.getUIntType(), ZERO_LOC},
            false,
            parent,
            ZERO_LOC,
            AccessSpecifier::Public,
            true
    ) {
        set_compiler_decl(true);
    }
    Value *call(InterpretScope *call_scope, ASTAllocator& allocator, FunctionCall *call, Value *parent_val, bool evaluate_refs) final {
        if(call->values.empty()) {
            call_scope->error("get_raw_loc_of expects a single argument", call);
            return nullptr;
        }
        const auto first_arg = call->values[0];
        const auto first_arg_eval = first_arg ? first_arg->evaluated_value(*call_scope) : first_arg;
        return new (allocator.allocate<IntNumValue>()) IntNumValue(first_arg_eval->encoded_location().encoded, call_scope->global->typeBuilder.getU64Type(), call->encoded_location());
    }

};

class InterpretGetLineNo : public FunctionDeclaration {
public:

    U64Type uIntType;

    explicit InterpretGetLineNo() : FunctionDeclaration(
            ZERO_LOC_ID("get_line_no"),
            {&uIntType, ZERO_LOC},
            false,
            nullptr,
            ZERO_LOC,
            AccessSpecifier::Public,
            true
    ), uIntType() {
        set_compiler_decl(true);
    }
    Value *call(InterpretScope *call_scope, ASTAllocator& allocator, FunctionCall *call, Value *parent_val, bool evaluate_refs) final {
        const auto loc = call_scope->global->loc_man.getLocation(call->encoded_location());
        return new (allocator.allocate<IntNumValue>()) IntNumValue(loc.lineStart + 1, call_scope->global->typeBuilder.getU64Type(), call->encoded_location());
    }

};

class InterpretGetCharacterNo : public FunctionDeclaration {
public:

    explicit InterpretGetCharacterNo(TypeBuilder& cache, ASTNode* parent) : FunctionDeclaration(
            ZERO_LOC_ID("get_char_no"),
            {cache.getUIntType(), ZERO_LOC},
            false,
            parent,
            ZERO_LOC,
            AccessSpecifier::Public,
            true
    ) {
        set_compiler_decl(true);
    }
    Value *call(InterpretScope *call_scope, ASTAllocator& allocator, FunctionCall *call, Value *parent_val, bool evaluate_refs) final {
        const auto loc = call_scope->global->loc_man.getLocation(call->encoded_location());
        return new (allocator.allocate<IntNumValue>()) IntNumValue(loc.charStart + 1, call_scope->global->typeBuilder.getU64Type(), call->encoded_location());
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

    explicit InterpretGetCallerLineNo(TypeBuilder& cache, ASTNode* parent) : FunctionDeclaration(
            ZERO_LOC_ID("get_caller_line_no"),
            {cache.getUIntType(), ZERO_LOC},
            false,
            parent,
            ZERO_LOC,
            AccessSpecifier::Public,
            true
    ) {
        set_compiler_decl(true);
    }
    Value *call(InterpretScope *call_scope, ASTAllocator& allocator, FunctionCall *call, Value *parent_val, bool evaluate_refs) final {
        const auto global = call_scope->global;;
        const auto runtime_call = get_runtime_call(global);
        if(runtime_call) {
            const auto loc = global->loc_man.getLocation(runtime_call->encoded_location());
            return new (allocator.allocate<IntNumValue>()) IntNumValue(loc.lineStart + 1, call_scope->global->typeBuilder.getU64Type(), ZERO_LOC);
        } else {
            return new (allocator.allocate<IntNumValue>()) IntNumValue(0, call_scope->global->typeBuilder.getU64Type(), ZERO_LOC);
        }
    }

};

class InterpretGetCallerCharacterNo : public FunctionDeclaration {
public:

    explicit InterpretGetCallerCharacterNo(TypeBuilder& cache, ASTNode* parent) : FunctionDeclaration(
            ZERO_LOC_ID("get_caller_char_no"),
            {cache.getUIntType(), ZERO_LOC},
            false,
            parent,
            ZERO_LOC,
            AccessSpecifier::Public,
            true
    ) {
        set_compiler_decl(true);
    }
    Value *call(InterpretScope *call_scope, ASTAllocator& allocator, FunctionCall *self_call, Value *parent_val, bool evaluate_refs) final {
        const auto global = call_scope->global;;
        const auto runtime_call = get_runtime_call(global);
        if(runtime_call) {
            const auto loc = global->loc_man.getLocation(runtime_call->encoded_location());
            return new (allocator.allocate<IntNumValue>()) IntNumValue(loc.charStart + 1, call_scope->global->typeBuilder.getU64Type(), ZERO_LOC);
        } else {
            return new (allocator.allocate<IntNumValue>()) IntNumValue(0, call_scope->global->typeBuilder.getU64Type(), ZERO_LOC);
        }
    }

};

class InterpretGetCallLoc : public FunctionDeclaration {
public:

    explicit InterpretGetCallLoc(TypeBuilder& cache, ASTNode* parent) : FunctionDeclaration(
            ZERO_LOC_ID("get_call_loc"),
            {cache.getUIntType(), ZERO_LOC},
            false,
            parent,
            ZERO_LOC,
            AccessSpecifier::Public,
            true
    ) {
        set_compiler_decl(true);
    }
    Value *call(InterpretScope *call_scope, ASTAllocator& allocator, FunctionCall *call, Value *parent_val, bool evaluate_refs) final {
        if(call->values.empty()) {
            call_scope->error("get_call_loc expects a single argument", call);
            return nullptr;
        }
        const auto argVal = call->values.back();
        const auto arg = argVal->evaluated_value(*call_scope);
        if(!arg || arg->kind() != ValueKind::IntN) {
            call_scope->error("get_call_loc expects a integer argument", arg);
            return nullptr;
        }
        const auto num = arg->get_number();
        const auto global = call_scope->global;
        const auto call_number = num.value();
        const auto stack_size = global->call_stack.size();
        if(stack_size == 0) {
            return new (allocator.allocate<IntNumValue>()) IntNumValue(call->encoded_location().encoded, call_scope->global->typeBuilder.getU64Type(), ZERO_LOC);
        }
        const auto final_call = call_number > stack_size ? global->call_stack.front() : global->call_stack[stack_size - 1 - call_number];
        return new (allocator.allocate<IntNumValue>()) IntNumValue(final_call->encoded_location().encoded, call_scope->global->typeBuilder.getU64Type(), ZERO_LOC);
    }

};

class InterpretDecodeLocation : public FunctionDeclaration {
public:

    explicit InterpretDecodeLocation(TypeBuilder& cache, ASTNode* parent) : FunctionDeclaration(
        ZERO_LOC_ID("decode_location"),
        {cache.getAnyType(), ZERO_LOC},
        false,
        parent,
        ZERO_LOC,
        AccessSpecifier::Public,
        true
    ) {
        set_compiler_decl(true);
    }
    Value *call(InterpretScope *call_scope, ASTAllocator& allocator, FunctionCall *call, Value *parent_val, bool evaluate_refs) final {
        if(call->values.empty()) {
            call_scope->error("decode_location expects a single argument", call);
            return nullptr;
        }

        const auto global = call_scope->global;
        auto& typeBuilder = global->typeBuilder;

        if(call->generic_list.size() != 1) {
            call_scope->error("decode_location requires a single generic argument, the struct definition representing the test", call);
            return new (allocator.allocate<NullValue>()) NullValue(typeBuilder.getNullPtrType(), ZERO_LOC);
        }

        const auto elem_type = call->generic_list[0];
        const auto test_def = elem_type->get_direct_linked_canonical_node();
        if(!test_def || test_def->kind() != ASTNodeKind::StructDecl) {
            call_scope->error("decode_location requires a single generic argument, the struct definition representing the test", call);
            return new (allocator.allocate<NullValue>()) NullValue(typeBuilder.getNullPtrType(), ZERO_LOC);
        }
        const auto decode_loc_def = test_def->as_struct_def_unsafe();

        const auto argVal = call->values.back();
        const auto arg = argVal->evaluated_value(*call_scope);
        if(!arg || arg->kind() != ValueKind::IntN) {
            call_scope->error("decode_location expects a u64 location argument", arg);
            return nullptr;
        }
        const auto num = arg->get_number();

        const auto decoded = global->loc_man.getLocation(num.value());
        const auto loc = call->encoded_location();
        const auto structValue = new (allocator.allocate<StructValue>()) StructValue(decode_loc_def->known_type(), decode_loc_def, decode_loc_def, loc);

        auto filePathView = global->loc_man.getPathForFileId(decoded.fileId);
        const auto filePathStr = new (allocator.allocate<StringValue>()) StringValue(chem::string_view(filePathView), typeBuilder.getStringType(), loc);
        structValue->values.emplace("filename", StructMemberInitializer { "filename", filePathStr });

        const auto lineNoVal = new (allocator.allocate<IntNumValue>()) IntNumValue(decoded.lineStart, typeBuilder.getUIntType(), loc);
        structValue->values.emplace("line", StructMemberInitializer { "line", lineNoVal });

        const auto charNoVal = new (allocator.allocate<IntNumValue>()) IntNumValue(decoded.charStart, typeBuilder.getUIntType(), loc);
        structValue->values.emplace("character", StructMemberInitializer { "character", charNoVal });

        return structValue;

    }

};

class InterpretDefined : public FunctionDeclaration {
public:

    FunctionParam valueParam;

    explicit InterpretDefined(TypeBuilder& cache) : FunctionDeclaration(
            ZERO_LOC_ID("defined"),
            {cache.getBoolType(), ZERO_LOC},
            false,
            nullptr,
            ZERO_LOC,
            AccessSpecifier::Public,
            true
    ), valueParam("value", { cache.getStringType(), ZERO_LOC }, 0, nullptr, false, this, ZERO_LOC) {
        set_compiler_decl(true);
        params.emplace_back(&valueParam);
    }
    Value *call(InterpretScope *call_scope, ASTAllocator& allocator, FunctionCall *call, Value *parent_val, bool evaluate_refs) final {
        if(call->values.empty()) return new (allocator.allocate<BoolValue>()) BoolValue(false, call_scope->global->typeBuilder.getBoolType(), ZERO_LOC);
        auto val = call->values[0]->evaluated_value(*call_scope);
        if(val->val_kind() != ValueKind::String) return new (allocator.allocate<BoolValue>()) BoolValue(false, call_scope->global->typeBuilder.getBoolType(), ZERO_LOC);
        if(!call_scope->global->build_compiler) {
            return new (allocator.allocate<BoolValue>()) BoolValue(false, call_scope->global->typeBuilder.getBoolType(), ZERO_LOC);
        }
        auto& definitions = call_scope->global->build_compiler->current_job->definitions;
        auto found = definitions.find(val->get_the_string().str());
        return new (allocator.allocate<BoolValue>()) BoolValue(found != definitions.end(), call_scope->global->typeBuilder.getBoolType(), ZERO_LOC);
    }
};

// function is used to error out at compile time
class InterpretError : public FunctionDeclaration {
public:

    FunctionParam valueParam;

    explicit InterpretError(TypeBuilder& cache, ASTNode* parent_node) : FunctionDeclaration(
            ZERO_LOC_ID("error"),
            {cache.getVoidType(), ZERO_LOC},
            false,
            parent_node,
            ZERO_LOC,
            AccessSpecifier::Public,
            true
    ), valueParam("value", { cache.getStringType(), ZERO_LOC }, 0, nullptr, false, this, ZERO_LOC) {
        set_compiler_decl(true);
        params.emplace_back(&valueParam);
    }
    Value *call(InterpretScope *call_scope, ASTAllocator& allocator, FunctionCall *call, Value *parent_val, bool evaluate_refs) final {
        if(call->values.empty()) return new (allocator.allocate<BoolValue>()) BoolValue(false, call_scope->global->typeBuilder.getBoolType(), ZERO_LOC);
        auto val = call->values[0]->evaluated_value(*call_scope);
        if(val->val_kind() != ValueKind::String) return new (allocator.allocate<BoolValue>()) BoolValue(false, call_scope->global->typeBuilder.getBoolType(), ZERO_LOC);
        call_scope->error(val->get_the_string().view(), call);
    }
};

class InterpretSatisfies : public FunctionDeclaration {
public:
    explicit InterpretSatisfies(TypeBuilder& cache, ASTNode* parent_node) : FunctionDeclaration(
            ZERO_LOC_ID("satisfies"),
            {cache.getBoolType(), ZERO_LOC},
            false,
            parent_node,
            ZERO_LOC,
            AccessSpecifier::Public,
            true
    ) {
        set_compiler_decl(true);
    }
    inline Value* get_bool(ASTAllocator& allocator, TypeBuilder& typeBuilder, bool value) {
        return new (allocator.allocate<BoolValue>()) BoolValue(value, typeBuilder.getBoolType(), ZERO_LOC);
    }
    Value *call(InterpretScope *call_scope, ASTAllocator& allocator, FunctionCall *call, Value *parent_val, bool evaluate_refs) final {
        auto& typeBuilder = call_scope->global->typeBuilder;
        if(call->generic_list.size() != 2) {
            call_scope->error("intrinsics::satisfies requires 2 generic arguments", call);
            return get_bool(allocator, typeBuilder, false);
        }
        const auto type1 = call->generic_list[0];
        const auto type2 = call->generic_list[1];
        return get_bool(allocator, typeBuilder, type1->satisfies(type2));
    }
};

class InterpretValueSatisfies : public FunctionDeclaration {
public:

    FunctionParam valueParam;
    FunctionParam valueParam2;

    explicit InterpretValueSatisfies(TypeBuilder& cache, ASTNode* parent_node) : FunctionDeclaration(
            ZERO_LOC_ID("value_satisfies"),
            {cache.getBoolType(), ZERO_LOC},
            false,
            parent_node,
            ZERO_LOC,
            AccessSpecifier::Public,
            true
    ),  valueParam("value", { cache.getAnyType(), ZERO_LOC }, 0, nullptr, false, this, ZERO_LOC),
        valueParam2("value2", { cache.getAnyType(), ZERO_LOC }, 1, nullptr, false, this, ZERO_LOC)
    {
        set_compiler_decl(true);
        params.emplace_back(&valueParam);
        params.emplace_back(&valueParam2);
    }
    inline Value* get_bool(ASTAllocator& allocator, TypeBuilder& typeBuilder, bool value) {
        return new (allocator.allocate<BoolValue>()) BoolValue(value, typeBuilder.getBoolType(), ZERO_LOC);
    }
    Value *call(InterpretScope *call_scope, ASTAllocator& allocator, FunctionCall *call, Value *parent_val, bool evaluate_refs) final {
        if(call->values.size() != 2) {
            call_scope->error("intrinsics::value_satisfies requires 2 arguments", call);
            return nullptr;
        }
        auto& typeBuilder = call_scope->global->typeBuilder;
        const auto val_one = call->values[0];
        const auto val_two = call->values[1];
        const auto first_type = val_one->getType();
        if(first_type) {
            return get_bool(allocator, typeBuilder, first_type->satisfies(val_two, false));
        } else {
            return get_bool(allocator, typeBuilder, false);
        }
    }
};

class InterpretIsPtrNull : public FunctionDeclaration {
public:

    PointerType ptrType;
    FunctionParam valueParam;

    NullValue nullVal;

    explicit InterpretIsPtrNull(TypeBuilder& cache, ASTNode* parent_node) : FunctionDeclaration(
            ZERO_LOC_ID("isNull"),
            {cache.getBoolType(), ZERO_LOC},
            false,
            parent_node,
            ZERO_LOC,
            AccessSpecifier::Public,
            true
    ), nullVal(cache.getNullPtrType(), ZERO_LOC), ptrType(cache.getAnyType(), ZERO_LOC),
                                                                            valueParam("value", { &ptrType, ZERO_LOC }, 0, nullptr, false, this, ZERO_LOC)
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

    PointerType ptrType;
    FunctionParam valueParam;

    NullValue nullVal;

    explicit InterpretIsPtrNotNull(TypeBuilder& cache, ASTNode* parent_node) : FunctionDeclaration(
            ZERO_LOC_ID("isNotNull"),
            {cache.getBoolType(), ZERO_LOC},
            false,
            parent_node,
            ZERO_LOC,
            AccessSpecifier::Public,
            true
    ), nullVal(cache.getNullPtrType(), ZERO_LOC), ptrType(cache.getAnyType(), ZERO_LOC),
                                                                               valueParam("value", { &ptrType, ZERO_LOC }, 0, nullptr, false, this, ZERO_LOC)
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

    FunctionParam destValueParam;
    FunctionParam sourceValueParam;

    explicit InterpretMemCopy(TypeBuilder& cache, ASTNode* parent_node) : FunctionDeclaration(
            ZERO_LOC_ID("copy"),
            {cache.getBoolType(), ZERO_LOC},
            false,
            parent_node,
            ZERO_LOC,
            AccessSpecifier::Public,
            true
    ), destValueParam("dest_value", { cache.getStringType(), ZERO_LOC }, 0, nullptr, false, this, ZERO_LOC),
        sourceValueParam("source_value", { cache.getStringType(), ZERO_LOC }, 1, nullptr, false, this, ZERO_LOC)
    {
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

    explicit InterpretGetTarget(TypeBuilder& cache, ASTNode* parent_node) : FunctionDeclaration(
            ZERO_LOC_ID("get_target"),
            {cache.getStringType(), ZERO_LOC},
            false,
            parent_node,
            ZERO_LOC,
            AccessSpecifier::Public,
            true
    ) {
        set_compiler_decl(true);
    }
    Value *call(InterpretScope *call_scope, ASTAllocator& allocator, FunctionCall *call, Value *parent_val, bool evaluate_refs) final {
        auto& global = *call_scope->global;
        auto& triple = global.build_compiler->current_job->target_triple;
        const auto triple_ptr = allocator.allocate_str(triple.data(), triple.size());
        return new (allocator.allocate<StringValue>()) StringValue(chem::string_view(triple_ptr, triple.size()), global.typeBuilder.getStringType(), call->encoded_location());
    }
};

class InterpretGetLocFilePath : public FunctionDeclaration {
public:

    explicit InterpretGetLocFilePath(TypeBuilder& cache, ASTNode* parent_node) : FunctionDeclaration(
            ZERO_LOC_ID("get_loc_file_path"),
            {cache.getStringType(), ZERO_LOC},
            false,
            parent_node,
            ZERO_LOC,
            AccessSpecifier::Public,
            true
    ) {
        set_compiler_decl(true);
    }
    Value *call(InterpretScope *call_scope, ASTAllocator& allocator, FunctionCall *call, Value *parent_val, bool evaluate_refs) final {
        if(call->values.size() != 1) {
            call_scope->error("get_loc_file_path expects a single argument", call);
            return nullptr;
        }
        const auto eval_value = call->values.back()->evaluated_value(*call_scope);
        if(eval_value == nullptr || eval_value->kind() != ValueKind::IntN) {
            call_scope->error("couldn't evaluate value in get_loc_file_path", call);
            return nullptr;
        }
        auto& loc_man = call_scope->global->loc_man;
        auto location = loc_man.getLocation(eval_value->as_int_num_value_unsafe()->value);
        auto fileId = loc_man.getPathForFileId(location.fileId);
        return new (allocator.allocate<StringValue>()) StringValue(chem::string_view(fileId.data(), fileId.size()), call_scope->global->typeBuilder.getStringType(), call->encoded_location());
    }
};

class InterpretGetBuildDir : public FunctionDeclaration {
public:

    explicit InterpretGetBuildDir(TypeBuilder& cache, ASTNode* parent_node) : FunctionDeclaration(
            ZERO_LOC_ID("get_build_dir"),
            {cache.getStringType(), ZERO_LOC},
            false,
            parent_node,
            ZERO_LOC,
            AccessSpecifier::Public,
            true
    ) {
        set_compiler_decl(true);
    }
    Value *call(InterpretScope *call_scope, ASTAllocator& allocator, FunctionCall *call, Value *parent_val, bool evaluate_refs) final {
        // TODO this should not use build compiler (not available in lsp)
        if(!call_scope->global->build_compiler) {
            return new (allocator.allocate<StringValue>()) StringValue("", call_scope->global->typeBuilder.getStringType(), call->encoded_location());
        }
        auto& fileId = call_scope->global->build_compiler->options->build_dir;
        return new (allocator.allocate<StringValue>()) StringValue(chem::string_view(fileId.data(), fileId.size()), call_scope->global->typeBuilder.getStringType(), call->encoded_location());
    }
};

class InterpretGetCurrentFilePath : public FunctionDeclaration {
public:

    explicit InterpretGetCurrentFilePath(TypeBuilder& cache, ASTNode* parent_node) : FunctionDeclaration(
            ZERO_LOC_ID("get_current_file_path"),
            {cache.getStringType(), ZERO_LOC},
            false,
            parent_node,
            ZERO_LOC,
            AccessSpecifier::Public,
            true
    ) {
        set_compiler_decl(true);
    }
    Value *call(InterpretScope *call_scope, ASTAllocator& allocator, FunctionCall *call, Value *parent_val, bool evaluate_refs) final {
        auto& loc_man = call_scope->global->loc_man;
        auto location = loc_man.getLocation(call->encoded_location());
        auto fileId = loc_man.getPathForFileId(location.fileId);
        return new (allocator.allocate<StringValue>()) StringValue(chem::string_view(fileId.data(), fileId.size()), call_scope->global->typeBuilder.getStringType(), call->encoded_location());
    }
};

class InterpretGetModuleScope : public FunctionDeclaration {
public:

    explicit InterpretGetModuleScope(TypeBuilder& cache, ASTNode* parent_node) : FunctionDeclaration(
            ZERO_LOC_ID("get_module_scope"),
            {cache.getStringType(), ZERO_LOC},
            false,
            parent_node,
            ZERO_LOC,
            AccessSpecifier::Public,
            true
    ) {
        set_compiler_decl(true);
    }
    Value *call(InterpretScope *call_scope, ASTAllocator& allocator, FunctionCall *call, Value *parent_val, bool evaluate_refs) final {
        auto caller_func = call_scope->global->current_func_type->get_parent();
        if(!caller_func) {
            return new (allocator.allocate<StringValue>()) StringValue("", call_scope->global->typeBuilder.getStringType(), call->encoded_location());
        }
        const auto scope = caller_func->get_mod_scope();
        if(!scope) {
            return new (allocator.allocate<StringValue>()) StringValue("", call_scope->global->typeBuilder.getStringType(), call->encoded_location());
        }
        return new (allocator.allocate<StringValue>()) StringValue(scope->scope_name, call_scope->global->typeBuilder.getStringType(), call->encoded_location());
    }
};

class InterpretGetModuleName : public FunctionDeclaration {
public:

    explicit InterpretGetModuleName(TypeBuilder& cache, ASTNode* parent_node) : FunctionDeclaration(
            ZERO_LOC_ID("get_module_name"),
            {cache.getStringType(), ZERO_LOC},
            false,
            parent_node,
            ZERO_LOC,
            AccessSpecifier::Public,
            true
    ) {
        set_compiler_decl(true);
    }
    Value *call(InterpretScope *call_scope, ASTAllocator& allocator, FunctionCall *call, Value *parent_val, bool evaluate_refs) final {
        auto caller_func = call_scope->global->current_func_type->get_parent();
        if(!caller_func) {
            return new (allocator.allocate<StringValue>()) StringValue("", call_scope->global->typeBuilder.getStringType(), call->encoded_location());
        }
        const auto scope = caller_func->get_mod_scope();
        if(!scope) {
            return new (allocator.allocate<StringValue>()) StringValue("", call_scope->global->typeBuilder.getStringType(), call->encoded_location());
        }
        return new (allocator.allocate<StringValue>()) StringValue(scope->module_name, call_scope->global->typeBuilder.getStringType(), call->encoded_location());
    }
};

class InterpretGetModuleDir : public FunctionDeclaration {
public:

    explicit InterpretGetModuleDir(TypeBuilder& cache, ASTNode* parent_node) : FunctionDeclaration(
            ZERO_LOC_ID("get_module_dir"),
            {cache.getStringType(), ZERO_LOC},
            false,
            parent_node,
            ZERO_LOC,
            AccessSpecifier::Public,
            true
    ) {
        set_compiler_decl(true);
    }
    Value *call(InterpretScope *call_scope, ASTAllocator& allocator, FunctionCall *call, Value *parent_val, bool evaluate_refs) final {
        auto caller_func = call_scope->global->current_func_type->get_parent();
        if(!caller_func) {
            return new (allocator.allocate<StringValue>()) StringValue("", call_scope->global->typeBuilder.getStringType(), call->encoded_location());
        }
        const auto scope = caller_func->get_mod_scope();
        if(!scope) {
            return new (allocator.allocate<StringValue>()) StringValue("", call_scope->global->typeBuilder.getStringType(), call->encoded_location());
        }
        const auto container = scope->container;
        if(container->type != LabModuleType::Directory || container->paths.empty()) {
            return new (allocator.allocate<StringValue>()) StringValue("", call_scope->global->typeBuilder.getStringType(), call->encoded_location());
        }
        return new (allocator.allocate<StringValue>()) StringValue(container->paths[0].to_chem_view(), call_scope->global->typeBuilder.getStringType(), call->encoded_location());
    }
};

// only direct child functions are supported at the moment
class InterpretGetChildFunction : public FunctionDeclaration {
public:

    FunctionParam nameParam;

    explicit InterpretGetChildFunction(TypeBuilder& cache, ASTNode* parent_node) : FunctionDeclaration(
            ZERO_LOC_ID("get_child_fn"),
            {cache.getAnyType(), ZERO_LOC},
            false,
            parent_node,
            ZERO_LOC,
            AccessSpecifier::Public,
            true
    ), nameParam("name", { cache.getStringType(), ZERO_LOC }, 1, nullptr, false, this, 0)
   {
        set_compiler_decl(true);
        params = { &nameParam };
    };

    Value *call(InterpretScope *call_scope, ASTAllocator& allocator, FunctionCall *call, Value *parent_val, bool evaluate_refs) final {
        if(call->values.size() != 1) {
            call_scope->error("get_child_fn expects a single argument", call);
            return new (allocator.allocate<NullValue>()) NullValue(call_scope->global->typeBuilder.getNullPtrType(), 0);
        }
        if(call->generic_list.size() == 0) {
            call_scope->error("get_child_fn expects a single generic argument", call);
            return new (allocator.allocate<NullValue>()) NullValue(call_scope->global->typeBuilder.getNullPtrType(), 0);
        }
        const auto nameVal = call->values[0]->evaluated_value(*call_scope);
        if(nameVal->val_kind() != ValueKind::String) {
            call_scope->error("expected second argument to get_child_fn to be a string", call);
            return new(allocator.allocate<NullValue>()) NullValue(call_scope->global->typeBuilder.getNullPtrType(), 0);
        }
        auto type = call->generic_list.front();
        const auto container = type->get_members_container();
        if(container) {
            const auto value = container->direct_child_function(nameVal->get_the_string());
            if(value) {
                const auto id = new(allocator.allocate<VariableIdentifier>()) VariableIdentifier(value->name_view(), value->known_type(), 0, true);
                id->linked = value;
                return new(allocator.allocate<AccessChain>()) AccessChain({id}, value->known_type(), 0);
            }
        }
        return new (allocator.allocate<NullValue>()) NullValue(call_scope->global->typeBuilder.getNullPtrType(), 0);
    }

};

class InterpretTypeToString : public FunctionDeclaration {
public:

    FunctionParam param;
    std::ostringstream ostring;
    RepresentationVisitor visitor;

    explicit InterpretTypeToString(TypeBuilder& cache, ASTNode* parent_node) : FunctionDeclaration(
            ZERO_LOC_ID("type_to_string"),
            {cache.getStringType(), ZERO_LOC},
            true,
            parent_node,
            ZERO_LOC,
            AccessSpecifier::Public,
            true
    ), param("value", { cache.getAnyType(), ZERO_LOC }, 0, nullptr, false, this, 0), visitor(ostring)
    {
        visitor.interpret_representation = true;
        set_compiler_decl(true);
        params = { &param };
    }

    Value *call(InterpretScope *call_scope, ASTAllocator& allocator, FunctionCall *call, Value *parent_val, bool evaluate_refs) override {
        if(call->generic_list.empty()) {
            call_scope->error("type_to_string expects a single type parameter", call);
            return nullptr;
        }
        const auto childType = call->generic_list.front()->pure_type(allocator);
        visitor.visit(childType);
        const auto& str = ostring.str();
        const auto returnValue = new (allocator.allocate<StringValue>()) StringValue(chem::string_view(allocator.allocate_str(str.data(), str.size()), str.size()), call_scope->global->typeBuilder.getStringType(), call->encoded_location());
        ostring.str("");
        ostring.clear();
        return returnValue;
    }
};

class InterpretForget : public FunctionDeclaration {
public:

    FunctionParam param;

    explicit InterpretForget(TypeBuilder& cache, ASTNode* parent_node) : FunctionDeclaration(
            ZERO_LOC_ID("forget"),
            {cache.getVoidType(), ZERO_LOC},
            true,
            parent_node,
            ZERO_LOC,
            AccessSpecifier::Public,
            true
    ), param("thing", { cache.getAnyType(), ZERO_LOC }, 0, nullptr, false, this, ZERO_LOC) {
        params = { &param };
        set_compiler_decl(true);
    }

    Value *call(InterpretScope *call_scope, ASTAllocator& allocator, FunctionCall *call, Value *parent_val, bool evaluate_refs) override {
        if(call->values.empty()) {
            call_scope->error("call requires a single argument", call);
            return new (allocator.allocate<BoolValue>()) BoolValue(false, call_scope->global->typeBuilder.getBoolType(), ZERO_LOC);
        }
        const auto arg = call->values.front();
        const auto node = arg->linked_node();
        const auto result = node != nullptr && call_scope->global->backend_context->forget(node);
        return new (allocator.allocate<BoolValue>()) BoolValue(result, call_scope->global->typeBuilder.getBoolType(), ZERO_LOC);
    }
};

class InterpretGetSingleMarkedDeclPointer : public FunctionDeclaration {
public:

    explicit InterpretGetSingleMarkedDeclPointer(TypeBuilder& cache, ASTNode* parent_node) : FunctionDeclaration(
        ZERO_LOC_ID("get_single_marked_decl_ptr"),
        {cache.getAnyType(), ZERO_LOC},
        false,
        parent_node,
        ZERO_LOC,
        AccessSpecifier::Public,
        true
    ) {
        set_compiler_decl(true);
    }

    Value *call(InterpretScope *call_scope, ASTAllocator& allocator, FunctionCall *call, Value *parent_val, bool evaluate_refs) override {

        auto& global = *call_scope->global;
        auto& typeBuilder = global.typeBuilder;

        auto& controller = global.build_compiler->controller;
        const auto evalValue = call->values.back()->evaluated_value(*call_scope);
        if(evalValue->kind() != ValueKind::String) {
            return new (allocator.allocate<NullValue>()) NullValue(typeBuilder.getNullPtrType(), call->encoded_location());
        }

        const auto marked = controller.get_single_marked(evalValue->as_string_unsafe()->value);
        if(!marked) {
            return new (allocator.allocate<NullValue>()) NullValue(typeBuilder.getNullPtrType(), call->encoded_location());
        }

        if(marked->node->kind() != ASTNodeKind::FunctionDecl) {
            return new (allocator.allocate<NullValue>()) NullValue(typeBuilder.getNullPtrType(), call->encoded_location());
        }

        const auto decl = marked->node->as_function_unsafe();
        const auto ptr = new (allocator.allocate<VariableIdentifier>()) VariableIdentifier(decl->name_view(), decl->known_type(), call->encoded_location());
        ptr->linked = decl;

        return ptr;

    }

};

class InterpretGetTests : public FunctionDeclaration {
public:

    // struct {
    //  id : int,
    //  name : *char,
    //  group : *char,
    //  ptr : (env : &mut TestEnv) => void,
    //  file : *char,
    //  timeout : uint,
    //  retry : uint,
    //  benchmark : bool,
    //  lineNum : uint,
    //  charNum : uint
    // }

    explicit InterpretGetTests(TypeBuilder& cache, ASTNode* parent_node) : FunctionDeclaration(
            ZERO_LOC_ID("get_tests"),
            {cache.getAnyType(), ZERO_LOC},
            false,
            parent_node,
            ZERO_LOC,
            AccessSpecifier::Public,
            true
    ) {
        set_compiler_decl(true);
    }

    Value *call(InterpretScope *call_scope, ASTAllocator& allocator, FunctionCall *call, Value *parent_val, bool evaluate_refs) override {

        auto& global = *call_scope->global;
        auto& typeBuilder = global.typeBuilder;

        // validation
        if(call->generic_list.size() != 1) {
            call_scope->error("get_tests requires a single generic argument, the struct definition representing the test", call);
            return new (allocator.allocate<NullValue>()) NullValue(global.typeBuilder.getNullPtrType(), ZERO_LOC);
        }

        const auto elem_type = call->generic_list[0];
        const auto test_def = elem_type->get_direct_linked_canonical_node();
        if(!test_def || test_def->kind() != ASTNodeKind::StructDecl) {
            call_scope->error("get_tests requires a single generic argument, the struct definition representing the test", call);
            return new (allocator.allocate<NullValue>()) NullValue(global.typeBuilder.getNullPtrType(), ZERO_LOC);
        }
        const auto test_def_struct = test_def->as_struct_def_unsafe();

        // getting the tests collection
        auto& controller = global.build_compiler->controller;
        const auto annot_def = controller.get_definition("test");
        auto& collection = controller.get_collection(annot_def->collection_id);

        // creating the return
        const auto arrType = new (allocator.allocate<ArrayType>()) ArrayType(elem_type, collection.nodes.size());
        const auto arrVal = new (allocator.allocate<ArrayValue>()) ArrayValue(call->encoded_location(), arrType);

        const auto emptyStringVal = new (allocator.allocate<StringValue>()) StringValue("", typeBuilder.getStringType(), call->encoded_location());

        // putting tests inside the list
        int i = INT_MAX / 2;
        for(auto& node : collection.nodes) {
            if(node.node->kind() == ASTNodeKind::FunctionDecl) {

                const auto decl = node.node->as_function_unsafe();

                // creating the struct value for given test
                const auto value = new (allocator.allocate<StructValue>()) StructValue(elem_type, test_def_struct, test_def_struct, call->encoded_location());
                arrVal->values.emplace_back(value);

                // id
                const auto idArgs = controller.get_args(node.node, "test.id");
                auto testId = i;
                if(idArgs && !idArgs->empty()) {
                    const auto arg = (*idArgs)[0];
                    auto num = arg->get_number();
                    if(num.has_value()) {
                        const auto userId = (int) num.value();
                        if(userId > 0 && userId < INT_MAX / 2) {
                            testId = userId;
                        }
                    }
                }
                const auto idVal = new (allocator.allocate<IntNumValue>()) IntNumValue(testId, typeBuilder.getIntType(), call->encoded_location());
                value->values.emplace("id", StructMemberInitializer{"id", idVal});

                // name
                auto name_view = decl->name_view();
                const auto nameArgs = controller.get_args(node.node, "test.name");
                if(nameArgs && !nameArgs->empty() && (*nameArgs)[0]->kind() == ValueKind::String) {
                    name_view = (*nameArgs)[0]->get_the_string();
                }
                const auto nameVal = new (allocator.allocate<StringValue>()) StringValue(name_view, typeBuilder.getStringType(), call->encoded_location());
                value->values.emplace("name", StructMemberInitializer{"name", nameVal});

                // group
                const auto groupArgs = controller.get_args(node.node, "test.group");
                auto groupVal = emptyStringVal;
                if(groupArgs && !groupArgs->empty() && (*groupArgs)[0]->kind() == ValueKind::String) {
                    groupVal = new (allocator.allocate<StringValue>()) StringValue((*groupArgs)[0]->get_the_string(), typeBuilder.getStringType(), call->encoded_location());
                }
                value->values.emplace("group", StructMemberInitializer{"group", groupVal});

                // ptr
                const auto ptr = new (allocator.allocate<VariableIdentifier>()) VariableIdentifier(decl->name_view(), decl->known_type(), call->encoded_location());
                ptr->linked = decl;
                value->values.emplace("ptr", StructMemberInitializer{"ptr", ptr});

                // file
                const auto fileScope = decl->get_file_scope();
                auto fileVal = emptyStringVal;
                if(fileScope) {
                    fileVal = new (allocator.allocate<StringValue>()) StringValue(fileScope->file_path, typeBuilder.getStringType(), call->encoded_location());
                }
                value->values.emplace("file", StructMemberInitializer{"file", fileVal});

                // timeout
                const auto timeArgs = controller.get_args(node.node, "test.timeout");
                unsigned timeout = 1000 * 60 * 60 * 1; // 1 hour
                if(timeArgs && !timeArgs->empty() && (*timeArgs)[0]->kind() == ValueKind::IntN) {
                    timeout = (*timeArgs)[0]->as_int_num_value_unsafe()->value;
                }
                const auto timeoutVal = new (allocator.allocate<IntNumValue>()) IntNumValue(timeout, typeBuilder.getUIntType(), call->encoded_location());
                value->values.emplace("timeout", StructMemberInitializer{"timeout", timeoutVal});

                // retry
                const auto retryArgs = controller.get_args(node.node, "test.retry");
                // default retries is 0
                int retries = 0;
                if(retryArgs && !retryArgs->empty()) {
                    const auto val = (*retryArgs)[0];
                    const auto num = val->get_number();
                    if(num.has_value()) {
                        retries = (int) num.value();
                        if(retries < 0) {
                            retries = 0;
                        } else if(retries > 999999) {
                            retries = 999999;
                        }
                    }
                }
                const auto retryVal = new (allocator.allocate<IntNumValue>()) IntNumValue(retries, typeBuilder.getUIntType(), call->encoded_location());
                value->values.emplace("retry", StructMemberInitializer{"retry", retryVal});

                // pass on crash
                const auto pass_on_crash = controller.is_marked(node.node, "test.pass_on_crash");
                const auto crashVal = new (allocator.allocate<BoolValue>()) BoolValue(pass_on_crash, typeBuilder.getBoolType(), call->encoded_location());
                value->values.emplace("pass_on_crash", StructMemberInitializer{"pass_on_crash", crashVal});

                // benchmark
                const auto marked_bench = controller.is_marked(node.node, "test.benchmark");
                const auto benchVal = new (allocator.allocate<BoolValue>()) BoolValue(marked_bench, typeBuilder.getBoolType(), call->encoded_location());
                value->values.emplace("benchmark", StructMemberInitializer{"benchmark", benchVal});

                // line number + char number
                const auto locData = global.loc_man.getLocation(decl->encoded_location());
                const auto lineNumValue = new (allocator.allocate<IntNumValue>()) IntNumValue(locData.lineStart, typeBuilder.getUIntType(), call->encoded_location());
                value->values.emplace("lineNum", StructMemberInitializer{"lineNum", lineNumValue});
                const auto charNumValue = new (allocator.allocate<IntNumValue>()) IntNumValue(locData.charStart, typeBuilder.getUIntType(), call->encoded_location());
                value->values.emplace("charNum", StructMemberInitializer{"charNum", charNumValue});

                i++;
            }
        }

        return arrVal;
    }

};

class InterpretGetLambdaFnPtr : public FunctionDeclaration {
public:

    FunctionParam param;

    explicit InterpretGetLambdaFnPtr(
        TypeBuilder& cache,
        ASTNode* parent_node
    ) : FunctionDeclaration(
            ZERO_LOC_ID("get_lambda_fn_ptr"),
            {cache.getPtrToVoid(), ZERO_LOC},
            true,
            parent_node,
            ZERO_LOC,
            AccessSpecifier::Public,
            true
    ), param("thing", { cache.getAnyType(), ZERO_LOC }, 0, nullptr, false, this, ZERO_LOC) {
        params = { &param };
        set_compiler_decl(true);
    }

    Value *call(InterpretScope *call_scope, ASTAllocator& allocator, FunctionCall *call, Value *parent_val, bool evaluate_refs) override {
        if(call->values.empty()) {
            call_scope->error("call requires a single argument", call);
            return new (allocator.allocate<NullValue>()) NullValue(call_scope->global->typeBuilder.getNullPtrType(), ZERO_LOC);
        }
        const auto arg = call->values.front()->evaluated_value(*call_scope);
        const auto extractionKind = ExtractionKind::LambdaFnPtr;
        return new (allocator.allocate<ExtractionValue>()) ExtractionValue(
                arg, create_extraction_value_type(call_scope->global->typeBuilder, extractionKind), extractionKind, call->encoded_location()
        );
    }
};

class InterpretGetLambdaCapPtr : public FunctionDeclaration {
public:

    FunctionParam param;

    explicit InterpretGetLambdaCapPtr(
            TypeBuilder& cache,
            ASTNode* parent_node
    ) : FunctionDeclaration(
            ZERO_LOC_ID("get_lambda_cap_ptr"),
            {cache.getPtrToVoid(), ZERO_LOC},
            true,
            parent_node,
            ZERO_LOC,
            AccessSpecifier::Public,
            true
    ), param("thing", { cache.getAnyType(), ZERO_LOC }, 0, nullptr, false, this, ZERO_LOC) {
        params = { &param };
        set_compiler_decl(true);
    }

    Value *call(InterpretScope *call_scope, ASTAllocator& allocator, FunctionCall *call, Value *parent_val, bool evaluate_refs) override {
        if(call->values.empty()) {
            call_scope->error("call requires a single argument", call);
            return new (allocator.allocate<NullValue>()) NullValue(call_scope->global->typeBuilder.getNullPtrType(), ZERO_LOC);
        }
        const auto arg = call->values.front()->evaluated_value(*call_scope);
        const auto extractionKind = ExtractionKind::LambdaCapturedPtr;
        return new (allocator.allocate<ExtractionValue>()) ExtractionValue(
                arg, create_extraction_value_type(call_scope->global->typeBuilder, extractionKind), extractionKind, call->encoded_location()
        );
    }
};


class InterpretGetLambdaCapDestructor : public FunctionDeclaration {
public:

    FunctionParam param;

    explicit InterpretGetLambdaCapDestructor(
            TypeBuilder& cache,
            ASTNode* parent_node
    ) : FunctionDeclaration(
            ZERO_LOC_ID("get_lambda_cap_destructor"),
            {cache.getPtrToVoid(), ZERO_LOC},
            true,
            parent_node,
            ZERO_LOC,
            AccessSpecifier::Public,
            true
    ), param("thing", { cache.getAnyType(), ZERO_LOC }, 0, nullptr, false, this, ZERO_LOC) {
        params = { &param };
        set_compiler_decl(true);
    }

    Value *call(InterpretScope *call_scope, ASTAllocator& allocator, FunctionCall *call, Value *parent_val, bool evaluate_refs) override {
        if(call->values.empty()) {
            call_scope->error("call requires a single argument", call);
            return new (allocator.allocate<NullValue>()) NullValue(call_scope->global->typeBuilder.getNullPtrType(), ZERO_LOC);
        }
        const auto arg = call->values.front()->evaluated_value(*call_scope);
        const auto extractionKind = ExtractionKind::LambdaCapturedDestructor;
        return new (allocator.allocate<ExtractionValue>()) ExtractionValue(
                arg, create_extraction_value_type(call_scope->global->typeBuilder, extractionKind), extractionKind, call->encoded_location()
        );
    }
};

class InterpretSizeOfLambdaCaptured : public FunctionDeclaration {
public:

    FunctionParam param;

    explicit InterpretSizeOfLambdaCaptured(
            TypeBuilder& cache,
            ASTNode* parent_node
    ) : FunctionDeclaration(
            ZERO_LOC_ID("sizeof_lambda_captured"),
            {cache.getU64Type(), ZERO_LOC},
            true,
            parent_node,
            ZERO_LOC,
            AccessSpecifier::Public,
            true
    ), param("thing", { cache.getAnyType(), ZERO_LOC }, 0, nullptr, false, this, ZERO_LOC) {
        params = { &param };
        set_compiler_decl(true);
    }

    Value *call(InterpretScope *call_scope, ASTAllocator& allocator, FunctionCall *call, Value *parent_val, bool evaluate_refs) override {
        if(call->values.empty()) {
            call_scope->error("call requires a single argument", call);
            return new (allocator.allocate<NullValue>()) NullValue(call_scope->global->typeBuilder.getNullPtrType(), ZERO_LOC);
        }
        const auto arg = call->values.front()->evaluated_value(*call_scope);
        const auto extractionKind = ExtractionKind::SizeOfLambdaCaptured;
        return new (allocator.allocate<ExtractionValue>()) ExtractionValue(
                arg, create_extraction_value_type(call_scope->global->typeBuilder, extractionKind), extractionKind, call->encoded_location()
        );
    }
};


class InterpretAlignOfLambdaCaptured : public FunctionDeclaration {
public:

    FunctionParam param;

    explicit InterpretAlignOfLambdaCaptured(
            TypeBuilder& cache,
            ASTNode* parent_node
    ) : FunctionDeclaration(
            ZERO_LOC_ID("alignof_lambda_captured"),
            {cache.getU64Type(), ZERO_LOC},
            false,
            parent_node,
            ZERO_LOC,
            AccessSpecifier::Public,
            true
    ), param("thing", { cache.getAnyType(), ZERO_LOC }, 0, nullptr, false, this, ZERO_LOC) {
        params = { &param };
        set_compiler_decl(true);
    }

    Value *call(InterpretScope *call_scope, ASTAllocator& allocator, FunctionCall *call, Value *parent_val, bool evaluate_refs) override {
        if(call->values.empty()) {
            call_scope->error("call requires a single argument", call);
            return new (allocator.allocate<NullValue>()) NullValue(call_scope->global->typeBuilder.getNullPtrType(), ZERO_LOC);
        }
        const auto arg = call->values.front()->evaluated_value(*call_scope);
        const auto extractionKind = ExtractionKind::AlignOfLambdaCaptured;
        return new (allocator.allocate<ExtractionValue>()) ExtractionValue(
                arg, create_extraction_value_type(call_scope->global->typeBuilder, extractionKind), extractionKind, call->encoded_location()
        );
    }
};

class InterpretDestructCallSite : public FunctionDeclaration {
public:

    explicit InterpretDestructCallSite(
            TypeBuilder& cache,
            ASTNode* parent_node
    ) : FunctionDeclaration(
            ZERO_LOC_ID("destruct_call_site"),
            {cache.getVoidType(), ZERO_LOC},
            false,
            parent_node,
            ZERO_LOC,
            AccessSpecifier::Public,
            true
    ) {
        set_compiler_decl(true);
    }

    Value *call(InterpretScope *call_scope, ASTAllocator& allocator, FunctionCall *call, Value *parent_val, bool evaluate_refs) override {
        call_scope->global->backend_context->destruct_call_site(call->encoded_location());
        return nullptr;
    }

};

class InterpretGetBackendName : public FunctionDeclaration {
public:

    explicit InterpretGetBackendName(
            TypeBuilder& cache,
            ASTNode* parent_node
    ) : FunctionDeclaration(
            ZERO_LOC_ID("get_backend_name"),
            {cache.getStringType(), ZERO_LOC},
            false,
            parent_node,
            ZERO_LOC,
            AccessSpecifier::Public,
            true
    ) {
        set_compiler_decl(true);
    }

    Value *call(InterpretScope *call_scope, ASTAllocator& allocator, FunctionCall *call, Value *parent_val, bool evaluate_refs) override {
        const auto nameView = call_scope->global->backend_context->name();
        auto& typeBuilder = call_scope->global->typeBuilder;
        return new (allocator.allocate<StringValue>()) StringValue(
            chem::string_view(nameView), typeBuilder.getStringType(), call->encoded_location()
        );
    }

};

class InterpretEmitRaw : public FunctionDeclaration {
public:

    FunctionParam param;

    explicit InterpretEmitRaw(
        TypeBuilder& cache,
        ASTNode* parent_node
    ) : FunctionDeclaration(
        ZERO_LOC_ID("emit"),
        {cache.getBoolType(), ZERO_LOC},
        false,
        parent_node,
        ZERO_LOC,
        AccessSpecifier::Public,
        true
    ), param("value", { cache.getStringType(), ZERO_LOC }, 0, nullptr, false, this, ZERO_LOC) {
        params = { &param };
        set_compiler_decl(true);
    }

    Value *call(InterpretScope *call_scope, ASTAllocator& allocator, FunctionCall *call, Value *parent_val, bool evaluate_refs) override {
        auto& typeBuilder = call_scope->global->typeBuilder;
        if(call->values.empty()) {
            return new (allocator.allocate<BoolValue>()) BoolValue(false, typeBuilder.getBoolType(), call->encoded_location());
        }
        const auto value = call->values.front()->evaluated_value(*call_scope);
        if(!value || value->kind() != ValueKind::String) {
            return new (allocator.allocate<BoolValue>()) BoolValue(false, typeBuilder.getBoolType(), call->encoded_location());
        }
        call_scope->global->backend_context->emit(value->as_string_unsafe()->value);
        return new (allocator.allocate<BoolValue>()) BoolValue(true, typeBuilder.getBoolType(), call->encoded_location());
    }

};

class InterpretRawLiteral : public FunctionDeclaration {
public:

    FunctionParam param;

    explicit InterpretRawLiteral(
            TypeBuilder& cache,
            ASTNode* parent_node
    ) : FunctionDeclaration(
            ZERO_LOC_ID("raw"),
            {cache.getAnyType(), ZERO_LOC},
            false,
            parent_node,
            ZERO_LOC,
            AccessSpecifier::Public,
            true
    ), param("value", { cache.getStringType(), ZERO_LOC }, 0, nullptr, false, this, ZERO_LOC) {
        params = { &param };
        set_compiler_decl(true);
    }

    Value *call(InterpretScope *call_scope, ASTAllocator& allocator, FunctionCall *call, Value *parent_val, bool evaluate_refs) override {
        auto& typeBuilder = call_scope->global->typeBuilder;
        if(call->values.empty()) {
            return new (allocator.allocate<BoolValue>()) BoolValue(false, typeBuilder.getBoolType(), call->encoded_location());
        }
        const auto value = call->values.front()->evaluated_value(*call_scope);
        if(!value || value->kind() != ValueKind::String) {
            return new (allocator.allocate<BoolValue>()) BoolValue(false, typeBuilder.getBoolType(), call->encoded_location());
        }
        return new (allocator.allocate<RawLiteral>()) RawLiteral(value->as_string_unsafe()->value, typeBuilder.getAnyType(), call->encoded_location());
    }

};

class InterpretMultiple : public FunctionDeclaration {
public:

    FunctionParam param;

    explicit InterpretMultiple(
            TypeBuilder& cache,
            ASTNode* parent_node
    ) : FunctionDeclaration(
            ZERO_LOC_ID("multiple"),
            {cache.getAnyType(), ZERO_LOC},
            false,
            parent_node,
            ZERO_LOC,
            AccessSpecifier::Public,
            true
    ), param("value", { cache.getAnyType(), ZERO_LOC }, 0, nullptr, false, this, ZERO_LOC) {
        params = { &param };
        set_compiler_decl(true);
        setIsVariadic(true);
    }

    Value *call(InterpretScope *call_scope, ASTAllocator& allocator, FunctionCall *call, Value *parent_val, bool evaluate_refs) override {
        auto& typeBuilder = call_scope->global->typeBuilder;
        if(call->values.empty()) {
            return new (allocator.allocate<BoolValue>()) BoolValue(false, typeBuilder.getBoolType(), call->encoded_location());
        }
        const auto multi = new (allocator.allocate<MultipleValue>()) MultipleValue(call->values.back()->getType(), call->encoded_location());
        for(const auto val : call->values) {
            multi->values.emplace_back(val->evaluated_value(*call_scope));
        }
        return multi;
    }

};

class InterpretGetLibsDir : public FunctionDeclaration {
public:

    explicit InterpretGetLibsDir(
            TypeBuilder& cache,
            ASTNode* parent_node
    ) : FunctionDeclaration(
            ZERO_LOC_ID("get_libs_dir"),
            {cache.getStringType(), ZERO_LOC},
            false,
            parent_node,
            ZERO_LOC,
            AccessSpecifier::Public,
            true
    ) {
        set_compiler_decl(true);
    }

    Value *call(InterpretScope *call_scope, ASTAllocator& allocator, FunctionCall *call, Value *parent_val, bool evaluate_refs) override {
        auto& typeBuilder = call_scope->global->typeBuilder;
#ifdef DEBUG
        auto source_dir = PROJECT_SOURCE_DIR;
        auto libs_dir = std::string(source_dir) + "/lang/libs";
#else
        auto libs_dir = resolve_sibling(call_scope->global->build_compiler->options->exe_path, "libs");
#endif
        const auto libs_dir_ptr = allocator.allocate_str(libs_dir.data(), libs_dir.size());
        return new (allocator.allocate<StringValue>()) StringValue(chem::string_view(libs_dir_ptr, libs_dir.size()), typeBuilder.getStringType(), call->encoded_location());
    }

};

class ChildFnCache {
private:
    ASTNode* parent;
    ASTNode* writeStrNoLen = nullptr;
    ASTNode* writeStr = nullptr;

    // chemical types
    ASTNode* writeI8 = nullptr;
    ASTNode* writeI16 = nullptr;
    ASTNode* writeI32 = nullptr;
    ASTNode* writeI64 = nullptr;
    ASTNode* writeU8 = nullptr;
    ASTNode* writeU16 = nullptr;
    ASTNode* writeU32 = nullptr;
    ASTNode* writeU64 = nullptr;

    // c like types
    ASTNode* writeChar = nullptr;
    ASTNode* writeUChar = nullptr;
    ASTNode* writeShort = nullptr;
    ASTNode* writeUShort = nullptr;
    ASTNode* writeInt = nullptr;
    ASTNode* writeUInt = nullptr;
    ASTNode* writeLong = nullptr;
    ASTNode* writeULong = nullptr;
    ASTNode* writeLongLong = nullptr;
    ASTNode* writeULongLong = nullptr;
    ASTNode* writeFloat = nullptr;
    ASTNode* writeDouble = nullptr;
public:
    ChildFnCache(ASTNode* node) : parent(node) {

    }
    ASTNode* getWriteStrNoLen() {
        if(writeStrNoLen == nullptr) {
            writeStrNoLen = parent->child("writeStrNoLen");
        }
        return writeStrNoLen;
    }
    ASTNode* getWriteStr() {
        if(writeStr == nullptr) {
            writeStr = parent->child("writeStr");
        }
        return writeStr;
    }
    ASTNode* getWriteI8() {
        if(writeI8 == nullptr) {
            writeI8 = parent->child("writeI8");
        }
        return writeI8;
    }
    ASTNode* getWriteI16() {
        if(writeI16 == nullptr) {
            writeI16 = parent->child("writeI16");
        }
        return writeI16;
    }
    ASTNode* getWriteI32() {
        if(writeI32 == nullptr) {
            writeI32 = parent->child("writeI32");
        }
        return writeI32;
    }
    ASTNode* getWriteI64() {
        if(writeI64 == nullptr) {
            writeI64 = parent->child("writeI64");
        }
        return writeI64;
    }
    ASTNode* getWriteU8() {
        if(writeU8 == nullptr) {
            writeU8 = parent->child("writeU8");
        }
        return writeU8;
    }
    ASTNode* getWriteU16() {
        if(writeU16 == nullptr) {
            writeU16 = parent->child("writeU16");
        }
        return writeU16;
    }
    ASTNode* getWriteU32() {
        if(writeU32 == nullptr) {
            writeU32 = parent->child("writeU32");
        }
        return writeU32;
    }
    ASTNode* getWriteU64() {
        if(writeU64 == nullptr) {
            writeU64 = parent->child("writeU64");
        }
        return writeU64;
    }
    ASTNode* getWriteChar() {
        if(writeChar == nullptr) {
            writeChar = parent->child("writeChar");
        }
        return writeChar;
    }
    ASTNode* getWriteUChar() {
        if(writeUChar == nullptr) {
            writeUChar = parent->child("writeUChar");
        }
        return writeUChar;
    }
    ASTNode* getWriteShort() {
        if(writeShort == nullptr) {
            writeShort = parent->child("writeShort");
        }
        return writeShort;
    }
    ASTNode* getWriteUShort() {
        if(writeUShort == nullptr) {
            writeUShort = parent->child("writeUShort");
        }
        return writeUShort;
    }
    ASTNode* getWriteInt() {
        if(writeInt == nullptr) {
            writeInt = parent->child("writeInt");
        }
        return writeInt;
    }
    ASTNode* getWriteUInt() {
        if(writeUInt == nullptr) {
            writeUInt = parent->child("writeUInt");
        }
        return writeUInt;
    }
    ASTNode* getWriteLong() {
        if(writeLong == nullptr) {
            writeLong = parent->child("writeLong");
        }
        return writeLong;
    }
    ASTNode* getWriteULong() {
        if(writeULong == nullptr) {
            writeULong = parent->child("writeULong");
        }
        return writeULong;
    }
    ASTNode* getWriteLongLong() {
        if(writeLongLong == nullptr) {
            writeLongLong = parent->child("writeLongLong");
        }
        return writeLongLong;
    }
    ASTNode* getWriteULongLong() {
        if(writeULongLong == nullptr) {
            writeULongLong = parent->child("writeULongLong");
        }
        return writeULongLong;
    }
    ASTNode* getWriteFloat() {
        if(writeFloat == nullptr) {
            writeFloat = parent->child("writeFloat");
        }
        return writeFloat;
    }
    ASTNode* getWriteDouble() {
        if(writeDouble == nullptr) {
            writeDouble = parent->child("writeDouble");
        }
        return writeDouble;
    }
};

class InterpretExprStrBlockValue : public FunctionDeclaration {
public:

    explicit InterpretExprStrBlockValue(
            TypeBuilder& cache,
            ASTNode* parent_node
    ) : FunctionDeclaration(
            ZERO_LOC_ID("expr_str_block_value"),
            {cache.getI32Type(), ZERO_LOC},
            false,
            parent_node,
            ZERO_LOC,
            AccessSpecifier::Public,
            true
    ) {
        set_compiler_decl(true);
    }

    ASTNode* write_obj_call(
            TypeBuilder& typeBuilder,
            ASTAllocator& allocator,
            VariableIdentifier* selfId,
            Value* argument,
            ASTNode* function_node,
            chem::string_view function_name,
            ASTNode* parent_node,
            SourceLocation loc
    ) {
        const auto child_type = function_node->known_type();
        const auto chain = new (allocator.allocate<AccessChain>()) AccessChain(child_type, loc);
        const auto write_call = new (allocator.allocate<FunctionCall>()) FunctionCall(chain, typeBuilder.getVoidType(), loc);
        write_call->values.emplace_back(selfId);
        // TODO: unsafe cast to chain value
        chain->values.emplace_back((ChainValue*) argument);
        const auto childId = new (allocator.allocate<VariableIdentifier>()) VariableIdentifier(
                function_name, child_type, loc
        );
        childId->linked = function_node;
        childId->setType(child_type);
        chain->values.emplace_back(childId);
        const auto wrapper = new (allocator.allocate<AccessChainNode>()) AccessChainNode(loc, parent_node);
        wrapper->chain.setType(typeBuilder.getVoidType());
        wrapper->chain.values.emplace_back(write_call);
        return wrapper;
    }

    ASTNode* write_call(
        TypeBuilder& typeBuilder,
        ASTAllocator& allocator,
        VariableIdentifier* selfId,
        Value* argument,
        ASTNode* function_node,
        chem::string_view function_name,
        ASTNode* parent_node,
        SourceLocation loc,
        Value* second_arg = nullptr
    ) {
        const auto child_type = function_node->known_type();
        const auto chain = new (allocator.allocate<AccessChain>()) AccessChain(child_type, loc);
        const auto write_call = new (allocator.allocate<FunctionCall>()) FunctionCall(chain, typeBuilder.getVoidType(), loc);
        write_call->values.emplace_back(argument);
        if(second_arg) {
            write_call->values.emplace_back(second_arg);
        }
        chain->values.emplace_back(selfId);
        const auto childId = new (allocator.allocate<VariableIdentifier>()) VariableIdentifier(
                function_name, child_type, loc
        );
        childId->linked = function_node;
        childId->setType(child_type);
        chain->values.emplace_back(childId);
        const auto wrapper = new (allocator.allocate<AccessChainNode>()) AccessChainNode(loc, parent_node);
        wrapper->chain.setType(typeBuilder.getVoidType());
        wrapper->chain.values.emplace_back(write_call);
        return wrapper;
    }

    ASTNode* process_value(
            ChildFnCache& cache,
            TypeBuilder& typeBuilder,
            Value* val,
            VariableIdentifier* selfId,
            ASTAllocator& allocator,
            ASTNode* parent_node,
            SourceLocation loc
    ) {
        const auto t = val->getType()->canonical();
        if(t->isStringType()) {
            if(val->kind() == ValueKind::String) {
                const auto str = val->as_string_unsafe();
                const auto sizeVal = new (allocator.allocate<IntNumValue>()) IntNumValue(str->length, typeBuilder.getU64Type(), loc);
                return write_call(typeBuilder, allocator, selfId, val, cache.getWriteStr(), "writeStr", parent_node, loc, sizeVal);
            } else {
                return write_call(typeBuilder, allocator, selfId, val, cache.getWriteStrNoLen(), "writeStrNoLen", parent_node, loc);
            }
        } else {
            switch(t->kind()) {
                case BaseTypeKind::IntN:
                    switch(t->as_intn_type_unsafe()->IntNKind()) {
                        case IntNTypeKind::I8:
                            return write_call(typeBuilder, allocator, selfId, val, cache.getWriteI8(), "writeI8", parent_node, loc);
                        case IntNTypeKind::I16:
                            return write_call(typeBuilder, allocator, selfId, val, cache.getWriteI16(), "writeI16", parent_node, loc);
                        case IntNTypeKind::I32:
                            return write_call(typeBuilder, allocator, selfId, val, cache.getWriteI32(), "writeI32", parent_node, loc);
                        case IntNTypeKind::I64:
                            return write_call(typeBuilder, allocator, selfId, val, cache.getWriteI64(), "writeI64", parent_node, loc);
                        case IntNTypeKind::U8:
                            return write_call(typeBuilder, allocator, selfId, val, cache.getWriteU8(), "writeU8", parent_node, loc);
                        case IntNTypeKind::U16:
                            return write_call(typeBuilder, allocator, selfId, val, cache.getWriteU16(), "writeU16", parent_node, loc);
                        case IntNTypeKind::U32:
                            return write_call(typeBuilder, allocator, selfId, val, cache.getWriteU32(), "writeU32", parent_node, loc);
                        case IntNTypeKind::U64:
                            return write_call(typeBuilder, allocator, selfId, val, cache.getWriteU64(), "writeU64", parent_node, loc);
                        // handling c like types
                        case IntNTypeKind::Char:
                            return write_call(typeBuilder, allocator, selfId, val, cache.getWriteChar(), "writeChar", parent_node, loc);
                        case IntNTypeKind::UChar:
                            return write_call(typeBuilder, allocator, selfId, val, cache.getWriteUChar(), "writeUChar", parent_node, loc);
                        case IntNTypeKind::Short:
                            return write_call(typeBuilder, allocator, selfId, val, cache.getWriteShort(), "writeShort", parent_node, loc);
                        case IntNTypeKind::UShort:
                            return write_call(typeBuilder, allocator, selfId, val, cache.getWriteUShort(), "writeUShort", parent_node, loc);
                        case IntNTypeKind::Int:
                            return write_call(typeBuilder, allocator, selfId, val, cache.getWriteInt(), "writeInt", parent_node, loc);
                        case IntNTypeKind::UInt:
                            return write_call(typeBuilder, allocator, selfId, val, cache.getWriteUInt(), "writeUInt", parent_node, loc);
                        case IntNTypeKind::Long:
                            return write_call(typeBuilder, allocator, selfId, val, cache.getWriteLong(), "writeLong", parent_node, loc);
                        case IntNTypeKind::ULong:
                            return write_call(typeBuilder, allocator, selfId, val, cache.getWriteULong(), "writeULong", parent_node, loc);
                        case IntNTypeKind::LongLong:
                            return write_call(typeBuilder, allocator, selfId, val, cache.getWriteLongLong(), "writeLongLong", parent_node, loc);
                        case IntNTypeKind::ULongLong:
                            return write_call(typeBuilder, allocator, selfId, val, cache.getWriteULongLong(), "writeULongLong", parent_node, loc);
                        default:
#ifdef DEBUG
                            throw std::runtime_error("unexpected branch1: intrinsics::put_str_expr");
#endif
                            return nullptr;
                    }
                case BaseTypeKind::Float:
                    return write_call(typeBuilder, allocator, selfId, val, cache.getWriteFloat(), "writeFloat", parent_node, loc);
                case BaseTypeKind::Double:
                    return write_call(typeBuilder, allocator, selfId, val, cache.getWriteDouble(), "writeDouble", parent_node, loc);
                case BaseTypeKind::Linked:
                case BaseTypeKind::Generic: {
                    const auto node = t->get_direct_linked_canonical_node();
                    if(!node) return nullptr;
                    const auto child = node->child("stream");
                    if(!child) return nullptr;
                    // TODO: verify the method signature at some point
                    return write_obj_call(typeBuilder, allocator, selfId, val, child, "stream", parent_node, loc);
                }
                default:
                    return nullptr;
            }
        }
    }

    Value *call(InterpretScope *call_scope, ASTAllocator& allocator, FunctionCall *call, Value *parent_val, bool evaluate_refs) override {

        auto& typeBuilder = call_scope->global->typeBuilder;
        const auto loc = call->encoded_location();

        if(call->values.size() < 2) {
            call_scope->error("intrinsics::expr_str_block_value expects two arguments", call);
            return new (allocator.allocate<NullValue>()) NullValue(typeBuilder.getNullPtrType(), loc);
        }

        const auto first = call->values[0]->evaluated_value(*call_scope)->copy(allocator);
        const auto second = call->values[1]->evaluated_value(*call_scope)->copy(allocator);

        const auto parent_type = call_scope->global->current_func_type;
        const auto func = parent_type->as_function();
        ASTNode* parent_node = func ? func : parent_type->get_parent();
        const auto blkValue = new (allocator.allocate<BlockValue>()) BlockValue(parent_node, loc);

        // returning a integer value
        const auto ZeroNum = new (allocator.allocate<IntNumValue>()) IntNumValue(0, typeBuilder.getI32Type(), loc);
        blkValue->setCalculatedValue(ZeroNum);

        if(second->kind() != ValueKind::ExpressiveString && second->kind() != ValueKind::String) {
            call_scope->error("unknown value being passed to intrinsics::expr_str_block_value", second);
            return blkValue;
        }

        const auto node = first->getType()->get_direct_linked_canonical_node();
        if(!node) {
            call_scope->error("couldn't evaluate the self argument in intrinsics::expr_str_block_value", first);
            return blkValue;
        }

        // initializing the struct using a var init
        const auto init = new (allocator.allocate<VarInitStatement>()) VarInitStatement(
            false, false, ZERO_LOC_ID("__chx_sev"), nullptr, first, parent_node, loc
        );
        blkValue->scope.nodes.emplace_back(init);

        // access to the struct
        const auto initId = new (allocator.allocate<VariableIdentifier>()) VariableIdentifier(
            "__chx_sev", first->getType(), loc
        );
        initId->linked = init;
        initId->setType(init->known_type());

        // function cache stores the pointers
        ChildFnCache fnCache(node);

        // creating the method calls by going over each value
        if(second->kind() == ValueKind::ExpressiveString) {
            const auto str = second->as_expressive_str_unsafe();
            for (const auto val: str->values) {
                const auto wrapper = process_value(fnCache, typeBuilder, val, initId, allocator, parent_node, loc);
                if(wrapper == nullptr) {
                    call_scope->error("unknown value being used in expressive string", val);
                } else {
                    blkValue->scope.nodes.emplace_back(wrapper);
                }
            }
        } else {
            const auto wrapper = process_value(fnCache, typeBuilder, second, initId, allocator, parent_node, loc);
            if(wrapper == nullptr) {
                call_scope->error("unknown value being used in expressive string", second);
            } else {
                blkValue->scope.nodes.emplace_back(wrapper);
            }
        }

        // additional arguments
        auto start = call->values.data() + 2;
        const auto end = call->values.data() + call->values.size();
        while(start != end) {
            const auto value = *start;
            const auto wrapper = process_value(fnCache, typeBuilder, value, initId, allocator, parent_node, loc);
            if(wrapper == nullptr) {
                call_scope->error("unknown value being used in expressive string", value);
            } else {
                blkValue->scope.nodes.emplace_back(wrapper);
            }
            start++;
        }

        return blkValue;
    }

};

class MemNamespace : public Namespace {
public:

    InterpretMemCopy memCopyFn;

    explicit MemNamespace(
            TypeBuilder& cache,
            ASTNode* parent_node
    ) : Namespace(ZERO_LOC_ID("mem"), parent_node, ZERO_LOC, AccessSpecifier::Public), memCopyFn(cache, this) {
        set_compiler_decl(true);
        nodes = { &memCopyFn };
    }

};

class PtrNamespace : public Namespace {
public:

    InterpretIsPtrNull isNullFn;
    InterpretIsPtrNotNull isNotNullFn;

    explicit PtrNamespace(
            TypeBuilder& cache,
            ASTNode* parent_node
    ) : Namespace(ZERO_LOC_ID("ptr"), parent_node, ZERO_LOC, AccessSpecifier::Public),
        isNullFn(cache, this), isNotNullFn(cache, this)
    {
        set_compiler_decl(true);
        nodes = { &isNullFn, &isNotNullFn };
    }

};

class IntrinsicsNamespace : public Namespace {
public:

    // sub namespaces
    MemNamespace memNamespace;
    PtrNamespace ptrNamespace;

    InterpretSupports interpretSupports;
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
    InterpretValueSatisfies satisfiesValueFn;

    InterpretGetRawLocation get_raw_location;
    InterpretGetRawLocOf get_raw_loc_of;
    InterpretGetCallLoc get_call_loc;
    InterpretDecodeLocation decode_location;
    InterpretGetLineNo get_line_no;
    InterpretGetCharacterNo get_char_no;
    InterpretGetCallerLineNo get_caller_line_no;
    InterpretGetCallerCharacterNo get_caller_char_no;
    InterpretGetTarget get_target_fn;
    InterpretGetBuildDir get_build_dir;
    InterpretGetCurrentFilePath get_current_file_path;
    InterpretGetLocFilePath get_loc_file_path;
    InterpretGetModuleScope get_module_scope;
    InterpretGetModuleName get_module_name;
    InterpretGetModuleDir get_module_dir;

    InterpretGetLambdaFnPtr get_lambda_fn_ptr;
    InterpretGetLambdaCapPtr get_lambda_cap_ptr;
    InterpretGetLambdaCapDestructor get_lambda_cap_destructor;
    InterpretSizeOfLambdaCaptured sizeof_lambda_captured;
    InterpretAlignOfLambdaCaptured alignof_lambda_captured;

    InterpretDestructCallSite destruct_call_site;
    InterpretExprStrBlockValue expr_str_blk_val;

    InterpretGetTests get_tests_fn;
    InterpretGetSingleMarkedDeclPointer get_single_marked_decl_ptr;

    InterpretGetBackendName get_backend_name;
    InterpretEmitRaw emit_raw;
    InterpretRawLiteral raw_literal;
    InterpretMultiple multiple_value;

    InterpretGetLibsDir get_libs_dir;

    // TODO get_child_fn should be removed
    // we should use get destructor explicitly
    InterpretGetChildFunction get_child_fn;
    InterpretForget forget_fn;
    InterpretError error_fn;

    IntrinsicsNamespace(
            TypeBuilder& cache
    ) : Namespace(ZERO_LOC_ID("intrinsics"), nullptr, ZERO_LOC, AccessSpecifier::Public),
        memNamespace(cache, this), ptrNamespace(cache, this),
        interpretSupports(cache, this), printFn(cache, this), printlnFn(cache, this), to_stringFn(cache, this), type_to_stringFn(cache, this),
        wrapFn(cache, this), unwrapFn(cache, this), retStructPtr(cache, this), verFn(cache, this), isTccFn(cache, this), isClangFn(cache, this),
        sizeFn(cache, this), vectorNode(cache, this), satisfiesFn(cache, this), satisfiesValueFn(cache, this), get_target_fn(cache, this),
        get_build_dir(cache, this), get_current_file_path(cache, this), get_raw_location(cache, this), get_raw_loc_of(cache, this),
        get_call_loc(cache, this), decode_location(cache, this), get_char_no(cache, this), get_caller_line_no(cache, this), get_caller_char_no(cache, this),
        get_loc_file_path(cache, this), get_module_scope(cache, this), get_module_name(cache, this), get_module_dir(cache, this),
        get_child_fn(cache, this), forget_fn(cache, this), error_fn(cache, this), get_tests_fn(cache, this), get_single_marked_decl_ptr(cache, this),
        get_lambda_fn_ptr(cache, this), get_lambda_cap_ptr(cache, this), get_lambda_cap_destructor(cache, this),
        sizeof_lambda_captured(cache, this), alignof_lambda_captured(cache, this), destruct_call_site(cache, this),
        expr_str_blk_val(cache, this), get_backend_name(cache, this), emit_raw(cache, this), raw_literal(cache, this),
        multiple_value(cache, this), get_libs_dir(cache, this)
    {
        set_compiler_decl(true);
        nodes = {
            &memNamespace, &ptrNamespace,
            &interpretSupports, &printFn, &printlnFn, &to_stringFn, &type_to_stringFn, &wrapFn, &unwrapFn,
            &retStructPtr, &verFn, &isTccFn, &isClangFn, &sizeFn, &vectorNode, &satisfiesFn, &satisfiesValueFn, &get_raw_location,
            &get_raw_loc_of, &get_call_loc, &decode_location, &get_line_no, &get_char_no, &get_caller_line_no, &get_caller_char_no,
            &get_target_fn, &get_build_dir, &get_current_file_path, &get_loc_file_path, &get_tests_fn, &get_single_marked_decl_ptr,
            &get_module_scope, &get_module_name, &get_module_dir, &get_child_fn, &forget_fn, &error_fn,
            &get_lambda_fn_ptr, &get_lambda_cap_ptr, &get_lambda_cap_destructor, &sizeof_lambda_captured, &alignof_lambda_captured,
            &destruct_call_site, &expr_str_blk_val, &get_backend_name, &emit_raw, &raw_literal,
            &multiple_value, &get_libs_dir
        };
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

    DefValue(DefDecl *defDecl) : StructValue(
            defDecl->known_type(),
            defDecl,
            defDecl,
            ZERO_LOC
    ) {}

};

struct DefThing {

    DefDecl decl;
    DefValue defValue;
    VarInitStatement defStmt;

    DefThing() : defValue(&decl), defStmt(true, false, ZERO_LOC_ID("def"), TypeLoc(defValue.getRefType(), ZERO_LOC), &defValue, nullptr, ZERO_LOC, AccessSpecifier::Public) {
        defStmt.set_compiler_decl(true);
    }

    void declare_value(ASTAllocator& allocator, const chem::string_view& name, BaseType* type, Value* value) {
        const auto member = new (allocator.allocate<StructMember>()) StructMember(name, {type, ZERO_LOC}, nullptr, &decl, ZERO_LOC, true);
        decl.insert_variable_no_check(member);
        defValue.values.emplace(name, StructMemberInitializer{ name, value });
    }

    void clear_values() {
        decl.clear_variables_and_indexes();
        defValue.values.clear();
    }

};

struct GlobalContainer {

    IntrinsicsNamespace intrinsics_namespace;
    InterpretDefined defined;
    DefThing defThing;

    GlobalContainer(
            TypeBuilder& cache
    ) : intrinsics_namespace(cache), defined(cache) {

    }

};

// this puts the target data of the current executable
void prepare_executable_target_data(TargetData& data) {
#ifdef _WIN32
    data.windows = true;
    data.win32 = true;
#endif

#ifdef _WIN64
    data.windows = true;
    data.win64 = true;
#endif

    data.is64Bit = sizeof(void*) == 8;

#ifdef __linux__
    data.isLinux = true;
    data.isUnix = true;
#endif

#ifdef __APPLE__
    data.macos = true;
    data.unix = true;
#endif

#ifdef __FreeBSD__
    data.freebsd = true;
    data.unix = true;
#endif

#ifdef __ANDROID__
    data.android = true;
    data.unix = true;
#endif

#ifdef __CYGWIN__
    data.cygwin = true;
    data.unix = true;
#endif

#ifdef __MINGW32__
    data.mingw32 = true;
    data.win32 = true;
    data.windows = true;
#endif

#ifdef __MINGW64__
    data.mingw64 = true;
    data.win64 = true;
    data.windows = true;
#endif

// Detect architecture using predefined macros
#ifdef __x86_64__
    data.x86_64 = true;
#endif

#ifdef __i386__
    data.i386 = true;
#endif

#ifdef __arm__
    data.arm = true;
#endif

#ifdef __aarch64__
    data.aarch64 = true;
#endif

    data.little_endian = IS_LITTLE_ENDIAN;

}

#ifdef COMPILER_BUILD

void init_target_data(llvm::Triple& triple, TargetData& data) {

    const auto arhType = triple.getArch();

    // Check for Windows
    if (triple.isOSWindows()) {
        data.windows = true;
        if (arhType == llvm::Triple::x86) {
            data.win32 = true;
        } else if (arhType == llvm::Triple::x86_64) {
            data.win64 = true;
        }
    }

    // Check for Linux
    if (triple.isOSLinux()) {
        data.isLinux = true;
        data.isUnix = true;
    }

    // Check for macOS
    if (triple.isMacOSX()) {
        data.macos = true;
        data.isUnix = true;
    }

    // Check for FreeBSD
    if (triple.isOSFreeBSD()) {
        data.freebsd = true;
        data.isUnix = true;
    }

    // Check for Android
    if (triple.isAndroid()) {
        data.android = true;
        data.isUnix = true;
    }

    // Check for Cygwin
    if (triple.isOSCygMing()) {
        data.cygwin = true;
        data.isUnix = true;
    }

    if(triple.isArch64Bit()) {
        data.is64Bit = true;
    }

    // Check for architecture
    switch(arhType) {
        case llvm::Triple::x86_64:
            data.x86_64 = true;
            break;
        case llvm::Triple::x86:
            data.i386 = true;
            break;
        case llvm::Triple::arm:
            data.arm = true;
            break;
        case llvm::Triple::aarch64:
            data.aarch64 = true;
            break;
        default:
            break;
    }

    data.little_endian = triple.isLittleEndian();

}

void GlobalInterpretScope::prepare_target_data(TargetData& data, const std::string& target_triple) {

    if(target_triple.empty()) {
        prepare_executable_target_data(data);
        return;
    }

    auto triple = llvm::Triple(target_triple);
    init_target_data(triple, data);
}

#else

void GlobalInterpretScope::prepare_target_data(TargetData& data, const std::string& target_triple) {

    if(target_triple.empty()) {
        prepare_executable_target_data(data);
        return;
    }

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
        data.x86_64 = true;
    } else if (arch == "i386") {
        data.i386 = true;
    } else if (arch == "arm") {
        data.arm = true;
    } else if (arch == "aarch64") {
        data.aarch64 = true;
    }

    // Determine operating system
    if (sys == "linux") {
        data.isLinux = true;
        data.isUnix = true;
    } else if (sys == "windows") {
        data.win32 = true;
        data.windows = true;
    } else if (sys == "darwin") {
        data.macos = true;
        data.isUnix = true;
    } else if (sys == "freebsd") {
        data.freebsd = true;
        data.isUnix = true;
    } else if (sys == "android") {
        data.android = true;
        data.isUnix = true;
    } else if (sys == "cygwin") {
        data.cygwin = true;
        data.isUnix = true;
    } else if (sys == "mingw32") {
        data.mingw32 = true;
        data.win32 = true;
        data.windows = true;
    } else if (sys == "mingw64") {
        data.win64 = true;
        data.windows = true;
    }

    if (arch == "x86_64" || arch == "i386" || arch == "aarch64") {
        data.little_endian = true;
    } else if (arch == "arm") {
        if (target_triple.find("arm-be") != std::string::npos || abi == "be") {
            data.little_endian = false;
        } else if (target_triple.find("arm-le") != std::string::npos || abi == "le") {
            data.little_endian = true;
        } else {
            data.little_endian = IS_LITTLE_ENDIAN;
        }
    } else if (arch.find("be") != std::string::npos) {
        data.little_endian = false;
    } else {
        data.little_endian = IS_LITTLE_ENDIAN;
    }

}

#endif

BoolValue* boolValue(ASTAllocator& allocator, TypeBuilder& typeBuilder, bool value) {
    return new (allocator.allocate<BoolValue>()) BoolValue(value, typeBuilder.getBoolType(), ZERO_LOC);
}

void declare_def_values(ASTAllocator& allocator, TypeBuilder& typeBuilder, DefThing& defThing, const TargetData& data) {
    const auto boolType = typeBuilder.getBoolType();
    defThing.declare_value(allocator, "tcc", boolType, boolValue(allocator, typeBuilder, data.tcc));
    defThing.declare_value(allocator, "clang", boolType, boolValue(allocator, typeBuilder, data.clang));
    defThing.declare_value(allocator, "cbi", boolType, boolValue(allocator, typeBuilder, data.cbi));
    defThing.declare_value(allocator, "lsp", boolType, boolValue(allocator, typeBuilder, data.lsp));
    defThing.declare_value(allocator, "test", boolType, boolValue(allocator, typeBuilder, data.test));
    defThing.declare_value(allocator, "debug", boolType, boolValue(allocator, typeBuilder, data.debug));
    defThing.declare_value(allocator, "debug_quick", boolType, boolValue(allocator, typeBuilder, data.debug_quick));
    defThing.declare_value(allocator, "debug_complete", boolType, boolValue(allocator, typeBuilder, data.debug_complete));
    defThing.declare_value(allocator, "release", boolType, boolValue(allocator, typeBuilder, data.release));
    defThing.declare_value(allocator, "release_safe", boolType, boolValue(allocator, typeBuilder, data.release_safe));
    defThing.declare_value(allocator, "release_small", boolType, boolValue(allocator, typeBuilder, data.release_small));
    defThing.declare_value(allocator, "release_fast", boolType, boolValue(allocator, typeBuilder, data.release_fast));
    defThing.declare_value(allocator, "little_endian", boolType, boolValue(allocator, typeBuilder, data.little_endian));
    defThing.declare_value(allocator, "big_endian", boolType, boolValue(allocator, typeBuilder, data.big_endian));
    defThing.declare_value(allocator, "is64Bit", boolType, boolValue(allocator, typeBuilder, data.is64Bit));
    defThing.declare_value(allocator, "windows", boolType, boolValue(allocator, typeBuilder, data.windows));
    defThing.declare_value(allocator, "posix", boolType, boolValue(allocator, typeBuilder, data.posix));
    defThing.declare_value(allocator, "win32", boolType, boolValue(allocator, typeBuilder, data.win32));
    defThing.declare_value(allocator, "win64", boolType, boolValue(allocator, typeBuilder, data.win64));
    defThing.declare_value(allocator, "linux", boolType, boolValue(allocator, typeBuilder, data.isLinux));
    defThing.declare_value(allocator, "macos", boolType, boolValue(allocator, typeBuilder, data.macos));
    defThing.declare_value(allocator, "freebsd", boolType, boolValue(allocator, typeBuilder, data.freebsd));
    defThing.declare_value(allocator, "unix", boolType, boolValue(allocator, typeBuilder, data.isUnix));
    defThing.declare_value(allocator, "gnu", boolType, boolValue(allocator, typeBuilder, data.gnu));
    defThing.declare_value(allocator, "android", boolType, boolValue(allocator, typeBuilder, data.android));
    defThing.declare_value(allocator, "cygwin", boolType, boolValue(allocator, typeBuilder, data.cygwin));
    defThing.declare_value(allocator, "mingw32", boolType, boolValue(allocator, typeBuilder, data.mingw32));
    defThing.declare_value(allocator, "x86_64", boolType, boolValue(allocator, typeBuilder, data.x86_64));
    defThing.declare_value(allocator, "i386", boolType, boolValue(allocator, typeBuilder, data.i386));
    defThing.declare_value(allocator, "arm", boolType, boolValue(allocator, typeBuilder, data.arm));
    defThing.declare_value(allocator, "aarch64", boolType, boolValue(allocator, typeBuilder, data.aarch64));
}

void create_target_data_in_def(GlobalInterpretScope& scope, DefThing& defThing, const TargetData& data) {
    // declaring native definitions like windows and stuff
    auto& allocator = scope.allocator;
    // we change the global interpret scope for each job, so we must redeclare def value
    scope.values["def"] = &defThing.defValue;
    declare_def_values(allocator, scope.typeBuilder, defThing, data);
}

void GlobalInterpretScope::rebind_container(SymbolResolver& resolver, GlobalContainer* container_ptr, const TargetData& data) {
    auto& container = *container_ptr;

    // from previous (job / lsp request) global interpret scope, user may have introduced declarations into
    // these namespaces (which may be invalid, because their allocators have been destroyed)
    container.intrinsics_namespace.extended.clear();
    container.intrinsics_namespace.memNamespace.extended.clear();
    container.intrinsics_namespace.ptrNamespace.extended.clear();

    // declare the nodes inside the symbol resolver
    TopLevelDeclSymDeclare declarer(resolver);

    // this would re-insert the children into extended map of namespaces
    // TODO: do not visit the namespace for this
    declarer.visit(&container.intrinsics_namespace);

    // TODO these symbols will be removed when module ends
    // TODO use exported declare or rename function to convey meaning better
    resolver.declare(container.defined.name_view(), &container.defined);
    resolver.declare(container.defThing.decl.name_view(), &container.defThing.decl);
    resolver.declare(container.defThing.defStmt.id_view(), &container.defThing.defStmt);

    container.defThing.clear_values();
    // we recreate the target data, because the allocator disposes at the end of each job
    // and this method is called after disposal of the allocator when the job ends, and a new job starts
    // also the target data depends on the target, each target can have different variables
    create_target_data_in_def(*this, container.defThing, data);

}

GlobalContainer* GlobalInterpretScope::create_container(SymbolResolver& resolver, const TargetData& data) {

    auto& typeCache = resolver.comptime_scope.typeBuilder;
    const auto container_ptr = new GlobalContainer(typeCache);
    auto& container = *container_ptr;

    // declare the nodes inside the symbol resolver
    TopLevelDeclSymDeclare declarer(resolver);

    // namespaces
    declarer.visit(&container.intrinsics_namespace);
    declarer.visit(&container.defined);

    // definitions using defThing
    declarer.visit(&container.defThing.decl);
    declarer.visit(&container.defThing.defStmt);

    create_target_data_in_def(*this, container.defThing, data);

    return container_ptr;
}

BoolValue* get_global_condition(GlobalContainer* container, const chem::string_view& name) {
    auto& values = container->defThing.defValue.values;
    auto found = values.find(name);
    if(found != values.end()) {
        const auto val = found->second.value;
        if(val->kind() == ValueKind::Bool) {
            return val->as_bool_unsafe();
        }
    }
    return nullptr;
}

//bool set_global_condition(GlobalContainer* container, const chem::string_view& name, bool enable) {
//    const auto cond = get_global_condition(container, name);
//    if(cond) {
//        cond->value = enable;
//        return true;
//    } else {
//        return false;
//    }
//}

std::optional<bool> is_condition_enabled(GlobalContainer* container, const chem::string_view& name) {
    const auto cond = get_global_condition(container, name);
    if(cond) {
        return cond->value;
    } else {
        return std::nullopt;
    }
}

std::optional<bool> is_condition_enabled(GlobalContainer* container, IffyBase* base) {
    if(base == nullptr) return std::nullopt;
    if(base->is_id) {
        const auto if_id = (IffyCondId*) base;
        auto value = is_condition_enabled(container, if_id->value);
        if(value.has_value()) {
            if(if_id->is_negative) {
                return !value.value();
            }
        }
        return value;
    } else {
        const auto if_expr = (IffyCondExpr*) base;
        auto value = is_condition_enabled(container, if_expr->left);
        if(value.has_value()) {
            if(value.value() && if_expr->op == IffyExprOp::Or) {
                // value is true, in or expression, we do not need to resolve second
                return true;
            } else if(!value.value() && if_expr->op == IffyExprOp::And) {
                // value is false, in and expression, we do not need to resolve second
                return false;
            } else {
                // in 'and', first is true, depends on second (true if second is true, false if second is false)
                // in 'or' , first is false, depends on second (true if second is true, false if second is false)
                return is_condition_enabled(container, if_expr->right);
            }
        }
    }
    return std::nullopt;
}

void GlobalInterpretScope::dispose_container(GlobalContainer* container) {
    delete container;
}