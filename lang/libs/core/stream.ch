public namespace core {
public namespace stream {

/**
 * A sink for the values of an expressive string.
 *
 * `spec` is the packed format spec of `${expr:spec}` (validated by the compiler
 * and decoded with `std::decode_format_spec`); `0` means "no spec", which is the
 * plain rendering and the only path most callers take.
 */
public interface Stream {

    /// Write exactly `length` bytes from `value`. A formatter always knows the
    /// extent of what it renders, so there is deliberately no unbounded
    /// NUL-terminated variant: a `*char` of unknown length must be given as a
    /// `string_view` / `str` / `string` (which carry their length) instead of
    /// being scanned with `strlen` inside a safe formatting call.
    unsafe func writeStr(&self, value : *char, length : ubigint, spec : u64);

    func writeSigned(&self, value : bigint, spec : u64);

    func writeUnsigned(&self, value : ubigint, spec : u64);

    func writeChar(&self, value : char, spec : u64);

    func writeUChar(&self, value : uchar, spec : u64);

    func writeBool(&self, value : bool, spec : u64);

    func writeFloat(&self, value : float, spec : u64);

    func writeDouble(&self, value : double, spec : u64);

}

}
}
