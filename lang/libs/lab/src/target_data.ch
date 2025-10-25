public struct TargetData {

    // compilers
    var tcc : bool = false;
    var clang : bool = false;

    // job specific
    var cbi : bool = false;
    var lsp : bool = false;
    var test : bool = false;
    var debug : bool = false;
    var debug_quick : bool = false;
    var debug_complete : bool = false;
    var release : bool = false;
    var release_safe : bool = false;
    var release_small : bool = false;
    var release_fast : bool = false;

    var posix : bool = false;
    var gnu : bool = false;

    // basic platform flags
    var is64Bit : bool = false;
    var little_endian : bool = false;
    var big_endian : bool = false;
    var windows : bool = false;
    var win32 : bool = false;
    var win64 : bool = false;
    var linux : bool = false;
    var macos : bool = false;
    var freebsd : bool = false;
    var isUnix : bool = false;
    var android : bool = false;
    var cygwin : bool = false;
    var mingw32 : bool = false;
    var mingw64 : bool = false;
    var emscripten : bool = false;

    // architecture
    var x86_64 : bool = false;
    var x86 : bool = false;
    var i386 : bool = false;
    var arm : bool = false;
    var aarch64 : bool = false;
    var powerpc : bool = false;
    var powerpc64 : bool = false;
    var riscv : bool = false;
    var s390x : bool = false;
    var wasm32 : bool = false;
    var wasm64 : bool = false;

};