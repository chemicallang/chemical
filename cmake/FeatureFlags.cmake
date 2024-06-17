# Define options for each feature
set(FEAT_JUST_IN_TIME OFF CACHE BOOL  "Enable LLVM JIT")

# Create an empty list to store enabled features
set(COMPILER_FEATURES "")

if(FEAT_JUST_IN_TIME)
    message(STATUS "LLVM JIT Enabled")
    list(APPEND COMPILER_FEATURES "FEAT_JUST_IN_TIME")
endif()
