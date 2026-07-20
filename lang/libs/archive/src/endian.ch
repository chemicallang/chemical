public namespace archive {

public func read_u16_le(data : *u8, offset : size_t) : u16 {
    return (data[offset] as u16) | ((data[offset + 1] as u16) << 8)
}

public func read_u32_le(data : *u8, offset : size_t) : u32 {
    return (data[offset] as u32) | ((data[offset + 1] as u32) << 8) |
           ((data[offset + 2] as u32) << 16) | ((data[offset + 3] as u32) << 24)
}

public func read_u64_le(data : *u8, offset : size_t) : u64 {
    return (data[offset] as u64) | ((data[offset + 1] as u64) << 8) |
           ((data[offset + 2] as u64) << 16) | ((data[offset + 3] as u64) << 24) |
           ((data[offset + 4] as u64) << 32) | ((data[offset + 5] as u64) << 40) |
           ((data[offset + 6] as u64) << 48) | ((data[offset + 7] as u64) << 56)
}

public func write_u16_le(out : *mut u8, offset : size_t, val : u16) {
    out[offset] = (val & 0xFFu16) as u8
    out[offset + 1] = ((val >> 8) & 0xFFu16) as u8
}

public func write_u32_le(out : *mut u8, offset : size_t, val : u32) {
    out[offset] = (val & 0xFFu32) as u8
    out[offset + 1] = ((val >> 8) & 0xFFu32) as u8
    out[offset + 2] = ((val >> 16) & 0xFFu32) as u8
    out[offset + 3] = ((val >> 24) & 0xFFu32) as u8
}

public func write_u64_le(out : *mut u8, offset : size_t, val : u64) {
    out[offset] = (val & 0xFFu64) as u8
    out[offset + 1] = ((val >> 8) & 0xFFu64) as u8
    out[offset + 2] = ((val >> 16) & 0xFFu64) as u8
    out[offset + 3] = ((val >> 24) & 0xFFu64) as u8
    out[offset + 4] = ((val >> 32) & 0xFFu64) as u8
    out[offset + 5] = ((val >> 40) & 0xFFu64) as u8
    out[offset + 6] = ((val >> 48) & 0xFFu64) as u8
    out[offset + 7] = ((val >> 56) & 0xFFu64) as u8
}

} // end namespace archive
