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

public func <T> __wrap_murmur_hash(value : T) : uint {
    return murmurhash(&value, sizeof(T), 0)
}

@comptime
func <T> is_type_number() : bool {
    return T is int || T is uint || T is short || T is ushort || T is long || T is ulong || T is bigint || T is ubigint || T is float || T is double;
}

@comptime
func <T> is_type_ref_number() : bool {
    return T is &int || T is &uint || T is &short || T is &ushort || T is &long || T is &ulong || T is &bigint || T is &ubigint || T is &float || T is &double;
}

public interface Eq {

    func equals(&self, &other) : bool

}

@comptime
func <T> compare(value : T, value2 : T) : bool {
    type ptr_any = *any
    if(T is char || T is uchar || is_type_number<T>() || compiler::satisfies(ptr_any, T)) {
        return compiler::wrap(value == value2)
    } else if(T is Eq) {
       const comp = value as Eq
       return compiler::wrap(comp.equals(value2))
    }
    compiler::error("couldn't determine the hash function for the given type");
    return 0;
}

public interface Hashable {

    func hash(&self) : uint

}

@comptime
func <T> hash(value : T) : uint {
    type ptr_any = *any
    if(T is char || T is uchar || T is &char || T is &uchar) {
        return compiler::wrap(value as uint)
    } else if(T is short || T is ushort) {
        return compiler::wrap(value * KnuthsMultiplicativeConstant)
    } else if(compiler::satisfies(ptr_any, T)) {
        return compiler::wrap(murmurhash(value, sizeof(T), 0))
    } else if(is_type_number<T>()) {
        return compiler::wrap(__wrap_murmur_hash<T>(value))
    } else if(is_type_ref_number<T>()) {
        return compiler::wrap(__wrap_murmur_hash<T>(value))
    } else if (T is Hashable) {
        const hashable = value as Hashable
        return compiler::wrap(hashable.hash())
    }
    compiler::println("unknown value type for hashing ", compiler::type_to_string<T>());
    compiler::error("couldn't determine the hash function for the given type");
    return 0;
}