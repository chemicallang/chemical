public namespace regex {

    public struct CharRange {
        var lo : i64
        var hi : i64
    }

    public struct CharClass {
        var ranges : std::vector<CharRange>
        var negated : bool
    }

    public struct NFAState {
        var kind : i64
        var ch : i64
        var class_idx : i64
        var out1 : i64
        var out2 : i64
        var save_idx : i64
    }

    public struct PatchPtr {
        var state_idx : i64
        var slot : i64
    }

    public struct VisitedFrame {
        var state_idx : i64
        var pos : size_t
    }

    public struct Frag {
        var start : i64
        var outs : std::vector<PatchPtr>
    }

    public struct Captures {
        var matched : bool
        var positions : std::vector<i64>

        public func size(&self) : size_t {
            return self.positions.size()
        }
    }

    public struct Regex {
        var states : std::vector<NFAState>
        var classes : std::vector<CharClass>
        var start_state : i64
        var num_captures : i64
        var pattern : std::string
        var compile_error : std::string

        @constructor
        public func init() : Regex {
            return Regex {
                states : std::vector<NFAState>(),
                classes : std::vector<CharClass>(),
                start_state : -1,
                num_captures : 0,
                pattern : std::string(),
                compile_error : std::string()
            }
        }

        public func is_match(&self, text : std::string_view) : bool {
            if(self.start_state < 0) return false
            var text_len = text.size()
            var i : size_t = 0
            while(i < text_len) {
                if(run_prog(self.states, self.classes, self.start_state, text, i, text_len)) {
                    return true
                }
                i = i + 1
            }
            return run_prog(self.states, self.classes, self.start_state, text, text_len, text_len)
        }

        public func find(&self, text : std::string_view, start_pos : *mut i64, end_pos : *mut i64) : bool {
            *start_pos = -1
            *end_pos = -1
            if(self.start_state < 0) return false
            return find_from(self.states, self.classes, self.start_state, text, 0, start_pos, end_pos)
        }

        public func captures(&self, text : std::string_view) : Captures {
            if(self.start_state < 0) {
                return Captures { matched : false, positions : std::vector<i64>() }
            }
            var text_len = text.size()
            var i : size_t = 0
            while(i <= text_len) {
                var positions = std::vector<i64>()
                var found = find_captures(self.states, self.classes, self.start_state, text, i, self.num_captures, &mut positions)
                if(found) {
                    return Captures { matched : true, positions : positions }
                }
                i = i + 1
            }
            return Captures { matched : false, positions : std::vector<i64>() }
        }

        public func replace(&self, text : std::string_view, replacement : std::string_view) : std::string {
            var result = std::string()
            var pos : size_t = 0
            var text_len = text.size()
            while(pos <= text_len) {
                var start_out : i64 = 0
                var end_out : i64 = 0
                var found = find_from(self.states, self.classes, self.start_state, text, pos, &mut start_out, &mut end_out)
                if(!found) {
                    while(pos < text_len) {
                        result.append(text.get(pos))
                        pos = pos + 1
                    }
                    break
                }

                while(pos < (start_out as size_t)) {
                    result.append(text.get(pos))
                    pos = pos + 1
                }
                
                var replen : size_t = 0
                while(replen < replacement.size()) {
                    result.append(replacement.get(replen))
                    replen = replen + 1
                }
                
                if(pos == (end_out as size_t)) {
                    if(pos < text_len) {
                        result.append(text.get(pos))
                        pos = pos + 1
                    } else {
                        break
                    }
                } else {
                    pos = end_out as size_t
                }
            }
            return result
        }

        public func split(&self, text : std::string_view, out : *mut std::vector<std::string_view>) {
            var pos : size_t = 0
            var text_len = text.size()
            while(pos <= text_len) {
                var start_out : i64 = 0
                var end_out : i64 = 0
                var found = find_from(self.states, self.classes, self.start_state, text, pos, &mut start_out, &mut end_out)
                if(!found) {
                    var remaining = text.subview(pos, text_len)
                    out.push(remaining)
                    break
                }

                var seg = text.subview(pos, start_out as size_t)
                out.push(seg)
                
                if(start_out == end_out) {
                    if((end_out as size_t) < text_len) {
                        out.push(text.subview(end_out as size_t, (end_out as size_t) + 1))
                        pos = (end_out as size_t) + 1
                    } else {
                        break
                    }
                } else {
                    pos = end_out as size_t
                }
            }
        }
    }

    // =========================================================================
    // Top-level compile function
    // =========================================================================
    public func compile(pattern : std::string_view) : Regex {
        var re = Regex::init()
        re.pattern.append_view(pattern)
        
        var ok = compile_internal(pattern, re.states, re.classes, &mut re.start_state, &mut re.num_captures, re.compile_error)

        if(!ok) {
            if(re.compile_error.empty()) {
                re.compile_error.append_view("unknown regex error")
            }
        }
        return re
    }

    // =========================================================================
    // NFA helpers
    // =========================================================================
    internal func new_state(states : &mut std::vector<NFAState>,
        kind : i64, ch : i64, class_idx : i64,
        out1 : i64, out2 : i64, save_idx : i64) : i64 {
        var idx = states.size() as i64
        states.push(NFAState {
            kind : kind, ch : ch, class_idx : class_idx,
            out1 : out1, out2 : out2, save_idx : save_idx
        })
        return idx
    }

    internal func make_patch(state_idx : i64, slot : i64) : PatchPtr {
        return PatchPtr { state_idx : state_idx, slot : slot }
    }

    internal func copy_patch_list(src : &std::vector<PatchPtr>) : std::vector<PatchPtr> {
        var result = std::vector<PatchPtr>()
        var i : size_t = 0
        while(i < src.size()) {
            result.push(src.get(i))
            i = i + 1
        }
        return result
    }

    internal func patch_list(outs : &mut std::vector<PatchPtr>, target : i64,
        states : &mut std::vector<NFAState>) {
        var i : size_t = 0
        while(i < outs.size()) {
            var p = outs.get(i)
            if(p.slot == 0) {
                states.get_ptr(p.state_idx as size_t).out1 = target
            } else {
                states.get_ptr(p.state_idx as size_t).out2 = target
            }
            i = i + 1
        }
    }

    internal func append_patch_list(dst : &mut std::vector<PatchPtr>, src : &std::vector<PatchPtr>) {
        var i : size_t = 0
        while(i < src.size()) {
            dst.push(src.get(i))
            i = i + 1
        }
    }

    // =========================================================================
    // Parser state
    // =========================================================================
    public struct Parser {
        var pattern : std::string_view
        var pos : size_t
        var group_count : i64
        var error : std::string
    }

    internal func peek_char(p : &Parser) : i64 {
        if(p.pos >= p.pattern.size()) return -1
        return p.pattern.get(p.pos) as i64
    }

    internal func advance_char(p : &mut Parser) : i64 {
        if(p.pos >= p.pattern.size()) return -1
        var c = p.pattern.get(p.pos)
        p.pos = p.pos + 1
        return c as i64
    }

    internal func expect_char(p : &mut Parser, expected : i64) : bool {
        var c = peek_char(p)
        if(c != expected) {
            p.error.append_view("expected '")
            p.error.append(expected as char)
            p.error.append_view("' at pos ")
            p.error.append_integer(p.pos as bigint)
            return false
        }
        p.pos = p.pos + 1
        return true
    }

    // =========================================================================
    // Parse helpers
    // =========================================================================
    internal func parse_class_item(p : &mut Parser) : i64 {
        var c = advance_char(p)
        if(c < 0) return -1
        if(c == '\\' as i64) {
            return parse_escape_char(p)
        }
        return c
    }

    internal func parse_escape_char(p : &mut Parser) : i64 {
        var c = advance_char(p)
        if(c < 0) {
            p.error.append_view("trailing backslash")
            return -1
        }
        if(c == 'd' as i64) return -2
        if(c == 'D' as i64) return -3
        if(c == 'w' as i64) return -4
        if(c == 'W' as i64) return -5
        if(c == 's' as i64) return -6
        if(c == 'S' as i64) return -7
        return c
    }

    internal func is_escape_class(c : i64) : bool {
        return c == -2 || c == -3 || c == -4 || c == -5 || c == -6 || c == -7
    }

    // =========================================================================
    // Character class compilation
    // =========================================================================
    internal func parse_char_class(p : &mut Parser,
        classes : &mut std::vector<CharClass>) : i64 {
        var negated = false
        var c = peek_char(p)
        if(c == '^' as i64) {
            negated = true
            advance_char(p)
        }

        var idx = classes.size() as i64
        classes.push(CharClass { ranges : std::vector<CharRange>(), negated : negated })
        var cd = classes.get_ptr(idx as size_t)
        var first = true

        loop {
            c = peek_char(p)
            if(c < 0) {
                p.error.append_view("unclosed character class")
                return -1
            }
            if(!first && c == ']' as i64) {
                advance_char(p)
                break
            }
            first = false

            var lo = parse_class_item(p)
            if(lo < 0) return -1

            c = peek_char(p)
            if(c == '-' as i64) {
                var next_pos = p.pos + 1
                var next : i64 = -1
                if(next_pos < p.pattern.size()) {
                    next = p.pattern.get(next_pos) as i64
                }
                if(next >= 0 && next != ']' as i64) {
                    advance_char(p)
                    var hi = parse_class_item(p)
                    if(hi < 0) return -1
                    if(lo > hi) {
                        var tmp = lo
                        lo = hi
                        hi = tmp
                    }
                    cd.ranges.push(CharRange { lo : lo, hi : hi })
                    continue
                }
            }

            cd.ranges.push(CharRange { lo : lo, hi : lo })
        }

        return idx
    }

    internal func make_escape_class(esc : i64, classes : &mut std::vector<CharClass>) : i64 {
        var idx = classes.size() as i64
        classes.push(CharClass { ranges : std::vector<CharRange>(), negated : false })
        var cd = classes.get_ptr(idx as size_t)
        
        if(esc == -2) {
            cd.ranges.push(CharRange { lo : 48, hi : 57 })
        } else if(esc == -3) {
            cd.negated = true
            cd.ranges.push(CharRange { lo : 48, hi : 57 })
        } else if(esc == -4) {
            cd.ranges.push(CharRange { lo : 48, hi : 57 })
            cd.ranges.push(CharRange { lo : 65, hi : 90 })
            cd.ranges.push(CharRange { lo : 97, hi : 122 })
            cd.ranges.push(CharRange { lo : 95, hi : 95 })
        } else if(esc == -5) {
            cd.negated = true
            cd.ranges.push(CharRange { lo : 48, hi : 57 })
            cd.ranges.push(CharRange { lo : 65, hi : 90 })
            cd.ranges.push(CharRange { lo : 97, hi : 122 })
            cd.ranges.push(CharRange { lo : 95, hi : 95 })
        } else if(esc == -6) {
            cd.ranges.push(CharRange { lo : 9, hi : 13 })
            cd.ranges.push(CharRange { lo : 32, hi : 32 })
        } else if(esc == -7) {
            cd.negated = true
            cd.ranges.push(CharRange { lo : 9, hi : 13 })
            cd.ranges.push(CharRange { lo : 32, hi : 32 })
        }
        return idx
    }

    // =========================================================================
    // Compile pattern to NFA
    // =========================================================================
    internal func compile_internal(pattern : std::string_view,
        states : &mut std::vector<NFAState>,
        classes : &mut std::vector<CharClass>,
        start_state : *mut i64,
        num_captures : *mut i64,
        error : &mut std::string) : bool {

        if(pattern.size() == 0) {
            var match = new_state(states, 3, 0, 0, -1, -1, 0)
            *start_state = match
            *num_captures = 1
            return true
        }

        var p = Parser {
            pattern : pattern, pos : 0,
            group_count : 0, error : std::string()
        }

        var frag = parse_regex(p, states, classes, &mut p.group_count)

        if(p.error.empty() && p.pos != pattern.size()) {
            p.error.append_view("unexpected character at pos ")
            p.error.append_integer(p.pos as bigint)
        }

        if(!p.error.empty()) {
            error.append_view(p.error.to_view())
            return false
        }

        var match_state = new_state(states, 3, 0, 0, -1, -1, 0)

        patch_list(frag.outs, match_state, states)

        *start_state = frag.start
        *num_captures = p.group_count + 1
        return true
    }

    internal func parse_regex(p : &mut Parser,
        states : &mut std::vector<NFAState>,
        classes : &mut std::vector<CharClass>,
        group_count : *mut i64) : Frag {

        return parse_alternation(p, states, classes, group_count)
    }

    internal func parse_alternation(p : &mut Parser,
        states : &mut std::vector<NFAState>,
        classes : &mut std::vector<CharClass>,
        group_count : *mut i64) : Frag {

        var frag = parse_concatenation(p, states, classes, group_count)

        while(peek_char(p) == '|' as i64) {
            advance_char(p)
            var rhs = parse_concatenation(p, states, classes, group_count)

            var split = new_state(states, 1, 0, 0, frag.start, rhs.start, 0)

            var new_outs = copy_patch_list(frag.outs)
            append_patch_list(new_outs, rhs.outs)

            frag.start = split
            frag.outs.clear()
            append_patch_list(frag.outs, new_outs)
        }

        return frag
    }

    internal func parse_concatenation(p : &mut Parser,
        states : &mut std::vector<NFAState>,
        classes : &mut std::vector<CharClass>,
        group_count : *mut i64) : Frag {

        var frag = parse_repetition(p, states, classes, group_count)

        loop {
            var c = peek_char(p)
            if(c < 0 || c == ')' as i64 || c == '|' as i64) {
                break
            }
            var rhs = parse_repetition(p, states, classes, group_count)

            patch_list(frag.outs, rhs.start, states)

            frag.outs.clear()
            append_patch_list(frag.outs, rhs.outs)
        }

        return frag
    }

    internal func parse_repetition(p : &mut Parser,
        states : &mut std::vector<NFAState>,
        classes : &mut std::vector<CharClass>,
        group_count : *mut i64) : Frag {

        var frag = parse_atom(p, states, classes, group_count)

        var c = peek_char(p)
        if(c == '*' as i64) {
            advance_char(p)
            var split = new_state(states, 1, 0, 0, frag.start, -1, 0)
            var split_outs = std::vector<PatchPtr>()
            split_outs.push(make_patch(split, 1))

            var jmp = new_state(states, 2, 0, 0, split, -1, 0)
            patch_list(frag.outs, jmp, states)

            frag.start = split
            frag.outs.clear()
            append_patch_list(frag.outs, split_outs)
        } else if(c == '+' as i64) {
            advance_char(p)
            var split = new_state(states, 1, 0, 0, frag.start, -1, 0)
            var split_outs = std::vector<PatchPtr>()
            split_outs.push(make_patch(split, 1))

            patch_list(frag.outs, split, states)

            frag.outs.clear()
            append_patch_list(frag.outs, split_outs)
        } else if(c == '?' as i64) {
            advance_char(p)
            var split = new_state(states, 1, 0, 0, frag.start, -1, 0)
            var split_outs = std::vector<PatchPtr>()
            split_outs.push(make_patch(split, 1))

            append_patch_list(frag.outs, split_outs)

            frag.start = split
        }

        return frag
    }

    internal func parse_atom(p : &mut Parser,
        states : &mut std::vector<NFAState>,
        classes : &mut std::vector<CharClass>,
        group_count : *mut i64) : Frag {

        var c = peek_char(p)
        if(c < 0) {
            p.error.append_view("unexpected end of pattern")
            return make_empty_frag()
        }

        if(c == '(' as i64) {
            advance_char(p)
            var group_idx = *group_count
            *group_count = *group_count + 1

            var save_slot = (group_idx + 1) * 2
            var save_start = new_state(states, 8, 0, 0, -1, -1, save_slot)

            var inner = parse_alternation(p, states, classes, group_count)

            if(!expect_char(p, ')' as i64)) {
                return make_empty_frag()
            }

            var save_end = new_state(states, 8, 0, 0, -1, -1, save_slot + 1)

            states.get_ptr(save_start as size_t).out1 = inner.start
            patch_list(inner.outs, save_end, states)

            var outs = std::vector<PatchPtr>()
            outs.push(make_patch(save_end, 0))

            return Frag { start : save_start, outs : outs }
        }

        if(c == ')' as i64) {
            p.error.append_view("unmatched ')'")
            return make_empty_frag()
        }

        if(c == '[' as i64) {
            advance_char(p)
            var class_idx = parse_char_class(p, classes)
            if(class_idx < 0) return make_empty_frag()
            var st = new_state(states, 5, 0, class_idx, -1, -1, 0)
            var outs = std::vector<PatchPtr>()
            outs.push(make_patch(st, 0))
            return Frag { start : st, outs : outs }
        }

        if(c == '.' as i64) {
            advance_char(p)
            var st = new_state(states, 4, 0, 0, -1, -1, 0)
            var outs = std::vector<PatchPtr>()
            outs.push(make_patch(st, 0))
            return Frag { start : st, outs : outs }
        }

        if(c == '^' as i64) {
            advance_char(p)
            var st = new_state(states, 6, 0, 0, -1, -1, 0)
            var outs = std::vector<PatchPtr>()
            outs.push(make_patch(st, 0))
            return Frag { start : st, outs : outs }
        }

        if(c == '$' as i64) {
            advance_char(p)
            var st = new_state(states, 7, 0, 0, -1, -1, 0)
            var outs = std::vector<PatchPtr>()
            outs.push(make_patch(st, 0))
            return Frag { start : st, outs : outs }
        }

        if(c == '\\' as i64) {
            advance_char(p)
            var esc = parse_escape_char(p)
            if(esc == -1) return make_empty_frag()

            if(is_escape_class(esc)) {
                var class_idx = make_escape_class(esc, classes)
                var st = new_state(states, 5, 0, class_idx, -1, -1, 0)
                var outs = std::vector<PatchPtr>()
                outs.push(make_patch(st, 0))
                return Frag { start : st, outs : outs }
            }

            var st = new_state(states, 0, esc, 0, -1, -1, 0)
            var outs = std::vector<PatchPtr>()
            outs.push(make_patch(st, 0))
            return Frag { start : st, outs : outs }
        }

        advance_char(p)
        var st = new_state(states, 0, c, 0, -1, -1, 0)
        var outs = std::vector<PatchPtr>()
        outs.push(make_patch(st, 0))
        return Frag { start : st, outs : outs }
    }

    internal func make_empty_frag() : Frag {
        return Frag { start : -1, outs : std::vector<PatchPtr>() }
    }

    // =========================================================================
    // NFA Execution
    // =========================================================================
    internal func add_state(set : *mut std::vector<i64>, sid : i64,
        states : &std::vector<NFAState>) {
        if(sid < 0) return

        var i : size_t = 0
        unsafe {
            while(i < set.size()) {
                if(set.get(i) == sid) return
                i = i + 1
            }

            set.push(sid)
        }

        var s = states.get(sid as size_t)
        if(s.kind == 1) {
            add_state(set, s.out1, states)
            add_state(set, s.out2, states)
        } else if(s.kind == 2) {
            add_state(set, s.out1, states)
        } else if(s.kind == 8) {
            add_state(set, s.out1, states)
        }
    }

    internal func match_class_char(cd : *CharClass, ch : i64) : bool {
        var i : size_t = 0
        while(i < cd.ranges.size()) {
            var r = cd.ranges.get(i)
            if(ch >= r.lo && ch <= r.hi) {
                return !cd.negated
            }
            i = i + 1
        }
        return cd.negated
    }

    internal func run_prog(states : &std::vector<NFAState>,
        classes : &std::vector<CharClass>,
        start : i64, text : std::string_view,
        text_start : size_t, text_end : size_t) : bool {

        var v1 = std::vector<i64>()
        var v2 = std::vector<i64>()
        var current = &mut v1
        var next = &mut v2

        add_state(current, start, states)

        var pos = text_start
        while(pos < text_end) {
            if(has_match_state(current, states)) return true

            expand_non_consuming(current, states, pos, text.size())

            var ch = text.get(pos) as i64

            unsafe { next.clear() }

            var j : size_t = 0
            var cur_size : size_t = 0
            unsafe { cur_size = current.size() }
            while(j < cur_size) {
                var sid : i64 = 0
                unsafe { sid = current.get(j) }
                var s = states.get(sid as size_t)

                if(s.kind == 0) {
                    if(s.ch == ch) {
                        add_state(next, s.out1, states)
                    }
                } else if(s.kind == 4) {
                    if(ch != 10) {
                        add_state(next, s.out1, states)
                    }
                } else if(s.kind == 5) {
                    var cd = classes.get_ptr(s.class_idx as size_t)
                    if(match_class_char(cd, ch)) {
                        add_state(next, s.out1, states)
                    }
                }
                j = j + 1
                unsafe { cur_size = current.size() }
            }

            var tmp = current
            current = next
            next = tmp

            pos = pos + 1
        }

        expand_non_consuming(current, states, pos, text.size())
        if(has_match_state(current, states)) return true
        return false
    }


    internal func has_match_state(set : *std::vector<i64>,
        states : &std::vector<NFAState>) : bool {
        var k : size_t = 0
        var set_size : size_t = 0
        unsafe { set_size = set.size() }
        while(k < set_size) {
            var sid : i64 = 0
            unsafe { sid = set.get(k) }
            if(states.get(sid as size_t).kind == 3) return true
            k = k + 1
        }
        return false
    }

    internal func expand_non_consuming(set : *mut std::vector<i64>,
        states : &std::vector<NFAState>,
        pos : size_t, text_len : size_t) {
        var i : size_t = 0
        var set_size : size_t = 0
        unsafe { set_size = set.size() }
        while(i < set_size) {
            var sid : i64 = 0
            unsafe { sid = set.get(i) }
            var s = states.get(sid as size_t)
            if(s.kind == 6) {
                if(pos == 0) {
                    add_state(set, s.out1, states)
                }
            } else if(s.kind == 7) {
                if(pos == text_len) {
                    add_state(set, s.out1, states)
                }
            } else if(s.kind == 8) {
                add_state(set, s.out1, states)
            }
            i = i + 1
            unsafe { set_size = set.size() }
        }
    }

    internal func copy_i64_vec(src : &std::vector<i64>) : std::vector<i64> {
        var dst = std::vector<i64>()
        var i : size_t = 0
        while(i < src.size()) {
            dst.push(src.get(i))
            i = i + 1
        }
        return dst
    }

    internal func copy_visited_vec(src : &std::vector<VisitedFrame>) : std::vector<VisitedFrame> {
        var dst = std::vector<VisitedFrame>()
        var i : size_t = 0
        while(i < src.size()) {
            dst.push(src.get(i))
            i = i + 1
        }
        return dst
    }

    internal func make_capture_positions(num_captures : i64) : std::vector<i64> {
        var positions = std::vector<i64>()
        var total = num_captures * 2
        var i : i64 = 0
        while(i < total) {
            positions.push(-1)
            i = i + 1
        }
        return positions
    }

    internal func visited_contains(path : &std::vector<VisitedFrame>, sid : i64, pos : size_t) : bool {
        var i : size_t = 0
        while(i < path.size()) {
            var item = path.get(i)
            if(item.state_idx == sid && item.pos == pos) return true
            i = i + 1
        }
        return false
    }

    internal func run_prog_with_captures_impl(states : &std::vector<NFAState>,
        classes : &std::vector<CharClass>,
        sid : i64, text : std::string_view,
        pos : size_t, text_end : size_t,
        positions : &mut std::vector<i64>,
        path : &mut std::vector<VisitedFrame>) : bool {

        if(sid < 0) return false
        if(visited_contains(path, sid, pos)) return false

        path.push(VisitedFrame { state_idx : sid, pos : pos })

        var result = false
        var state = states.get(sid as size_t)

        if(state.kind == 3) {
            result = pos == text_end
        } else if(state.kind == 0) {
            if(pos < text_end) {
                var ch = text.get(pos) as i64
                if(state.ch == ch) {
                    result = run_prog_with_captures_impl(states, classes, state.out1, text, pos + 1, text_end, positions, path)
                }
            }
        } else if(state.kind == 1) {
            var left_positions = copy_i64_vec(positions)
            var left_path = copy_visited_vec(path)
            if(run_prog_with_captures_impl(states, classes, state.out1, text, pos, text_end, left_positions, left_path)) {
                positions.clear()
                append_i64_vec(positions, left_positions)
                result = true
            } else {
                var right_positions = copy_i64_vec(positions)
                var right_path = copy_visited_vec(path)
                if(run_prog_with_captures_impl(states, classes, state.out2, text, pos, text_end, right_positions, right_path)) {
                    positions.clear()
                    append_i64_vec(positions, right_positions)
                    result = true
                }
            }
        } else if(state.kind == 2) {
            result = run_prog_with_captures_impl(states, classes, state.out1, text, pos, text_end, positions, path)
        } else if(state.kind == 4) {
            if(pos < text_end) {
                var ch = text.get(pos) as i64
                if(ch != 10) {
                    result = run_prog_with_captures_impl(states, classes, state.out1, text, pos + 1, text_end, positions, path)
                }
            }
        } else if(state.kind == 5) {
            if(pos < text_end) {
                var ch = text.get(pos) as i64
                var cd = classes.get_ptr(state.class_idx as size_t)
                if(match_class_char(cd, ch)) {
                    result = run_prog_with_captures_impl(states, classes, state.out1, text, pos + 1, text_end, positions, path)
                }
            }
        } else if(state.kind == 6) {
            if(pos == 0) {
                result = run_prog_with_captures_impl(states, classes, state.out1, text, pos, text_end, positions, path)
            }
        } else if(state.kind == 7) {
            if(pos == text.size()) {
                result = run_prog_with_captures_impl(states, classes, state.out1, text, pos, text_end, positions, path)
            }
        } else if(state.kind == 8) {
            if(state.save_idx >= 0 && (state.save_idx as size_t) < positions.size()) {
                *positions.get_ptr(state.save_idx as size_t) = pos as i64
            }
            result = run_prog_with_captures_impl(states, classes, state.out1, text, pos, text_end, positions, path)
        }

        path.remove_last()
        return result
    }

    internal func append_i64_vec(dst : &mut std::vector<i64>, src : &std::vector<i64>) {
        var i : size_t = 0
        while(i < src.size()) {
            dst.push(src.get(i))
            i = i + 1
        }
    }

    internal func replace_i64_vec(dst : *mut std::vector<i64>, src : &std::vector<i64>) {
        unsafe {
            dst.clear()
            var i : size_t = 0
            while(i < src.size()) {
                dst.push(src.get(i))
                i = i + 1
            }
        }
    }

    internal func run_prog_with_captures(states : &std::vector<NFAState>,
        classes : &std::vector<CharClass>,
        start : i64, text : std::string_view,
        text_start : size_t, text_end : size_t,
        num_captures : i64,
        positions_out : *mut std::vector<i64>) : bool {

        var positions = make_capture_positions(num_captures)
        if(positions.size() >= 2) {
            *positions.get_ptr(0) = text_start as i64
            *positions.get_ptr(1) = text_end as i64
        }
        var path = std::vector<VisitedFrame>()
        var matched = run_prog_with_captures_impl(states, classes, start, text, text_start, text_end, positions, path)
        if(matched) {
            replace_i64_vec(positions_out, positions)
            return true
        }
        return false
    }

    internal func run_prog_exact(states : &std::vector<NFAState>,
        classes : &std::vector<CharClass>,
        start : i64, text : std::string_view,
        text_start : size_t, text_end : size_t) : bool {

        var positions = std::vector<i64>()
        var path = std::vector<VisitedFrame>()
        return run_prog_with_captures_impl(states, classes, start, text, text_start, text_end, positions, path)
    }

    internal func find_captures(states : &std::vector<NFAState>,
        classes : &std::vector<CharClass>,
        start : i64, text : std::string_view,
        text_start : size_t, num_captures : i64,
        positions_out : *mut std::vector<i64>) : bool {

        var found = false
        var best_positions = std::vector<i64>()

        var end_pos = text_start
        while(end_pos <= text.size()) {
            var positions = std::vector<i64>()
            if(run_prog_with_captures(states, classes, start, text, text_start, end_pos, num_captures, &mut positions)) {
                found = true
                best_positions.clear()
                append_i64_vec(best_positions, positions)
            }
            end_pos = end_pos + 1
        }

        if(found) {
            replace_i64_vec(positions_out, best_positions)
            return true
        }
        return false
    }

    internal func find_match(states : &std::vector<NFAState>,
        classes : &std::vector<CharClass>,
        start : i64, text : std::string_view,
        text_start : size_t, match_end : *mut i64) : bool {

        var found = false
        var best : i64 = -1

        var end_pos = text_start
        while(end_pos <= text.size()) {
            if(run_prog_exact(states, classes, start, text, text_start, end_pos)) {
                found = true
                best = end_pos as i64
            }
            end_pos = end_pos + 1
        }

        if(found) {
            *match_end = best
            return true
        }
        return false
    }

    internal func find_from(states : &std::vector<NFAState>,
        classes : &std::vector<CharClass>,
        start_state : i64, text : std::string_view,
        search_start : size_t,
        start_out : *mut i64, end_out : *mut i64) : bool {

        var text_len = text.size()
        var i = search_start
        while(i <= text_len) {
            var match_end : i64 = 0
            if(find_match(states, classes, start_state, text, i, &mut match_end)) {
                *start_out = i as i64
                *end_out = match_end
                return true
            }
            i = i + 1
        }
        return false
    }

}
