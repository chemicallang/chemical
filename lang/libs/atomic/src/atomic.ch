if(intrinsics::get_backend_name() == "C") {
    comptime {
        intrinsics::emit("#" + "ifdef _WIN32\n#" + "include \"" + intrinsics::get_libs_dir() + "/c/atomic/win/atomic.h\"\n#" + "else\n#" + "include \"" + intrinsics::get_libs_dir() + "/c/atomic/nix/atomic.h\"\n#" + "endif")
    }
}

enum memory_order {
    relaxed,
    consume,
    acquire,
    release,
    acq_rel,
    seq_cst
};

private comptime func signal_arg_call(name : *char, arg : any) : any {
    return intrinsics::wrap(intrinsics::multiple(intrinsics::raw(name), intrinsics::raw("("), arg, intrinsics::raw(")")))
}

private comptime func two_arg_call(name : *char, arg1 : any, arg2 : any) : any {
    return intrinsics::wrap(intrinsics::multiple(intrinsics::raw(name),intrinsics::raw("("),arg1,intrinsics::raw(", "),arg2,intrinsics::raw(")")))
}

private comptime func three_arg_call(name : *char, arg1 : any, arg2 : any, arg3 : any) : any {
    return intrinsics::wrap(intrinsics::multiple(intrinsics::raw(name),intrinsics::raw("("),arg1,intrinsics::raw(", "),arg2,intrinsics::raw(", "),arg3,intrinsics::raw(")")))
}

private comptime func four_arg_call(name : *char, arg1 : any, arg2 : any, arg3 : any, arg4 : any) : any {
    return intrinsics::wrap(intrinsics::multiple(intrinsics::raw(name),intrinsics::raw("("),arg1,intrinsics::raw(", "),arg2,intrinsics::raw(", "),arg3,intrinsics::raw(", "),arg4,intrinsics::raw(")")))
}

// ------- u64 functions ----------

public comptime func atomic_load_u64(x : *u64) : u64 {
    // unsigned long long atomic_load_u64(unsigned long long* x)
    return signal_arg_call("atomic_load_u64", x) as u64
}

public comptime func atomic_store_u64(x : *mut u64, y : u64) {
    // void atomic_store_u64(unsigned long long* x, unsigned long long y)
    return two_arg_call("atomic_store_u64", x, y) as void
}

public comptime func atomic_compare_exchange_weak_u64(x : *mut u64, expected : *mut u64, y : u64) : int {
    // int atomic_compare_exchange_weak_u64(unsigned long long* x, unsigned long long* expected, unsigned long long y)
    return three_arg_call("atomic_compare_exchange_weak_u64", x, expected, y) as int
}

public comptime func atomic_compare_exchange_strong_u64(x : *mut u64, expected : *mut u64, y : u64) : int {
    // int atomic_compare_exchange_strong_u64(unsigned long long* x, unsigned long long* expected, unsigned long long y)
    return three_arg_call("atomic_compare_exchange_strong_u64", x, expected, y) as int
}

public comptime func atomic_exchange_u64(x : *mut u64, y : u64) : u64 {
    // unsigned long long atomic_exchange_u64(unsigned long long* x, unsigned long long y)
    return two_arg_call("atomic_exchange_u64", x, y) as u64
}

public comptime func atomic_fetch_add_u64(x : *mut u64, y : u64) : u64 {
    // unsigned long long atomic_fetch_add_u64(unsigned long long* x, unsigned long long y)
    return two_arg_call("atomic_fetch_add_u64", x, y) as u64
}

public comptime func atomic_fetch_sub_u64(x : *mut u64, y : u64) : u64 {
    // unsigned long long atomic_fetch_sub_u64(unsigned long long* x, unsigned long long y)
    return two_arg_call("atomic_fetch_sub_u64", x, y) as u64
}

public comptime func atomic_fetch_and_u64(x : *mut u64, y : u64) : u64 {
    // unsigned long long atomic_fetch_and_u64(unsigned long long* x, unsigned long long y)
    return two_arg_call("atomic_fetch_and_u64", x, y) as u64
}

public comptime func atomic_fetch_or_u64(x : *mut u64, y : u64) : u64 {
    // unsigned long long atomic_fetch_or_u64(unsigned long long* x, unsigned long long y)
    return two_arg_call("atomic_fetch_or_u64", x, y) as u64
}

public comptime func atomic_fetch_xor_u64(x : *mut u64, y : u64) : u64 {
    // unsigned long long atomic_fetch_xor_u64(unsigned long long* x, unsigned long long y)
    return two_arg_call("atomic_fetch_xor_u64", x, y) as u64
}

// ------- u32 functions ----------

public comptime func atomic_load_u32(x : *u32) : u32 {
    // unsigned atomic_load_u32(unsigned* x)
    return signal_arg_call("atomic_load_u32", x) as u32
}

public comptime func atomic_store_u32(x : *mut u32, y : u32) {
    // void atomic_store_u32(unsigned* x, unsigned y)
    return two_arg_call("atomic_store_u32", x, y) as void
}

public comptime func atomic_compare_exchange_weak_u32(x : *mut u32, expected : *mut u32, y : u32) : int {
    // int atomic_compare_exchange_weak_u32(unsigned* x, unsigned* expected, unsigned y)
    return three_arg_call("atomic_compare_exchange_weak_u32", x, expected, y) as int
}

public comptime func atomic_compare_exchange_strong_u32(x : *mut u32, expected : *mut u32, y : u32) : int {
    // int atomic_compare_exchange_strong_u32(unsigned* x, unsigned* expected, unsigned y)
    return three_arg_call("atomic_compare_exchange_strong_u32", x, expected, y) as int
}

public comptime func atomic_exchange_u32(x : *mut u32, y : u32) : u32 {
    // unsigned atomic_exchange_u32(unsigned* x, unsigned y)
    return two_arg_call("atomic_exchange_u32", x, y) as u32
}

public comptime func atomic_fetch_add_u32(x : *mut u32, y : u32) : u32 {
    // unsigned atomic_fetch_add_u32(unsigned* x, unsigned y)
    return two_arg_call("atomic_fetch_add_u32", x, y) as u32
}

public comptime func atomic_fetch_sub_u32(x : *mut u32, y : u32) : u32 {
    // unsigned atomic_fetch_sub_u32(unsigned* x, unsigned y)
    return two_arg_call("atomic_fetch_sub_u32", x, y) as u32
}

public comptime func atomic_fetch_and_u32(x : *mut u32, y : u32) : u32 {
    // unsigned atomic_fetch_and_u32(unsigned* x, unsigned y)
    return two_arg_call("atomic_fetch_and_u32", x, y) as u32
}

public comptime func atomic_fetch_or_u32(x : *mut u32, y : u32) : u32 {
    // unsigned atomic_fetch_or_u32(unsigned* x, unsigned y)
    return two_arg_call("atomic_fetch_or_u32", x, y) as u32
}

public comptime func atomic_fetch_xor_u32(x : *mut u32, y : u32) : u32 {
    // unsigned atomic_fetch_xor_u32(unsigned* x, unsigned y)
    return two_arg_call("atomic_fetch_xor_u32", x, y) as u32
}

// ------- u16 functions ----------

public comptime func atomic_load_u16(x : *u16) : u16 {
    // unsigned short atomic_load_u16(unsigned short* x)
    return signal_arg_call("atomic_load_u16", x) as u16
}

public comptime func atomic_store_u16(x : *mut u16, y : u16) {
    // void atomic_store_u16(void* x, unsigned short y)
    return two_arg_call("atomic_store_u16", x, y) as void
}

public comptime func atomic_compare_exchange_weak_u16(x : *mut u16, expected : *mut u16, y : u16) : int {
    // int atomic_compare_exchange_weak_u16(void* x, unsigned short* expected, unsigned short y)
    return three_arg_call("atomic_compare_exchange_weak_u16", x, expected, y) as int
}

public comptime func atomic_compare_exchange_strong_u16(x : *mut u16, expected : *mut u16, y : u16) : int {
    // int atomic_compare_exchange_strong_u16(unsigned short* x, unsigned short* expected, unsigned short y)
    return three_arg_call("atomic_compare_exchange_strong_u16", x, expected, y) as int
}

public comptime func atomic_exchange_u16(x : *mut u16, y : u16) : u16 {
    // unsigned short atomic_exchange_u16(unsigned short* x, unsigned short y)
    return two_arg_call("atomic_exchange_u16", x, y) as u16
}

public comptime func atomic_fetch_add_u16(x : *mut u16, y : u16) : u16 {
    // unsigned short atomic_fetch_add_u16(unsigned short* x, unsigned short y)
    return two_arg_call("atomic_fetch_add_u16", x, y) as u16
}

public comptime func atomic_fetch_sub_u16(x : *mut u16, y : u16) : u16 {
    // unsigned short atomic_fetch_sub_u16(unsigned short* x, unsigned short y)
    return two_arg_call("atomic_fetch_sub_u16", x, y) as u16
}

public comptime func atomic_fetch_and_u16(x : *mut u16, y : u16) : u16 {
    // unsigned short atomic_fetch_and_u16(unsigned short* x, unsigned short y)
    return two_arg_call("atomic_fetch_and_u16", x, y) as u16
}

public comptime func atomic_fetch_or_u16(x : *mut u16, y : u16) : u16 {
    // unsigned short atomic_fetch_or_u16(unsigned short* x, unsigned short y)
    return two_arg_call("atomic_fetch_or_u16", x, y) as u16
}

public comptime func atomic_fetch_xor_u16(x : *mut u16, y : u16) : u16 {
    // unsigned short atomic_fetch_xor_u16(unsigned short* x, unsigned short y)
    return two_arg_call("atomic_fetch_xor_u16", x, y) as u16
}


// ------- u8 functions ----------


public comptime func atomic_load_u8(x : *u8) : u8 {
    // unsigned char atomic_load_byte(unsigned char* x);
    return signal_arg_call("atomic_load_byte", x) as u8
}

public comptime func atomic_store_u8(x : *mut u8, y : u8) {
    // void atomic_store_byte(unsigned char* x, unsigned char y);
    return two_arg_call("atomic_store_byte", x, y) as void
}

public comptime func atomic_compare_exchange_weak_u8(x : *mut u8, expected : *mut u8, y : u8) : int {
    // int atomic_compare_exchange_weak_byte(unsigned char* x, unsigned char* expected, unsigned char y);
    return three_arg_call("atomic_compare_exchange_weak_byte", x, expected, y) as int
}

public comptime func atomic_compare_exchange_strong_u8(x : *mut u8, expected : *mut u8, y : u8) : int {
    // int atomic_compare_exchange_strong_byte(unsigned char* x, unsigned char* expected, unsigned char y);
    return three_arg_call("atomic_compare_exchange_strong_byte", x, expected, y) as int
}

public comptime func atomic_exchange_u8(x : *mut u8, y : u8) : u8 {
    // unsigned char atomic_exchange_byte(unsigned char* x, unsigned char y);
    return two_arg_call("atomic_exchange_byte", x, y) as u8
}

public comptime func atomic_fetch_add_u8(x : *mut u8, y : u8) : u8 {
    // unsigned char atomic_fetch_add_byte(unsigned char* x, unsigned char y);
    return two_arg_call("atomic_fetch_add_byte", x, y) as u8
}

public comptime func atomic_fetch_sub_u8(x : *mut u8, y : u8) : u8 {
    // unsigned char atomic_fetch_sub_byte(unsigned char* x, unsigned char y);
    return two_arg_call("atomic_fetch_sub_byte", x, y) as u8
}

public comptime func atomic_fetch_and_u8(x : *mut u8, y : u8) : u8 {
    // unsigned char atomic_fetch_and_byte(unsigned char* x, unsigned char y);
    return two_arg_call("atomic_fetch_and_byte", x, y) as u8
}

public comptime func atomic_fetch_or_u8(x : *mut u8, y : u8) : u8 {
    // unsigned char atomic_fetch_or_byte(unsigned char* x, unsigned char y);
    return two_arg_call("atomic_fetch_or_byte", x, y) as u8
}

public comptime func atomic_fetch_xor_u8(x : *mut u8, y : u8) : u8 {
    // unsigned char atomic_fetch_xor_byte(unsigned char* x, unsigned char y);
    return two_arg_call("atomic_fetch_xor_byte", x, y) as u8
}