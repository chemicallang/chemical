import "./LexTokenType.ch"

public struct Position {
    var line : uint
    var character : uint
}

@compiler:interface
public struct CSTToken {

    func get_value(&self, into : string*);

    func type(&self) : LexTokenType;

    func position(&self) : Position*;

    func tokens(&self) : VecRef<CSTToken>*;

    func accept(&self, converter : CSTConverter*);

}

func (token : CSTToken*) value() : string {
    const ret = compiler::return_struct() as string*;
    token.get_value(ret);
}