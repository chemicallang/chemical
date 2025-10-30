public namespace std {

public struct unordered_map_node<Key, Value> {
    var key : Key;
    var value : Value;
    var next : *mut unordered_map_node<Key, Value>; // Pointer to next node in the chain
};

public comptime const LOAD_FACTOR_THRESHOLD : float = 0.75f

public struct unordered_map<Key, Value> {

    var table : *mut *mut unordered_map_node<Key, Value>; // Array of buckets (pointers to linked lists)
    var capacity : size_t;
    var _size : size_t;

    func hash_now(&self, key : &Key) : size_t {
        return hash<Key>(key);
    }

    func hash_with_capacity(&self, key : &Key) : size_t {
        return (hash_now(key) & (capacity - 1));
    }

    func compare_now(key : &Key, key2 : &Key) : bool {
        return compare<Key>(key, key2);
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
        capacity = 16;
        _size = 0;
        table = malloc(capacity * sizeof(*mut unordered_map_node<Key, Value>)) as *mut *mut unordered_map_node<Key, Value>;
        memset(table, 0, capacity * sizeof(*mut unordered_map_node<Key, Value>));
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

        if ((_size as float) / capacity > LOAD_FACTOR_THRESHOLD) {
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
            value = *ptr;
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