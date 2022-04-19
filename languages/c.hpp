constexpr auto c_identifier_begin_char = choice(range('a', 'z'), range('A', 'Z'), '_');
constexpr auto c_identifier_char = choice(range('a', 'z'), range('A', 'Z'), '_', range('0', '9'));
template <class T> constexpr auto c_keyword(T t) {
	return sequence(t, not_(c_identifier_char));
}
template <class... T> constexpr auto c_keywords(T... arguments) {
	return choice(c_keyword(arguments)...);
}

constexpr auto c_comment = choice(
	sequence("/*", repetition(but("*/")), optional("*/")),
	sequence("//", repetition(but('\n')))
);
constexpr auto c_escape = sequence('\\', any_char());
constexpr auto c_string_or_character = sequence(
	optional(choice('L', "u8", 'u', 'U')),
	choice(sequence(
		'"',
		repetition(choice(c_escape, but('"'))),
		optional('"')
	), sequence(
		'\'',
		repetition(choice(c_escape, but('\''))),
		optional('\'')
	))
);
constexpr auto c_number = sequence(
	choice(
		// hex
		sequence(
			'0',
			choice('x', 'X'),
			zero_or_more(hex_digit),
			optional('.'),
			zero_or_more(hex_digit),
			// exponent
			optional(sequence(
				choice('p', 'P'),
				optional(choice('+', '-')),
				one_or_more(range('0', '9'))
			))
		),
		// decimal or octal
		sequence(
			choice(
				sequence(one_or_more(range('0', '9')), optional('.'), zero_or_more(range('0', '9'))),
				sequence('.', one_or_more(range('0', '9')))
			),
			// exponent
			optional(sequence(
				choice('e', 'E'),
				optional(choice('+', '-')),
				one_or_more(range('0', '9'))
			))
		)
	),
	// suffix
	zero_or_more(choice('u', 'U', 'l', 'L', 'f', 'F'))
);

constexpr auto c_syntax = repetition(choice(
	// comments
	highlight(1, c_comment),
	// strings and characters
	highlight(4, c_string_or_character),
	// numbers
	highlight(4, c_number),
	// keywords
	highlight(2, c_keywords(
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
		"return",
		"struct",
		"enum",
		"union",
		"typedef",
		"sizeof",
		"static",
		"extern",
		"inline"
	)),
	// types
	highlight(3, c_keywords(
		"void",
		"char",
		"short",
		"int",
		"long",
		"float",
		"double",
		"unsigned",
		"signed",
		"const"
	)),
	// identifiers
	sequence(c_identifier_begin_char, zero_or_more(c_identifier_char)),
	any_char()
));
