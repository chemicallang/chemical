// Copyright (c) Chemical Language Foundation 2025.

#include "MIRLowerer.h"
#include "MIRABI.h"
#include "MIRPrototype.h"

#include <cstdlib>
#include <iostream>
#include <optional>

#include "ast/base/BaseType.h"
#include "ast/types/GenericType.h"
#include "ast/types/DynamicType.h"
#include "ast/structures/InterfaceDefinition.h"
#include "ast/structures/ImplDefinition.h"
#include "ast/base/Value.h"
#include "ast/utils/Operation.h"
#include "ast/values/IntNumValue.h"
#include "ast/values/BoolValue.h"
#include "ast/values/FloatValue.h"
#include "ast/values/DoubleValue.h"
#include "ast/values/StringValue.h"
#include "ast/values/Negative.h"
#include "ast/values/NotValue.h"
#include "ast/values/BitwiseNot.h"
#include "ast/values/Expression.h"
#include "ast/values/CastedValue.h"
#include "ast/values/FunctionCall.h"
#include "ast/values/VariableIdentifier.h"
#include "ast/values/DereferenceValue.h"
#include "ast/values/IndexOperator.h"
#include "ast/values/AddrOfValue.h"
#include "ast/values/ReferenceOfValue.h"
#include "ast/values/ArrayValue.h"
#include "ast/values/StructValue.h"
#include "ast/values/RuntimeValue.h"
#include "ast/values/DynamicValue.h"
#include "ast/values/ComptimeValue.h"
#include "ast/structures/CapturedComptimeVariable.h"
#include "ast/values/StructMemberInitializer.h"
#include "ast/values/ValueNode.h"
#include "ast/values/AccessChain.h"
#include "ast/structures/FunctionDeclaration.h"
#include "ast/structures/FunctionParam.h"
#include "ast/structures/StructDefinition.h"
#include "ast/structures/VariantDefinition.h"
#include "ast/structures/VariantMember.h"
#include "ast/structures/UnionDef.h"
#include "ast/structures/Scope.h"
#include "ast/structures/BlockScope.h"
#include "ast/structures/UnsafeBlock.h"
#include "ast/structures/If.h"
#include "ast/structures/WhileLoop.h"
#include "ast/structures/ForLoop.h"
#include "ast/structures/EnumMember.h"
#include "ast/statements/VarInit.h"
#include "ast/statements/Return.h"
#include "ast/statements/Assignment.h"
#include "ast/statements/Break.h"
#include "ast/statements/Continue.h"
#include "ast/statements/IncDecNode.h"
#include "ast/statements/InlineAsmStmt.h"
#include "ast/statements/AccessChainNode.h"
#include "ast/statements/SwitchStatement.h"
#include "ast/values/IncDecValue.h"
#include "ast/statements/ValueWrapperNode.h"
#include "ast/values/SizeOfValue.h"
#include "ast/values/AlignOfValue.h"
#include "ast/values/OffsetOfValue.h"
#include "ast/values/UnsafeValue.h"
#include "ast/values/ZeroedValue.h"
#include "ast/values/IfValue.h"
#include "ast/values/IsValue.h"
#include "ast/values/VariantCase.h"
#include "ast/values/VariantCaseVariable.h"
#include "ast/values/LambdaFunction.h"
#include "ast/values/ExtractionValue.h"
#include "ast/values/NewTypedValue.h"
#include "ast/values/NewValue.h"
#include "ast/values/PlacementNewValue.h"
#include "ast/statements/DestructStmt.h"
#include "ast/statements/DeallocStmt.h"
#include "ast/statements/PlacementNewNode.h"
#include "ast/statements/EmbeddedNode.h"
#include "ast/values/EmbeddedValue.h"
#include "ast/types/CapturingFunctionType.h"
#include "ast/structures/CapturedVariable.h"
#include "ast/values/SwitchValue.h"
#include "ast/values/LoopValue.h"
#include "ast/values/PatternMatchExpr.h"
#include "ast/values/InValue.h"
#include "ast/values/RuntimeBlockValue.h"
#include "ast/values/BlockValue.h"
#include "ast/statements/PatternMatchExprNode.h"
#include "ast/statements/Return.h"
#include "ast/statements/Break.h"
#include "ast/statements/Continue.h"
#include "ast/values/AwaitExpression.h"
#include "compiler/async/AsyncCTypes.h"
#include "compiler/async/AwaitNormalizePass.h"
#include "ast/types/ArrayType.h"
#include "ast/structures/StructMember.h"
#include "ast/structures/DoWhileLoop.h"
#include "ast/structures/ForInLoop.h"
#include "ast/structures/InheritedType.h"
#include "ast/structures/BaseDefMember.h"
#include "ast/structures/UnionDef.h"
#include "ast/structures/UnnamedStruct.h"
#include "ast/structures/UnnamedUnion.h"
#include "ast/types/StructType.h"
#include "compiler/symres/ImplementationsIndex.h"
#include "compiler/symres/CoreNodes.h"
#include "MIREmitter.h"

namespace mir {
static bool mir_slot_is_destructible(BaseType* type) {
    if (type == nullptr) return false;
    if (type->get_destructor() != nullptr) return true;
    const auto canonical = type->canonical();
    if (canonical->kind() != BaseTypeKind::Array) return false;
    const auto arr = canonical->as_array_type_unsafe();
    if (!arr->has_array_size() || arr->elem_type == nullptr) return false;
    return arr->elem_type->canonical()->get_destructor() != nullptr;
}

std::string MIRLowerer::async_slot_field(unsigned id) const {
    return "__chx_slot_" + std::to_string(id);
}
std::string MIRLowerer::async_child_field(unsigned id) const {
    return "__chx_child_" + std::to_string(id);
}
std::string MIRLowerer::async_drop_flag_field(unsigned id) const {
    return "__chx_drop_" + std::to_string(id);
}
std::string MIRLowerer::async_resident_field(ASTNode* node) const {
    auto it = async_resident_.find(node);
    if (it == async_resident_.end()) return std::string();
    return async_slot_field(it->second);
}
PlaceId MIRLowerer::async_resident_place(ASTNode* node) const {
    auto it = var_places_.find(node);
    return it == var_places_.end() ? MIR_INVALID_ID : it->second;
}
void MIRLowerer::async_emit_spill(const AwaitSite&) {}
void MIRLowerer::async_emit_reload(const AwaitSite&) {}
void MIRLowerer::async_emit_pending_return() {}

static std::string mir_sym_name(const MIRModule& module, SymbolId sym) {
    if (sym == MIR_INVALID_ID || sym >= module.symbols.size()) return std::string();
    const MIRSymbolRecord& r = module.symbols.get(sym);
    return std::string(module.symbols.name_data(r), r.name_length);
}

bool MIRLowerer::lower_async_function(FunctionDeclaration* decl, MIRArena& arena,
                                      MIRFunction& func, std::string& error) {
    var_places_.clear();
    name_places_.clear();
    destructibles_.clear();
    call_temps_.clear();
    stmt_temps_.clear();
    pattern_bases_.clear();
    pattern_results_.clear();
    async_ = true;
    arena_ = &arena;
    async_pre_decls_.clear();
    async_post_decls_.clear();
    async_resident_.clear();
    async_drop_flag_slots_.clear();
    async_site_index_.clear();
    // hoist nested awaits into VarInitStatements before planning the frame
    if (async_normalize_) async_normalize_(decl);
    async_plan_ = build_async_plan(decl);
    const AsyncCTypes t = resolve_async_c_types(decl);
    if (!t.ok) {
        error = "async lowering could not resolve the core::async protocol types";
        async_ = false;
        return false;
    }
    async_inner_ = t.inner;
    async_handle_ = t.handle;
    async_table_ = t.table;
    async_poll_base_ = t.poll;
    async_context_ = t.context_ptr;

    async_mangled_.assign(mangle(decl));
    if (async_mangled_.empty()) {
        const chem::string_view nv = decl->name_view();
        async_mangled_.assign(nv.data(), nv.size());
    }
    async_frame_type_ = types_.named_struct(async_mangled_ + "__frame");
    async_frame_ptr_type_ = types_.pointer_type(async_frame_type_, true);

    // resident slots (parameters + locals that cross a suspension) and drop flags
    std::vector<bool> crosses(async_plan_.slots.size(), false);
    for (auto& site : async_plan_.sites) {
        for (auto id : site.live_slots) {
            if (id < crosses.size()) crosses[id] = true;
        }
    }
    for (size_t id = 0; id < async_plan_.slots.size(); ++id) {
        const auto& slot = async_plan_.slots[id];
        if (slot.node == nullptr) continue;
        const bool is_param = slot.node->kind() == ASTNodeKind::FunctionParam;
        if (!crosses[id] && !is_param) continue;
        if (slot.type == nullptr) continue;
        async_resident_[slot.node] = static_cast<unsigned>(id);
    }
    for (auto& site : async_plan_.sites) {
        for (auto id : site.live_drops) async_drop_flag_slots_.insert(id);
    }
    for (size_t id = 0; id < async_plan_.slots.size(); ++id) {
        const auto& slot = async_plan_.slots[id];
        if (slot.node != nullptr && slot.node->kind() == ASTNodeKind::FunctionParam &&
            async_resident_.count(slot.node) != 0 && slot.destructible) {
            async_drop_flag_slots_.insert(static_cast<unsigned>(id));
        }
    }

    // per-site poll types
    async_site_poll_.assign(async_plan_.sites.size(), MIR_INVALID_ID);
    for (size_t i = 0; i < async_plan_.sites.size(); ++i) {
        const AsyncCTypes st = resolve_async_c_types_from_handle(
            const_cast<BaseType*>(async_plan_.sites[i].awaited_handle_type));
        async_site_poll_[i] = st.ok ? types_.map(st.poll) : types_.map(async_poll_base_);
    }
    async_site_index_.clear();
    for (size_t i = 0; i < async_plan_.sites.size(); ++i) {
        if (async_plan_.sites[i].var_init != nullptr) {
            async_site_index_[async_plan_.sites[i].var_init] = i;
        }
    }

    async_emit_frame_helpers(decl);
    if (!async_lower_poll_body(decl, error)) { async_ = false; return false; }
    if (!async_lower_drop_body(decl, error)) { async_ = false; return false; }
    if (!async_lower_ramp(decl, func, error)) { async_ = false; return false; }
    async_ = false;
    return true;
}

void MIRLowerer::async_emit_frame_helpers(FunctionDeclaration* decl) {
    const std::string frame = async_mangled_ + "__frame";
    const std::string inner_c = c_type_of(module_, types_.map(async_inner_));
    const std::string ctx_c = c_type_of(module_, types_.map(async_context_));
    const std::string table_c = c_type_of(module_, types_.map(async_handle_));
    // (the table type is not directly needed; the vtable is spelled below)
    (void)table_c;

    async_pre_decls_ += "struct " + frame + ";\n";
    async_pre_decls_ += "struct " + frame + " { uint32_t __state; " + inner_c + " __result; " +
                        ctx_c + " __cx;";
    for (size_t i = 0; i < async_plan_.sites.size(); ++i) {
        BaseType* ct = async_plan_.sites[i].awaited_handle_type;
        async_pre_decls_ += " " +
                            (ct ? c_type_of(module_, types_.map(ct)) : std::string("void*")) + " " +
                            async_child_field(static_cast<unsigned>(i)) + ";";
    }
    for (size_t id = 0; id < async_plan_.slots.size(); ++id) {
        BaseType* st = async_plan_.slots[id].type;
        const std::string nm = async_slot_field(static_cast<unsigned>(id));
        if (st != nullptr && st->kind() == BaseTypeKind::Array) {
            auto* arr = st->as_array_type_unsafe();
            async_pre_decls_ += " " + c_type_of(module_, types_.map(arr->elem_type)) + " " + nm +
                                "[" + std::to_string(arr->get_array_size()) + "];";
        } else {
            async_pre_decls_ += " " + (st ? c_type_of(module_, types_.map(st))
                                          : std::string("void*")) + " " + nm + ";";
        }
    }
    for (size_t id = 0; id < async_plan_.slots.size(); ++id) {
        if (async_drop_flag_slots_.count(static_cast<unsigned>(id)) != 0) {
            async_pre_decls_ += " uint8_t " + async_drop_flag_field(static_cast<unsigned>(id)) +
                                ";";
        }
    }
    async_pre_decls_ += " };\n";

    // The frame's *runtime* fields must also live in the MIR type table: the C
    // emitter spells frame access from the struct text above, but the LLVM
    // emitter lays the frame out and resolves field names from the table.
    // Without this the frame LLVM type is an opaque `{ i8 }`, so every
    // `__chx__af->…` GEP is out of bounds and the poll/ramp emit invalid IR.
    {
        const TypeId void_ptr = types_.pointer_type(types_.void_type(), true);
        std::vector<MIRFieldInfo> fs;
        auto push = [&](const std::string& fname, TypeId ft) {
            MIRFieldInfo fi;
            fi.type = ft;
            fi.kind = static_cast<uint8_t>(MIRFieldKind::Direct);
            fi.index = static_cast<uint32_t>(fs.size());
            fi.name_offset = module_.types.append_field_name(fname.data(), static_cast<uint32_t>(fname.size()));
            fi.name_length = static_cast<uint32_t>(fname.size());
            fs.push_back(fi);
        };
        push("__state", types_.u32_type());
        push("__result", async_inner_ ? types_.map(async_inner_) : types_.opaque_type());
        push("__cx", async_context_ ? types_.map(async_context_) : void_ptr);
        for (size_t i = 0; i < async_plan_.sites.size(); ++i) {
            BaseType* ct = async_plan_.sites[i].awaited_handle_type;
            push(async_child_field(static_cast<unsigned>(i)),
                 ct ? types_.map(ct) : void_ptr);
        }
        for (size_t id = 0; id < async_plan_.slots.size(); ++id) {
            BaseType* st = async_plan_.slots[id].type;
            TypeId ft = void_ptr;
            if (st != nullptr) {
                if (st->kind() == BaseTypeKind::Array) {
                    auto* arr = st->as_array_type_unsafe();
                    ft = types_.sized_array_type(types_.map(arr->elem_type),
                                                 arr->get_array_size());
                } else {
                    ft = types_.map(st);
                }
            }
            push(async_slot_field(static_cast<unsigned>(id)), ft);
        }
        for (size_t id = 0; id < async_plan_.slots.size(); ++id) {
            if (async_drop_flag_slots_.count(static_cast<unsigned>(id)) == 0) continue;
            MIRTypeRecord r;
            r.kind = MIRTypeKind::Int;
            r.size = 1;
            r.alignment = 1;
            r.int_kind = IK_U8;
            push(async_drop_flag_field(static_cast<unsigned>(id)), module_.types.intern(r));
        }
        const uint32_t fidx = module_.types.append_fields(fs);
        module_.types.update(async_frame_type_, [&](MIRTypeRecord& fr) {
            fr.data_count = static_cast<uint32_t>(fs.size());
            fr.field_index = fidx;
        });
        // a frame is an aggregate: give it its size/alignment/field offsets too,
        // so `sizeof(frame)` and slot addressing come from MIR
        compute_aggregate_layout(module_.types, async_frame_type_);
    }

    const std::string poll_ptr = c_type_of(module_, types_.pointer_type(types_.map(async_poll_base_), true));
    const std::string handle_ptr = c_type_of(module_, types_.pointer_type(types_.map(async_handle_), true));
    async_pre_decls_ += "static void " + async_mangled_ + "__poll(" + poll_ptr +
                        " __chx__async_ret, void* __frame, " + ctx_c + " __cx);\n";
    async_pre_decls_ += "static void " + async_mangled_ + "__drop(void* __frame);\n";

    // drop definition
    async_post_decls_ += "static void " + async_mangled_ + "__drop(void* __frame) {\n";
    async_post_decls_ += "    struct " + frame + "* __chx__af = (struct " + frame +
                         "*) __frame;\n";
    async_post_decls_ += "    switch(__chx__af->__state) {\n";
    for (size_t i = 0; i < async_plan_.sites.size(); ++i) {
        async_post_decls_ += "        case " + std::to_string(i + 1) + ":\n";
        for (auto id : async_plan_.sites[i].live_drops) {
            const auto& slot = async_plan_.slots[id];
            if (slot.type == nullptr) continue;
            SymbolId dtor = destructor_symbol(slot.type);
            std::string dname = mir_sym_name(module_, dtor);
            if (dname.empty()) continue;
            const std::string field = "__chx__af->" + async_slot_field(id);
            // an array slot destroys each element, last to first
            BaseType* canon = slot.type->canonical();
            std::string call;
            if (canon && canon->kind() == BaseTypeKind::Array) {
                const std::string iv = "__chx_di" + std::to_string(id);
                call = "for (unsigned long " + iv + " = " +
                       std::to_string(canon->as_array_type_unsafe()->get_array_size()) +
                       "; " + iv + "-- > 0;) " + dname + "(&" + field + "[" + iv + "])";
            } else {
                call = dname + "(&" + field + ")";
            }
            if (async_drop_flag_slots_.count(id) != 0) {
                async_post_decls_ += "            if(__chx__af->" + async_drop_flag_field(id) +
                                     ") { " + call + "; }\n";
            } else {
                async_post_decls_ += "            " + call + ";\n";
            }
        }
        const std::string child = "__chx__af->" + async_child_field(static_cast<unsigned>(i));
        async_post_decls_ += "            if(" + child + ".frame != (void*) 0) { " + child +
                             ".vtbl->drop(" + child + ".frame); " + child +
                             ".frame = (void*) 0; }\n";
        async_post_decls_ += "            break;\n";
    }
    async_post_decls_ += "        default: break;\n    }\n";
    async_post_decls_ += "    chemical_async_frame_free(__frame, sizeof(struct " + frame +
                         "), _Alignof(struct " + frame + "));\n}\n";
    // vtable definition (references __poll and __drop, both defined above)
    const std::string table_t = c_type_of(module_, types_.map(async_table_));
    async_post_decls_ += "static " + table_t + " " + async_mangled_ + "__vtbl = (" + table_t +
                         "){ .poll = " + async_mangled_ + "__poll, .drop = " + async_mangled_ +
                         "__drop };\n";
    (void)handle_ptr;
}

bool MIRLowerer::async_lower_await_var_init(VarInitStatement* stmt, size_t site_index,
                                            std::string& error) {
    if (site_index >= async_plan_.sites.size()) {
        error = "async await has no matching site";
        return false;
    }
    const AwaitSite& site = async_plan_.sites[site_index];
    auto* await = stmt->value ? stmt->value->as_await_expression_unsafe() : nullptr;
    if (await == nullptr || await->getInner() == nullptr) {
        error = "async await has no operand";
        return false;
    }
    // The child future lives in a frame field so it survives suspension.
    const std::string child_name = async_child_field(static_cast<unsigned>(site_index));
    const ConstantId child_fc = module_.constants.add_string(MIR_INVALID_ID, child_name.data(),
                                                             static_cast<uint32_t>(child_name.size()));
    PlaceId child_place = async_child_places_[site_index];
    if (child_place == MIR_INVALID_ID) {
        child_place = builder_->alloca_frame(types_.map(site.awaited_handle_type), child_fc);
        async_child_places_[site_index] = child_place;
    }
    // result place (the awaited value)
    PlaceId result_place = async_result_places_[site_index];
    // lower the operand and store it in the child slot
    MIRExprResult child = lower_expr(await->getInner(), error);
    if (!child.ok()) return false;
    if (child.kind == MIRExprKind::Place) {
        const PlaceId cp = static_cast<PlaceId>(child.id);
        const TypeId cpt = builder_->function().places[cp].type;
        builder_->field_store_ptr(async_poll_af_val_, child_fc, builder_->load(cp, cpt));
        // the child is consumed by the await; don't destroy the source again
        mark_moved(cp);
    } else {
        builder_->field_store_ptr(async_poll_af_val_, child_fc,
                                  static_cast<ValueId>(child.id));
    }
    // poll
    std::vector<MIROperand> ops;
    ops.push_back(MIROperand::place(async_poll_frame_, async_frame_ptr_type_));
    ops.push_back(MIROperand::place(child_place, types_.map(site.awaited_handle_type)));
    ops.push_back(MIROperand::place(result_place, types_.map(site.awaited_type)));
    ops.push_back(MIROperand::type(async_site_poll_[site_index]));
    ops.push_back(MIROperand::constant(
        module_.constants.add_int(MIR_INVALID_ID, site_index + 1), MIR_INVALID_ID));
    ops.push_back(MIROperand::place(async_poll_ret_, types_.pointer_type(types_.map(async_poll_base_), true)));
    ops.push_back(MIROperand::type(types_.map(async_poll_base_)));
    // spills
    std::vector<std::pair<PlaceId, ConstantId>> spills;
    for (auto id : site.live_slots) {
        const auto& slot = async_plan_.slots[id];
        if (slot.node == site.var_init) continue;
        if (async_resident_.count(slot.node) != 0) continue;
        PlaceId lp = place_for_linked(slot.node);
        if (lp == MIR_INVALID_ID) continue;
        const std::string fname = async_slot_field(id);
        const ConstantId fc = module_.constants.add_string(MIR_INVALID_ID, fname.data(),
                                                           static_cast<uint32_t>(fname.size()));
        spills.emplace_back(lp, fc);
    }
    async_spills_[site_index] = spills;
    ops.push_back(MIROperand::constant(
        module_.constants.add_int(MIR_INVALID_ID, spills.size()), MIR_INVALID_ID));
    for (auto& sp : spills) {
        ops.push_back(MIROperand::place(sp.first, MIR_INVALID_ID));
        ops.push_back(MIROperand::constant(sp.second, MIR_INVALID_ID));
    }
    builder_->emit(MIROpcode::AsyncAwait, MIR_NULL, ops.data(), static_cast<uint32_t>(ops.size()));
    // the continuation after the await is reached from both the first pass and
    // the resume block, so give it its own block
    const BlockId cont = builder_->create_block();
    builder_->br(cont);
    builder_->set_block(cont);
    async_cont_blocks_[site_index] = cont;
    return true;
}

bool MIRLowerer::async_lower_poll_body(FunctionDeclaration* decl, std::string& error) {
    MIRFunction& fn = async_poll_;
    fn.clear();
    const std::string poll_name = async_mangled_ + "__poll";
    MIRSymbolRecord rec;
    rec.kind = MIRSymbolKind::Function;
    rec.linkage = MIRLinkage::Internal;
    SymbolId sym = module_.symbols.add(rec, poll_name.data(),
                                       static_cast<uint32_t>(poll_name.size()), poll_name.data(),
                                       static_cast<uint32_t>(poll_name.size()));
    fn.symbol = sym;
    const TypeId poll_ptr = types_.pointer_type(types_.map(async_poll_base_), true);
    const TypeId void_ptr = types_.pointer_type(types_.void_type(), true);
    const TypeId ctx_ptr = types_.map(async_context_);
    std::vector<TypeId> pts{poll_ptr, void_ptr, ctx_ptr};
    const TypeId ftype = types_.function_signature(types_.void_type(), pts);
    module_.symbols.set_type(sym, ftype);
    fn.function_type = ftype;

    MIRBuilder b(*arena_, module_, fn);
    builder_ = &b;
    const BlockId entry = b.create_block();
    b.set_block(entry);
    fn.entry_block = entry;

    ValueId p0 = b.param(poll_ptr);
    PlaceId ret_place = b.alloca(poll_ptr, MIRStorageClass::Parameter);
    b.store(ret_place, p0);
    ValueId p1 = b.param(void_ptr);
    PlaceId frame_arg = b.alloca(void_ptr, MIRStorageClass::Parameter);
    b.store(frame_arg, p1);
    ValueId p2 = b.param(ctx_ptr);
    PlaceId cx_place = b.alloca(ctx_ptr, MIRStorageClass::Parameter);
    b.store(cx_place, p2);
    async_poll_ret_ = ret_place;
    async_poll_cx_ = cx_place;

    ValueId fv = b.load(frame_arg, void_ptr);
    ValueId af = b.cast(fv, async_frame_ptr_type_);
    PlaceId af_place = b.alloca(async_frame_ptr_type_, MIRStorageClass::FramePtr);
    b.store(af_place, af);
    async_poll_frame_ = af_place;
    async_poll_af_val_ = af;
    // stash the context in the frame so child polls can reach the waker
    {
        const ConstantId cx_fc = module_.constants.add_string(MIR_INVALID_ID, "__cx", 4);
        ValueId cxv = b.load(cx_place, ctx_ptr);
        b.field_store_ptr(af, cx_fc, cxv);
    }

    var_places_.clear();
    name_places_.clear();
    destructibles_.clear();
    call_temps_.clear();
    stmt_temps_.clear();
    pattern_bases_.clear();
    pattern_results_.clear();
    async_child_places_.assign(async_plan_.sites.size(), MIR_INVALID_ID);
    async_result_places_.assign(async_plan_.sites.size(), MIR_INVALID_ID);
    async_spills_.assign(async_plan_.sites.size(), {});
    async_cont_blocks_.assign(async_plan_.sites.size(), MIR_INVALID_ID);

    const size_t nsites = async_plan_.sites.size();
    const BlockId l0_b = b.create_block();
    std::vector<BlockId> resume_b(nsites);
    for (size_t i = 0; i < nsites; ++i) resume_b[i] = b.create_block();
    const BlockId default_b = b.create_block();
    std::vector<BlockId> chain(nsites + 1);
    for (size_t i = 0; i <= nsites; ++i) chain[i] = b.create_block();

    const TypeId u32 = types_.u32_type();
    const ConstantId state_fc = module_.constants.add_string(MIR_INVALID_ID, "__state", 7);
    ValueId st = b.field_load_ptr(af, state_fc, u32);
    const ConstantId eqc = module_.constants.add_int(MIR_INVALID_ID,
                                                     static_cast<uint64_t>(MIRBinaryOp::Eq));
    for (size_t i = 0; i <= nsites; ++i) {
        const ValueId cval = b.const_int(u32, module_.constants.add_int(u32, i));
        ValueId iseq = b.compare(st, cval, eqc, types_.bool_type());
        const BlockId target = (i == 0) ? l0_b : resume_b[i - 1];
        b.cond_br(iseq, target, chain[i]);
        b.set_block(chain[i]);
    }
    b.br(default_b);

    // default: Ready(af->__result)
    b.set_block(default_b);
    b.emit(MIROpcode::AsyncFinish, MIR_NULL,
           {MIROperand::place(af_place, async_frame_ptr_type_),
            MIROperand::place(ret_place, poll_ptr),
            MIROperand::type(types_.map(async_poll_base_))});

    // L0: body
    b.set_block(l0_b);
    for (FunctionParam* p : decl->params) {
        // the frame stores the declared type (aggregates by value), so the poll
        // parameter is the declared type too (not the hidden pointer form)
        TypeId pt = p && p->type ? types_.map(p->type) : types_.opaque_type();
        const std::string rf = async_resident_field(p);
        if (!rf.empty()) {
            const ConstantId fc = module_.constants.add_string(MIR_INVALID_ID, rf.data(),
                                                               static_cast<uint32_t>(rf.size()));
            PlaceId place = b.alloca_frame(pt, fc);
            bind(p, place);
            bind_name(p->name, place);
        } else {
            PlaceId place = b.alloca(pt, MIRStorageClass::Parameter);
            // load the spilled value from the frame slot
            auto it = std::find_if(async_plan_.slots.begin(), async_plan_.slots.end(),
                                   [&](const AsyncFrameSlot& s) { return s.node == p; });
            if (it != async_plan_.slots.end()) {
                const unsigned sid = static_cast<unsigned>(it - async_plan_.slots.begin());
                const std::string sf = async_slot_field(sid);
                const ConstantId fc = module_.constants.add_string(
                    MIR_INVALID_ID, sf.data(), static_cast<uint32_t>(sf.size()));
                ValueId sv = b.field_load_ptr(af, fc, pt);
                b.store(place, sv);
            }
            bind(p, place);
            bind_name(p->name, place);
        }
    }
    if (decl->body.has_value()) {
        if (!lower_scope_nodes(decl->body->nodes, error)) {
            builder_ = nullptr;
            return false;
        }
    }
    if (!b.current_block_terminated()) {
        emit_drops();
        b.emit(MIROpcode::AsyncFinish, MIR_NULL,
               {MIROperand::place(af_place, async_frame_ptr_type_),
                MIROperand::place(ret_place, poll_ptr),
                MIROperand::type(types_.map(async_poll_base_))});
    }

    // resume blocks
    for (size_t i = 0; i < nsites; ++i) {
        b.set_block(resume_b[i]);
        const AwaitSite& site = async_plan_.sites[i];
        // reload spilled slots
        for (auto& sp : async_spills_[i]) {
            // sp.first is the local place; the frame field name is the constant
            const MIRConstant& c = module_.constants.get(sp.second);
            std::string fname(module_.constants.data.data_at(c.data_offset), c.data_count);
            const TypeId lpt = builder_->function().places[sp.first].type;
            const ConstantId fc = module_.constants.add_string(MIR_INVALID_ID, fname.data(),
                                                               static_cast<uint32_t>(fname.size()));
            ValueId sv = b.field_load_ptr(async_poll_af_val_, fc, lpt);
            b.store(sp.first, sv);
        }
        // AsyncAwait with resume_state = 0
        std::vector<MIROperand> ops;
        ops.push_back(MIROperand::place(async_poll_frame_, async_frame_ptr_type_));
        ops.push_back(MIROperand::place(async_child_places_[i],
                                        types_.map(site.awaited_handle_type)));
        ops.push_back(MIROperand::place(async_result_places_[i], types_.map(site.awaited_type)));
        ops.push_back(MIROperand::type(async_site_poll_[i]));
        ops.push_back(MIROperand::constant(module_.constants.add_int(MIR_INVALID_ID, 0),
                                           MIR_INVALID_ID));
        ops.push_back(MIROperand::place(async_poll_ret_,
                                        types_.pointer_type(types_.map(async_poll_base_), true)));
        ops.push_back(MIROperand::type(types_.map(async_poll_base_)));
        ops.push_back(MIROperand::constant(
            module_.constants.add_int(MIR_INVALID_ID, async_spills_[i].size()), MIR_INVALID_ID));
        for (auto& sp : async_spills_[i]) {
            ops.push_back(MIROperand::place(sp.first, MIR_INVALID_ID));
            ops.push_back(MIROperand::constant(sp.second, MIR_INVALID_ID));
        }
        b.emit(MIROpcode::AsyncAwait, MIR_NULL, ops.data(), static_cast<uint32_t>(ops.size()));
        const BlockId cont = async_cont_blocks_[i];
        b.br(cont != MIR_INVALID_ID ? cont : default_b);
    }

    builder_ = nullptr;
    if (!b.ok()) {
        error = b.error() ? b.error() : "async poll builder failure";
        return false;
    }
    return true;
}

namespace {
/** The type of the aggregate field named `name`, or MIR_INVALID_ID. */
TypeId mir_named_field_type(const MIRTypeTable& tt, TypeId agg, const char* name) {
    if (agg >= tt.size()) return MIR_INVALID_ID;
    const std::string want(name);
    const MIRTypeRecord& r = tt.get(agg);
    for (uint32_t i = 0; i < r.data_count; ++i) {
        const MIRFieldInfo* f = tt.field_of(agg, i);
        if (f && tt.field_name_of(*f) == want) return f->type;
    }
    return MIR_INVALID_ID;
}
} // namespace

bool MIRLowerer::async_lower_drop_body(FunctionDeclaration* decl, std::string& error) {
    (void)decl;
    MIRFunction& fn = async_drop_;
    fn.clear();

    const TypeId void_ptr = types_.pointer_type(types_.void_type(), true);
    const TypeId u64 = types_.u64_type();
    const TypeId u32 = types_.u32_type();
    MIRTypeRecord u8_rec;
    u8_rec.kind = MIRTypeKind::Int;
    u8_rec.size = 1;
    u8_rec.alignment = 1;
    u8_rec.int_kind = IK_U8;
    const TypeId u8 = module_.types.intern(u8_rec);

    auto func_symbol = [&](const std::string& name, const std::vector<TypeId>& params,
                           TypeId ret, MIRLinkage linkage) -> SymbolId {
        MIRSymbolRecord rec;
        rec.kind = MIRSymbolKind::Function;
        rec.linkage = linkage;
        const TypeId ft = types_.function_signature(ret, params);
        SymbolId sym = module_.symbols.add(rec, name.data(), static_cast<uint32_t>(name.size()),
                                           name.data(), static_cast<uint32_t>(name.size()));
        module_.symbols.set_type(sym, ft);
        return sym;
    };

    const std::string drop_name = async_mangled_ + "__drop";
    const SymbolId drop_sym =
        func_symbol(drop_name, {void_ptr}, types_.void_type(), MIRLinkage::Internal);
    fn.symbol = drop_sym;
    fn.function_type = module_.symbols.get(drop_sym).type;

    MIRBuilder b(*arena_, module_, fn);
    builder_ = &b;
    const BlockId entry = b.create_block();
    b.set_block(entry);
    fn.entry_block = entry;

    const ValueId p0 = b.param(void_ptr);
    const PlaceId frame_arg = b.alloca(void_ptr, MIRStorageClass::Parameter);
    b.store(frame_arg, p0);

    const ValueId fv = b.load(frame_arg, void_ptr);
    const ValueId af = b.cast(fv, async_frame_ptr_type_);
    const PlaceId af_place = b.alloca(async_frame_ptr_type_, MIRStorageClass::FramePtr);
    b.store(af_place, af);

    const ConstantId state_fc = module_.constants.add_string(MIR_INVALID_ID, "__state", 7);
    const ValueId st = b.field_load_ptr(af, state_fc, u32);
    const ConstantId eqc =
        module_.constants.add_int(MIR_INVALID_ID, static_cast<uint64_t>(MIRBinaryOp::Eq));
    const ConstantId nec =
        module_.constants.add_int(MIR_INVALID_ID, static_cast<uint64_t>(MIRBinaryOp::Ne));

    const size_t nsites = async_plan_.sites.size();
    const BlockId done_b = b.create_block();
    std::vector<BlockId> case_b(nsites), next_b(nsites);
    for (size_t i = 0; i < nsites; ++i) {
        case_b[i] = b.create_block();
        next_b[i] = b.create_block();
    }
    for (size_t i = 0; i < nsites; ++i) {
        const ValueId cval = b.const_int(u32, module_.constants.add_int(u32, i + 1));
        const ValueId iseq = b.compare(st, cval, eqc, types_.bool_type());
        b.cond_br(iseq, case_b[i], next_b[i]);
        b.set_block(next_b[i]);
    }
    b.br(done_b);

    for (size_t i = 0; i < nsites; ++i) {
        b.set_block(case_b[i]);
        for (auto id : async_plan_.sites[i].live_drops) {
            const auto& slot = async_plan_.slots[id];
            if (slot.type == nullptr) continue;
            const SymbolId dtor = destructor_symbol(slot.type);
            if (dtor == MIR_INVALID_ID) continue;
            BaseType* canon = slot.type->canonical();
            TypeId slot_ty = types_.map(slot.type);
            if (canon && canon->kind() == BaseTypeKind::Array) {
                auto* arr = canon->as_array_type_unsafe();
                slot_ty = types_.sized_array_type(types_.map(arr->elem_type),
                                                  arr->get_array_size());
            }
            const std::string fname = async_slot_field(id);
            const ConstantId fc = module_.constants.add_string(
                MIR_INVALID_ID, fname.data(), static_cast<uint32_t>(fname.size()));
            const PlaceId place = b.alloca_frame(slot_ty, fc);
            if (async_drop_flag_slots_.count(id) != 0) {
                const std::string df = async_drop_flag_field(id);
                const ConstantId dfc = module_.constants.add_string(
                    MIR_INVALID_ID, df.data(), static_cast<uint32_t>(df.size()));
                const ValueId flag = b.field_load_ptr(af, dfc, u8);
                b.drop(place, dtor, flag);
            } else {
                b.destroy(place, dtor);
            }
        }
        // drop the live child future: `if (af->child.frame) { vtbl->drop(frame); frame = 0; }`
        BaseType* handle_bt = async_plan_.sites[i].awaited_handle_type;
        if (handle_bt == nullptr) { b.br(done_b); continue; }
        const TypeId handle_ty = types_.map(handle_bt);
        const std::string child_name = async_child_field(static_cast<unsigned>(i));
        const ConstantId cfc = module_.constants.add_string(
            MIR_INVALID_ID, child_name.data(), static_cast<uint32_t>(child_name.size()));
        const PlaceId child_temp = b.alloca(handle_ty, MIRStorageClass::Local);
        const ValueId hv = b.field_load_ptr(af, cfc, handle_ty);
        b.store(child_temp, hv);
        const ConstantId frame_fc =
            module_.constants.add_string(MIR_INVALID_ID, "frame", 5);
        const ConstantId vtbl_fc =
            module_.constants.add_string(MIR_INVALID_ID, "vtbl", 4);
        const ValueId cframe = b.field_load(child_temp, frame_fc, void_ptr);
        const ValueId zero = b.const_null(void_ptr, module_.constants.add_null(void_ptr));
        const ValueId nz = b.compare(cframe, zero, nec, types_.bool_type());
        const BlockId do_b = b.create_block();
        const BlockId end_b = b.create_block();
        b.cond_br(nz, do_b, end_b);
        b.set_block(do_b);
        const TypeId vtbl_ptr_ty = mir_named_field_type(module_.types, handle_ty, "vtbl");
        const TypeId table_ty = (vtbl_ptr_ty != MIR_INVALID_ID && vtbl_ptr_ty < module_.types.size())
                                    ? module_.types.get(vtbl_ptr_ty).element
                                    : MIR_INVALID_ID;
        const TypeId drop_fn_ty = mir_named_field_type(module_.types, table_ty, "drop");
        if (vtbl_ptr_ty != MIR_INVALID_ID && drop_fn_ty != MIR_INVALID_ID) {
            const ValueId vtbl = b.field_load(child_temp, vtbl_fc, vtbl_ptr_ty);
            const ValueId drop_fn = b.field_load_ptr(vtbl, module_.constants.add_string(
                                                                     MIR_INVALID_ID, "drop", 4),
                                                     drop_fn_ty);
            MIROperand arg = MIROperand::value(cframe, void_ptr);
            b.call_indirect(drop_fn, types_.void_type(), &arg, 1);
            b.field_store(child_temp, frame_fc, zero);
        }
        b.br(end_b);
        b.set_block(end_b);
        b.br(done_b);
    }

    b.set_block(done_b);
    const SymbolId free_sym = func_symbol("chemical_async_frame_free",
                                          {void_ptr, u64, u64}, types_.void_type(),
                                          MIRLinkage::External);
    const ValueId sz = b.size_of(async_frame_type_, u64);
    const ValueId al = b.align_of(async_frame_type_, u64);
    MIROperand fargs[3] = {MIROperand::value(fv, void_ptr), MIROperand::value(sz, u64),
                           MIROperand::value(al, u64)};
    b.call_scalar(free_sym, types_.void_type(), fargs, 3);
    b.ret_void();

    builder_ = nullptr;
    if (!b.ok()) {
        error = b.error() ? b.error() : "async drop builder failure";
        return false;
    }
    return true;
}

bool MIRLowerer::async_lower_ramp(FunctionDeclaration* decl, MIRFunction& func,
                                  std::string& error) {
    // The ramp returns the handle; for the eager path it runs the body inline.
    const bool eager = async_plan_.sites.empty();
    const TypeId handle_t = types_.map(async_handle_);
    sret_ = true;
    sret_ret_type_ = handle_t;
    sret_ptr_type_ = types_.pointer_type(handle_t, true);
    FunctionParam* self_param = decl->has_self_param() ? decl->get_self_param() : nullptr;
    current_self_param_ = self_param;
    bool self_in_params = false;
    for (FunctionParam* p : decl->params) {
        if (p == self_param) { self_in_params = true; break; }
    }
    if (self_in_params) self_param = nullptr;
    {
        std::vector<TypeId> ptypes;
        ptypes.push_back(sret_ptr_type_);
        if (self_param) {
            TypeId pt = self_param->type ? types_.map(self_param->type) : types_.opaque_type();
            if (pt != MIR_INVALID_ID && module_.types.get(pt).kind == MIRTypeKind::Array) {
                pt = types_.pointer_type(module_.types.get(pt).element, true);
            } else if (abi::is_aggregate(module_, pt)) {
                pt = types_.pointer_type(pt, true);
            }
            ptypes.push_back(pt);
        }
        for (FunctionParam* p : decl->params) {
            TypeId pt = p && p->type ? types_.map(p->type) : types_.opaque_type();
            if (pt != MIR_INVALID_ID && module_.types.get(pt).kind == MIRTypeKind::Array) {
                pt = types_.pointer_type(module_.types.get(pt).element, true);
            } else if (abi::is_aggregate(module_, pt)) {
                pt = types_.pointer_type(pt, true);
            }
            ptypes.push_back(pt);
        }
        const TypeId ftype = types_.function_signature(types_.void_type(), ptypes);
        func.symbol = intern_function(decl);
        module_.symbols.set_type(func.symbol, ftype);
        func.function_type = ftype;
    }

    MIRBuilder b(*arena_, module_, func);
    builder_ = &b;
    const BlockId entry = b.create_block();
    b.set_block(entry);
    func.entry_block = entry;
    sret_ptr_ = b.param(sret_ptr_type_);
    const PlaceId sret_place = b.alloca(sret_ptr_type_, MIRStorageClass::Parameter);
    b.store(sret_place, sret_ptr_);

    var_places_.clear();
    name_places_.clear();
    destructibles_.clear();
    call_temps_.clear();
    stmt_temps_.clear();
    pattern_bases_.clear();
    pattern_results_.clear();

    // frame alloc
    ValueId af = b.async_frame_alloc(async_frame_type_, async_frame_ptr_type_);
    PlaceId af_place = b.alloca(async_frame_ptr_type_, MIRStorageClass::FramePtr);
    b.store(af_place, af);
    async_ramp_af_ = af_place;
    async_ramp_af_val_ = af;
    // set state = 0 and cx = 0
    const ConstantId st_fc = module_.constants.add_string(MIR_INVALID_ID, "__state", 7);
    const ConstantId cx_fc = module_.constants.add_string(MIR_INVALID_ID, "__cx", 4);
    ValueId zero = b.const_int(types_.u32_type(), module_.constants.add_int(types_.u32_type(), 0));
    b.field_store_ptr(af, st_fc, zero);
    ValueId nullv = b.const_null(types_.map(async_context_),
                                 module_.constants.add_int(types_.map(async_context_), 0));
    b.field_store_ptr(af, cx_fc, nullv);

    // bind self/params and spill parameters into the frame
    if (self_param) {
        TypeId pt = self_param->type ? types_.map(self_param->type) : types_.opaque_type();
        if (pt != MIR_INVALID_ID && module_.types.get(pt).kind == MIRTypeKind::Array) {
            pt = types_.pointer_type(module_.types.get(pt).element, true);
        } else if (abi::is_aggregate(module_, pt)) {
            pt = types_.pointer_type(pt, true);
        }
        ValueId pv = b.param(pt);
        PlaceId place = b.alloca(pt, MIRStorageClass::Parameter);
        b.store(place, pv);
        bind(self_param, place);
        bind_name(self_param->name, place);
    }
    for (FunctionParam* p : decl->params) {
        TypeId pt = p && p->type ? types_.map(p->type) : types_.opaque_type();
        if (pt != MIR_INVALID_ID && module_.types.get(pt).kind == MIRTypeKind::Array) {
            pt = types_.pointer_type(module_.types.get(pt).element, true);
        } else if (abi::is_aggregate(module_, pt)) {
            pt = types_.pointer_type(pt, true);
        }
        ValueId pv = b.param(pt);
        PlaceId place = b.alloca(pt, MIRStorageClass::Parameter);
        b.store(place, pv);
        bind(p, place);
        bind_name(p->name, place);
        // store the parameter into its frame slot
        auto it = std::find_if(async_plan_.slots.begin(), async_plan_.slots.end(),
                               [&](const AsyncFrameSlot& s) { return s.node == p; });
        if (it != async_plan_.slots.end()) {
            const unsigned sid = static_cast<unsigned>(it - async_plan_.slots.begin());
            const std::string sf = async_slot_field(sid);
            const ConstantId fc = module_.constants.add_string(MIR_INVALID_ID, sf.data(),
                                                               static_cast<uint32_t>(sf.size()));
            const MIRTypeRecord& pr = module_.types.get(pt);
            ValueId src = b.load(place, pt);
            const TypeId decl_t = p && p->type ? types_.map(p->type) : types_.opaque_type();
            if (pr.kind == MIRTypeKind::Pointer || pr.kind == MIRTypeKind::Reference) {
                // an aggregate parameter arrives as a hidden pointer: copy the
                // pointee value into the frame. A genuine reference/pointer
                // parameter stores the pointer itself (no dereference).
                if (abi::is_aggregate(module_, decl_t)) {
                    src = b.load_indirect(src, decl_t);
                }
            }
            b.field_store_ptr(af, fc, src);
            if (async_drop_flag_slots_.count(sid) != 0) {
                const std::string df = async_drop_flag_field(sid);
                const ConstantId dfc = module_.constants.add_string(MIR_INVALID_ID, df.data(),
                                                                    static_cast<uint32_t>(df.size()));
                ValueId one = b.const_int(types_.u32_type(),
                                          module_.constants.add_int(types_.u32_type(), 1));
                b.field_store_ptr(af, dfc, one);
            }
        }
    }

    if (eager) {
        // run the body inline, redirecting `return e` into the frame result
        async_in_ramp_ = true;
        async_ramp_result_ = MIR_INVALID_ID;
        async_ramp_done_ = b.create_block();
        if (decl->body.has_value()) {
            if (!lower_scope_nodes(decl->body->nodes, error)) {
                builder_ = nullptr;
                return false;
            }
        }
        if (!b.current_block_terminated()) b.br(async_ramp_done_);
        b.set_block(async_ramp_done_);
        async_in_ramp_ = false;
    }
    // return the handle
    const ConstantId vtbl_c = module_.constants.add_string(
        MIR_INVALID_ID, (async_mangled_ + "__vtbl").data(),
        static_cast<uint32_t>((async_mangled_ + "__vtbl").size()));
    ValueId state_one = b.const_int(types_.u32_type(),
                                    module_.constants.add_int(types_.u32_type(), eager ? 1 : 0));
    (void)state_one;
    const ConstantId ramp_state_c =
        module_.constants.add_int(types_.u32_type(), eager ? 1 : 0);
    b.emit(MIROpcode::AsyncRampFinish, MIR_NULL,
           {MIROperand::place(af_place, async_frame_ptr_type_),
            MIROperand::place(sret_place, sret_ptr_type_),
            MIROperand::constant(ramp_state_c, MIR_INVALID_ID),
            MIROperand::constant(vtbl_c, MIR_INVALID_ID),
            MIROperand::type(handle_t)});

    builder_ = nullptr;
    if (!b.ok()) {
        error = b.error() ? b.error() : "async ramp builder failure";
        return false;
    }
    return true;
}

/**
 * True when `decl` is an application's `async func main`. `declare_module`
 * renames such an entry to `kAsyncMainInternalName` before any prototype is
 * emitted, so the ramp never owns the `main` symbol and the synchronous
 * trampoline can. (An application's `main` is marked `no_mangle` by symres, so
 * before the rename the name is exactly `main`.)
 */
bool MIRLowerer::is_async_main_entry(FunctionDeclaration* decl) {
    if (!decl || async_main_emitted_) return false;
    if (!decl->is_async() || !decl->is_no_mangle()) return false;
    return decl->name_view() == chem::string_view(kAsyncMainInternalName);
}

bool MIRLowerer::lower_async_main_trampoline(FunctionDeclaration* decl, const MIRFunction& ramp,
                                             MIRArena& arena, MIRFunction& func,
                                             std::string& error) {
    if (!is_async_main_entry(decl)) return false;
    error.clear();
    const AsyncCTypes t = resolve_async_c_types(decl);
    if (!t.ok) {
        error = "async main: could not resolve the core::async protocol types";
        return false;
    }
    if (ramp.function_type == MIR_INVALID_ID || ramp.function_type >= module_.types.size()) {
        error = "async main: the ramp has no function type";
        return false;
    }

    const TypeId void_ptr = types_.pointer_type(types_.void_type(), true);
    const TypeId i32 = types_.int_c_type();
    const TypeId handle_t = types_.map(t.handle);
    const TypeId handle_ptr_t = types_.pointer_type(handle_t, true);
    const TypeId poll_t = types_.map(t.poll);
    const TypeId poll_ptr_t = types_.pointer_type(poll_t, true);
    const TypeId ctx_ptr_t = types_.map(t.context_ptr);
    const TypeId table_t = types_.map(t.table);
    const TypeId table_ptr_t = types_.pointer_type(table_t, true);
    const TypeId poll_fn_t = mir_named_field_type(module_.types, table_t, "poll");
    const TypeId drop_fn_t = mir_named_field_type(module_.types, table_t, "drop");
    if (poll_fn_t == MIR_INVALID_ID || drop_fn_t == MIR_INVALID_ID) {
        error = "async main: FutureTable is missing its poll/drop slots";
        return false;
    }
    // `*mut Context` is the `__cx` parameter type, so the local to take the
    // address of is the pointee (`struct Context`), exactly like the legacy
    // wrapper's `struct Context __chx__async_cx = {0};`.
    TypeId ctx_val_t = ctx_ptr_t;
    {
        const MIRTypeRecord& cr = module_.types.get(ctx_ptr_t);
        if (cr.kind == MIRTypeKind::Pointer || cr.kind == MIRTypeKind::Reference) {
            ctx_val_t = cr.element;
        }
    }

    // The entry signature mirrors the ramp's, minus the hidden sret slot.
    func.clear();
    {
        const MIRTypeRecord& ftr = module_.types.get(ramp.function_type);
        std::vector<TypeId> ptypes;
        for (uint32_t i = 1; i < ftr.data_count; ++i) {
            if (ftr.data_offset + i >= module_.types.data_size()) break;
            ptypes.push_back(module_.types.data_at(ftr.data_offset + i));
        }
        const TypeId ftype = types_.function_signature(i32, ptypes);
        MIRSymbolRecord rec;
        rec.kind = MIRSymbolKind::Function;
        // the C entry point must have external linkage
        rec.linkage = MIRLinkage::External;
        rec.type = ftype;
        const char* const main_name = "main";
        const SymbolId sym =
            module_.symbols.add(rec, main_name, 4, main_name, 4);
        func.symbol = sym;
        module_.symbols.set_type(sym, ftype);
        func.function_type = ftype;
    }

    MIRBuilder b(arena, module_, func);
    builder_ = &b;
    const BlockId entry = b.create_block();
    b.set_block(entry);
    func.entry_block = entry;

    const MIRTypeRecord& ftr = module_.types.get(func.function_type);
    std::vector<TypeId> ptypes;
    std::vector<ValueId> params;
    for (uint32_t i = 0; i < ftr.data_count; ++i) {
        const TypeId pt = module_.types.data_at(ftr.data_offset + i);
        ptypes.push_back(pt);
        params.push_back(b.param(pt));
    }

    // call the ramp: it writes the FutureHandle<T> through its sret slot
    const PlaceId handle_place = b.alloca(handle_t, MIRStorageClass::Local);
    {
        MIRArgScope _ag_scope(*this);
        std::vector<MIROperand>& args = _ag_scope.v;
        args.reserve(params.size() + 1);
        args.push_back(MIROperand::value(b.address_of(handle_place, handle_ptr_t), handle_ptr_t));
        for (size_t i = 0; i < params.size(); ++i) {
            args.push_back(MIROperand::value(params[i], ptypes[i]));
        }
        b.call_scalar(intern_function(decl), types_.void_type(), args.data(),
                      static_cast<uint32_t>(args.size()));
    }

    const PlaceId poll_place = b.alloca(poll_t, MIRStorageClass::Local);
    const PlaceId cx_place = b.alloca(ctx_val_t, MIRStorageClass::Local);
    // a zeroed context is the "no waker" executor context, matching block_on
    b.zero_init(cx_place);

    const ConstantId frame_fc = module_.constants.add_string(MIR_INVALID_ID, "frame", 5);
    const ConstantId vtbl_fc = module_.constants.add_string(MIR_INVALID_ID, "vtbl", 4);
    const ConstantId poll_fc = module_.constants.add_string(MIR_INVALID_ID, "poll", 4);
    const ConstantId drop_fc = module_.constants.add_string(MIR_INVALID_ID, "drop", 4);
    const ConstantId tag_fc =
        module_.constants.add_string(MIR_INVALID_ID, "__chx__vt_621827", 16);
    const ConstantId eq_op =
        module_.constants.add_int(MIR_INVALID_ID, static_cast<uint64_t>(MIRBinaryOp::Eq));

    const BlockId loop_b = b.create_block();
    const BlockId done_b = b.create_block();
    b.br(loop_b);

    // poll until Ready (tag 0)
    b.set_block(loop_b);
    const ValueId frame = b.field_load(handle_place, frame_fc, void_ptr);
    const ValueId vtbl = b.field_load(handle_place, vtbl_fc, table_ptr_t);
    const ValueId poll_fn = b.field_load_ptr(vtbl, poll_fc, poll_fn_t);
    {
        MIROperand poll_args[3] = {
            MIROperand::value(b.address_of(poll_place, poll_ptr_t), poll_ptr_t),
            MIROperand::value(frame, void_ptr),
            MIROperand::value(b.address_of(cx_place, ctx_ptr_t), ctx_ptr_t),
        };
        b.call_indirect(poll_fn, types_.void_type(), poll_args, 3);
    }
    const ValueId tag = b.field_load(poll_place, tag_fc, i32);
    const ValueId zero = b.const_int(i32, module_.constants.add_int(i32, 0));
    const ValueId ready = b.compare(tag, zero, eq_op, types_.bool_type());
    b.cond_br(ready, done_b, loop_b);

    b.set_block(done_b);
    // `Unit` is the zero-sized result of an async function with no value
    BaseType* inner_canon = t.inner ? t.inner->canonical() : nullptr;
    const bool inner_void =
        inner_canon == nullptr || inner_canon->kind() == BaseTypeKind::Void;
    ASTNode* inner_node = (!inner_void && t.inner) ? t.inner->linked_node() : nullptr;
    const bool inner_unit = inner_node != nullptr &&
                            inner_node->kind() == ASTNodeKind::StructDecl &&
                            inner_node->as_struct_def_unsafe()->name_view() ==
                                chem::string_view("Unit");
    ValueId result = MIR_NULL;
    if (!inner_void && !inner_unit) {
        const ConstantId ready_fc =
            module_.constants.add_string(MIR_INVALID_ID, "Ready.value", 11);
        const ValueId payload = b.field_load(poll_place, ready_fc, types_.map(t.inner));
        result = b.cast(payload, i32);
    } else {
        result = b.const_int(i32, module_.constants.add_int(i32, 0));
    }
    // drop the handle (cancels a suspended frame once)
    const ValueId frame2 = b.field_load(handle_place, frame_fc, void_ptr);
    const ValueId vtbl2 = b.field_load(handle_place, vtbl_fc, table_ptr_t);
    const ValueId drop_fn = b.field_load_ptr(vtbl2, drop_fc, drop_fn_t);
    {
        MIROperand drop_args[1] = {MIROperand::value(frame2, void_ptr)};
        b.call_indirect(drop_fn, types_.void_type(), drop_args, 1);
    }
    b.ret(result);

    builder_ = nullptr;
    if (!b.ok()) {
        error = b.error() ? b.error() : "async main trampoline builder failure";
        return false;
    }
    async_main_emitted_ = true;
    return true;
}


} // namespace mir
