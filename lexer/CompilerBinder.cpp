// Copyright (c) Qinetik 2024.

#include <sstream>
#include <utility>
#include "lexer/model/CompilerBinder.h"
#include "lexer/model/CompilerBinderTCC.h"
#include "utils/PathUtils.h"
#include "integration/libtcc/LibTccInteg.h"

void handle_error(void *opaque, const char *msg){
    const auto binder = (CompilerBinderTCC*) opaque;
    binder->diagnostics.emplace_back(msg);
}

CompilerBinder::CompilerBinder() {

}

CompilerBinderTCC::CompilerBinderTCC(std::string exe_path) : CompilerBinder(), exe_path(std::move(exe_path)) {

}

BinderResult CompilerBinderTCC::compile(
    const std::string& cbi_name,
    const std::string& program,
    CBIData& cbiData
) {
    auto found = compiled.find(cbi_name);
    if(found != compiled.end()) {
        return BinderResult { 1, "cbi has already been compiled " + cbi_name };
    }
    auto state = tcc_new();
    if(!state) {
        return BinderResult { 1, "couldn't initialize tcc state in tcc compiler binder" };
    }
    tcc_set_error_func(state, this, handle_error);
    auto tcc_dir = resolve_non_canon_parent_path(exe_path, "packages/tcc");
    auto include_dir = resolve_rel_child_path_str(tcc_dir, "include");
    auto lib_dir = resolve_rel_child_path_str(tcc_dir, "lib");
    int result;
    result = tcc_add_include_path(state, include_dir.c_str());
    if(result == -1) {
        return BinderResult { 1, "couldn't add include path 'packages/tcc/include' in tcc compiler binder" };
    }
    result = tcc_add_library_path(state, lib_dir.c_str());
    if(result == -1) {
        return BinderResult { 1, "couldn't add library path 'packages/tcc/lib' in tcc compiler binder" };
    }
    result = tcc_set_output_type(state, TCC_OUTPUT_MEMORY);
    if(result == -1) {
        return BinderResult { 1, "couldn't set tcc output memory in tcc compiler binder" };
    }

    // compile
    result = tcc_compile_string(state, program.c_str());
    if(result == -1) {
        return BinderResult { 1, "couldn't compile c code in binder for cbi " + cbi_name };
    }

    // add functions like malloc and free
    prepare_tcc_state_for_jit(state);

    // any other functions user require, he would mention by including cbi types
    // in that case, compiler will expose symbols that correspond to that type
    for(auto& cbiType : cbiData.cbiTypes) {
        switch(cbiType.kind) {
            case CBIImportKind::Lexer:
                // TODO lexer functions should be declared here
                break;
        }
    }


    // relocate the code
    result = tcc_relocate(state);
    if(result == -1) {
        return BinderResult { 1, "couldn't relocate c code in binder for cbi " + cbi_name };
    }

    compiled[cbi_name] = state;
    return BinderResult { 0, "" };
}

void* CompilerBinderTCC::provide_func(const std::string& cbi_name, const std::string& funcName) {
    auto complete_cached_name = cbi_name + ':' + funcName;
    auto found = cached_func.find(complete_cached_name);
    if(found != cached_func.end()) {
        return found->second;
    } else {
        auto cbi = compiled.find(cbi_name);
        if(cbi != compiled.end()) {
            auto sym = tcc_get_symbol(cbi->second, funcName.c_str());
            if(sym) {
                cached_func[complete_cached_name] = sym;
            }
            return sym;
        } else {
            return nullptr;
        }
    }
}

CompilerBinderTCC::~CompilerBinderTCC() {
    for(auto& unit : compiled) {
        tcc_delete(unit.second);
    }
}