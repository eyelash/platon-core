constexpr auto python_comment = sequence('#', repetition(but('\n')));

constexpr auto python_syntax = repetition(choice(
	// comments
	highlight(Style::COMMENT, python_comment),
	// literals
	highlight(Style::WORD, highlight(Style::LITERAL, c_keywords(
		"None",
		"False",
		"True"
	))),
	// keywords
	sequence(
		highlight(Style::KEYWORD, c_keyword("def")),
		zero_or_more(' '),
		optional(highlight(Style::FUNCTION, c_identifier))
	),
	sequence(
		highlight(Style::KEYWORD, c_keyword("class")),
		zero_or_more(' '),
		optional(highlight(Style::TYPE, c_identifier))
	),
	highlight(Style::WORD, highlight(Style::KEYWORD, c_keywords(
		"lambda",
		"if",
		"elif",
		"else",
		"for",
		"in",
		"while",
		"break",
		"continue",
		"return",
		"import"
	))),
	// operators
	highlight(Style::WORD, highlight(Style::OPERATOR, c_keywords(
		"and",
		"or",
		"not",
		"is",
		"in"
	))),
	highlight(Style::WORD, highlight(Style::OPERATOR, choice(
		"**", "//", '+', '-', '*', '/', '%',
		"==", "!=", "<=", ">=", '<', '>'
	))),
	// identifiers
	highlight(Style::WORD, c_identifier),
	any_char()
));
