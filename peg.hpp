class PEG {
	void skip_character(const char*& g) {
		if (*g == '(') {
			++g;
			skip_choice(g);
			++g;
		}
		else {
			if (*g == '\\') {
				++g;
			}
			++g;
		}
	}
	void skip_repetition(const char*& g) {
		skip_character(g);
		if (*g == '*') {
			++g;
		}
	}
	void skip_sequence(const char*& g) {
		while (*g != '\0' && *g != ')' && *g != '|') {
			skip_repetition(g);
		}
	}
	void skip_choice(const char*& g) {
		while (true) {
			skip_sequence(g);
			if (*g != '|') {
				break;
			}
			++g;
		}
	}
	bool parse_character(const char*& g, const char*& s) {
		if (*g == '(') {
			++g;
			const bool result = parse_choice(g, s);
			++g;
			return result;
		}
		else {
			if (*g == '\\') {
				++g;
			}
			if (*g == *s) {
				++g;
				++s;
				return true;
			}
			++g;
			return false;
		}
	}
	bool parse_repetition(const char*& g, const char*& s) {
		const char* g_start = g;
		const char* s_start = s;
		bool result = parse_character(g, s);
		if (*g == '*') {
			while (result) {
				g = g_start;
				s_start = s;
				result = parse_character(g, s);
			}
			++g;
			s = s_start;
			return true;
		}
		return result;
	}
	bool parse_sequence(const char*& g, const char*& s) {
		while (*g != '\0' && *g != ')' && *g != '|') {
			if (!parse_repetition(g, s)) {
				skip_sequence(g);
				return false;
			}
		}
		return true;
	}
	bool parse_choice(const char*& g, const char*& s) {
		const char* start = s;
		while (true) {
			if (parse_sequence(g, s)) {
				if (*g == '|') ++g;
				skip_choice(g);
				return true;
			}
			if (*g != '|') {
				return false;
			}
			++g;
			s = start;
		}
	}
public:
	void skip(const char*& g) {
		skip_choice(g);
	}
	bool parse(const char*& g, const char*& s) {
		return parse_choice(g, s);
	}
};

#ifdef TEST
#include <cstdio>
void test_peg() {
	auto assert_peg = [](const char* g, const char* s, bool expected_result = true) {
		auto assert = [=](bool condition) {
			if (!condition) {
				printf("error:\n");
				printf("  grammar: \"%s\"\n", g);
				printf("  string: \"%s\"\n", s);
				printf("  expected result: %s\n", expected_result ? "true" : "false");
			}
		};
		const bool result = PEG().parse(g, s);
		assert(result == expected_result);
		assert(*g == '\0');
		if (result) assert(*s == '\0');
	};
	assert_peg("", "");
	assert_peg("a", "a");
	assert_peg("a", "b", false);
	assert_peg("\\(", "(");
	assert_peg("(a)", "a");
	assert_peg("a*", "");
	assert_peg("a*", "aa");
	assert_peg("abc", "abc");
	assert_peg("abc", "adc", false);
	assert_peg("a|b|c", "b");
	assert_peg("a|b|c", "d", false);

	assert_peg("(abc)*", "abcabc");
	assert_peg("(a|b|c)*", "bb");
}
#endif
