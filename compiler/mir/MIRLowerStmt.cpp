// Copyright (c) Chemical Language Foundation 2025.
//
// MIR lowering: statement and control-flow lowering.
// Split out of MIRLowerer.cpp (action D4, mir-migration-evaluation.md).
// AST-aware: this is the front-end lowering boundary, never a backend.

#include "MIRLowererHelpers.h"

namespace mir {

bool MIRLowerer::lower_asm_operand(Value* expr, MIROperand& out, std::string& error) {
    Value* t = expr;
    if (t && t->val_kind() == ValueKind::AccessChain) {
        auto* c = t->as_access_chain_unsafe();
        if (c->values.size() == 1) t = c->values[0];
    }
    if (!t) {
        error = "inline asm operand is null";
        return false;
    }
    // a memory operand `*ptr`: keep the pointer value; the emitter spells `*<ptr>`
    if (t->val_kind() == ValueKind::DereferenceValue) {
        auto* d = t->as_dereference_value_unsafe();
        MIRExprResult p = lower_expr(d->getValue(), error);
        if (!p.ok()) return false;
        if (p.kind != MIRExprKind::Value && p.kind != MIRExprKind::Address) {
            error = "inline asm dereference operand is not a pointer";
            return false;
        }
        out = MIROperand::value(static_cast<ValueId>(p.id), p.type);
        return true;
    }
    // a direct local / parameter / member lvalue
    std::string perr;
    const PlaceId place = resolve_place_or_lower(t, perr);
    if (place != MIR_INVALID_ID) {
        const TypeId pt = builder_->function().places[place].type;
        out = MIROperand::place(place, pt);
        return true;
    }
    // fall back to its address (field / index lvalues)
    MIRExprResult a = lower_address_of(t, error);
    if (a.ok() && (a.kind == MIRExprKind::Value || a.kind == MIRExprKind::Address)) {
        out = MIROperand::value(static_cast<ValueId>(a.id), a.type);
        return true;
    }
    error = "unsupported inline asm operand: " + perr;
    return false;
}

void MIRLowerer::note_source(ASTNode* node) {
    if (!builder_ || !source_loc_provider_ || !node) return;
    const uint64_t enc = node->encoded_location().encoded;
    if (enc == 0) return;
    MIRSourceLoc loc;
    if (!source_loc_provider_(enc, loc)) return;
    builder_->set_source(builder_->add_source(loc));
}

bool MIRLowerer::lower_stmt(ASTNode* node, std::string& error) {
    if (!node) return true;
    note_source(node);

    switch (node->kind()) {
        case ASTNodeKind::VarInitStmt: {
            auto* vi = node->as_var_init();
            TypeId vt = types_.map(vi->known_type());
            PlaceId place = MIR_INVALID_ID;
            if (async_) {
                const MIRNumberedName rf = async_resident_field(vi);
                if (!rf.empty()) {
                    const ConstantId fc = module_.constants.add_string(
                        MIR_INVALID_ID, rf.data(), static_cast<uint32_t>(rf.size()));
                    place = builder_->alloca_frame(vt, fc);
                }
            }
            if (place == MIR_INVALID_ID) place = builder_->alloca(vt, MIRStorageClass::Local);
            if (place == MIR_INVALID_ID) {
                error = "failed to allocate place for variable";
                return false;
            }
            bind(vi, place);
            bind_name(vi->name_view(), place);
            if (async_ && vi->value && vi->value->kind() == ValueKind::AwaitExpr) {
                auto it = async_site_index_.find(vi);
                if (it != async_site_index_.end()) {
                    async_result_places_[it->second] = place;
                    return async_lower_await_var_init(vi, it->second, error);
                }
                // no site: a transparent (non-suspending) await. Fall through to
                // the normal lowering, which handles `AwaitExpr` as its operand.
            }
            if (vi->known_type() || (vi->value && vi->value->getType())) {
                BaseType* vtype = vi->known_type();
                if (!vtype && vi->value) vtype = vi->value->getType();
                const MIRTypeRecord& vr = module_.types.get(vt);
                if (!(vr.flags & TF_TYPEDEF) && abi::is_aggregate(module_, vt)) {
                    // A declaration without an initializer may be built field
                    // by field (the destructible then tracks per-field flags).
                    register_destructible(place, vtype, vi->value != nullptr);
                }
            }
            if (vi->value) {
                // moving a destructible value out of another local/param
                {
                    Value* iv = vi->value;
                    if (iv->val_kind() == ValueKind::AccessChain) {
                        auto* c = iv->as_access_chain_unsafe();
                        if (c->values.size() == 1) iv = c->values[0];
                    }
                    if (iv->val_kind() == ValueKind::Identifier) {
                        auto* iid = iv->as_identifier_unsafe();
                        PlaceId src = place_for_linked(iid->linked);
                        if (src == MIR_INVALID_ID) src = place_for_name(iid->value);
                        if (src != MIR_INVALID_ID) mark_moved(src);
                    }
                }
                // convert to the declared type first (implicit constructors, e.g.
                // `var s : std::string = \`...\``)
                MIRExprResult r = vi->known_type()
                                      ? lower_arg_converted(vi->value, vi->known_type(), error)
                                      : lower_expr(vi->value, error);
                if (!r.ok()) return false;
                if (r.kind == MIRExprKind::Place) {
                    const PlaceId src = static_cast<PlaceId>(r.id);
                    const TypeId spt = builder_->function().places[src].type;
                    if (vt != MIR_INVALID_ID && vt < module_.types.size() &&
                        spt != MIR_INVALID_ID && spt < module_.types.size()) {
                        const MIRTypeRecord& dr = module_.types.get(vt);
                        const MIRTypeRecord& sr = module_.types.get(spt);
                        if ((dr.kind == MIRTypeKind::Pointer ||
                             dr.kind == MIRTypeKind::Reference) &&
                            sr.kind == MIRTypeKind::Array) {
                            // `const t = <array>` where `t` is a slice/pointer:
                            // decay the array to its first element. A move_init
                            // would copy the whole array into a pointer-sized
                            // slot and corrupt the frame.
                            const TypeId ptrt = types_.pointer_type(sr.element, true);
                            builder_->store(place, builder_->address_of(src, ptrt));
                            return true;
                        }
                    }
                    // aggregate initializer: move the temporary into the variable
                    builder_->move_init(place, src, vt);
                    return true;
                }
                if (r.kind == MIRExprKind::Address) {
                    // copy the pointee into the variable (e.g. `var d = *c`)
                    builder_->copy_ptr_to_place(place, static_cast<ValueId>(r.id));
                    return true;
                }
                if (r.kind != MIRExprKind::Value) {
                    error = "invalid initializer expression";
                    return false;
                }
                if (vt != MIR_INVALID_ID && vt < module_.types.size() &&
                    module_.types.get(vt).kind == MIRTypeKind::Array) {
                    // initialize an array from a pointer (e.g. a string literal):
                    // copy only the literal's bytes, leaving the rest zeroed
                    if (vi->value->val_kind() == ValueKind::String) {
                        auto* sv = vi->value->as_string_unsafe();
                        uint64_t n = static_cast<uint64_t>(sv->value.size()) + 1; // include NUL
                        const uint64_t arr_len = module_.types.get(vt).data_count;
                        if (arr_len != 0 && n > arr_len) n = arr_len;
                        // The literal only covers `n` bytes, so zero the tail of
                        // the destination explicitly (the alloca is no longer
                        // unconditionally zeroed; see evaluation §1.2 / action C1).
                        builder_->zero_init(place);
                        const TypeId ut = types_.u32_type();
                        const ValueId sz = builder_->const_int(
                            ut, module_.constants.add_int(ut, n));
                        builder_->memcpy_size(place, static_cast<ValueId>(r.id), sz);
                    } else {
                        builder_->copy_ptr_to_place(place, static_cast<ValueId>(r.id));
                    }
                    return true;
                }
                ValueId val = r.id;
                if (r.type != vt) val = builder_->cast(val, vt);
                builder_->store(place, val);
            }
            // Inside a comptime-template inline, a `const` local whose value is
            // computable at comptime must also be visible to nested comptime
            // calls (a nested comptime constructor, `intrinsics::size`, …) that
            // are evaluated through the interpret scope. Publish it by name so
            // both the identifier lowering and the interpreter's scope see the
            // folded value. `comptime_bindings_` is save/restored around the
            // inline, so this never leaks past the body.
            if (!inline_frames_.empty() && vi->is_const() && comptime_value_eval_ && vi->value) {
                if (Value* cv = comptime_value_eval_(vi->value)) {
                    const auto nm = vi->name_view();
                    set_comptime_binding(std::string(nm.data(), nm.size()), cv);
                }
            }
            return true;
        }
        case ASTNodeKind::AssignmentStmt: {
            auto* as = node->as_assignment();
            if (!as->lhs) {
                error = "assignment has no target";
                return false;
            }
            Value* lhs = as->lhs;
            // an unqualified field assignment inside a method resolves to
            // `self.field`; synthesize the member chain `self.field`
            std::unique_ptr<VariableIdentifier> implicit_self;
            std::unique_ptr<AccessChain> implicit_chain;
            if (lhs->val_kind() == ValueKind::Identifier && current_self_param_) {
                auto* lid = lhs->as_identifier_unsafe();
                if (is_implicit_field_node(lid->linked)) {
                    implicit_self = std::make_unique<VariableIdentifier>(
                        current_self_param_->name, current_self_param_->type, ZERO_LOC);
                    implicit_self->linked = current_self_param_;
                    std::vector<Value*> vals = {implicit_self.get(), lhs};
                    implicit_chain = std::make_unique<AccessChain>(
                        std::move(vals), lhs->getType(), ZERO_LOC);
                    lhs = implicit_chain.get();
                }
            }
            if (lhs->val_kind() == ValueKind::AccessChain) {
                auto* chain = lhs->as_access_chain_unsafe();
                if (chain->values.size() == 1) {
                    lhs = chain->values[0];
                } else if (chain->values.size() >= 2) {
                    const size_t n = chain->values.size();
                    // walk to the base of the last field
                    MIRExprResult first = lower_expr(chain->values[0], error);
                    if (!first.ok()) return false;
                    PlaceId cur_place = MIR_INVALID_ID;
                    ValueId cur_ptr = MIR_NULL;
                    bool have_ptr = false;
                    if (first.kind == MIRExprKind::Place) {
                        cur_place = static_cast<PlaceId>(first.id);
                    } else if (first.kind == MIRExprKind::Value ||
                               first.kind == MIRExprKind::Address) {
                        cur_ptr = static_cast<ValueId>(first.id);
                        have_ptr = true;
                    } else {
                        error = "field assignment base is not a place or pointer";
                        return false;
                    }
                    for (size_t i = 1; i + 1 < n; ++i) {
                        std::string fname;
                        if (!member_field_path(chain->values[i - 1], chain->values[i], fname, error)) return false;
                        const ConstantId fc = module_.constants.add_string(
                            MIR_INVALID_ID, fname.data(), static_cast<uint32_t>(fname.size()));
                        const TypeId ft = types_.map(chain->values[i]->getType());
                        const MIRTypeRecord& ftr = module_.types.get(ft);
                        const ValueId faddr = have_ptr
                                                  ? builder_->field_addr_ptr(cur_ptr, fc, ft)
                                                  : builder_->field_addr(cur_place, fc, ft);
                        // an intermediate pointer/reference field is dereferenced
                        // so the next field is reached through it (`a.b->c`)
                        cur_ptr = (ftr.kind == MIRTypeKind::Pointer ||
                                   ftr.kind == MIRTypeKind::Reference)
                                      ? builder_->load_indirect(faddr, ft)
                                      : faddr;
                        have_ptr = true;
                    }
                    std::string fname;
                    if (!member_field_path(chain->values[n - 2], chain->values.back(), fname, error)) return false;
                    const ConstantId fc = module_.constants.add_string(
                        MIR_INVALID_ID, fname.data(), static_cast<uint32_t>(fname.size()));
                    MIRExprResult r = lower_expr(as->value, error);
                    if (!r.ok()) return false;
                    if (r.kind == MIRExprKind::Value &&
                        as->assOp != Operation::Assignment) {
                        // compound assignment on a struct field: load, apply, store
                        MIRBinaryOp bop = MIRBinaryOp::Add;
                        if (!compound_binary(as->assOp, bop)) {
                            error = "unsupported compound field assignment operator";
                            return false;
                        }
                        const TypeId ft = types_.map(chain->values.back()->getType());
                        const ValueId cur = have_ptr ? builder_->field_load_ptr(cur_ptr, fc, ft)
                                                     : builder_->field_load(cur_place, fc, ft);
                        emit_compound_assignment_checks(bop, cur,
                                                        static_cast<ValueId>(r.id), ft);
                        const ConstantId opc = module_.constants.add_int(
                            MIR_INVALID_ID, static_cast<uint64_t>(bop));
                        if (abi::is_struct_like(module_, ft)) {
                            error = "compound field assignment operator overloading is not yet supported";
                            return false;
                        }
                        const ValueId nv = builder_->binary(cur, r.id, opc, ft);
                        if (!have_ptr) {
                            builder_->field_store(cur_place, fc, nv);
                        } else {
                            builder_->field_store_ptr(cur_ptr, fc, nv);
                        }
                        return true;
                    }
                    PlaceId fflag = MIR_INVALID_ID;
                    if (as->assOp == Operation::Assignment) {
                        // a simple assignment destroys the previous value of a
                        // destructible field before overwriting it
                        BaseType* fbt = chain->values.back()->getType();
                        const SymbolId fdtor = destructor_symbol(fbt);
                        fflag = (!have_ptr && cur_place != MIR_INVALID_ID)
                                    ? drop_flag_for_field(
                                          cur_place,
                                          chem::string_view(fname.data(), fname.size()))
                                    : MIR_INVALID_ID;
                        if (fdtor != MIR_INVALID_ID) {
                            const TypeId ft = types_.map(fbt);
                            const ValueId faddr =
                                have_ptr ? builder_->field_addr_ptr(cur_ptr, fc, ft)
                                         : builder_->field_addr(cur_place, fc, ft);
                            const TypeId ptrt = types_.pointer_type(ft, true);
                            MIROperand arg = MIROperand::value(faddr, ptrt);
                            if (fflag != MIR_INVALID_ID) {
                                // a field of a bare local may be uninitialized:
                                // only destroy the old value when it is live
                                const ValueId fv = builder_->load(fflag, types_.bool_type());
                                const BlockId do_b = builder_->create_block();
                                const BlockId end_b = builder_->create_block();
                                builder_->cond_br(fv, do_b, end_b);
                                builder_->set_block(do_b);
                                builder_->call_scalar(fdtor, types_.void_type(), &arg, 1);
                                builder_->br(end_b);
                                builder_->set_block(end_b);
                            } else {
                                builder_->call_scalar(fdtor, types_.void_type(), &arg, 1);
                            }
                        }
                    }
                    if (r.kind == MIRExprKind::Place) {
                        // a move from a local clears the source's drop flag so it
                        // isn't destroyed again after being moved into the field
                        {
                            Value* rv = as->value;
                            if (rv && rv->val_kind() == ValueKind::AccessChain) {
                                auto* c = rv->as_access_chain_unsafe();
                                if (c->values.size() == 1) rv = c->values[0];
                            }
                            if (rv && rv->val_kind() == ValueKind::Identifier) {
                                auto* rid = rv->as_identifier_unsafe();
                                PlaceId src = place_for_linked(rid->linked);
                                if (src == MIR_INVALID_ID) src = place_for_name(rid->value);
                                if (src != MIR_INVALID_ID) mark_moved(src);
                            }
                        }
                        if (!have_ptr) {
                            builder_->field_store_place(cur_place, fc,
                                                        static_cast<PlaceId>(r.id));
                        } else {
                            const TypeId vt = builder_->function().places[r.id].type;
                            const ValueId sptr = builder_->address_of(
                                static_cast<PlaceId>(r.id), types_.pointer_type(vt, false));
                            const ValueId sv = builder_->load_indirect(sptr, vt);
                            builder_->field_store_ptr(cur_ptr, fc, sv);
                        }
                        if (fflag != MIR_INVALID_ID) builder_->set_drop(fflag, true);
                        return true;
                    }
                    if (r.kind == MIRExprKind::Address) {
                        const TypeId ft2 = types_.map(chain->values.back()->getType());
                        const ValueId loaded =
                            builder_->load_indirect(static_cast<ValueId>(r.id), ft2);
                        if (!have_ptr) {
                            builder_->field_store(cur_place, fc, loaded);
                        } else {
                            builder_->field_store_ptr(cur_ptr, fc, loaded);
                        }
                        if (fflag != MIR_INVALID_ID) builder_->set_drop(fflag, true);
                        return true;
                    }
                    if (r.kind != MIRExprKind::Value) {
                        error = "aggregate field assignment not yet supported";
                        return false;
                    }
                    if (!have_ptr) {
                        builder_->field_store(cur_place, fc, r.id);
                    } else {
                        builder_->field_store_ptr(cur_ptr, fc, r.id);
                    }
                    if (fflag != MIR_INVALID_ID) builder_->set_drop(fflag, true);
                    return true;
                }
            }
            if (lhs->val_kind() == ValueKind::DereferenceValue) {
                auto* d = lhs->as_dereference_value_unsafe();
                MIRExprResult p = lower_expr(d->getValue(), error);
                if (!p.ok()) return false;
                MIRExprResult r = lower_expr(as->value, error);
                if (!r.ok()) return false;
                const TypeId et = types_.map(lhs->getType());
                if (r.kind == MIRExprKind::Place) {
                    builder_->emit(MIROpcode::Store, MIR_NULL,
                                   {MIROperand::value(static_cast<ValueId>(p.id), p.type),
                                    MIROperand::place(static_cast<PlaceId>(r.id), et)});
                    return true;
                }
                if (r.kind == MIRExprKind::Address) {
                    const ValueId loaded =
                        builder_->load_indirect(static_cast<ValueId>(r.id), et);
                    builder_->store_indirect(static_cast<ValueId>(p.id), loaded);
                    return true;
                }
                if (r.kind != MIRExprKind::Value) {
                    error = "aggregate dereference assignment not yet supported";
                    return false;
                }
                if (as->assOp != Operation::Assignment) {
                    MIRBinaryOp bop = MIRBinaryOp::Add;
                    if (!compound_binary(as->assOp, bop)) {
                        error = "unsupported compound dereference assignment operator";
                        return false;
                    }
                    const ConstantId opc = module_.constants.add_int(
                        MIR_INVALID_ID, static_cast<uint64_t>(bop));
                    const ValueId cur =
                        builder_->load_indirect(static_cast<ValueId>(p.id), et);
                    emit_compound_assignment_checks(bop, cur,
                                                    static_cast<ValueId>(r.id), et);
                    if (abi::is_struct_like(module_, et)) {
                        error = "compound dereference assignment operator overloading is not yet supported";
                        return false;
                    }
                    const ValueId nv = builder_->binary(cur, r.id, opc, et);
                    builder_->store_indirect(static_cast<ValueId>(p.id), nv);
                } else {
                    builder_->store_indirect(static_cast<ValueId>(p.id), r.id);
                }
                return true;
            }
            if (lhs->val_kind() == ValueKind::IndexOperator) {
                auto* io = lhs->as_index_op_unsafe();
                MIRExprResult base = lower_expr(io->parent_val, error);
                if (!base.ok()) return false;
                maybe_register_call_temp(io->parent_val, base);
                MIRExprResult idx = lower_expr(io->idx, error);
                if (!idx.ok()) return false;
                emit_array_index_bounds_check(base, idx);
                if (base.kind == MIRExprKind::Place) {
                    const TypeId bt = builder_->function().places[base.id].type;
                    if (abi::is_struct_like(module_, bt)) {
                        // `x[i] = v` on a struct uses the `IndexMut` method
                        BaseType* recv_t = io->parent_val ? io->parent_val->getType() : nullptr;
                        FunctionDeclaration* fd = resolve_method_fallback(recv_t, "index_mut");
                        if (!fd) fd = resolve_method_fallback(recv_t, "index");
                        if (!fd) {
                            error = "index operator overloading is not yet supported";
                            return false;
                        }
                        VariableIdentifier op_id(chem::string_view("index"), nullptr, ZERO_LOC);
                        op_id.linked = fd;
                        FunctionCall op_call(&op_id, fd->returnType, ZERO_LOC);
                        op_call.values.push_back(io->idx);
                        MIRExprResult ref = lower_method_call(base, recv_t, &op_call, error);
                        if (!ref.ok()) return false;
                        if (ref.kind != MIRExprKind::Value && ref.kind != MIRExprKind::Address) {
                            error = "index operator did not return an address";
                            return false;
                        }
                        MIRExprResult r = lower_expr(as->value, error);
                        if (!r.ok()) return false;
                        const TypeId et = types_.map(io->getType());
                        ValueId rv = MIR_NULL;
                        if (r.kind == MIRExprKind::Place) {
                            rv = builder_->load(static_cast<PlaceId>(r.id), et);
                        } else if (r.kind == MIRExprKind::Address) {
                            rv = builder_->load_indirect(static_cast<ValueId>(r.id), et);
                        } else if (r.kind == MIRExprKind::Value) {
                            rv = static_cast<ValueId>(r.id);
                        } else {
                            error = "index assignment value is not a value";
                            return false;
                        }
                        builder_->store_indirect(static_cast<ValueId>(ref.id), rv);
                        return true;
                    }
                }
                MIRExprResult r = lower_expr(as->value, error);
                if (!r.ok()) return false;
                const TypeId et = types_.map(io->getType());
                if (r.kind == MIRExprKind::Place) {
                    if (base.kind == MIRExprKind::Place) {
                        // The element now owns the source value, so its scope
                        // destructor must not run at exit.
                        mark_moved(static_cast<PlaceId>(r.id));
                        builder_->element_store_place(static_cast<PlaceId>(base.id),
                                                      static_cast<ValueId>(idx.id),
                                                      static_cast<PlaceId>(r.id));
                    } else {
                        const ValueId eaddr = builder_->index_addr_ptr(
                            static_cast<ValueId>(base.id),
                            static_cast<ValueId>(idx.id), et);
                        // The element now owns the source value, so its scope
                        // destructor must not run at exit (same as the place
                        // branch above).
                        mark_moved(static_cast<PlaceId>(r.id));
                        builder_->emit(MIROpcode::Store, MIR_NULL,
                                       {MIROperand::value(eaddr, et),
                                        MIROperand::place(static_cast<PlaceId>(r.id), et)});
                    }
                    return true;
                }
                if (r.kind == MIRExprKind::Address) {
                    const ValueId loaded =
                        builder_->load_indirect(static_cast<ValueId>(r.id), et);
                    if (base.kind == MIRExprKind::Place) {
                        builder_->element_store(static_cast<PlaceId>(base.id),
                                                static_cast<ValueId>(idx.id), loaded);
                    } else {
                        builder_->index_store(static_cast<ValueId>(base.id),
                                              static_cast<ValueId>(idx.id), loaded);
                    }
                    return true;
                }
                if (r.kind != MIRExprKind::Value) {
                    error = "aggregate index assignment not yet supported";
                    return false;
                }
                const bool compound = (as->assOp != Operation::Assignment);
                if (compound && abi::is_struct_like(module_, et)) {
                    error = "compound index assignment operator overloading is not yet supported";
                    return false;
                }
                ConstantId opc = MIR_INVALID_ID;
                MIRBinaryOp bop = MIRBinaryOp::Add;
                if (compound) {
                    if (!compound_binary(as->assOp, bop)) {
                        error = "unsupported compound index assignment operator";
                        return false;
                    }
                    opc = module_.constants.add_int(MIR_INVALID_ID,
                                                   static_cast<uint64_t>(bop));
                }
                if (base.kind == MIRExprKind::Place) {
                    const PlaceId bp = static_cast<PlaceId>(base.id);
                    ValueId val = r.id;
                    if (compound) {
                        const ValueId cur =
                            builder_->element_load(bp, static_cast<ValueId>(idx.id), et);
                        emit_compound_assignment_checks(bop, cur,
                                                        static_cast<ValueId>(r.id), et);
                        val = builder_->binary(cur, r.id, opc, et);
                    }
                    builder_->element_store(bp, static_cast<ValueId>(idx.id), val);
                } else {
                    const ValueId bv = static_cast<ValueId>(base.id);
                    ValueId val = r.id;
                    if (compound) {
                        const ValueId cur =
                            builder_->index_load(bv, static_cast<ValueId>(idx.id), et);
                        emit_compound_assignment_checks(bop, cur,
                                                        static_cast<ValueId>(r.id), et);
                        val = builder_->binary(cur, r.id, opc, et);
                    }
                    builder_->index_store(bv, static_cast<ValueId>(idx.id), val);
                }
                return true;
            }
            if (lhs->val_kind() != ValueKind::Identifier) {
                error = "assignment target is not a simple local variable yet";
                return false;
            }
            auto* id = lhs->as_identifier_unsafe();
            PlaceId place = place_for_linked(id->linked);
            if (place == MIR_INVALID_ID && id->linked &&
                (id->linked->kind() == ASTNodeKind::PatternMatchId ||
                 id->linked->kind() == ASTNodeKind::VariantCaseVariable)) {
                // assignment to a destructured variant payload field
                MIRExprResult addr = lower_address_of(lhs, error);
                if (!addr.ok()) return false;
                if (addr.kind != MIRExprKind::Value && addr.kind != MIRExprKind::Address) {
                    error = "variant payload assignment target is not addressable";
                    return false;
                }
                MIRExprResult r = lower_expr(as->value, error);
                if (!r.ok()) return false;
                const TypeId et = types_.map(id->getType());
                ValueId rv = MIR_NULL;
                if (r.kind == MIRExprKind::Place) {
                    rv = builder_->load(static_cast<PlaceId>(r.id), et);
                } else if (r.kind == MIRExprKind::Address) {
                    rv = builder_->load_indirect(static_cast<ValueId>(r.id), et);
                } else if (r.kind == MIRExprKind::Value) {
                    rv = static_cast<ValueId>(r.id);
                } else {
                    error = "variant payload assignment value is not a value";
                    return false;
                }
                builder_->store_indirect(static_cast<ValueId>(addr.id), rv);
                return true;
            }
            if (place == MIR_INVALID_ID) {
                // store the RHS into a global, applying a compound operator
                // (`g += x`) by loading the current value first
                auto store_global = [&](ValueId addr, TypeId vt, MIRExprResult r) -> bool {
                    ValueId rv = MIR_NULL;
                    bool have = false;
                    if (r.kind == MIRExprKind::Place) {
                        rv = builder_->load(static_cast<PlaceId>(r.id), vt);
                        have = true;
                    } else if (r.kind == MIRExprKind::Address) {
                        const MIRTypeRecord& gtr = module_.types.get(vt);
                        if (gtr.kind == MIRTypeKind::Pointer ||
                            gtr.kind == MIRTypeKind::Reference) {
                            rv = static_cast<ValueId>(r.id);
                        } else {
                            rv = builder_->load_indirect(static_cast<ValueId>(r.id), vt);
                        }
                        have = true;
                    } else if (r.kind == MIRExprKind::Value) {
                        rv = static_cast<ValueId>(r.id);
                        have = true;
                    }
                    if (!have) {
                        error = "aggregate global assignment not yet supported";
                        return false;
                    }
                    if (as->assOp != Operation::Assignment) {
                        MIRBinaryOp bop = MIRBinaryOp::Add;
                        if (!compound_binary(as->assOp, bop)) {
                            error = "unsupported compound global assignment operator";
                            return false;
                        }
                        const ConstantId opc = module_.constants.add_int(
                            MIR_INVALID_ID, static_cast<uint64_t>(bop));
                        const ValueId cur = builder_->load_indirect(addr, vt);
                        emit_compound_assignment_checks(bop, cur, rv, vt);
                        const ValueId nv = builder_->binary(cur, rv, opc, vt);
                        builder_->store_indirect(addr, nv);
                        return true;
                    }
                    builder_->store_indirect(addr, rv);
                    return true;
                };
                if (id->linked && id->linked->kind() == ASTNodeKind::VarInitStmt) {
                    auto* vi = id->linked->as_var_init();
                    const SymbolId gs = intern_global(vi);
                    const TypeId vt = types_.map(id->getType());
                    const TypeId ptrt = types_.pointer_type(vt, true);
                    const ValueId addr = builder_->global_addr(gs, ptrt);
                    MIRExprResult r = lower_expr(as->value, error);
                    if (!r.ok()) return false;
                    return store_global(addr, vt, r);
                }
                const std::string gname(id->value.data(), id->value.size());
                if (!gname.empty()) {
                    const SymbolId gs = intern_named_global(gname);
                    const TypeId vt = types_.map(id->getType());
                    const TypeId ptrt = types_.pointer_type(vt, true);
                    const ValueId addr = builder_->global_addr(gs, ptrt);
                    MIRExprResult r = lower_expr(as->value, error);
                    if (!r.ok()) return false;
                    return store_global(addr, vt, r);
                }
                error = "assignment target does not resolve to a local place";
                return false;
            }
            const TypeId place_type = builder_->function().places[place].type;
            if (as->assOp != Operation::Assignment && abi::is_struct_like(module_, place_type)) {
                // `p += s` on a struct uses an overloaded `add_assign`-style method
                const char* method = compound_method_name(as->assOp);
                BaseType* lhs_ast = as->lhs ? as->lhs->getType() : nullptr;
                FunctionDeclaration* fd =
                    method ? resolve_method_fallback(lhs_ast, method) : nullptr;
                if (!fd) {
                    error = "compound assignment operator overloading is not yet supported";
                    return false;
                }
                VariableIdentifier op_id(
                    chem::string_view(method, std::strlen(method)), nullptr, ZERO_LOC);
                op_id.linked = fd;
                FunctionCall op_call(&op_id, fd->returnType, ZERO_LOC);
                op_call.values.push_back(as->value);
                MIRExprResult recv = MIRExprResult::place(place, place_type);
                MIRExprResult rr = lower_method_call(recv, lhs_ast, &op_call, error);
                return rr.ok();
            }
            // a move from a local clears the source's drop flag
            {
                Value* rv = as->value;
                if (rv && rv->val_kind() == ValueKind::AccessChain) {
                    auto* c = rv->as_access_chain_unsafe();
                    if (c->values.size() == 1) rv = c->values[0];
                }
                if (rv && rv->val_kind() == ValueKind::Identifier) {
                    auto* rid = rv->as_identifier_unsafe();
                    PlaceId src = place_for_linked(rid->linked);
                    if (src == MIR_INVALID_ID) src = place_for_name(rid->value);
                    if (src != MIR_INVALID_ID) mark_moved(src);
                }
            }
            // convert to the target's declared type first (implicit constructors,
            // e.g. `s = \`...\``)
            MIRExprResult r = as->lhs->getType()
                                  ? lower_arg_converted(as->value, as->lhs->getType(), error)
                                  : lower_expr(as->value, error);
            if (!r.ok()) return false;
            auto destroy_old_dest = [&]() {
                // a simple assignment destroys the previous value of a
                // destructible local before overwriting it
                if (as->assOp != Operation::Assignment || as->is_first_init) return;
                for (auto& d : destructibles_) {
                    if (d.place == place) {
                        if (!d.fields.empty()) {
                            destroy_field_wise(d);
                        } else {
                            const ValueId fv = builder_->load(d.flag, types_.bool_type());
                            builder_->drop(d.place, d.dtor, fv);
                        }
                        break;
                    }
                }
            };
            auto mark_initialized = [&]() {
                // the place now holds a live value again (e.g. after a move or a
                // re-initializing assignment), so re-arm its drop flag
                for (auto& d : destructibles_) {
                    if (d.place == place) {
                        builder_->set_drop(d.flag, true);
                        for (auto& f : d.fields) {
                            if (f.flag != MIR_INVALID_ID) builder_->set_drop(f.flag, true);
                        }
                        break;
                    }
                }
            };
            if (r.kind == MIRExprKind::Place) {
                const PlaceId src = static_cast<PlaceId>(r.id);
                const TypeId spt = builder_->function().places[src].type;
                if (place_type != MIR_INVALID_ID && place_type < module_.types.size() &&
                    spt != MIR_INVALID_ID && spt < module_.types.size()) {
                    const MIRTypeRecord& dr = module_.types.get(place_type);
                    const MIRTypeRecord& sr = module_.types.get(spt);
                    if ((dr.kind == MIRTypeKind::Pointer ||
                         dr.kind == MIRTypeKind::Reference) &&
                        sr.kind == MIRTypeKind::Array) {
                        // array-to-pointer decay on assignment (see the
                        // VarInitStmt path above); never copy the array into a
                        // pointer-sized destination.
                        destroy_old_dest();
                        const TypeId ptrt = types_.pointer_type(sr.element, true);
                        builder_->store(place, builder_->address_of(src, ptrt));
                        mark_initialized();
                        return true;
                    }
                }
                destroy_old_dest();
                builder_->emit(MIROpcode::Store, MIR_NULL,
                               {MIROperand::place(place, place_type),
                                MIROperand::place(static_cast<PlaceId>(r.id), place_type)});
                mark_initialized();
                return true;
            }
            if (r.kind == MIRExprKind::Address) {
                destroy_old_dest();
                const MIRTypeRecord& ptr = module_.types.get(place_type);
                if (ptr.kind == MIRTypeKind::Pointer || ptr.kind == MIRTypeKind::Reference) {
                    // the target is a pointer/reference: the address is the value
                    builder_->store(place, static_cast<ValueId>(r.id));
                } else if (ptr.kind == MIRTypeKind::Array) {
                    // arrays are not assignable in C: the emitter copies the
                    // source bytes from the address (a load would declare an
                    // array initialized from a dereference, which is invalid C)
                    builder_->emit(MIROpcode::Store, MIR_NULL,
                                   {MIROperand::place(place, place_type),
                                    MIROperand::value(static_cast<ValueId>(r.id), r.type)});
                } else {
                    const ValueId loaded =
                        builder_->load_indirect(static_cast<ValueId>(r.id), place_type);
                    builder_->store(place, loaded);
                }
                mark_initialized();
                return true;
            }
            if (r.kind != MIRExprKind::Value) {
                error = "aggregate assignment not yet supported";
                return false;
            }
            ValueId val = r.id;
            if (as->assOp != Operation::Assignment) {
                MIROpcode cop = MIROpcode::Nop;
                MIRBinaryOp bop = MIRBinaryOp::Add;
                if (!(map_binary(as->assOp, cop, bop) && cop == MIROpcode::Binary) &&
                    !compound_binary(as->assOp, bop)) {
                    error = "unsupported compound assignment operator (" +
                            std::to_string(static_cast<int>(as->assOp)) + ")";
                    return false;
                }
                const ValueId cur = builder_->load(place, place_type);
                emit_compound_assignment_checks(bop, cur, val, place_type);
                const ConstantId opc = module_.constants.add_int(
                    MIR_INVALID_ID, static_cast<uint64_t>(bop));
                val = builder_->binary(cur, val, opc, place_type);
            } else if (r.type != place_type) {
                val = builder_->cast(val, place_type);
            }
            builder_->store(place, val);
            mark_initialized();
            return true;
        }
        case ASTNodeKind::ReturnStmt: {
            auto* rs = node->as_return();
            if (async_) {
                const TypeId inner_t = types_.map(async_inner_);
                const ConstantId res_fc =
                    module_.constants.add_string(MIR_INVALID_ID, "__result", 8);
                if (async_in_ramp_) {
                    if (rs->value) {
                        MIRExprResult r = lower_expr(rs->value, error);
                        if (!r.ok()) return false;
                        ValueId v = (r.kind == MIRExprKind::Place)
                                        ? builder_->load(static_cast<PlaceId>(r.id), inner_t)
                                        : static_cast<ValueId>(r.id);
                        builder_->field_store_ptr(async_ramp_af_val_, res_fc, v);
                    }
                    emit_drops();
                    builder_->br(async_ramp_done_);
                    return true;
                }
                if (rs->value) {
                    MIRExprResult r = lower_expr(rs->value, error);
                    if (!r.ok()) return false;
                    ValueId v = (r.kind == MIRExprKind::Place)
                                    ? builder_->load(static_cast<PlaceId>(r.id), inner_t)
                                    : static_cast<ValueId>(r.id);
                    builder_->field_store_ptr(async_poll_af_val_, res_fc, v);
                }
                emit_drops();
                builder_->emit(
                    MIROpcode::AsyncFinish, MIR_NULL,
                    {MIROperand::place(async_poll_frame_, async_frame_ptr_type_),
                     MIROperand::place(async_poll_ret_,
                                       types_.pointer_type(types_.map(async_poll_base_), true)),
                     MIROperand::type(types_.map(async_poll_base_))});
                return true;
            }
            // a `return` inside an inlined comptime template branches to the
            // inline end block with the value, instead of returning the function
            if (!inline_frames_.empty()) {
                // Copy the frame: lowering the returned expression may inline a
                // nested `@expand` template, which push_back()s to
                // `inline_frames_` and can reallocate it. Holding a reference
                // across that call left `frame` dangling (garbage result_place).
                ComptimeInlineFrame frame = inline_frames_.back();
                MIRExprResult r = MIRExprResult::void_result();
                if (!frame.void_result && rs->value) {
                    r = lower_expr(rs->value, error);
                    if (!r.ok()) return false;
                }
                // a returned local destructible is *moved out* of the splice, not
                // dropped: clear its drop flag before the scope drops run (as the
                // non-inline return path does)
                if (!frame.void_result && rs->value) {
                    Value* rv = rs->value;
                    for (int guard = 0; guard < 4 && rv; ++guard) {
                        if (rv->val_kind() == ValueKind::AccessChain) {
                            auto* c = rv->as_access_chain_unsafe();
                            if (c->values.size() == 1) { rv = c->values[0]; continue; }
                            break;
                        }
                        if (rv->val_kind() == ValueKind::UnsafeValue) {
                            auto* u = static_cast<UnsafeValue*>(rv);
                            if (u->getValue()) { rv = u->getValue(); continue; }
                            break;
                        }
                        break;
                    }
                    if (rv->val_kind() == ValueKind::Identifier) {
                        auto* id = rv->as_identifier_unsafe();
                        PlaceId rp = place_for_linked(id->linked);
                        if (rp == MIR_INVALID_ID) rp = place_for_name(id->value);
                        if (rp != MIR_INVALID_ID) mark_moved(rp);
                    }
                }
                emit_drops_from(frame.dmark);
                if (!frame.void_result) {
                    const bool rsret = abi::needs_sret(module_, frame.result_type);
                    if (r.kind == MIRExprKind::Place) {
                        const PlaceId sp = static_cast<PlaceId>(r.id);
                        const TypeId pt = builder_->function().places[sp].type;
                        if (rsret && pt == frame.result_type) {
                            builder_->move_init(frame.result_place, sp, frame.result_type);
                        } else {
                            builder_->store(frame.result_place, builder_->load(sp, pt));
                        }
                    } else if (r.kind == MIRExprKind::Address) {
                        const ValueId av = static_cast<ValueId>(r.id);
                        if (rsret) {
                            const ValueId loaded = builder_->load_indirect(av, frame.result_type);
                            builder_->copy_init(frame.result_place, loaded, frame.result_type);
                        } else {
                            builder_->store(frame.result_place, av);
                        }
                    } else if (r.kind == MIRExprKind::Value) {
                        const ValueId vv = static_cast<ValueId>(r.id);
                        if (rsret) {
                            builder_->copy_init(frame.result_place, vv, frame.result_type);
                        } else {
                            builder_->store(frame.result_place, vv);
                        }
                    } else {
                        error = "comptime template returned a void value";
                        return false;
                    }
                }
                builder_->br(frame.end_block);
                return true;
            }
            // returning a local destructible moves it out (do not drop it here)
            if (rs->value) {
                Value* rv = rs->value;
                // unwrap `unsafe(...)`, a 1-element chain, etc. to find the moved local
                for (int guard = 0; guard < 4 && rv; ++guard) {
                    if (rv->val_kind() == ValueKind::AccessChain) {
                        auto* c = rv->as_access_chain_unsafe();
                        if (c->values.size() == 1) { rv = c->values[0]; continue; }
                        break;
                    }
                    if (rv->val_kind() == ValueKind::UnsafeValue) {
                        auto* u = static_cast<UnsafeValue*>(rv);
                        if (u->getValue()) { rv = u->getValue(); continue; }
                        break;
                    }
                    if (rv->val_kind() == ValueKind::FunctionCall) {
                        auto* fc = rv->as_func_call_unsafe();
                        std::string cn;
                        if (fc->parent_val && fc->parent_val->val_kind() == ValueKind::Identifier) {
                            auto* cid = fc->parent_val->as_identifier_unsafe();
                            cn.assign(cid->value.data(), cid->value.size());
                        }
                        if (cn == "unsafe" && fc->values.size() == 1) { rv = fc->values[0]; continue; }
                        break;
                    }
                    break;
                }
                if (rv->val_kind() == ValueKind::Identifier) {
                    auto* id = rv->as_identifier_unsafe();
                    PlaceId rp = place_for_linked(id->linked);
                    if (rp == MIR_INVALID_ID) rp = place_for_name(id->value);
                    if (rp != MIR_INVALID_ID) mark_moved(rp);
                }
            }
            if (!rs->value) {
                if (destructor_def_) emit_destructor_members(error);
                emit_drops();
                builder_->ret_void();
                return true;
            }
            MIRExprResult r;
            if (sret_) {
                // convert to the declared return type first (implicit constructors)
                r = lower_arg_converted(rs->value, return_type_, error);
            } else {
                r = lower_expr(rs->value, error);
            }
            if (!r.ok()) return false;
            emit_drops();
            if (r.kind == MIRExprKind::Void) {
                // `return void_call()` in a void function
                if (destructor_def_) emit_destructor_members(error);
                builder_->ret_void();
                return true;
            }
            if (sret_) {
                if (r.kind == MIRExprKind::Place) {
                    builder_->emit(MIROpcode::Store, MIR_NULL,
                                   {MIROperand::value(sret_ptr_, sret_ptr_type_),
                                    MIROperand::place(static_cast<PlaceId>(r.id), sret_ret_type_)});
                } else if (r.kind == MIRExprKind::Address) {
                    // `return &raw obj.field` / an aggregate address: copy the
                    // pointee into the sret destination
                    const ValueId loaded =
                        builder_->load_indirect(static_cast<ValueId>(r.id), sret_ret_type_);
                    builder_->emit(MIROpcode::Store, MIR_NULL,
                                   {MIROperand::value(sret_ptr_, sret_ptr_type_),
                                    MIROperand::value(loaded, sret_ret_type_)});
                } else if (r.kind == MIRExprKind::Value) {
                    builder_->emit(MIROpcode::Store, MIR_NULL,
                                   {MIROperand::value(sret_ptr_, sret_ptr_type_),
                                    MIROperand::value(static_cast<ValueId>(r.id), sret_ret_type_)});
                } else {
                    error = "returning an aggregate value is not yet supported";
                    return false;
                }
                builder_->ret_void();
                return true;
            }
            if (r.kind == MIRExprKind::Place) {
                // a by-value aggregate (typedef struct, e.g. a fat pointer) return
                const TypeId pt = builder_->function().places[r.id].type;
                builder_->ret(builder_->load(static_cast<PlaceId>(r.id), pt));
                return true;
            }
            if (r.kind != MIRExprKind::Value && r.kind != MIRExprKind::Address) {
                error = "returning a non-value expression is not yet supported";
                return false;
            }
            builder_->ret(r.id);
            return true;
        }
        case ASTNodeKind::EmbeddedNode: {
            auto* en = node->as_embedded_node_unsafe();
            ASTNode* repl = en->replacement;
            if (!repl && embedded_repl_) repl = embedded_repl_(en);
            if (repl) {
                en->replacement = repl;
                if (repl->kind() == ASTNodeKind::Scope) {
                    return lower_scope(*repl->as_scope_unsafe(), error);
                }
                return lower_stmt(repl, error);
            }
            error = "embedded node has no replacement";
            return false;
        }
        case ASTNodeKind::ValueWrapper: {
            auto* vw = node->as_value_wrapper();
            MIRExprResult r = lower_expr(vw->value, error);
            if (!r.ok()) return false;
            // a bare struct-returning call statement owns a temporary
            destroy_call_temp(vw->value, r);
            return true;
        }
        case ASTNodeKind::ValueNode: {
            auto* vn = node->as_value_node_unsafe();
            MIRExprResult r = lower_expr(vn->value, error);
            if (!r.ok()) return false;
            destroy_call_temp(vn->value, r);
            return true;
        }
        case ASTNodeKind::IfStmt: {
            auto* ifs = node->as_if_stmt_unsafe();
            // a condition the resolver proved constant (a `comptime if`, or a
            // condition such as `def.is64Bit`) carries its taken scope here
            if (ifs->computed_scope.has_value()) {
                Scope* taken = ifs->computed_scope.value();
                if (taken) return lower_scope(*taken, error);
                return true;
            }
            if (ifs->is_comptime()) {
                // compile-time condition: lower only the taken scope
                std::string rerr;
                Scope* taken = comptime_if_resolver_ ? comptime_if_resolver_(ifs, rerr) : nullptr;
                if (taken) return lower_scope(*taken, error);
                // if the condition cannot be resolved at comptime, fall back to
                // lowering it as a runtime branch (matches the legacy backend)
            }
            const BlockId merge = builder_->create_block();
            if (!lower_if(ifs, merge, error)) return false;
            builder_->set_block(merge);
            return true;
        }
        case ASTNodeKind::WhileLoopStmt:
            return lower_while(node->as_while_loop_unsafe(), error);
        case ASTNodeKind::DoWhileLoopStmt:
            return lower_do_while(node->as_do_while_loop_unsafe(), error);
        case ASTNodeKind::LoopBlock:
            return lower_loop_block(node->as_loop_block_unsafe(), error);
        case ASTNodeKind::ForLoopStmt:
            return lower_for(node->as_for_loop_unsafe(), error);
        case ASTNodeKind::ForInLoopStmt:
            return lower_for_in(node->as_for_in_loop_unsafe(), error);
        case ASTNodeKind::PlacementNewNode: {
            auto* pn = node->as_placement_new_node_unsafe();
            MIRExprResult r = lower_expr(&pn->value, error);
            return r.ok();
        }
        case ASTNodeKind::DeleteStmt: {
            // `delete ptr` / `destruct ptr` / `destruct[count] ptr`
            auto* ds = node->as_destruct_stmt_unsafe();
            BaseType* idt = ds->identifier ? ds->identifier->getType() : nullptr;
            BaseType* ct = idt ? idt->canonical() : nullptr;
            BaseType* elem_ast = nullptr;
            if (ct && ct->kind() == BaseTypeKind::Array) {
                elem_ast = ct->as_array_type_unsafe()->elem_type;
            } else if (ct && ct->kind() == BaseTypeKind::Pointer) {
                elem_ast = ct->as_pointer_type_unsafe()->type;
            }
            const SymbolId dtor = elem_ast ? destructor_symbol(elem_ast) : MIR_INVALID_ID;
            if (dtor == MIR_INVALID_ID && !ds->getFreeAfter()) return true;
            MIRExprResult p = lower_expr(ds->identifier, error);
            if (!p.ok()) return false;
            const ValueId ptr = (p.kind == MIRExprKind::Place)
                                    ? builder_->load(static_cast<PlaceId>(p.id), p.type)
                                    : static_cast<ValueId>(p.id);
            const TypeId vpt = types_.pointer_type(types_.void_type(), true);
            const TypeId void_t = types_.void_type();
            if (dtor != MIR_INVALID_ID) {
                const TypeId bool_t = types_.bool_type();
                const TypeId u64t = types_.u64_type();
                const ConstantId eq_op =
                    module_.constants.add_int(MIR_INVALID_ID,
                                              static_cast<uint64_t>(MIRBinaryOp::Eq));
                const ValueId null_v =
                    builder_->const_null(vpt, module_.constants.add_null(vpt));
                const ValueId is_null = builder_->compare(ptr, null_v, eq_op, bool_t);
                const BlockId nonnull_b = builder_->create_block();
                const BlockId done_b = builder_->create_block();
                builder_->cond_br(is_null, done_b, nonnull_b);
                builder_->set_block(nonnull_b);
                const TypeId eptr = types_.pointer_type(types_.map(elem_ast), true);
                if (ds->is_array) {
                    // `destruct[count] ptr`: destroy each element, last to first
                    const ConstantId add_op =
                        module_.constants.add_int(MIR_INVALID_ID,
                                                  static_cast<uint64_t>(MIRBinaryOp::Add));
                    const ConstantId sub_op =
                        module_.constants.add_int(MIR_INVALID_ID,
                                                  static_cast<uint64_t>(MIRBinaryOp::Sub));
                    const ConstantId gt_op =
                        module_.constants.add_int(MIR_INVALID_ID,
                                                  static_cast<uint64_t>(MIRBinaryOp::Gt));
                    ValueId count = builder_->const_int(
                        u64t, module_.constants.add_int(u64t, 0));
                    if (ds->array_value) {
                        MIRExprResult cv = lower_expr(ds->array_value, error);
                        if (!cv.ok()) return false;
                        count = (cv.kind == MIRExprKind::Place)
                                    ? builder_->load(static_cast<PlaceId>(cv.id), u64t)
                                    : static_cast<ValueId>(cv.id);
                    }
                    const PlaceId iv = builder_->alloca(u64t, MIRStorageClass::Local);
                    builder_->store(iv, count);
                    const BlockId head = builder_->create_block();
                    const BlockId body = builder_->create_block();
                    const BlockId loop_done = builder_->create_block();
                    builder_->br(head);
                    builder_->set_block(head);
                    const ValueId cur = builder_->load(iv, u64t);
                    const ValueId zero =
                        builder_->const_int(u64t, module_.constants.add_int(u64t, 0));
                    const ValueId gt = builder_->compare(cur, zero, gt_op, bool_t);
                    builder_->cond_br(gt, body, loop_done);
                    builder_->set_block(body);
                    const ValueId one =
                        builder_->const_int(u64t, module_.constants.add_int(u64t, 1));
                    const ValueId dec = builder_->binary(cur, one, sub_op, u64t);
                    builder_->store(iv, dec);
                    const ValueId ep = builder_->binary(ptr, dec, add_op, eptr);
                    MIROperand a = MIROperand::value(ep, eptr);
                    builder_->call_scalar(dtor, void_t, &a, 1);
                    builder_->br(head);
                    builder_->set_block(loop_done);
                } else {
                    MIROperand a = MIROperand::value(ptr, vpt);
                    builder_->call_scalar(dtor, void_t, &a, 1);
                }
                builder_->br(done_b);
                builder_->set_block(done_b);
            }
            if (ds->getFreeAfter()) emit_free(ptr);
            return true;
        }
        case ASTNodeKind::DeallocStmt: {
            auto* da = static_cast<DeallocStmt*>(node);
            MIRExprResult p = lower_expr(da->ptr, error);
            if (!p.ok()) return false;
            const ValueId ptr = (p.kind == MIRExprKind::Place)
                                    ? builder_->load(static_cast<PlaceId>(p.id), p.type)
                                    : static_cast<ValueId>(p.id);
            emit_free(ptr);
            return true;
        }
        case ASTNodeKind::BreakStmt: {
            if (break_targets_.empty()) {
                error = "break outside a loop";
                return false;
            }
            auto* bs = node->as_break_stmt_unsafe();
            if (bs->value && !break_value_dests_.empty() &&
                break_value_dests_.back() != MIR_INVALID_ID) {
                const PlaceId dest = break_value_dests_.back();
                const TypeId vt = break_value_types_.back();
                MIRExprResult r = lower_expr(bs->value, error);
                if (!r.ok()) return false;
                if (r.kind == MIRExprKind::Place) {
                    builder_->store(dest, builder_->load(static_cast<PlaceId>(r.id), vt));
                } else if (r.kind == MIRExprKind::Address) {
                    builder_->store(dest, builder_->load_indirect(static_cast<ValueId>(r.id), vt));
                } else if (r.kind == MIRExprKind::Value) {
                    builder_->store(dest, static_cast<ValueId>(r.id));
                } else {
                    error = "loop break value does not produce a value";
                    return false;
                }
            }
            if (!loop_destructible_marks_.empty()) {
                emit_drops_from(loop_destructible_marks_.back());
            }
            builder_->br(break_targets_.back());
            return true;
        }
        case ASTNodeKind::ContinueStmt: {
            if (continue_targets_.empty()) {
                error = "continue outside a loop";
                return false;
            }
            if (!loop_destructible_marks_.empty()) {
                emit_drops_from(loop_destructible_marks_.back());
            }
            builder_->br(continue_targets_.back());
            return true;
        }
        case ASTNodeKind::IncDecNode: {
            auto* n = node->as_inc_dec_node_unsafe();
            MIRExprResult r =
                lower_incdec_value(n->value.getValue(), n->value.increment, n->value.post, error);
            return r.ok();
        }
        case ASTNodeKind::AccessChainNode: {
            auto* acn = node->as_access_chain_node_unsafe();
            auto& vals = acn->chain.values;
            if (vals.size() == 2 && vals[1] && vals[1]->val_kind() == ValueKind::FunctionCall) {
                MIRExprResult recv = lower_receiver(vals, 1, error);
                if (!recv.ok()) return false;
                MIRExprResult r = lower_method_call(recv, vals[0]->getType(),
                                                    vals[1]->as_func_call_unsafe(), error);
                return r.ok();
            }
            MIRExprResult r = lower_expr(&acn->chain, error);
            if (!r.ok()) return false;
            // a bare call/access-chain expression statement owns a temporary
            if (acn->chain.values.size() == 1) {
                destroy_call_temp(acn->chain.values[0], r);
            }
            return true;
        }
        case ASTNodeKind::SwitchStmt: {
            auto* sw = node->as_switch_stmt_unsafe();
            const BlockId end_b = builder_->create_block();
            std::vector<BlockId> scope_blocks(sw->scopes.size());
            for (size_t i = 0; i < sw->scopes.size(); ++i) {
                scope_blocks[i] = builder_->create_block();
            }
            const BlockId default_b =
                (sw->defScopeInd >= 0) ? scope_blocks[sw->defScopeInd] : end_b;
            if (!lower_switch_dispatch(*sw, scope_blocks, default_b, error)) return false;

            for (size_t i = 0; i < sw->scopes.size(); ++i) {
                builder_->set_block(scope_blocks[i]);
                if (!lower_scope(sw->scopes[i], error)) return false;
                if (!builder_->current_block_terminated()) builder_->br(end_b);
            }
            builder_->set_block(end_b);
            return true;
        }
        case ASTNodeKind::PatternMatchExprNode:
            return lower_pattern_match_node(node->as_pattern_match_expr_node_unsafe(), error);
        case ASTNodeKind::Block: {
            auto* b = node->as_block_scope_unsafe();
            return lower_scope_nodes_scoped(b->nodes, node->encoded_location().encoded, error);
        }
        case ASTNodeKind::Scope: {
            auto* s = node->as_scope_unsafe();
            return lower_scope_nodes_scoped(s->nodes, s->encoded_location().encoded, error);
        }
        case ASTNodeKind::UnsafeBlock: {
            auto* u = node->as_unsafe_block_unsafe();
            return lower_scope_nodes_scoped(u->scope.nodes, node->encoded_location().encoded, error);
        }
        case ASTNodeKind::InlineAsmStmt: {
            auto* ia = node->as_inline_asm_unsafe();
            MIRArgScope _ops_scope(*this);
            std::vector<MIROperand>& ops = _ops_scope.v;
            const ConstantId tmpl = module_.constants.add_string(
                MIR_INVALID_ID, ia->asm_template.data(),
                static_cast<uint32_t>(ia->asm_template.size()));
            ops.push_back(MIROperand::constant(tmpl, MIR_INVALID_ID));
            auto count_const = [&](size_t n) {
                const ConstantId c =
                    module_.constants.add_int(MIR_INVALID_ID, static_cast<uint64_t>(n));
                return MIROperand::constant(c, MIR_INVALID_ID);
            };
            auto emit_section = [&](std::vector<AsmOperand>& list) -> bool {
                ops.push_back(count_const(list.size()));
                for (AsmOperand& op : list) {
                    const ConstantId cc = module_.constants.add_string(
                        MIR_INVALID_ID, op.constraint.data(),
                        static_cast<uint32_t>(op.constraint.size()));
                    ops.push_back(MIROperand::constant(cc, MIR_INVALID_ID));
                    MIROperand o;
                    if (!lower_asm_operand(op.expr, o, error)) return false;
                    ops.push_back(o);
                }
                return true;
            };
            if (!emit_section(ia->output_operands)) return false;
            if (!emit_section(ia->input_operands)) return false;
            ops.push_back(count_const(ia->clobbers.size()));
            for (chem::string_view& cl : ia->clobbers) {
                const ConstantId cc = module_.constants.add_string(
                    MIR_INVALID_ID, cl.data(), static_cast<uint32_t>(cl.size()));
                ops.push_back(MIROperand::constant(cc, MIR_INVALID_ID));
            }
            builder_->emit(MIROpcode::InlineAsm, MIR_NULL, ops.data(),
                           static_cast<uint32_t>(ops.size()));
            return true;
        }
        case ASTNodeKind::TypealiasStmt:
            // a local `type` alias has no runtime effect
            return true;
        case ASTNodeKind::ComptimeBlock:
            // the block is evaluated by the interpreter; it has no runtime effect
            if (comptime_block_eval_) comptime_block_eval_(node);
            return true;
        default:
            error = "unsupported statement kind during MIR lowering (kind " +
                    std::to_string(static_cast<int>(node->kind())) + ")";
            return false;
    }
}

MIRExprResult MIRLowerer::lower_incdec_value(Value* target, bool increment, bool post,
                                             std::string& error) {
    Value* t = target;
    if (t && t->val_kind() == ValueKind::AccessChain) {
        auto* c = t->as_access_chain_unsafe();
        if (c->values.size() == 1) t = c->values[0];
    }
    if (!t) {
        error = "increment/decrement target is null";
        return MIRExprResult::error();
    }
    const TypeId vt = types_.map(t->getType());
    if (abi::is_struct_like(module_, vt)) {
        // `p++` / `--p` on a struct uses the `Increment`/`Decrement` methods
        const char* method = increment ? (post ? "inc_post" : "inc_pre")
                                       : (post ? "dec_post" : "dec_pre");
        BaseType* ast_t = t->getType();
        FunctionDeclaration* fd = resolve_method_fallback(ast_t, method);
        if (!fd) {
            error = "increment/decrement operator overloading is not yet supported";
            return MIRExprResult::error();
        }
        MIRExprResult recv = lower_receiver(std::vector<Value*>{t}, 1, error);
        if (!recv.ok()) return recv;
        VariableIdentifier op_id(chem::string_view(method, std::strlen(method)), nullptr,
                                 ZERO_LOC);
        op_id.linked = fd;
        FunctionCall op_call(&op_id, fd->returnType, ZERO_LOC);
        return lower_method_call(recv, ast_t, &op_call, error);
    }
    const ConstantId opc = module_.constants.add_int(
        MIR_INVALID_ID, static_cast<uint64_t>(increment ? MIRBinaryOp::Add : MIRBinaryOp::Sub));
    const TypeId it = types_.u32_type();
    const ValueId one = builder_->const_int(it, module_.constants.add_int(it, 1));

    // a simple local / parameter
    if (t->val_kind() == ValueKind::Identifier) {
        auto* id = t->as_identifier_unsafe();
        if (id->linked && id->linked->kind() == ASTNodeKind::CapturedVariable) {
            auto* cap = id->linked->as_captured_var_unsafe();
            if (cap->capture_by_ref) {
                // a by-reference capture is a pointer to the enclosing local:
                // `count++` increments the referent, never the pointer itself
                MIRExprResult cr = lower_captured_variable(cap, false, error);
                if (!cr.ok()) return cr;
                const ValueId ptr = static_cast<ValueId>(cr.id);
                const MIRTypeRecord& rr = module_.types.get(cr.type);
                const TypeId et =
                    (rr.kind == MIRTypeKind::Reference || rr.kind == MIRTypeKind::Pointer)
                        ? rr.element
                        : cr.type;
                const ValueId cur = builder_->load_indirect(ptr, et);
                const ValueId nv = builder_->binary(cur, one, opc, et);
                builder_->store_indirect(ptr, nv);
                return MIRExprResult::value(post ? cur : nv, et);
            }
        }
        PlaceId place = place_for_linked(id->linked);
        if (place == MIR_INVALID_ID) place = place_for_name(id->value);
        if (place != MIR_INVALID_ID) {
            const MIRTypeRecord& pr = module_.types.get(vt);
            if (pr.kind == MIRTypeKind::Reference) {
                // `ref++` on a reference increments the referent
                const ValueId ptr = builder_->load(place, vt);
                const TypeId et = pr.element;
                const ValueId cur = builder_->load_indirect(ptr, et);
                const ValueId nv = builder_->binary(cur, one, opc, et);
                builder_->store_indirect(ptr, nv);
                return MIRExprResult::value(post ? cur : nv, vt);
            }
            const ValueId cur = builder_->load(place, vt);
            const ValueId nv = builder_->binary(cur, one, opc, vt);
            builder_->store(place, nv);
            // a post-increment/decrement expression yields the *old* value
            return MIRExprResult::value(post ? cur : nv, vt);
        }
    }

    // any other lvalue (global, field, element, `*ptr`): go through its address
    MIRExprResult addr;
    if (t->val_kind() == ValueKind::DereferenceValue) {
        addr = lower_expr(t->as_dereference_value_unsafe()->getValue(), error);
    } else {
        addr = lower_address_of(t, error);
    }
    if (!addr.ok() || (addr.kind != MIRExprKind::Value && addr.kind != MIRExprKind::Address)) {
        error = "increment/decrement target is not an addressable lvalue";
        return MIRExprResult::error();
    }
    const ValueId ptr = static_cast<ValueId>(addr.id);
    const ValueId cur = builder_->load_indirect(ptr, vt);
    const ValueId nv = builder_->binary(cur, one, opc, vt);
    builder_->store_indirect(ptr, nv);
    return MIRExprResult::value(post ? cur : nv, vt);
}

bool MIRLowerer::lower_scope(Scope& scope, std::string& error) {
    return lower_scope_nodes_scoped(scope.nodes, scope.encoded_location().encoded, error);
}

bool MIRLowerer::lower_scope_nodes_scoped(std::vector<ASTNode*>& nodes, uint64_t encoded_loc,
                                          std::string& error) {
    if (!debug_info_ || !builder_) return lower_scope_nodes(nodes, error);
    // The builder is captured here because lowering a nested function (a lambda)
    // swaps `builder_` for its own and restores it before returning; the matching
    // pop must go to the builder that recorded the push.
    MIRBuilder* const b = builder_;
    MIRSourceLoc loc{};
    const bool has_loc =
        encoded_loc != 0 && source_loc_provider_ && source_loc_provider_(encoded_loc, loc);
    b->push_lexical_scope(loc, has_loc);
    const bool ok = lower_scope_nodes(nodes, error);
    b->pop_lexical_scope();
    return ok;
}

bool MIRLowerer::lower_scope_nodes(std::vector<ASTNode*>& nodes, std::string& error) {
    const size_t mark = destructibles_.size();
    for (ASTNode* n : nodes) {
        // statements after a terminator (break/continue/return) are unreachable
        if (builder_->current_block_terminated()) break;
        const size_t smark = stmt_temps_.size();
        if (!lower_stmt(n, error)) return false;
        if (!builder_->current_block_terminated()) flush_stmt_temps(smark);
        else stmt_temps_.resize(smark);
    }
    // destroy locals declared in this scope (unless they were moved out)
    if (!builder_->current_block_terminated()) {
        emit_drops_from(mark);
    }
    destructibles_.resize(mark);
    return true;
}

bool MIRLowerer::lower_if(IfStatement* stmt, BlockId merge, std::string& error) {
    const BlockId then_b = builder_->create_block();
    const BlockId else_b = builder_->create_block();

    MIRExprResult c = lower_expr(stmt->condition, error);
    if (!c.ok()) return false;
    builder_->cond_br(c.id, then_b, else_b);

    builder_->set_block(then_b);
    if (!lower_scope(stmt->ifBody, error)) return false;
    if (!builder_->current_block_terminated()) builder_->br(merge);

    BlockId current = else_b;
    for (size_t i = 0; i < stmt->elseIfs.size(); ++i) {
        const BlockId body_b = builder_->create_block();
        const BlockId next_else = builder_->create_block();
        builder_->set_block(current);
        MIRExprResult ec = lower_expr(stmt->elseIfs[i].first, error);
        if (!ec.ok()) return false;
        builder_->cond_br(ec.id, body_b, next_else);
        builder_->set_block(body_b);
        if (!lower_scope(stmt->elseIfs[i].second, error)) return false;
        if (!builder_->current_block_terminated()) builder_->br(merge);
        current = next_else;
    }

    builder_->set_block(current);
    if (stmt->elseBody.has_value()) {
        if (!lower_scope(stmt->elseBody.value(), error)) return false;
    }
    if (!builder_->current_block_terminated()) builder_->br(merge);
    return true;
}

bool MIRLowerer::lower_scope_value(Scope& scope, PlaceId dest, TypeId dest_t,
                                   std::string& error) {
    // Debug info: this scope body is a lexical block (see lower_scope_nodes_scoped).
    // The builder is captured because lowering a lambda inside the body swaps and
    // restores `builder_`; the pop must reach the builder that pushed.
    MIRBuilder* const ls_builder = debug_info_ ? builder_ : nullptr;
    if (ls_builder) {
        MIRSourceLoc ls_loc{};
        const uint64_t enc = scope.encoded_location().encoded;
        const bool has_loc =
            enc != 0 && source_loc_provider_ && source_loc_provider_(enc, ls_loc);
        ls_builder->push_lexical_scope(ls_loc, has_loc);
    }
    const size_t mark = destructibles_.size();
    const size_t smark = stmt_temps_.size();
    auto finish = [&]() -> bool {
        if (!builder_->current_block_terminated()) {
            emit_drops_from(mark);
            flush_stmt_temps(smark);
        } else {
            stmt_temps_.resize(smark);
        }
        destructibles_.resize(mark);
        if (ls_builder) ls_builder->pop_lexical_scope();
        return true;
    };

    const size_t n = scope.nodes.size();
    for (size_t i = 0; i < n; ++i) {
        if (builder_->current_block_terminated()) break;
        ASTNode* node = scope.nodes[i];
        if (i + 1 == n) {
            // the trailing node of a value scope may be a statement-form
            // if/switch/loop that must be lowered in value position
            Value* v = nullptr;
            switch (node->kind()) {
                case ASTNodeKind::IfStmt:
                    return lower_if_stmt_into(*node->as_if_stmt_unsafe(), dest, dest_t, error) &&
                           finish();
                case ASTNodeKind::SwitchStmt:
                    return lower_switch_into(*node->as_switch_stmt_unsafe(), dest, dest_t, error) &&
                           finish();
                case ASTNodeKind::LoopBlock:
                    return lower_loop_block_into(*node->as_loop_block_unsafe(), dest, dest_t, error) &&
                           finish();
                case ASTNodeKind::AccessChainNode:
                    v = &node->as_access_chain_node_unsafe()->chain;
                    break;
                default:
                    v = Value::get_first_value_from_value_node(node);
                    break;
            }
            if (v) {
                // a moved branch value (a destructured variant payload) consumes
                // the source it was moved from
                if (v->is_ref_moved()) clear_moved_ref(v);
                MIRExprResult r = lower_expr(v, error);
                if (!r.ok()) return false;
                if (r.kind == MIRExprKind::Place) {
                    // the trailing local is moved out into the result, so its
                    // destructor must not run at scope exit
                    mark_moved(static_cast<PlaceId>(r.id));
                    builder_->store(dest, builder_->load(static_cast<PlaceId>(r.id), dest_t));
                } else if (r.kind == MIRExprKind::Address) {
                    builder_->store(dest, builder_->load_indirect(static_cast<ValueId>(r.id), dest_t));
                } else if (r.kind == MIRExprKind::Value) {
                    builder_->store(dest, static_cast<ValueId>(r.id));
                } else {
                    error = "if-value branch does not produce a value";
                    return false;
                }
                break;
            }
        }
        if (!lower_stmt(node, error)) return false;
    }
    return finish();
}

bool MIRLowerer::lower_switch_dispatch(SwitchStatement& sw,
                                       std::vector<BlockId>& scope_blocks,
                                       BlockId default_b, std::string& error) {
    MIRExprResult scrut = lower_expr(sw.expression, error);
    if (!scrut.ok()) return false;

    // variant switch: compare the discriminant, not the whole value
    const std::string tag_name("__chx__vt_621827");
    const ConstantId tag_fc = module_.constants.add_string(
        MIR_INVALID_ID, tag_name.data(), static_cast<uint32_t>(tag_name.size()));
    const TypeId tag_t = types_.u32_type();
    bool variant_switch = false;
    if (sw.expression && sw.expression->getType()) {
        BaseType* ct = sw.expression->getType()->canonical();
        if (ct && ct->kind() == BaseTypeKind::Reference) {
            ct = ct->as_reference_type()->type->canonical();
        } else if (ct && ct->kind() == BaseTypeKind::Pointer) {
            ct = ct->as_pointer_type()->type->canonical();
        }
        ASTNode* ln = ct ? ct->get_direct_linked_canonical_node() : nullptr;
        if (ln && (ln->kind() == ASTNodeKind::VariantDecl ||
                   ln->kind() == ASTNodeKind::VariantMember)) {
            ValueId tag = MIR_NULL;
            if (scrut.kind == MIRExprKind::Place) {
                tag = builder_->field_load(static_cast<PlaceId>(scrut.id), tag_fc, tag_t);
            } else if (scrut.kind == MIRExprKind::Address ||
                       scrut.kind == MIRExprKind::Value) {
                tag = builder_->field_load_ptr(static_cast<ValueId>(scrut.id), tag_fc, tag_t);
            } else {
                error = "variant switch expression is not a variant lvalue";
                return false;
            }
            scrut = MIRExprResult::value(tag, tag_t);
            variant_switch = true;
        }
    }

    const TypeId bool_t = types_.bool_type();
    const ConstantId eqc = module_.constants.add_int(
        MIR_INVALID_ID, static_cast<uint64_t>(MIRBinaryOp::Eq));

    const size_t n = sw.cases.size();
    for (size_t i = 0; i < n; ++i) {
        auto& c = sw.cases[i];
        const BlockId target = (c.second >= 0) ? scope_blocks[c.second] : default_b;
        if (c.first == nullptr) {
            builder_->br(target);
            break;
        }
        const BlockId next_b = (i + 1 < n) ? builder_->create_block() : default_b;
        MIRExprResult cv;
        if (variant_switch && c.first->val_kind() == ValueKind::VariantCase) {
            auto* vc = c.first->as_variant_case_unsafe();
            const long idx =
                (vc->member && vc->member->parent())
                    ? vc->member->parent()->direct_child_index(vc->member->name)
                    : 0;
            const ValueId ci = builder_->const_int(
                tag_t, module_.constants.add_int(
                           tag_t, static_cast<uint64_t>(idx < 0 ? 0 : idx)));
            cv = MIRExprResult::value(ci, tag_t);
        } else {
            cv = lower_expr(c.first, error);
        }
        if (!cv.ok()) return false;
        const ValueId cond = builder_->compare(scrut.id, cv.id, eqc, bool_t);
        builder_->cond_br(cond, target, next_b);
        if (i + 1 < n) {
            builder_->set_block(next_b);
        } else {
            break;
        }
    }
    if (n == 0) builder_->br(default_b);
    return true;
}

MIRExprResult MIRLowerer::lower_switch_value(SwitchValue* value, std::string& error) {
    SwitchStatement& sw = value->stmt;
    const TypeId vt = types_.map(value->getType());
    const PlaceId dest = builder_->alloca(vt, MIRStorageClass::Temporary);
    if (!lower_switch_into(sw, dest, vt, error)) return MIRExprResult::error();
    if (abi::is_aggregate(module_, vt)) {
        return MIRExprResult::place(dest, vt);
    }
    return MIRExprResult::value(builder_->load(dest, vt), vt);
}

bool MIRLowerer::lower_switch_into(SwitchStatement& sw, PlaceId dest, TypeId vt,
                                   std::string& error) {
    const BlockId end_b = builder_->create_block();
    std::vector<BlockId> scope_blocks(sw.scopes.size());
    for (size_t i = 0; i < sw.scopes.size(); ++i) {
        scope_blocks[i] = builder_->create_block();
    }
    const BlockId default_b = (sw.defScopeInd >= 0) ? scope_blocks[sw.defScopeInd] : end_b;
    if (!lower_switch_dispatch(sw, scope_blocks, default_b, error)) return false;
    for (size_t i = 0; i < sw.scopes.size(); ++i) {
        builder_->set_block(scope_blocks[i]);
        if (!lower_scope_value(sw.scopes[i], dest, vt, error)) return false;
        if (!builder_->current_block_terminated()) builder_->br(end_b);
    }
    builder_->set_block(end_b);
    return true;
}

MIRExprResult MIRLowerer::lower_if_value(IfValue* value, std::string& error) {
    const TypeId vt = types_.map(value->getType());
    const PlaceId dest = builder_->alloca(vt, MIRStorageClass::Temporary);
    if (!lower_if_stmt_into(value->stmt, dest, vt, error)) return MIRExprResult::error();
    // for aggregates the caller wants the temp's address; for scalars the value
    if (abi::is_aggregate(module_, vt)) {
        return MIRExprResult::place(dest, vt);
    }
    return MIRExprResult::value(builder_->load(dest, vt), vt);
}

bool MIRLowerer::lower_if_stmt_into(IfStatement& stmt, PlaceId dest, TypeId vt,
                                    std::string& error) {
    const BlockId merge = builder_->create_block();
    const BlockId then_b = builder_->create_block();
    const BlockId else_b = builder_->create_block();
    MIRExprResult c = lower_expr(stmt.condition, error);
    if (!c.ok()) return false;
    builder_->cond_br(c.id, then_b, else_b);

    builder_->set_block(then_b);
    if (!lower_scope_value(stmt.ifBody, dest, vt, error)) return false;
    if (!builder_->current_block_terminated()) builder_->br(merge);

    BlockId current = else_b;
    for (size_t i = 0; i < stmt.elseIfs.size(); ++i) {
        const BlockId body_b = builder_->create_block();
        const BlockId next_else = builder_->create_block();
        builder_->set_block(current);
        MIRExprResult ec = lower_expr(stmt.elseIfs[i].first, error);
        if (!ec.ok()) return false;
        builder_->cond_br(ec.id, body_b, next_else);
        builder_->set_block(body_b);
        if (!lower_scope_value(stmt.elseIfs[i].second, dest, vt, error)) return false;
        if (!builder_->current_block_terminated()) builder_->br(merge);
        current = next_else;
    }

    builder_->set_block(current);
    if (stmt.elseBody.has_value()) {
        if (!lower_scope_value(stmt.elseBody.value(), dest, vt, error)) return false;
    } else {
        error = "if-value has no else branch";
        return false;
    }
    if (!builder_->current_block_terminated()) builder_->br(merge);

    builder_->set_block(merge);
    return true;
}

MIRExprResult MIRLowerer::lower_loop_value(LoopValue* value, std::string& error) {
    const TypeId vt = types_.map(value->getType());
    const PlaceId dest = builder_->alloca(vt, MIRStorageClass::Temporary);
    if (!lower_loop_block_into(value->stmt, dest, vt, error)) {
        return MIRExprResult::error();
    }
    if (abi::is_aggregate(module_, vt)) {
        return MIRExprResult::place(dest, vt);
    }
    return MIRExprResult::value(builder_->load(dest, vt), vt);
}

bool MIRLowerer::lower_loop_block_into(LoopBlock& loop, PlaceId dest, TypeId vt,
                                       std::string& error) {
    const BlockId body_b = builder_->create_block();
    const BlockId exit_b = builder_->create_block();

    builder_->br(body_b);
    builder_->set_block(body_b);
    break_targets_.push_back(exit_b);
    continue_targets_.push_back(body_b);
    loop_destructible_marks_.push_back(destructibles_.size());
    break_value_dests_.push_back(dest);
    break_value_types_.push_back(vt);
    if (!lower_scope(loop.body, error)) {
        break_targets_.pop_back();
        continue_targets_.pop_back();
        loop_destructible_marks_.pop_back();
        break_value_dests_.pop_back();
        break_value_types_.pop_back();
        return false;
    }
    if (!builder_->current_block_terminated()) builder_->br(body_b);
    break_targets_.pop_back();
    continue_targets_.pop_back();
    loop_destructible_marks_.pop_back();
    break_value_dests_.pop_back();
    break_value_types_.pop_back();

    builder_->set_block(exit_b);
    return true;
}

bool MIRLowerer::lower_loop_block(LoopBlock* loop, std::string& error) {
    return lower_loop_block_into(*loop, MIR_INVALID_ID, MIR_INVALID_ID, error);
}

bool MIRLowerer::lower_while(WhileLoop* loop, std::string& error) {
    const BlockId cond_b = builder_->create_block();
    const BlockId body_b = builder_->create_block();
    const BlockId exit_b = builder_->create_block();

    builder_->br(cond_b);
    builder_->set_block(cond_b);
    MIRExprResult c = lower_expr(loop->condition, error);
    if (!c.ok()) return false;
    builder_->cond_br(c.id, body_b, exit_b);

    builder_->set_block(body_b);
    break_targets_.push_back(exit_b);
    continue_targets_.push_back(cond_b);
    loop_destructible_marks_.push_back(destructibles_.size());
    break_value_dests_.push_back(MIR_INVALID_ID);
    break_value_types_.push_back(MIR_INVALID_ID);
    if (!lower_scope(loop->body, error)) {
        break_targets_.pop_back();
        continue_targets_.pop_back();
        loop_destructible_marks_.pop_back();
        break_value_dests_.pop_back();
        break_value_types_.pop_back();
        return false;
    }
    if (!builder_->current_block_terminated()) builder_->br(cond_b);
    break_targets_.pop_back();
    continue_targets_.pop_back();
    loop_destructible_marks_.pop_back();
    break_value_dests_.pop_back();
    break_value_types_.pop_back();

    builder_->set_block(exit_b);
    return true;
}

bool MIRLowerer::lower_do_while(DoWhileLoop* loop, std::string& error) {
    const BlockId body_b = builder_->create_block();
    const BlockId cond_b = builder_->create_block();
    const BlockId exit_b = builder_->create_block();

    builder_->br(body_b);
    builder_->set_block(body_b);
    break_targets_.push_back(exit_b);
    continue_targets_.push_back(cond_b);
    loop_destructible_marks_.push_back(destructibles_.size());
    break_value_dests_.push_back(MIR_INVALID_ID);
    break_value_types_.push_back(MIR_INVALID_ID);
    if (!lower_scope(loop->body, error)) {
        break_targets_.pop_back();
        continue_targets_.pop_back();
        loop_destructible_marks_.pop_back();
        break_value_dests_.pop_back();
        break_value_types_.pop_back();
        return false;
    }
    if (!builder_->current_block_terminated()) builder_->br(cond_b);
    break_targets_.pop_back();
    continue_targets_.pop_back();
    loop_destructible_marks_.pop_back();
    break_value_dests_.pop_back();
    break_value_types_.pop_back();

    builder_->set_block(cond_b);
    MIRExprResult c = lower_expr(loop->condition, error);
    if (!c.ok()) return false;
    builder_->cond_br(c.id, body_b, exit_b);

    builder_->set_block(exit_b);
    return true;
}

bool MIRLowerer::lower_for(ForLoop* loop, std::string& error) {
    if (loop->initializer) {
        if (!lower_stmt(loop->initializer, error)) return false;
    }

    const BlockId cond_b = builder_->create_block();
    const BlockId body_b = builder_->create_block();
    const BlockId step_b = builder_->create_block();
    const BlockId exit_b = builder_->create_block();

    builder_->br(cond_b);
    builder_->set_block(cond_b);
    if (loop->conditionExpr) {
        MIRExprResult c = lower_expr(loop->conditionExpr, error);
        if (!c.ok()) return false;
        builder_->cond_br(c.id, body_b, exit_b);
    } else {
        builder_->br(body_b);
    }

    builder_->set_block(body_b);
    break_targets_.push_back(exit_b);
    continue_targets_.push_back(step_b);
    loop_destructible_marks_.push_back(destructibles_.size());
    break_value_dests_.push_back(MIR_INVALID_ID);
    break_value_types_.push_back(MIR_INVALID_ID);
    if (!lower_scope(loop->body, error)) {
        break_targets_.pop_back();
        continue_targets_.pop_back();
        loop_destructible_marks_.pop_back();
        break_value_dests_.pop_back();
        break_value_types_.pop_back();
        return false;
    }
    if (!builder_->current_block_terminated()) builder_->br(step_b);
    break_targets_.pop_back();
    continue_targets_.pop_back();
    loop_destructible_marks_.pop_back();
    break_value_dests_.pop_back();
    break_value_types_.pop_back();

    builder_->set_block(step_b);
    if (loop->incrementerExpr) {
        if (!lower_stmt(loop->incrementerExpr, error)) return false;
    }
    if (!builder_->current_block_terminated()) builder_->br(cond_b);

    builder_->set_block(exit_b);
    return true;
}

FunctionDeclaration* MIRLowerer::resolve_iter_impl(BaseType* container_type, const char* which) {
    if (!container_type || !impls_index_ || !core_nodes_) return nullptr;
    ASTNode* node = container_type->get_linked_node(true, false);
    if (!node) node = container_type->get_direct_linked_canonical_node();
    if (!node) return nullptr;
    MembersContainer* container = node->get_members_container();
    if (!container) return nullptr;
    const std::string w(which);
    if (w == "data") return impls_index_->get_linear_data_impl(*core_nodes_, container);
    if (w == "size") return impls_index_->get_linear_size_impl(*core_nodes_, container);
    if (w == "begin") return impls_index_->get_iterable_begin_impl(*core_nodes_, container);
    if (w == "valid") return impls_index_->get_iterable_valid_impl(*core_nodes_, container);
    if (w == "current") return impls_index_->get_iterable_current_impl(*core_nodes_, container);
    if (w == "next") return impls_index_->get_iterable_next_impl(*core_nodes_, container);
    if (w == "rbegin")
        return impls_index_->get_reversible_iterable_rbegin_impl(*core_nodes_, container);
    if (w == "previous")
        return impls_index_->get_reversible_iterable_previous_impl(*core_nodes_, container);
    if (w == "count")
        return impls_index_->get_reversible_iterable_count_impl(*core_nodes_, container);
    if (w == "begin_chunks")
        return impls_index_->get_chunked_begin_chunks_impl(*core_nodes_, container);
    if (w == "valid_chunk")
        return impls_index_->get_chunked_valid_chunk_impl(*core_nodes_, container);
    if (w == "current_chunk")
        return impls_index_->get_chunked_current_chunk_impl(*core_nodes_, container);
    if (w == "next_chunk")
        return impls_index_->get_chunked_next_chunk_impl(*core_nodes_, container);
    if (w == "rbegin_chunks")
        return impls_index_->get_chunked_rbegin_chunks_impl(*core_nodes_, container);
    if (w == "previous_chunk")
        return impls_index_->get_chunked_previous_chunk_impl(*core_nodes_, container);
    if (w == "total_size")
        return impls_index_->get_chunked_total_size_impl(*core_nodes_, container);
    return nullptr;
}

MIRExprResult MIRLowerer::call_method_values(FunctionDeclaration* fd, MIRExprResult recv,
                                             BaseType* recv_type,
                                             const std::vector<Value*>& args,
                                             std::string& error) {
    if (!fd) {
        error = "iterable method could not be resolved";
        return MIRExprResult::error();
    }
    VariableIdentifier id(chem::string_view(), nullptr, ZERO_LOC);
    id.linked = fd;
    FunctionCall call(&id, fd->returnType, ZERO_LOC);
    call.values = args;
    return lower_method_call(recv, recv_type, &call, error);
}

bool MIRLowerer::lower_for_in(ForInLoop* loop, std::string& error) {
    MIRExprResult it = lower_expr(loop->expr, error);
    if (!it.ok()) return false;
    BaseType* expr_ast = loop->expr->getType();
    BaseType* expr_c = expr_ast ? expr_ast->canonical() : nullptr;
    if (!expr_c) {
        error = "for-in iterable has no type";
        return false;
    }
    const bool is_array = expr_c->kind() == BaseTypeKind::Array;
    const bool reversed = loop->is_reversed();

    // a stable place for the iterable (the protocol methods take `&self`)
    PlaceId recv = MIR_INVALID_ID;
    TypeId recv_t = MIR_INVALID_ID;
    if (it.kind == MIRExprKind::Place) {
        recv = static_cast<PlaceId>(it.id);
        recv_t = builder_->function().places[recv].type;
    } else if (it.kind == MIRExprKind::Value || it.kind == MIRExprKind::Address) {
        recv_t = it.type;
        recv = builder_->alloca(recv_t, MIRStorageClass::Temporary);
        builder_->store(recv, static_cast<ValueId>(it.id));
    } else {
        error = "for-in iterable does not produce a value";
        return false;
    }
    const MIRExprResult recv_place = MIRExprResult::place(recv, recv_t);

    const TypeId elem_t = types_.map(loop->getIterationElementActualType());
    const TypeId elem_ptr_t = types_.pointer_type(elem_t, true);
    const TypeId u64t = types_.u64_type();
    const TypeId bool_t = types_.bool_type();
    const TypeId u32t = types_.u32_type();

    // the loop variable holds a pointer to the current element
    const PlaceId var_place = builder_->alloca(elem_ptr_t, MIRStorageClass::Temporary);
    bind(loop, var_place);
    bind_name(loop->id, var_place);

    PlaceId idx_place = MIR_INVALID_ID;
    TypeId idx_t = MIR_INVALID_ID;
    if (loop->index_init) {
        idx_t = types_.map(loop->index_init->known_type());
        idx_place = builder_->alloca(idx_t, MIRStorageClass::Temporary);
        bind(loop->index_init, idx_place);
        bind_name(loop->index_init->id_view(), idx_place);
    }

    const ConstantId add_op =
        module_.constants.add_int(MIR_INVALID_ID, static_cast<uint64_t>(MIRBinaryOp::Add));
    const ConstantId sub_op =
        module_.constants.add_int(MIR_INVALID_ID, static_cast<uint64_t>(MIRBinaryOp::Sub));
    const ConstantId ne_op =
        module_.constants.add_int(MIR_INVALID_ID, static_cast<uint64_t>(MIRBinaryOp::Ne));
    const ValueId one32 = builder_->const_int(u32t, module_.constants.add_int(u32t, 1));
    static uint64_t s_forin_counter = 0;
    // the loop counter, spelled once into a stack buffer: every cursor name
    // built from it is only bound as a lowering name (a map key)
    const uint64_t uniq = s_forin_counter++;

    auto emit_idx_step = [&](bool increment) {
        if (idx_place == MIR_INVALID_ID) return;
        const ValueId cur = builder_->load(idx_place, idx_t);
        const ValueId one = builder_->const_int(idx_t, module_.constants.add_int(idx_t, 1));
        builder_->store(idx_place,
                        builder_->binary(cur, one, increment ? add_op : sub_op, idx_t));
    };
    auto emit_var_step = [&](bool increment) {
        const ValueId cur = builder_->load(var_place, elem_ptr_t);
        builder_->store(var_place,
                        builder_->binary(cur, one32, increment ? add_op : sub_op, elem_ptr_t));
    };
    auto store_ptr = [&](PlaceId dest, MIRExprResult r, TypeId dest_t) -> bool {
        if (r.kind == MIRExprKind::Place) {
            builder_->store(dest, builder_->load(static_cast<PlaceId>(r.id), r.type));
        } else if (r.kind == MIRExprKind::Value || r.kind == MIRExprKind::Address) {
            builder_->store(dest, static_cast<ValueId>(r.id));
        } else {
            error = "for-in expression does not produce a value";
            return false;
        }
        return true;
    };

    // run one iteration structure: `cond_b` already current
    auto run_loop = [&](BlockId cond_b, auto&& emit_cond, auto&& emit_init,
                        auto&& emit_step) -> bool {
        const BlockId body_b = builder_->create_block();
        const BlockId step_b = builder_->create_block();
        const BlockId exit_b = builder_->create_block();
        builder_->br(cond_b);
        builder_->set_block(cond_b);
        if (!emit_cond(body_b, exit_b)) return false;
        builder_->set_block(body_b);
        if (!emit_init()) return false;
        break_targets_.push_back(exit_b);
        continue_targets_.push_back(step_b);
        loop_destructible_marks_.push_back(destructibles_.size());
        break_value_dests_.push_back(MIR_INVALID_ID);
        break_value_types_.push_back(MIR_INVALID_ID);
        if (!lower_scope(loop->body, error)) {
            break_targets_.pop_back();
            continue_targets_.pop_back();
            loop_destructible_marks_.pop_back();
            break_value_dests_.pop_back();
            break_value_types_.pop_back();
            return false;
        }
        if (!builder_->current_block_terminated()) builder_->br(step_b);
        break_targets_.pop_back();
        continue_targets_.pop_back();
        loop_destructible_marks_.pop_back();
        break_value_dests_.pop_back();
        break_value_types_.pop_back();
        builder_->set_block(step_b);
        if (!emit_step()) return false;
        if (!builder_->current_block_terminated()) builder_->br(cond_b);
        builder_->set_block(exit_b);
        return true;
    };

    if (is_array) {
        const uint64_t n = expr_c->as_array_type_unsafe()->get_array_size();
        const MIRTypeRecord& rr = module_.types.get(recv_t);
        ValueId data_v;
        if (rr.kind == MIRTypeKind::Pointer || rr.kind == MIRTypeKind::Reference) {
            // an array parameter decays to a pointer to its first element
            data_v = builder_->load(recv, recv_t);
            if (data_v != MIR_NULL && recv_t != elem_ptr_t) {
                data_v = builder_->cast(data_v, elem_ptr_t);
            }
        } else {
            data_v = builder_->address_of(recv, elem_ptr_t);
        }
        const ValueId size_v = builder_->const_int(u64t, module_.constants.add_int(u64t, n));
        const ValueId end_v = builder_->binary(data_v, size_v, add_op, elem_ptr_t);
        const PlaceId data_place = builder_->alloca(elem_ptr_t, MIRStorageClass::Temporary);
        builder_->store(data_place, data_v);
        if (idx_place != MIR_INVALID_ID) {
            if (loop->is_reversed_counter()) {
                builder_->store(idx_place, size_v);
            } else {
                builder_->store(idx_place,
                                builder_->const_int(idx_t, module_.constants.add_int(idx_t, 0)));
            }
        }
        if (reversed) {
            builder_->store(var_place, end_v);
        } else {
            builder_->store(var_place, data_v);
        }
        const BlockId cond_b = builder_->create_block();
        return run_loop(
            cond_b,
            [&](BlockId body_b, BlockId exit_b) -> bool {
                const ValueId v = builder_->load(var_place, elem_ptr_t);
                const ValueId e = reversed ? builder_->load(data_place, elem_ptr_t) : end_v;
                builder_->cond_br(builder_->compare(v, e, ne_op, bool_t), body_b, exit_b);
                return true;
            },
            [&]() -> bool {
                if (reversed) {
                    emit_idx_step(false);
                    emit_var_step(false);
                }
                return true;
            },
            [&]() -> bool {
                if (!reversed) {
                    emit_idx_step(true);
                    emit_var_step(true);
                }
                return true;
            });
    }

    // Linear iteration (data/size)
    FunctionDeclaration* data_fd = resolve_iter_impl(expr_c, "data");
    FunctionDeclaration* size_fd = resolve_iter_impl(expr_c, "size");
    if (data_fd && size_fd) {
        MIRExprResult dr = call_method_values(data_fd, recv_place, expr_c, {}, error);
        if (!dr.ok()) return false;
        MIRExprResult sr = call_method_values(size_fd, recv_place, expr_c, {}, error);
        if (!sr.ok()) return false;
        ValueId data_v = (dr.kind == MIRExprKind::Place)
                             ? builder_->load(static_cast<PlaceId>(dr.id), dr.type)
                             : static_cast<ValueId>(dr.id);
        ValueId size_v = (sr.kind == MIRExprKind::Place)
                             ? builder_->load(static_cast<PlaceId>(sr.id), sr.type)
                             : static_cast<ValueId>(sr.id);
        if (data_v != MIR_NULL && dr.type != elem_ptr_t) data_v = builder_->cast(data_v, elem_ptr_t);
        const ValueId end_v = builder_->binary(data_v, size_v, add_op, elem_ptr_t);
        const PlaceId data_place = builder_->alloca(elem_ptr_t, MIRStorageClass::Temporary);
        builder_->store(data_place, data_v);
        if (idx_place != MIR_INVALID_ID) {
            if (loop->is_reversed_counter()) {
                builder_->store(idx_place, size_v);
            } else {
                builder_->store(idx_place,
                                builder_->const_int(idx_t, module_.constants.add_int(idx_t, 0)));
            }
        }
        if (reversed) {
            builder_->store(var_place, end_v);
        } else {
            builder_->store(var_place, data_v);
        }
        const BlockId cond_b = builder_->create_block();
        return run_loop(
            cond_b,
            [&](BlockId body_b, BlockId exit_b) -> bool {
                const ValueId v = builder_->load(var_place, elem_ptr_t);
                const ValueId e = reversed ? builder_->load(data_place, elem_ptr_t) : end_v;
                builder_->cond_br(v == MIR_NULL ? v : builder_->compare(v, e, ne_op, bool_t),
                                  body_b, exit_b);
                return true;
            },
            [&]() -> bool {
                if (reversed) {
                    emit_idx_step(false);
                    emit_var_step(false);
                }
                return true;
            },
            [&]() -> bool {
                if (!reversed) {
                    emit_idx_step(true);
                    emit_var_step(true);
                }
                return true;
            });
    }

    // Chunked iteration (begin_chunks / valid_chunk / current_chunk / next_chunk)
    {
        FunctionDeclaration* begin_fd =
            resolve_iter_impl(expr_c, reversed ? "rbegin_chunks" : "begin_chunks");
        FunctionDeclaration* valid_fd = resolve_iter_impl(expr_c, "valid_chunk");
        FunctionDeclaration* current_fd = resolve_iter_impl(expr_c, "current_chunk");
        FunctionDeclaration* next_fd =
            resolve_iter_impl(expr_c, reversed ? "previous_chunk" : "next_chunk");
        if (begin_fd && valid_fd && current_fd && next_fd) {
            const TypeId cursor_t = types_.map(begin_fd->returnType);
            const PlaceId cursor_place = builder_->alloca(cursor_t, MIRStorageClass::Temporary);
            const MIRNumberedName cursor_name("__chx_forin_chunk_cursor_", uniq);
            bind_name(cursor_name.view(), cursor_place);
            MIRExprResult br = call_method_values(begin_fd, recv_place, expr_c, {}, error);
            if (!br.ok()) return false;
            if (!store_ptr(cursor_place, br, cursor_t)) return false;

            if (idx_place != MIR_INVALID_ID) {
                if (loop->is_reversed_counter()) {
                    FunctionDeclaration* total_fd = resolve_iter_impl(expr_c, "total_size");
                    MIRExprResult tr =
                        call_method_values(total_fd, recv_place, expr_c, {}, error);
                    if (!tr.ok()) return false;
                    if (!store_ptr(idx_place, tr, idx_t)) return false;
                } else {
                    builder_->store(
                        idx_place,
                        builder_->const_int(idx_t, module_.constants.add_int(idx_t, 0)));
                }
            }

            const BlockId ocond = builder_->create_block();
            const BlockId obody = builder_->create_block();
            const BlockId osteps = builder_->create_block();
            const BlockId oexit = builder_->create_block();
            builder_->br(ocond);
            builder_->set_block(ocond);
            {
                VariableIdentifier cid(chem::string_view(cursor_name.data(), cursor_name.size()),
                                       begin_fd->returnType, ZERO_LOC);
                std::vector<Value*> vargs{&cid};
                MIRExprResult vr =
                    call_method_values(valid_fd, recv_place, expr_c, vargs, error);
                if (!vr.ok()) return false;
                builder_->cond_br(vr.id, obody, oexit);
            }
            builder_->set_block(obody);
            {
                VariableIdentifier cid(chem::string_view(cursor_name.data(), cursor_name.size()),
                                       begin_fd->returnType, ZERO_LOC);
                std::vector<Value*> vargs{&cid};
                MIRExprResult cr =
                    call_method_values(current_fd, recv_place, expr_c, vargs, error);
                if (!cr.ok()) return false;
                const TypeId chunk_t = types_.map(current_fd->returnType);
                PlaceId chunk_place = MIR_INVALID_ID;
                if (cr.kind == MIRExprKind::Place) {
                    chunk_place = static_cast<PlaceId>(cr.id);
                } else {
                    chunk_place = builder_->alloca(chunk_t, MIRStorageClass::Temporary);
                    builder_->store(chunk_place, static_cast<ValueId>(cr.id));
                }
                const ConstantId fc_ptr = module_.constants.add_string(MIR_INVALID_ID, "ptr", 3);
                const ConstantId fc_len = module_.constants.add_string(MIR_INVALID_ID, "len", 3);
                const ValueId cptr = builder_->field_load(chunk_place, fc_ptr, elem_ptr_t);
                const ValueId clen = builder_->field_load(chunk_place, fc_len, u64t);
                const ValueId inner_end_v = builder_->binary(cptr, clen, add_op, elem_ptr_t);
                const PlaceId inner_end_place =
                    builder_->alloca(elem_ptr_t, MIRStorageClass::Temporary);
                if (reversed) {
                    builder_->store(inner_end_place, cptr);
                    builder_->store(var_place, inner_end_v);
                } else {
                    builder_->store(var_place, cptr);
                    builder_->store(inner_end_place, inner_end_v);
                }

                const BlockId icond = builder_->create_block();
                const BlockId ibody = builder_->create_block();
                const BlockId istep = builder_->create_block();
                builder_->br(icond);
                builder_->set_block(icond);
                {
                    const ValueId v = builder_->load(var_place, elem_ptr_t);
                    const ValueId e = builder_->load(inner_end_place, elem_ptr_t);
                    // normal inner completion advances the chunk; `break` exits
                    builder_->cond_br(builder_->compare(v, e, ne_op, bool_t), ibody, osteps);
                }
                builder_->set_block(ibody);
                if (reversed) {
                    emit_idx_step(false);
                    emit_var_step(false);
                }
                break_targets_.push_back(oexit);
                continue_targets_.push_back(istep);
                loop_destructible_marks_.push_back(destructibles_.size());
                break_value_dests_.push_back(MIR_INVALID_ID);
                break_value_types_.push_back(MIR_INVALID_ID);
                if (!lower_scope(loop->body, error)) {
                    break_targets_.pop_back();
                    continue_targets_.pop_back();
                    loop_destructible_marks_.pop_back();
                    break_value_dests_.pop_back();
                    break_value_types_.pop_back();
                    return false;
                }
                if (!builder_->current_block_terminated()) builder_->br(istep);
                break_targets_.pop_back();
                continue_targets_.pop_back();
                loop_destructible_marks_.pop_back();
                break_value_dests_.pop_back();
                break_value_types_.pop_back();
                builder_->set_block(istep);
                if (!reversed) {
                    emit_idx_step(true);
                    emit_var_step(true);
                }
                if (!builder_->current_block_terminated()) builder_->br(icond);
            }
            builder_->set_block(osteps);
            {
                VariableIdentifier cid(chem::string_view(cursor_name.data(), cursor_name.size()),
                                       begin_fd->returnType, ZERO_LOC);
                std::vector<Value*> vargs{&cid};
                MIRExprResult nr =
                    call_method_values(next_fd, recv_place, expr_c, vargs, error);
                if (!nr.ok()) return false;
                if (!store_ptr(cursor_place, nr, cursor_t)) return false;
            }
            if (!builder_->current_block_terminated()) builder_->br(ocond);
            builder_->set_block(oexit);
            return true;
        }
    }

    // Iterable protocol (begin/valid/current/next)
    {
        FunctionDeclaration* begin_fd = resolve_iter_impl(expr_c, reversed ? "rbegin" : "begin");
        FunctionDeclaration* valid_fd = resolve_iter_impl(expr_c, "valid");
        FunctionDeclaration* current_fd = resolve_iter_impl(expr_c, "current");
        FunctionDeclaration* next_fd = resolve_iter_impl(expr_c, reversed ? "previous" : "next");
        if (!begin_fd || !valid_fd || !current_fd || !next_fd) {
            error = "container does not implement a supported iteration protocol";
            return false;
        }
        const TypeId cursor_t = types_.map(begin_fd->returnType);
        const PlaceId cursor_place = builder_->alloca(cursor_t, MIRStorageClass::Temporary);
        const MIRNumberedName cursor_name("__chx_forin_cursor_", uniq);
        bind_name(cursor_name.view(), cursor_place);
        MIRExprResult br = call_method_values(begin_fd, recv_place, expr_c, {}, error);
        if (!br.ok()) return false;
        if (!store_ptr(cursor_place, br, cursor_t)) return false;

        if (idx_place != MIR_INVALID_ID) {
            if (loop->is_reversed_counter()) {
                FunctionDeclaration* count_fd = resolve_iter_impl(expr_c, "count");
                MIRExprResult cr = call_method_values(count_fd, recv_place, expr_c, {}, error);
                if (!cr.ok()) return false;
                if (!store_ptr(idx_place, cr, idx_t)) return false;
            } else {
                builder_->store(idx_place,
                                builder_->const_int(idx_t, module_.constants.add_int(idx_t, 0)));
            }
        }
        const BlockId cond_b = builder_->create_block();
        return run_loop(
            cond_b,
            [&](BlockId body_b, BlockId exit_b) -> bool {
                VariableIdentifier cid(chem::string_view(cursor_name.data(), cursor_name.size()),
                                       begin_fd->returnType, ZERO_LOC);
                std::vector<Value*> vargs{&cid};
                MIRExprResult vr = call_method_values(valid_fd, recv_place, expr_c, vargs, error);
                if (!vr.ok()) return false;
                builder_->cond_br(vr.id, body_b, exit_b);
                return true;
            },
            [&]() -> bool {
                VariableIdentifier cid(chem::string_view(cursor_name.data(), cursor_name.size()),
                                       begin_fd->returnType, ZERO_LOC);
                std::vector<Value*> vargs{&cid};
                MIRExprResult cr = call_method_values(current_fd, recv_place, expr_c, vargs, error);
                if (!cr.ok()) return false;
                ValueId cv = (cr.kind == MIRExprKind::Place)
                                 ? builder_->load(static_cast<PlaceId>(cr.id), cr.type)
                                 : static_cast<ValueId>(cr.id);
                if (cv != MIR_NULL && cr.type != elem_ptr_t) cv = builder_->cast(cv, elem_ptr_t);
                builder_->store(var_place, cv);
                return true;
            },
            [&]() -> bool {
                VariableIdentifier cid(chem::string_view(cursor_name.data(), cursor_name.size()),
                                       begin_fd->returnType, ZERO_LOC);
                std::vector<Value*> vargs{&cid};
                MIRExprResult nr = call_method_values(next_fd, recv_place, expr_c, vargs, error);
                if (!nr.ok()) return false;
                if (!store_ptr(cursor_place, nr, cursor_t)) return false;
                emit_idx_step(!reversed);
                return true;
            });
    }
}

} // namespace mir
