#pragma once

#include "buffer.hpp"
#include <string>

class JSONString: public std::string {
	static constexpr std::size_t strlen(const char* s, std::size_t i = 0) {
		return *s == '\0' ? i : strlen(s + 1, i + 1);
	}
public:
	void push_back_string(const char* first, const char* last) {
		push_back('"');
		for (; first != last; ++first) {
			const char c = *first;
			if (c == '"' || c == '\\')
				push_back('\\');
			push_back(c);
		}
		push_back('"');
	}
	void push_back_string(const char* s) {
		push_back_string(s, s + strlen(s));
	}
};

class Editor {
	Buffer buffer;
	Newlines newlines;
public:
	Editor(const char* path): buffer(path), newlines(buffer.begin(), buffer.end()) {}
	std::size_t get_total_lines() const {
		return newlines.get_total_lines();
	}
	const char* render(std::size_t first_line, std::size_t last_line) const {
		static JSONString json;
		json.clear();
		json.push_back('[');
		for (std::size_t i = first_line; i < last_line; ++i) {
			if (i > first_line) json.push_back(',');
			json.push_back('{');
			json.push_back_string("text");
			json.push_back(':');
			if (i < get_total_lines()) {
				const std::size_t index0 = newlines.get_index(i);
				const std::size_t index1 = newlines.get_index(i + 1);
				json.push_back_string(buffer.begin() + index0, buffer.begin() + index1);
			}
			else {
				json.push_back_string("");
			}
			json.push_back('}');
		}
		json.push_back(']');
		return json.c_str();
	}
};
