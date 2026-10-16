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
#include "ast/values/RuntimeValue.h"
#include "ast/values/RuntimeBlockValue.h"

#include <string>
#include <vector>

namespace mir {

namespace {

bool value_is_template(Value* v) {
    if (!v) return false;
    const ValueKind k = v->val_kind();
    return k == ValueKind::RuntimeValue || k == ValueKind::RuntimeBlockValue;
}

bool scope_has_template(Scope* s);

bool stmt_has_template(ASTNode* n) {
    if (!n) return false;
    switch (n->kind()) {
        case ASTNodeKind::ReturnStmt:
            return value_is_template(n->as_return()->value);
        case ASTNodeKind::IfStmt: {
            auto* ifs = n->as_if_stmt_unsafe();
            if (scope_has_template(&ifs->ifBody)) return true;
            for (auto& e : ifs->elseIfs) {
                if (scope_has_template(&e.second)) return true;
            }
            if (ifs->elseBody.has_value() && scope_has_template(&ifs->elseBody.value())) return true;
            return false;
        }
        case ASTNodeKind::Scope:
            return scope_has_template(n->as_scope());
        default:
            return false;
    }
}

bool scope_has_template(Scope* s) {
    if (!s) return false;
    for (ASTNode* n : s->nodes) {
        if (stmt_has_template(n)) return true;
    }
    return false;
}

} // namespace

bool MIRLowerer::is_comptime_template_fn(FunctionDeclaration* fd) {
    if (!fd || !fd->is_comptime() || !fd->body.has_value()) return false;
    // constructors keep the existing implicit-constructor path (it also runs
    // `comptime_ctor_eval`); inlining them crashes the compiler today (see
    // lang/docs/runtime-value-replacement.md step 2).
    if (fd->is_constructor_fn()) return false;
    for (FunctionParam* p : fd->params) {
        if (!p || !p->type) continue;
        BaseType* t = p->type->canonical();
        if (t && (t->kind() == BaseTypeKind::Runtime ||
                  t->kind() == BaseTypeKind::MaybeRuntime)) {
            return true;
        }
    }
    return scope_has_template(&fd->body.value());
}

MIRExprResult MIRLowerer::inline_comptime_template(FunctionDeclaration* fd, MIRExprResult recv,
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
    for (size_t i = 0; ok && i < fd->params.size(); ++i) {
        FunctionParam* p = fd->params[i];
        if (!p || p == callee_self) continue;
        Value* arg = (i < call_values.size()) ? call_values[i] : p->defValue;
        if (!arg) {
            error = "missing comptime template argument";
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

    auto saved_var = std::move(var_places_);
    auto saved_name = std::move(name_places_);
    auto saved_comptime = std::move(comptime_bindings_);
    FunctionParam* saved_self = current_self_param_;
    const size_t dmark = destructibles_.size();
    var_places_.clear();
    name_places_.clear();
    comptime_bindings_.clear();
    current_self_param_ = callee_self;

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
            if (agg && pt == r.type) {
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
            comptime_bindings_[std::string(p->name.data(), p->name.size())] = b.cv;
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
            ok = lower_scope_nodes(fd->body.value().nodes, error);
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

    var_places_ = std::move(saved_var);
    name_places_ = std::move(saved_name);
    comptime_bindings_ = std::move(saved_comptime);
    current_self_param_ = saved_self;
    destructibles_.resize(dmark);
    return ok ? result : MIRExprResult::error();
}

} // namespace mir
