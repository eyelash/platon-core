#pragma once

#include "prism/prism.hpp"
#include <memory>

class LanguageInterface {
public:
	virtual ~LanguageInterface() = default;
	virtual void invalidate(std::size_t index) = 0;
	virtual void highlight(const Input& input, std::vector<Span>& spans, std::size_t index0, std::size_t index1) = 0;
	virtual void get_word(const Input& input, std::size_t index, std::size_t& word_start, std::size_t& word_end) = 0;
	virtual void get_next_word(const Input& input, std::size_t index, std::size_t& word_start, std::size_t& word_end) = 0;
	virtual void get_previous_word(const Input& input, std::size_t index, std::size_t& word_start, std::size_t& word_end) = 0;
};

class NoLanguage final: public LanguageInterface {
public:
	void invalidate(std::size_t index) override {}
	void highlight(const Input& input, std::vector<Span>& spans, std::size_t index0, std::size_t index1) override {}
	void get_word(const Input& input, std::size_t index, std::size_t& word_start, std::size_t& word_end) override {
		word_start = word_end = index;
	}
	void get_next_word(const Input& input, std::size_t index, std::size_t& word_start, std::size_t& word_end) override {
		word_start = word_end = index;
	}
	void get_previous_word(const Input& input, std::size_t index, std::size_t& word_start, std::size_t& word_end) override {
		word_start = word_end = index;
	}
};

class PrismLanguage final: public LanguageInterface {
	const Language* language;
	Cache cache;
public:
	PrismLanguage(const Language* language): language(language) {}
	void invalidate(std::size_t index) override {
		cache.invalidate(index);
	}
	void highlight(const Input& input, std::vector<Span>& spans, std::size_t index0, std::size_t index1) override {
		for (const Span& span: prism::highlight(language, &input, cache, index0, index1)) {
			spans.push_back({span.start - index0, span.end - index0, span.style});
		}
	}
	void get_word(const Input& input, std::size_t index, std::size_t& word_start, std::size_t& word_end) override {
		word_start = word_end = index;
	}
	void get_next_word(const Input& input, std::size_t index, std::size_t& word_start, std::size_t& word_end) override {
		word_start = word_end = index;
	}
	void get_previous_word(const Input& input, std::size_t index, std::size_t& word_start, std::size_t& word_end) override {
		word_start = word_end = index;
	}
};

std::unique_ptr<LanguageInterface> get_language(const char* file_name) {
	const Language* language = prism::get_language(file_name);
	if (language) {
		return std::make_unique<PrismLanguage>(language);
	}
	else {
		return std::make_unique<NoLanguage>();
	}
}
