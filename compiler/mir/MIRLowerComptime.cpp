// Copyright (c) Chemical Language Foundation 2025.
//
// Comptime-template inlining (Phase 5 of the interpretation plan; see
// lang/docs/runtime-value-replacement.md).
//
// A `comptime func` whose body returns a runtime template (`%runtime_value` /
// `%runtime_block_value`) or which takes `%runtime` / `%maybe_runtime` / `self`
// operands is *code shaping*, not value computation. Today the AST interpreter
// runs it to build an AST template; this file lowers the body directly instead:
// comptime operands are folded to constants, runtime operands are bound to the
// caller's argument results, and the body's `return` becomes the call's value.
// The folded comptime parameters are also published (by name) so a nested
// comptime intrinsic inside the body folds with them visible — the compiler
// callbacks push `comptime_binding_names()` into the interpret scope.

#include "MIRLowerer.h"
#include "MIRLowererInternal.h"
#include "MIRLowererHelpers.h"

#include "MIRBuilder.h"
#include "MIRABI.h"

#include "ast/structures/FunctionDeclaration.h"
#include "ast/structures/FunctionParam.h"
#include "ast/structures/Scope.h"
#include "ast/structures/If.h"
#include "ast/statements/Return.h"
#include "ast/types/RuntimeType.h"
#include "ast/types/MaybeRuntimeType.h"

#include <string>
#include <vector>

namespace mir {

bool MIRLowerer::is_comptime_template_fn(FunctionDeclaration* fd) {
    if (!fd || !fd->is_comptime() || !fd->body.has_value()) return false;
    // The explicit, declared form: `@expand comptime func`. The compiler never
    // inspects the body to decide.
    if (fd->is_expand_fn()) return true;
    // A marker parameter also makes a comptime function a template:
    // `%runtime<T>` / `%maybe_runtime<T>` (a runtime operand) or `%literal`
    // (a comptime operand). Inspect the RAW kind — `BaseType::canonical()`
    // deliberately unwraps exactly these markers (see BaseType.cpp).
    for (FunctionParam* p : fd->params) {
        if (!p || !p->type) continue;
        const BaseTypeKind k = p->type->kind();
        if (k == BaseTypeKind::Runtime || k == BaseTypeKind::MaybeRuntime ||
            k == BaseTypeKind::Literal) {
            return true;
        }
    }
    return false;
}

void MIRLowerer::set_comptime_binding(const std::string& key, Value* value) {
    if (scope_journal_) {
        ScopeUndo u;
        u.kind = 2;
        u.name = key;
        auto it = comptime_bindings_.find(key);
        if (it != comptime_bindings_.end()) { u.existed = true; u.cval = it->second; }
        scope_undo_.push_back(std::move(u));
    }
    comptime_bindings_[key] = value;
}

void MIRLowerer::undo_scope_from(size_t mark) {
    while (scope_undo_.size() > mark) {
        ScopeUndo u = std::move(scope_undo_.back());
        scope_undo_.pop_back();
        switch (u.kind) {
            case 0:
                if (u.existed) var_places_[u.node] = u.place;
                else var_places_.erase(u.node);
                break;
            case 1:
                if (u.existed) name_places_[u.name] = u.place;
                else name_places_.erase(u.name);
                break;
            case 2:
                if (u.existed) comptime_bindings_[u.name] = u.cval;
                else comptime_bindings_.erase(u.name);
                break;
            default: break;
        }
    }
}

MIRExprResult MIRLowerer::inline_comptime_template(FunctionDeclaration* fd, MIRExprResult recv,
                                                   FunctionCall* call,
                                                   const std::vector<Value*>& call_values,
                                                   std::string& error) {
    if (!fd || !fd->body.has_value()) {
        error = "comptime template has no body";
        return MIRExprResult::error();
    }
    const TypeId ret = fd->returnType ? types_.map(fd->returnType) : types_.void_type();
    const bool is_void = is_void_type(module_, ret);
    const bool sret = !is_void && abi::needs_sret(module_, ret);
    FunctionParam* callee_self = fd->has_self_param() ? fd->get_self_param() : nullptr;

    // Evaluate every argument in the CALLER's context first — its binding state
    // (name_places_/var_places_) is still live, and the arguments are caller
    // expressions. Then swap to the callee's bindings for the body.
    struct BoundArg {
        bool runtime = false;
        MIRExprResult r{};
        Value* cv = nullptr;
    };
    std::vector<BoundArg> bounds(fd->params.size());
    bool ok = true;
    // `fd->params` includes the receiver but `call_values` does not, so the
    // argument index must skip the receiver slot rather than reuse the param
    // index.
    size_t argi = 0;
    for (size_t i = 0; ok && i < fd->params.size(); ++i) {
        FunctionParam* p = fd->params[i];
        if (!p || p == callee_self) continue;
        Value* arg = (argi < call_values.size()) ? call_values[argi] : p->defValue;
        ++argi;
        if (!arg) {
            error = "missing comptime template argument '" +
                    std::string(p->name.data(), p->name.size()) + "' for '" +
                    fd->name_str() + "' (i=" + std::to_string(i) +
                    " params=" + std::to_string(fd->params.size()) +
                    " values=" + std::to_string(call_values.size()) +
                    " self=" + (callee_self ? "y" : "n") + ")";
            ok = false;
            break;
        }
        BaseType* dtyp = p->type;
        const bool is_runtime = dtyp && dtyp->kind() == BaseTypeKind::Runtime;
        const bool is_maybe = dtyp && dtyp->kind() == BaseTypeKind::MaybeRuntime;
        // `%runtime<T>` / `%maybe_runtime<T>` are markers; lower the operand as
        // its underlying type so the argument conversion sees a plain type.
        BaseType* effective = dtyp;
        if (is_runtime) effective = dtyp->as_runtime_type_unsafe()->underlying;
        else if (is_maybe) effective = dtyp->as_maybe_runtime_type_unsafe()->underlying;
        BoundArg b;
        if (is_runtime) {
            b.runtime = true;
            b.r = lower_arg_converted(arg, effective, error);
            if (!b.r.ok()) { ok = false; break; }
        } else {
            Value* cv = arg;
            if (comptime_value_eval_) {
                if (Value* e = comptime_value_eval_(arg)) cv = e;
            }
            if (is_maybe && cv && cv->val_kind() == ValueKind::Identifier) {
                // a `%maybe_runtime` argument that stays a plain identifier is a
                // runtime operand; bind it like one.
                b.runtime = true;
                b.r = lower_arg_converted(arg, effective, error);
                if (!b.r.ok()) { ok = false; break; }
            } else {
                b.cv = cv;
            }
        }
        bounds[i] = b;
    }
    if (!ok) return MIRExprResult::error();

    FunctionParam* saved_self = current_self_param_;
    const bool saved_async = async_;
    const size_t dmark = destructibles_.size();
    // Splice: layer the callee's bindings over the caller's (do NOT clear them),
    // so the body is lowered in the caller's scope — a call-site macro, not a
    // hygienic inline. The journal records every added binding so the caller's
    // scope is restored exactly afterwards.
    const bool saved_journal = scope_journal_;
    scope_journal_ = true;
    const size_t umark = scope_undo_.size();
    current_self_param_ = callee_self;
    // an @expand function is a synchronous `comptime func`: its `return` is an
    // inline-return, even when spliced into an async caller (otherwise it would
    // be treated as an async return and complete the caller early)
    async_ = false;

    // Bind a runtime operand into a fresh place of the parameter's type.
    auto bind_runtime = [&](FunctionParam* p, MIRExprResult r) -> bool {
        TypeId pt = p->type ? types_.map(p->type) : r.type;
        if (pt == MIR_INVALID_ID) pt = r.type;
        if (pt == MIR_INVALID_ID) return false;
        const bool agg = abi::is_aggregate(module_, pt);
        PlaceId place = builder_->alloca(pt, MIRStorageClass::Temporary);
        if (place == MIR_INVALID_ID) return false;
        if (r.kind == MIRExprKind::Place) {
            const PlaceId sp = static_cast<PlaceId>(r.id);
            // a reference (`&mut self`) or pointer parameter receives the
            // *address* of the argument place; the value is already indirect
            BaseType* dtyp = p->type;
            BaseType* eff = dtyp;
            if (dtyp && dtyp->kind() == BaseTypeKind::Runtime) {
                eff = dtyp->as_runtime_type_unsafe()->underlying;
            } else if (dtyp && dtyp->kind() == BaseTypeKind::MaybeRuntime) {
                eff = dtyp->as_maybe_runtime_type_unsafe()->underlying;
            }
            const bool ref_param = eff && (eff->kind() == BaseTypeKind::Reference ||
                                           eff->kind() == BaseTypeKind::Pointer);
            if (ref_param && r.type != pt) {
                builder_->store(place, builder_->address_of(sp, pt));
            } else if (agg && pt == r.type) {
                builder_->move_init(place, sp, pt);
            } else {
                builder_->store(place, builder_->load(sp, r.type));
            }
        } else if (r.kind == MIRExprKind::Address) {
            const ValueId av = static_cast<ValueId>(r.id);
            if (agg) {
                builder_->copy_init(place, builder_->load_indirect(av, pt), pt);
            } else {
                builder_->store(place, av);
            }
        } else if (r.kind == MIRExprKind::Value) {
            const ValueId vv = static_cast<ValueId>(r.id);
            if (agg) {
                builder_->copy_init(place, vv, pt);
            } else {
                builder_->store(place, vv);
            }
        } else {
            return false;
        }
        bind(p, place);
        bind_name(p->name, place);
        return true;
    };

    if (callee_self && ok) {
        if (recv.kind != MIRExprKind::Place && recv.kind != MIRExprKind::Value &&
            recv.kind != MIRExprKind::Address) {
            error = "comptime template receiver is not bindable";
            ok = false;
        } else if (!bind_runtime(callee_self, recv)) {
            error = "comptime template receiver is not bindable";
            ok = false;
        }
    }

    for (size_t i = 0; ok && i < fd->params.size(); ++i) {
        FunctionParam* p = fd->params[i];
        if (!p || p == callee_self) continue;
        BoundArg& b = bounds[i];
        if (b.runtime) {
            if (!bind_runtime(p, b.r)) {
                error = "comptime template argument is not bindable";
                ok = false;
                break;
            }
        } else {
            set_comptime_binding(std::string(p->name.data(), p->name.size()), b.cv);
        }
    }

    MIRExprResult result = MIRExprResult::void_result();
    if (ok) {
        ComptimeInlineFrame frame;
        frame.end_block = builder_->create_block();
        frame.result_type = ret;
        frame.void_result = is_void;
        frame.dmark = dmark;
        if (!is_void) {
            frame.result_place = builder_->alloca(ret, MIRStorageClass::Temporary);
            if (frame.result_place == MIR_INVALID_ID) {
                error = "failed to allocate comptime template result";
                ok = false;
            }
        }
        if (ok) {
            inline_frames_.push_back(frame);
            // establish the call site so call-site introspection intrinsics
            // (intrinsics::get_call_loc, …) resolve to this call, exactly as the
            // interpreter's frame would
            if (comptime_call_site_ && call) comptime_call_site_(call, true);
            ok = lower_scope_nodes(fd->body.value().nodes, error);
            if (comptime_call_site_ && call) comptime_call_site_(call, false);
            if (ok && !builder_->current_block_terminated()) {
                emit_drops_from(dmark);
                builder_->br(frame.end_block);
            }
            inline_frames_.pop_back();
            if (ok) {
                builder_->set_block(frame.end_block);
                if (is_void) {
                    result = MIRExprResult::void_result();
                } else if (sret) {
                    result = MIRExprResult::place(frame.result_place, ret);
                } else {
                    result = MIRExprResult::value(builder_->load(frame.result_place, ret), ret);
                }
            }
        }
    }

    scope_journal_ = saved_journal;
    undo_scope_from(umark);
    current_self_param_ = saved_self;
    async_ = saved_async;
    destructibles_.resize(dmark);
    return ok ? result : MIRExprResult::error();
}

} // namespace mir
