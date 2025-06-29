public enum SemanticTokenTypes {
	Namespace,
	/*
	 * Represents a generic type. Acts as a fallback for types which can't be mapped to
	 * a specific type like class or enum.
	 */
	Type,
	Class,
	Enum,
	Interface,
	Struct,
	TypeParameter,
	Parameter,
	Variable,
	Property,
	EnumMember,
	Event,
	Function,
	Method,
	Macro,
	Keyword,
	Modifier,
	Comment,
	String,
	Number,
	Regexp,
	Operator,
	/*
	 * @since 3.17.0
	 */
	Decorator,
	MAX_VALUE
};