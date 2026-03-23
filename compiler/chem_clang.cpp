// Copyright (c) Chemical Language Foundation 2025.


/*
 * The point of this file is to contain all the Clang C++ API interaction so that:
 * 1. The compile time of other files is kept under control.
 * 2. Provide a C interface to the Clang functions we need for self-hosting purposes.
 * 3. Prevent C++ from infecting the rest of the project.
 */

#if __GNUC__ >= 8
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wclass-memaccess"
#endif

#include <clang/Frontend/ASTUnit.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/AST/APValue.h>
#include <clang/AST/Attr.h>
#include <clang/AST/Expr.h>
#include <clang/AST/RecordLayout.h>

#if __GNUC__ >= 8
#pragma GCC diagnostic pop
#endif

// Detect additions to the enum
void ZigClang_detect_enum_BO(clang::BinaryOperatorKind op) {
    switch (op) {
        case clang::BO_PtrMemD:
        case clang::BO_PtrMemI:
        case clang::BO_Cmp:
        case clang::BO_Mul:
        case clang::BO_Div:
        case clang::BO_Rem:
        case clang::BO_Add:
        case clang::BO_Sub:
        case clang::BO_Shl:
        case clang::BO_Shr:
        case clang::BO_LT:
        case clang::BO_GT:
        case clang::BO_LE:
        case clang::BO_GE:
        case clang::BO_EQ:
        case clang::BO_NE:
        case clang::BO_And:
        case clang::BO_Xor:
        case clang::BO_Or:
        case clang::BO_LAnd:
        case clang::BO_LOr:
        case clang::BO_Assign:
        case clang::BO_Comma:
        case clang::BO_MulAssign:
        case clang::BO_DivAssign:
        case clang::BO_RemAssign:
        case clang::BO_AddAssign:
        case clang::BO_SubAssign:
        case clang::BO_ShlAssign:
        case clang::BO_ShrAssign:
        case clang::BO_AndAssign:
        case clang::BO_XorAssign:
        case clang::BO_OrAssign:
            break;
    }
}

// Detect additions to the enum
void ZigClang_detect_enum_UO(clang::UnaryOperatorKind op) {
    switch (op) {
        case clang::UO_AddrOf:
        case clang::UO_Coawait:
        case clang::UO_Deref:
        case clang::UO_Extension:
        case clang::UO_Imag:
        case clang::UO_LNot:
        case clang::UO_Minus:
        case clang::UO_Not:
        case clang::UO_Plus:
        case clang::UO_PostDec:
        case clang::UO_PostInc:
        case clang::UO_PreDec:
        case clang::UO_PreInc:
        case clang::UO_Real:
            break;
    }
}


// Detect additions to the enum
void ZigClang_detect_enum_CK(clang::CastKind x) {
    switch (x) {
        case clang::CK_ARCConsumeObject:
        case clang::CK_ARCExtendBlockObject:
        case clang::CK_ARCProduceObject:
        case clang::CK_ARCReclaimReturnedObject:
        case clang::CK_AddressSpaceConversion:
        case clang::CK_AnyPointerToBlockPointerCast:
        case clang::CK_ArrayToPointerDecay:
        case clang::CK_AtomicToNonAtomic:
        case clang::CK_BaseToDerived:
        case clang::CK_BaseToDerivedMemberPointer:
        case clang::CK_BitCast:
        case clang::CK_BlockPointerToObjCPointerCast:
        case clang::CK_BooleanToSignedIntegral:
        case clang::CK_BuiltinFnToFnPtr:
        case clang::CK_CPointerToObjCPointerCast:
        case clang::CK_ConstructorConversion:
        case clang::CK_CopyAndAutoreleaseBlockObject:
        case clang::CK_Dependent:
        case clang::CK_DerivedToBase:
        case clang::CK_DerivedToBaseMemberPointer:
        case clang::CK_Dynamic:
        case clang::CK_FixedPointCast:
        case clang::CK_FixedPointToBoolean:
        case clang::CK_FixedPointToFloating:
        case clang::CK_FixedPointToIntegral:
        case clang::CK_FloatingCast:
        case clang::CK_FloatingComplexCast:
        case clang::CK_FloatingComplexToBoolean:
        case clang::CK_FloatingComplexToIntegralComplex:
        case clang::CK_FloatingComplexToReal:
        case clang::CK_FloatingRealToComplex:
        case clang::CK_FloatingToBoolean:
        case clang::CK_FloatingToFixedPoint:
        case clang::CK_FloatingToIntegral:
        case clang::CK_FunctionToPointerDecay:
        case clang::CK_IntToOCLSampler:
        case clang::CK_IntegralCast:
        case clang::CK_IntegralComplexCast:
        case clang::CK_IntegralComplexToBoolean:
        case clang::CK_IntegralComplexToFloatingComplex:
        case clang::CK_IntegralComplexToReal:
        case clang::CK_IntegralRealToComplex:
        case clang::CK_IntegralToBoolean:
        case clang::CK_IntegralToFixedPoint:
        case clang::CK_IntegralToFloating:
        case clang::CK_IntegralToPointer:
        case clang::CK_LValueBitCast:
        case clang::CK_LValueToRValue:
        case clang::CK_LValueToRValueBitCast:
        case clang::CK_MatrixCast:
        case clang::CK_MemberPointerToBoolean:
        case clang::CK_NoOp:
        case clang::CK_NonAtomicToAtomic:
        case clang::CK_NullToMemberPointer:
        case clang::CK_NullToPointer:
        case clang::CK_ObjCObjectLValueCast:
        case clang::CK_PointerToBoolean:
        case clang::CK_PointerToIntegral:
        case clang::CK_ReinterpretMemberPointer:
        case clang::CK_ToUnion:
        case clang::CK_ToVoid:
        case clang::CK_UncheckedDerivedToBase:
        case clang::CK_UserDefinedConversion:
        case clang::CK_VectorSplat:
        case clang::CK_ZeroToOCLOpaqueType:
            break;
    }
};

// Detect additions to the enum
void ZigClang_detect_enum_TypeClass(clang::Type::TypeClass ty) {
    switch (ty) {
        case clang::Type::Builtin:
        case clang::Type::Complex:
        case clang::Type::Pointer:
        case clang::Type::BlockPointer:
        case clang::Type::LValueReference:
        case clang::Type::RValueReference:
        case clang::Type::MemberPointer:
        case clang::Type::ConstantArray:
        case clang::Type::IncompleteArray:
        case clang::Type::VariableArray:
        case clang::Type::DependentSizedArray:
        case clang::Type::DependentSizedExtVector:
        case clang::Type::DependentAddressSpace:
        case clang::Type::DependentBitInt:
        case clang::Type::Vector:
        case clang::Type::DependentVector:
        case clang::Type::ExtVector:
        case clang::Type::FunctionProto:
        case clang::Type::FunctionNoProto:
        case clang::Type::UnresolvedUsing:
        case clang::Type::Using:
        case clang::Type::Paren:
        case clang::Type::Typedef:
        case clang::Type::MacroQualified:
        case clang::Type::ConstantMatrix:
        case clang::Type::DependentSizedMatrix:
        case clang::Type::Adjusted:
        case clang::Type::Decayed:
        case clang::Type::TypeOfExpr:
        case clang::Type::TypeOf:
        case clang::Type::Decltype:
        case clang::Type::UnaryTransform:
        case clang::Type::Record:
        case clang::Type::Enum:
        case clang::Type::Attributed:
        case clang::Type::BTFTagAttributed:
        case clang::Type::BitInt:
        case clang::Type::TemplateTypeParm:
        case clang::Type::SubstTemplateTypeParm:
        case clang::Type::SubstTemplateTypeParmPack:
        case clang::Type::TemplateSpecialization:
        case clang::Type::Auto:
        case clang::Type::DeducedTemplateSpecialization:
        case clang::Type::InjectedClassName:
        case clang::Type::DependentName:
        case clang::Type::PackExpansion:
        case clang::Type::ObjCTypeParam:
        case clang::Type::ObjCObject:
        case clang::Type::ObjCInterface:
        case clang::Type::ObjCObjectPointer:
        case clang::Type::Pipe:
        case clang::Type::Atomic:
            break;
    }
}

// Detect additions to the enum
void ZigClang_detect_enum_StmtClass(clang::Stmt::StmtClass x) {
    switch (x) {
        case clang::Stmt::NoStmtClass:
        case clang::Stmt::GCCAsmStmtClass:
        case clang::Stmt::MSAsmStmtClass:
        case clang::Stmt::BreakStmtClass:
        case clang::Stmt::CXXCatchStmtClass:
        case clang::Stmt::CXXForRangeStmtClass:
        case clang::Stmt::CXXTryStmtClass:
        case clang::Stmt::CapturedStmtClass:
        case clang::Stmt::CompoundStmtClass:
        case clang::Stmt::ContinueStmtClass:
        case clang::Stmt::CoreturnStmtClass:
        case clang::Stmt::CoroutineBodyStmtClass:
        case clang::Stmt::DeclStmtClass:
        case clang::Stmt::DoStmtClass:
        case clang::Stmt::ForStmtClass:
        case clang::Stmt::GotoStmtClass:
        case clang::Stmt::IfStmtClass:
        case clang::Stmt::IndirectGotoStmtClass:
        case clang::Stmt::MSDependentExistsStmtClass:
        case clang::Stmt::NullStmtClass:
        case clang::Stmt::OMPCanonicalLoopClass:
        case clang::Stmt::OMPAtomicDirectiveClass:
        case clang::Stmt::OMPBarrierDirectiveClass:
        case clang::Stmt::OMPCancelDirectiveClass:
        case clang::Stmt::OMPCancellationPointDirectiveClass:
        case clang::Stmt::OMPCriticalDirectiveClass:
        case clang::Stmt::OMPDepobjDirectiveClass:
        case clang::Stmt::OMPDispatchDirectiveClass:
        case clang::Stmt::OMPErrorDirectiveClass:
        case clang::Stmt::OMPFlushDirectiveClass:
        case clang::Stmt::OMPInteropDirectiveClass:
        case clang::Stmt::OMPDistributeDirectiveClass:
        case clang::Stmt::OMPDistributeParallelForDirectiveClass:
        case clang::Stmt::OMPDistributeParallelForSimdDirectiveClass:
        case clang::Stmt::OMPDistributeSimdDirectiveClass:
        case clang::Stmt::OMPForDirectiveClass:
        case clang::Stmt::OMPForSimdDirectiveClass:
        case clang::Stmt::OMPGenericLoopDirectiveClass:
        case clang::Stmt::OMPMaskedTaskLoopDirectiveClass:
        case clang::Stmt::OMPMaskedTaskLoopSimdDirectiveClass:
        case clang::Stmt::OMPMasterTaskLoopDirectiveClass:
        case clang::Stmt::OMPMasterTaskLoopSimdDirectiveClass:
        case clang::Stmt::OMPParallelForDirectiveClass:
        case clang::Stmt::OMPParallelForSimdDirectiveClass:
        case clang::Stmt::OMPParallelGenericLoopDirectiveClass:
        case clang::Stmt::OMPParallelMaskedTaskLoopDirectiveClass:
        case clang::Stmt::OMPParallelMaskedTaskLoopSimdDirectiveClass:
        case clang::Stmt::OMPParallelMasterTaskLoopDirectiveClass:
        case clang::Stmt::OMPParallelMasterTaskLoopSimdDirectiveClass:
        case clang::Stmt::OMPSimdDirectiveClass:
        case clang::Stmt::OMPTargetParallelForSimdDirectiveClass:
        case clang::Stmt::OMPTargetParallelGenericLoopDirectiveClass:
        case clang::Stmt::OMPTargetSimdDirectiveClass:
        case clang::Stmt::OMPTargetTeamsDistributeDirectiveClass:
        case clang::Stmt::OMPTargetTeamsDistributeParallelForDirectiveClass:
        case clang::Stmt::OMPTargetTeamsDistributeParallelForSimdDirectiveClass:
        case clang::Stmt::OMPTargetTeamsDistributeSimdDirectiveClass:
        case clang::Stmt::OMPTargetTeamsGenericLoopDirectiveClass:
        case clang::Stmt::OMPTaskLoopDirectiveClass:
        case clang::Stmt::OMPTaskLoopSimdDirectiveClass:
        case clang::Stmt::OMPTeamsDistributeDirectiveClass:
        case clang::Stmt::OMPTeamsDistributeParallelForDirectiveClass:
        case clang::Stmt::OMPTeamsDistributeParallelForSimdDirectiveClass:
        case clang::Stmt::OMPTeamsDistributeSimdDirectiveClass:
        case clang::Stmt::OMPTeamsGenericLoopDirectiveClass:
        case clang::Stmt::OMPTileDirectiveClass:
        case clang::Stmt::OMPUnrollDirectiveClass:
        case clang::Stmt::OMPMaskedDirectiveClass:
        case clang::Stmt::OMPMasterDirectiveClass:
        case clang::Stmt::OMPMetaDirectiveClass:
        case clang::Stmt::OMPOrderedDirectiveClass:
        case clang::Stmt::OMPParallelDirectiveClass:
        case clang::Stmt::OMPParallelMaskedDirectiveClass:
        case clang::Stmt::OMPParallelMasterDirectiveClass:
        case clang::Stmt::OMPParallelSectionsDirectiveClass:
        case clang::Stmt::OMPScanDirectiveClass:
        case clang::Stmt::OMPSectionDirectiveClass:
        case clang::Stmt::OMPSectionsDirectiveClass:
        case clang::Stmt::OMPSingleDirectiveClass:
        case clang::Stmt::OMPTargetDataDirectiveClass:
        case clang::Stmt::OMPTargetDirectiveClass:
        case clang::Stmt::OMPTargetEnterDataDirectiveClass:
        case clang::Stmt::OMPTargetExitDataDirectiveClass:
        case clang::Stmt::OMPTargetParallelDirectiveClass:
        case clang::Stmt::OMPTargetParallelForDirectiveClass:
        case clang::Stmt::OMPTargetTeamsDirectiveClass:
        case clang::Stmt::OMPTargetUpdateDirectiveClass:
        case clang::Stmt::OMPTaskDirectiveClass:
        case clang::Stmt::OMPTaskgroupDirectiveClass:
        case clang::Stmt::OMPTaskwaitDirectiveClass:
        case clang::Stmt::OMPTaskyieldDirectiveClass:
        case clang::Stmt::OMPTeamsDirectiveClass:
        case clang::Stmt::ObjCAtCatchStmtClass:
        case clang::Stmt::ObjCAtFinallyStmtClass:
        case clang::Stmt::ObjCAtSynchronizedStmtClass:
        case clang::Stmt::ObjCAtThrowStmtClass:
        case clang::Stmt::ObjCAtTryStmtClass:
        case clang::Stmt::ObjCAutoreleasePoolStmtClass:
        case clang::Stmt::ObjCForCollectionStmtClass:
        case clang::Stmt::ReturnStmtClass:
        case clang::Stmt::SEHExceptStmtClass:
        case clang::Stmt::SEHFinallyStmtClass:
        case clang::Stmt::SEHLeaveStmtClass:
        case clang::Stmt::SEHTryStmtClass:
        case clang::Stmt::CaseStmtClass:
        case clang::Stmt::DefaultStmtClass:
        case clang::Stmt::SwitchStmtClass:
        case clang::Stmt::AttributedStmtClass:
        case clang::Stmt::BinaryConditionalOperatorClass:
        case clang::Stmt::ConditionalOperatorClass:
        case clang::Stmt::AddrLabelExprClass:
        case clang::Stmt::ArrayInitIndexExprClass:
        case clang::Stmt::ArrayInitLoopExprClass:
        case clang::Stmt::ArraySubscriptExprClass:
        case clang::Stmt::ArrayTypeTraitExprClass:
        case clang::Stmt::AsTypeExprClass:
        case clang::Stmt::AtomicExprClass:
        case clang::Stmt::BinaryOperatorClass:
        case clang::Stmt::CompoundAssignOperatorClass:
        case clang::Stmt::BlockExprClass:
        case clang::Stmt::CXXBindTemporaryExprClass:
        case clang::Stmt::CXXBoolLiteralExprClass:
        case clang::Stmt::CXXConstructExprClass:
        case clang::Stmt::CXXTemporaryObjectExprClass:
        case clang::Stmt::CXXDefaultArgExprClass:
        case clang::Stmt::CXXDefaultInitExprClass:
        case clang::Stmt::CXXDeleteExprClass:
        case clang::Stmt::CXXDependentScopeMemberExprClass:
        case clang::Stmt::CXXFoldExprClass:
        case clang::Stmt::CXXInheritedCtorInitExprClass:
        case clang::Stmt::CXXNewExprClass:
        case clang::Stmt::CXXNoexceptExprClass:
        case clang::Stmt::CXXNullPtrLiteralExprClass:
        case clang::Stmt::CXXParenListInitExprClass:
        case clang::Stmt::CXXPseudoDestructorExprClass:
        case clang::Stmt::CXXRewrittenBinaryOperatorClass:
        case clang::Stmt::CXXScalarValueInitExprClass:
        case clang::Stmt::CXXStdInitializerListExprClass:
        case clang::Stmt::CXXThisExprClass:
        case clang::Stmt::CXXThrowExprClass:
        case clang::Stmt::CXXTypeidExprClass:
        case clang::Stmt::CXXUnresolvedConstructExprClass:
        case clang::Stmt::CXXUuidofExprClass:
        case clang::Stmt::CallExprClass:
        case clang::Stmt::CUDAKernelCallExprClass:
        case clang::Stmt::CXXMemberCallExprClass:
        case clang::Stmt::CXXOperatorCallExprClass:
        case clang::Stmt::UserDefinedLiteralClass:
        case clang::Stmt::BuiltinBitCastExprClass:
        case clang::Stmt::CStyleCastExprClass:
        case clang::Stmt::CXXFunctionalCastExprClass:
        case clang::Stmt::CXXAddrspaceCastExprClass:
        case clang::Stmt::CXXConstCastExprClass:
        case clang::Stmt::CXXDynamicCastExprClass:
        case clang::Stmt::CXXReinterpretCastExprClass:
        case clang::Stmt::CXXStaticCastExprClass:
        case clang::Stmt::ObjCBridgedCastExprClass:
        case clang::Stmt::ImplicitCastExprClass:
        case clang::Stmt::CharacterLiteralClass:
        case clang::Stmt::ChooseExprClass:
        case clang::Stmt::CompoundLiteralExprClass:
        case clang::Stmt::ConceptSpecializationExprClass:
        case clang::Stmt::ConvertVectorExprClass:
        case clang::Stmt::CoawaitExprClass:
        case clang::Stmt::CoyieldExprClass:
        case clang::Stmt::DeclRefExprClass:
        case clang::Stmt::DependentCoawaitExprClass:
        case clang::Stmt::DependentScopeDeclRefExprClass:
        case clang::Stmt::DesignatedInitExprClass:
        case clang::Stmt::DesignatedInitUpdateExprClass:
        case clang::Stmt::ExpressionTraitExprClass:
        case clang::Stmt::ExtVectorElementExprClass:
        case clang::Stmt::FixedPointLiteralClass:
        case clang::Stmt::FloatingLiteralClass:
        case clang::Stmt::ConstantExprClass:
        case clang::Stmt::ExprWithCleanupsClass:
        case clang::Stmt::FunctionParmPackExprClass:
        case clang::Stmt::GNUNullExprClass:
        case clang::Stmt::GenericSelectionExprClass:
        case clang::Stmt::ImaginaryLiteralClass:
        case clang::Stmt::ImplicitValueInitExprClass:
        case clang::Stmt::InitListExprClass:
        case clang::Stmt::IntegerLiteralClass:
        case clang::Stmt::LambdaExprClass:
        case clang::Stmt::MSPropertyRefExprClass:
        case clang::Stmt::MSPropertySubscriptExprClass:
        case clang::Stmt::MaterializeTemporaryExprClass:
        case clang::Stmt::MatrixSubscriptExprClass:
        case clang::Stmt::MemberExprClass:
        case clang::Stmt::NoInitExprClass:
        case clang::Stmt::OMPArrayShapingExprClass:
        case clang::Stmt::OMPIteratorExprClass:
        case clang::Stmt::ObjCArrayLiteralClass:
        case clang::Stmt::ObjCAvailabilityCheckExprClass:
        case clang::Stmt::ObjCBoolLiteralExprClass:
        case clang::Stmt::ObjCBoxedExprClass:
        case clang::Stmt::ObjCDictionaryLiteralClass:
        case clang::Stmt::ObjCEncodeExprClass:
        case clang::Stmt::ObjCIndirectCopyRestoreExprClass:
        case clang::Stmt::ObjCIsaExprClass:
        case clang::Stmt::ObjCIvarRefExprClass:
        case clang::Stmt::ObjCMessageExprClass:
        case clang::Stmt::ObjCPropertyRefExprClass:
        case clang::Stmt::ObjCProtocolExprClass:
        case clang::Stmt::ObjCSelectorExprClass:
        case clang::Stmt::ObjCStringLiteralClass:
        case clang::Stmt::ObjCSubscriptRefExprClass:
        case clang::Stmt::OffsetOfExprClass:
        case clang::Stmt::OpaqueValueExprClass:
        case clang::Stmt::UnresolvedLookupExprClass:
        case clang::Stmt::UnresolvedMemberExprClass:
        case clang::Stmt::PackExpansionExprClass:
        case clang::Stmt::ParenExprClass:
        case clang::Stmt::ParenListExprClass:
        case clang::Stmt::PredefinedExprClass:
        case clang::Stmt::PseudoObjectExprClass:
        case clang::Stmt::RecoveryExprClass:
        case clang::Stmt::RequiresExprClass:
        case clang::Stmt::SYCLUniqueStableNameExprClass:
        case clang::Stmt::ShuffleVectorExprClass:
        case clang::Stmt::SizeOfPackExprClass:
        case clang::Stmt::SourceLocExprClass:
        case clang::Stmt::StmtExprClass:
        case clang::Stmt::StringLiteralClass:
        case clang::Stmt::SubstNonTypeTemplateParmExprClass:
        case clang::Stmt::SubstNonTypeTemplateParmPackExprClass:
        case clang::Stmt::TypeTraitExprClass:
        case clang::Stmt::UnaryExprOrTypeTraitExprClass:
        case clang::Stmt::UnaryOperatorClass:
        case clang::Stmt::VAArgExprClass:
        case clang::Stmt::LabelStmtClass:
        case clang::Stmt::WhileStmtClass:
            break;
    }
}

void ZigClang_detect_enum_APValueKind(clang::APValue::ValueKind x) {
    switch (x) {
        case clang::APValue::None:
        case clang::APValue::Indeterminate:
        case clang::APValue::Int:
        case clang::APValue::Float:
        case clang::APValue::FixedPoint:
        case clang::APValue::ComplexInt:
        case clang::APValue::ComplexFloat:
        case clang::APValue::LValue:
        case clang::APValue::Vector:
        case clang::APValue::Array:
        case clang::APValue::Struct:
        case clang::APValue::Union:
        case clang::APValue::MemberPointer:
        case clang::APValue::AddrLabelDiff:
            break;
    }
}

void ZigClang_detect_enum_DeclKind(clang::Decl::Kind x) {
    switch (x) {
        case clang::Decl::AccessSpec:
        case clang::Decl::Block:
        case clang::Decl::Captured:
        case clang::Decl::Empty:
        case clang::Decl::Export:
        case clang::Decl::ExternCContext:
        case clang::Decl::FileScopeAsm:
        case clang::Decl::Friend:
        case clang::Decl::FriendTemplate:
        case clang::Decl::ImplicitConceptSpecialization:
        case clang::Decl::Import:
        case clang::Decl::LifetimeExtendedTemporary:
        case clang::Decl::LinkageSpec:
        case clang::Decl::Using:
        case clang::Decl::UsingEnum:
        case clang::Decl::HLSLBuffer:
        case clang::Decl::Label:
        case clang::Decl::Namespace:
        case clang::Decl::NamespaceAlias:
        case clang::Decl::ObjCCompatibleAlias:
        case clang::Decl::ObjCCategory:
        case clang::Decl::ObjCCategoryImpl:
        case clang::Decl::ObjCImplementation:
        case clang::Decl::ObjCInterface:
        case clang::Decl::ObjCProtocol:
        case clang::Decl::ObjCMethod:
        case clang::Decl::ObjCProperty:
        case clang::Decl::BuiltinTemplate:
        case clang::Decl::Concept:
        case clang::Decl::ClassTemplate:
        case clang::Decl::FunctionTemplate:
        case clang::Decl::TypeAliasTemplate:
        case clang::Decl::VarTemplate:
        case clang::Decl::TemplateTemplateParm:
        case clang::Decl::Enum:
        case clang::Decl::Record:
        case clang::Decl::CXXRecord:
        case clang::Decl::ClassTemplateSpecialization:
        case clang::Decl::ClassTemplatePartialSpecialization:
        case clang::Decl::TemplateTypeParm:
        case clang::Decl::ObjCTypeParam:
        case clang::Decl::TypeAlias:
        case clang::Decl::Typedef:
        case clang::Decl::UnresolvedUsingTypename:
        case clang::Decl::UnresolvedUsingIfExists:
        case clang::Decl::UsingDirective:
        case clang::Decl::UsingPack:
        case clang::Decl::UsingShadow:
        case clang::Decl::ConstructorUsingShadow:
        case clang::Decl::Binding:
        case clang::Decl::Field:
        case clang::Decl::ObjCAtDefsField:
        case clang::Decl::ObjCIvar:
        case clang::Decl::Function:
        case clang::Decl::CXXDeductionGuide:
        case clang::Decl::CXXMethod:
        case clang::Decl::CXXConstructor:
        case clang::Decl::CXXConversion:
        case clang::Decl::CXXDestructor:
        case clang::Decl::MSProperty:
        case clang::Decl::NonTypeTemplateParm:
        case clang::Decl::Var:
        case clang::Decl::Decomposition:
        case clang::Decl::ImplicitParam:
        case clang::Decl::OMPCapturedExpr:
        case clang::Decl::ParmVar:
        case clang::Decl::VarTemplateSpecialization:
        case clang::Decl::VarTemplatePartialSpecialization:
        case clang::Decl::EnumConstant:
        case clang::Decl::IndirectField:
        case clang::Decl::MSGuid:
        case clang::Decl::OMPDeclareMapper:
        case clang::Decl::OMPDeclareReduction:
        case clang::Decl::TemplateParamObject:
        case clang::Decl::UnnamedGlobalConstant:
        case clang::Decl::UnresolvedUsingValue:
        case clang::Decl::OMPAllocate:
        case clang::Decl::OMPRequires:
        case clang::Decl::OMPThreadPrivate:
        case clang::Decl::ObjCPropertyImpl:
        case clang::Decl::PragmaComment:
        case clang::Decl::PragmaDetectMismatch:
        case clang::Decl::RequiresExprBody:
        case clang::Decl::StaticAssert:
        case clang::Decl::TopLevelStmt:
        case clang::Decl::TranslationUnit:
            break;
    }
}

void ZigClang_detect_enum_BuiltinTypeKind(clang::BuiltinType::Kind x) {
    switch (x) {
    case clang::BuiltinType::OCLImage1dRO:
        break;
    case clang::BuiltinType::OCLImage1dArrayRO:
        break;
    case clang::BuiltinType::OCLImage1dBufferRO:
        break;
    case clang::BuiltinType::OCLImage2dRO:
        break;
    case clang::BuiltinType::OCLImage2dArrayRO:
        break;
    case clang::BuiltinType::OCLImage2dDepthRO:
        break;
    case clang::BuiltinType::OCLImage2dArrayDepthRO:
        break;
    case clang::BuiltinType::OCLImage2dMSAARO:
        break;
    case clang::BuiltinType::OCLImage2dArrayMSAARO:
        break;
    case clang::BuiltinType::OCLImage2dMSAADepthRO:
        break;
    case clang::BuiltinType::OCLImage2dArrayMSAADepthRO:
        break;
    case clang::BuiltinType::OCLImage3dRO:
        break;
    case clang::BuiltinType::OCLImage1dWO:
        break;
    case clang::BuiltinType::OCLImage1dArrayWO:
        break;
    case clang::BuiltinType::OCLImage1dBufferWO:
        break;
    case clang::BuiltinType::OCLImage2dWO:
        break;
    case clang::BuiltinType::OCLImage2dArrayWO:
        break;
    case clang::BuiltinType::OCLImage2dDepthWO:
        break;
    case clang::BuiltinType::OCLImage2dArrayDepthWO:
        break;
    case clang::BuiltinType::OCLImage2dMSAAWO:
        break;
    case clang::BuiltinType::OCLImage2dArrayMSAAWO:
        break;
    case clang::BuiltinType::OCLImage2dMSAADepthWO:
        break;
    case clang::BuiltinType::OCLImage2dArrayMSAADepthWO:
        break;
    case clang::BuiltinType::OCLImage3dWO:
        break;
    case clang::BuiltinType::OCLImage1dRW:
        break;
    case clang::BuiltinType::OCLImage1dArrayRW:
        break;
    case clang::BuiltinType::OCLImage1dBufferRW:
        break;
    case clang::BuiltinType::OCLImage2dRW:
        break;
    case clang::BuiltinType::OCLImage2dArrayRW:
        break;
    case clang::BuiltinType::OCLImage2dDepthRW:
        break;
    case clang::BuiltinType::OCLImage2dArrayDepthRW:
        break;
    case clang::BuiltinType::OCLImage2dMSAARW:
        break;
    case clang::BuiltinType::OCLImage2dArrayMSAARW:
        break;
    case clang::BuiltinType::OCLImage2dMSAADepthRW:
        break;
    case clang::BuiltinType::OCLImage2dArrayMSAADepthRW:
        break;
    case clang::BuiltinType::OCLImage3dRW:
        break;
    case clang::BuiltinType::OCLIntelSubgroupAVCMcePayload:
        break;
    case clang::BuiltinType::OCLIntelSubgroupAVCImePayload:
        break;
    case clang::BuiltinType::OCLIntelSubgroupAVCRefPayload:
        break;
    case clang::BuiltinType::OCLIntelSubgroupAVCSicPayload:
        break;
    case clang::BuiltinType::OCLIntelSubgroupAVCMceResult:
        break;
    case clang::BuiltinType::OCLIntelSubgroupAVCImeResult:
        break;
    case clang::BuiltinType::OCLIntelSubgroupAVCRefResult:
        break;
    case clang::BuiltinType::OCLIntelSubgroupAVCSicResult:
        break;
    case clang::BuiltinType::OCLIntelSubgroupAVCImeResultSingleReferenceStreamout:
        break;
    case clang::BuiltinType::OCLIntelSubgroupAVCImeResultDualReferenceStreamout:
        break;
    case clang::BuiltinType::OCLIntelSubgroupAVCImeSingleReferenceStreamin:
        break;
    case clang::BuiltinType::OCLIntelSubgroupAVCImeDualReferenceStreamin:
        break;
    case clang::BuiltinType::SveInt8:
        break;
    case clang::BuiltinType::SveInt16:
        break;
    case clang::BuiltinType::SveInt32:
        break;
    case clang::BuiltinType::SveInt64:
        break;
    case clang::BuiltinType::SveUint8:
        break;
    case clang::BuiltinType::SveUint16:
        break;
    case clang::BuiltinType::SveUint32:
        break;
    case clang::BuiltinType::SveUint64:
        break;
    case clang::BuiltinType::SveFloat16:
        break;
    case clang::BuiltinType::SveFloat32:
        break;
    case clang::BuiltinType::SveFloat64:
        break;
    case clang::BuiltinType::SveBFloat16:
        break;
    case clang::BuiltinType::SveMFloat8:
        break;
    case clang::BuiltinType::SveInt8x2:
        break;
    case clang::BuiltinType::SveInt16x2:
        break;
    case clang::BuiltinType::SveInt32x2:
        break;
    case clang::BuiltinType::SveInt64x2:
        break;
    case clang::BuiltinType::SveUint8x2:
        break;
    case clang::BuiltinType::SveUint16x2:
        break;
    case clang::BuiltinType::SveUint32x2:
        break;
    case clang::BuiltinType::SveUint64x2:
        break;
    case clang::BuiltinType::SveFloat16x2:
        break;
    case clang::BuiltinType::SveFloat32x2:
        break;
    case clang::BuiltinType::SveFloat64x2:
        break;
    case clang::BuiltinType::SveBFloat16x2:
        break;
    case clang::BuiltinType::SveMFloat8x2:
        break;
    case clang::BuiltinType::SveInt8x3:
        break;
    case clang::BuiltinType::SveInt16x3:
        break;
    case clang::BuiltinType::SveInt32x3:
        break;
    case clang::BuiltinType::SveInt64x3:
        break;
    case clang::BuiltinType::SveUint8x3:
        break;
    case clang::BuiltinType::SveUint16x3:
        break;
    case clang::BuiltinType::SveUint32x3:
        break;
    case clang::BuiltinType::SveUint64x3:
        break;
    case clang::BuiltinType::SveFloat16x3:
        break;
    case clang::BuiltinType::SveFloat32x3:
        break;
    case clang::BuiltinType::SveFloat64x3:
        break;
    case clang::BuiltinType::SveBFloat16x3:
        break;
    case clang::BuiltinType::SveMFloat8x3:
        break;
    case clang::BuiltinType::SveInt8x4:
        break;
    case clang::BuiltinType::SveInt16x4:
        break;
    case clang::BuiltinType::SveInt32x4:
        break;
    case clang::BuiltinType::SveInt64x4:
        break;
    case clang::BuiltinType::SveUint8x4:
        break;
    case clang::BuiltinType::SveUint16x4:
        break;
    case clang::BuiltinType::SveUint32x4:
        break;
    case clang::BuiltinType::SveUint64x4:
        break;
    case clang::BuiltinType::SveFloat16x4:
        break;
    case clang::BuiltinType::SveFloat32x4:
        break;
    case clang::BuiltinType::SveFloat64x4:
        break;
    case clang::BuiltinType::SveBFloat16x4:
        break;
    case clang::BuiltinType::SveMFloat8x4:
        break;
    case clang::BuiltinType::SveBool:
        break;
    case clang::BuiltinType::SveBoolx2:
        break;
    case clang::BuiltinType::SveBoolx4:
        break;
    case clang::BuiltinType::SveCount:
        break;
    case clang::BuiltinType::MFloat8:
        break;
    case clang::BuiltinType::DMR1024:
        break;
    case clang::BuiltinType::VectorQuad:
        break;
    case clang::BuiltinType::VectorPair:
        break;
    case clang::BuiltinType::RvvInt8mf8:
        break;
    case clang::BuiltinType::RvvInt8mf4:
        break;
    case clang::BuiltinType::RvvInt8mf2:
        break;
    case clang::BuiltinType::RvvInt8m1:
        break;
    case clang::BuiltinType::RvvInt8m2:
        break;
    case clang::BuiltinType::RvvInt8m4:
        break;
    case clang::BuiltinType::RvvInt8m8:
        break;
    case clang::BuiltinType::RvvUint8mf8:
        break;
    case clang::BuiltinType::RvvUint8mf4:
        break;
    case clang::BuiltinType::RvvUint8mf2:
        break;
    case clang::BuiltinType::RvvUint8m1:
        break;
    case clang::BuiltinType::RvvUint8m2:
        break;
    case clang::BuiltinType::RvvUint8m4:
        break;
    case clang::BuiltinType::RvvUint8m8:
        break;
    case clang::BuiltinType::RvvInt16mf4:
        break;
    case clang::BuiltinType::RvvInt16mf2:
        break;
    case clang::BuiltinType::RvvInt16m1:
        break;
    case clang::BuiltinType::RvvInt16m2:
        break;
    case clang::BuiltinType::RvvInt16m4:
        break;
    case clang::BuiltinType::RvvInt16m8:
        break;
    case clang::BuiltinType::RvvUint16mf4:
        break;
    case clang::BuiltinType::RvvUint16mf2:
        break;
    case clang::BuiltinType::RvvUint16m1:
        break;
    case clang::BuiltinType::RvvUint16m2:
        break;
    case clang::BuiltinType::RvvUint16m4:
        break;
    case clang::BuiltinType::RvvUint16m8:
        break;
    case clang::BuiltinType::RvvInt32mf2:
        break;
    case clang::BuiltinType::RvvInt32m1:
        break;
    case clang::BuiltinType::RvvInt32m2:
        break;
    case clang::BuiltinType::RvvInt32m4:
        break;
    case clang::BuiltinType::RvvInt32m8:
        break;
    case clang::BuiltinType::RvvUint32mf2:
        break;
    case clang::BuiltinType::RvvUint32m1:
        break;
    case clang::BuiltinType::RvvUint32m2:
        break;
    case clang::BuiltinType::RvvUint32m4:
        break;
    case clang::BuiltinType::RvvUint32m8:
        break;
    case clang::BuiltinType::RvvInt64m1:
        break;
    case clang::BuiltinType::RvvInt64m2:
        break;
    case clang::BuiltinType::RvvInt64m4:
        break;
    case clang::BuiltinType::RvvInt64m8:
        break;
    case clang::BuiltinType::RvvUint64m1:
        break;
    case clang::BuiltinType::RvvUint64m2:
        break;
    case clang::BuiltinType::RvvUint64m4:
        break;
    case clang::BuiltinType::RvvUint64m8:
        break;
    case clang::BuiltinType::RvvFloat16mf4:
        break;
    case clang::BuiltinType::RvvFloat16mf2:
        break;
    case clang::BuiltinType::RvvFloat16m1:
        break;
    case clang::BuiltinType::RvvFloat16m2:
        break;
    case clang::BuiltinType::RvvFloat16m4:
        break;
    case clang::BuiltinType::RvvFloat16m8:
        break;
    case clang::BuiltinType::RvvBFloat16mf4:
        break;
    case clang::BuiltinType::RvvBFloat16mf2:
        break;
    case clang::BuiltinType::RvvBFloat16m1:
        break;
    case clang::BuiltinType::RvvBFloat16m2:
        break;
    case clang::BuiltinType::RvvBFloat16m4:
        break;
    case clang::BuiltinType::RvvBFloat16m8:
        break;
    case clang::BuiltinType::RvvFloat32mf2:
        break;
    case clang::BuiltinType::RvvFloat32m1:
        break;
    case clang::BuiltinType::RvvFloat32m2:
        break;
    case clang::BuiltinType::RvvFloat32m4:
        break;
    case clang::BuiltinType::RvvFloat32m8:
        break;
    case clang::BuiltinType::RvvFloat64m1:
        break;
    case clang::BuiltinType::RvvFloat64m2:
        break;
    case clang::BuiltinType::RvvFloat64m4:
        break;
    case clang::BuiltinType::RvvFloat64m8:
        break;
    case clang::BuiltinType::RvvBool1:
        break;
    case clang::BuiltinType::RvvBool2:
        break;
    case clang::BuiltinType::RvvBool4:
        break;
    case clang::BuiltinType::RvvBool8:
        break;
    case clang::BuiltinType::RvvBool16:
        break;
    case clang::BuiltinType::RvvBool32:
        break;
    case clang::BuiltinType::RvvBool64:
        break;
    case clang::BuiltinType::RvvInt8mf8x2:
        break;
    case clang::BuiltinType::RvvInt8mf8x3:
        break;
    case clang::BuiltinType::RvvInt8mf8x4:
        break;
    case clang::BuiltinType::RvvInt8mf8x5:
        break;
    case clang::BuiltinType::RvvInt8mf8x6:
        break;
    case clang::BuiltinType::RvvInt8mf8x7:
        break;
    case clang::BuiltinType::RvvInt8mf8x8:
        break;
    case clang::BuiltinType::RvvInt8mf4x2:
        break;
    case clang::BuiltinType::RvvInt8mf4x3:
        break;
    case clang::BuiltinType::RvvInt8mf4x4:
        break;
    case clang::BuiltinType::RvvInt8mf4x5:
        break;
    case clang::BuiltinType::RvvInt8mf4x6:
        break;
    case clang::BuiltinType::RvvInt8mf4x7:
        break;
    case clang::BuiltinType::RvvInt8mf4x8:
        break;
    case clang::BuiltinType::RvvInt8mf2x2:
        break;
    case clang::BuiltinType::RvvInt8mf2x3:
        break;
    case clang::BuiltinType::RvvInt8mf2x4:
        break;
    case clang::BuiltinType::RvvInt8mf2x5:
        break;
    case clang::BuiltinType::RvvInt8mf2x6:
        break;
    case clang::BuiltinType::RvvInt8mf2x7:
        break;
    case clang::BuiltinType::RvvInt8mf2x8:
        break;
    case clang::BuiltinType::RvvInt8m1x2:
        break;
    case clang::BuiltinType::RvvInt8m1x3:
        break;
    case clang::BuiltinType::RvvInt8m1x4:
        break;
    case clang::BuiltinType::RvvInt8m1x5:
        break;
    case clang::BuiltinType::RvvInt8m1x6:
        break;
    case clang::BuiltinType::RvvInt8m1x7:
        break;
    case clang::BuiltinType::RvvInt8m1x8:
        break;
    case clang::BuiltinType::RvvInt8m2x2:
        break;
    case clang::BuiltinType::RvvInt8m2x3:
        break;
    case clang::BuiltinType::RvvInt8m2x4:
        break;
    case clang::BuiltinType::RvvInt8m4x2:
        break;
    case clang::BuiltinType::RvvUint8mf8x2:
        break;
    case clang::BuiltinType::RvvUint8mf8x3:
        break;
    case clang::BuiltinType::RvvUint8mf8x4:
        break;
    case clang::BuiltinType::RvvUint8mf8x5:
        break;
    case clang::BuiltinType::RvvUint8mf8x6:
        break;
    case clang::BuiltinType::RvvUint8mf8x7:
        break;
    case clang::BuiltinType::RvvUint8mf8x8:
        break;
    case clang::BuiltinType::RvvUint8mf4x2:
        break;
    case clang::BuiltinType::RvvUint8mf4x3:
        break;
    case clang::BuiltinType::RvvUint8mf4x4:
        break;
    case clang::BuiltinType::RvvUint8mf4x5:
        break;
    case clang::BuiltinType::RvvUint8mf4x6:
        break;
    case clang::BuiltinType::RvvUint8mf4x7:
        break;
    case clang::BuiltinType::RvvUint8mf4x8:
        break;
    case clang::BuiltinType::RvvUint8mf2x2:
        break;
    case clang::BuiltinType::RvvUint8mf2x3:
        break;
    case clang::BuiltinType::RvvUint8mf2x4:
        break;
    case clang::BuiltinType::RvvUint8mf2x5:
        break;
    case clang::BuiltinType::RvvUint8mf2x6:
        break;
    case clang::BuiltinType::RvvUint8mf2x7:
        break;
    case clang::BuiltinType::RvvUint8mf2x8:
        break;
    case clang::BuiltinType::RvvUint8m1x2:
        break;
    case clang::BuiltinType::RvvUint8m1x3:
        break;
    case clang::BuiltinType::RvvUint8m1x4:
        break;
    case clang::BuiltinType::RvvUint8m1x5:
        break;
    case clang::BuiltinType::RvvUint8m1x6:
        break;
    case clang::BuiltinType::RvvUint8m1x7:
        break;
    case clang::BuiltinType::RvvUint8m1x8:
        break;
    case clang::BuiltinType::RvvUint8m2x2:
        break;
    case clang::BuiltinType::RvvUint8m2x3:
        break;
    case clang::BuiltinType::RvvUint8m2x4:
        break;
    case clang::BuiltinType::RvvUint8m4x2:
        break;
    case clang::BuiltinType::RvvInt16mf4x2:
        break;
    case clang::BuiltinType::RvvInt16mf4x3:
        break;
    case clang::BuiltinType::RvvInt16mf4x4:
        break;
    case clang::BuiltinType::RvvInt16mf4x5:
        break;
    case clang::BuiltinType::RvvInt16mf4x6:
        break;
    case clang::BuiltinType::RvvInt16mf4x7:
        break;
    case clang::BuiltinType::RvvInt16mf4x8:
        break;
    case clang::BuiltinType::RvvInt16mf2x2:
        break;
    case clang::BuiltinType::RvvInt16mf2x3:
        break;
    case clang::BuiltinType::RvvInt16mf2x4:
        break;
    case clang::BuiltinType::RvvInt16mf2x5:
        break;
    case clang::BuiltinType::RvvInt16mf2x6:
        break;
    case clang::BuiltinType::RvvInt16mf2x7:
        break;
    case clang::BuiltinType::RvvInt16mf2x8:
        break;
    case clang::BuiltinType::RvvInt16m1x2:
        break;
    case clang::BuiltinType::RvvInt16m1x3:
        break;
    case clang::BuiltinType::RvvInt16m1x4:
        break;
    case clang::BuiltinType::RvvInt16m1x5:
        break;
    case clang::BuiltinType::RvvInt16m1x6:
        break;
    case clang::BuiltinType::RvvInt16m1x7:
        break;
    case clang::BuiltinType::RvvInt16m1x8:
        break;
    case clang::BuiltinType::RvvInt16m2x2:
        break;
    case clang::BuiltinType::RvvInt16m2x3:
        break;
    case clang::BuiltinType::RvvInt16m2x4:
        break;
    case clang::BuiltinType::RvvInt16m4x2:
        break;
    case clang::BuiltinType::RvvUint16mf4x2:
        break;
    case clang::BuiltinType::RvvUint16mf4x3:
        break;
    case clang::BuiltinType::RvvUint16mf4x4:
        break;
    case clang::BuiltinType::RvvUint16mf4x5:
        break;
    case clang::BuiltinType::RvvUint16mf4x6:
        break;
    case clang::BuiltinType::RvvUint16mf4x7:
        break;
    case clang::BuiltinType::RvvUint16mf4x8:
        break;
    case clang::BuiltinType::RvvUint16mf2x2:
        break;
    case clang::BuiltinType::RvvUint16mf2x3:
        break;
    case clang::BuiltinType::RvvUint16mf2x4:
        break;
    case clang::BuiltinType::RvvUint16mf2x5:
        break;
    case clang::BuiltinType::RvvUint16mf2x6:
        break;
    case clang::BuiltinType::RvvUint16mf2x7:
        break;
    case clang::BuiltinType::RvvUint16mf2x8:
        break;
    case clang::BuiltinType::RvvUint16m1x2:
        break;
    case clang::BuiltinType::RvvUint16m1x3:
        break;
    case clang::BuiltinType::RvvUint16m1x4:
        break;
    case clang::BuiltinType::RvvUint16m1x5:
        break;
    case clang::BuiltinType::RvvUint16m1x6:
        break;
    case clang::BuiltinType::RvvUint16m1x7:
        break;
    case clang::BuiltinType::RvvUint16m1x8:
        break;
    case clang::BuiltinType::RvvUint16m2x2:
        break;
    case clang::BuiltinType::RvvUint16m2x3:
        break;
    case clang::BuiltinType::RvvUint16m2x4:
        break;
    case clang::BuiltinType::RvvUint16m4x2:
        break;
    case clang::BuiltinType::RvvInt32mf2x2:
        break;
    case clang::BuiltinType::RvvInt32mf2x3:
        break;
    case clang::BuiltinType::RvvInt32mf2x4:
        break;
    case clang::BuiltinType::RvvInt32mf2x5:
        break;
    case clang::BuiltinType::RvvInt32mf2x6:
        break;
    case clang::BuiltinType::RvvInt32mf2x7:
        break;
    case clang::BuiltinType::RvvInt32mf2x8:
        break;
    case clang::BuiltinType::RvvInt32m1x2:
        break;
    case clang::BuiltinType::RvvInt32m1x3:
        break;
    case clang::BuiltinType::RvvInt32m1x4:
        break;
    case clang::BuiltinType::RvvInt32m1x5:
        break;
    case clang::BuiltinType::RvvInt32m1x6:
        break;
    case clang::BuiltinType::RvvInt32m1x7:
        break;
    case clang::BuiltinType::RvvInt32m1x8:
        break;
    case clang::BuiltinType::RvvInt32m2x2:
        break;
    case clang::BuiltinType::RvvInt32m2x3:
        break;
    case clang::BuiltinType::RvvInt32m2x4:
        break;
    case clang::BuiltinType::RvvInt32m4x2:
        break;
    case clang::BuiltinType::RvvUint32mf2x2:
        break;
    case clang::BuiltinType::RvvUint32mf2x3:
        break;
    case clang::BuiltinType::RvvUint32mf2x4:
        break;
    case clang::BuiltinType::RvvUint32mf2x5:
        break;
    case clang::BuiltinType::RvvUint32mf2x6:
        break;
    case clang::BuiltinType::RvvUint32mf2x7:
        break;
    case clang::BuiltinType::RvvUint32mf2x8:
        break;
    case clang::BuiltinType::RvvUint32m1x2:
        break;
    case clang::BuiltinType::RvvUint32m1x3:
        break;
    case clang::BuiltinType::RvvUint32m1x4:
        break;
    case clang::BuiltinType::RvvUint32m1x5:
        break;
    case clang::BuiltinType::RvvUint32m1x6:
        break;
    case clang::BuiltinType::RvvUint32m1x7:
        break;
    case clang::BuiltinType::RvvUint32m1x8:
        break;
    case clang::BuiltinType::RvvUint32m2x2:
        break;
    case clang::BuiltinType::RvvUint32m2x3:
        break;
    case clang::BuiltinType::RvvUint32m2x4:
        break;
    case clang::BuiltinType::RvvUint32m4x2:
        break;
    case clang::BuiltinType::RvvInt64m1x2:
        break;
    case clang::BuiltinType::RvvInt64m1x3:
        break;
    case clang::BuiltinType::RvvInt64m1x4:
        break;
    case clang::BuiltinType::RvvInt64m1x5:
        break;
    case clang::BuiltinType::RvvInt64m1x6:
        break;
    case clang::BuiltinType::RvvInt64m1x7:
        break;
    case clang::BuiltinType::RvvInt64m1x8:
        break;
    case clang::BuiltinType::RvvInt64m2x2:
        break;
    case clang::BuiltinType::RvvInt64m2x3:
        break;
    case clang::BuiltinType::RvvInt64m2x4:
        break;
    case clang::BuiltinType::RvvInt64m4x2:
        break;
    case clang::BuiltinType::RvvUint64m1x2:
        break;
    case clang::BuiltinType::RvvUint64m1x3:
        break;
    case clang::BuiltinType::RvvUint64m1x4:
        break;
    case clang::BuiltinType::RvvUint64m1x5:
        break;
    case clang::BuiltinType::RvvUint64m1x6:
        break;
    case clang::BuiltinType::RvvUint64m1x7:
        break;
    case clang::BuiltinType::RvvUint64m1x8:
        break;
    case clang::BuiltinType::RvvUint64m2x2:
        break;
    case clang::BuiltinType::RvvUint64m2x3:
        break;
    case clang::BuiltinType::RvvUint64m2x4:
        break;
    case clang::BuiltinType::RvvUint64m4x2:
        break;
    case clang::BuiltinType::RvvFloat16mf4x2:
        break;
    case clang::BuiltinType::RvvFloat16mf4x3:
        break;
    case clang::BuiltinType::RvvFloat16mf4x4:
        break;
    case clang::BuiltinType::RvvFloat16mf4x5:
        break;
    case clang::BuiltinType::RvvFloat16mf4x6:
        break;
    case clang::BuiltinType::RvvFloat16mf4x7:
        break;
    case clang::BuiltinType::RvvFloat16mf4x8:
        break;
    case clang::BuiltinType::RvvFloat16mf2x2:
        break;
    case clang::BuiltinType::RvvFloat16mf2x3:
        break;
    case clang::BuiltinType::RvvFloat16mf2x4:
        break;
    case clang::BuiltinType::RvvFloat16mf2x5:
        break;
    case clang::BuiltinType::RvvFloat16mf2x6:
        break;
    case clang::BuiltinType::RvvFloat16mf2x7:
        break;
    case clang::BuiltinType::RvvFloat16mf2x8:
        break;
    case clang::BuiltinType::RvvFloat16m1x2:
        break;
    case clang::BuiltinType::RvvFloat16m1x3:
        break;
    case clang::BuiltinType::RvvFloat16m1x4:
        break;
    case clang::BuiltinType::RvvFloat16m1x5:
        break;
    case clang::BuiltinType::RvvFloat16m1x6:
        break;
    case clang::BuiltinType::RvvFloat16m1x7:
        break;
    case clang::BuiltinType::RvvFloat16m1x8:
        break;
    case clang::BuiltinType::RvvFloat16m2x2:
        break;
    case clang::BuiltinType::RvvFloat16m2x3:
        break;
    case clang::BuiltinType::RvvFloat16m2x4:
        break;
    case clang::BuiltinType::RvvFloat16m4x2:
        break;
    case clang::BuiltinType::RvvFloat32mf2x2:
        break;
    case clang::BuiltinType::RvvFloat32mf2x3:
        break;
    case clang::BuiltinType::RvvFloat32mf2x4:
        break;
    case clang::BuiltinType::RvvFloat32mf2x5:
        break;
    case clang::BuiltinType::RvvFloat32mf2x6:
        break;
    case clang::BuiltinType::RvvFloat32mf2x7:
        break;
    case clang::BuiltinType::RvvFloat32mf2x8:
        break;
    case clang::BuiltinType::RvvFloat32m1x2:
        break;
    case clang::BuiltinType::RvvFloat32m1x3:
        break;
    case clang::BuiltinType::RvvFloat32m1x4:
        break;
    case clang::BuiltinType::RvvFloat32m1x5:
        break;
    case clang::BuiltinType::RvvFloat32m1x6:
        break;
    case clang::BuiltinType::RvvFloat32m1x7:
        break;
    case clang::BuiltinType::RvvFloat32m1x8:
        break;
    case clang::BuiltinType::RvvFloat32m2x2:
        break;
    case clang::BuiltinType::RvvFloat32m2x3:
        break;
    case clang::BuiltinType::RvvFloat32m2x4:
        break;
    case clang::BuiltinType::RvvFloat32m4x2:
        break;
    case clang::BuiltinType::RvvFloat64m1x2:
        break;
    case clang::BuiltinType::RvvFloat64m1x3:
        break;
    case clang::BuiltinType::RvvFloat64m1x4:
        break;
    case clang::BuiltinType::RvvFloat64m1x5:
        break;
    case clang::BuiltinType::RvvFloat64m1x6:
        break;
    case clang::BuiltinType::RvvFloat64m1x7:
        break;
    case clang::BuiltinType::RvvFloat64m1x8:
        break;
    case clang::BuiltinType::RvvFloat64m2x2:
        break;
    case clang::BuiltinType::RvvFloat64m2x3:
        break;
    case clang::BuiltinType::RvvFloat64m2x4:
        break;
    case clang::BuiltinType::RvvFloat64m4x2:
        break;
    case clang::BuiltinType::RvvBFloat16mf4x2:
        break;
    case clang::BuiltinType::RvvBFloat16mf4x3:
        break;
    case clang::BuiltinType::RvvBFloat16mf4x4:
        break;
    case clang::BuiltinType::RvvBFloat16mf4x5:
        break;
    case clang::BuiltinType::RvvBFloat16mf4x6:
        break;
    case clang::BuiltinType::RvvBFloat16mf4x7:
        break;
    case clang::BuiltinType::RvvBFloat16mf4x8:
        break;
    case clang::BuiltinType::RvvBFloat16mf2x2:
        break;
    case clang::BuiltinType::RvvBFloat16mf2x3:
        break;
    case clang::BuiltinType::RvvBFloat16mf2x4:
        break;
    case clang::BuiltinType::RvvBFloat16mf2x5:
        break;
    case clang::BuiltinType::RvvBFloat16mf2x6:
        break;
    case clang::BuiltinType::RvvBFloat16mf2x7:
        break;
    case clang::BuiltinType::RvvBFloat16mf2x8:
        break;
    case clang::BuiltinType::RvvBFloat16m1x2:
        break;
    case clang::BuiltinType::RvvBFloat16m1x3:
        break;
    case clang::BuiltinType::RvvBFloat16m1x4:
        break;
    case clang::BuiltinType::RvvBFloat16m1x5:
        break;
    case clang::BuiltinType::RvvBFloat16m1x6:
        break;
    case clang::BuiltinType::RvvBFloat16m1x7:
        break;
    case clang::BuiltinType::RvvBFloat16m1x8:
        break;
    case clang::BuiltinType::RvvBFloat16m2x2:
        break;
    case clang::BuiltinType::RvvBFloat16m2x3:
        break;
    case clang::BuiltinType::RvvBFloat16m2x4:
        break;
    case clang::BuiltinType::RvvBFloat16m4x2:
        break;
    case clang::BuiltinType::WasmExternRef:
        break;
    case clang::BuiltinType::AMDGPUBufferRsrc:
        break;
    case clang::BuiltinType::AMDGPUNamedWorkgroupBarrier:
        break;
    case clang::BuiltinType::HLSLResource:
        break;
    case clang::BuiltinType::Void:
        break;
    case clang::BuiltinType::Bool:
        break;
    case clang::BuiltinType::Char_U:
        break;
    case clang::BuiltinType::UChar:
        break;
    case clang::BuiltinType::WChar_U:
        break;
    case clang::BuiltinType::Char8:
        break;
    case clang::BuiltinType::Char16:
        break;
    case clang::BuiltinType::Char32:
        break;
    case clang::BuiltinType::UShort:
        break;
    case clang::BuiltinType::UInt:
        break;
    case clang::BuiltinType::ULong:
        break;
    case clang::BuiltinType::ULongLong:
        break;
    case clang::BuiltinType::UInt128:
        break;
    case clang::BuiltinType::Char_S:
        break;
    case clang::BuiltinType::SChar:
        break;
    case clang::BuiltinType::WChar_S:
        break;
    case clang::BuiltinType::Short:
        break;
    case clang::BuiltinType::Int:
        break;
    case clang::BuiltinType::Long:
        break;
    case clang::BuiltinType::LongLong:
        break;
    case clang::BuiltinType::Int128:
        break;
    case clang::BuiltinType::ShortAccum:
        break;
    case clang::BuiltinType::Accum:
        break;
    case clang::BuiltinType::LongAccum:
        break;
    case clang::BuiltinType::UShortAccum:
        break;
    case clang::BuiltinType::UAccum:
        break;
    case clang::BuiltinType::ULongAccum:
        break;
    case clang::BuiltinType::ShortFract:
        break;
    case clang::BuiltinType::Fract:
        break;
    case clang::BuiltinType::LongFract:
        break;
    case clang::BuiltinType::UShortFract:
        break;
    case clang::BuiltinType::UFract:
        break;
    case clang::BuiltinType::ULongFract:
        break;
    case clang::BuiltinType::SatShortAccum:
        break;
    case clang::BuiltinType::SatAccum:
        break;
    case clang::BuiltinType::SatLongAccum:
        break;
    case clang::BuiltinType::SatUShortAccum:
        break;
    case clang::BuiltinType::SatUAccum:
        break;
    case clang::BuiltinType::SatULongAccum:
        break;
    case clang::BuiltinType::SatShortFract:
        break;
    case clang::BuiltinType::SatFract:
        break;
    case clang::BuiltinType::SatLongFract:
        break;
    case clang::BuiltinType::SatUShortFract:
        break;
    case clang::BuiltinType::SatUFract:
        break;
    case clang::BuiltinType::SatULongFract:
        break;
    case clang::BuiltinType::Half:
        break;
    case clang::BuiltinType::Float:
        break;
    case clang::BuiltinType::Double:
        break;
    case clang::BuiltinType::LongDouble:
        break;
    case clang::BuiltinType::Float16:
        break;
    case clang::BuiltinType::BFloat16:
        break;
    case clang::BuiltinType::Float128:
        break;
    case clang::BuiltinType::Ibm128:
        break;
    case clang::BuiltinType::NullPtr:
        break;
    case clang::BuiltinType::ObjCId:
        break;
    case clang::BuiltinType::ObjCClass:
        break;
    case clang::BuiltinType::ObjCSel:
        break;
    case clang::BuiltinType::OCLSampler:
        break;
    case clang::BuiltinType::OCLEvent:
        break;
    case clang::BuiltinType::OCLClkEvent:
        break;
    case clang::BuiltinType::OCLQueue:
        break;
    case clang::BuiltinType::OCLReserveID:
        break;
    case clang::BuiltinType::Dependent:
        break;
    case clang::BuiltinType::Overload:
        break;
    case clang::BuiltinType::BoundMember:
        break;
    case clang::BuiltinType::UnresolvedTemplate:
        break;
    case clang::BuiltinType::PseudoObject:
        break;
    case clang::BuiltinType::UnknownAny:
        break;
    case clang::BuiltinType::BuiltinFn:
        break;
    case clang::BuiltinType::ARCUnbridgedCast:
        break;
    case clang::BuiltinType::IncompleteMatrixIdx:
        break;
    case clang::BuiltinType::ArraySection:
        break;
    case clang::BuiltinType::OMPArrayShaping:
        break;
    case clang::BuiltinType::OMPIterator:
        break;
    }
}

void ZigClang_detect_enum_CallingConv(clang::CallingConv x) {
    switch (x) {
        case clang::CC_C:
        case clang::CC_X86StdCall:
        case clang::CC_X86FastCall:
        case clang::CC_X86ThisCall:
        case clang::CC_X86VectorCall:
        case clang::CC_X86Pascal:
        case clang::CC_Win64:
        case clang::CC_X86_64SysV:
        case clang::CC_X86RegCall:
        case clang::CC_AAPCS:
        case clang::CC_AAPCS_VFP:
        case clang::CC_IntelOclBicc:
        case clang::CC_SpirFunction:
        case clang::CC_Swift:
        case clang::CC_SwiftAsync:
        case clang::CC_PreserveMost:
        case clang::CC_PreserveAll:
        case clang::CC_AArch64VectorCall:
        case clang::CC_AArch64SVEPCS:
            break;
    }
}

void ZigClang_detect_enum_StorageClass(clang::StorageClass x) {
    switch (x) {
        case clang::SC_None:
        case clang::SC_Extern:
        case clang::SC_Static:
        case clang::SC_PrivateExtern:
        case clang::SC_Auto:
        case clang::SC_Register:
            break;
    }
}

void ZigClang_detect_enum_RoundingMode(llvm::RoundingMode x) {
    switch (x) {
        case llvm::RoundingMode::TowardZero:
        case llvm::RoundingMode::NearestTiesToEven:
        case llvm::RoundingMode::TowardPositive:
        case llvm::RoundingMode::TowardNegative:
        case llvm::RoundingMode::NearestTiesToAway:
        case llvm::RoundingMode::Dynamic:
        case llvm::RoundingMode::Invalid:
            break;
    }
}


void ZigClang_detect_enum_ConstantExprKind(clang::Expr::ConstantExprKind x) {
    switch (x) {
        case clang::Expr::ConstantExprKind::Normal:
        case clang::Expr::ConstantExprKind::NonClassTemplateArgument:
        case clang::Expr::ConstantExprKind::ClassTemplateArgument:
        case clang::Expr::ConstantExprKind::ImmediateInvocation:
            break;
    }
}

// Get a pointer to a static variable in libc++ from LLVM and make sure that
// it matches our own.
//
// This check is needed because if static/dynamic linking is mixed incorrectly,
// it's possible for Clang and LLVM to end up with duplicate "copies" of libc++.
//
// This is not benign: Static variables are not shared, so equality comparisons
// that depend on pointers to static variables will fail. One such failure is
// std::generic_category(), which causes POSIX error codes to compare as unequal
// when passed between LLVM and Clang.
//
// See also: https://github.com/ziglang/zig/issues/11168
bool ZigClangIsLLVMUsingSeparateLibcxx() {

    // Temporarily create an InMemoryFileSystem, so that we can perform a file
    // lookup that is guaranteed to fail.
    auto FS = new llvm::vfs::InMemoryFileSystem(true);
    auto StatusOrErr = FS->status("foo.txt");
    delete FS;

    // This should return a POSIX (generic_category) error code, but if LLVM has
    // its own copy of libc++ this will actually be a separate category instance.
    assert(!StatusOrErr);
    auto EC = StatusOrErr.getError();
    return EC.category() != std::generic_category();
}