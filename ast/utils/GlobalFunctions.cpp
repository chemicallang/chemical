// Copyright (c) Qinetik 2024.

#include "GlobalFunctions.h"

#include "sstream"
#include <utility>
#include "ast/types/VoidType.h"
#include "ast/values/IntValue.h"
#include "ast/values/StructValue.h"
#include "compiler/SymbolResolver.h"
#include "ast/structures/Namespace.h"
#include "ast/structures/StructDefinition.h"
#include "ast/structures/FunctionParam.h"
#include "ast/types/PointerType.h"
#include "ast/types/ReferencedType.h"
#include "preprocess/RepresentationVisitor.h"

namespace InterpretVector {

    class InterpretVectorNode;

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
        InterpretVectorNode* node;
        explicit InterpretVectorConstructor(InterpretVectorNode* node) : node(node), FunctionDeclaration(
                "constructor",
                std::vector<std::unique_ptr<FunctionParam>> {},
                std::make_unique<ReferencedType>("Vector", node),
                false,
                std::nullopt
        ) {
            annotations.emplace_back(AnnotationKind::Constructor);
        }
        Value *call(InterpretScope *call_scope, std::vector<std::unique_ptr<Value>> &call_args, Value *parent_val, InterpretScope *fn_scope) override {
            return new InterpretVectorVal(node);
        }
    };

    class InterpretVectorSize : public FunctionDeclaration {
    public:
        explicit InterpretVectorSize(InterpretVectorNode* node) : FunctionDeclaration(
            "size",
            std::vector<std::unique_ptr<FunctionParam>> {},
            std::make_unique<IntType>(),
            false,
            std::nullopt
        ) {
            params.emplace_back(std::make_unique<FunctionParam>("self", std::make_unique<PointerType>(std::make_unique<ReferencedType>("Vector", (ASTNode*) node)), 0, std::nullopt, this));
        }
        Value *call(InterpretScope *call_scope, std::vector<std::unique_ptr<Value>> &call_args, Value *parent_val, InterpretScope *fn_scope) override {
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
                std::nullopt
        ) {
            params.emplace_back(std::make_unique<FunctionParam>("self", std::make_unique<PointerType>(std::make_unique<ReferencedType>("Vector", (ASTNode*) node)), 0, std::nullopt, this));
        }
        Value *call(InterpretScope *call_scope, std::vector<std::unique_ptr<Value>> &call_args, Value *parent_val, InterpretScope *fn_scope) override {
            return static_cast<InterpretVectorVal*>(parent_val)->values[call_args[0]->evaluated_value(*call_scope)->as_int()]->scope_value(*call_scope);
        }
    };
    class InterpretVectorPush : public FunctionDeclaration {
    public:
        explicit InterpretVectorPush(InterpretVectorNode* node) : FunctionDeclaration(
                "push",
                std::vector<std::unique_ptr<FunctionParam>> {},
                std::make_unique<VoidType>(),
                false,
                std::nullopt
        ) {
            params.emplace_back(std::make_unique<FunctionParam>("self", std::make_unique<PointerType>(std::make_unique<ReferencedType>("Vector", (ASTNode*) node)), 0, std::nullopt, this));
        }
        Value *call(InterpretScope *call_scope, std::vector<std::unique_ptr<Value>> &call_args, Value *parent_val, InterpretScope *fn_scope) override {
            static_cast<InterpretVectorVal*>(parent_val)->values.emplace_back(call_args[0]->scope_value(*call_scope));
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
                std::nullopt
        ) {
            params.emplace_back(std::make_unique<FunctionParam>("self", std::make_unique<PointerType>(std::make_unique<ReferencedType>("Vector", (ASTNode*) node)), 0, std::nullopt, this));
        }
        Value *call(InterpretScope *call_scope, std::vector<std::unique_ptr<Value>> &call_args, Value *parent_val, InterpretScope *fn_scope) override {
            auto& ref = static_cast<InterpretVectorVal*>(parent_val)->values;
            ref.erase(ref.begin() + call_args[0]->evaluated_value(*call_scope)->as_int());
            return nullptr;
        }
    };

    class InterpretVectorNode : public StructDefinition {
    public:
        InterpretVectorNode() : StructDefinition("Vector", std::nullopt) {
            functions["constructor"] = std::make_unique<InterpretVectorConstructor>(this);
            functions["size"] = std::make_unique<InterpretVectorSize>(this);
            functions["get"] = std::make_unique<InterpretVectorGet>(this);
            functions["push"] = std::make_unique<InterpretVectorPush>(this);
            functions["erase"] = std::make_unique<InterpretVectorErase>(this);
        }
    };

}

class InterpretPrint : public FunctionDeclaration {
public:
    std::ostringstream ostring;
    RepresentationVisitor visitor;
    explicit InterpretPrint() : FunctionDeclaration(
            "print",
            std::vector<std::unique_ptr<FunctionParam>> {},
            std::make_unique<VoidType>(),
            true,
            std::nullopt
    ), visitor(ostring) {
        visitor.interpret_representation = true;
    }
    Value *call(InterpretScope *call_scope, std::vector<std::unique_ptr<Value>> &call_args, Value *parent_val, InterpretScope *fn_scope) override {
        ostring.str("");
        ostring.clear();
        for (auto const &value: call_args) {
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

void GlobalInterpretScope::prepare_compiler_functions(SymbolResolver& resolver) {

    global_nodes["compiler"] = std::make_unique<Namespace>("compiler");
    auto compiler_ns = (Namespace*) global_nodes["compiler"].get();
    resolver.declare("compiler", compiler_ns);

    compiler_ns->nodes.emplace_back(new InterpretPrint());
    compiler_ns->nodes.emplace_back(new InterpretVector::InterpretVectorNode());

    compiler_ns->declare_top_level(resolver);
    compiler_ns->declare_and_link(resolver);

}