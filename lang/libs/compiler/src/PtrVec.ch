
@compiler.interface
public struct PtrVec {

    func _get(&self, i : uint) : *void;

    func _set(&mut self, i : uint, ptr : *void);

    func _push(&mut self, ptr : *void);

    func _erase(&mut self, i : uint);

    func _size(&self) : size_t;

}

public struct VecRef<T> : private PtrVec {

    func get(&self, i : uint) : *mut T {
        return _get(i) as *mut T;
    }

    func set(&mut self, i : uint, ptr : *mut T) {
        _set(i, ptr);
    }

    func push(&mut self, ptr : *mut T) {
        _push(ptr);
    }

    func erase(&mut self, i : uint) {
        _erase(i);
    }

    func size(&self) : size_t {
        return _size();
    }

}