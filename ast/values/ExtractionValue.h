// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/base/Value.h"

enum class ExtractionKind {
    LambdaFnPtr,
    LambdaCapturedPtr,
    LambdaCapturedDestructor,
    SizeOfLambdaCaptured,
    AlignOfLambdaCaptured
};

class ExtractionValue : public Value {
public:

    /**
     * the value from which to extract
     */
    Value* value;

    /**
     * the thing to extract
     */
    ExtractionKind extractionKind;

    /**
     * constructor
     */
    ExtractionValue(
        Value* value,
        ExtractionKind kind,
        SourceLocation location
    ) : Value(ValueKind::ExtractionValue, location), value(value), extractionKind(kind) {

    }

    Value* copy(ASTAllocator &allocator) override {
        return new (allocator.allocate<ExtractionValue>()) ExtractionValue(
            value->copy(allocator),
            extractionKind,
            encoded_location()
        );
    }

    BaseType* create_type(ASTAllocator &allocator) override;

};