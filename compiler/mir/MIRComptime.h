// Copyright (c) Chemical Language Foundation 2025.
//
// MIR comptime driver (Phase 5.3 of the interpretation plan).
//
// A `comptime func` call during lowering used to be evaluated by the AST
// interpreter. This driver lowers the callee into its own MIR module and runs it
// on `MIRInterpreter`, then bridges the raw result back to an AST `Value*` that
// the lowering keeps folding. It starts with scalar returns and scalar/string
// arguments; anything out of scope returns nullptr so the caller keeps the AST
// path (progressively widened until the interpreter can be deleted).
//
// See lang/docs/mir-interpretation-plan.md §5.3.

#pragma once

#include <memory>
#include <string>

class FunctionDeclaration;
class FunctionCall;
class ComptimeBlock;
class Value;
class ASTAllocator;
class TypeBuilder;
class InterpretScope;
class SourceLocation;

namespace mir {

class MIRLowerer;

struct MIRComptimeContext {
    /** The live lowerer whose callbacks a nested lowerer copies. */
    MIRLowerer* host = nullptr;
    /** Where the bridged result value (an AST `Value*`) is allocated. */
    ASTAllocator* allocator = nullptr;
    /** Type builder for the bridged result's type. */
    TypeBuilder* type_builder = nullptr;
    /** The caller's comptime scope, used to fold the argument expressions. */
    InterpretScope* scope = nullptr;
};

class MIRComptime {
public:
    MIRComptime();
    ~MIRComptime();

    MIRComptime(const MIRComptime&) = delete;
    MIRComptime& operator=(const MIRComptime&) = delete;

    /**
     * Evaluate `fd(...)` on the MIR engine. Returns the result as an AST value,
     * or nullptr when the call is out of scope for the driver (an intrinsic, a
     * template, a non-scalar return, an unsupported argument, …) — the caller
     * then keeps the AST interpreter.
     */
    Value* evaluate(const MIRComptimeContext& ctx, FunctionDeclaration* fd,
                    FunctionCall* call, bool& handled, std::string& error);

    /**
     * Evaluate a statement-level `comptime { ... }` block on the MIR engine.
     * The block's statements are lowered as a synthetic void function (its own
     * scope: a comptime block cannot see the enclosing function's runtime
     * locals) and run. Returns false with `error` when the engine cannot run it.
     */
    bool evaluate_block(const MIRComptimeContext& ctx, ComptimeBlock* block, std::string& error);

    /**
     * Fold one comptime expression. A comptime call (`f(g(1))`) is lowered and
     * run by the driver (the AST layer cannot run a body); anything else folds
     * through `evaluated_value`. Used by the compiler's value-eval callback so a
     * nested fold (`llvm_mem_order(order)` → `convert_mem_order_to_llvm(order)`)
     * does not need the deleted interpreter.
     */
    Value* evaluate_value(const MIRComptimeContext& ctx, Value* v, std::string& error);

    /** Number of functions lowered into the driver's module (diagnostics). */
    unsigned lowered_function_count() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace mir
