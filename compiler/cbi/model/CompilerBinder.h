// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "CBIData.h"
#include "compiler/cbi/model/Model.h"
#include <unordered_map>
#include "std/chem_string_view.h"
#include "std/unordered_map.h"
#include "compiler/cbi/bindings/CBI.h"
#include "CBIFunctionIndex.h"

class ASTProcessor;

class Parser;

class CSTToken;

struct CBIFunctionKey {
    chem::string_view key;
    CBIFunctionType type;
    bool operator==(const CBIFunctionKey& other) const noexcept {
        return key == other.key && type == other.type;
    }
};

// Custom hash function for HookKey
struct CBIFunctionHash {
    size_t operator()(const CBIFunctionKey& k) const noexcept {
        return std::hash<chem::string_view>{}(k.key) ^ (size_t(k.type) << 1);
    }
};

/**
 * compiler binder based on tiny c compiler
 */
class CompilerBinder {

    /**
     * contains a map between cbi_name and module data
     */
    util::unordered_string_map<CBIData> data;

public:

    /**
     * all the functions user has asked us to hook
     */
    std::unordered_map<CBIFunctionKey, void*, CBIFunctionHash> hooks_;

    /**
     * a map between interface names like Lexer, SourceProvider and their actual symbols
     * these symbols correspond to function pointers in the our source code
     */
    std::unordered_map<chem::string_view, std::span<const std::pair<chem::string_view, void*>>> interface_maps;

    /**
     * path to current executable, resources required by tcc are located relative to it
     */
    std::string exe_path;

    /**
     * constructor
     */
    explicit CompilerBinder(std::string exe_path);

    /**
     * get the built cbi map
     */
    inline const util::unordered_string_map<CBIData>& get_cbi_map() {
        return data;
    }

    /**
     * check whether this cbi exists already
     */
    inline bool contains_cbi(const std::string_view& name) {
        return data.contains(name);
    }

    /**
     * cbi by this name is stored and
     * protects from overriding existing cbi
     */
    bool store_cbi(std::string name, TCCState* state) {
        if (data.contains(name)) return false;
        data[std::move(name)] = {state};
        return true;
    }

    /**
     * imports the given compiler interfaces
     */
    static void import_compiler_interface(const std::span<const std::pair<chem::string_view, void*>>& interface, TCCState* state);

    /**
     * index a function with given cbi function type and key
     */
    inline void registerHook(CBIFunctionType type, const chem::string_view& key, void* function) {
        hooks_.emplace(CBIFunctionKey{key, type}, function);
    }

    /**
     * find a function
     */
    [[nodiscard]]
    void* findHook(const chem::string_view& key, CBIFunctionType type) const noexcept {
        auto it = hooks_.find(CBIFunctionKey{key, type});
        return it != hooks_.end() ? it->second : nullptr;
    }

    /**
     * indexes the given function
     */
    const char* index_function(CBIFunctionIndex& index, TCCState* state);

    /**
     * a destructor is used to destruct the TCC state
     */
    ~CompilerBinder();

};