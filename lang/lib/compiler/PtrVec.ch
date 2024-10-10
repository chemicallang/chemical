@compiler:interface
public struct PtrVec {

    func _get(&self, i : uint) : *void;

    func _set(&self, i : uint, ptr : *void);

    func _push(&self, ptr : *void);

    func _erase(&self, i : uint);

    func _size(&self) : size_t;

}

public struct VecRef<T> : private PtrVec {

    func get(&self, i : uint) : *T {
        return _get(i) as *T;
    }

    func set(&self, i : uint, ptr : *T) {
        _set(i, ptr);
    }

    func push(&self, ptr : *T) {
        _push(ptr);
    }

    func erase(&self, i : uint) {
        _erase(i);
    }

    func size(&self) : size_t {
        return _size();
    }

}