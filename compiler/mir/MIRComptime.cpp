// Copyright (c) Chemical Language Foundation 2025.
//
// See MIRComptime.h and lang/docs/mir-interpretation-plan.md §5.3.

#include "MIRComptime.h"

#include "MIRModule.h"
#include "MIRTypeBuilder.h"
#include "MIRTaskContext.h"
#include "MIRInterpreter.h"
#include "MIRLowerer.h"
#include "MIRABI.h"
#include "MIRLayout.h"

#include "ast/structures/FunctionDeclaration.h"
#include "ast/structures/FunctionParam.h"
#include "ast/structures/Scope.h"
#include "ast/structures/If.h"
#include "ast/statements/Return.h"
#include "ast/statements/VarInit.h"
#include "ast/statements/Assignment.h"
#include "ast/values/FunctionCall.h"
#include "ast/values/AccessChain.h"
#include "ast/values/CastedValue.h"
#include "ast/values/Expression.h"
#include "ast/values/DereferenceValue.h"
#include "ast/values/NotValue.h"
#include "ast/values/Negative.h"
#include "ast/values/BitwiseNot.h"
#include "ast/values/UnsafeValue.h"
#include "ast/values/AddrOfValue.h"
#include "ast/values/ReferenceOfValue.h"
#include "ast/values/IndexOperator.h"
#include "ast/values/IsValue.h"
#include "ast/values/BlockValue.h"
#include "ast/values/ArrayValue.h"
#include "ast/values/StructValue.h"
#include "ast/values/IntNumValue.h"
#include "ast/values/BoolValue.h"
#include "ast/values/StringValue.h"
#include "ast/values/NullValue.h"
#include "ast/values/FloatValue.h"
#include "ast/values/DoubleValue.h"
#include "ast/values/StructMemberInitializer.h"
#include "ast/types/IntNType.h"
#include "ast/base/TypeBuilder.h"
#include "ast/base/ASTAllocator.h"
#include "ast/base/InterpretScope.h"

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <unordered_set>
#include <vector>

namespace mir {

struct MIRComptime::Impl {
    MIRModule module;
    MIRTypeBuilder types;
    std::unique_ptr<MIRTaskContext> task;
    std::unique_ptr<MIRInterpreter> interp;
    std::unordered_map<FunctionDeclaration*, SymbolId> lowered;
    bool dirty = false;
    bool mangler_set = false;

    Impl() : module(), types(module), task(std::make_unique<MIRTaskContext>(module, types)) {}
};

MIRComptime::MIRComptime() : impl_(new Impl()) {}
MIRComptime::~MIRComptime() = default;

unsigned MIRComptime::lowered_function_count() const {
    return static_cast<unsigned>(impl_->module.functions.size());
}

namespace {

/** A scalar return type the driver can bridge back to an AST value. */
bool scalar_return(BaseType* canonical) {
    if (!canonical) return false;
    switch (canonical->kind()) {
        case BaseTypeKind::IntN:
        case BaseTypeKind::Bool:
        case BaseTypeKind::Pointer:
        case BaseTypeKind::Reference:
        case BaseTypeKind::NullPtr:
        case BaseTypeKind::Struct:
        case BaseTypeKind::Union:
        case BaseTypeKind::Void:
            return true;
        case BaseTypeKind::Linked: {
            // a self-referencing struct type canonicalizes to its Linked form
            ASTNode* n = canonical->get_direct_linked_canonical_node();
            return n && (n->kind() == ASTNodeKind::StructDecl ||
                         n->kind() == ASTNodeKind::UnionDecl ||
                         n->kind() == ASTNodeKind::VariantDecl);
        }
        default:
            return false;
    }
}

bool is_self_param(FunctionDeclaration* fd, FunctionParam* p) {
    return fd->has_self_param() && fd->get_self_param() == p;
}

// ── eligibility: can the MIR engine reproduce this function's result? ───────
// The AST interpreter has its own semantics for a few things the engine does
// not: call-site/location intrinsics, `is_comptime`/`is_runtime`,
// `intrinsics::vector`, and string `+`/comparison. Rather than (silently) change
// those results, the driver declines any function that touches them.
/**
 * Intrinsics whose *interpreter* semantics the MIR engine does not reproduce:
 * call-site/location introspection, comptime-mode detection, the
 * interpreter-only `vector`, and expressive-string materialization.
 */
bool intrinsic_diverges(FunctionDeclaration* fd) {
    const chem::string_view name = fd->name_view();
    static const char* kDivergent[] = {
        "get_call_loc", "get_line_no", "get_char_no", "get_caller_line_no",
        "get_caller_char_no", "get_loc_file_path", "get_raw_location",
        "get_current_file_path", "expr_str_block_value", "vector",
    };
    for (const char* n : kDivergent) {
        if (name == chem::string_view(n)) return true;
    }
    // the interpreter-only `intrinsics::vector<T>` type: its constructor is
    // `constructor` and its methods `size`/`push`/`get`/`remove`, so identify it
    // by the owning definition's name rather than the (common) method names
    ASTNode* par = fd->get_parent();
    if (par && par->kind() == ASTNodeKind::StructDecl &&
        par->as_struct_def_unsafe()->name_view() == chem::string_view("vector")) {
        return true;
    }
    return false;
}

struct Eligibility {
    bool ok = true;
    std::unordered_set<FunctionDeclaration*> callees;
};

bool value_eligible(Value* v, Eligibility& e);
bool scope_eligible(Scope* s, Eligibility& e);

bool stmt_eligible(ASTNode* n, Eligibility& e) {
    if (!n) return true;
    switch (n->kind()) {
        case ASTNodeKind::ReturnStmt:
            return value_eligible(n->as_return()->value, e);
        case ASTNodeKind::VarInitStmt:
            return value_eligible(n->as_var_init()->value, e);
        case ASTNodeKind::AssignmentStmt: {
            auto* as = n->as_assignment();
            return value_eligible(as->lhs, e) && value_eligible(as->value, e);
        }
        case ASTNodeKind::IfStmt: {
            auto* ifs = n->as_if_stmt_unsafe();
            if (!value_eligible(ifs->condition, e)) return false;
            if (!scope_eligible(&ifs->ifBody, e)) return false;
            for (auto& ei : ifs->elseIfs) {
                if (!scope_eligible(&ei.second, e)) return false;
            }
            if (ifs->elseBody.has_value() && !scope_eligible(&ifs->elseBody.value(), e)) return false;
            return true;
        }
        case ASTNodeKind::Scope:
            return scope_eligible(n->as_scope(), e);
        default:
            return true;
    }
}

bool scope_eligible(Scope* s, Eligibility& e) {
    if (!s) return true;
    for (ASTNode* n : s->nodes) {
        if (!stmt_eligible(n, e)) return false;
    }
    return true;
}

bool value_eligible(Value* v, Eligibility& e) {
    if (!v || !e.ok) return e.ok;
    switch (v->val_kind()) {
        case ValueKind::FunctionCall: {
            auto* call = v->as_func_call_unsafe();
            FunctionDeclaration* fd = call->safe_linked_func();
            if (fd) {
                if (!fd->body.has_value()) {
                    // A no-body callee is a compiler intrinsic (folded by the
                    // lowerer at lowering time) or an extern the engine binds as
                    // a host function — both are fine, EXCEPT the intrinsics
                    // whose interpreter semantics the engine does not reproduce.
                    if (intrinsic_diverges(fd)) {
                        if (std::getenv("CT_STAT")) {
                            std::fprintf(stderr, "[CTSTAT] INELIG divergent intrinsic %s\n",
                                         fd->name_str().c_str());
                        }
                        e.ok = false;
                        return false;
                    }
                    return true; // no callee to lower
                }
                e.callees.insert(fd);
            }
            if (call->parent_val && call->parent_val != v &&
                !value_eligible(call->parent_val, e)) {
                return false;
            }
            for (Value* a : call->values) {
                if (!value_eligible(a, e)) return false;
            }
            return true;
        }
        case ValueKind::AccessChain: {
            auto* ch = v->as_access_chain_unsafe();
            for (Value* c : ch->values) {
                if (!value_eligible(c, e)) return false;
            }
            return true;
        }
        case ValueKind::CastedValue:
            return value_eligible(v->as_casted_value_unsafe()->value, e);
        case ValueKind::DereferenceValue:
            return value_eligible(v->as_dereference_value_unsafe()->getValue(), e);
        case ValueKind::NotValue:
            return value_eligible(v->as_not_value_unsafe()->getValue(), e);
        case ValueKind::NegativeValue:
            return value_eligible(v->as_negative_value_unsafe()->getValue(), e);
        case ValueKind::BitwiseNot:
            return value_eligible(v->as_bitwise_not_unsafe()->getValue(), e);
        case ValueKind::UnsafeValue:
            return value_eligible(static_cast<UnsafeValue*>(v)->getValue(), e);
        case ValueKind::AddrOfValue:
            return value_eligible(v->as_addr_of_value_unsafe()->value, e);
        case ValueKind::ReferenceOfValue:
            return value_eligible(v->as_reference_of_value_unsafe()->value, e);
        case ValueKind::IndexOperator: {
            auto* io = v->as_index_op_unsafe();
            return value_eligible(io->parent_val, e) && value_eligible(io->idx, e);
        }
        case ValueKind::IsValue: {
            auto* isv = v->as_is_value_unsafe();
            ASTNode* ln = isv->type ? isv->type->get_direct_linked_node() : nullptr;
            if (!ln || ln->kind() != ASTNodeKind::VariantMember) {
                // a type test (`T is int`): the engine folds it only when the
                // comptime result is known; decline otherwise
                if (!isv->get_comp_time_result().has_value()) {
                    e.ok = false;
                    return false;
                }
            }
            return value_eligible(isv->value, e);
        }
        case ValueKind::BlockValue:
            return scope_eligible(&static_cast<BlockValue*>(v)->scope, e);
        case ValueKind::Expression: {
            // The interpreter concatenates string literals with `+`; the engine
            // would do pointer arithmetic — decline that. Equality on string
            // literals is fine: the driver interns identical contents to one
            // buffer, so the engine's pointer comparison matches. Pointer
            // arithmetic/comparison (`str + 1`, `p < q`) matches natively.
            auto* ex = v->as_expression_unsafe();
            const bool str_operand =
                (ex->firstValue && ex->firstValue->val_kind() == ValueKind::String) ||
                (ex->secondValue && ex->secondValue->val_kind() == ValueKind::String);
            if (str_operand && ex->operation != Operation::IsEqual &&
                ex->operation != Operation::IsNotEqual) {
                e.ok = false;
                return false;
            }
            return value_eligible(ex->firstValue, e) && value_eligible(ex->secondValue, e);
        }
        case ValueKind::ArrayValue: {
            auto* av = v->as_array_value_unsafe();
            for (Value* el : av->values) {
                if (!value_eligible(el, e)) return false;
            }
            return true;
        }
        case ValueKind::StructValue: {
            auto* sv = v->as_struct_value_unsafe();
            for (auto& kv : sv->values) {
                if (!value_eligible(kv.second.value, e)) return false;
            }
            return true;
        }
        default:
            return true;
    }
}

/** Read a scalar field of MIR type `r` at `p` back into an AST value. */
Value* read_scalar_value(const MIRComptimeContext& ctx, const MIRTypeRecord& r,
                         const uint8_t* p, SourceLocation loc) {
    switch (r.kind) {
        case MIRTypeKind::Bool: {
            uint8_t b = 0;
            std::memcpy(&b, p, 1);
            return new (ctx.allocator->allocate<BoolValue>())
                BoolValue(b != 0, ctx.type_builder->getBoolType(), loc);
        }
        case MIRTypeKind::Int: {
            uint64_t v = 0;
            std::memcpy(&v, p, r.size > 8 ? 8 : r.size);
            if ((r.flags & TF_SIGNED) && r.size > 0 && r.size < 8) {
                const uint64_t sign = 1ull << (r.size * 8 - 1);
                if (v & sign) v |= ~((sign << 1) - 1);
            }
            auto* it = static_cast<IntNType*>((r.flags & TF_SIGNED)
                                                  ? (BaseType*) ctx.type_builder->getI64Type()
                                                  : (BaseType*) ctx.type_builder->getU64Type());
            return new (ctx.allocator->allocate<IntNumValue>()) IntNumValue(v, it, loc);
        }
        case MIRTypeKind::Float: {
            if (r.size == 8) {
                double d = 0;
                std::memcpy(&d, p, 8);
                return new (ctx.allocator->allocate<DoubleValue>())
                    DoubleValue(d, ctx.type_builder->getDoubleType(), loc);
            }
            float f = 0;
            std::memcpy(&f, p, 4);
            return new (ctx.allocator->allocate<FloatValue>())
                FloatValue(f, ctx.type_builder->getFloatType(), loc);
        }
        case MIRTypeKind::Pointer:
        case MIRTypeKind::Reference:
        case MIRTypeKind::Function: {
            uint64_t v = 0;
            std::memcpy(&v, p, 8);
            if (!v) {
                return new (ctx.allocator->allocate<NullValue>())
                    NullValue(ctx.type_builder->getNullPtrType(), loc);
            }
            return new (ctx.allocator->allocate<IntNumValue>())
                IntNumValue(v, ctx.type_builder->getU64Type(), loc);
        }
        default:
            return nullptr; // nested aggregate: out of scope for now
    }
}

/** Rebuild an AST struct value from a flat MIR struct's bytes. */
Value* read_struct_value(const MIRComptimeContext& ctx, MIRTypeBuilder& types, MIRModule& module,
                         BaseType* ast_type, const uint8_t* bytes, SourceLocation loc) {
    if (!ast_type) return nullptr;
    ASTNode* nd = ast_type->canonical() ? ast_type->canonical()->get_direct_linked_canonical_node()
                                        : nullptr;
    if (!nd || nd->kind() != ASTNodeKind::StructDecl) return nullptr;
    auto* sd = nd->as_struct_def_unsafe();
    const TypeId tid = types.map(ast_type);
    if (tid == MIR_INVALID_ID || tid >= module.types.size()) return nullptr;
    compute_aggregate_layout(module.types, tid);
    const MIRTypeRecord rec = module.types.get(tid);
    if (rec.kind != MIRTypeKind::Struct || rec.field_index == MIR_INVALID_ID) return nullptr;
    auto* sv = new (ctx.allocator->allocate<StructValue>())
        StructValue(ast_type, sd, sd, loc);
    for (uint32_t i = 0; i < rec.data_count; ++i) {
        const MIRFieldInfo& fi = module.types.fields[rec.field_index + i];
        const MIRTypeRecord fr = module.types.get(fi.type);
        Value* fv = read_scalar_value(ctx, fr, bytes + fi.offset, loc);
        if (!fv) return nullptr;
        const char* nm = module.types.field_names.data_at(fi.name_offset);
        const chem::string_view name(nm, fi.name_length);
        sv->values.emplace(name, StructMemberInitializer{name, fv});
    }
    return sv;
}

/** Marshal one comptime argument into a raw 64-bit cell. */
bool marshal_arg(Value* av, uint64_t& cell, std::vector<std::vector<char>>& keep,
                 std::unordered_map<std::string, size_t>& interned) {
    if (!av) return false;
    switch (av->val_kind()) {
        case ValueKind::IntN:
            cell = av->as_int_num_value_unsafe()->get_num_value();
            return true;
        case ValueKind::Bool:
            cell = av->as_bool_unsafe()->value ? 1u : 0u;
            return true;
        case ValueKind::NullValue:
            cell = 0;
            return true;
        case ValueKind::String: {
            // intern identical contents to one buffer so `*char` equality
            // matches the interpreter's content comparison for those literals
            auto* sv = av->as_string_unsafe();
            const std::string key(sv->value.data(), sv->value.size());
            auto it = interned.find(key);
            if (it == interned.end()) {
                keep.emplace_back(sv->value.data(), sv->value.data() + sv->value.size());
                keep.back().push_back('\0');
                it = interned.emplace(key, keep.size() - 1).first;
            }
            cell = reinterpret_cast<uint64_t>(keep[it->second].data());
            return true;
        }
        default:
            return false; // aggregate / lambda / … : out of scope for now
    }
}

} // namespace

Value* MIRComptime::evaluate(const MIRComptimeContext& ctx, FunctionDeclaration* fd,
                             FunctionCall* call, bool& handled, std::string& error) {
    handled = false;
#define CT_DECLINE(reason) do { if (std::getenv("CT_STAT")) { \
    std::fprintf(stderr, "[CTSTAT] DECLINE %s: %s\n", fd && fd->name_str().size() ? fd->name_str().c_str() : "?", reason); } \
    return nullptr; } while (0)
    if (!ctx.host || !ctx.allocator || !ctx.type_builder || !ctx.scope) return nullptr;
    if (!fd || !fd->body.has_value()) return nullptr;
    if (!fd->is_comptime()) return nullptr;
    // a template (an `@expand` splice, or a marker-parameter template) is
    // lowered at its call site, not evaluated
    if (fd->is_expand_fn()) return nullptr;
    if (!call) return nullptr;
    BaseType* rtc = fd->returnType ? fd->returnType->canonical() : nullptr;
    if (!scalar_return(rtc)) CT_DECLINE("non-scalar/struct return");
    // methods carry a receiver the driver does not marshal yet; a generic
    // instantiation binds its parameters from the instantiation site, which the
    // driver's type mapping is not yet scoped to
    if (fd->has_self_param()) CT_DECLINE("method (self receiver)");

    // unique mangled names inside the driver's module: generic instantiations
    // share the host's mangled name, which a module-local lookup would collide on
    if (!impl_->mangler_set) {
        MIRLowerer* host = ctx.host;
        impl_->task->lowerer.set_mangler_into([host](ASTNode* n, std::string& out) {
            out.assign(host->mangled_name_copy(n));
            out += '#';
            out += std::to_string(reinterpret_cast<uintptr_t>(n));
        });
        impl_->mangler_set = true;
    }

    // 0. eligibility — this function and everything it calls, transitively:
    // every reachable function must have a body the engine can run and use no
    // construct whose interpreter semantics differ
    std::vector<FunctionDeclaration*> reachable;
    {
        std::unordered_set<FunctionDeclaration*> seen;
        reachable.push_back(fd);
        for (size_t i = 0; i < reachable.size(); ++i) {
            FunctionDeclaration* d = reachable[i];
            if (!seen.insert(d).second) continue;
            if (!d->body.has_value()) return nullptr;
            Eligibility e;
            if (!scope_eligible(&d->body.value(), e) || !e.ok) CT_DECLINE("ineligible body");
            for (FunctionDeclaration* c : e.callees) reachable.push_back(c);
        }
    }

    // 1. fold + marshal the arguments
    std::vector<uint64_t> cells;
    std::vector<std::vector<char>> keep;
    std::unordered_map<std::string, size_t> interned;
    for (size_t i = 0; i < fd->params.size(); ++i) {
        FunctionParam* p = fd->params[i];
        if (!p || is_self_param(fd, p)) continue;
        Value* a = (i < call->values.size()) ? call->values[i] : p->defValue;
        if (!a) return nullptr;
        Value* av = a->evaluated_value(*ctx.scope);
        if (!av) return nullptr;
        uint64_t cell = 0;
        if (!marshal_arg(av, cell, keep, interned)) return nullptr;
        cells.push_back(cell);
    }

    // 2. lower the callee and every function it reaches
    for (FunctionDeclaration* d : reachable) {
        if (impl_->lowered.count(d)) continue;
        impl_->task->begin_function(false); // keep the arena: the MIR stays live
        MIRFunction fn;
        if (!impl_->task->lowerer.lower_function(d, impl_->task->arena, fn, error)) {
            error.clear();
            return nullptr;
        }
        impl_->lowered.emplace(d, fn.symbol);
        impl_->module.functions.push_back(std::move(fn));
        impl_->dirty = true;
    }
    const SymbolId target = impl_->lowered.count(fd) ? impl_->lowered[fd] : MIR_INVALID_ID;
    if (target == MIR_INVALID_ID) return nullptr;

    // 3. (re)prepare the engine when the module grew
    if (impl_->dirty || !impl_->interp) {
        impl_->interp = std::make_unique<MIRInterpreter>(impl_->module);
        impl_->dirty = false;
    }

    // 4. run the callee
    const std::string name =
        impl_->module.symbols.mangled_name(impl_->module.symbols.get(target));
    // void result: run the function for its comptime side effects; the caller
    // treats a null result as the void call
    if (rtc->kind() == BaseTypeKind::Void) {
        uint64_t ignored = 0;
        if (!impl_->interp->call(name, cells.data(), static_cast<uint32_t>(cells.size()),
                                 ignored, error)) {
            error.clear();
            return nullptr;
        }
        handled = true;
        return nullptr;
    }

    // aggregate result: sret (the result bytes are read back below)
    if (abi::needs_sret(impl_->module, impl_->types.map(rtc))) {
        const TypeId tid = impl_->types.map(rtc);
        if (tid == MIR_INVALID_ID || tid >= impl_->module.types.size()) return nullptr;
        compute_aggregate_layout(impl_->module.types, tid);
        const MIRTypeRecord rec = impl_->module.types.get(tid);
        std::vector<uint8_t> buf(rec.size ? rec.size : 1, 0);
        if (!impl_->interp->call_sret(name, cells.data(), static_cast<uint32_t>(cells.size()),
                                      buf.data(), error)) {
            if (std::getenv("CT_STAT")) {
                std::fprintf(stderr, "[CTSTAT] AGGFAIL %s call_sret\n", fd->name_str().c_str());
            }
            error.clear();
            return nullptr;
        }
        Value* bridged = read_struct_value(ctx, impl_->types, impl_->module, rtc, buf.data(),
                                           call->encoded_location());
        if (!bridged && std::getenv("CT_STAT")) {
            std::fprintf(stderr, "[CTSTAT] AGGFAIL %s tid=%u\n", fd->name_str().c_str(), tid);
        }
        if (bridged && std::getenv("CT_STAT")) {
            std::fprintf(stderr, "[CTSTAT] DRV %s fd=%p struct\n", fd->name_str().c_str(),
                         (void*) fd);
        }
        if (bridged) handled = true;
        return bridged;
    }

    uint64_t out = 0;
    if (!impl_->interp->call(name, cells.data(), static_cast<uint32_t>(cells.size()),
                             out, error)) {
        error.clear();
        return nullptr;
    }
    handled = true;
    if (std::getenv("CT_STAT")) {
        std::fprintf(stderr, "[CTSTAT] DRV %s fd=%p out=%llu sym=%s\n", fd->name_str().c_str(),
                     (void*) fd, (unsigned long long) out, name.c_str());
    }

    // 5. bridge the raw result back to an AST value
    const SourceLocation loc = call->encoded_location();
    switch (rtc->kind()) {
        case BaseTypeKind::IntN:
            return new (ctx.allocator->allocate<IntNumValue>())
                IntNumValue(out, static_cast<IntNType*>(rtc), loc);
        case BaseTypeKind::Bool:
            return new (ctx.allocator->allocate<BoolValue>())
                BoolValue(out != 0, ctx.type_builder->getBoolType(), loc);
        case BaseTypeKind::Pointer:
        case BaseTypeKind::Reference:
        case BaseTypeKind::NullPtr:
            return out ? new (ctx.allocator->allocate<IntNumValue>())
                             IntNumValue(out, ctx.type_builder->getU64Type(), loc)
                       : (Value*) new (ctx.allocator->allocate<NullValue>())
                             NullValue(ctx.type_builder->getNullPtrType(), loc);
        default:
            return nullptr;
    }
}

} // namespace mir
