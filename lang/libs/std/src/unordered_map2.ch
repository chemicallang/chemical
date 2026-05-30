public namespace std {

public struct unordered_map_node<Key : Hashable | Eq, Value> {
    var key : Key;
    var value : Value;
    var next : *mut unordered_map_node<Key, Value>; // Pointer to next node in the chain
};

public comptime const LOAD_FACTOR_THRESHOLD2 : float = 0.75f

public struct unordered_map<Key : Hashable | Eq, Value> {

    var table : *mut *mut unordered_map_node<Key, Value>; // Array of buckets (pointers to linked lists)
    var capacity : size_t;
    var _size : size_t;

    func hash_now(&self, key : &Key) : size_t {
        return key.hash()
    }

    func hash_with_capacity(&self, key : &Key) : size_t {
        return (hash_now(key) & (capacity - 1));
    }

    func compare_now(key : &Key, key2 : &Key) : bool {
        return key.equals(key2)
    }

    // Resize and rehash
    func resize(&mut self) : void {
        var newCapacity = capacity * 2;
        var newTable = malloc(newCapacity * sizeof(*mut unordered_map_node<Key, Value>)) as *mut *mut unordered_map_node<Key, Value>;

        // Initialize new table to nullptr (empty buckets)
        memset(newTable, 0, newCapacity * sizeof(*mut unordered_map_node<Key, Value>));

        // Rehash all elements into the new table
        for (var i = 0; i < capacity; i++) {
            var currentNode = table[i];
            while (currentNode != null) {
                var nextNode = currentNode.next; // Save next pointer before re-linking

                var index = hash_now(currentNode.key) & (newCapacity - 1);
                // Reinsert the node into the new table's bucket chain
                currentNode.next = newTable[index];
                newTable[index] = currentNode;

                currentNode = nextNode;
            }
        }

        // Free old table
        dealloc table;
        table = newTable;
        capacity = newCapacity;
    }

    @make
    func make() {
        const cap = 16u
        const allocated = malloc(cap * sizeof(*mut unordered_map_node<Key, Value>)) as *mut *mut unordered_map_node<Key, Value>;
        memset(allocated, 0, cap * sizeof(*mut unordered_map_node<Key, Value>));
        return unordered_map<Key, Value> {
            table : allocated,
            capacity : cap,
            _size : 0
        }
    }

    @delete
    func delete(&self) {
        // Free all nodes in the chains
        for (var i = 0; i < capacity; i++) {
            var currentNode = table[i];
            while (currentNode != null) {
                var nextNode = currentNode.next;
                // Call destructors on the key and value before freeing the node
                destruct &currentNode.key;
                destruct &currentNode.value;
                dealloc currentNode;
                currentNode = nextNode;
            }
        }
        dealloc table;
    }

    // Insert or update a key-value pair
    func insert(&mut self, key : Key, value : Value) {

        if ((_size as float) / capacity > LOAD_FACTOR_THRESHOLD2) {
            resize();
        }

        var index = hash_with_capacity(key);
        var currentNode = table[index];

        // Check if the key already exists in the chain, and update if so
        while (currentNode != null) {
            if (compare_now(currentNode.key, key)) {
                currentNode.value = value; // Update value
                return;
            }
            currentNode = currentNode.next;
        }

        // Insert the new node at the front of the chain
        const newNode = malloc(sizeof(unordered_map_node<Key, Value>)) as *mut unordered_map_node<Key, Value>;
        new (newNode) unordered_map_node<Key, Value> {
            key : key,
            value : value,
            next : table[index]
        }
        table[index] = newNode;
        _size++;
    }

    func get_ptr(&self, key : &Key) : *mut Value {
        var index : size_t = hash_with_capacity(key);
        var currentNode = table[index];
        while (currentNode != null) {
            if (compare_now(currentNode.key, key)) {
                return &mut currentNode.value;
            }
            currentNode = currentNode.next;
        }
        return null;
    }

    // Find a value by key
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

    // Remove a key-value pair
    func erase(&mut self, key : &Key) : bool {
        var index : size_t = hash_with_capacity(key);
        var currentNode = table[index];
        var previousNode : *mut unordered_map_node<Key, Value> = null;

        while (currentNode != null) {
            if (compare_now(currentNode.key, key)) {
                if (previousNode != null) {
                    previousNode.next = currentNode.next; // Unlink the node
                } else {
                    table[index] = currentNode.next; // Remove from the front of the chain
                }
                // Call destructors on the key and value before freeing the node
                destruct &currentNode.key;
                destruct &currentNode.value;
                dealloc currentNode;
                _size--;
                return true;
            }
            previousNode = currentNode;
            currentNode = currentNode.next;
        }
        return false;
    }

    // Remove all key-value pairs
    func clear(&mut self) : void {
        for (var i = 0; i < capacity; i++) {
            var currentNode = table[i];
            while (currentNode != null) {
                var nextNode = currentNode.next;
                // Call destructors on the key and value before freeing the node
                destruct &currentNode.key;
                destruct &currentNode.value;
                dealloc currentNode;
                currentNode = nextNode;
            }
            table[i] = null;
        }
        _size = 0;
    }

    // Get the size of the map
    func size(&self) : size_t {
        return _size;
    }

    // check if the map is empty
    func empty(&self) : bool {
        return _size == 0;
    }

    // Check if the map is empty
    func isEmpty(&self) : bool {
        return _size == 0;
    }

    func iterator(&self) : unordered_map_iterator<Key, Value> {
        var it = unordered_map_iterator<Key, Value> {
            map : &self,
            bucket : 0,
            node : null
        }
        // find first non-empty bucket
        while (it.bucket < capacity && table[it.bucket] == null) {
            it.bucket += 1;
        }
        if (it.bucket < capacity) {
            it.node = table[it.bucket];
        } else {
            it.node = null; // end
        }
        return it;
    }

    impl core::iterable::Iterable<unordered_map_node<Key, Value>, unordered_map_iterator<Key, Value>> for unordered_map<Key, Value> {

        func begin(&self) : unordered_map_iterator<Key, Value> {
            return iterator()
        }

        func valid(&self, c : unordered_map_iterator<Key, Value>) : bool {
            return c.valid()
        }

        func current(&self, c : unordered_map_iterator<Key, Value>) : &unordered_map_node<Key, Value> {
            return *c.node
        }

        func next(&self, c : unordered_map_iterator<Key, Value>) : unordered_map_iterator<Key, Value> {
            // TODO: without explicit type it doesn't work
            var next_cursor : unordered_map_iterator<Key, Value> = c
            next_cursor.next()
            return next_cursor
        }

    }

    impl core::iterable::ReversibleIterable<unordered_map_node<Key, Value>, unordered_map_iterator<Key, Value>> for unordered_map<Key, Value> {
        func rbegin(&self) : unordered_map_iterator<Key, Value> {
            var it = iterator()
            var prev : unordered_map_iterator<Key, Value> = it
            while (it.valid()) {
                prev = it
                it.next()
            }
            return prev
        }

        func previous(&self, c : unordered_map_iterator<Key, Value>) : unordered_map_iterator<Key, Value> {
            var it = iterator()
            var prev : unordered_map_iterator<Key, Value> = unordered_map_iterator<Key, Value> {
                map : &self,
                bucket : 0,
                node : null
            }
            while (it.valid()) {
                if (it.bucket == c.bucket && it.node == c.node) {
                    return prev
                }
                prev = it
                it.next()
            }
            return prev
        }

        func count(&self) : size_t {
            return size()
        }
    }

};

public struct unordered_map_iterator<Key, Value> {

    var map : *unordered_map<Key, Value>;
    var bucket : size_t;
    var node : *mut unordered_map_node<Key, Value>;

    func next(&mut self) {
        if (node == null) {
            return;
        }
        // if there is a next node in chain, go there
        if (node.next != null) {
            node = node.next;
            return;
        }

        // otherwise scan forward for next non-empty bucket
        bucket += 1;
        while (bucket < map.capacity && map.table[bucket] == null) {
            bucket += 1;
        }

        if (bucket < map.capacity) {
            node = map.table[bucket];
        } else {
            node = null; // reached end
        }
    }

    // accessors
    public func key(&self) : &Key {
        return node.key;
    }
    public func value(&self) : &Value {
        return node.value;
    }
    public func valid(&self) : bool {
        return node != null;
    }

};

}
