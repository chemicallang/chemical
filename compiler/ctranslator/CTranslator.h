// Copyright (c) Qinetik 2024.

#pragma once

#include <unordered_map>
#include <memory>
#include <string>
#include "ast/base/BaseType.h"
#include "compiler/clangfwd.h"

/**
 * when provided a c type string like char, float, long long
 * it'll make a chemical type
 */
typedef BaseType*(*CTypeMakerFn)(clang::BuiltinType*);

using TypeMakersMap = std::unordered_map<int, CTypeMakerFn>;

struct CTranslatorError {
    std::string message;
};

class CTranslator {
public:

    /**
     * errors that occurred during translation
     */
    std::vector<CTranslatorError> errors;

    /**
     * type makers function map
     */
    TypeMakersMap type_makers;

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