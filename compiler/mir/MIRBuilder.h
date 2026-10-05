// Copyright (c) Chemical Language Foundation 2025.
//
// Typed MIR construction API. The builder owns a per-function instruction
// stream and dense value/place tables, and makes invalid MIR hard to build.
// See mir-implementation-plan.md §4 Stage 2.
//
// Blocks are emitted in creation order: create the blocks you will emit, in the
// order you will emit them, then set_block() before adding instructions. This
// keeps each block's instruction range contiguous in the linear instruction
// stream.

#pragma once

#include "MIRTypes.h"
#include "MIRArena.h"
#include "MIRModule.h"
#include "MIRFunction.h"
#include "MIRInstruction.h"

#include <initializer_list>

namespace mir {

class MIRBuilder {
public:
    MIRBuilder(MIRArena& arena, MIRModule& module, MIRFunction& function)
        : arena_(arena), module_(module), func_(function) {}

    // ── state ──────────────────────────────────────────────────────────────
    MIRArena& arena() { return arena_; }
    MIRModule& module() { return module_; }
    MIRFunction& function() { return func_; }
    bool ok() const { return ok_; }
    const char* error() const { return error_; }

    // ── blocks ─────────────────────────────────────────────────────────────
    /** Reserve a new block id. Emit it via set_block() in creation order. */
    BlockId create_block(uint16_t arg_count = 0);
    /** Make `block` current and set its instruction start if not set. */
    void set_block(BlockId block);
    BlockId current_block() const { return current_block_; }
    bool current_block_terminated() const { return terminated_; }

    // ── value / place creation ─────────────────────────────────────────────
    ValueId new_value(TypeId type, uint8_t flags = VF_NONE);
    PlaceId new_place(TypeId type, MIRStorageClass sc, uint32_t move_path_id = 0);

    /** Append a move-path root node for an owned place; returns its path id. */
    uint32_t new_move_path(PlaceId place, TypeId type);

    // ── generic emit ───────────────────────────────────────────────────────
    /** Emit an instruction with explicit operands. Returns its stream index. */
    uint32_t emit(MIROpcode op, uint32_t result_or_place,
                  const MIROperand* operands, uint32_t count);
    uint32_t emit(MIROpcode op, uint32_t result_or_place,
                  std::initializer_list<MIROperand> operands);

    // ── constants ──────────────────────────────────────────────────────────
    ValueId const_int(TypeId type, ConstantId constant);
    ValueId const_bool(TypeId type, ConstantId constant);
    ValueId const_null(TypeId type, ConstantId constant);
    ValueId const_float(TypeId type, ConstantId constant);
    ValueId const_double(TypeId type, ConstantId constant);
    ValueId const_string(TypeId type, ConstantId constant);
    /** A function parameter, materialized as an SSA value. */
    ValueId param(TypeId type);

    // ── storage ────────────────────────────────────────────────────────────
    /** Store through a pointer: `*address = value`. */
    void store_indirect(ValueId address, ValueId value);
    /** Load through a pointer: `result = *address`. */
    ValueId load_indirect(ValueId address, TypeId type);
    /** Address of a module-level global: `&symbol`. */
    ValueId global_addr(SymbolId symbol, TypeId pointer_type);
    PlaceId alloca(TypeId type, MIRStorageClass sc = MIRStorageClass::Local);
    ValueId load(PlaceId place, TypeId type);
    void store(PlaceId place, ValueId value);
    /** Load a struct field by name: `result = base.<field>`. */
    ValueId field_load(PlaceId base, ConstantId field_name, TypeId field_type);
    /** Load through an index: `result = base[index]`. */
    ValueId index_load(ValueId base, ValueId index, TypeId element_type);
    /** Store through an index: `base[index] = value`. */
    void index_store(ValueId base, ValueId index, ValueId value);
    /** Load from a local array place: `result = base[index]`. */
    ValueId element_load(PlaceId base, ValueId index, TypeId element_type);
    /** Store to a local array place: `base[index] = value`. */
    void element_store(PlaceId base, ValueId index, ValueId value);
    /** Store a struct field by name: `base.<field> = value`. */
    void field_store(PlaceId base, ConstantId field_name, ValueId value);
    ValueId address_of(PlaceId place, TypeId pointer_type);
    /** Address of a struct field; result value is typed as `field_type`. */
    ValueId field_addr(PlaceId base, ConstantId field_index, TypeId field_type);
    /** Address of an array element; result value is typed as `element_type`. */
    ValueId index_addr(PlaceId base, ValueId index, TypeId element_type);
    ValueId gep(ValueId base, TypeId result_type,
                std::initializer_list<ValueId> indices);

    // ── arithmetic / conversion ────────────────────────────────────────────
    ValueId unary(ValueId operand, ConstantId op, TypeId result_type);
    ValueId binary(ValueId lhs, ValueId rhs, ConstantId op, TypeId result_type);
    ValueId compare(ValueId lhs, ValueId rhs, ConstantId predicate, TypeId result_type);
    ValueId cast(ValueId operand, TypeId result_type);
    ValueId select(ValueId cond, ValueId true_v, ValueId false_v, TypeId result_type);

    // ── calls ──────────────────────────────────────────────────────────────
    /** Scalar/void call producing an SSA value. */
    ValueId call_scalar(SymbolId callee, TypeId result_type,
                        const MIROperand* args, uint32_t count);
    /** Struct-return call: writes the result into `result_place`. */
    void call_sret(SymbolId callee, PlaceId result_place,
                   const MIROperand* args, uint32_t count);
    ValueId call_indirect(ValueId fn, TypeId result_type,
                          const MIROperand* args, uint32_t count);

    // ── lifetime ───────────────────────────────────────────────────────────
    void init(PlaceId dest, SymbolId ctor, const MIROperand* args, uint32_t count);
    void copy_init(PlaceId dest, ValueId src, TypeId type);
    void move_init(PlaceId dest, PlaceId src, TypeId type);
    void assign(PlaceId dest, ValueId src, TypeId type);
    void destroy(PlaceId place, SymbolId dtor);
    void drop(PlaceId place, SymbolId dtor, ValueId flag = MIR_NULL);
    void set_drop(PlaceId flag_place, bool value);
    void memcpy(PlaceId dest, PlaceId src, ValueId size);
    void memset(PlaceId dest, ValueId byte, ValueId size);

    // ── terminators ────────────────────────────────────────────────────────
    void ret(ValueId value);
    void ret_void();
    void br(BlockId target);
    void cond_br(ValueId cond, BlockId true_block, BlockId false_block);
    void unreachable();

    // ── cleanup scopes ─────────────────────────────────────────────────────
    uint32_t push_cleanup_scope();
    uint32_t pop_cleanup_scope();
    void register_owned(PlaceId place);
    uint32_t current_cleanup_scope() const { return cleanup_sp_; }

private:
    void note_result_value(ValueId id, uint32_t inst_index);
    void note_value_uses(const MIROperand* operands, uint32_t count);
    uint32_t emit_impl(MIROpcode op, uint32_t result_or_place,
                       const MIROperand* operands, uint32_t count);

    MIRArena& arena_;
    MIRModule& module_;
    MIRFunction& func_;
    BlockId current_block_ = MIR_INVALID_ID; // MIR_INVALID_ID = no current block
    bool terminated_ = true; // no current block yet
    bool ok_ = true;
    const char* error_ = nullptr;
    uint32_t cleanup_sp_ = MIR_INVALID_ID;
};

} // namespace mir
