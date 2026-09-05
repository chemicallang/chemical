// Compact / pretty JSON emission from a JsonValue into a std::string.
// Escaping shares the single implementation in encode.ch (json_escape_into).

func (output : &mut std::string) append_value_inner(value : &JsonValue, pretty : bool, indent : int) {
    switch(value) {
        default => { output.append_char_ptr("UNKNOWN") }
        Null() => { output.append_char_ptr("null") }
        Bool(value) => {
            if(value) {
                output.append_char_ptr("true")
            } else {
                output.append_char_ptr("false")
            }
        }
        Number(value) => { output.append_view(value.to_view()) }
        String(value) => { json_escape_into(output, value.data(), value.size()) }
        Object(values) => {
            if(pretty) { output.append_char_ptr("{\n") } else { output.append('{') }
            var itr = values.iterator();
            var first = true;
            while (itr.valid()) {
                if (!first) {
                    if(pretty) { output.append_char_ptr(",\n") } else { output.append(',') }
                }
                first = false;

                if(pretty) {
                    for (var i = 0; i < indent + 2; i++) { output.append(' ') }
                }

                var key = itr.key()
                json_escape_into(output, key.data(), key.size())
                if(pretty) { output.append_char_ptr(": ") } else { output.append(':') }
                var val = itr.value();
                output.append_value_inner(val, pretty, indent + 2);
                itr.next();
            }
            if(pretty) {
                output.append_char_ptr("\n")
                for (var i = 0; i < indent; i++) { output.append(' ') }
            }
            output.append('}');
        }
        Array(values) => {
            if(pretty) { output.append_char_ptr("[\n") } else { output.append('[') }
            var current = values.data();
            const end = current + values.size();
            var first = true;
            while (current != end) {
                if (!first) {
                    if(pretty) { output.append_char_ptr(",\n") } else { output.append(',') }
                }
                first = false;

                if(pretty) {
                    for (var i = 0; i < indent + 2; i++) { output.append(' ') }
                }
                output.append_value_inner(&*current, pretty, indent + 2);

                current++;
            }
            if(pretty) {
                output.append_char_ptr("\n")
                for (var i = 0; i < indent; i++) { output.append(' ') }
            }
            output.append(']');
        }
    }
}

public func (output : &mut std::string) append_value(value : &JsonValue) {
    output.append_value_inner(value, false, 0)
}

public func (output : &mut std::string) append_value_pretty(value : &JsonValue, indent : int = 0) {
    output.append_value_inner(value, true, indent)
}