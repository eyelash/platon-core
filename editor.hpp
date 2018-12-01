#pragma once

#include "piece_table.hpp"
#include "buffer.hpp"
#include "json.hpp"

class Editor {
	PieceTable buffer;
	Newlines newlines;
	std::size_t cursor = 0;
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
					writer.write_member("text").write_string(buffer.get_iterator(index0), buffer.get_iterator(index1));
					writer.write_member("cursors").write_array([&](JSONArrayWriter& writer) {
						if (index0 <= cursor && cursor < index1) {
							writer.write_element().write_number(cursor - index0);
						}
					});
				});
			}
		});
		return json.c_str();
	}
	void insert(const char* text) {
		for (const char* c = text; *c; ++c) {
			buffer.insert(cursor, *c);
			newlines.insert(cursor);
			++cursor;
		}
	}
	void backspace() {
		if (cursor > 0) {
			buffer.remove(cursor - 1);
			newlines.remove(cursor - 1);
			--cursor;
		}
	}
	void set_cursor(std::size_t column, std::size_t row) {
		row = std::min(row, get_total_lines() - 1);
		cursor = newlines.get_index(row) + column;
		cursor = std::min(cursor, newlines.get_index(row + 1) - 1);
	}
};
