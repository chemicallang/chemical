// Copyright (c) Chemical Language Foundation 2025.
//
// MIR lowering: calls, methods, patterns and symbol interning.
// Split out of MIRLowerer.cpp (action D4, mir-migration-evaluation.md).
// AST-aware: this is the front-end lowering boundary, never a backend.

#include "MIRLowererHelpers.h"
#include "compiler/safety/Variance.h"
#include "ast/structures/GenericStructDecl.h"

namespace mir {

chem::string_view MIRLowerer::member_name_view(Value* v) {
    Value* t = v;
    if (t && t->val_kind() == ValueKind::AccessChain) {
        auto* c = t->as_access_chain_unsafe();
        if (c->values.size() == 1) t = c->values[0];
    }
    if (!t || t->val_kind() != ValueKind::Identifier) return chem::string_view();
    // the bytes are owned by the AST (which outlives the lowering call)
    return t->as_identifier_unsafe()->value;
}

bool MIRLowerer::member_name(Value* v, std::string& out, std::string& error) {
    const chem::string_view name = member_name_view(v);
    if (name.empty()) {
        error = "member is not an identifier";
        return false;
    }
    out.assign(name.data(), name.size());
    return true;
}

bool MIRLowerer::member_field_path(Value* base, Value* member, std::string& out,
                                   std::string& error) {
    Value* m = member;
    if (m && m->val_kind() == ValueKind::AccessChain) {
        auto* c = m->as_access_chain_unsafe();
        if (c->values.size() == 1) m = c->values[0];
    }
    ASTNode* mnode = m ? m->linked_node() : nullptr;
    BaseType* bt = base ? base->getType() : nullptr;
    // unwrap a pointer/reference base (`&self` / `self` by pointer) so the
    // container node is reachable for inherited-field resolution
    if (bt) {
        BaseType* bc = bt->canonical();
        if (bc && bc->kind() == BaseTypeKind::Pointer) {
            bt = bc->as_pointer_type_unsafe()->type;
        } else if (bc && bc->kind() == BaseTypeKind::Reference) {
            bt = bc->as_reference_type_unsafe()->type;
        }
    }
    ASTNode* cn = bt ? bt->get_direct_linked_canonical_node() : nullptr;
    if (mnode && cn) {
        // `inherited_field_path` only writes `out` on success, so a direct write
        // avoids the intermediate string + copy this used to build per access.
        if (inherited_field_path(cn, mnode, out)) {
            return true;
        }
    }
    // no intermediate string: the view is copied straight into `out`
    const chem::string_view mname = member_name_view(member);
    if (mname.empty()) {
        error = "member is not an identifier";
        return false;
    }
    out.assign(mname.data(), mname.size());
    return true;
}

FunctionParam* MIRLowerer::aggregate_param_operand(Value* v) {
    if (aggregate_param_ptrs_.empty()) return nullptr;
    Value* t = v;
    if (t && t->val_kind() == ValueKind::AccessChain) {
        auto* c = t->as_access_chain_unsafe();
        if (c->values.size() != 1) return nullptr;
        t = c->values[0];
    }
    if (!t || t->val_kind() != ValueKind::Identifier) return nullptr;
    ASTNode* linked = t->linked_node();
    if (!linked || linked->kind() != ASTNodeKind::FunctionParam) return nullptr;
    auto* fp = linked->as_func_param_unsafe();
    auto it = aggregate_param_ptrs_.find(fp);
    return it == aggregate_param_ptrs_.end() ? nullptr : fp;
}

bool MIRLowerer::lower_call_args(const std::vector<Value*>& values,
                                 std::vector<MIROperand>& args, std::string& error) {
    return lower_call_args_for(nullptr, false, values, args, error);
}

bool MIRLowerer::lower_call_args_for(FunctionDeclaration* fd, bool self_included,
                                     const std::vector<Value*>& values,
                                     std::vector<MIROperand>& args, std::string& error,
                                     const FunctionType* ast_fn) {
    const size_t offset = self_included ? 1 : 0;
    for (size_t i = 0; i < values.size(); ++i) {
        BaseType* param_type = nullptr;
        if (fd && i + offset < fd->params.size()) {
            param_type = fd->params[i + offset]->type;
        } else if (!fd && ast_fn && i + offset < ast_fn->params.size()) {
            FunctionParam* ap = ast_fn->params[i + offset];
            param_type = ap ? ap->type : nullptr;
        }
        MIRExprResult r = lower_arg_converted(values[i], param_type, error);
        if (!r.ok()) return false;
        // a non-pointer value passed to a reference parameter must be
        // materialized in a temporary whose address is passed
        if (param_type && param_type->kind() == BaseTypeKind::Reference &&
            r.kind == MIRExprKind::Value && r.type != MIR_INVALID_ID &&
            r.type < module_.types.size()) {
            const MIRTypeKind rk = module_.types.get(r.type).kind;
            if (rk != MIRTypeKind::Pointer && rk != MIRTypeKind::Reference) {
                BaseType* inner = param_type->as_reference_type()->type;
                const TypeId it = types_.map(inner);
                const PlaceId tmp = builder_->alloca(it, MIRStorageClass::Temporary);
                builder_->store(tmp, r.id);
                const TypeId ptrt = types_.pointer_type(it, true);
                r = MIRExprResult::value(builder_->address_of(tmp, ptrt), ptrt);
            }
        }
        // track destructible temporaries passed by value (destroyed after the call)
        if (r.kind == MIRExprKind::Place && values[i]) {
            Value* v0 = values[i];
            if (v0->val_kind() == ValueKind::AccessChain) {
                auto* c = v0->as_access_chain_unsafe();
                if (c->values.size() == 1) v0 = c->values[0];
            }
            const ValueKind vk = v0->val_kind();
            const bool by_value =
                !(param_type && (param_type->kind() == BaseTypeKind::Reference ||
                                 param_type->kind() == BaseTypeKind::Pointer));
            const bool known_param = param_type != nullptr;
            if (vk == ValueKind::StructValue || vk == ValueKind::FunctionCall) {
                BaseType* at = v0->getType();
                // when the parameter type is unknown (an indirect/function-pointer
                // call) conservatively treat a temporary as caller-owned
                if (at && (!by_value || !known_param)) {
                    call_temps_.emplace_back(static_cast<PlaceId>(r.id), at);
                }
            } else if (vk == ValueKind::Identifier && by_value) {
                // a local passed by value is moved out; an array decays to a
                // pointer and stays owned by the caller
                BaseType* at = v0->getType();
                BaseType* canon = at ? at->canonical() : nullptr;
                if (!(canon && canon->kind() == BaseTypeKind::Array)) {
                    const PlaceId pid = static_cast<PlaceId>(r.id);
                    const TypeId pt = builder_->function().places[pid].type;
                    builder_->emit(MIROpcode::MoveOut, MIR_NULL,
                                   {MIROperand::place(pid, pt)});
                    mark_moved(pid);
                }
            }
        }
        if (!push_call_arg(r, args, error)) return false;
    }
    return true;
}

void MIRLowerer::destroy_call_temps(std::string& error) {
    // A temporary passed by reference (`&T`) is owned by the caller and must be
    // destroyed after the call; by-value temporaries are owned by the callee's
    // parameter instead.
    for (auto& [place, type] : call_temps_) {
        const SymbolId dtor = destructor_symbol(type);
        if (dtor == MIR_INVALID_ID) continue;
        builder_->destroy(place, dtor);
    }
    call_temps_.clear();
    (void)error;
}

void MIRLowerer::destroy_call_temps_from(size_t mark, std::string& error) {
    // destroy only the temporaries created for the call that started at `mark`,
    // leaving the enclosing call's in-progress temporaries intact
    for (size_t i = mark; i < call_temps_.size(); ++i) {
        const SymbolId dtor = destructor_symbol(call_temps_[i].second);
        if (dtor == MIR_INVALID_ID) continue;
        builder_->destroy(call_temps_[i].first, dtor);
    }
    call_temps_.resize(mark);
    (void)error;
}

SymbolId MIRLowerer::destructor_symbol(BaseType* type) {
    if (!type) return MIR_INVALID_ID;
    BaseType* canon = type->canonical();
    if (!canon) return MIR_INVALID_ID;
    if (canon->kind() == BaseTypeKind::Array) {
        // an array of destructibles drops each element
        return destructor_symbol(canon->as_array_type_unsafe()->elem_type);
    }
    ASTNode* node = canon->get_direct_linked_canonical_node();
    if (!node) return MIR_INVALID_ID;
    FunctionDeclaration* dtor = nullptr;
    if (node->kind() == ASTNodeKind::StructDecl) {
        dtor = node->as_struct_def_unsafe()->destructor_func();
    } else if (node->kind() == ASTNodeKind::VariantDecl) {
        dtor = node->as_variant_def_unsafe()->destructor_func();
    } else if (node->kind() == ASTNodeKind::UnionDecl) {
        dtor = node->as_union_def_unsafe()->destructor_func();
    }
    if (!dtor || !dtor->body.has_value()) return MIR_INVALID_ID;
    return intern_function(dtor);
}

SymbolId MIRLowerer::extern_symbol(const char* name, TypeId ftype) {
    auto it = extern_symbols_.find(name);
    if (it != extern_symbols_.end()) return it->second;
    MIRSymbolRecord rec;
    rec.kind = MIRSymbolKind::Function;
    rec.linkage = MIRLinkage::External;
    rec.type = ftype;
    const uint32_t len = static_cast<uint32_t>(std::strlen(name));
    SymbolId s = module_.symbols.add(rec, name, len, name, len);
    extern_symbols_[name] = s;
    return s;
}

ValueId MIRLowerer::emit_malloc(TypeId size_v, TypeId u64t, TypeId ptr_t) {
    const TypeId ft = types_.function_signature(ptr_t, {u64t});
    const SymbolId sym = extern_symbol("malloc", ft);
    MIROperand arg = MIROperand::value(size_v, u64t);
    const ValueId ptr = builder_->call_scalar(sym, ptr_t, &arg, 1);
    // `new T` is a safe operation: allocation failure must panic, not produce a
    // null pointer that is later written through.
    const TypeId bool_t = types_.bool_type();
    const ValueId null_v = builder_->const_null(
        ptr_t, module_.constants.add_null(ptr_t));
    const ConstantId ne = module_.constants.add_int(
        MIR_INVALID_ID, static_cast<uint64_t>(MIRBinaryOp::Ne));
    emit_abort_unless(builder_->compare(ptr, null_v, ne, bool_t));
    return ptr;
}

void MIRLowerer::emit_free(ValueId ptr) {
    const TypeId vpt = types_.pointer_type(types_.void_type(), true);
    const TypeId ft = types_.function_signature(types_.void_type(), {vpt});
    const SymbolId sym = extern_symbol("free", ft);
    const ValueId p = (ptr != MIR_NULL) ? builder_->cast(ptr, vpt) : ptr;
    MIROperand arg = MIROperand::value(p, vpt);
    builder_->call_scalar(sym, types_.void_type(), &arg, 1);
}

void MIRLowerer::emit_member_dtor(ValueId self_ptr, const chem::string_view& member,
                                  BaseType* mt, std::string& error) {
    if (!mt) return;
    BaseType* mc = mt->canonical();
    if (!mc) return;
    if (mc->kind() == BaseTypeKind::CapturingFunction) {
        // a `std::function` field owns a closure instance; destroy that
        BaseType* inst = const_cast<BaseType*>(
            mc->as_capturing_func_type_unsafe()->instance_type.getType());
        emit_member_dtor(self_ptr, member, inst, error);
        return;
    }
    if (mc->kind() == BaseTypeKind::Array) {
        BaseType* elem = mc->as_array_type_unsafe()->elem_type;
        const SymbolId dtor = destructor_symbol(elem);
        if (dtor == MIR_INVALID_ID) return;
        const uint64_t n = mc->as_array_type_unsafe()->get_array_size();
        if (n == 0) return;
        const TypeId ft = types_.map(mt);
        const ConstantId fc = module_.constants.add_string(
            MIR_INVALID_ID, member.data(), static_cast<uint32_t>(member.size()));
        const TypeId eptr = types_.pointer_type(types_.map(elem), true);
        // `field_addr_ptr` spells an array field as a pointer-to-array
        // (`Elem (*v)[N]`); cast it to an element pointer for the loop
        const ValueId faddr =
            builder_->cast(builder_->field_addr_ptr(self_ptr, fc, ft), eptr);
        const TypeId u64t = types_.u64_type();
        const ConstantId add_op = module_.constants.add_int(
            MIR_INVALID_ID, static_cast<uint64_t>(MIRBinaryOp::Add));
        for (uint64_t i = 0; i < n; ++i) {
            const ValueId off = builder_->const_int(u64t, module_.constants.add_int(u64t, i));
            const ValueId ep = (i == 0) ? faddr : builder_->binary(faddr, off, add_op, eptr);
            MIROperand a = MIROperand::value(ep, eptr);
            builder_->call_scalar(dtor, types_.void_type(), &a, 1);
        }
        return;
    }
    const SymbolId dtor = destructor_symbol(mt);
    if (dtor == MIR_INVALID_ID) return;
    const TypeId ft = types_.map(mt);
    const ConstantId fc = module_.constants.add_string(
        MIR_INVALID_ID, member.data(), static_cast<uint32_t>(member.size()));
    const ValueId faddr = builder_->field_addr_ptr(self_ptr, fc, ft);
    MIROperand a = MIROperand::value(faddr, types_.pointer_type(ft, true));
    builder_->call_scalar(dtor, types_.void_type(), &a, 1);
}

void MIRLowerer::emit_destructor_members(std::string& error) {
    if (!destructor_def_ || !current_self_param_) return;
    PlaceId selfp = place_for_linked(current_self_param_);
    if (selfp == MIR_INVALID_ID) selfp = place_for_name(current_self_param_->name);
    if (selfp == MIR_INVALID_ID) return;
    const TypeId st = builder_->function().places[selfp].type;
    const MIRTypeRecord& sr = module_.types.get(st);
    ValueId self_ptr = MIR_NULL;
    if (sr.kind == MIRTypeKind::Pointer || sr.kind == MIRTypeKind::Reference) {
        self_ptr = builder_->load(selfp, st);
    } else {
        self_ptr = builder_->address_of(selfp, types_.pointer_type(st, true));
    }
    if (destructor_def_->kind() == ASTNodeKind::VariantDecl) {
        emit_variant_destructor_members(destructor_def_->as_variant_def_unsafe(), self_ptr, error);
        return;
    }
    for (auto& inh : destructor_def_->inherited) {
        auto* linked = inh.type ? inh.type->get_direct_linked_struct() : nullptr;
        if (linked) {
            emit_member_dtor(self_ptr, linked->name_view(), inh.type, error);
        }
    }
    for (auto* var : destructor_def_->variables()) {
        emit_member_dtor(self_ptr, var->name, var->known_type(), error);
    }
}

void MIRLowerer::emit_variant_destructor_members(VariantDefinition* vd, ValueId self_ptr,
                                                 std::string& error) {
    if (!vd) return;
    auto& vars = vd->variables();
    if (vars.empty()) return;
    const TypeId tag_t = types_.u32_type();
    const ConstantId tag_fc =
        module_.constants.add_string(MIR_INVALID_ID, "__chx__vt_621827", 16);
    const ValueId tag = builder_->field_load_ptr(self_ptr, tag_fc, tag_t);
    const ConstantId eq_op =
        module_.constants.add_int(MIR_INVALID_ID, static_cast<uint64_t>(MIRBinaryOp::Eq));
    const BlockId done = builder_->create_block();
    unsigned index = 0;
    for (auto* var : vars) {
        auto* member = var ? var->as_variant_member_unsafe() : nullptr;
        if (!member) {
            index++;
            continue;
        }
        const BlockId next = builder_->create_block();
        const BlockId match = builder_->create_block();
        const ValueId idx_v =
            builder_->const_int(tag_t, module_.constants.add_int(tag_t, index));
        const ValueId is_eq = builder_->compare(tag, idx_v, eq_op, types_.bool_type());
        builder_->cond_br(is_eq, match, next);
        builder_->set_block(match);
        for (auto& p : member->values) {
            auto* param = p.second;
            if (!param) continue;
            std::string path(member->name.data(), member->name.size());
            path += ".";
            path += std::string(param->name.data(), param->name.size());
            emit_member_dtor(self_ptr, chem::string_view(path.data(), path.size()), param->type,
                             error);
        }
        if (!builder_->current_block_terminated()) builder_->br(done);
        builder_->set_block(next);
        index++;
    }
    if (!builder_->current_block_terminated()) builder_->br(done);
    builder_->set_block(done);
}

namespace {

/** Mirrors `MembersContainer::any_member_has_destructor` for one field type. */
bool member_requires_dtor(BaseType* mt) {
    if (!mt) return false;
    BaseType* c = mt->canonical();
    if (!c) return false;
    if (c->kind() == BaseTypeKind::CapturingFunction) {
        BaseType* inst = const_cast<BaseType*>(
            c->as_capturing_func_type_unsafe()->instance_type.getType()
        );
        return inst != nullptr && inst->requires_destructor();
    }
    if (c->kind() == BaseTypeKind::Array) {
        BaseType* elem = c->as_array_type_unsafe()->elem_type;
        return elem != nullptr && elem->requires_destructor();
    }
    return c->requires_destructor();
}

} // namespace

void MIRLowerer::register_destructible(PlaceId place, BaseType* type,
                                       bool whole_value_initialized) {
    const SymbolId dtor = destructor_symbol(type);
    if (dtor == MIR_INVALID_ID) return;

    // A bare local whose struct only has generated drop glue may be initialized
    // field by field, so each owned field gets its own drop flag. Any other
    // destructible keeps the single whole-value flag (and therefore requires a
    // whole-value construction).
    std::vector<MIRDropField> field_flags;
    if (!whole_value_initialized && !async_ && place < builder_->function().places.size() &&
        builder_->function().places[place].storage_class ==
            static_cast<uint8_t>(MIRStorageClass::Local)) {
        BaseType* canon = type ? type->canonical() : nullptr;
        ASTNode* node = canon ? canon->get_direct_linked_canonical_node() : nullptr;
        if (canon && canon->kind() == BaseTypeKind::Array) {
            // A fixed array of destructible elements can be built element by
            // element when the index is a compile-time constant. Very large
            // arrays keep the whole-value drop (partial init stays rejected).
            BaseType* elem = canon->as_array_type_unsafe()->elem_type;
            const uint64_t n = canon->as_array_type_unsafe()->get_array_size();
            if (elem && n > 0 && n <= 64 && destructor_symbol(elem) != MIR_INVALID_ID) {
                for (uint64_t i = 0; i < n; ++i) {
                    MIRDropField f = build_drop_field(chem::string_view{}, elem, 2);
                    f.array_index = static_cast<uint32_t>(i);
                    field_flags.push_back(std::move(f));
                }
            }
        } else if (node && node->kind() == ASTNodeKind::StructDecl &&
                   !canon->has_user_destructor()) {
            StructDefinition* sd = node->as_struct_def_unsafe();
            for (auto& inh : sd->inherited) {
                if (!member_requires_dtor(inh.type)) continue;
                auto* linked = inh.type->get_direct_linked_struct();
                if (!linked) continue;
                field_flags.push_back(
                    build_drop_field(linked->name_view(), inh.type, 1)
                );
            }
            for (auto* var : sd->variables()) {
                if (!var || !var->known_type()) continue;
                if (!member_requires_dtor(var->known_type())) continue;
                field_flags.push_back(
                    build_drop_field(var->name, var->known_type(), 1)
                );
            }
        }
    }
    const bool field_wise = !field_flags.empty();
    if (field_wise) {
        builder_->function().places[place].field_wise_drop = true;
    }

    PlaceId flag = MIR_INVALID_ID;
    if (async_ && place < builder_->function().places.size()) {
        // a frame-resident destructible uses the frame drop flag the drop
        // function checks, so completion and cancellation agree on liveness
        const MIRPlaceDef& pd = builder_->function().places[place];
        if (pd.storage_class == static_cast<uint8_t>(MIRStorageClass::FrameField) &&
            pd.frame_field < module_.constants.size()) {
            const MIRConstant& c = module_.constants.get(pd.frame_field);
            static constexpr std::string_view k_slot_prefix = "__chx_slot_";
            const std::string_view fname(module_.constants.data.data_at(c.data_offset),
                                         c.data_count);
            if (fname.rfind(k_slot_prefix, 0) == 0) {
                // `__chx_drop_<slot>` built in one buffer: no `fname` copy and no
                // `substr` temporary per registered destructible
                std::string df("__chx_drop_");
                df.append(fname.data() + k_slot_prefix.size(),
                          fname.size() - k_slot_prefix.size());
                const ConstantId dfc = module_.constants.add_string(
                    MIR_INVALID_ID, df.data(), static_cast<uint32_t>(df.size()));
                flag = builder_->alloca_frame(types_.bool_type(), dfc);
            }
        }
    }
    if (flag == MIR_INVALID_ID) {
        flag = builder_->alloca(types_.bool_type(), MIRStorageClass::Local);
    }
    // The root flag starts armed only when the declaration actually constructs
    // the value. An uninitialized bare declaration leaves it false, so an
    // overwrite's guarded drop does nothing until `mark_initialized` arms it.
    builder_->set_drop(flag, whole_value_initialized);
    destructibles_.push_back({place, flag, dtor, std::move(field_flags)});
}

void MIRLowerer::emit_member_dtor_cond(ValueId self_ptr, const chem::string_view& member,
                                       BaseType* mt, PlaceId flag) {
    std::string err;
    if (flag == MIR_INVALID_ID) {
        emit_member_dtor(self_ptr, member, mt, err);
        return;
    }
    const ValueId fv = builder_->load(flag, types_.bool_type());
    const BlockId do_b = builder_->create_block();
    const BlockId end_b = builder_->create_block();
    builder_->cond_br(fv, do_b, end_b);
    builder_->set_block(do_b);
    emit_member_dtor(self_ptr, member, mt, err);
    if (!builder_->current_block_terminated()) builder_->br(end_b);
    builder_->set_block(end_b);
}

PlaceId MIRLowerer::drop_flag_for_element(PlaceId place, uint64_t index) {
    for (auto& d : destructibles_) {
        if (d.place != place) continue;
        for (auto& f : d.fields) {
            if (f.array_index == static_cast<uint32_t>(index)) return f.flag;
        }
    }
    return MIR_INVALID_ID;
}

MIRLowerer::MIRDropField MIRLowerer::build_drop_field(const chem::string_view& name,
                                                      BaseType* type, int depth) {
    MIRDropField f;
    f.name = name;
    f.type = type;
    BaseType* canon = type ? type->canonical() : nullptr;
    ASTNode* node = canon ? canon->get_direct_linked_canonical_node() : nullptr;
    if (depth < 2 && canon && node && node->kind() == ASTNodeKind::StructDecl &&
        !canon->has_user_destructor()) {
        auto* sd = node->as_struct_def_unsafe();
        for (auto& inh : sd->inherited) {
            if (!member_requires_dtor(inh.type)) continue;
            auto* linked = inh.type->get_direct_linked_struct();
            if (!linked) continue;
            f.children.push_back(
                build_drop_field(linked->name_view(), inh.type, depth + 1)
            );
        }
        for (auto* var : sd->variables()) {
            if (!var || !var->known_type()) continue;
            if (!member_requires_dtor(var->known_type())) continue;
            f.children.push_back(
                build_drop_field(var->name, var->known_type(), depth + 1)
            );
        }
        if (!f.children.empty()) return f;
    }
    f.flag = builder_->alloca(types_.bool_type(), MIRStorageClass::Local);
    if (f.flag != MIR_INVALID_ID) builder_->set_drop(f.flag, false);
    return f;
}

void MIRLowerer::destroy_drop_fields(ValueId self_ptr,
                                     std::vector<MIRDropField>& fields) {
    for (auto& f : fields) {
        if (builder_->current_block_terminated()) break;
        if (!f.children.empty()) {
            const TypeId ft = types_.map(f.type);
            const ConstantId fc = module_.constants.add_string(
                MIR_INVALID_ID, f.name.data(), static_cast<uint32_t>(f.name.size())
            );
            const ValueId faddr = builder_->field_addr_ptr(self_ptr, fc, ft);
            destroy_drop_fields(faddr, f.children);
        } else if (f.flag != MIR_INVALID_ID) {
            emit_member_dtor_cond(self_ptr, f.name, f.type, f.flag);
            builder_->set_drop(f.flag, false);
        }
    }
}

void MIRLowerer::set_drop_fields(std::vector<MIRDropField>& fields, bool value) {
    for (auto& f : fields) {
        if (f.flag != MIR_INVALID_ID) builder_->set_drop(f.flag, value);
        set_drop_fields(f.children, value);
    }
}

MIRLowerer::MIRDropField* MIRLowerer::drop_field_for_path(
        PlaceId place, const std::vector<std::string>& path) {
    if (path.empty()) return nullptr;
    for (auto& d : destructibles_) {
        if (d.place != place) continue;
        std::vector<MIRDropField>* list = &d.fields;
        MIRDropField* node = nullptr;
        for (const std::string& name : path) {
            node = nullptr;
            const chem::string_view key(name.data(), name.size());
            for (auto& f : *list) {
                if (f.name == key) {
                    node = &f;
                    break;
                }
            }
            if (!node) return nullptr;
            list = &node->children;
        }
        return node;
    }
    return nullptr;
}

void MIRLowerer::mark_path_initialized(PlaceId place,
                                       const std::vector<std::string>& path) {
    MIRDropField* node = drop_field_for_path(place, path);
    if (!node) return;
    if (node->flag != MIR_INVALID_ID) builder_->set_drop(node->flag, true);
    set_drop_fields(node->children, true);
}

void MIRLowerer::mark_raw_escaped(PlaceId place) {
    for (auto& d : destructibles_) {
        if (d.place != place || d.fields.empty()) continue;
        d.raw_escaped = true;
        if (place < builder_->function().places.size()) {
            builder_->function().places[place].raw_escaped = true;
        }
        // The whole-value path takes over: arm the root flag at the escape
        // point (the raw pointer may write subobjects the flags don't see).
        builder_->set_drop(d.flag, true);
        return;
    }
}

void MIRLowerer::mark_member_initialized(PlaceId place) {
    if (place >= builder_->function().places.size()) return;
    const uint32_t path = builder_->function().places[place].move_path_id;
    if (path == MIR_INVALID_ID) return;
    // Only a value with no owned subobjects: a user destructor may read any
    // member, but a whole-value drop must not destroy uninitialized owned
    // fields.
    if (builder_->function().move_paths[path].first_child != MIR_INVALID_ID) return;
    for (auto& d : destructibles_) {
        if (d.place != place) continue;
        if (!d.fields.empty()) return;
        builder_->set_drop(d.flag, true);
        return;
    }
}

void MIRLowerer::destroy_field_wise(MIRDestructible& d) {
    if (d.fields.empty()) return;
    if (d.place >= builder_->function().places.size()) return;
    const TypeId pt = builder_->function().places[d.place].type;
    const ValueId self_ptr = builder_->address_of(
        d.place, types_.pointer_type(pt, true)
    );
    if (d.fields[0].array_index != MIR_INVALID_ID) {
        // Fixed-array elements are destroyed last-to-first, matching the
        // whole-array `drop` the emitter expands.
        const TypeId et = types_.map(d.fields[0].type);
        const TypeId eptr = types_.pointer_type(et, true);
        const TypeId ut = types_.u32_type();
        const SymbolId dtor = destructor_symbol(d.fields[0].type);
        if (dtor == MIR_INVALID_ID) return;
        for (size_t i = d.fields.size(); i-- > 0;) {
            MIRDropField& f = d.fields[i];
            if (f.flag == MIR_INVALID_ID) continue;
            if (builder_->current_block_terminated()) break;
            const ValueId fv = builder_->load(f.flag, types_.bool_type());
            const BlockId do_b = builder_->create_block();
            const BlockId end_b = builder_->create_block();
            builder_->cond_br(fv, do_b, end_b);
            builder_->set_block(do_b);
            const ValueId idx = builder_->const_int(
                ut, module_.constants.add_int(ut, f.array_index)
            );
            const ValueId addr = builder_->index_addr(d.place, idx, et);
            MIROperand arg = MIROperand::value(addr, eptr);
            builder_->call_scalar(dtor, types_.void_type(), &arg, 1);
            if (!builder_->current_block_terminated()) builder_->br(end_b);
            builder_->set_block(end_b);
            builder_->set_drop(f.flag, false);
        }
        return;
    }
    destroy_drop_fields(self_ptr, d.fields);
}

void MIRLowerer::maybe_register_call_temp(Value* v, MIRExprResult r) {
    if (r.kind != MIRExprKind::Place || !v) return;
    if (v->val_kind() == ValueKind::AccessChain) {
        auto* c = v->as_access_chain_unsafe();
        if (c->values.size() == 1) v = c->values[0];
    }
    if (v->val_kind() != ValueKind::FunctionCall) return;
    register_stmt_temp(static_cast<PlaceId>(r.id), v->getType());
}

void MIRLowerer::destroy_call_temp(Value* v, MIRExprResult r) {
    if (r.kind != MIRExprKind::Place || !v) return;
    if (v->val_kind() == ValueKind::AccessChain) {
        auto* c = v->as_access_chain_unsafe();
        if (c->values.size() == 1) v = c->values[0];
    }
    if (v->val_kind() != ValueKind::FunctionCall) return;
    register_stmt_temp(static_cast<PlaceId>(r.id), v->getType());
}

void MIRLowerer::register_stmt_temp(PlaceId place, BaseType* type) {
    if (place == MIR_INVALID_ID || !type) return;
    if (destructor_symbol(type) == MIR_INVALID_ID) return;
    for (auto& t : stmt_temps_) {
        if (t.first == place) return;
    }
    stmt_temps_.emplace_back(place, type);
}

void MIRLowerer::cancel_stmt_temp(PlaceId place) {
    for (size_t i = 0; i < stmt_temps_.size(); ++i) {
        if (stmt_temps_[i].first == place) {
            stmt_temps_.erase(stmt_temps_.begin() + static_cast<long>(i));
            return;
        }
    }
}

void MIRLowerer::flush_stmt_temps(size_t mark) {
    for (size_t i = mark; i < stmt_temps_.size(); ++i) {
        const SymbolId dtor = destructor_symbol(stmt_temps_[i].second);
        if (dtor == MIR_INVALID_ID) continue;
        builder_->destroy(stmt_temps_[i].first, dtor);
    }
    stmt_temps_.resize(mark);
}
MIRExprResult MIRLowerer::lower_arg_converted(Value* arg, BaseType* param_type,
                                              std::string& error) {
    BaseType* conv_type = param_type;
    if (param_type) {
        if (param_type->kind() == BaseTypeKind::Reference) {
            conv_type = param_type->as_reference_type()->type;
        } else if (param_type->kind() == BaseTypeKind::Pointer) {
            conv_type = param_type->as_pointer_type()->type;
        }
    }
    // an untyped constructor call for the parameter's own type
    // (`std::string_view("...")`): the callee identifier may not be linked, so
    // match its name against the parameter type and construct directly.
    if (conv_type && arg && arg->val_kind() == ValueKind::FunctionCall) {
        auto* acall = arg->as_func_call_unsafe();
        // a view, not a `std::string`: this runs for every call argument that is
        // itself a call, and the name is only compared below
        chem::string_view cname;
        bool have = false;
        if (acall->parent_val) {
            if (acall->parent_val->val_kind() == ValueKind::Identifier) {
                cname = acall->parent_val->as_identifier_unsafe()->value;
                have = true;
            } else if (acall->parent_val->val_kind() == ValueKind::AccessChain) {
                auto* ch = acall->parent_val->as_access_chain_unsafe();
                if (!ch->values.empty()) {
                    cname = member_name_view(ch->values.back());
                    have = !cname.empty();
                }
            }
        }
        ASTNode* ln = conv_type->get_direct_linked_canonical_node();
        if (!ln) ln = conv_type->get_direct_linked_node();
        if ((!ln || ln->kind() != ASTNodeKind::StructDecl) &&
            conv_type->kind() == BaseTypeKind::Generic) {
            if (auto* g = conv_type->as_generic_type()) {
                if (g->referenced) {
                    ln = g->referenced->get_direct_linked_canonical_node();
                    if (!ln) ln = g->referenced->get_direct_linked_node();
                }
            }
        }
        if (have && ln && ln->kind() == ASTNodeKind::StructDecl) {
            auto* sd = ln->as_struct_def_unsafe();
            if (sd->name_view() ==
                chem::string_view(cname.data(), static_cast<unsigned>(cname.size()))) {
                FunctionDeclaration* ctor = sd->constructor_func(acall->values);
                if (!ctor && acall->values.empty()) ctor = conv_type->get_def_constructor();
                const TypeId mt = types_.map(conv_type);
                const PlaceId tmp = builder_->alloca(mt, MIRStorageClass::Temporary);
                MIRArgScope _ag_scope(*this);
                std::vector<MIROperand>& cargs = _ag_scope.v;
                const bool ctor_self = ctor && ctor->get_self_param() != nullptr;
                if (!lower_call_args_for(ctor, ctor_self, acall->values, cargs, error)) {
                    return MIRExprResult::error();
                }
                if (ctor && !append_default_args(ctor, acall->values.size(), ctor_self, cargs,
                                                 error)) {
                    return MIRExprResult::error();
                }
                if (ctor && ctor->is_comptime() && comptime_eval_) {
                    Value* ev = comptime_eval_(acall, ctor);
                    if (ev) return lower_expr(ev, error);
                }
                if (ctor) {
                    const SymbolId sym = intern_function(ctor);
                    builder_->zero_init(tmp);
                    builder_->init(tmp, sym, cargs.data(), static_cast<uint32_t>(cargs.size()));
                }
                return MIRExprResult::place(tmp, mt);
            }
        }
    }
    if (conv_type) {
        // an untyped struct literal (`f({ a: 1, b: 2 })`) takes its type from the
        // expected parameter/return type
        if (arg && arg->val_kind() == ValueKind::StructValue) {
            BaseType* at = arg->getType();
            ASTNode* an = at ? at->get_direct_linked_canonical_node() : nullptr;
            if (!an) {
                BaseType* saved = expected_struct_type_;
                expected_struct_type_ = conv_type;
                MIRExprResult r = lower_expr(arg, error);
                expected_struct_type_ = saved;
                return r;
            }
        }
        // a lambda converted to a `std::function` is wrapped by the instance
        // type's comptime `make` constructor (fn ptr + closure data)
        CapturingFunctionType* cap = nullptr;
        if (conv_type) cap = conv_type->get_cap_func_type();
        if (!cap && arg && arg->val_kind() == ValueKind::LambdaFunc && arg->getType()) {
            cap = arg->getType()->get_cap_func_type();
        }
        // `std::function<T>` may resolve to the instance struct directly (a
        // `%capture` typealias instantiation), so also accept a struct target
        BaseType* cap_inst = cap ? const_cast<BaseType*>(cap->instance_type.getType()) : nullptr;
        if (!cap_inst && conv_type) {
            ASTNode* cln = conv_type->get_direct_linked_canonical_node();
            if (cln && cln->kind() == ASTNodeKind::StructDecl) {
                cap_inst = cln->as_struct_def_unsafe()->known_type();
            }
        }
        if (cap_inst && arg && arg->val_kind() == ValueKind::LambdaFunc) {
            BaseType* inst = cap_inst;
            FunctionDeclaration* imp = nullptr;
            ASTNode* in = inst ? inst->get_direct_linked_canonical_node() : nullptr;
            if (in && in->kind() == ASTNodeKind::StructDecl) {
                // the closure wrapper is the instance type's `make` constructor
                ASTNode* mk = in->as_struct_def_unsafe()->child("make");
                if (mk && mk->kind() == ASTNodeKind::FunctionDecl) {
                    imp = mk->as_function_unsafe();
                }
            }
            if (!imp && inst) imp = inst->implicit_constructor_for(arg);
            if (imp) {
                // an `@expand` lambda constructor is lowered at this site
                if (is_comptime_template_fn(imp)) {
                    std::vector<Value*> one{arg};
                    return inline_comptime_template(imp, MIRExprResult::void_result(), nullptr,
                                                    one, error);
                }
                if (imp->is_comptime() && comptime_ctor_eval_) {
                    Value* ev = comptime_ctor_eval_(imp, arg);
                    if (ev) return lower_expr(ev, error);
                }
                const TypeId mt = types_.map(inst);
                const PlaceId tmp = builder_->alloca(mt, MIRStorageClass::Temporary);
                MIRArgScope _ag_scope(*this);
                std::vector<MIROperand>& cargs = _ag_scope.v;
                std::vector<Value*> one{arg};
                if (!lower_call_args(one, cargs, error)) return MIRExprResult::error();
                const SymbolId sym = intern_function(imp);
                builder_->init(tmp, sym, cargs.data(), static_cast<uint32_t>(cargs.size()));
                return MIRExprResult::place(tmp, mt);
            }
        }
        FunctionDeclaration* imp = nullptr;
        ASTNode* ln = conv_type->get_direct_linked_canonical_node();
        if (ln && ln->kind() == ASTNodeKind::StructDecl) {
            imp = ln->as_struct_def_unsafe()->implicit_constructor_func(arg);
        }
        if (!imp) imp = conv_type->implicit_constructor_for(arg);
        if (imp) {
            // an `@expand` implicit constructor is lowered at this site; the
            // interpreter would otherwise return an eagerly-built struct value
            if (is_comptime_template_fn(imp)) {
                std::vector<Value*> one{arg};
                return inline_comptime_template(imp, MIRExprResult::void_result(), nullptr,
                                                one, error);
            }
            if (imp->is_comptime() && comptime_ctor_eval_) {
                Value* ev = comptime_ctor_eval_(imp, arg);
                if (ev) return lower_expr(ev, error);
            }
            const TypeId mt = types_.map(conv_type);
            const PlaceId tmp = builder_->alloca(mt, MIRStorageClass::Temporary);
            MIRArgScope _ag_scope(*this);
            std::vector<MIROperand>& cargs = _ag_scope.v;
            std::vector<Value*> one{arg};
            if (!lower_call_args(one, cargs, error)) return MIRExprResult::error();
            const SymbolId sym = intern_function(imp);
            builder_->init(tmp, sym, cargs.data(), static_cast<uint32_t>(cargs.size()));
            return MIRExprResult::place(tmp, mt);
        }
    }
    return lower_expr(arg, error);
}

bool MIRLowerer::push_call_arg(MIRExprResult r, std::vector<MIROperand>& args,
                               std::string& error) {
    if (r.kind == MIRExprKind::Place) {
        const PlaceId pid = static_cast<PlaceId>(r.id);
        const TypeId pt = builder_->function().places[pid].type;
        const MIRTypeRecord& prec = module_.types.get(pt);
        if (prec.kind == MIRTypeKind::Pointer || prec.kind == MIRTypeKind::Reference) {
            args.push_back(MIROperand::value(builder_->load(pid, pt), pt));
        } else if (prec.kind == MIRTypeKind::Array) {
            // arrays decay to a pointer to their first element
            const TypeId eptr = types_.pointer_type(prec.element, false);
            args.push_back(MIROperand::value(builder_->address_of(pid, eptr), eptr));
        } else if (abi::is_aggregate(module_, pt)) {
            const TypeId ptrt = types_.pointer_type(pt, false);
            args.push_back(MIROperand::value(builder_->address_of(pid, ptrt), ptrt));
        } else {
            // a typedef struct (e.g. a dynamic fat pointer) is passed by value
            args.push_back(MIROperand::value(builder_->load(pid, pt), pt));
        }
    } else if (r.kind == MIRExprKind::Address) {
        args.push_back(MIROperand::value(static_cast<ValueId>(r.id), r.type));
    } else if (r.kind == MIRExprKind::Void) {
        error = "void expression used as a call argument";
        return false;
    } else {
        args.push_back(MIROperand::value(static_cast<ValueId>(r.id), r.type));
    }
    return true;
}

bool MIRLowerer::append_default_args(FunctionDeclaration* fd, size_t provided,
                                     bool self_included, std::vector<MIROperand>& args,
                                     std::string& error) {
    if (!fd) return true;
    auto& params = fd->params;
    const size_t offset = self_included ? 1 : 0;
    for (size_t i = provided + offset; i < params.size(); ++i) {
        if (fd->isVariadic() && i + 1 == params.size()) break; // variadic tail
        Value* dv = params[i]->defValue;
        if (!dv) {
            error = "missing required argument '" +
                    std::string(params[i]->name.data(), params[i]->name.size()) + "'";
            return false;
        }
        MIRExprResult r = lower_arg_converted(dv, params[i]->type, error);
        if (!r.ok()) return false;
        if (!push_call_arg(r, args, error)) return false;
    }
    return true;
}

MIRExprResult MIRLowerer::lower_member_chain(Value* const* values, size_t count,
                                             std::string& error) {
    if (count == 0) {
        error = "empty member chain";
        return MIRExprResult::error();
    }
    MIRExprResult first = lower_expr(values[0], error);
    if (!first.ok()) return first;
    if (count == 1) return first;
    // a struct-returning call used as a member/index base owns a temporary. A
    // scalar field read destroys it immediately (statement-local); an aggregate
    // field address escapes, so it is kept alive until the enclosing scope ends.
    const bool base_is_call = (values[0]->val_kind() == ValueKind::FunctionCall);
    PlaceId cur_place = MIR_INVALID_ID;
    ValueId cur_ptr = MIR_NULL;
    bool have_ptr = false;
    if (first.kind == MIRExprKind::Place) {
        cur_place = static_cast<PlaceId>(first.id);
    } else if (first.kind == MIRExprKind::Value || first.kind == MIRExprKind::Address) {
        cur_ptr = static_cast<ValueId>(first.id);
        have_ptr = true;
    } else {
        error = "member access base is not a place or pointer";
        return MIRExprResult::error();
    }
    for (size_t i = 1; i < count; ++i) {
        std::string fname;
        if (!member_field_path(values[i - 1], values[i], fname, error)) return MIRExprResult::error();
        const ConstantId fc = module_.constants.add_string(
            MIR_INVALID_ID, fname.data(), static_cast<uint32_t>(fname.size()));
        const TypeId ft = types_.map(values[i]->getType());
        const bool last = (i + 1 == count);
        if (!have_ptr) {
            if (last) {
                if (abi::is_aggregate(module_, ft)) {
                    if (base_is_call) maybe_register_call_temp(values[0], first);
                    return MIRExprResult::address(builder_->field_addr(cur_place, fc, ft), ft);
                }
                const ValueId lv = builder_->field_load(cur_place, fc, ft);
                if (base_is_call) destroy_call_temp(values[0], first);
                return MIRExprResult::value(lv, ft);
            }
            const ValueId faddr = builder_->field_addr(cur_place, fc, ft);
            const MIRTypeRecord& ftr = module_.types.get(ft);
            cur_ptr = (ftr.kind == MIRTypeKind::Pointer ||
                       ftr.kind == MIRTypeKind::Reference)
                          ? builder_->load_indirect(faddr, ft)
                          : faddr;
            have_ptr = true;
        } else {
            if (last) {
                if (abi::is_aggregate(module_, ft)) {
                    if (base_is_call) maybe_register_call_temp(values[0], first);
                    return MIRExprResult::address(builder_->field_addr_ptr(cur_ptr, fc, ft), ft);
                }
                const ValueId lv = builder_->field_load_ptr(cur_ptr, fc, ft);
                if (base_is_call) destroy_call_temp(values[0], first);
                return MIRExprResult::value(lv, ft);
            }
            const ValueId faddr = builder_->field_addr_ptr(cur_ptr, fc, ft);
            const MIRTypeRecord& ftr = module_.types.get(ft);
            cur_ptr = (ftr.kind == MIRTypeKind::Pointer ||
                       ftr.kind == MIRTypeKind::Reference)
                          ? builder_->load_indirect(faddr, ft)
                          : faddr;
        }
    }
    error = "empty member chain";
    return MIRExprResult::error();
}

MIRExprResult MIRLowerer::lower_variant_payload(VariantCaseVariable* var, bool as_address,
                                                std::string& error) {
    if (!var || !var->member_param) {
        error = "invalid variant case variable";
        return MIRExprResult::error();
    }
    SwitchStatement* sw = var->parent();
    VariantMember* vm = var->member_param->parent();
    if (!sw || !sw->expression || !vm) {
        error = "variant case variable has no switch expression";
        return MIRExprResult::error();
    }
    MIRExprResult base = lower_expr(sw->expression, error);
    if (!base.ok()) return base;
    const std::string mname(vm->name.data(), vm->name.size());
    const std::string pname(var->member_param->name.data(), var->member_param->name.size());
    const std::string fname = mname + "." + pname;
    const ConstantId fc = module_.constants.add_string(
        MIR_INVALID_ID, fname.data(), static_cast<uint32_t>(fname.size()));
    const TypeId ft = types_.map(var->known_type());
    if (base.kind == MIRExprKind::Place) {
        const PlaceId bp = static_cast<PlaceId>(base.id);
        if (as_address || abi::is_aggregate(module_, ft)) {
            return MIRExprResult::address(builder_->field_addr(bp, fc, ft), ft);
        }
        return MIRExprResult::value(builder_->field_load(bp, fc, ft), ft);
    }
    if (base.kind == MIRExprKind::Address || base.kind == MIRExprKind::Value) {
        const ValueId bv = static_cast<ValueId>(base.id);
        if (as_address || abi::is_aggregate(module_, ft)) {
            return MIRExprResult::address(builder_->field_addr_ptr(bv, fc, ft), ft);
        }
        return MIRExprResult::value(builder_->field_load_ptr(bv, fc, ft), ft);
    }
    error = "variant case expression is not a variant lvalue";
    return MIRExprResult::error();
}

MIRExprResult MIRLowerer::lower_pattern_base(PatternMatchExpr* pm, std::string& error) {
    auto it = pattern_bases_.find(pm);
    if (it != pattern_bases_.end()) return it->second;
    MIRExprResult base = lower_expr(pm->expression, error);
    if (!base.ok()) return base;
    pattern_bases_[pm] = base;
    return base;
}

MIRExprResult MIRLowerer::lower_pattern_payload(PatternMatchExpr* pm, VariantMemberParam* param,
                                                bool as_address, std::string& error) {
    if (!pm || !pm->member || !param) {
        error = "invalid pattern match payload";
        return MIRExprResult::error();
    }
    MIRExprResult base = lower_pattern_base(pm, error);
    if (!base.ok()) return base;
    const chem::string_view mname = pm->member->name;
    const chem::string_view pname = param->name;
    std::string fname(mname.data(), mname.size());
    fname.push_back('.');
    fname.append(pname.data(), pname.size());
    const ConstantId fc = module_.constants.add_string(
        MIR_INVALID_ID, fname.data(), static_cast<uint32_t>(fname.size()));
    const TypeId ft = types_.map(param->type);
    if (base.kind == MIRExprKind::Place) {
        const PlaceId bp = static_cast<PlaceId>(base.id);
        if (as_address || abi::is_aggregate(module_, ft)) {
            return MIRExprResult::address(builder_->field_addr(bp, fc, ft), ft);
        }
        return MIRExprResult::value(builder_->field_load(bp, fc, ft), ft);
    }
    if (base.kind == MIRExprKind::Address || base.kind == MIRExprKind::Value) {
        const ValueId bv = static_cast<ValueId>(base.id);
        if (as_address || abi::is_aggregate(module_, ft)) {
            return MIRExprResult::address(builder_->field_addr_ptr(bv, fc, ft), ft);
        }
        return MIRExprResult::value(builder_->field_load_ptr(bv, fc, ft), ft);
    }
    error = "pattern match expression is not a variant lvalue";
    return MIRExprResult::error();
}

MIRExprResult MIRLowerer::lower_pattern_cond(PatternMatchExpr* pm, std::string& error) {
    if (!pm || !pm->member) {
        error = "invalid pattern match condition";
        return MIRExprResult::error();
    }
    MIRExprResult base = lower_pattern_base(pm, error);
    if (!base.ok()) return base;
    const std::string tag_name("__chx__vt_621827");
    const ConstantId tag_fc = module_.constants.add_string(
        MIR_INVALID_ID, tag_name.data(), static_cast<uint32_t>(tag_name.size()));
    const TypeId it = types_.u32_type();
    ValueId tag = MIR_NULL;
    if (base.kind == MIRExprKind::Place) {
        tag = builder_->field_load(static_cast<PlaceId>(base.id), tag_fc, it);
    } else if (base.kind == MIRExprKind::Address || base.kind == MIRExprKind::Value) {
        tag = builder_->field_load_ptr(static_cast<ValueId>(base.id), tag_fc, it);
    } else {
        error = "pattern match expression is not a variant lvalue";
        return MIRExprResult::error();
    }
    VariantDefinition* vd = pm->member->parent();
    const long idx = vd ? vd->direct_child_index(pm->member->name) : 0;
    const ValueId c = builder_->const_int(
        it, module_.constants.add_int(it, static_cast<uint64_t>(idx < 0 ? 0 : idx)));
    const TypeId bool_t = types_.bool_type();
    const ConstantId pred = module_.constants.add_int(
        MIR_INVALID_ID, static_cast<uint64_t>(MIRBinaryOp::Eq));
    return MIRExprResult::value(builder_->compare(tag, c, pred, bool_t), bool_t);
}

MIRExprResult MIRLowerer::lower_pattern_match_value(PatternMatchExpr* pm, std::string& error) {
    const PatternElseExprKind k = pm->elseExpression.kind;
    MIRExprResult cond = lower_pattern_cond(pm, error);
    if (!cond.ok()) return cond;

    // `if(var Some(x) = v)`: a plain condition; the payload is read through the
    // cached base when `x` is used in the body.
    if (k == PatternElseExprKind::None) return cond;

    auto store_into = [&](PlaceId dest, TypeId dest_t, MIRExprResult r) -> bool {
        if (r.kind == MIRExprKind::Place) {
            builder_->store(dest, builder_->load(static_cast<PlaceId>(r.id), dest_t));
        } else if (r.kind == MIRExprKind::Address) {
            builder_->store(dest, builder_->load_indirect(static_cast<ValueId>(r.id), dest_t));
        } else if (r.kind == MIRExprKind::Value) {
            builder_->store(dest, static_cast<ValueId>(r.id));
        } else {
            error = "pattern match operand does not produce a value";
            return false;
        }
        return true;
    };

    if (k == PatternElseExprKind::DefValue) {
        if (pm->param_names.empty() || !pm->param_names[0] || !pm->param_names[0]->member_param) {
            error = "invalid pattern match payload";
            return MIRExprResult::error();
        }
        const TypeId pt = types_.map(pm->param_names[0]->member_param->type);
        const PlaceId res = builder_->alloca(pt, MIRStorageClass::Temporary);
        const BlockId then_b = builder_->create_block();
        const BlockId else_b = builder_->create_block();
        const BlockId merge = builder_->create_block();
        builder_->cond_br(cond.id, then_b, else_b);
        builder_->set_block(then_b);
        MIRExprResult payload =
            lower_pattern_payload(pm, pm->param_names[0]->member_param, false, error);
        if (!payload.ok()) return payload;
        if (!store_into(res, pt, payload)) return MIRExprResult::error();
        if (!builder_->current_block_terminated()) builder_->br(merge);
        builder_->set_block(else_b);
        MIRExprResult def = lower_expr(pm->elseExpression.value, error);
        if (!def.ok()) return def;
        if (!store_into(res, pt, def)) return MIRExprResult::error();
        if (!builder_->current_block_terminated()) builder_->br(merge);
        builder_->set_block(merge);
        pattern_results_[pm] = MIRExprResult::place(res, pt);
        if (abi::is_aggregate(module_, pt)) return MIRExprResult::place(res, pt);
        return MIRExprResult::value(builder_->load(res, pt), pt);
    }

    // Return / Break / Continue / Unreachable: guard on the tag, then the payload
    const BlockId then_b = builder_->create_block();
    const BlockId else_b = builder_->create_block();
    builder_->cond_br(cond.id, then_b, else_b);
    builder_->set_block(else_b);
    switch (k) {
        case PatternElseExprKind::Return: {
            ReturnStatement rs(pm->elseExpression.value, nullptr, pm->encoded_location());
            if (!lower_stmt(&rs, error)) return MIRExprResult::error();
            break;
        }
        case PatternElseExprKind::Break: {
            if (break_targets_.empty()) {
                error = "pattern break outside a loop";
                return MIRExprResult::error();
            }
            if (!loop_destructible_marks_.empty()) {
                emit_drops_from(loop_destructible_marks_.back());
            }
            builder_->br(break_targets_.back());
            break;
        }
        case PatternElseExprKind::Continue: {
            if (continue_targets_.empty()) {
                error = "pattern continue outside a loop";
                return MIRExprResult::error();
            }
            if (!loop_destructible_marks_.empty()) {
                emit_drops_from(loop_destructible_marks_.back());
            }
            builder_->br(continue_targets_.back());
            break;
        }
        case PatternElseExprKind::Unreachable:
            builder_->unreachable();
            break;
        default:
            builder_->br(then_b);
            break;
    }
    builder_->set_block(then_b);
    VariantMemberParam* first =
        (!pm->param_names.empty() && pm->param_names[0]) ? pm->param_names[0]->member_param
                                                         : nullptr;
    return lower_pattern_payload(pm, first, false, error);
}

bool MIRLowerer::lower_pattern_match_node(PatternMatchExprNode* node, std::string& error) {
    MIRExprResult r = lower_pattern_match_value(&node->value, error);
    return r.ok();
}

MIRExprResult MIRLowerer::lower_pattern_identifier(PatternMatchIdentifier* id,
                                                   std::string& error) {
    PatternMatchExpr* pm = id->matchExpr;
    if (!pm) {
        error = "pattern identifier has no match expression";
        return MIRExprResult::error();
    }
    // a DefValue result temp holds only the first destructured parameter; the
    // other parameters read their own payload field
    VariantMemberParam* first =
        (!pm->param_names.empty() && pm->param_names[0]) ? pm->param_names[0]->member_param
                                                         : nullptr;
    if (first && id->member_param == first) {
        auto it = pattern_results_.find(pm);
        if (it != pattern_results_.end()) {
            MIRExprResult r = it->second;
            if (r.kind == MIRExprKind::Place) {
                const TypeId t = r.type;
                if (abi::is_aggregate(module_, t)) return r;
                return MIRExprResult::value(builder_->load(static_cast<PlaceId>(r.id), t), t);
            }
            return r;
        }
    }
    return lower_pattern_payload(pm, id->member_param, false, error);
}

MIRExprResult MIRLowerer::lower_receiver(const std::vector<Value*>& values, size_t count,
                                         std::string& error) {
    if (count == 1 && values[0]) {
        Value* v = values[0];
        if (v->val_kind() == ValueKind::AccessChain) {
            auto* c = v->as_access_chain_unsafe();
            if (c->values.size() == 1) v = c->values[0];
        }
        const ValueKind k = v->val_kind();
        if (k == ValueKind::DereferenceValue) {
            // the pointer itself is the receiver address
            MIRExprResult p = lower_expr(v->as_dereference_value_unsafe()->getValue(), error);
            if (!p.ok()) return p;
            return MIRExprResult::address(static_cast<ValueId>(p.id), p.type);
        }
        if (k == ValueKind::Identifier || k == ValueKind::AccessChain ||
            k == ValueKind::IndexOperator) {
            // an unqualified field receiver inside a method resolves through
            // `self` (handled by `lower_expr` as an implicit member access)
            if (k == ValueKind::Identifier) {
                auto* rid = v->as_identifier_unsafe();
                if (is_implicit_field_node(rid->linked) && current_self_param_) {
                    return lower_expr(v, error);
                }
            }
            // prefer the place (needed for `&self` scalar receivers)
            std::string perr;
            const PlaceId p = resolve_place_or_lower(v, perr);
            if (p != MIR_INVALID_ID) {
                const TypeId pt = builder_->function().places[p].type;
                return MIRExprResult::place(p, pt);
            }
            MIRExprResult a = lower_address_of(v, error);
            if (a.ok()) return a;
        }
        if (k == ValueKind::FunctionCall) {
            // a struct-returning call as a method receiver: keep its temporary
            // alive until the end of the statement (destroyed after the chain)
            MIRExprResult r = lower_expr(v, error);
            if (!r.ok()) return r;
            maybe_register_call_temp(v, r);
            return r;
        }
    }
    return lower_member_chain(values.data(), count, error);
}

MIRExprResult MIRLowerer::lower_method_call(MIRExprResult recv, BaseType* recv_type,
                                            FunctionCall* call, std::string& error) {
    ASTNode* lk = call->parent_val ? call->parent_val->linked_node() : nullptr;
    FunctionDeclaration* fd = lk ? lk->as_function() : nullptr;
    if (!fd && call->parent_val && call->parent_val->val_kind() == ValueKind::AccessChain) {
        auto* ch = call->parent_val->as_access_chain_unsafe();
        if (!ch->values.empty()) {
            std::string mname, merr;
            if (member_name(ch->values.back(), mname, merr)) {
                fd = resolve_method_fallback(recv_type, mname);
            }
        }
    }
    if (!fd) {
        error = "method call to unresolved function";
        return MIRExprResult::error();
    }
    if (!recv.ok()) return recv;

    // a comptime method must be evaluated before its arguments are lowered (the
    // arguments may be comptime-only values such as expressive strings)
    if (fd->is_comptime() && comptime_eval_) {
        if (is_comptime_template_fn(fd)) {
            return inline_comptime_template(fd, recv, call, call->values, error);
        }
        Value* evaluated = comptime_eval_(call, fd);
        if (!evaluated) {
            // a void comptime call whose evaluation produced no value (e.g. an
            // inline-asm fence) is a no-op
            if (is_void_type(module_, types_.map(fd->returnType))) {
                return MIRExprResult::void_result();
            }
            error = "comptime call to '" + fd->name_str() + "' could not be evaluated";
            return MIRExprResult::error();
        }
        return lower_expr(evaluated, error);
    }

    // dynamic (interface) dispatch: `((vt_t*) fat.second)->method(fat.first, args...)`
    if (recv_type && recv_type->canonical() &&
        recv_type->canonical()->kind() == BaseTypeKind::Dynamic) {
        auto* dt = recv_type->canonical()->as_dynamic_type_unsafe();
        BaseType* iface_type = dt ? dt->referenced : nullptr;
        ASTNode* iface_node =
            iface_type ? iface_type->get_direct_linked_canonical_node() : nullptr;
        if (!iface_node) {
            error = "dynamic receiver interface could not be resolved";
            return MIRExprResult::error();
        }
        std::string vt_name = "__chx_";
        {
            const std::string& mn = mangle(iface_node);
            if (mn.empty()) {
                const chem::string_view nv = iface_node->as_interface_def_unsafe()->name_view();
                vt_name.append(nv.data(), nv.size());
            } else {
                vt_name.append(mn.data(), mn.size());
            }
        }
        vt_name += "_vt_t";
        const ConstantId vt_c = module_.constants.add_string(
            MIR_INVALID_ID, vt_name.data(), static_cast<uint32_t>(vt_name.size()));
        const chem::string_view mv = fd->name_view();
        const std::string m_name(mv.data(), mv.size());
        const ConstantId m_c = module_.constants.add_string(
            MIR_INVALID_ID, m_name.data(), static_cast<uint32_t>(m_name.size()));
        ValueId fat = MIR_NULL;
        TypeId fat_t = recv.type;
        if (recv.kind == MIRExprKind::Place) {
            fat_t = builder_->function().places[recv.id].type;
            fat = builder_->load(recv.id, fat_t);
        } else if (recv.kind == MIRExprKind::Address) {
            // an lvalue address: the operand is a pointer to the fat pointer
            fat_t = types_.pointer_type(recv.type, true);
            fat = static_cast<ValueId>(recv.id);
        } else {
            fat = static_cast<ValueId>(recv.id);
        }
        const size_t cmark = call_temps_.size();
        MIRArgScope _dargs(*this);
        std::vector<MIROperand>& dargs = _dargs.v;
        if (!lower_call_args_for(fd, true, call->values, dargs, error)) {
            return MIRExprResult::error();
        }
        if (!append_default_args(fd, call->values.size(), true, dargs, error)) {
            return MIRExprResult::error();
        }
        const TypeId dtype = types_.map(call->getType());
        if (abi::is_aggregate(module_, dtype)) {
            error = "dynamic method returning an aggregate is not yet supported";
            return MIRExprResult::error();
        }
        const ValueId dv = builder_->dynamic_call(
            fat, fat_t, vt_c, m_c, dargs.data(), static_cast<uint32_t>(dargs.size()), dtype);
        destroy_call_temps_from(cmark, error);
        if (is_void_type(module_, dtype)) return MIRExprResult::void_result();
        return MIRExprResult::value(dv, dtype);
    }

    // Two-phase approximation: evaluate the call arguments before the
    // receiver borrow is activated. `v.push(v.len())` needs this; a mutable
    // borrow in the arguments is still seen as a conflicting live loan.
    const size_t cmark = call_temps_.size();
    MIRArgScope _arg_scope(*this);
    std::vector<MIROperand>& call_args = _arg_scope.v;
    if (!lower_call_args_for(fd, true, call->values, call_args, error)) {
        return MIRExprResult::error();
    }
    if (!append_default_args(fd, call->values.size(), true, call_args, error)) {
        return MIRExprResult::error();
    }
    MIRArgScope _recv_scope(*this);
    std::vector<MIROperand>& recv_args = _recv_scope.v;
    if (!append_receiver_arg(recv, recv_type, fd, recv_args, error)) {
        return MIRExprResult::error();
    }
    MIRArgScope _final_scope(*this);
    std::vector<MIROperand>& args = _final_scope.v;
    args.insert(args.end(), recv_args.begin(), recv_args.end());
    args.insert(args.end(), call_args.begin(), call_args.end());

    const SymbolId sym = intern_method_function(fd, recv_type);
    const TypeId type = types_.map(call->getType());
    return emit_resolved_call(sym, type, args, cmark, error);
}

bool MIRLowerer::append_receiver_arg(MIRExprResult recv, BaseType* recv_type,
                                     FunctionDeclaration* fd, std::vector<MIROperand>& args,
                                     std::string& error) {
    // the self parameter (methods and extension functions)
    FunctionParam* self = fd->get_self_param();
    if (!self && fd->isExtensionFn() && !fd->params.empty()) self = fd->params[0];
    BaseType* self_t = self ? self->type : nullptr;
    const bool by_value =
        !self_t || (self_t->kind() != BaseTypeKind::Reference &&
                    self_t->kind() != BaseTypeKind::Pointer);
    BaseType* x_type = nullptr;
    if (self_t) {
        if (self_t->kind() == BaseTypeKind::Reference) {
            x_type = self_t->as_reference_type()->type;
        } else if (self_t->kind() == BaseTypeKind::Pointer) {
            x_type = self_t->as_pointer_type()->type;
        }
    }

    // A reference receiver is a real borrow: the checker must see the loan
    // (autoref), not just an address. The receiver borrow is created after the
    // arguments (see lower_method_call) so shared argument borrows are
    // two-phase-compatible.
    auto push_receiver_address = [&](ValueId address, TypeId address_type) {
        if (self_t != nullptr && self_t->kind() == BaseTypeKind::Reference) {
            const TypeId ref_t = types_.map(self_t);
            if (ref_t != MIR_INVALID_ID) {
                const ConstantId region = module_.constants.add_int(
                    types_.u32_type(), fresh_region_id());
                const ValueId borrowed = builder_->borrow(
                    address, region, ref_t,
                    self_t->as_reference_type()->is_mutable);
                if (borrowed != MIR_INVALID_ID) {
                    args.push_back(MIROperand::value(borrowed, ref_t));
                    return;
                }
            }
        }
        args.push_back(MIROperand::value(address, address_type));
    };

    if (by_value) {
        if (recv.kind == MIRExprKind::Place) {
            const TypeId rt = builder_->function().places[recv.id].type;
            if (abi::is_aggregate(module_, rt)) {
                // an aggregate `self` is passed as a hidden pointer
                const TypeId ptrt = types_.pointer_type(rt, true);
                args.push_back(MIROperand::value(builder_->address_of(recv.id, ptrt), ptrt));
            } else {
                args.push_back(MIROperand::value(builder_->load(recv.id, rt), rt));
            }
        } else {
            args.push_back(MIROperand::value(static_cast<ValueId>(recv.id), recv.type));
        }
    } else if (recv.kind == MIRExprKind::Place) {
        const TypeId rt = builder_->function().places[recv.id].type;
        const MIRTypeRecord& rr = module_.types.get(rt);
        const bool is_ptr = (rr.kind == MIRTypeKind::Pointer ||
                             rr.kind == MIRTypeKind::Reference);
        // the receiver is already a pointer/reference whose pointee is the
        // method's target type (or a Chemical struct derived from it): pass the
        // pointer value so the C compiler performs the implicit derived->base
        // upcast. Otherwise (e.g. `&int` receiver for a `&&int` method) we must
        // take the address of the place instead.
        bool pass_pointer = false;
        if (is_ptr && x_type) {
            const TypeId xt = types_.map(x_type);
            if (rr.element == xt) {
                pass_pointer = true;
            } else if (recv_type) {
                BaseType* rc = recv_type->canonical();
                BaseType* recv_elem = nullptr;
                if (rc && rc->kind() == BaseTypeKind::Pointer) {
                    recv_elem = rc->as_pointer_type()->type;
                } else if (rc && rc->kind() == BaseTypeKind::Reference) {
                    recv_elem = rc->as_reference_type()->type;
                }
                if (recv_elem) {
                    ASTNode* en = recv_elem->get_direct_linked_canonical_node();
                    ASTNode* xn = x_type->get_direct_linked_canonical_node();
                    if (en && en->kind() == ASTNodeKind::StructDecl &&
                        xn && xn->kind() == ASTNodeKind::StructDecl) {
                        pass_pointer = struct_derives_from(en->as_struct_def_unsafe(),
                                                           xn->known_type());
                    }
                }
            }
        }
        if (pass_pointer) {
            args.push_back(MIROperand::value(builder_->load(recv.id, rt), rt));
        } else {
            const TypeId ptrt = types_.pointer_type(rt, true);
            push_receiver_address(builder_->address_of(recv.id, ptrt), ptrt);
        }
    } else if (recv.kind == MIRExprKind::Address) {
        // an lvalue address (field / element): already a pointer to the receiver
        const TypeId ptrt = types_.pointer_type(recv.type, true);
        push_receiver_address(static_cast<ValueId>(recv.id), ptrt);
    } else {
        // an rvalue: materialize a temporary and pass its address
        const MIRTypeRecord& rr = module_.types.get(recv.type);
        const bool is_ptr = (rr.kind == MIRTypeKind::Pointer ||
                             rr.kind == MIRTypeKind::Reference);
        if (is_ptr && x_type && rr.element == types_.map(x_type)) {
            args.push_back(MIROperand::value(static_cast<ValueId>(recv.id), recv.type));
        } else if (rr.kind == MIRTypeKind::Opaque && self) {
            // untyped receiver node: trust the resolved method's receiver type
            const TypeId st = types_.map(self->type);
            args.push_back(MIROperand::value(static_cast<ValueId>(recv.id), st));
        } else {
            const TypeId rt = recv.type;
            const PlaceId tmp = builder_->alloca(rt, MIRStorageClass::Temporary);
            builder_->store(tmp, static_cast<ValueId>(recv.id));
            const TypeId ptrt = types_.pointer_type(rt, true);
            push_receiver_address(builder_->address_of(tmp, ptrt), ptrt);
        }
    }
    return true;
}

MIRExprResult MIRLowerer::emit_resolved_call(SymbolId sym, TypeId ret_type,
                                             std::vector<MIROperand>& args, size_t cmark,
                                             std::string& error) {
    if (is_void_type(module_, ret_type)) {
        builder_->call_scalar(sym, ret_type, args.data(), static_cast<uint32_t>(args.size()));
        destroy_call_temps_from(cmark, error);
        return MIRExprResult::void_result();
    }
    if (abi::needs_sret(module_, ret_type)) {
        const PlaceId res = builder_->alloca(ret_type, MIRStorageClass::Temporary);
        if (res == MIR_INVALID_ID) {
            error = "failed to allocate call result place";
            return MIRExprResult::error();
        }
        builder_->call_sret(sym, res, args.data(), static_cast<uint32_t>(args.size()));
        destroy_call_temps_from(cmark, error);
        return MIRExprResult::place(res, ret_type);
    }
    const ValueId v =
        builder_->call_scalar(sym, ret_type, args.data(), static_cast<uint32_t>(args.size()));
    destroy_call_temps_from(cmark, error);
    return MIRExprResult::value(v, ret_type);
}

MIRExprResult MIRLowerer::lower_operator_overload(Value* lhs_v, MIRExprResult lhs, Value* rhs_v,
                                                  MIRExprResult rhs, const std::string& method,
                                                  BaseType* ret_ast, std::string& error) {
    if (!lhs.ok()) return lhs;
    if (!rhs.ok()) return rhs;
    BaseType* lhs_t = lhs_v ? lhs_v->getType() : nullptr;
    BaseType* rhs_t = rhs_v ? rhs_v->getType() : nullptr;
    FunctionDeclaration* fd = resolve_method_fallback(lhs_t, method);
    if (!fd && rhs_t) fd = resolve_method_fallback(rhs_t, method);
    if (!fd) {
        error = "could not resolve operator method '" + method + "'";
        return MIRExprResult::error();
    }
    // synthesize a call node so the method-call machinery (receiver + args +
    // implicit conversions) can be reused
    VariableIdentifier id(chem::string_view(method.data(), method.size()), nullptr, ZERO_LOC);
    id.linked = fd;
    FunctionCall call(&id, ret_ast, ZERO_LOC);
    call.values.push_back(rhs_v);
    return lower_method_call(lhs, lhs_t, &call, error);
}

MIRExprResult MIRLowerer::lower_unary_operator_overload(Value* operand_v, MIRExprResult operand,
                                                        const char* method, BaseType* ret_ast,
                                                        std::string& error) {
    if (!operand.ok()) return operand;
    BaseType* op_t = operand_v ? operand_v->getType() : nullptr;
    FunctionDeclaration* fd = resolve_method_fallback(op_t, method);
    if (!fd) {
        error = "could not resolve operator method '" + std::string(method) + "'";
        return MIRExprResult::error();
    }
    VariableIdentifier id(chem::string_view(method, std::strlen(method)), nullptr, ZERO_LOC);
    id.linked = fd;
    FunctionCall call(&id, ret_ast, ZERO_LOC);
    return lower_method_call(operand, op_t, &call, error);
}

SymbolId MIRLowerer::intern_global(VarInitStatement* vi) {
    auto it = global_symbols_.find(vi);
    if (it != global_symbols_.end()) return it->second;
    MIRSymbolRecord rec;
    rec.kind = MIRSymbolKind::Global;
    // Only the module whose files declare this global *defines* it. A global
    // reached from another module (symres links the identifier straight to the
    // declaring node) is a reference to that module's symbol: `External` here,
    // an LLVM declaration there. Emitting a definition in both objects is a
    // link error ("multiple definition of `submod_extern_globe_var'").
    const bool defined_here =
        !vi->is_extern() && owned_globals_.find(vi) != owned_globals_.end();
    rec.linkage = defined_here ? MIRLinkage::Internal : MIRLinkage::External;
    if (vi->getType()) {
        BaseType* gt = vi->getType();
        if (gt->kind() == BaseTypeKind::Array && !gt->as_array_type()->has_array_size()) {
            // an unsized array global is declared by the legacy as a C array and
            // decays to a pointer on use
            rec.type = types_.unsized_array_type(
                types_.map(gt->as_array_type()->known_child_type()));
        } else {
            rec.type = types_.map(gt);
        }
    }
    if (mangle(vi).empty()) {
        const chem::string_view nv = vi->name_view();
        mangle_buf_.assign(nv.data(), nv.size());
    }
    SymbolId s = module_.symbols.add(rec, mangle_buf_.data(),
                                     static_cast<uint32_t>(mangle_buf_.size()),
                                     mangle_buf_.data(), static_cast<uint32_t>(mangle_buf_.size()));
    global_symbols_[vi] = s;
    return s;
}

// ── global initializer constant expressions (action A3) ─────────────────────
//
// The target-neutral initializer tree (MIRConstExpr.h). This is the single AST
// walk that used to be duplicated by the C emitter (`const_value_expr`) and the
// LLVM emitter (`GlobalConstantLowerer`); both now render this tree instead.

SymbolId MIRLowerer::intern_named_global(const std::string& name) {
    auto it = named_globals_.find(name);
    if (it != named_globals_.end()) return it->second;
    MIRSymbolRecord rec;
    rec.kind = MIRSymbolKind::Global;
    rec.linkage = MIRLinkage::External;
    SymbolId s = module_.symbols.add(rec, name.data(), static_cast<uint32_t>(name.size()),
                                     name.data(), static_cast<uint32_t>(name.size()));
    named_globals_[name] = s;
    return s;
}

SymbolId MIRLowerer::intern_function(FunctionDeclaration* decl) {
    MIR_OP(intern);
    auto it = func_symbols_.find(decl);
    if (it != func_symbols_.end()) return it->second;

    // The declaration ABI (sret for struct-like returns, aggregates/arrays
    // passed by pointer, the static-interface erased self, the variadic tail
    // marker dropped) is the shared `mir_function_signature`, so an extern
    // (bodyless) function's LLVM prototype is exactly the one the C declaration
    // pass writes and LLVM call sites expect.
    TypeId ftype = mir_function_signature(types_, module_, decl);

    MIRSymbolRecord rec;
    rec.kind = MIRSymbolKind::Function;
    // A `public` (or `protected`) function is visible across modules, so its
    // definition must have external linkage; the C backend uses the same rule.
    // Only interning the *call* symbol is not enough: for a function that is
    // never visited by `mir_translate_after_declaration`'s declaration pass
    // (e.g. one nested inside a top-level `if`'s taken scope) this is the only
    // place its symbol — and therefore its linkage — is decided.
    rec.linkage = (decl->is_extern() || decl->is_linkage_public()) ? MIRLinkage::External
                                                                  : MIRLinkage::Internal;
    rec.has_body = !decl->is_extern();
    rec.type = ftype;

    // NOTE: mangled name is filled by the serial symbol builder (PR3b) using
    // NameMangler; MIR must not retain FunctionDeclaration pointers.
    if (mangle(decl).empty()) {
        const chem::string_view nv = decl->name_view();
        mangle_buf_.assign(nv.data(), nv.size());
    }
    SymbolId s = module_.symbols.add(rec, mangle_buf_.data(),
                                     static_cast<uint32_t>(mangle_buf_.size()),
                                     mangle_buf_.data(), static_cast<uint32_t>(mangle_buf_.size()));
    func_symbols_[decl] = s;
    // Signatures are built at interning time (before any body lowering) so a
    // callee's regions are available to its callers (S4.5).
    const uint32_t sig = intern_region_signature(decl);
    if (sig != MIR_INVALID_ID) module_.set_symbol_region_signature(s, sig);
    return s;
}

static chem::string_view region_lifetime_of(BaseType* type) {
    if (type == nullptr) return {};
    // Direct `&'a T` first; then aggregate/generic lifetime arguments such as
    // `Holder<'a>` or `View<'a, T>`. MIR types erase lifetimes, so the region
    // signature is the only carrier of the declared argument. A type with more
    // than one lifetime argument uses the first one until the full variance
    // model (S4.2b) lands.
    auto first_of = [](BaseType* t) -> chem::string_view {
        if (t == nullptr) return {};
        if (t->kind() == BaseTypeKind::Reference) {
            return t->as_reference_type()->lifetime;
        }
        if (t->kind() == BaseTypeKind::Generic) {
            for (chem::string_view lt : t->as_generic_type()->lifetime_args) {
                if (!lt.empty()) return lt;
            }
        }
        return {};
    };
    chem::string_view lt = first_of(type);
    if (!lt.empty()) return lt;
    return first_of(type->canonical());
}

uint32_t MIRLowerer::fresh_region_id() {
    if (!builder_) return MIR_INVALID_ID;
    MIRFunction& fn = builder_->function();
    fn.region_var_count += 1;
    return fn.region_var_count;
}

uint32_t MIRLowerer::intern_region_signature(FunctionDeclaration* decl) {
    if (!decl) return MIR_INVALID_ID;
    std::vector<std::string_view> lifetimes;
    if (decl->generic_parent) {
        for (chem::string_view name : decl->generic_parent->lifetime_params) {
            lifetimes.emplace_back(name.data(), name.size());
        }
    }
    // A method inside a generic struct/union/variant inherits the container's
    // lifetime parameters; the function declaration itself has no
    // `generic_parent` for those. They are the regions its `&self` fields use.
    {
        auto add_container_lifetime = [&](chem::string_view name) {
            if (name.empty()) return;
            const chem::string_view bare = strip_lifetime_quote(name);
            for (std::string_view existing : lifetimes) {
                if (chem::string_view(existing.data(), existing.size()) == bare) {
                    return;
                }
            }
            lifetimes.emplace_back(bare.data(), bare.size());
        };
        ASTNode* parent = decl->parent();
        if (parent != nullptr) {
            if (auto* container = parent->as_extendable_member_container()) {
                if (container->generic_parent != nullptr) {
                    for (chem::string_view name :
                         container->generic_parent->lifetime_params) {
                        add_container_lifetime(name);
                    }
                } else if (container->kind() == ASTNodeKind::StructDecl) {
                    for (chem::string_view name :
                         container->as_struct_def_unsafe()->lifetime_params) {
                        add_container_lifetime(name);
                    }
                }
            }
        }
    }
    std::vector<std::string_view> param_lifetimes;
    param_lifetimes.reserve(decl->params.size());
    for (FunctionParam* param : decl->params) {
        chem::string_view lt = region_lifetime_of(param ? param->type : nullptr);
        // `&self` is elided by the signature pass without writing a name on the
        // receiver; the region is the same one the elided output uses.
        if (lt.empty() && param &&
            (param->name_view() == "self" || param->name_view() == "this")) {
            lt = "elided";
        }
        param_lifetimes.emplace_back(lt.data(), lt.size());
    }
    chem::string_view return_lt = decl->return_lifetime;
    if (return_lt.empty()) return_lt = region_lifetime_of(decl->returnType);
    std::vector<std::pair<std::string_view, std::string_view>> outlives;
    if (decl->where_clause) {
        for (const LifetimeBound& bound : decl->where_clause->lifetime_bounds) {
            for (chem::string_view target : bound.bounds) {
                outlives.emplace_back(
                    std::string_view(bound.param_name.data(), bound.param_name.size()),
                    std::string_view(target.data(), target.size())
                );
            }
        }
    }
    MIRRegionSignature sig = make_region_signature(
        lifetimes, param_lifetimes,
        std::string_view(return_lt.data(), return_lt.size()),
        outlives
    );

    // S4.2b: an aggregate parameter can carry references in its fields. Their
    // declared lifetime is the aggregate's lifetime argument, not the
    // parameter's own region (an aggregate parameter has none). Record the
    // field name constant and the resolved region so MIRBorrowCheck can treat
    // `return p.field` exactly like a reference parameter return.
    VarianceAnalysis variance;
    sig.param_field_regions.resize(decl->params.size());
    for (size_t pi = 0; pi < decl->params.size(); ++pi) {
        FunctionParam* param = decl->params[pi];
        BaseType* pt = param ? param->type : nullptr;
        if (pt == nullptr) continue;
        BaseType* canonical = pt->canonical();
        if (canonical == nullptr) continue;
        BaseType* aggregate = canonical;
        if (canonical->kind() == BaseTypeKind::Reference) {
            // `&self` / `&Pair<'a, 'b>`: the fields belong to the referent.
            aggregate = canonical->as_reference_type_unsafe()->type;
        }
        if (aggregate == nullptr) continue;
        BaseType* agg_canonical = aggregate->canonical();
        if (agg_canonical == nullptr) continue;
        StructDefinition* sd = aggregate->get_direct_linked_struct();
        if (sd == nullptr) continue;
        // A generic instantiation can be a signature-only shallow copy whose
        // body is not finalized yet. The master declaration always has the
        // field list and the lifetime parameters, so use it for the mapping.
        StructDefinition* body = sd;
        if (sd->generic_parent != nullptr &&
            sd->generic_parent->kind() == ASTNodeKind::GenericStructDecl) {
            StructDefinition* master =
                sd->generic_parent->as_gen_struct_def_unsafe()->master_impl;
            if (master != nullptr && (sd->variables().empty() ||
                                      sd->lifetime_params.empty())) {
                body = master;
            }
        }
        std::vector<chem::string_view> decl_lifetimes = body->lifetime_params;
        if (decl_lifetimes.empty() && body->generic_parent != nullptr) {
            decl_lifetimes = body->generic_parent->lifetime_params;
        }
        if (decl_lifetimes.empty()) {
            continue;
        }        auto* gt = agg_canonical->kind() == BaseTypeKind::Generic
                       ? agg_canonical->as_generic_type_unsafe()
                       : (aggregate->kind() == BaseTypeKind::Generic
                              ? aggregate->as_generic_type_unsafe()
                              : nullptr);
        // Prefer the written lifetime arguments (`Pair<'a, 'b>`). A method's
        // `&self` often carries only the concrete/linked struct type, so fall
        // back to the declaration's own lifetime names: the method signature
        // already includes the container's lifetime parameters by name.
        std::vector<chem::string_view> actual_lifetimes;
        if (gt != nullptr && gt->lifetime_args.size() == decl_lifetimes.size()) {
            actual_lifetimes = gt->lifetime_args;
        } else {
            actual_lifetimes = decl_lifetimes;
        }
        if (actual_lifetimes.size() != decl_lifetimes.size()) continue;
        const TypeId tid = types_.map(aggregate);
        if (tid == MIR_INVALID_ID || tid >= module_.types.types.size()) continue;
        const MIRTypeRecord& rec = module_.types.types[tid];
        for (BaseDefMember* member : body->variables()) {
            auto* sm = member ? member->as_struct_member() : nullptr;
            if (sm == nullptr) continue;
            BaseType* ft = const_cast<BaseType*>(sm->type.getType());
            if (ft == nullptr) continue;
            const chem::string_view fname = sm->name_view();
            BaseType* ftc = ft->canonical();
            if (ftc == nullptr || ftc->kind() != BaseTypeKind::Reference) {
                // Only direct reference fields produce a reference value on a
                // load. Nested aggregates are followed in a later step.
                continue;
            }
            const std::vector<Variance> uses =
                variance.lifetime_uses(ftc, decl_lifetimes);
            uint32_t field_index = MIR_INVALID_ID;
            const std::string_view fname_sv(fname.data(), fname.size());
            for (uint32_t i = 0; i < rec.data_count; ++i) {
                const MIRFieldInfo* fi = module_.types.field_of(tid, i);
                if (fi != nullptr && module_.types.field_name_view(*fi) == fname_sv) {
                    field_index = i;
                    break;
                }
            }
            const ConstantId field_name = module_.constants.add_string(
                MIR_INVALID_ID, fname.data(), static_cast<uint32_t>(fname.size()));
            for (size_t li = 0; li < uses.size() && li < actual_lifetimes.size(); ++li) {
                if (uses[li] == Variance::Bivariant) continue;
                const chem::string_view actual = actual_lifetimes[li];
                const MIRRegionId region = region_id_for_lifetime(
                    lifetimes, std::string_view(actual.data(), actual.size()));
                if (region == MIR_INVALID_ID) continue;
                MIRParamFieldRegion fr;
                fr.field_index = field_index;
                fr.field_name = field_name;
                fr.region = region;
                fr.variance = static_cast<MIRVariance>(uses[li]);
                sig.param_field_regions[pi].push_back(fr);
            }
        }
    }
    return module_.add_region_signature(std::move(sig));
}

MIRExprResult MIRLowerer::lower_atomic_intrinsic(FunctionCall* call, FunctionDeclaration* fd,
                                                 std::string& error) {
    const AtomicIntrinsic kind = atomic_intrinsic_of(fd);
    const uint32_t n = static_cast<uint32_t>(call->values.size());

    // An ordering / scope / op selector: the interpreter has already folded it
    // (the library passes `llvm_mem_order(order)` / `scope_to_int(...)`, both
    // comptime calls captured into the enclosing %runtime_value's refs). LLVM
    // requires these to be compile-time constants, so a runtime value is an
    // error rather than a fallback.
    auto int_arg = [&](uint32_t i, int64_t& out) -> bool {
        if (i >= n) return false;
        Value* v = call->values[i];
        if (!v) return false;
        if (v->val_kind() == ValueKind::IntN) {
            out = static_cast<int64_t>(v->as_int_num_value_unsafe()->get_num_value());
            return true;
        }
        if (comptime_value_eval_) {
            Value* e = comptime_value_eval_(v);
            if (e && e->val_kind() == ValueKind::IntN) {
                out = static_cast<int64_t>(e->as_int_num_value_unsafe()->get_num_value());
                return true;
            }
        }
        return false;
    };
    auto const_op = [&](int64_t bits) {
        const ConstantId c = module_.constants.add_int(MIR_INVALID_ID,
                                                       static_cast<uint64_t>(bits));
        return MIROperand::constant(c, MIR_INVALID_ID);
    };

    // Lower an argument to a scalar MIR operand. Pointers stay pointers; a
    // place that already holds the value is loaded so the emitter always sees
    // an SSA operand (MIR atomic ops take a value, not an address-of-a-local).
    auto scalar_arg = [&](uint32_t i, MIROperand& out) -> bool {
        if (i >= n) return false;
        MIRExprResult r = lower_expr(call->values[i], error);
        if (!r.ok()) return false;
        if (r.kind == MIRExprKind::Place) {
            if (r.type == MIR_INVALID_ID || r.type >= module_.types.size()) return false;
            out = MIROperand::value(builder_->load(static_cast<PlaceId>(r.id), r.type), r.type);
            return true;
        }
        if (r.kind == MIRExprKind::Value || r.kind == MIRExprKind::Address) {
            out = MIROperand::value(static_cast<ValueId>(r.id), r.type);
            return true;
        }
        error = "atomic intrinsic argument produced no value";
        return false;
    };
    // Lower the memory operand and resolve what it points at (the atomic width).
    auto pointer_arg = [&](uint32_t i, MIROperand& out, TypeId& pointee) -> bool {
        if (!scalar_arg(i, out)) return false;
        const TypeId pt = out.type();
        if (pt == MIR_INVALID_ID || pt >= module_.types.size()) {
            error = "atomic intrinsic expects a pointer operand";
            return false;
        }
        const MIRTypeKind pk = module_.types.get(pt).kind;
        if (pk != MIRTypeKind::Pointer && pk != MIRTypeKind::Reference) {
            error = "atomic intrinsic expects a pointer operand";
            return false;
        }
        pointee = module_.types.get(pt).element;
        if (pointee == MIR_INVALID_ID || pointee >= module_.types.size()) {
            error = "atomic intrinsic pointer has no pointee type";
            return false;
        }
        return true;
    };

    MIRArgScope _ops_scope(*this);
    std::vector<MIROperand>& ops = _ops_scope.v;
    switch (kind) {
        case AtomicIntrinsic::Fence:
        case AtomicIntrinsic::SignalFence: {
            int64_t order = 0;
            int64_t scope = 0;
            if (!int_arg(0, order)) {
                error = "atomic fence ordering must be a compile-time constant";
                return MIRExprResult::error();
            }
            if (kind == AtomicIntrinsic::Fence && !int_arg(1, scope)) {
                error = "atomic fence scope must be a compile-time constant";
                return MIRExprResult::error();
            }
            ops.push_back(const_op(order));
            ops.push_back(const_op(scope));
            ops.push_back(const_op(kind == AtomicIntrinsic::SignalFence ? 1 : 0));
            builder_->emit(MIROpcode::AtomicFence, MIR_NULL, ops.data(),
                           static_cast<uint32_t>(ops.size()));
            return MIRExprResult::void_result();
        }
        case AtomicIntrinsic::Load: {
            MIROperand ptr;
            TypeId pointee = MIR_INVALID_ID;
            int64_t order = 0;
            int64_t scope = 0;
            if (!pointer_arg(0, ptr, pointee)) return MIRExprResult::error();
            if (!int_arg(1, order) || !int_arg(2, scope)) {
                error = "atomic load ordering must be a compile-time constant";
                return MIRExprResult::error();
            }
            ops = {ptr, const_op(order), const_op(scope)};
            const ValueId v = builder_->new_value(pointee);
            builder_->emit(MIROpcode::AtomicLoad, v, ops.data(),
                           static_cast<uint32_t>(ops.size()));
            return MIRExprResult::value(v, pointee);
        }
        case AtomicIntrinsic::Store: {
            MIROperand ptr;
            TypeId pointee = MIR_INVALID_ID;
            MIROperand value;
            int64_t order = 0;
            int64_t scope = 0;
            if (!pointer_arg(0, ptr, pointee)) return MIRExprResult::error();
            if (!scalar_arg(1, value)) return MIRExprResult::error();
            if (!int_arg(2, order) || !int_arg(3, scope)) {
                error = "atomic store ordering must be a compile-time constant";
                return MIRExprResult::error();
            }
            ops = {ptr, value, const_op(order), const_op(scope)};
            builder_->emit(MIROpcode::AtomicStore, MIR_NULL, ops.data(),
                           static_cast<uint32_t>(ops.size()));
            return MIRExprResult::void_result();
        }
        case AtomicIntrinsic::CmpXchgWeak:
        case AtomicIntrinsic::CmpXchgStrong: {
            MIROperand ptr;
            MIROperand expected;
            MIROperand value;
            TypeId pointee = MIR_INVALID_ID;
            int64_t success_order = 0;
            int64_t failure_order = 0;
            int64_t scope = 0;
            if (!pointer_arg(0, ptr, pointee)) return MIRExprResult::error();
            // `expected` is a pointer to the value to compare against; on
            // failure it receives the value that was actually there.
            if (!scalar_arg(1, expected)) return MIRExprResult::error();
            if (!scalar_arg(2, value)) return MIRExprResult::error();
            if (!int_arg(3, success_order) || !int_arg(4, failure_order) ||
                !int_arg(5, scope)) {
                error = "atomic compare-exchange ordering must be a compile-time constant";
                return MIRExprResult::error();
            }
            ops = {ptr, expected, value, const_op(success_order), const_op(failure_order),
                   const_op(scope),
                   const_op(kind == AtomicIntrinsic::CmpXchgWeak ? 1 : 0)};
            const TypeId bool_t = types_.bool_type();
            const ValueId v = builder_->new_value(bool_t);
            builder_->emit(MIROpcode::AtomicCmpXchg, v, ops.data(),
                           static_cast<uint32_t>(ops.size()));
            return MIRExprResult::value(v, bool_t);
        }
        case AtomicIntrinsic::Rmw: {
            int64_t op = 0;
            MIROperand ptr;
            MIROperand value;
            TypeId pointee = MIR_INVALID_ID;
            int64_t order = 0;
            int64_t scope = 0;
            if (!int_arg(0, op)) {
                error = "atomic operation must be a compile-time constant";
                return MIRExprResult::error();
            }
            if (!pointer_arg(1, ptr, pointee)) return MIRExprResult::error();
            if (!scalar_arg(2, value)) return MIRExprResult::error();
            if (!int_arg(3, order) || !int_arg(4, scope)) {
                error = "atomic operation ordering must be a compile-time constant";
                return MIRExprResult::error();
            }
            ops = {const_op(op), ptr, value, const_op(order), const_op(scope)};
            const ValueId v = builder_->new_value(pointee);
            builder_->emit(MIROpcode::AtomicRMW, v, ops.data(),
                           static_cast<uint32_t>(ops.size()));
            return MIRExprResult::value(v, pointee);
        }
        case AtomicIntrinsic::None:
            break;
    }
    error = "unrecognized atomic intrinsic '" + fd->name_str() + "'";
    return MIRExprResult::error();
}


} // namespace mir
