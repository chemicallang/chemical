// Copyright (c) Chemical Language Foundation 2025.

#include "MIRMoveCheck.h"

#include <deque>
#include <string>
#include <unordered_map>

namespace mir {

namespace {

MIRMoveState join_state(MIRMoveState a, MIRMoveState b) {
    return a == b ? a : MIRMoveState::Maybe;
}

const char* state_name(MIRMoveState s) {
    switch (s) {
        case MIRMoveState::Uninitialized: return "uninitialized";
        case MIRMoveState::Initialized: return "initialized";
        case MIRMoveState::Moved: return "moved";
        case MIRMoveState::Destroyed: return "destroyed";
        case MIRMoveState::PartiallyMoved: return "partially moved";
        case MIRMoveState::Maybe: return "conditionally initialized";
    }
    return "unknown";
}

void set_subtree(const MIRFunction& fn, std::vector<MIRMoveState>& states,
                 uint32_t path, MIRMoveState state) {
    if (path == MIR_INVALID_ID || path >= states.size()) return;
    states[path] = state;
    const uint32_t first = fn.move_paths[path].first_child;
    if (first == MIR_INVALID_ID) return;
    uint32_t child = first;
    while (child != MIR_INVALID_ID && child < fn.move_paths.size()) {
        const uint32_t next = fn.move_paths[child].next_sibling;
        set_subtree(fn, states, child, state);
        child = next;
    }
}

/**
 * Mark `path` (and its owned children) moved, and demote every ancestor to
 * `PartiallyMoved`: the parent still owns its other subobjects but can no longer
 * be destroyed or moved as a whole. Partial moves are rejected earlier by the
 * front end, but the MIR dataflow still has to keep the aggregate state honest.
 */
void mark_path_moved(const MIRFunction& fn, std::vector<MIRMoveState>& states,
                     uint32_t path) {
    if (path == MIR_INVALID_ID || path >= states.size()) return;
    set_subtree(fn, states, path, MIRMoveState::Moved);
    uint32_t parent = fn.move_paths[path].parent_path;
    while (parent != MIR_INVALID_ID && parent < states.size()) {
        if (states[parent] == MIRMoveState::Initialized ||
            states[parent] == MIRMoveState::PartiallyMoved) {
            states[parent] = MIRMoveState::PartiallyMoved;
        }
        parent = fn.move_paths[parent].parent_path;
    }
}

/**
 * Recompute every ancestor of a child that was just written or moved. A parent
 * is `Initialized` only when all of its owned children are; otherwise an
 * initialized parent is demoted to `PartiallyMoved` (a sibling was moved or is
 * still uninitialized). This is what keeps a construction temporary that writes
 * every field on a fresh (un-zeroed) alloca recognized as fully initialized.
 */
void refresh_ancestors(const MIRFunction& fn, std::vector<MIRMoveState>& states,
                       uint32_t path) {
    if (path == MIR_INVALID_ID || path >= fn.move_paths.size()) return;
    uint32_t parent = fn.move_paths[path].parent_path;
    while (parent != MIR_INVALID_ID && parent < fn.move_paths.size()) {
        bool all = true;
        for (uint32_t child = fn.move_paths[parent].first_child;
             child != MIR_INVALID_ID && child < fn.move_paths.size();
             child = fn.move_paths[child].next_sibling) {
            if (states[child] != MIRMoveState::Initialized) {
                all = false;
                break;
            }
        }
        if (all) {
            if (states[parent] != MIRMoveState::Moved &&
                states[parent] != MIRMoveState::Destroyed) {
                states[parent] = MIRMoveState::Initialized;
            }
        } else if (states[parent] == MIRMoveState::Initialized) {
            states[parent] = MIRMoveState::PartiallyMoved;
        }
        parent = fn.move_paths[parent].parent_path;
    }
}

uint32_t child_field_path(const MIRFunction& fn, uint32_t path, uint32_t field_index) {
    if (path == MIR_INVALID_ID || path >= fn.move_paths.size()) return MIR_INVALID_ID;
    uint32_t child = fn.move_paths[path].first_child;
    while (child != MIR_INVALID_ID && child < fn.move_paths.size()) {
        if (fn.move_paths[child].field_index == field_index) return child;
        child = fn.move_paths[child].next_sibling;
    }
    return MIR_INVALID_ID;
}

uint32_t child_array_path(const MIRFunction& fn, uint32_t path, uint32_t array_index) {
    if (path == MIR_INVALID_ID || path >= fn.move_paths.size()) return MIR_INVALID_ID;
    uint32_t child = fn.move_paths[path].first_child;
    while (child != MIR_INVALID_ID && child < fn.move_paths.size()) {
        if (fn.move_paths[child].array_index == array_index) return child;
        child = fn.move_paths[child].next_sibling;
    }
    return MIR_INVALID_ID;
}

uint32_t field_index_for_constant(const MIRModule& module, const MIRTypeTable& types,
                                  TypeId aggregate, ConstantId constant) {
    if (aggregate == MIR_INVALID_ID || aggregate >= types.types.size()) {
        return MIR_INVALID_ID;
    }
    const MIRConstant& c = module.constants.get(constant);
    if (c.kind != MIRConstantKind::String) return MIR_INVALID_ID;
    const std::string& name = module.constants.data.text(c.data_offset);
    const MIRTypeRecord& rec = types.types[aggregate];
    for (uint32_t i = 0; i < rec.data_count; ++i) {
        const MIRFieldInfo* field = types.field_of(aggregate, i);
        if (field == nullptr) continue;
        if (types.field_name_view(*field) == name) return i;
    }
    return MIR_INVALID_ID;
}

bool constant_index(const MIRModule& module, ValueId value,
                    const MIRFunction& fn, uint64_t& out) {
    if (value == MIR_INVALID_ID || value >= fn.values.size()) return false;
    const MIRValueDef& def = fn.values[value];
    if (def.def_inst == MIR_INVALID_ID || def.def_inst >= fn.instructions.size()) {
        return false;
    }
    const MIRInstruction& inst = fn.instructions[def.def_inst];
    if (inst.opcode() != MIROpcode::ConstInt || inst.operand_count == 0) return false;
    const MIROperand& op = fn.operands[inst.operand_offset];
    if (op.kind() != MIROperandKind::Constant) return false;
    const MIRConstant& c = module.constants.get(op.id);
    if (c.kind != MIRConstantKind::Int) return false;
    out = c.bits;
    return true;
}

/**
 * Project `path` through one field/element operand. A scalar field or a dynamic
 * array index has no dedicated node and falls back to the aggregate's path.
 */
uint32_t project_path(const MIRModule& module, const MIRFunction& fn,
                      uint32_t path, TypeId aggregate,
                      const MIROperand* extra, uint32_t extra_count) {
    if (path == MIR_INVALID_ID) return MIR_INVALID_ID;
    if (extra_count == 0) return path;
    if (extra[0].kind() == MIROperandKind::Constant) {
        const uint32_t field = field_index_for_constant(
            module, module.types, aggregate, extra[0].id
        );
        if (field != MIR_INVALID_ID) {
            const uint32_t child = child_field_path(fn, path, field);
            if (child != MIR_INVALID_ID) return child;
        }
        // A scalar field has no move-path node; its lifetime is the root's.
        return path;
    }
    if (extra[0].kind() == MIROperandKind::Value) {
        uint64_t index = 0;
        if (constant_index(module, extra[0].id, fn, index)) {
            const uint32_t child = child_array_path(fn, path, static_cast<uint32_t>(index));
            if (child != MIR_INVALID_ID) return child;
        }
        return path;
    }
    return path;
}

/**
 * Resolve the move path a load/store operand names. `extra` points at the
 * projection operands after the base; `extra_count` is their count. A dynamic
 * array index falls back to the root path (the element is unknown, but the
 * aggregate itself must still be live).
 */
uint32_t path_for_projection(const MIRModule& module, const MIRFunction& fn,
                             const MIROperand& base, const MIROperand* extra,
                             uint32_t extra_count) {
    if (base.kind() != MIROperandKind::Place || base.id >= fn.places.size()) {
        return MIR_INVALID_ID;
    }
    const PlaceId place = base.id;
    // Coroutine frame fields are initialized by the ramp/poll state machine,
    // not by the poll function's own linear lowering. Their lifetime is checked
    // by the async lowering (S4.1), not by this per-function dataflow.
    const auto storage = static_cast<MIRStorageClass>(fn.places[place].storage_class);
    if (storage == MIRStorageClass::FrameField || storage == MIRStorageClass::FramePtr) {
        return MIR_INVALID_ID;
    }
    const uint32_t path = fn.places[place].move_path_id;
    if (path == MIR_INVALID_ID) return MIR_INVALID_ID;
    return project_path(module, fn, path, fn.places[place].type, extra, extra_count);
}

MIRMoveState state_of(const std::vector<MIRMoveState>& states, uint32_t path) {
    if (path == MIR_INVALID_ID || path >= states.size()) return MIRMoveState::Initialized;
    return states[path];
}

/**
 * Derived field/element addresses (`field_addr`/`index_addr`) keep their base
 * move path. A later `load` through such a value is a read of that path, which
 * is how chained accesses like `s.field.nested` stay checked.
 */
using MIRAddressPaths = std::unordered_map<uint32_t, uint32_t>;

void report(std::vector<MIRMoveDiagnostic>& out, const MIRFunction& fn,
            uint32_t function_index, uint32_t block, uint32_t inst,
            uint32_t path, MIRMoveState state, const char* action) {
    MIRMoveDiagnostic d;
    d.function_index = function_index;
    d.block_index = block;
    d.inst_index = inst;
    d.path = path;
    const bool uninit = state == MIRMoveState::Uninitialized;
    d.code = uninit ? "SAFE-INIT-001" : "SAFE-MOVE-001";
    d.message = std::string(action) + " a value that is " + state_name(state);
    if (inst < fn.instructions.size()) {
        const uint32_t src = fn.instructions[inst].source_index;
        if (src != MIR_NO_SOURCE && src < fn.sources.size()) {
            d.loc = fn.sources[src];
            d.has_loc = true;
        }
    }
    out.push_back(std::move(d));
}

void require_initialized(std::vector<MIRMoveDiagnostic>& out, const MIRFunction& fn,
                         uint32_t function_index, uint32_t block, uint32_t inst,
                         uint32_t path, const std::vector<MIRMoveState>& states,
                         const char* action) {
    if (path == MIR_INVALID_ID) return;
    // A mutable raw pointer escaped from this place: its writes bypass the
    // move-path dataflow, so the value is trusted from that point on (the
    // `&raw mut` itself is an unsafe operation).
    if (path < fn.move_paths.size()) {
        const PlaceId root_place = fn.move_paths[path].root_place;
        if (root_place < fn.places.size() && fn.places[root_place].raw_escaped) {
            return;
        }
    }
    const MIRMoveState state = state_of(states, path);
    // `Maybe` means the paths reaching this point disagree. A value that is
    // only conditionally initialized may not be read, moved or unconditionally
    // destroyed; the guarded cleanup path uses a `Drop` with an explicit flag,
    // which is handled separately in apply_instruction.
    if (state != MIRMoveState::Initialized) {
        report(out, fn, function_index, block, inst, path, state, action);
    }
}

/**
 * Resolve a drop-flag value when it is a compile-time constant, either directly
 * (`const.bool`) or through a load of a place last written by `set_drop`. Returns
 * -1 when the value is not statically known.
 */
int constant_drop_flag(const MIRModule& module, const MIRFunction& fn, ValueId value) {
    if (value == MIR_INVALID_ID || value >= fn.values.size()) return -1;
    const MIRValueDef& def = fn.values[value];
    if (def.def_inst == MIR_INVALID_ID || def.def_inst >= fn.instructions.size()) {
        return -1;
    }
    const MIRInstruction& def_inst = fn.instructions[def.def_inst];
    if (def_inst.opcode() == MIROpcode::ConstBool && def_inst.operand_count > 0) {
        const MIROperand& op = fn.operands[def_inst.operand_offset];
        if (op.kind() == MIROperandKind::Constant) {
            const MIRConstant& c = module.constants.get(op.id);
            return c.kind == MIRConstantKind::Bool ? (c.bits ? 1 : 0) : -1;
        }
        return -1;
    }
    if (def_inst.opcode() != MIROpcode::Load || def_inst.operand_count == 0) {
        return -1;
    }
    const MIROperand& base = fn.operands[def_inst.operand_offset];
    if (base.kind() != MIROperandKind::Place) return -1;
    const PlaceId flag_place = base.id;

    // Find the block containing the load, then scan its preceding instructions
    // for the last set_drop of the same flag place.
    uint32_t block_start = 0;
    uint32_t block_end = 0;
    bool found_block = false;
    for (const MIRBlock& block : fn.blocks) {
        if (block.inst_start == MIR_INVALID_ID) continue;
        const uint32_t end = block.inst_start + block.inst_count;
        if (def.def_inst >= block.inst_start && def.def_inst < end) {
            block_start = block.inst_start;
            block_end = def.def_inst;
            found_block = true;
            break;
        }
    }
    if (!found_block) return -1;
    for (uint32_t i = block_end; i-- > block_start;) {
        const MIRInstruction& inst = fn.instructions[i];
        if (inst.opcode() != MIROpcode::SetDrop || inst.operand_count < 2) continue;
        const MIROperand& dst = fn.operands[inst.operand_offset];
        if (dst.kind() != MIROperandKind::Place || dst.id != flag_place) continue;
        const MIROperand& src = fn.operands[inst.operand_offset + 1];
        if (src.kind() != MIROperandKind::Constant) return -1;
        const MIRConstant& c = module.constants.get(src.id);
        return c.kind == MIRConstantKind::Bool ? (c.bits ? 1 : 0) : -1;
    }
    return -1;
}

uint32_t root_of_path(const MIRFunction& fn, uint32_t path) {
    while (path != MIR_INVALID_ID && path < fn.move_paths.size() &&
           fn.move_paths[path].parent_path != MIR_INVALID_ID) {
        path = fn.move_paths[path].parent_path;
    }
    return path;
}

/**
 * Apply a single-projection store (`p.field = v` / `p[i] = v` / a store through
 * a derived field address) to the move-path state. `base_path` is the move path
 * of the base subobject and `aggregate` its type.
 */
void apply_projection_store(const MIRModule& module, const MIRFunction& fn,
                            uint32_t function_index, uint32_t block,
                            uint32_t inst_index, uint32_t base_path,
                            TypeId aggregate, const MIROperand& projection,
                            std::vector<MIRMoveState>& states,
                            std::vector<MIRMoveDiagnostic>& out) {
    const uint32_t target = project_path(
        module, fn, base_path, aggregate, &projection, 1
    );
    const uint32_t root = root_of_path(fn, base_path);
    if (target == MIR_INVALID_ID || root == MIR_INVALID_ID) return;
    const PlaceId root_place = fn.move_paths[root].root_place;
    MIRStorageClass storage = MIRStorageClass::Local;
    if (root_place < fn.places.size()) {
        storage = static_cast<MIRStorageClass>(fn.places[root_place].storage_class);
    }
    if (storage == MIRStorageClass::Temporary ||
        storage == MIRStorageClass::Parameter ||
        storage == MIRStorageClass::Global) {
        set_subtree(fn, states, target, MIRMoveState::Initialized);
        refresh_ancestors(fn, states, target);
        return;
    }
    if (target == root) {
        // A projection with no owned move path (a scalar/union field, a dynamic
        // index, or a projection into a nested untracked aggregate). Writing it
        // does not initialize any owned sibling. When the aggregate owns nothing
        // at all, the value is usable for ownership purposes after a manual
        // field write (the `var s : string; s.state = ...` pattern); when it
        // does own subobjects, the root stays uninitialized until every owned
        // child is written.
        if (fn.move_paths[root].first_child == MIR_INVALID_ID) {
            const MIRMoveState state = states[root];
            if (state == MIRMoveState::Moved ||
                state == MIRMoveState::Destroyed ||
                state == MIRMoveState::PartiallyMoved ||
                state == MIRMoveState::Maybe) {
                report(out, fn, function_index, block, inst_index, root, state,
                       "write to a field of");
            } else {
                states[root] = MIRMoveState::Initialized;
            }
        } else {
            // A dynamic array index cannot update any element flag, so it may
            // only write into a fully initialized array. A scalar field of a
            // tracked struct is allowed while the value is live.
            const MIRMoveState state = states[root];
            const TypeId bt = fn.move_paths[root].field_type;
            const bool is_array = bt != MIR_INVALID_ID &&
                                  bt < module.types.types.size() &&
                                  module.types.types[bt].kind == MIRTypeKind::Array;
            if (state == MIRMoveState::Moved ||
                state == MIRMoveState::Destroyed ||
                (is_array && state != MIRMoveState::Initialized)) {
                report(out, fn, function_index, block, inst_index, root, state,
                       "write to a field of");
            }
        }
        return;
    }
    // A direct child or grandchild of a place the lowering gave per-child drop
    // flags may be initialized on its own; anything deeper (or a place without
    // flags) requires the destination sub-place to be fully initialized.
    uint32_t depth = 0;
    uint32_t p = target;
    while (p != root && p != MIR_INVALID_ID && depth <= 2) {
        p = fn.move_paths[p].parent_path;
        ++depth;
    }
    const bool partial_ok =
        p == root && depth <= 2 && root_place < fn.places.size() &&
        fn.places[root_place].field_wise_drop;
    const MIRMoveState root_state = states[root];
    if (partial_ok && root_state != MIRMoveState::Moved &&
        root_state != MIRMoveState::Destroyed) {
        set_subtree(fn, states, target, MIRMoveState::Initialized);
        refresh_ancestors(fn, states, target);
    } else {
        require_initialized(out, fn, function_index, block, inst_index, target,
                            states, "write to a field of");
    }
}

void apply_instruction(const MIRModule& module, const MIRFunction& fn,
                       uint32_t function_index, uint32_t block, uint32_t inst_index,
                       std::vector<MIRMoveState>& states, MIRAddressPaths& address_paths,
                       MIRAddressPaths& place_address_paths,
                       std::vector<MIRMoveDiagnostic>& out) {
    const MIRInstruction& inst = fn.instructions[inst_index];
    const MIROperand* ops = inst.operand_count == 0
                                ? nullptr
                                : &fn.operands[inst.operand_offset];

    switch (inst.opcode()) {
        case MIROpcode::Init:
        case MIROpcode::CopyInit:
        case MIROpcode::Assign: {
            if (ops == nullptr) break;
            const uint32_t path = path_for_projection(module, fn, ops[0], nullptr, 0);
            set_subtree(fn, states, path, MIRMoveState::Initialized);
            break;
        }
        case MIROpcode::MoveInit: {
            if (ops == nullptr || inst.operand_count < 2) break;
            const uint32_t dst = path_for_projection(module, fn, ops[0], nullptr, 0);
            const uint32_t src = path_for_projection(module, fn, ops[1], nullptr, 0);
            require_initialized(out, fn, function_index, block, inst_index, src,
                                states, "move from");
            set_subtree(fn, states, dst, MIRMoveState::Initialized);
            mark_path_moved(fn, states, src);
            break;
        }
        case MIROpcode::Destroy: {
            if (ops == nullptr) break;
            const uint32_t path = path_for_projection(module, fn, ops[0], nullptr, 0);
            require_initialized(out, fn, function_index, block, inst_index, path,
                                states, "destroy");
            set_subtree(fn, states, path, MIRMoveState::Destroyed);
            break;
        }
        case MIROpcode::Drop: {
            if (ops == nullptr) break;
            const uint32_t path = path_for_projection(module, fn, ops[0], nullptr, 0);
            if (path == MIR_INVALID_ID) break;
            if (inst.operand_count >= 3) {
                // Conditional drop: resolve a constant flag when the lowering
                // used `set_drop`; otherwise the path is only maybe destroyed.
                const MIROperand& flag = ops[2];
                const int known = flag.kind() == MIROperandKind::Value
                                      ? constant_drop_flag(module, fn, flag.id) : -1;
                if (known == 0) break; // flag is false: no state change
                const MIRMoveState state = state_of(states, path);
                if (known == 1) {
                    require_initialized(out, fn, function_index, block, inst_index, path,
                                        states, "drop");
                    set_subtree(fn, states, path, MIRMoveState::Destroyed);
                } else if (state == MIRMoveState::Initialized) {
                    set_subtree(fn, states, path, MIRMoveState::Maybe);
                }
                break;
            }
            require_initialized(out, fn, function_index, block, inst_index, path,
                                states, "drop");
            set_subtree(fn, states, path, MIRMoveState::Destroyed);
            break;
        }
        case MIROpcode::FieldAddr:
        case MIROpcode::IndexAddr: {
            // operands: base, field/index, type. Remember the projected move
            // path of the derived address so a later load through it is a read
            // of that place (`s.field.nested` lowers to field_addr + load).
            if (ops == nullptr || inst.operand_count < 2) break;
            uint32_t base_path = MIR_INVALID_ID;
            TypeId aggregate = MIR_INVALID_ID;
            if (ops[0].kind() == MIROperandKind::Place && ops[0].id < fn.places.size()) {
                const auto sc = static_cast<MIRStorageClass>(
                    fn.places[ops[0].id].storage_class
                );
                if (sc == MIRStorageClass::FrameField ||
                    sc == MIRStorageClass::FramePtr) {
                    break;
                }
                base_path = fn.places[ops[0].id].move_path_id;
                aggregate = fn.places[ops[0].id].type;
            } else if (ops[0].kind() == MIROperandKind::Value &&
                       ops[0].id < fn.values.size()) {
                const auto it = address_paths.find(ops[0].id);
                if (it == address_paths.end()) break;
                base_path = it->second;
                // A FieldAddr/IndexAddr value names the projected subobject, so
                // its MIR type is the aggregate that owns the next projection.
                aggregate = fn.values[ops[0].id].type;
            }
            if (base_path == MIR_INVALID_ID) break;
            const uint32_t projected = project_path(
                module, fn, base_path, aggregate, ops + 1, 1
            );
            if (projected != MIR_INVALID_ID &&
                inst.result_or_place != MIR_INVALID_ID) {
                address_paths[inst.result_or_place] = projected;
            }
            break;
        }
        case MIROpcode::Load: {
            if (ops == nullptr) break;
            // A load through a derived field/element address reads that place.
            if (ops[0].kind() == MIROperandKind::Value) {
                const auto it = address_paths.find(ops[0].id);
                if (it == address_paths.end()) break;
                require_initialized(out, fn, function_index, block, inst_index,
                                    it->second, states, "read from");
                break;
            }
            if (ops[0].kind() != MIROperandKind::Place) break;
            const uint32_t extra_count = inst.operand_count > 1 &&
                                                 ops[inst.operand_count - 1].kind() ==
                                                     MIROperandKind::Type
                                             ? inst.operand_count - 2
                                             : 0;
            const uint32_t path = path_for_projection(
                module, fn, ops[0], extra_count ? ops + 1 : nullptr, extra_count
            );
            require_initialized(out, fn, function_index, block, inst_index, path,
                                states, "read from");
            break;
        }
        case MIROpcode::AsyncAwait: {
            // operands: frame, child, result, ...
            if (ops == nullptr || inst.operand_count < 3) break;
            if (ops[2].kind() != MIROperandKind::Place) break;
            const uint32_t path = path_for_projection(module, fn, ops[2], nullptr, 0);
            set_subtree(fn, states, path, MIRMoveState::Initialized);
            break;
        }
        case MIROpcode::AsyncRampFinish: {
            // operands: frame, sret handle, state, vtable, handle type
            if (ops == nullptr || inst.operand_count < 2) break;
            if (ops[1].kind() != MIROperandKind::Place) break;
            const uint32_t path = path_for_projection(module, fn, ops[1], nullptr, 0);
            set_subtree(fn, states, path, MIRMoveState::Initialized);
            break;
        }
        case MIROpcode::AsyncFinish: {
            // operands: frame, return-place, poll type
            if (ops == nullptr || inst.operand_count < 2) break;
            if (ops[1].kind() != MIROperandKind::Place) break;
            const uint32_t path = path_for_projection(module, fn, ops[1], nullptr, 0);
            set_subtree(fn, states, path, MIRMoveState::Initialized);
            break;
        }
        case MIROpcode::AddressOf: {
            // A whole-place address: used by `memcpy(&raw mut x, ...)` and
            // placement-new style initialization. It is not a read, so loads
            // through it stay outside the move-path model; it is recorded so a
            // bulk-memory call can mark the destination initialized.
            if (ops == nullptr || ops[0].kind() != MIROperandKind::Place ||
                ops[0].id >= fn.places.size()) {
                break;
            }
            const auto sc = static_cast<MIRStorageClass>(
                fn.places[ops[0].id].storage_class
            );
            if (sc == MIRStorageClass::FrameField || sc == MIRStorageClass::FramePtr) {
                break;
            }
            const uint32_t path = fn.places[ops[0].id].move_path_id;
            if (path != MIR_INVALID_ID && inst.result_or_place != MIR_INVALID_ID) {
                place_address_paths[inst.result_or_place] = path;
            }
            break;
        }
        case MIROpcode::MemCpy:
        case MIROpcode::MemSet: {
            if (ops == nullptr || inst.operand_count == 0) break;
            uint32_t path = MIR_INVALID_ID;
            if (ops[0].kind() == MIROperandKind::Place) {
                path = path_for_projection(module, fn, ops[0], nullptr, 0);
            } else if (ops[0].kind() == MIROperandKind::Value) {
                const auto it = place_address_paths.find(ops[0].id);
                if (it != place_address_paths.end()) path = it->second;
            }
            set_subtree(fn, states, path, MIRMoveState::Initialized);
            refresh_ancestors(fn, states, path);
            break;
        }
        case MIROpcode::Call:
        case MIROpcode::CallIndirect: {
            // Struct-return calls write their result through the place operand
            // immediately after the callee (or after the function value).
            if (ops == nullptr || inst.operand_count < 2) break;
            const bool sret_result =
                ops[1].kind() == MIROperandKind::Place &&
                inst.result_or_place != MIR_INVALID_ID &&
                ops[1].id == inst.result_or_place;
            // A bulk-memory call is an unsafe initialization primitive: mark its
            // destination initialized (the source of `memcpy` is a read and is
            // deliberately not checked through a raw pointer here).
            if (ops[0].kind() == MIROperandKind::Symbol &&
                ops[0].id < module.symbols.size()) {
                const MIRSymbolRecord& callee = module.symbols.get(ops[0].id);
                const std::string_view name(
                    module.symbols.name_data(callee), callee.name_length
                );
                const bool is_copy = name == "memcpy" || name == "memmove" ||
                                     name == "__chx_memcpy";
                const bool is_set = name == "memset" || name == "__chx_memset";
                const uint32_t first_arg = sret_result ? 2 : 1;
                if ((is_copy || is_set) && inst.operand_count > first_arg) {
                    const MIROperand& dst = ops[first_arg];
                    uint32_t path = MIR_INVALID_ID;
                    if (dst.kind() == MIROperandKind::Place) {
                        path = path_for_projection(module, fn, dst, nullptr, 0);
                    } else if (dst.kind() == MIROperandKind::Value) {
                        const auto it = place_address_paths.find(dst.id);
                        if (it != place_address_paths.end()) path = it->second;
                    }
                    set_subtree(fn, states, path, MIRMoveState::Initialized);
                    refresh_ancestors(fn, states, path);
                }
            }
            if (!sret_result) break;
            const uint32_t path = path_for_projection(
                module, fn, ops[1], nullptr, 0
            );
            set_subtree(fn, states, path, MIRMoveState::Initialized);
            break;
        }
        case MIROpcode::Store: {
            if (ops == nullptr || inst.operand_count == 0) break;
            const bool dst_is_place = ops[0].kind() == MIROperandKind::Place;
            const uint32_t root = dst_is_place
                                      ? path_for_projection(module, fn, ops[0], nullptr, 0)
                                      : MIR_INVALID_ID;
            // Whole-place store: place, value. A full assignment reinitializes
            // the value (including after a move or a partial state).
            if (inst.operand_count == 2) {
                if (root != MIR_INVALID_ID) {
                    set_subtree(fn, states, root, MIRMoveState::Initialized);
                }
                // A place source is a move. The destination may be a raw sret
                // pointer (an aggregate return), so the source is checked even
                // when the destination has no move path. Constructor temporaries
                // are compiler-built and may legitimately be empty, so they are
                // trusted here.
                if (ops[1].kind() == MIROperandKind::Place &&
                    ops[1].id < fn.places.size()) {
                    const auto sc = static_cast<MIRStorageClass>(
                        fn.places[ops[1].id].storage_class
                    );
                    if (sc != MIRStorageClass::Temporary) {
                        const uint32_t src = path_for_projection(
                            module, fn, ops[1], nullptr, 0
                        );
                        require_initialized(out, fn, function_index, block,
                                            inst_index, src, states, "move from");
                        mark_path_moved(fn, states, src);
                    }
                }
                break;
            }
            if (inst.operand_count < 3) break;
            if (dst_is_place && root != MIR_INVALID_ID) {
                apply_projection_store(module, fn, function_index, block,
                                       inst_index, root,
                                       fn.places[ops[0].id].type, ops[1],
                                       states, out);
            } else if (!dst_is_place &&
                       ops[0].kind() == MIROperandKind::Value) {
                // A store through a derived field address (`s.inner.x = v`
                // lowers to field_addr(s, inner) + store(..., x, v)).
                const auto it = address_paths.find(ops[0].id);
                if (it != address_paths.end() && ops[0].id < fn.values.size()) {
                    // The address names the projected subobject; its MIR type
                    // is the aggregate that owns the next projection.
                    const TypeId aggregate = fn.values[ops[0].id].type;
                    apply_projection_store(module, fn, function_index, block,
                                           inst_index, it->second, aggregate,
                                           ops[1], states, out);
                }
            }
            // A place source is a move out of that place; a value source is a
            // copy and leaves its own path untouched.
            if (ops[2].kind() == MIROperandKind::Place) {
                const uint32_t src = path_for_projection(
                    module, fn, ops[2], nullptr, 0
                );
                require_initialized(out, fn, function_index, block, inst_index,
                                    src, states, "move from");
                mark_path_moved(fn, states, src);
            }
            break;
        }
        default:
            break;
    }
}

void collect_successors(const MIRFunction& fn, const MIRInstruction& term,
                        std::vector<BlockId>& out) {
    const MIROperand* ops = term.operand_count == 0
                                ? nullptr
                                : &fn.operands[term.operand_offset];
    switch (term.opcode()) {
        case MIROpcode::Br:
            if (ops != nullptr && term.operand_count >= 1 &&
                ops[0].kind() == MIROperandKind::Block) {
                out.push_back(ops[0].id);
            }
            break;
        case MIROpcode::CondBr:
            if (ops != nullptr) {
                for (uint16_t i = 1; i < term.operand_count; ++i) {
                    if (ops[i].kind() == MIROperandKind::Block) out.push_back(ops[i].id);
                }
            }
            break;
        case MIROpcode::Switch:
            if (ops != nullptr) {
                for (uint16_t i = 1; i < term.operand_count; ++i) {
                    if (ops[i].kind() == MIROperandKind::Block) out.push_back(ops[i].id);
                }
            }
            break;
        case MIROpcode::Throw:
            if (ops != nullptr && term.operand_count >= 2 &&
                ops[1].kind() == MIROperandKind::Block) {
                out.push_back(ops[1].id);
            }
            break;
        default:
            break;
    }
}

} // namespace

MIRMoveCheckResult check_function_moves(const MIRModule& module,
                                        const MIRFunction& function,
                                        uint32_t function_index) {
    MIRMoveCheckResult result;
    if (function.entry_block >= function.blocks.size() ||
        function.move_paths.empty()) {
        return result;
    }

    const uint32_t path_count = static_cast<uint32_t>(function.move_paths.size());
    std::vector<std::vector<MIRMoveState>> in(
        function.blocks.size(), std::vector<MIRMoveState>()
    );
    std::vector<bool> seen(function.blocks.size(), false);
    std::deque<BlockId> work;
    MIRAddressPaths address_paths;
    MIRAddressPaths place_address_paths;

    in[function.entry_block] = std::vector<MIRMoveState>(
        path_count, MIRMoveState::Uninitialized
    );
    // Parameters and globals are live on entry; their storage was initialized by
    // the caller or the module initializer.
    for (const MIRPlaceDef& place : function.places) {
        const auto storage = static_cast<MIRStorageClass>(place.storage_class);
        if (storage != MIRStorageClass::Parameter && storage != MIRStorageClass::Global) {
            continue;
        }
        if (place.move_path_id != MIR_INVALID_ID &&
            place.move_path_id < path_count) {
            set_subtree(function, in[function.entry_block], place.move_path_id,
                        MIRMoveState::Initialized);
        }
    }
    seen[function.entry_block] = true;
    work.push_back(function.entry_block);

    while (!work.empty()) {
        const BlockId block_id = work.front();
        work.pop_front();
        if (block_id >= function.blocks.size()) continue;
        const MIRBlock& block = function.blocks[block_id];
        if (block.inst_start == MIR_INVALID_ID) continue;

        std::vector<MIRMoveState> state = in[block_id];
        for (uint32_t i = 0; i < block.inst_count; ++i) {
            const uint32_t inst_index = block.inst_start + i;
            if (inst_index >= function.instructions.size()) break;
            apply_instruction(module, function, function_index, block_id, inst_index,
                              state, address_paths, place_address_paths,
                              result.diagnostics);
        }
        const uint32_t term_index = block.inst_start + block.inst_count;
        if (term_index >= function.instructions.size()) continue;
        std::vector<BlockId> successors;
        collect_successors(function, function.instructions[term_index], successors);
        for (BlockId succ : successors) {
            if (succ >= function.blocks.size()) continue;
            if (!seen[succ]) {
                in[succ] = state;
                seen[succ] = true;
                work.push_back(succ);
                continue;
            }
            bool changed = false;
            for (uint32_t p = 0; p < path_count; ++p) {
                const MIRMoveState joined = join_state(in[succ][p], state[p]);
                if (joined != in[succ][p]) {
                    in[succ][p] = joined;
                    changed = true;
                }
            }
            if (changed) work.push_back(succ);
        }
    }

    return result;
}

MIRMoveCheckResult check_module_moves(const MIRModule& module) {
    MIRMoveCheckResult result;
    for (uint32_t f = 0; f < module.functions.size(); ++f) {
        MIRMoveCheckResult one = check_function_moves(module, module.functions[f], f);
        for (MIRMoveDiagnostic& d : one.diagnostics) {
            result.diagnostics.push_back(std::move(d));
        }
    }
    return result;
}

} // namespace mir
