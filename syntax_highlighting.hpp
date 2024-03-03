#pragma once

#include "prism/prism.hpp"
#include <memory>

inline void write_color(const Color& color, JSONWriter& writer) {
	writer.write_array([&](JSONArrayWriter& writer) {
		writer.write_element().write_number(color.r * 255.f + .5f);
		writer.write_element().write_number(color.g * 255.f + .5f);
		writer.write_element().write_number(color.b * 255.f + .5f);
		writer.write_element().write_number(color.a * 255.f + .5f);
	});
}
inline void write_style(const Style& style, JSONWriter& writer) {
	writer.write_object([&](JSONObjectWriter& writer) {
		write_color(style.color, writer.write_member("color"));
		writer.write_member("bold").write_boolean(style.bold);
		writer.write_member("italic").write_boolean(style.italic);
	});
}
inline void write_theme(const Theme& theme, JSONWriter& writer) {
	writer.write_object([&](JSONObjectWriter& writer) {
		write_color(theme.background, writer.write_member("background"));
		write_color(theme.background_active, writer.write_member("background_active"));
		write_color(theme.selection, writer.write_member("selection"));
		write_color(theme.cursor, writer.write_member("cursor"));
		write_color(theme.number_background, writer.write_member("number_background"));
		write_color(theme.number_background_active, writer.write_member("number_background_active"));
		write_style(theme.number, writer.write_member("number"));
		write_style(theme.number_active, writer.write_member("number_active"));
		writer.write_member("styles").write_array([&](JSONArrayWriter& writer) {
			for (const Style& style: theme.styles) {
				write_style(style, writer.write_element());
			}
		});
	});
}

template <class E> class LanguageInterface {
public:
	virtual ~LanguageInterface() = default;
	virtual void invalidate(std::size_t index) = 0;
	virtual void highlight(const E& buffer, JSONWriter& writer, std::size_t index0, std::size_t index1) = 0;
	virtual void get_word(const E& buffer, std::size_t index, std::size_t& word_start, std::size_t& word_end) = 0;
	virtual void get_next_word(const E& buffer, std::size_t index, std::size_t& word_start, std::size_t& word_end) = 0;
	virtual void get_previous_word(const E& buffer, std::size_t index, std::size_t& word_start, std::size_t& word_end) = 0;
};

template <class E> class NoLanguage final: public LanguageInterface<E> {
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

template <class E> class PrismLanguage final: public LanguageInterface<E> {
	const char* language;
	prism::Tree tree;
public:
	PrismLanguage(const char* language): language(language) {}
	void invalidate(std::size_t index) override {
		tree.edit(index);
	}
	void highlight(const E& buffer, JSONWriter& writer, std::size_t index0, std::size_t index1) override {
		auto spans = prism::highlight(language, &buffer, tree, index0, index1);
		writer.write_array([&](JSONArrayWriter& writer) {
			for (const Span& span: spans) {
				writer.write_element().write_array([&](JSONArrayWriter& writer) {
					writer.write_element().write_number(std::max(span.start, index0) - index0);
					writer.write_element().write_number(std::min(span.end, index1) - index0);
					writer.write_element().write_number(span.style - Style::DEFAULT);
				});
			}
		});
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

template <class E> std::unique_ptr<LanguageInterface<E>> get_language(const E& buffer, const char* file_name) {
	const char* language = prism::get_language(file_name);
	if (language) {
		return std::make_unique<PrismLanguage<E>>(language);
	}
	else {
		return std::make_unique<NoLanguage<E>>();
	}
}
