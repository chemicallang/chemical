// fs.fs    -- filesystem library for the language
// Designed: cross-platform (POSIX + Windows), no heap allocations (caller buffers / stack buffers), uses Option/Result
// You supply the OS-specific externs and structs listed below (see comment block "OS-SPECIFIC EXTERNS & STRUCTS").

// Result<UnitTy, MyError> doesn't work
// because we can't contain the void type in our struct
// so we are going to use a Unit Type
public struct UnitTy {}
// TODO: top level struct values in constants not yet supported
// public comptime const Unit = UnitTy {}

// TODO: not yet supported to be inside namespace fs
public comptime const PATH_MAX_BUF = 4096;     // max path buffer for POSIX-style
public comptime const WIN_MAX_PATH = 32768;    // wide path limit for Windows (extended)
public comptime const SMALL_STACK_BUF = 4096;  // general-purpose stack buffer
public comptime const COPY_CHUNK = 64 * 1024;  // 64 KB copy chunk

public namespace fs {

// ----------------------
// Configuration constants
// ----------------------
public comptime const PATH_MAX_BUF = 4096;
public comptime const WIN_MAX_PATH = 32768;
public comptime const COPY_CHUNK = 64 * 1024; // 64 KiB
public comptime const DIR_ENT_NAME_MAX = 256;
public comptime const TEMP_NAME_MAX = 64;

} // end namespace fs