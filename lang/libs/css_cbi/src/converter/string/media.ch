// Media query serialization — all logic is in css_parser/src/converter/convert.ch
// The CBI only needs thin wrappers that pass `converter` as the emitter.

func (converter : &mut ASTConverter) writeKeyframesRule(rule : *mut CSSKeyframesRule, str : &mut std::string) {
    css_write_keyframes_rule(rule, str, converter.as_emitter())
}

// `className` is the root class the media rules are scoped under (empty for a
// global `#css` block). `decl_root` is the selector used for declarations
// written directly inside the media query when the block has no root class —
// `:root` for a global block — since such declarations have no selector of
// their own.
func (converter : &mut ASTConverter) writeMediaRule(rule : *mut CSSMediaRule, str : &mut std::string, className : std::string_view, decl_root : std::string_view = "") {
    css_write_media_rule(rule, str, className, converter.as_emitter(), decl_root)
}
