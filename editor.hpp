#pragma once

#include "buffer.hpp"
#include "json.hpp"

class Editor {
	Buffer buffer;
	Newlines newlines;
public:
	Editor(const char* path): buffer(path), newlines(buffer.begin(), buffer.end()) {}
	std::size_t get_total_lines() const {
		return newlines.get_total_lines();
	}
	const char* render(std::size_t first_line, std::size_t last_line) const {
		static std::string json;
		json.clear();
		JSONWriter writer(json);
		writer.write_array([&](JSONArrayWriter& writer) {
			for (std::size_t i = first_line; i < last_line; ++i) {
				writer.write_element().write_object([&](JSONObjectWriter& writer) {
					std::size_t index0 = 0;
					std::size_t index1 = 0;
					if (i < get_total_lines()) {
						index0 = newlines.get_index(i);
						index1 = newlines.get_index(i + 1);
					}
					writer.write_member("text").write_string(buffer.begin() + index0, buffer.begin() + index1);
				});
			}
		});
		return json.c_str();
	}
};
