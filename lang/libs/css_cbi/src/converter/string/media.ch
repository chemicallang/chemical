// Media query serialization — all logic is in css_parser/src/converter/convert.ch
// The CBI only needs thin wrappers that pass `converter` as the emitter.

func (converter : &mut ASTConverter) writeKeyframesRule(rule : *mut CSSKeyframesRule, str : &mut std::string) {
    css_write_keyframes_rule(rule, str, converter.as_emitter())
}

func (converter : &mut ASTConverter) writeMediaRule(rule : *mut CSSMediaRule, str : &mut std::string, className : std::string_view) {
    css_write_media_rule(rule, str, className, converter.as_emitter())
}
