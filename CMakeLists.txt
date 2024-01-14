cmake_minimum_required(VERSION 3.26)
project(Compiler)

set(CMAKE_CXX_STANDARD 17)

# Add the LspCpp library directory
add_subdirectory(LspCpp)

# Add Boost include directories
find_package(Boost)

# Compiler Sources
add_executable(Compiler main.cpp
        SourceProvider.cpp
        SourceProvider.h
        lexer/Lexer.cpp
        lexer/Lexer.h
        lexer/model/KeywordToken.h
        lexer/model/LexToken.h
        lexer/model/IntToken.h
)

# Link against the LspCpp library
target_link_libraries(Compiler PRIVATE lspcpp)

# Include the LspCpp headers explicitly
target_include_directories(Compiler PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/LspCpp/include
        # Add Boost include directories
        ${Boost_INCLUDE_DIRS}
        # Add any other necessary include directories from LspCpp
)

# Link against Boost libraries
target_link_libraries(Compiler PRIVATE ${Boost_LIBRARIES})