// Copyright (c) Qinetik 2024.

#include "ast/base/GlobalInterpretScope.h"
#include "ast/structures/FunctionParam.h"
#include "sstream"
#include <utility>
#include "ast/types/VoidType.h"
#include "ast/values/IntValue.h"
#include "ast/values/UBigIntValue.h"
#include "ast/values/RetStructParamValue.h"
#include "ast/values/StructValue.h"
#include "ast/values/FunctionCall.h"
#include "compiler/SymbolResolver.h"
#include "ast/structures/Namespace.h"
#include "ast/structures/StructDefinition.h"
#include "ast/structures/FunctionParam.h"
#include "ast/types/PointerType.h"
#include "ast/types/ReferencedType.h"
#include "ast/types/UBigIntType.h"
#include "ast/types/AnyType.h"
#include "preprocess/RepresentationVisitor.h"

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
            (StructDefinition*) node
        ) {

        }
    };

    class InterpretVectorConstructor : public FunctionDeclaration {
    public:
        explicit InterpretVectorConstructor(InterpretVectorNode* node) : FunctionDeclaration(
                "constructor",
                std::vector<std::unique_ptr<FunctionParam>> {},
                std::make_unique<ReferencedType>("Vector", node),
                false,
                node,
                std::nullopt
        ) {
            annotations.emplace_back(AnnotationKind::Constructor);
        }
        Value *call(InterpretScope *call_scope, FunctionCall *call, Value *parent_val) override {
            return new InterpretVectorVal((InterpretVectorNode*) parent_node);
        }
    };

    class InterpretVectorSize : public FunctionDeclaration {
    public:
        explicit InterpretVectorSize(InterpretVectorNode* node) : FunctionDeclaration(
            "size",
            std::vector<std::unique_ptr<FunctionParam>> {},
            std::make_unique<IntType>(),
            false,
            node,
            std::nullopt
        ) {
            params.emplace_back(std::make_unique<FunctionParam>("self", std::make_unique<PointerType>(std::make_unique<ReferencedType>("Vector", node)), 0, std::nullopt, this));
        }
        Value *call(InterpretScope *call_scope, FunctionCall *call, Value *parent_val) override {
            return new IntValue(static_cast<InterpretVectorVal*>(parent_val)->values.size());
        }
    };

    class InterpretVectorGet : public FunctionDeclaration {
    public:
        explicit InterpretVectorGet(InterpretVectorNode* node) : FunctionDeclaration(
                "get",
                std::vector<std::unique_ptr<FunctionParam>> {},
                std::make_unique<IntType>(),
                false,
                node,
                std::nullopt
        ) {
            params.emplace_back(std::make_unique<FunctionParam>("self", std::make_unique<PointerType>(std::make_unique<ReferencedType>("Vector", node)), 0, std::nullopt, this));
        }
        Value *call(InterpretScope *call_scope, FunctionCall *call, Value *parent_val) override {
            return static_cast<InterpretVectorVal*>(parent_val)->values[call->values[0]->evaluated_value(*call_scope)->as_int()]->scope_value(*call_scope);
        }
    };
    class InterpretVectorPush : public FunctionDeclaration {
    public:
        explicit InterpretVectorPush(InterpretVectorNode* node) : FunctionDeclaration(
                "push",
                std::vector<std::unique_ptr<FunctionParam>> {},
                std::make_unique<VoidType>(),
                false,
                node,
                std::nullopt
        ) {
            params.emplace_back(std::make_unique<FunctionParam>("self", std::make_unique<PointerType>(std::make_unique<ReferencedType>("Vector", node)), 0, std::nullopt, this));
        }
        Value *call(InterpretScope *call_scope, FunctionCall *call, Value *parent_val) override {
            static_cast<InterpretVectorVal*>(parent_val)->values.emplace_back(call->values[0]->scope_value(*call_scope));
            return nullptr;
        }
    };
    class InterpretVectorErase : public FunctionDeclaration {
    public:
        explicit InterpretVectorErase(InterpretVectorNode* node) : FunctionDeclaration(
                "erase",
                std::vector<std::unique_ptr<FunctionParam>> {},
                std::make_unique<VoidType>(),
                false,
                node,
                std::nullopt
        ) {
            params.emplace_back(std::make_unique<FunctionParam>("self", std::make_unique<PointerType>(std::make_unique<ReferencedType>("Vector", node)), 0, std::nullopt, this));
        }
        Value *call(InterpretScope *call_scope, FunctionCall *call, Value *parent_val) override {
            auto& ref = static_cast<InterpretVectorVal*>(parent_val)->values;
            ref.erase(ref.begin() + call->values[0]->evaluated_value(*call_scope)->as_int());
            return nullptr;
        }
    };

    InterpretVectorNode::InterpretVectorNode(ASTNode* parent_node) : StructDefinition("Vector", std::nullopt, parent_node) {
        functions["constructor"] = std::make_unique<InterpretVectorConstructor>(this);
        functions["size"] = std::make_unique<InterpretVectorSize>(this);
        functions["get"] = std::make_unique<InterpretVectorGet>(this);
        functions["push"] = std::make_unique<InterpretVectorPush>(this);
        functions["erase"] = std::make_unique<InterpretVectorErase>(this);
    }

}

class InterpretPrint : public FunctionDeclaration {
public:
    std::ostringstream ostring;
    RepresentationVisitor visitor;
    explicit InterpretPrint(ASTNode* parent_node) : FunctionDeclaration(
            "print",
            std::vector<std::unique_ptr<FunctionParam>> {},
            std::make_unique<VoidType>(),
            true,
            parent_node,
            std::nullopt
    ), visitor(ostring) {
        visitor.interpret_representation = true;
    }
    Value *call(InterpretScope *call_scope, FunctionCall *call, Value *parent_val) override {
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

class InterpretStrLen : public FunctionDeclaration {
public:
    explicit InterpretStrLen(ASTNode* parent_node) : FunctionDeclaration(
            "strlen",
            std::vector<std::unique_ptr<FunctionParam>> {},
            std::make_unique<UBigIntType>(),
            false,
            parent_node,
            std::nullopt
    ) {
        annotations.emplace_back(AnnotationKind::CompTime);
    }
    Value *call(InterpretScope *call_scope, FunctionCall *call, Value *parent_val) override {
        if(call->values.empty()) {
            call_scope->error("compiler::strlen called without arguments");
            return nullptr;
        }
        const auto val = call->values[0].get();
        if(val->value_type() != ValueType::String) {
            call_scope->error("compiler::strlen called with invalid arguments");
            return nullptr;
        }
        hybrid_ptr<Value> value{nullptr, false};
        if(val->reference()) {
            auto linked = val->linked_node();
            if(linked && !linked->as_func_param()) {
                auto holding = linked->holding_value();
                if(holding) {
                    value = hybrid_ptr<Value>{holding, false};
                }
            } else {
                value = call->values[0]->evaluated_value(*call_scope);
            }
        } else {
            value = call->values[0]->evaluated_value(*call_scope);
        }
        if(!value) {
            call_scope->error("couldn't get value for compiler::strlen");
            return nullptr;
        }
        return new UBigIntValue(value->as_string().length());
    }
};

class WrapValue : public Value {
public:
    std::unique_ptr<Value> underlying;
    explicit WrapValue(std::unique_ptr<Value> underlying) : underlying(std::move(underlying)) {

    }
    void accept(Visitor *visitor) override {
        throw std::runtime_error("compiler::wrap value cannot be visited");
    }
    Value *scope_value(InterpretScope &scope) override {
        return new WrapValue(std::unique_ptr<Value>(underlying->copy()));
    }
    hybrid_ptr<Value> evaluated_value(InterpretScope &scope) override {
        return hybrid_ptr<Value> { underlying.get(), false };
    }
};

class InterpretWrap : public FunctionDeclaration {
public:
    explicit InterpretWrap(ASTNode* parent_node) : FunctionDeclaration(
            "wrap",
            std::vector<std::unique_ptr<FunctionParam>> {},
            // TODO fix return type
            std::make_unique<VoidType>(),
            true,
            parent_node,
            std::nullopt
    ) {
        params.emplace_back(std::make_unique<FunctionParam>("value", std::make_unique<AnyType>(), 0, std::nullopt, this));
    }
    Value *call(InterpretScope *call_scope, FunctionCall *call, Value *parent_val) override {
        auto underlying = call->values[0]->copy();
        underlying->evaluate_children(*call_scope);
        return new WrapValue(std::unique_ptr<Value>(underlying));
    }
};

class InterpretRetStructPtr : public FunctionDeclaration {
public:
    explicit InterpretRetStructPtr(ASTNode* parent_node) : FunctionDeclaration(
            "return_struct",
            std::vector<std::unique_ptr<FunctionParam>> {},
            // TODO fix return type
            std::make_unique<PointerType>(std::make_unique<VoidType>()),
            true,
            parent_node,
            std::nullopt
    ) {
        annotations.emplace_back(AnnotationKind::CompTime);
    }
    Value *call(InterpretScope *call_scope, FunctionCall *call, Value *parent_val) override {
        return new RetStructParamValue();
    }
};

void GlobalInterpretScope::prepare_compiler_functions(SymbolResolver& resolver) {

    global_nodes["compiler"] = std::make_unique<Namespace>("compiler", nullptr);
    auto compiler_ns = (Namespace*) global_nodes["compiler"].get();
    resolver.declare("compiler", compiler_ns);

    compiler_ns->nodes.emplace_back(new InterpretPrint(compiler_ns));
    compiler_ns->nodes.emplace_back(new InterpretStrLen(compiler_ns));
    compiler_ns->nodes.emplace_back(new InterpretWrap(compiler_ns));
    compiler_ns->nodes.emplace_back(new InterpretRetStructPtr(compiler_ns));
    compiler_ns->nodes.emplace_back(new InterpretVector::InterpretVectorNode(compiler_ns));

    compiler_ns->declare_top_level(resolver);
    compiler_ns->declare_and_link(resolver);

}