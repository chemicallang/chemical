import "@std/string.ch"

struct HtmlString {

    var str : std::string

    func append_with_len(&mut self, value : *char, len : size_t) {
        str.append_with_len(value, len);
    }

    func append_char_ptr(&mut self, value : *char) {
        str.append_char_ptr(value);
    }

    func append(&mut self, value : char) {
        str.append(value);
    }

    func write_bool(&mut self, value : bool) {
        if(value) {
            str.append_with_len("true", 4);
        } else {
            str.append_with_len("false", 5);
        }
    }

    func data(&self) : *char {
        return str.data()
    }

    func size(&self) : size_t {
        return str.size();
    }

}