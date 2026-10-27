// Copyright (c) Chemical Language Foundation 2025.

#include "MIRBorrowCheck.h"

#include <string>
#include <unordered_map>
#include <vector>

namespace mir {

namespace {

struct LoanTarget {
    PlaceId root = MIR_INVALID_ID;
    uint32_t path = MIR_INVALID_ID;
    bool known = false;
};

struct Loan {
    LoanTarget target;
    bool mutable_loan = false;
    uint32_t start = MIR_INVALID_ID; // linear instruction index
    uint32_t end = MIR_INVALID_ID;
    uint32_t borrow_lin = MIR_INVALID_ID;
    bool escaped = false; // passed to a call: interprocedural escape is S4.5
};

struct InstRef {
    uint32_t block = MIR_INVALID_ID;
    uint32_t inst = MIR_INVALID_ID;
};

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

uint32_t child_field_path(const MIRFunction& fn, uint32_t path, uint32_t field) {
    if (path == MIR_INVALID_ID || path >= fn.move_paths.size()) return MIR_INVALID_ID;
    for (uint32_t child = fn.move_paths[path].first_child;
         child != MIR_INVALID_ID && child < fn.move_paths.size();
         child = fn.move_paths[child].next_sibling) {
        if (fn.move_paths[child].field_index == field) return child;
    }
    return MIR_INVALID_ID;
}

uint32_t child_array_path(const MIRFunction& fn, uint32_t path, uint32_t index) {
    if (path == MIR_INVALID_ID || path >= fn.move_paths.size()) return MIR_INVALID_ID;
    for (uint32_t child = fn.move_paths[path].first_child;
         child != MIR_INVALID_ID && child < fn.move_paths.size();
         child = fn.move_paths[child].next_sibling) {
        if (fn.move_paths[child].array_index == index) return child;
    }
    return MIR_INVALID_ID;
}

bool constant_index(const MIRModule& module, ValueId value, const MIRFunction& fn,
                    uint64_t& out) {
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

uint32_t project_path(const MIRModule& module, const MIRFunction& fn, uint32_t path,
                      TypeId aggregate, const MIROperand& projection) {
    if (path == MIR_INVALID_ID) return MIR_INVALID_ID;
    if (projection.kind() == MIROperandKind::Constant) {
        const uint32_t field = field_index_for_constant(
            module, module.types, aggregate, projection.id
        );
        if (field != MIR_INVALID_ID) {
            const uint32_t child = child_field_path(fn, path, field);
            if (child != MIR_INVALID_ID) return child;
        }
        return path;
    }
    if (projection.kind() == MIROperandKind::Value) {
        uint64_t index = 0;
        if (constant_index(module, projection.id, fn, index)) {
            const uint32_t child = child_array_path(fn, path, static_cast<uint32_t>(index));
            if (child != MIR_INVALID_ID) return child;
        }
        return path;
    }
    return path;
}

bool paths_overlap(const MIRFunction& fn, uint32_t a, uint32_t b) {
    if (a == b) return true;
    // An untracked subobject (a scalar field, a dynamic index) can overlap any
    // path below the same root.
    if (a == MIR_INVALID_ID || b == MIR_INVALID_ID) return true;
    uint32_t p = a;
    while (p != MIR_INVALID_ID && p < fn.move_paths.size()) {
        if (p == b) return true;
        p = fn.move_paths[p].parent_path;
    }
    p = b;
    while (p != MIR_INVALID_ID && p < fn.move_paths.size()) {
        if (p == a) return true;
        p = fn.move_paths[p].parent_path;
    }
    return false;
}

bool targets_overlap(const MIRFunction& fn, const LoanTarget& a, const LoanTarget& b) {
    if (!a.known || !b.known) return false;
    if (a.root != b.root) return false;
    return paths_overlap(fn, a.path, b.path);
}

void report(std::vector<MIRBorrowDiagnostic>& out, const MIRFunction& fn,
            uint32_t function_index, const std::vector<InstRef>& order, uint32_t lin,
            const char* code, std::string message) {
    MIRBorrowDiagnostic d;
    d.function_index = function_index;
    d.code = code;
    d.message = std::move(message);
    if (lin < order.size()) {
        d.block_index = order[lin].block;
        d.inst_index = order[lin].inst;
        if (d.inst_index < fn.instructions.size()) {
            const uint32_t src = fn.instructions[d.inst_index].source_index;
            if (src != MIR_NO_SOURCE && src < fn.sources.size()) {
                d.loc = fn.sources[src];
                d.has_loc = true;
            }
        }
    }
    out.push_back(std::move(d));
}

} // namespace

MIRBorrowCheckResult check_function_borrows(const MIRModule& module,
                                            const MIRFunction& function,
                                            uint32_t function_index) {
    MIRBorrowCheckResult result;
    // Fast path: no explicit borrows.
    bool has_borrow = false;
    for (const MIRInstruction& inst : function.instructions) {
        if (inst.opcode() == MIROpcode::Borrow || inst.opcode() == MIROpcode::BorrowMut) {
            has_borrow = true;
            break;
        }
    }
    if (!has_borrow) return result;

    // Linear order over non-terminator instructions and terminators.
    std::vector<InstRef> order;
    order.reserve(function.instructions.size());
    for (const MIRBlock& block : function.blocks) {
        if (block.inst_start == MIR_INVALID_ID) continue;
        for (uint32_t i = 0; i < block.inst_count; ++i) {
            order.push_back({block.id, block.inst_start + i});
        }
        const uint32_t term = block.inst_start + block.inst_count;
        if (term < function.instructions.size() &&
            is_terminator(function.instructions[term].opcode())) {
            order.push_back({block.id, term});
        }
    }

    std::unordered_map<ValueId, LoanTarget> address_targets;
    std::unordered_map<ValueId, uint32_t> value_loans;
    std::unordered_map<PlaceId, uint32_t> place_loans;
    std::vector<Loan> loans;

    auto operands_of = [&](const MIRInstruction& inst) {
        return inst.operand_count == 0 ? static_cast<const MIROperand*>(nullptr)
                                       : &function.operands[inst.operand_offset];
    };
    auto use_value = [&](const MIROperand& op, uint32_t lin, bool may_escape) {
        if (op.kind() != MIROperandKind::Value) return;
        const auto it = value_loans.find(op.id);
        if (it == value_loans.end()) return;
        Loan& loan = loans[it->second];
        loan.end = lin;
        if (may_escape) loan.escaped = true;
    };
    auto target_of_address = [&](const MIROperand& op, LoanTarget& out) -> bool {
        if (op.kind() != MIROperandKind::Value) return false;
        const auto at = address_targets.find(op.id);
        if (at != address_targets.end()) {
            out = at->second;
            return out.known;
        }
        // `&*p` / a reborrow through a reference value: inherit the source loan.
        const auto lt = value_loans.find(op.id);
        if (lt != value_loans.end() && lt->second < loans.size()) {
            out = loans[lt->second].target;
            return out.known;
        }
        return false;
    };

    for (uint32_t lin = 0; lin < order.size(); ++lin) {
        const InstRef ref = order[lin];
        if (ref.inst >= function.instructions.size()) continue;
        const MIRInstruction& inst = function.instructions[ref.inst];
        const MIROperand* ops = operands_of(inst);
        switch (inst.opcode()) {
            case MIROpcode::AddressOf: {
                if (ops == nullptr || ops[0].kind() != MIROperandKind::Place ||
                    ops[0].id >= function.places.size()) {
                    break;
                }
                LoanTarget t;
                t.root = ops[0].id;
                t.path = function.places[t.root].move_path_id;
                t.known = true;
                if (inst.result_or_place != MIR_INVALID_ID) {
                    address_targets[inst.result_or_place] = t;
                }
                break;
            }
            case MIROpcode::FieldAddr:
            case MIROpcode::IndexAddr: {
                if (ops == nullptr || inst.operand_count < 2) break;
                LoanTarget base;
                TypeId aggregate = MIR_INVALID_ID;
                if (ops[0].kind() == MIROperandKind::Place &&
                    ops[0].id < function.places.size()) {
                    base.root = ops[0].id;
                    base.path = function.places[base.root].move_path_id;
                    base.known = true;
                    aggregate = function.places[base.root].type;
                } else if (ops[0].kind() == MIROperandKind::Value) {
                    const auto at = address_targets.find(ops[0].id);
                    if (at != address_targets.end()) {
                        base = at->second;
                    } else {
                        target_of_address(ops[0], base);
                    }
                    if (ops[0].id < function.values.size()) {
                        aggregate = function.values[ops[0].id].type;
                    }
                }
                if (!base.known) break;
                LoanTarget projected = base;
                projected.path = project_path(
                    module, function, base.path, aggregate, ops[1]
                );
                if (inst.result_or_place != MIR_INVALID_ID) {
                    address_targets[inst.result_or_place] = projected;
                }
                break;
            }
            case MIROpcode::Borrow:
            case MIROpcode::BorrowMut: {
                if (ops == nullptr || inst.operand_count == 0) break;
                // The address operand is a use of any loan it reborrows.
                use_value(ops[0], lin, false);
                Loan loan;
                target_of_address(ops[0], loan.target);
                loan.mutable_loan = inst.opcode() == MIROpcode::BorrowMut;
                loan.start = lin;
                loan.end = lin;
                loan.borrow_lin = lin;
                const uint32_t idx = static_cast<uint32_t>(loans.size());
                loans.push_back(std::move(loan));
                if (inst.result_or_place != MIR_INVALID_ID) {
                    value_loans[inst.result_or_place] = idx;
                }
                break;
            }
            case MIROpcode::Load: {
                if (ops == nullptr || inst.operand_count == 0) break;
                // A dereference through a reference/pointer value uses the loan.
                if (ops[0].kind() == MIROperandKind::Value) {
                    use_value(ops[0], lin, false);
                }
                // Loading a reference local yields another value carrying its loan.
                if (ops[0].kind() == MIROperandKind::Place) {
                    const auto it = place_loans.find(ops[0].id);
                    if (it != place_loans.end() && inst.result_or_place != MIR_INVALID_ID) {
                        value_loans[inst.result_or_place] = it->second;
                        loans[it->second].end = lin;
                    }
                }
                break;
            }
            case MIROpcode::Store: {
                if (ops == nullptr || inst.operand_count < 2) break;
                const MIROperand& value = ops[inst.operand_count - 1];
                use_value(value, lin, false);
                // Whole-place store of a reference value: the place now holds it.
                if (inst.operand_count == 2 && ops[0].kind() == MIROperandKind::Place) {
                    if (value.kind() == MIROperandKind::Value) {
                        const auto it = value_loans.find(value.id);
                        if (it != value_loans.end()) {
                            place_loans[ops[0].id] = it->second;
                            break;
                        }
                        if (ops[0].id < function.places.size()) {
                            const TypeId pt = function.places[ops[0].id].type;
                            if (pt < module.types.types.size()) {
                                const MIRTypeRecord& rec = module.types.types[pt];
                                if (rec.kind == MIRTypeKind::Reference ||
                                    rec.kind == MIRTypeKind::Pointer) {
                                    place_loans.erase(ops[0].id);
                                }
                            }
                        }
                    }
                }
                break;
            }
            case MIROpcode::MoveInit:
            case MIROpcode::CopyInit: {
                if (ops == nullptr || inst.operand_count < 2) break;
                if (ops[0].kind() != MIROperandKind::Place ||
                    ops[1].kind() != MIROperandKind::Place) {
                    break;
                }
                const auto it = place_loans.find(ops[1].id);
                if (it != place_loans.end()) {
                    place_loans[ops[0].id] = it->second;
                    loans[it->second].end = lin;
                    if (inst.opcode() == MIROpcode::MoveInit) {
                        place_loans.erase(ops[1].id);
                    }
                }
                break;
            }
            case MIROpcode::Assign: {
                if (ops == nullptr || inst.operand_count < 2) break;
                use_value(ops[1], lin, false);
                if (ops[0].kind() == MIROperandKind::Place &&
                    ops[1].kind() == MIROperandKind::Value) {
                    const auto it = value_loans.find(ops[1].id);
                    if (it != value_loans.end()) place_loans[ops[0].id] = it->second;
                }
                break;
            }
            case MIROpcode::Copy: {
                if (ops == nullptr || inst.operand_count == 0) break;
                use_value(ops[0], lin, false);
                if (ops[0].kind() == MIROperandKind::Place &&
                    inst.result_or_place != MIR_INVALID_ID) {
                    const auto it = place_loans.find(ops[0].id);
                    if (it != place_loans.end()) {
                        value_loans[inst.result_or_place] = it->second;
                        loans[it->second].end = lin;
                    }
                }
                break;
            }
            case MIROpcode::Cast: {
                if (ops == nullptr || inst.operand_count == 0) break;
                use_value(ops[0], lin, false);
                if (ops[0].kind() == MIROperandKind::Value &&
                    inst.result_or_place != MIR_INVALID_ID) {
                    const auto it = value_loans.find(ops[0].id);
                    if (it != value_loans.end()) {
                        value_loans[inst.result_or_place] = it->second;
                    }
                }
                break;
            }
            case MIROpcode::Call:
            case MIROpcode::CallIndirect: {
                if (ops == nullptr) break;
                // A reference passed to a call may escape: mark it (the
                // interprocedural lifetime check is S4.5). Start at operand 1;
                // operand 1 may be the sret result place.
                uint32_t first = 1;
                if (ops[1].kind() == MIROperandKind::Place &&
                    inst.result_or_place != MIR_INVALID_ID &&
                    ops[1].id == inst.result_or_place) {
                    first = 2;
                }
                for (uint16_t i = first; i < inst.operand_count; ++i) {
                    use_value(ops[i], lin, true);
                }
                break;
            }
            case MIROpcode::Return: {
                if (ops == nullptr || inst.operand_count == 0) break;
                use_value(ops[0], lin, true);
                break;
            }
            default:
                break;
        }
    }

    // Conflicts between live loans.
    for (uint32_t i = 0; i < loans.size(); ++i) {
        const Loan& a = loans[i];
        if (a.escaped || !a.target.known) continue;
        for (uint32_t j = i + 1; j < loans.size(); ++j) {
            const Loan& b = loans[j];
            if (b.escaped || !b.target.known) continue;
            if (!(a.mutable_loan || b.mutable_loan)) continue;
            if (b.start > a.end || a.start > b.end) continue;
            if (!targets_overlap(function, a.target, b.target)) continue;
            const uint32_t at = a.start > b.start ? a.start : b.start;
            report(result.diagnostics, function, function_index, order, at,
                   "SAFE-BORROW-001",
                   "placing a mutable borrow here conflicts with another live borrow of the same place");
        }
    }

    // Moves/drops of a borrowed place.
    for (uint32_t lin = 0; lin < order.size(); ++lin) {
        const InstRef ref = order[lin];
        if (ref.inst >= function.instructions.size()) continue;
        const MIRInstruction& inst = function.instructions[ref.inst];
        const MIROperand* ops = operands_of(inst);
        if (ops == nullptr) continue;
        PlaceId moved_place = MIR_INVALID_ID;
        switch (inst.opcode()) {
            case MIROpcode::MoveInit:
                // dest, src: the source is the moved place.
                if (inst.operand_count >= 2 && ops[1].kind() == MIROperandKind::Place) {
                    moved_place = ops[1].id;
                }
                break;
            case MIROpcode::Drop:
            case MIROpcode::Destroy:
                if (ops[0].kind() == MIROperandKind::Place) moved_place = ops[0].id;
                break;
            case MIROpcode::MoveOut:
                if (ops[0].kind() == MIROperandKind::Place) moved_place = ops[0].id;
                break;
            case MIROpcode::Store:
                // A place source is a move out of that place (whole store).
                if (inst.operand_count == 2 && ops[1].kind() == MIROperandKind::Place) {
                    moved_place = ops[1].id;
                }
                break;
            default:
                break;
        }
        if (moved_place == MIR_INVALID_ID || moved_place >= function.places.size()) continue;
        LoanTarget moved;
        moved.root = moved_place;
        moved.path = function.places[moved_place].move_path_id;
        moved.known = true;
        for (const Loan& loan : loans) {
            if (loan.escaped || !loan.target.known) continue;
            if (lin < loan.start || lin > loan.end) continue;
            if (!targets_overlap(function, loan.target, moved)) continue;
            report(result.diagnostics, function, function_index, order, lin,
                   "SAFE-BORROW-002",
                   "moving or dropping a place while a borrow of it is live");
            break;
        }
    }

    return result;
}

MIRBorrowCheckResult check_module_borrows(const MIRModule& module) {
    MIRBorrowCheckResult result;
    for (uint32_t f = 0; f < module.functions.size(); ++f) {
        MIRBorrowCheckResult one = check_function_borrows(module, module.functions[f], f);
        for (MIRBorrowDiagnostic& d : one.diagnostics) {
            result.diagnostics.push_back(std::move(d));
        }
    }
    return result;
}

} // namespace mir
