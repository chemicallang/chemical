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
#include "ast/statements/Import.h"
#include "ast/types/DoubleType.h"
#include "ast/types/LongDoubleType.h"
#include "ast/types/FloatType.h"
#include "ast/types/Float128Type.h"
#include "ast/types/ComplexType.h"
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
#include "ast/values/Int128Value.h"
#include "ast/values/UInt128Value.h"
#include "ast/values/Expression.h"
#include "ast/values/VariableIdentifier.h"
#include "ast/values/IntValue.h"
#include "ast/values/UIntValue.h"
#include "ast/values/ShortValue.h"
#include "ast/values/NumberValue.h"
#include "compiler/ClangCodegen.h"
#include <clang/AST/Decl.h>
#include <clang/AST/Mangle.h>
#include "utils/PathUtils.h"
#include <filesystem>
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
        return new (translator.allocator.allocate<LinkedType>()) LinkedType(std::move(name), found->second, ZERO_LOC);
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
//    if(canon_ptr != ptr) { // reference
//        if(canon_ptr->isRecordType()) {
//            return make_type(&canonical);
//        }
//        const auto other = ptr->getAs<clang::TypedefType>();
//        auto str = type->getAsString();
//        return new (allocator.allocate<LinkedType>()) LinkedType(str, nullptr);
//    }
    const auto typeClass = ptr->getTypeClass();
    switch(typeClass) {
        case clang::Type::Adjusted:
            error("TODO: type with class Adjusted");
            break;
        case clang::Type::Decayed: {
            const auto decayed = ptr->getAs<clang::DecayedType>();
            auto origType = decayed->getOriginalType();
            const auto made = make_type(&origType);
            if(!made) return nullptr;
            return made;
        }
        case clang::Type::ConstantArray:{
            const auto arrayType = llvm::dyn_cast<clang::ConstantArrayType>(ptr);
            auto elemType = arrayType->getElementType();
            const auto element_type = make_type(&elemType);
            if(!element_type) return nullptr;
            return new (allocator.allocate<ArrayType>()) ArrayType(element_type, (int) arrayType->getSize().getLimitedValue(), ZERO_LOC);
        }
        case clang::Type::DependentSizedArray:
            error("TODO: type with class DependentSizedArray");
            break;
        case clang::Type::IncompleteArray:{
            const auto incomplete_arr = llvm::dyn_cast<clang::IncompleteArrayType>(ptr);
            auto elemType = incomplete_arr->getElementType();
            const auto element_type = make_type(&elemType);
            if(!element_type) return nullptr;
            return new (allocator.allocate<ArrayType>()) ArrayType(element_type, -1, ZERO_LOC);
        }
        case clang::Type::VariableArray:
            error("TODO: type with class VariableArray");
            break;
        case clang::Type::Atomic:
            error("TODO: type with class Atomic");
            break;
        case clang::Type::Attributed:{
            const auto attr = ptr->getAs<clang::AttributedType>();
            // TODO handle the attribute kind
            auto equivalentType = attr->getEquivalentType();
            return make_type(&equivalentType);
        }
        case clang::Type::BTFTagAttributed:
            error("TODO: type with class BTFTagAttributed");
            break;
        case clang::Type::BitInt:
            error("TODO: type with class BitInt");
            break;
        case clang::Type::BlockPointer:
            error("TODO: type with class BlockPointer");
            break;
        case clang::Type::Builtin:{
            auto builtIn = static_cast<clang::BuiltinType*>(const_cast<clang::Type*>(ptr));
            const auto type_maker = type_makers[builtIn->getKind()];
            if(type_maker) {
                return type_maker(allocator, builtIn, ZERO_LOC);
            } else {
                error("builtin type maker failed with kind " + std::to_string(builtIn->getKind()) + " with representation " + builtIn->getName(clang::PrintingPolicy{clang::LangOptions{}}).str() + " with actual " + type->getAsString());
                return nullptr;
            }
        }
        case clang::Type::Complex:{
            const auto complexType = ptr->getAs<clang::ComplexType>();
            auto elemType = complexType->getElementType();
            auto element_type = make_type(&elemType);
            return new (allocator.allocate<ComplexType>()) ComplexType(element_type, ZERO_LOC);
        }
        case clang::Type::Decltype:
            error("TODO: type with class Decltype");
            break;
        case clang::Type::Auto:
            error("TODO: type with class Auto");
            break;
        case clang::Type::DeducedTemplateSpecialization:
            error("TODO: type with class DeducedTemplateSpecialization");
            break;
        case clang::Type::DependentAddressSpace:
            error("TODO: type with class DependentAddressSpace");
            break;
        case clang::Type::DependentBitInt:
            error("TODO: type with class DependentBitInt");
            break;
        case clang::Type::DependentName:
            error("TODO: type with class DependentName");
            break;
        case clang::Type::DependentSizedExtVector:
            error("TODO: type with class DependentSizedExtVector");
            break;
        case clang::Type::DependentTemplateSpecialization:
            error("TODO: type with class DependentTemplateSpecialization");
            break;
        case clang::Type::DependentVector:
            error("TODO: type with class DependentVector");
            break;
        case clang::Type::Elaborated:{
            const auto elab = ptr->getAs<clang::ElaboratedType>();
            auto named_type = elab->getNamedType();
            return make_type(&named_type);
        }
        case clang::Type::FunctionNoProto:{
            const auto protoType = ptr->getAs<clang::FunctionNoProtoType>();
            auto retType = protoType->getReturnType();
            const auto returnType = make_type(&retType);
            if(!returnType) return nullptr;
            auto functionType = new (allocator.allocate<FunctionType>()) FunctionType({}, returnType, false, false, nullptr, ZERO_LOC);
            return functionType;
        }
        case clang::Type::FunctionProto:{
            const auto protoType = ptr->getAs<clang::FunctionProtoType>();
            auto retType = protoType->getReturnType();
            const auto returnType = make_type(&retType);
            if(!returnType) return nullptr;
            auto functionType = new (allocator.allocate<FunctionType>()) FunctionType({}, returnType, protoType->isVariadic(), false, nullptr, ZERO_LOC);
            unsigned i = 0;
            for(auto paramType : protoType->getParamTypes()) {
                auto param_type = make_type(&paramType);
                if(!param_type) {
                    return nullptr;
                }
                const auto param = new (allocator.allocate<FunctionParam>()) FunctionParam("", param_type, i, nullptr, false, functionType, ZERO_LOC);
                functionType->params.emplace_back(param);
                i++;
            }
            return functionType;
        }
        case clang::Type::InjectedClassName:
            error("TODO: type with class InjectedClassName");
            break;
        case clang::Type::MacroQualified:
            error("TODO: type with class MacroQualified");
            break;
        case clang::Type::ConstantMatrix:
            error("TODO: type with class ConstantMatrix");
            break;
        case clang::Type::DependentSizedMatrix:
            error("TODO: type with class DependentSizedMatrix");
            break;
        case clang::Type::MemberPointer:
            error("TODO: type with class MemberPointer");
            break;
        case clang::Type::ObjCObjectPointer:
            error("TODO: type with class ObjCObjectPointer");
            break;
        case clang::Type::ObjCObject:
            error("TODO: type with class ObjCObject");
            break;
        case clang::Type::ObjCInterface:
            error("TODO: type with class ObjCInterface");
            break;
        case clang::Type::ObjCTypeParam:
            error("TODO: type with class ObjCTypeParam");
            break;
        case clang::Type::PackExpansion:
            error("TODO: type with class PackExpansion");
            break;
        case clang::Type::Paren:{
            const auto parenType = ptr->getAs<clang::ParenType>();
            auto innerType = parenType->getInnerType();
            return make_type(&innerType);
        }
        case clang::Type::Pipe:
            error("TODO: type with class Pipe");
            break;
        case clang::Type::Pointer: {
            const auto ptrType = ptr->getAs<clang::PointerType>();
            auto point = ptrType->getPointeeType();
            auto pointee = make_type(&point);
            if(!pointee) {
                return nullptr;
            }
            return new (allocator.allocate<PointerType>()) PointerType(pointee, ZERO_LOC, is_mutable);
        }
        case clang::Type::LValueReference:
            error("TODO: type with class LValueReference");
            break;
        case clang::Type::RValueReference:
            error("TODO: type with class RValueReference");
            break;
        case clang::Type::SubstTemplateTypeParmPack:
            error("TODO: type with class SubstTemplateTypeParmPack");
            break;
        case clang::Type::SubstTemplateTypeParm:
            error("TODO: type with class SubstTemplateTypeParm");
            break;
        case clang::Type::Enum:{
            const auto enumType = ptr->getAs<clang::EnumType>();
            const auto decl = enumType->getDecl();
            return decl_type(*this, decl, decl->getNameAsString());
        }
        case clang::Type::Record: {
            const auto record_decl = ptr->getAsRecordDecl();
            return decl_type(*this, record_decl, record_decl->getNameAsString());
        }
        case clang::Type::TemplateSpecialization:
            error("TODO: type with class TemplateSpecialization");
            break;
        case clang::Type::TemplateTypeParm:
            error("TODO: type with class TemplateTypeParm");
            break;
        case clang::Type::TypeOfExpr:
            error("TODO: type with class TypeOfExpr");
            break;
        case clang::Type::TypeOf:
            error("TODO: type with class TypeOf");
            break;
        case clang::Type::Typedef: {
            const auto type_def_type = ptr->getAs<clang::TypedefType>();
            const auto decl = type_def_type->getDecl();
            return decl_type(*this, decl, decl->getNameAsString());
        }
        case clang::Type::UnaryTransform:
            error("TODO: type with class UnaryTransform");
            break;
        case clang::Type::UnresolvedUsing:
            error("TODO: type with class UnresolvedUsing");
            break;
        case clang::Type::Using:
            error("TODO: type with class Using");
            break;
        case clang::Type::Vector:
            error("TODO: type with class Vector");
            break;
        case clang::Type::ExtVector:
            error("TODO: type with class ExtVector");
            break;
    }
    error("unhandled type with representation " + type->getAsString());
    return nullptr;
}

inline Value* convert_to_number(ASTAllocator& alloc, bool is64Bit, unsigned int bitWidth, bool is_signed, uint64_t value, SourceLocation location) {
    return IntNumValue::create_number(alloc, bitWidth, is_signed, value, location);
}

EnumDeclaration* CTranslator::make_enum(clang::EnumDecl* decl) {
    auto enum_decl = new (allocator.allocate<EnumDeclaration>()) EnumDeclaration(decl->getNameAsString(), {}, parent_node, ZERO_LOC);
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
            mem_value = convert_to_number(allocator, is64Bit, bitWidth, init_val.isSigned(), value, ZERO_LOC);
        }
        enum_decl->members[mem->getNameAsString()] = new (allocator.allocate<EnumMember>()) EnumMember(mem->getNameAsString(), index, mem_value, enum_decl, ZERO_LOC);
        index++;
    }
    return enum_decl;
}

StructDefinition* CTranslator::make_struct(clang::RecordDecl* decl) {
    auto def = new (allocator.allocate<StructDefinition>()) StructDefinition({ decl->getNameAsString(), ZERO_LOC }, parent_node, ZERO_LOC);
    for(auto str : decl->fields()) {
        auto field_type = str->getType();
        auto field_type_conv = make_type(&field_type);
        if(!field_type_conv) {
            return nullptr;
        }
        def->variables[str->getNameAsString()] = new (allocator.allocate<StructMember>()) StructMember(str->getNameAsString(), field_type_conv, nullptr, def, ZERO_LOC);
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
                if(node->name().empty()) {
                    node->identifier.identifier = decl->getName();
                    return nullptr;
                }
            }
        }

    }

    return new (allocator.allocate<TypealiasStatement>()) TypealiasStatement({ decl->getNameAsString(), ZERO_LOC }, type, parent_node, ZERO_LOC);
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
        return convert_to_number(allocator, is64Bit, bitWidth, is_signed, real_val, ZERO_LOC);
    } else if (auto* floatLiteral = llvm::dyn_cast<clang::FloatingLiteral>(expr)) {
        // Handle floating-point literals
        llvm::APFloat value = floatLiteral->getValue();
        const clang::BuiltinType* builtinType = floatLiteral->getType()->getAs<clang::BuiltinType>();
        if (builtinType->getKind() == clang::BuiltinType::Float) {
            // Single precision (float)
            auto floatValue = value.convertToFloat();
            return new (allocator.allocate<FloatValue>()) FloatValue(floatValue, ZERO_LOC);
        } else {
            // Double precision
            auto doubleValue = value.convertToDouble();
            return new (allocator.allocate<DoubleValue>()) DoubleValue(doubleValue, ZERO_LOC);
        }
    } else if (auto* boolLiteral = llvm::dyn_cast<clang::CXXBoolLiteralExpr>(expr)) {
        bool value = boolLiteral->getValue();
        return new (allocator.allocate<BoolValue>()) BoolValue(value, ZERO_LOC);
    } else if (auto* declRef = llvm::dyn_cast<clang::DeclRefExpr>(expr)) {
        clang::ValueDecl* decl = declRef->getDecl();
        std::string name = decl->getNameAsString();
        auto found = declarations.find(decl);
        if(found != declarations.end()) {
            const auto id = new (allocator.allocate<VariableIdentifier>()) VariableIdentifier(name, ZERO_LOC, false);
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
                return new (allocator.allocate<Expression>()) Expression(lhs, rhs, opt.value(), is64Bit, ZERO_LOC);
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
    return new (allocator.allocate<VarInitStatement>()) VarInitStatement(false, decl->getNameAsString(), made_type, initial_value, parent_node, ZERO_LOC, specifier);
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
                ZERO_LOC
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
            { func_decl->getNameAsString(), ZERO_LOC },
            std::move(params),
            chem_type,
            func_decl->isVariadic(),
            parent_node,
            ZERO_LOC,
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

std::string get_file_name_for_decl(clang::Decl* decl, clang::SourceManager& SM) {
    clang::SourceLocation loc = decl->getLocation();
    if (loc.isInvalid()) {
        return "";
    }
    // Retrieve the file ID and the file name
    auto decomposed = SM.getDecomposedLoc(loc);
    clang::FileID fileID = decomposed.first;
    const clang::FileEntry* fileEntry = SM.getFileEntryForID(fileID);
    if (!fileEntry) {
        return "";
    }
    return fileEntry->getName().str();
}

std::string part_name_with_no(const std::string& path_str, unsigned int no) {
    std::filesystem::path file_path(path_str);
    auto directory = file_path.parent_path();
    std::string stem = file_path.stem().string(); // Filename without extension
    std::string extension = file_path.extension().string(); // File extension
    // Construct the new filename
    std::string new_filename = stem + "-" + std::to_string(no) + extension;
    // Construct the new path
    return (directory / new_filename).string();
}

bool is_unit_empty(std::vector<ASTNode*>& decls) {
    if(decls.empty()) return true;
    // only imports mus be present in an empty unit
    for(auto decl : decls) {
        const auto kind = decl->kind();
        if(kind != ASTNodeKind::ImportStmt) {
            return false;
        }
    }
    return true;
}

void CTranslator::translate(clang::ASTUnit *unit) {
    // set current unit
    current_unit = unit;
    auto& source_man = unit->getSourceManager();
    const auto check = check_decls_across_invocations;
    // translating declarations
    auto tud = unit->getASTContext().getTranslationUnitDecl();
    if(check) {
        for (auto decl: tud->decls()) {
            translate_checking(decl, source_man);
        }
    } else {
        for (auto decl: tud->decls()) {
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
    clang::DeclarationName declName = context.DeclarationNames.getIdentifier(&context.Idents.get(decl->name()));

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

clang::QualType Float128Type::clang_type(clang::ASTContext &context) {
    return context.Float128Ty;
}

clang::QualType LongDoubleType::clang_type(clang::ASTContext &context) {
    return context.LongDoubleTy;
}

clang::QualType ComplexType::clang_type(clang::ASTContext &context) {
    return context.getComplexType(elem_type->clang_type(context));
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

clang::ASTUnit* CTranslator::get_unit(
        const char** args_begin,
        const char** args_end,
        const char* resources_path
) {
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
            if(err->filename_ptr) {
                std::cerr << err->msg_ptr << " at " << err->filename_ptr << ":" << err->line << ":" << err->column << std::endl;
            } else {
                std::cerr << err->msg_ptr << std::endl;
            }
            i++;
        }
    }
    if (!unit) {
        diags_engine->dump();
        diags_engine->Reset();
        return nullptr;
    }
#ifdef DEBUG
    if (!errors.empty()) {
        std::cerr << std::to_string(errors.size()) << " errors occurred when translating C files" << std::endl;
        for (const auto &err : errors) {
            std::cerr << err.message << std::endl;
        }
    }
#endif
    return unit;
}

clang::ASTUnit* CTranslator::get_unit(
        const char* exe_path,
        const char* abs_path,
        const char* resources_path
) {
    const char* args[] = { exe_path, abs_path };
    return get_unit(args, args + 2, resources_path);
}

Value* convert_token_to_value(ASTAllocator& allocator, const clang::Token& token) {
    switch(token.getKind()) {
        case clang::tok::numeric_constant:{
            const auto data = token.getLiteralData();
            std::string_view view(data, token.getLength());
            return nullptr;
        }
        default:
            return nullptr;
    }
}

Value* convert_to_value(ASTAllocator& allocator, clang::MacroInfo* info) {
    auto tokens = info->tokens();
    if(tokens.size() == 1) {
        return convert_token_to_value(allocator, tokens.front());
    } else {
        // TODO
    }
    return nullptr;
}

void CTranslator::translate(
        const char** args_begin,
        const char** args_end,
        const char* resources_path
) {
    std::lock_guard guard(translation_mutex);
    // we delete the unit instantly (we don't need it)
    const auto unit = get_unit(args_begin, args_end, resources_path);
    auto& PP = unit->getPreprocessor();
    auto& Table = PP.getIdentifierTable();
    for (auto it = Table.begin(); it != Table.end(); ++it) {
        clang::IdentifierInfo *II = it->second;
        if (II->hasMacroDefinition()) {
            clang::MacroInfo *MI = PP.getMacroInfo(II);
            const auto value = convert_to_value(allocator, MI);
//            llvm::outs() << "Macro: " << II->getName() << " = ";
//            for (const auto &Tok : MI->tokens()) {
//                llvm::outs() << Tok.getName() << " ";
//            }
//            llvm::outs() << "\n";
        }
    }
    // actual translation
    translate(unit);
    // dedupe the nodes
    top_level_dedupe(nodes);
    // delete the unit (not needed, we already translated)
    delete unit;
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

clang::ASTUnit* CTranslator::get_unit_for_header(
        const std::string_view& exe_path,
        const std::string_view& header_path,
        const char* resources_path
) {
    std::vector<std::string> args;
    args.emplace_back(exe_path);
    args.emplace_back("-include");
    args.emplace_back(header_path);
    args.emplace_back("-x");
    args.emplace_back("c");
#ifdef WIN32
    args.emplace_back("NUL");
#else
    args.emplace_back("/dev/null");
#endif
    const char **args_begin;
    const char **args_end;
    convertToCharPointers(args, &args_begin, &args_end);
    const auto unit = get_unit(args_begin, args_end, resources_path);
    delete[] args_begin;
    return unit;
}

CTranslator::~CTranslator() = default;