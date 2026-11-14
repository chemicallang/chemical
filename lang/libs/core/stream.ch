public namespace core {
public namespace stream {

public interface Stream {

    /// Write exactly `length` bytes from `value`. A formatter always knows the
    /// extent of what it renders, so there is deliberately no unbounded
    /// NUL-terminated variant: a `*char` of unknown length must be given as a
    /// `string_view` / `str` / `string` (which carry their length) instead of
    /// being scanned with `strlen` inside a safe formatting call.
    unsafe func writeStr(&self, value : *char, length : ubigint);

    func writeSigned(&self, value : bigint);

    func writeUnsigned(&self, value : ubigint);

    func writeChar(&self, value : char);

    func writeUChar(&self, value : uchar);

    func writeBool(&self, value : bool);

    func writeFloat(&self, value : float);

    func writeDouble(&self, value : double);

}

}
}
