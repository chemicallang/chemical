import "@std/std.ch"
import "hashing/hash.ch"

@comptime
const EMPTY_KEY = -1

@comptime
const DELETED_KEY = -2

struct unordered_map_node<Key, Value> {
    var key : Key;
    var value : Value;
};

@comptime
const LOAD_FACTOR_THRESHOLD : float = 0.75f

struct unordered_map<Key, Value> {

    var table : *mut unordered_map_node<Key, Value>;
    var capacity : size_t;
    var size : size_t;

    func hash_now(&self, key : &Key) : size_t {
        return (hash<Key>(key) & (capacity - 1));
    }

    // Resize and rehash
    func resize(&self) : void {
        var newCapacity = capacity * 2;
        var newTable = malloc(newCapacity * #sizeof(unordered_map_node<Key, Value>)) as *mut unordered_map_node<Key, Value>;
        memset(newTable, EMPTY_KEY, newCapacity * #sizeof(unordered_map_node<Key, Value>));

        // Rehash all elements into the new table
        for (var i = 0; i < capacity; i++) {
            if (table[i].key != EMPTY_KEY && table[i].key != DELETED_KEY) {
                var index = table[i].key & (newCapacity - 1);
                while (newTable[index].key != EMPTY_KEY) {
                    index = (index + 1) & (newCapacity - 1); // Quadratic probing
                }
                newTable[index] = table[i];
            }
        }

        free(table);
        table = newTable;
        capacity = newCapacity;
    }

    @make
    func make() {
        capacity = 16;
        size = 0
        table = malloc(capacity * #sizeof(unordered_map_node<Key, Value>)) as *unordered_map_node<Key, Value>;
        memset(table, EMPTY_KEY, capacity * #sizeof(unordered_map_node<Key, Value>));
    }

    @delete
    func delete(&self) {
        free(table);
    }

    // Insert or update a key-value pair
    func insert(&self, key : &Key, value : &Value) {

        if ((size as float) / capacity > LOAD_FACTOR_THRESHOLD) {
            resize();
        }

        var index = hash_now(key);
        var attempt : size_t = 0;

        while (table[index].key != EMPTY_KEY && table[index].key != DELETED_KEY) {
            if (table[index].key == key) {
                table[index].value = value; // Update value
                return;
            }
            index = (index + (attempt * attempt)) & (capacity - 1); // Quadratic probing
            attempt++;
        }

        table[index].key = key;
        table[index].value = value;
        size++;
    }

    // Find a value by key
    func find(&self, key : &Key, value : &mut Value) : bool {
        var index : size_t = hash_now(key);
        var attempt : size_t = 0;

        while (table[index].key != EMPTY_KEY) {
            if (table[index].key != DELETED_KEY && table[index].key == key) {
                value = table[index].value;
                return true;
            }
            index = (index + (attempt * attempt)) & (capacity - 1); // Quadratic probing
            attempt++;
        }
        return false;
    }

    // Remove a key-value pair
    func erase(&self, key : &Key) : bool {
        var index : size_t = hash_now(key);
        var attempt : size_t = 0;

        while (table[index].key != EMPTY_KEY) {
            if (table[index].key != DELETED_KEY && table[index].key == key) {
                table[index].key = DELETED_KEY; // Mark as deleted
                size--;
                return true;
            }
            index = (index + (attempt * attempt)) & (capacity - 1); // Quadratic probing
            attempt++;
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