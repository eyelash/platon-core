#pragma once

#include "os.hpp"
#include "tree.hpp"
#include "json.hpp"
#include "syntax_highlighting.hpp"
#include <vector>
#include <fstream>

class TextBuffer {
	struct Info {
		using T = char;
		std::size_t chars;
		std::size_t codepoints;
		std::size_t newlines;
		constexpr Info(std::size_t chars, std::size_t codepoints, std::size_t newlines): chars(chars), codepoints(codepoints), newlines(newlines) {}
		constexpr Info(): chars(0), codepoints(0), newlines(0) {}
		constexpr Info(char c): chars(1), codepoints((c & 0xC0) != 0x80), newlines(c == '\n') {}
		constexpr Info operator +(const Info& info) const {
			return Info(chars + info.chars, codepoints + info.codepoints, newlines + info.newlines);
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
	class CodepointComp {
		std::size_t codepoints;
	public:
		constexpr CodepointComp(std::size_t codepoints): codepoints(codepoints) {}
		constexpr bool operator <(const Info& info) const {
			return codepoints < info.codepoints;
		}
	};
	class LineComp {
		std::size_t newlines;
	public:
		constexpr LineComp(std::size_t newlines): newlines(newlines) {}
		constexpr bool operator <(const Info& info) const {
			return newlines < info.newlines;
		}
	};
	Tree<Info> tree;
public:
	TextBuffer() {
		tree.insert(tree_end(), '\n');
	}
	TextBuffer(const char* path) {
		std::ifstream file(path);
		tree.append(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
		if (get_size() == 0 || *get_iterator(get_size() - 1) != '\n') {
			tree.insert(tree_end(), '\n');
		}
	}
	std::size_t get_size() const {
		return tree.get_info().chars;
	}
	std::size_t get_total_lines() const {
		return tree.get_info().newlines;
	}
	std::size_t get_index(std::size_t line) const {
		return line == 0 ? 0 : tree.get_sum(LineComp(line - 1)).chars + 1;
	}
	std::size_t get_line(std::size_t index) const {
		return tree.get_sum(CharComp(index)).newlines;
	}
	std::size_t get_codepoints_for_index(std::size_t index) const {
		return tree.get_sum(CharComp(index)).codepoints;
	}
	std::size_t get_index_for_codepoints(std::size_t codepoints) const {
		return tree.get_sum(CodepointComp(codepoints)).chars;
	}
	void insert(std::size_t index, char c) {
		tree.insert(CharComp(index), c);
	}
	void remove(std::size_t index) {
		tree.remove(CharComp(index));
	}
	Tree<Info>::Iterator get_iterator(std::size_t index) const {
		return tree.get(CharComp(index));
	}
	Tree<Info>::Iterator begin() const {
		return tree.begin();
	}
	Tree<Info>::Iterator end() const {
		return tree.end();
	}
	void save(const char* path) {
		std::ofstream file(path);
		std::copy(tree.begin(), tree.end(), std::ostreambuf_iterator<char>(file));
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
	bool is_reversed() const {
		return first > last;
	}
};

class Selections: public std::vector<Selection> {
public:
	Selections() {
		emplace_back(0);
	}
	bool find_selection(std::size_t cursor, std::size_t& index) const {
		std::size_t i;
		for (i = 0; i < size(); ++i) {
			const Selection& selection = operator [](i);
			if (selection.max() >= cursor) {
				if (selection.min() <= cursor) {
					index = i;
					return true;
				}
				else {
					break;
				}
			}
		}
		index = i;
		return false;
	}
};

class Editor {
	TextBuffer buffer;
	std::unique_ptr<LanguageInterface<TextBuffer>> language;
	Selections selections;
	std::size_t last_selection;
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
	static const char* get_file_name(const char* path) {
		const char* file_name = path;
		for (const char* i = path; *i != '\0'; ++i) {
			if (is_path_separator(*i)) {
				file_name = i + 1;
			}
		}
		return file_name;
	}
	std::size_t get_previous_index(std::size_t index) const {
		return buffer.get_index_for_codepoints(buffer.get_codepoints_for_index(index) - 1);
	}
	std::size_t get_next_index(std::size_t index) const {
		return buffer.get_index_for_codepoints(buffer.get_codepoints_for_index(index) + 1);
	}
	void delete_selections() {
		std::size_t offset = 0;
		for (Selection& selection: selections) {
			selection -= offset;
			if (selection.first != selection.last) {
				language->invalidate(selection.min());
				for (std::size_t i = selection.min(); i < selection.max(); ++i) {
					buffer.remove(selection.min());
					++offset;
				}
				selection = selection.min();
			}
		}
	}
	void collapse_selections(bool reverse_direction) {
		for (std::size_t i = 1; i < selections.size(); ++i) {
			if (selections[i-1].last == selections[i].last || selections[i-1].max() > selections[i].min()) {
				if (reverse_direction) {
					selections[i-1] = Selection(selections[i].max(), selections[i-1].min());
				}
				else {
					selections[i-1] = Selection(selections[i-1].min(), selections[i].max());
				}
				if (last_selection >= i) {
					--last_selection;
				}
				selections.erase(selections.begin() + i);
				--i;
			}
		}
	}
public:
	Editor(): language(std::make_unique<NoLanguage<TextBuffer>>()), last_selection(0) {}
	Editor(const char* path): buffer(path), language(get_language(buffer, get_file_name(path))), last_selection(0) {}
	std::size_t get_total_lines() const {
		return buffer.get_total_lines();
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
						index0 = buffer.get_index(i);
						index1 = buffer.get_index(i + 1);
					}
					writer.write_member("text").write_string(buffer.get_iterator(index0), buffer.get_iterator(index1));
					writer.write_member("number").write_number(i + 1);
					language->highlight(buffer, writer.write_member("spans"), index0, index1);
					render_selections(writer, index0, index1);
				});
			}
		});
		return json.c_str();
	}
	void insert_text(const char* text) {
		delete_selections();
		std::size_t offset = 0;
		for (Selection& selection: selections) {
			selection += offset;
			language->invalidate(selection.last);
			for (const char* c = text; *c; ++c) {
				buffer.insert(selection.last, *c);
				selection += 1;
				++offset;
			}
		}
	}
	void insert_newline() {
		delete_selections();
		std::size_t offset = 0;
		for (Selection& selection: selections) {
			selection += offset;
			language->invalidate(selection.last);
			buffer.insert(selection.last, '\n');
			selection += 1;
			++offset;
			auto i = buffer.get_iterator(buffer.get_index(buffer.get_line(selection.last) - 1));
			while (i != buffer.end() && (*i == ' ' || *i == '\t')) {
				buffer.insert(selection.last, *i);
				selection += 1;
				++offset;
				++i;
			}
		}
	}
	void delete_backward() {
		std::size_t offset = 0;
		for (Selection& selection: selections) {
			selection -= offset;
			if (selection.first == selection.last && selection.last > 0) {
				selection.last = get_previous_index(selection.last);
			}
			language->invalidate(selection.min());
			for (std::size_t i = selection.min(); i < selection.max(); ++i) {
				buffer.remove(selection.min());
				++offset;
			}
			selection = selection.min();
		}
		collapse_selections(true);
	}
	void delete_forward() {
		std::size_t last = buffer.get_size() - 1;
		std::size_t offset = 0;
		for (Selection& selection: selections) {
			selection -= offset;
			if (selection.first == selection.last && selection.last < last) {
				selection.last = get_next_index(selection.last);
			}
			language->invalidate(selection.min());
			for (std::size_t i = selection.min(); i < selection.max(); ++i) {
				buffer.remove(selection.min());
				--last;
				++offset;
			}
			selection = selection.min();
		}
		collapse_selections(false);
	}
	std::size_t get_index(std::size_t column, std::size_t line) const {
		if (line > get_total_lines() - 1) {
			return buffer.get_size() - 1;
		}
		else {
			std::size_t index = buffer.get_index(line) + column;
			return std::min(index, buffer.get_index(line + 1) - 1);
		}
	}
	void set_cursor(std::size_t column, std::size_t line) {
		selections.clear();
		selections.emplace_back(get_index(column, line));
		last_selection = 0;
	}
	void toggle_cursor(std::size_t column, std::size_t line) {
		const std::size_t cursor = get_index(column, line);
		std::size_t index;
		if (selections.find_selection(cursor, index)) {
			selections.erase(selections.begin() + index);
			if (last_selection == index) {
				last_selection = selections.size() - 1;
			}
			else if (last_selection > index) {
				--last_selection;
			}
		}
		else {
			selections.emplace(selections.begin() + index, cursor);
			last_selection = index;
		}
	}
	void extend_selection(std::size_t column, std::size_t line) {
		Selection& selection = selections[last_selection];
		selection.last = get_index(column, line);
		collapse_selections(selection.is_reversed());
	}
	void move_left(bool extend_selection = false) {
		for (Selection& selection: selections) {
			if (extend_selection) {
				if (selection.last > 0) {
					selection.last = get_previous_index(selection.last);
				}
			}
			else {
				if (selection.first == selection.last) {
					if (selection.last > 0) {
						selection = get_previous_index(selection.last);
					}
				}
				else {
					selection = selection.min();
				}
			}
		}
		collapse_selections(true);
	}
	void move_right(bool extend_selection = false) {
		const std::size_t last = buffer.get_size() - 1;
		for (Selection& selection: selections) {
			if (extend_selection) {
				if (selection.last < last) {
					selection.last = get_next_index(selection.last);
				}
			}
			else {
				if (selection.first == selection.last) {
					if (selection.last < last) {
						selection = get_next_index(selection.last);
					}
				}
				else {
					selection = selection.max();
				}
			}
		}
		collapse_selections(false);
	}
	std::size_t get_index2(std::size_t column, std::size_t line) const {
		// the column is measured in codepoints
		std::size_t codepoints = buffer.get_codepoints_for_index(buffer.get_index(line)) + column;
		std::size_t max_codepoints = buffer.get_codepoints_for_index(buffer.get_index(line + 1) - 1);
		return buffer.get_index_for_codepoints(std::min(codepoints, max_codepoints));
	}
	void move_up(bool extend_selection = false) {
		for (Selection& selection: selections) {
			std::size_t line = buffer.get_line(selection.last);
			std::size_t column = buffer.get_codepoints_for_index(selection.last) - buffer.get_codepoints_for_index(buffer.get_index(line));
			if (extend_selection) {
				if (line > 0) {
					selection.last = get_index2(column, line - 1);
				}
			}
			else {
				if (selection.first == selection.last) {
					if (line > 0) {
						selection = get_index2(column, line - 1);
					}
				}
				else {
					selection = selection.min();
				}
			}
		}
		collapse_selections(true);
	}
	void move_down(bool extend_selection = false) {
		const std::size_t last_line = get_total_lines() - 1;
		for (Selection& selection: selections) {
			std::size_t line = buffer.get_line(selection.last);
			std::size_t column = buffer.get_codepoints_for_index(selection.last) - buffer.get_codepoints_for_index(buffer.get_index(line));
			if (extend_selection) {
				if (line < last_line) {
					selection.last = get_index2(column, line + 1);
				}
			}
			else {
				if (selection.first == selection.last) {
					if (line < last_line) {
						selection = get_index2(column, line + 1);
					}
				}
				else {
					selection = selection.max();
				}
			}
		}
		collapse_selections(false);
	}
	void move_to_beginning_of_word(bool extend_selection = false) {
		for (Selection& selection: selections) {
			std::size_t word_start, word_end;
			language->get_previous_word(buffer, selection.last, word_start, word_end);
			if (extend_selection) {
				selection.last = word_start;
			}
			else {
				if (selection.first == selection.last) {
					selection = word_start;
				}
				else {
					selection = selection.min();
				}
			}
		}
		collapse_selections(true);
	}
	void move_to_end_of_word(bool extend_selection = false) {
		for (Selection& selection: selections) {
			std::size_t word_start, word_end;
			language->get_next_word(buffer, selection.last, word_start, word_end);
			if (extend_selection) {
				selection.last = word_end;
			}
			else {
				if (selection.first == selection.last) {
					selection = word_end;
				}
				else {
					selection = selection.max();
				}
			}
		}
		collapse_selections(false);
	}
	void move_to_beginning_of_line(bool extend_selection = false) {
		for (Selection& selection: selections) {
			selection.last = buffer.get_index(buffer.get_line(selection.last));
			if (!extend_selection) {
				selection.first = selection.last;
			}
		}
		collapse_selections(true);
	}
	void move_to_end_of_line(bool extend_selection = false) {
		for (Selection& selection: selections) {
			selection.last = buffer.get_index(buffer.get_line(selection.last) + 1) - 1;
			if (!extend_selection) {
				selection.first = selection.last;
			}
		}
		collapse_selections(false);
	}
	void select_all() {
		selections.clear();
		selections.emplace_back(0, buffer.get_size() - 1);
		last_selection = 0;
	}
	const char* get_theme() const {
		static std::string json;
		json.clear();
		JSONWriter writer(json);
		default_theme.write(writer);
		return json.c_str();
	}
	const char* copy() const {
		static std::string string;
		string.clear();
		auto i = selections.begin();
		if (i != selections.end()) {
			string.append(buffer.get_iterator(i->min()), buffer.get_iterator(i->max()));
			++i;
			while (i != selections.end()) {
				string.push_back('\n');
				string.append(buffer.get_iterator(i->min()), buffer.get_iterator(i->max()));
				++i;
			}
		}
		return string.c_str();
	}
	const char* cut() {
		const char* result = copy();
		delete_selections();
		return result;
	}
	void paste(const char* text) {
		std::size_t newlines = 0;
		for (const char* c = text; *c; ++c) {
			newlines += *c == '\n';
		}
		if (newlines + 1 == selections.size()) {
			delete_selections();
			newlines = 0;
			std::size_t offset = 0;
			for (const char* c = text; *c; ++c) {
				if (*c == '\n') {
					++newlines;
					selections[newlines] += offset;
				}
				else {
					Selection& selection = selections[newlines];
					language->invalidate(selection.last);
					buffer.insert(selection.last, *c);
					selection += 1;
					++offset;
				}
			}
		}
		else {
			insert_text(text);
		}
	}
	void save(const char* path) {
		buffer.save(path);
	}
};
