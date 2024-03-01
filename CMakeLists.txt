# CMakeList.txt : CMake project for ChemicalVS, include path and define
# project specific logic here.
#
cmake_minimum_required(VERSION 3.8)

project("ChemicalVS")

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

# Set CMP0074 policy so that CMake can use BOOST_ROOT environment variable
if (POLICY CMP0074)
    cmake_policy(SET CMP0074 NEW)
endif()

# Find Boost
find_package(Boost 1.84 COMPONENTS program_options filesystem REQUIRED)

# Add the LspCpp library directory
add_subdirectory(LspCpp)

if(MSVC)
    message(STATUS "Using VS at $ENV{VSINSTALLDIR}")
    set(MSVC_DIA_SDK_DIR "$ENV{VSINSTALLDIR}DIA SDK")
    if(IS_DIRECTORY ${MSVC_DIA_SDK_DIR})
        set(ZIG_DIA_GUIDS_LIB "${MSVC_DIA_SDK_DIR}/lib/amd64/diaguids.lib")
        string(REGEX REPLACE "\\\\" "\\\\\\\\" ZIG_DIA_GUIDS_LIB_ESCAPED "${ZIG_DIA_GUIDS_LIB}")
    endif()
endif()

# Add your common source files (excluding the main files)
set(COMMON_SOURCES "lexer/Lexer.cpp" "lexer/Lexer.h" "lexer/model/tokens/CharOperatorToken.h" "lexer/model/tokens/VariableToken.h" "lexer/model/tokens/NumberToken.h" "lexer/model/tokens/KeywordToken.h" "lexer/model/tokens/LexToken.h" "lexer/model/tokens/WhitespaceToken.h" "SourceProvider.h"
        lexer/Lexi.h
        lexer/minLsp/SemanticTokens.h
        lexer/LexConfig.h
        lexer/statements/LexAssignment.cpp
        lexer/statements/VarInitialization.cpp
        lexer/statements/VarInitialization.cpp
        lexer/statements/LexStatement.cpp
        lexer/utils/LexWhitespace.cpp
        lexer/utils/LexString.cpp
        lexer/utils/LexNumber.cpp
        lexer/statements/LexType.cpp
        lexer/model/tokens/TypeToken.h
        StreamSourceProvider.h
        StreamPosition.h
        lexer/Lexi.cpp
        lexer/model/LexError.h
        lexer/model/LexError.h
        lexer/model/LexError.h
        lexer/utils/LexValue.cpp
        lexer/model/TokenPosition.h
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
        parser/utils/Operation.h
        parser/utils/Operation.cpp
        parser/structures/ForLoop.cpp
        parser/structures/WhileLoop.cpp
        parser/structures/DoWhileLoop.cpp
        parser/structures/FunctionDefinition.cpp
        parser/structures/EnumDefinition.cpp
        parser/structures/InterfaceDefinition.cpp
        parser/structures/StructDefinition.cpp
        ast/values/IncDecValue.h
        ast/values/BoolValue.h
)

# Chemical LSP Executable Target
add_executable(ChemicalLsp Main.cpp ${COMMON_SOURCES}
        server/FileTracker.h
        server/FileTracker.cpp
        server/PrintUtils.h
        server/PrintUtils.cpp
)

# Add an executable target for the second executable
add_executable(Compiler Compiler.cpp ${COMMON_SOURCES})

# Add include directories for LspCpp
target_include_directories(ChemicalLsp PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/LspCpp/include
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}
)
# Add include directories for LspCpp
target_include_directories(Compiler PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# Link with Boost Library
target_link_libraries(ChemicalLsp Boost::program_options Boost::filesystem lspcpp)

# Set to C++ 20
if (CMAKE_VERSION VERSION_GREATER 3.12)
    set_property(TARGET ChemicalLsp PROPERTY CXX_STANDARD 20)
    set_property(TARGET Compiler PROPERTY CXX_STANDARD 20)
endif ()

# TODO: Add tests and install targets if needed.
