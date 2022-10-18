constexpr auto python_comment = sequence('#', repetition(but('\n')));

constexpr auto python_syntax = repetition(choice(
	// comments
	highlight(Style::COMMENT, python_comment),
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
	any_char()
));
