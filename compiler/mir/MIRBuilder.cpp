// Copyright (c) Chemical Language Foundation 2025.

#include "MIRBuilder.h"

namespace mir {

// ── blocks ─────────────────────────────────────────────────────────────────

BlockId MIRBuilder::create_block(uint16_t arg_count) {
    MIRBlock b;
    b.id = static_cast<BlockId>(func_.blocks.size());
    b.inst_start = MIR_INVALID_ID; // positioned by set_block()
    b.inst_count = 0;
    b.arg_count = arg_count;
    b.arg_type_offset = static_cast<uint16_t>(func_.block_arg_types.size());
    for (uint16_t i = 0; i < arg_count; ++i) func_.block_arg_types.push_back(MIR_INVALID_ID);
    if (!func_.blocks.push(arena_, b)) {
        ok_ = false;
        error_ = "create_block: blocks.push failed";
        return MIR_NULL;
    }
    return b.id;
}

void MIRBuilder::set_block(BlockId block) {
    if (block >= func_.blocks.size()) {
        ok_ = false;
        error_ = "set_block: block out of range";
        return;
    }
    MIRBlock& b = func_.blocks[block];
    if (b.inst_start == MIR_INVALID_ID) {
        b.inst_start = static_cast<uint32_t>(func_.instructions.size());
    }
    current_block_ = block;
    terminated_ = false;
}

// ── value / place creation ─────────────────────────────────────────────────

ValueId MIRBuilder::new_value(TypeId type, uint8_t flags) {
    MIRValueDef vd;
    vd.type = type;
    vd.def_inst = MIR_INVALID_ID;
    vd.use_count = 0;
    vd.flags = flags;
    func_.values.push_back(vd);
    return static_cast<ValueId>(func_.values.size() - 1);
}

PlaceId MIRBuilder::new_place(TypeId type, MIRStorageClass sc, uint32_t move_path_id) {
    MIRPlaceDef pd;
    pd.type = type;
    pd.storage_class = static_cast<uint8_t>(sc);
    pd.init_state = static_cast<uint8_t>(MIRInitState::Uninitialized);
    pd.move_path_id = move_path_id;
    func_.places.push_back(pd);
    return static_cast<PlaceId>(func_.places.size() - 1);
}

uint32_t MIRBuilder::new_move_path(PlaceId place, TypeId type) {
    MIRMovePathNode node;
    node.root_place = place;
    node.parent_path = 0;
    node.first_child = 0;
    node.next_sibling = 0;
    node.init_state = static_cast<uint8_t>(MIRInitState::Uninitialized);
    node.field_type = type;
    func_.move_paths.push_back(node);
    uint32_t id = static_cast<uint32_t>(func_.move_paths.size() - 1);
    if (place < func_.places.size()) func_.places[place].move_path_id = id;
    return id;
}

// ── emit ───────────────────────────────────────────────────────────────────

void MIRBuilder::note_value_uses(const MIROperand* operands, uint32_t count) {
    for (uint32_t i = 0; i < count; ++i) {
        const MIROperand& o = operands[i];
        if (o.kind() == MIROperandKind::Value && o.id < func_.values.size()) {
            MIRValueDef& vd = func_.values[o.id];
            if (vd.use_count < 0xFFFF) ++vd.use_count;
        }
    }
}

void MIRBuilder::note_result_value(ValueId id, uint32_t inst_index) {
    if (id < func_.values.size()) func_.values[id].def_inst = inst_index;
}

uint32_t MIRBuilder::emit_impl(MIROpcode op, uint32_t result_or_place,
                               const MIROperand* operands, uint32_t count) {
    if (current_block_ == MIR_INVALID_ID || terminated_) {
        ok_ = false;
        error_ = "emit: no current block or block already terminated";
        return MIR_INVALID_ID;
    }

    MIRInstruction inst = MIRInstruction::make(op);
    inst.result_or_place = result_or_place;
    inst.operand_offset = static_cast<uint32_t>(func_.operands.size());
    inst.operand_count = static_cast<uint16_t>(count);

    note_value_uses(operands, count);
    for (uint32_t i = 0; i < count; ++i) {
        if (!func_.operands.push(arena_, operands[i])) {
            ok_ = false;
            error_ = "emit: operand push failed (OOM)";
            return MIR_INVALID_ID;
        }
    }

    const uint32_t idx = static_cast<uint32_t>(func_.instructions.size());
    if (!func_.instructions.push(arena_, inst)) {
        ok_ = false;
        error_ = "emit: instruction push failed (OOM)";
        return MIR_INVALID_ID;
    }

    if (is_terminator(op)) terminated_ = true;
    else func_.blocks[current_block_].inst_count++;
    return idx;
}

uint32_t MIRBuilder::emit(MIROpcode op, uint32_t result_or_place,
                          const MIROperand* operands, uint32_t count) {
    return emit_impl(op, result_or_place, operands, count);
}

uint32_t MIRBuilder::emit(MIROpcode op, uint32_t result_or_place,
                          std::initializer_list<MIROperand> operands) {
    return emit_impl(op, result_or_place, operands.begin(),
                     static_cast<uint32_t>(operands.size()));
}

// ── constants ──────────────────────────────────────────────────────────────

ValueId MIRBuilder::const_int(TypeId type, ConstantId constant) {
    ValueId v = new_value(type, VF_CONSTANT | VF_PURE);
    uint32_t i = emit(MIROpcode::ConstInt, v, {MIROperand::constant(constant, type)});
    note_result_value(v, i);
    return v;
}

ValueId MIRBuilder::const_bool(TypeId type, ConstantId constant) {
    ValueId v = new_value(type, VF_CONSTANT | VF_PURE);
    uint32_t i = emit(MIROpcode::ConstBool, v, {MIROperand::constant(constant, type)});
    note_result_value(v, i);
    return v;
}

ValueId MIRBuilder::const_null(TypeId type, ConstantId constant) {
    ValueId v = new_value(type, VF_CONSTANT | VF_PURE);
    uint32_t i = emit(MIROpcode::ConstNull, v, {MIROperand::constant(constant, type)});
    note_result_value(v, i);
    return v;
}

ValueId MIRBuilder::const_float(TypeId type, ConstantId constant) {
    ValueId v = new_value(type, VF_CONSTANT | VF_PURE);
    uint32_t i = emit(MIROpcode::ConstFloat, v, {MIROperand::constant(constant, type)});
    note_result_value(v, i);
    return v;
}

ValueId MIRBuilder::const_double(TypeId type, ConstantId constant) {
    ValueId v = new_value(type, VF_CONSTANT | VF_PURE);
    uint32_t i = emit(MIROpcode::ConstDouble, v, {MIROperand::constant(constant, type)});
    note_result_value(v, i);
    return v;
}

ValueId MIRBuilder::const_string(TypeId type, ConstantId constant) {
    ValueId v = new_value(type, VF_CONSTANT | VF_PURE);
    uint32_t i = emit(MIROpcode::ConstString, v, {MIROperand::constant(constant, type)});
    note_result_value(v, i);
    return v;
}

ValueId MIRBuilder::param(TypeId type) {
    ValueId v = new_value(type, VF_NONE);
    uint32_t i = emit(MIROpcode::Param, v, {MIROperand::type(type)});
    note_result_value(v, i);
    return v;
}

// ── storage ────────────────────────────────────────────────────────────────

void MIRBuilder::store_indirect(ValueId address, ValueId value) {
    TypeId at = address < func_.values.size() ? func_.values[address].type : MIR_INVALID_ID;
    TypeId vt = value < func_.values.size() ? func_.values[value].type : MIR_INVALID_ID;
    emit(MIROpcode::Store, MIR_NULL,
         {MIROperand::value(address, at), MIROperand::value(value, vt)});
}

ValueId MIRBuilder::load_indirect(ValueId address, TypeId type) {
    ValueId v = new_value(type, VF_NONE);
    TypeId at = address < func_.values.size() ? func_.values[address].type : MIR_INVALID_ID;
    uint32_t i = emit(MIROpcode::Load, v,
                      {MIROperand::value(address, at), MIROperand::type(type)});
    note_result_value(v, i);
    return v;
}

ValueId MIRBuilder::global_addr(SymbolId symbol, TypeId pointer_type) {
    ValueId v = new_value(pointer_type, VF_ADDRESSABLE);
    uint32_t i = emit(MIROpcode::GlobalAddr, v,
                      {MIROperand::symbol(symbol, MIR_INVALID_ID), MIROperand::type(pointer_type)});
    note_result_value(v, i);
    return v;
}

ValueId MIRBuilder::function_addr(SymbolId symbol, TypeId pointer_type) {
    ValueId v = new_value(pointer_type, VF_ADDRESSABLE);
    uint32_t i = emit(MIROpcode::FunctionAddr, v,
                      {MIROperand::symbol(symbol, MIR_INVALID_ID), MIROperand::type(pointer_type)});
    note_result_value(v, i);
    return v;
}

PlaceId MIRBuilder::alloca(TypeId type, MIRStorageClass sc) {
    PlaceId p = new_place(type, sc);
    emit(MIROpcode::Alloca, p, {MIROperand::type(type)});
    return p;
}

ValueId MIRBuilder::load(PlaceId place, TypeId type) {
    ValueId v = new_value(type, VF_NONE);
    uint32_t i = emit(MIROpcode::Load, v,
                      {MIROperand::place(place, type), MIROperand::type(type)});
    note_result_value(v, i);
    return v;
}

void MIRBuilder::store(PlaceId place, ValueId value) {
    TypeId vt = value < func_.values.size() ? func_.values[value].type : MIR_INVALID_ID;
    emit(MIROpcode::Store, MIR_NULL,
         {MIROperand::place(place, vt), MIROperand::value(value, vt)});
}

ValueId MIRBuilder::field_load(PlaceId base, ConstantId field_name, TypeId field_type) {
    ValueId v = new_value(field_type, VF_NONE);
    TypeId bt = base < func_.places.size() ? func_.places[base].type : MIR_INVALID_ID;
    uint32_t i = emit(MIROpcode::Load, v,
                      {MIROperand::place(base, bt),
                       MIROperand::constant(field_name, MIR_INVALID_ID),
                       MIROperand::type(field_type)});
    note_result_value(v, i);
    return v;
}

ValueId MIRBuilder::field_load_ptr(ValueId base, ConstantId field_name, TypeId field_type) {
    ValueId v = new_value(field_type, VF_NONE);
    TypeId bt = base < func_.values.size() ? func_.values[base].type : MIR_INVALID_ID;
    uint32_t i = emit(MIROpcode::Load, v,
                      {MIROperand::value(base, bt),
                       MIROperand::constant(field_name, MIR_INVALID_ID),
                       MIROperand::type(field_type)});
    note_result_value(v, i);
    return v;
}

void MIRBuilder::field_store_ptr(ValueId base, ConstantId field_name, ValueId value) {
    TypeId bt = base < func_.values.size() ? func_.values[base].type : MIR_INVALID_ID;
    TypeId vt = value < func_.values.size() ? func_.values[value].type : MIR_INVALID_ID;
    emit(MIROpcode::Store, MIR_NULL,
         {MIROperand::value(base, bt),
          MIROperand::constant(field_name, MIR_INVALID_ID),
          MIROperand::value(value, vt)});
}

void MIRBuilder::field_store(PlaceId base, ConstantId field_name, ValueId value) {
    TypeId bt = base < func_.places.size() ? func_.places[base].type : MIR_INVALID_ID;
    TypeId vt = value < func_.values.size() ? func_.values[value].type : MIR_INVALID_ID;
    emit(MIROpcode::Store, MIR_NULL,
         {MIROperand::place(base, bt),
          MIROperand::constant(field_name, MIR_INVALID_ID),
          MIROperand::value(value, vt)});
}

void MIRBuilder::field_store_place(PlaceId base, ConstantId field_name, PlaceId value) {
    TypeId bt = base < func_.places.size() ? func_.places[base].type : MIR_INVALID_ID;
    TypeId vt = value < func_.places.size() ? func_.places[value].type : MIR_INVALID_ID;
    emit(MIROpcode::Store, MIR_NULL,
         {MIROperand::place(base, bt),
          MIROperand::constant(field_name, MIR_INVALID_ID),
          MIROperand::place(value, vt)});
}

void MIRBuilder::element_store_place(PlaceId base, ValueId index, PlaceId value) {
    TypeId bt = base < func_.places.size() ? func_.places[base].type : MIR_INVALID_ID;
    TypeId it = index < func_.values.size() ? func_.values[index].type : MIR_INVALID_ID;
    TypeId vt = value < func_.places.size() ? func_.places[value].type : MIR_INVALID_ID;
    emit(MIROpcode::Store, MIR_NULL,
         {MIROperand::place(base, bt), MIROperand::value(index, it),
          MIROperand::place(value, vt)});
}

ValueId MIRBuilder::index_load(ValueId base, ValueId index, TypeId element_type) {
    ValueId v = new_value(element_type, VF_NONE);
    TypeId bt = base < func_.values.size() ? func_.values[base].type : MIR_INVALID_ID;
    TypeId it = index < func_.values.size() ? func_.values[index].type : MIR_INVALID_ID;
    uint32_t i = emit(MIROpcode::Load, v,
                      {MIROperand::value(base, bt), MIROperand::value(index, it),
                       MIROperand::type(element_type)});
    note_result_value(v, i);
    return v;
}

void MIRBuilder::index_store(ValueId base, ValueId index, ValueId value) {
    TypeId bt = base < func_.values.size() ? func_.values[base].type : MIR_INVALID_ID;
    TypeId it = index < func_.values.size() ? func_.values[index].type : MIR_INVALID_ID;
    TypeId vt = value < func_.values.size() ? func_.values[value].type : MIR_INVALID_ID;
    emit(MIROpcode::Store, MIR_NULL,
         {MIROperand::value(base, bt), MIROperand::value(index, it),
          MIROperand::value(value, vt)});
}

ValueId MIRBuilder::element_load(PlaceId base, ValueId index, TypeId element_type) {
    ValueId v = new_value(element_type, VF_NONE);
    TypeId bt = base < func_.places.size() ? func_.places[base].type : MIR_INVALID_ID;
    TypeId it = index < func_.values.size() ? func_.values[index].type : MIR_INVALID_ID;
    uint32_t i = emit(MIROpcode::Load, v,
                      {MIROperand::place(base, bt), MIROperand::value(index, it),
                       MIROperand::type(element_type)});
    note_result_value(v, i);
    return v;
}

void MIRBuilder::element_store(PlaceId base, ValueId index, ValueId value) {
    TypeId bt = base < func_.places.size() ? func_.places[base].type : MIR_INVALID_ID;
    TypeId it = index < func_.values.size() ? func_.values[index].type : MIR_INVALID_ID;
    TypeId vt = value < func_.values.size() ? func_.values[value].type : MIR_INVALID_ID;
    emit(MIROpcode::Store, MIR_NULL,
         {MIROperand::place(base, bt), MIROperand::value(index, it),
          MIROperand::value(value, vt)});
}

ValueId MIRBuilder::address_of(PlaceId place, TypeId pointer_type) {
    ValueId v = new_value(pointer_type, VF_ADDRESSABLE);
    TypeId pt = place < func_.places.size() ? func_.places[place].type : MIR_INVALID_ID;
    uint32_t i = emit(MIROpcode::AddressOf, v,
                      {MIROperand::place(place, pt), MIROperand::type(pointer_type)});
    note_result_value(v, i);
    return v;
}

ValueId MIRBuilder::field_addr(PlaceId base, ConstantId field_index, TypeId field_type) {
    ValueId v = new_value(field_type, VF_ADDRESSABLE);
    TypeId bt = base < func_.places.size() ? func_.places[base].type : MIR_INVALID_ID;
    uint32_t i = emit(MIROpcode::FieldAddr, v,
                      {MIROperand::place(base, bt),
                       MIROperand::constant(field_index, MIR_INVALID_ID),
                       MIROperand::type(field_type)});
    note_result_value(v, i);
    return v;
}

ValueId MIRBuilder::field_addr_ptr(ValueId base, ConstantId field_name, TypeId field_type) {
    ValueId v = new_value(field_type, VF_ADDRESSABLE);
    TypeId bt = base < func_.values.size() ? func_.values[base].type : MIR_INVALID_ID;
    uint32_t i = emit(MIROpcode::FieldAddr, v,
                      {MIROperand::value(base, bt),
                       MIROperand::constant(field_name, MIR_INVALID_ID),
                       MIROperand::type(field_type)});
    note_result_value(v, i);
    return v;
}

ValueId MIRBuilder::index_addr(PlaceId base, ValueId index, TypeId element_type) {
    ValueId v = new_value(element_type, VF_ADDRESSABLE);
    TypeId bt = base < func_.places.size() ? func_.places[base].type : MIR_INVALID_ID;
    TypeId it = index < func_.values.size() ? func_.values[index].type : MIR_INVALID_ID;
    uint32_t i = emit(MIROpcode::IndexAddr, v,
                      {MIROperand::place(base, bt), MIROperand::value(index, it),
                       MIROperand::type(element_type)});
    note_result_value(v, i);
    return v;
}

ValueId MIRBuilder::index_addr_ptr(ValueId base, ValueId index, TypeId element_type) {
    ValueId v = new_value(element_type, VF_ADDRESSABLE);
    TypeId bt = base < func_.values.size() ? func_.values[base].type : MIR_INVALID_ID;
    TypeId it = index < func_.values.size() ? func_.values[index].type : MIR_INVALID_ID;
    uint32_t i = emit(MIROpcode::IndexAddr, v,
                      {MIROperand::value(base, bt), MIROperand::value(index, it),
                       MIROperand::type(element_type)});
    note_result_value(v, i);
    return v;
}

ValueId MIRBuilder::gep(ValueId base, TypeId result_type,
                        std::initializer_list<ValueId> indices) {
    MIROperand ops[8];
    uint32_t n = 0;
    TypeId bt = base < func_.values.size() ? func_.values[base].type : MIR_INVALID_ID;
    ops[n++] = MIROperand::value(base, bt);
    for (ValueId idx : indices) {
        if (n >= 7) break;
        TypeId it = idx < func_.values.size() ? func_.values[idx].type : MIR_INVALID_ID;
        ops[n++] = MIROperand::value(idx, it);
    }
    ops[n++] = MIROperand::type(result_type);
    ValueId v = new_value(result_type, VF_ADDRESSABLE);
    uint32_t i = emit_impl(MIROpcode::Gep, v, ops, n);
    note_result_value(v, i);
    return v;
}

// ── arithmetic ─────────────────────────────────────────────────────────────

ValueId MIRBuilder::unary(ValueId operand, ConstantId op, TypeId result_type) {
    TypeId ot = operand < func_.values.size() ? func_.values[operand].type : MIR_INVALID_ID;
    ValueId v = new_value(result_type, VF_PURE);
    uint32_t i = emit(MIROpcode::Unary, v,
                      {MIROperand::value(operand, ot),
                       MIROperand::constant(op, MIR_INVALID_ID),
                       MIROperand::type(result_type)});
    note_result_value(v, i);
    return v;
}

ValueId MIRBuilder::binary(ValueId lhs, ValueId rhs, ConstantId op, TypeId result_type) {
    TypeId lt = lhs < func_.values.size() ? func_.values[lhs].type : MIR_INVALID_ID;
    TypeId rt = rhs < func_.values.size() ? func_.values[rhs].type : MIR_INVALID_ID;
    ValueId v = new_value(result_type, VF_PURE);
    uint32_t i = emit(MIROpcode::Binary, v,
                      {MIROperand::value(lhs, lt), MIROperand::value(rhs, rt),
                       MIROperand::constant(op, MIR_INVALID_ID),
                       MIROperand::type(result_type)});
    note_result_value(v, i);
    return v;
}

ValueId MIRBuilder::compare(ValueId lhs, ValueId rhs, ConstantId predicate, TypeId result_type) {
    TypeId lt = lhs < func_.values.size() ? func_.values[lhs].type : MIR_INVALID_ID;
    TypeId rt = rhs < func_.values.size() ? func_.values[rhs].type : MIR_INVALID_ID;
    ValueId v = new_value(result_type, VF_PURE);
    uint32_t i = emit(MIROpcode::Compare, v,
                      {MIROperand::value(lhs, lt), MIROperand::value(rhs, rt),
                       MIROperand::constant(predicate, MIR_INVALID_ID),
                       MIROperand::type(result_type)});
    note_result_value(v, i);
    return v;
}

ValueId MIRBuilder::cast(ValueId operand, TypeId result_type) {
    TypeId ot = operand < func_.values.size() ? func_.values[operand].type : MIR_INVALID_ID;
    ValueId v = new_value(result_type, VF_PURE);
    uint32_t i = emit(MIROpcode::Cast, v,
                      {MIROperand::value(operand, ot), MIROperand::type(result_type)});
    note_result_value(v, i);
    return v;
}

ValueId MIRBuilder::select(ValueId cond, ValueId true_v, ValueId false_v, TypeId result_type) {
    TypeId ct = cond < func_.values.size() ? func_.values[cond].type : MIR_INVALID_ID;
    TypeId tt = true_v < func_.values.size() ? func_.values[true_v].type : MIR_INVALID_ID;
    TypeId ft = false_v < func_.values.size() ? func_.values[false_v].type : MIR_INVALID_ID;
    ValueId v = new_value(result_type, VF_PURE);
    uint32_t i = emit(MIROpcode::Select, v,
                      {MIROperand::value(cond, ct), MIROperand::value(true_v, tt),
                       MIROperand::value(false_v, ft), MIROperand::type(result_type)});
    note_result_value(v, i);
    return v;
}

// ── calls ──────────────────────────────────────────────────────────────────

ValueId MIRBuilder::call_scalar(SymbolId callee, TypeId result_type,
                                const MIROperand* args, uint32_t count) {
    ValueId v = new_value(result_type, VF_NONE);
    MIROperand* ops = arena_.allocate_array<MIROperand>(count + 1);
    if (!ops) {
        ok_ = false;
        return v;
    }
    ops[0] = MIROperand::symbol(callee, MIR_INVALID_ID);
    for (uint32_t i = 0; i < count; ++i) ops[i + 1] = args[i];
    uint32_t idx = emit_impl(MIROpcode::Call, v, ops, count + 1);
    note_result_value(v, idx);
    return v;
}

void MIRBuilder::call_sret(SymbolId callee, PlaceId result_place,
                           const MIROperand* args, uint32_t count) {
    MIROperand* ops = arena_.allocate_array<MIROperand>(count + 2);
    if (!ops) {
        ok_ = false;
        return;
    }
    TypeId rt = result_place < func_.places.size() ? func_.places[result_place].type : MIR_INVALID_ID;
    ops[0] = MIROperand::symbol(callee, MIR_INVALID_ID);
    ops[1] = MIROperand::place(result_place, rt);
    for (uint32_t i = 0; i < count; ++i) ops[i + 2] = args[i];
    // sret is encoded by the result place operand (kind Place) immediately
    // after the callee symbol; the emitter reads the callee ABI and writes into it.
    emit_impl(MIROpcode::Call, result_place, ops, count + 2);
}

ValueId MIRBuilder::call_indirect(ValueId fn, TypeId result_type,
                                  const MIROperand* args, uint32_t count) {
    ValueId v = new_value(result_type, VF_NONE);
    MIROperand* ops = arena_.allocate_array<MIROperand>(count + 1);
    if (!ops) {
        ok_ = false;
        return v;
    }
    TypeId ft = fn < func_.values.size() ? func_.values[fn].type : MIR_INVALID_ID;
    ops[0] = MIROperand::value(fn, ft);
    for (uint32_t i = 0; i < count; ++i) ops[i + 1] = args[i];
    uint32_t idx = emit_impl(MIROpcode::CallIndirect, v, ops, count + 1);
    note_result_value(v, idx);
    return v;
}

// ── lifetime ───────────────────────────────────────────────────────────────

void MIRBuilder::init(PlaceId dest, SymbolId ctor, const MIROperand* args, uint32_t count) {
    MIROperand* ops = arena_.allocate_array<MIROperand>(count + 2);
    if (!ops) {
        ok_ = false;
        return;
    }
    TypeId dt = dest < func_.places.size() ? func_.places[dest].type : MIR_INVALID_ID;
    ops[0] = MIROperand::place(dest, dt);
    ops[1] = MIROperand::symbol(ctor, MIR_INVALID_ID);
    for (uint32_t i = 0; i < count; ++i) ops[i + 2] = args[i];
    emit_impl(MIROpcode::Init, MIR_NULL, ops, count + 2);
    if (dest < func_.places.size()) {
        func_.places[dest].init_state = static_cast<uint8_t>(MIRInitState::Initialized);
    }
}

void MIRBuilder::copy_init(PlaceId dest, ValueId src, TypeId type) {
    TypeId st = src < func_.values.size() ? func_.values[src].type : type;
    emit(MIROpcode::CopyInit, MIR_NULL,
         {MIROperand::place(dest, type), MIROperand::value(src, st), MIROperand::type(type)});
    if (dest < func_.places.size()) {
        func_.places[dest].init_state = static_cast<uint8_t>(MIRInitState::Initialized);
    }
}

void MIRBuilder::move_init(PlaceId dest, PlaceId src, TypeId type) {
    emit(MIROpcode::MoveInit, MIR_NULL,
         {MIROperand::place(dest, type), MIROperand::place(src, type), MIROperand::type(type)});
    if (dest < func_.places.size()) {
        func_.places[dest].init_state = static_cast<uint8_t>(MIRInitState::Initialized);
    }
    if (src < func_.places.size()) {
        func_.places[src].init_state = static_cast<uint8_t>(MIRInitState::Moved);
    }
}

void MIRBuilder::assign(PlaceId dest, ValueId src, TypeId type) {
    TypeId st = src < func_.values.size() ? func_.values[src].type : type;
    emit(MIROpcode::Assign, MIR_NULL,
         {MIROperand::place(dest, type), MIROperand::value(src, st), MIROperand::type(type)});
    if (dest < func_.places.size()) {
        func_.places[dest].init_state = static_cast<uint8_t>(MIRInitState::Initialized);
    }
}

void MIRBuilder::destroy(PlaceId place, SymbolId dtor) {
    TypeId pt = place < func_.places.size() ? func_.places[place].type : MIR_INVALID_ID;
    emit(MIROpcode::Destroy, MIR_NULL,
         {MIROperand::place(place, pt), MIROperand::symbol(dtor, MIR_INVALID_ID)});
    if (place < func_.places.size()) {
        func_.places[place].init_state = static_cast<uint8_t>(MIRInitState::Destroyed);
    }
}

void MIRBuilder::drop(PlaceId place, SymbolId dtor, ValueId flag) {
    TypeId pt = place < func_.places.size() ? func_.places[place].type : MIR_INVALID_ID;
    if (flag == MIR_NULL) {
        emit(MIROpcode::Drop, MIR_NULL,
             {MIROperand::place(place, pt), MIROperand::symbol(dtor, MIR_INVALID_ID)});
    } else {
        TypeId ft = flag < func_.values.size() ? func_.values[flag].type : MIR_INVALID_ID;
        emit(MIROpcode::Drop, MIR_NULL,
             {MIROperand::place(place, pt), MIROperand::symbol(dtor, MIR_INVALID_ID),
              MIROperand::value(flag, ft)});
    }
    if (place < func_.places.size()) {
        func_.places[place].init_state = static_cast<uint8_t>(MIRInitState::Destroyed);
    }
}

void MIRBuilder::set_drop(PlaceId flag_place, bool value) {
    TypeId ft = flag_place < func_.places.size() ? func_.places[flag_place].type : MIR_INVALID_ID;
    ConstantId c = module_.constants.add_bool(ft, value);
    emit(MIROpcode::SetDrop, MIR_NULL,
         {MIROperand::place(flag_place, ft), MIROperand::constant(c, ft)});
}

void MIRBuilder::memcpy(PlaceId dest, PlaceId src, ValueId size) {
    TypeId dt = dest < func_.places.size() ? func_.places[dest].type : MIR_INVALID_ID;
    TypeId st = src < func_.places.size() ? func_.places[src].type : MIR_INVALID_ID;
    TypeId zt = size < func_.values.size() ? func_.values[size].type : MIR_INVALID_ID;
    emit(MIROpcode::MemCpy, MIR_NULL,
         {MIROperand::place(dest, dt), MIROperand::place(src, st),
          MIROperand::value(size, zt)});
}

void MIRBuilder::copy_ptr_to_place(PlaceId dest, ValueId src_ptr) {
    TypeId dt = dest < func_.places.size() ? func_.places[dest].type : MIR_INVALID_ID;
    TypeId st = src_ptr < func_.values.size() ? func_.values[src_ptr].type : MIR_INVALID_ID;
    emit(MIROpcode::MemCpy, MIR_NULL,
         {MIROperand::place(dest, dt), MIROperand::value(src_ptr, st)});
}

void MIRBuilder::memset(PlaceId dest, ValueId byte, ValueId size) {
    TypeId dt = dest < func_.places.size() ? func_.places[dest].type : MIR_INVALID_ID;
    TypeId bt = byte < func_.values.size() ? func_.values[byte].type : MIR_INVALID_ID;
    TypeId zt = size < func_.values.size() ? func_.values[size].type : MIR_INVALID_ID;
    emit(MIROpcode::MemSet, MIR_NULL,
         {MIROperand::place(dest, dt), MIROperand::value(byte, bt),
          MIROperand::value(size, zt)});
}

void MIRBuilder::zero_init(PlaceId dest) {
    TypeId dt = dest < func_.places.size() ? func_.places[dest].type : MIR_INVALID_ID;
    emit(MIROpcode::MemSet, MIR_NULL, {MIROperand::place(dest, dt)});
}

// ── terminators ────────────────────────────────────────────────────────────

void MIRBuilder::ret(ValueId value) {
    TypeId vt = value < func_.values.size() ? func_.values[value].type : MIR_INVALID_ID;
    emit(MIROpcode::Return, MIR_NULL, {MIROperand::value(value, vt)});
}

void MIRBuilder::ret_void() {
    emit(MIROpcode::Return, MIR_NULL, {});
}

void MIRBuilder::br(BlockId target) {
    emit(MIROpcode::Br, MIR_NULL, {MIROperand::block(target)});
}

void MIRBuilder::cond_br(ValueId cond, BlockId true_block, BlockId false_block) {
    TypeId ct = cond < func_.values.size() ? func_.values[cond].type : MIR_INVALID_ID;
    emit(MIROpcode::CondBr, MIR_NULL,
         {MIROperand::value(cond, ct), MIROperand::block(true_block),
          MIROperand::block(false_block)});
}

void MIRBuilder::unreachable() {
    emit(MIROpcode::Unreachable, MIR_NULL, {});
}

// ── cleanup scopes ─────────────────────────────────────────────────────────

uint32_t MIRBuilder::push_cleanup_scope() {
    MIRCleanupScope scope;
    scope.parent = (cleanup_sp_ == MIR_INVALID_ID) ? MIR_INVALID_ID : cleanup_sp_;
    scope.owned_start = static_cast<uint32_t>(func_.owned_places.size());
    scope.owned_count = 0;
    scope.unwind_target = MIR_NULL;
    func_.cleanups.push_back(scope);
    cleanup_sp_ = static_cast<uint32_t>(func_.cleanups.size() - 1);
    return cleanup_sp_;
}

uint32_t MIRBuilder::pop_cleanup_scope() {
    if (cleanup_sp_ == MIR_INVALID_ID) return MIR_INVALID_ID;
    uint32_t idx = cleanup_sp_;
    cleanup_sp_ = func_.cleanups[idx].parent;
    return idx;
}

void MIRBuilder::register_owned(PlaceId place) {
    func_.owned_places.push_back(place);
    if (cleanup_sp_ != MIR_INVALID_ID) {
        func_.cleanups[cleanup_sp_].owned_count++;
    }
}

} // namespace mir
