// Copyright (c) Qinetik 2025.

#include "DebugInfoBuilder.h"
#include <llvm/IR/DIBuilder.h>
#include <llvm/IR/DebugInfoMetadata.h>
#include "compiler/llvmimpl.h"
#include <filesystem>
#include "ast/base/ASTNode.h"
#include "compiler/mangler/NameMangler.h"
#include "ast/base/Value.h"
#include "ast/base/BaseType.h"
#include "core/source/LocationManager.h"
#include "compiler/Codegen.h"
#include "ast/structures/FunctionDeclaration.h"
#include "ast/statements/VarInit.h"
#include "ast/statements/Assignment.h"
#include "ast/statements/SwitchStatement.h"
#include "ast/structures/ImplDefinition.h"
#include "ast/structures/StructDefinition.h"
#include "ast/structures/VariantDefinition.h"
#include "ast/structures/InterfaceDefinition.h"
#include "ast/structures/UnionDef.h"
#include "ast/structures/WhileLoop.h"
#include "ast/structures/DoWhileLoop.h"
#include "ast/structures/LoopBlock.h"
#include "ast/structures/ForLoop.h"
#include "ast/statements/Break.h"
#include "ast/statements/Continue.h"
#include "ast/statements/Return.h"
#include "ast/structures/Namespace.h"
#include "ast/values/LambdaFunction.h"
#include "ast/statements/Unreachable.h"
#include "ast/structures/If.h"
#include "ast/structures/InitBlock.h"
#include "ast/structures/Namespace.h"
#include "ast/types/IntNType.h"
#include "ast/types/PointerType.h"
#include "ast/types/ReferenceType.h"
#include "ast/values/FunctionCall.h"

inline LocationManager::LocationPosData loc_node(DebugInfoBuilder* visitor, SourceLocation loc) {
    return visitor->loc_man.getLocationPos(loc);
}

std::pair<std::string, std::string> splitPath(const chem::string_view &absolutePath) {
    std::filesystem::path path(absolutePath.view());
    std::string fileName = path.filename().string();
    std::string directory = path.parent_path().string();
    return {fileName, directory};
}

inline llvm::StringRef to_ref(const chem::string_view& view) {
    return llvm::StringRef(view.data(), view.size());
}

DebugInfoBuilder::DebugInfoBuilder(
    LocationManager& loc_man,
    llvm::DIBuilder* builder,
    Codegen& gen,
    bool isEnabled
)  : loc_man(loc_man), builder(builder), gen(gen) {
    const auto m = gen.mode;
    const auto is_d = is_debug(m);
    isOptimized = !is_d;
    isEnabled = isEnabled;
    if(isEnabled) {
        diScopes.reserve(20);
        cachedTypes.reserve(60);
    }
}

void DebugInfoBuilder::update_builder(llvm::DIBuilder* new_builder) {
    builder = new_builder;
}

llvm::DICompileUnit* DebugInfoBuilder::createDiCompileUnit(const chem::string_view& abs_path) {
    unsigned MyCustomLangCode = 0x8001; // Vendor-specific code
    auto [fileName, dirPath] = splitPath(abs_path);
    diCompileUnit = builder->createCompileUnit(
            MyCustomLangCode,
            builder->createFile(fileName, dirPath),
            "Chemical",
            isOptimized,
            "",
            0 // <--- runtime version
    );
    diScopes.clear();
    return diCompileUnit;
}

void DebugInfoBuilder::start_di_compile_unit(llvm::DICompileUnit* unit) {
    if(isEnabled) {
#ifdef DEBUG
        if(!unit) {
            throw std::runtime_error("cannot start an empty di compile unit");
        }
#endif
        diCompileUnit = unit;
        diScopes.push_back(unit);
    }
}

llvm::DILocation* DebugInfoBuilder::di_loc(const Position& position) {
#ifdef DEBUG
    if(diScopes.empty()) {
        throw std::runtime_error("expected a scope to be present, when creating a di location");
    }
    if(diScopes.back() == diCompileUnit) {
        throw std::runtime_error("scope for a location cannot be a di compile unit");
    }
#endif
    return llvm::DILocation::get(*gen.ctx, position.line + 1, position.character + 1, diScopes.back());
}

llvm::DIType* get_cached_type(DebugInfoBuilder& di, ASTNode* node) {
    // checking if the type exists in ptr cache
    const auto cacheTy = di.cachedTypes.find(node);
    if(cacheTy != di.cachedTypes.end()) {
        return cacheTy->second;
    } else {
        return nullptr;
    }
}

llvm::DIType* to_di_type(DebugInfoBuilder& di, ASTNode* node, bool replaceable) {
    switch(node->kind()) {
        case ASTNodeKind::StructDecl: {
            const auto def = node->as_struct_def_unsafe();
            const auto cached_type = get_cached_type(di, node);
            if(cached_type) {
                return cached_type;
            }
            return replaceable ? di.create_replaceable_type(def) : di.create_struct_type(def);
        }
        default:
            const auto locId = node->get_located_id();
            return di.builder->createUnspecifiedType(locId ? to_ref(locId->identifier) : "UNSPECIFIED");
    }
}

llvm::DIType* to_di_type(DebugInfoBuilder& di, BaseType* type, bool replaceable) {
    switch(type->kind()) {
        case BaseTypeKind::IntN: {
            const auto intNType = type->as_intn_type_unsafe();
            const auto num_bits = intNType->num_bits(di.gen.is64Bit);
            switch(intNType->IntNKind()) {
                case IntNTypeKind::Char:
                    return di.builder->createBasicType("char", num_bits, llvm::dwarf::DW_ATE_signed_char);
                case IntNTypeKind::Short:
                    return di.builder->createBasicType("short", num_bits, llvm::dwarf::DW_ATE_signed);
                case IntNTypeKind::Int:
                    return di.builder->createBasicType("int", num_bits, llvm::dwarf::DW_ATE_signed);
                case IntNTypeKind::Long:
                    return di.builder->createBasicType("long", num_bits, llvm::dwarf::DW_ATE_signed);
                case IntNTypeKind::BigInt:
                    return di.builder->createBasicType("bigint", num_bits, llvm::dwarf::DW_ATE_signed);
                case IntNTypeKind::Int128:
                    return di.builder->createBasicType("int128", num_bits, llvm::dwarf::DW_ATE_signed);
                case IntNTypeKind::UChar:
                    return di.builder->createBasicType("uchar", num_bits, llvm::dwarf::DW_ATE_unsigned_char);
                case IntNTypeKind::UShort:
                    return di.builder->createBasicType("ushort", num_bits, llvm::dwarf::DW_ATE_unsigned);
                case IntNTypeKind::UInt:
                    return di.builder->createBasicType("uint", num_bits, llvm::dwarf::DW_ATE_unsigned);
                case IntNTypeKind::ULong:
                    return di.builder->createBasicType("ulong", num_bits, llvm::dwarf::DW_ATE_unsigned);
                case IntNTypeKind::UBigInt:
                    return di.builder->createBasicType("ubigint", num_bits, llvm::dwarf::DW_ATE_unsigned);
                case IntNTypeKind::UInt128:
                    return di.builder->createBasicType("int128", num_bits, llvm::dwarf::DW_ATE_unsigned);
            }
        }
        case BaseTypeKind::Float:
            return di.builder->createBasicType("float", 32, llvm::dwarf::DW_ATE_float);
        case BaseTypeKind::Double:
            return di.builder->createBasicType("double", 64, llvm::dwarf::DW_ATE_decimal_float);
        case BaseTypeKind::Pointer: {
            const auto ptrType = type->as_pointer_type_unsafe();
            const auto pointee = to_di_type(di, ptrType->type, replaceable);
            return di.builder->createPointerType(pointee, 64);
        }
        case BaseTypeKind::Reference: {
            const auto ptrType = type->as_reference_type_unsafe();
            const auto pointee = to_di_type(di, ptrType->type, replaceable);
            return di.builder->createReferenceType(llvm::dwarf::DW_TAG_reference_type, pointee, 64);
        }
        case BaseTypeKind::Linked: {
            const auto linkedType = type->as_linked_type_unsafe();
            return to_di_type(di, linkedType->linked, replaceable);
        }
        default:
            return di.builder->createUnspecifiedType(type->representation());
    }
}

void DebugInfoBuilder::end_di_compile_unit() {
    if(isEnabled) {
#ifdef DEBUG
        if(diCompileUnit != diScopes.back()) {
            throw std::runtime_error("current scope is not a compile unit");
        }
#endif
#ifdef DEBUG
        if(!replaceAbleTypes.empty()) {
            throw std::runtime_error("replaceable types isn't empty");
        }
#endif
        diCompileUnit = nullptr;
        diScopes.pop_back();
    }
}

/**
 * this method MUST always be called when the you use true for to_di_type which can include
 * replaceable types, this should be called right after you complete your type
 */
void finalizeReplaceableType(DebugInfoBuilder& di) {
    // this method is called recursively (in to_di_type), so we must create a copy
    auto copied_replaceable = di.replaceAbleTypes;
    di.replaceAbleTypes.clear();
    for(auto& rep : copied_replaceable) {
        const auto finalizedType = to_di_type(di, rep.first, false);
        if(rep.second->isTemporary()) {
            rep.second->replaceAllUsesWith(finalizedType);
        } else {
#ifdef DEBUG
            throw std::runtime_error("type no longer temporary");
#endif
        }
    }
}

void DebugInfoBuilder::finalize() {
    cachedTypes.clear();
    diScopes.clear();
    builder->finalize();
}

llvm::DICompositeType* DebugInfoBuilder::create_replaceable_type(StructDefinition* def) {
    const auto loc = loc_node(this, def->encoded_location());
    auto& pos = loc.start;
    auto replaceAble = builder->createReplaceableCompositeType(
            llvm::dwarf::DW_TAG_structure_type,
            to_ref(def->name_view()),
            diCompileUnit,
            diCompileUnit->getFile(),
            pos.line
    );
    replaceAbleTypes.emplace_back(def, replaceAble);
    return replaceAble;
}

// Helper to align an offset (in bits) to the given alignment (in bits).
static inline uint64_t alignTo(uint64_t offset, uint64_t alignment) {
    return ((offset + alignment - 1) / alignment) * alignment;
}

llvm::DICompositeType* DebugInfoBuilder::create_struct_type(StructDefinition* def) {

    // creating the actual type
    auto& dataLayout = gen.module->getDataLayout();
    const auto loc = loc_node(this, def->encoded_location());
    auto& pos = loc.start;
    const auto llvm_type = def->llvm_type(gen);
    const auto alignmentInBits = dataLayout.getABITypeAlign(llvm_type).value() * 8;
    std::vector<llvm::Metadata*> structEles;
    uint64_t varOffset = 0;
    for(const auto var : def->variables()) {
        const auto var_loc = loc_node(this, var->encoded_location());
        const auto var_ll_type = var->llvm_type(gen);
        const auto var_bit_size = var_ll_type->getScalarSizeInBits();
        // Get the ABI alignment in bytes and convert to bits.
        const auto var_bit_align = dataLayout.getABITypeAlign(var_ll_type).value() * 8;
        const auto mem_di_type = to_di_type(*this, var->known_type(), true);
        // Align the current offset to the required alignment.
        varOffset = alignTo(varOffset, var_bit_align);
        const auto mem_type = builder->createMemberType(
                diCompileUnit,
                to_ref(var->name),
                diCompileUnit->getFile(),
                var_loc.start.line,
                var_bit_size,
                var_bit_align,
                varOffset,
                llvm::DINode::DIFlags::FlagZero,
                mem_di_type
        );
        structEles.emplace_back(mem_type);
    }

    const auto diNodeArr = builder->getOrCreateArray(structEles);
    const auto structType = builder->createStructType(
            diCompileUnit,
            to_ref(def->name_view()),
            diCompileUnit->getFile(),
            pos.line, llvm_type->getScalarSizeInBits(),
            alignmentInBits,
            llvm::DINode::DIFlags::FlagZero,
            nullptr,
            diNodeArr
    );

    // save this type in cache
    cachedTypes[def] = structType;

    // replace any replaceable types for this definition, that may have been formed
    // if a pointer exists to definition inside the struct (self referential) we create a replaceable type
    // then we replace all usages of that replaceable type once we create complete struct type
    finalizeReplaceableType(*this);

    return structType;
}

void DebugInfoBuilder::instr(llvm::Instruction* inst, const Position& position) {
    if(isEnabled) {
        inst->setDebugLoc(di_loc(position));
    }
}

void DebugInfoBuilder::instr(llvm::Instruction* inst, SourceLocation source_loc) {
    if(inst && isEnabled) {
        const auto location = loc_node(this, source_loc);
        inst->setDebugLoc(di_loc(location.start));
    }
}

llvm::DIScope* DebugInfoBuilder::create(FunctionTypeBody* decl, llvm::Function* func) {
    const auto alreadyProgram = func->getSubprogram();
    if(alreadyProgram) {
        return alreadyProgram;
    }
    const auto location = loc_node(this, decl->get_location());
    const auto as_func = decl->as_function();
    const auto name_view = as_func ? to_ref(as_func->name_view()) : func->getName();
#ifdef DEBUG
    if(diScopes.empty()) {
        throw std::runtime_error("expected a compile unit scope to be present when starting a function scope");
    }
#endif
    std::vector<llvm::Metadata*> mds;
    for(const auto param : decl->params) {
        const auto diParam = to_di_type(*this, param->type, false);
        if(diParam) {
            mds.emplace_back(diParam);
        }
    }
    const auto typeArray = builder->getOrCreateTypeArray(mds);
    const auto subroutineType = builder->createSubroutineType(typeArray);
    // currently all functions go into the di compile unit scope (we don't know if it's right)
    // however saving the ll file fails if we use any other scope
    llvm::DISubprogram *SP = builder->createFunction(
            diCompileUnit,
            name_view,
            func->getName(),  // Linkage name
            diCompileUnit->getFile(),
            location.start.line + 1,    // Line number of the function
            subroutineType,
            location.start.character,
            llvm::DINode::FlagFwdDecl,
            llvm::DISubprogram::SPFlagDefinition
    );
    func->setSubprogram(SP);
    return SP;
}

void DebugInfoBuilder::start_nested_function_scope(FunctionTypeBody *decl, llvm::Function* func) {
    if(!isEnabled) {
        return;
    }
    diScopes.push_back(create(decl, func));
}

void DebugInfoBuilder::start_function_scope(FunctionTypeBody *decl, llvm::Function* func) {
    if(!isEnabled) {
        return;
    }
    diScopes.push_back(create(decl, func));
}

void DebugInfoBuilder::end_function_scope() {
    if(!isEnabled) {
        return;
    }
    const auto last_scope = diScopes.back();
    if (llvm::isa<llvm::DISubprogram>(last_scope)) {
        // scope is a DISubprogram
        auto *subprogram = llvm::cast<llvm::DISubprogram>(last_scope);
        builder->finalizeSubprogram(subprogram);
    } else {
#ifdef DEBUG
        throw std::runtime_error("ending function scope is not a subprogram");
#endif
    }
    diScopes.pop_back();
}

void DebugInfoBuilder::start_scope(SourceLocation source_loc) {
    if(!isEnabled) {
        return;
    }
#ifdef DEBUG
    if(diScopes.empty()) {
        throw std::runtime_error("expected a function scope to be present, when starting a lexical block");
    }
#endif
    const auto start = loc_node(this, source_loc).start;
    const auto lexBlock = builder->createLexicalBlock(diScopes.back(), diCompileUnit->getFile(), start.line + 1, start.character + 1);
    diScopes.push_back(lexBlock);
}

void DebugInfoBuilder::end_scope() {
    if(!isEnabled) {
        return;
    }
    diScopes.pop_back();
}

void DebugInfoBuilder::declare(VarInitStatement *init, llvm::Value* val) {
    if(!isEnabled) {
        return;
    }
    const auto location = loc_node(this, init->encoded_location());
    if(init->is_top_level()) {
        builder->createGlobalVariableExpression(
            diScopes.back(),
            to_ref(init->name_view()),
            gen.mangler.mangle(init),
            diCompileUnit->getFile(),
            location.start.line + 1,
            to_di_type(*this, init->known_type(), false),
            !init->is_exported(),
            init->value != nullptr,
            builder->createExpression()
        );
    } else {
        llvm::DILocalVariable* Var = builder->createAutoVariable(
                diScopes.back(),
                to_ref(init->name_view()),
                diCompileUnit->getFile(),
                location.start.line + 1,
                to_di_type(*this, init->known_type(), false)
        );
        const auto loc = di_loc(location.start);
        if (!init->is_const() && llvm::isa<llvm::Instruction>(val)) {
            llvm::Instruction *inst = llvm::cast<llvm::Instruction>(val);
            builder->insertDeclare(
                    init->llvm_ptr,                                         // Variable allocation
                    Var,
                    builder->createExpression(),
                    loc,
                    inst
            );
        } else {
            builder->insertDbgValueIntrinsic(
               val,
               Var,
               builder->createExpression(),
               loc,
               gen.builder->GetInsertBlock()
            );
        }
    }
}