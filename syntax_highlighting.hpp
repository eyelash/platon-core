#pragma once

#include "json.hpp"
#include <utility>
#include <vector>
#include <algorithm>
#include <memory>

class Color {
	static constexpr float hue_function(float h) {
		return h <= 60.f ? (h / 60.f) : h <= 180.f ? 1.f : h <= 240.f ? (4.f - h / 60.f) : 0.f;
	}
	static constexpr Color hue(float h) {
		return Color(
			hue_function(h < 240.f ? h + 120.f : h - 240.f),
			hue_function(h),
			hue_function(h < 120.f ? h + 240.f : h - 120.f)
		);
	}
public:
	float r;
	float g;
	float b;
	float a;
	constexpr Color(float r, float g, float b, float a = 1.f): r(r), g(g), b(b), a(a) {}
	constexpr Color operator +(const Color& c) const {
		return Color(
			(r * a * (1.f - c.a) + c.r * c.a) / (a * (1.f - c.a) + c.a),
			(g * a * (1.f - c.a) + c.g * c.a) / (a * (1.f - c.a) + c.a),
			(b * a * (1.f - c.a) + c.b * c.a) / (a * (1.f - c.a) + c.a),
			a * (1.f - c.a) + c.a
		);
	}
	static constexpr Color hsv(float h, float s, float v) {
		return hue(h) + Color(1.f, 1.f, 1.f, 1.f - s / 100.f) + Color(0.f, 0.f, 0.f, 1.f - v / 100.f);
	}
	static constexpr Color hsl(float h, float s, float l) {
		return hue(h) + Color(.5f, .5f, .5f, 1.f - s / 100.f) + (l < 50.f ? Color(0.f, 0.f, 0.f, 1.f - l / 50.f) : Color(1.f, 1.f, 1.f, l / 50.f - 1.f));
	}
	constexpr Color with_alpha(float a) const {
		return Color(r, g, b, this->a * a);
	}
	void write(JSONWriter& writer) const {
		writer.write_array([&](JSONArrayWriter& writer) {
			writer.write_element().write_number(r * 255.f + .5f);
			writer.write_element().write_number(g * 255.f + .5f);
			writer.write_element().write_number(b * 255.f + .5f);
			writer.write_element().write_number(a * 255.f + .5f);
		});
	}
};

enum class Weight: int {
	NORMAL = 400,
	BOLD = 700
};

class Style {
public:
	static constexpr int BOLD = 1 << 0;
	static constexpr int ITALIC = 1 << 1;
	Color color;
	Weight weight;
	bool italic;
	constexpr Style(const Color& color, Weight weight = Weight::NORMAL, bool italic = false): color(color), weight(weight), italic(italic) {}
	constexpr Style(const Color& color, int attributes): color(color), weight(attributes & BOLD ? Weight::BOLD : Weight::NORMAL), italic(attributes & ITALIC) {}
	void write(JSONWriter& writer) const {
		writer.write_object([&](JSONObjectWriter& writer) {
			color.write(writer.write_member("color"));
			writer.write_member("weight").write_number(static_cast<int>(weight));
			writer.write_member("bold").write_boolean(weight == Weight::BOLD);
			writer.write_member("italic").write_boolean(italic);
		});
	}
	static constexpr int INHERIT = 0;
	static constexpr int WORD = 1;
	static constexpr int DEFAULT = 2;
	static constexpr int COMMENT = 3;
	static constexpr int KEYWORD = 4;
	static constexpr int OPERATOR = 5;
	static constexpr int TYPE = 6;
	static constexpr int LITERAL = 7;
	static constexpr int STRING = 8;
	static constexpr int FUNCTION = 9;
};

struct Theme {
	Color background;
	Color background_active;
	Color selection;
	Color cursor;
	Color number_background;
	Color number_background_active;
	Style number;
	Style number_active;
	Style styles[8];
	void write(JSONWriter& writer) const {
		writer.write_object([&](JSONObjectWriter& writer) {
			background.write(writer.write_member("background"));
			background_active.write(writer.write_member("background_active"));
			selection.write(writer.write_member("selection"));
			cursor.write(writer.write_member("cursor"));
			number_background.write(writer.write_member("number_background"));
			number_background_active.write(writer.write_member("number_background_active"));
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

#include "themes/one_dark.hpp"
#include "themes/monokai.hpp"

class Span {
public:
	std::size_t first;
	std::size_t last;
	int style;
	constexpr Span(std::size_t first, std::size_t last, int style): first(first), last(last), style(style) {}
};

class StringParseContext {
	const char* s;
public:
	StringParseContext(const char* s): s(s) {}
	using SavePoint = const char*;
	operator bool() const {
		return *s != '\0';
	}
	char operator *() const {
		return *s;
	}
	StringParseContext& operator ++() {
		++s;
		return *this;
	}
	SavePoint save() const {
		return s;
	}
	SavePoint save_with_style() {
		return s;
	}
	void set_style(SavePoint save_point, int style) {}
	void restore(SavePoint save_point) {
		s = save_point;
	}
};

template <class I> class HighlightParseContext {
	I i;
	I end;
	std::size_t pos;
	std::vector<Span> spans;
public:
	HighlightParseContext(I i, I end): i(i), end(end), pos(0) {}
	using SavePoint = std::tuple<I, std::size_t, std::size_t>;
	operator bool() const {
		return i != end;
	}
	char operator *() const {
		return *i;
	}
	HighlightParseContext& operator ++() {
		++i;
		++pos;
		return *this;
	}
	SavePoint save() const {
		return std::make_tuple(i, pos, spans.size());
	}
	SavePoint save_with_style() {
		const std::size_t index = spans.size();
		spans.emplace_back(pos, pos, 0);
		return std::make_tuple(i, pos, index);
	}
	void set_style(const SavePoint& save_point, int style) {
		Span& span = spans[std::get<2>(save_point)];
		span.last = pos;
		span.style = style;
	}
	void restore(const SavePoint& save_point) {
		i = std::get<0>(save_point);
		pos = std::get<1>(save_point);
		spans.erase(spans.begin() + std::get<2>(save_point), spans.end());
	}
	const std::vector<Span>& get_spans() const {
		return spans;
	}
};

template <class F> class Char {
	F f;
public:
	constexpr Char(F f): f(f) {}
	template <class C> bool match(C& c) const {
		if (c && f(*c)) {
			++c;
			return true;
		}
		return false;
	}
};

class String {
	const char* string;
	std::size_t length;
public:
	constexpr String(const char* string): string(string), length(std::char_traits<char>::length(string)) {}
	template <class C> bool match(C& c) const {
		auto save_point = c.save();
		for (const char* s = string; *s != '\0'; ++s) {
			if (c && *c == *s) {
				++c;
			}
			else {
				c.restore(save_point);
				return false;
			}
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
};

template <class T0, class T1> class Sequence {
	T0 t0;
	T1 t1;
public:
	constexpr Sequence(T0 t0, T1 t1): t0(t0), t1(t1) {}
	template <class C> bool match(C& c) const {
		auto save_point = c.save();
		if (t0.match(c)) {
			if (t1.match(c)) {
				return true;
			}
			else {
				c.restore(save_point);
				return false;
			}
		}
		else {
			return false;
		}
	}
};

template <class T0, class T1> class Choice {
	T0 t0;
	T1 t1;
public:
	constexpr Choice(T0 t0, T1 t1): t0(t0), t1(t1) {}
	template <class C> bool match(C& c) const {
		if (t0.match(c)) {
			return true;
		}
		else {
			return t1.match(c);
		}
	}
};

template <class T> class Repetition {
	T t;
public:
	constexpr Repetition(T t): t(t) {}
	template <class C> bool match(C& c) const {
		while (t.match(c)) {}
		return true;
	}
};

template <class T> class Optional {
	T t;
public:
	constexpr Optional(T t): t(t) {}
	template <class C> bool match(C& c) const {
		if (t.match(c)) {
			return true;
		}
		else {
			return true;
		}
	}
};

template <class T> class Not {
	T t;
public:
	constexpr Not(T t): t(t) {}
	template <class C> bool match(C& c) const {
		auto save_point = c.save();
		if (t.match(c)) {
			c.restore(save_point);
			return false;
		}
		else {
			return true;
		}
	}
};

template <class T> class Highlight {
	T t;
	int style;
public:
	constexpr Highlight(T t, int style): t(t), style(style) {}
	template <class C> bool match(C& c) const {
		auto save_point = c.save_with_style();
		if (t.match(c)) {
			c.set_style(save_point, style);
			return true;
		}
		else {
			c.restore(save_point);
			return false;
		}
	}
};

template <class F> class Recursive {
	F f;
public:
	constexpr Recursive(F f): f(f) {}
	template <class C> bool match(C& c) const {
		return f(*this).match(c);
	}
};

constexpr auto get_language_node(char c) {
	return Char([c](char i) {
		return i == c;
	});
}
constexpr String get_language_node(const char* s) {
	return String(s);
}
template <class T> constexpr T get_language_node(T language_node) {
	return language_node;
}

constexpr auto range(char first, char last) {
	return Char([first, last](char c) {
		return c >= first && c <= last;
	});
}
constexpr auto any_char() {
	return Char([](char c) {
		return true;
	});
}
template <class T0, class T1> constexpr auto sequence(T0 t0, T1 t1) {
	return Sequence(get_language_node(t0), get_language_node(t1));
}
template <class T0, class T1, class T2, class... T> constexpr auto sequence(T0 t0, T1 t1, T2 t2, T... t) {
	return sequence(sequence(t0, t1), t2, t...);
}
template <class T0, class T1> constexpr auto choice(T0 t0, T1 t1) {
	return Choice(get_language_node(t0), get_language_node(t1));
}
template <class T0, class T1, class T2, class... T> constexpr auto choice(T0 t0, T1 t1, T2 t2, T... t) {
	return choice(choice(t0, t1), t2, t...);
}
template <class T> constexpr auto repetition(T child) {
	return Repetition(get_language_node(child));
}
template <class T> constexpr auto optional(T child) {
	return Optional(get_language_node(child));
}
template <class T> constexpr auto not_(T child) {
	return Not(get_language_node(child));
}
template <class T> constexpr auto highlight(int style, T child) {
	return Highlight(get_language_node(child), style);
}
template <class T> constexpr auto zero_or_more(T child) {
	return repetition(child);
}
template <class T> constexpr auto one_or_more(T child) {
	return sequence(child, repetition(child));
}
template <class T> constexpr auto but(T child) {
	return sequence(not_(child), any_char());
}
constexpr auto end() {
	return not_(any_char());
}
template <class F> constexpr auto recursive(F f) {
	return Recursive(f);
}

constexpr auto hex_digit = choice(range('0', '9'), range('a', 'f'), range('A', 'F'));

#include "languages/c.hpp"
#include "languages/cplusplus.hpp"
#include "languages/java.hpp"
#include "languages/xml.hpp"
#include "languages/javascript.hpp"
#include "languages/python.hpp"
#include "languages/rust.hpp"
#include "languages/haskell.hpp"

template <class E> class LanguageInterface {
public:
	virtual ~LanguageInterface() = default;
	virtual void invalidate(std::size_t index) = 0;
	virtual void highlight(const E& buffer, JSONWriter& writer, std::size_t index0, std::size_t index1) = 0;
	virtual void get_word(const E& buffer, std::size_t index, std::size_t& word_start, std::size_t& word_end) = 0;
	virtual void get_next_word(const E& buffer, std::size_t index, std::size_t& word_start, std::size_t& word_end) = 0;
	virtual void get_previous_word(const E& buffer, std::size_t index, std::size_t& word_start, std::size_t& word_end) = 0;
};

template <class E> class NoLanguage: public LanguageInterface<E> {
public:
	void invalidate(std::size_t index) override {}
	void highlight(const E& buffer, JSONWriter& writer, std::size_t index0, std::size_t index1) override {
		writer.write_array([](JSONArrayWriter& writer) {});
	}
	void get_word(const E& buffer, std::size_t index, std::size_t& word_start, std::size_t& word_end) override {
		word_start = word_end = index;
	}
	void get_next_word(const E& buffer, std::size_t index, std::size_t& word_start, std::size_t& word_end) override {
		word_start = word_end = index;
	}
	void get_previous_word(const E& buffer, std::size_t index, std::size_t& word_start, std::size_t& word_end) override {
		word_start = word_end = index;
	}
};

template <class T> class Reverse {
	T& t;
public:
	Reverse(T& t): t(t) {}
	auto begin() {
		return t.rbegin();
	}
	auto end() {
		return t.rend();
	}
};
template <class T> Reverse<T> reverse(T& t) {
	return Reverse<T>(t);
}

template <class E, class T> class LanguageImplementation: public LanguageInterface<E> {
	T syntax;
	std::vector<Span> highlights;
	std::vector<Span> words;
	static void change_style(std::vector<Span>& spans, std::size_t pos, int style, int previous_style) {
		if (style == previous_style) {
			return;
		}
		// TODO: handle empty spans
		if (previous_style != Style::DEFAULT) {
			spans.back().last = pos;
		}
		if (style != Style::DEFAULT) {
			spans.emplace_back(pos, 0, style);
		}
	}
	template <class F> static void flatten(const std::vector<Span>& spans, F f, std::size_t& i, std::vector<Span>& result, int outer_style = Style::DEFAULT) {
		const Span& span = spans[i];
		++i;
		const int style = f(span.style) ? span.style : outer_style;
		change_style(result, span.first, style, outer_style);
		while (i < spans.size() && spans[i].last <= span.last) {
			flatten(spans, f, i, result, style);
		}
		change_style(result, span.last, outer_style, style);
	}
	template <class F> static std::vector<Span> flatten(const std::vector<Span>& spans, F f) {
		std::vector<Span> result;
		std::size_t i = 0;
		while (i < spans.size()) {
			flatten(spans, f, i, result);
		}
		return result;
	}
	static std::vector<Span> flatten_highlights(const std::vector<Span>& spans) {
		return flatten(spans, [](int style) { return style >= Style::DEFAULT; });
	}
	static std::vector<Span> flatten_words(const std::vector<Span>& spans) {
		return flatten(spans, [](int style) { return style == Style::WORD; });
	}
	void parse_if_necessary(const E& buffer) {
		if (highlights.empty() && words.empty()) {
			HighlightParseContext context(buffer.begin(), buffer.end());
			syntax.match(context);
			highlights = flatten_highlights(context.get_spans());
			words = flatten_words(context.get_spans());
		}
	}
public:
	LanguageImplementation(T syntax): syntax(syntax) {}
	void invalidate(std::size_t index) override {
		highlights.clear();
		words.clear();
	}
	void highlight(const E& buffer, JSONWriter& writer, std::size_t index0, std::size_t index1) override {
		if (buffer.get_size() > 10000) {
			// disable syntax highlighting for large files until we have incremental syntax highlighting
			writer.write_array([](JSONArrayWriter& writer) {});
			return;
		}
		parse_if_necessary(buffer);
		auto i = std::upper_bound(highlights.begin(), highlights.end(), index0, [](std::size_t index0, const Span& span) {
			return index0 < span.last;
		});
		writer.write_array([&](JSONArrayWriter& writer) {
			for (; i != highlights.end() && i->first < index1; ++i) {
				writer.write_element().write_array([&](JSONArrayWriter& writer) {
					writer.write_element().write_number(std::max(i->first, index0) - index0);
					writer.write_element().write_number(std::min(i->last, index1) - index0);
					writer.write_element().write_number(i->style - Style::DEFAULT);
				});
			}
		});
	}
	void get_word(const E& buffer, std::size_t index, std::size_t& word_start, std::size_t& word_end) override {
		if (buffer.get_size() > 10000) {
			word_start = word_end = index;
			return;
		}
		parse_if_necessary(buffer);
		auto i = std::lower_bound(words.begin(), words.end(), index, [](const Span& span, std::size_t index) {
			return span.last < index;
		});
		if (i != words.end() && i->first <= index) {
			word_start = i->first;
			word_end = i->last;
		}
		else {
			word_start = word_end = index;
		}
	}
	void get_next_word(const E& buffer, std::size_t index, std::size_t& word_start, std::size_t& word_end) override {
		if (buffer.get_size() > 10000) {
			word_start = word_end = index;
			return;
		}
		parse_if_necessary(buffer);
		auto i = std::upper_bound(words.begin(), words.end(), index, [](std::size_t index, const Span& span) {
			return index < span.last;
		});
		if (i != words.end()) {
			word_start = i->first;
			word_end = i->last;
		}
		else {
			word_start = word_end = buffer.get_size() - 1;
		}
	}
	void get_previous_word(const E& buffer, std::size_t index, std::size_t& word_start, std::size_t& word_end) override {
		if (buffer.get_size() > 10000) {
			word_start = word_end = index;
			return;
		}
		parse_if_necessary(buffer);
		auto i = std::upper_bound(words.rbegin(), words.rend(), index, [](std::size_t index, const Span& span) {
			return index > span.first;
		});
		if (i != words.rend()) {
			word_start = i->first;
			word_end = i->last;
		}
		else {
			word_start = word_end = 0;
		}
	}
};

template <class T> bool match_string(const T& t, const char* string) {
	StringParseContext context(string);
	return t.match(context);
}

template <class T> constexpr auto ends_with(T t) {
	const auto e = sequence(t, end());
	return sequence(repetition(but(e)), e);
}

template <class T0, class T1> class Language {
	T0 file_name;
	T1 syntax;
public:
	constexpr Language(T0 file_name, T1 syntax): file_name(file_name), syntax(syntax) {}
	bool check_file_name(const char* file_name) const {
		return match_string(this->file_name, file_name);
	}
	template <class E> std::unique_ptr<LanguageInterface<E>> instantiate() const {
		return std::make_unique<LanguageImplementation<E, T1>>(syntax);
	}
};

template <class... T> class Languages;
template <> class Languages<> {
public:
	constexpr Languages() {}
	template <class E> std::unique_ptr<LanguageInterface<E>> get_language(const char* file_name) const {
		return std::make_unique<NoLanguage<E>>();
	}
};
template <class T0, class... T> class Languages<T0, T...> {
	T0 t0;
	Languages<T...> languages;
public:
	constexpr Languages(T0 t0, T... t): t0(t0), languages(t...) {}
	template <class E> std::unique_ptr<LanguageInterface<E>> get_language(const char* file_name) const {
		if (t0.check_file_name(file_name)) {
			return t0.template instantiate<E>();
		}
		return languages.template get_language<E>(file_name);
	}
};
template <class... T> Languages(T...) -> Languages<T...>;

template <class T0, class T1> constexpr auto language(T0 file_name, T1 syntax) {
	return Language(file_name, syntax);
}
template <class... T> constexpr auto languages(T... t) {
	return Languages(t...);
}

template <class E> std::unique_ptr<LanguageInterface<E>> get_language(const E& buffer, const char* file_name) {
	return languages(
		language(ends_with(".c"), c_syntax),
		language(ends_with(choice(".cpp", ".cc", ".hpp", ".hh", ".h")), cplusplus_syntax),
		language(ends_with(".java"), java_syntax),
		language(ends_with(choice(".xml", ".svg")), xml_syntax),
		language(ends_with(".js"), javascript_syntax),
		language(ends_with(".py"), python_syntax),
		language(ends_with(".rs"), rust_syntax),
		language(ends_with(".hs"), haskell_syntax)
	).get_language<E>(file_name);
}
