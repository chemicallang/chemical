public namespace std {

public struct ordered_map_node<Key : Hashable + Eq, Value> {
    var key : Key;
    var value : Value;
    var hash_next : *mut ordered_map_node<Key, Value>;
    var order_prev : *mut ordered_map_node<Key, Value>;
    var order_next : *mut ordered_map_node<Key, Value>;
};

public comptime const ORDERED_LOAD_FACTOR_THRESHOLD : float = 0.75f

public struct ordered_map<Key : Hashable + Eq, Value> {

    var table : *mut *mut ordered_map_node<Key, Value>;
    var capacity : size_t;
    var _size : size_t;
    var head : *mut ordered_map_node<Key, Value>;
    var tail : *mut ordered_map_node<Key, Value>;

    func hash_now(&self, key : &Key) : size_t {
        return key.hash()
    }

    func hash_with_capacity(&self, key : &Key) : size_t {
        return (hash_now(key) & (capacity - 1));
    }

    func compare_now(key : &Key, key2 : &Key) : bool {
        return key.equals(key2)
    }

    func resize(&mut self) : void {
        var newCapacity = capacity * 2;
        var newTable = malloc(newCapacity * sizeof(*mut ordered_map_node<Key, Value>)) as *mut *mut ordered_map_node<Key, Value>;
        memset(newTable, 0, newCapacity * sizeof(*mut ordered_map_node<Key, Value>));

        var node = head;
        while (node != null) {
            var nextNode = node.order_next;
            var index = hash_now(&node.key) & (newCapacity - 1);
            node.hash_next = newTable[index];
            newTable[index] = node;
            node = nextNode;
        }

        dealloc table;
        table = newTable;
        capacity = newCapacity;
    }

    @make
    func make() {
        const cap = 16u
        const allocated = malloc(cap * sizeof(*mut ordered_map_node<Key, Value>)) as *mut *mut ordered_map_node<Key, Value>;
        memset(allocated, 0, cap * sizeof(*mut ordered_map_node<Key, Value>));
        return ordered_map<Key, Value> {
            table : allocated,
            capacity : cap,
            _size : 0,
            head : null,
            tail : null
        }
    }

    @delete
    func delete(&self) {
        var node = head;
        while (node != null) {
            var nextNode = node.order_next;
            destruct &node.key;
            destruct &node.value;
            dealloc node;
            node = nextNode;
        }
        dealloc table;
    }

    func insert(&mut self, key : Key, value : Value) {
        if ((_size as float) / capacity > ORDERED_LOAD_FACTOR_THRESHOLD) {
            resize();
        }

        var index = hash_with_capacity(&key);
        var currentNode = table[index];

        while (currentNode != null) {
            if (compare_now(&currentNode.key, &key)) {
                currentNode.value = value;
                return;
            }
            currentNode = currentNode.hash_next;
        }

        const newNode = malloc(sizeof(ordered_map_node<Key, Value>)) as *mut ordered_map_node<Key, Value>;
        new (newNode) ordered_map_node<Key, Value> {
            key : key,
            value : value,
            hash_next : table[index],
            order_prev : tail,
            order_next : null
        }
        table[index] = newNode;
        if (tail != null) {
            tail.order_next = newNode;
        } else {
            head = newNode;
        }
        tail = newNode;
        _size++;
    }

    func get_ptr(&self, key : &Key) : *mut Value {
        var index : size_t = hash_with_capacity(key);
        var currentNode = table[index];
        while (currentNode != null) {
            if (compare_now(&currentNode.key, key)) {
                return &raw mut currentNode.value;
            }
            currentNode = currentNode.hash_next;
        }
        return null;
    }

    func find(&self, key : &Key, value : &mut Value) : bool {
        const ptr = get_ptr(key)
        if(ptr != null) {
            unsafe { *value = *ptr; }
            return true;
        } else {
            return false;
        }
    }

    func contains(&self, key : &Key) : bool {
        return get_ptr(key) != null
    }

    func erase(&mut self, key : &Key) : bool {
        var index : size_t = hash_with_capacity(key);
        var currentNode = table[index];
        var previousNode : *mut ordered_map_node<Key, Value> = null;

        while (currentNode != null) {
            if (compare_now(&currentNode.key, key)) {
                if (previousNode != null) {
                    previousNode.hash_next = currentNode.hash_next;
                } else {
                    table[index] = currentNode.hash_next;
                }

                var prev = currentNode.order_prev;
                var next = currentNode.order_next;
                if (prev != null) {
                    prev.order_next = next;
                } else {
                    head = next;
                }
                if (next != null) {
                    next.order_prev = prev;
                } else {
                    tail = prev;
                }

                destruct &currentNode.key;
                destruct &currentNode.value;
                dealloc currentNode;
                _size--;
                return true;
            }
            previousNode = currentNode;
            currentNode = currentNode.hash_next;
        }
        return false;
    }

    func clear(&mut self) : void {
        var node = head;
        while (node != null) {
            var nextNode = node.order_next;
            destruct &node.key;
            destruct &node.value;
            dealloc node;
            node = nextNode;
        }
        for (var i = 0; i < capacity; i++) {
            table[i] = null;
        }
        head = null;
        tail = null;
        _size = 0;
    }

    func size(&self) : size_t {
        return _size;
    }

    func empty(&self) : bool {
        return _size == 0;
    }

    func isEmpty(&self) : bool {
        return _size == 0;
    }

    func iterator(&self) : ordered_map_iterator<Key, Value> {
        return ordered_map_iterator<Key, Value> {
            node : head
        }
    }

    impl core::iterable::Iterable<ordered_map_node<Key, Value>, ordered_map_iterator<Key, Value>> for ordered_map<Key, Value> {

        func begin(&self) : ordered_map_iterator<Key, Value> {
            return iterator()
        }

        func valid(&self, c : ordered_map_iterator<Key, Value>) : bool {
            return c.valid()
        }

        func current(&self, c : ordered_map_iterator<Key, Value>) : &ordered_map_node<Key, Value> {
            return &*c.node
        }

        func next(&self, c : ordered_map_iterator<Key, Value>) : ordered_map_iterator<Key, Value> {
            var next_cursor : ordered_map_iterator<Key, Value> = c
            next_cursor.next()
            return next_cursor
        }

    }

    impl core::iterable::ReversibleIterable<ordered_map_node<Key, Value>, ordered_map_iterator<Key, Value>> for ordered_map<Key, Value> {
        func rbegin(&self) : ordered_map_iterator<Key, Value> {
            return ordered_map_iterator<Key, Value> {
                node : tail
            }
        }

        func previous(&self, c : ordered_map_iterator<Key, Value>) : ordered_map_iterator<Key, Value> {
            var prev_cursor : ordered_map_iterator<Key, Value> = c
            prev_cursor.prev()
            return prev_cursor
        }

        func count(&self) : size_t {
            return size()
        }
    }

};

public struct ordered_map_iterator<Key, Value> {

    var node : *mut ordered_map_node<Key, Value>;

    func next(&mut self) {
        if (node == null) return;
        node = node.order_next;
    }

    func prev(&mut self) {
        if (node == null) return;
        node = node.order_prev;
    }

    public func key(&self) : &Key {
        return &node.key;
    }
    public func value(&self) : &Value {
        return &node.value;
    }
    public func valid(&self) : bool {
        return node != null;
    }

};

}
