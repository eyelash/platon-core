constexpr auto cplusplus_syntax = repetition(choice(
	// comments
	highlight(Style::COMMENT, c_comment),
	// strings and characters
	highlight(Style::LITERAL, c_string_or_character),
	// numbers
	highlight(Style::LITERAL, c_number),
	// literals
	highlight(Style::LITERAL, c_keywords(
		"nullptr",
		"false",
		"true"
	)),
	// keywords
	highlight(Style::KEYWORD, c_keywords(
		"this",
		"auto",
		"constexpr",
		"consteval",
		"if",
		"else",
		"for",
		"while",
		"do",
		"switch",
		"case",
		"default",
		"goto",
		"break",
		"continue",
		"try",
		"catch",
		"throw",
		"return",
		"new",
		"delete",
		"class",
		"struct",
		"enum",
		"union",
		"final",
		"public",
		"protected",
		"private",
		"static",
		"virtual",
		"override",
		"noexcept",
		"explicit",
		"friend",
		"mutable",
		"operator",
		"template",
		"typename",
		"namespace",
		"using",
		"typedef",
		"module",
		"import",
		"export"
	)),
	// types
	highlight(Style::TYPE, c_keywords(
		"void",
		"bool",
		"char",
		"short",
		"int",
		"long",
		"float",
		"double",
		"unsigned",
		"signed",
		"const",
		"volatile"
	)),
	// identifiers
	sequence(c_identifier_begin_char, repetition(c_identifier_char)),
	any_char()
));
