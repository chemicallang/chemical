public namespace core {
public namespace stream {

public interface Stream {

    func writeStr(&self, value : *char, length : ubigint);

    func writeStrNoLen(&self, value : *char);

    func writeI8(&self, value : i8);

    func writeI16(&self, value : i16);

    func writeI32(&self, value : i32);

    func writeI64(&self, value : i64);

    func writeU8(&self, value : u8);

    func writeU16(&self, value : u16);

    func writeU32(&self, value : u32);

    func writeU64(&self, value : u64);

    func writeChar(&self, value : char);

    func writeUChar(&self, value : uchar);

    func writeShort(&self, value : short);

    func writeUShort(&self, value : ushort);

    func writeInt(&self, value : int);

    func writeUInt(&self, value : uint);

    func writeLong(&self, value : long);

    func writeULong(&self, value : ulong);

    func writeLongLong(&self, value : longlong);

    func writeULongLong(&self, value : ulonglong);

    func writeFloat(&self, value : float);

    func writeDouble(&self, value : double);

}

}
}
