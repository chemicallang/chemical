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
#include <set>
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
class IfValue;
class SwitchStatement;
class SwitchValue;
class LoopValue;
class LoopBlock;
class WhileLoop;
class DoWhileLoop;
class ForLoop;
class FunctionCall;
class ExtendableMembersContainerNode;
class VariantCaseVariable;
class VariantMemberParam;
class PatternMatchExpr;
class PatternMatchExprNode;
class PatternMatchIdentifier;
class LambdaFunction;
class CapturedVariable;
class ExtractionValue;
class FunctionTypeBody;
class CapturingFunctionType;
class BaseType;
class ForInLoop;
class ImplementationsIndex;
class CoreNodes;

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

    /** Generated C for lambda functions produced while lowering a body. */
    std::vector<std::string>& lambda_defs() { return lambda_defs_; }
    void clear_lambda_defs() { lambda_defs_.clear(); }

    /** Pre-register a callee symbol (mangled name) so calls resolve to it. */
    void set_function_symbol(FunctionDeclaration* decl, SymbolId symbol) {
        func_symbols_[decl] = symbol;
    }

    /** On-demand mangler for symbols not pre-registered (e.g. methods). */
    void set_mangler(std::function<std::string(ASTNode*)> fn) {
        mangler_ = std::move(fn);
    }

    /** Mangler for a dynamic-dispatch vtable global (interface + impl type). */
    void set_vtable_mangler(std::function<std::string(ASTNode*, BaseType*)> fn) {
        vtable_mangler_ = std::move(fn);
    }

    /** Resolve the taken scope of a `comptime if` (evaluated by the interpreter). */
    void set_comptime_if_resolver(std::function<Scope*(IfStatement*, std::string&)> fn) {
        comptime_if_resolver_ = std::move(fn);
    }

    /** Evaluate a comptime function call (interpreter-backed). */
    void set_comptime_eval(std::function<Value*(FunctionCall*, FunctionDeclaration*)> fn) {
        comptime_eval_ = std::move(fn);
    }

    /** Evaluate an arbitrary comptime-known value (e.g. `def.windows`). */
    void set_comptime_value_eval(std::function<Value*(Value*)> fn) {
        comptime_value_eval_ = std::move(fn);
    }

    /** Evaluate an implicit-constructor comptime call (interpreter-backed). */
    void set_comptime_ctor_eval(std::function<Value*(FunctionDeclaration*, Value*)> fn) {
        comptime_ctor_eval_ = std::move(fn);
    }

    /** Fallback lookup of a top-level function by name (symbol table). */
    void set_symbol_lookup(std::function<FunctionDeclaration*(const std::string&)> fn) {
        symbol_lookup_ = std::move(fn);
    }

    /** Notify the host which function body is currently being lowered (comptime). */
    void set_current_func(std::function<void(FunctionTypeBody*)> fn) {
        set_current_func_ = std::move(fn);
    }

    /** Provide the symbol-resolution implementation index (for iterable protocols). */
    void set_impls(ImplementationsIndex* idx, CoreNodes* core) {
        impls_index_ = idx;
        core_nodes_ = core;
    }

private:
    MIRExprResult lower_expr(Value* value, std::string& error);
    bool lower_stmt(ASTNode* node, std::string& error);
    bool lower_scope(Scope& scope, std::string& error);
    /** Lower a list of statements as a scope, destroying its locals at the end. */
    bool lower_scope_nodes(std::vector<ASTNode*>& nodes, std::string& error);
    bool lower_if(IfStatement* stmt, BlockId merge, std::string& error);
    /** Lower an `if`-value expression: store the taken branch's value into a temp. */
    MIRExprResult lower_if_value(IfValue* value, std::string& error);
    /** Lower a value scope, storing its trailing value into `dest`. */
    bool lower_scope_value(Scope& scope, PlaceId dest, TypeId dest_t, std::string& error);
    /** Emit the case-comparison dispatch of a switch, branching into scope_blocks/default. */
    bool lower_switch_dispatch(SwitchStatement& sw, std::vector<BlockId>& scope_blocks,
                               BlockId default_b, std::string& error);
    /** Lower an if statement in value position, storing the taken branch into `dest`. */
    bool lower_if_stmt_into(IfStatement& stmt, PlaceId dest, TypeId vt, std::string& error);
    /** Lower a `switch(x) { ... }` value expression: store the taken scope's value. */
    MIRExprResult lower_switch_value(SwitchValue* value, std::string& error);
    /** Lower a switch statement in value position, storing the taken scope into `dest`. */
    bool lower_switch_into(SwitchStatement& sw, PlaceId dest, TypeId vt, std::string& error);
    /** Lower a `loop { break v }` value expression. */
    MIRExprResult lower_loop_value(LoopValue* value, std::string& error);
    /** Lower a loop block in value position, storing the break value into `dest`. */
    bool lower_loop_block_into(LoopBlock& loop, PlaceId dest, TypeId vt, std::string& error);
    /** Lower an infinite `loop { ... }` statement (exits via break/return). */
    bool lower_loop_block(LoopBlock* loop, std::string& error);
    bool lower_while(WhileLoop* loop, std::string& error);
    bool lower_do_while(DoWhileLoop* loop, std::string& error);
    bool lower_for(ForLoop* loop, std::string& error);
    /** Lower a `for (x in iterable)` loop (array / linear / iterable / chunked). */
    bool lower_for_in(ForInLoop* loop, std::string& error);
    /** Resolve an iterable-protocol impl method (e.g. "data", "begin", "valid"). */
    FunctionDeclaration* resolve_iter_impl(BaseType* container_type, const char* which);
    /** Lower a call to a resolved method with already-built argument values. */
    MIRExprResult call_method_values(FunctionDeclaration* fd, MIRExprResult recv,
                                     BaseType* recv_type, const std::vector<Value*>& args,
                                     std::string& error);
    /** Intern an extern C function symbol (malloc/free) with the given signature. */
    SymbolId extern_symbol(const char* name, TypeId ftype);
    /** Emit a `malloc(size)` call yielding `ptr_t`. */
    ValueId emit_malloc(TypeId size_v, TypeId u64t, TypeId ptr_t);
    /** Emit a `free(ptr)` call. */
    void emit_free(ValueId ptr);
    MIRExprResult lower_incdec_value(Value* target, bool increment, bool post,
                                     std::string& error);
    /** Lower an inline-asm operand to a Place or a pointer Value (`*ptr`). */
    bool lower_asm_operand(Value* expr, MIROperand& out, std::string& error);
    /** Lower a variant-case payload variable to its field lvalue/value. */
    MIRExprResult lower_variant_payload(VariantCaseVariable* var, bool as_address,
                                        std::string& error);
    /** Lower the matched lvalue of a pattern match (cached per expression). */
    MIRExprResult lower_pattern_base(PatternMatchExpr* pm, std::string& error);
    /** Lower a pattern match's matched-member payload to its lvalue/value. */
    MIRExprResult lower_pattern_payload(PatternMatchExpr* pm, VariantMemberParam* param,
                                        bool as_address, std::string& error);
    /** Lower a pattern match's member-tag comparison (`tag == index`). */
    MIRExprResult lower_pattern_cond(PatternMatchExpr* pm, std::string& error);
    /** Lower a pattern match expression (value position). */
    MIRExprResult lower_pattern_match_value(PatternMatchExpr* pm, std::string& error);
    /** Lower a `var Pattern(...) = expr else ...` statement. */
    bool lower_pattern_match_node(PatternMatchExprNode* node, std::string& error);
    /** Resolve a destructured pattern identifier to its payload. */
    MIRExprResult lower_pattern_identifier(PatternMatchIdentifier* id, std::string& error);
    /**
     * If `value` is a moved reference (a destructured variant payload), clear the
     * source variant's drop flag so its destructor does not free the payload the
     * destination now owns.
     */
    void clear_moved_ref(Value* value);
    PlaceId resolve_place(Value* v, std::string& error);
    /** Like resolve_place, but also lowers call results to their temporary place. */
    PlaceId resolve_place_or_lower(Value* v, std::string& error);
    /** Resolve an extension method by name on the receiver's canonical container. */
    FunctionDeclaration* resolve_method_fallback(BaseType* recv_type, const std::string& name);
    /** The struct/variant/union container a receiver type denotes (or nullptr). */
    ExtendableMembersContainerNode* receiver_user_container(BaseType* recv_type);
    /**
     * Intern the symbol for a method call. Interface default methods are emitted
     * once per implementing struct (with the interface's `active_user` set), so
     * their symbol must be resolved against the receiver's container.
     */
    SymbolId intern_method_function(FunctionDeclaration* decl, BaseType* recv_type);
    MIRExprResult lower_address_of(Value* inner, std::string& error);
    bool member_name(Value* v, std::string& out, std::string& error);
    /** Resolve a (possibly inherited) member to its dotted C field path. */
    bool member_field_path(Value* base, Value* member, std::string& out, std::string& error);
    bool lower_call_args(const std::vector<Value*>& values, std::vector<MIROperand>& args,
                         std::string& error);
    bool lower_call_args_for(FunctionDeclaration* fd, bool self_included,
                             const std::vector<Value*>& values, std::vector<MIROperand>& args,
                             std::string& error,
                             const std::vector<BaseType*>* extra_params = nullptr);
    MIRExprResult lower_arg_converted(Value* arg, BaseType* param_type, std::string& error);
    bool push_call_arg(MIRExprResult r, std::vector<MIROperand>& args, std::string& error);
    void destroy_call_temps(std::string& error);
    void destroy_call_temps_from(size_t mark, std::string& error);
    SymbolId destructor_symbol(BaseType* type);
    void register_destructible(PlaceId place, BaseType* type);
    void mark_moved(PlaceId place);
    void emit_drops();
    /** Drop destructibles registered at/after `mark`, innermost first. */
    void emit_drops_from(size_t mark);
    bool append_default_args(FunctionDeclaration* fd, size_t provided, bool self_included,
                             std::vector<MIROperand>& args, std::string& error);
    /** Walk `values[0..count-1]` as a member chain, returning the last field. */
    MIRExprResult lower_member_chain(const std::vector<Value*>& values, size_t count,
                                     std::string& error);
    /** Lower a method-call receiver: prefer its place, else the member chain. */
    MIRExprResult lower_receiver(const std::vector<Value*>& values, size_t count,
                                 std::string& error);
    MIRExprResult lower_method_call(MIRExprResult recv, BaseType* recv_type, FunctionCall* call,
                                    std::string& error);
    /** Register a struct-returning call temporary for destruction. */
    void maybe_register_call_temp(Value* v, MIRExprResult r);
    /** Destroy a call-returning-struct temporary immediately (statement-local). */
    void destroy_call_temp(Value* v, MIRExprResult r);
    /** Register a struct temporary for destruction at the end of the statement. */
    void register_stmt_temp(PlaceId place, BaseType* type);
    /** Cancel a pending statement-temp destruction (the value was moved out). */
    void cancel_stmt_temp(PlaceId place);
    /** Destroy all statement temporaries registered since `mark`. */
    void flush_stmt_temps(size_t mark);
    /** Apply default field values / base default constructors for unset fields. */
    bool apply_struct_defaults(PlaceId temp, ExtendableMembersContainerNode* def,
                               const std::set<std::string>& explicit_fields,
                               std::string& error);
    /** Append the receiver operand for a method/extension call to `args`. */
    bool append_receiver_arg(MIRExprResult recv, BaseType* recv_type,
                             FunctionDeclaration* fd, std::vector<MIROperand>& args,
                             std::string& error);
    /** Emit a resolved scalar/sret call and destroy the argument temporaries. */
    MIRExprResult emit_resolved_call(SymbolId sym, TypeId ret_type,
                                     std::vector<MIROperand>& args, size_t cmark,
                                     std::string& error);
    /** Lower an overloaded binary operator by calling its `core::ops` method. */
    MIRExprResult lower_operator_overload(Value* lhs_v, MIRExprResult lhs, Value* rhs_v,
                                          MIRExprResult rhs, const std::string& method,
                                          BaseType* ret_ast, std::string& error);
    /** Lower an overloaded unary operator by calling its `core::ops` method. */
    MIRExprResult lower_unary_operator_overload(Value* operand_v, MIRExprResult operand,
                                                const char* method, BaseType* ret_ast,
                                                std::string& error);
    /** Lower a lambda expression to a generated function (and closure). */
    MIRExprResult lower_lambda(LambdaFunction* lamb, std::string& error);
    /** Lower an `intrinsics::get_lambda_*` extraction value. */
    MIRExprResult lower_extraction_value(ExtractionValue* ev, std::string& error);
    /** Resolve a captured variable inside a lambda body to its closure field. */
    MIRExprResult lower_captured_variable(CapturedVariable* cap, bool as_address,
                                          std::string& error);
    /** Ensure a capturing lambda's closure struct/function/temp are generated. */
    bool materialize_capturing_lambda(LambdaFunction* lamb, std::string& error);
    /** Generate a non-capturing lambda function and yield its function pointer. */
    MIRExprResult materialize_noncapturing_lambda(LambdaFunction* lamb, std::string& error);
    /** Lower a call through a capturing function instance (`std::function`). */
    MIRExprResult lower_capturing_function_call(MIRExprResult recv, BaseType* recv_type,
                                                FunctionCall* call, std::string& error);

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
            name_places_[std::string(name.data(), name.size())] = place;
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
    std::unordered_map<std::string, SymbolId> named_method_symbols_;
    std::unordered_map<ASTNode*, SymbolId> global_symbols_;
    std::unordered_map<std::string, SymbolId> named_globals_;
    std::function<std::string(ASTNode*)> mangler_;
    std::function<std::string(ASTNode*, BaseType*)> vtable_mangler_;
    std::function<Scope*(IfStatement*, std::string&)> comptime_if_resolver_;
    std::function<Value*(FunctionCall*, FunctionDeclaration*)> comptime_eval_;
    std::function<Value*(Value*)> comptime_value_eval_;
    std::function<Value*(FunctionDeclaration*, Value*)> comptime_ctor_eval_;
    std::function<FunctionDeclaration*(const std::string&)> symbol_lookup_;
    std::function<void(FunctionTypeBody*)> set_current_func_;
    ImplementationsIndex* impls_index_ = nullptr;
    CoreNodes* core_nodes_ = nullptr;
    std::unordered_map<std::string, SymbolId> extern_symbols_;
    /** Struct temporaries whose destruction is deferred to the statement end. */
    std::vector<std::pair<PlaceId, BaseType*>> stmt_temps_;
    FunctionTypeBody* current_func_ = nullptr;
    std::vector<BlockId> break_targets_;
    std::vector<BlockId> continue_targets_;
    std::vector<size_t> loop_destructible_marks_;
    /** Result temp + type for a `loop { break v }` value; parallel to break_targets_. */
    std::vector<PlaceId> break_value_dests_;
    std::vector<TypeId> break_value_types_;
    /** Pattern-match caches: matched lvalue, and the DefValue result temp. */
    std::unordered_map<PatternMatchExpr*, MIRExprResult> pattern_bases_;
    std::unordered_map<PatternMatchExpr*, MIRExprResult> pattern_results_;
    std::vector<std::string> lambda_defs_;
    /** A materialized capturing lambda: closure type/temp + fn/dtor pointers. */
    struct LambdaMaterialization {
        TypeId cap_type = MIR_INVALID_ID;
        PlaceId cap_temp = MIR_INVALID_ID;
        ValueId fn_ptr = MIR_NULL;
        TypeId fn_ptr_type = MIR_INVALID_ID;
        ValueId dtor_ptr = MIR_NULL;
        TypeId dtor_type = MIR_INVALID_ID;
    };
    std::unordered_map<LambdaFunction*, LambdaMaterialization> lambda_materializations_;
    /** The current lambda's closure pointer (a `*struct cap`), while lowering it. */
    ValueId lambda_closure_ptr_ = MIR_NULL;
    TypeId lambda_closure_type_ = MIR_INVALID_ID;
    bool sret_ = false;
    ValueId sret_ptr_ = MIR_NULL;
    TypeId sret_ret_type_ = MIR_INVALID_ID;
    TypeId sret_ptr_type_ = MIR_INVALID_ID;
    BaseType* return_type_ = nullptr; // current function's declared return type
    BaseType* expected_struct_type_ = nullptr; // context type for an untyped struct literal

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
