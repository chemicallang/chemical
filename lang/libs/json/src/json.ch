/* ParseResult: return status */
public struct ParseResult {
    var ok : bool;
    var pos : size_t; /* byte index where error occurred */
    var msg : *char; /* short error message */
    func Ok() : ParseResult { 
        return ParseResult {
            ok : true,
            pos : 0,
            msg : null
        };
    }
    func Err(p : size_t, m : *char) : ParseResult {
        return ParseResult {
            ok : false,
            pos : p,
            msg : m
        }
    }
};

@static
public interface JsonSaxHandler {

    /* Value callbacks */
    func on_null(&self, )
    func on_bool(&self, value : bool)
    func on_number(&self, data : *char, len : size_t)
    func on_string(&self, data : *char, len : size_t)

    /* Structural callbacks */
    func on_object_begin(&self)
    func on_object_end(&self)
    func on_array_begin(&self)
    func on_array_end(&self)
    func on_key(&self, data : *char, len : size_t)

}

public struct JsonParser {

    /* Configuration limits */
    var max_depth : size_t;
    var max_string : size_t; /* maximum unescaped string length */

    // a scratch buffer for small strings (fast path, no allocation)
    var scratch : [4096]char

    @make
    func make(max_depth_ : size_t = 128, max_string_ : size_t = 4096) {
        return JsonParser {
            max_depth : if(max_depth_ > 0) max_depth_ else 128,
            max_string : if(max_string_ > 0) max_string_ else 4096,
            scratch : [],
            s : null,
            len : 0,
            pos : 0,
            handler : null,
            heap_buf : null,
            heap_cap : 0,
            str_buf : null
        }
    }

    func parse(&mut self, buffer : *char, length : size_t, h : &mut JsonSaxHandler) : ParseResult {
        s = buffer; len = length; pos = 0; handler = &raw mut h;
        skip_ws();
        var r = parse_value(0);
        if (!r.ok) return r;
        skip_ws();
        if (pos != len) return ParseResult::Err(pos, "trailing data");
        return ParseResult::Ok();
    }

private:
    var s : *char;
    var len : size_t;
    var pos : size_t;
    var handler : *mut JsonSaxHandler;

    /* growable buffer for strings longer than the 4096 scratch */
    var heap_buf : *mut char = null;
    var heap_cap : size_t = 0;
    /* points at the active string buffer (scratch or heap) for the current parse */
    var str_buf : *mut char = null;

    func at_end(&self) : bool { return pos >= len; }
    func cur(&self) : char { return if(at_end()) '\0' else s[pos]; }
    func advance(&mut self, n : size_t = 1)  { pos += n; }

    func skip_ws(&mut self) {
        while (pos < len) {
            var c = s[pos] as uchar;
            if (c == ' ' || c == '\n' || c == '\r' || c == '\t') { pos++; continue; }
            break;
        }
    }

    func parse_value(&mut self, depth : size_t) : ParseResult {
        if (depth > max_depth) return ParseResult::Err(pos, "exceeded max depth");
        if (at_end()) return ParseResult::Err(pos, "unexpected end");
        var c : char = cur();
        if (c == '{') return parse_object(depth + 1);
        if (c == '[') return parse_array(depth + 1);
        if (c == '"') {
            return parse_string_value();
        }
        if (c == 't') return parse_literal("true", 4, 0);
        if (c == 'f') return parse_literal("false", 5, 1);
        if (c == 'n') return parse_literal("null", 4, 2);
        if (c == '-' || (c >= '0' && c <= '9')) {
            return parse_number();
        }
        return ParseResult::Err(pos, "unexpected character");
    }

    /* parse literal like true/false/null */
    func parse_literal(&mut self, lit : *char, litlen : size_t, type : int) : ParseResult {
        if (pos + litlen > len) return ParseResult::Err(pos, "unexpected end in literal");
        var i : size_t = 0;
        for (i = 0; i < litlen; ++i) {
            if (s[pos + i] != lit[i]) return ParseResult::Err(pos, "invalid literal");
        }
        pos += litlen;
        switch(type) {
            0 => { handler.on_bool(true); }
            1 => { handler.on_bool(false); }
            default, 2 => {  handler.on_null(); }
        }
        return ParseResult::Ok();
    }

    func parse_number(&mut self) : ParseResult {
        var start : size_t = pos;
        if (cur() == '-') advance();
        if (at_end()) return ParseResult::Err(pos, "unexpected end in number");
        if (cur() == '0') {
            advance();
            /* leading zero must not be followed by digit */
            if (!at_end() && isdigit(cur() as int)) return ParseResult::Err(pos, "leading zero in number");
        } else {
            if (!isdigit(cur() as int)) return ParseResult::Err(pos, "invalid number");
            while (!at_end() && isdigit(cur() as int)) { advance(); }
        }
        if (!at_end() && cur() == '.') {
            advance();
            if (at_end() || !isdigit(cur() as int)) return ParseResult::Err(pos, "invalid fraction");
            while (!at_end() && isdigit(cur() as int)) { advance(); }
        }
        if (!at_end() && (cur() == 'e' || cur() == 'E')) {
            advance();
            if (!at_end() && (cur() == '+' || cur() == '-')) advance();
            if (at_end() || !isdigit(cur() as int)) return ParseResult::Err(pos, "invalid exponent");
            while (!at_end() && isdigit(cur() as int)) { advance(); }
        }
        var nlen : size_t = pos - start;
        handler.on_number(s + start, nlen);
        return ParseResult::Ok();
    }

    func parse_object(&mut self, depth : size_t) : ParseResult {
        /* expect '{' */
        if (cur() != '{') return ParseResult::Err(pos, "expected '{'");
        advance();
        handler.on_object_begin();
        skip_ws();
        if (!at_end() && cur() == '}') { advance(); handler.on_object_end(); return ParseResult::Ok(); }
        loop {
            skip_ws();
            if (at_end()) return ParseResult::Err(pos, "unexpected end in object");
            if (cur() != '"') return ParseResult::Err(pos, "expected string key");
            /* parse key into string buffer */
            var r : ParseResult = parse_string_key();
            if (!r.ok) return r;
            skip_ws();
            if (at_end() || cur() != ':') return ParseResult::Err(pos, "expected ':' after key");
            advance();
            skip_ws();
            /* parse value */
            r = parse_value(depth);
            if (!r.ok) return r;
            skip_ws();
            if (at_end()) return ParseResult::Err(pos, "unexpected end in object");
            if (cur() == ',') { advance(); continue; }
            if (cur() == '}') { advance(); handler.on_object_end(); return ParseResult::Ok(); }
            return ParseResult::Err(pos, "expected ',' or '}' in object");
        }
    }

    func parse_array(&mut self, depth : size_t) : ParseResult {
        if (cur() != '[') return ParseResult::Err(pos, "expected '['");
        advance();
        handler.on_array_begin();
        skip_ws();
        if (!at_end() && cur() == ']') { advance(); handler.on_array_end(); return ParseResult::Ok(); }
        loop {
            skip_ws();
            var r : ParseResult = parse_value(depth);
            if (!r.ok) return r;
            skip_ws();
            if (at_end()) return ParseResult::Err(pos, "unexpected end in array");
            if (cur() == ',') { advance(); continue; }
            if (cur() == ']') { advance(); handler.on_array_end(); return ParseResult::Ok(); }
            return ParseResult::Err(pos, "expected ',' or ']' in array");
        }
    }

    /* Ensure there is room to write `need` more bytes at index cur_len.
       Returns a pointer to the active buffer (scratch or a growable heap buffer),
       or null if the resulting length would exceed max_string. The previously
       written bytes are preserved across any switch to the heap buffer. */
    func ensure_buf(&mut self, cur_len : size_t, need : size_t) : *mut char {
        var required : size_t = cur_len + need;
        if (required > self.max_string) return null;
        if (required <= 4096) {
            return &raw mut scratch[0];
        }
        if (self.heap_buf == null) {
            var init_cap : size_t = 8192;
            if (init_cap < required) init_cap = required;
            if (init_cap > self.max_string) init_cap = self.max_string;
            self.heap_cap = init_cap;
            self.heap_buf = malloc(self.heap_cap) as *mut char;
            memcpy(self.heap_buf, &raw mut scratch[0], cur_len);
            self.str_buf = self.heap_buf;
        } else if (required > self.heap_cap) {
            var newcap : size_t = self.heap_cap * 2;
            if (newcap < required) newcap = required;
            if (newcap > self.max_string) newcap = self.max_string;
            var newbuf = malloc(newcap) as *mut char;
            memcpy(newbuf, self.heap_buf, cur_len);
            free(self.heap_buf);
            self.heap_buf = newbuf;
            self.heap_cap = newcap;
        }
        return self.heap_buf;
    }

    func emit_char(&mut self, c : char, outlen : &mut size_t) : bool {
        var buf = self.ensure_buf(*outlen, 1);
        if (buf == null) return false;
        buf[*outlen] = c;
        *outlen += 1;
        return true;
    }

    func emit_codepoint(&mut self, cp : uint32_t, outlen : &mut size_t) : bool {
        if (cp <= 0x7F) {
            return self.emit_char(cp as char, outlen);
        } else if (cp <= 0x7FF) {
            var buf = self.ensure_buf(*outlen, 2);
            if (buf == null) return false;
            buf[*outlen] = (0xC0 | ((cp >> 6) & 0x1F)) as char;
            buf[*outlen + 1] = (0x80 | (cp & 0x3F)) as char;
            *outlen += 2;
            return true;
        } else if (cp <= 0xFFFF) {
            var buf = self.ensure_buf(*outlen, 3);
            if (buf == null) return false;
            buf[*outlen] = (0xE0 | ((cp >> 12) & 0x0F)) as char;
            buf[*outlen + 1] = (0x80 | ((cp >> 6) & 0x3F)) as char;
            buf[*outlen + 2] = (0x80 | (cp & 0x3F)) as char;
            *outlen += 3;
            return true;
        } else {
            var buf = self.ensure_buf(*outlen, 4);
            if (buf == null) return false;
            buf[*outlen] = (0xF0 | ((cp >> 18) & 0x07)) as char;
            buf[*outlen + 1] = (0x80 | ((cp >> 12) & 0x3F)) as char;
            buf[*outlen + 2] = (0x80 | ((cp >> 6) & 0x3F)) as char;
            buf[*outlen + 3] = (0x80 | (cp & 0x3F)) as char;
            *outlen += 4;
            return true;
        }
    }

    func free_str_buf(&mut self) {
        if (self.heap_buf != null) {
            free(self.heap_buf);
            self.heap_buf = null;
            self.heap_cap = 0;
        }
        self.str_buf = null;
    }

    /* parse a JSON string starting at current pos (expects '"').
       Unescape into a growable buffer. Strict: control chars (0x00-0x1F) forbidden.
       Calls handler.on_string with the resulting (unterminated) buffer. */
    func parse_string_value(&mut self) : ParseResult {
        var outlen : size_t = 0;
        var r = self.parse_string_body(&mut outlen);
        if (!r.ok) { self.free_str_buf(); return r; }
        handler.on_string(self.str_buf, outlen);
        self.free_str_buf();
        return ParseResult::Ok();
    }

    func parse_string_key(&mut self) : ParseResult {
        var outlen : size_t = 0;
        var r = self.parse_string_body(&mut outlen);
        if (!r.ok) { self.free_str_buf(); return r; }
        handler.on_key(self.str_buf, outlen);
        self.free_str_buf();
        return ParseResult::Ok();
    }

    func parse_string_body(&mut self, outlen : &mut size_t) : ParseResult {
        if (cur() != '"') return ParseResult::Err(pos, "expected '\"'");
        advance(); /* skip '"' */
        *outlen = 0 as size_t;
        self.str_buf = &raw mut scratch[0];
        while (!at_end()) {
            var c = cur() as uchar;
            if (c == '"') { advance(); return ParseResult::Ok(); }
            if (c <= 0x1F) return ParseResult::Err(pos, "control char in string");
            if (c == '\\') {
                /* escape */
                advance();
                if (at_end()) return ParseResult::Err(pos, "unterminated escape");
                var e = cur() as uchar;
                advance();
                if (e == '"') {
                    if (!self.emit_char('"', outlen)) return ParseResult::Err(pos, "string exceeds max_string");
                } else if (e == '\\') {
                    if (!self.emit_char('\\', outlen)) return ParseResult::Err(pos, "string exceeds max_string");
                } else if (e == '/') {
                    if (!self.emit_char('/', outlen)) return ParseResult::Err(pos, "string exceeds max_string");
                } else if (e == 'b') {
                    if (!self.emit_char('\b', outlen)) return ParseResult::Err(pos, "string exceeds max_string");
                } else if (e == 'f') {
                    if (!self.emit_char('\f', outlen)) return ParseResult::Err(pos, "string exceeds max_string");
                } else if (e == 'n') {
                    if (!self.emit_char('\n', outlen)) return ParseResult::Err(pos, "string exceeds max_string");
                } else if (e == 'r') {
                    if (!self.emit_char('\r', outlen)) return ParseResult::Err(pos, "string exceeds max_string");
                } else if (e == 't') {
                    if (!self.emit_char('\t', outlen)) return ParseResult::Err(pos, "string exceeds max_string");
                } else if (e == 'u') {
                    /* expect 4 hex digits */
                    if (pos + 4 > len) return ParseResult::Err(pos, "incomplete unicode escape");
                    var code : uint32_t = 0;
                    for (var k : uint = 0; k < 4; ++k) {
                        var hc = s[pos + k] as uchar;
                        code <<= 4;
                        if (hc >= '0' && hc <= '9') code |= (hc - '0');
                        else if (hc >= 'A' && hc <= 'F') code |= (hc - 'A' + 10);
                        else if (hc >= 'a' && hc <= 'f') code |= (hc - 'a' + 10);
                        else return ParseResult::Err(pos + k, "invalid hex in unicode escape");
                    }
                    pos += 4;
                    /* handle surrogate pairs for UTF-16 escapes */
                    if (code >= 0xD800 && code <= 0xDBFF) {
                        /* high surrogate, expect \uXXXX for low surrogate */
                        if (pos + 2 >= len || s[pos] != '\\' || s[pos+1] != 'u') return ParseResult::Err(pos, "expected low surrogate");
                        pos += 2;
                        if (pos + 4 > len) return ParseResult::Err(pos, "incomplete unicode escape");
                        var lo : uint32_t = 0;
                        for (var k : uint = 0; k < 4; ++k) {
                            var hc = s[pos + k] as uchar;
                            lo <<= 4;
                            if (hc >= '0' && hc <= '9') lo |= (hc - '0');
                            else if (hc >= 'A' && hc <= 'F') lo |= (hc - 'A' + 10);
                            else if (hc >= 'a' && hc <= 'f') lo |= (hc - 'a' + 10);
                            else return ParseResult::Err(pos + k, "invalid hex in unicode escape");
                        }
                        pos += 4;
                        if (!(lo >= 0xDC00 && lo <= 0xDFFF)) return ParseResult::Err(pos, "invalid low surrogate");
                        var full : uint32_t = (0x10000 + (((code - 0xD800) << 10) | (lo - 0xDC00))) as uint32_t;
                        if (!self.emit_codepoint(full, outlen)) return ParseResult::Err(pos, "string exceeds max_string or invalid unicode");
                    } else {
                        if (!self.emit_codepoint(code, outlen)) return ParseResult::Err(pos, "string exceeds max_string or invalid unicode");
                    }
                } else return ParseResult::Err(pos, "invalid escape");
            } else {
                /* regular character: copy as-is (assume UTF-8 in source) */
                if (!self.emit_char(c as char, outlen)) { return ParseResult::Err(pos, "string exceeds max_string"); }
                advance();
            }
        }
        return ParseResult::Err(pos, "unterminated string");
    }
};
