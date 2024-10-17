// Copyright (c) Qinetik 2024.

#include <clang/Frontend/ASTUnit.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/AST/APValue.h>
#include <clang/AST/Attr.h>
#include <clang/AST/Expr.h>
#include <clang/AST/Type.h>
#include <llvm/ADT/APFloat.h>
#include <clang/AST/RecordLayout.h>
#include "ast/base/ASTNode.h"
#include "ast/base/BaseType.h"
#include <memory>
#include "ast/structures/FunctionParam.h"
#include "ast/structures/FunctionDeclaration.h"
#include "ast/structures/EnumDeclaration.h"
#include "compiler/ctranslator/CTranslator.h"
#include "ast/types/PointerType.h"
#include "ast/types/ReferenceType.h"
#include "ast/types/LinkedType.h"
#include "ast/types/ArrayType.h"
#include "ast/structures/StructDefinition.h"
#include "ast/statements/Typealias.h"
#include "ast/statements/VarInit.h"
#include "ast/types/DoubleType.h"
#include "ast/types/FloatType.h"
#include "ast/types/VoidType.h"
#include "ast/types/BoolType.h"
#include "ast/types/PointerType.h"
#include "ast/values/BoolValue.h"
#include "ast/values/DoubleValue.h"
#include "ast/values/FloatValue.h"
#include "ast/values/BigIntValue.h"
#include "ast/values/UBigIntValue.h"
#include "ast/values/UShortValue.h"
#include "ast/values/UCharValue.h"
#include "ast/values/LongValue.h"
#include "ast/values/ULongValue.h"
#include "ast/values/Expression.h"
#include "ast/values/VariableIdentifier.h"
#include "ast/values/IntValue.h"
#include "ast/values/UIntValue.h"
#include "ast/values/ShortValue.h"
#include "ast/values/NumberValue.h"
#include "compiler/ClangCodegen.h"
#include <clang/AST/Decl.h>
#include <clang/AST/Mangle.h>
#include "compiler/Codegen.h"

struct ErrorMsg {
    const char *filename_ptr; // can be null
    unsigned long filename_len;
    const char *msg_ptr;
    unsigned long msg_len;
    const char *source; // valid until the ASTUnit is freed. can be null
    unsigned line; // 0 based
    unsigned column; // 0 based
    unsigned offset; // byte offset into source
};

BaseType* decl_type(CTranslator& translator, clang::Decl* decl, std::string name) {
    auto found = translator.declarations.find(decl);
    if(found != translator.declarations.end()) {
        return new (translator.allocator.allocate<LinkedType>()) LinkedType(std::move(name), found->second, nullptr);
    } else {
        return nullptr;
//        const auto maker = translator.node_makers[decl->getKind()];
//        if(maker) {
//            auto cDecl = maker(&translator, decl);
//            if(cDecl) {
//                translator.before_nodes.emplace_back(cDecl);
//                return new(translator.allocator.allocate<LinkedType>()) LinkedType(std::move(name), cDecl, nullptr);
//            } else {
//                return nullptr;
//            };
//        } else {
//            return nullptr;
//        }
    }
}

BaseType* CTranslator::make_type(clang::QualType* type) {
    auto canonical = type->getCanonicalType();
    auto ptr = type->getTypePtr();
    auto canon_ptr = canonical.getTypePtr();
    const auto is_mutable = !type->isConstQualified();
    if(ptr->isPointerType()) {
        auto point = ptr->getPointeeType();
        auto pointee = make_type(&point);
        if(!pointee) {
            return nullptr;
        }
        return new (allocator.allocate<PointerType>()) PointerType(pointee, nullptr, is_mutable);
    }
//    if(canon_ptr != ptr) { // reference
//        if(canon_ptr->isRecordType()) {
//            return make_type(&canonical);
//        }
//        const auto other = ptr->getAs<clang::TypedefType>();
//        auto str = type->getAsString();
//        return new (allocator.allocate<LinkedType>()) LinkedType(str, nullptr);
//    }
    if(ptr->isBuiltinType()) {
        auto builtIn = static_cast<clang::BuiltinType*>(const_cast<clang::Type*>(ptr));
        const auto type_maker = type_makers[builtIn->getKind()];
        if(type_maker) {
            return type_maker(allocator, builtIn);
        } else {
            error("builtin type maker failed with kind " + std::to_string(builtIn->getKind()) + " with representation " + builtIn->getName(clang::PrintingPolicy{clang::LangOptions{}}).str() + " with actual " + type->getAsString());
            return nullptr;
        }
    } else if(ptr->isRecordType()){
        const auto record_decl = ptr->getAsRecordDecl();
        return decl_type(*this, record_decl, record_decl->getNameAsString());
    } else if(ptr->isTypedefNameType()){
        const auto type_def_type = ptr->getAs<clang::TypedefType>();
        const auto decl = type_def_type->getDecl();
        return decl_type(*this, decl, decl->getNameAsString());
    }
//    else if(ptr->isArrayType()) { // couldn't make use of it
//        auto point = ptr->getAsArrayTypeUnsafe();
//        auto elem_type = point->getElementType();
//        auto pointee = new_type(translator, &elem_type);
//        if(!pointee) {
//            return nullptr;
//        }
//        return new ArrayType(std::unique_ptr<BaseType>(pointee), -1);
//    }
    else {
        error("unknown type given to new_type with representation " + type->getAsString());
        return nullptr;
    };
}

Value* convert_to_number(ASTAllocator& alloc, bool is64Bit, unsigned int bitWidth, bool is_signed, uint64_t value, CSTToken* token = nullptr) {
    switch(bitWidth) {
        case 1:
            return new (alloc.allocate<BoolValue>()) BoolValue((bool) value, token);
        case 8:
            if(is_signed) {
                return new (alloc.allocate<CharValue>()) CharValue((char) value, token);
            } else {
                return new (alloc.allocate<UCharValue>()) UCharValue((unsigned char) value, token);
            }
        case 16:
            if(is_signed) {
                return new (alloc.allocate<ShortValue>()) ShortValue((short) value, token);
            } else {
                return new (alloc.allocate<UShortValue>()) UShortValue((unsigned short) value, token);
            }
        case 32:
            if(is_signed) {
                return new (alloc.allocate<IntValue>()) IntValue((int) value, token);
            } else {
                return new (alloc.allocate<UIntValue>()) UIntValue((unsigned int) value, token);
            }
        case 64:
            if(is_signed) {
                return new (alloc.allocate<BigIntValue>()) BigIntValue((long long) value, token);
            } else {
                return new (alloc.allocate<UBigIntValue>()) UBigIntValue((unsigned long long) value, token);
            }
        default:
#ifdef DEBUG
            throw std::runtime_error("value couldn't be created");
#endif
            return nullptr;
    }
}

EnumDeclaration* CTranslator::make_enum(clang::EnumDecl* decl) {
    auto enum_decl = new (allocator.allocate<EnumDeclaration>()) EnumDeclaration(decl->getNameAsString(), {}, parent_node, nullptr);
    std::unordered_map<std::string, std::unique_ptr<EnumMember>> members;
    unsigned index = 0;
    for(auto mem : decl->enumerators()) {
        Value* mem_value = nullptr;
        auto init_expr = mem->getInitExpr();
        if(init_expr) {
            mem_value = make_expr(init_expr);
        } else {
            const auto& init_val = mem->getInitVal();
            auto bitWidth = init_val.getBitWidth();
            auto value = init_val.getLimitedValue();
            mem_value = convert_to_number(allocator, is64Bit, bitWidth, init_val.isSigned(), value, nullptr);
        }
        enum_decl->members[mem->getNameAsString()] = new (allocator.allocate<EnumMember>()) EnumMember(mem->getNameAsString(), index, mem_value, enum_decl, nullptr);
        index++;
    }
    return enum_decl;
}

StructDefinition* CTranslator::make_struct(clang::RecordDecl* decl) {
    auto def = new (allocator.allocate<StructDefinition>()) StructDefinition(decl->getNameAsString(), parent_node, nullptr);
    for(auto str : decl->fields()) {
        auto field_type = str->getType();
        auto field_type_conv = make_type(&field_type);
        if(!field_type_conv) {
            return nullptr;
        }
        def->variables[str->getNameAsString()] = new (allocator.allocate<StructMember>()) StructMember(str->getNameAsString(), field_type_conv, nullptr, def, nullptr);
    }
    return def;
}

TypealiasStatement* CTranslator::make_typealias(clang::TypedefDecl* decl) {
    if(decl->getName().starts_with("__")) {
        return nullptr;
    }
    auto decl_type = decl->getUnderlyingType();
    auto type = make_type(&decl_type);
    if(type == nullptr) return nullptr;

    // do not create a typealias for when user typedef's a struct with same name
    const auto type_kind = type->kind();
    if(type_kind == BaseTypeKind::Linked) {
        const auto linked_type = (LinkedType*) type;
        const auto linked = linked_type->linked;
        const auto node_id = linked->ns_node_identifier();
        if(node_id == decl->getName()) {
            return nullptr;
        }


        // typealias for a struct where struct is unnamed
        if(linked_type->type.empty()) {
            const auto linked_kind = linked->kind();
            if(linked_kind == ASTNodeKind::StructDecl) {
                const auto node = linked->as_struct_def_unsafe();
                if(node->name.empty()) {
                    node->name = decl->getName();
                    return nullptr;
                }
            }
        }

    }

    return new (allocator.allocate<TypealiasStatement>()) TypealiasStatement(decl->getNameAsString(), type, parent_node, nullptr);
}

std::optional<Operation> convert_to_op(clang::BinaryOperatorKind kind) {
    switch(kind) {
        case clang::BinaryOperatorKind::BO_PtrMemD: // ".*"
            return std::nullopt;
        case clang::BinaryOperatorKind::BO_PtrMemI: // "->*"
            return std::nullopt;
        case clang::BinaryOperatorKind::BO_Mul: // "*"
            return Operation::Multiplication;
        case clang::BinaryOperatorKind::BO_Div: // "/"
            return Operation::Division;
        case clang::BinaryOperatorKind::BO_Rem: // "%"
            return Operation::Modulus;
        case clang::BinaryOperatorKind::BO_Add: // "+"
            return Operation::Addition;
        case clang::BinaryOperatorKind::BO_Sub: // "-"
            return Operation::Subtraction;
        case clang::BinaryOperatorKind::BO_Shl: // "<<"
            return Operation::LeftShift;
        case clang::BinaryOperatorKind::BO_Shr: // ">>"
            return Operation::RightShift;
        case clang::BinaryOperatorKind::BO_Cmp: // "<=>"
            return std::nullopt;
        case clang::BinaryOperatorKind::BO_LT: // "<"
            return Operation::LessThan;
        case clang::BinaryOperatorKind::BO_GT: // ">"
            return Operation::GreaterThan;
        case clang::BinaryOperatorKind::BO_LE: // "<="
            return Operation::LessThanOrEqual;
        case clang::BinaryOperatorKind::BO_GE: // ">="
            return Operation::GreaterThanOrEqual;
        case clang::BinaryOperatorKind::BO_EQ: // "=="
            return Operation::IsEqual;
        case clang::BinaryOperatorKind::BO_NE: // "!="
            return Operation::IsNotEqual;
        case clang::BinaryOperatorKind::BO_And: // "&"
            return Operation::BitwiseAND;
        case clang::BinaryOperatorKind::BO_Xor: // "^"
            return Operation::BitwiseXOR;
        case clang::BinaryOperatorKind::BO_Or: // "|"
            return Operation::BitwiseOR;
        case clang::BinaryOperatorKind::BO_LAnd: // "&&"
            return Operation::LogicalAND;
        case clang::BinaryOperatorKind::BO_LOr: // "||"
            return Operation::LogicalOR;
        case clang::BinaryOperatorKind::BO_Assign: // "="
            return Operation::Assignment;
        case clang::BinaryOperatorKind::BO_MulAssign: // "*="
            return Operation::MultiplyBy;
        case clang::BinaryOperatorKind::BO_DivAssign: // "/="
            return Operation::DivideBy;
        case clang::BinaryOperatorKind::BO_RemAssign: // "%="
            return Operation::ModuloBy;
        case clang::BinaryOperatorKind::BO_AddAssign: // "+="
            return Operation::AddTo;
        case clang::BinaryOperatorKind::BO_SubAssign: // "-="
            return Operation::SubtractFrom;
        case clang::BinaryOperatorKind::BO_ShlAssign: // "<<="
            return Operation::ShiftLeftBy;
        case clang::BinaryOperatorKind::BO_ShrAssign: // ">>="
            return Operation::ShiftRightBy;
        case clang::BinaryOperatorKind::BO_AndAssign: // "&="
            return Operation::ANDWith;
        case clang::BinaryOperatorKind::BO_XorAssign: // "^="
            return std::nullopt;
        case clang::BinaryOperatorKind::BO_OrAssign: // "|="
            return std::nullopt;
        case clang::BinaryOperatorKind::BO_Comma: // ","
            return std::nullopt;
        default:
            return std::nullopt;
    }
}

Value* CTranslator::make_expr(clang::Expr* expr) {
    if (auto* intLiteral = llvm::dyn_cast<clang::IntegerLiteral>(expr)) {
        // Handle integer literals
        const auto is_signed = intLiteral->getType()->isSignedIntegerType();
        auto value = intLiteral->getValue();
        const auto bitWidth = value.getBitWidth();
        auto real_val = value.getLimitedValue();
        return convert_to_number(allocator, is64Bit, bitWidth, is_signed, real_val, nullptr);
    } else if (auto* floatLiteral = llvm::dyn_cast<clang::FloatingLiteral>(expr)) {
        // Handle floating-point literals
        llvm::APFloat value = floatLiteral->getValue();
        const clang::BuiltinType* builtinType = floatLiteral->getType()->getAs<clang::BuiltinType>();
        if (builtinType->getKind() == clang::BuiltinType::Float) {
            // Single precision (float)
            auto floatValue = value.convertToFloat();
            return new (allocator.allocate<FloatValue>()) FloatValue(floatValue, nullptr);
        } else {
            // Double precision
            auto doubleValue = value.convertToDouble();
            return new (allocator.allocate<DoubleValue>()) DoubleValue(doubleValue, nullptr);
        }
    } else if (auto* boolLiteral = llvm::dyn_cast<clang::CXXBoolLiteralExpr>(expr)) {
        bool value = boolLiteral->getValue();
        return new (allocator.allocate<BoolValue>()) BoolValue(value, nullptr);
    } else if (auto* declRef = llvm::dyn_cast<clang::DeclRefExpr>(expr)) {
        clang::ValueDecl* decl = declRef->getDecl();
        std::string name = decl->getNameAsString();
        auto found = declarations.find(decl);
        if(found != declarations.end()) {
            const auto id = new (allocator.allocate<VariableIdentifier>()) VariableIdentifier(name, nullptr, false);
            id->linked = found->second;
            return id;
        } else {
            return nullptr;
        }
    } else {
        // Handle other expression types as needed
        // Placeholder for more complex expressions like binary operations, function calls, etc.
        // Example for binary operation
        if (auto* binOp = llvm::dyn_cast<clang::BinaryOperator>(expr)) {
            const auto lhs = make_expr(binOp->getLHS());
            const auto rhs = make_expr(binOp->getRHS());
            const auto opt = convert_to_op(binOp->getOpcode());
            if(opt.has_value()) {
                return new (allocator.allocate<Expression>()) Expression(lhs, rhs, opt.value(), is64Bit, nullptr);
            } else {
                return nullptr;
            }
        }
    }

    // If the expression type is not handled, return nullptr or throw an error
    return nullptr;
}

VarInitStatement* CTranslator::make_var_init(clang::VarDecl* decl) {
    auto type = decl->getType();
    auto made_type = make_type(&type);
    if(!made_type) {
        return nullptr;
    }
    AccessSpecifier specifier;
    auto info = decl->getLinkageAndVisibility();
    switch(info.getLinkage()) {
        case clang::NoLinkage:
        case clang::VisibleNoLinkage:
            specifier = AccessSpecifier::Private;
            break;
        case clang::ModuleLinkage:
        case clang::InternalLinkage:
            specifier = AccessSpecifier::Internal;
            specifier = AccessSpecifier::Internal;
            break;
        case clang::ExternalLinkage:
        case clang::UniqueExternalLinkage:
            specifier = AccessSpecifier::Public;
            break;
    }
    const auto initializer = decl->getInit();
    auto initial_value = initializer ? (Value*) make_expr(initializer) : nullptr;
    return new (allocator.allocate<VarInitStatement>()) VarInitStatement(false, decl->getNameAsString(), made_type, initial_value, parent_node, nullptr, specifier);
}

FunctionDeclaration* CTranslator::make_func(clang::FunctionDecl* func_decl) {
    // skip builtins
    if(func_decl->getName().starts_with("__")) {
        return nullptr;
    }
    if(func_decl->getName() == "_atoflt") {
        int i = 0;
    }
    // Check if the declaration is for the printf function
    // Extract function parameters
    std::vector<FunctionParam*> params;
    unsigned index = 0;
    bool skip_fn = false;
    for (const auto *param: func_decl->parameters()) {
        auto type = param->getType();
        auto chem_type = make_type(&type);
        if (!chem_type) {
            skip_fn = true;
            continue;
        }
        auto param_name = param->getNameAsString();
        if(param_name.empty()) {
            param_name = "_";
        }
        params.emplace_back(new (allocator.allocate<FunctionParam>()) FunctionParam(
                param_name,
                chem_type,
                index,
                nullptr,
                false,
                nullptr,
                nullptr
        ));
        index++;
    }
    if(skip_fn) {
        return nullptr;
    }
    auto ret_type = func_decl->getReturnType();
    auto chem_type = make_type(&ret_type);
    if (!chem_type) {
        return nullptr;
    }
    auto decl = new (allocator.allocate<FunctionDeclaration>()) FunctionDeclaration(
            func_decl->getNameAsString(),
            std::move(params),
            chem_type,
            func_decl->isVariadic(),
            parent_node,
            nullptr,
            std::nullopt
    );
    decl->assign_params();
    return decl;
}

std::string get_decl_name(const clang::Decl* decl) {
    if (const auto* namedDecl = llvm::dyn_cast<clang::NamedDecl>(decl)) {
        return namedDecl->getNameAsString();
    }
    return "<unnamed>";
}

std::size_t hash_decl(clang::Decl* decl, clang::SourceManager& SM, clang::SourceLocation loc) {
    if(loc.isInvalid()) return 0;
    const auto Decomposed = SM.getDecomposedLoc(loc);
    const auto fileID = Decomposed.first;
    const auto fileEntry = SM.getFileEntryForID(fileID);
    if(!fileEntry) return 0;
    const auto& unId = fileEntry->getUniqueID();
    std::size_t seed = 0;
    // we don't hash the unId.getDevice, maybe it would be useful someday
    seed ^= std::hash<uint64_t>()(unId.getFile()) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    seed ^= std::hash<unsigned int>()(Decomposed.second) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    return seed;
}

void print_loc_for_decl(clang::Decl* decl, clang::SourceManager& SM) {
    clang::SourceLocation loc = decl->getLocation();
    if (loc.isInvalid()) {
        std::cerr << "Invalid source location" << std::endl;
        return;
    }

    // Retrieve the file ID and the file name
    auto decomposed = SM.getDecomposedLoc(loc);
    clang::FileID fileID = decomposed.first;
    const clang::FileEntry* fileEntry = SM.getFileEntryForID(fileID);
    if (!fileEntry) {
        std::cerr << "Cannot find file entry for the location" << std::endl;
        return;
    }
    const auto& unId = fileEntry->getUniqueID();

    std::string filename = fileEntry->getName().str();

    // Determine if it is a header or source file based on extension
    std::string filetype = "source file";
    if (filename.find(".h") != std::string::npos || filename.find(".hpp") != std::string::npos) {
        filetype = "header file";
    }

    const auto offset = decomposed.second;

    unsigned line = SM.getSpellingLineNumber(loc);
    unsigned column = SM.getSpellingColumnNumber(loc);

    std::cout << "Defined " <<  get_decl_name(decl) << " in " << filetype << ": " << filename << ":" << line << ":" << column << " e: " << loc.getRawEncoding() << " f: " << fileID.getHashValue() << " unId: " << unId.getDevice() << ':' << unId.getFile() << " uid: " << fileEntry->getUID() << " o: " << offset << std::endl;

//    std::cout << "Declaration is defined in " << filetype << ": " << filename << std::endl;
}

void invalid_loc_err(clang::Decl* decl, clang::SourceManager& SM, clang::SourceLocation loc) {
    unsigned line = SM.getSpellingLineNumber(loc);
    unsigned column = SM.getSpellingColumnNumber(loc);
    clang::FileID fileID = SM.getFileID(loc);
    const clang::FileEntry* fileEntry = SM.getFileEntryForID(fileID);
    const auto& unId = fileEntry->getUniqueID();
    if (!fileEntry) {
        std::cerr << "Cannot find file entry for the location" << std::endl;
        return;
    }
    std::string filename = fileEntry->getName().str();
    // Determine if it is a header or source file based on extension
    std::string filetype = "source file";
    if (filename.find(".h") != std::string::npos || filename.find(".hpp") != std::string::npos) {
        filetype = "header file";
    }
    std::cout << "invalid location for " <<  get_decl_name(decl) << " in " << filetype << ": " << filename << ":" << line << ":" << column << " e: " << loc.getRawEncoding() << " f: " << fileID.getHashValue() << " unId: " << unId.getDevice() << ':' << unId.getFile() << " uid: " << fileEntry->getUID() << std::endl;
}

void CTranslator::translate_no_check(clang::Decl* decl) {
    auto maker = node_makers[decl->getKind()];
    if(maker) {
        auto node = maker(this, decl);
        if (node) {
            declarations[decl] = node;
            nodes.emplace_back(node);
        }
    } else {
        error("couldn't convert decl with kind " + std::to_string(decl->getKind()) + " & kind name " + decl->getDeclKindName());
    }
}

void CTranslator::translate_checking(clang::Decl* decl, clang::SourceManager& sourceMan) {
    const auto hashed_decl = hash_decl(decl, sourceMan, decl->getLocation());
    if(hashed_decl == 0) {
        return;
    }
    // check if node has already been translated and resuse that
    auto translated_pair = translated_map.find(hashed_decl);
    if (translated_pair != translated_map.end()) {
        const auto node = translated_pair->second;
        declarations[decl] = node;
        // check node has not been declared in this module before
        auto found = declared_in_module.find(node);
        if (found == declared_in_module.end()) {
            nodes.emplace_back(node);
            declared_in_module[node] = true;
        }
        return;
    }
    auto maker = node_makers[decl->getKind()];
    if(maker) {
        auto node = maker(this, decl);
        if (node) {
            declarations[decl] = node;
            translated_map[hashed_decl] = node;
            declared_in_module[node] = true;
            nodes.emplace_back(node);
        }
    } else {
        error("couldn't convert decl with kind " + std::to_string(decl->getKind()) + " & kind name " + decl->getDeclKindName());
    }
}

void CTranslator::translate(clang::ASTUnit *unit) {
    // set current unit
    current_unit = unit;
    auto& source_man = unit->getSourceManager();
    const auto check = check_decls_across_invocations;
    // translating declarations
    auto tud = unit->getASTContext().getTranslationUnitDecl();
    for (auto decl: tud->decls()) {
        if(check) {
            translate_checking(decl, source_man);
        } else {
            translate_no_check(decl);
        }
    }
}

clang::ASTUnit *ClangLoadFromCommandLine(
        const char **args_begin,
        const char **args_end,
        struct ErrorMsg **errors_ptr,
        unsigned long *errors_len,
        const char *resources_path,
        clang::IntrusiveRefCntPtr<clang::DiagnosticsEngine> diags
) {

    std::shared_ptr<clang::PCHContainerOperations> pch_container_ops = std::make_shared<clang::PCHContainerOperations>();

    bool only_local_decls = true;
    bool user_files_are_volatile = true;
    bool allow_pch_with_compiler_errors = false;
    bool single_file_parse = false;
    bool for_serialization = false;
    bool retain_excluded_conditional_blocks = false;
    bool store_preambles_in_memory = false;
    llvm::StringRef preamble_storage_path = llvm::StringRef();
    clang::ArrayRef<clang::ASTUnit::RemappedFile> remapped_files = std::nullopt;
    std::unique_ptr<clang::ASTUnit> err_unit;
    llvm::IntrusiveRefCntPtr<llvm::vfs::FileSystem> VFS = nullptr;
    std::optional<llvm::StringRef> ModuleFormat = std::nullopt;
    std::unique_ptr<clang::ASTUnit> ast_unit_unique_ptr = clang::ASTUnit::LoadFromCommandLine(
            args_begin, args_end,
            pch_container_ops,
            diags,
            resources_path,
            store_preambles_in_memory,
            preamble_storage_path,
            only_local_decls,
            clang::CaptureDiagsKind::All,
            remapped_files,
            true, // remapped files keep original name
            0, // precompiled preable after n parses
            clang::TU_Complete,
            false, // cache code completion results
            false, // include brief comments in code completion
            allow_pch_with_compiler_errors,
            clang::SkipFunctionBodiesScope::None,
            single_file_parse,
            user_files_are_volatile,
            for_serialization,
            retain_excluded_conditional_blocks,
            ModuleFormat,
            &err_unit,
            VFS);
    clang::ASTUnit *ast_unit = ast_unit_unique_ptr.release();

    *errors_len = 0;

    // Early failures in LoadFromCommandLine may return with ErrUnit unset.
    if (!ast_unit && !err_unit) {
        return nullptr;
    }

    if (diags->hasErrorOccurred()) {
        // Take ownership of the err_unit ASTUnit object so that it won't be
        // free'd when we return, invalidating the error message pointers
        clang::ASTUnit *unit = ast_unit ? ast_unit : err_unit.release();
        ErrorMsg *errors = nullptr;

        for (clang::ASTUnit::stored_diag_iterator it = unit->stored_diag_begin(),
                     it_end = unit->stored_diag_end(); it != it_end; ++it) {
            switch (it->getLevel()) {
                case clang::DiagnosticsEngine::Ignored:
                case clang::DiagnosticsEngine::Note:
                case clang::DiagnosticsEngine::Remark:
                case clang::DiagnosticsEngine::Warning:
                    continue;
                case clang::DiagnosticsEngine::Error:
                case clang::DiagnosticsEngine::Fatal:
                    break;
            }

            llvm::StringRef msg_str_ref = it->getMessage();

            *errors_len += 1;
            errors = reinterpret_cast<ErrorMsg *>(realloc(errors, sizeof(ErrorMsg) * *errors_len));
            if (errors == nullptr) abort();
            ErrorMsg *msg = &errors[*errors_len - 1];
            memset(msg, 0, sizeof(*msg));

            msg->msg_ptr = (const char *) msg_str_ref.bytes_begin();
            msg->msg_len = msg_str_ref.size();

            clang::FullSourceLoc fsl = it->getLocation();

            // Ensure the source location is valid before expanding it
            if (fsl.isInvalid()) {
                std::cerr << "FullSourceLoc is invalid" << std::endl;
                continue;
            }
            // Expand the location if possible
            fsl = fsl.getFileLoc();

            // The only known way to obtain a Loc without a manager associated
            // to it is if you have a lot of errors clang emits "too many errors
            // emitted, stopping now"
            if (fsl.hasManager()) {
                const clang::SourceManager &SM = fsl.getManager();

                clang::PresumedLoc presumed_loc = SM.getPresumedLoc(fsl);
                assert(!presumed_loc.isInvalid());

                msg->line = presumed_loc.getLine() - 1;
                msg->column = presumed_loc.getColumn() - 1;

                clang::StringRef filename = presumed_loc.getFilename();
                if (!filename.empty()) {
                    msg->filename_ptr = (const char *) filename.bytes_begin();
                    msg->filename_len = filename.size();
                }

                bool invalid;
                clang::StringRef buffer = fsl.getBufferData(&invalid);

                if (!invalid) {
                    msg->source = (const char *) buffer.bytes_begin();
                    msg->offset = SM.getFileOffset(fsl);
                }
            } else {
                std::cerr << "NO Filemanager associated" << std::endl;
            }
        }

        *errors_ptr = errors;

        return nullptr;
    }

    return ast_unit;
}

//----------------------------- Clang Codegen -------------------------------
//--------------------------------------------------------------------------
//--------------------------------------------------------------------------

class ClangCodegenImpl {
public:

    clang::CompilerInstance compiler;

    std::unique_ptr<clang::MangleContext> mangler;

};

ClangCodegen::ClangCodegen(std::string target_triple, ManglerKind manglerKind) : impl(new ClangCodegenImpl()) {

    auto& compiler = impl->compiler;
    compiler.createDiagnostics();

    auto TO = std::make_shared<clang::TargetOptions>();
    TO->Triple = std::move(target_triple);
    compiler.setTarget(clang::TargetInfo::CreateTargetInfo(compiler.getDiagnostics(), TO));

    // Create the necessary file manager and source manager
    compiler.createFileManager();
    compiler.createSourceManager(compiler.getFileManager());

    // Initialize the preprocessor
    compiler.createPreprocessor(clang::TU_Complete);

    // Set language options (these are essential)
    clang::LangOptions &langOpts = compiler.getLangOpts();
    langOpts.CPlusPlus = true;  // Enable C++ mode
    langOpts.RTTI = true;       // Enable RTTI if needed
    langOpts.CXXExceptions = true; // Enable C++ exceptions

    // Create AST context
    compiler.createASTContext();

    // create mangler kind
    switch_mangler_kind(manglerKind);

}

void ClangCodegen::switch_to_cpp() {
    auto& compiler = impl->compiler;
    // Set language options (these are essential)
    auto& langOpts = compiler.getLangOpts();
    langOpts.CPlusPlus = true;  // Enable C++ mode
    langOpts.RTTI = true;       // Enable RTTI if needed
    langOpts.CXXExceptions = true; // Enable C++ exceptions
}

void ClangCodegen::switch_to_c() {
    auto& compiler = impl->compiler;
    // Set language options (these are essential)
    auto& langOpts = compiler.getLangOpts();
    langOpts.CPlusPlus = false;  // Enable C++ mode
    langOpts.RTTI = false;       // Enable RTTI if needed
    langOpts.CXXExceptions = false; // Enable C++ exceptions
}

void ClangCodegen::switch_mangler_kind(ManglerKind kind) {
    auto& compiler = impl->compiler;
    auto& context = compiler.getASTContext();
    switch(kind) {
        case ManglerKind::Itanium:
            impl->mangler.reset(clang::ItaniumMangleContext::create(context, compiler.getDiagnostics()));
            break;
        case ManglerKind::Microsoft:
            impl->mangler.reset(clang::MicrosoftMangleContext::create(context, compiler.getDiagnostics()));
            break;
    }
}

std::string ClangCodegen::mangled_name(clang::FunctionDecl* funcDecl) {
    auto& mangler = *impl->mangler;
    std::string mangledName;
    llvm::raw_string_ostream ostream(mangledName);
    if (mangler.shouldMangleDeclName(funcDecl)) {
        mangler.mangleName(funcDecl, ostream);
    } else {
        ostream << funcDecl->getName();
    }
    ostream.flush();
    return mangledName;
}

std::string ClangCodegen::mangled_name(FunctionDeclaration* decl) {

    clang::ASTContext &context = impl->compiler.getASTContext();
    auto unit = context.getTranslationUnitDecl();

    clang::QualType returnType = decl->returnType->clang_type(context);
    clang::FunctionProtoType::ExtProtoInfo protoInfo;

    std::vector<clang::QualType> args;
    for(auto& arg : decl->params) {
        args.emplace_back(arg->type->clang_type(context));
    }

    auto funcType = context.getFunctionType(returnType, args, protoInfo);
    clang::DeclarationName declName = context.DeclarationNames.getIdentifier(&context.Idents.get(decl->name));

    clang::FunctionDecl *funcDecl = clang::FunctionDecl::Create(
            context, unit, clang::SourceLocation(),
            clang::SourceLocation(),
            declName,
            funcType, nullptr, clang::SC_Extern
    );

    // setting parameters
    llvm::SmallVector<clang::ParmVarDecl*> params;
    for(auto& param : decl->params) {
        clang::IdentifierInfo &paramId = context.Idents.get(param->name);
        clang::ParmVarDecl *clangParam = clang::ParmVarDecl::Create(
                context, unit, clang::SourceLocation(), clang::SourceLocation(),
                &paramId, param->type->clang_type(context), nullptr, clang::SC_None, nullptr);
        params.emplace_back(clangParam);
    }
    funcDecl->setParams(params);

    return mangled_name(funcDecl);

}

ClangCodegen::~ClangCodegen() = default;

//------------------------------ Clang Support -----------------------------
//--------------------------------------------------------------------------
//--------------------------------------------------------------------------

clang::QualType BaseType::clang_type(clang::ASTContext &gen) {
    throw std::runtime_error("BaseType::clang_type called");
}

clang::QualType IntNType::clang_type(clang::ASTContext &context) {
    return context.getIntTypeForBitwidth(num_bits(), !is_unsigned());
}

clang::QualType BoolType::clang_type(clang::ASTContext &context) {
    return context.BoolTy;
}

clang::QualType DoubleType::clang_type(clang::ASTContext &context) {
    return context.DoubleTy;
}

clang::QualType FloatType::clang_type(clang::ASTContext &context) {
    return context.FloatTy;
}

clang::QualType VoidType::clang_type(clang::ASTContext &context) {
    return context.VoidTy;
}

clang::QualType PointerType::clang_type(clang::ASTContext &context) {
    return context.getPointerType(type->clang_type(context));
}

clang::QualType ReferenceType::clang_type(clang::ASTContext &context) {
    return context.getPointerType(type->clang_type(context));
}

// ------------------------------ C Translation -----------------------------
//--------------------------------------------------------------------------
//--------------------------------------------------------------------------

// Function to convert std::vector<std::string> to char**
void convertToCharPointers(const std::vector<std::string> &args, const char ***begin, const char ***end) {
    // Allocate memory for the array of char pointers
    const char** argv = new const char* [args.size()];
    // Get c_str for each string basically
    for (size_t i = 0; i < args.size(); ++i) {
        argv[i] = args[i].c_str();
    }
    // Set begin and end pointers
    *begin = argv;
    *end = argv + args.size();
}

CTranslator::CTranslator(
    ASTAllocator& allocator,
    bool is64Bit
) : allocator(allocator), is64Bit(is64Bit),
    diags_engine(clang::CompilerInstance::createDiagnostics(new clang::DiagnosticOptions))
{
    init_type_makers();
    init_node_makers();
}

void CTranslator::translate(
        const char** args_begin,
        const char** args_end,
        const char* resources_path
) {
    std::lock_guard guard(translation_mutex);
    //    std::cout << "[TranslateC] Processing " << abs_path << " with resources " << resources_path << " & compiler at "<< exe_path << std::endl;
    ErrorMsg *errorMsg;
    unsigned long errors_len = 0;
    auto unit = ClangLoadFromCommandLine(
            args_begin,
            args_end,
            &errorMsg,
            &errors_len,
            resources_path,
            diags_engine
    );
    if(errors_len > 0) {
        std::cerr << std::to_string(errors_len) << " errors occurred when loading C files" << std::endl;
        unsigned i = 0;
        while (i < errors_len) {
            const auto err = errorMsg + i;
            std::cerr << err->msg_ptr << " at " << err->filename_ptr << ":" << err->line << ":" << err->column << std::endl;
            i++;
        }
    }
    if (!unit) {
        diags_engine->dump();
        diags_engine->Reset();
        return;
    }
    translate(unit);
    // dedupe the nodes
    top_level_dedupe(nodes);
    delete unit;
//    if (!errors.empty()) {
//        std::cerr << std::to_string(errors.size()) << " errors occurred when translating C files" << std::endl;
//    }
//    for (const auto &err : errors) {
//        std::cerr << err.message << std::endl;
//    }
}

void CTranslator::translate(
        const char *exe_path,
        const char *abs_path,
        const char *resources_path
) {
    const char* args[] = { exe_path, abs_path };
    translate(args, args + 2, resources_path);
}

void CTranslator::translate(std::vector<std::string>& args, const char* resources_path) {
    const char **args_begin;
    const char **args_end;
    convertToCharPointers(args, &args_begin, &args_end);
    translate(args_begin, args_end, resources_path);
    delete[] args_begin;
}

CTranslator::~CTranslator() = default;