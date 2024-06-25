import "../../std/std.ch"

@cbi:global("compiler")
struct String {

    var data : char*
    var length : size_t
    var capacity : size_t

    /**
     * initializes an empty string with initial capacity
    explicit String(initial_capacity : ulong) {
        data = malloc(initial_capacity) as char*;
        data[0] = '\0';
        length = 0;
        capacity = initial_capacity;
    }
    */

    /**
     * resize the string to a new capacity
     */
    func resize(new_capacity : ulong) {
        var new_data = realloc(data, new_capacity) as char*;
        if (new_data) {
            data = new_data;
            capacity = new_capacity;
        }
    }

    /**
     * append a character to a string
     */
    func append_char(c : char) {
        if(length + 1 >= capacity) {
            resize(length + 1 + 10);
        }
        data[length] = c;
        length++;
        data[length] = '\0';
    }

    /**
     * append a string to this string, with given length
     */
    func append_str_of_len(s : char*, len : size_t) {
        if(length + len >= capacity) {
            resize(length + len + 3);
        }
        memcpy(data + length, s, len);
        length += len;
        data[length] = '\0';
    }

    /**
     * append another string to this string
     */
    func append_str(s : char*) {
        var len = strlen(s);
        append_str_of_len(s, len);
    }

    /**
     * frees the string
     */
    @destructor
    func delete() {
        free(data);
    }

};