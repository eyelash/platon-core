class CplusplusRawStringDelimiterStart {
	std::string& delimiter;
public:
	CplusplusRawStringDelimiterStart(std::string& delimiter): delimiter(delimiter) {}
	static constexpr bool is_delimiter_char(char c) {
		return c >= 0x21 && c <= 0x7E && c != '(' && c != '\\';
	}
	template <class I> std::unique_ptr<SourceNode> match(I& i, const I& end) const {
		return repetition(Char([this](char c) {
			if (is_delimiter_char(c)) {
				delimiter.push_back(c);
				return true;
			}
			return false;
		})).match(i, end);
	}
};
class CplusplusRawStringDelimiterEnd {
	std::string& delimiter;
public:
	CplusplusRawStringDelimiterEnd(std::string& delimiter): delimiter(delimiter) {}
	template <class I> std::unique_ptr<SourceNode> match(I& i, const I& end) const {
		return String(delimiter.c_str()).match(i, end);
	}
};
inline CplusplusRawStringDelimiterStart cplusplus_raw_string_delimiter_start(std::string& delimiter) {
	return CplusplusRawStringDelimiterStart(delimiter);
}

class CplusplusRawString {
public:
	constexpr CplusplusRawString() {}
	template <class I> std::unique_ptr<SourceNode> match(I& i, const I& end) const {
		std::string delimiter;
		return sequence(
			optional(choice('L', "u8", 'u', 'U')),
			"R\"",
			CplusplusRawStringDelimiterStart(delimiter),
			'(',
			repetition(but(sequence(')', CplusplusRawStringDelimiterEnd(delimiter), '\"'))),
			optional(sequence(')', CplusplusRawStringDelimiterEnd(delimiter), '\"'))
		).match(i, end);
	}
};
constexpr CplusplusRawString cplusplus_raw_string() {
	return CplusplusRawString();
}

constexpr auto cplusplus_syntax = repetition(choice(
	// comments
	highlight(Style::COMMENT, c_comment),
	// strings and characters
	highlight(Style::WORD, highlight(Style::LITERAL, cplusplus_raw_string())),
	highlight(Style::WORD, highlight(Style::LITERAL, c_string)),
	highlight(Style::WORD, highlight(Style::LITERAL, c_character)),
	// numbers
	highlight(Style::WORD, highlight(Style::LITERAL, c_number)),
	// literals
	highlight(Style::WORD, highlight(Style::LITERAL, c_keywords(
		"nullptr",
		"false",
		"true"
	))),
	// keywords
	highlight(Style::WORD, highlight(Style::KEYWORD, c_keywords(
		"this",
		"auto",
		"constexpr",
		"consteval",
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
		"try",
		"catch",
		"throw",
		"return",
		"new",
		"delete",
		"class",
		"struct",
		"enum",
		"union",
		"final",
		"public",
		"protected",
		"private",
		"static",
		"virtual",
		"override",
		"noexcept",
		"explicit",
		"friend",
		"mutable",
		"operator",
		"template",
		"typename",
		"namespace",
		"using",
		"typedef",
		"module",
		"import",
		"export"
	))),
	// types
	highlight(Style::WORD, highlight(Style::TYPE, c_keywords(
		"void",
		"bool",
		"char",
		"short",
		"int",
		"long",
		"float",
		"double",
		"unsigned",
		"signed",
		"const",
		"volatile"
	))),
	// identifiers
	highlight(Style::WORD, sequence(c_identifier_begin_char, zero_or_more(c_identifier_char))),
	any_char()
));
