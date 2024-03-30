# CMakeList.txt : CMake project for chemical, include path and define
# project specific logic here.
#
cmake_minimum_required(VERSION 3.8)
message("CMake version: ${CMAKE_VERSION}")
project("chemical")

# use default out/host or out-win/host folder if cmake prefix path is not given
if(NOT DEFINED CMAKE_PREFIX_PATH)
    if(WIN32)
        set(CMAKE_PREFIX_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../out-win/host")
    else()
        set(CMAKE_PREFIX_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../out/host")
    endif()
endif()

# We copied this from Zig CMakeLists
set(ZIG_USE_LLVM_CONFIG ON)
set(CMAKE_USER_MAKE_RULES_OVERRIDE ${CMAKE_CURRENT_SOURCE_DIR}/cmake/c_flag_overrides.cmake)
set(CMAKE_USER_MAKE_RULES_OVERRIDE_CXX ${CMAKE_CURRENT_SOURCE_DIR}/cmake/cxx_flag_overrides.cmake)
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})

# Debug builds error messages that are more helpful
set(CMAKE_BUILD_TYPE "Debug")
if(CMAKE_BUILD_TYPE MATCHES Debug)
    add_definitions(-DDEBUG)
endif()

# Verbose builds produce logs as well
option(VERBOSE "Enable verbose output (for debugging)" OFF)
if(VERBOSE)
    add_definitions(-DVERBOSE) # Define VERBOSE macro
endif()

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
if(CLANG_LIBRARIES AND CLANG_LIBRARIES MATCHES "CLANG_LIBRARIES-NOTFOUND")
    # Remove NOTFOUND from CLANG_LIBRARIES
    string(REPLACE "CLANG_LIBRARIES-NOTFOUND;" "" CLANG_LIBRARIES "${CLANG_LIBRARIES}")
endif()

# Add the LspCpp library directory
add_subdirectory(LspCpp)

# Add your common source files (excluding the main files)
set(COMMON_SOURCES
        lexer/Lexer.cpp
        lexer/Lexer.h
        lexer/model/tokens/CharOperatorToken.h
        lexer/model/tokens/VariableToken.h
        lexer/model/tokens/NumberToken.h
        lexer/model/tokens/KeywordToken.h
        lexer/model/tokens/LexToken.h
        lexer/model/tokens/RawToken.h
        stream/SourceProvider.h
        lexer/Lexi.h
        lexer/statements/LexAssignment.cpp
        lexer/statements/VarInitialization.cpp
        lexer/statements/VarInitialization.cpp
        lexer/statements/LexStatement.cpp
        lexer/utils/LexWhitespace.cpp
        lexer/utils/LexString.cpp
        lexer/utils/LexNumber.cpp
        lexer/statements/LexType.cpp
        lexer/model/tokens/TypeToken.h
        stream/StreamSourceProvider.h
        stream/StreamPosition.h
        lexer/Lexi.cpp
        lexer/utils/LexValue.cpp
        compiler/clang_driver.cpp
        utils/JsonUtils.cpp
        utils/JsonUtils.h
        utils/FileUtils.h
        utils/FileUtils.cpp
        lexer/model/tokens/StringToken.h
        lexer/model/tokens/CharToken.h
        lexer/statements/Comment.cpp
        lexer/model/tokens/CommentToken.h
        lexer/utils/Helpers.cpp
        lexer/statements/AccessChain.cpp
        lexer/model/tokens/MultilineCommentToken.h
        lexer/structures/ForBlock.cpp
        lexer/model/tokens/StringOperatorToken.h
        lexer/model/tokens/StringOperatorToken.h
        lexer/structures/WhileBlock.cpp
        lexer/structures/IfBlock.cpp
        lexer/structures/Function.cpp
        lexer/utils/Expression.cpp
        lexer/structures/Block.cpp
        lexer/statements/Import.cpp
        lexer/structures/Enum.cpp
        lexer/model/tokens/FunctionToken.h
        lexer/model/tokens/ParameterToken.h
        lexer/model/tokens/PropertyToken.h
        lexer/model/tokens/StructToken.h
        lexer/model/tokens/ModifierToken.h
        lexer/model/tokens/ClassToken.h
        lexer/model/tokens/InterfaceToken.h
        lexer/model/tokens/MethodToken.h
        lexer/model/tokens/AbstractStringToken.h
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
        parser/Parser.h
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
        parser/Parser.cpp
        parser/statements/VarInit.cpp
        parser/values/Value.cpp
        lexer/model/LexTokenType.h
        utils/Utils.h
        utils/Utils.cpp
        parser/Persi.cpp
        lexer/utils/TypeUtils.h
        lexer/utils/TypeUtils.cpp
        parser/utils/Consumers.cpp
        parser/statements/VarAssign.cpp
        ast/structures/Scope.h
        parser/structures/Scope.cpp
        ast/values/AccessChain.h
        parser/expression/AccessChain.cpp
        ast/values/FunctionCall.h
        ast/values/IndexOperator.h
        parser/expression/Expression.cpp
        ast/values/NotValue.h
        parser/statements/If.cpp
        parser/structures/ForLoop.cpp
        parser/structures/WhileLoop.cpp
        parser/structures/DoWhileLoop.cpp
        parser/structures/FunctionDefinition.cpp
        parser/structures/EnumDefinition.cpp
        parser/structures/InterfaceDefinition.cpp
        parser/structures/StructDefinition.cpp
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
        parser/statements/Import.cpp
        ast/base/LoopASTNode.h
        ast/structures/ImplDefinition.h
        lexer/structures/ImplDef.cpp
        parser/structures/ImplDef.cpp
        ast/statements/Comment.h
        parser/statements/Comment.cpp
        ast/base/Interpretable.h
        server/SemanticLinker.cpp
        server/SemanticLinker.h
        lexer/statements/AnnotationMacro.cpp
        lexer/model/tokens/MacroToken.h
        server/SemanticAnalyzer.h
        server/FoldingRangeAnalyzer.cpp
        server/FoldingRangeAnalyzer.h
        server/CompletionItemAnalyzer.h
        server/CompletionItemAnalyzer.cpp
        server/ScopeAnalyzer.h
        server/ScopeAnalyzer.cpp
        parser/utils/ValueAndOperatorStack.h
        compiler/Codegen.h
        compiler/Codegen.cpp
        ast/base/BaseType.h
        compiler/Operation.cpp
        compiler/ASTLinker.h
        compiler/ASTLinker.cpp
        compiler/ASTPointer.h
        compiler/Branch.cpp
        ast/types/Int32Type.h
        ast/types/FloatType.h
        ast/types/StringType.h
        ast/types/DoubleType.h
        ast/values/TernaryValue.h
        typecheck/TypeChecker.h
        typecheck/TypeChecker.cpp
        ast/types/BoolType.h
        ast/types/CharType.h
        ast/types/ArrayType.h
        parser/utils/Type.cpp
        ast/base/Visitor.h
        ast/types/VoidType.h
        ast/types/AnyType.h
        utils/CmdUtils.h
        compiler/Object.cpp
        compiler/Linker.cpp
        common/Diagnostic.h
        lexer/values/StructValue.cpp
        ast/values/StructValue.h
        parser/values/StructValue.cpp
        ast/types/ReferencedType.h
        stream/SourceProviderImpl.cpp
        ast/structures/FunctionDecl.cpp
        ast/utils/GlobalFunctions.h
        ast/utils/GlobalFunctions.cpp
        ast/statements/MacroValueStatement.h
        parser/statements/AnnotationMacro.cpp
        ast/utils/InterpretValues.h
        common/Diagnostic.cpp
        lexer/structures/Collection.cpp
        stream/StreamStructValue.h
        stream/StreamStructValue.cpp
        lexer/model/tokens/UserToken.h
        ast/structures/Scope.cpp
        ast/structures/LoopScope.cpp
        ast/types/PointerType.h
        ast/types/GenericType.h
        ast/base/Value.cpp
        ast/statements/VarInit.cpp
        ast/structures/StructDefinition.cpp
        ast/values/IntValue.cpp
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
        ast/types/AnyType.cpp
        ast/values/FunctionCall.cpp
        ast/values/BoolValue.cpp
        ast/values/DoubleValue.cpp
        ast/values/FloatValue.cpp
        ast/types/ArrayType.cpp
        ast/types/BoolType.cpp
        ast/types/CharType.cpp
        ast/types/DoubleType.cpp
        ast/types/FloatType.cpp
        ast/types/Int32Type.cpp
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
        parser/statements/Typealias.cpp
        ast/statements/Continue.cpp
        ast/statements/Return.cpp
        ast/statements/SwitchStatement.cpp
        ast/statements/SwitchStatement.h
        ast/structures/TryCatch.cpp
        ast/structures/TryCatch.h
        lexer/structures/TryCatch.cpp
        lexer/structures/Switch.cpp
        parser/structures/Switch.cpp
        parser/structures/TryCatch.cpp
)

# Chemical LSP Executable Target
add_executable(ChemicalLsp Main.cpp ${COMMON_SOURCES}
        server/FileTracker.h
        server/FileTracker.cpp
        server/PrintUtils.h
        server/PrintUtils.cpp
        server/LspSemanticTokens.cpp
)

# Add an executable target for the second executable
add_executable(Interpreter Interpreter.cpp ${COMMON_SOURCES})

# add chemical llvm executable
add_executable(Compiler Compiler.cpp ${COMMON_SOURCES}
        compiler/clang_cc1main.cpp
        compiler/clang_driver.cpp
        compiler/clang_cc1as_main.cpp
        compiler/InvokeExtension.cpp
        compiler/cc1gen_reproducer_main.cpp
        compiler/JitInterpret.cpp
        compiler/llvmfwd.h
        compiler/llvmimpl.h
)

# Compiler uses llvm
target_compile_definitions(Compiler PRIVATE COMPILER_BUILD)
# Lsp uses boost library
target_compile_definitions(ChemicalLsp PRIVATE LSP_BUILD)

# Add include directories for LspCpp
target_include_directories(ChemicalLsp PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/LspCpp/include
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# Add include directories for LspCpp
target_include_directories(Interpreter PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Add include directories for Compiler
target_include_directories(Compiler PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        # Include llvm directories for the compiler executable
        ${LLVM_INCLUDE_DIRS}
        ${LLD_INCLUDE_DIRS}
        ${CLANG_INCLUDE_DIRS}
)

# find the libraries that correspond to llvm
# llvm_map_components_to_libnames(LLVM_EXTRA_LIBS Option)

# Indicate which libraries are present in these vars
message(STATUS "LLvm Libs ${LLVM_LIBRARIES}")
message(STATUS "Clang Libs ${CLANG_LIBRARIES}")
message(STATUS "LLD Libs ${LLD_LIBRARIES}")

if (MSVC)
    # Disable warning C4267: 'argument': conversion from 'size_t' to '...', possible loss of data
    # For Microsoft Visual Studio Compiler Only
    target_compile_options(Compiler PRIVATE /wd4267)
    target_compile_options(Interpreter PRIVATE /wd4267)
    target_compile_options(ChemicalLsp PRIVATE /wd4267)
endif()

# https://discourse.llvm.org/t/undefined-reference-to-typeinfo-for-llvm-genericoptionvalue/71526/3
set(NO_RTTI "-fno-rtti")
target_compile_options(Compiler PRIVATE ${NO_RTTI})
if(WIN32)
    target_compile_options(Compiler PRIVATE "/MT")
endif()

# Linking interpreter with Boost library
# This should be removed eventually we don't want boost to boost our exe size
target_link_libraries(Interpreter)

# Link compiler against CLANG, LLVM, LLD
target_link_libraries(Compiler ${CLANG_LIBRARIES} ${LLVM_LIBRARIES} ${LLD_LIBRARIES})

# Link Lsp with Boost Library
target_link_libraries(ChemicalLsp Boost::program_options Boost::filesystem lspcpp)

# Set to C++ 20
message(STATUS "Setting Standard C++ 20")
set_property(TARGET ChemicalLsp PROPERTY CXX_STANDARD 20)
set_property(TARGET Interpreter PROPERTY CXX_STANDARD 20)
set_property(TARGET Compiler PROPERTY CXX_STANDARD 20)