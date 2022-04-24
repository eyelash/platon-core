constexpr auto javascript_number = sequence(
	choice(
		// hex
		sequence(
			'0',
			choice('x', 'X'),
			one_or_more(hex_digit)
		),
		// binary
		sequence(
			'0',
			choice('b', 'B'),
			one_or_more(range('0', '1'))
		),
		// octal
		sequence(
			'0',
			choice('o', 'O'),
			one_or_more(range('0', '7'))
		),
		// decimal
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
	optional('n')
);

constexpr auto javascript_syntax = repetition(choice(
	// comments
	highlight(Style::COMMENT, c_comment),
	// numbers
	highlight(Style::LITERAL, javascript_number),
	// literals
	highlight(Style::LITERAL, java_keywords(
		"null",
		"false",
		"true"
	)),
	// keywords
	highlight(Style::KEYWORD, java_keywords(
		"function",
		"this",
		"var",
		"let",
		"const",
		"if",
		"else",
		"for",
		"in",
		"of",
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
		"extends",
		"static",
		"import",
		"export"
	)),
	// identifiers
	sequence(java_identifier_begin_char, repetition(java_identifier_char)),
	any_char()
));
