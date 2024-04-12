#pragma once

#include "os.hpp"
#include "tree.hpp"
#include "prism/prism.hpp"
#include <vector>
#include <fstream>

class TextBuffer final: public Input {
	struct Info {
		using T = char;
		// these sizes are tuned for a node size of 128 bytes
		static constexpr std::size_t LEAF_SIZE = 80;
		static constexpr std::size_t INODE_SIZE = 12;
		std::size_t bytes;
		std::size_t codepoints;
		std::size_t newlines;
		constexpr Info(std::size_t bytes, std::size_t codepoints, std::size_t newlines): bytes(bytes), codepoints(codepoints), newlines(newlines) {}
		constexpr Info(): bytes(0), codepoints(0), newlines(0) {}
		constexpr Info(char c): bytes(1), codepoints((c & 0xC0) != 0x80), newlines(c == '\n') {}
		constexpr Info operator +(const Info& info) const {
			return Info(bytes + info.bytes, codepoints + info.codepoints, newlines + info.newlines);
		}
	};
	class ByteComp {
		std::size_t bytes;
	public:
		constexpr ByteComp(std::size_t bytes): bytes(bytes) {}
		constexpr bool operator <(const Info& info) const {
			return bytes < info.bytes;
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
	Info get_info() const {
		return tree.get_info();
	}
	Info get_info_for_index(std::size_t index) const {
		return tree.get_sum(ByteComp(index));
	}
	Info get_info_for_codepoints(std::size_t codepoints) const {
		return tree.get_sum(CodepointComp(codepoints));
	}
	Info get_info_for_line_start(std::size_t line) const {
		return line == 0 ? Info() : tree.get_sum(LineComp(line - 1)) + Info('\n');
	}
	Info get_info_for_line_end(std::size_t line) const {
		return tree.get_sum(LineComp(line));
	}
	std::size_t get_size() const {
		return get_info().bytes;
	}
	std::size_t get_total_lines() const {
		return get_info().newlines;
	}
	void insert(std::size_t index, char c) {
		tree.insert(ByteComp(index), c);
	}
	void remove(std::size_t index) {
		tree.remove(ByteComp(index));
	}
	Tree<Info>::Iterator get_iterator(std::size_t index) const {
		return tree.get(ByteComp(index));
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
	std::pair<Input::Chunk, std::size_t> get_chunk(std::size_t index) const override {
		const auto iterator = get_iterator(index);
		auto leaf = iterator.get_leaf();
		return {{leaf, leaf->children.get_data(), leaf->children.get_size()}, index - iterator.get_index()};
	}
	Input::Chunk get_next_chunk(const void* chunk) const override {
		auto next_leaf = static_cast<const Tree<Info>::Leaf*>(chunk)->next_leaf;
		if (next_leaf) {
			return {next_leaf, next_leaf->children.get_data(), next_leaf->children.get_size()};
		}
		else {
			return {nullptr, "", 0};
		}
	}
};

struct Selection {
	std::size_t tail;
	std::size_t head;
	constexpr Selection(std::size_t head): tail(head), head(head) {}
	constexpr Selection(std::size_t tail, std::size_t head): tail(tail), head(head) {}
	Selection& operator +=(std::size_t n) {
		tail += n;
		head += n;
		return *this;
	}
	Selection& operator -=(std::size_t n) {
		tail -= n;
		head -= n;
		return *this;
	}
	constexpr bool is_empty() const {
		return tail == head;
	}
	constexpr bool is_reversed() const {
		return tail > head;
	}
	constexpr std::size_t min() const {
		return is_reversed() ? head : tail;
	}
	constexpr std::size_t max() const {
		return is_reversed() ? tail : head;
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

struct RenderedLine {
	std::string text;
	std::size_t number;
	std::vector<Span> spans;
	std::vector<Range> selections;
	std::vector<std::size_t> cursors;
};

class Editor {
	TextBuffer buffer;
	const Language* language;
	mutable Cache cache;
	Selections selections;
	std::size_t last_selection;
	void highlight(std::vector<Span>& spans, std::size_t index0, std::size_t index1) const {
		if (language == nullptr) {
			return;
		}
		for (const Span& span: prism::highlight(language, &buffer, cache, index0, index1)) {
			spans.emplace_back(span.start - index0, span.end - index0, span.style);
		}
	}
	void get_word(std::size_t index, std::size_t& word_start, std::size_t& word_end) const {
		word_start = word_end = index;
	}
	void get_next_word(std::size_t index, std::size_t& word_start, std::size_t& word_end) const {
		word_start = word_end = index;
	}
	void get_previous_word(std::size_t index, std::size_t& word_start, std::size_t& word_end) const {
		word_start = word_end = index;
	}
	void render_selections(RenderedLine& line, std::size_t index0, std::size_t index1) const {
		for (const Selection& selection: selections) {
			if (selection.max() > index0 && selection.min() < index1) {
				line.selections.emplace_back(std::max(selection.min(), index0) - index0, std::min(selection.max(), index1) - index0);
			}
		}
		for (const Selection& selection: selections) {
			if (selection.head >= index0 && selection.head < index1) {
				line.cursors.emplace_back(selection.head - index0);
			}
		}
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
		if (index == 0) {
			return 0;
		}
		const std::size_t codepoints = buffer.get_info_for_index(index).codepoints - 1;
		return buffer.get_info_for_codepoints(codepoints).bytes;
	}
	std::size_t get_next_index(std::size_t index) const {
		if (index == buffer.get_size() - 1) {
			return index;
		}
		const std::size_t codepoints = buffer.get_info_for_index(index).codepoints + 1;
		return buffer.get_info_for_codepoints(codepoints).bytes;
	}
	std::size_t get_index_above(std::size_t index) const {
		const std::size_t line = buffer.get_info_for_index(index).newlines;
		if (line == 0) {
			return 0;
		}
		const std::size_t column = buffer.get_info_for_index(index).codepoints - buffer.get_info_for_line_start(line).codepoints;
		const std::size_t codepoints = buffer.get_info_for_line_start(line - 1).codepoints + column;
		const std::size_t max_codepoints = buffer.get_info_for_line_end(line - 1).codepoints;
		return buffer.get_info_for_codepoints(std::min(codepoints, max_codepoints)).bytes;
	}
	std::size_t get_index_below(std::size_t index) const {
		const std::size_t line = buffer.get_info_for_index(index).newlines;
		if (line == buffer.get_total_lines() - 1) {
			return buffer.get_size() - 1;
		}
		const std::size_t column = buffer.get_info_for_index(index).codepoints - buffer.get_info_for_line_start(line).codepoints;
		const std::size_t codepoints = buffer.get_info_for_line_start(line + 1).codepoints + column;
		const std::size_t max_codepoints = buffer.get_info_for_line_end(line + 1).codepoints;
		return buffer.get_info_for_codepoints(std::min(codepoints, max_codepoints)).bytes;
	}
	void delete_selections() {
		std::size_t offset = 0;
		for (Selection& selection: selections) {
			selection -= offset;
			if (!selection.is_empty()) {
				cache.invalidate(selection.min());
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
			if (selections[i-1].head == selections[i].head || selections[i-1].max() > selections[i].min()) {
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
	Editor(): language(nullptr), last_selection(0) {}
	Editor(const char* path): buffer(path), language(prism::get_language(get_file_name(path))), last_selection(0) {}
	std::size_t get_total_lines() const {
		return buffer.get_total_lines();
	}
	void render(RenderedLine& line, std::size_t i) const {
		std::size_t index0 = 0;
		std::size_t index1 = 0;
		if (i < get_total_lines()) {
			index0 = buffer.get_info_for_line_start(i).bytes;
			index1 = buffer.get_info_for_line_start(i + 1).bytes;
		}
		line.text = std::string(buffer.get_iterator(index0), buffer.get_iterator(index1));
		line.number = i + 1;
		highlight(line.spans, index0, index1);
		render_selections(line, index0, index1);
	}
	RenderedLine render(std::size_t i) const {
		RenderedLine line;
		render(line, i);
		return line;
	}
	std::vector<RenderedLine> render(std::size_t first_line, std::size_t last_line) const {
		std::vector<RenderedLine> lines;
		for (std::size_t i = first_line; i < last_line; ++i) {
			RenderedLine& line = lines.emplace_back();
			render(line, i);
		}
		return lines;
	}
	void insert_text(const char* text) {
		delete_selections();
		std::size_t offset = 0;
		for (Selection& selection: selections) {
			selection += offset;
			cache.invalidate(selection.head);
			for (const char* c = text; *c; ++c) {
				buffer.insert(selection.head, *c);
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
			cache.invalidate(selection.head);
			buffer.insert(selection.head, '\n');
			selection += 1;
			++offset;
			const std::size_t line = buffer.get_info_for_index(selection.head - 1).newlines;
			const std::size_t index = buffer.get_info_for_line_start(line).bytes;
			auto i = buffer.get_iterator(index);
			while (i != buffer.end() && (*i == ' ' || *i == '\t')) {
				buffer.insert(selection.head, *i);
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
			if (selection.is_empty() && selection.head > 0) {
				selection.head = get_previous_index(selection.head);
			}
			cache.invalidate(selection.min());
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
			if (selection.is_empty() && selection.head < last) {
				selection.head = get_next_index(selection.head);
			}
			cache.invalidate(selection.min());
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
		const std::size_t index = buffer.get_info_for_line_start(line).bytes + column;
		const std::size_t max_index = buffer.get_info_for_line_end(line).bytes;
		return std::min(index, max_index);
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
		selection.head = get_index(column, line);
		collapse_selections(selection.is_reversed());
	}
	void move_left(bool extend_selection = false) {
		for (Selection& selection: selections) {
			if (extend_selection) {
				selection.head = get_previous_index(selection.head);
			}
			else {
				if (selection.is_empty()) {
					selection = get_previous_index(selection.head);
				}
				else {
					selection = selection.min();
				}
			}
		}
		collapse_selections(true);
	}
	void move_right(bool extend_selection = false) {
		for (Selection& selection: selections) {
			if (extend_selection) {
				selection.head = get_next_index(selection.head);
			}
			else {
				if (selection.is_empty()) {
					selection = get_next_index(selection.head);
				}
				else {
					selection = selection.max();
				}
			}
		}
		collapse_selections(false);
	}
	void move_up(bool extend_selection = false) {
		for (Selection& selection: selections) {
			if (extend_selection) {
				selection.head = get_index_above(selection.head);
			}
			else {
				if (selection.is_empty()) {
					selection = get_index_above(selection.head);
				}
				else {
					selection = selection.min();
				}
			}
		}
		collapse_selections(true);
	}
	void move_down(bool extend_selection = false) {
		for (Selection& selection: selections) {
			if (extend_selection) {
				selection.head = get_index_below(selection.head);
			}
			else {
				if (selection.is_empty()) {
					selection = get_index_below(selection.head);
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
			get_previous_word(selection.head, word_start, word_end);
			if (extend_selection) {
				selection.head = word_start;
			}
			else {
				if (selection.is_empty()) {
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
			get_next_word(selection.head, word_start, word_end);
			if (extend_selection) {
				selection.head = word_end;
			}
			else {
				if (selection.is_empty()) {
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
			const std::size_t line = buffer.get_info_for_index(selection.head).newlines;
			selection.head = buffer.get_info_for_line_start(line).bytes;
			if (!extend_selection) {
				selection.tail = selection.head;
			}
		}
		collapse_selections(true);
	}
	void move_to_end_of_line(bool extend_selection = false) {
		for (Selection& selection: selections) {
			const std::size_t line = buffer.get_info_for_index(selection.head).newlines;
			selection.head = buffer.get_info_for_line_end(line).bytes;
			if (!extend_selection) {
				selection.tail = selection.head;
			}
		}
		collapse_selections(false);
	}
	void select_all() {
		selections.clear();
		selections.emplace_back(0, buffer.get_size() - 1);
		last_selection = 0;
	}
	const Theme& get_theme() const {
		return prism::get_theme("one-dark");
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
					cache.invalidate(selection.head);
					buffer.insert(selection.head, *c);
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
