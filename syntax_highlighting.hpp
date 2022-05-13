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
	static constexpr int INHERIT = 0;
	static constexpr int WORD = 1;
	static constexpr int DEFAULT = 2;
	static constexpr int COMMENT = 3;
	static constexpr int KEYWORD = 4;
	static constexpr int TYPE = 5;
	static constexpr int LITERAL = 6;
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
	Style styles[5];
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

#include "themes/default.hpp"
#include "themes/one_dark.hpp"

class Span {
public:
	std::size_t first;
	std::size_t last;
	int style;
	constexpr Span(std::size_t first, std::size_t last, int style): first(first), last(last), style(style) {}
};

class SourceNode {
	std::size_t length;
	std::vector<std::unique_ptr<SourceNode>> children;
	int style;
public:
	SourceNode(std::size_t length): length(length), style(0) {}
	SourceNode(std::vector<std::unique_ptr<SourceNode>>&& children): length(0), children(std::move(children)), style(0) {
		for (const auto& child: this->children) {
			length += child->get_length();
		}
	}
	SourceNode(std::unique_ptr<SourceNode>&& child, int style): length(child->get_length()), style(style) {
		children.emplace_back(std::move(child));
	}
	std::size_t get_length() const {
		return length;
	}
	const std::vector<std::unique_ptr<SourceNode>>& get_children() const {
		return children;
	}
	int get_style() const {
		return style;
	}
};

class Char {
	char c;
public:
	constexpr Char(char c): c(c) {}
	template <class I> std::unique_ptr<SourceNode> match(I& i, const I& end) const {
		if (i != end && *i == c) {
			++i;
			return std::make_unique<SourceNode>(1);
		}
		return nullptr;
	}
};

class Range {
	char first;
	char last;
public:
	constexpr Range(char first, char last): first(first), last(last) {}
	template <class I> std::unique_ptr<SourceNode> match(I& i, const I& end) const {
		if (i != end && *i >= first && *i <= last) {
			++i;
			return std::make_unique<SourceNode>(1);
		}
		return nullptr;
	}
};

class AnyChar {
public:
	template <class I> std::unique_ptr<SourceNode> match(I& i, const I& end) const {
		if (i != end) {
			++i;
			return std::make_unique<SourceNode>(1);
		}
		return nullptr;
	}
};

class String {
	const char* string;
public:
	constexpr String(const char* string): string(string) {}
	template <class I> std::unique_ptr<SourceNode> match(I& i, const I& end) const {
		const I begin = i;
		std::size_t length = 0;
		for (const char* s = string; *s != '\0'; ++s) {
			if (i != end && *i == *s) {
				++i;
				++length;
			}
			else {
				i = begin;
				return nullptr;
			}
		}
		return std::make_unique<SourceNode>(length);
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

template <class... T> class Sequence;
template <> class Sequence<> {
public:
	template <class I> bool match(I& i, const I& end, std::vector<std::unique_ptr<SourceNode>>& source_children) const {
		return true;
	}
	template <class I> std::unique_ptr<SourceNode> match(I& i, const I& end) const {
		return std::make_unique<SourceNode>(0);
	}
};
template <class T0, class... T> class Sequence<T0, T...> {
	T0 t0;
	Sequence<T...> sequence;
public:
	constexpr Sequence(const T0& t0, const T&... t): t0(t0), sequence(t...) {}
	template <class I> bool match(I& i, const I& end, std::vector<std::unique_ptr<SourceNode>>& source_children) const {
		if (auto source_node = t0.match(i, end)) {
			source_children.emplace_back(std::move(source_node));
			return sequence.match(i, end, source_children);
		}
		else {
			return false;
		}
	}
	template <class I> std::unique_ptr<SourceNode> match(I& i, const I& end) const {
		const I begin = i;
		std::vector<std::unique_ptr<SourceNode>> source_children;
		if (match(i, end, source_children)) {
			return std::make_unique<SourceNode>(std::move(source_children));
		}
		else {
			i = begin;
			return nullptr;
		}
	}
};
template <class... T> Sequence(const T&...) -> Sequence<T...>;

template <class... T> class Choice;
template <> class Choice<> {
public:
	template <class I> std::unique_ptr<SourceNode> match(I& i, const I& end) const {
		return nullptr;
	}
};
template <class T0, class... T> class Choice<T0, T...> {
	T0 t0;
	Choice<T...> choice;
public:
	constexpr Choice(const T0& t0, const T&... t): t0(t0), choice(t...) {}
	template <class I> std::unique_ptr<SourceNode> match(I& i, const I& end) const {
		if (auto source_node = t0.match(i, end)) {
			return source_node;
		}
		else {
			return choice.match(i, end);
		}
	}
};
template <class... T> Choice(const T&...) -> Choice<T...>;

template <class T> class Repetition {
	T t;
public:
	constexpr Repetition(const T& t): t(t) {}
	template <class I> std::unique_ptr<SourceNode> match(I& i, const I& end) const {
		std::vector<std::unique_ptr<SourceNode>> source_children;
		while (auto source_node = t.match(i, end)) {
			source_children.emplace_back(std::move(source_node));
		}
		return std::make_unique<SourceNode>(std::move(source_children));
	}
};

template <class T> class Optional {
	T t;
public:
	constexpr Optional(const T& t): t(t) {}
	template <class I> std::unique_ptr<SourceNode> match(I& i, const I& end) const {
		if (auto source_node = t.match(i, end)) {
			return source_node;
		}
		else {
			return std::make_unique<SourceNode>(0);
		}
	}
};

template <class T> class Not {
	T t;
public:
	constexpr Not(const T& t): t(t) {}
	template <class I> std::unique_ptr<SourceNode> match(I& i, const I& end) const {
		const I begin = i;
		if (t.match(i, end)) {
			i = begin;
			return nullptr;
		}
		else {
			return std::make_unique<SourceNode>(0);
		}
	}
};

template <class T> class But {
	T t;
public:
	constexpr But(const T& t): t(t) {}
};

class End {
public:
};

template <class T> class Highlight {
	T t;
	int style;
public:
	constexpr Highlight(const T& t, int style): t(t), style(style) {}
	template <class I> std::unique_ptr<SourceNode> match(I& i, const I& end) const {
		if (auto source_node = t.match(i, end)) {
			return std::make_unique<SourceNode>(std::move(source_node), style);
		}
		else {
			return nullptr;
		}
	}
};

constexpr Char get_language_node(char c) {
	return Char(c);
}
constexpr String get_language_node(const char* s) {
	return String(s);
}
template <class T> constexpr T get_language_node(T language_node) {
	return language_node;
}

constexpr Range range(char first, char last) {
	return Range(first, last);
}
constexpr AnyChar any_char() {
	return AnyChar();
}
template <class... T> constexpr auto sequence(T... children) {
	return Sequence(get_language_node(children)...);
}
template <class... T> constexpr auto choice(T... children) {
	return Choice(get_language_node(children)...);
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

constexpr auto hex_digit = choice(range('0', '9'), range('a', 'f'), range('A', 'F'));

#include "languages/c.hpp"
#include "languages/cplusplus.hpp"
#include "languages/java.hpp"
#include "languages/javascript.hpp"
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

template <class E, class T> class LanguageImplementation: public LanguageInterface<E> {
	T syntax;
	std::unique_ptr<SourceNode> source_node;
public:
	LanguageImplementation(const T& syntax): syntax(syntax) {}
	void invalidate(std::size_t index) override {
		source_node = nullptr;
	}
	static void get_spans(std::size_t index0, std::size_t index1, std::vector<Span>& spans, const std::unique_ptr<SourceNode>& node, std::size_t index = 0, int outer_style = Style::DEFAULT) {
		if (index > index1) return;
		std::size_t end_index = index + node->get_length();
		if (end_index < index0) return;
		const int style = node->get_style() >= Style::DEFAULT ? node->get_style() : outer_style;
		if (node->get_children().empty()) {
			index = std::max(index, index0) - index0;
			end_index = std::min(end_index, index1) - index0;
			if (index != end_index) {
				if (!spans.empty() && spans.back().last == index && spans.back().style == style) {
					spans.back().last = end_index;
				}
				else {
					spans.emplace_back(index, end_index, style);
				}
			}
		}
		else {
			for (const auto& child: node->get_children()) {
				get_spans(index0, index1, spans, child, index, style);
				index += child->get_length();
			}
		}
	}
	static bool get_word(std::size_t index, std::size_t& word_start, std::size_t& word_end, const std::unique_ptr<SourceNode>& node, std::size_t start_index = 0) {
		if (start_index > index) return false;
		std::size_t end_index = start_index + node->get_length();
		if (end_index < index) return false;
		if (node->get_style() == Style::WORD) {
			word_start = start_index;
			word_end = end_index;
			return true;
		}
		for (const auto& child: node->get_children()) {
			if (get_word(index, word_start, word_end, child, start_index)) return true;
			start_index += child->get_length();
		}
		return false;
	}
	static bool get_next_word(std::size_t index, std::size_t& word_start, std::size_t& word_end, const std::unique_ptr<SourceNode>& node, std::size_t start_index = 0) {
		std::size_t end_index = start_index + node->get_length();
		if (end_index <= index) return false;
		if (node->get_style() == Style::WORD) {
			word_start = start_index;
			word_end = end_index;
			return true;
		}
		for (auto i = node->get_children().begin(); i != node->get_children().end(); ++i) {
			const auto& child = *i;
			if (get_next_word(index, word_start, word_end, child, start_index)) return true;
			start_index += child->get_length();
		}
		return false;
	}
	static bool get_previous_word(std::size_t index, std::size_t& word_start, std::size_t& word_end, const std::unique_ptr<SourceNode>& node, std::size_t start_index = 0) {
		if (start_index >= index) return false;
		std::size_t end_index = start_index + node->get_length();
		if (node->get_style() == Style::WORD) {
			word_start = start_index;
			word_end = end_index;
			return true;
		}
		for (auto i = node->get_children().rbegin(); i != node->get_children().rend(); ++i) {
			const auto& child = *i;
			end_index -= child->get_length();
			if (get_previous_word(index, word_start, word_end, child, end_index)) return true;
		}
		return false;
	}
	void highlight(const E& buffer, JSONWriter& writer, std::size_t index0, std::size_t index1) override {
		if (buffer.get_size() > 10000) {
			// disable syntax highlighting for large files until we have incremental syntax highlighting
			writer.write_array([](JSONArrayWriter& writer) {});
			return;
		}
		if (source_node == nullptr) {
			auto i = buffer.begin();
			source_node = syntax.match(i, buffer.end());
		}
		std::vector<Span> spans;
		get_spans(index0, index1, spans, source_node);
		writer.write_array([&](JSONArrayWriter& writer) {
			for (const auto& span: spans) {
				writer.write_element().write_array([&](JSONArrayWriter& writer) {
					writer.write_element().write_number(span.first);
					writer.write_element().write_number(span.last);
					writer.write_element().write_number(span.style - Style::DEFAULT);
				});
			}
		});
	}
	void get_word(const E& buffer, std::size_t index, std::size_t& word_start, std::size_t& word_end) override {
		word_start = index;
		word_end = index;
		if (buffer.get_size() > 10000) {
			return;
		}
		if (source_node == nullptr) {
			auto i = buffer.begin();
			source_node = syntax.match(i, buffer.end());
		}
		get_word(index, word_start, word_end, source_node);
	}
	void get_next_word(const E& buffer, std::size_t index, std::size_t& word_start, std::size_t& word_end) override {
		if (buffer.get_size() > 10000) {
			word_start = word_end = buffer.get_size() - 1;
			return;
		}
		if (source_node == nullptr) {
			auto i = buffer.begin();
			source_node = syntax.match(i, buffer.end());
		}
		if (!get_next_word(index, word_start, word_end, source_node)) {
			word_start = word_end = buffer.get_size() - 1;
		}
	}
	void get_previous_word(const E& buffer, std::size_t index, std::size_t& word_start, std::size_t& word_end) override {
		if (buffer.get_size() > 10000) {
			word_start = word_end = 0;
			return;
		}
		if (source_node == nullptr) {
			auto i = buffer.begin();
			source_node = syntax.match(i, buffer.end());
		}
		if (!get_previous_word(index, word_start, word_end, source_node)) {
			word_start = word_end = 0;
		}
	}
};

template <class T> bool match_string(const T& t, const char* string) {
	return t.match(string, string + std::char_traits<char>::length(string)) != nullptr;
}

template <class T> constexpr auto ends_with(T t) {
	const auto e = sequence(t, end());
	return sequence(repetition(but(e)), e);
}

template <class E> std::unique_ptr<LanguageInterface<E>> get_language(const E& buffer, const char* file_name) {
	if (match_string(ends_with(".c"), file_name)) {
		return std::make_unique<LanguageImplementation<E, decltype(c_syntax)>>(c_syntax);
	}
	if (match_string(ends_with(choice(".cpp", ".cc", ".hpp", ".hh", ".h")), file_name)) {
		return std::make_unique<LanguageImplementation<E, decltype(cplusplus_syntax)>>(cplusplus_syntax);
	}
	if (match_string(ends_with(".java"), file_name)) {
		return std::make_unique<LanguageImplementation<E, decltype(java_syntax)>>(java_syntax);
	}
	if (match_string(ends_with(".js"), file_name)) {
		return std::make_unique<LanguageImplementation<E, decltype(javascript_syntax)>>(javascript_syntax);
	}
	if (match_string(ends_with(".hs"), file_name)) {
		return std::make_unique<LanguageImplementation<E, decltype(haskell_syntax)>>(haskell_syntax);
	}
	return std::make_unique<NoLanguage<E>>();
}
