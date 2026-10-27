// Copyright (c) Chemical Language Foundation 2025.
//
// MIR lowering: expression lowering.
// Split out of MIRLowerer.cpp (action D4, mir-migration-evaluation.md).
// AST-aware: this is the front-end lowering boundary, never a backend.

#include "MIRLowererHelpers.h"

namespace mir {

MIRExprResult MIRLowerer::lower_expr(Value* value, std::string& error) {
    if (!value) {
        error = "null value node";
        return MIRExprResult::error();
    }

    const TypeId type = types_.map(value->getType());

    switch (value->val_kind()) {
        case ValueKind::IntN: {
            auto* n = value->as_int_num_value_unsafe();
            ConstantId c = module_.constants.add_int(type, n->get_num_value());
            return MIRExprResult::value(builder_->const_int(type, c), type);
        }
        case ValueKind::Bool: {
            auto* b = value->as_bool_unsafe();
            ConstantId c = module_.constants.add_bool(type, b->value);
            return MIRExprResult::value(builder_->const_bool(type, c), type);
        }
        case ValueKind::Float: {
            auto* f = value->as_float_unsafe();
            ConstantId c = module_.constants.add_float(type, f->value);
            return MIRExprResult::value(builder_->const_float(type, c), type);
        }
        case ValueKind::Double: {
            auto* d = value->as_double_unsafe();
            ConstantId c = module_.constants.add_double(type, d->value);
            return MIRExprResult::value(builder_->const_double(type, c), type);
        }
        case ValueKind::String: {
            auto* s = value->as_string_unsafe();
            // use the actual data size: symres rewrites `length` to the target
            // array size (for padding), which would over-read the string data
            ConstantId c = module_.constants.add_string(type, s->value.data(),
                                                        static_cast<uint32_t>(s->value.size()));
            return MIRExprResult::value(builder_->const_string(type, c), type);
        }
        case ValueKind::GenericInstIdentifier:
            // a generic function reference is relinked to its concrete
            // instantiated FunctionDecl, so it lowers like an identifier
        case ValueKind::Identifier: {
            auto* id = value->as_identifier_unsafe();
            if (!comptime_bindings_.empty() && !id->value.empty()) {
                // transparent lookup: no temporary `std::string` per identifier
                auto it = comptime_bindings_.find(id->value);
                if (it != comptime_bindings_.end()) {
                    note_comptime_binding_consumed();
                    return lower_expr(it->second, error);
                }
            }
            if (id->linked && id->linked->kind() == ASTNodeKind::VariantCaseVariable) {
                return lower_variant_payload(id->linked->as_variant_case_var_unsafe(), false, error);
            }
            if (id->linked && id->linked->kind() == ASTNodeKind::PatternMatchId) {
                return lower_pattern_identifier(id->linked->as_patt_match_id_unsafe(), error);
            }
            if (id->linked && id->linked->kind() == ASTNodeKind::CapturedVariable) {
                return lower_captured_variable(id->linked->as_captured_var_unsafe(), false, error);
            }
            if (id->linked && id->linked->kind() == ASTNodeKind::ForInLoopStmt) {
                auto* fl = id->linked->as_for_in_loop_unsafe();
                const PlaceId place = place_for_linked(id->linked);
                if (place == MIR_INVALID_ID) {
                    error = "for-in loop variable does not resolve to a place";
                    return MIRExprResult::error();
                }
                const TypeId pt = builder_->function().places[place].type; // `*elem`
                const ValueId pv = builder_->load(place, pt);
                if (fl->is_reference()) {
                    return MIRExprResult::value(pv, pt);
                }
                const TypeId elem = module_.types.get(pt).element;
                if (abi::is_aggregate(module_, elem)) {
                    // an aggregate element is a pointer to the element (lvalue)
                    return MIRExprResult::value(pv, pt);
                }
                return MIRExprResult::value(builder_->load_indirect(pv, elem), elem);
            }
            if (id->linked && id->linked->kind() == ASTNodeKind::FunctionDecl) {
                // a function used as a value (function pointer)
                FunctionDeclaration* fd = id->linked->as_function_unsafe();
                const SymbolId sym = intern_function(fd);
                return MIRExprResult::value(builder_->function_addr(sym, type), type);
            }
            if (id->linked && id->linked->kind() == ASTNodeKind::EnumMember) {
                auto* em = id->linked->as_enum_member();
                if (em && em->init_value) return lower_expr(em->init_value, error);
                const uint64_t idx = em ? static_cast<uint64_t>(em->get_default_index()) : 0;
                const ConstantId c = module_.constants.add_int(type, idx);
                return MIRExprResult::value(builder_->const_int(type, c), type);
            }
            if (is_implicit_field_node(id->linked) && current_self_param_) {
                // an unqualified field access inside a method resolves to
                // `self.field` (the field lives on the receiver)
                VariableIdentifier self_id(current_self_param_->name,
                                           current_self_param_->type, ZERO_LOC);
                self_id.linked = current_self_param_;
                Value* chain[2] = {&self_id, id};
                return lower_member_chain(chain, 2, error);
            }
            PlaceId place = place_for_linked(id->linked);
            if (place == MIR_INVALID_ID) place = place_for_name(id->value);
            if (place != MIR_INVALID_ID) {
                if (abi::is_aggregate(module_, type)) {
                    return MIRExprResult::place(place, type);
                }
                return MIRExprResult::value(builder_->load(place, type), type);
            }
            if (id->linked && id->linked->kind() == ASTNodeKind::VarInitStmt) {
                auto* vi = id->linked->as_var_init();
                if (vi->is_comptime() && vi->value) {
                    // module-level comptime constants are inlined at their use
                    return lower_expr(vi->value, error);
                }
                const SymbolId gs = intern_global(vi);
                // an unsized array global decays to a pointer to its first element
                if (BaseType* at = id->getType(); at && at->kind() == BaseTypeKind::Array &&
                    !at->as_array_type()->has_array_size()) {
                    const ValueId addr = builder_->global_addr(gs, type);
                    return MIRExprResult::value(addr, type);
                }
                // an array global decays to a pointer to its first element
                const TypeId ptrt = (type < module_.types.size() &&
                                     module_.types.get(type).kind == MIRTypeKind::Array)
                                        ? types_.pointer_type(module_.types.get(type).element, false)
                                        : types_.pointer_type(type, false);
                const ValueId addr = builder_->global_addr(gs, ptrt);
                if (abi::is_aggregate(module_, type)) {
                    return MIRExprResult::address(addr, ptrt);
                }
                return MIRExprResult::value(builder_->load_indirect(addr, type), type);
            }
            // unresolved identifier: treat as a module-level / extern global
            {
                const std::string gname(id->value.data(), id->value.size());
                if (!gname.empty()) {
                    const SymbolId gs = intern_named_global(gname);
                    const TypeId ptrt = (type < module_.types.size() &&
                                         module_.types.get(type).kind == MIRTypeKind::Array)
                                            ? types_.pointer_type(module_.types.get(type).element, false)
                                            : types_.pointer_type(type, false);
                    const ValueId addr = builder_->global_addr(gs, ptrt);
                    if (abi::is_aggregate(module_, type)) {
                        return MIRExprResult::address(addr, ptrt);
                    }
                    return MIRExprResult::value(builder_->load_indirect(addr, type), type);
                }
            }
            error = "identifier does not resolve to a local place (name '" +
                    std::string(id->value.data(), id->value.size()) + "')";
            return MIRExprResult::error();
        }
        case ValueKind::NegativeValue: {
            auto* n = value->as_negative_value_unsafe();
            MIRExprResult in = lower_expr(n->getValue(), error);
            if (!in.ok()) return in;
            if (abi::is_struct_like(module_, in.type)) {
                return lower_unary_operator_overload(n->getValue(), in, "neg", n->getType(), error);
            }
            ConstantId op = module_.constants.add_int(MIR_INVALID_ID, static_cast<uint64_t>(MIRUnaryOp::Neg));
            return MIRExprResult::value(builder_->unary(in.id, op, type), type);
        }
        case ValueKind::NotValue: {
            auto* n = value->as_not_value_unsafe();
            MIRExprResult in = lower_expr(n->getValue(), error);
            if (!in.ok()) return in;
            if (abi::is_struct_like(module_, in.type)) {
                return lower_unary_operator_overload(n->getValue(), in, "not", n->getType(), error);
            }
            ConstantId op = module_.constants.add_int(MIR_INVALID_ID, static_cast<uint64_t>(MIRUnaryOp::Not));
            return MIRExprResult::value(builder_->unary(in.id, op, type), type);
        }
        case ValueKind::BitwiseNot: {
            auto* n = value->as_bitwise_not_unsafe();
            MIRExprResult in = lower_expr(n->getValue(), error);
            if (!in.ok()) return in;
            if (abi::is_struct_like(module_, in.type)) {
                return lower_unary_operator_overload(n->getValue(), in, "bitnot", n->getType(),
                                                     error);
            }
            ConstantId op = module_.constants.add_int(MIR_INVALID_ID, static_cast<uint64_t>(MIRUnaryOp::BitNot));
            return MIRExprResult::value(builder_->unary(in.id, op, type), type);
        }
        case ValueKind::Expression: {
            auto* e = value->as_expression_unsafe();
            if (e->operation == Operation::LogicalAND || e->operation == Operation::LogicalOR) {
                const TypeId bool_t = types_.bool_type();
                MIRExprResult a = lower_expr(e->firstValue, error);
                if (!a.ok()) return a;
                const PlaceId res = builder_->alloca(bool_t, MIRStorageClass::Temporary);
                builder_->store(res, a.id);
                const BlockId rhs_b = builder_->create_block();
                const BlockId end_b = builder_->create_block();
                if (e->operation == Operation::LogicalAND) {
                    builder_->cond_br(a.id, rhs_b, end_b);
                } else {
                    builder_->cond_br(a.id, end_b, rhs_b);
                }
                builder_->set_block(rhs_b);
                MIRExprResult b = lower_expr(e->secondValue, error);
                if (!b.ok()) return b;
                builder_->store(res, b.id);
                builder_->br(end_b);
                builder_->set_block(end_b);
                return MIRExprResult::value(builder_->load(res, bool_t), bool_t);
            }
            MIRExprResult lhs = lower_expr(e->firstValue, error);
            if (!lhs.ok()) return lhs;
            MIRExprResult rhs = lower_expr(e->secondValue, error);
            if (!rhs.ok()) return rhs;
            MIROpcode op;
            MIRBinaryOp bop;
            if (!map_binary(e->operation, op, bop)) {
                error = "unsupported binary operator (short-circuit/logical operators need CFG)";
                return MIRExprResult::error();
            }
            if (abi::is_struct_like(module_, lhs.type) || abi::is_struct_like(module_, rhs.type)) {
                const char* method = operator_method_name(e->operation);
                if (!method) {
                    error = "unsupported operator overload";
                    return MIRExprResult::error();
                }
                return lower_operator_overload(e->firstValue, lhs, e->secondValue, rhs, method,
                                               e->getType(), error);
            }
            const TypeId operand_type = lhs.type != MIR_INVALID_ID ? lhs.type : type;
            if (bop == MIRBinaryOp::Div || bop == MIRBinaryOp::Rem) {
                emit_division_check(static_cast<ValueId>(lhs.id),
                                    static_cast<ValueId>(rhs.id), operand_type, bop);
            } else if (bop == MIRBinaryOp::Shl || bop == MIRBinaryOp::Shr) {
                emit_shift_check(static_cast<ValueId>(rhs.id), operand_type);
            }
            ConstantId opc = module_.constants.add_int(MIR_INVALID_ID, static_cast<uint64_t>(bop));
            if (op == MIROpcode::Compare) {
                return MIRExprResult::value(builder_->compare(lhs.id, rhs.id, opc, type), type);
            }
            return MIRExprResult::value(builder_->binary(lhs.id, rhs.id, opc, type), type);
        }
        case ValueKind::CastedValue: {
            auto* c = value->as_casted_value_unsafe();
            MIRExprResult in = lower_expr(c->value, error);
            if (!in.ok()) return in;
            if (type != MIR_INVALID_ID && type < module_.types.size() &&
                module_.types.get(type).kind == MIRTypeKind::Array) {
                // casting a pointer to an array: copy the bytes into a temp array
                const PlaceId tmp = builder_->alloca(type, MIRStorageClass::Temporary);
                if (in.kind == MIRExprKind::Value || in.kind == MIRExprKind::Address) {
                    builder_->copy_ptr_to_place(tmp, static_cast<ValueId>(in.id));
                } else if (in.kind == MIRExprKind::Place) {
                    const TypeId pt = builder_->function().places[in.id].type;
                    const ValueId ptr = builder_->address_of(
                        static_cast<PlaceId>(in.id), types_.pointer_type(pt, false));
                    builder_->copy_ptr_to_place(tmp, ptr);
                }
                return MIRExprResult::place(tmp, type);
            }
            if (in.kind == MIRExprKind::Place) {
                // a cast of an aggregate place to a pointer/interface type
                // (`value as StaticInterface`) yields the place's address
                if (type != MIR_INVALID_ID && type < module_.types.size()) {
                    const MIRTypeKind tk = module_.types.get(type).kind;
                    if (tk == MIRTypeKind::Pointer || tk == MIRTypeKind::Opaque) {
                        const TypeId pt = builder_->function().places[in.id].type;
                        const MIRTypeKind pk = module_.types.get(pt).kind;
                        if (pk == MIRTypeKind::Pointer || pk == MIRTypeKind::Reference) {
                            // the place already holds a pointer (an array/pointer
                            // param decayed to a pointer, or a pointer/reference
                            // local): cast the pointer *value*, not its address
                            const ValueId loaded = builder_->load(
                                static_cast<PlaceId>(in.id), pt);
                            return MIRExprResult::value(builder_->cast(loaded, type), type);
                        }
                        const ValueId addr = builder_->address_of(
                            static_cast<PlaceId>(in.id), types_.pointer_type(pt, true));
                        return MIRExprResult::value(builder_->cast(addr, type), type);
                    }
                }
                return in;
            }
            if (in.type == type) return in;
            return MIRExprResult::value(builder_->cast(in.id, type), type);
        }
        case ValueKind::AccessChain: {
            auto* chain = value->as_access_chain_unsafe();
            if (chain->values.size() == 1) {
                return lower_expr(chain->values[0], error);
            }
            // enum member access: `EnumName.Member` -> its constant value
            if (ASTNode* lk = chain->linked_node()) {
                if (lk->kind() == ASTNodeKind::EnumMember) {
                    auto* em = lk->as_enum_member();
                    if (em && em->init_value) return lower_expr(em->init_value, error);
                    const uint64_t idx =
                        em ? static_cast<uint64_t>(em->get_default_index()) : 0;
                    const ConstantId c = module_.constants.add_int(type, idx);
                    return MIRExprResult::value(builder_->const_int(type, c), type);
                }
            }
            // static method reference: `Type::method` -> a function pointer
            if (chain->values.size() >= 2) {
                if (ASTNode* lk = chain->values.back()->linked_node()) {
                    if (lk->kind() == ASTNodeKind::VarInitStmt) {
                        // qualified module-level constant/global: resolve the leaf
                        return lower_expr(chain->values.back(), error);
                    }
                    if (lk->kind() == ASTNodeKind::FunctionDecl) {
                        auto* fd = lk->as_function();
                        ASTNode* base = chain->values[0]->linked_node();
                        const bool base_is_type =
                            base && (base->kind() == ASTNodeKind::StructDecl ||
                                     base->kind() == ASTNodeKind::ImplDecl ||
                                     base->kind() == ASTNodeKind::InterfaceDecl ||
                                     base->kind() == ASTNodeKind::NamespaceDecl ||
                                     base->kind() == ASTNodeKind::VariantDecl ||
                                     base->kind() == ASTNodeKind::UnionDecl ||
                                     base->kind() == ASTNodeKind::EnumDecl ||
                                     base->kind() == ASTNodeKind::GenericStructDecl);
                        if (base_is_type) {
                            const SymbolId sym = intern_function(fd);
                            return MIRExprResult::value(builder_->function_addr(sym, type),
                                                        type);
                        }
                    }
                }
            }
            // a member access on a comptime-known global (e.g. `def.is64Bit`)
            if (comptime_value_eval_ && chain->values.size() >= 2) {
                Value* b = chain->values[0];
                if (b && b->val_kind() == ValueKind::AccessChain) {
                    auto* c = b->as_access_chain_unsafe();
                    if (c->values.size() == 1) b = c->values[0];
                }
                ASTNode* bl = b ? b->linked_node() : nullptr;
                if (bl && bl->kind() == ASTNodeKind::VarInitStmt &&
                    bl->as_var_init()->is_comptime()) {
                    Value* ev = comptime_value_eval_(value);
                    if (ev && ev != value) return lower_expr(ev, error);
                }
            }
            // member access: walk the chain from the base, taking the address of
            // each intermediate field and loading the last
            return lower_member_chain(chain->values.data(), chain->values.size(), error);
        }
        case ValueKind::StructValue: {
            auto* sv = value->as_struct_value_unsafe();
            TypeId st = type;
            StructDefinition* sd = nullptr;
            UnionDef* ud = nullptr;
            UnnamedStruct* us = nullptr;
            UnnamedUnion* uu = nullptr;
            StructType* stc = nullptr;
            auto bind_container = [&](BaseType* ctype) {
                if (!ctype) return;
                ASTNode* cn = ctype->get_direct_linked_canonical_node();
                if (!cn) cn = ctype->get_direct_linked_node();
                if (!cn) return;
                if (cn->kind() == ASTNodeKind::StructDecl) {
                    sd = cn->as_struct_def_unsafe();
                } else if (cn->kind() == ASTNodeKind::UnionDecl) {
                    ud = cn->as_union_def_unsafe();
                } else if (cn->kind() == ASTNodeKind::UnnamedStruct) {
                    us = cn->as_unnamed_struct_unsafe();
                } else if (cn->kind() == ASTNodeKind::UnnamedUnion) {
                    uu = cn->as_unnamed_union_unsafe();
                } else if (cn->kind() == ASTNodeKind::StructType) {
                    stc = static_cast<StructType*>(cn);
                }
            };
            bind_container(value->getType());
            auto unresolved = [&]() -> bool {
                return (!sd && !ud && !us && !uu && !stc) || st == MIR_INVALID_ID ||
                       st >= module_.types.size() ||
                       module_.types.get(st).kind == MIRTypeKind::Opaque;
            };
            if (unresolved() && expected_struct_type_) {
                st = types_.map(expected_struct_type_);
                sd = nullptr;
                ud = nullptr;
                us = nullptr;
                stc = nullptr;
                bind_container(expected_struct_type_);
            }
            if (unresolved()) {
                // e.g. a generic struct literal whose instantiation MIR has not
                // resolved: fall back rather than emit `void*` field stores
                error = "struct literal with an unresolved (generic?) type is not yet supported";
                return MIRExprResult::error();
            }
            auto lookup_member_type = [&](const chem::string_view& nm) -> BaseType* {
                if (sd) {
                    auto r = sd->variable_type_w_index(nm);
                    if (r.second != -1) return r.first;
                }
                if (ud) {
                    for (BaseDefMember* v : ud->variables()) {
                        if (v && v->name == nm) return v->known_type();
                    }
                }
                if (us) {
                    for (BaseDefMember* v : us->variables()) {
                        if (v && v->name == nm) return v->known_type();
                    }
                }
                if (uu) {
                    for (BaseDefMember* v : uu->variables()) {
                        if (v && v->name == nm) return v->known_type();
                    }
                }
                if (stc) {
                    auto r = stc->variable_type_w_index(nm);
                    if (r.second != -1) return r.first;
                }
                return nullptr;
            };
            const PlaceId temp = builder_->alloca(st, MIRStorageClass::Temporary);
            if (temp == MIR_INVALID_ID) {
                error = "failed to allocate struct temporary";
                return MIRExprResult::error();
            }
            // The names the literal sets explicitly, computed before emission so
            // the zero_init can be skipped when nothing is left unset.
            MIRFieldNameSet explicit_fields;
            for (auto& kv : sv->values) {
                explicit_fields.insert(kv.first);
            }
            // Fields the literal *fully* defines. An array literal shorter than
            // the field's declared size lowers to a temp of exactly the literal's
            // length and copies only those bytes (the empty `[]` case is handled
            // by the array lowering, but a partial one decays), so the field's
            // tail is defined only by the enclosing zero_init -- treat such a
            // field as unset for the zero decision.
            MIRFieldNameSet zero_defined_fields = explicit_fields;
            for (auto& kv : sv->values) {
                Value* iv = kv.second.value;
                if (!iv || iv->val_kind() != ValueKind::ArrayValue) continue;
                BaseType* ft = nullptr;
                auto mtw = sv->child_type_w_index(kv.first);
                if (mtw.second != -1) ft = mtw.first;
                auto* fat = ft ? ft->as_array_type() : nullptr;
                if (!fat || !fat->has_array_size()) continue;
                if (static_cast<uint64_t>(iv->as_array_value_unsafe()->values.size()) <
                    fat->get_array_size()) {
                    zero_defined_fields.erase(kv.first);
                }
            }
            // Zero the temp so members that are not explicitly set are
            // well-defined (matches the legacy backend's default initialization)
            // -- but only when a member really is left unset. When the literal
            // defines every member (or the rest default-construct / take a
            // default value), the whole-temp zero is dead bytes the legacy
            // backend never emitted (evaluation §1.2, action C1).
            const bool all_defined =
                sd && struct_literal_defines_all_fields(sd, zero_defined_fields);
            if (!all_defined) {
                builder_->zero_init(temp);
            }
            auto it = sv->values.begin();
            while (it != sv->values.end()) {
                auto& init = it.value();
                const ConstantId fc = module_.constants.add_string(
                    MIR_INVALID_ID, init.name.data(), static_cast<uint32_t>(init.name.size()));
                // An empty array literal for a fixed-size array field
                // (`buffer : []`, the `std::default_function_instance` shape) means
                // "all zeroes". The partial-array guard above has already forced
                // the whole-temp `zero_init`, so every byte of the field is
                // defined: materializing a zeroed scratch array and copying its
                // whole width into the field is dead work the legacy backend never
                // emitted (action C1). Skip the store and rely on the temp zero.
                if (!all_defined && init.value &&
                    init.value->val_kind() == ValueKind::ArrayValue &&
                    init.value->as_array_value_unsafe()->values.empty()) {
                    BaseType* ft = nullptr;
                    auto mtw = sv->child_type_w_index(init.name);
                    if (mtw.second != -1) ft = mtw.first;
                    auto* fat = ft ? ft->canonical()->as_array_type() : nullptr;
                    if (fat && fat->has_array_size()) {
                        ++it;
                        continue;
                    }
                }
                // implicit conversion: if the member type has an implicit
                // constructor for the initializer value, construct into the field
                FunctionDeclaration* implicit = nullptr;
                BaseType* member_type = nullptr;
                {
                    auto mtw = sv->child_type_w_index(init.name);
                    if (mtw.second != -1) {
                        member_type = mtw.first;
                        ASTNode* ln = member_type->get_direct_linked_canonical_node();
                        if (ln && ln->kind() == ASTNodeKind::StructDecl) {
                            implicit = ln->as_struct_def_unsafe()->implicit_constructor_func(init.value);
                        }
                        if (!implicit) implicit = member_type->implicit_constructor_for(init.value);
                    }
                }
                // an anonymous union/struct literal field initializer has no
                // spellable C type for a temporary: initialize the field in place
                if (member_type && init.value &&
                    init.value->val_kind() == ValueKind::StructValue) {
                    ASTNode* mn = member_type->get_direct_linked_canonical_node();
                    if (mn && (mn->kind() == ASTNodeKind::UnnamedUnion ||
                               mn->kind() == ASTNodeKind::UnnamedStruct)) {
                        const TypeId mt = types_.map(member_type);
                        const ValueId faddr = builder_->field_addr(temp, fc, mt);
                        if (!store_anon_aggregate(faddr, init.value->as_struct_value_unsafe(),
                                                  error, !all_defined)) {
                            return MIRExprResult::error();
                        }
                        ++it;
                        continue;
                    }
                }
                if (implicit) {
                    // an `@expand` constructor is lowered at this site
                    if (is_comptime_template_fn(implicit)) {
                        MIRExprResult er = inline_comptime_template(
                            implicit, MIRExprResult::void_result(), nullptr,
                            std::vector<Value*>{init.value}, error);
                        if (!er.ok()) return er;
                        if (er.kind == MIRExprKind::Place) {
                            builder_->field_store_place(temp, fc, static_cast<PlaceId>(er.id));
                        } else if (er.kind == MIRExprKind::Value) {
                            builder_->field_store(temp, fc, static_cast<ValueId>(er.id));
                        } else {
                            error = "comptime constructor did not produce a value";
                            return MIRExprResult::error();
                        }
                        ++it;
                        continue;
                    }
                    if (implicit->is_comptime() && comptime_ctor_eval_) {
                        Value* evaluated = comptime_ctor_eval_(implicit, init.value);
                        if (evaluated) {
                            MIRExprResult er = lower_expr(evaluated, error);
                            if (!er.ok()) return er;
                            if (er.kind == MIRExprKind::Place) {
                                builder_->field_store_place(temp, fc, static_cast<PlaceId>(er.id));
                            } else if (er.kind == MIRExprKind::Value) {
                                builder_->field_store(temp, fc, static_cast<ValueId>(er.id));
                            } else {
                                error = "comptime constructor did not produce a value";
                                return MIRExprResult::error();
                            }
                            ++it;
                            continue;
                        }
                    }
                    const TypeId mt = types_.map(member_type);
                    const PlaceId tmp = builder_->alloca(mt, MIRStorageClass::Temporary);
                    MIRArgScope _ag_scope(*this);
                    std::vector<MIROperand>& cargs = _ag_scope.v;
                    std::vector<Value*> one{init.value};
                    if (!lower_call_args(one, cargs, error)) return MIRExprResult::error();
                    const SymbolId sym = intern_function(implicit);
                    builder_->zero_init(tmp);
                    builder_->init(tmp, sym, cargs.data(), static_cast<uint32_t>(cargs.size()));
                    builder_->field_store_place(temp, fc, tmp);
                    ++it;
                    continue;
                }
                MIRExprResult r = member_type
                                      ? lower_arg_converted(init.value, member_type, error)
                                      : lower_expr(init.value, error);
                if (!r.ok()) return r;
                // an array initializer for a pointer/reference field decays to a
                // pointer to its first element (never a byte copy, which would
                // overrun the pointer-sized field)
                if (r.kind == MIRExprKind::Place && member_type) {
                    BaseType* mtc = member_type->canonical();
                    BaseType* it0 = init.value ? init.value->getType() : nullptr;
                    BaseType* atc = it0 ? it0->canonical() : nullptr;
                    if (mtc && (mtc->kind() == BaseTypeKind::Pointer ||
                                mtc->kind() == BaseTypeKind::Reference) &&
                        atc && atc->kind() == BaseTypeKind::Array) {
                        const TypeId ptr_t = types_.map(member_type);
                        builder_->field_store(
                            temp, fc, builder_->address_of(static_cast<PlaceId>(r.id), ptr_t));
                        ++it;
                        continue;
                    }
                }
                if ((r.kind == MIRExprKind::Place || r.kind == MIRExprKind::Address) &&
                    member_type) {
                    BaseType* mtc = member_type->canonical();
                    if (mtc && (mtc->kind() == BaseTypeKind::Pointer ||
                                mtc->kind() == BaseTypeKind::Reference)) {
                        // a pointer/reference field takes a pointer: the RHS
                        // pointer value, or the address of an aggregate RHS (the
                        // emitter derefs a pointer-typed place RHS, which would
                        // be wrong here)
                        ValueId pv = MIR_NULL;
                        if (r.kind == MIRExprKind::Address) {
                            pv = static_cast<ValueId>(r.id);
                        } else {
                            const TypeId pt = builder_->function().places[r.id].type;
                            const MIRTypeRecord& pr = module_.types.get(pt);
                            pv = (pr.kind == MIRTypeKind::Pointer ||
                                  pr.kind == MIRTypeKind::Reference)
                                     ? builder_->load(static_cast<PlaceId>(r.id), pt)
                                     : builder_->address_of(static_cast<PlaceId>(r.id),
                                                            types_.pointer_type(pt, true));
                        }
                        builder_->field_store(temp, fc, pv);
                        ++it;
                        continue;
                    }
                }
                if (r.kind == MIRExprKind::Place) {
                    const PlaceId src = static_cast<PlaceId>(r.id);
                    builder_->field_store_place(temp, fc, src);
                    // a destructible local moved into the struct must not be
                    // destroyed again at scope exit (unwraps the single-element
                    // AccessChain the parser usually produces)
                    Value* v0 = init.value;
                    if (v0 && v0->val_kind() == ValueKind::AccessChain) {
                        auto* ch = v0->as_access_chain_unsafe();
                        if (ch->values.size() == 1) v0 = ch->values[0];
                    }
                    if (v0 && v0->val_kind() == ValueKind::Identifier) {
                        BaseType* at = v0->getType();
                        BaseType* canon = at ? at->canonical() : nullptr;
                        if (!(canon && canon->kind() == BaseTypeKind::Array)) {
                            mark_moved(src);
                        }
                    }
                    ++it;
                    continue;
                }
                if (r.kind == MIRExprKind::Address) {
                    // an lvalue address as a field initializer: load the pointee
                    if (!member_type) member_type = lookup_member_type(init.name);
                    const TypeId ft = member_type ? types_.map(member_type) : r.type;
                    builder_->field_store(
                        temp, fc,
                        builder_->load_indirect(static_cast<ValueId>(r.id), ft));
                    ++it;
                    continue;
                }
                if (r.kind != MIRExprKind::Value) {
                    error = "unsupported struct field initializer";
                    return MIRExprResult::error();
                }
                // if the field is an aggregate value but the initializer is a
                // pointer/reference, dereference to get the value
                if (!member_type) {
                    member_type = lookup_member_type(init.name);
                }
                if (member_type) {
                    const MIRTypeRecord& rt = module_.types.get(r.type);
                    if ((rt.kind == MIRTypeKind::Pointer ||
                         rt.kind == MIRTypeKind::Reference) &&
                        abi::is_aggregate(module_, types_.map(member_type))) {
                        builder_->field_store(
                            temp, fc,
                            builder_->load_indirect(r.id, types_.map(member_type)));
                        ++it;
                        continue;
                    }
                }
                builder_->field_store(temp, fc, r.id);
                ++it;
            }
            // initialize members that were not explicitly set with their default
            // values, and default-construct inherited bases
            if (sd) {
                ExtendableMembersContainerNode* defc =
                    sd ? static_cast<ExtendableMembersContainerNode*>(sd)
                       : (ud ? static_cast<ExtendableMembersContainerNode*>(ud) : nullptr);
                if (defc && !apply_struct_defaults(temp, defc, explicit_fields, error)) {
                    return MIRExprResult::error();
                }
            }
            return MIRExprResult::place(temp, st);
        }
        case ValueKind::FunctionCall: {
            auto* call = value->as_func_call_unsafe();
            // `intrinsics::llvm::atomic_*` lower to first-class MIR atomic ops.
            // This must precede the captured-comptime-ref shortcut: the atomic
            // intrinsics are comptime calls, so inside a `%runtime_value(...)`
            // the interpreter records the *call node itself* as the ref's
            // evaluated value (their operands resolve through their own captured
            // refs), and interpreting them here would require a live LLVM
            // IRBuilder and a real `llvm::Value` for each runtime pointer.
            if (FunctionDeclaration* afd = call->safe_linked_func()) {
                if (afd->is_compiler_decl() &&
                    atomic_intrinsic_of(afd) != AtomicIntrinsic::None) {
                    return lower_atomic_intrinsic(call, afd, error);
                }
            }
            FunctionDeclaration* fd = nullptr;
            MIRArgScope _ag_scope(*this);
            std::vector<MIROperand>& args = _ag_scope.v;

            // variant construction: `Variant.Member(args)` / `ns.Variant.Member(args)`
            {
                ASTNode* vleaf = nullptr;
                if (call->parent_val) {
                    if (call->parent_val->val_kind() == ValueKind::AccessChain) {
                        auto* ch = call->parent_val->as_access_chain_unsafe();
                        if (!ch->values.empty()) vleaf = ch->values.back()->linked_node();
                    } else {
                        vleaf = call->parent_val->linked_node();
                    }
                }
                if (vleaf && vleaf->kind() == ASTNodeKind::VariantMember) {
                    auto* vm = vleaf->as_variant_member_unsafe();
                    VariantDefinition* vd = vm->parent();
                    const TypeId vt = types_.map(call->getType());
                    const PlaceId temp = builder_->alloca(vt, MIRStorageClass::Temporary);
                    // The whole-temp zero defines the bytes the active case does
                    // not write -- but the only bytes ever read for a variant case
                    // are its discriminator and its own payload, so the zero is
                    // dead once every payload field is written (the union tail is
                    // never read). It is still needed when a payload parameter is
                    // omitted without a default, a payload is a partially-written
                    // array, or the variant has an inherited base. The legacy
                    // backend emitted no zero at all; see action C1 in
                    // mir-migration-evaluation.md.
                    bool payload_defined = true;
                    {
                        size_t pi = 0;
                        for (auto& entry : vm->values) {
                            VariantMemberParam* param = entry.second;
                            Value* arg = nullptr;
                            if (pi < call->values.size()) {
                                arg = call->values[pi];
                                pi++;
                            } else if (param) {
                                arg = param->def_value;
                            }
                            if (!arg) {
                                payload_defined = false;
                                break;
                            }
                            BaseType* pt = param ? param->type : nullptr;
                            BaseType* canon = pt ? pt->canonical() : nullptr;
                            if (canon && canon->kind() == BaseTypeKind::Array) {
                                // a partial array literal defines only its own
                                // bytes; the tail depends on the zero
                                payload_defined = false;
                                break;
                            }
                        }
                    }
                    if (!(payload_defined && vd && vd->inherited.empty())) {
                        builder_->zero_init(temp);
                    }
                    const std::string tag_name("__chx__vt_621827");
                    const ConstantId tag_fc = module_.constants.add_string(
                        MIR_INVALID_ID, tag_name.data(), static_cast<uint32_t>(tag_name.size()));
                    const TypeId it = types_.u32_type();
                    const long idx = vd ? vd->direct_child_index(vm->name) : 0;
                    const ValueId tag = builder_->const_int(
                        it, module_.constants.add_int(
                                it, static_cast<uint64_t>(idx < 0 ? 0 : idx)));
                    builder_->field_store(temp, tag_fc, tag);
                    size_t ai = 0;
                    for (auto& entry : vm->values) {
                        VariantMemberParam* param = entry.second;
                        Value* arg = nullptr;
                        if (ai < call->values.size()) {
                            arg = call->values[ai];
                            ai++;
                        } else if (param) {
                            // an omitted parameter with a default value
                            arg = param->def_value;
                        }
                        if (!arg) continue;
                        // `<case>.<param>` into one reusable buffer: this ran three
                        // `std::string`s per variant argument
                        std::string& fname = variant_field_path_buf();
                        fname.assign(vm->name.data(), vm->name.size());
                        fname += '.';
                        fname.append(entry.first.data(), entry.first.size());
                        const ConstantId fc = module_.constants.add_string(
                            MIR_INVALID_ID, fname.data(), static_cast<uint32_t>(fname.size()));
                        MIRExprResult r = lower_arg_converted(
                            arg, param ? param->type : nullptr, error);
                        if (!r.ok()) return r;
                        if (r.kind == MIRExprKind::Place) {
                            const PlaceId src = static_cast<PlaceId>(r.id);
                            builder_->field_store_place(temp, fc, src);
                            // a destructible local moved into the variant must not
                            // be destroyed again at scope exit (mirrors the
                            // function-call argument move handling). The argument
                            // is often a single-element AccessChain wrapping the
                            // identifier, so unwrap it first.
                            Value* v0 = arg;
                            if (v0 && v0->val_kind() == ValueKind::AccessChain) {
                                auto* ch = v0->as_access_chain_unsafe();
                                if (ch->values.size() == 1) v0 = ch->values[0];
                            }
                            if (v0 && v0->val_kind() == ValueKind::Identifier) {
                                BaseType* at = v0->getType();
                                BaseType* canon = at ? at->canonical() : nullptr;
                                if (!(canon && canon->kind() == BaseTypeKind::Array)) {
                                    mark_moved(src);
                                }
                            }
                        } else if (r.kind == MIRExprKind::Address) {
                            const TypeId ft = param && param->type ? types_.map(param->type) : r.type;
                            const ValueId loaded =
                                builder_->load_indirect(static_cast<ValueId>(r.id), ft);
                            builder_->field_store(temp, fc, loaded);
                        } else {
                            builder_->field_store(temp, fc, static_cast<ValueId>(r.id));
                        }
                    }
                    if (!apply_struct_defaults(temp, vd, {}, error)) {
                        return MIRExprResult::error();
                    }
                    return MIRExprResult::place(temp, vt);
                }
            }

            // aggregate construction: `Type(args)` / `ns.Type(args)` /
            // `ns.Generic<...>(args)`
            ASTNode* ctor_node = nullptr;
            if (call->parent_val) {
                // a call whose parent is itself a call is a function-pointer call,
                // not a constructor: `parent->linked_node()` would be the inner
                // call's *return type* (a struct), which is misleading
                const bool parent_is_call =
                    call->parent_val->val_kind() == ValueKind::FunctionCall;
                ASTNode* plk = parent_is_call ? nullptr : call->parent_val->linked_node();
                if (plk && plk->as_struct_def()) {
                    ctor_node = plk;
                } else if (call->parent_val->val_kind() == ValueKind::AccessChain) {
                    auto* ch = call->parent_val->as_access_chain_unsafe();
                    if (!ch->values.empty()) {
                        ASTNode* lk = ch->values.back()->linked_node();
                        if (lk && (lk->as_struct_def() || lk->kind() == ASTNodeKind::GenericStructDecl)) {
                            ctor_node = lk;
                        }
                    }
                }
            }
            if (!ctor_node && call->getType() && call->parent_val &&
                call->parent_val->val_kind() == ValueKind::AccessChain) {
                // unlinked constructor call (`std::string_view(...)`): match the
                // callee name against the call's aggregate type name
                auto* ch = call->parent_val->as_access_chain_unsafe();
                std::string mname, merr;
                if (!ch->values.empty() && member_name(ch->values.back(), mname, merr)) {
                    ASTNode* cn = call->getType()->get_direct_linked_canonical_node();
                    if (cn && cn->kind() == ASTNodeKind::StructDecl) {
                        auto* sd = cn->as_struct_def_unsafe();
                        if (sd->name_view() ==
                            chem::string_view(mname.data(), static_cast<unsigned>(mname.size()))) {
                            ctor_node = cn;
                        }
                    }
                }
            }
            if (ctor_node) {
                BaseType* ctype = call->getType();
                const TypeId st = types_.map(ctype);
                const PlaceId place = builder_->alloca(st, MIRStorageClass::Temporary);
                StructDefinition* sd = nullptr;
                if (ctype) {
                    ASTNode* cn = ctype->get_direct_linked_canonical_node();
                    if (cn && cn->kind() == ASTNodeKind::StructDecl) {
                        sd = cn->as_struct_def_unsafe();
                    }
                }
                if (!sd) sd = ctor_node->as_struct_def();
                FunctionDeclaration* ctor = sd ? sd->constructor_func(call->values) : nullptr;
                if (!ctor && ctype && call->values.empty()) ctor = ctype->get_def_constructor();
                // an `@expand` constructor is lowered at this call site
                if (ctor && is_comptime_template_fn(ctor)) {
                    return inline_comptime_template(ctor, MIRExprResult::void_result(),
                                                    call, call->values, error);
                }
                // evaluate a comptime constructor before lowering its arguments
                // (they may be comptime-only values such as expressive strings)
                if (ctor && ctor->is_comptime() && comptime_eval_) {
                    Value* ev = comptime_eval_(call, ctor);
                    if (ev) return lower_expr(ev, error);
                }
                MIRArgScope _ag_scope(*this);
                std::vector<MIROperand>& cargs = _ag_scope.v;
                // constructors have no `self` parameter in `params` (the result
                // is passed through the hidden sret pointer)
                const bool ctor_self = ctor && ctor->get_self_param() != nullptr;
                if (!lower_call_args_for(ctor, ctor_self, call->values, cargs, error)) {
                    return MIRExprResult::error();
                }
                if (ctor && !append_default_args(ctor, call->values.size(), ctor_self, cargs,
                                                 error)) {
                    return MIRExprResult::error();
                }
                if (ctor) {
                    const SymbolId sym = intern_function(ctor);
                    // the result temp's zero is dead when the construction writes
                    // every member: a `return T{...}` constructor's result is a
                    // whole-value copy of a fully defined temporary, and a
                    // generated constructor's body is exactly
                    // apply_struct_defaults_ptr over the container
                    // (evaluation §1.2, action C1)
                    ExtendableMembersContainerNode* cdef =
                        ctor->parent() ? ctor->parent()->as_extendable_member_container() : nullptr;
                    const bool covered = construction_writes_whole_value(ctor, cdef);
                    if (!covered) {
                        builder_->zero_init(place);
                    }
                    builder_->init(place, sym, cargs.data(), static_cast<uint32_t>(cargs.size()));
                } else if (!cargs.empty()) {
                    error = "no matching constructor for aggregate construction";
                    return MIRExprResult::error();
                }
                return MIRExprResult::place(place, st);            }

            // method call: `receiver.method(args)`
            if (call->parent_val && call->parent_val->val_kind() == ValueKind::AccessChain) {
                auto* chain = call->parent_val->as_access_chain_unsafe();
                if (chain->values.size() >= 2) {
                    // resolve the callee first: a namespace-qualified function
                    // (e.g. `lab::curr_dir()`) must not be mistaken for a
                    // constructor even when it returns an aggregate
                    ASTNode* callee_node = call->parent_val->linked_node();
                    FunctionDeclaration* mfd = callee_node ? callee_node->as_function() : nullptr;
                    if (!mfd) {
                        // fallback: the method identifier may not have been linked
                        // (extension methods on an interface receiver); resolve it
                        // by name on the receiver's canonical container.
                        std::string mname, merr;
                        if (member_name(chain->values.back(), mname, merr)) {
                            mfd = resolve_method_fallback(chain->values[0]->getType(), mname);
                        }
                    }
                    // namespace-qualified aggregate construction: the callee is
                    // not a function, the receiver is not an addressable place,
                    // and the result is an aggregate
                    // a yes/no answer: the silent form builds no diagnostic string
                    // (this runs for every method call)
                    const bool recv_is_place =
                        resolve_place_silent(chain->values[0]) != MIR_INVALID_ID;
                    const TypeId ct = types_.map(call->getType());
                    // a struct-member callee is a field call (`st.task()`), never
                    // an aggregate construction -- the receiver may be a captured
                    // variable (not a place), which must not trigger this path
                    const bool callee_is_field =
                        callee_node && callee_node->kind() == ASTNodeKind::StructMember;
                    if (!mfd && !recv_is_place && !callee_is_field &&
                        abi::is_aggregate(module_, ct)) {
                        const PlaceId place = builder_->alloca(ct, MIRStorageClass::Temporary);
                        FunctionDeclaration* ctor = nullptr;
                        if (call->getType()) {
                            ASTNode* cn = call->getType()->get_direct_linked_canonical_node();
                            if (cn && cn->kind() == ASTNodeKind::StructDecl) {
                                ctor = cn->as_struct_def_unsafe()->constructor_func(call->values);
                            }
                        }
                        if (!ctor && call->getType() && call->values.empty()) {
                            ctor = call->getType()->get_def_constructor();
                        }
                        MIRArgScope _ag_scope(*this);
                        std::vector<MIROperand>& cargs = _ag_scope.v;
                        const bool ctor_self = ctor && ctor->get_self_param() != nullptr;
                        if (!lower_call_args_for(ctor, ctor_self, call->values, cargs, error)) {
                            return MIRExprResult::error();
                        }
                        if (ctor && !append_default_args(ctor, call->values.size(), ctor_self, cargs,
                                                         error)) {
                            return MIRExprResult::error();
                        }
                        if (ctor && ctor->is_comptime() && comptime_eval_) {
                            Value* ev = comptime_eval_(call, ctor);
                            if (ev) return lower_expr(ev, error);
                        }
                        if (ctor) {
                            const SymbolId sym = intern_function(ctor);
                            // as above: a `return T{...}` constructor writes the
                            // whole result, and a generated constructor covers
                            // every field exactly when default init does (C1)
                            ExtendableMembersContainerNode* cdef =
                                ctor->parent() ? ctor->parent()->as_extendable_member_container()
                                               : nullptr;
                            const bool covered = construction_writes_whole_value(ctor, cdef);
                            if (!covered) {
                                builder_->zero_init(place);
                            }
                            builder_->init(place, sym, cargs.data(), static_cast<uint32_t>(cargs.size()));
                        } else if (!cargs.empty()) {
                            error = "no matching constructor for aggregate construction";
                            return MIRExprResult::error();
                        }
                        return MIRExprResult::place(place, ct);
                    }
                    const bool needs_receiver =
                        mfd && (mfd->get_self_param() != nullptr || mfd->isExtensionFn());
                    if (needs_receiver) {
                        const size_t n = chain->values.size();
                        MIRExprResult recv = lower_receiver(chain->values, n - 1, error);
                        if (!recv.ok()) return recv;
                        BaseType* rtype = chain->values[n - 2]->getType();
                        return lower_method_call(recv, rtype, call, error);
                    }
                    // a non-self method called on a call receiver: evaluate the
                    // receiver for its side effects (no receiver is passed)
                    if (!needs_receiver && chain->values[0] &&
                        chain->values[0]->val_kind() == ValueKind::FunctionCall) {
                        MIRExprResult recv = lower_expr(chain->values[0], error);
                        if (!recv.ok()) return recv;
                        maybe_register_call_temp(chain->values[0], recv);
                    }
                    // otherwise: namespace/type-qualified free function or a
                    // static method -- fall through to the direct-call path,
                    // which passes no receiver.
                }
            }
            {
                // when the callee is itself a call (`f()()` / `factory.create()()`),
                // the outer call goes through the result, not the inner function
                const bool parent_is_call =
                    call->parent_val && call->parent_val->val_kind() == ValueKind::FunctionCall;
                ASTNode* linked_fn =
                    (!parent_is_call && call->parent_val) ? call->parent_val->linked_node()
                                                          : nullptr;
                fd = linked_fn ? linked_fn->as_function() : nullptr;
                if (!fd) {
                    // `obj.field(args)` where `field` is a capturing function
                    // (`std::function`) member: dispatch through the closure's
                    // fn/data pointers. The access-chain's type can resolve to
                    // the *inner* function signature inside a generic
                    // instantiation, so key on the member's declared type.
                    if (linked_fn && linked_fn->kind() == ASTNodeKind::StructMember &&
                        call->parent_val &&
                        call->parent_val->val_kind() == ValueKind::AccessChain) {
                        auto* sm = linked_fn->as_struct_member_unsafe();
                        BaseType* ft_ast = sm ? sm->type : nullptr;
                        BaseType* ftc = ft_ast ? ft_ast->canonical() : nullptr;
                        bool field_is_cap =
                            ftc && (ftc->kind() == BaseTypeKind::CapturingFunction ||
                                    ftc->get_cap_func_type() != nullptr);
                        if (!field_is_cap && ftc) {
                            ASTNode* cln = ftc->get_direct_linked_canonical_node();
                            if (cln && cln->kind() == ASTNodeKind::StructDecl) {
                                auto* sd = cln->as_struct_def_unsafe();
                                field_is_cap = sd->child("get_fn_ptr") != nullptr &&
                                               sd->child("get_data_ptr") != nullptr;
                            }
                        }
                        if (field_is_cap) {
                            MIRExprResult fpr = lower_expr(call->parent_val, error);
                            if (!fpr.ok()) return fpr;
                            return lower_capturing_function_call(fpr, ft_ast, call, error);
                        }
                    }
                    // function-pointer variable/parameter call -> call_indirect
                    PlaceId fp = linked_fn ? place_for_linked(linked_fn) : MIR_INVALID_ID;
                    if (fp == MIR_INVALID_ID && call->parent_val &&
                        call->parent_val->val_kind() == ValueKind::Identifier) {
                        fp = place_for_linked(call->parent_val->as_identifier_unsafe()->linked);
                    }
                    if (fp == MIR_INVALID_ID) {
                        // a call through a capturing function instance that is not
                        // a plain place (e.g. `make_fn()()`): lower the receiver
                        // expression and dispatch through it indirectly
                        if (call->parent_val && call->parent_val->getType()) {
                            BaseType* rt = call->parent_val->getType();
                            BaseType* rtc = rt->canonical();
                            bool is_cap = rtc && rtc->get_cap_func_type() != nullptr;
                            if (!is_cap && rtc) {
                                ASTNode* cln = rtc->get_direct_linked_canonical_node();
                                if (cln && cln->kind() == ASTNodeKind::StructDecl) {
                                    auto* sd = cln->as_struct_def_unsafe();
                                    is_cap = sd->child("get_fn_ptr") != nullptr &&
                                             sd->child("get_data_ptr") != nullptr;
                                }
                            }
                            if (is_cap) {
                                MIRExprResult recv = lower_expr(call->parent_val, error);
                                if (!recv.ok()) return recv;
                                return lower_capturing_function_call(recv, rt, call, error);
                            }
                            // a call returning a plain function pointer:
                            // `factory.create_lamb()()`
                            if (!is_cap && rtc && rtc->kind() == BaseTypeKind::Function) {
                                MIRExprResult fpr = lower_expr(call->parent_val, error);
                                if (!fpr.ok()) return fpr;
                                const ValueId fnptr =
                                    (fpr.kind == MIRExprKind::Place)
                                        ? builder_->load(static_cast<PlaceId>(fpr.id), fpr.type)
                                        : static_cast<ValueId>(fpr.id);
                                // the AST function type itself carries the parameter
                                // types: no per-call `std::vector<BaseType*>`
                                FunctionType* ft_fn = rtc->get_canonical_function_type();
                                MIRArgScope _ag_scope(*this);
                                std::vector<MIROperand>& cargs = _ag_scope.v;
                                const size_t cmark = call_temps_.size();
                                if (!lower_call_args_for(nullptr, false, call->values, cargs,
                                                         error, ft_fn)) {
                                    return MIRExprResult::error();
                                }
                                const TypeId crt = types_.map(call->getType());
                                if (is_void_type(module_, crt)) {
                                    builder_->call_indirect(fnptr, crt, cargs.data(),
                                                            static_cast<uint32_t>(cargs.size()));
                                    destroy_call_temps_from(cmark, error);
                                    return MIRExprResult::void_result();
                                }
                                if (abi::needs_sret(module_, crt)) {
                                    const PlaceId tmp =
                                        builder_->alloca(crt, MIRStorageClass::Temporary);
                                    builder_->zero_init(tmp);
                                    builder_->call_indirect_sret(
                                        fnptr, tmp, cargs.data(),
                                        static_cast<uint32_t>(cargs.size()));
                                    destroy_call_temps_from(cmark, error);
                                    return MIRExprResult::place(tmp, crt);
                                }
                                const ValueId v = builder_->call_indirect(
                                    fnptr, crt, cargs.data(),
                                    static_cast<uint32_t>(cargs.size()));
                                destroy_call_temps_from(cmark, error);
                                return MIRExprResult::value(v, crt);
                            }
                        }
                        // `obj.field(args)` where `field` is a function-typed
                        // struct member: load the function pointer and dispatch
                        if (linked_fn && linked_fn->kind() == ASTNodeKind::StructMember &&
                            call->parent_val &&
                            call->parent_val->val_kind() == ValueKind::AccessChain) {
                            auto* sm = linked_fn->as_struct_member_unsafe();
                            BaseType* ft_ast = sm ? sm->type : nullptr;
                            FunctionType* ft_fn =
                                ft_ast ? ft_ast->canonical()->get_canonical_function_type()
                                       : nullptr;
                            MIRExprResult fpr = lower_expr(call->parent_val, error);
                            if (!fpr.ok()) return fpr;
                            if (fpr.type != MIR_INVALID_ID && fpr.type < module_.types.size() &&
                                module_.types.get(fpr.type).kind == MIRTypeKind::Struct) {
                                return lower_capturing_function_call(fpr, ft_ast, call, error);
                            }
                            const ValueId fnptr =
                                (fpr.kind == MIRExprKind::Place)
                                    ? builder_->load(static_cast<PlaceId>(fpr.id), fpr.type)
                                    : static_cast<ValueId>(fpr.id);
                            MIRArgScope _ag_scope(*this);
                            std::vector<MIROperand>& cargs = _ag_scope.v;
                            const size_t cmark = call_temps_.size();
                            if (!lower_call_args_for(nullptr, false, call->values, cargs, error,
                                                     ft_fn)) {
                                return MIRExprResult::error();
                            }
                            const TypeId rt = types_.map(call->getType());
                            if (is_void_type(module_, rt)) {
                                builder_->call_indirect(fnptr, rt, cargs.data(),
                                                        static_cast<uint32_t>(cargs.size()));
                                destroy_call_temps_from(cmark, error);
                                return MIRExprResult::void_result();
                            }
                            if (abi::needs_sret(module_, rt)) {
                                const PlaceId tmp =
                                    builder_->alloca(rt, MIRStorageClass::Temporary);
                                builder_->zero_init(tmp);
                                builder_->call_indirect_sret(
                                    fnptr, tmp, cargs.data(),
                                    static_cast<uint32_t>(cargs.size()));
                                destroy_call_temps_from(cmark, error);
                                return MIRExprResult::place(tmp, rt);
                            }
                            const ValueId v =
                                builder_->call_indirect(fnptr, rt, cargs.data(),
                                                        static_cast<uint32_t>(cargs.size()));
                            destroy_call_temps_from(cmark, error);
                            return MIRExprResult::value(v, rt);
                        }
                        std::string callee;
                        int last_linked_kind = -2;
                        int pv_kind = -1;
                        if (call->parent_val) {
                            Value* pv = call->parent_val;
                            pv_kind = static_cast<int>(pv->val_kind());
                            if (pv->val_kind() == ValueKind::AccessChain) {
                                auto* ch = pv->as_access_chain_unsafe();
                                if (!ch->values.empty()) {
                                    std::string n, e;
                                    if (member_name(ch->values.back(), n, e)) callee = n;
                                    ASTNode* ll = ch->values.back()->linked_node();
                                    last_linked_kind = ll ? static_cast<int>(ll->kind()) : -1;
                                }
                            } else if (pv->val_kind() == ValueKind::Identifier) {
                                auto* id = pv->as_identifier_unsafe();
                                callee.assign(id->value.data(), id->value.size());
                                last_linked_kind = id->linked ? static_cast<int>(id->linked->kind()) : -1;
                            }
                        }
                        error = "call to unresolved function '" + callee + "' (linked kind " +
                                std::to_string(linked_fn
                                                   ? static_cast<int>(linked_fn->kind())
                                                   : -1) +
                                ", pv kind " + std::to_string(pv_kind) + ", last linked " +
                                std::to_string(last_linked_kind) + ")";
                        return MIRExprResult::error();
                    }
                    const TypeId ft = builder_->function().places[fp].type;
                    // the function pointer's AST function type (its parameter
                    // types decide by-value vs by-reference) — read directly, so
                    // no per-call `std::vector<BaseType*>` is built
                    FunctionType* ast_fn = nullptr;
                    {
                        BaseType* fp_ast = nullptr;
                        if (linked_fn) {
                            if (linked_fn->kind() == ASTNodeKind::FunctionParam) {
                                fp_ast = linked_fn->as_func_param_unsafe()->type;
                            } else if (linked_fn->kind() == ASTNodeKind::VarInitStmt) {
                                fp_ast = linked_fn->as_var_init()->known_type();
                            }
                        }
                        if (!fp_ast && call->parent_val) fp_ast = call->parent_val->getType();
                        if (fp_ast) ast_fn = fp_ast->canonical()->get_canonical_function_type();
                    }
                    if (ft != MIR_INVALID_ID && ft < module_.types.size()) {
                        MIRTypeRecord eff = module_.types.get(ft);
                        if ((eff.kind == MIRTypeKind::Pointer ||
                             eff.kind == MIRTypeKind::Reference) &&
                            eff.element < module_.types.size()) {
                            eff = module_.types.get(eff.element);
                        }
                        if (eff.kind == MIRTypeKind::Struct) {
                            // capturing function instance (`std::function`) call
                            BaseType* recv_ast =
                                call->parent_val ? call->parent_val->getType() : nullptr;
                            return lower_capturing_function_call(MIRExprResult::place(fp, ft),
                                                                 recv_ast, call, error);
                        }
                    }
                    const ValueId fnptr = builder_->load(fp, ft);
                    MIRArgScope _ag_scope(*this);
                    std::vector<MIROperand>& cargs = _ag_scope.v;
                    const size_t cmark = call_temps_.size();
                    if (!lower_call_args_for(nullptr, false, call->values, cargs, error,
                                             ast_fn)) {
                        return MIRExprResult::error();
                    }
                    const TypeId rt = types_.map(call->getType());
                    if (is_void_type(module_, rt)) {
                        builder_->call_indirect(fnptr, rt, cargs.data(), static_cast<uint32_t>(cargs.size()));
                        destroy_call_temps_from(cmark, error);
                        return MIRExprResult::void_result();
                    }
                    if (abi::needs_sret(module_, rt)) {
                        // aggregate return: use the sret convention (the callee
                        // returns void and writes through a leading pointer)
                        const PlaceId tmp = builder_->alloca(rt, MIRStorageClass::Temporary);
                        builder_->zero_init(tmp);
                        builder_->call_indirect_sret(fnptr, tmp, cargs.data(),
                                                     static_cast<uint32_t>(cargs.size()));
                        destroy_call_temps_from(cmark, error);
                        return MIRExprResult::place(tmp, rt);
                    }
                    const ValueId v = builder_->call_indirect(fnptr, rt, cargs.data(),
                                                              static_cast<uint32_t>(cargs.size()));
                    destroy_call_temps_from(cmark, error);
                    return MIRExprResult::value(v, rt);
                }
            }
            // an unqualified method call inside a method (`reserve(n)`) is a
            // call on the implicit receiver `self`
            if (fd && fd->has_self_param() && current_self_param_ && call->parent_val &&
                call->parent_val->val_kind() == ValueKind::Identifier) {
                VariableIdentifier self_id(current_self_param_->name,
                                           current_self_param_->type, ZERO_LOC);
                self_id.linked = current_self_param_;
                MIRExprResult recv = lower_expr(&self_id, error);
                if (!recv.ok()) return recv;
                return lower_method_call(recv, current_self_param_->type, call, error);
            }
            // `intrinsics::forget(x)` clears x's drop flag (the value is
            // deliberately not destroyed at scope exit); it has no runtime
            // effect, so model it directly instead of via the comptime value
            if (fd->is_compiler_decl() && fd->name_view() == chem::string_view("forget") &&
                call->values.size() == 1) {
                PlaceId p = resolve_place(call->values.front(), error);
                if (p != MIR_INVALID_ID) mark_moved(p);
                return MIRExprResult::void_result();
            }
            // comptime-mode intrinsics: fold from the function being lowered.
            // Interpretation is the same semantics as compiled mode, so
            // is_interpretation() is constant false.
            if (fd->name_view() == chem::string_view("is_comptime") ||
                fd->name_view() == chem::string_view("is_runtime") ||
                fd->name_view() == chem::string_view("is_interpretation")) {
                const bool is_ct = fd->name_view() == chem::string_view("is_comptime") &&
                                   current_fn_is_comptime_;
                const TypeId bt = types_.bool_type();
                return MIRExprResult::value(
                    builder_->const_bool(bt, module_.constants.add_bool(bt, is_ct)), bt);
            }
            if (fd->is_comptime() && comptime_eval_) {
                if (is_comptime_template_fn(fd)) {
                    return inline_comptime_template(fd, MIRExprResult::void_result(),
                                                    call, call->values, error);
                }
                Value* evaluated = comptime_eval_(call, fd);
                if (!evaluated) {
                    // a void comptime call whose evaluation produced no value
                    // (e.g. an inline-asm fence) is a no-op
                    if (is_void_type(module_, type)) return MIRExprResult::void_result();
                    error = "comptime call to '" + fd->name_str() + "' could not be evaluated";
                    return MIRExprResult::error();
                }
                return lower_expr(evaluated, error);
            }
            // a non-self interface default called as `receiver.method(...)` is
            // still emitted per implementing struct
            BaseType* direct_recv_type = nullptr;
            if (call->parent_val && call->parent_val->val_kind() == ValueKind::AccessChain) {
                auto* dch = call->parent_val->as_access_chain_unsafe();
                if (dch->values.size() >= 2) {
                    direct_recv_type = dch->values[dch->values.size() - 2]->getType();
                }
            }
            SymbolId sym = intern_method_function(fd, direct_recv_type);

            args.reserve(args.size() + call->values.size());
            const size_t cmark = call_temps_.size();
            if (!lower_call_args_for(fd, false, call->values, args, error)) {
                return MIRExprResult::error();
            }
            if (!append_default_args(fd, call->values.size(), false, args, error)) {
                return MIRExprResult::error();
            }

            if (is_void_type(module_, type)) {
                builder_->call_scalar(sym, type, args.data(), static_cast<uint32_t>(args.size()));
                destroy_call_temps_from(cmark, error);
                return MIRExprResult::void_result();
            }
            if (abi::needs_sret(module_, type)) {
                const PlaceId res = builder_->alloca(type, MIRStorageClass::Temporary);
                if (res == MIR_INVALID_ID) {
                    error = "failed to allocate struct-return result place";
                    return MIRExprResult::error();
                }
                builder_->call_sret(sym, res, args.data(), static_cast<uint32_t>(args.size()));
                destroy_call_temps_from(cmark, error);
                return MIRExprResult::place(res, type);
            }
            ValueId v = builder_->call_scalar(sym, type, args.data(), static_cast<uint32_t>(args.size()));
            destroy_call_temps_from(cmark, error);
            return MIRExprResult::value(v, type);
        }
        case ValueKind::AddrOfValue: {
            auto* a = value->as_addr_of_value_unsafe();
            return lower_address_of(a->value, error, a->is_mutable);
        }
        case ValueKind::ReferenceOfValue: {
            auto* r = value->as_reference_of_value_unsafe();
            // `&x`: an aggregate parameter passed as a hidden pointer yields its
            // referent; everything else takes the address of the expression.
            Value* inner = r->value;
            if (inner && inner->val_kind() == ValueKind::AccessChain) {
                auto* c = inner->as_access_chain_unsafe();
                if (c->values.size() == 1) inner = c->values[0];
            }
            if (inner && inner->val_kind() == ValueKind::Identifier) {
                auto* id = inner->as_identifier_unsafe();
                ASTNode* ln = id->linked;
                bool hidden = false;
                if (ln && ln->kind() == ASTNodeKind::FunctionParam) {
                    BaseType* pt = ln->as_func_param_unsafe()->type;
                    hidden = pt && pt->kind() != BaseTypeKind::Dynamic &&
                             pt->isStructLikeType();
                }
                if (!hidden) {
                    PlaceId p = place_for_linked(ln);
                    if (p == MIR_INVALID_ID) p = place_for_name(id->value);
                    if (p != MIR_INVALID_ID) {
                        const TypeId ptrt = types_.pointer_type(
                            builder_->function().places[p].type, true);
                        return MIRExprResult::value(builder_->address_of(p, ptrt), ptrt);
                    }
                }
            }
            return lower_address_of(r->value, error);
        }
        case ValueKind::ArrayValue: {
            auto* arr = value->as_array_value_unsafe();
            TypeId at = type;
            bool decay = false;
            TypeId elem_t = MIR_INVALID_ID;
            // an empty literal `[]` in a pointer context is a null pointer
            if (arr->values.empty() && at != MIR_INVALID_ID && at < module_.types.size() &&
                (module_.types.get(at).kind == MIRTypeKind::Pointer ||
                 module_.types.get(at).kind == MIRTypeKind::Reference)) {
                const ConstantId c = module_.constants.add_int(at, 0);
                return MIRExprResult::value(builder_->const_null(at, c), at);
            }
            if (at == MIR_INVALID_ID || at >= module_.types.size() ||
                module_.types.get(at).kind != MIRTypeKind::Array) {
                // an unsized array literal (`const x = [...]`, a pointer context)
                // decays to a pointer to its first element: build a sized array
                // temporary for the literal and return its address
                BaseType* elem_bt = arr->known_elem_type();
                if (elem_bt) elem_t = types_.map(elem_bt);
                if (elem_t == MIR_INVALID_ID ||
                    static_cast<uint32_t>(arr->values.size()) == 0) {
                    error = "array literal without an array type";
                    return MIRExprResult::error();
                }
                at = types_.sized_array_type(elem_t,
                                             static_cast<uint32_t>(arr->values.size()));
                decay = true;
            }
            const PlaceId place = builder_->alloca(at, MIRStorageClass::Temporary);
            // Zero the array so elements not covered by the literal are 0 -- but
            // only when the literal leaves tail elements. A literal that fills
            // the array (the common case, including the decayed unsized form,
            // which is sized to the literal) writes every element, so the
            // whole-array zero is dead bytes the legacy backend never emitted
            // (evaluation §1.2, action C1).
            const bool full_literal =
                at != MIR_INVALID_ID && at < module_.types.size() &&
                module_.types.get(at).kind == MIRTypeKind::Array &&
                arr->values.size() >=
                    static_cast<size_t>(module_.types.get(at).data_count);
            if (!full_literal) {
                builder_->zero_init(place);
            }
            const TypeId ut = types_.u32_type();
            BaseType* elem_bt = arr->known_elem_type();
            for (size_t i = 0; i < arr->values.size(); ++i) {
                MIRExprResult e = lower_arg_converted(arr->values[i], elem_bt, error);
                if (!e.ok()) return e;
                ValueId val = MIR_NULL;
                if (e.kind == MIRExprKind::Place) {
                    const PlaceId pid = static_cast<PlaceId>(e.id);
                    const TypeId pt = builder_->function().places[pid].type;
                    const ValueId idx = builder_->const_int(
                        ut, module_.constants.add_int(ut, static_cast<uint64_t>(i)));
                    if (abi::is_aggregate(module_, pt)) {
                        builder_->element_store_place(place, idx, pid);
                        // a destructible local moved into the array must not be
                        // destroyed again at scope exit
                        Value* ev = arr->values[i];
                        if (ev && ev->val_kind() == ValueKind::AccessChain) {
                            auto* ch = ev->as_access_chain_unsafe();
                            if (ch->values.size() == 1) ev = ch->values[0];
                        }
                        if (ev && ev->val_kind() == ValueKind::Identifier) {
                            BaseType* at = ev->getType();
                            BaseType* canon = at ? at->canonical() : nullptr;
                            if (!(canon && canon->kind() == BaseTypeKind::Array)) {
                                mark_moved(pid);
                            }
                        }
                        continue;
                    }
                    val = builder_->load(pid, pt);
                    builder_->element_store(place, idx, val);
                    continue;
                }
                const ValueId idx = builder_->const_int(
                    ut, module_.constants.add_int(ut, static_cast<uint64_t>(i)));
                if (e.kind == MIRExprKind::Address) {
                    const TypeId et = types_.map(elem_bt);
                    if (abi::is_aggregate(module_, et)) {
                        // `*ptr` element: copy the pointee into the element
                        builder_->element_store(place, idx,
                                                builder_->load_indirect(e.id, et));
                    } else {
                        builder_->element_store(place, idx, static_cast<ValueId>(e.id));
                    }
                    continue;
                }
                if (e.kind == MIRExprKind::Value) {
                    val = static_cast<ValueId>(e.id);
                } else {
                    error = "unsupported array element expression";
                    return MIRExprResult::error();
                }
                builder_->element_store(place, idx, val);
            }
            if (decay) {
                const TypeId ptrt = types_.pointer_type(elem_t, true);
                return MIRExprResult::value(builder_->address_of(place, ptrt), ptrt);
            }
            return MIRExprResult::place(place, at);
        }
        case ValueKind::IndexOperator: {
            auto* io = value->as_index_op_unsafe();
            MIRExprResult base = lower_expr(io->parent_val, error);
            if (!base.ok()) return base;
            maybe_register_call_temp(io->parent_val, base);
            MIRExprResult idx = lower_expr(io->idx, error);
            if (!idx.ok()) return idx;
            // a reference used as an index is automatically dereferenced
            if (idx.type != MIR_INVALID_ID && idx.type < module_.types.size() &&
                module_.types.get(idx.type).kind == MIRTypeKind::Reference) {
                const TypeId it = module_.types.get(idx.type).element;
                const ValueId pv = (idx.kind == MIRExprKind::Place)
                                       ? builder_->load(static_cast<PlaceId>(idx.id), idx.type)
                                       : static_cast<ValueId>(idx.id);
                idx = MIRExprResult::value(builder_->load_indirect(pv, it), it);
            }
            emit_array_index_bounds_check(base, idx);
            if (base.kind == MIRExprKind::Place) {
                const TypeId bt = builder_->function().places[base.id].type;
                if (abi::is_struct_like(module_, bt)) {
                    // `x[i]` on a struct uses the `Index`/`IndexMut` method
                    BaseType* recv_t = io->parent_val ? io->parent_val->getType() : nullptr;
                    FunctionDeclaration* fd = resolve_method_fallback(recv_t, "index");
                    if (!fd) fd = resolve_method_fallback(recv_t, "index_mut");
                    if (!fd) {
                        error = "index operator overloading is not yet supported";
                        return MIRExprResult::error();
                    }
                    VariableIdentifier op_id(chem::string_view("index"), nullptr, ZERO_LOC);
                    op_id.linked = fd;
                    FunctionCall op_call(&op_id, fd->returnType, ZERO_LOC);
                    op_call.values.push_back(io->idx);
                    MIRExprResult r = lower_method_call(base, recv_t, &op_call, error);
                    if (!r.ok()) return r;
                    // the method returns `&Output`: dereference to the element
                    if (r.kind == MIRExprKind::Value || r.kind == MIRExprKind::Address) {
                        if (abi::is_aggregate(module_, type)) {
                            return MIRExprResult::address(static_cast<ValueId>(r.id), type);
                        }
                        return MIRExprResult::value(
                            builder_->load_indirect(static_cast<ValueId>(r.id), type), type);
                    }
                    return r;
                }
            }
            if (abi::is_aggregate(module_, type)) {
                // aggregate element: return its address (an lvalue), not a load
                const TypeId ptrt = types_.pointer_type(type, true);
                if (base.kind == MIRExprKind::Place) {
                    return MIRExprResult::address(
                        builder_->index_addr(static_cast<PlaceId>(base.id),
                                             static_cast<ValueId>(idx.id), type), type);
                }
                if (base.kind == MIRExprKind::Value || base.kind == MIRExprKind::Address) {
                    return MIRExprResult::address(
                        builder_->index_addr_ptr(static_cast<ValueId>(base.id),
                                                 static_cast<ValueId>(idx.id), type), type);
                }
                error = "index base is not a place or pointer value";
                return MIRExprResult::error();
            }
            if (base.kind == MIRExprKind::Place) {
                return MIRExprResult::value(
                    builder_->element_load(static_cast<PlaceId>(base.id),
                                           static_cast<ValueId>(idx.id), type), type);
            }
            if (base.kind == MIRExprKind::Value || base.kind == MIRExprKind::Address) {
                return MIRExprResult::value(
                    builder_->index_load(static_cast<ValueId>(base.id),
                                         static_cast<ValueId>(idx.id), type), type);
            }
            error = "index base is not a place or pointer value";
            return MIRExprResult::error();
        }
        case ValueKind::NullValue: {
            const ConstantId c = module_.constants.add_null(type);
            return MIRExprResult::value(builder_->const_null(type, c), type);
        }
        case ValueKind::DereferenceValue: {
            auto* d = value->as_dereference_value_unsafe();
            MIRExprResult in = lower_expr(d->getValue(), error);
            if (!in.ok()) return in;
            if (in.kind != MIRExprKind::Value && in.kind != MIRExprKind::Address) {
                error = "dereference operand is not a pointer value";
                return MIRExprResult::error();
            }
            if (abi::is_aggregate(module_, type)) {
                return MIRExprResult::address(static_cast<ValueId>(in.id), in.type);
            }
            return MIRExprResult::value(
                builder_->load_indirect(static_cast<ValueId>(in.id), type), type);
        }
        case ValueKind::IncDecValue: {
            auto* n = value->as_inc_dec_value_unsafe();
            return lower_incdec_value(n->getValue(), n->increment, n->post, error);
        }
        case ValueKind::PatternMatchExpr: {
            return lower_pattern_match_value(value->as_pattern_match_expr_unsafe(), error);
        }
        case ValueKind::ExtractionValue:
            return lower_extraction_value(static_cast<ExtractionValue*>(value), error);
        case ValueKind::NewTypedValue: {
            // `new T` -> `malloc(sizeof(T))`
            auto* nt = static_cast<NewTypedValue*>(value);
            BaseType* inner = const_cast<BaseType*>(nt->type.getType());
            const TypeId inner_t = types_.map(inner);
            const TypeId u64t = types_.u64_type();
            const TypeId ptr_t = types_.pointer_type(inner_t, true);
            const ValueId size_v = builder_->size_of(inner_t, u64t);
            return MIRExprResult::value(emit_malloc(size_v, u64t, ptr_t), ptr_t);
        }
        case ValueKind::NewValue: {
            // `new value` -> allocate, then store the value into the allocation
            auto* nv = static_cast<NewValue*>(value);
            BaseType* vt_ast = nv->value ? nv->value->getType() : nullptr;
            const TypeId vt = types_.map(vt_ast);
            const TypeId u64t = types_.u64_type();
            const TypeId ptr_t = types_.pointer_type(vt, true);
            const ValueId size_v = builder_->size_of(vt, u64t);
            const ValueId mem = emit_malloc(size_v, u64t, ptr_t);
            MIRExprResult r = lower_expr(nv->value, error);
            if (!r.ok()) return r;
            if (r.kind == MIRExprKind::Place) {
                const TypeId rt = builder_->function().places[r.id].type;
                builder_->store_indirect(mem, builder_->load(static_cast<PlaceId>(r.id), rt));
            } else if (r.kind == MIRExprKind::Value || r.kind == MIRExprKind::Address) {
                builder_->store_indirect(mem, static_cast<ValueId>(r.id));
            } else {
                error = "new value does not produce a value";
                return MIRExprResult::error();
            }
            return MIRExprResult::value(mem, ptr_t);
        }
        case ValueKind::PlacementNewValue: {
            // `new (ptr) value` -> store the value through `ptr`
            auto* pn = static_cast<PlacementNewValue*>(value);
            MIRExprResult pp = lower_expr(pn->pointer, error);
            if (!pp.ok()) return pp;
            ValueId mem = (pp.kind == MIRExprKind::Place)
                              ? builder_->load(static_cast<PlaceId>(pp.id), pp.type)
                              : static_cast<ValueId>(pp.id);
            MIRExprResult r = lower_expr(pn->value, error);
            if (!r.ok()) return r;
            if (r.kind == MIRExprKind::Place) {
                const TypeId rt = builder_->function().places[r.id].type;
                builder_->store_indirect(mem, builder_->load(static_cast<PlaceId>(r.id), rt));
            } else if (r.kind == MIRExprKind::Value || r.kind == MIRExprKind::Address) {
                builder_->store_indirect(mem, static_cast<ValueId>(r.id));
            } else {
                error = "placement new value does not produce a value";
                return MIRExprResult::error();
            }
            return MIRExprResult::value(mem, pp.type);
        }
        case ValueKind::BlockValue: {
            // a `{ ... }` statement-expression: its trailing value (or the
            // symres-calculated value) is the result
            auto* bv = static_cast<BlockValue*>(value);
            Scope* scope = &bv->scope;
            Value* calculated = bv->getCalculatedValue();
            const TypeId vt = type;
            const PlaceId dest = builder_->alloca(vt, MIRStorageClass::Temporary);
            if (calculated) {
                const size_t mark = destructibles_.size();
                for (ASTNode* n : scope->nodes) {
                    if (builder_->current_block_terminated()) break;
                    if (!lower_stmt(n, error)) return MIRExprResult::error();
                }
                MIRExprResult r = lower_expr(calculated, error);
                if (!r.ok()) return r;
                if (r.kind == MIRExprKind::Place) {
                    builder_->store(dest, builder_->load(static_cast<PlaceId>(r.id), vt));
                } else if (r.kind == MIRExprKind::Address) {
                    builder_->store(dest,
                                    builder_->load_indirect(static_cast<ValueId>(r.id), vt));
                } else if (r.kind == MIRExprKind::Value) {
                    builder_->store(dest, static_cast<ValueId>(r.id));
                } else {
                    error = "block value does not produce a value";
                    return MIRExprResult::error();
                }
                if (!builder_->current_block_terminated()) {
                    emit_drops_from(mark);
                }
                destructibles_.resize(mark);
            } else if (!lower_scope_value(*scope, dest, vt, error)) {
                return MIRExprResult::error();
            }
            if (abi::is_aggregate(module_, vt)) {
                return MIRExprResult::place(dest, vt);
            }
            return MIRExprResult::value(builder_->load(dest, vt), vt);
        }
        case ValueKind::InValue: {
            // `x in a, b, c` -> a chain of equality tests (or `!in` -> not-equal)
            auto* in = static_cast<InValue*>(value);
            const TypeId bool_t = types_.bool_type();
            MIRExprResult v = lower_expr(in->value, error);
            if (!v.ok()) return v;
            const PlaceId res = builder_->alloca(bool_t, MIRStorageClass::Temporary);
            builder_->store(
                res, builder_->const_bool(
                         bool_t, module_.constants.add_bool(bool_t, in->is_negating)));
            const BlockId end_b = builder_->create_block();
            const ConstantId eqc = module_.constants.add_int(
                MIR_INVALID_ID, static_cast<uint64_t>(MIRBinaryOp::Eq));
            for (Value* cv : in->values) {
                MIRExprResult c = lower_expr(cv, error);
                if (!c.ok()) return c;
                const ValueId cond = builder_->compare(v.id, c.id, eqc, bool_t);
                const BlockId hit_b = builder_->create_block();
                const BlockId next_b = builder_->create_block();
                builder_->cond_br(cond, hit_b, next_b);
                builder_->set_block(hit_b);
                builder_->store(res,
                                builder_->const_bool(
                                    bool_t, module_.constants.add_bool(bool_t, !in->is_negating)));
                builder_->br(end_b);
                builder_->set_block(next_b);
            }
            builder_->br(end_b);
            builder_->set_block(end_b);
            return MIRExprResult::value(builder_->load(res, bool_t), bool_t);
        }
        case ValueKind::IsValue: {
            auto* isv = value->as_is_value_unsafe();
            ASTNode* ln = isv->type ? isv->type->get_direct_linked_node() : nullptr;
            if (ln && ln->kind() == ASTNodeKind::VariantMember) {
                auto* vm = ln->as_variant_member_unsafe();
                VariantDefinition* vd = vm->parent();
                MIRExprResult v = lower_expr(isv->value, error);
                if (!v.ok()) return v;
                const std::string tag_name("__chx__vt_621827");
                const ConstantId tag_fc = module_.constants.add_string(
                    MIR_INVALID_ID, tag_name.data(), static_cast<uint32_t>(tag_name.size()));
                const TypeId it = types_.u32_type();
                ValueId tag = MIR_NULL;
                if (v.kind == MIRExprKind::Place) {
                    tag = builder_->field_load(static_cast<PlaceId>(v.id), tag_fc, it);
                } else if (v.kind == MIRExprKind::Address || v.kind == MIRExprKind::Value) {
                    tag = builder_->field_load_ptr(static_cast<ValueId>(v.id), tag_fc, it);
                } else {
                    error = "is-check operand is not a variant lvalue";
                    return MIRExprResult::error();
                }
                const long idx = vd ? vd->direct_child_index(vm->name) : 0;
                const ValueId c = builder_->const_int(
                    it, module_.constants.add_int(
                            it, static_cast<uint64_t>(idx < 0 ? 0 : idx)));
                const TypeId bool_t = types_.bool_type();
                const ConstantId pred = module_.constants.add_int(
                    MIR_INVALID_ID,
                    static_cast<uint64_t>(isv->is_negating ? MIRBinaryOp::Ne : MIRBinaryOp::Eq));
                return MIRExprResult::value(builder_->compare(tag, c, pred, bool_t), bool_t);
            }
            if (std::optional<bool> comp = isv->get_comp_time_result(); comp.has_value()) {
                const TypeId bool_t = types_.bool_type();
                return MIRExprResult::value(
                    builder_->const_bool(
                        bool_t, module_.constants.add_bool(bool_t, comp.value())),
                    bool_t);
            }
            error = "unsupported is-expression (non-variant)";
            return MIRExprResult::error();
        }
        case ValueKind::IfValue: {
            return lower_if_value(value->as_if_value_unsafe(), error);
        }
        case ValueKind::SwitchValue: {
            return lower_switch_value(value->as_switch_value_unsafe(), error);
        }
        case ValueKind::LoopValue: {
            return lower_loop_value(value->as_loop_value_unsafe(), error);
        }
        case ValueKind::SizeOfValue: {
            auto* s = value->as_sizeof_value_unsafe();
            BaseType* ft = s->for_type.getType()
                               ? resolve_sizeof_operand(const_cast<BaseType*>(s->for_type.getType()))
                               : nullptr;
            const TypeId tid = ft ? types_.map(ft) : types_.opaque_type();
            return MIRExprResult::value(builder_->size_of(tid, type), type);
        }
        case ValueKind::AlignOfValue: {
            auto* a = static_cast<AlignOfValue*>(value);
            BaseType* ft = a->for_type.getType()
                               ? resolve_sizeof_operand(const_cast<BaseType*>(a->for_type.getType()))
                               : nullptr;
            const TypeId tid = ft ? types_.map(ft) : types_.opaque_type();
            return MIRExprResult::value(builder_->align_of(tid, type), type);
        }
        case ValueKind::OffsetOfValue: {
            auto* o = value->as_offset_of_value_unsafe();
            BaseType* ft = o->for_type.getType()
                               ? resolve_sizeof_operand(const_cast<BaseType*>(o->for_type.getType()))
                               : nullptr;
            const TypeId tid = ft ? types_.map(ft) : types_.opaque_type();
            const std::string mn(o->member_name.data(), o->member_name.size());
            const ConstantId fc =
                module_.constants.add_string(MIR_INVALID_ID, mn.data(),
                                            static_cast<uint32_t>(mn.size()));
            return MIRExprResult::value(builder_->offset_of(tid, fc, type), type);
        }
        case ValueKind::UnsafeValue: {
            // `unsafe(expr)` is a compile-time safety marker only
            auto* u = static_cast<UnsafeValue*>(value);
            if (u->getValue()) return lower_expr(u->getValue(), error);
            error = "unsafe value has no inner expression";
            return MIRExprResult::error();
        }
        case ValueKind::ZeroedValue: {
            const TypeId zt = type;
            const PlaceId tmp = builder_->alloca(zt, MIRStorageClass::Temporary);
            builder_->zero_init(tmp);
            if (abi::is_aggregate(module_, zt)) return MIRExprResult::place(tmp, zt);
            return MIRExprResult::value(builder_->load(tmp, zt), zt);
        }
        case ValueKind::DynamicValue: {
            auto* dv = static_cast<DynamicValue*>(value);
            BaseType* iface_t = dv->getInterfaceType();
            ASTNode* iface_node =
                iface_t ? iface_t->get_direct_linked_canonical_node() : nullptr;
            if (!iface_node || (!vtable_mangler_ && !vtable_mangler_into_)) {
                error = "dynamic value interface could not be resolved";
                return MIRExprResult::error();
            }
            // copied out of the reused buffer: the object's address is lowered
            // below (which may mangle another vtable and reuse the buffer), and
            // the symbol is only interned after that
            const std::string vt =
                vtable_mangle(iface_node, dv->value ? dv->value->getType() : nullptr);
            if (vt.empty()) {
                error = "dynamic value vtable name is empty";
                return MIRExprResult::error();
            }
            const TypeId ft = types_.fat_pointer_type();
            const TypeId voidp = types_.pointer_type(types_.opaque_type(), true);
            const PlaceId temp = builder_->alloca(ft, MIRStorageClass::Temporary);
            // A fat pointer is `{ void* first; void* second; }` with no padding,
            // and both fields are written just below -- the whole-temp zero was
            // dead bytes (evaluation §1.2, action C1).
            ValueId obj_ptr = MIR_NULL;
            // A fat pointer is an *escaping* address: it routinely outlives the
            // call that produced it (that is the whole point of `ret : dyn I`).
            // For a by-value aggregate parameter the parameter place is a copy
            // living in this frame, so pointing at it would hand the caller (or
            // anyone else) a dangling pointer once we return. Use the argument
            // pointer instead — the caller's object — exactly like the
            // interpreter (which shares the value) and the pre-copy MIR model
            // did (`&param` of an aggregate param is documented as the pointer).
            FunctionParam* agg_param = aggregate_param_operand(dv->value);
            if (agg_param) {
                obj_ptr = aggregate_param_ptrs_[agg_param];
            } else {
                MIRExprResult obj = lower_address_of(dv->value, error);
                if (!obj.ok()) return obj;
                if (obj.kind == MIRExprKind::Place) {
                    const TypeId pt = builder_->function().places[obj.id].type;
                    obj_ptr = builder_->address_of(obj.id, types_.pointer_type(pt, true));
                } else if (obj.kind == MIRExprKind::Value || obj.kind == MIRExprKind::Address) {
                    obj_ptr = static_cast<ValueId>(obj.id);
                } else {
                    error = "dynamic value operand is not addressable";
                    return MIRExprResult::error();
                }
            }
            const ConstantId first_c =
                module_.constants.add_string(MIR_INVALID_ID, "first", 5);
            builder_->field_store(temp, first_c, obj_ptr);
            const SymbolId gs = intern_named_global(vt);
            const ValueId vt_addr = builder_->global_addr(gs, voidp);
            const ConstantId second_c =
                module_.constants.add_string(MIR_INVALID_ID, "second", 6);
            builder_->field_store(temp, second_c, vt_addr);
            return MIRExprResult::place(temp, ft);
        }
        case ValueKind::ComptimeValue:
            return lower_expr(static_cast<ComptimeValue*>(value)->getValue(), error);
        case ValueKind::LambdaFunc: {
            return lower_lambda(value->as_lambda_func_unsafe(), error);
        }
        case ValueKind::AwaitExpr: {
            // a non-suspending (transparent) await: use the operand's value.
            // Suspending awaits are hoisted into VarInitStatements by the async
            // normalization pass and handled by `async_lower_await_var_init`.
            auto* aw = value->as_await_expression_unsafe();
            if (!aw || !aw->getInner()) {
                error = "await expression has no operand";
                return MIRExprResult::error();
            }
            return lower_expr(aw->getInner(), error);
        }
        case ValueKind::EmbeddedValue: {
            auto* ev = value->as_embedded_value_unsafe();
            Value* repl = nullptr;
            auto it = embedded_value_cache_.find(ev);
            if (it != embedded_value_cache_.end()) {
                repl = it->second;
            } else if (embedded_value_repl_) {
                repl = embedded_value_repl_(ev);
                if (repl) embedded_value_cache_[ev] = repl;
            }
            if (!repl) {
                error = "embedded value has no replacement";
                return MIRExprResult::error();
            }
            return lower_expr(repl, error);
        }
        default:
            error = "unsupported value kind during MIR lowering (kind " +
                    std::to_string(static_cast<int>(value->val_kind())) + ")";
            return MIRExprResult::error();
    }
}



void MIRLowerer::emit_array_index_bounds_check(const MIRExprResult& base,
                                               const MIRExprResult& index) {
    TypeId array_type = MIR_INVALID_ID;
    switch (base.kind) {
        case MIRExprKind::Place:
            if (base.id < builder_->function().places.size()) {
                array_type = builder_->function().places[base.id].type;
            }
            break;
        case MIRExprKind::Address:
        case MIRExprKind::Value:
            array_type = base.type;
            break;
        default:
            return;
    }
    if (array_type == MIR_INVALID_ID || array_type >= module_.types.size()) return;
    const MIRTypeRecord& rec = module_.types.get(array_type);
    if (rec.kind != MIRTypeKind::Array) return;
    if (index.id == MIR_NULL || index.id >= builder_->function().values.size()) return;

    const TypeId u64t = types_.u64_type();
    const TypeId bool_t = types_.bool_type();
    const TypeId void_t = types_.void_type();
    const ValueId idx_u = builder_->cast(static_cast<ValueId>(index.id), u64t);
    const ValueId len = builder_->const_int(u64t, module_.constants.add_int(u64t, rec.data_count));
    const ConstantId lt = module_.constants.add_int(
        MIR_INVALID_ID, static_cast<uint64_t>(MIRBinaryOp::Lt));
    const ValueId in_bounds = builder_->compare(idx_u, len, lt, bool_t);
    const BlockId ok_b = builder_->create_block();
    const BlockId abort_b = builder_->create_block();
    builder_->cond_br(in_bounds, ok_b, abort_b);
    builder_->set_block(abort_b);
    const TypeId abort_ft = types_.function_signature(void_t, {});
    const SymbolId abort_sym = extern_symbol("abort", abort_ft);
    builder_->call_scalar(abort_sym, void_t, nullptr, 0);
    builder_->unreachable();
    builder_->set_block(ok_b);
}


void MIRLowerer::emit_abort_unless(ValueId condition) {
    const BlockId ok_b = builder_->create_block();
    const BlockId abort_b = builder_->create_block();
    builder_->cond_br(condition, ok_b, abort_b);
    builder_->set_block(abort_b);
    const TypeId void_t = types_.void_type();
    const TypeId abort_ft = types_.function_signature(void_t, {});
    const SymbolId abort_sym = extern_symbol("abort", abort_ft);
    builder_->call_scalar(abort_sym, void_t, nullptr, 0);
    builder_->unreachable();
    builder_->set_block(ok_b);
}

void MIRLowerer::emit_division_check(ValueId lhs, ValueId rhs, TypeId type,
                                     MIRBinaryOp op) {
    if (type == MIR_INVALID_ID || type >= module_.types.size()) return;
    const MIRTypeRecord& rec = module_.types.get(type);
    if (rec.kind != MIRTypeKind::Int) return; // floats have defined Inf/NaN results

    const TypeId bool_t = types_.bool_type();
    const ConstantId ne = module_.constants.add_int(
        MIR_INVALID_ID, static_cast<uint64_t>(MIRBinaryOp::Ne));
    const ConstantId eq = module_.constants.add_int(
        MIR_INVALID_ID, static_cast<uint64_t>(MIRBinaryOp::Eq));
    const ConstantId band = module_.constants.add_int(
        MIR_INVALID_ID, static_cast<uint64_t>(MIRBinaryOp::BitAnd));
    const ValueId zero = builder_->const_int(type, module_.constants.add_int(type, 0));
    const ValueId nonzero = builder_->compare(rhs, zero, ne, bool_t);
    emit_abort_unless(nonzero);

    if ((rec.flags & TF_SIGNED) == 0) return;
    // Signed MIN / -1 traps (the only signed division overflow).
    if (op != MIRBinaryOp::Div && op != MIRBinaryOp::Rem) return;
    const ValueId minus_one = builder_->const_int(
        type, module_.constants.add_int(type, ~static_cast<uint64_t>(0)));
    const uint32_t bits = rec.size * 8u;
    const uint64_t min_bits = bits >= 64u ? (1ull << 63u) : (1ull << (bits - 1u));
    const ValueId min_value = builder_->const_int(
        type, module_.constants.add_int(type, min_bits));
    const ValueId rhs_is_minus_one = builder_->compare(rhs, minus_one, eq, bool_t);
    const ValueId lhs_is_min = builder_->compare(lhs, min_value, eq, bool_t);
    const ValueId overflow = builder_->binary(rhs_is_minus_one, lhs_is_min, band, bool_t);
    const ValueId safe = builder_->compare(overflow, builder_->const_bool(
        bool_t, module_.constants.add_bool(bool_t, false)), eq, bool_t);
    emit_abort_unless(safe);
}

void MIRLowerer::emit_shift_check(ValueId amount, TypeId type) {
    if (type == MIR_INVALID_ID || type >= module_.types.size()) return;
    const MIRTypeRecord& rec = module_.types.get(type);
    if (rec.kind != MIRTypeKind::Int) return;
    TypeId amount_type = type;
    if (amount < builder_->function().values.size()) {
        const TypeId at = builder_->function().values[amount].type;
        if (at != MIR_INVALID_ID && at < module_.types.size()) amount_type = at;
    }
    const MIRTypeRecord& amount_rec = module_.types.get(amount_type);
    if (amount_rec.kind != MIRTypeKind::Int) return;
    const TypeId bool_t = types_.bool_type();
    const ConstantId lt = module_.constants.add_int(
        MIR_INVALID_ID, static_cast<uint64_t>(MIRBinaryOp::Lt));
    const ConstantId ge = module_.constants.add_int(
        MIR_INVALID_ID, static_cast<uint64_t>(MIRBinaryOp::Ge));
    if ((amount_rec.flags & TF_SIGNED) != 0) {
        const ValueId zero = builder_->const_int(
            amount_type, module_.constants.add_int(amount_type, 0));
        const ValueId non_negative = builder_->compare(amount, zero, ge, bool_t);
        emit_abort_unless(non_negative);
    }
    const ValueId width = builder_->const_int(
        amount_type, module_.constants.add_int(
            amount_type, static_cast<uint64_t>(rec.size) * 8u));
    const ValueId in_range = builder_->compare(amount, width, lt, bool_t);
    emit_abort_unless(in_range);
}


void MIRLowerer::emit_compound_assignment_checks(MIRBinaryOp op, ValueId lhs,
                                                 ValueId rhs, TypeId type) {
    if (op == MIRBinaryOp::Div || op == MIRBinaryOp::Rem) {
        emit_division_check(lhs, rhs, type, op);
    } else if (op == MIRBinaryOp::Shl || op == MIRBinaryOp::Shr) {
        emit_shift_check(rhs, type);
    }
}

} // namespace mir
