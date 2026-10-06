// Copyright (c) Chemical Language Foundation 2025.
//
// AST-to-MIR lowering. A direct switch over resolved AST value/node kinds.
// Unsupported constructs return a diagnostic result (never a fallback); the
// caller turns that into a compile error. See mir-implementation-plan.md §4
// Stage 3 and mir-design.md §16.

#pragma once

#include "MIRModule.h"
#include "MIRTypeBuilder.h"
#include "MIRBuilder.h"
#include "std/chem_string_view.h"
#include "compiler/async/AsyncLoweringPlan.h"

#include <functional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

class ASTNode;
class Value;
class FunctionDeclaration;
class FunctionParam;
class VarInitStatement;
class Scope;
class IfStatement;
class WhileLoop;
class ForLoop;
class FunctionCall;

namespace mir {

enum class MIRExprKind : uint8_t {
    Value = 0,   // result is an SSA value
    Place = 1,   // result is an addressable place
    Address = 2, // result is a pointer/reference
    Void = 3,    // no result
    Error = 4,   // unsupported / malformed
};

struct MIRExprResult {
    uint32_t id = MIR_NULL;
    TypeId type = MIR_INVALID_ID;
    MIRExprKind kind = MIRExprKind::Error;

    static MIRExprResult value(ValueId v, TypeId t) { return MIRExprResult{v, t, MIRExprKind::Value}; }
    static MIRExprResult place(PlaceId p, TypeId t) { return MIRExprResult{p, t, MIRExprKind::Place}; }
    static MIRExprResult address(ValueId v, TypeId t) { return MIRExprResult{v, t, MIRExprKind::Address}; }
    static MIRExprResult void_result() { return MIRExprResult{MIR_NULL, MIR_INVALID_ID, MIRExprKind::Void}; }
    static MIRExprResult error() { return MIRExprResult{}; }
    bool ok() const { return kind != MIRExprKind::Error; }
};

/**
 * Lowers one resolved function at a time. Holds per-function state (variable ->
 * place bindings, callee -> symbol bindings). Not thread-safe; one instance is
 * owned by each worker (or created per function).
 */
class MIRLowerer {
public:
    MIRLowerer(MIRModule& module, MIRTypeBuilder& types)
        : module_(module), types_(types) {}

    /**
     * Lower `decl` into `func` using `arena`. On the first unsupported
     * construct, returns false and sets `error`; `func` is then discarded by
     * the caller and compilation fails.
     */
    bool lower_function(FunctionDeclaration* decl, MIRArena& arena,
                        MIRFunction& func, std::string& error);

    /**
     * For an async function, `lower_function` produces the *ramp* as `func` and
     * the coroutine `__poll` here, with the frame/vtable helpers in the pre/post
     * declaration text. The caller emits: pre-decls, poll, post-decls, ramp.
     */
    MIRFunction& async_poll() { return async_poll_; }
    std::string& async_pre_decls() { return async_pre_decls_; }
    std::string& async_post_decls() { return async_post_decls_; }
    bool is_async_lowering() const { return async_; }

    MIRTypeBuilder& types() { return types_; }
    MIRModule& module() { return module_; }

    /** Pre-register a callee symbol (mangled name) so calls resolve to it. */
    void set_function_symbol(FunctionDeclaration* decl, SymbolId symbol) {
        func_symbols_[decl] = symbol;
    }

    /** On-demand mangler for symbols not pre-registered (e.g. methods). */
    void set_mangler(std::function<std::string(ASTNode*)> fn) {
        mangler_ = std::move(fn);
    }

    /** Resolve the taken scope of a `comptime if` (evaluated by the interpreter). */
    void set_comptime_if_resolver(std::function<Scope*(IfStatement*, std::string&)> fn) {
        comptime_if_resolver_ = std::move(fn);
    }

    /** Evaluate a comptime function call (interpreter-backed). */
    void set_comptime_eval(std::function<Value*(FunctionCall*, FunctionDeclaration*)> fn) {
        comptime_eval_ = std::move(fn);
    }

    /** Evaluate an implicit-constructor comptime call (interpreter-backed). */
    void set_comptime_ctor_eval(std::function<Value*(FunctionDeclaration*, Value*)> fn) {
        comptime_ctor_eval_ = std::move(fn);
    }

    /** Fallback lookup of a top-level function by name (symbol table). */
    void set_symbol_lookup(std::function<FunctionDeclaration*(const std::string&)> fn) {
        symbol_lookup_ = std::move(fn);
    }

private:
    MIRExprResult lower_expr(Value* value, std::string& error);
    bool lower_stmt(ASTNode* node, std::string& error);
    bool lower_scope(Scope& scope, std::string& error);
    /** Lower a list of statements as a scope, destroying its locals at the end. */
    bool lower_scope_nodes(std::vector<ASTNode*>& nodes, std::string& error);
    bool lower_if(IfStatement* stmt, BlockId merge, std::string& error);
    bool lower_while(WhileLoop* loop, std::string& error);
    bool lower_for(ForLoop* loop, std::string& error);
    bool lower_incdec_value(Value* target, bool increment, bool post, std::string& error,
                            ValueId& out);
    PlaceId resolve_place(Value* v, std::string& error);
    /** Like resolve_place, but also lowers call results to their temporary place. */
    PlaceId resolve_place_or_lower(Value* v, std::string& error);
    /** Resolve an extension method by name on the receiver's canonical container. */
    FunctionDeclaration* resolve_method_fallback(BaseType* recv_type, const std::string& name);
    MIRExprResult lower_address_of(Value* inner, std::string& error);
    bool member_name(Value* v, std::string& out, std::string& error);
    bool lower_call_args(const std::vector<Value*>& values, std::vector<MIROperand>& args,
                         std::string& error);
    bool lower_call_args_for(FunctionDeclaration* fd, bool self_included,
                             const std::vector<Value*>& values, std::vector<MIROperand>& args,
                             std::string& error);
    MIRExprResult lower_arg_converted(Value* arg, BaseType* param_type, std::string& error);
    bool push_call_arg(MIRExprResult r, std::vector<MIROperand>& args, std::string& error);
    void destroy_call_temps(std::string& error);
    SymbolId destructor_symbol(BaseType* type);
    void register_destructible(PlaceId place, BaseType* type);
    void mark_moved(PlaceId place);
    void emit_drops();
    bool append_default_args(FunctionDeclaration* fd, size_t provided, bool self_included,
                             std::vector<MIROperand>& args, std::string& error);
    MIRExprResult lower_method_call(Value* receiver, FunctionCall* call, std::string& error);

    // ── async / coroutine lowering ─────────────────────────────────────────
    bool lower_async_function(FunctionDeclaration* decl, MIRArena& arena, MIRFunction& func,
                              std::string& error);
    void async_emit_frame_helpers(FunctionDeclaration* decl);
    bool async_lower_poll_body(FunctionDeclaration* decl, std::string& error);
    bool async_lower_ramp(FunctionDeclaration* decl, MIRFunction& func, std::string& error);
    bool async_lower_await_var_init(VarInitStatement* stmt, size_t site_index, std::string& error);
    void async_emit_spill(const struct AwaitSite& site);
    void async_emit_reload(const struct AwaitSite& site);
    void async_emit_pending_return();
    std::string async_slot_field(unsigned id) const;
    std::string async_child_field(unsigned id) const;
    std::string async_drop_flag_field(unsigned id) const;
    std::string async_resident_field(ASTNode* node) const;
    PlaceId async_resident_place(ASTNode* node) const;

    SymbolId intern_function(FunctionDeclaration* decl);
    SymbolId intern_global(VarInitStatement* vi);
    SymbolId intern_named_global(const std::string& name);
    PlaceId place_for_linked(ASTNode* linked) const;
    void bind(ASTNode* linked, PlaceId place) { var_places_[linked] = place; }
    void bind_name(const chem::string_view& name, PlaceId place) {
        if (!name.empty()) {
            name_places_.emplace(std::string(name.data(), name.size()), place);
        }
    }
    PlaceId place_for_name(const chem::string_view& name) const {
        if (name.empty()) return MIR_INVALID_ID;
        auto it = name_places_.find(std::string(name.data(), name.size()));
        return it == name_places_.end() ? MIR_INVALID_ID : it->second;
    }

    MIRModule& module_;
    MIRTypeBuilder& types_;
    MIRBuilder* builder_ = nullptr;
    MIRArena* arena_ = nullptr;
    std::unordered_map<ASTNode*, PlaceId> var_places_;
    std::unordered_map<std::string, PlaceId> name_places_;
    std::vector<std::pair<PlaceId, BaseType*>> call_temps_;
    struct MIRDestructible {
        PlaceId place;
        PlaceId flag;
        SymbolId dtor;
    };
    std::vector<MIRDestructible> destructibles_;
    std::unordered_map<FunctionDeclaration*, SymbolId> func_symbols_;
    std::unordered_map<ASTNode*, SymbolId> global_symbols_;
    std::unordered_map<std::string, SymbolId> named_globals_;
    std::function<std::string(ASTNode*)> mangler_;
    std::function<Scope*(IfStatement*, std::string&)> comptime_if_resolver_;
    std::function<Value*(FunctionCall*, FunctionDeclaration*)> comptime_eval_;
    std::function<Value*(FunctionDeclaration*, Value*)> comptime_ctor_eval_;
    std::function<FunctionDeclaration*(const std::string&)> symbol_lookup_;
    std::vector<BlockId> break_targets_;
    std::vector<BlockId> continue_targets_;
    bool sret_ = false;
    ValueId sret_ptr_ = MIR_NULL;
    TypeId sret_ret_type_ = MIR_INVALID_ID;
    TypeId sret_ptr_type_ = MIR_INVALID_ID;

    // ── async / coroutine lowering state ───────────────────────────────────
    bool async_ = false;
    BaseType* async_inner_ = nullptr;     // T (the async result type)
    BaseType* async_handle_ = nullptr;    // FutureHandle<T>
    BaseType* async_table_ = nullptr;     // FutureTable<T>
    BaseType* async_poll_base_ = nullptr; // Poll<T> of this function
    BaseType* async_context_ = nullptr;   // *mut Context
    TypeId async_frame_type_ = MIR_INVALID_ID;     // `<mangled>__frame`
    TypeId async_frame_ptr_type_ = MIR_INVALID_ID; // `<mangled>__frame*`
    AsyncLoweringPlan async_plan_;
    MIRFunction async_poll_;
    std::string async_pre_decls_;
    std::string async_post_decls_;
    // poll-function lowering state
    PlaceId async_poll_ret_ = MIR_INVALID_ID; // Poll<T>* __chx__async_ret
    PlaceId async_poll_frame_ = MIR_INVALID_ID; // <frame>* __chx__af
    PlaceId async_poll_cx_ = MIR_INVALID_ID;  // Context* __cx
    ValueId async_poll_af_val_ = MIR_NULL;    // the frame pointer value
    ValueId async_ramp_af_val_ = MIR_NULL;    // the ramp frame pointer value
    std::unordered_map<ASTNode*, unsigned> async_resident_;
    std::unordered_set<unsigned> async_drop_flag_slots_;
    std::vector<TypeId> async_site_poll_;
    std::vector<BlockId> async_resume_blocks_;
    // ramp lowering state
    PlaceId async_ramp_af_ = MIR_INVALID_ID;
    PlaceId async_ramp_result_ = MIR_INVALID_ID;
    BlockId async_ramp_done_ = MIR_INVALID_ID;
    bool async_in_ramp_ = false;
    // per-site lowering info
    std::string async_mangled_;
    std::unordered_map<ASTNode*, size_t> async_site_index_;
    std::vector<PlaceId> async_child_places_;
    std::vector<PlaceId> async_result_places_;
    std::vector<std::vector<std::pair<PlaceId, uint32_t>>> async_spills_;
    std::vector<BlockId> async_cont_blocks_;
};

} // namespace mir
