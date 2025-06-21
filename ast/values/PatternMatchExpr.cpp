// Copyright (c) Chemical Language Foundation 2025.

#include "PatternMatchExpr.h"
#include "compiler/symres/SymbolResolver.h"
#include "ast/structures/VariantDefinition.h"
#include "ast/types/VoidType.h"
#include "ast/structures/VariantMember.h"

#ifdef COMPILER_BUILD

#include "compiler/Codegen.h"
#include "compiler/llvmimpl.h"

llvm::Type* PatternMatchIdentifier::llvm_type(Codegen &gen) {
    return member_param->type->llvm_type(gen);
}

llvm::Value* PatternMatchIdentifier::llvm_load(Codegen &gen, SourceLocation location) {
    if(matchExpr->elseExpression.kind == PatternElseExprKind::DefValue) {
        return matchExpr->llvm_expr;
    }
    return Value::load_value(gen, member_param->type, member_param->type->llvm_type(gen), llvm_pointer(gen), encoded_location());
}

llvm::Value* PatternMatchIdentifier::llvm_pointer(Codegen &gen) {
    const auto elseKind = matchExpr->elseExpression.kind;
    const auto holder_pointer = matchExpr->llvm_expr;
    if(elseKind == PatternElseExprKind::DefValue) {
        return holder_pointer;
    }
    const auto linked_member = member_param->parent();
    const auto linked_def = linked_member->parent();
    return linked_def->get_param_pointer(gen, holder_pointer, member_param);
}

llvm::Value* PatternMatchExpr::llvm_value(Codegen &gen, BaseType *type) {
    const auto pointer = expression->llvm_pointer(gen);
    const auto elseKind = elseExpression.kind;
    if(elseKind == PatternElseExprKind::DefValue) {

        const auto param = param_names[0];
        const auto mem_param = param->member_param;
        const auto mem = mem_param->parent();
        const auto variant_def = mem->parent();

        const auto paramType = mem_param->type->llvm_type(gen);

        // check if variant type is the one we need
        const auto loadInst = variant_def->load_type_int(gen, pointer, expression->encoded_location());

        // compare the type integer (if its the member that we expected)
        const auto index = variant_def->direct_child_index(mem->name);
        const auto compareResult = gen.builder->CreateICmpEQ(loadInst, gen.builder->getInt32(index));

        const auto loadBlock = llvm::BasicBlock::Create(*gen.ctx, "", gen.current_function);
        const auto avoidBlock = llvm::BasicBlock::Create(*gen.ctx, "", gen.current_function);
        const auto endBlock = llvm::BasicBlock::Create(*gen.ctx, "", gen.current_function);

        // if it is go into a block that loads the value we need
        // if it is not, go into a block that loads the else value
        gen.CreateCondBr(compareResult, loadBlock, avoidBlock, encoded_location());

        // the block that loads the value
        gen.SetInsertPoint(loadBlock);
        const auto expectedPtr = variant_def->get_param_pointer(gen, pointer, mem_param);
        const auto loaded = Value::load_value(gen, mem_param->type, paramType, expectedPtr, encoded_location());
        gen.CreateBr(endBlock, encoded_location());

        // the block that loads the else value
        gen.SetInsertPoint(avoidBlock);
        const auto loadedElse = elseExpression.value->llvm_value(gen, nullptr);
        gen.CreateBr(endBlock, elseExpression.value->encoded_location());

        // the block that decides the final value
        gen.SetInsertPoint(endBlock);
        const auto phi = gen.builder->CreatePHI(paramType, 2);
        phi->addIncoming(loaded, loadBlock);
        phi->addIncoming(loadedElse, avoidBlock);

        // set the value to llvm expression
        llvm_expr = phi;

    } else {

        if(elseKind == PatternElseExprKind::Return) {

            const auto mem = member;
            const auto variant_def = mem->parent();

            // check the variant type
            const auto loadInst = variant_def->load_type_int(gen, pointer, expression->encoded_location());

            // compare the type integer (if its the member that we did NOT expect (then return))
            const auto index = variant_def->direct_child_index(mem->name);
            const auto compareResult = gen.builder->CreateICmpNE(loadInst, gen.builder->getInt32(index));

            const auto returnBlock = llvm::BasicBlock::Create(*gen.ctx, "", gen.current_function);
            const auto endBlock = llvm::BasicBlock::Create(*gen.ctx, "", gen.current_function);

            // if not equal, return otherwise continue to end block
            gen.CreateCondBr(compareResult, returnBlock, endBlock, encoded_location());

            gen.SetInsertPoint(returnBlock);
            gen.writeReturnStmtFor(elseExpression.value, encoded_location());

            gen.SetInsertPoint(endBlock);

        }

        llvm_expr = pointer;

    }
    return nullptr;
}

void PatternMatchExpr::llvm_conditional_branch(Codegen &gen, llvm::BasicBlock *then_block, llvm::BasicBlock *otherwise_block) {

    const auto pointer = expression->llvm_pointer(gen);

    // must be set
    llvm_expr = pointer;

    const auto mem = member;
    const auto variant_def = mem->parent();

    // check the variant type
    const auto loadInst = variant_def->load_type_int(gen, pointer, expression->encoded_location());

    // compare the type integer (if its the member that we did NOT expect (then return))
    const auto index = variant_def->direct_child_index(mem->name);
    const auto compareResult = gen.builder->CreateICmpEQ(loadInst, gen.builder->getInt32(index));

    // conditional branch to blocks
    gen.CreateCondBr(compareResult, then_block, otherwise_block, encoded_location());

}

llvm::Type* PatternMatchExpr::llvm_type(Codegen &gen) {
    return nullptr;
}

#endif

BaseType* PatternMatchIdentifier::known_type() {
    return member_param->type;
}

ASTNode* PatternMatchIdentifier::child(const chem::string_view &name) {
    return member_param->child(name);
}

VariantMember* PatternMatchExpr::find_member_from_expr(ASTAllocator& allocator, ASTDiagnoser& diagnoser) {
    const auto type = expression->create_type(allocator);
    if(!type) {
        diagnoser.error("couldn't resolve linked declaration", expression);
        return nullptr;
    }
    const auto linked_node = type->get_direct_linked_canonical_node();
    if(!linked_node) {
        diagnoser.error("couldn't resolve linked declaration", expression);
        return nullptr;
    }
    if(linked_node->kind() != ASTNodeKind::VariantDecl) {
        diagnoser.error("linked declaration is not a variant", expression);
        return nullptr;
    }
    const auto decl = linked_node->as_variant_def_unsafe();
    const auto found_child = decl->child(member_name);
    if(!found_child) {
        diagnoser.error(this) << "couldn't find member with name '" << member_name << "'";
        return nullptr;
    }
    if(found_child->kind() != ASTNodeKind::VariantMember) {
        diagnoser.error("member is not a variant member", this);
        return nullptr;
    }
    return found_child->as_variant_member_unsafe();
}

bool PatternMatchExpr::link(SymbolResolver &linker, Value *&value_ptr, BaseType *expected_type) {
    if(!expression->link(linker, expression, nullptr)) {
        return false;
    }
    const auto child_member = find_member_from_expr(linker.allocator, linker);
    if(!child_member) {
        return false;
    }
    // set the member, so we don't need to resolve it again
    member = child_member;
    auto& params = child_member->values;
    if(elseExpression.kind == PatternElseExprKind::DefValue && param_names.size() != 1) {
        linker.error("must destructure one member for default value to work", this);
        return false;
    }
    for(const auto nameId : param_names) {
        auto found = params.find(nameId->identifier);
        if(found == params.end()) {
            linker.error("couldn't find parameter in variant member", nameId);
        } else {
            nameId->member_param = found->second;
            // we declare this id, so anyone can link with it
            linker.declare(nameId->identifier, nameId);
        }
    }
    auto& elseVal = elseExpression.value;
    if (elseVal && !elseVal->link(linker, elseVal, nullptr)) {
        return false;
    }
}

BaseType* PatternMatchExpr::create_type(ASTAllocator &allocator) {
    return new (allocator.allocate<VoidType>()) VoidType();
}