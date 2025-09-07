func (parser : &mut Parser) consume(type : TokenType) : bool {
    if(parser.getToken().type == type) {
        parser.increment();
        return true;
    } else {
        return false;
    }
}