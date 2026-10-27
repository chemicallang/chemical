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
#include "MIRHotOps.h"
#include "std/chem_string_view.h"
#include "compiler/async/AsyncLoweringPlan.h"

#include <deque>
#include <functional>
#include <mutex>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

class ASTNode;
class Value;
class StructValue;
class ArrayValue;
class StructDefinition;
class VariantMember;
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
class FunctionType;
class CapturingFunctionType;
class BaseType;
class ForInLoop;
class ImplementationsIndex;
class CoreNodes;
class EmbeddedNode;
class EmbeddedValue;
class VariantDefinition;

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

// ── allocation-light lowering support ──────────────────────────────────────
//
// The lowering hot path must not allocate per node. These two helpers replace
// the two worst offenders: a `std::set<std::string>` built for every struct
// literal, and `std::string` key construction on every name-map lookup.

/**
 * The set of fields a struct literal sets explicitly. A struct has a handful of
 * fields, so a linear scan over the (borrowed, unowned) `chem::string_view`
 * names beats `std::set<std::string>`, which heap-allocates a node and copies
 * every name. The views point into AST-owned storage that outlives the
 * surrounding lowering call, so no ownership is needed.
 *
 * The first `k_inline` names live in the object itself: one of these is built
 * (and copied) for every struct literal, and a `std::vector` would allocate
 * twice for even a two-field struct. Only a struct with more fields than
 * `k_inline` touches the heap.
 */
struct MIRFieldNameSet {
    static constexpr uint32_t k_inline = 8;
    chem::string_view inline_names[k_inline];
    uint32_t count = 0;
    std::vector<chem::string_view> overflow;

    void insert(const chem::string_view& n) {
        if (count < k_inline) {
            inline_names[count++] = n;
            return;
        }
        overflow.push_back(n);
    }
    bool contains(const chem::string_view& n) const {
        for (uint32_t i = 0; i < count; ++i) {
            if (inline_names[i] == n) return true;
        }
        for (const chem::string_view& x : overflow) {
            if (x == n) return true;
        }
        return false;
    }
    /** Only membership matters, so the erased slot is filled from the back. */
    void erase(const chem::string_view& n) {
        for (uint32_t i = 0; i < count; ++i) {
            if (inline_names[i] == n) {
                inline_names[i] = inline_names[--count];
                return;
            }
        }
        for (auto it = overflow.begin(); it != overflow.end(); ++it) {
            if (*it == n) {
                overflow.erase(it);
                return;
            }
        }
    }
};

/**
 * One reused buffer per nesting level. A lowering pass that needs a temporary
 * list acquires one for its scope and gives it back on exit, so after warm-up
 * the buffer keeps its capacity instead of heap-allocating per node. Indexed by
 * a depth counter, so a nested acquisition (a call inside an argument) takes a
 * deeper slot and cannot clobber an outer buffer. `std::deque`, so acquiring
 * cannot invalidate an outer buffer's address.
 */
template <class T>
class MIRScratchPool {
public:
    std::vector<T>& acquire() {
        if (depth_ == bufs_.size()) bufs_.emplace_back();
        std::vector<T>& v = bufs_[depth_++];
        v.clear();
        return v;
    }
    void release() { --depth_; }

private:
    std::deque<std::vector<T>> bufs_;
    size_t depth_ = 0;
};

/** A view over any of the name spellings the maps are keyed/looked up by. */
inline chem::string_view mir_name_view(const chem::string_view& s) { return s; }
inline chem::string_view mir_name_view(const std::string& s) {
    return chem::string_view(s.data(), s.size());
}
inline chem::string_view mir_name_view(const char* s) { return chem::string_view(s); }

/**
 * Transparent hashing/equality for the `std::string`-keyed lowering maps, so a
 * lookup by `chem::string_view` (an identifier, a field/parameter name) does not
 * construct a temporary `std::string`. The stored keys stay owned `std::string`s.
 */
struct MIRStringHash {
    using is_transparent = void;
    template <class T>
    size_t operator()(const T& s) const {
        return std::hash<chem::string_view>{}(mir_name_view(s));
    }
};

struct MIRStringEq {
    using is_transparent = void;
    template <class A, class B>
    bool operator()(const A& a, const B& b) const {
        return mir_name_view(a) == mir_name_view(b);
    }
};

/**
 * Lowers one resolved function at a time. Holds per-function state (variable ->
 * place bindings, callee -> symbol bindings). Not thread-safe; one instance is
 * owned by each worker (or created per function).
 */
struct MIRArgScope;
template <class T> struct MIRScratch;

/**
 * Comptime parameters of the inlined callee, keyed by name. Transparent
 * hash/equality: `lower_expr` looks a binding up by the identifier's
 * `chem::string_view`, so the per-identifier lookup builds no `std::string`.
 */
using MIRComptimeBindings =
    std::unordered_map<std::string, Value*, MIRStringHash, MIRStringEq>;

/**
 * `prefix` immediately followed by a decimal number, spelled into a stack
 * buffer.
 *
 * Every name MIR invents for a frame field, a cursor or a drop flag is copied
 * straight into a module byte pool (or used as a lookup key) and then thrown
 * away, so a `std::string` per name is pure waste — and these names are built
 * per async slot per await site, and per loop. `data()`/`size()` feed both a
 * `chem::string_view` and the `add_string(_, data, size)` spelling; a default
 * constructed value is the "no name" case (`empty()`).
 *
 * The buffer holds 48 bytes: the longest prefix used
 * (`__chx_forin_chunk_cursor_`) plus 20 digits still fits.
 */
struct MIRNumberedName {
    char bytes[48];
    uint32_t len = 0;

    MIRNumberedName() = default;

    MIRNumberedName(const char* prefix, uint64_t n) {
        while (prefix[len] != '\0') {
            bytes[len] = prefix[len];
            ++len;
        }
        char digits[20];
        int dn = 0;
        do {
            digits[dn++] = static_cast<char>('0' + (n % 10));
            n /= 10;
        } while (n);
        while (dn) bytes[len++] = digits[--dn];
    }

    bool empty() const { return len == 0; }
    const char* data() const { return bytes; }
    uint32_t size() const { return len; }
    chem::string_view view() const { return chem::string_view(bytes, len); }
};

class MIRLowerer {
public:
    friend struct MIRArgScope;
    template <class T> friend struct MIRScratch;
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
     * Lower the auto-generated stub of a static interface method into `func`.
     *
     * A method with a body lowers through `lower_function` (the interface's
     * default method); a bodyless one gets a trivial body — `ret 0` for an
     * integer-like return, `ret void`, `unreachable` otherwise — which is what
     * the C backend spells as `{ return 0; }`.
     *
     * The result is flagged `MF_WEAK_STUB`: a static interface's method and the
     * matching method of every impl mangle to the same symbol, so the stub must
     * lose to a real implementation found in another module's object.
     */
    bool lower_static_interface_stub(FunctionDeclaration* decl, MIRArena& arena,
                                     MIRFunction& func, std::string& error);

    /**
     * For an async function, `lower_function` produces the *ramp* as `func` and
     * the coroutine `__poll` here, with the frame/vtable helpers in the pre/post
     * declaration text. The caller emits: pre-decls, poll, post-decls, ramp.
     */
    MIRFunction& async_poll() { return async_poll_; }
    /**
     * The coroutine `__drop` (`void __drop(void* frame)`): destroys the live
     * frame slots and the live child future at the suspended state, then frees
     * the frame. The C backend emits an equivalent body as text; the LLVM
     * backend needs it as MIR because it defines the FutureTable's `drop` slot.
     */
    MIRFunction& async_drop() { return async_drop_; }
    /**
     * The internal symbol an application's `async func main` is renamed to
     * before translation, so the synchronous `int main` trampoline can own the
     * entry name (async-await design Section 5.5). The rename happens in
     * `ASTProcessor::declare_module`, before any prototype is emitted.
     */
    static constexpr const char* kAsyncMainInternalName = "__chx_async_main";
    /** True when `decl` is an application's `async func main`. */
    bool is_async_main_entry(FunctionDeclaration* decl);
    /**
     * Build the synchronous `int main` trampoline that drives `decl`'s ramp to
     * completion and returns its integer result (async-await design Section
     * 5.5). `ramp` is the MIR function `lower_function` produced for `decl`; its
     * hidden sret slot plus its parameters define the entry signature.
     *
     * Returns false with an empty `error` when `decl` is not the async entry
     * point, so a caller can safely call it for every async function.
     */
    bool lower_async_main_trampoline(FunctionDeclaration* decl, const MIRFunction& ramp,
                                     MIRArena& arena, MIRFunction& func, std::string& error);
    std::string& async_pre_decls() { return async_pre_decls_; }
    std::string& async_post_decls() { return async_post_decls_; }
    bool is_async_lowering() const { return async_; }

    MIRTypeBuilder& types() { return types_; }
    MIRModule& module() { return module_; }

    /** Generated C for lambda functions produced while lowering a body. */
    std::vector<std::string>& lambda_defs() { return lambda_defs_; }
    /**
     * MIR for the lambda functions materialized while lowering a body. They are
     * allocated in the enclosing function's arena, so they must be consumed
     * before that arena is reused. The LLVM backend emits them as ordinary
     * functions (the C backend uses `lambda_defs()` instead).
     */
    std::vector<MIRFunction>& lambda_functions() { return lambda_functions_; }
    void clear_lambda_defs() { lambda_defs_.clear(); lambda_functions_.clear(); }

    /** Pre-register a callee symbol (mangled name) so calls resolve to it. */
    void set_function_symbol(FunctionDeclaration* decl, SymbolId symbol) {
        func_symbols_[decl] = symbol;
    }

    /**
     * Register a top-level global that the module being lowered *declares*.
     *
     * A global is defined by exactly one module (the one whose files contain
     * its `VarInitStmt`); every other module only references it. The whole
     * program is one C translation unit, so the C backend cannot tell the
     * difference — but the LLVM backend emits one object per module, where a
     * reference interned as `MIRLinkage::Internal` becomes a second definition
     * and the link fails with "multiple definition". Anything not registered
     * here is therefore interned as `External` (a reference).
     */
    void add_owned_global(VarInitStatement* vi) { owned_globals_.insert(vi); }

    /**
     * Build the constant-expression initializer of a top-level global and record
     * a `MIRGlobal` definition for it (action A3 of mir-migration-evaluation.md).
     * Both backends render the same tree — the C emitter via `const_expr_c`, the
     * LLVM emitter via its own constant builder — so neither walks the AST. Also
     * interns the global's symbol (with the linkage `owned_globals_` implies).
     * Returns the initializer id (`MIR_INVALID_ID` when the global has no value).
     */
    ConstExprId build_global_initializer(VarInitStatement* vi);

    /** On-demand mangler for symbols not pre-registered (e.g. methods). */
    void set_mangler(std::function<std::string(ASTNode*)> fn) {
        mangler_ = std::move(fn);
    }
    /**
     * Append-style mangler: writes the mangled name into a reused buffer instead
     * of returning a fresh `std::string` (see `mangle`). Preferred over
     * `set_mangler` when both are set.
     */
    void set_mangler_into(std::function<void(ASTNode*, std::string&)> fn) {
        mangler_into_ = std::move(fn);
    }
    /**
     * The shared lock guarding every access to the `CTranslationContext` (the
     * mangler, comptime scope, allocator, `iface->active_user`). Set by the
     * translation driver; null in the standalone/serial build. Lets the lowerer
     * serialize the few shared-state operations while the rest runs lock-free.
     */
    void set_visitor_mutex(std::recursive_mutex* m) { visitor_mutex_ = m; }

    /**
     * The lowerer currently lowering on this thread. The translation driver
     * wires the shared comptime callbacks once (capturing the *driver* lowerer);
     * a parallel worker lowers through its own lowerer, so the callbacks consult
     * this to publish and read the worker's folded comptime parameter bindings
     * (`intrinsics::size`, `decode_location`, …) instead of another worker's or
     * the driver's. Null outside a lowering; the callbacks then fall back to the
     * driver's lowerer (the serial behaviour). Set around a function's lowering
     * in `mir_lower_and_emit_into`; a nested driver's own `lower_function` does
     * not overwrite it.
     */
    static MIRLowerer* current_lowering() noexcept;
    /** Set the per-thread current lowerer, returning the previous one. */
    static MIRLowerer* set_current_lowering(MIRLowerer* l) noexcept;

    /**
     * Copy the translation callbacks (mangler, comptime, embedded, debug, the
     * shared lock, the impl indexes) from another lowerer, so a parallel
     * worker's lowerer behaves exactly like the driver's without re-wiring each
     * callback. The `std::function`s capture the same shared state, so the
     * copies are equivalent.
     */
    void copy_callbacks_from(const MIRLowerer& o) {
        mangler_ = o.mangler_;
        mangler_into_ = o.mangler_into_;
        vtable_mangler_ = o.vtable_mangler_;
        vtable_mangler_into_ = o.vtable_mangler_into_;
        comptime_if_resolver_ = o.comptime_if_resolver_;
        comptime_eval_ = o.comptime_eval_;
        comptime_value_eval_ = o.comptime_value_eval_;
        comptime_ctor_eval_ = o.comptime_ctor_eval_;
        comptime_block_eval_ = o.comptime_block_eval_;
        comptime_call_site_ = o.comptime_call_site_;
        symbol_lookup_ = o.symbol_lookup_;
        set_current_func_ = o.set_current_func_;
        impls_index_ = o.impls_index_;
        core_nodes_ = o.core_nodes_;
        embedded_repl_ = o.embedded_repl_;
        embedded_value_repl_ = o.embedded_value_repl_;
        async_normalize_ = o.async_normalize_;
        source_loc_provider_ = o.source_loc_provider_;
        debug_info_ = o.debug_info_;
        visitor_mutex_ = o.visitor_mutex_;
    }

    /** Mangler for a dynamic-dispatch vtable global (interface + impl type). */
    void set_vtable_mangler(std::function<std::string(ASTNode*, BaseType*)> fn) {
        vtable_mangler_ = std::move(fn);
    }
    /**
     * Append-style vtable mangler (see `vtable_mangle`), preferred over
     * `set_vtable_mangler` when both are set — the string-returning form
     * allocates a `std::string` per `dyn<T>` value lowered.
     */
    void set_vtable_mangler_into(
        std::function<void(ASTNode*, BaseType*, std::string&)> fn) {
        vtable_mangler_into_ = std::move(fn);
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

    /** Interpret a `comptime { }` block (interpreter-backed, no runtime code). */
    void set_comptime_block_eval(std::function<void(ASTNode*)> fn) {
        comptime_block_eval_ = std::move(fn);
    }

    /**
     * Push/pop the interpreter's call stack for the duration of an `@expand`
     * inline. Call-site introspection intrinsics (`intrinsics::get_call_loc`,
     * `get_line_no`, …) resolve through that stack; inlining bypasses the
     * interpreter's own frame for the call, so without this the site would be
     * reported as the macro's own body.
     */
    void set_comptime_call_site(std::function<void(FunctionCall*, bool)> fn) {
        comptime_call_site_ = std::move(fn);
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

    /** Resolve a CBI embedded node (`#html` / `#universal`) to its replacement. */
    void set_embedded_repl(std::function<ASTNode*(EmbeddedNode*)> fn) {
        embedded_repl_ = std::move(fn);
    }

    /** Normalize an async function body (hoist nested awaits) before lowering. */
    void set_async_normalize(std::function<void(FunctionDeclaration*)> fn) {
        async_normalize_ = std::move(fn);
    }

    /** Resolve a CBI embedded *value* to its replacement value. */
    void set_embedded_value_repl(std::function<Value*(EmbeddedValue*)> fn) {
        embedded_value_repl_ = std::move(fn);
    }

    /**
     * Resolve a node's encoded source location into a `MIRSourceLoc`. Returns
     * false when the location is invalid/unknown. The core stays AST-free; the
     * driver supplies this from the `LocationManager`.
     */
    void set_source_loc_provider(std::function<bool(uint64_t, MIRSourceLoc&)> fn) {
        source_loc_provider_ = std::move(fn);
    }

    /**
     * Enable recording of debug-only MIR metadata — the source name of every
     * named place (`MIRPlaceDef::debug_name`) and the lexical extent of every
     * cleanup scope. Wire this from `options->debug_info` alongside
     * `set_source_loc_provider`; with it off (release / `debug_quick`) the
     * lowerer records no names and emits nothing extra.
     */
    void set_debug_info(bool on) { debug_info_ = on; }

    /**
     * The `FileScope`s of the files that were parsed for the module currently
     * being lowered. A non-retained function's body lives on the module
     * allocator, which is cleared once its module is compiled, so a body
     * reached through any other file may already be freed (a cross-module
     * reference only keeps the job-allocated *declaration* alive). Lowering
     * may only inspect the body of a function that is either body-retained or
     * declared in one of these files. The set must outlive the lowering; the
     * driver passes a per-module set.
     */
    void set_current_module_file_scopes(const std::unordered_set<ASTNode*>* scopes) {
        current_module_file_scopes_ = scopes;
    }

    /**
     * Compute and cache the "constructor writes the whole result" fact on `fn`
     * while its body is known to be alive (call this when lowering the
     * constructor itself, which happens in the module that owns it). A later
     * module then consumes the cached fact instead of re-reading a body its
     * module allocator already freed.
     */
    void prime_constructor_fact(FunctionDeclaration* fn);

private:
    MIRExprResult lower_expr(Value* value, std::string& error);
    bool lower_stmt(ASTNode* node, std::string& error);
    /** Attach `node`'s source location to subsequently emitted instructions. */
    void note_source(ASTNode* node);
    bool lower_scope(Scope& scope, std::string& error);
    /** Lower a list of statements as a scope, destroying its locals at the end. */
    bool lower_scope_nodes(std::vector<ASTNode*>& nodes, std::string& error);
    /**
     * `lower_scope_nodes` wrapped in a debug lexical scope: in a debug build the
     * scope's instruction extent and source location are recorded on the current
     * function so the LLVM emitter can build a matching `DILexicalBlock`. A plain
     * `lower_scope_nodes` call when debug info is off.
     */
    bool lower_scope_nodes_scoped(std::vector<ASTNode*>& nodes, uint64_t encoded_loc,
                                 std::string& error);
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

    /**
     * Emit a runtime bounds check for an index into a fixed-size array. The
     * check is a `Compare` + `CondBr` to an `abort()` call and `Unreachable`;
     * pointer/vector/struct indexing is left to its own API. Values that index
     * an array through an address (multi-dimensional arrays) use the address's
     * element type, so every dimension is checked.
     */
    void emit_array_index_bounds_check(const MIRExprResult& base, const MIRExprResult& index);

    /** Branch to an `abort()` call and `Unreachable` unless `condition` is true. */
    void emit_abort_unless(ValueId condition);

    /** Reject integer division/remainder by zero and signed MIN / -1. */
    void emit_division_check(ValueId lhs, ValueId rhs, TypeId type, MIRBinaryOp op);

    /** Reject shifts whose amount is negative or >= the type's bit width. */
    void emit_shift_check(ValueId amount, TypeId type);

    /** Apply the division/shift guards for a compound assignment operator. */
    void emit_compound_assignment_checks(MIRBinaryOp op, ValueId lhs, ValueId rhs, TypeId type);
    /** Emit a `malloc(size)` call yielding `ptr_t`. */
    ValueId emit_malloc(TypeId size_v, TypeId u64t, TypeId ptr_t);
    /** Emit a `free(ptr)` call. */
    void emit_free(ValueId ptr);
    MIRExprResult lower_incdec_value(Value* target, bool increment, bool post,
                                     std::string& error);
    /** Lower an inline-asm operand to a Place or a pointer Value (`*ptr`). */
    bool lower_asm_operand(Value* expr, MIROperand& out, std::string& error);
    /**
     * Lower an `intrinsics::llvm::atomic_*` call into a first-class MIR atomic
     * op. These intrinsics are the LLVM backend's spelling for atomics; they
     * cannot be interpreted during MIR construction (their operands are runtime
     * locations and an LLVM atomic instruction needs LLVM ordering constants,
     * not an interpreter `PointerValue`). The selectors (order / scope / op /
     * weak) must be compile-time constants, exactly as LLVM requires.
     */
    MIRExprResult lower_atomic_intrinsic(FunctionCall* call, FunctionDeclaration* fd,
                                         std::string& error);
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
    /**
     * `resolve_place` for a caller that only needs the yes/no answer and
     * discards the message — it builds no diagnostic string on the miss path.
     */
    PlaceId resolve_place_silent(Value* v);
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
    MIRExprResult lower_address_of(Value* inner, std::string& error,
                                    bool raw_mutable = false);
    bool member_name(Value* v, std::string& out, std::string& error);
    /**
     * The member name a `Value` names, as a view into AST-owned storage (empty
     * when it does not name a member). Use this on paths that only compare or
     * hash the name — `member_name` copies it into a `std::string`.
     */
    chem::string_view member_name_view(Value* v);
    /** Resolve a (possibly inherited) member to its dotted C field path. */
    bool member_field_path(Value* base, Value* member, std::string& out, std::string& error);
    /**
     * If `v` names a by-value aggregate parameter of the function being lowered,
     * return it (and its argument pointer is in `aggregate_param_ptrs_`).
     */
    FunctionParam* aggregate_param_operand(Value* v);
    bool lower_call_args(const std::vector<Value*>& values, std::vector<MIROperand>& args,
                         std::string& error);
    /**
     * Lower a call's arguments. `ast_fn` is the call's AST function type for an
     * indirect call (null for a direct one): it supplies the parameter types,
     * which decide by-value vs by-reference argument handling. Passed as the
     * AST node itself, so no per-call `std::vector<BaseType*>` is built.
     */
    bool lower_call_args_for(FunctionDeclaration* fd, bool self_included,
                             const std::vector<Value*>& values, std::vector<MIROperand>& args,
                             std::string& error, const FunctionType* ast_fn = nullptr);
    MIRExprResult lower_arg_converted(Value* arg, BaseType* param_type, std::string& error);
    bool push_call_arg(MIRExprResult r, std::vector<MIROperand>& args, std::string& error);
    void destroy_call_temps(std::string& error);
    void destroy_call_temps_from(size_t mark, std::string& error);
    SymbolId destructor_symbol(BaseType* type);
    struct MIRDropField {
        chem::string_view name;
        BaseType* type = nullptr;
        PlaceId flag = MIR_INVALID_ID;
        // `MIR_INVALID_ID` for a struct field; the element index otherwise.
        uint32_t array_index = MIR_INVALID_ID;
        // Nested owned fields of a generated-glue struct (one extra level).
        std::vector<MIRDropField> children;
    };
    struct MIRDestructible {
        PlaceId place;
        PlaceId flag;
        SymbolId dtor;
        // Non-empty for a bare local whose struct has generated drop glue: the
        // value may be built field by field, so each owned field carries its own
        // drop flag and scope exit destroys only the initialized ones.
        std::vector<MIRDropField> fields;
        // A mutable raw pointer to the value (or a subobject) escaped: writes
        // may bypass the per-field flags, so fall back to the whole-value drop.
        bool raw_escaped = false;
    };
    void register_destructible(PlaceId place, BaseType* type,
                               bool whole_value_initialized = true);
    /** Destroy one member of `self` (used to lower a `@delete` method). */
    void emit_member_dtor(ValueId self_ptr, const chem::string_view& member, BaseType* mt,
                          std::string& error);
    /** Destroy one member only when its per-field drop flag is true. */
    void emit_member_dtor_cond(ValueId self_ptr, const chem::string_view& member,
                               BaseType* mt, PlaceId flag);
    /** Per-element drop flag for `place[index]`, or MIR_INVALID_ID. */
    PlaceId drop_flag_for_element(PlaceId place, uint64_t index);
    /** Build the drop node for one owned field (recursing one level). */
    MIRDropField build_drop_field(const chem::string_view& name, BaseType* type,
                                  int depth);
    /** Destroy every initialized field in a drop-node list. */
    void destroy_drop_fields(ValueId self_ptr, std::vector<MIRDropField>& fields);
    /** Arm/clear every flag in a drop-node list. */
    void set_drop_fields(std::vector<MIRDropField>& fields, bool value);
    /** Find the drop node for a field path (`inner.x`), or nullptr. */
    MIRDropField* drop_field_for_path(PlaceId place,
                                      const std::vector<std::string>& path);
    /** Arm every flag under a field path (whole-subobject initialization). */
    void mark_path_initialized(PlaceId place, const std::vector<std::string>& path);
    /** A mutable raw pointer escaped from this place: use the whole-value drop. */
    void mark_raw_escaped(PlaceId place);
    /**
     * A member write to a non-field-wise local with no owned subobjects:
     * treat the value as initialized for its whole-value destructor (the
     * hand-built `var d : Destructible; d.field = ...` pattern).
     */
    void mark_member_initialized(PlaceId place);
    /** Destroy every initialized field of a field-wise destructible. */
    void destroy_field_wise(MIRDestructible& d);
    /** Emit StorageDead for every place registered at/after `mark`. */
    void emit_storage_dead_from(size_t mark);
    /** Places registered in the current lexical scope (S4.4c). */
    std::vector<PlaceId> scope_places_;
    /** Destroy every destructible member of the enclosing `@delete` method. */
    void emit_destructor_members(std::string& error);
    /** Destroy the active payload of a variant inside its `@delete` method. */
    void emit_variant_destructor_members(VariantDefinition* vd, ValueId self_ptr,
                                         std::string& error);
    void mark_moved(PlaceId place);
    void emit_drops();
    /** Drop destructibles registered at/after `mark`, innermost first. */
    void emit_drops_from(size_t mark);
    bool append_default_args(FunctionDeclaration* fd, size_t provided, bool self_included,
                             std::vector<MIROperand>& args, std::string& error);
    /** Walk `values[0..count-1]` as a member chain, returning the last field. */
    MIRExprResult lower_member_chain(Value* const* values, size_t count,
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
                               const MIRFieldNameSet& explicit_fields,
                               std::string& error);
    /**
     * Whether a struct literal with `explicit_fields` set will define every
     * runtime member of `def` (explicitly, or through `apply_struct_defaults`),
     * so the whole-temp `zero_init` can be skipped. Conservative: any member
     * that would be left uninitialized (no default, not a struct) returns false.
     */
    bool struct_literal_defines_all_fields(ExtendableMembersContainerNode* def,
                                           const MIRFieldNameSet& explicit_fields);
    /**
     * Whether default-initializing `def` is guaranteed to write every runtime
     * member of the value — i.e. whether `apply_struct_defaults` (or the
     * generated parameterless constructor that runs it through
     * `apply_struct_defaults_ptr`) leaves no byte undefined. When it holds, the
     * whole-value `zero_init` guarding a temporary that is only ever
     * default-initialized is dead (evaluation §1.2, action C1).
     *
     * Mirrors `apply_struct_defaults` exactly and is deliberately conservative:
     * a base/member built through a *user* constructor or any member with no
     * default value that is not a `struct` type returns false. `depth` guards an
     * illegal by-value cycle (which `apply_struct_defaults` would itself
     * recurse into).
     */
    bool default_init_writes_all_fields(ExtendableMembersContainerNode* def, int depth = 0);
    /**
     * Whether a constructor's result is a whole-value copy of a fully lowered
     * temporary, so its bytes are all defined and the `zero_init` guarding the
     * result temporary at a `T(args)` call site is dead. True for the omnipresent
     * `{ return T { field: expr, ... } }` shape (action C1).
     */
    bool constructor_writes_whole_value(FunctionDeclaration* ctor);
    /**
     * Whether the temporary at a `T(args)` construction site may skip its
     * whole-value `zero_init`: the constructor's body writes every byte, or the
     * constructor is compiler-generated and its body is exactly
     * `apply_struct_defaults_ptr` over `cdef` (action C1). The body proof is
     * cached on the declaration the first time it is computed while the body is
     * alive, so a later module never re-reads a freed body.
     */
    bool construction_writes_whole_value(FunctionDeclaration* ctor,
                                         ExtendableMembersContainerNode* cdef);
    /**
     * Whether `fn`'s body is safe to inspect: it is retained for the whole job,
     * or it was parsed as part of the module currently being lowered. A body from
     * an earlier module was freed by `mod_allocator->clear()` (only the
     * job-allocated declaration survives), so reading it is a use-after-free.
     */
    bool function_body_is_readable(FunctionDeclaration* fn);
    /** Like `apply_struct_defaults` but through a `self` pointer (generated ctor). */
    bool apply_struct_defaults_ptr(ValueId self_ptr, ExtendableMembersContainerNode* def,
                                   std::string& error);
    /** Store an anonymous struct/union literal's fields into `addr` in place.
     *  `enclosing_zeroed` is true when the enclosing aggregate temporary is
     *  already `zero_init`'d, so a field an empty array literal would zero is
     *  already defined and the store can be skipped. */
    bool store_anon_aggregate(ValueId addr, StructValue* sv, std::string& error,
                              bool enclosing_zeroed = false);
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
    bool async_lower_drop_body(FunctionDeclaration* decl, std::string& error);
    bool async_lower_ramp(FunctionDeclaration* decl, MIRFunction& func, std::string& error);
    bool async_lower_await_var_init(VarInitStatement* stmt, size_t site_index, std::string& error);
    void async_emit_spill(const struct AwaitSite& site);
    void async_emit_reload(const struct AwaitSite& site);
    void async_emit_pending_return();
    MIRNumberedName async_slot_field(unsigned id) const;
    MIRNumberedName async_child_field(unsigned id) const;
    MIRNumberedName async_drop_flag_field(unsigned id) const;
    MIRNumberedName async_resident_field(ASTNode* node) const;
    PlaceId async_resident_place(ASTNode* node) const;

    SymbolId intern_function(FunctionDeclaration* decl);
    /** Build this function's region signature from its declaration (S4.2). */
    uint32_t intern_region_signature(FunctionDeclaration* decl);
    /** Allocate a fresh function-local region variable (S4.4b). */
    uint32_t fresh_region_id();
    SymbolId intern_global(VarInitStatement* vi);
    SymbolId intern_named_global(const std::string& name);

    // ── global initializer constant expressions (action A3) ──────────────────
    /** A general `Value*` → constant-expression node (see MIRConstExpr.h). */
    ConstExprId build_const_expr(Value* value);
    /** A `(T){ .base = ..., .member = default }` default construction of `sd`. */
    ConstExprId build_const_struct_default(StructDefinition* sd);
    /** A struct literal, including its inherited bases and defaulted members. */
    ConstExprId build_const_struct_value(StructValue* sv);
    /** An array literal (positional children; the tail is left to the backend). */
    ConstExprId build_const_array(ArrayValue* av);
    /** A `FunctionCall`/`AccessChain`: variant construction, comptime fold, or a
     *  function reference / global-constant leaf. */
    ConstExprId build_const_call(Value* value);
    /** A variant-construction node (`Variant.Member(args)`). */
    ConstExprId build_const_variant(VariantMember* vm, FunctionCall* call);
    /** A bare variant member (`Variant.Member`, no args). */
    ConstExprId build_const_variant_bare(VariantMember* vm, BaseType* type);
    // ── inline splice scope ─────────────────────────────────────────────────
    // An `@expand` splice lowers the callee body in the *caller's* scope, so
    // the callee's bindings are layered on top of the caller's. This journal
    // records each binding added during the splice so it can be undone exactly,
    // leaving the caller's scope untouched.
    struct ScopeUndo {
        uint8_t kind = 0;   // 0 var_places_, 1 name_places_, 2 comptime
        ASTNode* node = nullptr;
        /**
         * A view of the *map's own key* (kinds 1/2), not a copy: a binding is
         * journalled for every local declared inside a splice/loop body, and
         * `std::unordered_map` keeps element addresses stable across rehash, so
         * undo can use the view and erase by iterator without reading it after
         * the element dies. A copy per binding would double the cost of the
         * journal.
         */
        chem::string_view name;
        bool existed = false;
        PlaceId place = MIR_INVALID_ID;
        Value* cval = nullptr;
    };
    std::vector<ScopeUndo> scope_undo_;
    bool scope_journal_ = false;
    void undo_scope_from(size_t mark);
    void set_comptime_binding(const std::string& key, Value* value);
    PlaceId place_for_linked(ASTNode* linked) const;
    void bind(ASTNode* linked, PlaceId place) {
        MIR_OP(place_bind);
        auto it = var_places_.find(linked);
        if (it == var_places_.end()) {
            var_places_.emplace(linked, place);
            if (scope_journal_) {
                ScopeUndo u;
                u.kind = 0;
                u.node = linked;
                scope_undo_.push_back(std::move(u));
            }
        } else {
            if (scope_journal_) {
                ScopeUndo u;
                u.kind = 0;
                u.node = linked;
                u.existed = true;
                u.place = it->second;
                scope_undo_.push_back(std::move(u));
            }
            it->second = place;
        }
    }
    /**
     * Mangle `n` into a reused buffer and return it. Valid until the next
     * `mangle` call — every caller uses it immediately and never holds two.
     */
    const std::string& mangle(ASTNode* n) {
        mangle_buf_.clear();
        if (mangler_into_) mangler_into_(n, mangle_buf_);
        else if (mangler_) mangle_buf_ = mangler_(n);
        return mangle_buf_;
    }
    /**
     * Mangle the vtable global of `iface` for `impl_type` into a reused buffer.
     * Valid until the next `vtable_mangle` call — use it immediately.
     */
    const std::string& vtable_mangle(ASTNode* iface, BaseType* impl_type) {
        vtable_mangle_buf_.clear();
        if (vtable_mangler_into_) vtable_mangler_into_(iface, impl_type, vtable_mangle_buf_);
        else if (vtable_mangler_) vtable_mangle_buf_ = vtable_mangler_(iface, impl_type);
        return vtable_mangle_buf_;
    }
public:
    /**
     * The mangled name of `n` as a copy. A driver that lowers into its own
     * module (`MIRComptime`) uses this to build a *unique* per-declaration name:
     * generic instantiations share the host's mangled name, which a
     * module-local `fn_by_name` lookup would then collide on.
     */
    std::string mangled_name_copy(ASTNode* n) { return std::string(mangle(n)); }
    /** Push/pop the interpreter call stack for a driven comptime call (a driver
     * reuses the host's call-site callback so location intrinsics resolve). */
    void invoke_call_site(FunctionCall* call, bool push) {
        if (comptime_call_site_) comptime_call_site_(call, push);
    }
    /**
     * Copy the callbacks a nested fold needs (not the declaration/lowering
     * callbacks, which a driver must not inherit): a value fold such as the
     * `int_arg` of an atomic intrinsic, and the call-site push. Both run with
     * the driver's published parameter bindings visible.
     */
    void copy_comptime_fold_callbacks_from(const MIRLowerer& o) {
        comptime_eval_ = o.comptime_eval_;
        comptime_value_eval_ = o.comptime_value_eval_;
        comptime_call_site_ = o.comptime_call_site_;
    }
private:
    // ── comptime-template inlining (Phase 5) ───────────────────────────────
    /**
     * True when `fd` is a `comptime func` that must be *lowered at its call
     * site*, not driven as a whole: an `@expand` splice, or a function with
     * `%runtime` / `%maybe_runtime` / `%literal` (or `self`) operands. Such a
     * call is expanded by inlining the callee's body — the comptime operands
     * folded to constants, the runtime operands bound to the caller's argument
     * results. See lang/docs/runtime-value-replacement.md.
     */
    bool is_comptime_template_fn(FunctionDeclaration* fd);
    /** Inline a comptime-template call; returns the body's `return` value. */
    MIRExprResult inline_comptime_template(FunctionDeclaration* fd, MIRExprResult recv,
                                           FunctionCall* call,
                                           const std::vector<Value*>& call_values,
                                           std::string& error);

    struct ComptimeInlineFrame {
        BlockId end_block = MIR_NULL;
        PlaceId result_place = MIR_INVALID_ID;
        TypeId result_type = MIR_INVALID_ID;
        size_t dmark = 0; // destructibles_ size at inline start
        bool void_result = false;
    };
    std::vector<ComptimeInlineFrame> inline_frames_;
    /**
     * Comptime parameters of the inlined callee, keyed by name, bound to their
     * folded values. Read by `lower_expr` for identifier references, and pushed
     * into the comptime interpret scope by the compiler callbacks so a nested
     * comptime intrinsic (`intrinsics::size`, `satisfies`, …) folds with the
     * callee's parameters visible.
     */
    MIRComptimeBindings comptime_bindings_;
    bool comptime_binding_consumed_ = false;
public:
    const MIRComptimeBindings& comptime_binding_names() const {
        return comptime_bindings_;
    }
    /** Snapshot / restore the comptime parameter bindings (driver use). */
    MIRComptimeBindings snapshot_comptime_bindings() const {
        return comptime_bindings_;
    }
    void restore_comptime_bindings(MIRComptimeBindings m) {
        comptime_bindings_ = std::move(m);
    }
    /** Publish a folded comptime parameter by name (driver use). */
    void publish_comptime_binding(const std::string& key, Value* value) {
        set_comptime_binding(key, value);
    }
    /**
     * Set when a published comptime parameter is consumed to fold a value while
     * a callee is being lowered. Such a lowering is *call-site dependent*, so the
     * driver must not reuse its MIR for another call site.
     */
    void note_comptime_binding_consumed() { comptime_binding_consumed_ = true; }
    bool take_comptime_binding_consumed() {
        const bool v = comptime_binding_consumed_;
        comptime_binding_consumed_ = false;
        return v;
    }
private:
    void bind_name(const chem::string_view& name, PlaceId place) {
        if (name.empty()) return;
        MIR_OP(name_bind);
        // Look up with the view and insert the key in place: a re-binding (every
        // local inside a loop body, on every iteration) allocates nothing, and a
        // fresh binding pays exactly one allocation — the map key — instead of
        // one for a temporary key plus another for the copy inside the map.
        auto it = name_places_.find(name);
        if (it == name_places_.end()) {
            it = name_places_.emplace(std::string(name.data(), name.size()), place).first;
            if (scope_journal_) {
                ScopeUndo u;
                u.kind = 1;
                u.name = chem::string_view(it->first.data(), it->first.size());
                scope_undo_.push_back(std::move(u));
            }
        } else {
            if (scope_journal_) {
                ScopeUndo u;
                u.kind = 1;
                u.name = chem::string_view(it->first.data(), it->first.size());
                u.existed = true;
                u.place = it->second;
                scope_undo_.push_back(std::move(u));
            }
            it->second = place;
        }
        // Debug-info builds only: remember the source identifier so the LLVM
        // emitter can attach a `DILocalVariable` + `llvm.dbg.declare` to the
        // place's alloca. The common release / debug_quick path pays nothing.
        if (debug_info_ && builder_ && place != MIR_INVALID_ID) {
            builder_->set_place_debug_name(place, name.data(),
                                           static_cast<uint32_t>(name.size()));
        }
    }
    PlaceId place_for_name(const chem::string_view& name) const {
        if (name.empty()) return MIR_INVALID_ID;
        MIR_OP(name_lookup);
        auto it = name_places_.find(name);
        return it == name_places_.end() ? MIR_INVALID_ID : it->second;
    }

    MIRModule& module_;
    MIRTypeBuilder& types_;
    MIRBuilder* builder_ = nullptr;
    MIRArena* arena_ = nullptr;
    std::unordered_map<ASTNode*, PlaceId> var_places_;
    std::unordered_map<std::string, PlaceId, MIRStringHash, MIRStringEq> name_places_;
    /**
     * For every by-value aggregate (struct/union/variant) parameter of the
     * function being lowered: the hidden *pointer* the argument arrived in
     * (i.e. the caller's object). The parameter's own place is a by-value
     * *copy* of that object, so an address that escapes the call — a `dyn<T>`
     * fat pointer is the only such address — must use this pointer instead, or
     * it would point at a dead callee frame. Matches the interpreter (which
     * shares the argument value) and the old AST LLVM backend (`llvm_pointer`
     * on a parameter yields the argument pointer).
     */
    std::unordered_map<FunctionParam*, ValueId> aggregate_param_ptrs_;
    std::vector<std::pair<PlaceId, BaseType*>> call_temps_;
    /**
     * Pooled buffers for the lists lowering builds per node — call/construction
     * arguments, function-signature parameter types, constant-tree children — so
     * none of them allocates a fresh `std::vector` per site. Driven by
     * `MIRArgScope` / `MIRScratch<T>`; one pool per element type so a nested
     * acquisition cannot clobber an outer buffer of a different kind.
     */
    MIRScratchPool<MIROperand> arg_pool_;
    MIRScratchPool<TypeId> type_pool_;
    MIRScratchPool<MIRConstChild> const_child_pool_;

    /** The scratch pool a `MIRScratch<T>` scope draws its buffer from. */
    template <class T> MIRScratchPool<T>& scratch_pool();
    std::vector<MIRDestructible> destructibles_;
    std::unordered_map<FunctionDeclaration*, SymbolId> func_symbols_;
    std::unordered_map<std::string, SymbolId, MIRStringHash, MIRStringEq> named_method_symbols_;
    std::unordered_map<ASTNode*, SymbolId> global_symbols_;
    std::unordered_set<VarInitStatement*> owned_globals_;
    std::unordered_map<std::string, SymbolId, MIRStringHash, MIRStringEq> named_globals_;
    std::function<std::string(ASTNode*)> mangler_;
    std::function<void(ASTNode*, std::string&)> mangler_into_;
    /** Shared `CTranslationContext` lock (see `set_visitor_mutex`), or null. */
    std::recursive_mutex* visitor_mutex_ = nullptr;
    /** Reused output for `mangle` (keeps capacity across declarations). */
    std::string mangle_buf_;
    std::function<std::string(ASTNode*, BaseType*)> vtable_mangler_;
    std::function<void(ASTNode*, BaseType*, std::string&)> vtable_mangler_into_;
    /** Reused output for `vtable_mangle` (keeps capacity across values). */
    std::string vtable_mangle_buf_;
    /**
     * Reused buffer for a variant payload's `<case>.<param>` designator. Every
     * use interns the bytes into the constant pool before lowering anything
     * else, so one buffer per lowerer is enough.
     */
    std::string variant_path_buf_;
    std::string& variant_field_path_buf() { return variant_path_buf_; }
    std::function<Scope*(IfStatement*, std::string&)> comptime_if_resolver_;
    std::function<Value*(FunctionCall*, FunctionDeclaration*)> comptime_eval_;
    std::function<Value*(Value*)> comptime_value_eval_;
    std::function<Value*(FunctionDeclaration*, Value*)> comptime_ctor_eval_;
    std::function<void(ASTNode*)> comptime_block_eval_;
    std::function<void(FunctionCall*, bool)> comptime_call_site_;
    std::function<FunctionDeclaration*(const std::string&)> symbol_lookup_;
    std::function<void(FunctionTypeBody*)> set_current_func_;
    /** True while lowering a `comptime func` (folds is_comptime()/is_runtime()). */
    bool current_fn_is_comptime_ = false;
    ImplementationsIndex* impls_index_ = nullptr;
    CoreNodes* core_nodes_ = nullptr;
    std::function<ASTNode*(EmbeddedNode*)> embedded_repl_;
    std::function<Value*(EmbeddedValue*)> embedded_value_repl_;
    std::unordered_map<EmbeddedValue*, Value*> embedded_value_cache_;
    std::function<void(FunctionDeclaration*)> async_normalize_;
    std::function<bool(uint64_t, MIRSourceLoc&)> source_loc_provider_;
    /** Record debug-only metadata (place names, lexical scopes). See set_debug_info. */
    bool debug_info_ = false;
    /**
     * Files parsed for the module currently being lowered, or null when unknown.
     * Guards body inspection against module-allocator reuse — see
     * `set_current_module_file_scopes`.
     */
    const std::unordered_set<ASTNode*>* current_module_file_scopes_ = nullptr;
    std::unordered_map<std::string, SymbolId, MIRStringHash, MIRStringEq> extern_symbols_;
    /** Struct temporaries whose destruction is deferred to the statement end. */
    std::vector<std::pair<PlaceId, BaseType*>> stmt_temps_;
    FunctionTypeBody* current_func_ = nullptr;
    /** The `self` parameter of the function currently being lowered (if any). */
    FunctionParam* current_self_param_ = nullptr;
    /** For a `@delete` method, the container whose members are auto-destroyed. */
    ExtendableMembersContainerNode* destructor_def_ = nullptr;
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
    std::vector<MIRFunction> lambda_functions_;
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
    MIRFunction async_drop_;
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
    /** set once the `int main` trampoline has been built, so it is emitted once */
    bool async_main_emitted_ = false;
    // per-site lowering info
    std::string async_mangled_;
    std::unordered_map<ASTNode*, size_t> async_site_index_;
    std::vector<PlaceId> async_child_places_;
    std::vector<PlaceId> async_result_places_;
    std::vector<std::vector<std::pair<PlaceId, uint32_t>>> async_spills_;
    std::vector<BlockId> async_cont_blocks_;
};

/**
 * RAII handle over a `MIRLowerer` scratch pool of element type `T` (see
 * `MIRScratchPool`). Use it in place of a local `std::vector<T>`:
 *
 *     MIRScratch<TypeId> _types(*this);
 *     std::vector<TypeId>& ptypes = _types.v;
 *
 * Recursion-safe (each nesting level takes a deeper pool slot) and
 * allocation-free after the pool warms up. Destroyed in reverse declaration
 * order, which is what keeps the pool's depth counter consistent.
 */
template <class T>
struct MIRScratch {
    MIRScratchPool<T>& pool;
    std::vector<T>& v;
    explicit MIRScratch(MIRLowerer& o) : pool(o.scratch_pool<T>()), v(pool.acquire()) {}
    ~MIRScratch() { pool.release(); }
    MIRScratch(const MIRScratch&) = delete;
    MIRScratch& operator=(const MIRScratch&) = delete;
};

/**
 * RAII handle over `MIRLowerer`'s pooled operand buffers. Construct it in place
 * of a local `std::vector<MIROperand>`:
 *
 *     MIRArgScope _args(*this);
 *     std::vector<MIROperand>& args = _args.v;
 *
 * and drop the local when it goes out of scope. Recursion-safe (each nesting
 * level takes a deeper pool slot) and allocation-free after the pool warms up.
 */
struct MIRArgScope {
    MIRScratch<MIROperand> scratch;
    std::vector<MIROperand>& v;
    explicit MIRArgScope(MIRLowerer& o) : scratch(o), v(scratch.v) {}
    MIRArgScope(const MIRArgScope&) = delete;
    MIRArgScope& operator=(const MIRArgScope&) = delete;
};

template <> inline MIRScratchPool<MIROperand>& MIRLowerer::scratch_pool<MIROperand>() {
    return arg_pool_;
}
template <> inline MIRScratchPool<TypeId>& MIRLowerer::scratch_pool<TypeId>() {
    return type_pool_;
}
template <> inline MIRScratchPool<MIRConstChild>& MIRLowerer::scratch_pool<MIRConstChild>() {
    return const_child_pool_;
}

} // namespace mir
