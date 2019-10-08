#pragma once

#include "json.hpp"
#include <utility>
#include <vector>
#include <algorithm>
#include <memory>

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
	std::size_t match_start;
	std::size_t match_max;
	int theme_index;
	constexpr Span(std::size_t first, std::size_t last, int theme_index): first(first), last(last), match_start(0), match_max(0), theme_index(theme_index) {}
};

template <class I> class MatchContext {
	I position;
	std::size_t max_index;
	std::vector<Span>& spans;
public:
	constexpr MatchContext(const I& position, std::size_t max_index, std::vector<Span>& spans): position(position), max_index(max_index), spans(spans) {}
	char get_char() const {
		return *position;
	}
	std::size_t get_index() const {
		return position.get_index();
	}
	MatchContext& operator ++() {
		++position;
		if (position.get_index() > max_index) {
			max_index = position.get_index();
		}
		return *this;
	}
	class Savepoint {
	public:
		I position;
		std::size_t spans_size;
		constexpr Savepoint(const I& position, std::size_t spans_size): position(position), spans_size(spans_size) {}
	};
	Savepoint save() const {
		return Savepoint(position, spans.size());
	}
	void restore(const Savepoint& savepoint) {
		position = savepoint.position;
		spans.erase(spans.begin() + savepoint.spans_size, spans.end());
	}
	void add_span(std::size_t first, std::size_t last, int theme_index) {
		spans.emplace_back(first, last, theme_index);
	}
	std::size_t get_max_index() const {
		return max_index;
	}
};

class Char {
	char c;
public:
	constexpr Char(char c): c(c) {}
	template <class I> bool match(I& context) const {
		if (context.get_char() == c) {
			++context;
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
	template <class I> bool match(I& context) const {
		const char c = context.get_char();
		if (c != '\0' && c >= first && c <= last) {
			++context;
			return true;
		}
		return false;
	}
};

class AnyChar {
public:
	template <class I> bool match(I& context) const {
		if (context.get_char() == '\0') {
			return false;
		}
		++context;
		return true;
	}
};

class String {
	const char* string;
public:
	constexpr String(const char* string): string(string) {}
	template <class I> bool match(I& context) const {
		const auto savepoint = context.save();
		for (const char* s = string; *s != '\0'; ++s) {
			if (*s != context.get_char()) {
				context.restore(savepoint);
				return false;
			}
			++context;
		}
		return true;
	}
};

class CaseInsensitiveString {
	const char* string;
	static constexpr char to_lower(char c) {
		if (c >= 'A' && c <= 'Z') {
			return c - 'A' + 'a';
		}
		return c;
	}
public:
	constexpr CaseInsensitiveString(const char* string): string(string) {}
	template <class I> bool match(I& context) const {
		const auto savepoint = context.save();
		for (const char* s = string; *s != '\0'; ++s) {
			if (to_lower(*s) != to_lower(context.get_char())) {
				context.restore(savepoint);
				return false;
			}
			++context;
		}
		return true;
	}
};

template <class... T> class Sequence;
template <> class Sequence<> {
public:
	template <class I> bool match(I& context) const {
		return true;
	}
};
template <class T0, class... T> class Sequence<T0, T...> {
	T0 t0;
	Sequence<T...> sequence;
public:
	constexpr Sequence(const T0& t0, const T&... t): t0(t0), sequence(t...) {}
	template <class I> bool match(I& context) const {
		const auto savepoint = context.save();
		if (!t0.match(context)) {
			return false;
		}
		if (!sequence.match(context)) {
			context.restore(savepoint);
			return false;
		}
		return true;
	}
};
template <class... T> Sequence(const T&...) -> Sequence<T...>;

template <class... T> class Choice;
template <> class Choice<> {
public:
	template <class I> bool match(I& context) const {
		return false;
	}
};
template <class T0, class... T> class Choice<T0, T...> {
	T0 t0;
	Choice<T...> choice;
public:
	constexpr Choice(const T0& t0, const T&... t): t0(t0), choice(t...) {}
	template <class I> bool match(I& context) const {
		if (t0.match(context)) {
			return true;
		}
		return choice.match(context);
	}
};
template <class... T> Choice(const T&...) -> Choice<T...>;

template <class T> class Repetition {
	T t;
public:
	constexpr Repetition(const T& t): t(t) {}
	template <class I> bool match(I& context) const {
		while (t.match(context));
		return true;
	}
};

template <class T> class Optional {
	T t;
public:
	constexpr Optional(const T& t): t(t) {}
	template <class I> bool match(I& context) const {
		t.match(context);
		return true;
	}
};

template <class T> class Not {
	T t;
public:
	constexpr Not(const T& t): t(t) {}
	template <class I> bool match(I& context) const {
		const auto savepoint = context.save();
		if (t.match(context)) {
			context.restore(savepoint);
			return false;
		}
		return true;
	}
};

template <class T> class But {
	T t;
public:
	constexpr But(const T& t): t(t) {}
	template <class I> bool match(I& context) const {
		const auto savepoint = context.save();
		if (t.match(context)) {
			context.restore(savepoint);
			return false;
		}
		return AnyChar().match(context);
	}
};

class End {
public:
	template <class I> bool match(I& context) const {
		if (context.get_char() == '\0') {
			return true;
		}
		return false;
	}
};

template <class T> class Highlight {
	T t;
	int theme_index;
public:
	constexpr Highlight(const T& t, int theme_index): t(t), theme_index(theme_index) {}
	template <class I> bool match(MatchContext<I>& context) const {
		const std::size_t start_index = context.get_index();
		if (t.match(context)) {
			context.add_span(start_index, context.get_index(), theme_index);
			return true;
		}
		return false;
	}
};

template <class T, class... A> constexpr auto keywords(const T& t, A&&... args) {
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

auto c_syntax = Choice(
	Highlight(c_comment, 1),
	Highlight(Choice(c_string, c_character, c_number_hex, c_number), 4),
	Highlight(keywords(c_ident_char, "if", "else", "for", "while", "do", "break", "continue", "switch", "case", "default", "return", "goto", "typedef", "struct", "enum", "union"), 2), // keywords
	Highlight(keywords(c_ident_char, "const", "static", "extern"), 2), // keywords
	Highlight(keywords(c_ident_char, "void", "char", "int", "unsigned", "long", "float", "double"), 3), // types
	c_ident
);

template <class E> class LanguageInterface {
	using I = decltype(std::declval<E>().get_iterator(0));
public:
	virtual ~LanguageInterface() = default;
	virtual void invalidate(std::size_t index) = 0;
	virtual void highlight(const E& buffer, JSONWriter& writer, std::size_t index0, std::size_t index1) = 0;
};

template <class E> class NoLanguage: public LanguageInterface<E> {
public:
	void invalidate(std::size_t index) override {}
	void highlight(const E& buffer, JSONWriter& writer, std::size_t index0, std::size_t index1) override {
		writer.write_array([](JSONArrayWriter& writer) {});
	}
};

template <class E, class T> class LanguageImplementation: public LanguageInterface<E> {
	T syntax;
	std::vector<Span> spans;
public:
	LanguageImplementation(const T& syntax): syntax(syntax) {}
	void invalidate(std::size_t index) override {
		auto iterator = std::lower_bound(spans.begin(), spans.end(), index, [](const Span& span, std::size_t index) {
			return span.match_max < index;
		});
		spans.erase(iterator, spans.end());
	}
	void update(const E& buffer, std::size_t max_index) {
		const std::size_t match_start = spans.empty() ? 0 : spans.back().match_start;
		const std::size_t match_max = spans.empty() ? 0 : spans.back().match_max;
		MatchContext context(buffer.get_iterator(match_start), match_max, spans);
		while (context.get_char() != '\0' && context.get_index() < max_index) {
			const std::size_t previous_size = spans.size();
			if (syntax.match(context)) {
				for (std::size_t i = previous_size; i < spans.size(); ++i) {
					spans[i].match_start = context.get_index();
					spans[i].match_max = context.get_max_index();
				}
			}
			else {
				++context;
			}
		}
	}
	void highlight(const E& buffer, JSONWriter& writer, std::size_t index0, std::size_t index1) override {
		update(buffer, index1);
		writer.write_array([&](JSONArrayWriter& writer) {
			auto i = std::upper_bound(spans.begin(), spans.end(), index0, [](std::size_t index0, const Span& span) {
				return index0 < span.last;
			});
			for (; i != spans.end(); ++i) {
				const Span& span = *i;
				if (span.first >= index1) {
					break;
				}
				writer.write_element().write_array([&](JSONArrayWriter& writer) {
					writer.write_element().write_number(std::max(span.first, index0) - index0);
					writer.write_element().write_number(std::min(span.last, index1) - index0);
					writer.write_element().write_number(span.theme_index);
				});
			}
		});
	}
};

template <class T> bool match_string(const T& t, const char* string) {
	class SimpleContext {
		const char* position;
	public:
		constexpr SimpleContext(const char* position): position(position) {}
		char get_char() const {
			return *position;
		}
		SimpleContext& operator ++() {
			++position;
			return *this;
		}
		const char* save() const {
			return position;
		}
		void restore(const char* savepoint) {
			position = savepoint;
		}
	};
	SimpleContext context(string);
	return t.match(context);
}

constexpr auto file_ending(const char* ending) {
	const auto e = Sequence(Char('.'), CaseInsensitiveString(ending), End());
	return Sequence(Repetition(But(e)), e);
}

template <class E> std::unique_ptr<LanguageInterface<E>> get_language(const E& buffer, const char* file_name) {
	if (match_string(file_ending("c"), file_name)) {
		return std::make_unique<LanguageImplementation<E, decltype(c_syntax)>>(c_syntax);
	}
	return std::make_unique<NoLanguage<E>>();
}
