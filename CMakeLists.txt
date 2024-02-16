# CMakeList.txt : CMake project for ChemicalVS, include source and define
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
# Set custom boost root
SET(BOOST_ROOT "D:\\Software\\Installed\\boost_1_84_0")


find_package(Boost 1.84 COMPONENTS program_options filesystem REQUIRED)

# Add the LspCpp library directory
add_subdirectory(LspCpp)

# Add source to this project's executable.
add_executable(ChemicalVS "lexer/Lexer.cpp" "lexer/Lexer.h" "lexer/model/AssignmentOperatorToken.h" "lexer/model/IdentifierToken.h" "lexer/model/IntToken.h" "lexer/model/KeywordToken.h" "lexer/model/LexToken.h" "lexer/model/SemiColonToken.h" "lexer/model/WhitespaceToken.h" "SourceProvider.cpp" "SourceProvider.h"
        utils/Lexi.h
        Main.cpp
        utils/PrintUtils.h
        lexer/minLsp/SemanticTokens.h
        lexer/LexConfig.h
        lexer/statements/LexAssignment.cpp
        lexer/statements/LexDeclaration.cpp
        lexer/statements/LexDeclaration.cpp
        lexer/statements/LexPreprocess.cpp
        lexer/statements/LexStatement.cpp
        lexer/utils/LexWhitespace.cpp
        lexer/utils/LexString.cpp
        lexer/utils/LexNumber.cpp
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
