public struct DebugJsonSaxHandler {

    var stream : CommandLineStream
    @make
    func make() {
        return DebugJsonSaxHandler {
            stream : CommandLineStream {}
        }
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

public struct ASTJsonHandler {

    var stack : std::vector<JsonValue>

    var key_stack : std::vector<std::string>

    var root : JsonValue;

    var have_root : bool = false

    @make
    func make() {
        return ASTJsonHandler {
            stack : std::vector<JsonValue>(),
            key_stack : std::vector<std::string>(),
            root : JsonValue.Null(),
            have_root : false
        }
    }

    func attach_value(&mut self, v : JsonValue) {
        if (!stack.empty()) {
            var top = stack.last_ptr();
            if (top is JsonValue.Array) {
                var Array(values) = *top else unreachable
                values.push(v);
            } else if (top is JsonValue.Object) {
                if (key_stack.empty()) {
                    var key = std::string();
                    var Object(values) = *top else unreachable
                    values.insert(key, v);
                } else {
                    var Object(values) = *top else unreachable
                    values.insert(key_stack.take_last(), v);
                }
            } else {
                root = v;
                have_root = true;
            }
        } else {
            root = v;
            have_root = true;
        }
    }

};

impl JsonSaxHandler for ASTJsonHandler {
    func on_null(&mut self) {
        attach_value(JsonValue.Null())
    }

    func on_bool(&mut self, v : bool) {
        attach_value(JsonValue.Bool(v))
    }

    func on_number(&mut self, s : *char, len : size_t) {
        var str = std::string()
        str.append_with_len(s, len)
        attach_value(JsonValue.Number(str))
    }

    func on_string(&mut self, s : *char, len : size_t) {
        var str = std::string()
        str.append_with_len(s, len)
        attach_value(JsonValue.String(str))
    }

    func on_object_begin(&mut self) {
        stack.push(JsonValue::Object(std::unordered_map<std::string, JsonValue>()));
    }

    func on_object_end(&mut self) {
        if (stack.empty()) {
            return;
        }
        attach_value(stack.take_last());
    }

    func on_array_begin(&mut self) {
        stack.push(JsonValue.Array(std::vector<JsonValue>()));
    }

    func on_array_end(&mut self) {
        if (stack.empty()) {
            return;
        }
        attach_value(stack.take_last());
    }

    func on_key(&mut self, s : *char, len : size_t) {
        key_stack.push(std::string(s, len))
    }
}
