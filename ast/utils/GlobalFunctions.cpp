// Copyright (c) Qinetik 2024.

#include "ast/base/GlobalInterpretScope.h"
#include "ast/structures/FunctionParam.h"
#include "sstream"
#include <utility>
#include "ast/types/VoidType.h"
#include "ast/types/StringType.h"
#include "ast/types/BoolType.h"
#include "ast/values/IntValue.h"
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
#include "ast/structures/FunctionParam.h"
#include "ast/types/PointerType.h"
#include "ast/types/ReferencedType.h"
#include "ast/types/GenericType.h"
#include "ast/types/UBigIntType.h"
#include "ast/types/AnyType.h"
#include "preprocess/RepresentationVisitor.h"
#include "utils/Version.h"

namespace InterpretVector {

    class InterpretVectorNode : public StructDefinition {
    public:
        explicit InterpretVectorNode(ASTNode* parent_node);
    };

    class InterpretVectorVal : public StructValue {
    public:
        std::vector<std::unique_ptr<Value>> values;
        explicit InterpretVectorVal(InterpretVectorNode* node) : StructValue(
            nullptr,
            {},
            {},
            (StructDefinition*) node,
            nullptr
        ) {

        }
    };

    class InterpretVectorConstructor : public FunctionDeclaration {
    public:
        explicit InterpretVectorConstructor(InterpretVectorNode* node) : FunctionDeclaration(
                "constructor",
                std::vector<std::unique_ptr<FunctionParam>> {},
                node->create_value_type(),
                false,
                node,
                nullptr,
                std::nullopt
        ) {
            annotations.emplace_back(AnnotationKind::Constructor);
        }
        Value *call(InterpretScope *call_scope, FunctionCall *call, Value *parent_val, bool evaluate_refs) override {
            return new InterpretVectorVal((InterpretVectorNode*) parent_node);
        }
    };

    class InterpretVectorSize : public FunctionDeclaration {
    public:
        explicit InterpretVectorSize(InterpretVectorNode* node) : FunctionDeclaration(
            "size",
            std::vector<std::unique_ptr<FunctionParam>> {},
            std::make_unique<IntType>(nullptr),
            false,
            node,
            nullptr,
            std::nullopt
        ) {
            params.emplace_back(std::make_unique<FunctionParam>("self", std::make_unique<PointerType>(std::make_unique<ReferencedType>("vector", nullptr, node), nullptr), 0, nullptr, this, nullptr));
        }
        Value *call(InterpretScope *call_scope, FunctionCall *call, Value *parent_val, bool evaluate_refs) override {
            return new IntValue(static_cast<InterpretVectorVal*>(parent_val)->values.size(), nullptr);
        }
    };

    class InterpretVectorGet : public FunctionDeclaration {
    public:
        explicit InterpretVectorGet(InterpretVectorNode* node) : FunctionDeclaration(
                "get",
                std::vector<std::unique_ptr<FunctionParam>> {},
                std::make_unique<ReferencedType>("T", nullptr, node->generic_params[0].get()),
                false,
                node,
                nullptr,
                std::nullopt
        ) {
            params.emplace_back(std::make_unique<FunctionParam>("self", std::make_unique<PointerType>(std::make_unique<ReferencedType>("vector", nullptr, node), nullptr), 0, nullptr, this, nullptr));
            params.emplace_back(std::make_unique<FunctionParam>("index", std::make_unique<AnyType>(nullptr), 0, nullptr, this, nullptr));
        }
        Value *call(InterpretScope *call_scope, FunctionCall *call, Value *parent_val, bool evaluate_refs) override {
            return static_cast<InterpretVectorVal*>(parent_val)->values[call->values[0]->evaluated_value(*call_scope)->as_int()]->scope_value(*call_scope);
        }
    };
    class InterpretVectorPush : public FunctionDeclaration {
    public:
        explicit InterpretVectorPush(InterpretVectorNode* node) : FunctionDeclaration(
                "push",
                std::vector<std::unique_ptr<FunctionParam>> {},
                std::make_unique<VoidType>(nullptr),
                false,
                node,
                nullptr,
                std::nullopt
        ) {
            params.emplace_back(std::make_unique<FunctionParam>("self", std::make_unique<PointerType>(std::make_unique<ReferencedType>("vector", nullptr, node), nullptr), 0, nullptr, this, nullptr));
            params.emplace_back(std::make_unique<FunctionParam>("value", std::make_unique<ReferencedType>("T", nullptr, node->generic_params[0].get()), 1, nullptr, this, nullptr));
        }
        Value *call(InterpretScope *call_scope, FunctionCall *call, Value *parent_val, bool evaluate_refs) override {
            static_cast<InterpretVectorVal*>(parent_val)->values.emplace_back(call->values[0]->scope_value(*call_scope));
            return nullptr;
        }
    };
    class InterpretVectorRemove : public FunctionDeclaration {
    public:
        explicit InterpretVectorRemove(InterpretVectorNode* node) : FunctionDeclaration(
                "remove",
                std::vector<std::unique_ptr<FunctionParam>> {},
                std::make_unique<VoidType>(nullptr),
                false,
                node,
                nullptr,
                std::nullopt
        ) {
            params.emplace_back(std::make_unique<FunctionParam>("self", std::make_unique<PointerType>(std::make_unique<ReferencedType>("vector", nullptr, node), nullptr), 0, nullptr, this, nullptr));
            params.emplace_back(std::make_unique<FunctionParam>("index", std::make_unique<AnyType>(nullptr), 0, nullptr, this, nullptr));
        }
        Value *call(InterpretScope *call_scope, FunctionCall *call, Value *parent_val, bool evaluate_refs) override {
            auto& ref = static_cast<InterpretVectorVal*>(parent_val)->values;
            ref.erase(ref.begin() + call->values[0]->evaluated_value(*call_scope)->as_int());
            return nullptr;
        }
    };

    InterpretVectorNode::InterpretVectorNode(ASTNode* parent_node) : StructDefinition("vector", parent_node, nullptr) {
        generic_params.emplace_back(new GenericTypeParameter("T", nullptr, this, 0, nullptr));
        insert_func(std::make_unique<InterpretVectorConstructor>(this));
        insert_func(std::make_unique<InterpretVectorSize>(this));
        insert_func(std::make_unique<InterpretVectorGet>(this));
        insert_func(std::make_unique<InterpretVectorPush>(this));
        insert_func(std::make_unique<InterpretVectorRemove>(this));
    }

}

class InterpretPrint : public FunctionDeclaration {
public:
    std::ostringstream ostring;
    RepresentationVisitor visitor;
    explicit InterpretPrint(ASTNode* parent_node) : FunctionDeclaration(
            "print",
            std::vector<std::unique_ptr<FunctionParam>> {},
            std::make_unique<VoidType>(nullptr),
            true,
            parent_node,
            nullptr,
            std::nullopt
    ), visitor(ostring) {
        visitor.interpret_representation = true;
    }
    Value *call(InterpretScope *call_scope, FunctionCall *call, Value *parent_val, bool evaluate_refs) override {
        ostring.str("");
        ostring.clear();
        for (auto const &value: call->values) {
            auto paramValue = value->evaluated_value(*call_scope);
            if(paramValue.get() == nullptr) {
                ostring.write("null", 4);
            } else {
                paramValue->accept(&visitor);
            }
        }
        std::cout << ostring.str();
        return nullptr;
    }
};

hybrid_ptr<Value> resolve_ref(Value* val, InterpretScope *call_scope) {
    hybrid_ptr<Value> value{nullptr, false};
    if(val->reference()) {
        auto linked = val->linked_node();
        if(linked && !linked->as_func_param()) {
            auto holding = linked->holding_value();
            if(holding) {
                value = hybrid_ptr<Value>{holding, false};
            }
        } else {
            value = val->evaluated_value(*call_scope);
        }
    } else {
        value = val->evaluated_value(*call_scope);
    }
    if(value && value->reference()) {
        return resolve_ref(value.get(), call_scope);
    }
    return value;
}

class InterpretSize : public FunctionDeclaration {
public:
    explicit InterpretSize(ASTNode* parent_node) : FunctionDeclaration(
            "size",
            std::vector<std::unique_ptr<FunctionParam>> {},
            std::make_unique<UBigIntType>(nullptr),
            false,
            parent_node,
            nullptr,
            std::nullopt
    ) {
        annotations.emplace_back(AnnotationKind::CompTime);
        params.emplace_back(std::make_unique<FunctionParam>("value", std::make_unique<AnyType>(nullptr), 0, nullptr, this, nullptr));
    }
    Value *call(InterpretScope *call_scope, FunctionCall *call, Value *parent_val, bool evaluate_refs) override {
        if(call->values.empty()) {
            call_scope->error("compiler::size called without arguments");
            return nullptr;
        }
        const auto val = call->values[0].get();
        const auto val_type = val->value_type();
        if(val_type != ValueType::String && val_type != ValueType::Array) {
            call_scope->error("compiler::size called with invalid arguments");
            return nullptr;
        }
        auto value = resolve_ref(val, call_scope);
        if(!value) {
            call_scope->error("couldn't get value for compiler::size");
            return nullptr;
        }
        switch(val_type) {
            case ValueType::String:
                return new UBigIntValue(value->as_string().length(), nullptr);
            case ValueType::Array:
                return new UBigIntValue(value->as_array_value()->array_size(), nullptr);
            default:
                return new UBigIntValue(0, nullptr);
        }
    }
};

class WrapValue : public Value {
public:
    std::unique_ptr<Value> underlying;
    explicit WrapValue(std::unique_ptr<Value> underlying) : underlying(std::move(underlying)) {

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
    Value *copy() override {
        return new WrapValue(std::unique_ptr<Value>(underlying->copy()));
    }
    hybrid_ptr<Value> evaluated_value(InterpretScope &scope) override {
        return hybrid_ptr<Value> { underlying.get(), false };
    }
    std::unique_ptr<Value> create_evaluated_value(InterpretScope &scope) override {
        return std::move(underlying);
    }
};

class InterpretWrap : public FunctionDeclaration {
public:
    explicit InterpretWrap(ASTNode* parent_node) : FunctionDeclaration(
            "wrap",
            std::vector<std::unique_ptr<FunctionParam>> {},
            std::make_unique<AnyType>(nullptr),
            true,
            parent_node,
            nullptr,
            std::nullopt
    ) {
        annotations.emplace_back(AnnotationKind::CompTime);
        // having a generic type parameter T requires that user gives type during function call to wrap
        // when we can successfully avoid giving type for generic parameters in functions, we should do this
//        generic_params.emplace_back(new GenericTypeParameter("T", nullptr, this));
//        returnType = std::make_unique<ReferencedType>("T", generic_params[0].get());
        params.emplace_back(std::make_unique<FunctionParam>("value", std::make_unique<AnyType>(nullptr), 0, nullptr, this, nullptr));
    }
    Value *call(InterpretScope *call_scope, FunctionCall *call, Value *parent_val, bool evaluate_refs) override {
        auto underlying = call->values[0]->copy();
        underlying->evaluate_children(*call_scope);
        return new WrapValue(std::unique_ptr<Value>(underlying));
    }
};

class InterpretUnwrap : public FunctionDeclaration {
public:
    explicit InterpretUnwrap(ASTNode* parent_node) : FunctionDeclaration(
            "unwrap",
            std::vector<std::unique_ptr<FunctionParam>> {},
            std::make_unique<AnyType>(nullptr),
            true,
            parent_node,
            nullptr,
            std::nullopt
    ) {
        annotations.emplace_back(AnnotationKind::CompTime);
        // having a generic type parameter T requires that user gives type during function call to wrap
        // when we can successfully avoid giving type for generic parameters in functions, we should do this
//        generic_params.emplace_back(new GenericTypeParameter("T", nullptr, this));
//        returnType = std::make_unique<ReferencedType>("T", generic_params[0].get());
        params.emplace_back(std::make_unique<FunctionParam>("value", std::make_unique<AnyType>(nullptr), 0, nullptr, this, nullptr));
    }
    Value *call(InterpretScope *call_scope, FunctionCall *call, Value *parent_val, bool evaluate_refs) override {
        return call->values[0]->evaluated_value(*call_scope)->copy();
    }
};

class InterpretRetStructPtr : public FunctionDeclaration {
public:
    explicit InterpretRetStructPtr(ASTNode* parent_node) : FunctionDeclaration(
            "return_struct",
            std::vector<std::unique_ptr<FunctionParam>> {},
            // TODO fix return type
            std::make_unique<PointerType>(std::make_unique<VoidType>(nullptr), nullptr),
            true,
            parent_node,
            nullptr,
            std::nullopt
    ) {
        annotations.emplace_back(AnnotationKind::CompTime);
    }
    Value *call(InterpretScope *call_scope, FunctionCall *call, Value *parent_val, bool evaluate_refs) override {
        return new RetStructParamValue(nullptr);
    }
};

class InterpretCompilerVersion : public FunctionDeclaration {
public:
    explicit InterpretCompilerVersion(ASTNode* parent_node) : FunctionDeclaration(
            "version",
            std::vector<std::unique_ptr<FunctionParam>> {},
            std::make_unique<StringType>(nullptr),
            false,
            parent_node,
            nullptr,
            std::nullopt
    ) {
        annotations.emplace_back(AnnotationKind::CompTime);
    }
    Value *call(InterpretScope *call_scope, FunctionCall *call, Value *parent_val, bool evaluate_refs) override {
        std::string val;
        val.append(std::to_string(PROJECT_VERSION_MAJOR));
        val.append(1, '.');
        val.append(std::to_string(PROJECT_VERSION_MINOR));
        val.append(1, '.');
        val.append(std::to_string(PROJECT_VERSION_PATCH));
        return new StringValue(std::move(val), nullptr);
    }
};

class InterpretIsTcc : public FunctionDeclaration {
public:
    explicit InterpretIsTcc(ASTNode* parent_node) : FunctionDeclaration(
            "is_tcc_based",
            std::vector<std::unique_ptr<FunctionParam>> {},
            std::make_unique<BoolType>(nullptr),
            false,
            parent_node,
            nullptr,
            std::nullopt
    ) {
        annotations.emplace_back(AnnotationKind::CompTime);
    }
    Value *call(InterpretScope *call_scope, FunctionCall *call, Value *parent_val, bool evaluate_refs) override {
#ifdef TCC_BUILD
        return new BoolValue(true, nullptr);
#else
        return new BoolValue(false, nullptr);
#endif
    }
};

class InterpretIsClang : public FunctionDeclaration {
public:
    explicit InterpretIsClang(ASTNode* parent_node) : FunctionDeclaration(
            "is_clang_based",
            std::vector<std::unique_ptr<FunctionParam>> {},
            std::make_unique<BoolType>(nullptr),
            false,
            parent_node,
            nullptr,
            std::nullopt
    ) {
        annotations.emplace_back(AnnotationKind::CompTime);
    }
    Value *call(InterpretScope *call_scope, FunctionCall *call, Value *parent_val, bool evaluate_refs) override {
#ifdef COMPILER_BUILD
        return new BoolValue(true, nullptr);
#else
        return new BoolValue(false, nullptr);
#endif
    }
};

//class InterpretConstruct : public FunctionDeclaration {
//public:
//    explicit InterpretConstruct(ASTNode* parent_node) : FunctionDeclaration(
//            "construct",
//            std::vector<std::unique_ptr<FunctionParam>> {},
//            std::make_unique<VoidType>(),
//            false,
//            parent_node,
//            std::nullopt
//    ) {
//        annotations.emplace_back(AnnotationKind::CompTime);
//        params.emplace_back(std::make_unique<FunctionParam>("ptr", std::make_unique<PointerType>(std::make_unique<VoidType>()), 0, std::nullopt, this));
//        params.emplace_back(std::make_unique<FunctionParam>("value", std::make_unique<AnyType>(), 1, std::nullopt, this));
//    }
//    Value *call(InterpretScope *call_scope, FunctionCall *call, Value *parent_val, bool evaluate_refs) override {
//
//    }
//};

std::unique_ptr<Namespace>& GlobalInterpretScope::create_compiler_namespace() {
    global_nodes["compiler"] = std::make_unique<Namespace>("compiler", nullptr, nullptr);
    auto compiler_ns = (Namespace*) global_nodes["compiler"].get();
    compiler_ns->annotations.emplace_back(AnnotationKind::CompTime);
    compiler_ns->nodes.emplace_back(new InterpretPrint(compiler_ns));
    compiler_ns->nodes.emplace_back(new InterpretWrap(compiler_ns));
    compiler_ns->nodes.emplace_back(new InterpretUnwrap(compiler_ns));
    compiler_ns->nodes.emplace_back(new InterpretRetStructPtr(compiler_ns));
    compiler_ns->nodes.emplace_back(new InterpretCompilerVersion(compiler_ns));
    compiler_ns->nodes.emplace_back(new InterpretIsTcc(compiler_ns));
    compiler_ns->nodes.emplace_back(new InterpretIsClang(compiler_ns));
    compiler_ns->nodes.emplace_back(new InterpretSize(compiler_ns));
//    compiler_ns->nodes.emplace_back(new InterpretConstruct(compiler_ns));
    compiler_ns->nodes.emplace_back(new InterpretVector::InterpretVectorNode(compiler_ns));
    return (std::unique_ptr<Namespace>&) global_nodes["compiler"];
}

void GlobalInterpretScope::prepare_compiler_namespace(SymbolResolver& resolver) {
    auto& compiler_ns = create_compiler_namespace();
    compiler_ns->declare_top_level(resolver, (std::unique_ptr<ASTNode>&) compiler_ns);
    compiler_ns->declare_and_link(resolver, (std::unique_ptr<ASTNode>&) compiler_ns);
}

void GlobalInterpretScope::rebind_compiler_namespace(SymbolResolver &resolver) {
    auto& compiler_ns = global_nodes["compiler"];
    compiler_ns->declare_top_level(resolver, compiler_ns);
}