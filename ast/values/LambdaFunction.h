// Copyright (c) Chemical Language Foundation 2025.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include "ast/base/Value.h"
#include "ast/base/InterpretScope.h"
#include "ast/structures/Scope.h"
#include "ast/types/FunctionType.h"
#include "std/except.h"
#include <vector>

class FunctionDeclaration;

class CapturedVariable;

class LambdaFunctionCopy : public Value {
public:
    using Value::Value;
    Value* copy(ASTAllocator &allocator) override;
};

/**
 * @brief Class representing an integer value.
 */
class LambdaFunction : public LambdaFunctionCopy, public FunctionTypeBody {
public:

    std::vector<CapturedVariable*> captureList;
    Scope scope;

    /**
     * A type-level witness for the generated capture struct. It contains one
     * field per captured binding (by-reference captures use their reference
     * type), so Send/Sync/'static derivation can inspect what the closure
     * actually owns. Null means it has not been built yet (or the lambda has
     * no captures and the caller did not request a witness).
     */
    TypeLoc capture_env = nullptr;

    /** The enclosing context declared this lambda as a `FnOnce` callable. */
    bool capture_target_once = false;


    /**
     * constructor
     */
    constexpr LambdaFunction(
            bool isVariadic,
            ASTNode* parent_node,
            SourceLocation location
    ) : LambdaFunctionCopy(ValueKind::LambdaFunc, this, location), FunctionTypeBody(nullptr, isVariadic, false, false), scope(parent_node, location) {

    }

    ASTNode* get_parent() final {
        return scope.parent();
    }

    SourceLocation get_location() final {
        return encoded_location();
    }

    LambdaFunction* as_lambda() final {
        return this;
    }

    int data_struct_index() {
        return has_self_param() ? 1 : 0;
    }


    /**
     * should generate a destructor for capture struct
     */
    bool has_destructor_for_capture();

    Value* call(InterpretScope* call_scope, ASTAllocator& func_allocator, Value* parent, std::vector<Value*>& call_args);

};