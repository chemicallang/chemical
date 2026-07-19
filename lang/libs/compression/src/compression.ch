// compression — compression interface and built-in algorithms.

public namespace compression {

using std::Result;
using std::vector;

// ---------------------------------------------------------------------------
// Built-in: RLE (Run-Length Encoding)
// ---------------------------------------------------------------------------

public struct RleCompressor {
}

public func (c : &mut RleCompressor) compress(data : *u8, data_len : size_t, out : *mut u8, out_len : *mut size_t, out_capacity : size_t) : Result<size_t, CompressionError> {
    var ipos : size_t = 0;
    var opos : size_t = 0;
    while(ipos < data_len) {
        if(opos + 2 > out_capacity) { return Result.Err(CompressionError.BufferTooSmall(opos + 2)); }
        var current = data[ipos];
        var run_len : u8 = 1;
        ipos += 1;
        while(ipos < data_len && run_len < 255 && data[ipos] == current) {
            run_len += 1;
            ipos += 1;
        }
        out[opos] = run_len;
        opos += 1;
        out[opos] = current;
        opos += 1;
    }
    if(opos + 1 > out_capacity) { return Result.Err(CompressionError.BufferTooSmall(opos + 1)); }
    out[opos] = 0;
    opos += 1;
    out_len[0] = opos;
    return Result.Ok(opos);
}

public func (c : &mut RleCompressor) decompress(data : *u8, data_len : size_t, out : *mut u8, out_len : *mut size_t, out_capacity : size_t) : Result<size_t, CompressionError> {
    var ipos : size_t = 0;
    var opos : size_t = 0;
    while(ipos < data_len) {
        var count = data[ipos];
        ipos += 1;
        if(count == 0) { break; }
        if(ipos >= data_len) { return Result.Err(CompressionError.InvalidData()); }
        var byte_val = data[ipos];
        ipos += 1;
        var i : u8 = 0;
        while(i < count) {
            if(opos >= out_capacity) { return Result.Err(CompressionError.BufferTooSmall(opos)); }
            out[opos] = byte_val;
            opos += 1;
            i += 1;
        }
    }
    out_len[0] = opos;
    return Result.Ok(opos);
}

public func (c : &mut RleCompressor) name() : std::string_view {
    return std::string_view("RLE", 3);
}

// ---------------------------------------------------------------------------
// Convenience functions
// ---------------------------------------------------------------------------

public func compress(data : *u8, data_len : size_t, out : *mut u8, out_len : *mut size_t, out_capacity : size_t) : Result<size_t, CompressionError> {
    var rle = RleCompressor{};
    return rle.compress(data, data_len, out, out_len, out_capacity);
}

public func decompress(data : *u8, data_len : size_t, out : *mut u8, out_len : *mut size_t, out_capacity : size_t) : Result<size_t, CompressionError> {
    var rle = RleCompressor{};
    return rle.decompress(data, data_len, out, out_len, out_capacity);
}

} // end namespace compression
