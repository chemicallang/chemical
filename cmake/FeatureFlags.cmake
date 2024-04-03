# Define options for each feature
set(FEAT_ASSEMBLY_GEN OFF CACHE BOOL  "Enable Assembly File Generation / Supporting .s extension as output")
set(FEAT_BITCODE_GEN OFF CACHE BOOL  "Enable Bitcode File Generation / Supporting .bc extension as output")
set(FEAT_LLVM_IR_GEN ON CACHE BOOL  "Enable LLVM IR Generation / Supporting .ll extension as output")

# Create an empty list to store enabled features
set(COMPILER_FEATURES "")

# Add enabled features to the list
if(FEAT_ASSEMBLY_GEN)
    message(STATUS "Assembly generation enabled, supporting .s files as output")
    list(APPEND COMPILER_FEATURES "FEAT_ASSEMBLY_GEN")
endif()

if(FEAT_BITCODE_GEN)
    message(STATUS "Bitcode generation enabled, supporting .bc files as output")
    list(APPEND COMPILER_FEATURES "FEAT_BITCODE_GEN")
endif()

if(FEAT_LLVM_IR_GEN)
    message(STATUS "LLVM IR generation enabled, supporting .ll files as output")
    list(APPEND COMPILER_FEATURES "FEAT_LLVM_IR_GEN")
endif()
