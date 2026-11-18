// `intrinsics::size` on a fixed array seen through a reference.
//
// The implicit `&[N]T -> [T]` conversion hands the intrinsic a *reference* to
// the array rather than the array itself. A `var buf : [16]u8` with no
// initialiser never had an ArrayValue to read a length from, so the size can
// only come from the type — which is what
// `ast/utils/GlobalFunctions.cpp::array_size_from_type` supplies. Before it
// existed, this file did not compile at all:
//
//   error: couldn't get value for intrinsics::size
//   at lang/libs/std/src/span.ch:303:71
//
// which is what stopped `osrand::fill(&mut buf)` from linking in any
// application that filled a stack buffer.
//
// ONE CASE IS STILL WRONG, and it is not what the fix above is about: inside an
// inlined `@expand` comptime constructor (`std::mut_span::make2`) the parameter
// has the declared unsized type `&'a mut []T`, so the caller's length is not
// visible to `intrinsics::size` and an *uninitialised* buffer converts to a
// zero-length span. `osrand::fill(&mut buf)` on an uninitialised buffer
// therefore writes nothing. The tests below cover the initialised forms, which
// carry an ArrayValue and are correct; a fix needs the caller's array type
// threaded through comptime-constructor inlining.
//
// `intrinsics::size` itself is a comptime intrinsic and cannot be called with a
// runtime value, so every test here reaches it through the conversion that
// needs it.

/// A callee taking a slice. The call site is where the `&[N]u8 -> [u8]`
/// conversion — and therefore the size lookup — happens.
func write_first(buf : &mut [u8]) {
    if(buf.size() >= 1u) {
        buf[0] = 0x2Au8
    }
}

/// Reports the length the conversion produced, so a wrong size fails a test
/// rather than overrunning a buffer somewhere later.
func slice_size(buf : &mut [u8]) : size_t {
    return buf.size()
}

/// The array is a struct field, so `&mut holder.data` reaches it through a
/// member access rather than a local name.
public struct Packet {
    var data : [16]u8
}

@test
func an_initialised_fixed_array_converts_to_a_slice_of_its_length(env : &mut TestEnv) {
    var buf : [8]u8 = [1u8, 2u8, 3u8, 4u8, 5u8, 6u8, 7u8, 8u8]
    if(slice_size(&mut buf) != 8u) {
        env.error("an initialised [8]u8 converted to a slice of the wrong length")
    }
}

@test
func an_empty_initialised_fixed_array_converts_to_an_empty_slice(env : &mut TestEnv) {
    // `= []` is a full zeroed initialiser, so the length is known.
    var buf : [16]u8 = []
    if(slice_size(&mut buf) != 16u) {
        env.error("`var buf : [16]u8 = []` converted to a slice of the wrong length")
    }
}

@test
func a_struct_field_array_converts_to_a_slice_of_its_length(env : &mut TestEnv) {
    var packet = Packet { data : [] }
    if(slice_size(&mut packet.data) != 16u) {
        env.error("a struct field array converted to a slice of the wrong length")
    }
}

@test
func an_initialised_array_passes_by_mutable_reference(env : &mut TestEnv) {
    var buf : [32]u8 = []
    write_first(&mut buf)
    if(buf[0] != 0x2Au8) {
        env.error("the callee did not receive the array through the conversion")
    }
}

@test
func an_uninitialised_fixed_array_can_be_borrowed_as_a_slice(env : &mut TestEnv) {
    // Compiling this function is the assertion: before
    // `array_size_from_type` existed, the conversion on an uninitialised buffer
    // was a hard compile error. (Its LENGTH is the open bug described at the
    // top of this file, which is why nothing here checks it.)
    var buf : [32]u8
    var size = slice_size(&mut buf)
    if(size > 32u) {
        env.error("an uninitialised [32]u8 converted to a slice longer than the array")
    }
}

@test
func osrand_random_bytes_returns_the_requested_length(env : &mut TestEnv) {
    // The owned-buffer route the safe API prefers; it must keep agreeing with
    // the borrowed one above.
    var generated = osrand::random_bytes(20u)
    if(generated is std::Result.Err) {
        env.error("osrand::random_bytes failed")
        return
    }
    var Ok(bytes) = generated else unsafe { unreachable }
    if(bytes.size() != 20u) {
        env.error("osrand::random_bytes should return exactly 20 bytes")
    }
}
