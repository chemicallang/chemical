func (parser : &mut Parser) consume(type : TokenType) : bool {
        unsafe {
        if(parser.getToken().type == type) {
            parser.increment();
            return true;
        } else {
            return false;
        }

        }
    }