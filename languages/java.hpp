constexpr auto java_identifier_begin_char = choice(range('a', 'z'), range('A', 'Z'), '$', '_');
constexpr auto java_identifier_char = choice(range('a', 'z'), range('A', 'Z'), '$', '_', range('0', '9'));
template <class T> constexpr auto java_keyword(T t) {
	return sequence(t, not_(java_identifier_char));
}
template <class... T> constexpr auto java_keywords(T... arguments) {
	return choice(java_keyword(arguments)...);
}

constexpr auto java_digits = sequence(
	range('0', '9'),
	repetition(sequence(zero_or_more('_'), range('0', '9')))
);
constexpr auto java_hex_digits = sequence(
	hex_digit,
	repetition(sequence(zero_or_more('_'), hex_digit))
);
constexpr auto java_binary_digits = sequence(
	range('0', '1'),
	repetition(sequence(zero_or_more('_'), range('0', '1')))
);

constexpr auto java_number = sequence(
	choice(
		// hex
		sequence(
			'0',
			choice('x', 'X'),
			optional(java_hex_digits),
			optional('.'),
			optional(java_hex_digits),
			// exponent
			optional(sequence(
				choice('p', 'P'),
				optional(choice('+', '-')),
				java_digits
			))
		),
		// binary
		sequence(
			'0',
			choice('b', 'B'),
			java_binary_digits
		),
		// decimal or octal
		sequence(
			choice(
				sequence(java_digits, optional('.'), optional(java_digits)),
				sequence('.', java_digits)
			),
			// exponent
			optional(sequence(
				choice('e', 'E'),
				optional(choice('+', '-')),
				java_digits
			))
		)
	),
	// suffix
	optional(choice('l', 'L', 'f', 'F', 'd', 'D'))
);

constexpr auto java_syntax = repetition(choice(
	// comments
	highlight(Style::COMMENT, c_comment),
	// numbers
	highlight(Style::LITERAL, java_number),
	// literals
	highlight(Style::LITERAL, java_keywords(
		"null",
		"false",
		"true"
	)),
	// keywords
	highlight(Style::KEYWORD, java_keywords(
		"this",
		"var",
		"if",
		"else",
		"for",
		"while",
		"do",
		"switch",
		"case",
		"default",
		"break",
		"continue",
		"try",
		"catch",
		"finally",
		"throw",
		"return",
		"new",
		"class",
		"interface",
		"enum",
		"extends",
		"implements",
		"abstract",
		"final",
		"public",
		"protected",
		"private",
		"static",
		"throws",
		"import",
		"package"
	)),
	// types
	highlight(Style::TYPE, java_keywords(
		"void",
		"boolean",
		"char",
		"byte",
		"short",
		"int",
		"long",
		"float",
		"double",
		"String"
	)),
	// identifiers
	sequence(java_identifier_begin_char, repetition(java_identifier_char)),
	any_char()
));
