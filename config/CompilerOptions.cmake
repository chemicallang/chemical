# Check if compiler supports __cplusplus macro update
# This is required because visual studio doesn't enable it by default
# https://stackoverflow.com/questions/74367383/cplusplus-apparently-not-set-correctly-in-visual-studio-2022-when-building-for
# so yeah if you wanna check for __cplusplus then this code snippet is required
include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-Zc:__cplusplus" COMPILER_SUPPORTS_ZC_CPLUSPLUS)
if(COMPILER_SUPPORTS_ZC_CPLUSPLUS)
    add_compile_options("-Zc:__cplusplus")
endif()