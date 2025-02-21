import "@std/std.ch"
import "hashing/hash.ch"

public namespace std {

public struct unordered_map_node<Key, Value> {
    var key : Key;
    var value : Value;
    var next : *mut unordered_map_node<Key, Value>; // Pointer to next node in the chain
};

@comptime
const LOAD_FACTOR_THRESHOLD : float = 0.75f

public struct unordered_map<Key, Value> {

    var table : *mut *mut unordered_map_node<Key, Value>; // Array of buckets (pointers to linked lists)
    var capacity : size_t;
    var size : size_t;

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
    func resize(&self) : void {
        var newCapacity = capacity * 2;
        var newTable = malloc(newCapacity * sizeof(*mut unordered_map_node<Key, Value>)) as *mut *mut unordered_map_node<Key, Value>;

        // Initialize new table to nullptr (empty buckets)
        memset(newTable, 0, newCapacity * sizeof(*mut unordered_map_node<Key, Value>));

        // Rehash all elements into the new table
        for (var i = 0; i < capacity; i++) {
            var currentNode = table[i];
            while (currentNode != null) {
                var index = hash_now(currentNode.key) & (newCapacity - 1);
                var newNode = malloc(sizeof(unordered_map_node<Key, Value>)) as *mut unordered_map_node<Key, Value>;
                newNode.key = currentNode.key;
                newNode.value = currentNode.value;
                newNode.next = newTable[index];
                newTable[index] = newNode;

                currentNode = currentNode.next;
            }
        }

        // Free old table
        free(table);
        table = newTable;
        capacity = newCapacity;
    }

    @make
    func make() {
        capacity = 16;
        size = 0;
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
                free(currentNode);
                currentNode = nextNode;
            }
        }
        free(table);
    }

    // Insert or update a key-value pair
    func insert(&self, key : &Key, value : &Value) {

        if ((size as float) / capacity > LOAD_FACTOR_THRESHOLD) {
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
        newNode.key = key;
        newNode.value = value;
        newNode.next = table[index];
        table[index] = newNode;
        size++;
    }

    func get_ptr(&self, key : &Key) : *Value {
        var index : size_t = hash_with_capacity(key);
        var currentNode = table[index];
        while (currentNode != null) {
            if (compare_now(currentNode.key, key)) {
                return &currentNode.value;
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
    func erase(&self, key : &Key) : bool {
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
                free(currentNode);
                size--;
                return true;
            }
            previousNode = currentNode;
            currentNode = currentNode.next;
        }
        return false;
    }

    // Get the size of the map
    func getSize(&self) : size_t {
        return size;
    }

    // Check if the map is empty
    func isEmpty(&self) : bool {
        return size == 0;
    }

};

}