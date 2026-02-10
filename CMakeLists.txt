# CMakeList.txt : CMake project for chemical, include path and define
# project specific logic here.
#
cmake_minimum_required(VERSION 3.12)
message("CMake version: ${CMAKE_VERSION}")
project("chemical" VERSION 0.0.29)

# Set C++ standard 20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# use default out/host or out-win/host folder if cmake prefix path is not given
if (NOT DEFINED CMAKE_PREFIX_PATH)
    if (WIN32)
        set(CMAKE_PREFIX_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../out-win/host")
    else ()
        set(CMAKE_PREFIX_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../out/host")
    endif ()
endif ()

# We copied this from Zig CMakeLists
set(ZIG_USE_LLVM_CONFIG ON) # linux build fails if we don't
set(CMAKE_USER_MAKE_RULES_OVERRIDE ${CMAKE_CURRENT_SOURCE_DIR}/cmake/c_flag_overrides.cmake)
set(CMAKE_USER_MAKE_RULES_OVERRIDE_CXX ${CMAKE_CURRENT_SOURCE_DIR}/cmake/cxx_flag_overrides.cmake)
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})

set(ZIG_STATIC off CACHE BOOL "Attempt to build a static zig executable (not compatible with glibc)")
set(ZIG_SHARED_LLVM off CACHE BOOL "Prefer linking against shared LLVM libraries")
set(ZIG_STATIC_LLVM ${ZIG_STATIC} CACHE BOOL "Prefer linking against static LLVM libraries")

if (ZIG_SHARED_LLVM AND ZIG_STATIC_LLVM)
    message(SEND_ERROR "-DZIG_SHARED_LLVM and -DZIG_STATIC_LLVM cannot both be enabled simultaneously")
endif()

option(BUILD_COMPILER "Build full Compiler (requires LLVM/Clang/LLD). Turn OFF to avoid building LLVM-linked Compiler." ON)
option(VERBOSE "Enable verbose output (for debugging)" OFF)
option(ENABLE_ASAN "Enable AddressSanitizer for Debug builds to catch bugs" OFF)
option(DISABLE_RTTI_EXCEPTIONS "Disable RTTI and exceptions for our own targets where safe" ON)

# Debug builds error messages that are more helpful
# set(CMAKE_BUILD_TYPE "Debug")
if (CMAKE_BUILD_TYPE MATCHES Debug)
    message(STATUS "Debug build enabled, Chemical will use project dir '${CMAKE_SOURCE_DIR}' for libraries inside 'lang/libs'")
    add_definitions(-DDEBUG)
    add_compile_definitions(PROJECT_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
    # Treat missing return as an error
    if (CMAKE_CXX_COMPILER_ID MATCHES "Clang|AppleClang|GNU")
        add_compile_options(-Wall -Wextra -Werror=return-type)
    elseif (MSVC)
        add_compile_options(/W4 /we4715)
    endif()
endif ()

# Treat mismatched struct/class tags as errors (causes runtime/linking issues)
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang|AppleClang|GNU")
    add_compile_options(-Werror=mismatched-tags)
elseif (MSVC)
    add_compile_options(/we4099)
endif()

# Verbose builds (should) produce logs as well
if (VERBOSE)
    add_definitions(-DVERBOSE) # Define VERBOSE macro
endif ()

# For printing the version in the compiler
configure_file(
        "${PROJECT_SOURCE_DIR}/utils/Version.h.in"
        "${PROJECT_SOURCE_DIR}/utils/Version.h"
)

# Enable verbose output
#set(CMAKE_VERBOSE_MAKEFILE ON)

# include some compiler options for better support
include(cmake/CompilerOptions.cmake)

# find these packages
if(BUILD_COMPILER)

    # Find llvm,clang,lld packages
    find_package(llvm 17)
    find_package(clang 17)
    find_package(lld 17)

    # Check if CLANG_LIBRARIES contains NOTFOUND
    if (CLANG_LIBRARIES AND CLANG_LIBRARIES MATCHES "CLANG_LIBRARIES-NOTFOUND")
        # Remove NOTFOUND from CLANG_LIBRARIES
        string(REPLACE "CLANG_LIBRARIES-NOTFOUND;" "" CLANG_LIBRARIES "${CLANG_LIBRARIES}")
    endif ()

    # Indicate which libraries are present in these vars
    message(STATUS "LLvm Include Dirs ${LLVM_INCLUDE_DIRS}")
    message(STATUS "LLVM Libs Dirs ${LLVM_LIBDIRS}")
    message(STATUS "LLvm Libs ${LLVM_LIBRARIES}")
    message(STATUS "Clang Libs ${CLANG_LIBRARIES}")
    message(STATUS "LLD Libs ${LLD_LIBRARIES}")
    message(STATUS "LLVM built with RTTI=${LLVM_ENABLE_RTTI} EH=${LLVM_ENABLE_EH}")

    # find the libraries that correspond to llvm
    # llvm_map_components_to_libnames(LLVM_EXTRA_LIBS Option)

endif()

# Base libtcc directory
set(LIBTCC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/lib/tcc)
# Headers for Tiny CC Lib
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/lib/libtcc/include)
# OS-specific dynamic library
if (WIN32)
    set(LIBTCC_LIB ${LIBTCC_DIR}/libtcc.lib)
elseif (APPLE)
    set(LIBTCC_LIB ${LIBTCC_DIR}/libtcc.dylib)
elseif (UNIX)
    set(LIBTCC_LIB ${LIBTCC_DIR}/libtcc.so)
else()
    message(FATAL_ERROR "Unsupported Operating System for libtcc")
endif()
message(STATUS "Using libtcc from: ${LIBTCC_LIB}")

# Add your common source files (excluding the main files)
set(COMMON_SOURCES
        parser/Parser.cpp
        parser/Parser.h
        stream/SourceProvider.h
        parser/statements/LexAssignment.cpp
        parser/statements/VarInitialization.cpp
        parser/statements/VarInitialization.cpp
        parser/statements/LexStatement.cpp
        parser/statements/LexType.cpp
        stream/StreamPosition.h
        parser/utils/LexValue.cpp
        compiler/clang_driver.cpp
        utils/JsonUtils.cpp
        utils/JsonUtils.h
        utils/FileUtils.h
        utils/FileUtils.cpp
        parser/utils/Helpers.cpp
        parser/statements/AccessChain.cpp
        parser/structures/ForBlock.cpp
        parser/structures/WhileBlock.cpp
        parser/structures/IfBlock.cpp
        parser/structures/Function.cpp
        parser/utils/Expression.cpp
        parser/structures/Block.cpp
        parser/statements/Import.cpp
        parser/structures/Enum.cpp
        parser/structures/Struct.cpp
        parser/structures/Interface.cpp
        parser/structures/DoWhile.cpp
        ast/base/Value.h
        ast/utils/Operation.h
        ast/values/StringValue.h
        ast/values/Expression.h
        ast/values/FloatValue.h
        ast/values/DoubleValue.h
        ast/base/ASTNode.h
        ast/statements/VarInit.h
        ast/statements/Assignment.h
        ast/structures/StructDefinition.h
        ast/structures/InterfaceDefinition.h
        ast/structures/ForLoop.h
        ast/structures/WhileLoop.h
        ast/structures/EnumDeclaration.h
        ast/structures/DoWhileLoop.h
        ast/structures/FunctionDeclaration.h
        ast/statements/Import.h
        ast/structures/If.h
        ast/statements/Return.h
        ast/statements/Break.h
        ast/statements/Continue.h
        utils/Utils.cpp
        ast/structures/Scope.h
        ast/values/AccessChain.h
        ast/values/FunctionCall.h
        ast/values/IndexOperator.h
        ast/values/NotValue.h
        ast/values/BoolValue.h
        ast/values/ArrayValue.h
        ast/values/VariableIdentifier.h
        ast/values/Negative.h
        ast/base/InterpretScope.h
        ast/base/GlobalInterpretScope.h
        ast/base/InterpretScope.cpp
        ast/base/GlobalInterpretScope.cpp
        ast/utils/Operation.cpp
        ast/base/LoopASTNode.h
        ast/structures/ImplDefinition.h
        parser/structures/ImplDef.cpp
        parser/statements/AnnotationMacro.cpp
        ast/base/BaseType.h
        compiler/Operation.cpp
        compiler/SymbolResolver.h
        compiler/symres/SymbolResolver.cpp
        compiler/ASTPointer.h
        ast/types/IntNType.h
        ast/types/FloatType.h
        ast/types/StringType.h
        ast/types/DoubleType.h
        ast/types/BoolType.h
        ast/types/ArrayType.h
        ast/types/VoidType.h
        ast/types/AnyType.h
        utils/CmdUtils.h
        core/diag/Diagnostic.h
        parser/values/StructValue.cpp
        ast/values/StructValue.h
        ast/types/LinkedType.h
        ast/structures/FunctionDecl.cpp
        ast/utils/GlobalFunctions.cpp
        core/diag/Diagnostic.cpp
        ast/structures/Scope.cpp
        ast/types/PointerType.h
        ast/types/GenericType.h
        ast/base/Value.cpp
        ast/statements/VarInit.cpp
        ast/structures/StructDefinition.cpp
        ast/values/IntNumValue.cpp
        ast/structures/If.cpp
        ast/structures/WhileLoop.cpp
        ast/structures/DoWhileLoop.cpp
        ast/structures/ForLoop.cpp
        ast/values/ArrayValue.cpp
        ast/values/StructValue.cpp
        ast/statements/Assignment.cpp
        ast/values/CharValue.cpp
        ast/values/StringValue.cpp
        ast/values/NotValue.cpp
        ast/values/Negative.cpp
        ast/values/FunctionCall.cpp
        ast/values/BoolValue.cpp
        ast/values/DoubleValue.cpp
        ast/values/FloatValue.cpp
        ast/types/DoubleType.cpp
        ast/types/FloatType.cpp
        ast/types/IntNType.cpp
        ast/types/PointerType.cpp
        ast/types/StringType.cpp
        ast/types/VoidType.cpp
        ast/values/VariableIdentifier.cpp
        ast/values/IndexOperator.cpp
        ast/values/AccessChain.cpp
        ast/types/LinkedType.cpp
        ast/types/GenericType.cpp
        ast/values/Expression.cpp
        parser/statements/Typealias.cpp
        ast/statements/Typealias.cpp
        ast/statements/Typealias.h
        ast/statements/Continue.cpp
        ast/statements/Return.cpp
        ast/statements/SwitchStatement.cpp
        ast/statements/SwitchStatement.h
        ast/structures/TryCatch.cpp
        ast/structures/TryCatch.h
        parser/structures/TryCatch.cpp
        parser/structures/Switch.cpp
        ast/statements/Import.cpp
        compiler/SelfInvocation.h
        compiler/SelfInvocation.cpp
        ast/values/CastedValue.cpp
        ast/values/CastedValue.h
        ast/values/LambdaFunction.h
        ast/values/LambdaFunction.cpp
        parser/values/LambdaValue.cpp
        core/diag/Position.h
        core/diag/Diagnoser.h
        ast/base/BaseTypeKind.h
        ast/base/BaseTypeKind.h
        ast/base/BaseType.cpp
        utils/ValueAndOperatorStack.h
        ast/base/AccessSpecifier.h
        ast/values/AddrOfValue.h
        ast/values/AddrOfValue.cpp
        ast/values/DerferenceValue.cpp
        ast/structures/InterfaceDefinition.cpp
        ast/structures/StructMember.h
        ast/structures/FunctionParam.h
        ast/structures/MembersContainer.h
        ast/structures/MembersContainer.cpp
        ast/structures/ImplDefinition.cpp
        ast/values/NullValue.h
        ast/values/NullValue.cpp
        ast/values/IntNumValue.h
        ast/types/StructType.h
        ast/types/StructType.cpp
        ast/structures/EnumDecl.cpp
        ast/structures/EnumMember.h
        ast/structures/CapturedVariable.h
        utils/StringHelpers.h
        utils/StringHelpers.cpp
        ast/statements/Break.cpp
        core/diag/DiagSeverity.h
        utils/Benchmark.h
        utils/Benchmark.cpp
        compiler/ASTDiagnoser.h
        compiler/ASTDiagnoser.cpp
        server/model/LexResult.h
        preprocess/ImportPathHandler.h
        preprocess/ImportPathHandler.cpp
        core/diag/Location.h
        core/diag/Range.h
        server/analyzers/Documentation.h
        preprocess/2c/2cASTVisitor.h
        preprocess/2c/2cASTVisitor.cpp
        preprocess/RepresentationVisitor.cpp
        preprocess/utils/RepresentationUtils.h
        preprocess/utils/RepresentationUtils.cpp
        compiler/ASTProcessorOptions.h
        compiler/ASTProcessor.h
        compiler/ASTDiag.h
        compiler/ASTProcessor.cpp
        utils/PathUtils.h
        compiler/cbi/bindings/CBI.h
        compiler/cbi/bindings/CBI.cpp
        parser/CompilerBinder.cpp
        ast/utils/ASTUtils.h
        ast/utils/ASTUtils.cpp
        integration/libtcc/LibTccInteg.h
        integration/libtcc/LibTcc.cpp
        utils/Version.h
        ast/base/Annotation.h
        ast/base/Annotation.cpp
        ast/base/AnnotableNode.h
        ast/base/AnnotationKind.h
        ast/types/FunctionType.h
        ast/types/FunctionType.cpp
        ast/base/AnnotationParent.h
        ast/base/ExtendableMembersContainerNode.h
        compiler/OutputMode.h
        compiler/CodegenEmitterOptions.h
        ast/statements/ThrowStatement.h
        ast/statements/ThrowStatement.cpp
        ast/structures/Namespace.h
        ast/structures/Namespace.cpp
        parser/structures/Namespace.cpp
        ast/base/ASTNode.cpp
        ast/values/SizeOfValue.h
        ast/values/SizeOfValue.cpp
        std/chem_string.h
        ast/structures/UnionDef.h
        ast/structures/UnionDef.cpp
        parser/structures/Union.cpp
        ast/types/UnionType.h
        ast/types/UnionType.cpp
        ast/structures/VariablesContainer.h
        ast/structures/BaseDefMember.h
        ast/structures/UnnamedUnion.h
        ast/types/LinkedValueType.h
        ast/types/LiteralType.h
        ast/types/Types.cpp
        ast/types/ArrayType.cpp
        ast/base/ASTAny.h
        ast/base/ASTAny.cpp
        ast/base/ChainPart.h
        ast/statements/UsingStmt.h
        ast/statements/Using.cpp
        ast/statements/DestructStmt.h
        ast/statements/DestructStmt.cpp
        ast/values/RetStructParamValue.h
        ast/values/RetStructParamValue.cpp
        compiler/lab/LabBuildContext.h
        compiler/lab/LabModule.h
        compiler/lab/LabBuildContext.cpp
        compiler/lab/LabModuleType.h
        compiler/lab/LabBuildCompiler.cpp
        compiler/lab/LabJob.h
        compiler/InvokeUtils.h
        compiler/lab/LabJobType.h
        std/chem_string.cpp
        ast/structures/MultiFunctionNode.h
        ast/structures/MultiFunctionNode.cpp
        ast/structures/GenericTypeParameter.h
        ast/utils/GenericUtils.h
        compiler/lab/LabBuildCompilerOptions.h
        compiler/lab/Utils.h
        stream/InputSource.h
        stream/FileInputSource.h
        stream/InputSource.cpp
        ast/structures/InheritedType.h
        ast/types/DynamicType.h
        ast/types/DynamicType.cpp
        ast/structures/VariantDefinition.h
        ast/structures/VariantDefinition.cpp
        ast/structures/VariantMemberParam.h
        parser/structures/Variant.cpp
        ast/base/ASTNodeKind.h
        ast/values/VariantCase.h
        ast/values/VariantCaseVariable.h
        ast/values/IsValue.cpp
        ast/values/ValueNode.h
        ast/values/ValueNode.cpp
        ast/base/ValueKind.h
        ast/structures/LoopBlock.h
        utils/Environment.h
        ast/base/ASTAnyKind.h
        ast/base/ASTUnit.h
        server/model/ASTResult.h
        preprocess/2c/CBeforeStmtVisitor.h
        preprocess/2c/CAfterStmtVisitor.h
        preprocess/2c/CTopLevelDeclVisitor.h
        preprocess/2c/CValueDeclVisitor.h
        ast/values/StructMemberInitializer.h
        ast/types/ReferenceType.h
        compiler/lab/BackendContext.h
        preprocess/2c/2cBackendContext.h
        compiler/backend/LLVMBackendContext.h
        ast/structures/UnsafeBlock.h
        ast/structures/UnsafeBlock.cpp
        ast/base/ASTAllocator.h
        ast/base/ASTAllocator.cpp
        compiler/cbi/bindings/BuildContextCBI.h
        compiler/cbi/bindings/BuildContextCBI.cpp
        compiler/cbi/bindings/CBIUtils.h
        compiler/cbi/bindings/SourceProviderCBI.cpp
        compiler/cbi/bindings/SourceProviderCBI.h
        compiler/cbi/bindings/PtrVecCBI.h
        compiler/cbi/bindings/PtrVecCBI.cpp
        compiler/cbi/bindings/ASTBuilderCBI.h
        compiler/cbi/bindings/ASTBuilderCBI.cpp
        compiler/cbi/bindings/ASTCBI.h
        compiler/cbi/bindings/LabCBIAddons.h
        compiler/cbi/bindings/LabCBIAddons.cpp
        ast/statements/ProvideStmt.h
        ast/statements/ProvideStmt.cpp
        ast/types/Float128Type.h
        ast/base/TypeBuilder.h
        compiler/lab/TargetData.h
        ast/types/LongDoubleType.h
        ast/types/ComplexType.h
        ast/structures/ComptimeBlock.h
        ast/structures/ComptimeBlock.cpp
        utils/inline_attr.h
        core/source/LocationManager.h
        core/source/LocationManager.cpp
        core/source/SourceLocation.h
        ast/base/LocatedIdentifier.h
        ast/base/BatchAllocator.h
        parser/utils/parse_num.h
        lexer/Lexer.h
        lexer/Lexer.cpp
        lexer/TokenType.h
        lexer/Token.h
        lexer/LexUnit.h
        std/chem_span.h
        std/chem_string_view.h
        std/small_vector.h
        ast/values/AlignOfValue.h
        compiler/cbi/bindings/BatchAllocatorCBI.h
        compiler/cbi/bindings/BatchAllocatorCBI.cpp
        compiler/cbi/bindings/LexerCBI.h
        compiler/cbi/bindings/LexerCBI.cpp
        ast/values/NewTypedValue.h
        ast/values/NewValue.h
        ast/values/PlacementNewValue.h
        ast/values/NewValue.cpp
        compiler/cbi/bindings/ParserCBI.cpp
        ast/statements/ValueWrapperNode.h
        compiler/cbi/bindings/SymbolResolverCBI.h
        compiler/cbi/bindings/SymbolResolverCBI.cpp
        ast/values/IncDecValue.h
        ast/values/ComptimeValue.h
        ast/values/UnsafeValue.h
        ast/types/ExpressionType.h
        ast/values/PointerValue.h
        ast/values/PointerValue.cpp
        ast/values/BlockValue.h
        ast/values/BlockValue.cpp
        preprocess/visitors/NonRecursiveVisitor.h
        ast/base/ast_fwd.h
        ast/base/IntNTypeKind.h
        ast/structures/GenericFuncDecl.h
        ast/structures/BaseGenericDecl.h
        compiler/generics/GenericInstantiator.h
        preprocess/visitors/RecursiveVisitor.h
        compiler/generics/GenericInstantiator.cpp
        ast/structures/GenericFuncDecl.cpp
        compiler/generics/GenInstantiatorAPI.h
        ast/statements/AliasStmt.h
        ast/statements/AliasStmt.cpp
        ast/values/TypeInsideValue.h
        compiler/symres/SymbolTable.h
        ast/structures/GenericStructDecl.h
        ast/structures/GenericStructDecl.cpp
        ast/structures/GenericMembersDecl.h
        compiler/symres/SymbolRange.h
        ast/values/WrapValue.h
        ast/structures/GenericVariantDecl.h
        ast/structures/GenericUnionDecl.h
        ast/structures/GenericInterfaceDecl.h
        ast/structures/GenericVariantDecl.cpp
        ast/structures/GenericInterfaceDecl.cpp
        ast/structures/GenericUnionDecl.cpp
        ast/structures/GenericTypeDecl.h
        ast/structures/GenericTypeDecl.cpp
        ast/structures/GenericImplDecl.h
        ast/structures/GenericImplDecl.cpp
        ast/structures/MembersIterators.h
        ast/base/DebugCast.h
        compiler/symres/SymbolResolver.h
        compiler/mangler/NameMangler.h
        compiler/mangler/NameMangler.cpp
        compiler/mangler/NameMangler.cpp
        ast/structures/FileScope.h
        ast/structures/ModuleScope.h
        compiler/lab/ModuleStorage.h
        compiler/Interpreter/Core.cpp
        compiler/processor/ASTFileMetaData.h
        compiler/processor/ModuleDependencyRecord.h
        compiler/symres/SymResScopeKind.h
        compiler/processor/ASTFileResult.h
        parser/utils/ParseModDecl.cpp
        parser/utils/ParseModDecl.h
        server/model/ASTResult.h
        server/model/LexResult.h
        compiler/cbi/model/Model.h
        compiler/cbi/model/CBIData.h
        compiler/cbi/model/CompilerBinder.h
        compiler/processor/ModuleFileData.h
        compiler/symres/NodeSymbolDeclarer.h
        compiler/lab/timestamp/Timestamp.h
        compiler/lab/timestamp/Timestamp.cpp
        ast/base/TypeLoc.h
        ast/base/TypeBuilder.cpp
        ast/types/NullPtrType.h
        compiler/cbi/model/CBIFunctionType.h
        compiler/cbi/model/CBIFunctionIndex.h
        compiler/lab/mod_conv/ModToLabConverter.h
        compiler/lab/mod_conv/ModToLabConverter.cpp
        compiler/lab/LabGetMethodInjection.h
        preprocess/StringViewHashEqual.h
        core/main/CompilerMain.h
        core/main/CompilerMain.cpp
        utils/CmdUtils2.h
        utils/CmdUtils.cpp
        compiler/typeverify/TypeVerify.h
        compiler/typeverify/TypeVerifyAPI.h
        compiler/typeverify/TypeVerify.cpp
        compiler/generics/InstantiationsContainer.h
        ast/values/PatternMatchExpr.h
        ast/values/PatternMatchExpr.cpp
        ast/types/CapturingFunctionType.h
        ast/values/ExtractionValue.h
        ast/values/ExtractionValue.cpp
        server/cbi/hooks.h
        ast/values/EmbeddedValue.h
        ast/values/EmbeddedValue.cpp
        ast/statements/EmbeddedNode.h
        compiler/cbi/bindings/ASTDiagnoserCBI.h
        compiler/cbi/bindings/ASTDiagnoserCBI.cpp
        compiler/symres/DeclareTopLevel.cpp
        compiler/symres/DeclareTopLevel.h
        compiler/symres/LinkSignature.h
        compiler/symres/LinkSignature.cpp
        compiler/symres/LinkSignatureAPI.h
        compiler/symres/SymResLinkBody.h
        compiler/symres/SymResLinkBody.cpp
        compiler/symres/SymResLinkBodyAPI.h
        compiler/cbi/model/ASTBuilder.h
        ast/statements/UnresolvedDecl.h
        ast/statements/AccessChainNode.h
        ast/statements/IncDecNode.h
        ast/statements/PatternMatchExprNode.h
        ast/statements/PlacementNewNode.h
        ast/values/InValue.h
        ast/values/InValue.cpp
        compiler/frontend/AnnotationController.h
        compiler/frontend/AnnotationController.cpp
        lang/libs/cstd/posix/io/spawn.ch
        ast/values/IfValue.h
        ast/values/SwitchValue.h
        ast/values/LoopValue.h
        ast/values/LoopValue.h
        integration/libtcc/TCCMode.h
        preprocess/2c/BufferedWriter.h
        ast/utils/IffyConditional.h
        ast/values/ExpressiveString.h
        ast/types/ExpressiveStringType.h
        ast/base/ChildResolution.h
        compiler/symres/ChildResolver.h
        std/chem_string_view_fast.h
        ast/values/DynamicValue.h
        ast/types/IfType.h
        ast/types/IfType.cpp
        ast/values/MultipleValue.h
        ast/values/RawLiteral.h
        compiler/lab/backend/atomics.h
        ast/types/MaybeRuntimeType.h
        compiler/symres/BeforeLinkSignature.h
        std/except.h
        std/except.cpp
        compiler/lab/LabJob.cpp
        compiler/lab/TargetConditionAPI.h
        preprocess/2c/CDestructionVisitor.h
        compiler/cbi/bindings/AnnotationControllerCBI.h
        compiler/cbi/bindings/AnnotationController.cpp
        ast/statements/Export.h
        parser/statements/Export.cpp
)

# Chemical LSP Executable Target
add_executable(ChemicalLsp core/targets/Main.cpp ${COMMON_SOURCES}
        server/WorkspaceManager.h
        server/WorkspaceManager.cpp
        server/utils/PrintUtils.h
        server/utils/PrintUtils.cpp
        server/LspSemanticTokens.cpp
        server/analyzers/FoldingRangeAnalyzer.h
        server/analyzers/FoldingRangeAnalyzer.cpp
        server/analyzers/CompletionItemAnalyzer.h
        server/analyzers/CompletionItemAnalyzer.cpp
        server/analyzers/SemanticTokensAnalyzer.h
        server/analyzers/SemanticTokensAnalyzer.cpp
        server/analyzers/DocumentSymbolsAnalyzer.h
        server/analyzers/DocumentSymbolsAnalyzer.cpp
        server/utils/lspfwd.h
        server/Importer.cpp
        server/analyzers/GotoDefAnalyzer.h
        server/analyzers/GotoDefAnalyzer.cpp
        server/analyzers/HoverAnalyzer.h
        server/analyzers/HoverAnalyzer.cpp
        server/analyzers/DocumentLinksAnalyzer.h
        server/analyzers/DocumentLinksAnalyzer.cpp
        server/analyzers/InlayHintAnalyzer.h
        server/analyzers/InlayHintAnalyzer.cpp
        server/analyzers/SignatureHelpAnalyzer.h
        server/analyzers/SignatureHelpAnalyzer.cpp
        server/analyzers/CaretPositionAnalyzer.h
        server/analyzers/CaretPositionAnalyzer.cpp
        server/analyzers/InlayHintAnalyzerApi.h
        server/build/ContextSerialization.h
        server/build/ContextSerialization.cpp
        server/build/ChildProcessBuild.h
        server/build/ChildProcessBuild.cpp
        server/build/ipc_process.h
        server/build/ipc_process.cpp
        server/utils/LRUCache.h
        server/utils/AnalyzerUtils.h
        server/utils/AnalyzerUtils.cpp
        server/model/ModuleData.h
        server/model/AnonymousFileData.h
        compiler/cbi/bindings/lsp/LSPHooks.h
        compiler/cbi/bindings/lsp/LSPHooks.cpp
        server/model/SemanticTokenScopes.h
)

# lsp framework
set(LSP_USE_SANITIZERS OFF CACHE BOOL "Turn off UBSan/ASan in lsp-framework" FORCE)
add_subdirectory(lib/lsp-framework)

# Add an executable target for the second executable
add_executable(Interpreter core/targets/Interpreter.cpp ${COMMON_SOURCES})

set(COMPILER_SOURCES
        compiler/Codegen.h
        compiler/Codegen.cpp
        compiler/clang_cc1main.cpp
        compiler/clang_driver.cpp
        compiler/clang_cc1as_main.cpp
        compiler/llvmfwd.h
        compiler/llvmimpl.h
        compiler/backend/CLANG.cpp
        compiler/ctranslator/CTranslator.h
        compiler/ctranslator/CTranslator.cpp
        compiler/chem_clang.h
        compiler/chem_clang.cpp
        compiler/backend/LLVM.cpp
        compiler/ASTCompiler.cpp
        compiler/backend/DebugInfoBuilder.h
        compiler/backend/DebugInfoBuilder.cpp
        compiler/llvm-ar.cpp
        utils/ProcessUtils.h
        compiler/ClangCodegen.h
        preprocess/2c/SubVisitor.h
        compiler/backend/include/LLVMArrayDestructor.h
        compiler/CodegenOptions.h
        compiler/backend/LLVMGen.h
        compiler/backend/LLVMGen.cpp
)

# add Compiler executable
if(BUILD_COMPILER)
    add_executable(Compiler core/targets/Compiler.cpp ${COMMON_SOURCES} ${COMPILER_SOURCES})
endif()

# add TCC Compiler executable
add_executable(TCCCompiler core/targets/Compiler.cpp ${COMMON_SOURCES})

# Compiler uses llvm, clang, lld
if(BUILD_COMPILER)
    target_compile_definitions(Compiler PRIVATE COMPILER_BUILD CLANG_LIBS LLD_LIBS)
endif()

# TCC Compiler does not use llvm and clang
target_compile_definitions(TCCCompiler PRIVATE TCC_BUILD)

# LSP has some definitions
target_compile_definitions(ChemicalLsp PRIVATE LSP_BUILD)

# Define common include directories
set(COMMON_INCLUDE_DIRS
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/lib/tsl-ordered-map
        ${CMAKE_CURRENT_SOURCE_DIR}/lib/ctpl
        ${CMAKE_CURRENT_SOURCE_DIR}/lib/rang
)

# Only add sanitizer flags in Debug builds
if(ENABLE_ASAN AND CMAKE_BUILD_TYPE MATCHES Debug)
    if(MSVC)
        # MSVC supports address sanitizer with /fsanitize=address.
        set(ASAN_COMPILE_FLAG "/fsanitize=address")
        set(ASAN_LINK_FLAG "/fsanitize=address")
    else()
        # GCC/Clang
        set(ASAN_COMPILE_FLAG "-fsanitize=address -fno-omit-frame-pointer")
        set(ASAN_LINK_FLAG "-fsanitize=address -fno-omit-frame-pointer")
    endif()
    # Apply these flags only for targets that do not link against LLVM/Clang libraries.
    # For example, applying to TCCCompiler:
    target_compile_options(TCCCompiler PRIVATE ${ASAN_COMPILE_FLAG})
    target_link_options(TCCCompiler PRIVATE ${ASAN_LINK_FLAG})
endif()

# Helper: disable RTTI + exceptions for a given target in a portable way.
# Usage: chem_disable_rtti_exceptions(<target> [ASSUME_NO_THIRD_PARTY])
# If ASSUME_NO_THIRD_PARTY is provided we won't add extra warnings/defs.
function(chem_disable_rtti_exceptions target)
    if(NOT TARGET ${target})
        message(WARNING "chem_disable_rtti_exceptions: target ${target} not found")
        return()
    endif()

    # GCC/Clang flags
    target_compile_options(${target} PRIVATE
            # only apply these flags for GNU or Clang
            $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:-fno-rtti -fno-exceptions>
    )

    # MSVC: disable RTTI with /GR-
    target_compile_options(${target} PRIVATE
            $<$<CXX_COMPILER_ID:MSVC>:/GR->
    )

    # MSVC: instruct libc++/STL headers not to use exceptions
    # _HAS_EXCEPTIONS=0 is the usual macro used to compile MSVC STL without exceptions.
    # WARNING: mixing CRT/third-party libs built with different _HAS_EXCEPTIONS values can break.
    target_compile_definitions(${target} PRIVATE
            $<$<CXX_COMPILER_ID:MSVC>:_HAS_EXCEPTIONS=0>
    )

    # Keep /EHsc to allow normal unwind metadata (so linking with libs that expect unwind works)
    target_compile_options(${target} PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/EHsc>)

    # GCC/Clang: set compile definition to help conditional code
    target_compile_definitions(${target} PRIVATE
            $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:DISABLE_RTTI_AND_EXCEPTIONS>
    )

    # Optionally: tell the linker to strip exceptions/RTTI symbols more aggressively at link-time
    # (GCC/Clang only) — this can further reduce size when using -s or -Wl,--gc-sections.
    target_link_options(${target} PRIVATE
            $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:-Wl,--gc-sections>
    )
endfunction()

# Disables rtti exceptions
if(DISABLE_RTTI_EXCEPTIONS)
    chem_disable_rtti_exceptions(Interpreter)
    chem_disable_rtti_exceptions(TCCCompiler)
    if(BUILD_COMPILER)
        chem_disable_rtti_exceptions(Compiler)
    endif()
    # chemical lsp needs exceptions and dynamic cast support
    # because it links with lsp-framework which uses exceptions and dynamic casts
    # maybe we could eliminate it in the future
    # chem_disable_rtti_exceptions(ChemicalLsp)
endif()

# Add include directories for LspCpp
target_include_directories(ChemicalLsp PRIVATE
        ${COMMON_INCLUDE_DIRS}
        ${CMAKE_SOURCE_DIR}/lib/lsp-framework
        ${LSP_GENERATED_FILES_DIR}
)

# Add include directories for LspCpp
target_include_directories(Interpreter PRIVATE
        ${COMMON_INCLUDE_DIRS}
)

# Add include directories for Compiler
if(BUILD_COMPILER)
    target_include_directories(Compiler PRIVATE
            ${COMMON_INCLUDE_DIRS}
            # Include llvm directories for the compiler executable
            ${LLVM_INCLUDE_DIRS}
            ${LLD_INCLUDE_DIRS}
            ${CLANG_INCLUDE_DIRS}
    )
endif()

# Add include directories for TCCCompiler
target_include_directories(TCCCompiler PRIVATE
        ${COMMON_INCLUDE_DIRS}
)

if (MSVC)
    # Disable warning C4267: 'argument': conversion from 'size_t' to '...', possible loss of data
    # For Microsoft Visual Studio Compiler Only
    if(BUILD_COMPILER)
        target_compile_options(Compiler PRIVATE /wd4267)
    endif()
    target_compile_options(TCCCompiler PRIVATE /wd4267)
    target_compile_options(Interpreter PRIVATE /wd4267)
    target_compile_options(ChemicalLsp PRIVATE /wd4267)
endif()

# https://discourse.llvm.org/t/undefined-reference-to-typeinfo-for-llvm-genericoptionvalue/71526/3
if(BUILD_COMPILER)
    set(NO_RTTI "-fno-rtti")
    target_compile_options(Compiler PRIVATE ${NO_RTTI})
endif()

if (MSVC)
    # Force “Multi-threaded static CRT” everywhere:
    #   – In Debug builds → /MTd
    #   – In Release/RelWithDebInfo/MinSizeRel → /MT
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

if (WIN32)
    # Linking Compiler executable statically is necessary because of LLVM
    # we link everything else statically on windows anyway
    if(BUILD_COMPILER)
        target_compile_options(Compiler PRIVATE "/MT")
    endif()
elseif(UNIX AND NOT APPLE)
    # Statically link C++ runtime + libgcc
    if(BUILD_COMPILER)
        target_link_options(Compiler PRIVATE -static-libstdc++ -static-libgcc)
    endif()
    target_link_options(TCCCompiler PRIVATE -static-libstdc++ -static-libgcc)
elseif(APPLE)
    # macOS: do nothing, can't fully statically link libc++
    message(STATUS "macOS detected, skipping static linking of standard libraries")
endif()

# Linking interpreter
target_link_libraries(Interpreter)

if(APPLE)
    if(BUILD_COMPILER)
        target_link_libraries(Compiler PRIVATE
                ${CLANG_LIBRARIES} ${LLVM_LIBRARIES} ${LLD_LIBRARIES}
                ${LIBTCC_LIB}
        )
    endif()
    target_link_libraries(TCCCompiler PRIVATE
            ${LIBTCC_LIB}
    )
elseif(UNIX AND NOT APPLE)
    if(BUILD_COMPILER)
        target_link_libraries(Compiler PRIVATE
                # ask linker to prefer static libraries from here onward
                -Wl,-Bstatic
                ${CLANG_LIBRARIES} ${LLVM_LIBRARIES} ${LLD_LIBRARIES}
                # switch back to dynamic mode for the next libs
                -Wl,-Bdynamic
                ${LIBTCC_LIB}
        )
    endif()
    target_link_libraries(TCCCompiler PRIVATE
            # ask linker to prefer static libraries from here onward
            -Wl,-Bstatic
            # switch back to dynamic mode for the next libs
            -Wl,-Bdynamic
            ${LIBTCC_LIB}
    )
else()
    # Windows branch
    # llvm and clang will automatically be always statically linked (these paths point to .a on linux, .lib on windows)
    if(BUILD_COMPILER)
        target_link_libraries(Compiler ${CLANG_LIBRARIES} ${LLVM_LIBRARIES} ${LLD_LIBRARIES} ${LIBTCC_LIB})
    endif()
    target_link_libraries(TCCCompiler ${LIBTCC_LIB})
endif()

# always search for the libtcc.so in the executable directory
if(APPLE)
    set_target_properties(TCCCompiler PROPERTIES
        INSTALL_RPATH "@executable_path"
        BUILD_WITH_INSTALL_RPATH TRUE
    )
    set_target_properties(ChemicalLsp PROPERTIES
        INSTALL_RPATH "@executable_path"
        BUILD_WITH_INSTALL_RPATH TRUE
    )
    if(BUILD_COMPILER)
        set_target_properties(Compiler PROPERTIES
            INSTALL_RPATH "@executable_path"
            BUILD_WITH_INSTALL_RPATH TRUE
        )
    endif()
elseif(UNIX)
    set_target_properties(TCCCompiler PROPERTIES
            INSTALL_RPATH "$ORIGIN"
            BUILD_WITH_INSTALL_RPATH TRUE
    )
    set_target_properties(ChemicalLsp PROPERTIES
            INSTALL_RPATH "$ORIGIN"
            BUILD_WITH_INSTALL_RPATH TRUE
    )
    if(BUILD_COMPILER)
        set_target_properties(Compiler PROPERTIES
                INSTALL_RPATH "$ORIGIN"
                BUILD_WITH_INSTALL_RPATH TRUE
        )
    endif()
endif()

# Link Lsp
target_link_libraries(ChemicalLsp ${LIBTCC_LIB} lsp)