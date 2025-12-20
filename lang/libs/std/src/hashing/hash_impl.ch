comptime const KnuthsMultiplicativeConstant = 2654435769ui32

protected impl Hashable for char {
    func hash(&self) : size_t {
        return *self as size_t;
    }
}

protected impl Hashable for uchar {
    func hash(&self) : size_t {
        return *self;
    }
}

protected impl Hashable for short {
    func hash(&self) : size_t {
        return *self * KnuthsMultiplicativeConstant;
    }
}

protected impl Hashable for ushort {
    func hash(&self) : size_t {
        return *self * KnuthsMultiplicativeConstant;
    }
}

protected impl Hashable for int {
    func hash(&self) : size_t {
        return murmurhash(&self as *char, sizeof(int), 0u)
    }
}

protected impl Hashable for uint {
    func hash(&self) : size_t {
        return murmurhash(&self as *char, sizeof(uint), 0u)
    }
}

protected impl Hashable for long {
    func hash(&self) : size_t {
        return murmurhash(&self as *char, sizeof(long), 0u)
    }
}

protected impl Hashable for ulong {
    func hash(&self) : size_t {
        return murmurhash(&self as *char, sizeof(ulong), 0u)
    }
}

protected impl Hashable for bigint {
    func hash(&self) : size_t {
        return murmurhash(&self as *char, sizeof(bigint), 0u)
    }
}

protected impl Hashable for ubigint {
    func hash(&self) : size_t {
        return murmurhash(&self as *char, sizeof(ubigint), 0u)
    }
}