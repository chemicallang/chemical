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
    return murmurhash(&value, sizeof(T), 0)
}

@comptime
func <T> is_type_number() : bool {
    compiler::println("checking if type is number:", compiler::type_to_string<T>());
    return T is int || T is uint || T is short || T is ushort || T is long || T is ulong || T is bigint || T is ubigint || T is float || T is double;
}

@comptime
func <T> hash(value : T) : uint {
    type ptr_any = *any
    type ref_any = &any
    if(T is char || T is uchar) {
        return compiler::wrap(value as uint)
    } else if(T is short || T is ushort) {
        return compiler::wrap(value * KnuthsMultiplicativeConstant)
    } else if(is_type_number<T>()) {
        return compiler::wrap(__wrap_murmur_hash(value))
    } else if(compiler::satisfies(ref_any, T)) {
        // it's a reference
        type child_type |= compiler::get_child_type<T>()
        return hash<child_type>(value)
    } else if(compiler::satisfies(ptr_any, T)) {
        return compiler::wrap(murmurhash(value, sizeof(T), 0))
    } else {
        compiler::println("unknown value type for hashing ", compiler::type_to_string<T>());
        compiler::error("couldn't determine the hash function for the given type");
        return 0;
    }
}