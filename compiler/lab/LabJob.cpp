// Copyright (c) Chemical Language Foundation 2025.

#include "LabJob.h"
#include "ast/utils/IffyConditional.h"
#include <optional>

std::optional<bool> is_condition_enabled(TargetData& target_data, const chem::string_view& name) {
    constexpr auto hasherFn = std::hash<chem::string_view>();
    switch(hasherFn(name)) {
        case hasherFn("tcc"):
            return target_data.tcc;
        case hasherFn("clang"):
            return target_data.clang;
        case hasherFn("cbi"):
            return target_data.cbi;
        case hasherFn("lsp"):
            return target_data.lsp;
        case hasherFn("test"):
            return target_data.test;
        case hasherFn("debug"):
            return target_data.debug;
        case hasherFn("debug_quick"):
            return target_data.debug_quick;
        case hasherFn("debug_complete"):
            return target_data.debug_complete;
        case hasherFn("release"):
            return target_data.release;
        case hasherFn("release_safe"):
            return target_data.release_safe;
        case hasherFn("release_small"):
            return target_data.release_small;
        case hasherFn("release_fast"):
            return target_data.release_fast;
        case hasherFn("posix"):
            return target_data.posix;
        case hasherFn("gnu"):
            return target_data.gnu;
        case hasherFn("is64Bit"):
            return target_data.is64Bit;
        case hasherFn("little_endian"):
            return target_data.little_endian;
        case hasherFn("big_endian"):
            return target_data.big_endian;
        case hasherFn("windows"):
            return target_data.windows;
        case hasherFn("win32"):
            return target_data.win32;
        case hasherFn("win64"):
            return target_data.win64;
        case hasherFn("linux"):
            return target_data.isLinux;
        case hasherFn("isLinux"):
            return target_data.isLinux;
        case hasherFn("macos"):
            return target_data.macos;
        case hasherFn("freebsd"):
            return target_data.freebsd;
        case hasherFn("isUnix"):
            return target_data.isUnix;
        case hasherFn("android"):
            return target_data.android;
        case hasherFn("cygwin"):
            return target_data.cygwin;
        case hasherFn("mingw32"):
            return target_data.mingw32;
        case hasherFn("mingw64"):
            return target_data.mingw64;
        case hasherFn("emscripten"):
            return target_data.emscripten;
        case hasherFn("x86_64"):
            return target_data.x86_64;
        case hasherFn("x86"):
            return target_data.x86;
        case hasherFn("i386"):
            return target_data.i386;
        case hasherFn("arm"):
            return target_data.arm;
        case hasherFn("aarch64"):
            return target_data.aarch64;
        case hasherFn("powerpc"):
            return target_data.powerpc;
        case hasherFn("powerpc64"):
            return target_data.powerpc64;
        case hasherFn("riscv"):
            return target_data.riscv;
        case hasherFn("s390x"):
            return target_data.s390x;
        case hasherFn("wasm32"):
            return target_data.wasm32;
        case hasherFn("wasm64"):
            return target_data.wasm64;
        default:
            return std::nullopt;
    }
}

std::optional<bool> resolve_target_condition(TargetData& data, IffyBase* base) {
    if(base == nullptr) return std::nullopt;
    if(base->is_id) {
        const auto if_id = (IffyCondId*) base;
        auto value = is_condition_enabled(data, if_id->value);
        if(value.has_value()) {
            if(if_id->is_negative) {
                return !value.value();
            }
        }
        return value;
    } else {
        const auto if_expr = (IffyCondExpr*) base;
        auto value = resolve_target_condition(data, if_expr->left);
        if(value.has_value()) {
            if(value.value() && if_expr->op == IffyExprOp::Or) {
                // value is true, in or expression, we do not need to resolve second
                return true;
            } else if(!value.value() && if_expr->op == IffyExprOp::And) {
                // value is false, in and expression, we do not need to resolve second
                return false;
            } else {
                // in 'and', first is true, depends on second (true if second is true, false if second is false)
                // in 'or' , first is false, depends on second (true if second is true, false if second is false)
                return resolve_target_condition(data, if_expr->right);
            }
        }
    }
    return std::nullopt;
}