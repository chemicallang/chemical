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

#include <functional>
#include <string>
#include <unordered_map>
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

private:
    MIRExprResult lower_expr(Value* value, std::string& error);
    bool lower_stmt(ASTNode* node, std::string& error);
    bool lower_scope(Scope& scope, std::string& error);
    bool lower_if(IfStatement* stmt, BlockId merge, std::string& error);
    bool lower_while(WhileLoop* loop, std::string& error);
    bool lower_for(ForLoop* loop, std::string& error);
    bool lower_incdec_value(Value* target, bool increment, std::string& error, ValueId& out);
    PlaceId resolve_place(Value* v, std::string& error);
    bool member_name(Value* v, std::string& out, std::string& error);
    bool lower_call_args(const std::vector<Value*>& values, std::vector<MIROperand>& args,
                         std::string& error);
    MIRExprResult lower_method_call(Value* receiver, FunctionCall* call, std::string& error);

    SymbolId intern_function(FunctionDeclaration* decl);
    PlaceId place_for_linked(ASTNode* linked) const;
    void bind(ASTNode* linked, PlaceId place) { var_places_[linked] = place; }

    MIRModule& module_;
    MIRTypeBuilder& types_;
    MIRBuilder* builder_ = nullptr;
    std::unordered_map<ASTNode*, PlaceId> var_places_;
    std::unordered_map<FunctionDeclaration*, SymbolId> func_symbols_;
    std::function<std::string(ASTNode*)> mangler_;
    std::vector<BlockId> break_targets_;
    std::vector<BlockId> continue_targets_;
    bool sret_ = false;
    ValueId sret_ptr_ = MIR_NULL;
    TypeId sret_ret_type_ = MIR_INVALID_ID;
    TypeId sret_ptr_type_ = MIR_INVALID_ID;
};

} // namespace mir
