// Copyright (c) Chemical Language Foundation 2025.

#include "MIRBorrowCheck.h"

#include <algorithm>
#include <string>
#include <unordered_map>
#include <vector>

namespace mir {

namespace {

struct LoanTarget {
    PlaceId root = MIR_INVALID_ID;
    uint32_t path = MIR_INVALID_ID;
    uint32_t param_index = MIR_INVALID_ID; // set for a parameter-rooted target
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

/** True when `from` outlives `to` (identity, 'static, elided, or a where edge). */
bool region_outlives(const MIRRegionSignature& sig, MIRRegionId from,
                     MIRRegionId to) {
    if (from == MIR_INVALID_ID || from == to || from == MIR_REGION_STATIC) {
        return true;
    }
    if (from == MIR_REGION_ELIDED && to == MIR_REGION_ELIDED) return true;
    std::vector<MIRRegionId> stack{from};
    std::vector<MIRRegionId> seen;
    while (!stack.empty()) {
        const MIRRegionId cur = stack.back();
        stack.pop_back();
        if (std::find(seen.begin(), seen.end(), cur) != seen.end()) continue;
        seen.push_back(cur);
        if (cur == to) return true;
        for (const auto& edge : sig.outlives) {
            if (edge.first == cur) stack.push_back(edge.second);
        }
    }
    return false;
}

uint32_t linear_of(uint32_t block, uint32_t inst, const std::vector<InstRef>& order) {
    for (uint32_t i = 0; i < order.size(); ++i) {
        if (order[i].block == block && order[i].inst == inst) return i;
    }
    return MIR_INVALID_ID;
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
    if (!d.has_loc && fn.has_decl_loc) {
        d.loc = fn.decl_loc;
        d.has_loc = true;
    }
    out.push_back(std::move(d));
}

} // namespace

MIRBorrowCheckResult check_function_borrows(const MIRModule& module,
                                            const MIRFunction& function,
                                            uint32_t function_index) {
    MIRBorrowCheckResult result;
    // No fast path: even a function with no explicit borrow can return a
    // reference parameter (the S4.6a outlives check), and a zero-loan CFG
    // liveness pass is trivial.

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
    // Reference values/places that carry a parameter's region (return outlives).
    std::unordered_map<ValueId, uint32_t> value_params;
    std::unordered_map<PlaceId, uint32_t> place_params;
    // A reference stored in a struct field: key = (place << 32) | field constant.
    std::unordered_map<uint64_t, uint32_t> field_loans;
    // A reference field sourced from a parameter (return-outlives check).
    std::unordered_map<uint64_t, uint32_t> field_params;
    auto field_key = [](PlaceId base, ConstantId field) -> uint64_t {
        return (static_cast<uint64_t>(base) << 32) | static_cast<uint64_t>(field);
    };
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
                t.param_index = function.places[t.root].param_index;
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
                    base.param_index = function.places[base.root].param_index;
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
                // A loaded reference parameter (or a local holding one) carries
                // the parameter's region for the return-outlives check.
                if (ops[0].kind() == MIROperandKind::Place &&
                    ops[0].id < function.places.size() &&
                    inst.result_or_place != MIR_INVALID_ID) {
                    const MIRPlaceDef& pd = function.places[ops[0].id];
                    uint32_t pidx = pd.param_index;
                    if (pidx == MIR_INVALID_ID) {
                        const auto pp = place_params.find(ops[0].id);
                        if (pp != place_params.end()) pidx = pp->second;
                    }
                    if (pidx != MIR_INVALID_ID &&
                        pd.type < module.types.types.size() &&
                        (module.types.types[pd.type].kind == MIRTypeKind::Reference ||
                         module.types.types[pd.type].kind == MIRTypeKind::Pointer)) {
                        value_params[inst.result_or_place] = pidx;
                    }
                }
                // Loading a reference local yields another value carrying its loan.
                if (ops[0].kind() == MIROperandKind::Place) {
                    const auto it = place_loans.find(ops[0].id);
                    if (it != place_loans.end() && inst.result_or_place != MIR_INVALID_ID) {
                        value_loans[inst.result_or_place] = it->second;
                        loans[it->second].end = lin;
                    }
                    // A reference stored in a struct field.
                    if (inst.operand_count >= 2 &&
                        ops[1].kind() == MIROperandKind::Constant &&
                        inst.result_or_place != MIR_INVALID_ID) {
                        const uint64_t fkey = field_key(ops[0].id, ops[1].id);
                        const auto fit = field_loans.find(fkey);
                        if (fit != field_loans.end()) {
                            value_loans[inst.result_or_place] = fit->second;
                            loans[fit->second].end = lin;
                        }
                        const auto fpi = field_params.find(fkey);
                        if (fpi != field_params.end()) {
                            value_params[inst.result_or_place] = fpi->second;
                        }
                    }
                }
                break;
            }
            case MIROpcode::Store: {
                if (ops == nullptr || inst.operand_count < 2) break;
                const MIROperand& value = ops[inst.operand_count - 1];
                use_value(value, lin, false);
                // A field store of a reference value records a field loan.
                if (inst.operand_count >= 3 &&
                    ops[0].kind() == MIROperandKind::Place &&
                    ops[1].kind() == MIROperandKind::Constant) {
                    const uint64_t key = field_key(ops[0].id, ops[1].id);
                    uint32_t loan = MIR_INVALID_ID;
                    uint32_t param = MIR_INVALID_ID;
                    if (value.kind() == MIROperandKind::Value) {
                        const auto vit = value_loans.find(value.id);
                        if (vit != value_loans.end()) loan = vit->second;
                        const auto vp = value_params.find(value.id);
                        if (vp != value_params.end()) param = vp->second;
                    } else if (value.kind() == MIROperandKind::Place) {
                        const auto vit = place_loans.find(value.id);
                        if (vit != place_loans.end()) loan = vit->second;
                        const auto pp = place_params.find(value.id);
                        if (pp != place_params.end()) param = pp->second;
                    }
                    if (loan != MIR_INVALID_ID) {
                        field_loans[key] = loan;
                    } else {
                        field_loans.erase(key);
                    }
                    if (param != MIR_INVALID_ID) {
                        field_params[key] = param;
                    } else {
                        field_params.erase(key);
                    }
                }
                // Whole-place store of a reference value: the place now holds it.
                if (inst.operand_count == 2 && ops[0].kind() == MIROperandKind::Place) {
                    // A place source is a move: carry references stored in its
                    // fields to the destination.
                    if (ops[1].kind() == MIROperandKind::Place) {
                        std::vector<std::pair<uint64_t, uint32_t>> moved;
                        std::vector<std::pair<uint64_t, uint32_t>> moved_params;
                        for (const auto& fl : field_loans) {
                            if (static_cast<PlaceId>(fl.first >> 32) != ops[1].id) continue;
                            const ConstantId field =
                                static_cast<ConstantId>(fl.first & 0xFFFFFFFFu);
                            moved.emplace_back(field_key(ops[0].id, field), fl.second);
                        }
                        for (const auto& fp : field_params) {
                            if (static_cast<PlaceId>(fp.first >> 32) != ops[1].id) continue;
                            const ConstantId field =
                                static_cast<ConstantId>(fp.first & 0xFFFFFFFFu);
                            moved_params.emplace_back(field_key(ops[0].id, field), fp.second);
                        }
                        for (auto it2 = field_loans.begin(); it2 != field_loans.end();) {
                            if (static_cast<PlaceId>(it2->first >> 32) == ops[0].id) {
                                it2 = field_loans.erase(it2);
                            } else {
                                ++it2;
                            }
                        }
                        for (auto it2 = field_params.begin(); it2 != field_params.end();) {
                            if (static_cast<PlaceId>(it2->first >> 32) == ops[0].id) {
                                it2 = field_params.erase(it2);
                            } else {
                                ++it2;
                            }
                        }
                        for (const auto& m : moved) {
                            field_loans[m.first] = m.second;
                            loans[m.second].end = lin;
                        }
                        for (const auto& m : moved_params) {
                            field_params[m.first] = m.second;
                        }
                        for (auto it2 = field_loans.begin(); it2 != field_loans.end();) {
                            if (static_cast<PlaceId>(it2->first >> 32) == ops[1].id) {
                                it2 = field_loans.erase(it2);
                            } else {
                                ++it2;
                            }
                        }
                        for (auto it2 = field_params.begin(); it2 != field_params.end();) {
                            if (static_cast<PlaceId>(it2->first >> 32) == ops[1].id) {
                                it2 = field_params.erase(it2);
                            } else {
                                ++it2;
                            }
                        }
                    }
                    if (value.kind() == MIROperandKind::Value) {
                        const auto vp = value_params.find(value.id);
                        if (vp != value_params.end()) {
                            place_params[ops[0].id] = vp->second;
                        } else {
                            place_params.erase(ops[0].id);
                        }
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
                {
                    const auto pp = place_params.find(ops[1].id);
                    if (pp != place_params.end()) {
                        place_params[ops[0].id] = pp->second;
                        if (inst.opcode() == MIROpcode::MoveInit) {
                            place_params.erase(ops[1].id);
                        }
                    } else {
                        place_params.erase(ops[0].id);
                    }
                }
                const auto it = place_loans.find(ops[1].id);
                if (it != place_loans.end()) {
                    place_loans[ops[0].id] = it->second;
                    loans[it->second].end = lin;
                    if (inst.opcode() == MIROpcode::MoveInit) {
                        place_loans.erase(ops[1].id);
                    }
                }
                // Transfer references stored in fields of the moved/copied value.
                {
                    std::vector<std::pair<uint64_t, uint32_t>> moved;
                    std::vector<std::pair<uint64_t, uint32_t>> moved_params;
                    for (const auto& fl : field_loans) {
                        const PlaceId base = static_cast<PlaceId>(fl.first >> 32);
                        if (base != ops[1].id) continue;
                        const ConstantId field = static_cast<ConstantId>(fl.first & 0xFFFFFFFFu);
                        moved.emplace_back(field_key(ops[0].id, field), fl.second);
                    }
                    for (const auto& fp : field_params) {
                        const PlaceId base = static_cast<PlaceId>(fp.first >> 32);
                        if (base != ops[1].id) continue;
                        const ConstantId field = static_cast<ConstantId>(fp.first & 0xFFFFFFFFu);
                        moved_params.emplace_back(field_key(ops[0].id, field), fp.second);
                    }
                    // The destination is overwritten (or freshly constructed):
                    // drop its previous field facts instead of merging them.
                    for (auto it2 = field_loans.begin(); it2 != field_loans.end();) {
                        if (static_cast<PlaceId>(it2->first >> 32) == ops[0].id) {
                            it2 = field_loans.erase(it2);
                        } else {
                            ++it2;
                        }
                    }
                    for (auto it2 = field_params.begin(); it2 != field_params.end();) {
                        if (static_cast<PlaceId>(it2->first >> 32) == ops[0].id) {
                            it2 = field_params.erase(it2);
                        } else {
                            ++it2;
                        }
                    }
                    for (const auto& m : moved) {
                        field_loans[m.first] = m.second;
                        loans[m.second].end = lin;
                    }
                    for (const auto& m : moved_params) {
                        field_params[m.first] = m.second;
                    }
                    if (inst.opcode() == MIROpcode::MoveInit) {
                        for (auto it2 = field_loans.begin(); it2 != field_loans.end();) {
                            if (static_cast<PlaceId>(it2->first >> 32) == ops[1].id) {
                                it2 = field_loans.erase(it2);
                            } else {
                                ++it2;
                            }
                        }
                        for (auto it2 = field_params.begin(); it2 != field_params.end();) {
                            if (static_cast<PlaceId>(it2->first >> 32) == ops[1].id) {
                                it2 = field_params.erase(it2);
                            } else {
                                ++it2;
                            }
                        }
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
                if (ops[0].kind() == MIROperandKind::Value &&
                    inst.result_or_place != MIR_INVALID_ID) {
                    const auto vp = value_params.find(ops[0].id);
                    if (vp != value_params.end()) {
                        value_params[inst.result_or_place] = vp->second;
                    }
                }
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
                    const auto vp = value_params.find(ops[0].id);
                    if (vp != value_params.end()) {
                        value_params[inst.result_or_place] = vp->second;
                    }
                }
                break;
            }
            case MIROpcode::Call:
            case MIROpcode::CallIndirect: {
                if (ops == nullptr) break;
                // Start at operand 1; operand 1 may be the sret result place.
                uint32_t first = 1;
                if (ops[1].kind() == MIROperandKind::Place &&
                    inst.result_or_place != MIR_INVALID_ID &&
                    ops[1].id == inst.result_or_place) {
                    first = 2;
                }
                // S4.5: use the callee's signature. A known callee that does
                // not return a reference cannot retain it (its own checker
                // would reject an escape), so the loan ends at the call. A
                // reference-returning callee may return one of its reference
                // arguments; the result then carries that loan. Unknown
                // callees (indirect, extern, no body) keep the conservative
                // escape mark.
                bool known = false;
                bool returns_reference = false;
                bool extern_like = false;
                if (inst.opcode() == MIROpcode::Call &&
                    ops[0].kind() == MIROperandKind::Symbol &&
                    ops[0].id < module.symbols.size()) {
                    const MIRSymbolRecord& rec = module.symbols.get(ops[0].id);
                    extern_like = !rec.has_body;
                    const TypeId ftype = rec.type;
                    if (!extern_like && ftype != MIR_INVALID_ID &&
                        ftype < module.types.types.size()) {
                        const MIRTypeRecord& fr = module.types.types[ftype];
                        if (fr.kind == MIRTypeKind::Function) {
                            known = true;
                            const TypeId ret = fr.element;
                            returns_reference =
                                ret != MIR_INVALID_ID && ret < module.types.types.size() &&
                                module.types.types[ret].kind == MIRTypeKind::Reference;
                        }
                    }
                }
                struct RefArg {
                    uint16_t op_index = 0;
                    uint32_t param_index = MIR_INVALID_ID;
                    uint32_t loan = MIR_INVALID_ID;
                    uint32_t source_param = MIR_INVALID_ID;
                };
                std::vector<RefArg> ref_args;
                for (uint16_t i = first; i < inst.operand_count; ++i) {
                    if (ops[i].kind() != MIROperandKind::Value) continue;
                    const auto it = value_loans.find(ops[i].id);
                    if (it == value_loans.end() || it->second >= loans.size()) {
                        continue;
                    }
                    RefArg ra;
                    ra.op_index = i;
                    ra.param_index = static_cast<uint32_t>(i - first);
                    ra.loan = it->second;
                    const auto vp = value_params.find(ops[i].id);
                    ra.source_param = vp != value_params.end()
                                          ? vp->second
                                          : loans[ra.loan].target.param_index;
                    ref_args.push_back(ra);
                }

                const MIRRegionSignature* callee_sig = nullptr;
                if (known) {
                    const uint32_t sid =
                        module.symbol_region_signature(ops[0].id);
                    if (sid != MIR_INVALID_ID &&
                        sid < module.region_signatures.size()) {
                        callee_sig = &module.region_signatures[sid];
                    }
                }

                // A reference result aliases the argument whose declared
                // region is the return region (or the unique argument whose
                // region outlives it). This makes `pick(&mut a, &mut b)` keep
                // the correct loan alive instead of dropping both.
                uint32_t mapped_loan = MIR_INVALID_ID;
                uint32_t mapped_source = MIR_INVALID_ID;
                if (known && returns_reference &&
                    inst.result_or_place != MIR_INVALID_ID) {
                    if (callee_sig != nullptr &&
                        callee_sig->return_region != MIR_INVALID_ID) {
                        const MIRRegionId ret = callee_sig->return_region;
                        std::vector<const RefArg*> identity;
                        std::vector<const RefArg*> sources;
                        for (const RefArg& ra : ref_args) {
                            if (ra.param_index >=
                                callee_sig->param_regions.size()) {
                                continue;
                            }
                            const MIRRegionId pr =
                                callee_sig->param_regions[ra.param_index];
                            if (pr == MIR_INVALID_ID) continue;
                            if (pr == ret) identity.push_back(&ra);
                            if (region_outlives(*callee_sig, pr, ret)) {
                                sources.push_back(&ra);
                            }
                        }
                        if (identity.size() == 1) {
                            mapped_loan = identity[0]->loan;
                            mapped_source = identity[0]->source_param;
                        } else if (identity.empty() && sources.size() == 1) {
                            mapped_loan = sources[0]->loan;
                            mapped_source = sources[0]->source_param;
                        } else if (ref_args.size() == 1) {
                            // Elided/static region: the single reference is
                            // still the only possible source.
                            mapped_loan = ref_args[0].loan;
                            mapped_source = ref_args[0].source_param;
                        }
                    } else if (ref_args.size() == 1) {
                        mapped_loan = ref_args[0].loan;
                        mapped_source = ref_args[0].source_param;
                    }
                }

                if (mapped_loan != MIR_INVALID_ID) {
                    for (uint16_t i = first; i < inst.operand_count; ++i) {
                        use_value(ops[i], lin, false);
                    }
                    value_loans[inst.result_or_place] = mapped_loan;
                    loans[mapped_loan].end = lin;
                    if (mapped_source != MIR_INVALID_ID) {
                        value_params[inst.result_or_place] = mapped_source;
                    }
                } else if (known && ref_args.size() <= 1) {
                    // Known callee with no retained reference: the borrow ends
                    // at the call.
                    for (uint16_t i = first; i < inst.operand_count; ++i) {
                        use_value(ops[i], lin, false);
                    }
                } else {
                    // Unknown/extern callee, or an ambiguous multi-reference
                    // result: interprocedural escape is not modeled yet.
                    for (uint16_t i = first; i < inst.operand_count; ++i) {
                        use_value(ops[i], lin, true);
                    }
                }
                break;
            }
            case MIROpcode::Return: {
                uint32_t loan = MIR_INVALID_ID;
                uint32_t return_param = MIR_INVALID_ID;
                PlaceId returned_place = MIR_INVALID_ID;
                if (inst.operand_count > 0) {
                    use_value(ops[0], lin, true);
                    if (ops[0].kind() == MIROperandKind::Value) {
                        const auto it = value_loans.find(ops[0].id);
                        if (it != value_loans.end()) loan = it->second;
                        const auto vp = value_params.find(ops[0].id);
                        if (vp != value_params.end()) return_param = vp->second;
                    } else if (ops[0].kind() == MIROperandKind::Place) {
                        returned_place = ops[0].id;
                        const auto it = place_loans.find(ops[0].id);
                        if (it != place_loans.end()) loan = it->second;
                        if (ops[0].id < function.places.size()) {
                            return_param = function.places[ops[0].id].param_index;
                        }
                        if (return_param == MIR_INVALID_ID) {
                            const auto pp = place_params.find(ops[0].id);
                            if (pp != place_params.end()) return_param = pp->second;
                        }
                    }
                } else {
                    // Aggregate returns use a hidden sret pointer: the value is
                    // stored to it immediately before `return` (which has no
                    // operand). Recover the source aggregate so its reference
                    // fields are checked too. The sret pointer is the first
                    // materialized parameter of the entry block.
                    auto is_sret_param = [&](ValueId value) -> bool {
                        if (value >= function.values.size()) return false;
                        const uint32_t def = function.values[value].def_inst;
                        if (def >= function.instructions.size()) return false;
                        if (function.instructions[def].opcode() != MIROpcode::Param) {
                            return false;
                        }
                        if (function.entry_block >= function.blocks.size()) {
                            return false;
                        }
                        return def == function.blocks[function.entry_block].inst_start;
                    };
                    for (uint32_t k = lin; k-- > 0;) {
                        const MIRInstruction& prev =
                            function.instructions[order[k].inst];
                        if (is_terminator(prev.opcode())) break;
                        if (prev.opcode() != MIROpcode::Store ||
                            prev.operand_count != 2) {
                            continue;
                        }
                        const MIROperand* pops = operands_of(prev);
                        if (pops != nullptr &&
                            pops[0].kind() == MIROperandKind::Value &&
                            pops[1].kind() == MIROperandKind::Place &&
                            is_sret_param(pops[0].id)) {
                            returned_place = pops[1].id;
                            break;
                        }
                    }
                }

                const MIRRegionSignature* sig = nullptr;
                if (function.region_signature != MIR_INVALID_ID &&
                    function.region_signature < module.region_signatures.size()) {
                    sig = &module.region_signatures[function.region_signature];
                }
                bool reported = false;
                auto check_param_region = [&](uint32_t param_index) -> bool {
                    if (param_index == MIR_INVALID_ID || sig == nullptr ||
                        sig->return_region == MIR_INVALID_ID ||
                        param_index >= sig->param_regions.size()) {
                        return true;
                    }
                    const MIRRegionId from = sig->param_regions[param_index];
                    if (region_outlives(*sig, from, sig->return_region)) {
                        return true;
                    }
                    report(result.diagnostics, function, function_index, order, lin,
                           "SAFE-LIFE-003",
                           "returned reference does not outlive the declared return lifetime (add a where 'a: 'b bound)");
                    return false;
                };

                if (loan != MIR_INVALID_ID && loan < loans.size()) {
                    const LoanTarget& target = loans[loan].target;
                    if (target.known && target.root < function.places.size()) {
                        const auto sc = static_cast<MIRStorageClass>(
                            function.places[target.root].storage_class
                        );
                        if (sc == MIRStorageClass::Local ||
                            sc == MIRStorageClass::Temporary ||
                            sc == MIRStorageClass::FrameField ||
                            sc == MIRStorageClass::FramePtr) {
                            report(result.diagnostics, function, function_index,
                                   order, lin, "SAFE-LIFE-001",
                                   "returning a reference to a local that does not outlive the function");
                            reported = true;
                        }
                        if (target.param_index != MIR_INVALID_ID) {
                            return_param = target.param_index;
                        }
                    }
                }
                if (!reported && !check_param_region(return_param)) {
                    reported = true;
                }
                // S4.6a: references stored in fields of a returned aggregate
                // escape with it; check them like a directly returned reference.
                if (!reported && returned_place != MIR_INVALID_ID) {
                    for (const auto& fl : field_loans) {
                        if (static_cast<PlaceId>(fl.first >> 32) !=
                            returned_place) {
                            continue;
                        }
                        const uint32_t field_loan = fl.second;
                        if (field_loan >= loans.size()) continue;
                        const LoanTarget& target = loans[field_loan].target;
                        if (!target.known ||
                            target.root >= function.places.size()) {
                            continue;
                        }
                        const auto sc = static_cast<MIRStorageClass>(
                            function.places[target.root].storage_class
                        );
                        if (sc == MIRStorageClass::Local ||
                            sc == MIRStorageClass::Temporary ||
                            sc == MIRStorageClass::FrameField ||
                            sc == MIRStorageClass::FramePtr) {
                            report(result.diagnostics, function, function_index,
                                   order, lin, "SAFE-LIFE-001",
                                   "returning a reference to a local that does not outlive the function");
                            reported = true;
                            break;
                        }
                    }
                }
                if (!reported && returned_place != MIR_INVALID_ID) {
                    for (const auto& fp : field_params) {
                        if (static_cast<PlaceId>(fp.first >> 32) !=
                            returned_place) {
                            continue;
                        }
                        if (!check_param_region(fp.second)) {
                            reported = true;
                            break;
                        }
                    }
                }
                break;
            }
            default:
                break;
        }
    }

    // ── CFG liveness over loans (S4.4b) ────────────────────────────────────
    const uint32_t nloans = static_cast<uint32_t>(loans.size());
    const uint32_t nblocks = static_cast<uint32_t>(function.blocks.size());
    std::vector<std::vector<bool>> block_use(
        nblocks, std::vector<bool>(nloans, false)
    );
    std::vector<std::vector<bool>> block_def(
        nblocks, std::vector<bool>(nloans, false)
    );
    auto inst_range_end = [&](const MIRBlock& block) -> uint32_t {
        uint32_t end = block.inst_start + block.inst_count;
        if (end < function.instructions.size() &&
            is_terminator(function.instructions[end].opcode())) {
            return end + 1;
        }
        return end;
    };
    for (uint32_t b = 0; b < nblocks; ++b) {
        const MIRBlock& block = function.blocks[b];
        if (block.inst_start == MIR_INVALID_ID) continue;
        const uint32_t end = inst_range_end(block);
        // Upward-exposed uses: a use only contributes to live-in when the loan
        // has not been defined earlier in this block (standard gen/kill). This
        // matters for loop bodies whose blocks define and use the same loan.
        std::vector<bool> defined_in_block(nloans, false);
        for (uint32_t ii = block.inst_start; ii < end && ii < function.instructions.size(); ++ii) {
            const MIRInstruction& inst = function.instructions[ii];
            const MIROperand* ops = inst.operand_count == 0
                                        ? nullptr
                                        : &function.operands[inst.operand_offset];
            for (uint16_t i = 0; ops != nullptr && i < inst.operand_count; ++i) {
                if (ops[i].kind() != MIROperandKind::Value) continue;
                const auto it = value_loans.find(ops[i].id);
                if (it != value_loans.end() && it->second < nloans &&
                    !defined_in_block[it->second]) {
                    block_use[b][it->second] = true;
                }
            }
            // Reading a reference local is a use of its loan even when the
            // loaded value is discarded.
            if (inst.opcode() == MIROpcode::Load && ops != nullptr &&
                ops[0].kind() == MIROperandKind::Place) {
                const auto it = place_loans.find(ops[0].id);
                if (it != place_loans.end() && it->second < nloans &&
                    !defined_in_block[it->second]) {
                    block_use[b][it->second] = true;
                }
                if (inst.operand_count >= 2 && ops[1].kind() == MIROperandKind::Constant) {
                    const auto fit = field_loans.find(field_key(ops[0].id, ops[1].id));
                    if (fit != field_loans.end() && fit->second < nloans &&
                        !defined_in_block[fit->second]) {
                        block_use[b][fit->second] = true;
                    }
                }
            }
            // Definitions kill liveness before them; record after the uses.
            if ((inst.opcode() == MIROpcode::Borrow ||
                 inst.opcode() == MIROpcode::BorrowMut) &&
                inst.result_or_place != MIR_INVALID_ID) {
                const auto it = value_loans.find(inst.result_or_place);
                if (it != value_loans.end() && it->second < nloans) {
                    block_def[b][it->second] = true;
                    defined_in_block[it->second] = true;
                }
            }
        }
    }

    std::vector<std::vector<BlockId>> successors(nblocks);
    for (uint32_t b = 0; b < nblocks; ++b) {
        const MIRBlock& block = function.blocks[b];
        if (block.inst_start == MIR_INVALID_ID) continue;
        const uint32_t term = block.inst_start + block.inst_count;
        if (term >= function.instructions.size()) continue;
        const MIRInstruction& t = function.instructions[term];
        const MIROperand* ops = t.operand_count == 0
                                    ? nullptr
                                    : &function.operands[t.operand_offset];
        switch (t.opcode()) {
            case MIROpcode::Br:
                if (ops && t.operand_count >= 1 && ops[0].kind() == MIROperandKind::Block) {
                    successors[b].push_back(ops[0].id);
                }
                break;
            case MIROpcode::CondBr:
            case MIROpcode::Switch:
                if (ops) {
                    for (uint16_t i = 1; i < t.operand_count; ++i) {
                        if (ops[i].kind() == MIROperandKind::Block) {
                            successors[b].push_back(ops[i].id);
                        }
                    }
                }
                break;
            case MIROpcode::Throw:
                if (ops && t.operand_count >= 2 && ops[1].kind() == MIROperandKind::Block) {
                    successors[b].push_back(ops[1].id);
                }
                break;
            default:
                break;
        }
    }

    std::vector<std::vector<bool>> live_in(
        nblocks, std::vector<bool>(nloans, false)
    );
    std::vector<std::vector<bool>> live_out(
        nblocks, std::vector<bool>(nloans, false)
    );
    bool changed = true;
    while (changed) {
        changed = false;
        for (uint32_t b = 0; b < nblocks; ++b) {
            std::vector<bool> out(nloans, false);
            for (BlockId succ : successors[b]) {
                if (succ >= nblocks) continue;
                for (uint32_t l = 0; l < nloans; ++l) {
                    if (live_in[succ][l]) out[l] = true;
                }
            }
            std::vector<bool> in = block_use[b];
            for (uint32_t l = 0; l < nloans; ++l) {
                if (out[l] && !block_def[b][l]) in[l] = true;
            }
            if (in != live_in[b] || out != live_out[b]) {
                live_in[b] = std::move(in);
                live_out[b] = std::move(out);
                changed = true;
            }
        }
    }

    auto target_from_address = [&](const MIROperand& op, LoanTarget& out) -> bool {
        if (op.kind() != MIROperandKind::Value) return false;
        const auto at = address_targets.find(op.id);
        if (at != address_targets.end()) {
            out = at->second;
            return out.known;
        }
        const auto lt = value_loans.find(op.id);
        if (lt != value_loans.end() && lt->second < loans.size()) {
            out = loans[lt->second].target;
            return out.known;
        }
        return false;
    };

    // Walk each block backwards from live_out so a conflict/move is only
    // reported while a loan is actually live at that instruction.
    for (uint32_t b = 0; b < nblocks; ++b) {
        const MIRBlock& block = function.blocks[b];
        if (block.inst_start == MIR_INVALID_ID) continue;
        std::vector<bool> live = live_out[b];
        // The terminator is the last element of the range; include its uses.
        uint32_t end = inst_range_end(block);
        for (uint32_t ii = end; ii-- > block.inst_start;) {
            if (ii >= function.instructions.size()) continue;
            const MIRInstruction& inst = function.instructions[ii];
            const MIROperand* ops = inst.operand_count == 0
                                        ? nullptr
                                        : &function.operands[inst.operand_offset];
            // A local's storage ends while a loan that refers to it is still
            // live: the reference outlives its referent (S4.4c).
            if (inst.opcode() == MIROpcode::StorageDead && ops != nullptr &&
                ops[0].kind() == MIROperandKind::Place) {
                for (uint32_t l = 0; l < nloans; ++l) {
                    if (!live[l] || loans[l].escaped || !loans[l].target.known) continue;
                    if (loans[l].target.root != ops[0].id) continue;
                    report(result.diagnostics, function, function_index, order,
                           linear_of(b, ii, order), "SAFE-LIFE-002",
                           "a borrow is still live when the storage of the local it refers to ends");
                    break;
                }
            }

            // Move/drop while a live loan targets an overlapping place.
            PlaceId moved_place = MIR_INVALID_ID;
            switch (inst.opcode()) {
                case MIROpcode::MoveInit:
                    if (inst.operand_count >= 2 && ops != nullptr &&
                        ops[1].kind() == MIROperandKind::Place) {
                        moved_place = ops[1].id;
                    }
                    break;
                case MIROpcode::MoveOut:
                case MIROpcode::Drop:
                case MIROpcode::Destroy:
                    if (ops != nullptr && ops[0].kind() == MIROperandKind::Place) {
                        moved_place = ops[0].id;
                    }
                    break;
                case MIROpcode::Store:
                    if (inst.operand_count == 2 &&
                        ops[1].kind() == MIROperandKind::Place) {
                        moved_place = ops[1].id;
                    }
                    break;
                default:
                    break;
            }
            if (moved_place != MIR_INVALID_ID && moved_place < function.places.size()) {
                LoanTarget moved;
                moved.root = moved_place;
                moved.path = function.places[moved_place].move_path_id;
                moved.known = true;
                for (uint32_t l = 0; l < nloans; ++l) {
                    if (!live[l] || loans[l].escaped || !loans[l].target.known) continue;
                    if (!targets_overlap(function, loans[l].target, moved)) continue;
                    report(result.diagnostics, function, function_index, order,
                           linear_of(b, ii, order), "SAFE-BORROW-002",
                           "moving or dropping a place while a borrow of it is live");
                    break;
                }
            }

            // Uses in this instruction make their loans live before it.
            for (uint16_t i = 0; ops != nullptr && i < inst.operand_count; ++i) {
                if (ops[i].kind() != MIROperandKind::Value) continue;
                const auto it = value_loans.find(ops[i].id);
                if (it != value_loans.end() && it->second < nloans) {
                    live[it->second] = true;
                }
            }
            if (inst.opcode() == MIROpcode::Load && ops != nullptr &&
                ops[0].kind() == MIROperandKind::Place) {
                const auto it = place_loans.find(ops[0].id);
                if (it != place_loans.end() && it->second < nloans) {
                    live[it->second] = true;
                }
                if (inst.operand_count >= 2 && ops[1].kind() == MIROperandKind::Constant) {
                    const auto fit = field_loans.find(field_key(ops[0].id, ops[1].id));
                    if (fit != field_loans.end() && fit->second < nloans) {
                        live[fit->second] = true;
                    }
                }
            }

            // A borrow definition: check conflicts against the loans already
            // live at this point, then the new loan is not live before it.
            if ((inst.opcode() == MIROpcode::Borrow ||
                 inst.opcode() == MIROpcode::BorrowMut) &&
                inst.result_or_place != MIR_INVALID_ID) {
                const auto it = value_loans.find(inst.result_or_place);
                if (it != value_loans.end() && it->second < nloans) {
                    const uint32_t l = it->second;
                    Loan& loan = loans[l];
                    // An escaped loan's lifetime is unknown; do not report it
                    // as the *source* of a conflict either.
                    for (uint32_t other = 0; !loan.escaped && other < nloans; ++other) {
                        if (other == l || !live[other]) continue;
                        if (loans[other].escaped || !loans[other].target.known) continue;
                        if (!(loan.mutable_loan || loans[other].mutable_loan)) continue;
                        if (!targets_overlap(function, loan.target, loans[other].target)) {
                            continue;
                        }
                        // A borrow *through another reference value* is a
                        // reborrow; the source loan is frozen, not aliased the
                        // same way. A fresh `&mut x` address is not a reborrow
                        // even though it targets the same place.
                        bool source_is_reborrow = false;
                        LoanTarget source;
                        if (ops != nullptr && ops[0].kind() == MIROperandKind::Value) {
                            const auto st = value_loans.find(ops[0].id);
                            if (st != value_loans.end() && st->second < loans.size()) {
                                source = loans[st->second].target;
                                source_is_reborrow = source.known;
                            }
                        }
                        if (source_is_reborrow && source.root == loans[other].target.root &&
                            source.path == loans[other].target.path) {
                            continue;
                        }
                        report(result.diagnostics, function, function_index, order,
                               linear_of(b, ii, order), "SAFE-BORROW-001",
                               "placing a mutable borrow here conflicts with another live borrow of the same place");
                        break;
                    }
                    live[l] = false;
                }
            }
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
