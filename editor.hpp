#pragma once

#include "piece_table.hpp"
#include "tree.hpp"
#include "json.hpp"
#include "syntax_highlighting.hpp"
#include <vector>

class Breaks {
	class Info {
	public:
		std::size_t chars;
		std::size_t lines;
		constexpr Info(std::size_t chars, std::size_t lines): chars(chars), lines(lines) {}
		using T = std::size_t;
		constexpr Info(): chars(0), lines(0) {}
		constexpr Info(std::size_t chars): chars(chars), lines(1) {}
		constexpr Info operator +(const Info& info) {
			return Info(chars + info.chars, lines + info.lines);
		}
	};
	class CharComp {
		std::size_t chars;
	public:
		constexpr CharComp(std::size_t chars): chars(chars) {}
		constexpr bool operator <(const Info& info) const {
			return chars < info.chars;
		}
	};
	class LineComp {
		std::size_t line;
	public:
		constexpr LineComp(std::size_t line): line(line) {}
		constexpr bool operator <(const Info& info) const {
			return line < info.lines;
		}
	};
	Tree<Info> tree;
public:
	template <class E> Breaks(const E& buffer) {
		auto iterator = buffer.begin();
		std::size_t index = 0;
		while (*iterator != '\0') {
			++index;
			if (*iterator == '\n') {
				tree.append(index);
				index = 0;
			}
			++iterator;
		}
	}
	std::size_t get_total_lines() const {
		return tree.get_info().lines;
	}
	std::size_t get_index(std::size_t line) const {
		return tree.get_sum(LineComp(line)).chars;
	}
	std::size_t get_line(std::size_t index) const {
		return tree.get_sum(CharComp(index)).lines;
	}
	void insert(std::size_t index, char c) {
		const Info sum = tree.get_sum(CharComp(index));
		const std::size_t line = tree.get(CharComp(index));
		tree.remove(CharComp(index));
		if (c == '\n') {
			tree.insert(CharComp(sum.chars), line - (index - sum.chars));
			tree.insert(CharComp(sum.chars), (index - sum.chars) + 1);
		}
		else {
			tree.insert(CharComp(sum.chars), line + 1);
		}
	}
	void remove(std::size_t index) {
		const Info sum = tree.get_sum(CharComp(index));
		std::size_t line = tree.get(CharComp(index));
		tree.remove(CharComp(index));
		--line;
		if (index - sum.chars == line) {
			line += tree.get(CharComp(sum.chars));
			tree.remove(CharComp(sum.chars));
			tree.insert(CharComp(sum.chars), line);
		}
		else {
			tree.insert(CharComp(sum.chars), line);
		}
	}
};

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
	std::size_t min() const {
		return std::min(first, last);
	}
	std::size_t max() const {
		return std::max(first, last);
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
			const Selection& selection = operator [](i);
			if (selection.max() >= cursor) {
				if (selection.min() <= cursor) {
					erase(begin() + i);
					return;
				}
				else {
					break;
				}
			}
		}
		emplace(begin() + i, cursor);
	}
};

class Editor {
	PieceTable buffer;
	Breaks newlines;
	Language language;
	Selections selections;
	void render_selections(JSONObjectWriter& writer, std::size_t index0, std::size_t index1) const {
		writer.write_member("selections").write_array([&](JSONArrayWriter& writer) {
			for (const Selection& selection: selections) {
				if (selection.max() > index0 && selection.min() < index1) {
					writer.write_element().write_array([&](JSONArrayWriter& writer) {
						writer.write_element().write_number(std::max(selection.min(), index0) - index0);
						writer.write_element().write_number(std::min(selection.max(), index1) - index0);
					});
				}
			}
		});
		writer.write_member("cursors").write_array([&](JSONArrayWriter& writer) {
			for (const Selection& selection: selections) {
				if (selection.last >= index0 && selection.last < index1) {
					writer.write_element().write_number(selection.last - index0);
				}
			}
		});
	}
public:
	Editor(): newlines(buffer) {}
	Editor(const char* path): buffer(path), newlines(buffer) {}
	std::size_t get_total_lines() const {
		return newlines.get_total_lines();
	}
	const char* render(std::size_t first_line, std::size_t last_line) {
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
					writer.write_member("number").write_number(i + 1);
					language.highlight(buffer, writer.write_member("spans"), index0, index1);
					render_selections(writer, index0, index1);
				});
			}
		});
		return json.c_str();
	}
	void insert(const char* text) {
		std::size_t offset = 0;
		for (Selection& selection: selections) {
			selection -= offset;
			if (selection.first != selection.last) {
				language.invalidate(selection.min());
				for (std::size_t i = selection.min(); i < selection.max(); ++i) {
					buffer.remove(selection.min());
					newlines.remove(selection.min());
					++offset;
				}
				selection.first = selection.last = selection.min();
			}
		}
		offset = 0;
		for (Selection& selection: selections) {
			selection += offset;
			language.invalidate(selection.last);
			for (const char* c = text; *c; ++c) {
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
			selection -= offset;
			if (selection.first == selection.last) {
				if (selection.last > 0) {
					selection.first = selection.last -= 1;
					language.invalidate(selection.last);
					buffer.remove(selection.last);
					newlines.remove(selection.last);
					++offset;
				}
			}
			else {
				language.invalidate(selection.min());
				for (std::size_t i = selection.min(); i < selection.max(); ++i) {
					buffer.remove(selection.min());
					newlines.remove(selection.min());
					++offset;
				}
				selection.first = selection.last = selection.min();
			}
		}
		// collapse selections
		for (std::size_t i = 1; i < selections.size(); ++i) {
			if (selections[i-1].last == selections[i].last) {
				selections.erase(selections.begin() + i);
				--i;
			}
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
	void move_left(bool extend_selection = false) {
		for (Selection& selection: selections) {
			if (extend_selection) {
				if (selection.last > 0) {
					selection.last -= 1;
				}
			}
			else {
				if (selection.first == selection.last) {
					if (selection.last > 0) {
						selection.first = selection.last -= 1;
					}
				}
				else {
					selection.first = selection.last = selection.min();
				}
			}
		}
		// collapse selections
		for (std::size_t i = 1; i < selections.size(); ++i) {
			if (selections[i].last == selections[i-1].last || selections[i].last < selections[i-1].first) {
				selections[i-1] = Selection(selections[i].first, selections[i-1].last);
				selections.erase(selections.begin() + i);
				--i;
			}
		}
	}
	void move_right(bool extend_selection = false) {
		const std::size_t last = buffer.get_size() - 1;
		for (Selection& selection: selections) {
			if (extend_selection) {
				if (selection.last < last) {
					selection.last += 1;
				}
			}
			else {
				if (selection.first == selection.last) {
					if (selection.last < last) {
						selection.first = selection.last += 1;
					}
				}
				else {
					selection.first = selection.last = selection.max();
				}
			}
		}
		// collapse selections
		for (std::size_t i = 1; i < selections.size(); ++i) {
			if (selections[i-1].last == selections[i].last || selections[i-1].last > selections[i].first) {
				selections[i-1] = Selection(selections[i-1].first, selections[i].last);
				selections.erase(selections.begin() + i);
				--i;
			}
		}
	}
	const char* get_theme() const {
		static std::string json;
		json.clear();
		JSONWriter writer(json);
		default_theme.write(writer);
		return json.c_str();
	}
	void save(const char* path) {
		buffer.save(path);
	}
};
