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
	highlight(Style::WORD, highlight(Style::KEYWORD, c_keywords(
		"lambda",
		"and",
		"or",
		"not",
		"if",
		"elif",
		"else",
		"for",
		"in",
		"while",
		"break",
		"continue",
		"return",
		"def",
		"class"
	))),
	// identifiers
	highlight(Style::WORD, sequence(c_identifier_begin_char, zero_or_more(c_identifier_char))),
	any_char()
));
