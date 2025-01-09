import "@std/hashing/murmur.ch"
import "@std/std.ch"

@comptime
const KnuthsMultiplicativeConstant = 2654435769ui32

func hash_short(key : short) : uint {
    return (key * KnuthsMultiplicativeConstant)
}

func hash_ushort(key : ushort) : uint {
    return (key * KnuthsMultiplicativeConstant)
}

func <T> __wrap_murmur_hash(value : T) : uint {
    return murmurhash(&value, #sizeof(T), 0)
}

@comptime
func <T> hash(value : T) : uint {
    if(T is char || T is uchar) {
        return value
    } else if(T is short || T is ushort) {
        return compiler::wrap(value * KnuthsMultiplicativeConstant)
    } else if(T is int || T is uint || T is long || T is ulong || T is bigint || T is ubigint || T is float || T is double) {
        return compiler::wrap(__wrap_murmur_hash(value))
    } else if(T is *void) {
        return compiler::wrap(murmurhash(value, #sizeof(T), 0))
    } else if(T is *char) {
        return compiler::wrap(murmurhash(value, strlen(value)))
    } else {
        compiler::error("couldn't determine the hash function for the given type");
    }
}