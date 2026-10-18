// Copyright (c) Chemical Language Foundation 2025.

#include "MIRLowerer.h"
#include "MIRLowererInternal.h"
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
#include "ast/values/DynamicValue.h"
#include "ast/values/ComptimeValue.h"
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
ConstExprId MIRLowerer::build_global_initializer(VarInitStatement* vi) {
    const SymbolId sym = intern_global(vi);
    ConstExprId init = MIR_INVALID_ID;
    if (vi->value) init = build_const_expr(vi->value);
    MIRGlobal g;
    g.symbol = sym;
    g.init = init;
    if (vi->is_thread_local()) g.flags |= MGD_THREAD_LOCAL;
    if (vi->is_volatile()) g.flags |= MGD_VOLATILE;
    module_.globals.push_back(g);
    return init;
}

ConstExprId MIRLowerer::build_const_expr(Value* value) {
    auto type_of = [&](Value* v) -> TypeId {
        return (v && v->getType()) ? types_.map(v->getType()) : MIR_INVALID_ID;
    };
    if (!value) return module_.const_exprs.add_zero(MIR_INVALID_ID);
    switch (value->val_kind()) {
        case ValueKind::IntN: {
            auto* v = value->as_int_num_value_unsafe();
            const ConstExprId id = module_.const_exprs.add_int(type_of(value), v->get_num_value());
            if (v->is_unsigned()) module_.const_exprs.exprs[id].flags |= MCF_UNSIGNED;
            return id;
        }
        case ValueKind::Float:
            return module_.const_exprs.add_float(type_of(value), value->as_float_unsafe()->value);
        case ValueKind::Double:
            return module_.const_exprs.add_double(type_of(value),
                                                  value->as_double_unsafe()->value);
        case ValueKind::Bool:
            return module_.const_exprs.add_bool(type_of(value), value->as_bool_unsafe()->value);
        case ValueKind::String: {
            auto* sv = value->as_string_unsafe();
            return module_.const_exprs.add_string(type_of(value), sv->value.data(),
                                                  static_cast<uint32_t>(sv->value.size()));
        }
        case ValueKind::NullValue:
            return module_.const_exprs.add_null(type_of(value));
        case ValueKind::NegativeValue: {
            Value* inner = value->as_negative_value_unsafe()->getValue();
            return module_.const_exprs.add_negate(type_of(value), build_const_expr(inner));
        }
        case ValueKind::ComptimeValue: {
            Value* inner = static_cast<ComptimeValue*>(value)->getValue();
            return inner ? build_const_expr(inner)
                         : module_.const_exprs.add_zero(type_of(value));
        }
        case ValueKind::CastedValue:
            return build_const_expr(value->as_casted_value_unsafe()->value);
        case ValueKind::ArrayValue:
            return build_const_array(value->as_array_value_unsafe());
        case ValueKind::StructValue:
            return build_const_struct_value(value->as_struct_value_unsafe());
        case ValueKind::FunctionCall:
        case ValueKind::AccessChain:
            return build_const_call(value);
        default:
            return module_.const_exprs.add_zero(type_of(value));
    }
}

ConstExprId MIRLowerer::build_const_array(ArrayValue* av) {
    const TypeId t = (av && av->getType()) ? types_.map(av->getType()) : MIR_INVALID_ID;
    MIRScratch<MIRConstChild> _kids(*this);
    std::vector<MIRConstChild>& kids = _kids.v;
    if (av) {
        kids.reserve(av->values.size());
        for (Value* v : av->values) {
            MIRConstChild c;
            c.expr = build_const_expr(v);
            kids.push_back(c);
        }
    }
    return module_.const_exprs.add_with_children(MIRConstKind::Aggregate, t, kids);
}

ConstExprId MIRLowerer::build_const_struct_default(StructDefinition* sd) {
    if (!sd) return module_.const_exprs.add_zero(MIR_INVALID_ID);
    const TypeId t = sd->known_type() ? types_.map(sd->known_type()) : MIR_INVALID_ID;
    MIRScratch<MIRConstChild> _kids(*this);
    std::vector<MIRConstChild>& kids = _kids.v;
    auto add = [&](const chem::string_view& name, ConstExprId id) {
        MIRConstChild c;
        c.expr = id;
        c.name_offset = module_.const_exprs.intern_name(name.data(),
                                                       static_cast<uint32_t>(name.size()));
        c.name_length = static_cast<uint32_t>(name.size());
        kids.push_back(c);
    };
    for (auto& inh : sd->inherited) {
        StructDefinition* base = inh.type ? inh.type->get_direct_linked_struct() : nullptr;
        if (!base) continue;
        add(base->name_view(), build_const_struct_default(base));
    }
    for (BaseDefMember* var : sd->variables()) {
        if (!var) continue;
        Value* def = var->default_value();
        if (!def) continue;
        add(var->name, build_const_expr(def));
    }
    return module_.const_exprs.add_with_children(MIRConstKind::Aggregate, t, kids, 0,
                                                 MCF_CAST_PREFIX);
}

ConstExprId MIRLowerer::build_const_struct_value(StructValue* sv) {
    const TypeId t = (sv && sv->getType()) ? types_.map(sv->getType()) : MIR_INVALID_ID;
    MIRScratch<MIRConstChild> _kids(*this);
    std::vector<MIRConstChild>& kids = _kids.v;
    auto add = [&](const chem::string_view& name, ConstExprId id) {
        MIRConstChild c;
        c.expr = id;
        c.name_offset = module_.const_exprs.intern_name(name.data(),
                                                       static_cast<uint32_t>(name.size()));
        c.name_length = static_cast<uint32_t>(name.size());
        kids.push_back(c);
    };
    if (sv) {
        // explicit values, in source order (an `ordered_map`) — matching
        // `const_value_expr`
        for (auto& entry : sv->values) {
            add(entry.second.name, build_const_expr(entry.second.value));
        }
        StructDefinition* sd =
            sv->getType() ? sv->getType()->get_direct_linked_struct() : nullptr;
        if (sd) {
            for (auto& inh : sd->inherited) {
                StructDefinition* base =
                    inh.type ? inh.type->get_direct_linked_struct() : nullptr;
                if (!base) continue;
                if (sv->values.find(base->name_view()) != sv->values.end()) continue;
                add(base->name_view(), build_const_struct_default(base));
            }
            for (BaseDefMember* var : sd->variables()) {
                if (!var) continue;
                if (!sd->isDirectChild(var)) continue;
                if (sv->values.find(var->name) != sv->values.end()) continue;
                Value* def = var->default_value();
                if (!def) continue;
                add(var->name, build_const_expr(def));
            }
        }
    }
    return module_.const_exprs.add_with_children(MIRConstKind::Aggregate, t, kids);
}

ConstExprId MIRLowerer::build_const_variant(VariantMember* vm, FunctionCall* call) {
    if (!vm) return module_.const_exprs.add_zero(MIR_INVALID_ID);
    VariantDefinition* vd = vm->parent();
    const long idx = vd ? vd->direct_child_index(vm->name) : 0;
    const TypeId t = (call && call->getType()) ? types_.map(call->getType()) : MIR_INVALID_ID;
    const std::string case_name(vm->name.data(), vm->name.size());
    MIRScratch<MIRConstChild> _kids(*this);
    std::vector<MIRConstChild>& kids = _kids.v;
    auto add = [&](const std::string& name, ConstExprId id) {
        MIRConstChild c;
        c.expr = id;
        c.name_offset = module_.const_exprs.intern_name(name.data(),
                                                       static_cast<uint32_t>(name.size()));
        c.name_length = static_cast<uint32_t>(name.size());
        kids.push_back(c);
    };
    size_t ai = 0;
    for (auto& entry : vm->values) {
        VariantMemberParam* param = entry.second;
        Value* arg = nullptr;
        if (call && ai < call->values.size()) {
            arg = call->values[ai];
            ai++;
        } else if (param) {
            arg = param->def_value;
        }
        if (!arg) continue;
        const std::string pname(entry.first.data(), entry.first.size());
        add(case_name + "." + pname, build_const_expr(arg));
    }
    if (vd) {
        for (auto& inh : vd->inherited) {
            StructDefinition* base =
                inh.type ? inh.type->get_direct_linked_struct() : nullptr;
            if (!base) continue;
            add(std::string(base->name_view().data(), base->name_view().size()),
                build_const_struct_default(base));
        }
    }
    return module_.const_exprs.add_with_children(MIRConstKind::Variant, t, kids,
                                                 idx < 0 ? 0u : static_cast<uint32_t>(idx));
}

ConstExprId MIRLowerer::build_const_variant_bare(VariantMember* vm, BaseType* type) {
    if (!vm) return module_.const_exprs.add_zero(MIR_INVALID_ID);
    VariantDefinition* vd = vm->parent();
    const long idx = vd ? vd->direct_child_index(vm->name) : 0;
    const TypeId t = type ? types_.map(type) : MIR_INVALID_ID;
    MIRScratch<MIRConstChild> _kids(*this);
    std::vector<MIRConstChild>& kids = _kids.v;
    auto add = [&](const char* name, uint32_t len, ConstExprId id) {
        MIRConstChild c;
        c.expr = id;
        c.name_offset = module_.const_exprs.intern_name(name, len);
        c.name_length = len;
        kids.push_back(c);
    };
    if (vd) {
        for (auto& inh : vd->inherited) {
            StructDefinition* base =
                inh.type ? inh.type->get_direct_linked_struct() : nullptr;
            if (!base) continue;
            const chem::string_view nv = base->name_view();
            add(nv.data(), static_cast<uint32_t>(nv.size()), build_const_struct_default(base));
        }
    }
    return module_.const_exprs.add_with_children(MIRConstKind::Variant, t, kids,
                                                 idx < 0 ? 0u : static_cast<uint32_t>(idx));
}

ConstExprId MIRLowerer::build_const_call(Value* value) {
    auto type_of = [&](Value* v) -> TypeId {
        return (v && v->getType()) ? types_.map(v->getType()) : MIR_INVALID_ID;
    };
    if (!value) return module_.const_exprs.add_zero(MIR_INVALID_ID);
    if (value->val_kind() == ValueKind::AccessChain) {
        auto* ch = value->as_access_chain_unsafe();
        if (ch->values.empty()) return module_.const_exprs.add_zero(type_of(value));
        Value* last = ch->values.back();
        if (last->val_kind() == ValueKind::FunctionCall) return build_const_call(last);
        ASTNode* linked = last->linked_node();
        if (linked) {
            if (linked->kind() == ASTNodeKind::VariantMember) {
                return build_const_variant_bare(linked->as_variant_member_unsafe(),
                                                value->getType());
            }
            if (linked->kind() == ASTNodeKind::FunctionDecl) {
                return module_.const_exprs.add_symbol(
                    type_of(value), intern_function(linked->as_function_unsafe()));
            }
            if (linked->kind() == ASTNodeKind::VarInitStmt) {
                auto* vi = linked->as_var_init_unsafe();
                if (vi->value) return build_const_expr(vi->value);
            }
        }
        return module_.const_exprs.add_zero(type_of(value));
    }
    auto* fc = value->as_func_call_unsafe();
    ASTNode* vleaf = nullptr;
    if (fc->parent_val) {
        if (fc->parent_val->val_kind() == ValueKind::AccessChain) {
            auto* ch = fc->parent_val->as_access_chain_unsafe();
            if (!ch->values.empty()) vleaf = ch->values.back()->linked_node();
        } else {
            vleaf = fc->parent_val->linked_node();
        }
    }
    if (vleaf && vleaf->kind() == ASTNodeKind::VariantMember) {
        return build_const_variant(vleaf->as_variant_member_unsafe(), fc);
    }
    // a comptime function call folds to its constant result. A `comptime func`
    // with a body is lowered and run by the MIR comptime driver; only values that
    // fold without a body (intrinsics, single-expression functions) still go
    // through the expression layer.
    if (FunctionDeclaration* fd = fc->safe_linked_func()) {
        if (fd->is_comptime() && comptime_eval_ && !is_comptime_template_fn(fd)) {
            Value* ev = comptime_eval_(fc, fd);
            if (ev && ev != fc) return build_const_expr(ev);
        }
    }
    if (comptime_value_eval_) {
        Value* ev = comptime_value_eval_(fc);
        if (ev && ev != fc) return build_const_expr(ev);
    }
    return module_.const_exprs.add_zero(type_of(value));
}


} // namespace mir
