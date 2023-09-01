constexpr auto haskell_identifier_char = choice(range('a', 'z'), '_', range('A', 'Z'), range('0', '9'), '\'');

constexpr auto haskell_block_comment = recursive([](auto haskell_block_comment) {
	return sequence(
		"{-",
		repetition(choice(
			haskell_block_comment,
			but("-}")
		)),
		optional("-}")
	);
});

constexpr auto haskell_syntax = repetition(choice(
	// comments
	highlight(Style::COMMENT, choice(
		haskell_block_comment,
		sequence("--", repetition(but('\n')))
	)),
	// keywords
	highlight(Style::KEYWORD, c_keywords(
		"if",
		"then",
		"else",
		"let",
		"in",
		"where",
		"case",
		"of",
		"do",
		"type",
		"newtype",
		"data",
		"class",
		"instance",
		"module",
		"import"
	)),
	// types
	highlight(Style::TYPE, sequence(range('A', 'Z'), repetition(haskell_identifier_char))),
	// identifiers
	sequence(choice(range('a', 'z'), '_'), repetition(haskell_identifier_char)),
	any_char()
));
