// Copyright (c) Chemical Language Foundation 2025.

#include "MIRBorrowCheck.h"

#include <algorithm>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace mir {

namespace {

struct LoanProjection {
    /** Constant field or array index, or MIR_INVALID_ID when it is dynamic. */
    uint32_t index = MIR_INVALID_ID;
    bool array = false;
};

struct LoanTarget {
    PlaceId root = MIR_INVALID_ID;
    uint32_t path = MIR_INVALID_ID;
    uint32_t param_index = MIR_INVALID_ID; // set for a parameter-rooted target
    bool known = false;
    /**
     * Constant projections from the root, used for borrow-overlap precision
     * (S4.5). The move-path tree only exists for owned aggregates, so an
     * array of `Copy` elements would otherwise make `a[0]` and `a[1]` the
     * same target.
     */
    std::vector<LoanProjection> projections;
};

struct Loan {
    LoanTarget target;
    bool mutable_loan = false;
    uint32_t start = MIR_INVALID_ID; // linear instruction index
    uint32_t end = MIR_INVALID_ID;
    uint32_t borrow_lin = MIR_INVALID_ID;
    bool escaped = false; // passed to a call: interprocedural escape is S4.5
    /**
     * A multi-borrow HRTB call may retain any of its arguments. Local
     * referents are kept live until the end of the function so storage-end,
     * move and alias checks conservatively cover a retained borrow.
     */
    bool pinned_live = false;
    /**
     * Declared region for a reference read out of an aggregate parameter's
     * field (S4.2b). `MIR_INVALID_ID` for ordinary source borrows, where the
     * parameter's own region is the right fallback.
     */
    MIRRegionId region = MIR_INVALID_ID;
    /**
     * A reference held in a coroutine frame field (S4.1). The borrow may have
     * been created before a suspension, so the poll function sees only the
     * frame field. While live, moving/dropping another frame field is a
     * use-after-move of the referent.
     */
    bool frame_loan = false;
    PlaceId frame_field_place = MIR_INVALID_ID;
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

/**
 * Borrow-overlap check using constant projections (S4.5). `a[0]` and `a[1]`
 * are disjoint even when the array element type has no move path; a dynamic
 * index is conservatively unknown and overlaps everything below its root.
 */
bool borrow_targets_overlap(const LoanTarget& a, const LoanTarget& b) {
    if (!a.known || !b.known) return false;
    if (a.root != b.root) return false;
    const size_t n = std::min(a.projections.size(), b.projections.size());
    for (size_t i = 0; i < n; ++i) {
        const LoanProjection& pa = a.projections[i];
        const LoanProjection& pb = b.projections[i];
        if (pa.index == MIR_INVALID_ID || pb.index == MIR_INVALID_ID) return true;
        if (pa.array != pb.array) return true;
        if (pa.index != pb.index) return false;
    }
    // One target is a prefix of (or equal to) the other: the prefix overlaps
    // the whole sub-place.
    return true;
}

/** Does this type (transitively, through variant/union payloads) hold a reference? */
bool contains_reference(const MIRModule& module, TypeId type, uint32_t depth = 0) {
    if (type == MIR_INVALID_ID || type >= module.types.types.size() ||
        depth > 6) {
        return false;
    }
    const MIRTypeRecord& rec = module.types.types[type];
    if (rec.kind == MIRTypeKind::Reference) return true;
    if (rec.kind != MIRTypeKind::Struct && rec.kind != MIRTypeKind::Union &&
        rec.kind != MIRTypeKind::Variant && rec.kind != MIRTypeKind::Array) {
        return false;
    }
    if (rec.kind == MIRTypeKind::Array) {
        return contains_reference(module, rec.element, depth + 1);
    }
    for (uint32_t i = 0; i < rec.data_count; ++i) {
        const MIRFieldInfo* field = module.types.field_of(type, i);
        if (field == nullptr) continue;
        if (contains_reference(module, field->type, depth + 1)) return true;
    }
    return false;
}

/** A destructor-bearing aggregate that stores a reference (dropck-relevant). */
bool observes_borrow(const MIRModule& module, TypeId type) {
    if (type == MIR_INVALID_ID || type >= module.types.types.size()) {
        return false;
    }
    const MIRTypeRecord& rec = module.types.types[type];
    const bool drop_glue =
        (rec.flags & (TF_HAS_DESTRUCTOR | TF_USER_DESTRUCTOR)) != 0;
    return drop_glue && contains_reference(module, type);
}

/**
 * A type that borrows storage through a raw pointer instead of a reference:
 * `std::string_view`, `std::span`, `std::Chunk`. These types deliberately do
 * not expose a reference field (they are C-compatible pointer+length pairs),
 * so the ordinary reference loan tracking would let them outlive their source.
 */
bool is_borrow_view_type(const MIRModule& module, TypeId type) {
    if (type == MIR_INVALID_ID || type >= module.types.types.size()) {
        return false;
    }
    const MIRTypeRecord& rec = module.types.types[type];
    if (rec.kind != MIRTypeKind::Struct && rec.kind != MIRTypeKind::Union) {
        return false;
    }
    const std::string_view name = module.types.name_view(type);
    if (name.empty()) return false;
    // MIR names are the emitted C names (`std_stdstring_view`,
    // `std_stdspan__cgs__0`), so match the type token as a substring.
    return name.find("string_view") != std::string_view::npos ||
           name.find("span") != std::string_view::npos ||
           name.find("Chunk") != std::string_view::npos;
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
    // Addresses/places that project into a packed aggregate. A safe reference
    // to one is rejected (SAFE-VALID-008): its ABI alignment is 1, so the
    // referent does not satisfy the reference type's alignment requirement.
    std::unordered_set<uint32_t> packed_places;
    std::unordered_set<uint32_t> packed_addresses;
    std::unordered_map<ValueId, uint32_t> value_loans;
    std::unordered_map<PlaceId, uint32_t> place_loans;
    // Ambiguous reference results may alias any of these loans.
    std::unordered_map<ValueId, std::vector<uint32_t>> value_aliases;
    std::unordered_map<PlaceId, std::vector<uint32_t>> place_aliases;
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

    // S4.2b: an aggregate parameter's direct reference fields carry a declared
    // region from the parameter type's lifetime arguments. Seed them as
    // caller-owned loans so `return p.field` is checked against the function's
    // declared return lifetime. `escaped` keeps the local-storage checks from
    // treating caller-owned storage as a local.
    MIRRegionSignature fn_region_sig;
    const bool has_fn_region_sig =
        function.region_signature != MIR_INVALID_ID &&
        module.region_signature_at(function.region_signature, fn_region_sig);
    if (has_fn_region_sig) {
        const MIRRegionSignature& param_sig = fn_region_sig;
        for (uint32_t p = 0; p < function.places.size(); ++p) {
            const MIRPlaceDef& pd = function.places[p];
            if (pd.storage_class != static_cast<uint8_t>(MIRStorageClass::Parameter)) {
                continue;
            }
            if (pd.param_index == MIR_INVALID_ID ||
                pd.param_index >= param_sig.param_field_regions.size()) {
                continue;
            }
            for (const MIRParamFieldRegion& fr :
                 param_sig.param_field_regions[pd.param_index]) {
                if (fr.field_name == MIR_INVALID_ID) continue;
                const uint64_t key = field_key(p, fr.field_name);
                if (field_loans.find(key) != field_loans.end()) continue;
                Loan loan;
                loan.target.root = p;
                loan.target.path = pd.move_path_id;
                loan.target.param_index = pd.param_index;
                loan.target.known = true;
                loan.start = 0;
                loan.end = 0;
                loan.escaped = true;
                loan.region = fr.region;
                const uint32_t idx = static_cast<uint32_t>(loans.size());
                loans.push_back(std::move(loan));
                field_loans[key] = idx;
                field_params[key] = pd.param_index;
            }
        }
    }

    // A borrowing-view parameter (`string_view`, `span`, `Chunk`) borrows
    // caller storage. Seed a caller-owned loan so a local copy or a derived
    // view (`var p = path; p.subview(...)`) keeps the caller's provenance
    // instead of looking like a view of the parameter slot itself.
    for (uint32_t p = 0; p < function.places.size(); ++p) {
        const MIRPlaceDef& pd = function.places[p];
        if (pd.storage_class != static_cast<uint8_t>(MIRStorageClass::Parameter)) {
            continue;
        }
        if (!is_borrow_view_type(module, pd.type)) continue;
        if (place_loans.find(p) != place_loans.end()) continue;
        Loan loan;
        loan.target.root = p;
        loan.target.path = pd.move_path_id;
        loan.target.param_index = pd.param_index;
        loan.target.known = true;
        loan.escaped = true;
        const uint32_t idx = static_cast<uint32_t>(loans.size());
        loans.push_back(std::move(loan));
        place_loans[p] = idx;
        if (pd.param_index != MIR_INVALID_ID) {
            place_params[p] = pd.param_index;
        }
    }

    // A coroutine poll/resume function receives references through frame
    // fields that were created before the suspension. Treat each reference
    // frame field as a live loan rooted at the frame pointer so a move/drop of
    // another frame field while it is live is rejected (S4.1).

    PlaceId frame_root = MIR_INVALID_ID;
    for (uint32_t p = 0; p < function.places.size(); ++p) {
        if (function.places[p].storage_class ==
            static_cast<uint8_t>(MIRStorageClass::FramePtr)) {
            frame_root = p;
            break;
        }
    }
    std::unordered_map<ConstantId, PlaceId> frame_field_places;
    if (frame_root != MIR_INVALID_ID) {
        for (uint32_t p = 0; p < function.places.size(); ++p) {
            const MIRPlaceDef& pd = function.places[p];
            if (pd.storage_class != static_cast<uint8_t>(MIRStorageClass::FrameField)) {
                continue;
            }
            if (pd.frame_field != MIR_INVALID_ID) {
                frame_field_places[pd.frame_field] = p;
            }
            if (pd.type >= module.types.types.size()) continue;
            const MIRTypeKind kind = module.types.types[pd.type].kind;
            if (kind != MIRTypeKind::Reference && kind != MIRTypeKind::Pointer) {
                continue;
            }
            Loan loan;
            loan.target.root = frame_root;
            loan.target.known = true;
            loan.frame_loan = true;
            loan.frame_field_place = p;
            const uint32_t idx = static_cast<uint32_t>(loans.size());
            loans.push_back(std::move(loan));
            place_loans[p] = idx;

        }
    }

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
        const auto aliases = value_aliases.find(op.id);
        if (aliases != value_aliases.end()) {
            for (uint32_t alias : aliases->second) {
                if (alias >= loans.size()) continue;
                loans[alias].end = lin;
                if (may_escape) loans[alias].escaped = true;
            }
        }
    };
    // Follow a raw pointer value to the known call that produced it. A
    // pointer-returning method with one reference argument (`string.data()`,
    // `vector.get_ptr()`) aliases that argument, so a view built from the
    // pointer retains the argument's loan.
    auto pointer_value_source = [&](ValueId vid, uint32_t& loan_out,
                                    uint32_t& source_out) -> bool {
        uint32_t v = vid;
        for (uint32_t depth = 0; depth < 4; ++depth) {
            if (v >= function.values.size()) return false;
            const uint32_t di = function.values[v].def_inst;
            if (di >= function.instructions.size()) return false;
            const MIRInstruction& def = function.instructions[di];
            const MIROperand* dops = def.operand_count == 0
                                         ? nullptr
                                         : &function.operands[def.operand_offset];
            if (def.opcode() == MIROpcode::Cast && dops != nullptr &&
                dops[0].kind() == MIROperandKind::Value) {
                v = dops[0].id;
                continue;
            }
            if (def.opcode() != MIROpcode::Call || dops == nullptr ||
                def.operand_count < 2 || dops[0].kind() != MIROperandKind::Symbol) {
                return false;
            }
            uint32_t found = MIR_INVALID_ID;
            for (uint16_t i = 1; i < def.operand_count; ++i) {
                if (dops[i].kind() != MIROperandKind::Value) continue;
                const auto it = value_loans.find(dops[i].id);
                if (it == value_loans.end() || it->second >= loans.size()) continue;
                if (found != MIR_INVALID_ID) return false; // ambiguous
                found = it->second;
                const auto vp = value_params.find(dops[i].id);
                source_out = vp != value_params.end()
                                 ? vp->second
                                 : loans[found].target.param_index;
            }
            if (found == MIR_INVALID_ID) return false;
            loan_out = found;
            return true;
        }
        return false;
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
                const PlaceId address_place = ops[0].id;
                t.root = address_place;
                t.path = function.places[t.root].move_path_id;
                t.param_index = function.places[t.root].param_index;
                t.known = true;
                t.projections.clear();
                // A borrow of a borrowing-view temporary is transparent: the
                // temporary itself owns no storage, so the loan must target the
                // view's source (`to_view().trim()` returns the original
                // string's loan, not the temporary's).
                if (is_borrow_view_type(module, function.places[t.root].type)) {
                    const auto view_loan = place_loans.find(address_place);
                    if (view_loan != place_loans.end() &&
                        view_loan->second < loans.size()) {
                        t = loans[view_loan->second].target;
                    }
                }
                if (inst.result_or_place != MIR_INVALID_ID) {
                    address_targets[inst.result_or_place] = t;
                    if (packed_places.count(address_place) != 0) {
                        packed_addresses.insert(inst.result_or_place);
                    }
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
                // A pointer/reference base addresses the pointee aggregate;
                // resolve field names and indices against the pointee type.
                if (aggregate != MIR_INVALID_ID && aggregate < module.types.types.size()) {
                    const MIRTypeRecord& ar = module.types.types[aggregate];
                    if (ar.kind == MIRTypeKind::Pointer ||
                        ar.kind == MIRTypeKind::Reference) {
                        aggregate = ar.element;
                    }
                }
                // The projection can arrive as a Place (a selected place)
                // or as an address-producing Value (FieldAddr/IndexAddr). Keep
                // both sets in sync: a packed array field reaches IndexAddr as
                // a Value, and its element reference must still be rejected.
                bool packed_projection =
                    packed_places.count(ops[0].id) != 0 ||
                    packed_addresses.count(ops[0].id) != 0;
                if (aggregate != MIR_INVALID_ID && aggregate < module.types.types.size() &&
                    module.types.types[aggregate].packed) {
                    packed_projection = true;
                }
                LoanTarget projected = base;
                projected.path = project_path(
                    module, function, base.path, aggregate, ops[1]
                );
                if (inst.opcode() == MIROpcode::FieldAddr) {
                    uint32_t field_index = MIR_INVALID_ID;
                    if (ops[1].kind() == MIROperandKind::Constant) {
                        field_index = field_index_for_constant(
                            module, module.types, aggregate, ops[1].id);
                    }
                    projected.projections.push_back({field_index, false});
                } else {
                    uint64_t index = 0;
                    if (ops[1].kind() == MIROperandKind::Value &&
                        constant_index(module, ops[1].id, function, index)) {
                        projected.projections.push_back(
                            {static_cast<uint32_t>(index), true});
                    } else {
                        projected.projections.push_back({MIR_INVALID_ID, true});
                    }
                }
                if (inst.result_or_place != MIR_INVALID_ID) {
                    address_targets[inst.result_or_place] = projected;
                    if (packed_projection) packed_places.insert(inst.result_or_place);
                }
                break;
            }
            case MIROpcode::Borrow:
            case MIROpcode::BorrowMut: {
                if (ops == nullptr || inst.operand_count == 0) break;
                const bool packed_borrow =
                    packed_addresses.count(ops[0].id) != 0 ||
                    packed_places.count(ops[0].id) != 0;
                if (packed_borrow) {
                    report(result.diagnostics, function, function_index, order, lin,
                           "SAFE-VALID-008",
                           "cannot create a reference to a field of a packed aggregate");
                }
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
                    value_aliases.erase(inst.result_or_place);
                }
                break;
            }
            case MIROpcode::Param: {
                if (frame_root != MIR_INVALID_ID &&
                    inst.result_or_place != MIR_INVALID_ID &&
                    inst.result_or_place < function.values.size() &&
                    frame_root < function.places.size() &&
                    function.values[inst.result_or_place].type ==
                        function.places[frame_root].type) {
                    LoanTarget t;
                    t.root = frame_root;
                    t.path = function.places[frame_root].move_path_id;
                    t.known = true;
                    address_targets[inst.result_or_place] = t;
                }
                break;
            }
            case MIROpcode::GlobalAddr:
            case MIROpcode::FunctionAddr: {
                if (inst.result_or_place != MIR_INVALID_ID) {
                    LoanTarget t;
                    // A global/function address has no frame place. Use the
                    // places-size sentinel so the multi-borrow contract can
                    // classify it as a static referent.
                    t.root = static_cast<PlaceId>(function.places.size());
                    t.known = true;
                    address_targets[inst.result_or_place] = t;
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
                    const bool is_frame_ptr =
                        pd.storage_class ==
                        static_cast<uint8_t>(MIRStorageClass::FramePtr);
                    uint32_t pidx = pd.param_index;
                    if (pidx == MIR_INVALID_ID) {
                        const auto pp = place_params.find(ops[0].id);
                        if (pp != place_params.end()) pidx = pp->second;
                    }
                    const bool is_ref_param =
                        pidx != MIR_INVALID_ID &&
                        pd.type < module.types.types.size() &&
                        (module.types.types[pd.type].kind == MIRTypeKind::Reference ||
                         module.types.types[pd.type].kind == MIRTypeKind::Pointer);
                    if (is_ref_param || is_frame_ptr) {
                        if (is_ref_param) {
                            value_params[inst.result_or_place] = pidx;
                        }
                        // The loaded value is the parameter's referent pointer.
                        // Remember which parameter place it came from so a
                        // field load through it (`self.field`) can resolve the
                        // S4.2b field facts. The coroutine frame pointer is
                        // tracked the same way for the S4.1 frame checks.
                        LoanTarget t;
                        t.root = ops[0].id;
                        t.path = pd.move_path_id;
                        t.param_index = pidx;
                        t.known = true;
                        address_targets[inst.result_or_place] = t;
                    }
                }
                // Loading a reference local yields another value carrying its loan.
                if (ops[0].kind() == MIROperandKind::Place) {
                    const auto it = place_loans.find(ops[0].id);
                    if (it != place_loans.end() && inst.result_or_place != MIR_INVALID_ID) {
                        value_loans[inst.result_or_place] = it->second;
                        loans[it->second].end = lin;
                    }
                    const auto pa = place_aliases.find(ops[0].id);
                    if (pa != place_aliases.end()) {
                        value_aliases[inst.result_or_place] = pa->second;
                    }
                }
                // A reference stored in a struct field, either through a place
                // base (`p.field`) or a pointer/reference base (`self.field`
                // after loading a reference parameter).
                if (inst.operand_count >= 2 &&
                    ops[1].kind() == MIROperandKind::Constant &&
                    inst.result_or_place != MIR_INVALID_ID) {
                    uint64_t fkey = 0;
                    bool have_key = false;
                    if (ops[0].kind() == MIROperandKind::Place) {
                        fkey = field_key(ops[0].id, ops[1].id);
                        have_key = true;
                    } else if (ops[0].kind() == MIROperandKind::Value) {
                        const auto at = address_targets.find(ops[0].id);

                        if (at != address_targets.end() && at->second.known) {
                            fkey = field_key(at->second.root, ops[1].id);
                            have_key = true;
                            if (at->second.root < function.places.size() &&
                                function.places[at->second.root].storage_class ==
                                    static_cast<uint8_t>(MIRStorageClass::FramePtr)) {

                                const auto fp = frame_field_places.find(ops[1].id);
                                if (fp != frame_field_places.end()) {
                                    const auto pl = place_loans.find(fp->second);
                                    if (pl != place_loans.end()) {
                                        value_loans[inst.result_or_place] = pl->second;
                                        loans[pl->second].end = lin;

                                    }
                                }
                            }
                        }
                    }
                    if (have_key) {
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
                // The coroutine frame pointer is spilled to its FramePtr place
                // in the prologue; remember the SSA value so field loads through
                // it resolve to the frame (S4.1).
                if (ops[0].kind() == MIROperandKind::Place &&
                    value.kind() == MIROperandKind::Value &&
                    ops[0].id < function.places.size() &&
                    function.places[ops[0].id].storage_class ==
                        static_cast<uint8_t>(MIRStorageClass::FramePtr)) {
                    LoanTarget t;
                    t.root = ops[0].id;
                    t.path = function.places[ops[0].id].move_path_id;
                    t.known = true;
                    address_targets[value.id] = t;
                }
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
                        if (loan == MIR_INVALID_ID) {
                            // A reference loaded from a parameter/global has an
                            // address target, not a pre-existing value loan.
                            // Materialize the caller-owned loan now so storing
                            // it into an aggregate field is tracked.
                            const auto at = address_targets.find(value.id);
                            if (at != address_targets.end() && at->second.known) {
                                Loan source;
                                source.target = at->second;
                                source.start = lin;
                                source.end = lin;
                                source.escaped = true;
                                loan = static_cast<uint32_t>(loans.size());
                                loans.push_back(std::move(source));
                                if (param == MIR_INVALID_ID) {
                                    param = at->second.param_index;
                                }
                            }
                        }
                    } else if (value.kind() == MIROperandKind::Place) {
                        const auto vit = place_loans.find(value.id);
                        if (vit != place_loans.end()) loan = vit->second;
                        const auto pp = place_params.find(value.id);
                        if (pp != place_params.end()) param = pp->second;
                        if (loan == MIR_INVALID_ID && value.id < function.places.size()) {
                            const MIRPlaceDef& vp = function.places[value.id];
                            if ((vp.storage_class ==
                                     static_cast<uint8_t>(MIRStorageClass::Parameter) ||
                                 vp.storage_class ==
                                     static_cast<uint8_t>(MIRStorageClass::Global)) &&
                                vp.type < module.types.types.size() &&
                                (module.types.types[vp.type].kind ==
                                     MIRTypeKind::Reference ||
                                 module.types.types[vp.type].kind ==
                                     MIRTypeKind::Pointer)) {
                                Loan source;
                                source.target.root = value.id;
                                source.target.path = vp.move_path_id;
                                source.target.param_index = vp.param_index;
                                source.target.known = true;
                                source.start = lin;
                                source.end = lin;
                                source.escaped = true;
                                loan = static_cast<uint32_t>(loans.size());
                                loans.push_back(std::move(source));
                                if (param == MIR_INVALID_ID) param = vp.param_index;
                            }
                        }
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
                        // A whole-place source is a move: carry the reference
                        // loan/parameter facts to the destination.
                        {
                            const auto lit = place_loans.find(ops[1].id);
                            if (lit != place_loans.end()) {
                                place_loans[ops[0].id] = lit->second;
                                loans[lit->second].end = lin;
                                place_loans.erase(ops[1].id);
                            } else {
                                place_loans.erase(ops[0].id);
                            }
                            const auto pit = place_params.find(ops[1].id);
                            if (pit != place_params.end()) {
                                place_params[ops[0].id] = pit->second;
                                place_params.erase(ops[1].id);
                            } else {
                                place_params.erase(ops[0].id);
                            }
                            const auto pal = place_aliases.find(ops[1].id);
                            if (pal != place_aliases.end()) {
                                place_aliases[ops[0].id] = pal->second;
                                place_aliases.erase(ops[1].id);
                            } else {
                                place_aliases.erase(ops[0].id);
                            }
                        }
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
                        const auto va = value_aliases.find(value.id);
                        if (va != value_aliases.end()) {
                            place_aliases[ops[0].id] = va->second;
                        } else {
                            place_aliases.erase(ops[0].id);
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
                const bool copying =
                    inst.opcode() == MIROpcode::CopyInit ||
                    (ops[1].id < function.places.size() &&
                     mir_type_is_copy(module.types, function.places[ops[1].id].type));
                {
                    const auto pp = place_params.find(ops[1].id);
                    if (pp != place_params.end()) {
                        place_params[ops[0].id] = pp->second;
                        if (!copying) {
                            place_params.erase(ops[1].id);
                        }
                    } else {
                        place_params.erase(ops[0].id);
                    }
                    const auto pa = place_aliases.find(ops[1].id);
                    if (pa != place_aliases.end()) {
                        place_aliases[ops[0].id] = pa->second;
                        if (!copying) {
                            place_aliases.erase(ops[1].id);
                        }
                    } else {
                        place_aliases.erase(ops[0].id);
                    }
                }
                const auto it = place_loans.find(ops[1].id);
                if (it != place_loans.end()) {
                    place_loans[ops[0].id] = it->second;
                    // A copy keeps the source loan live; a move transfers it.
                    if (!copying) {
                        loans[it->second].end = lin;
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
                        if (!copying) loans[m.second].end = lin;
                    }
                    for (const auto& m : moved_params) {
                        field_params[m.first] = m.second;
                    }
                    if (!copying) {
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
                    const auto va = value_aliases.find(ops[1].id);
                    if (va != value_aliases.end()) {
                        place_aliases[ops[0].id] = va->second;
                    } else {
                        place_aliases.erase(ops[0].id);
                    }
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
                    const auto pa = place_aliases.find(ops[0].id);
                    if (pa != place_aliases.end()) {
                        value_aliases[inst.result_or_place] = pa->second;
                    }
                }
                if (ops[0].kind() == MIROperandKind::Value &&
                    inst.result_or_place != MIR_INVALID_ID) {
                    const auto va = value_aliases.find(ops[0].id);
                    if (va != value_aliases.end()) {
                        value_aliases[inst.result_or_place] = va->second;
                    }
                }
                break;
            }
            case MIROpcode::Cast: {
                if (ops == nullptr || inst.operand_count == 0) break;
                use_value(ops[0], lin, false);
                if (ops[0].kind() == MIROperandKind::Value &&
                    inst.result_or_place != MIR_INVALID_ID) {
                    const auto at = address_targets.find(ops[0].id);
                    if (at != address_targets.end()) {
                        address_targets[inst.result_or_place] = at->second;
                    }
                    const auto it = value_loans.find(ops[0].id);
                    if (it != value_loans.end()) {
                        value_loans[inst.result_or_place] = it->second;
                    }
                    const auto vp = value_params.find(ops[0].id);
                    if (vp != value_params.end()) {
                        value_params[inst.result_or_place] = vp->second;
                    }
                    const auto va = value_aliases.find(ops[0].id);
                    if (va != value_aliases.end()) {
                        value_aliases[inst.result_or_place] = va->second;
                    }
                }
                break;
            }
            case MIROpcode::Init: {
                if (ops == nullptr || inst.operand_count < 2) break;
                if (ops[0].kind() != MIROperandKind::Place ||
                    ops[1].kind() != MIROperandKind::Symbol ||
                    ops[0].id >= function.places.size()) {
                    break;
                }
                const TypeId dest_type = function.places[ops[0].id].type;
                if (!is_borrow_view_type(module, dest_type)) break;
                uint32_t view_loan = MIR_INVALID_ID;
                uint32_t view_source = MIR_INVALID_ID;
                for (uint16_t i = 2; i < inst.operand_count; ++i) {
                    if (ops[i].kind() != MIROperandKind::Value) continue;
                    const auto vl = value_loans.find(ops[i].id);
                    if (vl != value_loans.end() && vl->second < loans.size()) {
                        view_loan = vl->second;
                        const auto vp = value_params.find(ops[i].id);
                        if (vp != value_params.end()) view_source = vp->second;
                        else view_source = loans[view_loan].target.param_index;
                        break;
                    }
                }
                if (view_loan == MIR_INVALID_ID) {
                    for (uint16_t i = 2; i < inst.operand_count; ++i) {
                        if (ops[i].kind() != MIROperandKind::Value) continue;
                        const auto at = address_targets.find(ops[i].id);
                        if (at != address_targets.end() && at->second.known) {
                            Loan source;
                            source.target = at->second;
                            source.start = lin;
                            source.end = lin;
                            view_loan = static_cast<uint32_t>(loans.size());
                            view_source = at->second.param_index;
                            loans.push_back(std::move(source));
                            break;
                        }
                        uint32_t source_loan = MIR_INVALID_ID;
                        uint32_t source_param = MIR_INVALID_ID;
                        if (pointer_value_source(ops[i].id, source_loan, source_param)) {
                            view_loan = source_loan;
                            view_source = source_param;
                            break;
                        }
                    }
                }
                if (view_loan != MIR_INVALID_ID) {
                    place_loans[ops[0].id] = view_loan;
                    loans[view_loan].end = lin;
                    if (view_source != MIR_INVALID_ID) {
                        place_params[ops[0].id] = view_source;
                    }
                }
                break;
            }
            case MIROpcode::Call:
            case MIROpcode::CallIndirect: {
                if (ops == nullptr) break;
                bool has_callsite_sig = false;
                MIRRegionSignature callsite_sig;
                if (inst.opcode() == MIROpcode::CallIndirect) {
                    const uint32_t sid =
                        function.call_region_signature_at(ref.inst);
                    if (sid != MIR_INVALID_ID &&
                        module.region_signature_at(sid, callsite_sig)) {
                        has_callsite_sig = true;
                    }
                }
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
                bool returns_view = false;
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
                            // An sret aggregate return has a `void` function
                            // type; the view fact is on the result place/value.
                            TypeId result_type = ret;
                            if (inst.result_or_place != MIR_INVALID_ID) {
                                if (inst.result_or_place < function.places.size()) {
                                    result_type = function.places[inst.result_or_place].type;
                                } else if (inst.result_or_place < function.values.size()) {
                                    result_type = function.values[inst.result_or_place].type;
                                }
                            }
                            returns_view = is_borrow_view_type(module, result_type);
                        }
                    }
                }
                if (has_callsite_sig) {
                    // The erased function-pointer type cannot carry lifetimes,
                    // but the lowering recorded the source function type's
                    // HRTB contract for this instruction. It is a callable
                    // contract, not a body: the region mapping below uses it to
                    // bind the result loan to the borrowed argument.
                    known = true;
                    TypeId result_type = MIR_INVALID_ID;
                    if (inst.result_or_place != MIR_INVALID_ID) {
                        if (inst.result_or_place < function.values.size()) {
                            result_type = function.values[inst.result_or_place].type;
                        } else if (inst.result_or_place < function.places.size()) {
                            result_type = function.places[inst.result_or_place].type;
                        }
                    }
                    returns_reference =
                        result_type != MIR_INVALID_ID &&
                        result_type < module.types.types.size() &&
                        module.types.types[result_type].kind ==
                            MIRTypeKind::Reference;
                    returns_view = is_borrow_view_type(module, result_type);
                }
                struct RefArg {
                    uint16_t op_index = 0;
                    uint32_t param_index = MIR_INVALID_ID;
                    uint32_t loan = MIR_INVALID_ID;
                    uint32_t source_param = MIR_INVALID_ID;
                    MIRRegionId region = MIR_INVALID_ID;
                    bool from_field = false;
                };
                std::vector<RefArg> ref_args;
                for (uint16_t i = first; i < inst.operand_count; ++i) {
                    uint32_t loan = MIR_INVALID_ID;
                    uint32_t source_param = MIR_INVALID_ID;
                    if (ops[i].kind() == MIROperandKind::Value) {
                        // `&global` / a function address is a static referent.
                        // Do not trust a pre-existing unknown-target value loan
                        // for it; materialize a known global-target loan.
                        bool global_addr = false;
                        if (ops[i].id < function.values.size()) {
                            const uint32_t def = function.values[ops[i].id].def_inst;
                            if (def < function.instructions.size()) {
                                const MIROpcode dop = function.instructions[def].opcode();
                                global_addr = dop == MIROpcode::GlobalAddr ||
                                              dop == MIROpcode::FunctionAddr;
                            }
                        }
                        if (global_addr && has_callsite_sig) {
                            Loan source;
                            source.target.root =
                                static_cast<PlaceId>(function.places.size());
                            source.target.known = true;
                            source.start = lin;
                            source.end = lin;
                            source.escaped = true;
                            loan = static_cast<uint32_t>(loans.size());
                            loans.push_back(std::move(source));
                        } else {
                            const auto at = address_targets.find(ops[i].id);
                            if (at != address_targets.end() && at->second.known &&
                                at->second.root >= function.places.size()) {
                                Loan source;
                                source.target = at->second;
                                source.start = lin;
                                source.end = lin;
                                source.escaped = true;
                                loan = static_cast<uint32_t>(loans.size());
                                loans.push_back(std::move(source));
                            } else {
                                const auto it = value_loans.find(ops[i].id);
                                if (it != value_loans.end() && it->second < loans.size()) {
                                    loan = it->second;
                                }
                                const auto vp = value_params.find(ops[i].id);
                                if (vp != value_params.end()) source_param = vp->second;
                            }
                        }
                    } else if (ops[i].kind() == MIROperandKind::Place &&
                               has_callsite_sig) {
                        // Place-formed reference arguments are needed by the
                        // HRTB callsite contract; ordinary calls keep the
                        // pre-existing value-only behavior so moving a local
                        // view into a normal call still ends its loan.
                        const auto it = place_loans.find(ops[i].id);
                        if (it != place_loans.end() && it->second < loans.size()) {
                            loan = it->second;
                        }
                        const auto pp = place_params.find(ops[i].id);
                        if (pp != place_params.end()) source_param = pp->second;
                    }
                    if (loan == MIR_INVALID_ID && has_callsite_sig &&
                        ops[i].kind() == MIROperandKind::Value) {
                        // A direct reference parameter is loaded from its
                        // parameter slot; the load carries an address target
                        // rather than a pre-existing place loan. Materialize a
                        // caller-owned loan for the call contract.
                        const auto at = address_targets.find(ops[i].id);
                        if (at != address_targets.end() && at->second.known) {
                            Loan source;
                            source.target = at->second;
                            source.start = lin;
                            source.end = lin;
                            source.escaped = true;
                            loan = static_cast<uint32_t>(loans.size());
                            loans.push_back(std::move(source));
                            source_param = at->second.param_index;
                        } else if (ops[i].id < function.values.size()) {
                            // `&global` / a function address is a static
                            // referent. It has no frame root, so mark a known
                            // target beyond the place table; the multi-borrow
                            // check treats that as the global/static case.
                            const uint32_t def = function.values[ops[i].id].def_inst;
                            if (def < function.instructions.size()) {
                                const MIROpcode dop = function.instructions[def].opcode();
                                if (dop == MIROpcode::GlobalAddr ||
                                    dop == MIROpcode::FunctionAddr) {
                                    Loan source;
                                    source.target.root =
                                        static_cast<PlaceId>(function.places.size());
                                    source.target.known = true;
                                    source.start = lin;
                                    source.end = lin;
                                    source.escaped = true;
                                    loan = static_cast<uint32_t>(loans.size());
                                    loans.push_back(std::move(source));
                                }
                            }
                        }
                    }
                    if (loan == MIR_INVALID_ID) continue;
                    RefArg ra;
                    ra.op_index = i;
                    ra.param_index = static_cast<uint32_t>(i - first);
                    ra.loan = loan;
                    ra.source_param = source_param != MIR_INVALID_ID
                                          ? source_param
                                          : loans[loan].target.param_index;
                    ref_args.push_back(ra);
                }
                const MIRRegionSignature* callee_sig = nullptr;
                MIRRegionSignature callee_sig_copy;
                if (has_callsite_sig) {
                    callee_sig = &callsite_sig;
                } else if (known) {
                    const uint32_t sid =
                        module.symbol_region_signature(ops[0].id);
                    if (sid != MIR_INVALID_ID &&
                        module.region_signature_at(sid, callee_sig_copy)) {
                        callee_sig = &callee_sig_copy;
                    }
                }
                // `: 'static T` means the callee returns storage it owns (an
                // arena copy, a literal), so no argument loan is retained.
                const bool static_return =
                    callee_sig != nullptr &&
                    callee_sig->return_region == MIR_REGION_STATIC;

                // A by-value aggregate argument can carry direct reference
                // fields (S4.2b HRTB). The callsite contract names those fields;
                // translate each actual field loan into the same RefArg list so
                // multi-borrow pinning and result-loan mapping see them too.
                if (has_callsite_sig && callee_sig != nullptr) {
                    for (uint32_t pi = 0; pi < callee_sig->param_field_regions.size(); ++pi) {
                        const uint32_t op_index = first + pi;
                        if (op_index >= inst.operand_count) continue;
                        const MIROperand& arg = ops[op_index];
                        PlaceId field_base = MIR_INVALID_ID;
                        if (arg.kind() == MIROperandKind::Place) {
                            field_base = arg.id;
                        } else if (arg.kind() == MIROperandKind::Value) {
                            const auto at = address_targets.find(arg.id);
                            if (at != address_targets.end() && at->second.known) {
                                field_base = at->second.root;
                            }
                        }
                        if (field_base == MIR_INVALID_ID) continue;
                        for (const MIRParamFieldRegion& fr : callee_sig->param_field_regions[pi]) {
                            if (fr.field_name == MIR_INVALID_ID) continue;
                            const auto it = field_loans.find(field_key(field_base, fr.field_name));
                            if (it == field_loans.end() || it->second >= loans.size()) continue;
                            RefArg ra;
                            ra.op_index = static_cast<uint16_t>(op_index);
                            ra.param_index = pi;
                            ra.loan = it->second;
                            const auto fp = field_params.find(field_key(field_base, fr.field_name));
                            ra.source_param = fp != field_params.end()
                                                  ? fp->second
                                                  : loans[ra.loan].target.param_index;
                            ra.region = fr.region;
                            ra.from_field = true;
                            ref_args.push_back(ra);
                        }
                    }
                }

                // A multi-borrow HRTB callback may store one argument borrow
                // into another. Full region inference is not available, so
                // accept only local/temporary referents and keep every loan
                // live until function exit. A parameter/global referent could
                // outlive the retained borrow in the caller, so it is rejected
                // rather than guessed safe.
                bool pinned_multi = false;
                if (callee_sig != nullptr) {
                    if (has_callsite_sig) {
                        // A by-value aggregate operand can also carry a
                        // whole-value loan from the direct value scan. It is not
                        // a direct reference parameter; the field scan below
                        // contributes its reference fields instead.
                        ref_args.erase(
                            std::remove_if(ref_args.begin(), ref_args.end(),
                                           [&](const RefArg& ra) {
                                               return !ra.from_field &&
                                                      (ra.param_index >=
                                                           callee_sig->param_regions.size() ||
                                                       callee_sig->param_regions[ra.param_index] ==
                                                           MIR_INVALID_ID);
                                           }),
                            ref_args.end());
                    }
                    // Region ids are needed by ordinary multi-reference calls
                    // too (result-loan mapping), not only HRTB calls.
                    for (RefArg& ra : ref_args) {
                        if (ra.region == MIR_INVALID_ID &&
                            ra.param_index < callee_sig->param_regions.size()) {
                            ra.region = callee_sig->param_regions[ra.param_index];
                        }
                    }
                }
                uint32_t hrtb_ref_params = 0;
                if (callee_sig != nullptr) {
                    for (MIRRegionId pr : callee_sig->param_regions) {
                        if (pr != MIR_INVALID_ID) ++hrtb_ref_params;
                    }
                    for (const auto& fields : callee_sig->param_field_regions) {
                        hrtb_ref_params += static_cast<uint32_t>(fields.size());
                    }
                }
                if (has_callsite_sig && hrtb_ref_params > 1) {
                    bool all_local = ref_args.size() == hrtb_ref_params;
                    bool all_nonlocal_safe = ref_args.size() == hrtb_ref_params;
                    MIRRegionId nonlocal_region = MIR_INVALID_ID;
                    for (const RefArg& ra : ref_args) {
                        if (ra.loan >= loans.size()) {
                            all_local = false;
                            all_nonlocal_safe = false;
                            break;
                        }
                        const LoanTarget& target = loans[ra.loan].target;
                        if (!target.known) {
                            all_local = false;
                            all_nonlocal_safe = false;
                            break;
                        }
                        if (target.root >= function.places.size()) {
                            // Global/static referents carry a known target but no
                            // local frame place. They outlive the current call;
                            // treat them as the non-local case (still require a
                            // shared declared region below).
                            all_local = false;
                            if (ra.region == MIR_INVALID_ID) {
                                all_nonlocal_safe = false;
                            } else if (nonlocal_region == MIR_INVALID_ID) {
                                nonlocal_region = ra.region;
                            } else if (nonlocal_region != ra.region) {
                                all_nonlocal_safe = false;
                            }
                            continue;
                        }
                        const auto sc = static_cast<MIRStorageClass>(
                            function.places[target.root].storage_class);
                        const bool local = sc == MIRStorageClass::Local ||
                                           sc == MIRStorageClass::Temporary;
                        if (!local) all_local = false;
                        if (local || (sc != MIRStorageClass::Parameter &&
                                      sc != MIRStorageClass::Global)) {
                            all_nonlocal_safe = false;
                        } else {
                            // A callback can only retain a parameter/global
                            // borrow if every non-local input shares the same
                            // declared region (or is static). Different caller
                            // regions could otherwise be stored into a longer
                            // lived referent, which full region inference must
                            // prove and this local analysis cannot.
                            if (ra.region == MIR_INVALID_ID) {
                                all_nonlocal_safe = false;
                            } else if (nonlocal_region == MIR_INVALID_ID) {
                                nonlocal_region = ra.region;
                            } else if (nonlocal_region != ra.region) {
                                all_nonlocal_safe = false;
                            }
                        }
                    }
                    if (all_local || all_nonlocal_safe) {
                        pinned_multi = true;
                        const uint32_t last =
                            order.empty() ? 0 : static_cast<uint32_t>(order.size() - 1);
                        for (const RefArg& ra : ref_args) {
                            if (ra.loan >= loans.size()) continue;
                            loans[ra.loan].pinned_live = true;
                            loans[ra.loan].end = last;
                        }
                    } else {
                        report(result.diagnostics, function, function_index, order, lin,
                               "SAFE-BORROW-099",
                               "a multi-borrow higher-ranked callback requires either all local referents or parameter/global referents sharing one region; full region inference is not implemented");
                    }
                }

                // A reference result aliases the argument whose declared
                // region is the return region (or the unique argument whose
                // region outlives it). This makes `pick(&mut a, &mut b)` keep
                // the correct loan alive instead of dropping both.
                uint32_t mapped_loan = MIR_INVALID_ID;
                uint32_t mapped_source = MIR_INVALID_ID;
                std::vector<uint32_t> ambiguous_loans;
                if (known && (returns_reference || returns_view) &&
                    inst.result_or_place != MIR_INVALID_ID) {
                    if (callee_sig != nullptr &&
                        callee_sig->return_region != MIR_INVALID_ID) {
                        const MIRRegionId ret = callee_sig->return_region;
                        std::vector<const RefArg*> identity;
                        std::vector<const RefArg*> sources;
                        for (const RefArg& ra : ref_args) {
                            const MIRRegionId pr = ra.region;
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
                        } else if (!identity.empty() || !sources.empty()) {
                            // The result may alias any of these candidates.
                            const auto& candidates =
                                !identity.empty() ? identity : sources;
                            mapped_loan = candidates[0]->loan;
                            mapped_source = candidates[0]->source_param;
                            for (const RefArg* ra : candidates) {
                                ambiguous_loans.push_back(ra->loan);
                            }
                        } else if (!static_return && ref_args.size() == 1) {
                            // Elided region: the single reference is the only
                            // possible source. A `'static` result owns its
                            // storage, so it retains no argument loan.
                            mapped_loan = ref_args[0].loan;
                            mapped_source = ref_args[0].source_param;
                        }
                    } else if (!static_return && ref_args.size() == 1) {
                        mapped_loan = ref_args[0].loan;
                        mapped_source = ref_args[0].source_param;
                    }
                }

                // A destructor-bearing aggregate returned from a single
                // reference argument (a borrow guard) carries that loan in its
                // place, so drop order can be checked by SAFE-LIFE-002.
                if (known && ref_args.size() == 1 &&
                    inst.result_or_place != MIR_INVALID_ID &&
                    inst.result_or_place < function.places.size()) {
                    const TypeId result_type =
                        function.places[inst.result_or_place].type;
                    if (observes_borrow(module, result_type)) {
                        const uint32_t loan = ref_args[0].loan;
                        place_loans[inst.result_or_place] = loan;
                        loans[loan].end = lin;
                        if (ref_args[0].source_param != MIR_INVALID_ID) {
                            place_params[inst.result_or_place] =
                                ref_args[0].source_param;
                        }
                    }
                }

                // A borrowing view (`string_view`, `span`, `Chunk`) carries its
                // source as a raw pointer, so it has no reference field for the
                // ordinary tracking. Attach the source loan to the result place:
                // a reference argument (the `&self` of `to_view`) or an
                // `&raw`/address target (`span(&raw mut a[0], 3)`).
                if (known && returns_view && !static_return &&
                    inst.result_or_place != MIR_INVALID_ID &&
                    inst.result_or_place < function.places.size()) {
                    uint32_t view_loan = mapped_loan;
                    uint32_t view_source = mapped_source;
                    if (view_loan == MIR_INVALID_ID) {
                        for (uint16_t i = first; i < inst.operand_count; ++i) {
                            if (ops[i].kind() != MIROperandKind::Value) continue;
                            const auto at = address_targets.find(ops[i].id);
                            if (at != address_targets.end() && at->second.known) {
                                Loan source;
                                source.target = at->second;
                                source.start = lin;
                                source.end = lin;
                                view_loan = static_cast<uint32_t>(loans.size());
                                view_source = at->second.param_index;
                                loans.push_back(std::move(source));
                                break;
                            }
                            uint32_t source_loan = MIR_INVALID_ID;
                            uint32_t source_param = MIR_INVALID_ID;
                            if (pointer_value_source(ops[i].id, source_loan, source_param)) {
                                view_loan = source_loan;
                                view_source = source_param;
                                break;
                            }
                        }
                    }
                    if (view_loan != MIR_INVALID_ID) {
                        place_loans[inst.result_or_place] = view_loan;
                        loans[view_loan].end = lin;
                        if (view_source != MIR_INVALID_ID) {
                            place_params[inst.result_or_place] = view_source;
                        }
                        if (!ambiguous_loans.empty()) {
                            place_aliases[inst.result_or_place] = ambiguous_loans;
                        }
                    }
                }

                if (mapped_loan != MIR_INVALID_ID && returns_reference) {
                    for (uint16_t i = first; i < inst.operand_count; ++i) {
                        use_value(ops[i], lin, false);
                    }
                    value_loans[inst.result_or_place] = mapped_loan;
                    loans[mapped_loan].end = lin;
                    if (!ambiguous_loans.empty()) {
                        value_aliases[inst.result_or_place] = ambiguous_loans;
                    } else {
                        value_aliases.erase(inst.result_or_place);
                    }
                    if (mapped_source != MIR_INVALID_ID) {
                        value_params[inst.result_or_place] = mapped_source;
                    }
                } else if (known && (ref_args.size() <= 1 || pinned_multi)) {
                    // Known callee with no retained reference, or a multi-borrow
                    // HRTB call whose local referents are pinned until function
                    // exit. The ordinary loan ends at the call.
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
                // A borrowing view returned through the sret pointer carries
                // its source loan on the returned place (S4.2b view tracking).
                if (returned_place != MIR_INVALID_ID) {
                    if (loan == MIR_INVALID_ID) {
                        const auto it = place_loans.find(returned_place);
                        if (it != place_loans.end()) loan = it->second;
                    }
                    if (return_param == MIR_INVALID_ID) {
                        const auto it = place_params.find(returned_place);
                        if (it != place_params.end()) return_param = it->second;
                    }
                }

                const MIRRegionSignature* sig = nullptr;
                if (has_fn_region_sig) {
                    sig = &fn_region_sig;
                }
                bool reported = false;
                auto check_region = [&](MIRRegionId from) -> bool {
                    if (sig == nullptr || sig->return_region == MIR_INVALID_ID ||
                        from == MIR_INVALID_ID) {
                        return true;
                    }
                    if (region_outlives(*sig, from, sig->return_region)) {
                        return true;
                    }
                    report(result.diagnostics, function, function_index, order, lin,
                           "SAFE-LIFE-003",
                           "returned reference does not outlive the declared return lifetime (add a where 'a: 'b bound)");
                    return false;
                };
                auto check_param_region = [&](uint32_t param_index) -> bool {
                    if (param_index == MIR_INVALID_ID || sig == nullptr ||
                        sig->return_region == MIR_INVALID_ID ||
                        param_index >= sig->param_regions.size()) {
                        return true;
                    }
                    return check_region(sig->param_regions[param_index]);
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
                // A reference read from an aggregate parameter's field has its
                // own declared region (S4.2b); prefer it over the parameter's
                // single region.
                const bool has_field_region =
                    loan != MIR_INVALID_ID && loan < loans.size() &&
                    loans[loan].region != MIR_INVALID_ID;
                if (!reported && has_field_region) {
                    if (!check_region(loans[loan].region)) {
                        reported = true;
                    }
                }
                if (!reported && !has_field_region &&
                    !check_param_region(return_param)) {
                    reported = true;
                }
                // An ambiguous result may come from any candidate argument.
                if (!reported && !has_field_region && ops != nullptr &&
                    inst.operand_count > 0 &&
                    ops[0].kind() == MIROperandKind::Value) {
                    const auto al = value_aliases.find(ops[0].id);
                    if (al != value_aliases.end()) {
                        for (uint32_t alias : al->second) {
                            if (alias >= loans.size()) continue;
                            const uint32_t alias_param =
                                loans[alias].target.param_index;
                            if (alias_param != MIR_INVALID_ID &&
                                !check_param_region(alias_param)) {
                                reported = true;
                                break;
                            }
                        }
                    }
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
                        const auto lit = field_loans.find(fp.first);
                        if (lit != field_loans.end() && lit->second < loans.size() &&
                            loans[lit->second].region != MIR_INVALID_ID) {
                            if (!check_region(loans[lit->second].region)) {
                                reported = true;
                                break;
                            }
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
                const auto al = value_aliases.find(ops[i].id);
                if (al != value_aliases.end()) {
                    for (uint32_t alias : al->second) {
                        if (alias < nloans && !defined_in_block[alias]) {
                            block_use[b][alias] = true;
                        }
                    }
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
            // Taking the address of a reference field (`h.r`, `self.r`) reads
            // the loan stored in that field. The subsequent Load is through a
            // pointer value, so the place+constant Load case above does not see
            // it.
            if ((inst.opcode() == MIROpcode::FieldAddr ||
                 inst.opcode() == MIROpcode::IndexAddr) &&
                ops != nullptr && inst.operand_count >= 2 &&
                ops[1].kind() == MIROperandKind::Constant) {
                PlaceId root = MIR_INVALID_ID;
                if (ops[0].kind() == MIROperandKind::Place) {
                    root = ops[0].id;
                } else if (ops[0].kind() == MIROperandKind::Value) {
                    const auto at = address_targets.find(ops[0].id);
                    if (at != address_targets.end() && at->second.known) {
                        root = at->second.root;
                    }
                }
                if (root != MIR_INVALID_ID) {
                    const auto fit = field_loans.find(field_key(root, ops[1].id));
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

    // Proper DFS back-edge detection (an edge to a block on the current DFS
    // stack). Block ids alone are not layout/topological order, so an
    // `edge to a lower id` test produces false loops at merge blocks.
    bool has_back_edge = false;
    {
        std::vector<uint8_t> color(nblocks, 0); // 0 unvisited, 1 on stack, 2 done
        struct DfsFrame { uint32_t node; size_t next; };
        std::vector<DfsFrame> stack;
        for (uint32_t start = 0; start < nblocks && !has_back_edge; ++start) {
            if (color[start] != 0) continue;
            color[start] = 1;
            stack.push_back({start, 0});
            while (!stack.empty()) {
                DfsFrame& f = stack.back();
                const std::vector<BlockId>& out = successors[f.node];
                if (f.next >= out.size()) {
                    color[f.node] = 2;
                    stack.pop_back();
                    continue;
                }
                const BlockId succ = out[f.next++];
                if (succ >= nblocks) continue;
                if (color[succ] == 1) {
                    has_back_edge = true;
                    break;
                }
                if (color[succ] == 0) {
                    color[succ] = 1;
                    stack.push_back({succ, 0});
                }
            }
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
                    if ((!live[l] && !loans[l].pinned_live) || loans[l].escaped ||
                        !loans[l].target.known) continue;
                    if (loans[l].frame_loan) continue; // frame storage owns the view
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
                    if (ops != nullptr && ops[0].kind() == MIROperandKind::Place &&
                        !(ops[0].id < function.places.size() &&
                          mir_type_is_copy(module.types,
                                           function.places[ops[0].id].type))) {
                        moved_place = ops[0].id;
                    }
                    break;
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
                    if ((!live[l] && !loans[l].pinned_live) || loans[l].escaped ||
                        !loans[l].target.known) continue;
                    if (!has_back_edge && loans[l].end != MIR_INVALID_ID &&
                        loans[l].end < linear_of(b, ii, order)) {
                        continue;
                    }

                    bool overlap = targets_overlap(function, loans[l].target, moved);
                    if (!overlap && loans[l].frame_loan) {
                        const MIRPlaceDef& mp = function.places[moved_place];
                        if (mp.storage_class ==
                                static_cast<uint8_t>(MIRStorageClass::FrameField) &&
                            moved_place != loans[l].frame_field_place) {
                            overlap = true;
                        }
                    }
                    if (!overlap) continue;
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
                const auto al = value_aliases.find(ops[i].id);
                if (al != value_aliases.end()) {
                    for (uint32_t alias : al->second) {
                        if (alias < nloans) live[alias] = true;
                    }
                }
            }
            if ((inst.opcode() == MIROpcode::Drop ||
                 inst.opcode() == MIROpcode::Destroy) &&
                ops != nullptr && ops[0].kind() == MIROperandKind::Place) {
                const auto it = place_loans.find(ops[0].id);
                if (it != place_loans.end() && it->second < nloans) {
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
            // Address of a reference field: the field loan is live from here.
            if ((inst.opcode() == MIROpcode::FieldAddr ||
                 inst.opcode() == MIROpcode::IndexAddr) &&
                ops != nullptr && inst.operand_count >= 2 &&
                ops[1].kind() == MIROperandKind::Constant) {
                PlaceId root = MIR_INVALID_ID;
                if (ops[0].kind() == MIROperandKind::Place) {
                    root = ops[0].id;
                } else if (ops[0].kind() == MIROperandKind::Value) {
                    const auto at = address_targets.find(ops[0].id);
                    if (at != address_targets.end() && at->second.known) {
                        root = at->second.root;
                    }
                }
                if (root != MIR_INVALID_ID) {
                    const auto fit = field_loans.find(field_key(root, ops[1].id));
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
                        if (other == l || (!live[other] && !loans[other].pinned_live)) continue;
                        if (loans[other].escaped || !loans[other].target.known) continue;
                        // `live` is a CFG over-approximation; in an acyclic
                        // function a loan whose last use precedes this point is
                        // dead even if a stale `place_loans` entry marks it.
                        if (!has_back_edge && loans[other].end != MIR_INVALID_ID &&
                            loans[other].end < linear_of(b, ii, order)) {
                            continue;
                        }
                        if (!(loan.mutable_loan || loans[other].mutable_loan)) continue;
                        if (!borrow_targets_overlap(loan.target, loans[other].target)) {
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
