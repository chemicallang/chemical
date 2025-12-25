
func isTagNameSelfClosing(view : &std::string_view) : bool {
    switch(fnv1_hash_view(view)) {
        comptime_fnv1_hash("area"), comptime_fnv1_hash("base"), comptime_fnv1_hash("br"), comptime_fnv1_hash("col"),
        comptime_fnv1_hash("embed"), comptime_fnv1_hash("hr"), comptime_fnv1_hash("img"), comptime_fnv1_hash("input"),
        comptime_fnv1_hash("link"), comptime_fnv1_hash("meta"), comptime_fnv1_hash("source"), comptime_fnv1_hash("track"),
        comptime_fnv1_hash("wbr") => {
            return true;
        }
        default => {
            return false;
        }
    }
}

func (htmlParser : &mut HtmlParser) parseElement(parser : *mut Parser, builder : *mut ASTBuilder) : *mut HtmlElement {

    const lt = parser.getToken();

    if(lt.type == TokenType.LessThan) {

        parser.increment();
        const id = parser.getToken();
        if(id.type != TokenType.TagName) {
            parser.error("expected an identifier after '<'");
            return null
        }
        parser.increment();

        var isSelfClosing = isTagNameSelfClosing(id.value);

        var element : *mut HtmlElement = builder.allocate<HtmlElement>();
        new (element) HtmlElement {
            HtmlChild : HtmlChild {
                kind : HtmlChildKind.Element
            },
            name : builder.allocate_view(id.value),
            isSelfClosing : isSelfClosing,
            attributes : std::vector<*HtmlAttribute>(),
            children : std::vector<*HtmlChild>()
        }

        while(true) {
            var attr = htmlParser.parseAttribute(parser, builder);
            if(attr != null) {
                element.attributes.push(attr)
            } else {
                break;
            }
        }

        // optional forward slash in self closing tags <br />
        const fs = parser.getToken();
        if(fs.type == TokenType.FwdSlash) {
            parser.increment();
            isSelfClosing = true;
        }

        const gt = parser.getToken();
        if(gt.type != TokenType.GreaterThan) {
            parser.error("expected a greater than sign '>' after the identifier");
        } else {
            parser.increment();
        }

        if(isSelfClosing) {
            return element;
        }

        while(true) {
            var child = htmlParser.parseElementChild(parser, builder);
            if(child != null) {
                element.children.push(child)
            } else {
                break;
            }
        }

        const last_lt = parser.getToken();
        if(last_lt.type == TokenType.TagEnd) {
            parser.increment()
            const last_id = parser.getToken();
            if(last_id.type == TokenType.TagName) {
                parser.increment();
                if(strncmp(last_id.value.data(), id.value.data(), id.value.size()) != 0) {
                    parser.error("expected correct identifier for ending tag");
                }
                const last_gt = parser.getToken();
                if(last_gt.type == TokenType.GreaterThan) {
                    parser.increment();
                } else {
                    parser.error("expected '>' after the ending tag");
                }
            } else {
                printf("expected id, got: %d, %s\n", last_id.type, last_id.value.data());
                fflush(null)
                parser.error("expected identifier after the '</'");
            }
        } else {
            parser.error("expected a '</' for ending the element");
        }

        return element;

    } else {
        return null;
    }
}
