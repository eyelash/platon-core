#pragma once

#include "json.hpp"
#include <utility>
#include <vector>

class Color {
	static constexpr Color hue(float h) {
		return
		    h <= 1.f ? Color(1.f, h-0.f, 0.f)
		  : h <= 2.f ? Color(2.f-h, 1.f, 0.f)
		  : h <= 3.f ? Color(0.f, 1.f, h-2.f)
		  : h <= 4.f ? Color(0.f, 4.f-h, 1.f)
		  : h <= 5.f ? Color(h-4.f, 0.f, 1.f)
		  : Color(1.f, 0.f, 6.f-h);
	}
	constexpr Color saturation(float s) const {
		return Color(red * s + 1.f - s, green * s + 1.f - s, blue * s + 1.f - s, alpha);
	}
	constexpr Color value(float v) const {
		return Color(red * v, green * v, blue * v, alpha);
	}
public:
	float red;
	float green;
	float blue;
	float alpha;
	constexpr Color(float red, float green, float blue, float alpha = 1.f): red(red), green(green), blue(blue), alpha(alpha) {}
	static constexpr Color hsv(float h, float s, float v) {
		return hue(h / 60.f).saturation(s / 100.f).value(v / 100.f);
	}
	void write(JSONWriter& writer) const {
		writer.write_array([&](JSONArrayWriter& writer) {
			writer.write_element().write_number(red * 255.f + .5f);
			writer.write_element().write_number(green * 255.f + .5f);
			writer.write_element().write_number(blue * 255.f + .5f);
			writer.write_element().write_number(alpha * 255.f + .5f);
		});
	}
};

enum class Weight: int {
	NORMAL = 400,
	BOLD = 700
};

class Style {
public:
	Color color;
	Weight weight;
	bool italic;
	constexpr Style(const Color& color, Weight weight = Weight::NORMAL, bool italic = false): color(color), weight(weight), italic(italic) {}
	void write(JSONWriter& writer) const {
		writer.write_object([&](JSONObjectWriter& writer) {
			color.write(writer.write_member("color"));
			writer.write_member("weight").write_number(static_cast<int>(weight));
			writer.write_member("italic").write_boolean(italic);
		});
	}
};

struct Theme {
	Color background;
	Color selection;
	Color cursor;
	Style number;
	Style number_active;
	Style styles[5];
	void write(JSONWriter& writer) const {
		writer.write_object([&](JSONObjectWriter& writer) {
			background.write(writer.write_member("background"));
			selection.write(writer.write_member("selection"));
			cursor.write(writer.write_member("cursor"));
			number.write(writer.write_member("number"));
			number_active.write(writer.write_member("number_active"));
			writer.write_member("styles").write_array([&](JSONArrayWriter& writer) {
				for (const Style& style: styles) {
					style.write(writer.write_element());
				}
			});
		});
	}
};

constexpr Theme default_theme = {
	Color::hsv(0, 0, 100), // background
	Color::hsv(60, 40, 100), // selection
	Color::hsv(0, 0, 20), // cursor
	Style(Color::hsv(0, 0, 60)), // number
	Style(Color::hsv(0, 0, 20)), // number_active
	{
		Style(Color::hsv(0, 0, 20)), // text
		Style(Color::hsv(0, 0, 60), Weight::NORMAL, true), // comments
		Style(Color::hsv(270, 80, 80), Weight::BOLD), // keywords
		Style(Color::hsv(210, 80, 80), Weight::BOLD), // types
		Style(Color::hsv(150, 80, 80)) // literals
	}
};

class Span {
public:
	std::size_t first;
	std::size_t last;
	int theme_index;
	Span(std::size_t first, std::size_t last, int theme_index): first(first), last(last), theme_index(theme_index) {}
};

class Spans {
	std::vector<Span> spans;
public:
	void add_span(std::size_t first, std::size_t last, int theme_index) {
		spans.emplace_back(first, last, theme_index);
	}
	std::size_t save() const {
		return spans.size();
	}
	void restore(std::size_t savepoint) {
		spans.erase(spans.begin() + savepoint, spans.end());
	}
	std::size_t size() const {
		return spans.size();
	}
	const Span& operator [](std::size_t i) const {
		return spans[i];
	}
	auto begin() const {
		return spans.begin();
	}
	auto end() const {
		return spans.end();
	}
	void clear() {
		spans.clear();
	}
};

class Char {
	char c;
public:
	constexpr Char(char c): c(c) {}
	template <class I> bool match(I& i, Spans& spans) const {
		if (*i == c) {
			++i;
			return true;
		}
		return false;
	}
};

class Range {
	char first;
	char last;
public:
	constexpr Range(char first, char last): first(first), last(last) {}
	template <class I> bool match(I& i, Spans& spans) const {
		if (*i != '\0' && *i >= first && *i <= last) {
			++i;
			return true;
		}
		return false;
	}
};

class AnyChar {
public:
	template <class I> bool match(I& i, Spans& spans) const {
		if (*i == '\0') {
			return false;
		}
		++i;
		return true;
	}
};

class String {
	const char* string;
public:
	constexpr String(const char* string): string(string) {}
	template <class I> bool match(I& i, Spans& spans) const {
		I copy = i;
		for (const char* s = string; *s != '\0'; ++s) {
			if (*s != *i) {
				i = copy;
				return false;
			}
			++i;
		}
		return true;
	}
	bool operator <(const String& s) const {
		const char* lhs = string;
		const char* rhs = s.string;
		while (*lhs == *rhs && *lhs != '\0') {
			++lhs;
			++rhs;
		}
		return *lhs < *rhs;
	}
	bool operator ==(const String& s) const {
		const char* lhs = string;
		const char* rhs = s.string;
		while (*lhs == *rhs && *lhs != '\0') {
			++lhs;
			++rhs;
		}
		return *lhs == *rhs;
	}
};

template <class... T> class Sequence;
template <> class Sequence<> {
public:
	template <class I> bool match(I& i, Spans& spans) const {
		return true;
	}
};
template <class T0, class... T> class Sequence<T0, T...> {
	T0 t0;
	Sequence<T...> sequence;
public:
	constexpr Sequence(const T0& t0, const T&... t): t0(t0), sequence(t...) {}
	template <class I> bool match(I& i, Spans& spans) const {
		I copy = i;
		std::size_t savepoint = spans.save();
		if (!t0.match(i, spans)) {
			return false;
		}
		if (!sequence.match(i, spans)) {
			i = copy;
			spans.restore(savepoint);
			return false;
		}
		return true;
	}
};
template <class... T> Sequence(const T&...) -> Sequence<T...>;

template <class... T> class Choice;
template <> class Choice<> {
public:
	template <class I> bool match(I& i, Spans& spans) const {
		return false;
	}
};
template <class T0, class... T> class Choice<T0, T...> {
	T0 t0;
	Choice<T...> choice;
public:
	constexpr Choice(const T0& t0, const T&... t): t0(t0), choice(t...) {}
	template <class I> bool match(I& i, Spans& spans) const {
		if (t0.match(i, spans)) {
			return true;
		}
		return choice.match(i, spans);
	}
};
template <class... T> Choice(const T&...) -> Choice<T...>;

template <class T> class Repetition {
	T t;
public:
	constexpr Repetition(const T& t): t(t) {}
	template <class I> bool match(I& i, Spans& spans) const {
		while (t.match(i, spans));
		return true;
	}
};

template <class T> class Optional {
	T t;
public:
	constexpr Optional(const T& t): t(t) {}
	template <class I> bool match(I& i, Spans& spans) const {
		t.match(i, spans);
		return true;
	}
};

template <class T> class Not {
	T t;
public:
	constexpr Not(const T& t): t(t) {}
	template <class I> bool match(I& i, Spans& spans) const {
		I start = i;
		std::size_t savepoint = spans.save();
		if (t.match(i, spans)) {
			i = start;
			spans.restore(savepoint);
			return false;
		}
		return true;
	}
};

template <class T> class But {
	T t;
public:
	constexpr But(const T& t): t(t) {}
	template <class I> bool match(I& i, Spans& spans) const {
		I start = i;
		std::size_t savepoint = spans.save();
		if (t.match(i, spans)) {
			i = start;
			spans.restore(savepoint);
			return false;
		}
		return AnyChar().match(i, spans);
	}
};

template <class T> class Highlight {
	T t;
	int theme_index;
public:
	constexpr Highlight(const T& t, int theme_index): t(t), theme_index(theme_index) {}
	template <class I> bool match(I& i, Spans& spans) const {
		I start = i;
		if (t.match(i, spans)) {
			spans.add_span(start.get_index(), i.get_index(), theme_index);
			return true;
		}
		return false;
	}
};

template <class T, class... A> auto keywords(const T& t, A&&... args) {
	return Sequence(Choice(String(std::forward<A>(args))...), Not(t));
}

constexpr auto c_comment = Choice(
	Sequence(String("//"), Repetition(But(Char('\n')))), // single line
	Sequence(String("/*"), Repetition(But(String("*/"))), String("*/")) // multi-line
);
constexpr auto c_preproc = Sequence(Char('#'), Repetition(But(Char('\n'))));
constexpr auto hex_prefix = Sequence(Char('0'), Choice(Char('x'), Char('X')));
constexpr auto digit = Range('0', '9');
constexpr auto hex_digit = Choice(Range('0', '9'), Range('a', 'f'), Range('A', 'F'));
constexpr auto exponent = Sequence(Choice(Char('e'), Char('E')), Optional(Char('-')), digit, Repetition(digit));
constexpr auto hex_exponent = Sequence(Choice(Char('p'), Char('P')), Optional(Char('-')), digit, Repetition(digit));
constexpr auto integer_suffix = Choice(
	Sequence(Choice(Char('l'), Char('L'), String("ll"), String("LL")), Optional(Choice(Char('u'), Char('U')))), // l before u
	Sequence(Choice(Char('u'), Char('U')), Optional(Choice(Char('l'), Char('L'), String("ll"), String("LL")))) // u before l
);
constexpr auto float_suffix = Choice(Char('f'), Char('F'), Char('l'), Char('L'));
constexpr auto c_number = Sequence(
	Choice(
		Sequence(digit, Repetition(digit), Optional(Char('.')), Repetition(digit)),
		Sequence(Char('.'), digit, Repetition(digit))
	),
	Choice(
		integer_suffix,
		Sequence(
			Optional(exponent),
			Optional(float_suffix)
		)
	)
);
constexpr auto c_number_hex = Sequence(
	Char('0'), Choice(Char('x'), Char('X')), // prefix
	Choice(
		Sequence(hex_digit, Repetition(hex_digit), Optional(Char('.')), Repetition(hex_digit)),
		Sequence(Char('.'), hex_digit, Repetition(hex_digit))
	),
	Choice(
		integer_suffix,
		Sequence(
			Optional(hex_exponent),
			Optional(float_suffix)
		)
	)
);
constexpr auto c_string = Sequence(
	Optional(Char('L')), // prefix
	Char('"'), // start character
	Repetition(Choice(String("\\\""), But(Char('"')))),
	Char('"') // end character
);
constexpr auto c_character = Sequence(
	Optional(Char('L')), // prefix
	Char('\''), // start character
	Repetition(Choice(String("\\'"), But(Char('\'')))),
	Char('\'') // end character
);
constexpr auto c_ident_start = Choice(Range('a', 'z'), Range('A', 'Z'), Char('_'));
constexpr auto c_ident_char = Choice(c_ident_start, Range('0', '9'));
constexpr auto c_ident = Sequence(c_ident_start, Repetition(c_ident_char));
constexpr auto c_whitespace = Choice(Char(' '), Char('\t'));

auto syntax = Choice(
	Highlight(c_comment, 1),
	Highlight(Choice(c_string, c_character, c_number_hex, c_number), 4),
	Highlight(keywords(c_ident_char, "if", "else", "for", "while", "do", "break", "continue", "switch", "case", "default", "return", "goto", "typedef", "struct", "enum", "union"), 2), // keywords
	Highlight(keywords(c_ident_char, "const", "static", "extern"), 2), // keywords
	Highlight(keywords(c_ident_char, "void", "char", "int", "unsigned", "long", "float", "double"), 3), // types
	c_ident
);

class Language {
	Spans spans;
public:
	template <class E> Language(const E& buffer) {
		update(buffer);
	}
	template <class E> void update(const E& buffer) {
		spans.clear();
		auto iterator = buffer.begin();
		while (*iterator != '\0') {
			if (!syntax.match(iterator, spans)) {
				++iterator;
			}
		}
	}
	void highlight(JSONWriter& writer, std::size_t index0, std::size_t index1) const {
		writer.write_array([&](JSONArrayWriter& writer) {
			for (auto& span: spans) {
				if (span.last > index0 && span.first < index1) {
					writer.write_element().write_array([&](JSONArrayWriter& writer) {
						writer.write_element().write_number(std::max(span.first, index0) - index0);
						writer.write_element().write_number(std::min(span.last, index1) - index0);
						writer.write_element().write_number(span.theme_index);
					});
				}
			}
		});
	}
};
