func (cssParser : &mut CSSParser) parseAtRule(om : &mut CSSOM, parser : *mut Parser, builder : *mut ASTBuilder) : bool {

    printf("css parsing an at rule\n");
    fflush(null)

    const token = parser.getToken();
    if(token.type == TokenType.At) {
        parser.increment();
    } else {
        return false;
    }

    const next = parser.getToken()
    if(next.type == TokenType.Identifier) {
        switch(fnv1_hash(next.value.data())) {
            comptime_fnv1_hash("global") => {
                om.global = cssParser.parseGlobalBlock(parser, builder)
                return true;
            }
            default => {
                parser.error("expected an at rule identifier like 'global' or 'media'");
            }
        }
    } else {
        parser.error("expected an at rule identifier like 'global' or 'media'");
    }

    return false;

}