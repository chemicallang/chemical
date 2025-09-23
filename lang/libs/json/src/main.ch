public variant JsonValue {
    Null()
    Bool(value : bool)
    Number(value : std::string)
    String(value : std::string)
    Object(values : std::unordered_map<std::string, JsonValue>)
    Array(values : std::vector<JsonValue>)
}

public func escape_and_print_string(str: &std::string) {
    printf("\"");
    var data = str.data();
    var len = str.size();
    var i = 0;
    while (i < len) {
        var ch = data[i];
        switch (ch) {
            '"'  => printf("\\\"");
            '\\' => printf("\\\\");
            '\n' => printf("\\n");
            '\r' => printf("\\r");
            '\t' => printf("\\t");
            default => printf("%c", ch);
        }
        i++;
    }
    printf("\"");
}

public func print_json_value(value : &JsonValue, indent: int = 0) {
    switch(value) {
        default => { printf("UNKNOWN"); }
        Null() => { printf("null") }
        Bool(value) => {
            if(value) {
                printf("true")
            } else {
                printf("false")
            }
        }
        Number(value) => { printf("%s", value.data()); }
        String(value) => { escape_and_print_string(value); }
        Object(values) => {
            printf("\{\n");
            var itr = values.iterator();
            var first = true;
            while (itr.valid()) {
                if (!first) printf(",\n");
                first = false;

                // indent
                for (var i = 0; i < indent + 2; i++) { printf(" "); }

                var key = itr.key()
                escape_and_print_string(key);
                printf(": ");
                var val = itr.value();
                print_json_value(val, indent + 2);
                itr.next();
            }
            printf("\n");
            for (var i = 0; i < indent; i++) { printf(" "); }
            printf("}");
        }
        Array(values) => {
            printf("[\n");
            var current = values.data();
            const end = current + values.size();
            var first = true;
            while (current != end) {
                if (!first) printf(",\n");
                first = false;

                for (var i = 0; i < indent + 2; i++) { printf(" "); }
                print_json_value(*current, indent + 2);

                current++;
            }
            printf("\n");
            for (var i = 0; i < indent; i++) { printf(" "); }
            printf("]");
        }
    }
}

public struct ASTJsonHandler : public JsonSaxHandler {

    var stack : std::vector<JsonValue>

    var current_key : std::string

    var have_current_key : bool = false

    var root : JsonValue;

    var have_root : bool = false

    @make
    func make() {
        root = JsonValue.Null()
    }

    // helper: move value `v` into either current container or become the root
    func attach_value(&self, v : JsonValue) {
        if (!stack.empty()) {
            // attach into top container
            var top = stack.last_ptr();
            // check whether top is array or object
            if (top is JsonValue.Array) {
                var Array(values) = *top else unreachable
                // it's an Array
                values.push(v);
            } else if (top is JsonValue.Object) {
                // it's an Object
                if (!have_current_key) {
                    // malformed SAX sequence -- object value without a key
                    // convert to None or ignore; here we'll insert with empty key to avoid crash
                    var key = std::string();
                    var Object(values) = *top else unreachable
                    values.insert(key, v);
                } else {
                    var Object(values) = *top else unreachable
                    values.insert(current_key.copy(), v);
                    // consume the current key
                    current_key.clear();
                    have_current_key = false;
                }
            } else {
                // top is something else (shouldn't happen). As a fallback, replace root.
                root = v;
                have_root = true;
            }
        } else {
            // no open container: this value becomes the root
            root = v;
            have_root = true;
        }
    }

    @override
    func on_null(&self) {
        attach_value(JsonValue.Null())
    }

    @override
    func on_bool(&self, v : bool) {
        attach_value(JsonValue.Bool(v))
    }

    @override
    func on_number(&self, s : *char, len : size_t) {
        var str = std::string()
        str.append_with_len(s, len)
        attach_value(JsonValue.Number(str))
    }

    @override
    func on_string(&self, s : *char, len : size_t) {
        var str = std::string()
        str.append_with_len(s, len)
        attach_value(JsonValue.String(str))
    }

    @override
    func on_object_begin(&self) {
        // push an empty object
        stack.push(JsonValue::Object(std::unordered_map<std::string, JsonValue>()));
        // clear any pending key (shouldn't be set when object begins, but safe)
        current_key.clear();
        have_current_key = false;
    }

    @override
    func on_object_end(&self) {
        if (stack.empty()) {
            // unmatched end; ignore
            return;
        }
        attach_value(stack.take_last());
    }

    @override
    func on_array_begin(&self) {
        stack.push(JsonValue.Array(std::vector<JsonValue>()));
    }

    @override
    func on_array_end(&self) {
        if (stack.empty()) {
            // unmatched end; ignore
            return;
        }
        attach_value(stack.take_last());
    }

    @override
    func on_key(&self, s : *char, len : size_t) {
        // copy the key into current_key; will be consumed by next value
        current_key.append_with_len(s, len);
        have_current_key = true;
    }

};