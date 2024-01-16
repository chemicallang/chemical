cmake_minimum_required(VERSION 3.24)
project(Compiler)

set(CMAKE_CXX_STANDARD 17)

# Set the Boost root directory
set(BOOST_ROOT "D:/Software/Installed/boost_1_84_0")

# Avoid New Boost Version Warning
set(Boost_NO_WARN_NEW_VERSIONS 1)

# Add Boost include directories
find_package(Boost REQUIRED COMPONENTS date_time chrono filesystem system thread program_options)

# Add the LspCpp library directory
add_subdirectory(LspCpp)

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

# Include the LspCpp headers explicitly
target_include_directories(Compiler PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/LspCpp/include
        # Add Boost include directories
        ${Boost_INCLUDE_DIRS}
        # Add any other necessary include directories from LspCpp
)

# Link against the LspCpp library
target_link_libraries(Compiler PRIVATE lspcpp)

# Link against Boost libraries
target_link_libraries(Compiler PRIVATE ${Boost_LIBRARIES})