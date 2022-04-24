constexpr auto haskell_identifier_char = choice(range('a', 'z'), '_', range('A', 'Z'), range('0', '9'), '\'');

class HaskellBlockComment {
public:
	constexpr HaskellBlockComment() {}
	template <class I> std::unique_ptr<SourceNode> match(I& i, const I& end) const {
		return sequence(
			"{-",
			repetition(choice(
				*this,
				but("-}")
			)),
			optional("-}")
		).match(i, end);
	}
};
constexpr HaskellBlockComment haskell_block_comment() {
	return HaskellBlockComment();
}

constexpr auto haskell_syntax = repetition(choice(
	// comments
	highlight(Style::COMMENT, choice(
		haskell_block_comment(),
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
