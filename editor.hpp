#pragma once

#include "piece_table.hpp"
#include "buffer.hpp"
#include "json.hpp"
#include <vector>

struct Selection {
	std::size_t first;
	std::size_t last;
	constexpr Selection(std::size_t first): first(first), last(first) {}
	constexpr Selection(std::size_t first, std::size_t last): first(first), last(last) {}
	Selection& operator +=(std::size_t n) {
		first += n;
		last += n;
		return *this;
	}
	Selection& operator -=(std::size_t n) {
		first -= n;
		last -= n;
		return *this;
	}
};

class Selections: public std::vector<Selection> {
public:
	Selections() {
		emplace_back(0);
	}
	void set_cursor(std::size_t cursor) {
		clear();
		emplace_back(cursor);
	}
	void toggle_cursor(std::size_t cursor) {
		std::size_t i;
		for (i = 0; i < size(); ++i) {
			Selection& selection = operator [](i);
			if (cursor == selection.last) {
				erase(begin() + i);
				return;
			}
			if (cursor < selection.last) {
				break;
			}
		}
		emplace(begin() + i, cursor);
	}
};

class Editor {
	PieceTable buffer;
	Newlines newlines;
	Selections selections;
	void render_selections(JSONObjectWriter& writer, std::size_t index0, std::size_t index1) const {
		writer.write_member("cursors").write_array([&](JSONArrayWriter& writer) {
			for (const Selection& selection: selections) {
				if (selection.last >= index0 && selection.last < index1) {
					writer.write_element().write_number(selection.last - index0);
				}
			}
		});
	}
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
					render_selections(writer, index0, index1);
				});
			}
		});
		return json.c_str();
	}
	void insert(const char* text) {
		for (const char* c = text; *c; ++c) {
			std::size_t offset = 0;
			for (Selection& selection: selections) {
				selection += offset;
				buffer.insert(selection.last, *c);
				newlines.insert(selection.last, *c);
				selection += 1;
				++offset;
			}
		}
	}
	void backspace() {
		std::size_t offset = 0;
		for (Selection& selection: selections) {
			if (selection.last == 0) {
				continue;
			}
			selection -= offset;
			selection -= 1;
			buffer.remove(selection.last);
			newlines.remove(selection.last);
			++offset;
		}
	}
	void set_cursor(std::size_t column, std::size_t row) {
		row = std::min(row, get_total_lines() - 1);
		std::size_t cursor = newlines.get_index(row) + column;
		cursor = std::min(cursor, newlines.get_index(row + 1) - 1);
		selections.set_cursor(cursor);
	}
	void toggle_cursor(std::size_t column, std::size_t row) {
		row = std::min(row, get_total_lines() - 1);
		std::size_t cursor = newlines.get_index(row) + column;
		cursor = std::min(cursor, newlines.get_index(row + 1) - 1);
		selections.toggle_cursor(cursor);
	}
};
