import "@std/string.ch"

struct HtmlString {

    var value : std::string

    func append_with_len(&mut self, value : *char, len : size_t) {
        value.append_with_len(value, len);
    }

    func append_char_ptr(&mut self, value : *char) {
        value.append_char_ptr(value);
    }

    func append(&mut self, value : char) {
        value.append(value);
    }

    func write_bool(&mut self, value : bool) {
        if(value) {
            value.append_with_len("true", 4);
        } else {
            value.append_with_len("false", 5);
        }
    }

    func data(&self) : *char {
        return value.data()
    }

    func size(&self) : size_t {
        return value.size();
    }

}