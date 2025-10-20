public variant JsonValue {
    Null()
    Bool(value : bool)
    Number(value : std::string)
    String(value : std::string)
    Object(values : std::unordered_map<std::string, JsonValue>)
    Array(values : std::vector<JsonValue>)
}

public interface JsonStringEmitter {

    func append_view(&self, view : &std::string_view)

    func append_char(&self, ch : char)

}

public struct JsonStringPrinter : JsonStringEmitter {
    var stream : CommandLineStream
    @make
    func make() {
        stream = CommandLineStream {}
    }
    @override
    func append_view(&self, view : &std::string_view) {
        stream.writeStr(view.data(), view.size())
    }
    @override
    func append_char(&self, ch : char) {
        stream.writeChar(ch)
    }
}

public struct JsonStringBuilder : JsonStringEmitter {
    var ptr : &std::string
    @override
    func append_view(&self, view : &std::string_view) {
        ptr.append_view(view)
    }
    @override
    func append_char(&self, ch : char) {
        ptr.append(ch)
    }
}

public func <T : JsonStringEmitter> escape_string_into(emitter : &T, str: &std::string) {
    emitter.append_view("\"");
    var data = str.data();
    var len = str.size();
    var i = 0;
    while (i < len) {
        var ch = data[i];
        switch (ch) {
            '"'  => emitter.append_view("\\\"");
            '\\' => emitter.append_view("\\\\");
            '\n' => emitter.append_view("\\n");
            '\r' => emitter.append_view("\\r");
            '\t' => emitter.append_view("\\t");
            default => emitter.append_char(ch)
        }
        i++;
    }
    emitter.append_view("\"");
}

public func <T : JsonStringEmitter> (emitter : &mut T) append_value(value : &JsonValue) {
    switch(value) {
        default => { emitter.append_view("UNKNOWN") }
        Null() => { emitter.append_view("null") }
        Bool(value) => {
            if(value) {
                emitter.append_view("true")
            } else {
                emitter.append_view("false")
            }
        }
        Number(value) => { emitter.append_view(value.to_view()) }
        String(value) => { escape_string_into(emitter, value); }
        Object(values) => {
            emitter.append_view("{");
            var itr = values.iterator();
            var first = true;
            while (itr.valid()) {
                if (!first) emitter.append_view(",");
                first = false;
                var key = itr.key()
                escape_string_into(emitter, key);
                emitter.append_view(":");
                var val = itr.value();
                emitter.append_value(val);
                itr.next();
            }
            emitter.append_view("}");
        }
        Array(values) => {
            emitter.append_view("[");
            var current = values.data();
            const end = current + values.size();
            var first = true;
            while (current != end) {
                if (!first) emitter.append_view(",");
                first = false;
                emitter.append_value(*current);
                current++;
            }
            emitter.append_view("]");
        }
    }
}

public func <T : JsonStringEmitter> (emitter : &mut T) append_value_pretty(value : &JsonValue, indent: int = 0) {
    switch(value) {
        default => { emitter.append_view("UNKNOWN") }
        Null() => { emitter.append_view("null") }
        Bool(value) => {
            if(value) {
                emitter.append_view("true")
            } else {
                emitter.append_view("false")
            }
        }
        Number(value) => { emitter.append_view(value.to_view()) }
        String(value) => { escape_string_into(emitter, value); }
        Object(values) => {
            emitter.append_view("{\n");
            var itr = values.iterator();
            var first = true;
            while (itr.valid()) {
                if (!first) emitter.append_view(",\n");
                first = false;

                // indent
                for (var i = 0; i < indent + 2; i++) { emitter.append_view(" "); }

                var key = itr.key()
                escape_string_into(emitter, key);
                emitter.append_view(": ");
                var val = itr.value();
                emitter.append_value_pretty(val, indent + 2);
                itr.next();
            }
            emitter.append_view("\n");
            for (var i = 0; i < indent; i++) { emitter.append_view(" "); }
            emitter.append_view("}");
        }
        Array(values) => {
            emitter.append_view("[\n");
            var current = values.data();
            const end = current + values.size();
            var first = true;
            while (current != end) {
                if (!first) emitter.append_view(",\n");
                first = false;

                for (var i = 0; i < indent + 2; i++) { emitter.append_view(" "); }
                emitter.append_value_pretty(*current, indent + 2);

                current++;
            }
            emitter.append_view("\n");
            for (var i = 0; i < indent; i++) { emitter.append_view(" "); }
            emitter.append_view("]");
        }
    }
}

public struct DebugJsonSaxHandler {

    var stream : CommandLineStream
    @make
    func make() {
        stream = CommandLineStream {}
    }

    func write_view(&self, view : &std::string_view) {
        stream.writeStr(view.data(), view.size())
    }

    func writeLine(&self) { stream.writeChar('\n') }

    func on_null(&self) {
        write_view(std::string_view("null\n"))
    }
    func on_bool(&self, value : bool) {
        write_view(std::string_view("bool : "))
        if(value) { write_view(std::string_view(" (true)\n")); } else { write_view(std::string_view(" (false)\n")); }
    }
    func on_number(&self, data : *char, len : size_t) {
        write_view(std::string_view("number : "))
        stream.writeStr(data, len)
        writeLine()
    }
    func on_string(&self, data : *char, len : size_t) {
        write_view(std::string_view("string : "))
        stream.writeStr(data, len)
        writeLine()
    }

    func on_object_begin(&self) {
       write_view(std::string_view("object begin\n"))
    }
    func on_object_end(&self) {
        write_view(std::string_view("object end\n"))
    }
    func on_array_begin(&self) {
        write_view(std::string_view("array begin\n"))
    }
    func on_array_end(&self) {
        write_view(std::string_view("array end\n"))
    }
    func on_key(&self, data : *char, len : size_t) {
        write_view(std::string_view("key : "))
        stream.writeStr(data, len)
        writeLine()
    }

}

public struct ASTJsonHandler : public JsonSaxHandler {

    var stack : std::vector<JsonValue>

    var key_stack : std::vector<std::string>

    var root : JsonValue;

    var have_root : bool = false

    // var d : DebugJsonSaxHandler

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
                if (key_stack.empty()) {
                    // malformed SAX sequence -- object value without a key
                    // convert to None or ignore; here we'll insert with empty key to avoid crash
                    var key = std::string();
                    var Object(values) = *top else unreachable
                    values.insert(key, v);
                } else {
                    var Object(values) = *top else unreachable
                    values.insert(key_stack.take_last(), v);
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
        // d.on_null()
        attach_value(JsonValue.Null())
    }

    @override
    func on_bool(&self, v : bool) {
        // d.on_bool(v)
        attach_value(JsonValue.Bool(v))
    }

    @override
    func on_number(&self, s : *char, len : size_t) {
        // d.on_number(s, len)
        var str = std::string()
        str.append_with_len(s, len)
        attach_value(JsonValue.Number(str))
    }

    @override
    func on_string(&self, s : *char, len : size_t) {
        // d.on_string(s, len)
        var str = std::string()
        str.append_with_len(s, len)
        attach_value(JsonValue.String(str))
    }

    @override
    func on_object_begin(&self) {
        // d.on_object_begin()
        // push an empty object
        stack.push(JsonValue::Object(std::unordered_map<std::string, JsonValue>()));
    }

    @override
    func on_object_end(&self) {
        // d.on_object_end()
        if (stack.empty()) {
            // unmatched end; ignore
            return;
        }
        attach_value(stack.take_last());
    }

    @override
    func on_array_begin(&self) {
        // d.on_array_begin()
        stack.push(JsonValue.Array(std::vector<JsonValue>()));
    }

    @override
    func on_array_end(&self) {
        // d.on_array_end()
        if (stack.empty()) {
            // unmatched end; ignore
            return;
        }
        attach_value(stack.take_last());
    }

    @override
    func on_key(&self, s : *char, len : size_t) {
        // d.on_key(s, len)
        key_stack.push(std::string(s, len))
    }

};