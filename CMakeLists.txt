# CMakeList.txt : CMake project for chemical, include path and define
# project specific logic here.
#
cmake_minimum_required(VERSION 3.12)
message("CMake version: ${CMAKE_VERSION}")
project("chemical" VERSION 1.0.1)

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

# Debug builds error messages that are more helpful
# set(CMAKE_BUILD_TYPE "Debug")
if (CMAKE_BUILD_TYPE MATCHES Debug)
    add_definitions(-DDEBUG)
endif ()

# Verbose builds produce logs as well
option(VERBOSE "Enable verbose output (for debugging)" OFF)
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
# Use static libraries
set(Boost_USE_STATIC_LIBS ON)
# Use debug version of Boost libraries
set(Boost_USE_DEBUG_LIBS ON)
# Don't warn about boost new version
set(Boost_NO_WARN_NEW_VERSIONS 1)

# set custom boost root (not required) (set BOOST_ROOT environment variable)
# set(BOOST_ROOT "path\\to\\boost")

# Enable this option if Boost Not Found to check for Boost
# set(Boost_DEBUG 1)

# include some compiler options for better support
include(cmake/CompilerOptions.cmake)
include(cmake/FeatureFlags.cmake)

# Set CMP0074 policy so that CMake can use BOOST_ROOT environment variable
if (POLICY CMP0074)
    cmake_policy(SET CMP0074 NEW)
endif ()

# Find Boost
find_package(Boost 1.74 COMPONENTS program_options filesystem REQUIRED)

# find these packages
find_package(llvm 17)
find_package(clang 17)
find_package(lld 17)

# Check if CLANG_LIBRARIES contains NOTFOUND
if (CLANG_LIBRARIES AND CLANG_LIBRARIES MATCHES "CLANG_LIBRARIES-NOTFOUND")
    # Remove NOTFOUND from CLANG_LIBRARIES
    string(REPLACE "CLANG_LIBRARIES-NOTFOUND;" "" CLANG_LIBRARIES "${CLANG_LIBRARIES}")
endif ()

# Add the LspCpp library directory
add_subdirectory(LspCpp)

# Include directories for libtcc
set(LIBTCC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/lib/libtcc)
if(WIN32)
    set(LIBTCC_LIB_DIR ${LIBTCC_DIR}/win-x64)
    set(LIBTCC_LIB ${LIBTCC_LIB_DIR}/libtcc.lib)
elseif(UNIX)
    set(LIBTCC_LIB_DIR ${LIBTCC_DIR}/lin-x64)
    set(LIBTCC_LIB ${LIBTCC_LIB_DIR}/libtcc.so)
else()
    message(FATAL_ERROR "Unknown Operating System, Couldn't Determine libtcc to use")
endif()

# Add your common source files (excluding the main files)
set(COMMON_SOURCES
        lexer/Lexer.cpp
        lexer/Lexer.h
        lexer/model/tokens/LexToken.h
        stream/SourceProvider.h
        lexer/Lexi.h
        lexer/statements/LexAssignment.cpp
        lexer/statements/VarInitialization.cpp
        lexer/statements/VarInitialization.cpp
        lexer/statements/LexStatement.cpp
        lexer/utils/LexWhitespace.cpp
        lexer/utils/LexNumber.cpp
        lexer/statements/LexType.cpp
        stream/StreamPosition.h
        lexer/Lexi.cpp
        lexer/utils/LexValue.cpp
        compiler/clang_driver.cpp
        utils/JsonUtils.cpp
        utils/JsonUtils.h
        utils/FileUtils.h
        utils/FileUtils.cpp
        lexer/statements/Comment.cpp
        lexer/utils/Helpers.cpp
        lexer/statements/AccessChain.cpp
        lexer/structures/ForBlock.cpp
        lexer/structures/WhileBlock.cpp
        lexer/structures/IfBlock.cpp
        lexer/structures/Function.cpp
        lexer/utils/Expression.cpp
        lexer/structures/Block.cpp
        lexer/statements/Import.cpp
        lexer/structures/Enum.cpp
        lexer/structures/Struct.cpp
        lexer/structures/Interface.cpp
        lexer/structures/DoWhile.cpp
        ast/base/Value.h
        ast/utils/Operation.h
        ast/values/IntValue.h
        ast/values/StringValue.h
        ast/values/Expression.h
        ast/values/FloatValue.h
        ast/values/DoubleValue.h
        ast/values/CharValue.h
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
        lexer/model/LexTokenType.h
        utils/Utils.h
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
        ast/base/ValueType.h
        ast/utils/ExpressionEvaluator.h
        ast/utils/ValueType.cpp
        ast/utils/ExpressionEvaluator.cpp
        ast/base/GlobalInterpretScope.h
        ast/base/InterpretScope.cpp
        ast/base/GlobalInterpretScope.cpp
        ast/utils/Operation.cpp
        ast/structures/LoopScope.h
        ast/base/LoopASTNode.h
        ast/structures/ImplDefinition.h
        lexer/structures/ImplDef.cpp
        ast/statements/Comment.h
        ast/base/Interpretable.h
        lexer/statements/AnnotationMacro.cpp
        ast/base/BaseType.h
        compiler/Operation.cpp
        compiler/SymbolResolver.h
        compiler/SymbolResolver.cpp
        compiler/ASTPointer.h
        ast/types/IntNType.h
        ast/types/FloatType.h
        ast/types/StringType.h
        ast/types/DoubleType.h
        ast/values/TernaryValue.h
        ast/types/BoolType.h
        ast/types/CharType.h
        ast/types/ArrayType.h
        ast/base/Visitor.h
        ast/types/VoidType.h
        ast/types/AnyType.h
        utils/CmdUtils.h
        integration/common/Diagnostic.h
        lexer/values/StructValue.cpp
        ast/values/StructValue.h
        ast/types/ReferencedType.h
        stream/SourceProviderImpl.cpp
        ast/structures/FunctionDecl.cpp
        ast/utils/GlobalFunctions.cpp
        ast/statements/MacroValueStatement.h
        integration/common/Diagnostic.cpp
        lexer/structures/Collection.cpp
        ast/structures/Scope.cpp
        ast/structures/LoopScope.cpp
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
        ast/types/ReferencedType.cpp
        ast/types/GenericType.cpp
        ast/values/Expression.cpp
        lexer/statements/Typealias.cpp
        ast/statements/Typealias.cpp
        ast/statements/Typealias.h
        ast/statements/Continue.cpp
        ast/statements/Return.cpp
        ast/statements/SwitchStatement.cpp
        ast/statements/SwitchStatement.h
        ast/structures/TryCatch.cpp
        ast/structures/TryCatch.h
        lexer/structures/TryCatch.cpp
        lexer/structures/Switch.cpp
        ast/statements/Import.cpp
        compiler/SelfInvocation.h
        compiler/SelfInvocation.cpp
        ast/values/CastedValue.cpp
        ast/values/CastedValue.h
        ast/types/FunctionType.h
        ast/types/FunctionType.cpp
        ast/values/LambdaFunction.h
        ast/values/LambdaFunction.cpp
        lexer/values/LambdaValue.cpp
        cst/base/CSTToken.h
        utils/fwd/vector.h
        cst/base/CompoundCSTToken.h
        cst/CSTConvert.cpp
        cst/base/CSTConverter.h
        cst/base/CSTVisitor.h
        integration/common/Position.h
        cst/base/CSTDiagnoser.h
        cst/CSTDiagnoser.cpp
        ast/base/BaseTypeKind.h
        ast/base/BaseTypeKind.h
        ast/base/BaseType.cpp
        cst/utils/ValueAndOperatorStack.h
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
        ast/values/UIntValue.h
        ast/values/IntNumValue.h
        ast/values/ShortValue.h
        ast/values/UShortValue.h
        ast/values/LongValue.h
        ast/values/ULongValue.h
        ast/values/BigIntValue.h
        ast/values/UBigIntValue.h
        utils/StrUtils.h
        utils/StrUtils.cpp
        ast/types/StructType.h
        ast/types/StructType.cpp
        ast/types/IntType.h
        ast/types/UIntType.h
        ast/types/ShortType.h
        ast/types/UShortType.h
        ast/types/UBigIntType.h
        ast/types/BigIntType.h
        ast/types/ULongType.h
        ast/values/NumberValue.h
        ast/values/NumberValue.cpp
        ast/structures/EnumDecl.cpp
        ast/structures/EnumMember.h
        ast/structures/CapturedVariable.h
        ast/values/UInt128Value.h
        ast/values/Int128Value.h
        cst/utils/StringHelpers.h
        cst/utils/StringHelpers.cpp
        cst/base/CSTToken.cpp
        ast/statements/Break.cpp
        integration/common/DiagSeverity.h
        preprocess/ImportGraphMaker.h
        preprocess/ImportGraphMaker.cpp
        utils/Benchmark.h
        utils/Benchmark.cpp
        utils/fwd/functional.h
        cst/utils/CSTUtils.h
        compiler/ASTDiagnoser.h
        compiler/ASTDiagnoser.cpp
        cst/utils/CSTUtils.cpp
        integration/ide/model/LexResult.h
        integration/ide/model/ImportUnit.h
        integration/ide/model/FlatIGFile.h
        integration/ide/model/ImportUnitCache.h
        preprocess/ImportGraphVisitor.h
        preprocess/ImportPathHandler.h
        preprocess/ImportPathHandler.cpp
        preprocess/CSTSymbolResolver.h
        preprocess/CSTSymbolResolver.cpp
        preprocess/BaseSymbolResolver.h
        server/analyzers/GotoDefAnalyzer.h
        integration/common/Location.h
        integration/common/Range.h
        server/analyzers/GotoDefAnalyzer.cpp
        server/analyzers/HoverAnalyzer.h
        server/analyzers/HoverAnalyzer.cpp
        compiler/PrimitiveTypeMap.h
        compiler/PrimitiveTypemap.cpp
        server/analyzers/Documentation.h
        preprocess/2cASTVisitor.h
        preprocess/2cASTVisitor.cpp
        ast/utils/CommonVisitor.cpp
        ast/utils/CommonVisitor.h
        preprocess/RepresentationVisitor.cpp
        preprocess/utils/RepresentationUtils.h
        preprocess/utils/RepresentationUtils.cpp
        compiler/ASTProcessorOptions.h
        compiler/ASTProcessor.h
        compiler/ASTDiag.h
        compiler/ASTProcessor.cpp
        utils/PathUtils.h
        integration/ide/bindings/SourceProviderCBI.h
        integration/ide/bindings/LexerCBI.h
        integration/ide/bindings/CBI.cpp
        lexer/model/CompilerBinder.h
        lexer/CompilerBinder.cpp
        lexer/model/CompilerBinderTCC.h
        lexer/model/CompilerBinderCommon.h
        ast/utils/ASTUtils.h
        ast/utils/ASTUtils.cpp
        integration/libtcc/LibTccInteg.h
        integration/libtcc/LibTcc.cpp
        utils/Version.h
        ast/base/Annotation.h
        ast/base/Annotation.cpp
        ast/base/AnnotableNode.h
        ast/base/AnnotationKind.h
        ast/structures/BaseFunctionType.h
        ast/structures/BaseFunctionType.cpp
        ast/base/AnnotationParent.h
        ast/structures/ExtensionFunction.h
        ast/base/ExtendableBase.h
        ast/base/ExtendableBase.cpp
        ast/base/ExtendableMembersContainerNode.h
        ast/structures/ExtensionFuncReceiver.h
        ast/structures/ExtensionFunction.cpp
        ast/structures/BaseFunctionParam.h
        preprocess/ShrinkingVisitor.h
        preprocess/ShrinkingVisitor.cpp
        compiler/OutputMode.h
        compiler/CodegenEmitterOptions.h
        ast/statements/ThrowStatement.h
        ast/statements/ThrowStatement.cpp
        ast/structures/Namespace.h
        ast/structures/Namespace.cpp
        lexer/structures/Namespace.cpp
        ast/base/ASTNode.cpp
        ast/values/SizeOfValue.h
        ast/values/SizeOfValue.cpp
        std/chem_string.h
        ast/structures/UnionDef.h
        ast/structures/UnionDef.cpp
        lexer/structures/Union.cpp
        std/hybrid_ptr.h
        ast/types/UnionType.h
        ast/types/UnionType.cpp
        ast/structures/VariablesContainer.h
        ast/structures/BaseDefMember.h
        ast/structures/UnnamedUnion.h
        ast/values/NamespaceIdentifier.h
        ast/types/ReferencedValueType.h
        ast/types/LiteralType.h
        ast/types/Types.cpp
        ast/types/ArrayType.cpp
        ast/base/ChainValue.h
        ast/base/ASTAny.h
        ast/base/ASTAny.cpp
        ast/base/ChainPart.h
        ast/statements/UsingStmt.h
        ast/statements/Using.cpp
        ast/statements/DeleteStmt.h
        ast/statements/DeleteStmt.cpp
        ast/values/RetStructParamValue.h
        ast/values/RetStructParamValue.cpp
        lexer/utils/AnnotationModifiers.h
        lexer/utils/ValueCreators.h
        lexer/utils/MacroLexers.h
        integration/ide/bindings/BuildContextCBI.h
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
        cst/CSTToken.cpp
        ast/utils/GenericUtils.h
        ast/values/TypeLinkedValue.h
        ast/values/TypeLinkedValue.cpp
        compiler/lab/LabBuildCompilerOptions.h
        compiler/lab/Utils.h
)

# Chemical LSP Executable Target
add_executable(ChemicalLsp Main.cpp ${COMMON_SOURCES}
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
        server/analyzers/DocumentLinksAnalyzer.h
        server/analyzers/DocumentLinksAnalyzer.cpp
        server/utils/WorkspaceImportGraphImporter.h
)

# Add an executable target for the second executable
add_executable(Interpreter Interpreter.cpp ${COMMON_SOURCES})

set(COMPILER_SOURCES
        compiler/Codegen.h
        compiler/Codegen.cpp
        compiler/clang_cc1main.cpp
        compiler/clang_driver.cpp
        compiler/clang_cc1as_main.cpp
        compiler/llvmfwd.h
        compiler/llvmimpl.h
        compiler/ctranslator/TranslateC.cpp
        compiler/ctranslator/CTranslator.h
        compiler/ctranslator/CTranslator.cpp
        compiler/chem_clang.h
        compiler/chem_clang.cpp
        compiler/llvmbackend/LLVM.cpp
        compiler/ASTCompiler.cpp
        compiler/ASTCompiler.h
        compiler/llvm-ar.cpp
        utils/ProcessUtils.h
)

# add Compiler executable
add_executable(Compiler Compiler.cpp ${COMMON_SOURCES} ${COMPILER_SOURCES})

# add TCC Compiler executable
add_executable(TCCCompiler Compiler.cpp ${COMMON_SOURCES})

# status messages
message(STATUS "FEATURES ${COMPILER_FEATURES}")
# Indicate which libraries are present in these vars
message(STATUS "LLvm Include Dirs ${LLVM_INCLUDE_DIRS}")
message(STATUS "LLVM Libs Dirs ${LLVM_LIBDIRS}")
message(STATUS "LLvm Libs ${LLVM_LIBRARIES}")
message(STATUS "Clang Libs ${CLANG_LIBRARIES}")
message(STATUS "LLD Libs ${LLD_LIBRARIES}")

# Compiler uses llvm
target_compile_definitions(Compiler PRIVATE COMPILER_BUILD CLANG_LIBS LLD_LIBS ${COMPILER_FEATURES})

# TCC Compiler does not use llvm and clang
target_compile_definitions(TCCCompiler PRIVATE TCC_BUILD)

# Lsp uses boost library
target_compile_definitions(ChemicalLsp PRIVATE LSP_BUILD)

# Define common include directories
set(COMMON_INCLUDE_DIRS
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/lib/tsl-ordered-map
        ${CMAKE_CURRENT_SOURCE_DIR}/lib/ctpl
        ${CMAKE_CURRENT_SOURCE_DIR}/lib/rang
)

# Add include directories for LspCpp
target_include_directories(ChemicalLsp PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/LspCpp/include
        ${LIBTCC_DIR}/include
        ${COMMON_INCLUDE_DIRS}
)

# Add include directories for LspCpp
target_include_directories(Interpreter PRIVATE
        ${COMMON_INCLUDE_DIRS}
)

# Add include directories for Compiler
target_include_directories(Compiler PRIVATE
        ${COMMON_INCLUDE_DIRS}
        # Include llvm directories for the compiler executable
        ${LLVM_INCLUDE_DIRS}
        ${LLD_INCLUDE_DIRS}
        ${CLANG_INCLUDE_DIRS}
        ${LIBTCC_DIR}/include
)

# Add include directories for TCCCompiler
target_include_directories(TCCCompiler PRIVATE
        ${COMMON_INCLUDE_DIRS}
        ${LIBTCC_DIR}/include
)

# find the libraries that correspond to llvm
# llvm_map_components_to_libnames(LLVM_EXTRA_LIBS Option)

# Indicate which libraries are present in these vars
message(STATUS "LLvm Include Dirs ${LLVM_INCLUDE_DIRS}")
message(STATUS "LLVM Libs Container ${LLVM_LIBDIRS}")
message(STATUS "LLvm Libs ${LLVM_LIBRARIES}")
message(STATUS "Clang Libs ${CLANG_LIBRARIES}")
message(STATUS "LLD Libs ${LLD_LIBRARIES}")

if (MSVC)
    # Disable warning C4267: 'argument': conversion from 'size_t' to '...', possible loss of data
    # For Microsoft Visual Studio Compiler Only
    target_compile_options(Compiler PRIVATE /wd4267)
    target_compile_options(TCCCompiler PRIVATE /wd4267)
    target_compile_options(Interpreter PRIVATE /wd4267)
    target_compile_options(ChemicalLsp PRIVATE /wd4267)
endif ()

# https://discourse.llvm.org/t/undefined-reference-to-typeinfo-for-llvm-genericoptionvalue/71526/3
set(NO_RTTI "-fno-rtti")
target_compile_options(Compiler PRIVATE ${NO_RTTI})
if (WIN32)
    target_compile_options(Compiler PRIVATE "/MT")
endif ()

# Linking interpreter with Boost library
# This should be removed eventually we don't want boost to boost our exe size
target_link_libraries(Interpreter)

# Link compiler against CLANG, LLVM, LLD, LibTCC
target_link_libraries(Compiler ${CLANG_LIBRARIES} ${LLVM_LIBRARIES} ${LLD_LIBRARIES} ${LIBTCC_LIB})

# Link TCC Compiler against LibTCC
target_link_libraries(TCCCompiler ${LIBTCC_LIB})

# Link Lsp with Boost Library
target_link_libraries(ChemicalLsp Boost::program_options Boost::filesystem lspcpp ${LIBTCC_LIB})

# Set to C++ 20
message(STATUS "Setting Standard C++ 20")
set_property(TARGET ChemicalLsp PROPERTY CXX_STANDARD 20)
set_property(TARGET Interpreter PROPERTY CXX_STANDARD 20)
set_property(TARGET Compiler PROPERTY CXX_STANDARD 20)
set_property(TARGET TCCCompiler PROPERTY CXX_STANDARD 20)