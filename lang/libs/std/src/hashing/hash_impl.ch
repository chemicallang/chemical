impl Hashable for char {
    func hash(&self) : size_t {
        return *self as size_t;
    }
}

impl Hashable for uchar {
    func hash(&self) : size_t {
        return *self;
    }
}

impl Hashable for short {
    func hash(&self) : size_t {
        return *self * KnuthsMultiplicativeConstant;
    }
}

impl Hashable for ushort {
    func hash(&self) : size_t {
        return *self * KnuthsMultiplicativeConstant;
    }
}

impl Hashable for int {
    func hash(&self) : size_t {
        return murmurhash(&self as *char, sizeof(int), 0u)
    }
}

impl Hashable for uint {
    func hash(&self) : size_t {
        return murmurhash(&self as *char, sizeof(uint), 0u)
    }
}

impl Hashable for long {
    func hash(&self) : size_t {
        return murmurhash(&self as *char, sizeof(long), 0u)
    }
}

impl Hashable for ulong {
    func hash(&self) : size_t {
        return murmurhash(&self as *char, sizeof(ulong), 0u)
    }
}

impl Hashable for bigint {
    func hash(&self) : size_t {
        return murmurhash(&self as *char, sizeof(bigint), 0u)
    }
}

impl Hashable for ubigint {
    func hash(&self) : size_t {
        return murmurhash(&self as *char, sizeof(ubigint), 0u)
    }
}