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


find_package(Boost 1.84 COMPONENTS program_options filesystem REQUIRED)

# Add the LspCpp library directory
add_subdirectory(LspCpp)

# Add path to this project's executable.
add_executable(ChemicalVS "lexer/Lexer.cpp" "lexer/Lexer.h" "lexer/model/tokens/CharOperatorToken.h" "lexer/model/tokens/VariableToken.h" "lexer/model/tokens/IntToken.h" "lexer/model/tokens/KeywordToken.h" "lexer/model/tokens/LexToken.h" "lexer/model/tokens/WhitespaceToken.h" "SourceProvider.h"
        utils/Lexi.h
        Main.cpp
        utils/PrintUtils.h
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
        server/FileTracker.h
        server/FileTracker.cpp
        StreamSourceProvider.h
        StreamPosition.h
        utils/Lexi.cpp
        utils/PrintUtils.cpp
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
        lexer/statements/Conditional.cpp
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
)

# Add include directories for LspCpp
target_include_directories(ChemicalVS PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/LspCpp/include
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# Link with Boost Library
target_link_libraries(ChemicalVS Boost::program_options Boost::filesystem lspcpp)


if (CMAKE_VERSION VERSION_GREATER 3.12)
    set_property(TARGET ChemicalVS PROPERTY CXX_STANDARD 20)
endif ()

# TODO: Add tests and install targets if needed.
