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
#include "ast/structures/ComptimeBlock.h"
#include "ast/structures/EnumDeclaration.h"
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
#include "ast/types/VoidType.h"
#include "ast/base/TypeBuilder.h"
#include "ast/base/TypeLoc.h"
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
    /** true while a callee is being lowered: a re-entrant fold must use a fresh
     * driver instance so it does not clobber the in-progress lowering state. */
    bool lowering = false;

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
        case BaseTypeKind::Function:
            return true;
        case BaseTypeKind::Linked: {
            // a self-referencing struct type canonicalizes to its Linked form;
            // an enum is carried as its underlying integer
            ASTNode* n = canonical->get_direct_linked_canonical_node();
            return n && (n->kind() == ASTNodeKind::StructDecl ||
                         n->kind() == ASTNodeKind::UnionDecl ||
                         n->kind() == ASTNodeKind::VariantDecl ||
                         n->kind() == ASTNodeKind::EnumDecl);
        }
        default:
            return false;
    }
}

bool is_self_param(FunctionDeclaration* fd, FunctionParam* p) {
    return fd->has_self_param() && fd->get_self_param() == p;
}

/** A parameter that is a runtime operand (`%runtime<T>` / `%maybe_runtime<T>` /
 * `%literal`) has no comptime value to publish. */
bool is_runtime_marker_param(FunctionParam* p) {
    if (!p || !p->type) return false;
    const BaseTypeKind k = p->type->kind();
    return k == BaseTypeKind::Runtime || k == BaseTypeKind::MaybeRuntime ||
           k == BaseTypeKind::Literal;
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
        "get_call_loc", "get_line_no", "get_char_no",
        "get_caller_char_no", "get_loc_file_path", "get_raw_location",
        "get_current_file_path", "expr_str_block_value", "vector", "decode_location",
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

/** The expression of the single `return` in `body`, if it has exactly one. */
Value* single_return_expr(Scope& body) {
    Value* found = nullptr;
    for (ASTNode* n : body.nodes) {
        if (n && n->kind() == ASTNodeKind::ReturnStmt) {
            if (found) return nullptr;
            found = n->as_return()->value;
        }
    }
    return found;
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

/** Write a flat AST struct value's scalar fields into MIR bytes (receiver args). */
bool write_struct_bytes(MIRTypeBuilder& types, MIRModule& module, StructValue* sv,
                        std::vector<uint8_t>& buf) {
    BaseType* rt = sv->getType();
    const TypeId rtid = rt ? types.map(rt) : MIR_INVALID_ID;
    if (rtid == MIR_INVALID_ID || rtid >= module.types.size()) return false;
    compute_aggregate_layout(module.types, rtid);
    const MIRTypeRecord rec = module.types.get(rtid);
    buf.assign(rec.size ? rec.size : 1, 0);
    if (rec.field_index == MIR_INVALID_ID) return true;
    for (uint32_t i = 0; i < rec.data_count; ++i) {
        const MIRFieldInfo& fi = module.types.fields[rec.field_index + i];
        const char* nm = module.types.field_names.data_at(fi.name_offset);
        auto it = sv->values.find(chem::string_view(nm, fi.name_length));
        if (it == sv->values.end() || !it->second.value) return false;
        Value* fv = it->second.value;
        const MIRTypeRecord fr = module.types.get(fi.type);
        const size_t n = fr.size > 8 ? 8 : fr.size;
        switch (fv->val_kind()) {
            case ValueKind::IntN: {
                const uint64_t v = fv->as_int_num_value_unsafe()->get_num_value();
                std::memcpy(buf.data() + fi.offset, &v, n);
                break;
            }
            case ValueKind::Bool: {
                const uint8_t b = fv->as_bool_unsafe()->value ? 1 : 0;
                std::memcpy(buf.data() + fi.offset, &b, n);
                break;
            }
            case ValueKind::Float: {
                const float f = fv->as_float_unsafe()->value;
                std::memcpy(buf.data() + fi.offset, &f, n);
                break;
            }
            case ValueKind::Double: {
                const double d = fv->as_double_unsafe()->value;
                std::memcpy(buf.data() + fi.offset, &d, n);
                break;
            }
            case ValueKind::NullValue: {
                const uint64_t z = 0;
                std::memcpy(buf.data() + fi.offset, &z, n);
                break;
            }
            default:
                return false; // nested aggregate: out of scope
        }
    }
    return true;
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
    // A result type the engine cannot carry as a cell (array/slice, variant):
    // fold the function's single return expression as an AST value — the §5.4
    // expression layer (the expression is self-contained, e.g. an intrinsic
    // call, so no statements run).
    if (!scalar_return(rtc)) {
        if (Value* ret = single_return_expr(fd->body.value())) {
            if (Value* v = ret->evaluated_value(*ctx.scope)) {
                handled = true;
                if (std::getenv("CT_STAT")) {
                    std::fprintf(stderr, "[CTSTAT] DRV %s fd=%p folded\n",
                                 fd->name_str().c_str(), (void*) fd);
                }
                return v;
            }
        }
        CT_DECLINE("unsupported result");
    }
    // a generic instantiation binds its parameters from the instantiation site,
    // which the driver's type mapping is not yet scoped to
    (void) 0;

    // unique mangled names inside the driver's module: generic instantiations
    // share the host's mangled name, which a module-local lookup would collide on
    if (!impl_->mangler_set) {
        MIRLowerer* host = ctx.host;
        impl_->task->lowerer.copy_comptime_fold_callbacks_from(*host);
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

    // 1. fold + marshal the arguments (the receiver first for a method)
    std::vector<uint64_t> cells;
    std::vector<std::vector<char>> keep;
    std::vector<std::vector<uint8_t>> keep_structs;
    std::unordered_map<std::string, size_t> interned;
    if (fd->has_self_param()) {
        Value* rexpr = nullptr;
        if (call->parent_val && call->parent_val->val_kind() == ValueKind::AccessChain) {
            auto* ch = call->parent_val->as_access_chain_unsafe();
            if (!ch->values.empty()) rexpr = ch->values.front();
        }
        Value* rv = rexpr ? evaluate_value(ctx, rexpr, error) : nullptr;
        if (rv && rv->val_kind() == ValueKind::StructValue) {
            keep_structs.emplace_back();
            if (!write_struct_bytes(impl_->types, impl_->module, rv->as_struct_value_unsafe(),
                                    keep_structs.back())) {
                return nullptr;
            }
            cells.push_back(reinterpret_cast<uint64_t>(keep_structs.back().data()));
        } else {
            // a comptime method whose receiver is a runtime value: the body must
            // not dereference `self` (the engine has no comptime bytes for it)
            cells.push_back(0);
        }
    }
    // `call->values` holds the explicit arguments only — `self` (and any other
    // leading implicit parameter) is passed as the receiver, exactly as the
    // interpreter's FunctionDeclaration::call maps them: arg i is
    // func_param_for_arg_at(i), whose offset skips those implicit parameters.
    std::vector<std::pair<std::string, Value*>> publish_bindings;
    const size_t supplied = call->values.size();
    const size_t expected = fd->expectedArgsSize();
    const size_t total = supplied > expected ? supplied : expected;
    for (size_t i = 0; i < total; ++i) {
        FunctionParam* p = fd->func_param_for_arg_at(static_cast<unsigned>(i));
        if (!p) return nullptr;
        Value* a = i < supplied ? call->values[i] : p->defValue;
        if (!a) return nullptr;
        Value* av = evaluate_value(ctx, a, error);
        if (!av) {
            if (std::getenv("CT_STAT")) std::fprintf(stderr, "[CTSTAT] ARGFAIL %s\n", fd->name_str().c_str());
            return nullptr;
        }
        uint64_t cell = 0;
        if (!marshal_arg(av, cell, keep, interned)) {
            if (std::getenv("CT_STAT")) std::fprintf(stderr, "[CTSTAT] MARSHALFAIL %s kind=%d\n",
                                                     fd->name_str().c_str(), (int) av->val_kind());
            return nullptr;
        }
        cells.push_back(cell);
        if (!is_runtime_marker_param(p)) {
            publish_bindings.emplace_back(std::string(p->name.data(), p->name.size()), av);
        }
    }

    // publish the callee's folded comptime parameters, so a nested fold reached
    // during lowering (`llvm_mem_order(order)` in an atomic intrinsic argument)
    // resolves them
    struct BindingRestore {
        MIRLowerer* host = nullptr;
        std::unordered_map<std::string, Value*> saved;
        ~BindingRestore() { if (host) host->restore_comptime_bindings(std::move(saved)); }
    } binding_restore{ctx.host, ctx.host->snapshot_comptime_bindings()};
    for (auto& kv : publish_bindings) ctx.host->publish_comptime_binding(kv.first, kv.second);

    // 2. lower the callee and every function it reaches. The call site is
    // pushed for the whole evaluation so location intrinsics folded during
    // lowering resolve against it (as the @expand inline does).
    impl_->task->lowerer.invoke_call_site(call, true);
    struct CallSitePop {
        MIRLowerer* l; FunctionCall* c;
        ~CallSitePop() { if (l) l->invoke_call_site(c, false); }
    } call_site_pop{&impl_->task->lowerer, call};
    impl_->lowering = true;
    struct LoweringGuard {
        Impl* impl;
        ~LoweringGuard() { impl->lowering = false; }
    } lowering_guard{impl_.get()};
    for (FunctionDeclaration* d : reachable) {
        if (impl_->lowered.count(d)) continue;
        impl_->task->begin_function(false); // keep the arena: the MIR stays live
        MIRFunction fn;
        if (!impl_->task->lowerer.lower_function(d, impl_->task->arena, fn, error)) {
            if (std::getenv("CT_STAT")) {
                std::fprintf(stderr, "[CTSTAT] LOWERFAIL %s: %s\n", d->name_str().c_str(),
                             error.c_str());
            }
            error.clear();
            return nullptr;
        }
        impl_->lowered.emplace(d, fn.symbol);
        impl_->module.functions.push_back(std::move(fn));
        impl_->dirty = true;
    }
    const SymbolId target = impl_->lowered.count(fd) ? impl_->lowered[fd] : MIR_INVALID_ID;
    if (target == MIR_INVALID_ID) {
        if (std::getenv("CT_STAT")) std::fprintf(stderr, "[CTSTAT] NOTARGET %s\n", fd->name_str().c_str());
        return nullptr;
    }

    // 3. (re)prepare the engine when the module grew
    if (impl_->dirty || !impl_->interp) {
        impl_->interp = std::make_unique<MIRInterpreter>(impl_->module);
        impl_->dirty = false;
    }

    // 4. run the callee
    const std::string name =
        impl_->module.symbols.mangled_name(impl_->module.symbols.get(target));
    // A function-pointer result cannot travel as an engine cell: fold the
    // function's (single) return expression as an AST value (the §5.4
    // expression layer).
    if (rtc->kind() == BaseTypeKind::Function) {
        Value* ret = single_return_expr(fd->body.value());
        if (!ret) return nullptr;
        Value* v = ret->evaluated_value(*ctx.scope);
        if (!v) return nullptr;
        handled = true;
        if (std::getenv("CT_STAT")) {
            std::fprintf(stderr, "[CTSTAT] DRV %s fd=%p fnptr\n", fd->name_str().c_str(),
                         (void*) fd);
        }
        return v;
    }

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
        if (std::getenv("CT_STAT")) {
            std::fprintf(stderr, "[CTSTAT] CALLFAIL %s: %s\n", fd->name_str().c_str(),
                         error.c_str());
        }
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
        case BaseTypeKind::Linked: {
            // an enum result is carried as its underlying integer
            ASTNode* n = rtc->get_direct_linked_canonical_node();
            if (n && n->kind() == ASTNodeKind::EnumDecl) {
                auto* en = static_cast<EnumDeclaration*>(n);
                auto* it = en->underlying_integer_type
                               ? en->underlying_integer_type
                               : ctx.type_builder->getIntType();
                return new (ctx.allocator->allocate<IntNumValue>()) IntNumValue(out, it, loc);
            }
            return nullptr;
        }
        default:
            return nullptr;
    }
}
Value* MIRComptime::evaluate_value(const MIRComptimeContext& ctx, Value* v,
                                    std::string& error) {
    if (!v || !ctx.scope) return nullptr;
    if (v->val_kind() == ValueKind::AccessChain) {
        auto* chain = v->as_access_chain_unsafe();
        if (chain->values.size() == 1) return evaluate_value(ctx, chain->values.front(), error);
    }
    if (v->val_kind() == ValueKind::FunctionCall) {
        auto* call = v->as_func_call_unsafe();
        FunctionDeclaration* callee = call->safe_linked_func();
        if (callee && callee->is_comptime() && callee->body.has_value() &&
            !callee->is_expand_fn()) {
            if (impl_->lowering) {
                // reached during another callee's lowering: use a fresh driver so
                // the in-progress lowerer state is not clobbered
                MIRComptime nested;
                return nested.evaluate_value(ctx, v, error);
            }
            bool handled = false;
            std::string nested_error;
            Value* r = evaluate(ctx, callee, call, handled, nested_error);
            if (handled) return r;
            error = nested_error;
            return nullptr;
        }
    }
    return v->evaluated_value(*ctx.scope);
}

bool MIRComptime::evaluate_block(const MIRComptimeContext& ctx, ComptimeBlock* block,
                                 std::string& error) {
    if (!ctx.host || !ctx.allocator || !ctx.type_builder || !block) {
        error = "comptime block has no lowering context";
        return false;
    }
    // A comptime block runs in its own scope: it must not read the enclosing
    // function's runtime locals, so model it as a synthetic void function.
    auto* fd = new (ctx.allocator->allocate<FunctionDeclaration>())
        FunctionDeclaration(chem::string_view("__comptime_block"),
                            TypeLoc(ctx.type_builder->getVoidType(), block->encoded_location()), false,
                            block->parent(), block->encoded_location());
    fd->set_comptime(true);
    fd->body.emplace(fd, block->encoded_location());
    fd->body->nodes = block->body.nodes;

    if (!impl_->mangler_set) {
        MIRLowerer* host = ctx.host;
        impl_->task->lowerer.copy_comptime_fold_callbacks_from(*host);
        impl_->task->lowerer.set_mangler_into([host](ASTNode* n, std::string& out) {
            out.assign(host->mangled_name_copy(n));
            out += '#';
            out += std::to_string(reinterpret_cast<uintptr_t>(n));
        });
        impl_->mangler_set = true;
    }

    std::vector<FunctionDeclaration*> reachable;
    {
        std::unordered_set<FunctionDeclaration*> seen;
        reachable.push_back(fd);
        for (size_t i = 0; i < reachable.size(); ++i) {
            FunctionDeclaration* d = reachable[i];
            if (!seen.insert(d).second) continue;
            if (!d->body.has_value()) {
                error = "comptime block reaches a declaration without a body";
                return false;
            }
            Eligibility e;
            if (!scope_eligible(&d->body.value(), e) || !e.ok) {
                error = "comptime block uses a construct the MIR engine does not support";
                return false;
            }
            for (FunctionDeclaration* c : e.callees) reachable.push_back(c);
        }
    }

    for (FunctionDeclaration* d : reachable) {
        if (impl_->lowered.count(d)) continue;
        impl_->task->begin_function(false);
        MIRFunction fn;
        if (!impl_->task->lowerer.lower_function(d, impl_->task->arena, fn, error)) {
            return false;
        }
        impl_->lowered.emplace(d, fn.symbol);
        impl_->module.functions.push_back(std::move(fn));
        impl_->dirty = true;
    }
    const SymbolId target = impl_->lowered.count(fd) ? impl_->lowered[fd] : MIR_INVALID_ID;
    if (target == MIR_INVALID_ID) {
        error = "comptime block was not lowered";
        return false;
    }
    if (impl_->dirty || !impl_->interp) {
        impl_->interp = std::make_unique<MIRInterpreter>(impl_->module);
        impl_->dirty = false;
    }
    const std::string name =
        impl_->module.symbols.mangled_name(impl_->module.symbols.get(target));
    uint64_t ignored = 0;
    return impl_->interp->call(name, nullptr, 0, ignored, error);
}


} // namespace mir
