public namespace core {

public namespace iterable {

interface Linear<T> {

    func data(&self) : *T

    func size(&self) : u64

}

}

}

public struct span<T> {


}

impl core::iterable::Linear<T> for span<T> {
    func data(&self) : *T {
        return _data;
    }
    func size(&self) : u64 {
        return _size;
    }
}