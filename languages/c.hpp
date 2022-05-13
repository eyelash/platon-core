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
constexpr auto c_digits = sequence(
	range('0', '9'),
	repetition(sequence(optional('\''), range('0', '9')))
);
constexpr auto c_hex_digits = sequence(
	hex_digit,
	repetition(sequence(optional('\''), hex_digit))
);
constexpr auto c_binary_digits = sequence(
	range('0', '1'),
	repetition(sequence(optional('\''), range('0', '1')))
);
constexpr auto c_number = sequence(
	choice(
		// hex
		sequence(
			'0',
			choice('x', 'X'),
			optional(c_hex_digits),
			optional('.'),
			optional(c_hex_digits),
			// exponent
			optional(sequence(
				choice('p', 'P'),
				optional(choice('+', '-')),
				c_digits
			))
		),
		// binary
		sequence(
			'0',
			choice('b', 'B'),
			c_binary_digits
		),
		// decimal or octal
		sequence(
			choice(
				sequence(c_digits, optional('.'), optional(c_digits)),
				sequence('.', c_digits)
			),
			// exponent
			optional(sequence(
				choice('e', 'E'),
				optional(choice('+', '-')),
				c_digits
			))
		)
	),
	// suffix
	zero_or_more(choice('u', 'U', 'l', 'L', 'f', 'F'))
);

constexpr auto c_syntax = repetition(choice(
	// comments
	highlight(Style::COMMENT, c_comment),
	// strings and characters
	highlight(Style::LITERAL, c_string_or_character),
	// numbers
	highlight(Style::WORD, highlight(Style::LITERAL, c_number)),
	// keywords
	highlight(Style::WORD, highlight(Style::KEYWORD, c_keywords(
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
	))),
	// types
	highlight(Style::WORD, highlight(Style::TYPE, c_keywords(
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
	))),
	// identifiers
	highlight(Style::WORD, sequence(c_identifier_begin_char, zero_or_more(c_identifier_char))),
	any_char()
));
