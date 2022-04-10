constexpr auto c_identifier_begin_character = choice(range('a', 'z'), range('A', 'Z'), '_');
constexpr auto c_identifier_character = choice(range('a', 'z'), range('A', 'Z'), '_', range('0', '9'));
constexpr auto c_hex_digit = choice(range('0', '9'), range('a', 'f'), range('A', 'F'));
constexpr auto c_escape = sequence('\\', any_char());

constexpr auto c_keyword(const char* s) -> decltype(sequence(s, not_(c_identifier_character))) {
	return sequence(s, not_(c_identifier_character));
}
template <class... T> constexpr auto c_keywords(T... arguments) -> decltype(choice(c_keyword(arguments)...)) {
	return choice(c_keyword(arguments)...);
}

constexpr auto c_syntax = repetition(choice(
	// comments
	highlight(1, choice(
		sequence("/*", repetition(but("*/")), optional("*/")),
		sequence("//", repetition(but('\n')))
	)),
	// strings
	highlight(4, sequence('"', repetition(choice(c_escape, but('"'))), optional('"'))),
	highlight(4, sequence('\'', choice(c_escape, but('\'')), '\'')),
	// numbers
	highlight(4, sequence(
		choice(
			// hex
			sequence(
				'0',
				choice('x', 'X'),
				zero_or_more(c_hex_digit),
				optional('.'),
				zero_or_more(c_hex_digit),
				// exponent
				optional(sequence(
					choice('p', 'P'),
					optional(choice('+', '-')),
					one_or_more(range('0', '9'))
				))
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
		zero_or_more(choice('u', 'U', 'l', 'L', 'f', 'F'))
	)),
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
		"sizeof"
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
	sequence(c_identifier_begin_character, repetition(c_identifier_character)),
	any_char()
));
