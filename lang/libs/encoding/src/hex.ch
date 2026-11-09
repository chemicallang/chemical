// hex — hex encoding and decoding utilities.
//
// Raw pointer/out-buffer entry points. The canonical owned-buffer API lives at
// `encoding::hex_encode` / `encoding::hex_encode_upper` / `encoding::hex_decode`
// (see `safe.ch`).

public namespace encoding {

using std::Result;

const HEX_DIGITS_LOWER = "0123456789abcdef";
const HEX_DIGITS_UPPER = "0123456789ABCDEF";

func hex_to_nibble(c : char) : int {
        unsafe {
        if(c >= '0' && c <= '9') { return (c as int) - ('0' as int); }
        if(c >= 'a' && c <= 'f') { return (c as int) - ('a' as int) + 10; }
        if(c >= 'A' && c <= 'F') { return (c as int) - ('A' as int) + 10; }
        return -1;

        }
    }

// Shared decoding core used by the canonical `hex_decode(&str)` entry point.
func hex_decode_view(hex : std::string_view) : Result<std::vector<u8>, EncodingError> {
    if(hex.size() % 2 != 0) { return Result.Err(EncodingError.InvalidInput()); }
    var byte_count = hex.size() / 2;
    var vec = std::vector<u8>();
    if(!vec.try_reserve(byte_count)) { return Result.Err(EncodingError.OutOfMemory()); }
    var i : size_t = 0;
    while(i < hex.size()) {
        var high = hex_to_nibble(hex.at(i));
        if(high < 0) { return Result.Err(EncodingError.InvalidInput()); }
        var low = hex_to_nibble(hex.at(i + 1));
        if(low < 0) { return Result.Err(EncodingError.InvalidInput()); }
        if(!vec.try_push(((high as u8) << 4) | (low as u8))) {
            return Result.Err(EncodingError.OutOfMemory());
        }
        i += 2;
    }
    return Result.Ok(vec);
}


} // end namespace encoding
