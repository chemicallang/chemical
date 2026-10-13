// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/base/Value.h"

enum class ExtractionKind {
    LambdaFnPtr,
    LambdaCapturedPtr,
    LambdaCapturedDestructor,
    SizeOfLambdaCaptured,
    AlignOfLambdaCaptured,
    ReinterpretLLVMValue,
};

class TypeBuilder;

BaseType* create_extraction_value_type(TypeBuilder& builder, ExtractionKind kind);

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
            BaseType* type,
            ExtractionKind kind,
            SourceLocation location
    ) : Value(ValueKind::ExtractionValue, type, location), value(value), extractionKind(kind) {

    }

    Value* copy(ASTAllocator &allocator) override {
        return new (allocator.allocate<ExtractionValue>()) ExtractionValue(
            value->copy(allocator),
            getType(),
            extractionKind,
            encoded_location()
        );
    }


};