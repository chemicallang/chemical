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

/* Helper: append codepoint as UTF-8 into out buffer. Return false if not enough space. */
func append_utf8(cp : uint32_t, out : *mut char, outpos : &mut size_t, outcap : size_t) : bool {
    if (cp <= 0x7F) {
        if (*outpos + 1 > outcap) return false;
        out[outpos++] = cp as char;
        return true;
    } else if (cp <= 0x7FF) {
        if (*outpos + 2 > outcap) return false;
        out[outpos++] = (0xC0 | ((cp >> 6) & 0x1F)) as char;
        out[outpos++] = (0x80 | (cp & 0x3F)) as char;
        return true;
    } else if (cp <= 0xFFFF) {
        if (*outpos + 3 > outcap) return false;
        out[outpos++] = (0xE0 | ((cp >> 12) & 0x0F)) as char;
        out[outpos++] = (0x80 | ((cp >> 6) & 0x3F)) as char;
        out[outpos++] = (0x80 | (cp & 0x3F)) as char;
        return true;
    } else if (cp <= 0x10FFFF) {
        if (*outpos + 4 > outcap) return false;
        out[outpos++] = (0xF0 | ((cp >> 18) & 0x07)) as char;
        out[outpos++] = (0x80 | ((cp >> 12) & 0x3F)) as char;
        out[outpos++] = (0x80 | ((cp >> 6) & 0x3F)) as char;
        out[outpos++] = (0x80 | (cp & 0x3F)) as char;
        return true;
    }
    return false;
}

public struct JsonParser {

    /* Configuration limits */
    var max_depth : size_t;
    var max_string : size_t; /* maximum unescaped string length */

    // a scratch buffer for usage
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
            handler : null
        }
    }

    func parse(&mut self, buffer : *char, length : size_t, h : &mut JsonSaxHandler) : ParseResult {
        s = buffer; len = length; pos = 0; handler = &mut h;
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
            /* use stack buffer for unescaped string; small default, no heap */
            var out = &mut scratch[0]
            var outcap : size_t = 4096;
            var outlen : size_t = 0;
            var r : ParseResult = parse_string_inplace(out, outcap, outlen);
            if (!r.ok) return r;
            handler.on_string(out, outlen);
            return ParseResult::Ok();
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
        var i : size_t;
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
            /* parse key into stack buffer */
            var out = &mut scratch[0]
            var outcap : size_t = 4096;
            var outlen : size_t = 0;
            var r : ParseResult = parse_string_inplace(out, outcap, outlen);
            if (!r.ok) return r;
            handler.on_key(out, outlen);
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

    /* parse a JSON string starting at current pos (expects '"').
       Unescape into provided out buffer with capacity outcap. outlen set to length.
       Uses strict rules: control characters (0x00-0x1F) are forbidden.
    */
    func parse_string_inplace(&mut self, out : *mut char, outcap : size_t, outlen : &mut size_t) : ParseResult {
        if (cur() != '"') return ParseResult::Err(pos, "expected '\"'");
        advance(); /* skip '"' */
        *outlen = 0 as size_t;
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
                    if (*outlen + 1 > outcap) return ParseResult::Err(pos, "string too long"); out[outlen++] = '"';
                } else if (e == '\\') {
                    if (*outlen + 1 > outcap) return ParseResult::Err(pos, "string too long"); out[outlen++] = '\\';
                } else if (e == '/') {
                    if (*outlen + 1 > outcap) return ParseResult::Err(pos, "string too long"); out[outlen++] = '/';
                } else if (e == 'b') {
                    if (*outlen + 1 > outcap) return ParseResult::Err(pos, "string too long"); out[outlen++] = '\b';
                } else if (e == 'f') {
                    if (*outlen + 1 > outcap) return ParseResult::Err(pos, "string too long"); out[outlen++] = '\f';
                } else if (e == 'n') {
                    if (*outlen + 1 > outcap) return ParseResult::Err(pos, "string too long"); out[outlen++] = '\n';
                } else if (e == 'r') {
                    if (*outlen + 1 > outcap) return ParseResult::Err(pos, "string too long"); out[outlen++] = '\r';
                } else if (e == 't') {
                    if (*outlen + 1 > outcap) return ParseResult::Err(pos, "string too long"); out[outlen++] = '\t';
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
                        if (!append_utf8(full, out, outlen, outcap)) return ParseResult::Err(pos, "string too long or invalid unicode");
                    } else {
                        if (!append_utf8(code, out, outlen, outcap)) return ParseResult::Err(pos, "string too long or invalid unicode");
                    }
                } else return ParseResult::Err(pos, "invalid escape");
                if (*outlen > max_string) return ParseResult::Err(pos, "string exceeds max_string");
            } else {
                /* regular character: copy as-is (assume UTF-8 in source) */
                if (*outlen + 1 > outcap) return ParseResult::Err(pos, "string too long");
                out[outlen++] = c as char;
                advance();
                if (*outlen > max_string) return ParseResult::Err(pos, "string exceeds max_string");
            }
        }
        return ParseResult::Err(pos, "unterminated string");
    }
};
