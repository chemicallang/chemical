// Copyright (c) Qinetik 2024.

#pragma once

#include <unordered_map>
#include <memory>
#include <string>
#include "ast/base/BaseType.h"
#include "compiler/clangfwd.h"
#include "compiler/chem_clang.h"

/**
 * when provided a clang builtin type pointer
 * it'll make a chemical type
 *
 * this expects a builtin type of the same kind, which is being used to index it on vector type_makers
 */
typedef BaseType*(*CTypeMakerFn)(clang::BuiltinType*);

/**
 * a simple struct to represent errors during translation
 * we'll expand this later
 */
struct CTranslatorError {
    std::string message;
};

/**
 * The point of this class to provide storage for translation process
 * for example storage for indexed types, errors and stuff
 */
class CTranslator {
public:

    /**
     * errors that occurred during translation
     */
    std::vector<CTranslatorError> errors;

    /**
     * type makers functions vector
     * enum BuiltinType::Kind, this enum is used as an index on this vector
     * last enum entry is being used as size of the vector: ZigClangBuiltinTypeOMPIterator
     */
    std::vector<CTypeMakerFn> type_makers = std::vector<CTypeMakerFn>(ZigClangBuiltinTypeOMPIterator + 1);

    /**
     * constructor
     */
    CTranslator();

    /**
     * initializes type makers
     */
    void init_type_makers();

    /**
     * put an error in errors, called when an error occurs during translation
     */
    void error(const std::string& err);

};