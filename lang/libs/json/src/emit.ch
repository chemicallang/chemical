public interface JsonStringEmitter {

    func append_view(&self, view : &std::string_view)

    func append_char(&self, ch : char)

}

public struct JsonStringPrinter {
    var stream : CommandLineStream
    @make
    func make() {
        return JsonStringPrinter {
            stream : CommandLineStream {}
        }
    }
}

impl JsonStringEmitter for JsonStringPrinter {
    func append_view(&self, view : &std::string_view) {
        stream.writeStr(view.data(), view.size())
    }
    func append_char(&self, ch : char) {
        stream.writeChar(ch)
    }
}

public struct JsonStringBuilder {
    var ptr : &mut std::string
}

impl JsonStringEmitter for JsonStringBuilder {
    func append_view(&mut self, view : &std::string_view) {
        ptr.append_view(view)
    }
    func append_char(&mut self, ch : char) {
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
            '\b' => emitter.append_view("\\b");
            '\f' => emitter.append_view("\\f");
            '\n' => emitter.append_view("\\n");
            '\r' => emitter.append_view("\\r");
            '\t' => emitter.append_view("\\t");
            default => {
                const uch = ch as uchar;
                if (uch <= 0x1F) {
                    emitter.append_view("\\u00");
                    const hex = "0123456789abcdef";
                    emitter.append_char(hex[(uch >> 4) & 0xF]);
                    emitter.append_char(hex[uch & 0xF]);
                } else {
                    emitter.append_char(ch)
                }
            }
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
        String(value) => { escape_string_into(emitter, &value); }
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
                emitter.append_value(&*current);
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
        String(value) => { escape_string_into(emitter, &value); }
        Object(values) => {
            emitter.append_view("{\n");
            var itr = values.iterator();
            var first = true;
            while (itr.valid()) {
                if (!first) emitter.append_view(",\n");
                first = false;

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
                emitter.append_value_pretty(&*current, indent + 2);

                current++;
            }
            emitter.append_view("\n");
            for (var i = 0; i < indent; i++) { emitter.append_view(" "); }
            emitter.append_view("]");
        }
    }
}
